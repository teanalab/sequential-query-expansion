/* -*- C++ -*- */

// Author: Alexander Kotov (UIUC), 2011
// $Id: ConceptQExpSim.cpp,v 1.6 2011/05/12 16:24:46 akotov2 Exp $

#include "common_headers.hpp"
#include "IndexManager.hpp"
#include "BasicDocStream.hpp"
#include "ResultFile.hpp"
#include "SimpleKLRetMethod.hpp"
#include "RetParamManager.hpp"
#include "ConceptNetClient.hpp"
#include "FloatFreqVector.hpp"
#include "PorterStemmer.hpp"
#include "StrUtils.hpp"
#include "TermGraph.hpp"

using namespace lemur::api;
using namespace lemur::retrieval;
using namespace lemur::utility;
using namespace lemur::parse;

#define SPACE_CHARS " \t\n"
#define WEIGHT_THRESH_DEF 0.001
#define NUM_TOP_SHOW_DEF 10
#define MAX_DOC_RANK_DEF 10
#define ORIG_QUERY_MULT_DEF 1

typedef struct TermEff_ {
  TermEff_() : map(0), idf(0), relWeight(0), expPhrase(""), expRel("")
  { }

  TermEff_(double map_, double idf_, double wgt_, const std::string& phrase,
            const std::string& rel) : map(map_), idf(idf_), relWeight(wgt_),
            expPhrase(phrase), expRel(rel)
  { }

  double map;
  double idf;
  unsigned int idfRank;
  double relWeight;
  unsigned int relRank;
  std::string expPhrase;
  std::string expRel;
} TermEff;

typedef std::map<TERMID_T, TermEff> TermMap;
typedef TermMap::iterator TermMapIt;
typedef TermMap::const_iterator cTermMapIt;

typedef struct PhraseInfo_ {
  PhraseInfo_() : map(0), expPhrase(""), expRel("")
  { }

  PhraseInfo_(double map_, const std::string &phrase, const std::string &rel) : map(map_), expPhrase(phrase), expRel(rel)
  { }

  double map;
  std::string expPhrase;
  std::string expRel;
} PhraseInfo;

typedef std::map<TERMID_T, PhraseInfo> PhraseMap;
typedef PhraseMap::iterator PhraseMapIt;
typedef PhraseMap::const_iterator cPhraseMapIt;

typedef struct RelEff_ {
  RelEff_() : numImp(0), numHurt(0), numNeut(0)
  { }

  RelEff_(uint imp, uint neut, uint hurt) : numImp(imp), numNeut(neut), numHurt(hurt)
  { }

  unsigned int numImp;
  unsigned int numNeut;
  unsigned int numHurt;
} RelEff;

typedef std::map<std::string, RelEff> RelEffMap;
typedef RelEffMap::iterator RelEffIt;
typedef RelEffMap::const_iterator cRelEffIt;

typedef std::map<std::string, unsigned int> RelCntMap;
typedef RelCntMap::iterator RelCntIt;
typedef RelCntMap::const_iterator cRelCntIt;

typedef std::map<TERMID_T, unsigned int> TermCntMap;
typedef TermCntMap::iterator TermCntMapIt;
typedef TermCntMap::const_iterator cTermCntMapIt;

typedef std::vector<TERMID_T> RankVec;

typedef std::map<std::string, std::vector<std::string> > QryTermMap;
typedef QryTermMap::iterator QryTermMapIt;
typedef QryTermMap::const_iterator cQryTermMapIt;

namespace LocalParameter {
  // file with unstemmed queries
  std::string unstemQryFile;
  // file with query judgments
  std::string judgmentsFile;
  // URL of ConceptNet server
  std::string serverUrl;
  // file with collection term graph
  std::string graphFile;
  // threshold for edge weight in order for the edge to be included in the collection term graph
  double weightThresh;
  // number of top expansion terms to show for each query term
  int numTopShow;
  // multiplier for the weights of the original query terms
  int origQueryMult;
  // maximum rank of the document to consider for counting occurrences of query terms
  int maxDocRank;
  
