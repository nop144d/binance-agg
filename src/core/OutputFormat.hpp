#pragma once

#include "Aggregator.hpp"

namespace binagg
{

// "2026-01-12T14:23:20Z"
std::string FormatUtcTimestamp(int64_t epoch_ms);

// One block per window start, ordered as ExtractElapsed returns them:
//
//   timestamp=2026-01-12T14:23:20Z
//   symbol=BTCUSDT trades=154 volume=23.51000000 min=43012.10000000 max=43189.40000000
std::string FormatWindows(std::span<const ElapsedWindow> windows);

} // namespace binagg
