/* -*- C++ -*- */

// Author: Alexander Kotov (UIUC), 2011
// $Id: ConceptQExp.cpp,v 1.15 2011/06/24 00:10:29 akotov2 Exp $

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

#define ORIG_LM_COEF_DEF 0.7
#define WEIGHT_THRESH_DEF 0.001
#define EXP_RADIUS_DEF 3
#define MAX_EXP_TERMS_DEF 100
#define MAX_PATH_LEN_DEF 3
#define MAX_CONT_SIZE_DEF 100
#define WALK_ALPHA_DEF 0.8
#define WALK_STEPS_DEF 3

namespace LocalParameter {
  // file with collection term graph
  std::string colGraphFile;
  // file with concept graphs for each query
  std::string conGraphFile;
  // file with unstemmed queries (for ConceptNet graph only)
  std::string unstemQryFile;
  // query expansion method
  std::string expMethod;
  // file with expansion candidates
  std::string outFile;
  // radius of query context
  int expRadius;
  // maximum path length
  int maxPathLen;
  // maximum size of the concept's context (for collection graph only)
  int maxContSize;
  // maximum number of expansion terms
  int maxExpTerms;
  // coefficient for interpolation of the expansion language model
  double origModCoef;
  // threshold for edge weight in order for the edge to be included in the collection term graph
  double weightThresh;
  // parameter alpha of random walk
  double walkAlpha;
  // number of iterations of random walk
  int walkSteps;
  // use ConceptNet
  int useConNet;

  void get() {
    colGraphFile = ParamGetString("colGraphFile", "");
    conGraphFile = ParamGetString("conGraphFile", "");
    unstemQryFile = ParamGetString("unstemQuery", "");
    expMethod = ParamGetString("expMethod", "");
    outFile = ParamGetString("outFile", "");
    expRadius = ParamGetInt("expRadius", EXP_RADIUS_DEF);
    maxPathLen = ParamGetInt("maxPathLen", MAX_PATH_LEN_DEF);
    maxContSize = ParamGetInt("maxContSize", MAX_CONT_SIZE_DEF);
    maxExpTerms = ParamGetInt("maxExpTerms", MAX_EXP_TERMS_DEF);
    origModCoef = ParamGetDouble("origModCoef", ORIG_LM_COEF_DEF);
    weightThresh = ParamGetDouble("weightThresh", WEIGHT_THRESH_DEF);
    walkAlpha = ParamGetDouble("walkAlpha", WALK_ALPHA_DEF);
    walkSteps = ParamGetInt("walkSteps", WALK_STEPS_DEF);
    useConNet = ParamGetInt("useConNet", 1);
  }
}

void GetAppParam() {
  RetrievalParameter::get();
  SimpleKLParameter::get();
  LocalParameter::get();
}

void expLangMod(SimpleKLQueryModel *queryLM, IndexedRealVector *expTermVec) {
  ExpLM expLM;
  COUNT_T maxExpTerms;

  if(LocalParameter::maxExpTerms != 0) {
    maxExpTerms = expTermVec->size() < LocalParameter::maxExpTerms ? expTermVec->size() : LocalParameter::maxExpTerms;
  } else {
    maxExpTerms = expTermVec->size();
  }

  if(expTermVec->size()) {
    for(int i = 0; i < maxExpTerms; i++) {
      expLM.addTerm(expTermVec->at(i).ind, expTermVec->at(i).val);
    }
    expLM.normalize();
    queryLM->interpolateWith(expLM, LocalParameter::origModCoef, expLM.size());
  }
}

void storeExpCands(ofstream& outFile, Index *index, const std::string& queryId, const IndexedRealVector *expTerms) {
  outFile << queryId << std::endl;
  for(unsigned int i = 0; i < expTerms->size(); i++) {
    outFile << index->term(expTerms->at(i).ind) << " " << expTerms->at(i).val << std::endl;
  }
}

void getContInter(const ConInfoSet *context1, const ConInfoSet *context2, IndexedRealVector *expTerms) {
  TermWeightMap contMap1, contMap2, *smCont, *lgCont;
  TermWeightMapIt sit, lit;
  IndexedRealVector::iterator eit;

  for(cConInfoSetIt cit1 = context1->begin(); cit1 != context1->end(); cit1++) {
    contMap1[cit1->id] = cit1->weight;
  }

  for(cConInfoSetIt cit2 = context2->begin(); cit2 != context2->end(); cit2++) {
    contMap2[cit2->id] = cit2->weight;
  }


  if(contMap1.size() >= contMap2.size()) {
    lgCont = &contMap1;
    smCont = &contMap2;
  } else {
    lgCont = &contMap2;
    smCont = &contMap1;
  }

  for(sit = smCont->begin(); sit != smCont->end(); sit++) {
    if((lit = lgCont->find(sit->first)) != lgCont->end()) {
      expTerms->IncreaseValueFor(sit->first, sit->second);
    }
  }
}