  void get() {
    unstemQryFile = ParamGetString("unstemQuery", "");
    judgmentsFile = ParamGetString("judgmentsFile", "");
    serverUrl = ParamGetString("serverUrl", "");
    graphFile = ParamGetString("graphFile", "");
    weightThresh = ParamGetDouble("weightThresh", WEIGHT_THRESH_DEF);
    numTopShow = ParamGetInt("numTopShow", NUM_TOP_SHOW_DEF);
    origQueryMult = ParamGetInt("origQueryMult", ORIG_QUERY_MULT_DEF);
    maxDocRank = ParamGetInt("maxDocRank", MAX_DOC_RANK_DEF);
  }
}

void GetAppParam() {
  RetrievalParameter::get();
  SimpleKLParameter::get();
  LocalParameter::get();
}

float queryMAP(Index *index, SimpleKLRetMethod *retMethod, const SimpleKLQueryModel *queryModel, IndexedRealVector *queryJudg) {
  COUNT_T docCnt, relDocs = 0, totRelDocs = queryJudg->size();
  COUNT_T maxInd;
  double totMAP = 0;

  IndexedRealVector queryResults(index->docCount());
  retMethod->scoreCollection(*queryModel, queryResults);
  maxInd = queryResults.size() < RetrievalParameter::resultCount ? queryResults.size() : RetrievalParameter::resultCount;
  queryResults.Sort();

  for(docCnt = 1; docCnt <= maxInd; docCnt++) {
    if(queryJudg->FindByIndex(queryResults[docCnt-1].ind) != queryJudg->end()) {
      ++relDocs;
      totMAP += ((float) relDocs) / docCnt;
    }
  }

  return relDocs != 0 ? totMAP / totRelDocs : 0.0;
}

void readQueries(const std::string &qryFile, QryTermMap &queryMap) {
  TextQuery *query = NULL;
  DocStream *qStream = NULL;
  TERMID_T queryId;

  try {
    qStream = new lemur::parse::BasicDocStream(qryFile);
  } catch (lemur::api::Exception &ex) {
    ex.writeMessage(std::cerr);
    throw lemur::api::Exception("ConceptNetQExp", "Can't open file with unstemmed queries, check parameter unstemQuery");
  }

  qStream->startDocIteration();
  while(qStream->hasMore()) {
    std::vector<std::string> queryTerms;
    Document *d = qStream->nextDoc();
    query = new TextQuery(*d);
    query->startTermIteration();
    while(query->hasMore()) {
      queryTerms.push_back(query->nextTerm()->spelling());
    }
    queryMap[query->id()] = queryTerms;
    delete query;
  }

  delete qStream;
}

void parseExpPhrase(Index *index, PorterStemmer *stemmer, const std::string& expPhrase, FloatFreqVector& freqVec) {
  TERMID_T expTermId;
  std::vector<std::string> expTerms;
  splitStr(expPhrase, expTerms, " ");
  for(unsigned int i = 0; i < expTerms.size(); i++) {
    expTermId = index->term(stemmer->stemWord((char*)expTerms[i].c_str()));
    if(expTermId == 0) {
      continue;
    } else {
      freqVec.addVal(expTermId, 1);
    }
  }
}

void incRelCnt(RelCntMap &cntMap, const std::string& rel) {
  RelCntIt it;

  if((it = cntMap.find(rel)) != cntMap.end()) {
    it->second++;
  } else {
    cntMap.insert(std::make_pair(rel, 1));
  }
}

