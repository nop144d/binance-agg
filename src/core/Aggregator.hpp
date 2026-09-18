#pragma once

namespace binagg
{

struct Trade {
    std::string symbol;
    double price;
    double quantity;
    int64_t trade_time_ms{ 0 };
};

struct WindowStats
{
	uint64_t trades{ 0 };
	double volume{ 0.0 };
	double min_price{ std::numeric_limits<double>::max() };
	double max_price{ std::numeric_limits<double>::lowest() };
};

struct WindowKey {
	int64_t window_start_ms{ 0 };
	std::string symbol;

	friend auto operator<=>(const WindowKey&, const WindowKey&) = default;
};

class Aggregator
{
public:
	Aggregator(int64_t window_ms);
	Aggregator(const Aggregator&) = delete;
	Aggregator& operator=(const Aggregator&) = delete;
	void AddTrade(std::string_view trade);

private:
	static std::expected<Trade, std::string> ParseTrade(std::string_view trade_str);

	int64_t window_ms_;
	std::map<WindowKey, WindowStats> windows_;
};

} // namespace binagg
