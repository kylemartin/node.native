
#include <functional>
#include <sstream>

#include "native/readline.h"

namespace native
{
    readline::readline(tty::ReadStream* in, tty::WriteStream* out)
			: ttyi(in)
			, ttyo(out)
			, rl_callback(), line_buf(), line_pos(0)
			, history(), history_index(0)
		{
      history.push_back("");
		}

    readline* readline::create() {
		  return create(process::instance().stdin(), process::instance().stdout());
		};

    readline* readline::create(Stream* stream_in, Stream* stream_out)
		{
      assert(stream_in->type() == UV_TTY);
      assert(stream_out->type() == UV_TTY);
      assert(stream_in->readable());
      assert(stream_out->writable());
      tty::ReadStream* tty_in = reinterpret_cast<tty::ReadStream*>(stream_in);
      tty::WriteStream* tty_out = reinterpret_cast<tty::WriteStream*>(stream_out);
			return new readline(tty_in, tty_out);
		}

	//		bool read_start(std::function<void(native::detail::resval e, const char* buf, ssize_t len)> callback)
	//		{
	//			return read_start<0>(callback);
	//		}
	//
	//		template<int max_alloc_size>
	//		bool read_start(std::function<void(native::detail::resval e, const char* buf, ssize_t len)> callback)
	//		{
	//			return ttyi->read_start<max_alloc_size>(callback);
	//		}

		bool readline::write(const char& c, std::function<void()> callback) {
			return ttyo->write(Buffer(std::string(1,c)), callback);
		}

		bool readline::write(const char* buf, int len, std::function<void()> callback)
		{
			return ttyo->write(Buffer(buf, len), callback);
		}

		bool readline::write(const std::string& buf, std::function<void()> callback)
		{
			return ttyo->write(Buffer(buf), callback);
		}

		bool readline::write(const std::vector<char>& buf, std::function<void()> callback)
		{
			return ttyo->write(Buffer(buf), callback);
		}

		void readline::pause() {
			ttyi->pause();
		}

		void readline::destroy() {
			ttyi->destroy();
			ttyo->destroy();
//			ttyi.reset();
//			ttyo.reset();
		}

		void readline::set_prompt(const std::string& prompt) {
			this->prompt = prompt;
		}

		void readline::clear_screen() {
//			write("\x1b[H\x1b[2J", [](native::detail::resval e){});
      write("\x1b[H\x1b[2J", [](){});
		}

