/* -*- C++ -*- */

// Author: Alexander Kotov (UIUC), 2011
// $Id: ConceptRankExp.cpp,v 1.9 2011/07/07 20:47:28 akotov2 Exp $

#include "common_headers.hpp"
#include "IndexManager.hpp"
#include "BasicDocStream.hpp"
#include "ResultFile.hpp"
#include "RetParamManager.hpp"
#include "ExpLM.hpp"
#include "ConUtils.hpp"
#include "StrUtils.hpp"
#include "Features.hpp"

using namespace lemur::api;
using namespace lemur::retrieval;
using namespace lemur::utility;
using namespace lemur::parse;

#define ORIG_LM_COEF_DEF 0.7
#define MAX_EXP_TERMS_DEF 100
#define MAX_OUT_TERMS_DEF 20

namespace LocalParameter {
  // used feature set (full or baseline)
  std::string featSet;
  // file with extracted features
  std::string featFile;
  // output file with expansion terms
  std::string termFile;
  // maximum number of top scoring terms to use for expansion
  int maxExpTerms;
  // maximum number of top scoring terms to output
  int maxOutTerms;
  // coefficient for interpolation of the expansion language model
  double origModCoef;
  float interc;
  float numQryTermsW;
  float topDocScoreW;
  float expTDocScoreW;
  float topTermFracW;
  float numCanDocsW;
  float avgCDocScoreW;
  float maxCDocScoreW;
  float idfW;
  float fanOutW;
  float spActScoreW;
  float spActRankW;
  float rndWalkScoreW;
  float pathFindScoreW;
  float avgColCorW;
  float maxColCorW;
  float avgTopCorW;
  float maxTopCorW;
  float avgTopPCorW;
  float maxTopPCorW;
  float avgQDistW;
  float maxQDistW;
  float avgPWeightW;
  float maxPWeightW;

  void get() {
    featSet = ParamGetString("featSet", "");
    featFile = ParamGetString("featFile", "");
    termFile = ParamGetString("termFile", "");
    maxExpTerms = ParamGetInt("maxExpTerms", MAX_EXP_TERMS_DEF);
    maxOutTerms = ParamGetInt("maxOutTerms", MAX_OUT_TERMS_DEF);
    origModCoef = ParamGetDouble("origModCoef", ORIG_LM_COEF_DEF);
    interc = ParamGetDouble("interc", 0);
    numQryTermsW = ParamGetDouble("numQryTerms", 0);
    topDocScoreW = ParamGetDouble("topDocScore", 0);
    expTDocScoreW = ParamGetDouble("expTDocScore", 0);
    topTermFracW = ParamGetDouble("topTermFrac", 0);
    numCanDocsW = ParamGetDouble("numCanDocs", 0);
    avgCDocScoreW = ParamGetDouble("avgCDocScore", 0);
    maxCDocScoreW = ParamGetDouble("maxCDocScore", 0);
    idfW = ParamGetDouble("idf", 0);
    fanOutW = ParamGetDouble("fanOut", 0);
    spActScoreW = ParamGetDouble("spActScore", 0);
    spActRankW = ParamGetDouble("spActRank", 0);
    rndWalkScoreW = ParamGetDouble("rndWalkScore", 0);
    pathFindScoreW = ParamGetDouble("pathFindScore", 0);
    avgColCorW = ParamGetDouble("avgColCor", 0);
    maxColCorW = ParamGetDouble("maxColCor", 0);
    avgTopCorW = ParamGetDouble("avgTopCor", 0);
    maxTopCorW = ParamGetDouble("maxTopCor", 0);
    avgTopPCorW = ParamGetDouble("avgTopPCor", 0);
    maxTopPCorW = ParamGetDouble("maxTopPCor", 0);
    avgQDistW = ParamGetDouble("avgQDist", 0);
    maxQDistW = ParamGetDouble("maxQDist", 0);
    avgPWeightW = ParamGetDouble("avgPWeight", 0);
    maxPWeightW = ParamGetDouble("maxPWeight", 0);
  }
}

void GetAppParam() {
  RetrievalParameter::get();
  SimpleKLParameter::get();
  LocalParameter::get();
}

