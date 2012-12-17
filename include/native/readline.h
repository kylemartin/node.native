#ifndef __NATIVE__READLINE_H__
#define __NATIVE__READLINE_H__

#include <functional>
#include <sstream>

#include "native/base.h"
#include "native/tty.h"

namespace native
{
	namespace event {
		/**
		 *  @brief 'line' event.
		 *
		 *  @remark
		 *  Callback function has the following signature.
		 *  @code void callback() @endcode
		 */
		/**
		 *  @brief 'sigint' event.
		 *
		 *  @remark
		 *  Callback function has the following signature.
		 *  @code void callback() @endcode
		 */
		struct sigint: public util::callback_def<> {};
		/**
		 *  @brief 'sigtstp' event.
		 *
		 *  @remark
		 *  Callback function has the following signature.
		 *  @code void callback() @endcode
		 */
		struct sigtstp: public util::callback_def<> {};
		/**
		 *  @brief 'sigcont' event.
		 *
		 *  @remark
		 *  Callback function has the following signature.
		 *  @code void callback() @endcode
		 */
		struct sigcont: public util::callback_def<> {};
	}

	class readline
	{
	private:
		std::shared_ptr<tty::ReadStream> ttyi;
		std::shared_ptr<tty::WriteStream> ttyo;
		std::function<void(native::detail::resval, std::string)> rl_callback;

		std::string prompt;

		std::string line_buf;
		size_t line_pos;

		std::vector<std::string> history;
		unsigned int history_index;

		readline(tty::ReadStream* in, tty::WriteStream* out);

	public:

		static readline* create();

		static readline* create(Stream* stream_in, Stream* stream_out);

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

		bool write(const char& c, std::function<void()> callback);

		bool write(const char* buf, int len, std::function<void()> callback);

		bool write(const std::string& buf, std::function<void()> callback);

		bool write(const std::vector<char>& buf, std::function<void()> callback);

		void pause();

		void destroy();

		void set_prompt(const std::string& prompt);

		void clear_screen();

  private:
		void read();
  public:
		typedef std::function<void(native::detail::resval, std::string)> rl_callback_t;
		void read(rl_callback_t callback) {
			// store callback
			rl_callback = callback;
			read();
		}

		void history_add(const std::string& line);

	private:
		void write_prompt();

		void refresh_line();

		void handle_line(native::detail::resval e, std::string line);
	};

} // namespace native

#endif /* __NATIVE__READLINE_H__ */
