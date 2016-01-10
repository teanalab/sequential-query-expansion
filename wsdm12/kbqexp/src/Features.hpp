/* -*- C++ -*- */

// Author: Alexander Kotov (UIUC), 2011
// $Id: Features.hpp,v 1.2 2011/06/20 23:43:08 akotov2 Exp $

#ifndef FEATURES_HPP_
#define FEATURES_HPP_

typedef struct Features_ {
  Features_() : expCon(""), numQryTerms(0), topDocScore(0), expTDocScore(0), topTermFrac(0),
                numCanDocs(0), avgCDocScore(0), maxCDocScore(0), idf(0), fanOut(0), spActScore(0),
                spActRank(0), rndWalkScore(0), pathFindScore(0), avgColCor(0), maxColCor(0),
                avgTopCor(0), maxTopCor(0), avgTopPCor(0), maxTopPCor(0), avgQDist(0), maxQDist(0),
                avgPWeight(0), maxPWeight(0), map(0)
  { }
  // expansion concept
  std::string expCon;
  /////////////////////////////////////////////////////////////////////////////////////////////////
  // features of the query
  /////////////////////////////////////////////////////////////////////////////////////////////////
  // number of query terms
  float numQryTerms;
  // retrieval score of the document ranked 1
  float topDocScore;

  /////////////////////////////////////////////////////////////////////////////////////////////////
  // features of the expansion candidate
  /////////////////////////////////////////////////////////////////////////////////////////////////
  // retrieval score of the document ranked 1 for the expanded query
  float expTDocScore;
  // ratio of the number of occurrences of the candidate term over all the terms in the top 10 retrieved documents
  float topTermFrac;
  // the number of top n documents, containing the candidate
  float numCanDocs;
  // average retrieval score of the documents, containing the candidate
  float avgCDocScore;
  // maximum retrieval score of the document, containing the candidate
  float maxCDocScore;
  // IDF of the candidate term
  float idf;
  // fan out number of the candidate in the query concept graph
  float fanOut;
  // score of the candidate in the spreading activation of the query
  float spActScore;
  // rank of the candidate in the spreading activation of the query
  float spActRank;
  // score of the candidate using the Finite Random Walk method
  float rndWalkScore;
  // score of the candidate using the Path Finding method
  float pathFindScore;

  /////////////////////////////////////////////////////////////////////////////////////////////////
  // features of the expansion candidate with respect to query terms
  /////////////////////////////////////////////////////////////////////////////////////////////////
  // average co-occurrence of the candidate with query terms in the collection
  float avgColCor;
  // maximum co-occurrence of the candidate with query terms in the collection
  float maxColCor;
  // average co-occurrence of the candidate with query terms in the top n retrieved documents
  float avgTopCor;
  // maximum co-occurrence of the candidate with query terms in the top n retrieved documents
  float maxTopCor;
  // average co-occurrence of the candidate with pairs of query terms in the top n retrieved results
  float avgTopPCor;
  // maximum co-occurrence of the candidate with pairs of query terms in the top n retrieved results
  float maxTopPCor;
  // average distance of the candidate to the query terms in the query concept graph
  float avgQDist;
  // maximum distance of the candidate to the query terms in the query concept graph
  float maxQDist;
  // average weight of the paths to the concept from the query terms in the query concept graph
  float avgPWeight;
  // maximum weight of the paths to the concept from the query terms in the query concept graph
  float maxPWeight;
  // MAP of the query expanded with the candidate term
  float map;
} Features;

typedef std::vector<Features> FeatVec;

typedef std::map<unsigned int, FeatVec> FeatMap;
typedef FeatMap::iterator FeatMapIt;
typedef FeatMap::const_iterator cFeatMapIt;

#endif /* FEATURES_HPP_ */
