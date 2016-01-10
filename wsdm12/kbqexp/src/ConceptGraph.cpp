/* -*- C++ -*- */

// Author: Alexander Kotov (UIUC), 2011
// $Id: ConceptGraph.cpp,v 1.16 2011/08/02 19:39:39 akotov2 Exp $

#include "ConceptGraph.hpp"

bool ConceptGraph::addToChain(ConChain& chain, TERMID_T conId, const std::string& relType, float weight) {
  for(ConChainIt it = chain.begin(); it != chain.end(); it++) {
    if(it->id == conId)
      return false;
  }
  chain.push_back(RelInfo(conId, relType, weight));
  return true;
}

ConInfoSet* ConceptGraph::getContextChain(const TERMID_T qryTermId, unsigned int maxDist) {
  TERMID_T conId;
  Relation* curRel;
  IdSet visited;
  IdSetIt visIt;
  Concept* curCon;
  ConInfo curConInfo;
  ConInfoSet *context = new ConInfoSet();
  std::queue<ConInfo> fringe;

  if(hasConcept(qryTermId)) {
    fringe.push(ConInfo(qryTermId, 0, 1));
    visited.insert(qryTermId);
  }

  while(!fringe.empty()) {
    curConInfo = fringe.front();
    fringe.pop();
    if(curConInfo.dist != 0) {
      context->insert(curConInfo);
    }

    if(curConInfo.dist < maxDist) {
      getConcept(curConInfo.id, &curCon);
      Concept::RelIterator concIt(curCon);
      concIt.startIteration();
      while(concIt.hasMore()) {
        concIt.getNext(conId, &curRel);
        if((visIt = visited.find(conId)) == visited.end()) {
          visited.insert(conId);
          fringe.push(ConInfo(conId, curConInfo.dist+1, curConInfo.weight*curRel->weight));
        }
      }
    }
  }

  return context;
}

ConInfoSet* ConceptGraph::getContextDist(const TERMID_T qryTermId, unsigned int maxDist) {
  TERMID_T conId;
  Relation* curRel;
  IdSet visited;
  IdSetIt visIt;
  Concept* curCon;
  ConInfo curConInfo;
  ConInfoSet *context = new ConInfoSet();
  std::queue<ConInfo> fringe;

  if(hasConcept(qryTermId)) {
    fringe.push(ConInfo(qryTermId, 0, 1));
    visited.insert(qryTermId);
  }

  while(!fringe.empty()) {
    curConInfo = fringe.front();
    fringe.pop();
    if(curConInfo.dist != 0) {
      context->insert(curConInfo);
    }

    if(curConInfo.dist < maxDist) {
      getConcept(curConInfo.id, &curCon);
      Concept::RelIterator concIt(curCon);
      concIt.startIteration();
      while(concIt.hasMore()) {
        concIt.getNext(conId, &curRel);
        if((visIt = visited.find(conId)) == visited.end()) {
          visited.insert(conId);
          fringe.push(ConInfo(conId, curConInfo.dist+1, curRel->weight*exp(-(float)curConInfo.dist+1)));
        }
      }
    }
  }

  return context;
}

ConChainList* ConceptGraph::getPaths(const TERMID_T srcId, const TERMID_T trgId, unsigned int maxDist) {
  TERMID_T conId;
  Relation* curRel;
  Concept* curCon;
  std::queue<ConChain> fringe;
  ConChainList *paths = new ConChainList();
  ConChain curPath;
  int curDist;
  float curWeight;

  if(hasConcept(srcId)) {
    curPath.push_back(RelInfo(srcId, "", 1));
    fringe.push(curPath);
  }

  while(!fringe.empty()) {
    curPath = fringe.front();
    curDist = curPath.size()-1;
    curWeight = curPath.back().weight;
    fringe.pop();
    if(curDist < maxDist) {
      getConcept(curPath.back().id, &curCon);
      Concept::RelIterator conIt(curCon);
      conIt.startIteration();
      while(conIt.hasMore()) {
        conIt.getNext(conId, &curRel);
        if(conId == trgId) {
          // DEBUG
          //float weight = curWeight*curRel->weight*exp(-(float)curDist);
          curPath.push_back(RelInfo(conId, curRel->type, curRel->weight*exp(-(float)curDist)));
          paths->push_back(curPath);
        } else {
          // DEBUG
          //float weight = curWeight*curRel->weight*exp(-(float)curDist);
          ConChain curCopy = curPath;
          if(addToChain(curCopy, conId, curRel->type, curRel->weight*exp(-(float)curDist))) {
            fringe.push(curCopy);
          }
        }
      }
    }
  }

  return paths;
}