void incRelEff(RelEffMap &effMap, const std::string& rel, int res) {
  RelEffIt it;

  if((it = effMap.find(rel)) != effMap.end()) {
    switch(res) {
      case 1:
        it->second.numImp++;
        break;
      case 0:
        it->second.numNeut++;
        break;
      case -1:
        it->second.numHurt++;
        break;
    }
  } else {
    switch(res) {
      case 1:
        effMap.insert(std::make_pair(rel, RelEff(1, 0, 0)));
        break;
      case 0:
        effMap.insert(std::make_pair(rel, RelEff(0, 1, 0)));
        break;
      case -1:
        effMap.insert(std::make_pair(rel, RelEff(0, 0, 1)));
        break;
    }
  }
}

void countQryTermRes(Index *index, SimpleKLRetMethod *retMethod, const SimpleKLQueryModel *queryModel, TermCntMap &cntMap) {
  COUNT_T docRank, maxRank, termCnt;
  TERMID_T termId;
  TermCntMapIt dit;

  IndexedRealVector queryResults(index->docCount());
  retMethod->scoreCollection(*queryModel, queryResults);
  maxRank = queryResults.size() < LocalParameter::maxDocRank ? queryResults.size() : LocalParameter::maxDocRank;
  queryResults.Sort();

  for(TermCntMapIt it = cntMap.begin(); it != cntMap.end(); it++) {
    TermCntMap termDocs;
    termId = it->first;
    DocInfoList *docList = index->docInfoList(termId);
    docList->startIteration();
    while(docList->hasMore()) {
      DocInfo *di = docList->nextEntry();
      termDocs[di->docID()] = di->termCount();
    }
    delete docList;

    termCnt = 0;
    for(docRank = 0; docRank < maxRank; docRank++) {
      if((dit = termDocs.find(queryResults[docRank].ind)) != termDocs.end()) {
        termCnt += dit->second;
      }
    }

    cntMap[termId] = termCnt;
  }
}

void rankExpUnits(TermMap &termMap, PhraseMap &phraseMap, RankVec &termRanks, RankVec &phraseRanks) {
  unsigned int i;
  TermMapIt tit;
  PhraseMapIt pit;
  IndexedRealVector sortVec;

  for(tit = termMap.begin(); tit != termMap.end(); tit++) {
    sortVec.PushValue(tit->first, tit->second.idf);
  }
  sortVec.Sort();
  for(i = 1; i <= sortVec.size(); i++) {
    termMap[sortVec[i-1].ind].idfRank = i;
  }

  sortVec.clear();
  for(tit = termMap.begin(); tit != termMap.end(); tit++) {
    sortVec.PushValue(tit->first, tit->second.relWeight);
  }
  sortVec.Sort();
  for(i = 1; i <= sortVec.size(); i++) {
    termMap[sortVec[i-1].ind].relRank = i;
  }

  sortVec.clear();
  for(tit = termMap.begin(); tit != termMap.end(); tit++) {
    sortVec.PushValue(tit->first, tit->second.map);
  }
  sortVec.Sort();
  for(i = 1; i <= sortVec.size(); i++) {
    termRanks.push_back(sortVec[i-1].ind);
  }

  sortVec.clear();
  for(pit = phraseMap.begin(); pit != phraseMap.end(); pit++) {
    sortVec.PushValue(pit->first, pit->second.map);
  }
  sortVec.Sort();
  for(i = 1; i <= sortVec.size(); i++) {
    phraseRanks.push_back(sortVec[i-1].ind);
  }
}

