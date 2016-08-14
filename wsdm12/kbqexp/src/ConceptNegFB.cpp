/* -*- C++ -*- */

// Author: Alexander Kotov (UIUC), 2011
// $Id: ConceptNegFB.cpp,v 1.15 2011/05/15 00:22:46 akotov2 Exp $

#include "common_headers.hpp"
#include "IndexManager.hpp"
#include "BasicDocStream.hpp"
#include "ResultFile.hpp"
#include "RetParamManager.hpp"
#include "ExpLM.hpp"
#include "ConUtils.hpp"
#include "ConceptGraph.hpp"
#include "ConceptGraphUtils.hpp"
#include "TermGraph.hpp"

using namespace lemur::api;
using namespace lemur::retrieval;
using namespace lemur::utility;
using namespace lemur::parse;

#define MAX_DOC_RANK_DEF 10
#define MAX_CONT_SIZE_DEF 100
#define NUM_PATH_TERMS_DEF 10
#define NUM_EXP_TERMS_DEF 100
#define MAX_PATH_LEN_DEF 3
#define NEG_FB_COEF_DEF 0.7
#define ORIG_MOD_COEF_DEF 0.7
#define WEIGHT_THRESH_DEF 0.001

namespace LocalParameter {
  // file with term collection graph
  std::string colGraphFile;
  // file with query concept graph
  std::string conGraphFile;
  // file with unstemmed queries
  std::string unstemQryFile;
  // feedback method (NegFB or ExpNegFB)
  std::string fbMethod;
  // maximum rank of the document to consider in the initial retrieval results
  int maxDocRank;
  // number of top weighted terms in the negative language model to consider for path finding
  int numPathTerms;
  // number of top scoring expansion terms to output
  int numExpTerms;
  // maximum length of the path in the concept graph
  int maxPathLen;
  // maximum size of the concept's context
  int maxContSize;
  // coefficient for negative feedback retrieval score
  double negFBCoef;
  // coefficient for interpolating the expansion language model
  double origModCoef;
  // threshold for edge weight in order for edge to be included in the term graph
  double weightThresh;
  // use ConceptNet
  int useConNet;

  void get() {
    colGraphFile = ParamGetString("colGraphFile", "");
    conGraphFile = ParamGetString("conGraphFile", "");
    unstemQryFile = ParamGetString("unstemQuery", "");
    fbMethod = ParamGetString("fbMethod", "");
    maxDocRank = ParamGetInt("maxDocRank", MAX_DOC_RANK_DEF);
    numPathTerms = ParamGetInt("numPathTerms", NUM_PATH_TERMS_DEF);
    numExpTerms = ParamGetInt("numExpTerms", NUM_EXP_TERMS_DEF);
    maxPathLen = ParamGetInt("maxPathLen", MAX_PATH_LEN_DEF);
    maxContSize = ParamGetInt("maxContSize", MAX_CONT_SIZE_DEF);
    origModCoef = ParamGetDouble("origModCoef", ORIG_MOD_COEF_DEF);
    negFBCoef = ParamGetDouble("negFBCoef", NEG_FB_COEF_DEF);
    weightThresh = ParamGetDouble("weightThresh", WEIGHT_THRESH_DEF);
    useConNet = ParamGetInt("useConNet", 0);
  }
}

SimpleKLQueryModel* getNegModel(Index *index, SimpleKLRetMethod *retMethod, SimpleKLQueryModel *origQryModel,
                                IndexedRealVector &origQryResults) {
  SimpleKLQueryModel *negQryModel = new SimpleKLQueryModel(*index);
  COUNT_T maxDocRank, numExpTerms;
  if(LocalParameter::maxDocRank != 0) {
    maxDocRank = origQryResults.size() < LocalParameter::maxDocRank ? origQryResults.size() : LocalParameter::maxDocRank;
  } else {
    maxDocRank = origQryResults.size();
  }
  PseudoFBDocs fbDocs(origQryResults, maxDocRank);
  retMethod->updateQuery(*negQryModel, fbDocs);

  return negQryModel;
}

