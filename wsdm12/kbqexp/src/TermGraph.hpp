/* -*- C++ -*- */

// Author: Alexander Kotov (UIUC), 2011
// $Id: TermGraph.hpp,v 1.3 2011/05/21 09:54:44 akotov2 Exp $

#ifndef TERMGRAPH_HPP_
#define TERMGRAPH_HPP_

#include "common_headers.hpp"
#include "Index.hpp"
#include "DocInfoList.hpp"
#include "SparseMatrix.hpp"
#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

// designators for tokens
#define TOK_THRESH 1
// needed for input file parsing
#define SPACE_CHARS " \t\n"

typedef SparseMatrix<float> TermMatrix;

typedef std::set<lemur::api::TERMID_T> IdSet;
typedef IdSet::iterator IdSetIt;
typedef IdSet::const_iterator cIdSetIt;

typedef std::set<std::string> StrSet;
typedef StrSet::iterator StrSetIt;
typedef StrSet::const_iterator cStrSetIt;

class TermGraph {
public:
  TermGraph(lemur::api::Index* index) : _index(index), _tmat(NULL)
  { }

  ~TermGraph() {
    if(_tmat != NULL)
      delete _tmat;
  }

  void printTermMatrix(const TermMatrix *tm=NULL);

  inline TermMatrix* getColMatrix() { return _tmat; }

  bool termExists(const lemur::api::TERMID_T termID) const { return  _tmat->exists(termID, NULL); }

  bool relExists(const lemur::api::TERMID_T srcTermID, const lemur::api::TERMID_T trgTermID, float *weight) const {
    return _tmat->exists(srcTermID, trgTermID, weight);
  }

  bool getTermID(const std::string& term, lemur::api::TERMID_T *termID);

  bool getTermByID(const lemur::api::TERMID_T termID, std::string& term);

  float termPairWeight(const lemur::api::TERMID_T id1, const lemur::api::TERMID_T id2) const;

  // construct the term relationship graph from the file
  void loadFile(const std::string& fileName, double scoreThresh) throw(lemur::api::Exception);

  void storeTermMatrixToFile(const TermMatrix *tm, const char *fname);

  // read query file
  void readQueryFile(StrSet& qSet, const std::string& fileName);

protected:
  typedef std::vector<std::string> StrVec;

protected:
  void rstrip(std::string& str, const char *s=SPACE_CHARS);
  void lstrip(std::string& str, const char *s=SPACE_CHARS);
  void stripLine(std::string& str, const char *s=SPACE_CHARS);
  unsigned int split(const std::string& str, std::vector<std::string>& segs, const char *seps=" ",
                     const std::string::size_type lpos=0, const std::string::size_type rpos=std::string::npos);

protected:
  TermMatrix *_tmat;
  lemur::api::Index* _index;
};

#endif /* TERMGRAPH_HPP_ */