void processQueryTerm(Index *index, TermGraph *graph, ConceptNetClient *client, PorterStemmer *stemmer, SimpleKLRetMethod *retMethod,
                      const SimpleKLQueryModel *origQueryModel, std::string& term, TERMID_T qryTermId, IndexedRealVector* queryJudg,
                      double origMAP, RelEffMap &relEff, TermMap &termMap, PhraseMap &phraseMap, int origQueryMult, bool verbose=false) {
  std::string expPhrase, expTerm, expRel;
  TERMID_T expTermId, expPhraseId;
  double expTermCnt, expMAP, termIDF;
  float relWeight;
  COUNT_T docCount = index->docCount();
  expPhraseId = 1;

  ContextIterator* contIt = client->getTermContext(term);
  if(contIt != NULL) {
    while(contIt->hasNext()) {
      FloatFreqVector expTermFreqs;
      SimpleKLQueryModel *expQueryModel = new SimpleKLQueryModel(*index);
      contIt->moveNext();
      expPhrase = contIt->getPhrase();
      stripStr(expPhrase, SPACE_CHARS);
      expRel = contIt->getRelation();

      if(verbose)
        std::cout << "         Expanding query with concept '" << expPhrase << "' (" << expRel << ") MAP=";

      parseExpPhrase(index, stemmer, expPhrase, expTermFreqs);
      origQueryModel->startIteration();
      while(origQueryModel->hasMore()) {
        QueryTerm *qt = origQueryModel->nextTerm();
        expQueryModel->setCount(qt->id(), qt->weight()*origQueryMult);
        delete qt;
      }
      expTermFreqs.startIteration();
      while(expTermFreqs.hasMore()) {
        expTermFreqs.nextFreq(expTermId, expTermCnt);
        expQueryModel->incCount(expTermId, expTermCnt);
      }

      expMAP = queryMAP(index, retMethod, expQueryModel, queryJudg);

      phraseMap[expPhraseId] = PhraseInfo(expMAP, expPhrase, expRel);

      if(verbose) {
        std::cout << expMAP << " ";
      }

      if(expMAP > origMAP) {
        incRelEff(relEff, expRel, 1);
        if(verbose)
          std::cout << "+" << std::endl;
      } else if(expMAP == origMAP) {
        incRelEff(relEff, expRel, 0);
        if(verbose)
          std::cout << "0" << std::endl;
      } else {
        incRelEff(relEff, expRel, -1);
        if(verbose)
          std::cout << "-" << std::endl;
      }

      delete expQueryModel;

      // expanding query with each term in related concept
      expTermFreqs.startIteration();
      while(expTermFreqs.hasMore()) {
        SimpleKLQueryModel *termExpQueryModel = new SimpleKLQueryModel(*index);
        expTermFreqs.nextFreq(expTermId, expTermCnt);
        if(verbose)
          std::cout << "         Expanding query with term '" << index->term(expTermId) << "' (" << expRel << ") MAP=";
        origQueryModel->startIteration();
        while(origQueryModel->hasMore()) {
          QueryTerm *qt = origQueryModel->nextTerm();
          termExpQueryModel->setCount(qt->id(), qt->weight()*origQueryMult);
          delete qt;
        }
        termExpQueryModel->incCount(expTermId, expTermCnt);
        expMAP = queryMAP(index, retMethod, termExpQueryModel, queryJudg);
        if(verbose) {
          std::cout << expMAP << " ";
          if(expMAP > origMAP) {
            std::cout << "+" << std::endl;
          } else if(expMAP == origMAP) {
            std::cout << "0" << std::endl;
          } else {
            std::cout << "-" << std::endl;
          }
        }
        graph->relExists(qryTermId, expTermId, &relWeight);
        termIDF = log(docCount / index->docCount(expTermId));
        termMap[expTermId] = TermEff(expMAP, termIDF, relWeight, expPhrase, expRel);
        delete termExpQueryModel;
      }

      expPhraseId++;
    }

    delete contIt;
  }
}

