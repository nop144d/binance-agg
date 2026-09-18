#include "app/Config.hpp"
#include "app/FileWriter.hpp"
#include "app/WSSession.hpp"
#include "core/Aggregator.hpp"
#include "core/Trade.hpp"

using namespace binagg;

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <config file>" << std::endl;
		return 1;
	}
	else {
		std::cout << "Config file: " << argv[1] << std::endl;
	}

    ssl::context ctx{ ssl::context::tlsv13_client };
	ctx.set_verify_mode(ssl::verify_none); // Disable certificate verification for testing purposes
    ctx.set_default_verify_paths();

    std::string host{"stream.binance.com"};
    std::string port{"9443"};
	const std::string target_base{ "/stream?streams=" };

	try {
		std::filesystem::path config_file_path(argv[1]);

		Config conf{ config_file_path };
		auto& symbols = conf.GetSymbols();
		std::string target = target_base;
		for (size_t i = 0; i < symbols.size(); ++i) {
			target += symbols[i] + "@trade";
			if (i < symbols.size() - 1) {
				target += "/";
			}
		}

		std::cout << "Symbols: ";
		for (const auto& symbol : symbols) {
			std::cout << symbol << " ";
		}
		std::cout << std::endl;
		std::cout << "Window (ms): " << conf.GetWindowMs() << std::endl;
		std::cout << "Flush Interval (ms): " << conf.GetFlushIntervalMs() << std::endl;
		std::cout << "Output File: " << conf.GetOutputFile() << std::endl;

		Aggregator agg{ conf.GetWindowMs() };

		FileWriter file_writer{ conf.GetOutputFile() };
		auto on_message = [&file_writer, &agg](std::string_view msg) {
			std::cout << "trade:\n" << msg << std::endl;

			auto result = Trade::FromJSON(msg);
			if (!result) {
				spdlog::error("Failed to parse trade: {}", result.error());
				return;
			}

			if (!agg.AddTrade(result.value())) {
				spdlog::error("Failed to add trade");
			}
			file_writer.Write(std::string(msg));
		};

		net::io_context ioc;
		std::make_shared<WSSession>(ioc, ctx)->run(
			std::move(host),
			std::move(port),
			std::move(target), on_message);

		ioc.run();
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

    return 0;
}
