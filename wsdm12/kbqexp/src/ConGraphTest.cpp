/* -*- C++ -*- */

#include "ConceptGraph.hpp"
#include <stdio.h>

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

void initRelWeights(StrFloatMap &relWeights) {
  relWeights["IsA"] = 0.260;
  relWeights["HasProperty"] = 0.142;
  relWeights["CapableOf"] = 0.128;
  relWeights["AtLocation"] = 0.079;
  relWeights["ConceptuallyRelatedTo"] = 0.069;
  relWeights["UsedFor"] = 0.069;
  relWeights["HasA"] = 0.053;
  relWeights["DefinedAs"] = 0.051;
  relWeights["ReceivesAction"] = 0.041;
  relWeights["PartOf"] = 0.030;
  relWeights["CausesDesire"] = 0.024;
  relWeights["LocatedNear"] = 0.016;
  relWeights["Causes"] = 0.010;
  relWeights["HasPrerequisite"] = 0.010;
  relWeights["Desires"] = 0.004;
  relWeights["InstanceOf"] = 0.004;
  relWeights["MadeOf"] = 0.004;
  relWeights["MotivatedByGoal"] = 0.004;
  relWeights["HasFirstSubevent"] = 0.002;
  relWeights["SimilarSize"] = 0.002;
}

void initRelGroups(StrUIntMap &relGroups) {
  relGroups["IsA"] = 1;
  relGroups["HasProperty"] = 1;
  relGroups["CapableOf"] = 1;
  relGroups["AtLocation"] = 1;
  relGroups["ConceptuallyRelatedTo"] = 1;
  relGroups["UsedFor"] = 1;
  relGroups["HasA"] = 2;
  relGroups["DefinedAs"] = 2;
  relGroups["ReceivesAction"] = 2;
  relGroups["PartOf"] = 2;
  relGroups["CausesDesire"] = 2;
  relGroups["LocatedNear"] = 2;
  relGroups["Causes"] = 2;
  relGroups["HasPrerequisite"] = 3;
  relGroups["Desires"] = 3;
  relGroups["InstanceOf"] = 3;
  relGroups["MadeOf"] = 3;
  relGroups["MotivatedByGoal"] = 3;
  relGroups["HasFirstSubevent"] = 3;
  relGroups["SimilarSize"] = 3;
}

void normRelWeights(ConceptGraph *graph) {
  TERMID_T srcConId, trgConId;
  StrUIntMap relGroups;
  StrUIntMapIt grpIt;
  StrFloatMap relWeights;
  cStrFloatMapIt it;
  Concept* con;
  Relation* rel;
  UIntFloatMap grpAvs;
  FloatSet relVals;
  FloatSetIt rit;
  unsigned int i, relGroup, curGroup, grpSize, curElem, stepSize;
  float grpSum;

  initRelWeights(relWeights);
  initRelGroups(relGroups);

  ConceptGraph::ConIterator conIt(graph);
  conIt.startIteration();
  while(conIt.hasMore()) {
    conIt.getNext(srcConId, &con);
    Concept::RelIterator relIt(con);
    relIt.startIteration();
    while(relIt.hasMore()) {
      relIt.getNext(trgConId, &rel);
      if(rel->weight != 0) {
        relVals.insert(rel->weight);
      }
    }
  }

  if(relVals.size() <= 3) {
    // DEBUG
    std::cerr << "Insufficient number of values for edge weights" << std::endl;
    return;
  }

  stepSize = relVals.size() / 3;
  curElem = 0;
  curGroup = 1;
  grpSum = 0;
  grpSize = 0;
  for(rit = relVals.begin(); rit != relVals.end(); rit++) {
    if(curGroup == 3) {
      grpSum += *rit;
      grpSize++;
      if(curElem == relVals.size()-1) {
        grpAvs[curGroup] = grpSum / grpSize;
      }
    } else {
      if(curElem == curGroup * stepSize) {
        grpAvs[curGroup] = grpSum / grpSize;
        curGroup++;
        grpSum = 0;
        grpSize = 0;
      }
      grpSum += *rit;
      grpSize++;
    }
    curElem++;
  }

  ConceptGraph::ConIterator conIt2(graph);
  conIt2.startIteration();
  while(conIt2.hasMore()) {
    conIt2.getNext(srcConId, &con);
    Concept::RelIterator relIt(con);
    relIt.startIteration();
    while(relIt.hasMore()) {
      relIt.getNext(trgConId, &rel);
      if(rel->weight == 0) {
        relGroup = (grpIt = relGroups.find(rel->type)) != relGroups.end() ? grpIt->second : 3;
        rel->weight = grpAvs[relGroup];
      }
    }
  }
}

