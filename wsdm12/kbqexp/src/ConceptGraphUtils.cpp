/* -*- C++ -*- */

// Author: Alexander Kotov (UIUC), 2011
// $Id: ConceptGraphUtils.cpp,v 1.12 2011/05/21 09:54:44 akotov2 Exp $

#include "ConceptGraphUtils.hpp"

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

bool loadQryConGraph(const std::string& fileName, Index *index, TermGraph *termGraph, QryGraphMap &qryGraphMap) {
  std::string line, relType, curQryId = "";
  std::string::size_type hpos;
  TERMID_T srcConId, trgConId;
  ConceptGraph *qryGraph = NULL;
  float relWeight;
  StrVec toks;

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
    hpos = line.find('#');
    if(hpos != std::string::npos) {
      if(curQryId.length()) {
        qryGraphMap[curQryId] = qryGraph;
      }
      curQryId = line.substr(hpos+1);
      qryGraph = new ConceptGraph();
    } else {
      splitStr(line, toks, " ");
      if(toks.size() == 1) {
        srcConId = index->term(toks[0]);
        if(!qryGraph->hasConcept(srcConId)) {
          qryGraph->addConcept(srcConId);
        }
      } else {
        trgConId = index->term(toks[0]);
        if(!qryGraph->hasConcept(trgConId)) {
          qryGraph->addConcept(trgConId);
        }
        if(termGraph != NULL) {
          if(termGraph->relExists(srcConId, trgConId, &relWeight) || termGraph->relExists(trgConId, srcConId, &relWeight)) {
            qryGraph->addRelation(srcConId, trgConId, toks[1], relWeight);
          } else {
            qryGraph->addRelation(srcConId, trgConId, toks[1], 0);
          }
        } else {
          qryGraph->addRelation(srcConId, trgConId, toks[1], 0);
        }
      }
    }
  }

  if(curQryId.length()) {
    qryGraphMap[curQryId] = qryGraph;
  }

  return true;
}

void addTermGraphRG(ConceptGraph *graph, Index *index, TermGraph *termGraph, TERMID_T srcTermId, int maxDist, int maxCont) {
  ConInfo curConInfo;
  TERMID_T curConId, trgConId;
  unsigned int curDist, curTerm;
  std::queue<ConInfo> fringe;
  TermMatrix* termMat = termGraph->getColMatrix();
  SortRelSetIt rit;

  if(srcTermId != 0 && termMat->exists(srcTermId, NULL)) {
    fringe.push(ConInfo(srcTermId, 0, 0));
    if(!graph->hasConcept(srcTermId)) {
      graph->addConcept(srcTermId);
    }
    while(!fringe.empty()) {
      curConInfo = fringe.front();
      curConId = curConInfo.id;
      curDist = curConInfo.dist;
      fringe.pop();
      // DEBUG
      // cout << fringe.size() << "(" << curDist << ")" << endl;
      if(curDist < maxDist && termMat->exists(curConId, NULL)) {
        SortRelSet relSet;
        TermMatrix::ConstColIterator cit(termMat, curConId);
        for(trgConId = cit.begin(); !cit.end(); trgConId = cit.next()) {
          relSet.insert(RelInfoExt(trgConId, "", cit.val(), getTermIDF(index, trgConId)));
        }
        for(rit = relSet.begin(), curTerm = 0; rit != relSet.end(); rit++, curTerm++) {
          if(maxCont != 0 && curTerm >= maxCont) {
            break;
          }
          if(!graph->hasConcept(rit->id)) {
            graph->addConcept(rit->id);
          }
          if(!graph->getRelation(curConId, rit->id, NULL)) {
            graph->addRelation(curConId, rit->id, "", rit->weight);
            fringe.push(ConInfo(rit->id, curDist+1, 0));
          }
        }
      }
    }
  }
}

void printChain(Index *index, const ConChain& path) {
  cConChainIt cit;
  for(cit = path.begin(); cit != path.end(); cit++) {
    if(cit->type.length()) {
      std::cout << "-" << cit->type << "->";
    }
    std::cout << index->term(cit->id);
  }
  std::cout << std::endl;
}

void printGraph(ConceptGraph *graph, Index *index) {
  TERMID_T srcConId, trgConId;
  Concept* con;
  Relation* rel;

  ConceptGraph::ConIterator conIt(graph);
  conIt.startIteration();
  while(conIt.hasMore()) {
    conIt.getNext(srcConId, &con);
    std::cout << index->term(srcConId) << ":";
    Concept::RelIterator relIt(con);
    relIt.startIteration();
    while(relIt.hasMore()) {
      relIt.getNext(trgConId, &rel);
      std::cout << " " << index->term(trgConId) << "(" << rel->weight << ")";
    }
    std::cout << std::endl;
  }
}

void normRelWeights(ConceptGraph *graph, Index *index, const StrFloatMap& relWeights) {
  TERMID_T srcConId, trgConId;
  StrUIntMap relGroups;
  StrUIntMapIt grpIt;
  cStrFloatMapIt it;
  Concept* con;
  Relation* rel;
  UIntFloatMap grpAvs;
  FloatSet relVals;
  FloatSetIt rit;
  unsigned int i, relGroup, curGroup, grpSize, curElem, stepSize;
  float grpSum;

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
    std::cerr << "Warning: insufficient number of values for edge weights" << std::endl;
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
        rel->weight = grpAvs[relGroup] * getTermIDF(index, trgConId);
      } else {
        rel->weight *= getTermIDF(index, trgConId);
      }
    }
  }
}
