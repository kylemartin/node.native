#ifndef __HTTP_H__
#define __HTTP_H__

#include <sstream>
#include "base.h"
#include "detail.h"
#include "events.h"
#include "net.h"
#include "process.h"

/*
# HTTP #

Sockets are created by a Server for incoming connections or by a
ClientRequest.
Parser factory manages pool of http_parser_context. Each Parser takes
ownership of a Socket and begins to read and feed data to the context.
Users of a Parser register callbacks for allocating and handling an
IncomingMessage once headers have been received. For servers, a
ServerRequest is allocated, for clients, a ClientResponse. Users can
attach data event handlers to the ServerRequest or ClientResponse to
receive post or body data respectively.

Since HTTP request/response are syntactically similar, the Message class
represents common features of both. It holds a status line, sets of leading
and trailing headers, and an optional body. A request/response flag is set by
Message subclasses to parse status line accordingly.
IncomingMessage and OutgoingMessage descend from Message and encapsulate the
differences between incoming and outgoing message traffic, independent of the
type of message (incoming request, incoming response, outgoing request,
outgoing response).
IncomingMessage/OutgoingMessage create request/response Parser to handle
data read from Socket. Message defines callbacks registered on Parser to handle
status line, headers, etc. as they are parsed.

 */
namespace native
{
    namespace http
    {
		// forward declarations
    	class IncomingMessage;
    	class OutgoingMessage;
        class ServerRequest;
        class ServerResponse;
        class ClientRequest;
        class ClientResponse;
    }

	namespace event { namespace http {
		namespace client {
			struct connect : public util::callback_def<native::http::ClientResponse*, net::Socket*, const Buffer&> {};
			struct upgrade : public util::callback_def<native::http::ClientResponse*, net::Socket*, const Buffer&> {};
            struct response : public util::callback_def<native::http::ClientResponse*> {};
		}

		namespace server {
			struct connect : public util::callback_def<native::http::ServerRequest*, net::Socket*, const Buffer&> {};
			struct upgrade : public util::callback_def<native::http::ServerRequest*, net::Socket*, const Buffer&> {};
            struct request : public util::callback_def<native::http::ServerRequest*, native::http::ServerResponse*> {};
            struct checkContinue : public util::callback_def<native::http::ServerRequest*, native::http::ServerResponse*> {};
		}
		struct socket : public util::callback_def<net::Socket*> {};
		struct Continue : public util::callback_def<>{};
		struct finish : public util::callback_def<> {};
	}}

    namespace http {

		/**
		 * Factory for Parser instances managing a parsing context.
		 *
		 * Allocates a pool of parser contexts for improving performance.
		 * Parser instances expect to process incoming messages from clients
		 * or servers. IncomingMessage instances are generated based on received
		 * headers and passed on to the on_incoming callback to determine if the
		 * body needs to be parsed. The on_incoming callback should be
		 * registered before starting to receive a message and implements the
		 * the logic for a handling a request/response.
		 */
		class Parser {
		private:
			/*
			 * This is the onIncoming callback which is specified for handling
			 * request and response IncomingMessages. This gets called from
			 * on_handle_headers_complete to determine if we should skip the body
			 * and to pass the IncomingMessage constructed from the headers,
			 * which can in turn be passed in a request/response event so the
			 * user can register event handlers on the IncomingMessage.
			 */
			typedef std::function<void(IncomingMessage*)> on_incoming_type;
			on_incoming_type on_incoming_;
			detail::http_parser_context context_;
			net::Socket* socket_;
			IncomingMessage* incoming_;

			/*
			 * This is a callback that must be registered to allow construction
			 * of classes derived from IncomingMessage. This is necessary
			 * because we want to emit events on the derived type. It might
			 * be possible/better to do this another way.
			 */
			typedef std::function<IncomingMessage*(net::Socket*)> alloc_incoming_type;
			alloc_incoming_type alloc_incoming_;

