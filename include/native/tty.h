/*
 * tty.h
 *
 *  Created on: Jul 13, 2012
 *      Author: kmartin
 */

#ifndef __NATIVE__TTY_H__
#define __NATIVE__TTY_H__

#include "net.h"
#include "detail/tty.h"

namespace native {


namespace event {

/**
 *  @brief 'resize' event.
 *
 *  @remark
 *  Callback function has the following signature.
 *  @code void callback() @endcode
 */
struct resize: public util::callback_def<> {};

}  // namespace event

namespace tty {

class ReadStream : public native::net::Socket {
public:
  ReadStream(uv_file file);
	bool isRaw();
	void setRawMode(bool mode);
	void destroy();
private:
	detail::tty tty_;
};

class WriteStream : public native::net::Socket {
public:
	WriteStream(uv_file file);
	void refreshSize();
	int columns() const;
	int rows() const;
private:
	detail::tty tty_;
	int columns_;
	int rows_;
};

bool isatty(uv_file file);
bool isatty(Stream* stream);

uv_handle_type guessHandleType(uv_file file);

}  // namespace tty

}  // namespace native


#endif /* __NATIVE__TTY_H__ */
