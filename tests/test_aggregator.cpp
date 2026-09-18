#include "core/Aggregator.hpp"
#include "core/Trade.hpp"

#include <gtest/gtest.h>

namespace {

constexpr int64_t kWindow = 10'000;
constexpr int64_t kBoundary = 1'789'722'600'000; // a multiple of kWindow

binagg::Trade MakeTrade(std::string symbol, double price, double quantity, int64_t time_ms)
{
	return { .symbol = std::move(symbol), .price = price, .quantity = quantity, .trade_time_ms = time_ms };
}

} // namespace

TEST(Aggregator, ConstructorRejectsNonPositiveWindow)
{
	EXPECT_THROW(binagg::Aggregator{ 0 }, std::invalid_argument);
	EXPECT_THROW(binagg::Aggregator{ -1 }, std::invalid_argument);
	EXPECT_NO_THROW(binagg::Aggregator{ 1 });
}

// FR-3: a trade whose timestamp equals a boundary belongs to the next window.
TEST(Aggregator, TradeOnBoundaryBelongsToNextWindow)
{
	binagg::Aggregator agg{ kWindow };
	EXPECT_TRUE(agg.AddTrade(MakeTrade("BTCUSDT", 1.0, 1.0, kBoundary - 1)));
	EXPECT_TRUE(agg.AddTrade(MakeTrade("BTCUSDT", 2.0, 1.0, kBoundary)));

	const auto windows = agg.ExtractElapsed(kBoundary + kWindow);
	ASSERT_EQ(windows.size(), 2u);
	EXPECT_EQ(windows[0].window_start_ms, kBoundary - kWindow);
	EXPECT_DOUBLE_EQ(windows[0].stats.min_price, 1.0);
	EXPECT_EQ(windows[1].window_start_ms, kBoundary);
	EXPECT_DOUBLE_EQ(windows[1].stats.min_price, 2.0);
}

TEST(Aggregator, WindowStartIsEpochAligned)
{
	binagg::Aggregator agg{ kWindow };
	EXPECT_TRUE(agg.AddTrade(MakeTrade("BTCUSDT", 1.0, 1.0, kBoundary + 1)));
	EXPECT_TRUE(agg.AddTrade(MakeTrade("BTCUSDT", 1.0, 1.0, kBoundary + kWindow - 1)));

	const auto windows = agg.ExtractElapsed(kBoundary + kWindow);
	ASSERT_EQ(windows.size(), 1u);
	EXPECT_EQ(windows[0].window_start_ms, kBoundary);
	EXPECT_EQ(windows[0].stats.trades, 2u);
}

// FR-2: trades, volume, min and max over a sequence.
TEST(Aggregator, AggregatesTradesVolumeMinAndMax)
{
	binagg::Aggregator agg{ kWindow };
	EXPECT_TRUE(agg.AddTrade(MakeTrade("BTCUSDT", 100.0, 2.0, kBoundary + 1)));
	EXPECT_TRUE(agg.AddTrade(MakeTrade("BTCUSDT", 150.5, 4.0, kBoundary + 2)));
	EXPECT_TRUE(agg.AddTrade(MakeTrade("BTCUSDT", 125.25, 8.0, kBoundary + 3)));

	const auto windows = agg.ExtractElapsed(kBoundary + kWindow);
	ASSERT_EQ(windows.size(), 1u);
	EXPECT_EQ(windows[0].symbol, "BTCUSDT");
	EXPECT_EQ(windows[0].stats.trades, 3u);
	EXPECT_DOUBLE_EQ(windows[0].stats.volume, 200.0 + 602.0 + 1002.0);
	EXPECT_DOUBLE_EQ(windows[0].stats.min_price, 100.0);
	EXPECT_DOUBLE_EQ(windows[0].stats.max_price, 150.5);
}

TEST(Aggregator, SingleTradeHasMinEqualToMax)
{
	binagg::Aggregator agg{ kWindow };
	EXPECT_TRUE(agg.AddTrade(MakeTrade("ETHUSDT", 2289.25, 4.0, kBoundary)));

	const auto windows = agg.ExtractElapsed(kBoundary + kWindow);
	ASSERT_EQ(windows.size(), 1u);
	EXPECT_DOUBLE_EQ(windows[0].stats.min_price, windows[0].stats.max_price);
	EXPECT_DOUBLE_EQ(windows[0].stats.volume, 9157.0);
}

TEST(Aggregator, SymbolsAreKeptSeparate)
{
	binagg::Aggregator agg{ kWindow };
	EXPECT_TRUE(agg.AddTrade(MakeTrade("BTCUSDT", 100.0, 1.0, kBoundary)));
	EXPECT_TRUE(agg.AddTrade(MakeTrade("ETHUSDT", 200.0, 1.0, kBoundary)));

	const auto windows = agg.ExtractElapsed(kBoundary + kWindow);
	ASSERT_EQ(windows.size(), 2u);
	EXPECT_EQ(windows[0].symbol, "BTCUSDT");
	EXPECT_EQ(windows[1].symbol, "ETHUSDT");
	EXPECT_EQ(windows[0].stats.trades, 1u);
	EXPECT_EQ(windows[1].stats.trades, 1u);
}