		private:
			Parser(http_parser_type type, net::Socket* socket)
				: on_incoming_()
				, context_(type)
				, socket_(socket)
				, incoming_(nullptr)
				, alloc_incoming_(nullptr)
			{
				// wrap callbacks in closures
				context_.on_headers_complete(
					[this](const native::detail::http_message& result){
						on_headers_complete(result);
					}
				);
				context_.on_body(
					[this](const char* buf, size_t off, size_t len){
						on_body(buf, off, len);
					}
				);
				context_.on_message_complete(
					[this](const native::detail::http_message& result){
						on_message_complete(result);
					}
				);
				context_.on_error(
					[this](const native::Exception& e){
						on_error(e);
					}
				);

				socket->on<native::event::data>([=](const Buffer& buf) {
					CRUMB();
						if(context_.feed_data(buf.base(), 0, buf.size()))
						{
							CRUMB();
							// parse end
							socket->pause();
						}
						else
						{
							CRUMB();
							// more reads required: keep reading!
						}
				});

				socket->on<native::event::end>([=](){
					CRUMB();
					// EOF
					if(!context_.is_finished())
					{
						// HTTP request was not properly parsed.
					}
				});

				socket->on<native::event::error>([=](const native::Exception& e){
					CRUMB();
				});

				socket->on<native::event::close>([=](){
					CRUMB();
				});
			}
		public:
			typedef std::shared_ptr<Parser> ptr;
			/**
			 * Create a parser of the specified type attached to the given socket
			 * which invokes the given callback with an IncomingMessage instance.
			 *
			 * @param socket
			 * @param type
			 * @param callback
			 * @return
			 */
			static Parser* create(
					http_parser_type type,
					net::Socket* socket,
					on_incoming_type callback = nullptr)
			{
				CRUMB();
				Parser* parser(new Parser(type, socket));

				if (callback) parser->on_incoming(callback);

				return parser;
			}

			void on_incoming(on_incoming_type callback) {
				on_incoming_ = callback;
			}

			void alloc_incoming(alloc_incoming_type callback) {
				alloc_incoming_ = callback;
			}
		private:
			/**
			 * Process headers and setup an IncomingMessage to pass to the
			 * handle_incoming callback
			 * @return
			 */
			int on_headers_complete(const native::detail::http_message& result); // Implemented after IncomingMessage defined

			int on_body(const char* buf, size_t off, size_t len) {
				CRUMB();
				return 0;
			}

			int on_message_complete(const native::detail::http_message& result);

			int on_error(const native::Exception& e) {
				CRUMB();
				return 0;
			}
		};

		/**
		 * This is the base class for incoming http messages (requests or
		 * responses for servers or clients respectively). These will be
		 * created by a Parser after parsing headers and will be passed to a
		 * request/response handler for further processing and registering
		 * event listeners. It implements the ReadableStream interface.
		 *
		 * Emits the following events:
		 *   data  - when a chunk of the body is received
		 *   end   - the body is finished and no more data events to come
		 *   close - the connection was terminated before finishing
		 *   error - there was an error with the connection or HTTP parsing
		 */
		class IncomingMessage : public EventEmitter
		{
		protected:
			net::Socket* socket_;
			bool complete_;
			bool readable_;
			bool paused_;
			detail::http_message::headers_type pendings_;
			bool endEmitted_;
			detail::http_message message_;

		public:
			IncomingMessage(net::Socket* socket)
			: socket_(socket)
			, complete_(false)
			, readable_(true)
			, paused_(false)
			, pendings_()
			, endEmitted_(false)
			, message_()
			{
				registerEvent<native::event::end>();
			}

//			IncomingMessage(const IncomingMessage& o)
//			: socket_(o.socket_)
//			, statusCode_(o.statusCode_)
//			, httpVersion_(o.httpVersion_)
//			, httpVersionMajor_(o.httpVersionMajor_)
//			, httpVersionMinor_(o.httpVersionMinor_)
//			, complete_(o.complete_)
//			, headers_(o.headers_)
//			, trailers_(o.trailers_)
//			, readable_(o.readable_)
//			, paused_(o.paused_)
//			, pendings_(o.pendings_)
//			, endEmitted_(o.endEmitted_)
//			, url_(o.url_)
//			, method_(o.method_)
//			{}