// expansion based on overlapping contexts of query terms
IndexedRealVector* expOverCont(Index *index, ConceptGraph *qryGraph, const TermIdPairVec& qryTerms, int expRadius) {
  ConInfoSet *context1, *context2;
  IndexedRealVector *expTerms = new IndexedRealVector();

  /*TermWeightMap *curInt, *prevInt = new TermWeightMap();

  if(qryTerms.size() > 1) {
    context = qryGraph->getContext(qryTerms[0], expRadius);

    for(cit = context->begin(); cit != context->end(); cit++) {
      prevInt->insert(std::make_pair(cit->id, 0));
    }
  } else {
    return expTerms;
  }

  for(int i = 1; i < qryTerms.size(); i++) {
    curInt = new TermWeightMap();
    context = getContext(index, termGraph, conNet, stemmer, qryTermIds[i].term, expRadius, useWeights);
    // DEBUG
    // std::cerr << "Context for query term '" << qryTermIds[i].term << "'" << std::endl;
    // for(cit = context->begin(); cit != context->end(); cit++) {
    //   std::cerr << "   " << index->term(cit->conTermId) << ": " << cit->weight << std::endl;
    // }

    for(cit = context->begin(); cit != context->end(); cit++) {
      if((mit = prevInt->find(cit->id)) != prevInt->end()) {
        curInt->insert(std::make_pair(cit->id, mit->second + 0));
      }
    }
    delete context;
    delete prevInt;
    prevInt = curInt;
    if(curInt->size() == 0)
      break;
  }

  // DEBUG
  //std::cerr << "Intersection:" << std::endl;
  for(mit = curInt->begin(); mit != curInt->end(); mit++) {
    if(!inTermVec(mit->first, qryTermIds)) {
      expTerms->PushValue(mit->first, mit->second);
    }
    //std::cerr << index->term(mit->first) << ": " << mit->second << std::endl;
  }*/

  for(int i = 0; i < qryTerms.size(); i++) {
    context1 = qryGraph->getContextDist(qryTerms[i].termId, expRadius);
    if(context1->size()) {
      for(int j = i+1; j < qryTerms.size(); j++) {
        context2 = qryGraph->getContextDist(qryTerms[j].termId, expRadius);
        // DEBUG
        //std::cout << context1->size() << "  " << context2->size() << std::endl;
        if(context1->size()) {
          getContInter(context1, context2, expTerms);
        }
        delete context2;
      }
    }
    delete context1;
  }

  return expTerms;
}

// expansion based on finding paths between the query terms
IndexedRealVector* expQTermPaths(Index *index, ConceptGraph *qryGraph, const TermIdPairVec& qryTerms, int maxPath) {
  ConChainList *paths = NULL;
  ConChainListIt pathIt;
  ConChain *path = NULL;
  ConChainIt chainIt;
  IndexedRealVector *expTerms = new IndexedRealVector();
  IndexedRealVector::iterator eit;

  if(qryTerms.size() > 1) {
    for(int i = 0; i < qryTerms.size(); i++) {
      if(qryGraph->hasConcept(qryTerms[i].termId)) {
        for(int j = i+1; j < qryTerms.size(); j++) {
          // DEBUG
          // std::cerr << "Finding paths from '" << qryTerms[i].term << "' to '" << qryTerms[j].term << std::endl;
          if(qryGraph->hasConcept(qryTerms[j].termId)) {
            paths = qryGraph->getPaths(qryTerms[i].termId, qryTerms[j].termId, LocalParameter::maxPathLen);
            for(pathIt = paths->begin(); pathIt != paths->end(); pathIt++) {
              for(chainIt = pathIt->begin(); chainIt != pathIt->end(); chainIt++) {
                if(chainIt->id != qryTerms[i].termId && chainIt->id != qryTerms[j].termId) {
                  if((eit = expTerms->FindByIndex(chainIt->id)) != expTerms->end()) {
                    if(chainIt->weight > eit->val) {
                      eit->val = chainIt->weight;
                    }
                  } else {
                    expTerms->PushValue(chainIt->id, chainIt->weight);
                  }
                }
                // DEBUG
                //if(chainIt->id == qryTerms[i].termId) {
                //  std::cout << "   " << index->term(chainIt->id);
                //} else {
                //  std::cout << "-" << chainIt->type << "(" << chainIt->weight << ")" << "->" << index->term(chainIt->id);
                //}
              }
              // DEBUG
              //std::cout << std::endl;
            }
            delete paths;
          } else {
            // DEBUG
            //std::cout << "No paths found!" << std::endl;
          }
        }
      }
    }
  } else {
    paths = qryGraph->getPaths(qryTerms[0].termId, qryTerms[0].termId, LocalParameter::maxPathLen);
    for(pathIt = paths->begin(); pathIt != paths->end(); pathIt++) {
      for(chainIt = pathIt->begin(); chainIt != pathIt->end(); chainIt++) {
        if(chainIt->id != qryTerms[0].termId) {
          if((eit = expTerms->FindByIndex(chainIt->id)) != expTerms->end()) {
            if(chainIt->weight > eit->val) {
              eit->val = chainIt->weight;
            }
          } else {
            expTerms->PushValue(chainIt->id, chainIt->weight);
          }
        }
      }
    }
    delete paths;
  }

  return expTerms;
}