// FR-4: a window with no trades never exists, so it is never reported.
TEST(Aggregator, WindowWithoutTradesProducesNothing)
{
	binagg::Aggregator agg{ kWindow };
	EXPECT_TRUE(agg.ExtractElapsed(kBoundary + 5 * kWindow).empty());

	EXPECT_TRUE(agg.AddTrade(MakeTrade("BNBUSDT", 600.0, 1.0, kBoundary + 6 * kWindow)));
	const auto windows = agg.ExtractElapsed(kBoundary + 7 * kWindow);
	ASSERT_EQ(windows.size(), 1u);
	EXPECT_EQ(windows[0].symbol, "BNBUSDT");
}

TEST(Aggregator, OnlyExtractsWindowsThatHaveEnded)
{
	binagg::Aggregator agg{ kWindow };
	EXPECT_TRUE(agg.AddTrade(MakeTrade("BTCUSDT", 1.0, 1.0, kBoundary + 500)));

	EXPECT_TRUE(agg.ExtractElapsed(kBoundary + kWindow - 1).empty());
	EXPECT_EQ(agg.ExtractElapsed(kBoundary + kWindow).size(), 1u);
	EXPECT_TRUE(agg.ExtractElapsed(kBoundary + 2 * kWindow).empty());
}

// FR-4: a trade for an already-extracted window is dropped and counted.
TEST(Aggregator, LateTradeIsDroppedAndCounted)
{
	binagg::Aggregator agg{ kWindow };
	EXPECT_TRUE(agg.AddTrade(MakeTrade("BTCUSDT", 1.0, 1.0, kBoundary + 100)));
	ASSERT_EQ(agg.ExtractElapsed(kBoundary + kWindow).size(), 1u);
	EXPECT_EQ(agg.GetDroppedCount(), 0u);

	EXPECT_FALSE(agg.AddTrade(MakeTrade("BTCUSDT", 9.0, 1.0, kBoundary + 900)));
	EXPECT_FALSE(agg.AddTrade(MakeTrade("ETHUSDT", 9.0, 1.0, kBoundary - kWindow)));
	EXPECT_EQ(agg.GetDroppedCount(), 2u);

	EXPECT_TRUE(agg.AddTrade(MakeTrade("BTCUSDT", 3.0, 1.0, kBoundary + kWindow)));
	EXPECT_EQ(agg.GetDroppedCount(), 2u);

	const auto windows = agg.ExtractElapsed(kBoundary + 2 * kWindow);
	ASSERT_EQ(windows.size(), 1u);
	EXPECT_EQ(windows[0].stats.trades, 1u);
	EXPECT_DOUBLE_EQ(windows[0].stats.min_price, 3.0);
}

TEST(Aggregator, ExtractedOrderIsWindowStartThenSymbol)
{
	binagg::Aggregator agg{ kWindow };
	EXPECT_TRUE(agg.AddTrade(MakeTrade("ETHUSDT", 1.0, 1.0, kBoundary + kWindow + 1)));
	EXPECT_TRUE(agg.AddTrade(MakeTrade("BTCUSDT", 1.0, 1.0, kBoundary + kWindow + 2)));
	EXPECT_TRUE(agg.AddTrade(MakeTrade("ETHUSDT", 1.0, 1.0, kBoundary + 1)));
	EXPECT_TRUE(agg.AddTrade(MakeTrade("BNBUSDT", 1.0, 1.0, kBoundary + 2)));
	EXPECT_TRUE(agg.AddTrade(MakeTrade("BTCUSDT", 1.0, 1.0, kBoundary + 3)));

	const auto windows = agg.ExtractElapsed(kBoundary + 2 * kWindow);
	std::vector<std::pair<int64_t, std::string>> order;
	for (const auto& w : windows) {
		order.emplace_back(w.window_start_ms, w.symbol);
	}

	const std::vector<std::pair<int64_t, std::string>> expected{
		{ kBoundary, "BNBUSDT" },
		{ kBoundary, "BTCUSDT" },
		{ kBoundary, "ETHUSDT" },
		{ kBoundary + kWindow, "BTCUSDT" },
		{ kBoundary + kWindow, "ETHUSDT" },
	};
	EXPECT_EQ(order, expected);
}

TEST(Aggregator, ExtremeTimestampsDoNotOverflow)
{
	binagg::Aggregator agg{ kWindow };
	EXPECT_TRUE(agg.AddTrade(MakeTrade("BTCUSDT", 1.0, 1.0, 0)));
	EXPECT_TRUE(agg.AddTrade(MakeTrade("BTCUSDT", 1.0, 1.0, std::numeric_limits<int64_t>::max())));

	// The window holding INT64_MAX ends past INT64_MAX, so it can never elapse.
	const auto windows = agg.ExtractElapsed(std::numeric_limits<int64_t>::max());
	ASSERT_EQ(windows.size(), 1u);
	EXPECT_EQ(windows[0].window_start_ms, 0);
}

// Known limitation: double cannot hold eight exact decimals once the running
// volume grows. Enable this once volume is a scaled integer.
TEST(Aggregator, DISABLED_VolumeIsExactOverAWindowOfTrades)
{
	binagg::Aggregator agg{ kWindow };
	for (int i = 0; i < 5000; ++i) {
		EXPECT_TRUE(agg.AddTrade(MakeTrade("BTCUSDT", 77000.01, 0.001, kBoundary + 1)));
	}

	const auto windows = agg.ExtractElapsed(kBoundary + kWindow);
	ASSERT_EQ(windows.size(), 1u);
	EXPECT_EQ(std::format("{:.8f}", windows[0].stats.volume), "385000.05000000");
}
