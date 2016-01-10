/* -*- C++ -*- */

// Author: Alexander Kotov (UIUC), 2011
// $Id: ConceptGraph.hpp,v 1.15 2011/06/20 23:43:07 akotov2 Exp $

#ifndef CONCEPTGRAPH_HPP_
#define CONCEPTGRAPH_HPP_

#include "IndexTypes.hpp"
#include "SparseMatrix.hpp"
#include "StrUtils.hpp"
#include <map>
#include <set>
#include <queue>
#include <list>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <math.h>

using namespace lemur::api;

typedef struct Relation_ {
  Relation_() : type(""), weight(0)
  { }

  Relation_(const std::string& t, float w) : type(t), weight(w)
  { }
  std::string type;
  float weight;
} Relation;

class Concept {
protected:
  typedef std::map<TERMID_T, Relation> RelMap;
  typedef RelMap::iterator RelMapIt;
  typedef RelMap::const_iterator cRelMapIt;
public:
  Concept()
  { }

  ~Concept()
  { }

  Relation& operator[](const TERMID_T tid) {
    return _relMap[tid];
  }

  void addRelation(const TERMID_T trgTermId, const Relation& rel) {
    _relMap[trgTermId] = rel;
  }

  void delRelation(const TERMID_T trgTermId) {
    RelMapIt it;
    if((it = _relMap.find(trgTermId)) != _relMap.end()) {
      _relMap.erase(it);
    }
  }

  bool hasRelation(const TERMID_T trgTermId) const {
    cRelMapIt it;
    return((it = _relMap.find(trgTermId)) != _relMap.end());
  }

  bool getRelation(const TERMID_T trgTermId, Relation** rel) {
    RelMapIt it;
    if((it = _relMap.find(trgTermId)) != _relMap.end()) {
      if(rel != NULL) {
        *rel = &it->second;
      }
      return true;
    } else {
      if(rel != NULL) {
        *rel = NULL;
      }
      return false;
    }
  }

  inline size_t getNumRels() const {
    return _relMap.size();
  }

  class RelIterator;

protected:
  RelMap _relMap;
};

class Concept::RelIterator {
public:
  RelIterator(Concept* c) : _conc(c)
  { }

  void startIteration() {
    _it = _conc->_relMap.begin();
  }

  bool hasMore() {
    return _it != _conc->_relMap.end();
  }

  void getNext(TERMID_T& termId, Relation** rel) {
    if(hasMore()) {
      termId = _it->first;
      if(rel != NULL) {
        *rel = &_it->second;
      }
      _it++;
    }
  }

protected:
  Concept* _conc;
  RelMapIt _it;
};

typedef struct ConInfo_ {
  ConInfo_() : id(0), dist(0), weight(0)
  { }

  ConInfo_(const TERMID_T conId, unsigned int conDist, float conWeight) : id(conId), dist(conDist), weight(conWeight)
  { }

  // ID of the concept
  TERMID_T id;
  // distance of the concept
  unsigned int dist;
  // weight of the concept
  float weight;
} ConInfo;

struct ConInfoCmp {
  bool operator()(const ConInfo &conInfo1, const ConInfo &conInfo2) const {
    return conInfo1.weight >= conInfo2.weight;
  }
};

typedef std::set<ConInfo, ConInfoCmp> ConInfoSet;
typedef ConInfoSet::iterator ConInfoSetIt;
typedef ConInfoSet::const_iterator cConInfoSetIt;

typedef struct RelInfo_ {
  RelInfo_() : id(0), type(""), weight(0)
  { }

  RelInfo_(const TERMID_T conId, const std::string& relType, float relWeight) : id(conId), type(relType), weight(relWeight)
  { }

  // ID of the target concept term
  TERMID_T id;
  // type of relation with the previous concept
  std::string type;
  // weight of relation
  float weight;
} RelInfo;

typedef struct RelInfoExt_ {
  RelInfoExt_() : id(0), type(""), weight(0), idf(0)
  { }

  RelInfoExt_(const TERMID_T conId, const std::string& relType, float relWeight, float conIDF) : id(conId), type(relType), weight(relWeight), idf(conIDF)
  { }

  // ID of the target concept term
  TERMID_T id;
  // type of relation with the previous concept
  std::string type;
  // weight of relation
  float weight;
  // IDF of the target concept term
  float idf;
} RelInfoExt;

struct RelInfoCmp {
  bool operator()(const RelInfoExt &relInfo1, const RelInfoExt &relInfo2) const {
    return relInfo1.idf >= relInfo2.idf;
  }
};

typedef std::set<RelInfoExt, RelInfoCmp> SortRelSet;
typedef SortRelSet::iterator SortRelSetIt;
typedef SortRelSet::const_iterator cSortRelSetIt;

typedef std::list<RelInfo> ConChain;
typedef ConChain::iterator ConChainIt;
typedef ConChain::const_iterator cConChainIt;

typedef std::list<ConChain> ConChainList;
typedef ConChainList::iterator ConChainListIt;

typedef std::map<TERMID_T, float> TermWeightMap;
typedef TermWeightMap::iterator TermWeightMapIt;
typedef TermWeightMap::const_iterator cTermWeightMapIt;

typedef std::set<TERMID_T> TermIdSet;
typedef TermIdSet::iterator TermIdSetIt;
typedef TermIdSet::const_iterator cTermIdSetIt;

