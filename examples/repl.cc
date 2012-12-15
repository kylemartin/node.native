/*
 * repl.cc
 *
 *  Created on: Jul 11, 2012
 *      Author: kmartin
 */

#include <iostream>

#include "native.h"

using namespace native;

int main(int argc, char* argv[]) {

//	uv_file ttyin_fd, ttyout_fd;
//	/* Make sure we have an FD that refers to a tty */
//#ifdef _WIN32
//	HANDLE handle;
//	handle = CreateFileA("conin$",
//			GENERIC_READ | GENERIC_WRITE,
//			FILE_SHARE_READ | FILE_SHARE_WRITE,
//			NULL,
//			OPEN_EXISTING,
//			FILE_ATTRIBUTE_NORMAL,
//			NULL);
//	ASSERT(handle != INVALID_HANDLE_VALUE);
//	ttyin_fd = _open_osfhandle((intptr_t) handle, 0);
//
//	handle = CreateFileA("conout$",
//			GENERIC_READ | GENERIC_WRITE,
//			FILE_SHARE_READ | FILE_SHARE_WRITE,
//			NULL,
//			OPEN_EXISTING,
//			FILE_ATTRIBUTE_NORMAL,
//			NULL);
//	ASSERT(handle != INVALID_HANDLE_VALUE);
//	ttyout_fd = _open_osfhandle((intptr_t) handle, 0);
//
//#elif defined(__APPLE__)
//	ttyin_fd = fileno(stdin);
//	ttyout_fd = fileno(stdout);
//#else /* unix */
//	// XXX: using /dev/tty is buggy on OSX, not tested elswhere
////	ttyin_fd = open("/dev/tty", O_RDONLY, 0);
////	ttyout_fd = open("/dev/tty", O_WRONLY, 0);
//#endif

	shell* s =  shell::create();

	s->add_command("test", [=](const std::vector<std::string>& args){
		std::cout << "num args: " << args.size() << std::endl;
	});

  s->run([](const native::Exception& e) {
    std::cerr << "shell terminated with error: " << e.message() << std::endl;
  });
}
