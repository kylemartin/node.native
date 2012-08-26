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

#ifndef __DEBUG_HELPERS__
#define __DEBUG_HELPERS__
#define DBG(msg) std::cerr << __FILE__ << ":" << __LINE__ << "> " << msg << std::endl;
#define CRUMB() std::cerr << __FILE__ << ":" << __LINE__ << "> " << __FUNCTION__ << std::endl;
#endif

#endif
