/*
 * tty.cc
 *
 *  Created on: Jul 14, 2012
 *      Author: kmartin
 */

#include "native.h"
using namespace native;

int main(int argc, char* argv[]) {
	return process::run([=]() {
		std::cerr << "running!" << std::endl;
	});
}
