/*
 * tty.h
 *
 *  Created on: Jul 11, 2012
 *      Author: kmartin
 */

#ifndef __NATIVE__DETAIL__TTY_H__
#define __NATIVE__DETAIL__TTY_H__

#include "base.h"
#include "stream.h"

namespace native {
namespace detail {

class tty : public stream {
public:
	tty(uv_file file, bool readable);
	virtual ~tty() {}

	bool is_raw();
	int set_raw_mode(bool mode);
	int get_window_size(int* width, int* height);

private:
	uv_tty_t tty_;
  int orig_termios_fd_;
  struct termios orig_termios_;
  bool raw_mode_;
};

}  // namespace detail
}  // namespace native


#endif /* __NATIVE__DETAIL__TTY_H__ */
