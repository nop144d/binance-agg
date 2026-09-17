#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>

namespace binagg
{

class Config
{
public:
	Config() = default;

	Config(const std::filesystem::path& config_file_path);

	auto GetSymbols() const -> const std::vector<std::string>& { return symbols_; }
	auto GetWindowMs() const -> int64_t { return window_ms; }
	auto GetFlushIntervalMs() const -> int64_t { return flush_interval_ms; }
	auto GetOutputFile() const -> const std::string& { return output_file; }

private:
	std::vector<std::string> symbols_{};
	int64_t window_ms{};
	int64_t flush_interval_ms{};
	std::string output_file;
};

} // namespace binagg
