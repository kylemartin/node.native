#ifndef __BASE_H__
#define __BASE_H__

#include <cassert>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <functional>
#include <map>
#include <algorithm>
#include <list>
#include <set>
#include <tuple>
#include <iostream>

#ifdef DEBUG
#define DBG(msg) std::cerr << __FILE__ << ":" << __LINE__ << "> " << msg << std::endl;
#define CRUMB() std::cerr << __FILE__ << ":" << __LINE__ << "> " << __PRETTY_FUNCTION__ << std::endl;
#define TRACE(exp) DBG(#exp) exp;
#else
#define DBG(msg)
#define CRUMB()
#define TRACE(exp) exp;
#endif

#endif
