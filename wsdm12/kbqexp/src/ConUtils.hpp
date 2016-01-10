/* -*- C++ -*- */

// Author: Alexander Kotov (UIUC), 2011
// $Id: ConUtils.hpp,v 1.6 2011/06/20 23:43:07 akotov2 Exp $

#ifndef CONUTILS_HPP_
#define CONUTILS_HPP_

#include "common_headers.hpp"
#include "StrUtils.hpp"
#include "BasicDocStream.hpp"
#include "PorterStemmer.hpp"
#include "SimpleKLRetMethod.hpp"
#include "TermGraph.hpp"
#include "ConceptGraph.hpp"

using namespace lemur::api;
using namespace lemur::retrieval;
using namespace lemur::parse;

typedef std::map<TERMID_T, TERM_T> IdTermMap;
typedef IdTermMap::iterator IdTermMapIt;
typedef IdTermMap::const_iterator cIdTermMap;

typedef struct TermIdPair_ {
  TermIdPair_() : term(""), termId(0)
  { }

  TermIdPair_(TERM_T& t, TERMID_T& tid) : term(t), termId(tid)
  { }

  TERM_T term;
  TERMID_T termId;
} TermIdPair;

typedef std::vector<TermIdPair> TermIdPairVec;

typedef std::map<std::string, TermIdPairVec> QryTermMap;
typedef QryTermMap::iterator QryTermMapIt;
typedef QryTermMap::const_iterator cQryTermMapIt;

typedef struct TermWeight_ {
  TermWeight_() : termId(0), weight(0)
  { }

  TermWeight_(TERMID_T tid, float w) : termId(tid), weight(w)
  { }

  TERMID_T termId;
  float weight;
} TermWeight;

typedef std::list<TermWeight> TermWeightList;
typedef TermWeightList::iterator TermWeightListIt;

typedef std::map<TERMID_T, float> TermWeightMap;
typedef TermWeightMap::iterator TermWeightMapIt;
typedef TermWeightMap::const_iterator cTermWeightMapIt;

typedef std::map<std::string, TermWeightMap> QryExpTerms;
typedef QryExpTerms::iterator QryExpTermsIt;
typedef QryExpTerms::const_iterator cQryExpTermsIt;

typedef std::map<TERMID_T, unsigned int> TermCntMap;
typedef TermCntMap::iterator TermCntMapIt;
typedef TermCntMap::const_iterator cTermCntMapIt;

typedef std::vector<std::string> StrVec;

float getTermIDF(Index* index, TERMID_T termId);

void printTermVec(Index *index, const IndexedRealVector *termVec);

bool inTermVec(TERMID_T termId, const TermIdPairVec &termVec);

void normQueryModel(SimpleKLQueryModel *queryModel);

void readQueriesUnstem(Index *index, PorterStemmer *stemmer, const std::string &qryFile, QryTermMap &queryMap);

void readQueries(Index *index, const std::string &qryFile, QryTermMap &queryMap);

TERMID_T stemAndGetId(Index *index, PorterStemmer *stemmer, const std::string& term);

TermWeightList* parseConPhrase(Index *index, PorterStemmer *stemmer, IdTermMap &termMap,
                               TERMID_T srcConId, std::string& conPhrase, double maxFrac);

TermWeightList* parseConPhraseIDF(Index *index, PorterStemmer *stemmer, IdTermMap &termMap,
                                  TERMID_T srcConId, std::string& conPhrase, double maxFrac);

bool readExpTermsFile(const std::string &fileName, Index *index, QryExpTerms &expTerms);

#endif /* CONUTILS_HPP_ */
