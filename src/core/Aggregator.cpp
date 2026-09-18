#include "Aggregator.hpp"
#include "Trade.hpp"

namespace binagg
{

Aggregator::Aggregator(int64_t window_ms) : window_ms_(window_ms)
{
	if (window_ms_ <= 0) {
		throw std::invalid_argument("window_ms must be greater than 0");
	}
}

void Aggregator::AddTrade(const Trade& trade)
{
	int64_t window_start_ms = (trade.trade_time_ms / window_ms_) * window_ms_;
	WindowKey key{ window_start_ms, trade.symbol };
	auto& stats = windows_[key];
	stats.trades++;
	stats.volume += trade.quantity * trade.price;
	stats.min_price = std::min(stats.min_price, trade.price);
	stats.max_price = std::max(stats.max_price, trade.price);
}

} // namespace binagg