class ConceptGraph {
public:
  // DEBUG
  ConceptGraph()
  { }

  ~ConceptGraph()
  { }

  void addConcept(TERMID_T conTermId) {
    _conMap.insert(std::make_pair(conTermId, Concept()));
  }

  void delConcept(TERMID_T conTermId);

  bool hasConcept(TERMID_T conTermId) const {
    cConceptMapIt it;
    return ((it = _conMap.find(conTermId)) != _conMap.end());
  }

  bool getConcept(TERMID_T conTermId, Concept** con);

  Concept& operator[](const TERMID_T tid) {
    return _conMap[tid];
  }

  size_t getNumConcepts() const {
    return _conMap.size();
  }

  // get the adjacency matrix for the graph (returns dimensionality)
  unsigned int getAdjMatD(float** &mat, TERMID_T* &termIDs);
  void getAdjMatSp(SparseMatrix<float>* &mat);

  // get the random walk matrix after specified number of steps (returns dimensionality)
  unsigned int getFinRandWalkMat(float** &mat, TERMID_T* &termIDs, float alpha, int numSteps);
  void getFinRandWalkMat(SparseMatrix<float>* &mat, float alpha, int numSteps);

  // get the random walk matrix after infinite number of steps (returns dimensionality)
  unsigned int getInfRandWalkMat(float** &mat, TERMID_T* &termIDs, float alpha);

  bool hasRelation(const TERMID_T termId1, const TERMID_T termId2) const;

  void addRelation(const TERMID_T termId1, const TERMID_T termId2, const std::string& type, double weight);

  void updateRelation(const TERMID_T termId1, const TERMID_T termId2, const std::string& type, double weight);

  bool getRelation(const TERMID_T termId1, const TERMID_T termId2, Relation** rel);

  TermWeightMap* getSpActWeights(const TermIdSet &qryTermIds, float distConst=0.8);

  // concepts in the context are scored by chaining the weights of relations along the paths
  ConInfoSet* getContextChain(const TERMID_T qryTermId, unsigned int maxDist);

  // concepts in the context are scored by discounting the weights of relations based on the distance
  ConInfoSet* getContextDist(const TERMID_T qryTermId, unsigned int maxDist);

  ConChainList* getPaths(const TERMID_T srcTermId, const TERMID_T trgTermId, unsigned int maxDist);

  bool loadGraph(const std::string& fileName);

  bool saveGraph(const std::string& fileName);

  void printGraph();

  void printChain(const ConChain& conChain) const;

  class ConIterator;

  void getMatCpySp(const SparseMatrix<float>* orig, SparseMatrix<float>* &copy);
  bool getMatInvSp(const SparseMatrix<float> &orig, SparseMatrix<float> &res);
  void getMatProdSp(const SparseMatrix<float>* mat1, const SparseMatrix<float>* mat2, SparseMatrix<float>* &prod);
  void getMatPowSp(const SparseMatrix<float>* orig, SparseMatrix<float>* &res, unsigned int pow);
  void multScalSp(SparseMatrix<float>* mat, float scal);
  void subtMatSp(SparseMatrix<float> &mat1, const SparseMatrix<float> &mat2);
  void getIdenMatSp(SparseMatrix<float> &mat, unsigned int size);

protected:
  typedef std::map<TERMID_T, Concept> ConceptMap;
  typedef ConceptMap::iterator ConceptMapIt;
  typedef ConceptMap::const_iterator cConceptMapIt;

  typedef std::set<TERMID_T> IdSet;
  typedef IdSet::iterator IdSetIt;

protected:
  bool addToChain(ConChain& chain, TERMID_T termId, const std::string& relType, float weight);
  // get the copy of matrix
  void getMatCpyD(const float** orig, float** &copy, unsigned int nrows, unsigned int ncols);

  // get matrix inverse
  bool getMatInvD(const float** orig, float** &res, unsigned int size);

  // get the product of two matrices
  void getMatProdD(const float** mat1, size_t nrows1, size_t ncols1, const float** mat2, size_t nrows2, size_t ncols2, float** &prod);

  // get the power of a matrix
  void getMatPowD(const float** orig, float** &res, unsigned int size, unsigned int pow);

  // multiply a matrix by a scalar
  void multScalD(float** mat, unsigned int nrows, unsigned int ncols, float scal);

  // subtract one matrix from the other
  void subtMatD(float** mat1, const float** mat2, size_t nrows, size_t ncols);

  // get identity matrix
  void getIdenMatD(float** &mat, unsigned int size);


protected:
  ConceptMap _conMap;
};

class ConceptGraph::ConIterator {
public:
  ConIterator(ConceptGraph *graph) : _graph(graph)
  { }

  void startIteration() {
    _it = _graph->_conMap.begin();
  }

  bool hasMore() {
    return _it != _graph->_conMap.end();
  }

  void getNext(TERMID_T &conId, Concept** con) {
    if(hasMore()) {
      conId = _it->first;
      if(con != NULL) {
        *con = &_it->second;
      }
      _it++;
    }
  }

protected:
  ConceptGraph *_graph;
  ConceptMapIt _it;
};

// delete matrix
void delMat(float** mat, unsigned int nrows);

#endif /* CONCEPTGRAPH_HPP_ */
