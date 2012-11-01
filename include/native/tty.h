/*
 * tty.h
 *
 *  Created on: Jul 13, 2012
 *      Author: kmartin
 */

#ifndef __NATIVE__TTY_H__
#define __NATIVE__TTY_H__

#include "net.h"

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
	bool isRaw() { return false; }
	void setRawMode(bool mode) {}
};

class WriteStream : public native::net::Socket {
	WriteStream(uv_file file) {
		registerEvent<event::resize>();
	}
	int columns() { return 0; }
	int rows() { return 0; }
private:
};

inline bool isatty(uv_file file) {
	return UV_TTY == uv_guess_handle(file);
}


}  // namespace tty

}  // namespace native


#endif /* __NATIVE__TTY_H__ */