int main(int argc, char* argv[]) {
  TERMID_T tid;
  Relation rel;
  TERMID_T* tids;
  unsigned int size;

  ConceptGraph *conGraph = new ConceptGraph();
  conGraph->addConcept(11);
  conGraph->addConcept(12);
  conGraph->addConcept(13);
  conGraph->addConcept(14);
  conGraph->addConcept(15);
  conGraph->addConcept(16);
  conGraph->addConcept(17);
  conGraph->addConcept(18);
  conGraph->addConcept(19);
  conGraph->addConcept(20);
  conGraph->addConcept(21);
  conGraph->addConcept(22);
  conGraph->addConcept(23);
  conGraph->addRelation(11, 12, "IsA", 0.4);
  conGraph->addRelation(11, 13, "CapableOf", 0.1);
  conGraph->addRelation(11, 14, "IsA", 0.1);
  conGraph->addRelation(12, 13, "IsA", 0.2);
  conGraph->addRelation(13, 18, "PartOf", 0.5);
  conGraph->addRelation(14, 13, "HasA", 0.6);
  conGraph->addRelation(15, 13, "IsA", 0.6);
  conGraph->addRelation(14, 15, "InstanceOf", 0.2);
  conGraph->addRelation(15, 16, "IsA", 0.8);
  conGraph->addRelation(15, 17, "PartOf", 0.3);
  conGraph->addRelation(17, 18, "Causes", 0.8);
  conGraph->addRelation(17, 19, "IsA", 0.9);
  conGraph->addRelation(17, 20, "PartOf", 0.4);
  conGraph->addRelation(18, 19, "Causes", 0.4);
  conGraph->addRelation(16, 20, "HasProperty", 0.4);
  conGraph->addRelation(16, 21, "AtLocation", 0.1);
  conGraph->addRelation(19, 21, "MadeOf", 0.7);
  conGraph->addRelation(21, 22, "CapableOf", 0.1);
  conGraph->addRelation(21, 19, "MadeOf", 0.7);
  conGraph->addRelation(22, 23, "IsA", 0.7);
  conGraph->addRelation(21, 23, "NewRel", 0.2);
  conGraph->addRelation(23, 20, "MadeOf", 0.6);

  std::cout << "Graph: " << std::endl;
  conGraph->printGraph();

  std::cout << "Context of 13 within 3 edges:" << std::endl;
  ConInfoSet *context = conGraph->getContextChain(13, 3);
  for(ConInfoSetIt it = context->begin(); it != context->end(); it++) {
    std::cout << "TermId=" << it->id << " Dist=" << it->dist << " Weight=" << it->weight << std::endl;
  }
  delete context;

  std::cout << "Paths from 11 to 16 of max. length 5:" << std::endl;
  ConChainList *paths = conGraph->getPaths(11, 16, 5);
  for(ConChainListIt it = paths->begin(); it != paths->end(); it++) {
    conGraph->printChain(*it);
  }
  delete paths;

  /*normRelWeights(conGraph);
  std::cout << "Modified graph: " << std::endl;
  conGraph->printGraph();

  conGraph->saveGraph("graph.txt");
  delete conGraph;

  conGraph = new ConceptGraph();
  if(conGraph->loadGraph("graph.txt")) {
    std::cout << "Loaded graph: " << std::endl;
    conGraph->printGraph();
  }*/

  {
    float** amat;
    size = conGraph->getAdjMatD(amat, tids);
    printf("Adjacency matrix:\n");
    printf("  ");
    for(int i = 0; i < size; i++) {
      printf("  %2u", tids[i]);
    }
    printf("\n");
    for(int i = 0; i < size; i++) {
      printf("%2u", tids[i]);
      for(int j = 0; j < size; j++) {
        printf(" %.1f", amat[i][j]);
      }
      printf("\n");
    }

    //delMat(amat, size);
    //delete[] tids;
  }

  /*{
    SparseMatrix<float>* amat = NULL;
    unsigned int r, c;
    conGraph->getAdjMatSp(amat);
    printf("Adjacency matrix:\n");
    printf("  ");
    SparseMatrix<float>::RowIterator rit(amat);
    for(r = rit.begin(); !rit.end(); r = rit.next()) {
      printf("  %2u", r);
    }
    printf("\n");
    for(r = rit.begin(); !rit.end(); r = rit.next()) {
       printf("%2u", r);
       SparseMatrix<float>::RowIterator rit2(amat);
       for(c = rit2.begin(); !rit2.end(); c = rit2.next()) {
         printf(" %.1f", (*amat)[r][c]);
       }
       printf("\n");
    }
    delete amat;
  }*/

  {
    float** wmat;
    size = conGraph->getFinRandWalkMat(wmat, tids, 0.8, 3);

    printf("Three-step random walk:\n");
    printf("  ");
    for(int i = 0; i < size; i++) {
      printf("    %2u", tids[i]);
    }
    printf("\n");
    for(int i = 0; i < size; i++) {
      printf("%2u", tids[i]);
      for(int j = 0; j < size; j++) {
        printf(" %.3f", wmat[i][j]);
      }
      printf("\n");
    }

    delMat(wmat, size);
    delete[] tids;
  }

  {
    SparseMatrix<float>* wmat = NULL;
    unsigned int r, c;
    conGraph->getFinRandWalkMat(wmat, 0.8, 3);
    printf("\nThree-step random walk:\n");
    printf("  ");
    SparseMatrix<float>::RowIterator rit(wmat);
    for(r = rit.begin(); !rit.end(); r = rit.next()) {
      printf("    %2u", r);
    }
    printf("\n");
    for(r = rit.begin(); !rit.end(); r = rit.next()) {
      printf("%2u", r);
      SparseMatrix<float>::RowIterator rit2(wmat);
      for(c = rit2.begin(); !rit2.end(); c = rit2.next()) {
        printf(" %.3f", (*wmat)[r][c]);
      }
      printf("\n");
    }
    delete wmat;
  }

  /*float** wmat;
  size = conGraph->getInfRandWalkMat(wmat, tids, 0.5);

  printf("\nInfinite random walk:\n");
  printf("  ");
  for(int i = 0; i < size; i++) {
    printf("    %2u", tids[i]);
  }
  printf("\n");
  for(int i = 0; i < size; i++) {
    printf("%2u", tids[i]);
    for(int j = 0; j < size; j++) {
      printf(" %.3f", wmat[i][j]);
    }
    printf("\n");
  }

  delMat(wmat, size);
  delete[] tids;*/

  TermIdSet qryTerms;
  qryTerms.insert(13);
  qryTerms.insert(21);
  TermWeightMap *spActWeights = conGraph->getSpActWeights(qryTerms);
  printf("\nSpreading activation weights:\n");
  for(TermWeightMapIt mit = spActWeights->begin(); mit != spActWeights->end(); mit++) {
    printf("%u %.4f\n", mit->first, mit->second);
  }
  delete spActWeights;

  delete conGraph;

  return 0;
}
