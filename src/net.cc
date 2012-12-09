
#include "native/net.h"

namespace native {
namespace net {

#undef DBG
#define DBG(msg) DEBUG_PRINT("[net::Socket] " << msg)

Socket::Socket(detail::stream* handle, Server* server, bool allowHalfOpen)
: Stream(handle, true, true)
, socket_type_(SocketType::None)
, stream_(nullptr)
, server_(server)
, flags_(0)
, allow_half_open_(allowHalfOpen)
, connecting_(false)
, destroyed_(false)
, bytes_written_(0)
, bytes_read_(0)
, pending_write_reqs_()
, connect_queue_size_(0)
, connect_queue_()
, on_data_()
, on_end_()
, timeout_emitter_(new TimeoutEmitter(this))
{
  // register events
  registerEvent<event::connect>();
  registerEvent<event::data>();
  registerEvent<event::end>();
  registerEvent<event::timeout>();
  registerEvent<event::drain>();
  registerEvent<event::error>();
  registerEvent<event::close>();
  registerEvent<internal_destroy_event>(); // only for Server class

  // init socket
  init_socket(handle);
}

bool Socket::end(const Buffer& buffer)
{
  DBG("end");
  if(connecting_ && ((flags_ & FLAG_SHUTDOWNQUED) == 0))
  {
    DBG("queue shutdown");
    if(buffer.size()) write(buffer);
    writable(false);
    flags_ |= FLAG_SHUTDOWNQUED;
  }

  if(!writable()) return true;
  writable(false);

  if(buffer.size()) write(buffer);

  if(!readable())
  {
    DBG("destroy soon");
    destroySoon();
  }
  else
  {
    DBG("shutting down");
    flags_ |= FLAG_SHUTDOWN;

    stream_->on_complete([&](detail::resval r){
      DBG("on complete, shutting down");
      assert(flags_ & FLAG_SHUTDOWN);
      assert(!writable());

      if(destroyed_) return;

      if((flags_ & FLAG_GOT_EOF) || !readable())
      {
        destroy();
      }
    });
    detail::resval rv = stream_->shutdown();
    if(!rv)
    {
      destroy(Exception(rv));
      return false;
    }
  }
  return true;
}

bool Socket::end(const std::string& str, const detail::encoding_type& encoding, int fd)
{
  // TODO: what about 'fd'?
  return end(Buffer(str, encoding));
}


bool Socket::end()
{
  return end(Buffer(nullptr));
}

// TODO: these overloads are inherited but not used for Socket.
// bool Socket::write(const Buffer& buffer) { assert(false); return false; } //-> virtual bool write(const Buffer& buffer, write_callback_type callback=nullptr)
bool Socket::write(const std::string& str, const std::string& encoding, int fd) { assert(false); return false; }

// TODO: this is not inherited from Stream - a new overload.
// callback is invoked after all data is written.
bool Socket::write(const Buffer& buffer, std::function<void()> callback)
{
  DBG("write:" << std::endl << buffer.str());
  bytes_written_ += buffer.size();

  if(connecting_)
  {
    connect_queue_size_ += buffer.size();
    connect_queue_.push_back(std::make_pair(std::shared_ptr<Buffer>(new Buffer(buffer)), callback));
    return false;
  }

  timers::active(timeout_emitter_);

  if(!stream_) throw Exception("This socket is closed.");

  stream_->on_complete([=](detail::resval r) {
    DBG("on complete");
    if(destroyed_) return;

    if(!r)
    {
      destroy(r);
      return;
    }

    timers::active(timeout_emitter_);

    pending_write_reqs_--;
    if(pending_write_reqs_ == 0) emit<event::drain>();

    if(callback) callback();

    if((pending_write_reqs_== 0) && (flags_ & FLAG_DESTROY_SOON))
    {
      destroy();
    }
  });

  detail::resval rv = stream_->write(buffer.base(), 0, buffer.size());
  if(!rv)
  {
    destroy(rv);
    return false;
  }

  pending_write_reqs_++;

  return stream_->write_queue_size() == 0;
}

void Socket::destroy(Exception exception)
{
  DBG("destroy with error");
  destroy_(true, exception);
}

void Socket::destroy()
{
  DBG("destroy no error");
  destroy_(false, Exception());
}

void Socket::destroySoon()
{
  DBG("destroy soon");
  writable(false);
  flags_ |= FLAG_DESTROY_SOON;

  if(pending_write_reqs_ == 0)
  {
    destroy();
  }
}

void Socket::pause()
{
  DBG("pause");
  if(stream_)
  {
    stream_->read_stop();
    stream_->unref();
  }
}

void Socket::resume()
{
  DBG("resume");
  if(stream_)
  {
    stream_->read_start();
    stream_->ref();
  }
}

void Socket::setTimeout(unsigned int msecs, std::function<void()> callback)
{
  if(msecs > 0)
  {
    timers::enroll(timeout_emitter_, msecs);
    timers::active(timeout_emitter_);

    if(callback) once<event::timeout>(callback);
  }
  else // msecs == 0
      {
    timers::unenroll(timeout_emitter_);;
      }
}

bool Socket::setNoDelay()
{
  if(socket_type_ == SocketType::IPv4 || socket_type_ == SocketType::IPv6)
  {
    auto x = dynamic_cast<detail::tcp*>(stream_);
    assert(x);

    x->set_no_delay(true);
    return true;
  }

  return false;
}

bool Socket::setKeepAlive(bool enable, unsigned int initialDelay)
{
  if(socket_type_ == SocketType::IPv4 || socket_type_ == SocketType::IPv6)
  {
    auto x = dynamic_cast<detail::tcp*>(stream_);
    assert(x);

    x->set_keepalive(enable, initialDelay/1000);
    return true;
  }

  return false;
}

// TODO: only IPv4 supported.
bool Socket::address(std::string& ip_or_pipe_name, int& port)
{
  if(!stream_) return SocketType::None;

  switch(socket_type_)
  {
  case SocketType::IPv4:
  case SocketType::IPv6:
  {
    auto x = dynamic_cast<detail::tcp*>(stream_);
    assert(x);

    auto addr = x->get_sock_name();
    if(addr)
    {
      ip_or_pipe_name = addr->ip;
      port = addr->port;
      return true;
    }
  }
  return false;

  default:
    return false;
  }
}

std::string Socket::readyState() const
{
  if(connecting_) return "opening";
  else if(readable() && writable()) return "open";
  else if(readable() && !writable()) return "readOnly";
  else if(!readable() && writable()) return "writeOnly";
  else return "closed";
}

std::size_t Socket::bufferSize() const
{
  if(stream_)
  {
    return stream_->write_queue_size() + connect_queue_size_;
  }
  return 0;
}

std::string Socket::remoteAddress() const
{
  if(socket_type_ == SocketType::IPv4 || socket_type_ == SocketType::IPv6)
  {
    auto x = dynamic_cast<detail::tcp*>(stream_);
    assert(x);

    auto addr = x->get_peer_name();
    if(addr) return addr->ip;
  }

  return std::string();
}

int Socket::remotePort() const
{
  if(socket_type_ == SocketType::IPv4 || socket_type_ == SocketType::IPv6)
  {
    auto x = dynamic_cast<detail::tcp*>(stream_);
    assert(x);

    auto addr = x->get_peer_name();
    if(addr) return addr->port;
  }

  return 0;
}

// TODO: host name lookup (DNS) not supported
bool Socket::connect(const std::string& ip_or_path, int port, event::connect::callback_type callback)
{
  DBG("connect");
  // recreate socket handle if needed
  if(destroyed_ || !stream_)
  {
    detail::stream* stream = nullptr;
    if(isIP(ip_or_path)) stream = new detail::tcp;
    else stream = new detail::pipe;
    assert(stream);

    init_socket(stream);
  }

  // add listener
  on<event::connect>(callback);

  // refresh timer
  timers::active(timeout_emitter_);

  connecting_ = true;
  writable(true);

  if(socket_type_ == SocketType::IPv4 || socket_type_ == SocketType::IPv6)
  {
    // IPv4
    auto x = dynamic_cast<detail::tcp*>(stream_);
    assert(x);

    auto rv = x->connect(ip_or_path, port);
    if(rv)
    {
      stream_->on_complete([=](detail::resval r){
        DBG("on_complete");
        if(destroyed_) return;

        assert(connecting_);
        connecting_ = false;

        if(!r)
        {
          connect_queue_cleanup();
          destroy(r);
        }
        else
        {
          readable(stream_->is_readable());
          writable(stream_->is_writable());

          timers::active(timeout_emitter_);

          if(readable()) stream_->read_start();

          emit<event::connect>();

          if(connect_queue_.size())
          {
            for(connect_queue_type::value_type c : connect_queue_)
            {
              write(*c.first, c.second);
            }
            connect_queue_cleanup();
          }

          if(flags_ & FLAG_SHUTDOWNQUED)
          {
            flags_ &= ~FLAG_SHUTDOWNQUED;
            end();
          }
        }
      });
      return true;
    }
    else
    {
      destroy(rv);
      return false;
    }
  }
  else if(socket_type_ == SocketType::Pipe)
  {
    // pipe
    auto x = dynamic_cast<detail::pipe*>(stream_);
    assert(x);

    x->connect(ip_or_path);
    return true;
  }
  else
  {
    assert(false);
    return false;
  }
}

void Socket::init_socket(detail::stream* stream)
{
  stream_ = stream;
  if(stream_)
  {
    // set on_read callback
    stream_->on_read([&](const char* buffer, std::size_t offset, std::size_t length, detail::stream* pending, detail::resval r){
      DBG("on_read");
      on_read(buffer, offset, length, r);
    });

    // identify socket type
    if(stream_->uv_stream()->type == UV_TCP)
    {
      // TODO: cannot identify IPv6 socket type!
      socket_type_ = SocketType::IPv4;
    }
    else if(stream_->uv_stream()->type == UV_NAMED_PIPE)
    {
      socket_type_ = SocketType::Pipe;
    }
  }
}

void Socket::destroy_(bool failed, Exception exception)
{
  DBG("destroy");
  if(destroyed_) return;

  connect_queue_cleanup();

  readable(false);
  writable(false);

  timers::unenroll(timeout_emitter_);;

  // Because of C++ declration order, we cannot call Server class member functions here.
  // Instead, Server handles this using delegate.
  if(server_)
  {
    //server_->connections_--;
    //server_->emitCloseIfDrained();
    emit<internal_destroy_event>();
  }

  if(stream_)
  {
    stream_->close();
    stream_->on_read(nullptr);
    stream_ = nullptr;
  }

  process::nextTick([=](){
    DBG("finishing destroy");
    if(failed) emit<event::error>(exception);

    // TODO: node.js implementation pass one argument whether there was errors or not.
    //emit<event::close>(failed);
    emit<event::close>();
  });

  destroyed_ = true;
}

void Socket::connect_queue_cleanup()
{
  connecting_ = false;
  connect_queue_size_ = 0;
  connect_queue_.clear();
}

void Socket::on_read(const char* buffer, std::size_t offset, std::size_t length, detail::resval rv)
{
  DBG("on read");
  timers::active(timeout_emitter_);

  std::size_t end_pos = offset + length;
  if(buffer)
  {
    // TODO: implement decoding
    // ..
    if(haveListener<event::data>()) emit<event::data>(Buffer(&buffer[offset], length));

    bytes_read_ += length;

    if(on_data_) on_data_(buffer, offset, end_pos);
  }
  else
  {
    if(rv.code() == error::eof)
    {
      readable(false);

      assert(!(flags_ & FLAG_GOT_EOF));
      flags_ |= FLAG_GOT_EOF;

      if(!writable()) destroy();

      if(!allow_half_open_) end();
      if(haveListener<event::end>()) emit<event::end>();
      if(on_end_) on_end_();
    }
    else if(rv.code() == ECONNRESET)
    {
      destroy();
    }
    else
    {
      destroy(Exception(rv));
    }
  }
}

#undef DBG
#define DBG(msg) DEBUG_PRINT("[net::Server] " << msg)
Server::Server(bool allowHalfOpen)
: EventEmitter()
, stream_(nullptr)
, connections_(0)
, max_connections_(0)
, allow_half_open_(allowHalfOpen)
, backlog_(128)
, socket_type_(SocketType::None)
, pipe_name_()
{
  DBG("constructing");
  registerEvent<event::listening>();
  registerEvent<event::connection>();
  registerEvent<event::close>();
  registerEvent<event::error>();
}

// listen over TCP socket
bool Server::listen(int port, const std::string& host, event::listening::callback_type listeningListener)
{
  DBG("listen");
  return listen_(host, port, listeningListener);
}

// listen over unix-socket
bool Server::listen(const std::string& path, event::listening::callback_type listeningListener)
{
  DBG("listen");
  return listen_(path, 0, listeningListener);
}

// TODO: implement Socket::pause() function.
void Server::pause(unsigned int msecs) {
  DBG("pause");
}

int Server::address(std::string& ip_or_pipe_name, int& port)
{
  if(!stream_) return SocketType::None;

  switch(socket_type_)
  {
  case SocketType::IPv4:
  {
    auto x = dynamic_cast<detail::tcp*>(stream_);
    assert(x);

    auto addr = x->get_sock_name();
    if(addr)
    {
      ip_or_pipe_name = addr->ip;
      port = addr->port;
    }
    assert(addr->is_ipv4);
  }
  break;

  case SocketType::IPv6:
  {
    auto x = dynamic_cast<detail::tcp*>(stream_);
    assert(x);

    auto addr = x->get_sock_name();
    if(addr)
    {
      ip_or_pipe_name = addr->ip;
      port = addr->port;
    }
    assert(!addr->is_ipv4);
  }
  break;

  case SocketType::Pipe:
  {
    ip_or_pipe_name = pipe_name_;
  }
  break;
  }

  return socket_type_;
}

void Server::emitCloseIfDrained()
{
  DBG("close if drained");
  if(stream_ || connections_) return;

  process::nextTick([&](){ DBG("emitting close if drained"); emit<event::close>(); });
}

detail::stream* Server::create_server_handle(const std::string& ip_or_pipe_name, int port)
{
  detail::resval rv;
  if(isIPv4(ip_or_pipe_name))
  {
    auto handle = new detail::tcp();
    assert(handle);

    if(!handle->bind(ip_or_pipe_name, port))
    {
      handle->close();
      return nullptr;
    }

    socket_type_ = SocketType::IPv4;
    return handle;
  }
  else if(isIPv6(ip_or_pipe_name))
  {
    auto handle = new detail::tcp();
    assert(handle);

    if(!handle->bind6(ip_or_pipe_name, port))
    {
      handle->close();
      return nullptr;
    }

    socket_type_ = SocketType::IPv6;
    return handle;
  }
  else
  {
    auto handle = new detail::pipe();
    assert(handle);

    if(!handle->bind(ip_or_pipe_name))
    {
      handle->close();
      return nullptr;
    }

    socket_type_ = SocketType::Pipe;
    pipe_name_ = ip_or_pipe_name;
    return handle;
  }
}

bool Server::listen_(const std::string& ip_or_pipe_name, int port, event::listening::callback_type listeningListener)
{
  DBG("listen");
  if(listeningListener) on<event::listening>(listeningListener);

  if(!stream_)
  {
    stream_ = create_server_handle(ip_or_pipe_name, port);
    if(!stream_)
    {
      process::nextTick([&](){
        emit<event::error>(Exception("Failed to create a server socket (1)."));
      });
      return false;
    }
  }

  stream_->on_connection([&](detail::stream* s, detail::resval r){
    DBG("on_connection");
    if(!r)
    {
      emit<event::error>(Exception(r, "Failed to accept client socket (1)."));
    }
    else
    {
      if(max_connections_ && connections_ > max_connections_)
      {
        // TODO: where is "error" event?
            s->close(); // just close down?
            return;
      }

      auto socket = new Socket(s, this, allow_half_open_);
      assert(socket);

      socket->once<Socket::internal_destroy_event>([&](){
        DBG("internal_destroy_event");
        connections_--;
        emitCloseIfDrained();
      });
      socket->resume();

      connections_++;
      emit<event::connection>(socket);

      socket->emit<event::connect>();
    }
  });

  auto rv = stream_->listen(backlog_);
  if(!rv)
  {
    stream_->close();
    stream_ = nullptr;
    process::nextTick([&](){
      emit<event::error>(Exception(rv, "Failed to initiate listening on server socket (1)."));
    });
    return false;
  }

  process::nextTick([&](){
    emit<event::listening>();
  });
  return true;
}

void Server::close()
{
  DBG("close");
  if(!stream_)
  {
    throw Exception("Server is not running (1).");
  }
  DBG((stream_->is_writable()?"writable":"not writable"));
  stream_->close();
  stream_ = nullptr;
  emitCloseIfDrained();
}


Server* createServer(std::function<void(Server*)> callback, bool allowHalfOpen)
{
  auto x = new Server(allowHalfOpen);
  assert(x);

  process::nextTick([=](){
    callback(x);
  });

  return x;
}

Socket* createSocket(std::function<void(Socket*)> callback, bool allowHalfOpen)
{
  auto x = new Socket(nullptr, nullptr, allowHalfOpen);
  assert(x);

  if (callback) {
    process::nextTick([=](){
      callback(x);
    });
  }

  return x;
}

}  // namespace net
}  // namespace native
