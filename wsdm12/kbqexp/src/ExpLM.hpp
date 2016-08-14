/* -*- C++ -*- */

// Author: Alexander Kotov (UIUC), 2011
// $Id: ExpLM.hpp,v 1.1 2011/02/27 02:15:36 akotov2 Exp $

#ifndef EXPLM_HPP_
#define EXPLM_HPP_

#include "common_headers.hpp"
#include "UnigramLM.hpp"

class ExpLM : public lemur::langmod::UnigramLM {
public:
  ExpLM() : _lexId("")
  {  }

  void addTerm(lemur::api::TERMID_T termId, double prob) {
    _langMod[termId] = prob;
  }

  virtual double prob(lemur::api::TERMID_T termId) const {
    cLangModIt it;
    return (it = _langMod.find(termId)) != _langMod.end() ? it->second : 0;
  }

  virtual const std::string lexiconID() const {
    return _lexId;
  }

  virtual void startIteration() const {
    ((ExpLM*)this)->reset();
  }

  virtual bool hasMore() const {
    return _it != _langMod.end();
  }

  virtual void nextWordProb(lemur::api::TERMID_T &termID, double &prob) const {
    termID = _it->first;
    prob = _it->second;
    ((ExpLM*)this)->increment();
  }

  void normalize() {
    double sum = 0;
    for(LangModIt it = _langMod.begin(); it != _langMod.end(); it++) {
      sum += it->second;
    }

    for(LangModIt it = _langMod.begin(); it != _langMod.end(); it++) {
      it->second = it->second / sum;
    }
  }

  inline size_t size() const {
    return _langMod.size();
  }

protected:
  void reset() {
    _it = _langMod.begin();
  }

  void increment() {
    _it++;
  }

protected:
  typedef std::map<lemur::api::TERMID_T, double> LangMod;
  typedef LangMod::iterator LangModIt;
  typedef LangMod::const_iterator cLangModIt;

protected:
  std::string _lexId;
  LangMod _langMod;
  cLangModIt _it;
};

#endif /* EXPLM_HPP_ */

