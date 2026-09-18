#pragma once

namespace binagg {

struct Trade {
    std::string symbol{};
    double price{};
    double quantity{};
    int64_t trade_time_ms{};

	static std::expected<Trade, std::string> FromJSON(std::string_view trade_str);
};

} // namespace binagg
