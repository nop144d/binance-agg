#include "app/App.hpp"
#include "app/Config.hpp"

#include <spdlog/cfg/env.h>
#include <spdlog/sinks/stdout_color_sinks.h>

using namespace binagg;

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <config file>" << std::endl;
		return 1;
	}

	spdlog::set_default_logger(spdlog::stderr_color_mt("binance-agg"));
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
	spdlog::cfg::load_env_levels();

	try {
		App app{ std::make_unique<Config>(std::filesystem::path{ argv[1] }) };
		return app.Run();
	}
	catch (const std::exception& e) {
		spdlog::critical("{}", e.what());
		return 1;
	}
}
