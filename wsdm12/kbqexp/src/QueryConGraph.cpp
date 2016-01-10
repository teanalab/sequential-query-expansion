/* -*- C++ -*- */

// Author: Alexander Kotov (UIUC), 2011
// $Id: QueryConGraph.cpp,v 1.10 2011/05/19 03:45:55 akotov2 Exp $

#include "common_headers.hpp"
#include "IndexManager.hpp"
#include "BasicDocStream.hpp"
#include "ResultFile.hpp"
#include "RetParamManager.hpp"
#include "ConceptNetClient.hpp"
#include "ConceptGraph.hpp"
#include "ConceptGraphUtils.hpp"

using namespace lemur::api;
using namespace lemur::retrieval;
using namespace lemur::utility;
using namespace lemur::parse;

#define MAX_PATH_LEN_DEF 3
#define MAX_CONT_SIZE_DEF 500
#define DOC_FRAC_MAX_DEF 0.1

namespace LocalParameter {
  // file with unstemmed queries
  std::string unstemQryFile;
  // output file
  std::string resultFile;
  // URL of ConceptNet server
  std::string serverUrl;
  // maximum length of the path in ConceptNet graph
  int maxPathLen;
  // maximum size of the context for a concept
  int maxContSize;
  // threshold for the fraction of documents in which a useful term may occur
  double docFracMax;

  void get() {
    unstemQryFile = ParamGetString("unstemQuery", "");
    resultFile = ParamGetString("resultFile", "");
    serverUrl = ParamGetString("serverUrl", "");
    maxPathLen = ParamGetInt("maxPathLen", MAX_PATH_LEN_DEF);
    maxContSize = ParamGetInt("maxContSize", MAX_CONT_SIZE_DEF);
    docFracMax = ParamGetDouble("docFracMax", DOC_FRAC_MAX_DEF);
  }
}

void GetAppParam() {
  RetrievalParameter::get();
  SimpleKLParameter::get();
  LocalParameter::get();
}

void addTermGraphCN(ConceptGraph *graph, Index *index, TermGraph *termGraph, ConceptNetClient *conNet,
                    PorterStemmer *stemmer, const std::string& srcTerm, StrFloatMap &relWeights,
                    double maxFrac, int maxDist, int maxCont) {
  IdTermMap id2term;
  ConInfo curConInfo;
  TERMID_T curConId, srcTermId;
  float relWeight;
  std::string conPhrase;
  unsigned int curDist, curTerm;
  std::queue<ConInfo> fringe;
  Relation *curRel = NULL;
  ContextIterator *contIt = NULL;
  TermWeightList *conTerms = NULL;
  TermWeightListIt termIt;
  SortRelSetIt rit;
  srcTermId = stemAndGetId(index, stemmer, srcTerm);

  if(srcTermId != 0) {
    id2term[srcTermId] = srcTerm;
    fringe.push(ConInfo(srcTermId, 0, 0));
    if(!graph->hasConcept(srcTermId)) {
      graph->addConcept(srcTermId);
    }
    while(!fringe.empty()) {
      curConInfo = fringe.front();
      curConId = curConInfo.id;
      curDist = curConInfo.dist;
      fringe.pop();
      // DEBUG
      //std::cout << fringe.size() << std::endl;
      if(curDist < maxDist) {
        contIt = conNet->getTermContext(id2term[curConId]);
        if(contIt != NULL) {
          SortRelSet relSet;
          while(contIt->hasNext()) {
            contIt->moveNext();
            conPhrase = contIt->getPhrase();
            conTerms = parseConPhraseIDF(index, stemmer, id2term, curConId, conPhrase, maxFrac);
            for(termIt = conTerms->begin(); termIt != conTerms->end(); termIt++) {
              relSet.insert(RelInfoExt(termIt->termId, contIt->getRelation(), 0, termIt->weight));
            }
            delete conTerms;
          }
          delete contIt;
          for(rit = relSet.begin(), curTerm = 0; rit != relSet.end(); rit++, curTerm++) {
            if(maxCont != 0 && curTerm >= maxCont) {
              break;
            }
            if (!graph->hasConcept(rit->id)) {
              graph->addConcept(rit->id);
            }
            if (!graph->getRelation(curConId, rit->id, &curRel)) {
              if(termGraph != NULL && termGraph->relExists(curConId, rit->id, &relWeight)) {
                graph->addRelation(curConId, rit->id, rit->type, relWeight);
              } else {
                graph->addRelation(curConId, rit->id, rit->type, 0);
              }
              fringe.push(ConInfo(rit->id, curDist+1, 0));
            } else {
              // relation between the same concepts exists
              if (relWeights[rit->type] > relWeights[curRel->type]) {
                // updating the type of existing relation
                graph->updateRelation(curConId, rit->id, rit->type, curRel->weight);
              }
            }
          }
        } else {
          // DEBUG
          // std::cerr << "      Term '" << id2term[curConId] << "' is not found in ConceptNet" << std::endl;
        }
      }
    }
  } else {
    // DEBUG
    // std::cerr << "Query term '" << srcTerm << "' is not found in the index" << std::endl;
  }
}

