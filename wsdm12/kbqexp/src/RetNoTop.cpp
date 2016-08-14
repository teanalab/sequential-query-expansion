/* -*- C++ -*- */

// Author: Alexander Kotov (UIUC), 2011
// $Id: RetNoTop.cpp,v 1.1 2011/05/13 04:09:36 akotov2 Exp $

#include "common_headers.hpp"
#include "IndexManager.hpp"
#include "BasicDocStream.hpp"
#include "ResultFile.hpp"
#include "RetParamManager.hpp"

using namespace lemur::api;
using namespace lemur::retrieval;
using namespace lemur::utility;

#define START_DOC_RANK_DEF 10

namespace LocalParameter {
  // initial document rank
  int startDocRank;

  void get() {
    startDocRank = ParamGetInt("startDocRank", START_DOC_RANK_DEF);
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
  SimpleKLQueryModel *qryModel;

  try {
    index = IndexManager::openIndex(RetrievalParameter::databaseIndex);
  } catch (lemur::api::Exception &ex) {
    ex.writeMessage(std::cerr);
    throw lemur::api::Exception("ConceptNegFB", "Can't open index. Check paramter index in the configuration file.");
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

    cout << "Processing query #" << query->id() << endl;

    qryModel = dynamic_cast<SimpleKLQueryModel*>(retMethod->computeTextQueryRep(*query));
    IndexedRealVector origQryResults(index->docCount());
    IndexedRealVector trunQryResults;
    retMethod->scoreCollection(*qryModel, origQryResults);
    origQryResults.Sort();
    for(int i = 0; i < origQryResults.size(); i++) {
      if(LocalParameter::startDocRank != 0 && i >= LocalParameter::startDocRank) {
        trunQryResults.PushValue(origQryResults[i].ind, origQryResults[i].val);
      }
    }
    trunQryResults.Sort();
    resFile.writeResults(query->id(), &trunQryResults, RetrievalParameter::resultCount);
    delete qryModel;
    delete query;
  }

  result.close();

  delete retMethod;
  delete qStream;
  delete index;
}
