#ifndef __NET_H__
#define __NET_H__

#include "base.h"
#include "detail.h"
#include "stream.h"
#include "error.h"
#include "process.h"
#include "timers.h"

namespace native {
using detail::Buffer;
/**
 *  Network classes and functions are defined under native::net namespace.
 */
namespace net {
/**
 *  @brief Gets the version of IP address format.
 *
 *  @param input    Input IP string.
 *
 *  @retval 4       Input string is a valid IPv4 address.
 *  @retval 6       Input string is a valid IPv6 address.
 *  @retval 0       Input string is not a valid IP address.
 */
inline int isIP(const std::string& input) {
  return detail::get_ip_version(input);
}

/**
 *  @brief Tests if the input string is valid IPv4 address.
 *
 *  @param input    Input IP string.
 *
 *  @retval true    Input string is a valid IPv4 address.
 *  @retval false    Input string is not a valid IPv4 address.
 */
inline bool isIPv4(const std::string& input) {
  return isIP(input) == 4;
}

/**
 *  @brief Test if the input string is valid IPv6 address.
 *
 *  @param input    Input IP string.
 *
 *  @retval true    Input string is a valid IPv6 address.
 *  @retval false    Input string is not a valid IPv6 address.
 */
inline bool isIPv6(const std::string& input) {
  return isIP(input) == 6;
}

/**
 *  Represents the type of socket stream.
 */
struct SocketType {
  enum {
    /**
     *  IPv4 socket.
     */
    IPv4,
    /**
     *  IPv6 socket.
     */
    IPv6,
    /**
     *  Unix pipe socket.
     */
    Pipe,
    /**
     *  Invalid type of socket.
     */
    None
  };
};

class Server;

/**
 *  @brief Socket class.
 *
 *	Emits events:
 *	  connect
 *	  data
 *	  end
 *	  timeout
 *	  drain
 *	  error
 *	  close
 *
 *  Socket class represents network socket streams.
 */
class Socket: public Stream {
  friend class Server;
  friend Socket* createSocket(std::function<void(Socket*)>, bool);

  static const int FLAG_GOT_EOF = 1 << 0;
  static const int FLAG_SHUTDOWN = 1 << 1;
  static const int FLAG_DESTROY_SOON = 1 << 2;
  static const int FLAG_SHUTDOWNQUED = 1 << 3;

protected:
  /**
   *  @brief Socket constructor.
   *
   *  @param handle           Pointer to native::detail::stream object.
   *  @param server           Pointer to native::net::Server object.
   *  @param allowHalfOpen    If true, ...
   *
   */
  Socket(detail::stream* handle = nullptr, Server* server = nullptr,
      bool allowHalfOpen = false);

  /**
   *  @brief Socket destructor.
   */
  virtual ~Socket();

public:
  /**
   *  @brief Sends the last data and close the socket stream.
   *
   *  @param buffer       Sending data.
   *
   *  @retval true        The data was sent and the socket was closed successfully.
   *  @retval false       There was an error while closing the socket stream.
   */
  virtual bool end(const Buffer& buffer);

  /**
   *  @brief Sends the last data and close the socket stream.
   *
   *  @param str          Sending string.
   *  @param encoding     Encoding name of the string.
   *  @param fd           Not implemented.
   *
   *  @retval true        The data was sent and the socket was closed successfully.
   *  @retval false       There was an error while closing the socket stream.
   */
  virtual bool end(const std::string& str,
      const detail::encoding_type& encoding = detail::et_ascii, int fd = -1);

  /**
   *  @brief Close the socket stream.
   *
   *  @retval true        The socket was closed successfully.
   *  @retval false       There was an error while closing the socket stream.
   */
  virtual bool end();

  // TODO: these overloads are inherited but not used for Socket.
  // virtual bool write(const Buffer& buffer) { assert(false); return false; }
  // virtual bool write(const Buffer& buffer, write_callback_type callback=nullptr)
  virtual bool write(const std::string& str, const std::string& encoding,
      int fd);

  // TODO: this is not inherited from Stream - a new overload.
  // callback is invoked after all data is written.
  /**
   *  @brief Writes data to the stream.
   *
   *  @param buffer       The data buffer.
   *  @param callback     Callback object that will be invoked after all the data is written.
   *
   *  @retval true        The request was successfully initiated.
   *  @retval false       The data was queued or the request was failed.
   */
  virtual bool write(const Buffer& buffer, std::function<void()> callback =
      nullptr);

