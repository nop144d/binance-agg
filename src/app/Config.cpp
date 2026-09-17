#include "Config.hpp"

namespace binagg
{

Config::Config(const std::filesystem::path& config_file_path)
{
	// check if the file exists
	if (!std::filesystem::exists(config_file_path)) {
		throw std::runtime_error("Config file does not exist: " + config_file_path.string());
	}

	std::ifstream config_file(config_file_path);

	nlohmann::json config;
	config_file >> config;

	symbols_ = config.value("symbols", std::vector<std::string>{});
	if (symbols_.empty()) {
		throw std::runtime_error("Config file must contain at least one symbol");
	}

	window_ms = config.value("window_ms", int64_t{0});
	if (window_ms <= 0) {
		throw std::runtime_error("Config file must contain a valid window_ms value");
	}

	flush_interval_ms = config.value("flush_interval_ms", int64_t{0});
	if (flush_interval_ms <= 0) {
		throw std::runtime_error("Config file must contain a valid flush_interval_ms value");
	}

	output_file = config.value("output_file", std::string{});
	if (output_file.empty()) {
		throw std::runtime_error("Config file must contain a valid output_file value");
	}
}

} // namespace binagg
