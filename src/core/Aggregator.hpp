#pragma once

namespace binagg
{

struct Trade;

struct WindowStats
{
	uint64_t trades{ 0 };
	double volume{ 0.0 };
	double min_price{ std::numeric_limits<double>::max() };
	double max_price{ std::numeric_limits<double>::lowest() };
};

struct ElapsedWindow {
	int64_t window_start_ms{ 0 };
	std::string symbol;
	WindowStats stats;
};

static_assert(std::is_nothrow_move_constructible_v<ElapsedWindow>,
	"vector reallocation would copy instead of move");

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
	[[nodiscard]] bool AddTrade(const Trade& trade);

	std::vector<ElapsedWindow> ExtractElapsed(int64_t now_ms);

	[[nodiscard]] uint64_t GetDroppedCount() const { return dropped_count_; }

private:
	int64_t window_ms_;
	int64_t extracted_until_ms_;
	uint64_t dropped_count_;
	std::map<WindowKey, WindowStats> windows_;
};

} // namespace binagg
