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
    struct result {
      std::string a, aaaa;
    };
    detail::dns::get_addr_info(hostname, net::SocketType::IPv4, [](int status, const std::vector<std::string>& results, int family) {
      std::cout << "dns::get_addr_info(IPv4)" << std::endl << "# results: " << results.size() << std::endl;
      for (std::string s : results) {
        std::cout << s << std::endl;
      }
      std::cout << std::endl << std::flush;
    });

    detail::dns::get_addr_info(hostname, net::SocketType::IPv6, [](int status, const std::vector<std::string>& results, int family) {
      std::cout << "dns::get_addr_info(IPv6)" << std::endl << "# results: " << results.size() << std::endl;
      for (std::string s : results) {
        std::cout << s << std::endl;
      }
      std::cout << std::endl << std::flush;
    });

    detail::dns::QueryGetHostByName(hostname, AF_INET, [](int status, const std::vector<std::string>& results, int family) {
      std::cout << "dns::QueryGetHostByName(IPv4)" << std::endl << "# results: " << results.size() << std::endl;
      for (std::string s : results) {
        std::cout << s << std::endl;
      }
      std::cout << std::endl;

      if (results.size() > 0) {
        detail::dns::QueryGetHostByAddr(results[0], [](int status, const std::vector<std::string>& results, int family) {
          std::cout << "dns::QueryGetHostByAddr(IPv4)" << std::endl << "# results: " << results.size() << std::endl;
          for (std::string s : results) {
            std::cout << s << std::endl;
          }
          std::cout << std::endl;
        });
      }
    });

    detail::dns::QueryGetHostByName(hostname, AF_INET6, [](int status, const std::vector<std::string>& results, int family) {
      std::cout << "dns::QueryGetHostByName(IPv6)" << std::endl << "# results: " << results.size() << std::endl;
      for (std::string s : results) {
        std::cout << s << std::endl;
      }
      std::cout << std::endl;

      if (results.size() > 0) {
        detail::dns::QueryGetHostByAddr(results[0], [](int status, const std::vector<std::string>& results, int family) {
          std::cout << "dns::QueryGetHostByAddr(IPv6)" << std::endl << "# results: " << results.size() << std::endl;
          for (std::string s : results) {
            std::cout << s << std::endl;
          }
          std::cout << std::endl;
        });
      }
    });

    detail::dns::get_addr_info("localhost", net::SocketType::IPv4, [](int status, const std::vector<std::string>& results, int family) {
      std::cout << "dns::get_addr_info(localhost, IPv4)" << std::endl << "# results: " << results.size() << std::endl;
      for (std::string s : results) {
        std::cout << s << std::endl;
      }
      std::cout << std::endl << std::flush;
    });

    detail::dns::get_addr_info("localhost", net::SocketType::IPv6, [](int status, const std::vector<std::string>& results, int family) {
      std::cout << "dns::get_addr_info(localhost, IPv6)" << std::endl << "# results: " << results.size() << std::endl;
      for (std::string s : results) {
        std::cout << s << std::endl;
      }
      std::cout << std::endl << std::flush;
    });

    detail::dns::QueryGetHostByName("localhost", AF_INET, [](int status, const std::vector<std::string>& results, int family) {
      std::cout << "dns::QueryGetHostByName(localhost, IPv4)" << std::endl << "# results: " << results.size() << std::endl;
      for (std::string s : results) {
        std::cout << s << std::endl;
      }
      std::cout << std::endl;
    });

    detail::dns::QueryGetHostByName("localhost", AF_INET6, [](int status, const std::vector<std::string>& results, int family) {
      std::cout << "dns::QueryGetHostByName(localhost, IPv6)" << std::endl << "# results: " << results.size() << std::endl;
      for (std::string s : results) {
        std::cout << s << std::endl;
      }
      std::cout << std::endl;
    });

    detail::dns::QueryGetHostByAddr("127.0.0.1", [](int status, const std::vector<std::string>& results, int family) {
      std::cout << "dns::QueryGetHostByAddr(127.0.0.1)" << std::endl << "# results: " << results.size() << std::endl;
      for (std::string s : results) {
        std::cout << s << std::endl;
      }
      std::cout << std::endl;
    });

    detail::dns::QueryGetHostByAddr("::1", [](int status, const std::vector<std::string>& results, int family) {
      std::cout << "dns::QueryGetHostByAddr(::1)" << std::endl << "# results: " << results.size() << std::endl;
      for (std::string s : results) {
        std::cout << s << std::endl;
      }
      std::cout << std::endl;
    });  });
}


