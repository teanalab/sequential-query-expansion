#include "TermGraph.hpp"

using namespace std;
using namespace lemur::api;

void TermGraph::loadFile(const std::string& fileName, double scoreThresh) throw(lemur::api::Exception) {
  string line;
  TERMID_T curTermID = 0, srcTermID = 0, trgTermID = 0;
  float prevWeight, curWeight;
  StrVec toks;

  ifstream inFile(fileName.c_str(), ifstream::in);
  if(!inFile.is_open()) {
    char msg[1024];
    snprintf(msg, 1024, "can't open file %s", fileName.c_str());
    throw lemur::api::Exception("TermGraph::loadFile", msg);
  }

  if(_tmat != NULL) {
    delete _tmat;
  }

  _tmat = new TermMatrix(0);
  while(getline(inFile, line)) {
    toks.clear();
    stripLine(line);
    if(!line.length())
      continue;
    split(line, toks);
    curWeight = atof(toks[1].c_str());
    if(curWeight >= TOK_THRESH) {
      srcTermID = _index->term(toks[0]);
    } else {
      if(curWeight > scoreThresh) {
        trgTermID = _index->term(toks[0]);
        if(srcTermID != trgTermID) {
          _tmat->set(srcTermID, trgTermID, curWeight);
        }
      }
    }
  }

  inFile.close();
}

float TermGraph::termPairWeight(const TERMID_T id1, const TERMID_T id2) const {
  float weight;
  if(_tmat->exists(id1, id2, &weight)) {
    return weight;
  } else {
    return 0.0;
  }
}

bool TermGraph::getTermID(const std::string& term, TERMID_T *id) {
  *id = _index->term(term);
}

bool TermGraph::getTermByID(const TERMID_T termID, std::string& term) {
  term = _index->term(termID);
}

void TermGraph::printTermMatrix(const TermMatrix *tm) {
  TermMatrix::ConstRowIterator *rit;

  if(tm == NULL) {
    rit = new TermMatrix::ConstRowIterator((const TermMatrix* ) _tmat);
  } else {
    rit = new TermMatrix::ConstRowIterator(tm);
  }

  for(unsigned int r = rit->begin(); !rit->end(); r = rit->next()) {
    cout << _index->term(r) << ": ";
    TermMatrix::ConstColIterator cit(&rit->row());
    for(unsigned int c = cit.begin(); !cit.end(); c = cit.next()) {
      cout << _index->term(c) << "(" << cit.val() << ") ";
    }
    cout << endl;
  }

  delete rit;
}

void TermGraph::storeTermMatrixToFile(const TermMatrix *tm, const char *fname) {
  FILE *f = fopen(fname, "w");

  fprintf(f, "graph [\n");

  TermMatrix::ConstRowIterator rit(tm);
  for(unsigned int r = rit.begin(); !rit.end(); r = rit.next()) {
    // DEBUG
    if(!tm->get(r).numCols()) {
      fprintf(stdout, "Lonely node #%d\n", r);
    }
    fprintf(f, "  node [\n");
    fprintf(f, "    id %d\n", r);
    fprintf(f, "    label \"%s\"\n", _index->term(r).c_str());
    fprintf(f, "  ]\n");
  }

  for(unsigned int r = rit.begin(); !rit.end(); r = rit.next()) {
    TermMatrix::ConstColIterator cit(tm, r);
    for(unsigned int c = cit.begin(); !cit.end(); c = cit.next()) {
      fprintf(f, "  edge [\n");
      fprintf(f, "    source %u\n", r);
      fprintf(f, "    target %u\n", c);
      fprintf(f, "    weight %.3f\n", cit.val());
      fprintf(f, "  ]\n");
    }
  }

  fprintf(f, "]\n");

  fclose(f);
}

void TermGraph::readQueryFile(StrSet& qset, const std::string& fname) {
  std::string line;
  std::ifstream file(fname.c_str());

  while(std::getline(file, line)) {
    if(line.find("<") == std::string::npos) {
      qset.insert(line);
    }
  }
  file.close();
}

void TermGraph::rstrip(string& str, const char *s) {
  string::size_type pos = str.find_first_not_of(s);
  str.erase(0, pos);
}

void TermGraph::lstrip(string& str, const char *s) {
  string::size_type pos = str.find_last_not_of(s);
  str.erase(pos + 1);
}

void TermGraph::stripLine(string& str, const char *s) {
  rstrip(str, s);
  lstrip(str, s);
}

unsigned int TermGraph::split(const string& str, vector<std::string>& segs, const char *seps,
                                   const string::size_type lpos, const string::size_type rpos) {
  string::size_type prev = lpos; // previous delimiter position
  string::size_type cur = lpos; // current position of the delimiter
  string::size_type rbound = rpos == std::string::npos ? str.length()-1 : rpos;

  if (lpos >= rpos || lpos >= str.length() || rbound >= str.length())
    return 0;

  while (1) {
    cur = str.find_first_of(seps, prev);
    if (cur == std::string::npos || cur > rbound) {
      segs.push_back(str.substr(prev, rbound-prev+1));
      break;
    } else {
      if (cur != prev)
        segs.push_back(str.substr(prev, cur-prev));
      prev = cur + 1;
    }
  }
  return segs.size();
}
