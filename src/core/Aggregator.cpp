#include "Aggregator.hpp"
#include "Trade.hpp"

namespace binagg
{

Aggregator::Aggregator(int64_t window_ms) :
		window_ms_(window_ms),
		extracted_until_ms_(std::numeric_limits<int64_t>::min()),
		dropped_count_(0)
{
	if (window_ms_ <= 0) {
		throw std::invalid_argument("window_ms must be greater than 0");
	}
	extracted_until_ms_ = std::numeric_limits<int64_t>::min() + window_ms_;
}

bool Aggregator::AddTrade(const Trade& trade)
{
	int64_t window_start_ms = (trade.trade_time_ms / window_ms_) * window_ms_;
	if (window_start_ms <= extracted_until_ms_ - window_ms_ || trade.trade_time_ms < 0) {
		++dropped_count_;
		return false;
	}

	WindowKey key{ window_start_ms, trade.symbol };
	auto& stats = windows_[key];
	stats.trades++;
	stats.volume += trade.quantity * trade.price;
	stats.min_price = std::min(stats.min_price, trade.price);
	stats.max_price = std::max(stats.max_price, trade.price);

	return true;
}

std::vector<ElapsedWindow> Aggregator::ExtractElapsed(int64_t now_ms)
{
	std::vector<ElapsedWindow> elapsed;
	elapsed.reserve(windows_.size());

	auto it = windows_.begin();
	while (it != windows_.end() && it->first.window_start_ms <= now_ms - window_ms_) {
		elapsed.push_back(ElapsedWindow{
			.window_start_ms = it->first.window_start_ms,
			.symbol = it->first.symbol,
			.stats = it->second,
			});
		it = windows_.erase(it);
	}

	if (now_ms > extracted_until_ms_) {
		extracted_until_ms_ = now_ms;
	}

	return elapsed;
}

} // namespace binagg
