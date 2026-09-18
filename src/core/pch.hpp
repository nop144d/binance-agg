#pragma once

// Precompiled header for binance_agg_core; propagates to the executable and the
// tests. Standard library and non-network third-party headers only — core must
// stay free of Boost so the tests can link it without the transport layer.

// Standard library
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

// Third-party
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
