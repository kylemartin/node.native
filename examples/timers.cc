/*
 * timers.cc
 *
 *  Created on: Jul 12, 2012
 *      Author: kmartin
 */

#include <iostream>

#include "native.h"
using namespace native;

int main(int argc, char* argv[]) {
	return run([=](){
		std::shared_ptr<TimeoutHandler> timeout1 = setTimeout([=](){
			std::cout << "timeout1!" << std::endl;
		}, 1000);
		std::shared_ptr<TimeoutHandler> timeout2 = setTimeout([=](){
			std::cout << "timeout2!" << std::endl;
		}, 1000);
		std::shared_ptr<TimeoutHandler> timeout3 = setTimeout([=](){
			std::cout << "timeout3!" << std::endl;
		}, 1000);
		std::shared_ptr<TimeoutHandler> timeout4 = setTimeout([=](){
			std::cout << "timeout4!" << std::endl;
		}, 2000);
		std::shared_ptr<TimeoutHandler> timeout5 = setTimeout([=](){
			std::cout << "timeout5!" << std::endl;
		}, 2000);
		std::shared_ptr<TimeoutHandler> timeout6 = setTimeout([=](){
			std::cout << "timeout6!" << std::endl;
		}, 2000);
//		timer* interval1 = setInterval([=](){
//			std::cout << "interval1!" << std::endl;
//		}, 1000);
	});
}
