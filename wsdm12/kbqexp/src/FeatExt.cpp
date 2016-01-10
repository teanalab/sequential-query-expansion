/* -*- C++ -*- */

// Author: Alexander Kotov (UIUC), 2011
// $Id: FeatExt.cpp,v 1.11 2011/07/03 03:10:24 akotov2 Exp $

#include "common_headers.hpp"
#include "IndexManager.hpp"
#include "BasicDocStream.hpp"
#include "ResultFile.hpp"
#include "SimpleKLRetMethod.hpp"
#include "RetParamManager.hpp"
#include "StrUtils.hpp"
#include "ConUtils.hpp"
#include "ConceptGraph.hpp"
#include "ConceptGraphUtils.hpp"
#include "TermGraph.hpp"
#include "Features.hpp"
#include <math.h>
#include <limits>

using namespace lemur::api;
using namespace lemur::retrieval;
using namespace lemur::utility;
using namespace lemur::parse;

#define WEIGHT_THRESH_DEF 0.001
#define DIST_CONST_DEF 0.8
#define NUM_TOP_TERMS_DEF 100
#define MAX_DOC_RANK_DEF 10
#define EXP_RADIUS_DEF 1
#define NUM_SPLITS_DEF 5

typedef std::map<TERMID_T, unsigned int> IdCnt;
typedef IdCnt::iterator IdCntIt;
typedef IdCnt::const_iterator cIdCntIt;

typedef std::map<TERMID_T, float> WgtMap;
typedef WgtMap::iterator WgtMapIt;
typedef WgtMap::const_iterator cWgtMapIt;

typedef std::map<DOCID_T, IdCnt> FreqMap;
typedef FreqMap::iterator FreqMapIt;
typedef FreqMap::const_iterator cFreqMapIt;

typedef std::set<DOCID_T> DocSet;
typedef DocSet::iterator DocSetIt;
typedef DocSet::const_iterator cDocSetIt;

typedef std::map<TERMID_T, DocSet> TermDocsMap;
typedef TermDocsMap::iterator TermDocsMapIt;
typedef TermDocsMap::const_iterator cTermDocsMapIt;

typedef struct CandInfo_ {
  CandInfo_() : fanOut(0)
  { }
  // branching factor of a concept in ConceptNet
  unsigned int fanOut;
  // distances to query terms
  IdCnt distMap;
  // weights of paths to query terms
  WgtMap wgtMap;
} CandInfo;

typedef std::map<TERMID_T, CandInfo> CandMap;
typedef CandMap::iterator CandMapIt;
typedef CandMap::const_iterator cCandMapIt;

typedef struct SAInfo_ {
  SAInfo_() : score(0), rank(0)
  { }

  SAInfo_(float s, float r) : score(s), rank(r)
  { }

  float score;
  float rank;
} SAInfo;

typedef std::map<TERMID_T, SAInfo> SAInfoMap;
typedef SAInfoMap::iterator SAInfoMapIt;
typedef SAInfoMap::const_iterator cSAInfoMapIt;

typedef struct Split_ {
  Split_ () : trainFile(NULL), testFile(NULL)
  { }

  ~Split_() {
    if(trainFile != NULL) {
      if(trainFile->is_open()) {
        trainFile->close();
      }
      delete trainFile;
    }
    if(testFile != NULL) {
      if(testFile->is_open()) {
        testFile->close();
      }
      delete testFile;
    }
  }

  IdSet trainSet;
  IdSet testSet;
  ofstream *trainFile;
  ofstream *testFile;
} Split;

typedef std::vector<Split> SplitVec;

namespace LocalParameter {
  // file with query judgments
  std::string judgFile;
  // file with concept graphs for each query
  std::string conGraphFile;
  // file with collection term graph
  std::string colGraphFile;
  // file with candidates using Random Walk
  std::string randWalkFile;
  // file with candidates using Path Finding
  std::string pathFindFile;
  // directory to store files with splits for cross-validation
  std::string outDir;
  // threshold for edge weight in order for the edge to be included in the collection term graph
  double weightThresh;
  // distance constant for spreading activation
  double distConst;
  // maximum rank of the document to consider for counting occurrences of query terms
  int maxDocRank;
  // expansion radius
  int expRadius;
  // number of splits for cross-validation
  int numSplits;

