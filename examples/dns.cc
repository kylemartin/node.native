/*
 * dns.cc
 *
 *  Created on: Dec 29, 2012
 *      Author: kmartin
 */

#include "native.h"
using namespace native;

int main(int argc, char** argv) {
  return process::run([=](){
    std::string hostname("xntria.com");
    if (argc > 1) {
      hostname = std::string(argv[1]);
    }
    detail::dns::get_addr_info(hostname, net::SocketType::IPv4, [](const std::vector<std::string>& results) {
      std::cout << "# results: " << results.size() << std::endl << std::flush;
      for (std::string s : results) {
        std::cout << s << std::endl << std::flush;
      }
    });
  });
}