  /**
   *  @brief Destorys the socket stream.
   *
   *  You should call this function only when there was an error.
   *
   *  @param exception        Exception object.
   */
  virtual void destroy(Exception exception);

  /**
   *  @brief Destorys the socket stream.
   *
   *  You should call this function only when there was an error.
   */
  virtual void destroy();

  /**
   *  @brief Destorys the socket stream after the write queue is drained.
   *
   *  You should call this function only when there was an error.
   */
  virtual void destroySoon();

  /**
   *  @brief Pauses reading from the socket stream.
   */
  virtual void pause();

  /**
   *  @brief Resumes reading from the socket stream.
   */
  virtual void resume();

  /**
   *  @brief Sets time-out callbacks.
   *
   *  @param msecs        Amount of time for the callback in milliseconds.
   *  @param callback     If the socket stream is not active for the specified time amount, this callback object will be invoked.
   */
  void setTimeout(unsigned int msecs, std::function<void()> callback = nullptr);

  bool setNoDelay();

  bool setKeepAlive(bool enable, unsigned int initialDelay = 0);

  // TODO: only IPv4 supported.
  bool address(std::string& ip_or_pipe_name, int& port);

  std::string readyState() const;

  std::size_t bufferSize() const;

  std::string remoteAddress() const;

  int remotePort() const;

  // TODO: host name lookup (DNS) not supported
  bool connect(const std::string& ip_or_path, int port = 0,
      event::connect::callback_type callback = nullptr);

  /**
   *  @brief Returns the number of bytes written to the socket stream.
   *
   *  @return     The number of bytes written.
   */
  std::size_t bytesWritten() const {
    return bytes_written_;
  }

  /**
   *  @brief Returns the number of bytes read from the socket stream.
   *
   *  @return     The number of bytes read.
   */
  std::size_t bytesRead() const {
    return bytes_read_;
  }

  detail::stream* stream() {
    return stream_;
  }
  const detail::stream* stream() const {
    return stream_;
  }

  bool connected() {
    // TODO: figure out better way to determine connected state
    return (stream_ && !connecting_ && !destroyed_ && (readable() || writable()));
  }
protected:
  void init_socket(detail::stream* stream);
private:

  void destroy_(bool failed, Exception exception);

  void connect_queue_cleanup();

  void on_read(const char* buffer, std::size_t offset, std::size_t length,
      detail::resval rv);

private:
  int socket_type_;

  Server* server_;

  int flags_;
  bool allow_half_open_;
  bool connecting_;
  bool destroyed_;

  std::size_t bytes_written_;
  std::size_t bytes_read_;
  std::size_t pending_write_reqs_;

  std::size_t connect_queue_size_;
  typedef std::vector<std::pair<std::shared_ptr<Buffer>, std::function<void()>>> connect_queue_type;
  connect_queue_type connect_queue_;

  std::function<void(const char*, std::size_t, std::size_t)> on_data_;
  std::function<void()> on_end_;

  std::shared_ptr<native::TimeoutHandler> timeout_emitter_;

  // only for Server class
  struct internal_destroy_event: public util::callback_def<> {
  };
};

class Server: public EventEmitter {
  friend Server* createServer(std::function<void(Server*)>, bool);

protected:
  Server(bool allowHalfOpen = false);

  virtual ~Server() {
  }

public:
  // listen over TCP socket
  bool listen(int port, const std::string& host = std::string("0.0.0.0"),
      event::listening::callback_type listeningListener = nullptr);

  // listen over unix-socket
  bool listen(const std::string& path,
      event::listening::callback_type listeningListener = nullptr);

  // TODO: implement Socket::pause() function.
  void pause(unsigned int msecs);

  void close();

  int address(std::string& ip_or_pipe_name, int& port);

  std::size_t maxConnections() const {
    return max_connections_;
  }
  void maxConnections(std::size_t value) {
    max_connections_ = value;
  }

  std::size_t connections() const {
    return connections_;
  }

protected:
  void emitCloseIfDrained();

  detail::stream* create_server_handle(const std::string& ip_or_pipe_name,
      int port);

  bool listen_(const std::string& ip_or_pipe_name, int port,
      event::listening::callback_type listeningListener);

public:
  detail::stream* stream_;
  std::size_t connections_;
  std::size_t max_connections_;
  bool allow_half_open_;
  int backlog_;
  int socket_type_;
  std::string pipe_name_;
};

Server* createServer(std::function<void(Server*)> callback, bool allowHalfOpen =
    false);

Socket* createSocket(std::function<void(Socket*)> callback = nullptr,
    bool allowHalfOpen = false);
}
}

#endif