SimpleKLQueryModel *getExpNegModel(Index *index, SimpleKLRetMethod *retMethod, ConceptGraph *qryGraph,
                                   SimpleKLQueryModel *origQryModel, IndexedRealVector &origQryResults,
                                   TermIdPairVec& qryTerms) {
  COUNT_T maxDocRank, maxExpTerms, maxPathTerms;
  TERMID_T qryTermId;
  IndexedRealVector negLangMod, expNegMod;
  ExpLM expNegLM;
  IndexedRealVector::iterator expIt;
  ConChainList *paths = NULL;
  ConChainListIt pathIt;
  ConChain *path = NULL;
  ConChainIt chainIt;
  SimpleKLQueryModel *negQryModel = new SimpleKLQueryModel(*index);
  if(LocalParameter::maxDocRank != 0) {
    maxDocRank = origQryResults.size() < LocalParameter::maxDocRank ? origQryResults.size() : LocalParameter::maxDocRank;
  } else {
    maxDocRank = origQryResults.size();
  }
  PseudoFBDocs fbDocs(origQryResults, maxDocRank);
  retMethod->updateQuery(*negQryModel, fbDocs);
  negQryModel->startIteration();
  while(negQryModel->hasMore()) {
    QueryTerm *qt = negQryModel->nextTerm();
    negLangMod.PushValue(qt->id(), qt->weight());
  }
  negLangMod.Sort();

  if(LocalParameter::numPathTerms != 0) {
    maxPathTerms = negLangMod.size() < LocalParameter::numPathTerms ? negLangMod.size() : LocalParameter::numPathTerms;
  } else {
    maxPathTerms = negLangMod.size();
  }

  // DEBUG
  //std::cout << "Negative language model:" << std::endl;
  //for(int i = 0; i < maxPathTerms; i++) {
  //  std::cout << index->term(negLangMod[i].ind) << " " << negLangMod[i].val << std::endl;
  //}

  for(int i = 0; i < maxPathTerms; i++) {
    // DEBUG
    // std::cout << "Finding paths to the negative term '" << index->term(negLangMod[i].ind) << "':" << std::endl;
    if(qryGraph->hasConcept(negLangMod[i].ind)) {
      for(int j = 0; j < qryTerms.size(); j++) {
        // DEBUG
        // std::cout << "   from query term '" << qryTerms[j].term << "':" << std::endl;
        paths = qryGraph->getPaths(qryTerms[j].termId, negLangMod[i].ind, LocalParameter::maxPathLen);
        for(pathIt = paths->begin(); pathIt != paths->end(); pathIt++) {
          for(chainIt = pathIt->begin(); chainIt != pathIt->end(); chainIt++) {
            if(chainIt->id != qryTerms[j].termId && chainIt->id != negLangMod[i].ind) {
              if((expIt = expNegMod.FindByIndex(chainIt->id)) != expNegMod.end()) {
                if(chainIt->weight > expIt->val) {
                  expIt->val = chainIt->weight;
                }
              } else {
                expNegMod.PushValue(chainIt->id, chainIt->weight);
              }
            }
            // DEBUG
            // if(chainIt->id == qryTerms[j].termId) {
            //   std::cout << "      " << index->term(chainIt->id);
            // } else {
            //   std::cout << "-" << chainIt->type << "(" << chainIt->weight << ")" << "->" << index->term(chainIt->id);
            // }
          }
          // DEBUG
          // std::cout << std::endl;
        }
        delete paths;
      }
    } else {
      // DEBUG
      // std::cout << "No paths found!" << std::endl;
    }
  }

  expNegMod.Sort();

  if(LocalParameter::numExpTerms != 0) {
    maxExpTerms = expNegMod.size() < LocalParameter::numExpTerms ? expNegMod.size() : LocalParameter::numExpTerms;
  } else {
    maxExpTerms = expNegMod.size();
  }

  std::cout << expNegMod.size() << " expansion terms" << endl;
  // DEBUG
  // std::cout << "Expansion terms:" << std::endl;
  for(int i = 0; i < maxExpTerms; i++) {
    // std::cout << i+1 << ". " << index->term(expNegMod[i].ind) << " " << expNegMod[i].val << std::endl;
    expNegLM.addTerm(expNegMod[i].ind, expNegMod[i].val);
  }

  expNegLM.normalize();

  negQryModel->interpolateWith(expNegLM, LocalParameter::origModCoef, expNegLM.size());

  return negQryModel;
}