			// TODO: add move constructor

			/*
			 * Accessors
			 */

			net::Socket* socket() { return socket_; }
			int statusCode() { return message_.status(); }
			std::string httpVersion() { return message_.version_string(); }
			int httpVersionMajor() { return message_.version_major(); }
			int httpVersionMinor() { return message_.version_minor(); }

			void destroy(const Exception& e) {
				socket_->destroy(e);
			}

			// TODO: handle encoding
			//void setEncoding();

			void pause() {}
			void resume() {}
			void _emitPending(std::function<void()> callback) {}
			void _emitData(const Buffer& buf) {}
			void _emitEnd() {}
			void _addHeaderLine(const std::string& field, const std::string& value) {}

			void end() {
				CRUMB();
				if (haveListener<native::event::end>()) {
					CRUMB();
					emit<native::event::end>();
				}
			}

		};


		inline int Parser::on_headers_complete(const native::detail::http_message& result) {
			CRUMB();
			if (!alloc_incoming_ || !on_incoming_) return 0;

			incoming_ = alloc_incoming_(socket_);

			on_incoming_(incoming_);

			return 0;
		}

		inline int Parser::on_message_complete(const native::detail::http_message& result) {
			CRUMB();
			if (incoming_) {
				incoming_->end();
			}
			return 0;
		}

		/**
		 * This is the base class for outgoing messages (responses or requests
		 * for servers or clients respectively).
		 * It implements the WritableStream interface
		 *
		 * Events emitted:
		 * 	 close - indicates connection was terminated before calling end()
		 */
		class OutgoingMessage : public EventEmitter
		{
		protected:
			net::Socket* socket_;
			Buffer output_;
			// TODO: handle output encodings
			bool writable_;
			bool last_;
			bool chunkedEncoding_;
			bool useChunkedEncodingByDefault_;
			bool sendDate_;
			bool hasBody_;
		  bool expectContinue_;
		  bool sent100_;
		  bool shouldKeepAlive_;
			Buffer trailer_;
			bool finished_;
			detail::http_message message_;
			Buffer header_;
			bool headerSent_;
		public:
			OutgoingMessage(net::Socket* socket_)
				: socket_(socket_),
				  output_(),
				  writable_(true),
				  last_(false),
				  chunkedEncoding_(false),
				  useChunkedEncodingByDefault_(true),
				  sendDate_(false),
				  hasBody_(false),
		      expectContinue_(false),
		      sent100_(false),
		      shouldKeepAlive_(false),
				  trailer_(),
				  finished_(false),
				  message_(),
				  header_(),
				  headerSent_(false)
			{}

			void destroy(const Exception& e) {
				socket_->destroy(e);
			}

			// TODO: handle encoding
			void _send(const Buffer& buf);

			// TODO: handle encoding
			void _writeRaw(const Buffer& buf);

			// TODO: handle encoding
			void _buffer(const Buffer& buf);

			/**
			 *
			 * @param firstLine
			 * @param headers
			 */
			void _storeHeader(const std::string& firstLine, const detail::http_message::headers_type& headers);

			void setHeader(const std::string& name, const std::string& value);

			std::string getHeader(const std::string& name) {
				return "";
			}

			void removeHeader(const std::string& name) {}

			detail::http_message::headers_type _renderHeaders() {
				return detail::http_message::headers_type();
			}

			// TODO: handle encoding
			void write(const Buffer& buf) {}

			void addTrailers(const detail::http_message::headers_type& headers) {}