int AppMain(int argc, char *argv[]) {
  Index *index = NULL;
  DocStream *qStream = NULL;
  TextQuery *query = NULL;
  ifstream *judgmentsStr = NULL;
  ResultFile *judgments = NULL;
  SimpleKLRetMethod *retMethod = NULL;
  SimpleKLQueryModel *origQueryModel = NULL;
  ConceptNetClient *client = NULL;
  PorterStemmer *stemmer = NULL;
  TermGraph *termGraph = NULL;
  QryTermMap qryMap;
  RelEffMap relEffMap;
  RelCntMap phraseRelCnts;
  RelCntMap termRelCnts;
  COUNT_T minTermCnt;
  double origMAP, maxMAP, maxQryMAP;
  std::string origTerm, maxTerm, maxExpTerm, maxQryTerm, maxPhrase, maxQryPhrase, maxRel, maxQryRel;
  uint termInd, qryTot = 0, qryImp = 0, qryHurt = 0;
  bool improved, hurt;

  if(!RetrievalParameter::databaseIndex.length()) {
    throw lemur::api::Exception("ConceptQExpSim", "Path to the index key file is not specified (parameter index)");
  }

  if(!RetrievalParameter::textQuerySet.length()) {
    throw lemur::api::Exception("ConceptQExpSim", "File with query topics is not specified in the configuration file (parameter textQuery)");
  }

  if(!LocalParameter::unstemQryFile.length()) {
    throw lemur::api::Exception("ConceptQExpSim", "File with unstemmed queries is not specified in the configuration file (parameter unstemQuery)");
  }

  if(!LocalParameter::judgmentsFile.length()) {
    throw lemur::api::Exception("ConceptQExpSim", "File with topic judgments is not specified in the configuration file (parameter judgmentsFile)");
  }

  if(!LocalParameter::serverUrl.length()) {
    throw lemur::api::Exception("ConceptQExpSim", "URL of ConceptNet server is not specified (parameter serverUrl)");
  }

  if(!LocalParameter::graphFile.length()) {
    throw lemur::api::Exception("ConceptQExpSim", "File with collection term graph is not specified");
  }

  client = new ConceptNetClient(LocalParameter::serverUrl);
  stemmer = new PorterStemmer();
  readQueries(LocalParameter::unstemQryFile, qryMap);

  try {
    index = IndexManager::openIndex(RetrievalParameter::databaseIndex);
  } catch (lemur::api::Exception &ex) {
    ex.writeMessage(std::cerr);
    throw lemur::api::Exception("ConceptNetQExpSim", "Can't open index. Check parameter index in the configuration file.");
  }

  judgmentsStr = new ifstream(LocalParameter::judgmentsFile.c_str(), ios::in | ios::binary);

  if(judgmentsStr->fail()) {
    throw lemur::api::Exception("ConceptNetQExp", "can't open the file with query judgments");
  }

  judgments = new ResultFile(false);
  judgments->load(*judgmentsStr, *index);

  termGraph = new TermGraph(index);

  try {
    cout << "Loading term graph...";
    cout.flush();
    termGraph->loadFile(LocalParameter::graphFile, LocalParameter::weightThresh);
  } catch (lemur::api::Exception &ex) {
    ex.writeMessage(cerr);
    throw lemur::api::Exception("ConceptNetQExpSim", "Problem loading term graph file. Check parameter graphFile in the configuration file.");
  }

  cout << "OK" << endl;
  cout.flush();

  lemur::retrieval::ArrayAccumulator accumulator(index->docCount());
  retMethod = new SimpleKLRetMethod(*index, SimpleKLParameter::smoothSupportFile, accumulator);
  retMethod->setDocSmoothParam(SimpleKLParameter::docPrm);
  retMethod->setQueryModelParam(SimpleKLParameter::qryPrm);

  try {
    qStream = new lemur::parse::BasicDocStream(RetrievalParameter::textQuerySet);
  } catch (lemur::api::Exception &ex) {
    ex.writeMessage(std::cerr);
    throw lemur::api::Exception("ConceptQExpBF", "Can't open query file, check parameter textQuery");
  }

  qStream->startDocIteration();
  while(qStream->hasMore()) {
    qryTot++;
    IndexedRealVector *queryJudgments = NULL;
    TermCntMap termCntMap;
    improved = false; hurt = false;
    Document *d = qStream->nextDoc();
    query = new TextQuery(*d);

    if(!judgments->findResult(query->id(), queryJudgments)) {
      std::cerr << "Can't find result judgments for query " << query->id() << std::endl;
      delete query;
      continue;
    }

    std::vector<std::string> qryTerms = qryMap[query->id()];
    std::cout << "Processing query #" << query->id() << " (";
    for(int i = 0; i < qryTerms.size(); i++) {
      std::cout << " " << qryTerms[i];
    }
    std::cout << " ) ";

    origQueryModel = dynamic_cast<SimpleKLQueryModel*>(retMethod->computeTextQueryRep(*query));
    origMAP = queryMAP(index, retMethod, origQueryModel, queryJudgments);
    std::cout << "MAP=" << origMAP << std::endl;

    query->startTermIteration();
    while(query->hasMore()) {
      const Term *t = query->nextTerm();
      termCntMap[index->term(t->spelling())] = 0;
    }

    countQryTermRes(index, retMethod, origQueryModel, termCntMap);

    termInd = 0;
    maxQryMAP = -1;
    maxQryTerm = "";
    maxExpTerm = "";
    maxQryPhrase = "";
    maxQryRel = "";
    query->startTermIteration();
    while(query->hasMore()) {
      TermMap termMap;
      PhraseMap phraseMap;
      RankVec termRanks, phraseRanks;
      const Term *t = query->nextTerm();
      origTerm = qryTerms[termInd];
      std::cout << "   Expanding query term " << origTerm << " (" << t->spelling() << ")" << std::endl;
      processQueryTerm(index, termGraph, client, stemmer, retMethod, origQueryModel, origTerm, index->term(t->spelling()), queryJudgments, origMAP, relEffMap, termMap, phraseMap, LocalParameter::origQueryMult, true);
      rankExpUnits(termMap, phraseMap, termRanks, phraseRanks);
      maxMAP = -1;
      maxTerm = "";
      maxPhrase = "";
      maxRel = "";
      if(phraseRanks.size()) {
        incRelCnt(phraseRelCnts, phraseMap[phraseRanks[0]].expRel);
        maxMAP = phraseMap[phraseRanks[0]].map;
        maxPhrase = phraseMap[phraseRanks[0]].expPhrase;
        maxRel = phraseMap[phraseRanks[0]].expRel;
      }
      if(termRanks.size()) {
        incRelCnt(termRelCnts, termMap[termRanks[0]].expRel);
        if(termMap[termRanks[0]].map > maxMAP) {
          maxMAP = termMap[termRanks[0]].map;
          maxTerm = index->term(termRanks[0]);
          maxPhrase = termMap[termRanks[0]].expPhrase;
          maxRel = termMap[termRanks[0]].expRel;
        }
      }
      if(maxMAP > maxQryMAP) {
        maxQryMAP = maxMAP;
        maxQryTerm = t->spelling();
        maxExpTerm = maxTerm;
        maxQryPhrase = maxPhrase;
        maxQryRel = maxRel;
      }
      std::cout << "   Top expansion phrases:" << std::endl;
      for(int i = 0; i < phraseRanks.size() && i < LocalParameter::numTopShow; i++) {
        std::cout << "      " << i+1 << ". '" << phraseMap[phraseRanks[i]].expPhrase << "' ("
                  << phraseMap[phraseRanks[i]].expRel << ") MAP=" << phraseMap[phraseRanks[i]].map << std::endl;
      }
      std::cout << "   Top expansion terms:" << std::endl;
      for(int i = 0; i < termRanks.size() && i < LocalParameter::numTopShow; i++) {
        std::cout << "      " << i+1 << ". '" << index->term(termRanks[i]) << "' ("
                  << termMap[termRanks[i]].expPhrase << ") (" << termMap[termRanks[i]].expRel << ") MAP="
                  << termMap[termRanks[i]].map << " IDF=" << termMap[termRanks[i]].idf << "("
                  << termMap[termRanks[i]].idfRank << ") Rel=" << termMap[termRanks[i]].relWeight << "("
                  << termMap[termRanks[i]].relRank << ")" << std::endl;
      }

      if(maxMAP != -1 && maxMAP-origMAP > 0) {
        if(!improved) {
          improved = true;
          qryImp++;
        }
        if(maxTerm.length()) {
          std::cout << "   Best improved with expansion term '" << maxTerm << "' (" << maxPhrase << ") (" << maxRel << ") MAP=" << maxMAP << std::endl;
        } else {
          std::cout << "   Best improved with expansion phrase '" << maxPhrase << "' (" << maxRel << ") MAP=" << maxMAP << std::endl;
        }
      } else if(maxMAP == -1 || maxMAP-origMAP == 0) {
        if(maxMAP == -1) {
          std::cout << "   No expansion concepts found" << std::endl;
        } else {
          if(maxTerm.length()) {
            std::cout << "   No changes with best expansion term '" << maxTerm << "' (" << maxPhrase << ") (" << maxRel << ") MAP=" << maxMAP << std::endl;
          } else {
            std::cout << "   No changes with best expansion phrase '" << maxPhrase << "' (" << maxRel << ") MAP=" << maxMAP << std::endl;
          }
        }
      } else {
        hurt = true;
        if(maxTerm.length()) {
          std::cout << "   Hurt with best expansion term '" << maxTerm << "' (" << maxPhrase << ") (" << maxRel << ") MAP=" << maxMAP << std::endl;
        } else {
          std::cout << "   Hurt with best expansion phrase '" << maxPhrase << "' (" << maxRel << ") MAP=" << maxMAP << std::endl;
        }
      }

      termInd++;
    }

    std::cout << "Query term counts in top " << LocalParameter::maxDocRank << " documents:" << std::endl;
    for(TermCntMapIt it = termCntMap.begin(); it != termCntMap.end(); it++) {
      std::cout << index->term(it->first) << ": " << it->second << std::endl;
    }

    std::cout << "Best query term to expand: '" << maxQryTerm << "'" << std::endl;
    if(maxExpTerm.length()) {
      std::cout << "Best expansion term: '" << maxExpTerm << "' ('" << maxQryPhrase << "') (" << maxQryRel << ") MAP=" << maxQryMAP << std::endl;
    } else {
      std::cout << "Best expansion concept: '" << maxQryPhrase << "' (" << maxQryRel << ") MAP=" << maxQryMAP << std::endl;
    }

    std::cout << std::endl << std::endl;

    if(!improved && hurt) {
      qryHurt++;
    }

    delete origQueryModel;
    delete query;
  }

  std::cout << "Total queries: " << qryTot << std::endl;
  std::cout << "Queries improved: " << qryImp << std::endl;
  std::cout << "Queries hurt: " << qryHurt << std::endl;
  std::cout << "Queries neutral: " << qryTot-(qryImp+qryHurt) << std::endl << std::endl;

  std::cout << "Number of times related concept corresponded to best expansion:" << std::endl;
  for(RelCntIt it = phraseRelCnts.begin(); it != phraseRelCnts.end(); it++) {
    std::cout << it->first << " : " << it->second << std::endl;
  }
  std::cout << std::endl;
  std::cout << "Number of times related term corresponded to best expansion:" << std::endl;
  for(RelCntIt it = termRelCnts.begin(); it != termRelCnts.end(); it++) {
    std::cout << it->first << " : " << it->second << std::endl;
  }
  std::cout << std::endl;
  std::cout << "Effectiveness of relations:" << std::endl;
  for(RelEffIt it = relEffMap.begin(); it != relEffMap.end(); it++) {
    std::cout << it->first << ": + (" << it->second.numImp << ") - (" << it->second.numHurt << ") 0 (" << it->second.numNeut << ")" << std::endl;
  }

  delete termGraph;
  delete stemmer;
  delete client;
  delete retMethod;
  delete judgments;
  delete judgmentsStr;
  delete qStream;
  delete index;
}
