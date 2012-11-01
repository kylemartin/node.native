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
	tty(uv_file file, int readable)
        : stream(reinterpret_cast<uv_stream_t*>(&tty_))
        ,  tty_(), raw_enabled_(false)
		, orig_termios_fd_(), orig_termios_()
    {
        int r = uv_tty_init(uv_default_loop(), &tty_, file, readable);
        assert(r == 0);

        tty_.data = this;
    }


	int enable_raw() {
		// TODO: handle error
//            	if (uv_tty_set_mode(ttyi->get<uv_tty_t>(), enabled ? 1 : 0) != 0) return -1;
//            	if (uv_tty_set_mode(ttyo->get<uv_tty_t>(), enabled ? 1 : 0) != 0) return -1;
//            	return 0;
		uv_tty_t* tty = &tty_;
		int fd = tty->fd;
		struct termios raw;

		if (!raw_enabled_) {
			/* on */

			if (tcgetattr(fd, &tty->orig_termios)) {
				goto fatal;
			}

			/* This is used for uv_tty_reset_mode() */
			if (orig_termios_fd_ == -1) {
				orig_termios_ = tty->orig_termios;
				orig_termios_fd_ = fd;
			}

			raw = tty->orig_termios;
			raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
			raw.c_oflag |= (ONLCR);
			raw.c_cflag |= (CS8);
			raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
			raw.c_cc[VMIN] = 1;
			raw.c_cc[VTIME] = 0;

			/* Put terminal in raw mode after draining */
			if (tcsetattr(fd, TCSADRAIN, &raw)) {
				goto fatal;
			}

			raw_enabled_ = true;

			tty->mode = 1;
			return 0;
		}

		fatal:
		// TODO: handle error
		return -1;
	}

	int disable_raw() {
		uv_tty_t* tty = &tty_;
		int fd = tty->fd;


		if (raw_enabled_) {
			/* off */

			/* Put terminal in original mode after flushing */
			if (!tcsetattr(fd, TCSAFLUSH, &tty->orig_termios)) {
				tty->mode = 0;
				return 0;
			}
		}
		// TODO: handle error
		return -1;
	}

	int get_window_size(int* width, int* height) {
		// TODO: handle error
		return uv_tty_get_winsize(&tty_, width, height);
	}

	virtual ~tty() {}

private:
	uv_tty_t tty_;
	bool raw_enabled_;
    int orig_termios_fd_;
    struct termios orig_termios_;
};

}  // namespace detail

}  // namespace native


#endif /* __NATIVE__DETAIL__TTY_H__ */