			bool end() { return false; }
			/**
			 * Signal the end of the outgoing message
			 * @param buf
			 * @return
			 */
			bool end(const Buffer& buf) {
            	CRUMB();
				return false;
#if 0
				// TODO: handle encoding
				if (finished_) { return false; }
				if (!_header) { // headers not yet sent
					// _storeHeader not yet called
					_implicitHeader();
				}
				if (data && !_hasBody) {
					// This type of message should not have a body
					data.clear();
				}

				bool hot = _headerSent == false
						&& data.size() > 0
						&& output.size() == 0
						&& socket_
						&& socket_->writable()
						&& socket_->_httpMessage == this;

				if (hot) {
					// Hot path. They're doing
					// 	 res.writeHead()
					//	 res.end(blah)
					// HACKY.

					if (chunkedEncoding_) {
						// TODO: do this without stringstream
						std::stringstream ss;
						ss << _header << std::hex << data.size() << "\r\n"
							<< data << "\r\n0\r\n"
							<< _trailer << "\r\n";
						ret - socket_->write(ss.str());
					} else {
						ret = socket_->write(header_ + data);
					}
					_headerSent = true;
				} else if (data.size() > 0) {
					// Normal body write
					ret = write(data);
				}

				if (!hot) {
					if (chunkedEncoding_) {
						ret = _send("0\r\n" + _trailer + "\r\n");
					} else {
						// Force a flush, HACK.
						ret = _send("");
					}
				}

				_finished = true;
				if (output.size() == 0 && socket_->_httpMessage == this) {
					_finish();
				}

				return ret;
#endif
			}

			void _finish() {
			  assert(socket_);
			  // TODO: Port DTrace
	//			  if (this instanceof ServerResponse) {
	//				DTRACE_HTTP_SERVER_RESPONSE(this.connection);
	//			  } else {
	//				assert(this instanceof ClientRequest);
	//				DTRACE_HTTP_CLIENT_REQUEST(this, this.connection);
	//			  }
			  emit<native::event::http::finish>();
			}

			void _flush() {}
		};

        class Server;

        typedef std::map<std::string, std::string, util::text::ci_less> headers_type;


        class ServerRequest : public IncomingMessage
        {
            friend class Server;

        protected:
            ServerRequest(const IncomingMessage& msg)
                : IncomingMessage(msg) // TODO: use move constructor
            {
            	CRUMB();
                assert(socket_);

//                registerEvent<native::event::data>();
//                registerEvent<native::event::end>();
//                registerEvent<native::event::close>();
//
//                socket_->on<native::event::data>([this](const Buffer& buffer){ emit<native::event::data>(buffer); });
//                socket_->on<native::event::end>([this](){ emit<native::event::end>(); });
//                socket_->on<native::event::close>([this](){ emit<native::event::close>(); });
            }

            virtual ~ServerRequest()
            {}
        public:
            http_method method() {
            	return message_.method();
            }
            detail::url_obj url() {
            	return message_.url();
            }
        };

        class ServerResponse : public OutgoingMessage
        {
            friend class Server;

        protected:
            ServerResponse(net::Socket* socket)
                : OutgoingMessage(socket)
            {
            	CRUMB();
                assert(socket_);

                registerEvent<native::event::close>();
                registerEvent<native::event::error>();

                socket_->on<native::event::close>([this](){ emit<native::event::close>(); });
            }

            virtual ~ServerResponse()
            {}

        private:

			virtual void _implicitHeader() {};

        public:
            void writeContinue() {}
            void writeHead(int statusCode, const std::string& reasonPhrase, const headers_type& headers);
            void writeHead(int statusCode, const headers_type& headers);

            int statusCode() const { return 0; }
            void statusCode(int value) {} // calling this after response was sent is error

            void setHeader(const std::string& name, const std::string& value) {}
            std::string getHeader(const std::string& name, const std::string& default_value=std::string()) { return default_value; }
            bool getHeader(const std::string& name, std::string& value) { return false; }
            bool removeHeader(const std::string& name) { return false; }

            void write(const Buffer& data)
            {
            	CRUMB();
                socket_->write(data);
            }

            void write(const std::string& data)
            {
              CRUMB();
              socket_->write(Buffer(data));
            }

            void addTrailers(const headers_type& headers) {}

            void end(const Buffer& data)
            {
            	CRUMB();
                assert(socket_);

                if(socket_->end(data))
                {
                	CRUMB();
                    socket_ = nullptr;
                }
                else
                {
                	CRUMB();
                    emit<native::event::error>(Exception("Failed to close the socket."));
                }
            }

            void end()
            {
              end(Buffer(nullptr));
            }
        };

