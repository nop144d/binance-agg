#include "Trade.hpp"

namespace binagg
{

using json = nlohmann::json;

std::expected<Trade, std::string> Trade::FromJSON(std::string_view trade_str)
{
    const json document = json::parse(trade_str.begin(), trade_str.end(), nullptr, /*allow_exceptions=*/false);
    if (document.is_discarded()) {
        return std::unexpected("Invalid JSON");
    }
    if (!document.is_object()) {
        return std::unexpected("Payload is not a JSON object");
    }

    const json* event = &document;
    if (auto data = document.find("data"); data != document.end()) {
        if (!data->is_object()) {
            return std::unexpected("\"data\" is not an object");
        }
        event = &*data;
    }

    auto string_field = [&](const char* key) -> const std::string* {
        auto it = event->find(key);
        if (it == event->end() || !it->is_string()) {
            return nullptr;
        }
        return it->get_ptr<const std::string*>();
        };

    const std::string* type = string_field("e");
    if (type == nullptr || *type != "trade") {
        return std::unexpected("not a trade event");
    }

    const std::string* symbol = string_field("s");
    if (symbol == nullptr || symbol->empty()) {
        return std::unexpected("Missing or invalid \"s\"");
    }

    auto double_field = [&](const char* key) -> std::optional<double> {
        auto it = event->find(key);
        if (it == event->end() || !it->is_string()) {
            return std::nullopt;
        }
        const std::string& text = it->get_ref<const std::string&>();

        double value{};
        const char* const first = text.data();
        const char* const last = first + text.size();
        const auto [ptr, ec] = std::from_chars(first, last, value, std::chars_format::fixed);
        if (ec != std::errc{} || ptr != last) {
            return std::nullopt;
        }

        if (!(value >= 0.0 && value <= std::numeric_limits<double>::max())) {
            return std::nullopt;
        }
        return value;
        };

    const std::optional<double> price = double_field("p");
    if (!price) {
        return std::unexpected("Missing or invalid \"p\"");
    }

    const std::optional<double> quantity = double_field("q");
    if (!quantity) {
        return std::unexpected("Missing or invalid \"q\"");
    }

    const auto time_it = event->find("T");
    if (time_it == event->end() || !time_it->is_number_integer()) {
        return std::unexpected("Missing or invalid \"T\"");
    }
    const auto trade_time = time_it->get<std::int64_t>();
    if (trade_time < 0) {
        return std::unexpected("Negative \"T\"");
    }

    return Trade{
        .symbol = *symbol,
        .price = *price,
        .quantity = *quantity,
        .trade_time_ms = trade_time,
    };
}

} // namespace binagg
