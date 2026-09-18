#include "app/App.hpp"
#include "app/Config.hpp"

using namespace binagg;

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <config file>" << std::endl;
		return 1;
	}

	try {
		App app{ std::make_unique<Config>(std::filesystem::path{ argv[1] }) };
		return app.Run();
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}