        class Server : public net::Server
        {
        public:
            Server()
                : net::Server()
            {
            	CRUMB();
                registerEvent<native::event::http::server::request>();
                registerEvent<native::event::http::server::checkContinue>();
                registerEvent<native::event::http::server::upgrade>();
                registerEvent<native::event::connection>();
                registerEvent<native::event::close>();
                registerEvent<native::event::error>();

                // Register connection listener
                on<native::event::connection>([&](net::Socket* socket){
                	CRUMB();
                    assert(socket);

                    // Create a Parser to handle incoming message
                    // TODO: check if allocated Parser are leaking
                    Parser* parser(Parser::create(HTTP_REQUEST, socket));

                    // The parser will start reading from the socket and parsing data

                    parser->alloc_incoming([](net::Socket* socket){
                    	return new ServerRequest(socket);
                    });

                    // After receiving headers it will create an IncomingMessage and invoke this callback
                    parser->on_incoming([=](IncomingMessage* msg) {
                    	CRUMB();
                    	// Create ServerRequest from IncomingMessage
                    	ServerRequest* req = static_cast<ServerRequest*>(msg);

                    	assert(req);

                    	// Do early header processing and decide whether body should be received

                    	// Request isn't finished yet, when it ends, emit request event
                    	req->on<native::event::end>([=](){
                    		CRUMB();
                    		// Create ServerResponse
                    		ServerResponse* res = new ServerResponse(msg->socket());
                    		assert(res);
                    		// Pass request and response to listener
                    		emit<native::event::http::server::request>(req, res);

                        	// TODO: handle cleanup of ServerRequest better
//                    		delete req;
                    	});

//                    	return 0; // Don't skip body
//                    	return 1; // Skip body
                    });
                });
            }

            virtual ~Server()
            {}

        public:
            //void close();
            //void listen(port, hostname, callback);
            //void listen(path, callback);
        };

        class ClientResponse : public IncomingMessage
        {
        public:
        	ClientResponse(net::Socket* socket)
        		: IncomingMessage(socket)
        	{
            	CRUMB();
        		assert(socket);
        		registerEvent<native::event::data>();
        		registerEvent<native::event::end>();
        		registerEvent<native::event::close>();
        	}
        };

        class ClientRequest : public OutgoingMessage
        {
        private:
        	http_method method_;
        	headers_type headers_;
        	std::string path_;

        public:
        	/**
        	 * Construct a ClientRequest for the given url
        	 *
        	 * Unlike node.js, we do not accept an options map. Instead users
        	 * must set parameters like method, headers, etc. on the request
        	 * object itself.
        	 *
        	 * @param url
        	 * @param callback
        	 */
        	ClientRequest(detail::url_obj url, std::function<void(ClientResponse*)> callback = nullptr)
        		: OutgoingMessage(net::createSocket())
        		, method_(HTTP_GET)
        		, headers_()
        		, path_(url.has_path() ? url.path() : "/")
        	{
            	CRUMB();
        		registerEvent<native::event::http::socket>();
        		registerEvent<native::event::http::client::response>();
        		registerEvent<native::event::error>();
        		registerEvent<event::connect>();
        		registerEvent<native::event::http::client::upgrade>();
        		registerEvent<native::event::http::Continue>();

        		int port = url.has_port() ? url.port() : 80;
        		std::string host = url.has_host() ? url.host() : "127.0.0.1"; // "localhost"; // TODO: resolve hostname

        		if (callback) {
					once<native::event::http::client::response>(callback);
        		}

        		// TODO: set headers
        		// TODO: set default host header
        		// TODO: set authorization header if requested
        		// TODO: disable chunk encoding by defauult based on method (GET, HEAD, CONNECT)
        		// TODO: setup request header based on method and expect header
        		// Setup socket
        		// TODO: handle unix socket
        		// TODO: utilize user agent if provided
        		// TODO: utilize user connection method if provided
        		// Connect socket
        		socket_->connect(host, port);
        		// Setup socket
        		init_socket();

        		_deferToConnect([this](){
                	CRUMB();
        			_flush(); // flush output once connected
        		});
        	}
        	virtual ~ClientRequest() {}