bool ConceptGraph::getConcept(TERMID_T conTermId, Concept** con) {
  ConceptMapIt it;
  if((it = _conMap.find(conTermId)) != _conMap.end()) {
    if(con != NULL) {
      *con = &it->second;
    }
    return true;
  } else {
    if(con != NULL) {
      *con = NULL;
    }
    return false;
  }
}

void ConceptGraph::delConcept(TERMID_T conTermId) {
  ConceptMapIt it;
  Concept *con;
  if((it = _conMap.find(conTermId)) != _conMap.end()) {
    _conMap.erase(it);
  }
  for(it = _conMap.begin(); it != _conMap.end(); it++) {
    it->second.delRelation(conTermId);
  }
}

bool ConceptGraph::hasRelation(const TERMID_T termId1, const TERMID_T termId2) const {
  cConceptMapIt it;
  if((it = _conMap.find(termId1)) != _conMap.end()) {
    return it->second.hasRelation(termId2);
  } else {
    return false;
  }
}

bool ConceptGraph::getRelation(const TERMID_T termId1, const TERMID_T termId2, Relation** rel) {
  ConceptMapIt cit;
  if((cit = _conMap.find(termId1)) != _conMap.end()) {
    return cit->second.getRelation(termId2, rel);
  } else {
    if(rel != NULL) {
      *rel = NULL;
    }
    return false;
  }
}

void ConceptGraph::addRelation(const TERMID_T termId1, const TERMID_T termId2, const std::string& type, double weight) {
  Relation rel(type, weight);
  _conMap[termId1].addRelation(termId2, rel);
  _conMap[termId2].addRelation(termId1, rel);
}

void ConceptGraph::updateRelation(const TERMID_T termId1, const TERMID_T termId2, const std::string& type, double weight) {
  Relation *rel;
  ConceptMapIt cit;

  if((cit = _conMap.find(termId1)) != _conMap.end()) {
    if(cit->second.getRelation(termId2, &rel)) {
      rel->type = type;
      rel->weight = weight;
      rel = &_conMap[termId2][termId1];
      rel->type = type;
      rel->weight = weight;
    }
  }
}

void ConceptGraph::printGraph() {
  TERMID_T relConId;
  Relation* rel;

  for(ConceptMapIt cit = _conMap.begin(); cit != _conMap.end(); cit++) {
    std::cout << cit->first << ":";
    Concept::RelIterator rit(&cit->second);
    rit.startIteration();
    while(rit.hasMore()) {
      rit.getNext(relConId, &rel);
      std::cout << " (" << relConId << "[" << rel->type << "]:" << rel->weight << ")";
    }
    std::cout << std::endl;
  }
}

void ConceptGraph::printChain(const ConChain &conChain) const {
  for(cConChainIt cit = conChain.begin(); cit != conChain.end(); cit++) {
    if(cit->type.length()) {
      std::cout << "-" << cit->type << "->";
    }
    std::cout << cit->id;
  }
  std::cout << std::endl;
}

bool ConceptGraph::loadGraph(const std::string& fileName) {
  std::string line, relType;
  TERMID_T srcConId, trgConId;
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
    splitStr(line, toks, " ");
    if(toks.size() == 1) {
      srcConId = atoi(toks[0].c_str());
      if(!hasConcept(srcConId)) {
        addConcept(srcConId);
      }
    } else {
      trgConId = atoi(toks[0].c_str());
      if(!hasConcept(trgConId)) {
        addConcept(trgConId);
      }
      addRelation(srcConId, trgConId, toks[1], 0);
    }
  }

  return true;
}

bool ConceptGraph::saveGraph(const std::string& fileName) {
  TERMID_T relConId;
  Relation* rel;
  std::ofstream outFile;

  outFile.open(fileName.c_str());
  if(!outFile.is_open()) {
    std::cerr << "Error: can't open file " << fileName << " for writing!" << std::endl;
    return false;
  }

  for(ConceptMapIt cit = _conMap.begin(); cit != _conMap.end(); cit++) {
    outFile << cit->first << std::endl;
    Concept::RelIterator rit(&cit->second);
    rit.startIteration();
    while(rit.hasMore()) {
      rit.getNext(relConId, &rel);
      outFile << relConId << " " << rel->type << std::endl;
    }
  }

  outFile.close();
}