// expansion based on a finite random walk on weighted concept graph
IndexedRealVector* expFinRandWalk(Index *index, ConceptGraph *qryGraph, const TermIdPairVec& qryTerms) {
  TERMID_T conId;
  SparseMatrix<float>* walkMat = NULL;
  IndexedRealVector *expTerms = new IndexedRealVector();

  qryGraph->getFinRandWalkMat(walkMat, LocalParameter::walkAlpha, LocalParameter::walkSteps);
  for(unsigned int i = 0; i < qryTerms.size(); i++) {
    Row<float> *row = NULL;
    if(walkMat->exists(qryTerms[i].termId, &row)) {
      SparseMatrix<float>::ColIterator cit(row);
      for(conId = cit.begin(); !cit.end(); conId = cit.next()) {
        if(!inTermVec(conId, qryTerms)) {
          expTerms->IncreaseValueFor(conId, cit.val());
        }
      }
    }
  }

  delete walkMat;

  return expTerms;
}

// expansion based on an infinite random walk on weighted concept graph
IndexedRealVector* expInfRandWalk(Index *index, ConceptGraph *qryGraph, const TermIdPairVec& qryTerms) {
  float** walkMat;
  TERMID_T* termIDs;

  IndexedRealVector *expTerms = new IndexedRealVector();
  unsigned int numCon = qryGraph->getInfRandWalkMat(walkMat, termIDs, LocalParameter::walkAlpha);
  for(unsigned int i = 0; i < qryTerms.size(); i++) {
    for(unsigned int j = 0; j < numCon; j++) {
      if(termIDs[j] == qryTerms[i].termId) {
        for(unsigned int k = 0; k < numCon; k++) {
          if(walkMat[j][k] != 0 && !inTermVec(termIDs[k], qryTerms)) {
            expTerms->IncreaseValueFor(termIDs[k], walkMat[j][k]);
          }
        }
        break;
      }
    }
  }

  for(unsigned int i = 0; i < numCon; i++) {
    delete[] walkMat[i];
  }
  delete[] walkMat;
  delete[] termIDs;

  return expTerms;
}

