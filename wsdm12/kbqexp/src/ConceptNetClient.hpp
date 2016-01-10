/* -*- C++ -*- */

// Author: Alexander Kotov (UIUC), 2011
// $Id: ConceptNetClient.hpp,v 1.2 2011/04/18 03:21:03 akotov2 Exp $

#ifndef CONCEPTNETCLIENT_HPP_
#define CONCEPTNETCLIENT_HPP_

#include <iostream>
#include "XmlRpcCpp.h"

#define NAME "ConceptNet Client"
#define VERSION "0.1"

class ContextIterator {
public:
  ContextIterator(XmlRpcValue c) : _context(c), _curPos(-1) {
  }

  inline void startIteration() {
    _curPos = -1;
  }

  inline bool hasNext() const {
    return _curPos + 1 < _context.arraySize();
  }

  inline std::string getRelation() const {
    return _curValue.structGetValue("relation").getString();
  }

  inline std::string getPhrase() const {
    return _curValue.structGetValue("concept").getString();
  }

  inline void moveNext() {
    if(hasNext())
      _curValue = _context.arrayGetItem(++_curPos);
  }

  inline XmlRpcValue getItem(size_t index) const {
    return _context.arrayGetItem(index);
  }

  inline int getSize() const {
    _context.arraySize();
  }
protected:
  int _curPos;
  XmlRpcValue _context;
  XmlRpcValue _curValue;
};

class ConceptNetClient {
public:
  ConceptNetClient(const std::string& servUrl);
  ContextIterator* getTermContext(const std::string& term);
  ~ConceptNetClient();
protected:
  XmlRpcClient *_client;
};


#endif /* CONCEPTNETCLIENT_HPP_ */
