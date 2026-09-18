#include "App.hpp"
#include "core/Config.hpp"
#include "core/OutputFormat.hpp"
#include "core/Trade.hpp"

namespace binagg
{

namespace
{

constexpr const char* kHost = "stream.binance.com";
constexpr const char* kPort = "9443";

} // namespace

App::App(std::unique_ptr<Config> config) :
	config_(std::move(config)),
	ssl_ctx_(ssl::context::tlsv13_client),
	flush_timer_(ioc_),
	signals_(ioc_, SIGINT, SIGTERM),
	aggregator_(config_->GetWindowMs()),
	writer_(config_->GetOutputFile())
{
	ssl_ctx_.set_verify_mode(ssl::verify_none); // TODO: enable verification for production
	ssl_ctx_.set_default_verify_paths();
}

App::~App() = default;

int64_t App::NowMs()
{
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

std::string App::BuildTarget() const
{
	std::string target{ "/stream?streams=" };

	const auto& symbols = config_->GetSymbols();
	for (size_t i = 0; i < symbols.size(); ++i) {
		if (i > 0) {
			target += "/";
		}
		target += symbols[i] + "@trade";
	}

	return target;
}

int App::Run()
{
	const std::string target = BuildTarget();

	spdlog::info("config: window={}ms flush={}ms output={}",
		config_->GetWindowMs(), config_->GetFlushIntervalMs(), config_->GetOutputFile());
	spdlog::info("connecting to wss://{}:{}{}", kHost, kPort, target);

	session_ = std::make_shared<WSSession>(ioc_, ssl_ctx_);
	session_active_ = true;
	session_->run(kHost, kPort, target,
		[this](std::string_view payload) { OnMessage(payload); },
		[this](beast::error_code ec) { OnSessionClosed(ec); });

	signals_.async_wait([this](const beast::error_code& ec, int signal_number) {
		if (!ec) {
			OnSignal(signal_number);
		}
		});

	ScheduleFlush();

	ioc_.run();

	spdlog::info("exiting with code {} (received={} rejected={} dropped={} written={})",
		exit_code_, received_, rejected_, aggregator_.GetDroppedCount(), written_);

	return exit_code_;
}

void App::OnMessage(std::string_view payload)
{
	++received_;
	spdlog::trace("trade: {}", payload);

	const auto trade = Trade::FromJSON(payload);
	if (!trade) {
		++rejected_;
		spdlog::warn("skipping payload ({})", trade.error());
		return;
	}

	if (!aggregator_.AddTrade(trade.value())) {
		spdlog::debug("dropped late trade {} T={}", trade->symbol, trade->trade_time_ms);
	}
}

void App::ScheduleFlush()
{
	flush_timer_.expires_after(std::chrono::milliseconds(config_->GetFlushIntervalMs()));
	flush_timer_.async_wait([this](const beast::error_code& ec) {
		if (ec) {
			return; // cancelled during shutdown
		}
		Flush();
		ScheduleFlush();
		});
}

void App::Flush()
{
	const auto windows = aggregator_.ExtractElapsed(NowMs());
	if (!windows.empty()) {
		writer_.Write(FormatWindows(windows));
		written_ += windows.size();
	}

	spdlog::info("flush: wrote {} symbol-window(s), received={} rejected={} dropped={}",
		windows.size(), received_, rejected_, aggregator_.GetDroppedCount());
}

void App::OnSignal(int signal_number)
{
	spdlog::info("received signal {}, shutting down", signal_number);
	BeginShutdown(0);
}

void App::OnSessionClosed(beast::error_code ec)
{
	session_active_ = false;

	if (shutting_down_) {
		FinishShutdown();
		return;
	}

	// Reconnection is out of scope, so a lost connection ends the process.
	spdlog::error("connection lost: {}", ec ? ec.message() : "closed by peer");
	BeginShutdown(1);
}

void App::BeginShutdown(int exit_code)
{
	if (shutting_down_) {
		return;
	}
	shutting_down_ = true;
	exit_code_ = exit_code;

	flush_timer_.cancel();
	signals_.cancel();
	signals_.clear(); // restore default disposition so a second signal kills us

	if (session_active_) {
		session_->close(); // FinishShutdown runs from OnSessionClosed
	}
	else {
		FinishShutdown();
	}
}

void App::FinishShutdown()
{
	// Order matters: the final flush submits the last block, and only then may
	// the writer stop, because Stop() drains what has already been submitted.
	Flush();
	writer_.Stop();

	spdlog::info("output file closed, connection closed");
}

} // namespace binagg
