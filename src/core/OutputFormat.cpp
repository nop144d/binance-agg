#include "OutputFormat.hpp"

namespace binagg
{

std::string FormatUtcTimestamp(int64_t epoch_ms)
{
	// std::chrono rather than gmtime_r: portable, and no shared static buffer.
	const std::chrono::sys_seconds seconds{
		std::chrono::floor<std::chrono::seconds>(std::chrono::milliseconds{ epoch_ms }) };
	return std::format("{:%Y-%m-%dT%H:%M:%SZ}", seconds);
}

std::string FormatWindows(std::span<const ElapsedWindow> windows)
{
	std::string out;

	int64_t current_start = std::numeric_limits<int64_t>::min();
	for (const ElapsedWindow& window : windows) {
		if (window.window_start_ms != current_start) {
			current_start = window.window_start_ms;
			out += std::format("timestamp={}\n", FormatUtcTimestamp(current_start));
		}

		out += std::format("symbol={} trades={} volume={:.8f} min={:.8f} max={:.8f}\n",
			window.symbol, window.stats.trades, window.stats.volume,
			window.stats.min_price, window.stats.max_price);
	}

	return out;
}

} // namespace binagg