int AppMain(int argc, char *argv[]) {
  Index *index = NULL;
  TextQuery *query = NULL;
  DocStream *qStream = NULL;
  SimpleKLRetMethod *retMethod = NULL;
  SimpleKLQueryModel *queryModel = NULL;
  IndexedRealVector *expTermVec = NULL;
  PorterStemmer *stemmer = NULL;
  TermGraph *termGraph = NULL;
  ConceptGraph *qryGraph = NULL;
  QryGraphMap qryGraphMap;
  QryTermMap qryTermMap;
  StrFloatMap relWeights;
  ofstream outFile;

  if(!RetrievalParameter::databaseIndex.length()) {
    throw lemur::api::Exception("ConceptQExp", "Path to the index key file is not specified (parameter index)");
  }

  if(!RetrievalParameter::textQuerySet.length()) {
    throw lemur::api::Exception("ConceptQExp", "File with query topics is not specified in the configuration file (parameter textQuery)");
  }

  if(!LocalParameter::useConNet && !LocalParameter::colGraphFile.length()) {
    throw lemur::api::Exception("ConceptQExp", "File with collection term graph is not specified (parameter colGraphFile)");
  }

  if(LocalParameter::useConNet && !LocalParameter::conGraphFile.length()) {
    throw lemur::api::Exception("ConceptQExp", "File with query concept graph is not specified (parameter conGraphFile)");
  }

  if(LocalParameter::useConNet && !LocalParameter::unstemQryFile.length()) {
    throw lemur::api::Exception("ConceptQExp", "File with unstemmed queries is not specified (parameter unstemQuery)");
  }

  if(!LocalParameter::expMethod.length()) {
    throw lemur::api::Exception("ConceptQExp", "Query expansion method is not specified (parameter expMethod)");
  }

  if(LocalParameter::expMethod.compare("OverCont") != 0 && LocalParameter::expMethod.compare("TermPath") != 0 &&
      LocalParameter::expMethod.compare("FinRandWalk") != 0 && LocalParameter::expMethod.compare("InfRandWalk") != 0) {
    std::string errMsg = std::string("Unknown query expansion method: ") + LocalParameter::expMethod;
    throw lemur::api::Exception("ConceptQExp", errMsg.c_str());
  }

  if(LocalParameter::outFile.length()) {
    outFile.open(LocalParameter::outFile.c_str());
    if(!outFile.is_open()) {
      throw lemur::api::Exception("ConceptQExp", "Can't open output file for writing. Check parameter outFile in the configuration file.");
    }
  } else {
    throw lemur::api::Exception("ConceptQExp", "File with expansion candidates is not specified");
  }

  try {
    index = IndexManager::openIndex(RetrievalParameter::databaseIndex);
  } catch (lemur::api::Exception &ex) {
    ex.writeMessage(std::cerr);
    throw lemur::api::Exception("ConceptQExp", "Can't open index. Check paramter index in the configuration file.");
  }

  if(LocalParameter::colGraphFile.length()) {
    termGraph = new TermGraph(index);

    try {
      cout << "Loading term graph...";
      cout.flush();
      termGraph->loadFile(LocalParameter::colGraphFile, LocalParameter::weightThresh);
    } catch (lemur::api::Exception &ex) {
      ex.writeMessage(cerr);
      throw lemur::api::Exception("ConceptNegFB", "Problem loading term graph file. Check parameter graphFile in the configuration file.");
    }

    cout << "OK" << endl;
    cout.flush();
  }

  if(LocalParameter::useConNet) {
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
    throw lemur::api::Exception("ConceptQExp", "Can't open query file, check parameter textQuery");
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
      if(!LocalParameter::useConNet) {
        cout.flush();
        addTermGraphRG(qryGraph, index, termGraph, qryTerms[i].termId, LocalParameter::maxPathLen, LocalParameter::maxContSize);
        cout << " " << qryGraph->getNumConcepts();
      }
    }

    cout << ") " << qryGraph->getNumConcepts() << endl;

    if(LocalParameter::useConNet) {
      normRelWeights(qryGraph, index, relWeights);
    }

    IndexedRealVector queryResults(index->docCount());
    queryModel = dynamic_cast<SimpleKLQueryModel*>(retMethod->computeTextQueryRep(*query));

    if(!LocalParameter::expMethod.compare("OverCont")) {
      expTermVec = expOverCont(index, qryGraph, qryTerms, LocalParameter::expRadius);
    } else if(!LocalParameter::expMethod.compare("TermPath")) {
      expTermVec = expQTermPaths(index, qryGraph, qryTerms, LocalParameter::maxPathLen);
    } else if(!LocalParameter::expMethod.compare("FinRandWalk")) {
      expTermVec = expFinRandWalk(index, qryGraph, qryTerms);
    } else if(!LocalParameter::expMethod.compare("InfRandWalk")) {
      expTermVec = expInfRandWalk(index, qryGraph, qryTerms);
    }

    // DEBUG
    if(expTermVec->size() != 0) {
      std::cout << expTermVec->size() << " expansion terms" << std::endl;
      // printTermVec(index, expTermVec);
    } else {
      std::cout << "No expansion terms found" << std::endl;
    }

    expTermVec->Sort();
    storeExpCands(outFile, index, query->id(), expTermVec);
    expLangMod(queryModel, expTermVec);
    retMethod->scoreCollection(*queryModel, queryResults);
    queryResults.Sort();
    resFile.writeResults(query->id(), &queryResults, RetrievalParameter::resultCount);

    delete expTermVec;
    delete qryGraph;
    delete queryModel;
    delete query;
  }

  outFile.close();
  result.close();

  if(termGraph != NULL)
    delete termGraph;
  if(stemmer != NULL)
    delete stemmer;
  delete retMethod;
  delete qStream;
  delete index;
}
