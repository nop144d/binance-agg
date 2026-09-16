#include <iostream>

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <config file>" << std::endl;
		return 1;
	}
	else {
		std::cout << "Config file: " << argv[1] << std::endl;
	}
    return 0;
}