		void readline::read() {

			// set raw mode
			// TODO: check for error setting raw mode
			ttyi->setRawMode(true);
			// display prompt
			write_prompt();
			// read input
      ttyi->resume();

      // TODO: handle read errors
			ttyi->on<event::data>([=](const Buffer& buffer){
					int width = ttyo->columns();
//					int height = ttyo->rows();

					char seq[2];
					char ext[2];

					std::size_t len = buffer.size();
					const char* buf = buffer.base();

					while (len > 0) {
						char c = *buf++; len--;

						switch (c) {
						case 13: /* enter */
						{
						  // TODO handle write error
              write('\n', [=](){});
              std::string line(line_buf);
              line_buf.clear();
							line_pos = 0;
							history_index = 0;
							handle_line(native::detail::resval(), line);
							write_prompt();
						} break;
						case 3: /* ctrl-c: cancel current line */
						  if (line_buf.size() > 0) {
						    line_buf.clear();
						    line_pos = 0;
						    history_index = 0;
              }
              write('\n', [=](){});
						  write_prompt();
							break;
						case 127: 	/* backspace */
						case 8:		  /* ctrl-h */
							if (line_pos > 0 && line_buf.size() > 0) {
								line_buf.erase(line_pos-1,1);
								line_pos--;
								refresh_line();
							}
							break;
						case 4:     /* ctrl-d: end of file */
              write('\n', [=](){});
              handle_line(native::detail::resval(UV_EOF), "");
							break;
						case 20:    /* ctrl-t: transpose characters */
							if (line_pos > 0 && line_pos < line_buf.size()) {
								int aux = line_buf[line_pos-1];
								line_buf[line_pos-1] = line_buf[line_pos];
								line_buf[line_pos] = aux;
								if (line_pos != line_buf.size()-1) line_pos++;
								refresh_line();
							}
							break;


            case 21: /* Ctrl+u, delete the whole line. */
              line_buf.clear();
              line_pos = 0;
              refresh_line();
              break;
            case 11: /* Ctrl+k, delete from current to end of line. */
              line_buf.erase(line_pos);
              refresh_line();
              break;
            case 1: /* Ctrl+a, go to the start of the line */
              line_pos = 0;
              refresh_line();
              break;
            case 5: /* ctrl+e, go to the end of the line */
              line_pos = line_buf.size();
              refresh_line();
              break;
            case 12: /* ctrl+l, clear screen */
              clear_screen();
              refresh_line();
              break;

						case 2:     /* ctrl-b */
							goto left_arrow;
						case 6:     /* ctrl-f */
							goto right_arrow;
						case 16: /* ctrl-p */
							seq[1] = 65;
							goto up_down_arrow;
						case 14: /* ctrl-n */
							seq[1] = 66;
							goto up_down_arrow;
						case 27: /* escape */
							assert(len >= 2);

							seq[0] = buf[0];
							seq[1] = buf[1];
							buf += 2;
							len -= 2;

							if (seq[0] == 91 && seq[1] == 68) {
	left_arrow:							/* left arrow */
								if (line_pos > 0) {
									line_pos--;
									refresh_line();
								}
							} else if (seq[0] == 91 && seq[1] == 67) {
	right_arrow:						/* right arrow */
								if (line_pos != line_buf.size()) {
									line_pos++;
									refresh_line();
								}
							} else if (seq[0] == 91 && (seq[1] == 65 || seq[1] == 66)) {
	up_down_arrow:						/* up and down arrow */
								// handle history

								if (history.size() > 1) {
									history[history.size()-1 - history_index] = line_buf;

									if (seq[1] == 65) {
									  if (history_index >= history.size()-1) break;
									  history_index += 1;
 									} else {
									  if (history_index <= 0) break;
									  history_index -= 1;
 									}

									line_buf = history[history.size()-1 - history_index];
									line_pos = line_buf.size();
									refresh_line();
								}

							} else if (seq[0] == 91 && seq[1] > 48 && seq[1] < 55 && len > 0) {
								/* extended escape */
								ext[0] = buf[0];
								buf++; len--;
								if (len > 0) {
									ext[1] = buf[1];
									buf++; len--;
								}
								if (seq[1] == 51 && ext[0] == 126) {
									/* delete */
									if (line_buf.size() > 0 && line_pos < line_buf.size()) {
										line_buf.erase(line_pos, 1);
										refresh_line();
									}
								}
							}
							break;
						default:
							if (line_pos == line_buf.size()) {
								line_buf.push_back(c);
								line_pos++;
								if (line_pos + prompt.size() < (unsigned int) width) {
									// avoid line refresh when space available
//                  write(c, [=](native::detail::resval e) {if (e) handle_line(e, "");});
								  // TODO: handle write error
									write(c, [=]() {});
								}
								else {
									refresh_line();
								}
							}
							else {
								line_buf.insert(line_pos, 1, c);
								line_pos++;
								refresh_line();
							}
							break;
						}
					}
			});

		}

		void readline::history_add(const std::string& line) {

			// TODO: handle overflow at maximum history
			if (line.size() > 0) {
			  history.back() = line;
				history.push_back("");
			}
		}

		std::string readline::get_prompt() {
		  return prompt;
		}

		void readline::write_prompt() {

			write(get_prompt(), []() {

				// TODO: handle error
			});
		}

		void readline::refresh_line() {

			int width = ttyo->columns();
//			int height = ttyo->rows();

      std::string p = get_prompt();
			size_t plen = p.size();
			size_t len = line_buf.size();
			size_t bpos = 0;
			size_t lpos = line_pos;

			// move buffer start until
			while ((plen+lpos) >= (size_t) width) {
				bpos++;
				len--;
				lpos--;
			}
			while ((plen+len) > (size_t) width) {
				len--;
			}
	//				std::cerr << "bpos: " << bpos << " plen: " << " len: " << len << " lpos: " << lpos << std::endl;
			std::stringstream ss;

      ss << "\x1b[0G"   						      // move cursor to left by size of prompt
			   << p             				        // write prompt
			   << line_buf.substr(bpos, len) 	  // write current line
			   << "\x1b[0K"						          // erase to end of line
			   << "\x1b[0G\x1b[" 				        // move cursor
			   << lpos+plen << "C";				      // to original position

//		   write (ss.str(), [=](native::detail::resval e){});
       write (ss.str(), [=](){});
		}

		void readline::handle_line(native::detail::resval e, std::string line) {
			rl_callback(e, line);
		}

		void readline::write(const std::string& str) {
		  write(str, [](){});
		}

		void readline::start_raw() {
		  // Move cursor to start of line, erase to end of line, then back to start
		  write("\x1b[0G\x1b[0K\x1b[0G", [](){});
		}

		void readline::end_raw() {
      write("\x1b[0G\x1b[0K\x1b[0G", [](){});
		}

} // namespace native