void expLangMod(SimpleKLQueryModel *qryModel, IndexedRealVector &expTerms) {
  ExpLM expLM;
  COUNT_T maxExpTerms;

  if(LocalParameter::maxExpTerms != 0) {
    maxExpTerms = expTerms.size() < LocalParameter::maxExpTerms ? expTerms.size() : LocalParameter::maxExpTerms;
  } else {
    maxExpTerms = expTerms.size();
  }

  if(expTerms.size()) {
    for(int i = 0; i < maxExpTerms; i++) {
      expLM.addTerm(expTerms[i].ind, expTerms[i].val);
    }
    expLM.normalize();
    qryModel->interpolateWith(expLM, LocalParameter::origModCoef, expLM.size());
  }
}

void storeExpCands(ofstream &outFile, Index *index, const std::string& queryId, const IndexedRealVector &expTerms) {
  outFile << queryId << std::endl;
  for(unsigned int i = 0; i < expTerms.size(); i++) {
    outFile << index->term(expTerms[i].ind) << " " << expTerms[i].val << std::endl;
  }
}

bool loadFeatMap(FeatMap &featMap, const std::string &featFile) {
  std::string line;
  unsigned int qryId, curQryId = 0;
  StrVec toks;
  FeatVec curQryFeats;

  std::ifstream fs(featFile.c_str(), std::ifstream::in);
  if(!fs.is_open()) {
    return false;
  }

  while(getline(fs, line)) {
    Features feats;
    toks.clear();
    stripStr(line, SPACE_CHARS);
    if(!line.length()) {
      continue;
    }
    splitStr(line, toks, "\t");
    qryId = atoi(toks[0].c_str());
    if(curQryId != 0 && qryId != curQryId) {
      featMap[curQryId] = curQryFeats;
      curQryFeats.clear();
    }
    feats.expCon = toks[1];
    feats.numQryTerms = atof(toks[2].c_str());
    feats.topDocScore = atof(toks[3].c_str());
    feats.expTDocScore = atof(toks[4].c_str());
    feats.topTermFrac = atof(toks[5].c_str());
    feats.numCanDocs = atof(toks[6].c_str());
    feats.avgCDocScore = atof(toks[7].c_str());
    feats.maxCDocScore = atof(toks[8].c_str());
    feats.idf = atof(toks[9].c_str());
    feats.fanOut = atof(toks[10].c_str());
    feats.spActScore = atof(toks[11].c_str());
    feats.spActRank = atof(toks[12].c_str());
    feats.rndWalkScore = atof(toks[13].c_str());
    feats.pathFindScore = atof(toks[14].c_str());
    feats.avgColCor = atof(toks[15].c_str());
    feats.maxColCor = atof(toks[16].c_str());
    feats.avgTopCor = atof(toks[17].c_str());
    feats.maxTopCor = atof(toks[18].c_str());
    feats.avgTopPCor = atof(toks[19].c_str());
    feats.maxTopPCor = atof(toks[20].c_str());
    feats.avgQDist = atof(toks[21].c_str());
    feats.maxQDist = atof(toks[22].c_str());
    feats.avgPWeight = atof(toks[23].c_str());
    feats.maxPWeight = atof(toks[24].c_str());
    curQryFeats.push_back(feats);
    curQryId = qryId;
  }

  if(curQryId != 0) {
    featMap[curQryId] = curQryFeats;
  }
}

float getExpTermWeightBL(Features &feats, float interc, float numQryTermsW, float topDocScoreW, float expTDocScoreW,
                       float topTermFracW, float numCanDocsW, float avgCDocScoreW, float maxCDocScoreW, float idfW,
                       float spActScoreW, float spActRankW, float avgColCorW, float maxColCorW, float avgTopCorW,
                       float maxTopCorW, float avgTopPCorW, float maxTopPCorW) {
  float dotProd = interc+numQryTermsW*feats.numQryTerms+topDocScoreW*feats.topDocScore+expTDocScoreW*feats.expTDocScore+ \
                  topTermFracW*feats.topTermFrac+numCanDocsW*feats.numCanDocs+avgCDocScoreW*feats.avgCDocScore+ \
                  maxCDocScoreW*feats.maxCDocScore+idfW*feats.idf+spActScoreW*feats.spActScore+spActRankW*feats.spActRank+ \
                  avgColCorW*feats.avgColCor+maxColCorW*feats.maxColCor+avgTopCorW*feats.avgTopCor+maxTopCorW*feats.maxTopCor+ \
                  avgTopPCorW*feats.avgTopPCor+maxTopPCorW*feats.maxTopPCor;
  return dotProd;
}