void storeQueryGraph(ofstream& outFile, const std::string& queryId, Index *index, ConceptGraph *conGraph) {
  TERMID_T srcConId, trgConId;
  Concept* con;
  Relation* rel;

  outFile << "#" << queryId << std::endl;
  ConceptGraph::ConIterator conIt(conGraph);
  conIt.startIteration();
  while(conIt.hasMore()) {
    conIt.getNext(srcConId, &con);
    outFile << index->term(srcConId) << std::endl;
    Concept::RelIterator relIt(con);
    relIt.startIteration();
    while(relIt.hasMore()) {
      relIt.getNext(trgConId, &rel);
      outFile << index->term(trgConId) << " " << rel->type << std::endl;
    }
  }
}

int AppMain(int argc, char *argv[]) {
  Index *index = NULL;
  TextQuery *query = NULL;
  DocStream *qStream = NULL;
  ConceptNetClient *conNet = NULL;
  PorterStemmer *stemmer = NULL;
  ConceptGraph *queryGraph = NULL;
  QryTermMap qryMap;
  StrFloatMap relWeights;
  ofstream outFile;

  if(!RetrievalParameter::databaseIndex.length()) {
    throw lemur::api::Exception("QueryConGraph", "Path to the index key file is not specified (parameter index)");
  }

  if(!RetrievalParameter::textQuerySet.length()) {
    throw lemur::api::Exception("QueryConGraph", "File with query topics is not specified (parameter textQuery)");
  }

  if(!LocalParameter::unstemQryFile.length()) {
    throw lemur::api::Exception("QueryConGraph", "File with unstemmed queries is not specified (parameter unstemQuery)");
  }

  if(LocalParameter::resultFile.length()) {
    outFile.open(LocalParameter::resultFile.c_str());
    if(!outFile.is_open()) {
      throw lemur::api::Exception("QueryConGraph", "Can't open output file for writing. Check parameter resultFile in the configuration file.");
    }
  } else {
    throw lemur::api::Exception("QueryConGraph", "Output file is not specified (parameter resultFile)");
  }

  if(!LocalParameter::serverUrl.length()) {
    throw lemur::api::Exception("QueryConGraph", "URL of ConceptNet server is not specified (parameter serverUrl)");
  }

  try {
    index = IndexManager::openIndex(RetrievalParameter::databaseIndex);
  } catch (lemur::api::Exception &ex) {
    ex.writeMessage(std::cerr);
    throw lemur::api::Exception("QueryConGraph", "Can't open index. Check parameter index in the configuration file.");
  }

  conNet = new ConceptNetClient(LocalParameter::serverUrl);
  stemmer = new PorterStemmer();
  readQueriesUnstem(index, stemmer, LocalParameter::unstemQryFile, qryMap);

  try {
    qStream = new lemur::parse::BasicDocStream(RetrievalParameter::textQuerySet);
  } catch (lemur::api::Exception &ex) {
    ex.writeMessage(std::cerr);
    throw lemur::api::Exception("QueryConGraph", "Can't open query file, check parameter textQuery");
  }

  initRelWeights(relWeights);

  qStream->startDocIteration();
  while(qStream->hasMore()) {
    Document *d = qStream->nextDoc();
    query = new TextQuery(*d);
    query->startTermIteration();
    while(query->hasMore()) {
      const Term *t = query->nextTerm();
    }
    queryGraph = new ConceptGraph();
    TermIdPairVec qryTerms = qryMap[query->id()];
    cout << "Processing query #" << query->id() << " (";
    for(int i = 0; i < qryTerms.size(); i++) {
      cout << " " << qryTerms[i].term;
      cout.flush();
      addTermGraphCN(queryGraph, index, NULL, conNet, stemmer, qryTerms[i].term, relWeights,
                     LocalParameter::docFracMax, LocalParameter::maxPathLen, LocalParameter::maxContSize);
    }
    cout << " ) " << queryGraph->getNumConcepts() << endl;
    storeQueryGraph(outFile, query->id(), index, queryGraph);

    delete queryGraph;
    delete query;
  }

  outFile.close();
  delete stemmer;
  delete conNet;
  delete qStream;
  delete index;
}