  void get() {
    judgFile = ParamGetString("judgFile", "");
    colGraphFile = ParamGetString("colGraphFile", "");
    conGraphFile = ParamGetString("conGraphFile", "");
    pathFindFile = ParamGetString("pathFindFile", "");
    randWalkFile = ParamGetString("randWalkFile", "");
    outDir = ParamGetString("outDir", "");
    weightThresh = ParamGetDouble("weightThresh", WEIGHT_THRESH_DEF);
    distConst = ParamGetDouble("distConst", DIST_CONST_DEF);
    maxDocRank = ParamGetInt("maxDocRank", MAX_DOC_RANK_DEF);
    expRadius = ParamGetInt("expRadius", EXP_RADIUS_DEF);
    numSplits = ParamGetInt("numSplits", NUM_SPLITS_DEF);
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

void ZScoreNormalize(FeatMap &featMap) {
  unsigned int i, numFeats = 0;
  float numQryTermsMean = 0, numQryTermsSS = 0, numQryTermsSDev = 0, topDocScoreMean = 0, topDocScoreSS = 0, topDocScoreSDev = 0;
  float expTDocScoreMean = 0, expTDocScoreSS = 0, expTDocScoreSDev = 0;
  float topTermFracMean = 0, topTermFracSS = 0, topTermFracSDev = 0, numCanDocsMean = 0, numCanDocsSS = 0, numCanDocsSDev = 0;
  float avgCDocScoreMean = 0, avgCDocScoreSS = 0, avgCDocScoreSDev = 0, maxCDocScoreMean = 0, maxCDocScoreSS = 0, maxCDocScoreSDev = 0;
  float idfMean = 0, idfSS = 0, idfSDev = 0, fanOutMean = 0, fanOutSS = 0, fanOutSDev = 0;
  float spActScoreMean = 0, spActScoreSS = 0, spActScoreSDev = 0, spActRankMean = 0, spActRankSS = 0, spActRankSDev = 0;
  float rndWalkScoreMean = 0, rndWalkScoreSS = 0, rndWalkScoreSDev = 0, pathFindScoreMean = 0, pathFindScoreSS = 0, pathFindScoreSDev = 0;
  float avgColCorMean = 0, avgColCorSS = 0, avgColCorSDev = 0, maxColCorMean = 0, maxColCorSS = 0, maxColCorSDev = 0;
  float avgTopCorMean = 0, avgTopCorSS = 0, avgTopCorSDev = 0, maxTopCorMean = 0, maxTopCorSS = 0, maxTopCorSDev = 0;
  float avgTopPCorMean = 0, avgTopPCorSS = 0, avgTopPCorSDev = 0, maxTopPCorMean = 0, maxTopPCorSS = 0, maxTopPCorSDev = 0;
  float avgQDistMean = 0, avgQDistSS = 0, avgQDistSDev = 0, maxQDistMean = 0, maxQDistSS = 0, maxQDistSDev = 0;
  float avgPWeightMean = 0, avgPWeightSS = 0, avgPWeightSDev = 0, maxPWeightMean = 0, maxPWeightSS = 0, maxPWeightSDev = 0;

  for(FeatMapIt mit = featMap.begin(); mit != featMap.end(); mit++) {
    FeatVec &featVec = mit->second;
    numFeats += featVec.size();
    for(i = 0; i < featVec.size(); i++) {
      numQryTermsMean += featVec[i].numQryTerms;
      numQryTermsSS += pow(featVec[i].numQryTerms, 2);
      topDocScoreMean += featVec[i].topDocScore;
      topDocScoreSS += pow(featVec[i].topDocScore, 2);
      expTDocScoreMean += featVec[i].expTDocScore;
      expTDocScoreSS += pow(featVec[i].expTDocScore, 2);
      topTermFracMean += featVec[i].topTermFrac;
      topTermFracSS += pow(featVec[i].topTermFrac, 2);
      numCanDocsMean += featVec[i].numCanDocs;
      numCanDocsSS += pow(featVec[i].numCanDocs, 2);
      avgCDocScoreMean += featVec[i].avgCDocScore;
      avgCDocScoreSS += pow(featVec[i].avgCDocScore, 2);
      maxCDocScoreMean += featVec[i].maxCDocScore;
      maxCDocScoreSS += pow(featVec[i].maxCDocScore, 2);
      idfMean += featVec[i].idf;
      idfSS += pow(featVec[i].idf, 2);
      fanOutMean += featVec[i].fanOut;
      fanOutSS += pow(featVec[i].fanOut, 2);
      spActScoreMean += featVec[i].spActScore;
      spActScoreSS += pow(featVec[i].spActScore, 2);
      spActRankMean += featVec[i].spActRank;
      spActRankSS += pow(featVec[i].spActRank, 2);
      rndWalkScoreMean += featVec[i].rndWalkScore;
      rndWalkScoreSS += pow(featVec[i].rndWalkScore, 2);
      pathFindScoreMean += featVec[i].pathFindScore;
      pathFindScoreSS += pow(featVec[i].pathFindScore, 2);
      avgColCorMean += featVec[i].avgColCor;
      avgColCorSS += pow(featVec[i].avgColCor, 2);
      maxColCorMean += featVec[i].maxColCor;
      maxColCorSS += pow(featVec[i].maxColCor, 2);
      avgTopCorMean += featVec[i].avgTopCor;
      avgTopCorSS += pow(featVec[i].avgTopCor, 2);
      maxTopCorMean += featVec[i].maxTopCor;
      maxTopCorSS += pow(featVec[i].maxTopCor, 2);
      avgTopPCorMean += featVec[i].avgTopPCor;
      avgTopPCorSS += pow(featVec[i].avgTopPCor, 2);
      maxTopPCorMean += featVec[i].maxTopPCor;
      maxTopPCorSS += pow(featVec[i].maxTopPCor, 2);
      avgQDistMean += featVec[i].avgQDist;
      avgQDistSS += pow(featVec[i].avgQDist, 2);
      maxQDistMean += featVec[i].maxQDist;
      maxQDistSS += pow(featVec[i].maxQDist, 2);
      avgPWeightMean += featVec[i].avgPWeight;
      avgPWeightSS += pow(featVec[i].avgPWeight, 2);
      maxPWeightMean += featVec[i].maxPWeight;
      maxPWeightSS += pow(featVec[i].maxPWeight, 2);
    }
  }

  numQryTermsMean /= numFeats;
  numQryTermsSDev = sqrt(1/((float)(numFeats-1))*(numQryTermsSS-numFeats*pow(numQryTermsMean,2)));
  topDocScoreMean /= numFeats;
  topDocScoreSDev = sqrt(1/((float)(numFeats-1))*(topDocScoreSS-numFeats*pow(topDocScoreMean,2)));
  expTDocScoreMean /= numFeats;
  expTDocScoreSDev = sqrt(1/((float)(numFeats-1))*(expTDocScoreSS-numFeats*pow(expTDocScoreMean,2)));
  topTermFracMean /= numFeats;
  topTermFracSDev = sqrt(1/((float)(numFeats-1))*(topTermFracSS-numFeats*pow(topTermFracMean,2)));
  numCanDocsMean /= numFeats;
  numCanDocsSDev = sqrt(1/((float)(numFeats-1))*(numCanDocsSS-numFeats*pow(numCanDocsMean,2)));
  avgCDocScoreMean /= numFeats;
  avgCDocScoreSDev = sqrt(1/((float)(numFeats-1))*(avgCDocScoreSS-numFeats*pow(avgCDocScoreMean,2)));
  maxCDocScoreMean /= numFeats;
  maxCDocScoreSDev = sqrt(1/((float)(numFeats-1))*(maxCDocScoreSS-numFeats*pow(maxCDocScoreMean,2)));
  idfMean /= numFeats;
  idfSDev = sqrt(1/((float)(numFeats-1))*(idfSS-numFeats*pow(idfMean,2)));
  fanOutMean /= numFeats;
  fanOutSDev = sqrt(1/((float)(numFeats-1))*(fanOutSS-numFeats*pow(fanOutMean,2)));
  spActScoreMean /= numFeats;
  spActScoreSDev = sqrt(1/((float)(numFeats-1))*(spActScoreSS-numFeats*pow(spActScoreMean,2)));
  spActRankMean /= numFeats;
  spActRankSDev = sqrt(1/((float)(numFeats-1))*(spActRankSS-numFeats*pow(spActRankMean,2)));
  rndWalkScoreMean /= numFeats;
  rndWalkScoreSDev = sqrt(1/((float)(numFeats-1))*(rndWalkScoreSS-numFeats*pow(rndWalkScoreMean,2)));
  pathFindScoreMean /= numFeats;
  pathFindScoreSDev = sqrt(1/((float)(numFeats-1))*(pathFindScoreSS-numFeats*pow(pathFindScoreMean,2)));
  avgColCorMean /= numFeats;
  avgColCorSDev = sqrt(1/((float)(numFeats-1))*(avgColCorSS-numFeats*pow(avgColCorMean,2)));
  maxColCorMean /= numFeats;
  maxColCorSDev = sqrt(1/((float)(numFeats-1))*(maxColCorSS-numFeats*pow(maxColCorMean,2)));
  avgTopCorMean /= numFeats;
  avgTopCorSDev = sqrt(1/((float)(numFeats-1))*(avgTopCorSS-numFeats*pow(avgTopCorMean,2)));
  maxTopCorMean /= numFeats;
  maxTopCorSDev = sqrt(1/((float)(numFeats-1))*(maxTopCorSS-numFeats*pow(maxTopCorMean,2)));
  avgTopPCorMean /= numFeats;
  avgTopPCorSDev = sqrt(1/((float)(numFeats-1))*(avgTopPCorSS-numFeats*pow(avgTopPCorMean,2)));
  maxTopPCorMean /= numFeats;
  maxTopPCorSDev = sqrt(1/((float)(numFeats-1))*(maxTopPCorSS-numFeats*pow(maxTopPCorMean,2)));
  avgQDistMean /= numFeats;
  avgQDistSDev = sqrt(1/((float)(numFeats-1))*(avgQDistSS-numFeats*pow(avgQDistMean,2)));
  maxQDistMean /= numFeats;
  maxQDistSDev = sqrt(1/((float)(numFeats-1))*(maxQDistSS-numFeats*pow(maxQDistMean,2)));
  avgPWeightMean /= numFeats;
  avgPWeightSDev = sqrt(1/((float)(numFeats-1))*(avgPWeightSS-numFeats*pow(avgPWeightMean,2)));
  maxPWeightMean /= numFeats;
  maxPWeightSDev = sqrt(1/((float)(numFeats-1))*(maxPWeightSS-numFeats*pow(maxPWeightMean,2)));

  for(FeatMapIt mit = featMap.begin(); mit != featMap.end(); mit++) {
    FeatVec &featVec = mit->second;
    for(i = 0; i < featVec.size(); i++) {
      if(numQryTermsSDev != 0)
        featVec[i].numQryTerms = (featVec[i].numQryTerms-numQryTermsMean)/numQryTermsSDev;
      if(topDocScoreSDev != 0)
        featVec[i].topDocScore = (featVec[i].topDocScore-topDocScoreMean)/topDocScoreSDev;
      if(expTDocScoreSDev != 0)
        featVec[i].expTDocScore = (featVec[i].expTDocScore-expTDocScoreMean)/expTDocScoreSDev;
      if(topTermFracSDev != 0)
        featVec[i].topTermFrac = (featVec[i].topTermFrac-topTermFracMean)/topTermFracSDev;
      if(numCanDocsSDev != 0)
        featVec[i].numCanDocs = (featVec[i].numCanDocs-numCanDocsMean)/numCanDocsSDev;
      if(avgCDocScoreSDev != 0)
        featVec[i].avgCDocScore = (featVec[i].avgCDocScore-avgCDocScoreMean)/avgCDocScoreSDev;
      if(maxCDocScoreSDev != 0)
        featVec[i].maxCDocScore = (featVec[i].maxCDocScore-maxCDocScoreMean)/maxCDocScoreSDev;
      if(idfSDev != 0)
        featVec[i].idf = (featVec[i].idf-idfMean)/idfSDev;
      if(fanOutSDev != 0)
        featVec[i].fanOut = (featVec[i].fanOut-fanOutMean)/fanOutSDev;
      if(spActScoreSDev != 0)
        featVec[i].spActScore = (featVec[i].spActScore-spActScoreMean)/spActScoreSDev;
      if(spActRankSDev != 0)
        featVec[i].spActRank = (featVec[i].spActRank-spActRankMean)/spActRankSDev;
      if(rndWalkScoreSDev != 0)
        featVec[i].rndWalkScore = (featVec[i].rndWalkScore-rndWalkScoreMean)/rndWalkScoreSDev;
      if(pathFindScoreSDev != 0)
        featVec[i].pathFindScore = (featVec[i].pathFindScore-pathFindScoreMean)/pathFindScoreSDev;
      if(avgColCorSDev != 0)
        featVec[i].avgColCor = (featVec[i].avgColCor-avgColCorMean)/avgColCorSDev;
      if(maxColCorSDev != 0)
        featVec[i].maxColCor = (featVec[i].maxColCor-maxColCorMean)/maxColCorSDev;
      if(avgTopCorSDev != 0)
        featVec[i].avgTopCor = (featVec[i].avgTopCor-avgTopCorMean)/avgTopCorSDev;
      if(maxTopCorSDev != 0)
        featVec[i].maxTopCor = (featVec[i].maxTopCor-maxTopCorMean)/maxTopCorSDev;
      if(avgTopPCorSDev != 0)
        featVec[i].avgTopPCor = (featVec[i].avgTopPCor-avgTopPCorMean)/avgTopPCorSDev;
      if(maxTopPCorSDev != 0)
        featVec[i].maxTopPCor = (featVec[i].maxTopPCor-maxTopPCorMean)/maxTopPCorSDev;
      if(avgQDistSDev != 0)
        featVec[i].avgQDist = (featVec[i].avgQDist-avgQDistMean)/avgQDistSDev;
      if(maxQDistSDev != 0)
        featVec[i].maxQDist = (featVec[i].maxQDist-maxQDistMean)/maxQDistSDev;
      if(avgPWeightSDev != 0)
        featVec[i].avgPWeight = (featVec[i].avgPWeight-avgPWeightMean)/avgPWeightSDev;
      if(maxPWeightSDev != 0)
        featVec[i].maxPWeight = (featVec[i].maxPWeight-maxPWeightMean)/maxPWeightSDev;
    }
  }
}

void createSplits(SplitVec &splits, const QryTermMap& qryTerms, int numSplits) {
  unsigned int curQry, curSplit, qryCnt = qryTerms.size();
  unsigned int splitSize = qryCnt / numSplits;
  unsigned int splitBounds[numSplits];

  for(unsigned int i = 1; i <= numSplits; i++) {
    splits.push_back(Split());
    if(i == numSplits) {
      splitBounds[numSplits-1] = qryCnt;
    } else {
      splitBounds[i-1] = i*splitSize;
    }
  }

  for(unsigned int i = 0; i < numSplits; i++) {
    curQry = 0;
    curSplit = 0;
    for(cQryTermMapIt it = qryTerms.begin(); it != qryTerms.end(); it++) {
      if(i == curSplit) {
        splits[i].testSet.insert(atoi(it->first.c_str()));
      } else {
        splits[i].trainSet.insert(atoi(it->first.c_str()));
      }

      if(++curQry == splitBounds[curSplit]) {
        curSplit++;
      }
    }
  }
}

void writeFeatVec(ofstream &outFile, unsigned int qryId, const FeatVec &featVec) {
  for(unsigned int i = 0; i < featVec.size(); i++) {
    outFile << qryId << "\t" << featVec[i].expCon << "\t" << featVec[i].numQryTerms << "\t" << featVec[i].topDocScore << "\t" \
            << featVec[i].expTDocScore << "\t" << featVec[i].topTermFrac << "\t" << featVec[i].numCanDocs << "\t" \
            << featVec[i].avgCDocScore << "\t" << featVec[i].maxCDocScore << "\t" << featVec[i].idf << "\t" \
            << featVec[i].fanOut << "\t" << featVec[i].spActScore << "\t" << featVec[i].spActRank << "\t" \
            << featVec[i].rndWalkScore << "\t" << featVec[i].pathFindScore << "\t" << featVec[i].avgColCor << "\t" \
            << featVec[i].maxColCor << "\t" << featVec[i].avgTopCor << "\t" << featVec[i].maxTopCor << "\t" \
            << featVec[i].avgTopPCor << "\t" << featVec[i].maxTopPCor << "\t" << featVec[i].avgQDist << "\t" \
            << featVec[i].maxQDist << "\t" << featVec[i].avgPWeight << "\t" << featVec[i].maxPWeight << "\t" \
            << featVec[i].map << std::endl;
  }
}

void storeFeatMap(const FeatMap &featMap, SplitVec &splits, const std::string &outDir, int numSplits) {
  IdSetIt sit;

  for(unsigned int i = 0; i < splits.size(); i++) {
    char num[9];
    snprintf(num, 8, "%u", i+1);
    std::string testPath = outDir + "/test." + num;
    std::string trainPath = outDir + "/train." + num;
    splits[i].testFile = new ofstream();
    splits[i].testFile->open(testPath.c_str());
    splits[i].trainFile = new ofstream();
    splits[i].trainFile->open(trainPath.c_str());
  }

  for(cFeatMapIt mit = featMap.begin(); mit != featMap.end(); mit++) {
    const FeatVec &featVec = mit->second;
    for(unsigned int i = 0; i < splits.size(); i++) {
      if((sit = splits[i].testSet.find(mit->first)) != splits[i].testSet.end()) {
        writeFeatVec(*splits[i].testFile, mit->first, featVec);
      } else {
        writeFeatVec(*splits[i].trainFile, mit->first, featVec);
      }
    }
  }
}

void getExpCands(CandMap &candMap, ConceptGraph *qryGraph, const TermIdPairVec &qryTerms, int expRadius) {
  CandMapIt mit;
  ConInfoSet *context;
  for(int i = 0; i < qryTerms.size(); i++) {
    context = qryGraph->getContextChain(qryTerms[i].termId, expRadius);
    for(ConInfoSetIt cit = context->begin(); cit != context->end(); cit++) {
      if(!inTermVec(cit->id, qryTerms)) {
        if((mit = candMap.find(cit->id)) != candMap.end()) {
          mit->second.distMap[qryTerms[i].termId] = cit->dist;
          mit->second.wgtMap[qryTerms[i].termId] = cit->weight;
        } else {
          Concept* con;
          CandInfo ci;
          qryGraph->getConcept(cit->id, &con);
          ci.fanOut = con->getNumRels();
          ci.distMap[qryTerms[i].termId] = cit->dist;
          ci.wgtMap[qryTerms[i].termId] = cit->weight;
          candMap[cit->id] = ci;
        }
      }
    }
    delete context;
  }
}

float topDocTermFrac(const TermCntMap& termCnts, COUNT_T sumCnts, TERMID_T termID) {
  cTermCntMapIt it;
  float termCnt = 0;
  if((it = termCnts.find(termID)) != termCnts.end()) {
    termCnt = it->second;
  }
  return termCnt / sumCnts;
}

float topDocCor(TERMID_T canId, TERMID_T qId1, TERMID_T qId2, const FreqMap &docTermCnts, const TermCntMap& termCnts) {
  cTermCntMapIt tcIt;
  cFreqMapIt tdIt;
  IdCntIt frIt;
  unsigned int canCnt, coCnt = 0;

  if(qId2 != 0) {
    if((tcIt = termCnts.find(canId)) != termCnts.end() && termCnts.find(qId1) != termCnts.end() && termCnts.find(qId2) != termCnts.end()) {
      canCnt = tcIt->second;
    } else {
      return 0;
    }
  } else {
    if((tcIt = termCnts.find(canId)) != termCnts.end() && termCnts.find(qId1) != termCnts.end()) {
      canCnt = tcIt->second;
    } else {
      return 0;
    }
  }

  for(tdIt = docTermCnts.begin(); tdIt != docTermCnts.end(); tdIt++) {
    if(qId2 == 0) {
      if(tdIt->second.find(canId) != tdIt->second.end() && tdIt->second.find(qId1) != tdIt->second.end()) {
        coCnt++;
      }
    } else {
      if(tdIt->second.find(canId) != tdIt->second.end() && tdIt->second.find(qId1) != tdIt->second.end() &&
         tdIt->second.find(qId2) != tdIt->second.end()) {
        coCnt++;
      }
    }
  }

  return ((float) coCnt) / canCnt;
}

void getCandTopDocStats(TERMID_T canId, IndexedRealVector &retResults, const TermDocsMap &termDocsMap,
                        float *numCanDocs, float *avgCDocScore, float *maxCDocScore) {
  cTermDocsMapIt mit;
  cDocSetIt dit;
  *numCanDocs = 0;
  *avgCDocScore = 0;
  *maxCDocScore = 0;

  if((mit = termDocsMap.find(canId)) != termDocsMap.end()) {
    *numCanDocs = mit->second.size();
    *maxCDocScore = -std::numeric_limits<float>::max();
    for(dit = mit->second.begin(); dit != mit->second.end(); dit++) {
      *avgCDocScore += retResults.FindByIndex(*dit)->val;
      if(retResults.FindByIndex(*dit)->val > *maxCDocScore) {
        *maxCDocScore = retResults.FindByIndex(*dit)->val;
      }
    }
    *avgCDocScore /= *numCanDocs;
  }
}

void getTermDocsMap(Index *index, IndexedRealVector &retResults, TermDocsMap &termDocsMap) {
  TermDocsMapIt it;
  COUNT_T maxRank;
  TermInfoList *tiList = NULL;
  TermInfo *termInfo = NULL;

  if(LocalParameter::maxDocRank != 0) {
    maxRank = retResults.size() < LocalParameter::maxDocRank ? retResults.size() : LocalParameter::maxDocRank;
  } else {
    maxRank = retResults.size();
  }

  for(int i = 0; i < maxRank; i++) {
    tiList = index->termInfoList(retResults[i].ind);
    tiList->startIteration();
    while(tiList->hasMore()) {
      termInfo = tiList->nextEntry();
      if((it = termDocsMap.find(termInfo->termID())) != termDocsMap.end()) {
        it->second.insert(retResults[i].ind);
      } else {
        DocSet docSet;
        docSet.insert(retResults[i].ind);
        termDocsMap[termInfo->termID()] = docSet;
      }
    }
    delete tiList;
  }
}

/*
 * Return the total number of terms and the frequencies of terms in the top documents as well as a map from documents to
 * the counts of all the terms in them
 */

COUNT_T getTopDocTermStats(Index *index, IndexedRealVector &retResults, TermCntMap &termCntMap, FreqMap &docTermCntMap) {
  TermCntMapIt it;
  COUNT_T maxRank, sumCnts = 0;
  TermInfoList *tiList = NULL;
  TermInfo *termInfo = NULL;

  if(LocalParameter::maxDocRank != 0) {
    maxRank = retResults.size() < LocalParameter::maxDocRank ? retResults.size() : LocalParameter::maxDocRank;
  } else {
    maxRank = retResults.size();
  }

  for(int i = 0; i < maxRank; i++) {
    IdCnt docTermCnts;
    tiList = index->termInfoList(retResults[i].ind);
    tiList->startIteration();
    while(tiList->hasMore()) {
      termInfo = tiList->nextEntry();
      docTermCnts[termInfo->termID()] = termInfo->count();
      if((it = termCntMap.find(termInfo->termID())) != termCntMap.end()) {
        it->second += termInfo->count();
      } else {
        termCntMap[termInfo->termID()] = termInfo->count();
      }
      sumCnts += termInfo->count();
    }
    docTermCntMap[retResults[i].ind] = docTermCnts;
    delete tiList;
  }

  return sumCnts;
}

void getCandSAInfo(ConceptGraph *qryGraph, const TermIdPairVec &qryTermInfo, SAInfoMap& infoMap)  {
  TermIdSet qryTerms;
  TermIdSetIt sit;
  TermWeightMapIt mit;
  IndexedRealVector sortCands;

  for(unsigned int i = 0; i < qryTermInfo.size(); i++) {
    qryTerms.insert(qryTermInfo[i].termId);
  }

  TermWeightMap *spActWeights = qryGraph->getSpActWeights(qryTerms, LocalParameter::distConst);

  for(TermWeightMapIt mit = spActWeights->begin(); mit != spActWeights->end(); mit++) {
    sortCands.PushValue(mit->first, mit->second);
  }
  delete spActWeights;
  sortCands.Sort();

  for(unsigned int i = 0; i < sortCands.size(); i++) {
    infoMap[sortCands[i].ind] = SAInfo(sortCands[i].val, i);
  }
}

void getQryCandFeats(Index *index, TermGraph *termGraph, Features &feats, TERMID_T canId, CandInfo &candInfo,
                  const IndexedRealVector &origQryRes, const TermIdPairVec &qryTerms, const FreqMap &docTermCnts,
                  const TermCntMap &termCnts) {
  float colCor, topCor, topPCor, qDist;

  for(int i = 0; i < qryTerms.size(); i++) {
    if(termGraph->relExists(canId, qryTerms[i].termId, &colCor) || termGraph->relExists(qryTerms[i].termId, canId, &colCor)) {
      feats.avgColCor += colCor;
      if(colCor > feats.maxColCor) {
        feats.maxColCor = colCor;
      }
    }
    topCor = topDocCor(canId, qryTerms[i].termId, 0, docTermCnts, termCnts);
    feats.avgTopCor += topCor;
    if(topCor > feats.maxTopCor) {
      feats.maxTopCor = topCor;
    }
    feats.avgQDist += candInfo.distMap[qryTerms[i].termId];
    if(candInfo.distMap[qryTerms[i].termId] > feats.maxQDist) {
      feats.maxQDist = candInfo.distMap[qryTerms[i].termId];
    }
    feats.avgPWeight += candInfo.wgtMap[qryTerms[i].termId];
    if(candInfo.wgtMap[qryTerms[i].termId] > feats.maxPWeight) {
      feats.maxPWeight = candInfo.wgtMap[qryTerms[i].termId];
    }
    for(int j = i+1; j < qryTerms.size(); j++) {
      topPCor = topDocCor(canId, qryTerms[i].termId, qryTerms[j].termId, docTermCnts, termCnts);
      feats.avgTopPCor += topPCor;
      if(topPCor > feats.maxTopPCor) {
        feats.maxTopPCor = topPCor;
      }
    }
  }

  feats.avgColCor /= qryTerms.size();
  feats.avgTopCor /= qryTerms.size();
  feats.avgQDist /= qryTerms.size();
  feats.avgPWeight /= qryTerms.size();
  if(qryTerms.size() > 1) {
    feats.avgTopPCor /= qryTerms.size()*(qryTerms.size()-1)/2;
  }
}

void processQuery(Index* index, ConceptGraph *qryGraph, TermGraph *termGraph, SimpleKLRetMethod *retMethod,
                  SimpleKLQueryModel *origQryModel, IndexedRealVector *qryJudg, unsigned int qryId,
                  const TermIdPairVec &qryTerms, const TermWeightMap &walkTerms, const TermWeightMap &pathTerms,
                  FeatMap &featMap, int expRadius) {
  FeatVec featVec;
  CandMap candMap;
  SAInfoMap saInfoMap;
  FreqMap docTermCnts;
  TermCntMap termCnts;
  TermDocsMap termDocsMap;
  cTermWeightMapIt etIt;
  SAInfoMapIt saIt;
  COUNT_T sumCnts;

  IndexedRealVector origQryRes(index->docCount());
  retMethod->scoreCollection(*origQryModel, origQryRes);
  origQryRes.Sort();
  sumCnts = getTopDocTermStats(index, origQryRes, termCnts, docTermCnts);
  // DEBUG
  cout << "  Term-document map...";
  cout.flush();
  getTermDocsMap(index, origQryRes, termDocsMap);
  cout << "OK" << endl;
  cout << "  Expansion candidates...";
  cout.flush();
  getExpCands(candMap, qryGraph, qryTerms, expRadius);
  cout << "OK" << endl;
  cout << "  Spreading activation...";
  cout.flush();
  getCandSAInfo(qryGraph, qryTerms, saInfoMap);
  cout << "OK" << endl;

  cout << "  Processing candidates...";
  cout.flush();
  for(CandMapIt it = candMap.begin(); it != candMap.end(); it++) {
    Features feats;
    TermDocsMap termDocsMap;
    feats.numQryTerms = qryTerms.size();
    feats.topDocScore = origQryRes[0].val;
    getTermDocsMap(index, origQryRes, termDocsMap);
    IndexedRealVector expQryRes(index->docCount());
    SimpleKLQueryModel expQryModel(*index);
    origQryModel->startIteration();
    while(origQryModel->hasMore()) {
      QueryTerm *qt = origQryModel->nextTerm();
      expQryModel.setCount(qt->id(), qt->weight());
      delete qt;
    }
    expQryModel.incCount(it->first, 1);
    retMethod->scoreCollection(expQryModel, expQryRes);
    expQryRes.Sort();
    feats.expTDocScore = expQryRes[0].val;
    feats.topTermFrac = topDocTermFrac(termCnts, sumCnts, it->first);
    getCandTopDocStats(it->first, origQryRes, termDocsMap, &feats.numCanDocs, &feats.avgCDocScore, &feats.maxCDocScore);
    // DEBUG
    // cout << "Candidate: " << index->term(it->first) << endl;
    // cout << "Number of documents: " << feats.numCanDocs << endl;
    // cout << "Avg. cdoc score: " << feats.avgCDocScore << endl;
    // cout << "Max. cdoc score: " << feats.maxCDocScore << endl;
    feats.idf = getTermIDF(index, it->first);
    feats.fanOut = it->second.fanOut;
    feats.spActScore = (saIt = saInfoMap.find(it->first)) != saInfoMap.end() ? saIt->second.score : 0;
    feats.spActRank = (saIt = saInfoMap.find(it->first)) != saInfoMap.end() ? saIt->second.rank : 0;
    // DEBUG
    // cout << "SA score: " << feats.spActScore << endl;
    // cout << "SA rank: " << feats.spActRank << endl;
    feats.rndWalkScore = (etIt = walkTerms.find(it->first)) != walkTerms.end() ? etIt->second : 0;
    feats.pathFindScore = (etIt = pathTerms.find(it->first)) != pathTerms.end() ? etIt->second : 0;
    feats.map = queryMAP(expQryRes, qryJudg);
    feats.expCon = index->term(it->first);
    getQryCandFeats(index, termGraph, feats, it->first, it->second, origQryRes, qryTerms, docTermCnts, termCnts);
    featVec.push_back(feats);
  }
  cout << "OK" << endl;

  featMap[qryId] = featVec;
}

int AppMain(int argc, char *argv[]) {
  unsigned int qryId;
  Index *index = NULL;
  TextQuery *query = NULL;
  ifstream *judgStr = NULL;
  ResultFile *judgFile = NULL;
  DocStream *qStream = NULL;
  SimpleKLQueryModel *qryModel = NULL;
  SimpleKLRetMethod *retMethod = NULL;
  TermGraph *termGraph = NULL;
  ConceptGraph *qryGraph = NULL;
  IndexedRealVector *qryJudg = NULL;
  QryGraphMap qryGraphMap;
  QryTermMap qryTermMap;
  QryExpTerms walkTermMap;
  QryExpTerms pathTermMap;
  StrFloatMap relWeights;
  FeatMap featMap;
  SplitVec splits;

  if(!RetrievalParameter::databaseIndex.length()) {
    throw lemur::api::Exception("FeatExt", "Path to the index key file is not specified (parameter index)");
  }

  if(!RetrievalParameter::textQuerySet.length()) {
    throw lemur::api::Exception("FeatExt", "File with query topics is not specified in the configuration file (parameter textQuery)");
  }

  if(!LocalParameter::judgFile.length()) {
    throw lemur::api::Exception("FeatExt", "File with topic judgments is not specified in the configuration file (parameter judgmentsFile)");
  }

  if(!LocalParameter::colGraphFile.length()) {
    throw lemur::api::Exception("FeatExt", "File with collection term graph is not specified (parameter colGraphFile)");
  }

  if(!LocalParameter::conGraphFile.length()) {
    throw lemur::api::Exception("FeatExt", "File with query concept graph is not specified (parameter conGraphFile)");
  }

  if(!LocalParameter::randWalkFile.length()) {
    throw lemur::api::Exception("FeatExt", "File with expansion candidates generated using random walk algorithm is not specified (parameter randWalkFile)");
  }

  if(!LocalParameter::pathFindFile.length()) {
    throw lemur::api::Exception("FeatExt", "File with expansion candidates generated using query path algorithm is not specified (parameter pathFindFile)");
  }

  if(!LocalParameter::outDir.length()) {
    throw lemur::api::Exception("FeatExt", "Directory to store split files is not specified (parameter outDir)");
  }

  try {
    index = IndexManager::openIndex(RetrievalParameter::databaseIndex);
  } catch (lemur::api::Exception &ex) {
    ex.writeMessage(std::cerr);
    throw lemur::api::Exception("FeatExt", "Can't open index. Check paramter index in the configuration file.");
  }

  judgStr = new ifstream(LocalParameter::judgFile.c_str(), ios::in | ios::binary);

  if(judgStr->fail()) {
    throw lemur::api::Exception("FeatExt", "can't open the file with query judgments");
  }

  judgFile = new ResultFile(false);
  judgFile->load(*judgStr, *index);

  readQueries(index, RetrievalParameter::textQuerySet, qryTermMap);

  cout << "Reading file with random walk candidates...";
  cout.flush();
  if(!readExpTermsFile(LocalParameter::randWalkFile, index, walkTermMap)) {
    throw lemur::api::Exception("FeatExt", "can't read file with random walk candidates");
  }
  cout << "OK" << endl;

  cout << "Reading file with path finding candidates...";
  cout.flush();
  if(!readExpTermsFile(LocalParameter::pathFindFile, index, pathTermMap)) {
    throw lemur::api::Exception("FeatExt", "can't read file with path finding candidates");
  }
  cout << "OK" << endl;

  termGraph = new TermGraph(index);

  try {
    cout << "Loading term graph...";
    cout.flush();
    termGraph->loadFile(LocalParameter::colGraphFile, LocalParameter::weightThresh);
  } catch (lemur::api::Exception &ex) {
    ex.writeMessage(cerr);
    throw lemur::api::Exception("FeatExt", "Problem loading term graph file. Check the parameter graphFile in the configuration file.");
  }

  cout << "OK" << endl;

  initRelWeights(relWeights);

  cout << "Loading concept graphs for queries...";
  cout.flush();
  if(!loadQryConGraph(LocalParameter::conGraphFile, index, termGraph, qryGraphMap)) {
    std::cout << "Failed" << std::endl;
  } else {
    std::cout << "OK" << std::endl;
  }

  lemur::retrieval::ArrayAccumulator accumulator(index->docCount());
  retMethod = new SimpleKLRetMethod(*index, SimpleKLParameter::smoothSupportFile, accumulator);
  retMethod->setDocSmoothParam(SimpleKLParameter::docPrm);
  retMethod->setQueryModelParam(SimpleKLParameter::qryPrm);

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
    const TermWeightMap &walkTerms = walkTermMap[query->id()];
    const TermWeightMap &pathTerms = pathTermMap[query->id()];

    if(!judgFile->findResult(query->id(), qryJudg)) {
      std::cerr << "Can't find result judgments for query " << query->id() << std::endl;
      delete query;
      continue;
    }

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
    cout << ")..." << endl;
    cout.flush();

    qryGraph = qryGraphMap[query->id()];
    cout << "  Normalizing weights...";
    normRelWeights(qryGraph, index, relWeights);
    cout << "OK" << endl;

    TermIdPairVec& qryTerms = qryTermMap[query->id()];

    qryModel = dynamic_cast<SimpleKLQueryModel*>(retMethod->computeTextQueryRep(*query));
    processQuery(index, qryGraph, termGraph, retMethod, qryModel, qryJudg, qryId, qryTerms,
                 walkTerms, pathTerms, featMap, LocalParameter::expRadius);
    cout << "Done with query!" << endl;

    delete qryModel;
    delete qryJudg;
    delete qryGraph;
    delete query;
  }

  ZScoreNormalize(featMap);
  createSplits(splits, qryTermMap, LocalParameter::numSplits);
  storeFeatMap(featMap, splits, LocalParameter::outDir, LocalParameter::numSplits);

  delete judgStr;
  delete judgFile;
  delete qStream;
  delete termGraph;
  delete index;
}
