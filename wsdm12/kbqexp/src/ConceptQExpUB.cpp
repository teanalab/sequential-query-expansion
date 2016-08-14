#include "common_headers.hpp"
#include "IndexManager.hpp"
#include "BasicDocStream.hpp"
#include "ResultFile.hpp"
#include "RetParamManager.hpp"
#include "ExpLM.hpp"
#include "ConUtils.hpp"
#include "ConceptGraph.hpp"
#include "ConceptGraphUtils.hpp"

using namespace lemur::api;
using namespace lemur::retrieval;
using namespace lemur::utility;
using namespace lemur::parse;

#define EXP_RADIUS_DEF 1

namespace LocalParameter {
  // file with query concept graph
  std::string conGraphFile;
  // file with unstemmed queries
  std::string unstemQryFile;
  // file with query judgments
  std::string judgFile;
  // expansion radius from query terms
  int expRadius;

  void get() {
    conGraphFile = ParamGetString("conGraphFile", "");
    unstemQryFile = ParamGetString("unstemQuery", "");
    judgFile = ParamGetString("judgFile", "");
    expRadius = ParamGetInt("expRadius", EXP_RADIUS_DEF);
  }
}

void GetAppParam() {
  RetrievalParameter::get();
  SimpleKLParameter::get();
  LocalParameter::get();
}

float queryMAP(IndexedRealVector &qryRes, IndexedRealVector *qryJudg) {
  IndexedRealVector::iterator it;
  COUNT_T docCnt, relDocs = 0, totRelDocs = qryJudg->size();
  COUNT_T maxInd;
  double totMAP = 0;

  maxInd = qryRes.size() < RetrievalParameter::resultCount ? qryRes.size() : RetrievalParameter::resultCount;

  for(docCnt = 1; docCnt <= maxInd; docCnt++) {
    if((it = qryJudg->FindByIndex(qryRes[docCnt-1].ind)) != qryJudg->end()) {
      ++relDocs;
      totMAP += ((float) relDocs) / docCnt;
    }
  }

  return relDocs != 0 ? totMAP / totRelDocs : 0.0;
}

IdSet *getQryExpTerms(ConceptGraph *qryGraph, const TermIdPairVec& qryTerms, int expRadius) {
  ConInfoSet *context;
  IdSet *expTids = new IdSet();

  for(int i = 0; i < qryTerms.size(); i++) {
    context = qryGraph->getContextChain(qryTerms[i].termId, expRadius);
    for(ConInfoSetIt cit = context->begin(); cit != context->end(); cit++) {
      expTids->insert(cit->id);
    }
    delete context;
  }

  return expTids;
}

void processQuery(Index* index, SimpleKLRetMethod *retMethod, SimpleKLQueryModel *origQryModel,
                  IndexedRealVector *qryJudg, const std::string &qryId, const IdSet *expTerms, ResultFile &resFile,
                  float *origMAP, float *maxMAP) {
  float expMAP;
  IndexedRealVector origQryRes(index->docCount());
  retMethod->scoreCollection(*origQryModel, origQryRes);
  origQryRes.Sort();
  IndexedRealVector *bestRes = &origQryRes;
  *origMAP = queryMAP(origQryRes, qryJudg);
  *maxMAP = 0;
  for(IdSetIt expIt = expTerms->begin(); expIt != expTerms->end(); expIt++) {
    IndexedRealVector* expQryRes = new IndexedRealVector(index->docCount());
    SimpleKLQueryModel *expQryModel = new SimpleKLQueryModel(*index);
    origQryModel->startIteration();
    while(origQryModel->hasMore()) {
      QueryTerm *qt = origQryModel->nextTerm();
      expQryModel->setCount(qt->id(), qt->weight());
      delete qt;
    }
    expQryModel->incCount(*expIt, 1);
    retMethod->scoreCollection(*expQryModel, *expQryRes);
    expQryRes->Sort();
    expMAP = queryMAP(*expQryRes, qryJudg);
    // DEBUG
    // std::cout << std::endl << index->term(*expIt) << " " << expMAP;
    if(expMAP > *maxMAP) {
      *maxMAP = expMAP;
      if(bestRes != &origQryRes) {
        delete bestRes;
      }
      bestRes = expQryRes;
    } else {
      delete expQryRes;
    }
    delete expQryModel;
  }

  resFile.writeResults(qryId, bestRes, RetrievalParameter::resultCount);

  if(bestRes != &origQryRes) {
    delete bestRes;
  }
}

