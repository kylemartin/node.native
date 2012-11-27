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

#if defined(DEBUG) && defined(DEBUG_ENABLED)
#define DEBUG_PREFIX " "
#define DBG(msg) std::cerr << __FILE__ << ":" << __LINE__ << ">" << DEBUG_PREFIX << msg << std::endl;
#define CRUMB() std::cerr << __FILE__ << ":" << __LINE__ << ">" << DEBUG_PREFIX << "" << __FUNCTION__ << std::endl;
#define TRACE(exp) DBG(#exp) exp;
#else
#define DBG(msg)
#define CRUMB()
#define TRACE(exp) exp;
#endif

#endif