float getExpTermWeightFull(Features &feats, float interc, float numQryTermsW, float topDocScoreW, float expTDocScoreW,
                       float topTermFracW, float numCanDocsW, float avgCDocScoreW, float maxCDocScoreW, float idfW,
                       float fanOutW, float spActScoreW, float spActRankW, float rndWalkScoreW, float pathFindScoreW,
                       float avgColCorW, float maxColCorW, float avgTopCorW, float maxTopCorW, float avgTopPCorW,
                       float maxTopPCorW, float avgQDistW, float maxQDistW, float avgPWeightW, float maxPWeightW) {
  float dotProd = interc+numQryTermsW*feats.numQryTerms+topDocScoreW*feats.topDocScore+expTDocScoreW*feats.expTDocScore+ \
                  topTermFracW*feats.topTermFrac+numCanDocsW*feats.numCanDocs+avgCDocScoreW*feats.avgCDocScore+ \
                  maxCDocScoreW*feats.maxCDocScore+idfW*feats.idf+fanOutW*feats.fanOut+spActScoreW*feats.spActScore+ \
                  spActRankW*feats.spActRank+rndWalkScoreW*feats.rndWalkScore+pathFindScoreW*feats.pathFindScore+ \
                  avgColCorW*feats.avgColCor+maxColCorW*feats.maxColCor+avgTopCorW*feats.avgTopCor+maxTopCorW*feats.maxTopCor+ \
                  avgTopPCorW*feats.avgTopPCor+maxTopPCorW*feats.maxTopPCor+avgQDistW*feats.avgQDist+maxQDistW*feats.maxQDist+ \
                  avgPWeightW*feats.avgPWeight+maxPWeightW*feats.maxPWeight;
  //return 1/(1+exp(-dotProd));
  return dotProd;
}

void getExpTermsBL(Index *index, FeatVec &featVec, IndexedRealVector &expTerms) {
  TERMID_T expTermId;
  float weight;
  for(int i = 0; i < featVec.size(); i++) {
    expTermId = index->term(featVec[i].expCon);
    weight = getExpTermWeightBL(featVec[i], LocalParameter::interc, LocalParameter::numQryTermsW, LocalParameter::topDocScoreW, LocalParameter::expTDocScoreW,
                              LocalParameter::topTermFracW, LocalParameter::numCanDocsW, LocalParameter::avgCDocScoreW, LocalParameter::maxCDocScoreW,
                              LocalParameter::idfW, LocalParameter::spActScoreW, LocalParameter::spActRankW, LocalParameter::avgColCorW, LocalParameter::maxColCorW,
                              LocalParameter::avgTopCorW, LocalParameter::maxTopCorW, LocalParameter::avgTopPCorW, LocalParameter::maxTopPCorW);
    expTerms.PushValue(expTermId, weight);
  }
}

// returns sorted list of expansion terms
void getExpTermsFull(Index *index, FeatVec &featVec, IndexedRealVector &expTerms) {
  TERMID_T expTermId;
  float weight;
  for(int i = 0; i < featVec.size(); i++) {
    expTermId = index->term(featVec[i].expCon);
    weight = getExpTermWeightFull(featVec[i], LocalParameter::interc, LocalParameter::numQryTermsW, LocalParameter::topDocScoreW, LocalParameter::expTDocScoreW,
                              LocalParameter::topTermFracW, LocalParameter::numCanDocsW, LocalParameter::avgCDocScoreW, LocalParameter::maxCDocScoreW,
                              LocalParameter::idfW, LocalParameter::fanOutW, LocalParameter::spActScoreW, LocalParameter::spActRankW, LocalParameter::rndWalkScoreW,
                              LocalParameter::pathFindScoreW, LocalParameter::avgColCorW, LocalParameter::maxColCorW, LocalParameter::avgTopCorW,
                              LocalParameter::maxTopCorW, LocalParameter::avgTopPCorW, LocalParameter::maxTopPCorW, LocalParameter::avgQDistW,
                              LocalParameter::maxQDistW, LocalParameter::avgPWeightW, LocalParameter::maxPWeightW);
    expTerms.PushValue(expTermId, weight);
  }
}

