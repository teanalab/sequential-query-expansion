/* -*- C++ -*- */

// Author: Alexander Kotov (UIUC), 2011
// $Id: ConUtils.cpp,v 1.7 2011/06/20 23:43:07 akotov2 Exp $

#include "ConUtils.hpp"

float getTermIDF(Index* index, TERMID_T termId) {
  return log(((float)index->docCount()) / index->docCount(termId));
}

void printTermVec(Index *index, const IndexedRealVector *termVec) {
  for(int i = 0; i < termVec->size(); i++) {
    std::cout << i+1 << ". " << index->term(termVec->at(i).ind) << ": " << termVec->at(i).val << std::endl;
  }
}

bool inTermVec(TERMID_T termId, const TermIdPairVec &termVec) {
  for(int i = 0; i < termVec.size(); i++) {
    if(termId == termVec[i].termId)
      return true;
  }

  return false;
}

void normQueryModel(SimpleKLQueryModel *queryModel) {
  double sumWeight = 0;

  queryModel->startIteration();
  while(queryModel->hasMore()) {
    QueryTerm *qt = queryModel->nextTerm();
    sumWeight += qt->weight();
  }

  queryModel->startIteration();
  while(queryModel->hasMore()) {
    QueryTerm *qt = queryModel->nextTerm();
    queryModel->setCount(qt->id(), qt->weight() / sumWeight);
  }
}

void readQueriesUnstem(Index *index, PorterStemmer *stemmer, const std::string &qryFile, QryTermMap &queryMap) {
  char *buf = NULL;
  TextQuery *query = NULL;
  DocStream *qStream = NULL;
  TERMID_T termId;
  TERM_T queryTerm;

  try {
    qStream = new BasicDocStream(qryFile);
  } catch (Exception &ex) {
    ex.writeMessage(std::cerr);
    throw Exception("readQueriesUnstem", "Can't open file with unstemmed queries");
  }

  qStream->startDocIteration();
  while(qStream->hasMore()) {
    TermIdPairVec queryTerms;
    Document *d = qStream->nextDoc();
    query = new TextQuery(*d);
    query->startTermIteration();
    while(query->hasMore()) {
      queryTerm = query->nextTerm()->spelling();
      buf = new char[queryTerm.size()+1];
      memset((void *) buf, 0, sizeof(char) * (queryTerm.size()+1));
      queryTerm.copy(buf,queryTerm.size()+1);
      termId = index->term(stemmer->stemWord(buf));
      queryTerms.push_back(TermIdPair(queryTerm, termId));
      delete buf;
    }
    queryMap[query->id()] = queryTerms;
    delete query;
  }

  delete qStream;
}

void readQueries(Index *index, const std::string &qryFile, QryTermMap &queryMap) {
  TextQuery *query = NULL;
  DocStream *qStream = NULL;
  TERM_T qryTerm;
  TERMID_T qryTermId;

  try {
    qStream = new BasicDocStream(qryFile);
  } catch (Exception &ex) {
    ex.writeMessage(std::cerr);
    throw Exception("readQueries", "Can't open file with queries");
  }

  qStream->startDocIteration();
  while(qStream->hasMore()) {
    TermIdPairVec queryTerms;
    Document *d = qStream->nextDoc();
    query = new TextQuery(*d);
    query->startTermIteration();
    while(query->hasMore()) {
      const Term *queryTerm = query->nextTerm();
      qryTerm = queryTerm->spelling();
      qryTermId = index->term(qryTerm);
      queryTerms.push_back(TermIdPair(qryTerm, qryTermId));
    }
    queryMap[query->id()] = queryTerms;
    delete query;
  }

  delete qStream;
}

TERMID_T stemAndGetId(Index *index, PorterStemmer *stemmer, const std::string& term) {
  TERMID_T termId;
  char* buf = new char[term.size()+1];
  memset((void *) buf, 0, sizeof(char)*(term.size()+1));
  term.copy(buf,term.size()+1);
  termId = index->term(stemmer->stemWord(buf));
  delete buf;
  return termId;
}

TermWeightList* parseConPhrase(Index *index, TermGraph *graph, PorterStemmer *stemmer, IdTermMap &termMap,
                               TERMID_T srcConId, std::string& conPhrase, double maxFrac) {
  TERMID_T trgConId;
  StrVec conTerms;
  stripStr(conPhrase, SPACE_CHARS);
  splitStr(conPhrase, conTerms, " ");
  TermWeightList *termList = new TermWeightList();
  for(int i = 0; i < conTerms.size(); i++) {
    trgConId = stemAndGetId(index, stemmer, conTerms[i]);
    if(trgConId != 0) {
      if(index->docCount(trgConId) < maxFrac*index->docCount()) {
        termMap[trgConId] = conTerms[i];
        termList->push_back(TermWeight(trgConId, 0));
      }
    }
  }

  return termList;
}

TermWeightList* parseConPhraseIDF(Index *index, PorterStemmer *stemmer, IdTermMap &termMap,
                                  TERMID_T srcConId, std::string& conPhrase, double maxFrac) {
  TERMID_T trgConId;
  StrVec conTerms;
  stripStr(conPhrase, SPACE_CHARS);
  splitStr(conPhrase, conTerms, " ");
  TermWeightList *termList = new TermWeightList();
  for(int i = 0; i < conTerms.size(); i++) {
    trgConId = stemAndGetId(index, stemmer, conTerms[i]);
    if(trgConId != 0) {
      if(index->docCount(trgConId) < maxFrac * index->docCount()) {
        termMap[trgConId] = conTerms[i];
        termList->push_back(TermWeight(trgConId, getTermIDF(index, trgConId)));
      }
    }
  }

  return termList;
}

bool readExpTermsFile(const std::string &fileName, Index *index, QryExpTerms &expTerms) {
  TERMID_T termId;
  float weight;
  StrVec toks;
  std::string line, curQryId = "";
  TermWeightMap curExpTerms;

  std::ifstream fs(fileName.c_str(), std::ifstream::in);
  if(!fs.is_open()) {
    std::cerr << "Error: can't open file " << fileName << " for reading!" << std::endl;
    return false;
  }

  while(getline(fs, line)) {
    toks.clear();
    stripStr(line, SPACE_CHARS);
    if(!line.length())
      continue;
    splitStr(line, toks, " ");
    if(toks.size() == 1) {
      if(curQryId.length()) {
        expTerms[curQryId] = curExpTerms;
        curExpTerms.clear();
      }
      curQryId = toks[0];
    } else {
      termId = index->term(toks[0]);
      weight = atof(toks[1].c_str());
      curExpTerms[termId] = weight;
    }
  }

  expTerms[curQryId] = curExpTerms;
  return true;
}
