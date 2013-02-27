/*
 * shell.h
 *
 *  Created on: Jun 28, 2012
 *      Author: kmartin
 */

#ifndef __NATIVE__SHELL_H__
#define __NATIVE__SHELL_H__

#include <memory>
#include <unordered_map>

#include "native/tty.h"
#include "native/readline.h"

namespace native {

// http://stackoverflow.com/a/4367244
//class shell_ostream {
//public:
//  shell_ostream(const shell_ostream& other) : stream_(other.stream_), oss() {}
//  shell_ostream(native::Stream* stream) : stream_(stream), oss() {}
//  template <typename T>
//  shell_ostream& operator<<(const T& output) {
//    oss << output;
//    return *this;
//  }
//  typedef std::ostream& (*STRFUNC)(std::ostream&);
//  shell_ostream& operator<<(STRFUNC f) {
//    f(oss);
//    return *this;
//  }
//  ~shell_ostream() {
//    stream_->write(Buffer(oss.str()), [](){});
//  }
//private:
//  native::Stream* stream_;
//  std::ostringstream oss;
//};

// http://stackoverflow.com/a/2212940
class shell_ostream : public std::ostream {
private:
  class shell_stringbuf : public std::stringbuf {
  private:
    native::Stream* stream_;
    native::readline* rl_;
  public:
    shell_stringbuf(native::readline* rl, native::Stream* stream) :
      stream_(stream), rl_(rl) {}

    virtual int sync() {
      rl_->start_raw();
      stream_->write(Buffer(str()), [](){});
      rl_->end_raw();
      str("");
      return 0;
    }
  };
  shell_stringbuf sb_;
public:
  shell_ostream(native::readline* rl, native::Stream* stream) :
    std::ostream(&sb_), sb_(rl, stream) {}
};


/**
 * A shell manages a tty and number of commands
 */
class shell {
private:
  shell(native::tty::ReadStream* in, native::tty::WriteStream* out);

public:
  typedef std::function<void(const std::string&)> regex_command_t;
  typedef std::function<void(const std::vector<std::string>&)> simple_command_t;

  static shell* create();
  static shell* create(native::tty::ReadStream* in, native::tty::WriteStream* out);

  void run(std::function<void()> terminate_callback,
      std::function<void(const native::Exception&)> error_callback);

//  void out(const std::string& text);
//  void err(const std::string& text);

  void add_command(std::string command,
      simple_command_t callback);

  bool remove_command(std::string command);

  void add_regex_command(std::string regex,
       regex_command_t callback);

  bool remove_regex_command(std::string regex);

  template<typename callback_t>
  void set_prompt_callback(callback_t callback) {
    prompt_callback_ = callback;
  }

  void set_prompt(std::string prompt) {
    prompt_string_ = prompt;
  }

//  shell_ostream out() {
//    return shell_ostream(process::instance().stdout());
//  }
//  shell_ostream err() {
//    return shell_ostream(process::instance().stderr());
//  }

private:
  std::shared_ptr<native::readline> rl_;
public:
  shell_ostream cout; //shell_ostream's depend on rl_ during construction
  shell_ostream cerr;
private:
  typedef std::unordered_map<std::string, regex_command_t> regex_commands_t;
  typedef std::unordered_map<std::string, simple_command_t> simple_commands_t;
  simple_commands_t simple_commands_; /** simple string commands */
  regex_commands_t regex_commands_; /** regex commands */
  std::function<std::string()> prompt_callback_;
  std::string prompt_string_;

  void parse_line(const std::string& line);
  std::string prompt();
};

}  // namespace native

#endif /* __NATIVE__SHELL_H__ */
