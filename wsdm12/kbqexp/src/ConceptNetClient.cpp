/* -*- C++ -*- */

// Author: Alexander Kotov (UIUC), 2011
// $Id: ConceptNetClient.cpp,v 1.2 2011/04/21 20:36:27 akotov2 Exp $

#include "ConceptNetClient.hpp"

ConceptNetClient::ConceptNetClient(const std::string& url) {
  XmlRpcClient::Initialize(NAME, VERSION);
  _client = new XmlRpcClient(url);
}

ConceptNetClient::~ConceptNetClient() {
  XmlRpcClient::Terminate();
  delete _client;
}

ContextIterator* ConceptNetClient::getTermContext(const std::string& term) {
  ContextIterator *contIt = NULL;
  XmlRpcValue paramArray = XmlRpcValue::makeArray();
  paramArray.arrayAppendItem(XmlRpcValue::makeString(term));

  try {
    XmlRpcValue result = _client->call("get_context", paramArray);
    contIt = new ContextIterator(result.structGetValue("context").getArray());
  } catch (XmlRpcFault &fault) {
    // std::cerr << "XML-RPC error: (" << fault.getFaultCode() << ") " << fault.getFaultString() << std::endl;
    return NULL;
  }

  return contIt;
}