int AppMain(int argc, char *argv[]) {
  Index *index = NULL;
  TextQuery *query = NULL;
  DocStream *qStream = NULL;
  ifstream *judgStr = NULL;
  ResultFile *judgFile = NULL;
  SimpleKLRetMethod *retMethod = NULL;
  SimpleKLQueryModel *origQryModel;
  PorterStemmer *stemmer = NULL;
  ConceptGraph *qryGraph = NULL;
  IdSet *qryExpTerms = NULL;
  QryGraphMap qryGraphMap;
  QryTermMap qryTermMap;
  StrFloatMap relWeights;
  float origMAP, maxMAP;
  unsigned int qryTot = 0, qryImp = 0, qryNeut = 0;

  if(!RetrievalParameter::databaseIndex.length()) {
    throw lemur::api::Exception("ConceptQExpUB", "Path to the index key file is not specified (parameter index)");
  }

  if(!RetrievalParameter::textQuerySet.length()) {
    throw lemur::api::Exception("ConceptQExpUB", "File with query topics is not specified (parameter textQuery)");
  }

  if(!LocalParameter::conGraphFile.length()) {
    throw lemur::api::Exception("ConceptQExpUB", "File with query concept graph is not specified (parameter conGraphFile)");
  }

  if(!LocalParameter::unstemQryFile.length()) {
    throw lemur::api::Exception("ConceptQExpUB", "File with unstemmed queries is not specified (parameter unstemQuery)");
  }

  if(!LocalParameter::judgFile.length()) {
    throw lemur::api::Exception("ConceptQExpUB", "File with query judgments is not specified (parameter judgFile)");
  }

  try {
    index = IndexManager::openIndex(RetrievalParameter::databaseIndex);
  } catch (lemur::api::Exception &ex) {
    ex.writeMessage(std::cerr);
    throw lemur::api::Exception("ConceptQExpUB", "Can't open index. Check paramter index in the configuration file.");
  }

  judgStr = new ifstream(LocalParameter::judgFile.c_str(), ios::in | ios::binary);
  if(judgStr->fail()) {
    throw lemur::api::Exception("ConceptQExpUB", "can't open the file with query judgments");
  }

  judgFile = new ResultFile(false);
  judgFile->load(*judgStr, *index);

  stemmer = new PorterStemmer();
  readQueriesUnstem(index, stemmer, LocalParameter::unstemQryFile, qryTermMap);
  std::cout << "Loading concept graphs for queries...";
  std::cout.flush();
  if(!loadQryConGraph(LocalParameter::conGraphFile, index, NULL, qryGraphMap)) {
    std::cout << "Failed" << std::endl;
  } else {
    std::cout << "OK" << std::endl;
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
    throw lemur::api::Exception("ConceptNetQExp", "Can't open query file, check parameter textQuery");
  }

  qStream->startDocIteration();
  while(qStream->hasMore()) {
    qryTot++;
    IndexedRealVector *qryJudg = NULL;
    query = new TextQuery(*qStream->nextDoc());

    if(!judgFile->findResult(query->id(), qryJudg)) {
      std::cerr << "Can't find judgments for query " << query->id() << std::endl;
      delete query;
      continue;
    }

    origQryModel = dynamic_cast<SimpleKLQueryModel*>(retMethod->computeTextQueryRep(*query));
    qryGraph = qryGraphMap[query->id()];
    TermIdPairVec& qryTerms = qryTermMap[query->id()];

    cout << "Processing query #" << query->id() << " (";
    query->startTermIteration();
    int i = 0;
    while(query->hasMore()) {
      if(i++ != 0) {
        cout << " ";
      }
      const Term *t = query->nextTerm();
      cout << t->spelling();
    }
    cout << ") ";

    qryExpTerms = getQryExpTerms(qryGraph, qryTerms, LocalParameter::expRadius);
    cout << qryExpTerms->size();
    processQuery(index, retMethod, origQryModel, qryJudg, query->id(), qryExpTerms, resFile, &origMAP, &maxMAP);
    cout << " origMAP=" << origMAP << " maxMAP=" << maxMAP;

    if(maxMAP-origMAP > 0) {
      qryImp++;
      cout << " +" << endl;
    } else if(maxMAP == 0 || maxMAP-origMAP == 0) {
      qryNeut++;
      cout << " 0" << endl;
    } else {
      cout << " -" << endl;
    }

    delete qryJudg;
    delete qryExpTerms;
    delete qryGraph;
    delete origQryModel;
    delete query;
  }

  std::cout << "Total queries: " << qryTot << std::endl;
  std::cout << "Improved queries: " << qryImp << std::endl;
  std::cout << "Neutral queries: " << qryNeut << std::endl;
  std::cout << "Hurt queries: " << qryTot-(qryImp+qryNeut) << std::endl;

  result.close();

  delete stemmer;
  delete retMethod;
  delete judgStr;
  delete judgFile;
  delete qStream;
  delete index;
}


