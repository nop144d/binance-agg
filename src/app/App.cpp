#include "App.hpp"
#include "Config.hpp"
#include "FileWriter.hpp"
#include "WSSession.hpp"
#include "core/Aggregator.hpp"
#include "core/Trade.hpp"

namespace binagg
{

App::App(std::unique_ptr<Config> config) : config_(std::move(config))
{
}

App::~App() = default;

int App::Run()
{
	ssl::context ctx{ ssl::context::tlsv13_client };
	ctx.set_verify_mode(ssl::verify_none); // TODO: disable verification for testing, enable for production
	ctx.set_default_verify_paths();

	std::string host{ "stream.binance.com" };
	std::string port{ "9443" };

	std::string target{ "/stream?streams=" };
	const auto& symbols = config_->GetSymbols();
	for (size_t i = 0; i < symbols.size(); ++i) {
		target += symbols[i] + "@trade";
		if (i < symbols.size() - 1) {
			target += "/";
		}
	}

	spdlog::info("config: window={}ms flush={}ms output={}",
		config_->GetWindowMs(), config_->GetFlushIntervalMs(), config_->GetOutputFile());
	spdlog::info("connecting to wss://{}:{}{}", host, port, target);

	Aggregator agg{ config_->GetWindowMs() };
	FileWriter file_writer{ config_->GetOutputFile() };

	auto on_message = [&file_writer, &agg](std::string_view msg) {
		spdlog::trace("trade: {}", msg);

		auto result = Trade::FromJSON(msg);
		if (!result) {
			spdlog::error("Failed to parse trade: {}", result.error());
			return;
		}

		if (!agg.AddTrade(result.value())) {
			spdlog::warn("Dropped late trade for an already-extracted window");
		}
		file_writer.Write(std::string(msg));
	};

	net::io_context ioc;
	std::make_shared<WSSession>(ioc, ctx)->run(
		std::move(host),
		std::move(port),
		std::move(target), on_message);

	ioc.run();

	return 0;
}

} // namespace binagg
