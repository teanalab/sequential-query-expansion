/* -*- C++ -*- */

#include "StrUtils.hpp"

// Author: Alexander Kotov (UIUC), 2011
// $Id: StrUtils.cpp,v 1.1 2011/04/20 19:30:37 akotov2 Exp $

void rStripStr(std::string& str, const char *s) {
  std::string::size_type pos = str.find_first_not_of(s);
  str.erase(0, pos);
}

void lStripStr(std::string& str, const char *s) {
  std::string::size_type pos = str.find_last_not_of(s);
  str.erase(pos + 1);
}

void stripStr(std::string& str, const char *s) {
  rStripStr(str, s);
  lStripStr(str, s);
}

unsigned int splitStr(const std::string& str, StrVec& segs, const char *seps,
                      const std::string::size_type lpos, const std::string::size_type rpos) {
  std::string::size_type prev = lpos; // previous delimiter position
  std::string::size_type cur = lpos; // current position of the delimiter
  std::string::size_type rbound = rpos == std::string::npos ? str.length()-1 : rpos;

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