        	void abort() {
            	CRUMB();
        		if (socket_) { // in-progress
        			socket_->destroy();
        		} else {
        			// not yet assigned a socket
        			// TODO: remove from pending requests
        			_deferToConnect([this](){
                    	CRUMB();
        				socket_->destroy();
        			});
        		}
        	}
        	void setTimeout(int64_t timeout, std::function<void()> callback=nullptr) {}
        	void setNoDelay(bool noDelay) {}
        	void setSocketKeepAlive(bool enable, int64_t initialDelay) {}

        private:
        	void _implicitHeader() {
            	CRUMB();
        		_storeHeader(
        				std::string(http_method_str(method_)) + " " + path_ + " HTTP/1.1\r\n",
        				_renderHeaders());
        	}

        	void _deferToConnect(std::function<void()> callback) {
            	CRUMB();
        		// if no socket then setup socket event handler
        		// if not yet connected setup connect event handler
        		// otherwise already connected so run callback
        	}

        	void init_socket() {
            	CRUMB();
                process::nextTick([this](){
                	// TODO: check if allocated Parser is leaking
					Parser* parser = Parser::create(HTTP_RESPONSE, socket_);

                    // The parser will start reading from the socket and parsing data

					// Setup parser for handling response
                    parser->alloc_incoming([](net::Socket* socket){
                    	return new ClientResponse(socket);
                    });

                    // TODO: set parser max. headers
					// TODO: Setup drain event
					// TODO: remove drain before setting it
					socket_->on<native::event::connect>([this](){
						this->on_socket_connect();
					});
					socket_->on<native::event::drain>(on_socket_drain);
					socket_->on<native::event::error>(on_socket_error);
					socket_->on<native::event::data>([this](const Buffer& buf){
						this->on_socket_data(buf);
					});
					socket_->on<native::event::end>(on_socket_end);
					socket_->on<native::event::close>(on_socket_close);

					// set on incoming callback on parser
					parser->on_incoming([this](IncomingMessage* res){
						this->on_incoming_message(res);
					});

					// Emit socket event
					emit<native::event::http::socket>(socket_);
                });
        	}

        	void on_incoming_message(IncomingMessage* msg) {
        		CRUMB();
        		ClientResponse* res = static_cast<ClientResponse*>(msg);

        		// if already created response then server sent double response
        		// TODO: destroy socket on double response
//        		this->response = res;

        		// TODO: handle response to CONNECT as UPGRADE

        		// TODO: handle response to HEAD request

        		// TODO: handle CONTINUE

        		// TODO: handle keep-alive

        		// Emit response event
        		emit<native::event::http::client::response>(res);
        		res->on<native::event::end>([this](){
        			this->on_response_end();
        		});
        	}

        	void on_response_end() {
        		// TODO: handle keep-alive after response end
        	}

        	void on_socket_connect() {
            	CRUMB();
                assert(socket_);
        	}

        	static void on_socket_drain() {
            	CRUMB();
        	}

        	static void on_socket_error(const Exception& e) {
            	CRUMB();
        	}

        	void on_socket_data(const Buffer& buf) {
            	CRUMB();
        	}

        	static void on_socket_end() {
            	CRUMB();
        	}

        	static void on_socket_close() {
            	CRUMB();
        	}
        };

        inline Server* createServer()
        {
        	CRUMB();
            auto server = new Server();

            return server;
        }

        inline ClientRequest* request(const std::string& url_string, std::function<void(ClientResponse*)> callback = nullptr)
        {
        	CRUMB();
        	detail::url_obj url;
        	url.parse(url_string.c_str(), url_string.length());

        	if (url.schema() != "http") {
        		throw Exception("Protocol: " + url.schema() + " not supported.");
        	}

        	ClientRequest* req = new ClientRequest(url, callback);

        	return req;
        }

        inline ClientRequest* get(const std::string& url_string, std::function<void(ClientResponse*)> callback = nullptr)
        {
        	CRUMB();
        	ClientRequest* req = request(url_string, callback);
        	req->end();
        	return req;
        }
    }
}

#endif