unsigned int ConceptGraph::getAdjMatD(float** &mat, TERMID_T* &termIDs) {
  ConceptMapIt cit;
  TERMID_T conId;
  Relation* rel;
  std::map<TERMID_T, unsigned int> id2ind;
  std::map<TERMID_T, unsigned int>::iterator iit;

  unsigned int i, size = _conMap.size();
  mat = new float*[size];
  termIDs = new TERMID_T[size];

  for(i = 0, cit = _conMap.begin(); cit != _conMap.end(); cit++, i++) {
    mat[i] = new float[size];
    id2ind[cit->first] = i;
    termIDs[i] = cit->first;
  }

  for(i = 0, cit = _conMap.begin(); cit != _conMap.end(); cit++, i++) {
    Concept::RelIterator rit(&cit->second);
    rit.startIteration();
    while(rit.hasMore()) {
      rit.getNext(conId, &rel);
      mat[i][id2ind[conId]] = rel->weight;
    }
  }

  return size;
}

void ConceptGraph::getAdjMatSp(SparseMatrix<float>* &mat) {
  ConceptMapIt cit;
  Relation* rel;
  TERMID_T conId;

  mat = new SparseMatrix<float>();
  for(cit = _conMap.begin(); cit != _conMap.end(); cit++) {
    Concept::RelIterator rit(&cit->second);
    rit.startIteration();
    while(rit.hasMore()) {
      rit.getNext(conId, &rel);
      mat->set(cit->first, conId, rel->weight);
    }
  }
}

void ConceptGraph::getMatCpyD(const float** orig, float** &copy, unsigned int nrows, unsigned int ncols) {
  copy = new float*[nrows];
  for(int i = 0; i < nrows; i++) {
    copy[i] = new float[ncols];
    for(int j = 0; j < ncols; j++) {
      copy[i][j] = orig[i][j];
    }
  }
}

void ConceptGraph::getMatCpySp(const SparseMatrix<float> *orig, SparseMatrix<float>* &copy) {
  unsigned int r, c;
  copy = new SparseMatrix<float>();
  SparseMatrix<float>::ConstRowIterator rit(orig);
  for(r = rit.begin(); !rit.end(); r = rit.next()) {
    SparseMatrix<float>::ConstColIterator cit(orig, r);
    for(c = cit.begin(); !cit.end(); c = cit.next()) {
      copy->set(r, c, cit.val());
    }
  }
}

void ConceptGraph::getMatProdD(const float** mat1, size_t nrows1, size_t ncols1,
                               const float** mat2, size_t nrows2, size_t ncols2, float** &prod) {
  float cur;
  if(ncols1 != nrows2)
    return;
  prod = new float*[nrows1];

  for(int i = 0; i < nrows1; i++) {
    prod[i] = new float[ncols2];
    // DEBUG
    //if(i % 100 == 0)
    //  std::cout << i << std::endl;
    for(int j = 0; j < ncols2; j++) {
      cur = 0;
      for(int k = 0; k < ncols1; k++) {
        cur += mat1[i][k]*mat2[k][j];
      }
      prod[i][j] = cur;
    }
  }
}

void ConceptGraph::getMatProdSp(const SparseMatrix<float> *mat1, const SparseMatrix<float> *mat2, SparseMatrix<float>* &prod) {
  unsigned int i, j, k;
  float cur, val;
  prod = new SparseMatrix<float>();
  SparseMatrix<float>::ConstRowIterator rit1(mat1);
  for(i = rit1.begin(); !rit1.end(); i = rit1.next()) {
    SparseMatrix<float>::ConstRowIterator rit2(mat1);
    for(j = rit2.begin(); !rit2.end(); j = rit2.next()) {
      cur = 0;
      SparseMatrix<float>::ConstColIterator cit(mat1, i);
      for(k = cit.begin(); !cit.end(); k = cit.next()) {
        if(mat2->exists(k, j, &val)) {
          cur += cit.val()*val;
        }
      }
      if(cur != 0) {
        prod->set(i, j, cur);
      }
    }
  }
}

void ConceptGraph::getMatPowD(const float** orig, float** &res, unsigned int size, unsigned int power) {
  float **prod, **cur_pow;
  getMatCpyD(orig, cur_pow, size, size);
  for(int p = 0; p < power-1; p++) {
    getMatProdD((const float**) cur_pow, size, size, orig, size, size, prod);
    delMat(cur_pow, size);
    cur_pow = prod;
  }
  res = cur_pow;
}