void applyNegFB(IndexedRealVector &origQryResults, IndexedRealVector &negQryResults) {
  IndexedRealVector::iterator it;

  for(int i = 0; i < origQryResults.size(); i++) {
    if(LocalParameter::maxDocRank != 0 && i < LocalParameter::maxDocRank) {
      if((it = negQryResults.FindByIndex(origQryResults[i].ind)) != negQryResults.end()) {
        negQryResults.erase(it);
      }
    } else {
      if((it = negQryResults.FindByIndex(origQryResults[i].ind)) != negQryResults.end()) {
        it->val = origQryResults[i].val-LocalParameter::negFBCoef*it->val;
      }
    }
  }
}

void GetAppParam() {
  RetrievalParameter::get();
  SimpleKLParameter::get();
  LocalParameter::get();
}

int AppMain(int argc, char *argv[]) {
  Index *index = NULL;
  TextQuery *query = NULL;
  DocStream *qStream = NULL;
  SimpleKLRetMethod *retMethod = NULL;
  SimpleKLQueryModel *origQryModel, *negQryModel;
  PorterStemmer *stemmer = NULL;
  TermGraph *termGraph = NULL;
  ConceptGraph *qryGraph = NULL;
  QryGraphMap qryGraphMap;
  QryTermMap qryTermMap;
  StrFloatMap relWeights;

  if(!RetrievalParameter::databaseIndex.length()) {
    throw lemur::api::Exception("ConceptNegFB", "Path to the index key file is not specified (parameter index)");
  }

  if(!RetrievalParameter::textQuerySet.length()) {
    throw lemur::api::Exception("ConceptNegFB", "File with query topics is not specified (parameter textQuery)");
  }

  if(!LocalParameter::fbMethod.compare("ExpNegFB") && !LocalParameter::useConNet && !LocalParameter::colGraphFile.length()) {
    throw lemur::api::Exception("ConceptNegFB", "File with collection term graph is not specified (parameter colGraphFile)");
  }

  if(!LocalParameter::fbMethod.compare("ExpNegFB") && LocalParameter::useConNet && !LocalParameter::conGraphFile.length()) {
    throw lemur::api::Exception("ConceptNegFB", "File with query concept graph is not specified (parameter conGraphFile)");
  }

  if(!LocalParameter::fbMethod.compare("ExpNegFB") && LocalParameter::useConNet && !LocalParameter::unstemQryFile.length()) {
    throw lemur::api::Exception("ConceptNegFB", "File with unstemmed queries is not specified (parameter unstemQuery)");
  }

  if(LocalParameter::fbMethod.compare("ExpNegFB") && LocalParameter::fbMethod.compare("NegFB")) {
    throw lemur::api::Exception("ConceptNegFB", "Unknown expansion method (parameter expMethod)");
  }

  try {
    index = IndexManager::openIndex(RetrievalParameter::databaseIndex);
  } catch (lemur::api::Exception &ex) {
    ex.writeMessage(std::cerr);
    throw lemur::api::Exception("ConceptNegFB", "Can't open index. Check paramter index in the configuration file.");
  }

  if(!LocalParameter::fbMethod.compare("ExpNegFB") && LocalParameter::colGraphFile.length()) {
    termGraph = new TermGraph(index);

    try {
      cout << "Loading term graph...";
      cout.flush();
      termGraph->loadFile(LocalParameter::colGraphFile, LocalParameter::weightThresh);
    } catch (lemur::api::Exception &ex) {
      ex.writeMessage(cerr);
      throw lemur::api::Exception("ConceptNegFB", "Problem opening term graph file. Check parameter graphFile in the configuration file.");
    }

    cout << "OK" << endl;
    cout.flush();
  }

  if(!LocalParameter::fbMethod.compare("ExpNegFB") && LocalParameter::useConNet) {
    stemmer = new PorterStemmer();
    readQueriesUnstem(index, stemmer, LocalParameter::unstemQryFile, qryTermMap);
    loadQryConGraph(LocalParameter::conGraphFile, index, termGraph, qryGraphMap);
    initRelWeights(relWeights);
  } else {
    readQueries(index, RetrievalParameter::textQuerySet, qryTermMap);
  }

  ofstream result(RetrievalParameter::resultFile.c_str());
  ResultFile resFile(RetrievalParameter::TRECresultFileFormat);
  resFile.openForWrite(result, *index);

  lemur::retrieval::ArrayAccumulator accumulator(index->docCount());
  retMethod = new SimpleKLRetMethod(*index, SimpleKLParameter::smoothSupportFile, accumulator);
  retMethod->setDocSmoothParam(SimpleKLParameter::docPrm);
  retMethod->setQueryModelParam(SimpleKLParameter::qryPrm);

  try {
    qStream = new lemur::parse::BasicDocStream(RetrievalParameter::textQuerySet);
  } catch (lemur::api::Exception &ex) {
    ex.writeMessage(std::cerr);
    throw lemur::api::Exception("ConceptNegFB", "Can't open query file, check parameter textQuery");
  }

  qStream->startDocIteration();
  while(qStream->hasMore()) {
    query = new TextQuery(*qStream->nextDoc());
    query->startTermIteration();
    while(query->hasMore()) {
      const Term *t = query->nextTerm();
    }
    if(LocalParameter::useConNet) {
      qryGraph = qryGraphMap[query->id()];
    } else {
      qryGraph = new ConceptGraph();
    }
    TermIdPairVec& qryTerms = qryTermMap[query->id()];
    cout << "Processing query #" << query->id() << " (";
    for(int i = 0; i < qryTerms.size(); i++) {
      if(i != 0) {
        cout << " ";
      }
      cout << qryTerms[i].term;
      if(!LocalParameter::fbMethod.compare("ExpNegFB") && !LocalParameter::useConNet) {
        cout.flush();
        addTermGraphRG(qryGraph, index, termGraph, qryTerms[i].termId, LocalParameter::maxPathLen, LocalParameter::maxContSize);
        cout << " " << qryGraph->getNumConcepts();
      }
    }

    cout << ") " << qryGraph->getNumConcepts() << endl;

    if(!LocalParameter::fbMethod.compare("ExpNegFB") && LocalParameter::useConNet) {
      normRelWeights(qryGraph, index, relWeights);
    }
    // DEBUG
    // cout << "Query graph:" << endl;
    // printGraph(qryGraph, index);
    origQryModel = dynamic_cast<SimpleKLQueryModel*>(retMethod->computeTextQueryRep(*query));
    IndexedRealVector origQryResults(index->docCount());
    IndexedRealVector negQryResults(index->docCount());
    retMethod->scoreCollection(*origQryModel, origQryResults);
    if(!LocalParameter::fbMethod.compare("NegFB")) {
      negQryModel = getNegModel(index, retMethod, origQryModel, origQryResults);
    } else if(!LocalParameter::fbMethod.compare("ExpNegFB")) {
      negQryModel = getExpNegModel(index, retMethod, qryGraph, origQryModel, origQryResults, qryTerms);
    }
    retMethod->scoreCollection(*negQryModel, negQryResults);
    applyNegFB(origQryResults, negQryResults);
    negQryResults.Sort();
    resFile.writeResults(query->id(), &negQryResults, RetrievalParameter::resultCount);
    delete negQryModel;
    delete qryGraph;
    delete origQryModel;
    delete query;
  }

  result.close();

  if(termGraph != NULL)
    delete termGraph;
  if(stemmer != NULL)
    delete stemmer;
  delete retMethod;
  delete qStream;
  delete index;
}
