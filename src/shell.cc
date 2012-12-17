/*
 * shell.cc
 *
 *  Created on: Jun 28, 2012
 *      Author: kmartin
 */
#include <string>

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include "native/shell.h"

namespace native {

shell* shell::create() {
  // TODO: handle platform specific initialization
  if (native::tty::isatty(process::instance().stdout())) {
    return new shell(
        reinterpret_cast<tty::ReadStream*>(process::instance().stdin()),
        reinterpret_cast<tty::WriteStream*>(process::instance().stdout())
    );
  }
  return nullptr;
}

shell* shell::create(native::tty::ReadStream* in, native::tty::WriteStream* out) {
  return new shell(in, out);
}

shell::shell(native::tty::ReadStream* in, native::tty::WriteStream* out)
: rl_(readline::create(in, out)), prompt_string_("> ") {}

void shell::add_regex_command(std::string regex,
    regex_command_t callback) {
  regex_commands_[regex] = callback;
}

bool shell::remove_regex_command(std::string regex) {
  if (regex_commands_.count(regex)) {
    regex_commands_.erase(regex);
    return true;
  }
  return false;
}

void shell::add_command(std::string command,
    simple_command_t callback) {
  simple_commands_[command] = callback;
}

bool shell::remove_command(std::string command) {
  if (simple_commands_.count(command)) {
    simple_commands_.erase(command);
    return true;
  }
  return false;
}

void shell::parse_line(const std::string& line) {

  for (regex_commands_t::value_type entry : regex_commands_) {
    boost::regex regex(entry.first);

    if (boost::regex_match(line, regex)) {
      entry.second(line);
      return;
    }
  }

// tokenize line
//		std::cerr << line << std::endl;
//		boost::regex rx(R"xxx(\s*[^\s]*)xxx");
  boost::regex rx(R"(\s+)");
  boost::sregex_token_iterator first{line.begin(), line.end(), rx, -1}, last;

//		for (; first != last; ++first) {
//			std::cout << first->str() << "\n";
//			// v.push_back(it->str()); or something similar
//		}

  if (first != last) {
    std::string cmd = *first;
    boost::sregex_token_iterator first_arg(first);
    std::vector<std::string> args(++first_arg, last);

    simple_commands_t::iterator i = simple_commands_.find(cmd);
    if (i != simple_commands_.end()) {
      (*i).second(args);
      return;
    }
  //	if (res.size() > 0) {
  //			std::vector<std::string>::iterator i = res.begin();
  //		std::string cmd = *res.begin();

    rl_->write("unknown command: " + *first + "\n", [](){});
  }
//		int n = 0;
//		std::stringstream ss;
//		for (std::string x : res) {
//			if (n != 0) ss << ", ";
//			ss << x;
//			n++;
//		}
//		global::get()->http_server()->set_response(line);
//		tty->write("readline: " + ss.str() + '\n', [](native::error e){});
}

std::string shell::prompt() {
  if (prompt_callback_) {
    return prompt_callback_();
  }
  return prompt_string_;
}

void shell::run(std::function<void(const native::Exception&)> callback) {

  rl_->set_prompt(prompt());

  rl_->read([=](native::detail::resval res, std::string line){
    if (!res)
    {
      // TODO: need to add callback to Socket::destroy
//      tty_->close([](){std::cerr << "native::repl::shell::run(): closed tty" << std::endl;});
      rl_->destroy();
      callback(Exception(res));
    }
    else
    {
      rl_->history_add(line);
      parse_line(line);
      rl_->set_prompt(prompt());
    }
  });
}


void shell::out(const std::string& text) {
  process::instance().stdout()->write(Buffer(text), [](){});
}
void shell::err(const std::string& text) {
  process::instance().stderr()->write(Buffer(text), [](){});
}

}  // namespace native

