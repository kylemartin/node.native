//// Copyright Joyent, Inc. and other Node contributors.
////
//// Permission is hereby granted, free of charge, to any person obtaining a
//// copy of this software and associated documentation files (the
//// "Software"), to deal in the Software without restriction, including
//// without limitation the rights to use, copy, modify, merge, publish,
//// distribute, sublicense, and/or sell copies of the Software, and to permit
//// persons to whom the Software is furnished to do so, subject to the
//// following conditions:
////
//// The above copyright notice and this permission notice shall be included
//// in all copies or substantial portions of the Software.
////
//// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
//// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
//// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
//// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
//// USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//#include <assert.h>
//#include <errno.h>
//#include <stdlib.h>
//#include <string.h>
//
#define CARES_STATICLIB

#include <string.h>
#include "ares.h"
//#include "node.h"
//#include "req_wrap.h"
#include "native/detail/tree.h"
//#include "uv.h"
//
#if defined(__OpenBSD__) || defined(__MINGW32__) || defined(_MSC_VER)
# include <nameser.h>
#else
# include <arpa/nameser.h>
#endif
//
//

#include "native/net.h"
#include "native/detail/req.h"
#include "native/detail/dns.h"

namespace native {
namespace detail {

//namespace node {
//
//namespace cares_wrap {
//
//using v8::Arguments;
//using v8::Array;
//using v8::Context;
//using v8::Function;
//using v8::Handle;
//using v8::HandleScope;
//using v8::Integer;
//using v8::Local;
//using v8::Null;
//using v8::Object;
//using v8::Persistent;
//using v8::String;
//using v8::Value;
//
//

//typedef class ReqWrap<uv_getaddrinfo_t> GetAddrInfoReqWrap;

class addr_req : public detail::req<uv_getaddrinfo_t> {
public:
  dns::on_complete_t on_complete_;
};

struct ares_task_t {
  UV_HANDLE_FIELDS
  ares_socket_t sock;
  uv_poll_t poll_watcher;
  RB_ENTRY(ares_task_t) node;
};


//static Persistent<String> oncomplete_sym;
static ares_channel ares_channel;
static uv_timer_t ares_timer;
static RB_HEAD(ares_task_list, ares_task_t) ares_tasks;


static int cmp_ares_tasks(const ares_task_t* a, const ares_task_t* b) {
  if (a->sock < b->sock) return -1;
  if (a->sock > b->sock) return 1;
  return 0;
}


RB_GENERATE_STATIC(ares_task_list, ares_task_t, node, cmp_ares_tasks)

/* This is called once per second by loop->timer. It is used to constantly */
/* call back into c-ares for possibly processing timeouts. */
static void ares_timeout(uv_timer_t* handle, int status) {
  assert(!RB_EMPTY(&ares_tasks));
  ares_process_fd(ares_channel, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
}


static void ares_poll_cb(uv_poll_t* watcher, int status, int events) {
  ares_task_t* task = container_of(watcher, ares_task_t, poll_watcher);

  /* Reset the idle timer */
  uv_timer_again(&ares_timer);

  if (status < 0) {
    /* An error happened. Just pretend that the socket is both readable and */
    /* writable. */
    ares_process_fd(ares_channel, task->sock, task->sock);
    return;
  }

  /* Process DNS responses */
  ares_process_fd(ares_channel,
                  events & UV_READABLE ? task->sock : ARES_SOCKET_BAD,
                  events & UV_WRITABLE ? task->sock : ARES_SOCKET_BAD);
}


static void ares_poll_close_cb(uv_handle_t* watcher) {
  ares_task_t* task = container_of(watcher, ares_task_t, poll_watcher);
  free(task);
}


/* Allocates and returns a new ares_task_t */
static ares_task_t* ares_task_create(uv_loop_t* loop, ares_socket_t sock) {
  ares_task_t* task = (ares_task_t*) malloc(sizeof *task);

  if (task == NULL) {
    /* Out of memory. */
    return NULL;
  }

  task->loop = loop;
  task->sock = sock;

  if (uv_poll_init_socket(loop, &task->poll_watcher, sock) < 0) {
    /* This should never happen. */
    free(task);
    return NULL;
  }

  return task;
}

/* Callback from ares when socket operation is started */
static void ares_sockstate_cb(void* data, ares_socket_t sock,
    int read, int write) {
  uv_loop_t* loop = (uv_loop_t*) data;
  ares_task_t* task;

  ares_task_t lookup_task;
  lookup_task.sock = sock;
  task = RB_FIND(ares_task_list, &ares_tasks, &lookup_task);

  if (read || write) {
    if (!task) {
      /* New socket */

      /* If this is the first socket then start the timer. */
      if (!uv_is_active((uv_handle_t*) &ares_timer)) {
        assert(RB_EMPTY(&ares_tasks));
        uv_timer_start(&ares_timer, ares_timeout, 1000, 1000);
      }

      task = ares_task_create(loop, sock);
      if (task == NULL) {
        /* This should never happen unless we're out of memory or something */
        /* is seriously wrong. The socket won't be polled, but the the query */
        /* will eventually time out. */
        return;
      }

      RB_INSERT(ares_task_list, &ares_tasks, task);
    }

    /* This should never fail. If it fails anyway, the query will eventually */
    /* time out. */
    uv_poll_start(&task->poll_watcher,
                  (read ? UV_READABLE : 0) | (write ? UV_WRITABLE : 0),
                  ares_poll_cb);

  } else {
    /* read == 0 and write == 0 this is c-ares's way of notifying us that */
    /* the socket is now closed. We must free the data associated with */
    /* socket. */
    assert(task &&
           "When an ares socket is closed we should have a handle for it");

    RB_REMOVE(ares_task_list, &ares_tasks, task);
    uv_close((uv_handle_t*) &task->poll_watcher, ares_poll_close_cb);

    if (RB_EMPTY(&ares_tasks)) {
      uv_timer_stop(&ares_timer);
    }
  }
}

static std::vector<std::string> HostentToAddresses(struct hostent* host) {
  std::vector<std::string> addresses;

  char ip[INET6_ADDRSTRLEN];
  for (int i = 0; host->h_addr_list[i]; ++i) {
    uv_inet_ntop(host->h_addrtype, host->h_addr_list[i], ip, sizeof(ip));
    addresses.emplace_back(ip);
  }

  return addresses;
}

static std::vector<std::string> HostentToNames(struct hostent* host) {
  std::vector<std::string> names;

  for (int i = 0; host->h_aliases[i]; ++i) {
    names.emplace_back(host->h_aliases[i]);
  }

  return names;
}

//static const char* AresErrnoString(int errorno) {
//  switch (errorno) {
//#define ERRNO_CASE(e) case ARES_##e: return #e;
//    ERRNO_CASE(SUCCESS)
//    ERRNO_CASE(ENODATA)
//    ERRNO_CASE(EFORMERR)
//    ERRNO_CASE(ESERVFAIL)
//    ERRNO_CASE(ENOTFOUND)
//    ERRNO_CASE(ENOTIMP)
//    ERRNO_CASE(EREFUSED)
//    ERRNO_CASE(EBADQUERY)
//    ERRNO_CASE(EBADNAME)
//    ERRNO_CASE(EBADFAMILY)
//    ERRNO_CASE(EBADRESP)
//    ERRNO_CASE(ECONNREFUSED)
//    ERRNO_CASE(ETIMEOUT)
//    ERRNO_CASE(EOF)
//    ERRNO_CASE(EFILE)
//    ERRNO_CASE(ENOMEM)
//    ERRNO_CASE(EDESTRUCTION)
//    ERRNO_CASE(EBADSTR)
//    ERRNO_CASE(EBADFLAGS)
//    ERRNO_CASE(ENONAME)
//    ERRNO_CASE(EBADHINTS)
//    ERRNO_CASE(ENOTINITIALIZED)
//    ERRNO_CASE(ELOADIPHLPAPI)
//    ERRNO_CASE(EADDRGETNETWORKPARAMS)
//    ERRNO_CASE(ECANCELLED)
//#undef ERRNO_CASE
//    default:
//      assert(0 && "Unhandled c-ares error");
//      return "(UNKNOWN)";
//  }
//}
//
//
//static void SetAresErrno(int errorno) {
//  HandleScope scope;
//  Handle<Value> key = String::NewSymbol("errno");
//  Handle<Value> value = String::NewSymbol(AresErrnoString(errorno));
//  Context::GetCurrent()->Global()->Set(key, value);
//}
//

class QueryWrap {
 public:
  typedef std::function<void(int, const std::vector<std::string>&, int)> on_complete_t;
  QueryWrap();
  virtual ~QueryWrap();
  on_complete_t GetOnComplete();
  void SetOnComplete(on_complete_t oncomplete);
  // Subclasses should implement the appropriate Send method.
  virtual int Send(const char* name);
  virtual int Send(const char* name, int family);
 protected:
  void* GetQueryArg();
  static void Callback(void *arg, int status, int timeouts,
      unsigned char* answer_buf, int answer_len);
  static void Callback(void *arg, int status, int timeouts,
      struct hostent* host);
  void CallOnComplete(std::vector<std::string> answer);
  void CallOnComplete(std::vector<std::string>, int family);
  void ParseError(int status);
  // Subclasses should implement the appropriate Parse method.
  virtual void Parse(unsigned char* buf, int len);
  virtual void Parse(struct hostent* host);
 private:
  on_complete_t on_complete_;
};

QueryWrap::QueryWrap() : on_complete_() {}

QueryWrap::~QueryWrap() {}

QueryWrap::on_complete_t QueryWrap::GetOnComplete() {
 return on_complete_;
}

void QueryWrap::SetOnComplete(on_complete_t oncomplete) {
 assert(oncomplete);
 on_complete_ = oncomplete;
}

// Subclasses should implement the appropriate Send method.
 int QueryWrap::Send(const char* name) {
 assert(0);
 return 0;
}

 int QueryWrap::Send(const char* name, int family) {
 assert(0);
 return 0;
}

void* QueryWrap::GetQueryArg() {
 return static_cast<void*>(this);
}

void QueryWrap::Callback(void *arg, int status, int timeouts,
   unsigned char* answer_buf, int answer_len) {
 QueryWrap* wrap = reinterpret_cast<QueryWrap*>(arg);

 if (status != ARES_SUCCESS) {
   wrap->ParseError(status);
 } else {
   wrap->Parse(answer_buf, answer_len);
 }

 delete wrap;
}

void QueryWrap::Callback(void *arg, int status, int timeouts,
   struct hostent* host) {
 QueryWrap* wrap = reinterpret_cast<QueryWrap*>(arg);

 if (status != ARES_SUCCESS) {
   wrap->ParseError(status);
 } else {
   wrap->Parse(host);
 }

 delete wrap;
}

void QueryWrap::CallOnComplete(std::vector<std::string> answer) {
 assert(on_complete_);
 on_complete_(0, answer, 0);
}

void QueryWrap::CallOnComplete(std::vector<std::string> answer, int family) {
 on_complete_(0, answer, family);
}

void QueryWrap::ParseError(int status) {
 assert(status != ARES_SUCCESS);
 // TODO: handle ares errors properly
// SetAresErrno(status);

 on_complete_(-1, std::vector<std::string>(), 0);
}

// Subclasses should implement the appropriate Parse method.
 void QueryWrap::Parse(unsigned char* buf, int len) {
 assert(0);
};

 void QueryWrap::Parse(struct hostent* host) {
 assert(0);
};

class QueryAWrap: public QueryWrap {
 public:
  int Send(const char* name) {
    ares_query(ares_channel, name, ns_c_in, ns_t_a, Callback, GetQueryArg());
    return 0;
  }

 protected:
  void Parse(unsigned char* buf, int len) {
    struct hostent* host;

    int status = ares_parse_a_reply(buf, len, &host, NULL, NULL);
    if (status != ARES_SUCCESS) {
      this->ParseError(status);
      return;
    }

    std::vector<std::string> addresses = HostentToAddresses(host);
    ares_free_hostent(host);

    this->CallOnComplete(addresses);
  }
};


class QueryAaaaWrap: public QueryWrap {
 public:
  int Send(const char* name) {
    ares_query(ares_channel,
               name,
               ns_c_in,
               ns_t_aaaa,
               Callback,
               GetQueryArg());
    return 0;
  }

 protected:
  void Parse(unsigned char* buf, int len) {
    struct hostent* host;

    int status = ares_parse_aaaa_reply(buf, len, &host, NULL, NULL);
    if (status != ARES_SUCCESS) {
      this->ParseError(status);
      return;
    }

    std::vector<std::string> addresses = HostentToAddresses(host);
    ares_free_hostent(host);

    this->CallOnComplete(addresses);
  }
};


class QueryCnameWrap: public QueryWrap {
 public:
  int Send(const char* name) {
    ares_query(ares_channel,
               name,
               ns_c_in,
               ns_t_cname,
               Callback,
               GetQueryArg());
    return 0;
  }

 protected:
  void Parse(unsigned char* buf, int len) {
    struct hostent* host;

    int status = ares_parse_a_reply(buf, len, &host, NULL, NULL);
    if (status != ARES_SUCCESS) {
      this->ParseError(status);
      return;
    }

    // A cname lookup always returns a single record but we follow the
    // common API here.
    std::vector<std::string> result(1);
    result[0] = host->h_name;
    ares_free_hostent(host);

    this->CallOnComplete(result);
  }
};

//
//class QueryMxWrap: public QueryWrap {
// public:
//  int Send(const char* name) {
//    ares_query(ares_channel, name, ns_c_in, ns_t_mx, Callback, GetQueryArg());
//    return 0;
//  }
//
// protected:
//  void Parse(unsigned char* buf, int len) {
//    struct ares_mx_reply* mx_start;
//    int status = ares_parse_mx_reply(buf, len, &mx_start);
//    if (status != ARES_SUCCESS) {
//      this->ParseError(status);
//      return;
//    }
//
//    Local<Array> mx_records = Array::New();
//    Local<String> exchange_symbol = String::NewSymbol("exchange");
//    Local<String> priority_symbol = String::NewSymbol("priority");
//    int i = 0;
//    for (struct ares_mx_reply* mx_current = mx_start;
//         mx_current;
//         mx_current = mx_current->next) {
//      Local<Object> mx_record = Object::New();
//      mx_record->Set(exchange_symbol, String::New(mx_current->host));
//      mx_record->Set(priority_symbol, Integer::New(mx_current->priority));
//      mx_records->Set(Integer::New(i++), mx_record);
//    }
//
//    ares_free_data(mx_start);
//
//    this->CallOnComplete(mx_records);
//  }
//};


class QueryNsWrap: public QueryWrap {
 public:
  int Send(const char* name) {
    ares_query(ares_channel, name, ns_c_in, ns_t_ns, Callback, GetQueryArg());
    return 0;
  }

 protected:
  void Parse(unsigned char* buf, int len) {
    struct hostent* host;

    int status = ares_parse_ns_reply(buf, len, &host);
    if (status != ARES_SUCCESS) {
      this->ParseError(status);
      return;
    }

    std::vector<std::string> names = HostentToNames(host);
    ares_free_hostent(host);

    this->CallOnComplete(names);
  }
};


class QueryTxtWrap: public QueryWrap {
 public:
  int Send(const char* name) {
    ares_query(ares_channel, name, ns_c_in, ns_t_txt, Callback, GetQueryArg());
    return 0;
  }

 protected:
  void Parse(unsigned char* buf, int len) {
    struct ares_txt_reply* txt_out;

    int status = ares_parse_txt_reply(buf, len, &txt_out);
    if (status != ARES_SUCCESS) {
      this->ParseError(status);
      return;
    }

    std::vector<std::string> txt_records;

    struct ares_txt_reply *current = txt_out;
    for (int i = 0; current; ++i, current = current->next) {
      txt_records.emplace_back(reinterpret_cast<char*>(current->txt));
    }

    ares_free_data(txt_out);

    this->CallOnComplete(txt_records);
  }
};


//class QuerySrvWrap: public QueryWrap {
// public:
//  int Send(const char* name) {
//    ares_query(ares_channel,
//               name,
//               ns_c_in,
//               ns_t_srv,
//               Callback,
//               GetQueryArg());
//    return 0;
//  }
//
// protected:
//  void Parse(unsigned char* buf, int len) {
//    HandleScope scope;
//
//    struct ares_srv_reply* srv_start;
//    int status = ares_parse_srv_reply(buf, len, &srv_start);
//    if (status != ARES_SUCCESS) {
//      this->ParseError(status);
//      return;
//    }
//
//    Local<Array> srv_records = Array::New();
//    Local<String> name_symbol = String::NewSymbol("name");
//    Local<String> port_symbol = String::NewSymbol("port");
//    Local<String> priority_symbol = String::NewSymbol("priority");
//    Local<String> weight_symbol = String::NewSymbol("weight");
//    int i = 0;
//    for (struct ares_srv_reply* srv_current = srv_start;
//         srv_current;
//         srv_current = srv_current->next) {
//      Local<Object> srv_record = Object::New();
//      srv_record->Set(name_symbol, String::New(srv_current->host));
//      srv_record->Set(port_symbol, Integer::New(srv_current->port));
//      srv_record->Set(priority_symbol, Integer::New(srv_current->priority));
//      srv_record->Set(weight_symbol, Integer::New(srv_current->weight));
//      srv_records->Set(Integer::New(i++), srv_record);
//    }
//
//    ares_free_data(srv_start);
//
//    this->CallOnComplete(srv_records);
//  }
//};
//

class GetHostByAddrWrap: public QueryWrap {
public:
  using QueryWrap::Send;
  int Send(const char* name) {
    int length, family;
    char address_buffer[sizeof(struct in6_addr)];

    if (uv_inet_pton(AF_INET, name, &address_buffer).code == UV_OK) {
      length = sizeof(struct in_addr);
      family = AF_INET;
    } else if (uv_inet_pton(AF_INET6, name, &address_buffer).code == UV_OK) {
      length = sizeof(struct in6_addr);
      family = AF_INET6;
    } else {
      return ARES_ENOTIMP;
    }

    ares_gethostbyaddr(ares_channel,
                       address_buffer,
                       length,
                       family,
                       Callback,
                       GetQueryArg());
    return 0;
  }

protected:
  void Parse(struct hostent* host) {
    this->CallOnComplete(HostentToNames(host));
  }
};

class GetHostByNameWrap: public QueryWrap {
public:
  using QueryWrap::Send;
  int Send(const char* name, int family) {
    ares_gethostbyname(ares_channel, name, family, Callback, GetQueryArg());
    return 0;
  }

 protected:
  void Parse(struct hostent* host) {
    this->CallOnComplete(HostentToAddresses(host), host->h_addrtype);
  }
};

template <class Wrap>
static resval Query(const std::string& name, QueryWrap::on_complete_t oncomplete) {
  Wrap* wrap = new Wrap();
  wrap->SetOnComplete(oncomplete);
  int r = wrap->Send(name.c_str());
  if (r) {
    return resval(r);
  }
  return resval();
}

template <class Wrap>
static resval QueryWithFamily(const std::string& name, int family, QueryWrap::on_complete_t oncomplete) {
  Wrap* wrap = new Wrap();
  wrap->SetOnComplete(oncomplete);
  int r = wrap->Send(name.c_str(), family);
  if (r) {
    return resval(r);
  }
  return resval();
}

resval dns::QueryGetHostByAddr(const std::string& name, on_complete_t oncomplete) {
  return Query<GetHostByAddrWrap>(name, oncomplete);
}

resval dns::QueryGetHostByName(const std::string& name, on_complete_t oncomplete) {
  return QueryWithFamily<GetHostByNameWrap>(name, AF_UNSPEC, oncomplete);
}
resval dns::QueryGetHostByName(const std::string& name, int family, on_complete_t oncomplete) {
  return QueryWithFamily<GetHostByNameWrap>(name, family, oncomplete);
}

void AfterGetAddrInfo(uv_getaddrinfo_t* req, int status, struct addrinfo* res) {
  addr_req* req_wrap = reinterpret_cast<addr_req*>(req->data);

  std::vector<std::string> results;

  struct addrinfo* address;
  int n = 0;
  char ip[INET6_ADDRSTRLEN];
  const char *addr;
  uv_err_t err;

  if (!status) {
    // Count the number of responses.
    for (address = res; address; address = address->ai_next) {
      n++;
    }

    // Create the response array.
    results = std::vector<std::string>(n);

    n = 0;

    address = res;
    while(address) {
      assert(address->ai_socktype == SOCK_STREAM);

      // Ignore random ai_family types.
      if (address->ai_family == AF_INET) {
        // Iterate over the IPv4 responses putting them in the results array.
        // Juggle pointers
        addr = (char*) &((struct sockaddr_in*) address->ai_addr)->sin_addr;
        err = uv_inet_ntop(address->ai_family,
                                    addr,
                                    ip,
                                    INET6_ADDRSTRLEN);
      } else if (address->ai_family == AF_INET6) {
        // Iterate over the IPv6 responses putting them in the results array.
        // Juggle pointers
        addr = (char*) &((struct sockaddr_in6*) address->ai_addr)->sin6_addr;
        err = uv_inet_ntop(address->ai_family,
                                    addr,
                                    ip,
                                    INET6_ADDRSTRLEN);
      }
      if (err.code != UV_OK)
        continue;

      results[n] = std::string(ip);
      n++;

      // Increment
      address = address->ai_next;
    }
  }

  uv_freeaddrinfo(res);

  assert(req_wrap->on_complete_);
  req_wrap->on_complete_(n, results, 0);
  delete req_wrap;
}

int dns::is_ip(const std::string& ip) {
  char address_buffer[sizeof(struct in6_addr)];
  if (uv_inet_pton(AF_INET, ip.c_str(), address_buffer).code == UV_OK) {
    return 4;
  }
  if (uv_inet_pton(AF_INET6, ip.c_str(), address_buffer).code == UV_OK) {
    return 6;
  }
  return 0;
}

int dns::get_addr_info(const std::string& hostname, int ipv
    , on_complete_t callback) {
  int fam = AF_UNSPEC;
  switch(ipv) {
  case net::SocketType::IPv6: {
    fam = AF_INET6;
  } break;
  case net::SocketType::IPv4: {
    fam = AF_INET;
  } break;
  default: break;
  }

  addr_req* req_wrap = new addr_req;

  req_wrap->on_complete_ = callback;

  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = fam;
  hints.ai_socktype = SOCK_STREAM;

  int r = uv_getaddrinfo(uv_default_loop(), &req_wrap->req_, AfterGetAddrInfo
      ,hostname.c_str(), nullptr, &hints);
  req_wrap->dispatched();
  if (r) {
    // TODO: need to setup a SetErrno equivalent
    // SetErrno(uv_last_error(uv_default_loop()));
    delete req_wrap;
    return detail::get_last_error();
  }

  return detail::resval();
}

void dns::initialize() {
  int r = ares_library_init(ARES_LIB_INIT_ALL);
  assert(r == ARES_SUCCESS);

  struct ares_options options;
  options.sock_state_cb = ares_sockstate_cb;
  options.sock_state_cb_data = uv_default_loop();

  /* We do the call to ares_init_option for caller. */
  r = ares_init_options(&ares_channel, &options, ARES_OPT_SOCK_STATE_CB);
  assert(r == ARES_SUCCESS);

  /* Initialize the timeout timer. The timer won't be started until the */
  /* first socket is opened. */
  uv_timer_init(uv_default_loop(), &ares_timer);
}

}  // namespace detail
}  // namespace native