int AppMain(int argc, char *argv[]) {
  unsigned int qryId;
  Index *index = NULL;
  TextQuery *query = NULL;
  DocStream *qStream = NULL;
  SimpleKLRetMethod *retMethod = NULL;
  SimpleKLQueryModel *qryModel = NULL;
  FeatMap featMap;
  FeatMapIt fit;
  ofstream termFile;

  if(!RetrievalParameter::databaseIndex.length()) {
    throw lemur::api::Exception("ConceptRankExp", "Path to the index key file is not specified (parameter index)");
  }

  if(!RetrievalParameter::textQuerySet.length()) {
    throw lemur::api::Exception("ConceptRankExp", "File with query topics is not specified in the configuration file (parameter textQuery)");
  }

  if(!LocalParameter::featFile.length()) {
    throw lemur::api::Exception("ConceptRankExp", "File with features is not specified (parameter featFile)");
  }

  if(!LocalParameter::termFile.length()) {
    throw lemur::api::Exception("ConceptRankExp", "File to store expansion terms is not specified (parameter termFile)");
  }

  if(!LocalParameter::featSet.length()) {
    throw lemur::api::Exception("ConceptRankExp", "Feature set is not specified (parameter featSet)");
  }

  if(LocalParameter::featSet.compare("base") != 0 && LocalParameter::featSet.compare("full") != 0) {
    throw lemur::api::Exception("ConceptRankExp", "Unknown feature set");
  }

  if(!loadFeatMap(featMap, LocalParameter::featFile)) {
    std::cerr << "Error: can't load feature file" << std::endl;
    return -1;
  }

  try {
    index = IndexManager::openIndex(RetrievalParameter::databaseIndex);
  } catch (lemur::api::Exception &ex) {
    ex.writeMessage(std::cerr);
    throw lemur::api::Exception("FeatExt", "Can't open index. Check paramter index in the configuration file.");
  }

  lemur::retrieval::ArrayAccumulator accumulator(index->docCount());
  retMethod = new SimpleKLRetMethod(*index, SimpleKLParameter::smoothSupportFile, accumulator);
  retMethod->setDocSmoothParam(SimpleKLParameter::docPrm);
  retMethod->setQueryModelParam(SimpleKLParameter::qryPrm);

  ofstream retRes(RetrievalParameter::resultFile.c_str());
  ResultFile resFile(RetrievalParameter::TRECresultFileFormat);
  resFile.openForWrite(retRes, *index);

  termFile.open(LocalParameter::termFile.c_str());
  if(!termFile.is_open()) {
    std::cerr << "Error: can't open file to save expansion terms" << std::endl;
    return -1;
  }

  try {
    qStream = new lemur::parse::BasicDocStream(RetrievalParameter::textQuerySet);
  } catch (lemur::api::Exception &ex) {
    ex.writeMessage(std::cerr);
    throw lemur::api::Exception("FeatExt", "Can't open query file, check parameter textQuery");
  }

  qStream->startDocIteration();
  while(qStream->hasMore()) {
    query = new TextQuery(*qStream->nextDoc());
    qryId = atoi(query->id().c_str());
    qryModel = dynamic_cast<SimpleKLQueryModel*>(retMethod->computeTextQueryRep(*query));
    query->startTermIteration();

    while(query->hasMore()) {
      const Term *t = query->nextTerm();
    }

    if((fit = featMap.find(qryId)) != featMap.end()) {
      IndexedRealVector expTerms;
      IndexedRealVector qryResults;
      cout << "Processing query #" << query->id() << endl;
      if(!LocalParameter::featSet.compare("base")) {
        getExpTermsBL(index, fit->second, expTerms);
      } else {
        getExpTermsFull(index, fit->second, expTerms);
      }
      expTerms.Sort();
      expLangMod(qryModel, expTerms);
      storeExpCands(termFile, index, query->id(), expTerms);
      retMethod->scoreCollection(*qryModel, qryResults);
      qryResults.Sort();
      resFile.writeResults(query->id(), &qryResults, RetrievalParameter::resultCount);
      delete qryModel;
    }
    delete query;
  }

  retRes.close();
  termFile.close();

  delete qStream;
  delete index;
}

