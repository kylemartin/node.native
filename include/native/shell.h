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

  void run(std::function<void(const native::Exception&)> callback);

  void out(const std::string& text);
  void err(const std::string& text);

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

private:
  std::shared_ptr<native::readline> rl_;
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
