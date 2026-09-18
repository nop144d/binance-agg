#include "core/Trade.hpp"

#include <gtest/gtest.h>

namespace {

// Captured from wss://stream.binance.com:9443/stream?streams=bnbusdt@trade
constexpr const char* kCombined =
	R"({"stream":"bnbusdt@trade","data":{"e":"trade","E":1789722601401,"s":"BNBUSDT","t":1587720407,"p":"755.17000000","q":"0.00800000","T":1789722601401,"m":false,"M":true}})";

// The same event as a raw stream delivers it, with no envelope.
constexpr const char* kRaw =
	R"({"e":"trade","E":1789722601401,"s":"BNBUSDT","t":1587720407,"p":"755.17000000","q":"0.00800000","T":1789722601401,"m":false,"M":true})";

} // namespace

// FR-6: a real payload parses.
TEST(Trade, ParsesCombinedStreamPayload)
{
	const auto trade = binagg::Trade::FromJSON(kCombined);
	ASSERT_TRUE(trade.has_value()) << trade.error();
	EXPECT_EQ(trade->symbol, "BNBUSDT");
	EXPECT_DOUBLE_EQ(trade->price, 755.17);
	EXPECT_DOUBLE_EQ(trade->quantity, 0.008);
	EXPECT_EQ(trade->trade_time_ms, 1789722601401);
}

TEST(Trade, ParsesRawStreamPayload)
{
	const auto trade = binagg::Trade::FromJSON(kRaw);
	ASSERT_TRUE(trade.has_value()) << trade.error();
	EXPECT_EQ(trade->symbol, "BNBUSDT");
	EXPECT_EQ(trade->trade_time_ms, 1789722601401);
}

TEST(Trade, IgnoresUnknownFields)
{
	const auto trade = binagg::Trade::FromJSON(
		R"({"e":"trade","s":"ETHUSDT","p":"2289.2","q":"1","T":1,"extra":{"nested":[1,2,3]}})");
	ASSERT_TRUE(trade.has_value()) << trade.error();
	EXPECT_EQ(trade->symbol, "ETHUSDT");
}

// FR-6: malformed input is rejected rather than crashing or guessing.
TEST(Trade, RejectsMalformedJson)
{
	EXPECT_FALSE(binagg::Trade::FromJSON(R"({"e":"trade","s":"BTCUSDT","p":"1")").has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON("not json at all").has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON("").has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON("[1,2,3]").has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON("42").has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON(R"({"stream":"x","data":"oops"})").has_value());
}

TEST(Trade, RejectsWrongOrMissingEventType)
{
	EXPECT_FALSE(binagg::Trade::FromJSON(R"({"e":"aggTrade","s":"BTCUSDT","p":"1","q":"1","T":1})").has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON(R"({"s":"BTCUSDT","p":"1","q":"1","T":1})").has_value());
}

TEST(Trade, RejectsMissingOrWrongTypedFields)
{
	EXPECT_FALSE(binagg::Trade::FromJSON(R"({"e":"trade","p":"1","q":"1","T":1})").has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON(R"({"e":"trade","s":"","p":"1","q":"1","T":1})").has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON(R"({"e":"trade","s":"BTCUSDT","q":"1","T":1})").has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON(R"({"e":"trade","s":"BTCUSDT","p":"1","T":1})").has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON(R"({"e":"trade","s":"BTCUSDT","p":"1","q":"1"})").has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON(R"({"e":"trade","s":"BTCUSDT","p":1.5,"q":"1","T":1})").has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON(R"({"e":"trade","s":"BTCUSDT","p":"1","q":"1","T":"1"})").has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON(R"({"e":"trade","s":"BTCUSDT","p":"1","q":"1","T":-5})").has_value());
}

// std::stod would have accepted every one of these.
TEST(Trade, RejectsDecimalsThatAreNotPlainNumbers)
{
	auto with_price = [](const char* price) {
		return std::string(R"({"e":"trade","s":"BTCUSDT","q":"1","T":1,"p":")") + price + R"("})";
	};

	EXPECT_FALSE(binagg::Trade::FromJSON(with_price("12abc")).has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON(with_price(" 1")).has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON(with_price("+1")).has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON(with_price("1e5")).has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON(with_price("0x1p3")).has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON(with_price("inf")).has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON(with_price("nan")).has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON(with_price("-5")).has_value());
	EXPECT_FALSE(binagg::Trade::FromJSON(with_price("")).has_value());

	EXPECT_TRUE(binagg::Trade::FromJSON(with_price("755.17000000")).has_value());
	EXPECT_TRUE(binagg::Trade::FromJSON(with_price("0")).has_value());
}

TEST(Trade, ReportsAReason)
{
	const auto result = binagg::Trade::FromJSON(R"({"e":"trade","s":"BTCUSDT","p":"1","q":"1"})");
	ASSERT_FALSE(result.has_value());
	EXPECT_NE(result.error().find("T"), std::string::npos);
}