void ConceptGraph::getMatPowSp(const SparseMatrix<float>* orig, SparseMatrix<float>* &res, unsigned int power) {
  SparseMatrix<float> *prod, *cur_pow;
  getMatCpySp(orig, cur_pow);
  for(int p = 0; p < power-1; p++) {
    getMatProdSp((const SparseMatrix<float>*) cur_pow, orig, prod);
    delete cur_pow;
    cur_pow = prod;
  }
  res = cur_pow;
}

void ConceptGraph::multScalD(float** mat, unsigned int nrows, unsigned int ncols, float scal) {
  for(int i = 0; i < nrows; i++) {
    for(int j = 0; j < ncols; j++) {
      mat[i][j] *= scal;
    }
  }
}

void ConceptGraph::multScalSp(SparseMatrix<float>* mat, float scal) {
  unsigned int r;
  SparseMatrix<float>::RowIterator rit(mat);
  for(r = rit.begin(); !rit.end(); r = rit.next()) {
    SparseMatrix<float>::ColIterator cit(mat, r);
    for(cit.begin(); !cit.end(); cit.next()) {
      float &val = cit.val();
      val *= scal;
    }
  }
}

unsigned int ConceptGraph::getFinRandWalkMat(float** &mat, TERMID_T* &termIDs, float alpha, int numSteps) {
  float** amat;
  unsigned int size = getAdjMatD(amat, termIDs);
  getMatPowD((const float**) amat, mat, size, numSteps);
  delMat(amat, size);
  multScalD(mat, size, size, (1-alpha)*pow(alpha, numSteps));
  return size;
}

void ConceptGraph::getFinRandWalkMat(SparseMatrix<float>* &mat, float alpha, int numSteps) {
  SparseMatrix<float>* amat = NULL;
  getAdjMatSp(amat);
  getMatPowSp((const SparseMatrix<float>*) amat, mat, numSteps);
  delete amat;
  multScalSp(mat, (1-alpha)*pow(alpha, numSteps));
}

unsigned int ConceptGraph::getInfRandWalkMat(float** &mat, TERMID_T* &termIDs, float alpha) {
  float **amat, **imat;
  unsigned int size = getAdjMatD(amat, termIDs);
  getIdenMatD(imat, size);
  multScalD(amat, size, size, alpha);
  subtMatD(imat, (const float**) amat, size, size);
  delMat(amat, size);
  if(!getMatInvD((const float**) imat, mat, size)) {
    std::cerr << "Error: Can't get matrix inverse!" << std::endl;
    delMat(imat, size);
  }
  delMat(imat, size);
  getIdenMatD(imat, size);
  subtMatD(mat, (const float**) imat, size, size);
  delMat(imat, size);
  return size;
}

bool ConceptGraph::getMatInvD(const float** orig, float** &res, unsigned int size) {
  float max_abs, max_val, temp;
  int i, j, row, col, max_row, ncols=2*size;

  float** mat = new float*[size];
  res = new float*[size];
  for(i = 0; i < size; i++) {
    mat[i] = new float[2*size];
    res[i] = new float[size];
    for(j = 0; j < ncols; j++) {
      if(j < size) {
        mat[i][j] = orig[i][j];
      } else if(j == size+i) {
        mat[i][j] = 1;
      } else {
        mat[i][j] = 0;
      }
    }
  }

  for(col = 0; col < size; col++) {
    max_row = col;
    max_abs = fabs(mat[col][col]);
    max_val = mat[col][col];
    // finding a pivot in the current column
    for(row = col+1; row < size; row++) {
      if(fabs(mat[row][col]) > max_abs) {
        max_abs = fabs(mat[row][col]);
        max_val = mat[row][col];
        max_row = row;
      }
    }

    if(max_abs != 0) {
      if(max_row != col) {
        // swap current row with max_row
        for(j = col; j < ncols; j++) {
          temp = mat[col][j];
          mat[col][j] = mat[max_row][j] / max_val;
          mat[max_row][j] = temp;
        }
      } else {
        for(j = col; j < ncols; j++) {
          mat[col][j] = mat[col][j] / max_val;
        }
      }
      if(col != size-1) {
        for(row = col+1; row < size; row++) {
          temp = mat[row][col];
          for(j = col; j < ncols; j++) {
            mat[row][j] -= mat[col][j]*temp;
          }
        }
      }
    } else {
      delMat(mat, size);
      return false;
    }
  }

  // DEBUG
  /*printf("Row-echelon form:\n");
  for(i = 0; i < size; i++) {
    for(j = 0; j < ncols; j++) {
      printf("%.2f ", mat[i][j]);
    }
    printf("\n");
  }
  printf("\n");*/

  for(col = size-1; col > 0; col--) {
    for(row = col-1; row >=0; row--) {
      temp = mat[row][col];
      for(j = col; j < ncols; j++) {
        mat[row][j] -= mat[col][j]*temp;
      }
    }
  }

  // DEBUG
  /*printf("Reduced row-echelon form:\n");
  for(i = 0; i < size; i++) {
    for(j = 0; j < ncols; j++) {
      printf("%.2f ", mat[i][j]);
    }
    printf("\n");
  }
  printf("\n");*/

  for(row = 0; row < size; row++) {
    for(col = size, j = 0; col < ncols; col++, j++) {
      res[row][j] = mat[row][col];
    }
  }

  delMat(mat, size);

  return true;
}

