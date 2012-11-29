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

#if defined(DEBUG)

#define DEBUG_PRINT(msg) do { std::cerr << __FILE__ << ":" << __LINE__ << "> " << msg << std::endl; } while(0);
#define DBG(msg) DEBUG_PRINT(msg)
#define CRUMB() DBG(__FUNCTION__)
#define TRACE(exp) DBG(#exp) exp;

#else
#define DEBUG_PRINT(msg)
#define DBG(msg)
#define CRUMB()
#define TRACE(exp) exp;
#endif

#endif
