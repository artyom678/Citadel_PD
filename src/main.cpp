#include "config.hpp"
#include "system_monitor.hpp"
#include <exception>
#include <iostream>



int main(int argc, char* argv[]) {

	try {

		Config config(argc, argv);

		SystemMonitor monitor(config);

		monitor.run();

		return 0;
	}
	catch(std::exception& err) {
		std::cerr << "Error : " << err.what() << std::endl;
		return 1;
	}

}