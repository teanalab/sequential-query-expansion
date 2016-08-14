/* -*- C++ -*- */

// Author: Alexander Kotov (UIUC), 2011
// $Id: ConceptGraphUtils.hpp,v 1.9 2011/05/17 01:50:05 akotov2 Exp $

#ifndef CONCEPTGRAPHUTILS_HPP_
#define CONCEPTGRAPHUTILS_HPP_

#include "common_headers.hpp"
#include "PorterStemmer.hpp"
#include "ConUtils.hpp"
#include "TermGraph.hpp"
#include "ConceptGraph.hpp"
#include <queue>

using namespace lemur::api;
using namespace lemur::utility;
using namespace lemur::parse;

typedef std::map<std::string, ConceptGraph*> QryGraphMap;
typedef QryGraphMap::iterator QryGraphMapIt;
typedef QryGraphMap::const_iterator cQryGraphMapIt;

typedef std::map<std::string, float> StrFloatMap;
typedef StrFloatMap::iterator StrFloatMapIt;
typedef StrFloatMap::const_iterator cStrFloatMapIt;

typedef std::map<std::string, unsigned int> StrUIntMap;
typedef StrUIntMap::iterator StrUIntMapIt;
typedef StrUIntMap::const_iterator cStrUIntMapIt;

typedef std::map<unsigned int, float> UIntFloatMap;
typedef UIntFloatMap::iterator UIntFloatMapIt;

struct FloatRank {
  bool operator()(float v1, float v2) const {
    return v2 < v1;
  }
};

typedef std::set<float, FloatRank> FloatSet;
typedef FloatSet::iterator FloatSetIt;

void initRelWeights(StrFloatMap &relWeights);

void initRelGroups(StrUIntMap &relGroups);

bool loadQryConGraph(const std::string& fileName, Index *index, TermGraph *termGraph, QryGraphMap &qryGraphMap);

void addTermGraphRG(ConceptGraph *graph, Index *index, TermGraph *termGraph, TERMID_T srcTermId, int maxDist, int maxCont);

void normRelWeights(ConceptGraph *graph, Index *index, const StrFloatMap& relWeights);

void printChain(Index *index, const ConChain& path);

void printGraph(ConceptGraph *graph, Index *index);

#endif /* CONCEPTGRAPHUTILS_HPP_ */
