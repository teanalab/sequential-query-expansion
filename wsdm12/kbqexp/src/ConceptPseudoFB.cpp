/* -*- C++ -*- */

// Author: Alexander Kotov (UIUC), 2011
// $Id: ConceptPseudoFB.cpp,v 1.1 2011/06/24 00:10:29 akotov2 Exp $

#include "common_headers.hpp"
#include "IndexManager.hpp"
#include "BasicDocStream.hpp"
#include "ResultFile.hpp"
#include "RetParamManager.hpp"
#include "ExpLM.hpp"
#include "ConUtils.hpp"

using namespace lemur::api;
using namespace lemur::retrieval;

#define MAX_EXP_TERMS_DEF 100
#define MAX_DOC_RANK_DEF 10
#define ORIG_LM_COEF_DEF 0.7

namespace LocalParameter {
  // file with with expansion candidates
  std::string termFile;
  // maximum number of expansion terms
  int maxExpTerms;
  // maximum rank of a document to use for pseudo-feedback
  int maxDocRank;
  // coefficient of the original language model when interpolating the expansion language model
  double origModCoef;

  void get() {
    termFile = ParamGetString("termFile", "");
    maxExpTerms = ParamGetInt("maxExpTerms", MAX_EXP_TERMS_DEF);
    maxDocRank = ParamGetInt("maxDocRank", MAX_DOC_RANK_DEF);
    origModCoef = ParamGetDouble("origModCoef", ORIG_LM_COEF_DEF);
  }
}

void GetAppParam() {
  RetrievalParameter::get();
  SimpleKLParameter::get();
  LocalParameter::get();
}

void expLangMod(SimpleKLQueryModel *queryLM, IndexedRealVector &expTermVec) {
  ExpLM expLM;
  COUNT_T maxExpTerms;

  if(LocalParameter::maxExpTerms != 0) {
    maxExpTerms = expTermVec.size() < LocalParameter::maxExpTerms ? expTermVec.size() : LocalParameter::maxExpTerms;
  } else {
    maxExpTerms = expTermVec.size();
  }

  if(expTermVec.size()) {
    for(int i = 0; i < maxExpTerms; i++) {
      expLM.addTerm(expTermVec[i].ind, expTermVec[i].val);
    }
    expLM.normalize();
    queryLM->interpolateWith(expLM, LocalParameter::origModCoef, expLM.size());
  }
}

void processQuery(const std::string &qryId, Index* index, SimpleKLRetMethod *retMethod, SimpleKLQueryModel *qryModel,
                  const TermWeightMap &expTermMap, ResultFile &resFile) {
  COUNT_T maxDocRank;
  IndexedRealVector expTerms;
  IndexedRealVector expQryRes(index->docCount());
  for(cTermWeightMapIt mit = expTermMap.begin(); mit != expTermMap.end(); mit++) {
    expTerms.PushValue(mit->first, mit->second);
  }
  expTerms.Sort();
  expLangMod(qryModel, expTerms);
  retMethod->scoreCollection(*qryModel, expQryRes);
  expQryRes.Sort();
  if(LocalParameter::maxDocRank != 0) {
    maxDocRank = expQryRes.size() < LocalParameter::maxDocRank ? expQryRes.size() : LocalParameter::maxDocRank;
  } else {
    maxDocRank = expQryRes.size();
  }
  PseudoFBDocs fbDocs(expQryRes, maxDocRank);
  retMethod->updateQuery(*qryModel, fbDocs);
  expQryRes.clear();
  retMethod->scoreCollection(*qryModel, expQryRes);
  expQryRes.Sort();
  resFile.writeResults(qryId, &expQryRes, RetrievalParameter::resultCount);
}

int AppMain(int argc, char *argv[]) {
  Index *index = NULL;
  TextQuery *query = NULL;
  DocStream *qStream = NULL;
  SimpleKLRetMethod *retMethod = NULL;
  SimpleKLQueryModel *queryModel = NULL;
  QryExpTerms qryExpTerms;

  if(!RetrievalParameter::databaseIndex.length()) {
    throw lemur::api::Exception("ConceptPseudoFB", "Path to the index key file is not specified (parameter index)");
  }

  if(!RetrievalParameter::resultFile.length()) {
    throw lemur::api::Exception("ConceptPseudoFB", "Path to the file with retrieval results is not specified (parameter resultFile)");
  }

  if(!LocalParameter::termFile.length()) {
    throw lemur::api::Exception("ConceptPseudoFB", "Path to the file with expansion terms is not specified (parameter termFile)");
  }

  try {
    index = IndexManager::openIndex(RetrievalParameter::databaseIndex);
  } catch (lemur::api::Exception &ex) {
    ex.writeMessage(std::cerr);
    throw lemur::api::Exception("ConceptPseudoFB", "Can't open index. Check parameter index in the configuration file.");
  }

  cout << "Loading expansion terms...";
  cout.flush();
  if(!readExpTermsFile(LocalParameter::termFile, index, qryExpTerms)) {
    throw lemur::api::Exception("ConceptPseudoFB", "can't read file with expansion terms");
  }
  cout << "OK" << endl;

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
    throw lemur::api::Exception("ConceptPseudoFB", "Can't open query file, check parameter textQuery");
  }

  qStream->startDocIteration();
  while(qStream->hasMore()) {
    query = new TextQuery(*qStream->nextDoc());
    cout << "Processing query #" << query->id() << endl;
    queryModel = dynamic_cast<SimpleKLQueryModel*>(retMethod->computeTextQueryRep(*query));
    query->startTermIteration();
    while(query->hasMore()) {
      const Term *t = query->nextTerm();
    }
    const TermWeightMap &expTerms = qryExpTerms[query->id()];
    processQuery(query->id(), index, retMethod, queryModel, expTerms, resFile);
    delete queryModel;
    delete query;
  }

  result.close();

  delete retMethod;
  delete qStream;
  delete index;
}
