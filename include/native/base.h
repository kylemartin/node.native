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

#define DEBUG_PRINT(msg) do { std::cerr << __FILE__ << ":" << __LINE__ << "> " << msg << std::endl << std::flush;  } while(0);
#define DBG(msg) DEBUG_PRINT(msg)
#define CRUMB() DBG(__FUNCTION__)
#define TRACE(exp) DBG(#exp) exp;

#else
#define DEBUG_PRINT(msg)
#define DBG(msg)
#define CRUMB()
#define TRACE(exp) exp;
#endif


#ifndef offset_of
// g++ in strict mode complains loudly about the system offsetof() macro
// because it uses NULL as the base address.
# define offset_of(type, member) \
  ((intptr_t) ((char *) &(((type *) 8)->member) - 8))
#endif

#ifndef container_of
# define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - offset_of(type, member)))
#endif

#endif // __BASE_H__