void ConceptGraph::subtMatD(float** mat1, const float** mat2, size_t nrows, size_t ncols) {
  for(unsigned int i = 0; i < nrows; i++) {
    for(unsigned int j = 0; j < ncols; j++) {
      mat1[i][j] -= mat2[i][j];
    }
  }
}

void ConceptGraph::getIdenMatD(float** &mat, unsigned int size) {
  mat = new float*[size];
  for(unsigned int i = 0; i < size; i++) {
    mat[i] = new float[size];
    for(unsigned int j = 0; j < size; j++) {
      mat[i][j] = i == j ? 1 : 0;
    }
  }
}

bool ConceptGraph::getMatInvSp(const SparseMatrix<float> &orig, SparseMatrix<float> &res) {

}

void ConceptGraph::subtMatSp(SparseMatrix<float> &mat1, const SparseMatrix<float> &mat2) {

}

void ConceptGraph::getIdenMatSp(SparseMatrix<float> &mat, unsigned int size) {

}

void delMat(float** mat, unsigned int nrows) {
  for(int i = 0; i < nrows; i++) {
    delete[] mat[i];
  }
  delete[] mat;
}

TermWeightMap* ConceptGraph::getSpActWeights(const TermIdSet &qryTermIds, float distConst) {
  float actScore;
  ConceptMapIt cit;
  cTermIdSetIt termIt, tempIt;
  IdSetIt visIt;
  TERMID_T srcConId, trgConId;
  Concept *curCon;
  Relation *rel;
  std::queue<TERMID_T> fringe;
  TermWeightMap *actScores = new TermWeightMap();

  for(cit = _conMap.begin(); cit != _conMap.end(); cit++) {
    (*actScores)[cit->first] = qryTermIds.find(cit->first) != qryTermIds.end() ? 1 : 0;
  }

  for(termIt = qryTermIds.begin(); termIt != qryTermIds.end(); termIt++) {
    IdSet visited;
    if(!getConcept(*termIt, &curCon)) {
      continue;
    }
    visited.insert(*termIt);
    Concept::RelIterator conIt(curCon);
    conIt.startIteration();
    while(conIt.hasMore()) {
      conIt.getNext(trgConId, &rel);
      fringe.push(trgConId);
      visited.insert(trgConId);
    }

    while(!fringe.empty()) {
      srcConId = fringe.front();
      // DEBUG
      // std::cout << "Activating " << srcConId << ": " << std::endl;
      fringe.pop();
      getConcept(srcConId, &curCon);
      Concept::RelIterator conIt(curCon);
      conIt.startIteration();
      if((tempIt = qryTermIds.find(srcConId)) == qryTermIds.end()) {
        actScore = 0;
        while(conIt.hasMore()) {
          conIt.getNext(trgConId, &rel);
          // DEBUG
          // std::cout << "Neighbor: " << trgConId << " Rel: " << rel->weight << " Weight: " << (*actScores)[trgConId] << std::endl;
          actScore += rel->weight*(*actScores)[trgConId];
          if((visIt = visited.find(trgConId)) == visited.end()) {
            fringe.push(trgConId);
            visited.insert(trgConId);
          }
        }
        if(distConst*actScore > (*actScores)[srcConId])
          (*actScores)[srcConId] = distConst*actScore;
        // DEBUG
        // std::cout << "Activation weight: " << (*actScores)[srcConId] << std::endl;
      } else {
        while(conIt.hasMore()) {
          conIt.getNext(trgConId, &rel);
          if((visIt = visited.find(trgConId)) == visited.end()) {
            fringe.push(trgConId);
            visited.insert(trgConId);
          }
        }
      }
    }
  }

  return actScores;
}
