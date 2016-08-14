/* -*- C++ -*- */

// Author: Alexander Kotov (UIUC), 2011
// $Id: StrUtils.hpp,v 1.1 2011/04/20 19:30:37 akotov2 Exp $

#ifndef STRUTILS_HPP_
#define STRUTILS_HPP_

#include <string>
#include <vector>

#define SPACE_CHARS " \t\n"

typedef std::vector<std::string> StrVec;

void rStripStr(std::string& str, const char *s);

void lStripStr(std::string& str, const char *s);

void stripStr(std::string& str, const char *s);

unsigned int splitStr(const std::string& str, StrVec& segs, const char *seps, const std::string::size_type lpos=0,
                      const std::string::size_type rpos=std::string::npos);

#endif /* STRUTILS_HPP_ */
