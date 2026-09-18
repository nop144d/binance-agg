#include "core/Config.hpp"

#include <gtest/gtest.h>

namespace {

class TempFile {
public:
	explicit TempFile(std::string_view contents)
		: path_(std::filesystem::temp_directory_path() /
			("binagg_cfg_" + std::to_string(counter_++) + ".json"))
	{
		std::ofstream(path_) << contents;
	}

	~TempFile() { std::error_code ec; std::filesystem::remove(path_, ec); }

	const std::filesystem::path& Path() const { return path_; }

private:
	static inline int counter_ = 0;
	std::filesystem::path path_;
};

} // namespace

TEST(Config, LoadsAllKeys)
{
	const TempFile file(R"({
		"symbols": ["btcusdt", "ethusdt", "bnbusdt"],
		"window_ms": 10000,
		"flush_interval_ms": 5000,
		"output_file": "aggregates.txt"
	})");

	const binagg::Config config{ file.Path() };
	EXPECT_EQ(config.GetSymbols(), (std::vector<std::string>{ "btcusdt", "ethusdt", "bnbusdt" }));
	EXPECT_EQ(config.GetWindowMs(), 10000);
	EXPECT_EQ(config.GetFlushIntervalMs(), 5000);
	EXPECT_EQ(config.GetOutputFile(), "aggregates.txt");
}

TEST(Config, ThrowsWhenFileIsMissing)
{
	EXPECT_THROW(binagg::Config{ std::filesystem::path{ "/no/such/config.json" } }, std::runtime_error);
}

TEST(Config, ThrowsOnMalformedJson)
{
	const TempFile file("{ this is not json");
	EXPECT_THROW(binagg::Config{ file.Path() }, std::exception);
}

TEST(Config, RejectsMissingOrInvalidValues)
{
	const TempFile no_symbols(R"({"symbols": [], "window_ms": 1, "flush_interval_ms": 1, "output_file": "o"})");
	EXPECT_THROW(binagg::Config{ no_symbols.Path() }, std::runtime_error);

	const TempFile no_window(R"({"symbols": ["btcusdt"], "flush_interval_ms": 1, "output_file": "o"})");
	EXPECT_THROW(binagg::Config{ no_window.Path() }, std::runtime_error);

	const TempFile zero_window(R"({"symbols": ["btcusdt"], "window_ms": 0, "flush_interval_ms": 1, "output_file": "o"})");
	EXPECT_THROW(binagg::Config{ zero_window.Path() }, std::runtime_error);

	const TempFile negative_flush(R"({"symbols": ["btcusdt"], "window_ms": 1, "flush_interval_ms": -5, "output_file": "o"})");
	EXPECT_THROW(binagg::Config{ negative_flush.Path() }, std::runtime_error);

	const TempFile no_output(R"({"symbols": ["btcusdt"], "window_ms": 1, "flush_interval_ms": 1, "output_file": ""})");
	EXPECT_THROW(binagg::Config{ no_output.Path() }, std::runtime_error);
}
