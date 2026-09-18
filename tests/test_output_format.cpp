#include "core/OutputFormat.hpp"

#include <gtest/gtest.h>

namespace {

binagg::ElapsedWindow MakeWindow(int64_t start_ms, std::string symbol, uint64_t trades,
	double volume, double min_price, double max_price)
{
	return {
		.window_start_ms = start_ms,
		.symbol = std::move(symbol),
		.stats = { .trades = trades, .volume = volume, .min_price = min_price, .max_price = max_price },
	};
}

} // namespace

TEST(OutputFormat, TimestampIsUtcWithSecondPrecision)
{
	EXPECT_EQ(binagg::FormatUtcTimestamp(1768227800123), "2026-01-12T14:23:20Z");
	EXPECT_EQ(binagg::FormatUtcTimestamp(1768227800000), "2026-01-12T14:23:20Z");
	EXPECT_EQ(binagg::FormatUtcTimestamp(0), "1970-01-01T00:00:00Z");
	EXPECT_EQ(binagg::FormatUtcTimestamp(951782400000), "2000-02-29T00:00:00Z");
}

// FR-5: the exact block from the task description.
TEST(OutputFormat, MatchesTheSpecifiedLayout)
{
	const std::vector<binagg::ElapsedWindow> windows{
		MakeWindow(1768227800000, "BTCUSDT", 154, 23.51, 43012.1, 43189.4),
		MakeWindow(1768227800000, "ETHUSDT", 231, 112.7, 2289.2, 2301.8),
	};

	EXPECT_EQ(binagg::FormatWindows(windows),
		"timestamp=2026-01-12T14:23:20Z\n"
		"symbol=BTCUSDT trades=154 volume=23.51000000 min=43012.10000000 max=43189.40000000\n"
		"symbol=ETHUSDT trades=231 volume=112.70000000 min=2289.20000000 max=2301.80000000\n");
}

TEST(OutputFormat, OneTimestampLinePerWindowStart)
{
	const std::vector<binagg::ElapsedWindow> windows{
		MakeWindow(1768227800000, "BTCUSDT", 1, 1.0, 1.0, 1.0),
		MakeWindow(1768227810000, "BTCUSDT", 2, 2.0, 1.0, 1.0),
		MakeWindow(1768227810000, "ETHUSDT", 3, 3.0, 1.0, 1.0),
	};

	EXPECT_EQ(binagg::FormatWindows(windows),
		"timestamp=2026-01-12T14:23:20Z\n"
		"symbol=BTCUSDT trades=1 volume=1.00000000 min=1.00000000 max=1.00000000\n"
		"timestamp=2026-01-12T14:23:30Z\n"
		"symbol=BTCUSDT trades=2 volume=2.00000000 min=1.00000000 max=1.00000000\n"
		"symbol=ETHUSDT trades=3 volume=3.00000000 min=1.00000000 max=1.00000000\n");
}

// FR-4: nothing elapsed means nothing written, not even a timestamp line.
TEST(OutputFormat, NoWindowsProducesNoOutput)
{
	EXPECT_EQ(binagg::FormatWindows(std::vector<binagg::ElapsedWindow>{}), "");
}

TEST(OutputFormat, NumbersUseFixedNotationAtEightDecimals)
{
	const std::vector<binagg::ElapsedWindow> windows{
		MakeWindow(0, "BTCUSDT", 1, 123456789012.5, 0.00000001, 99999.99999999),
	};

	EXPECT_EQ(binagg::FormatWindows(windows),
		"timestamp=1970-01-01T00:00:00Z\n"
		"symbol=BTCUSDT trades=1 volume=123456789012.50000000 min=0.00000001 max=99999.99999999\n");
}

TEST(OutputFormat, SingleWindowAndSymbol)
{
	const std::vector<binagg::ElapsedWindow> windows{
		MakeWindow(1768227800000, "SOLUSDT", 7, 1234.5, 98.76, 99.01),
	};

	EXPECT_EQ(binagg::FormatWindows(windows),
		"timestamp=2026-01-12T14:23:20Z\n"
		"symbol=SOLUSDT trades=7 volume=1234.50000000 min=98.76000000 max=99.01000000\n");
}
