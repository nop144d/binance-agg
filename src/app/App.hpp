#pragma once

#include "WSSession.hpp"
#include "core/Aggregator.hpp"
#include "core/FileWriter.hpp"

namespace binagg
{

class Config;

class App {
public:
	explicit App(std::unique_ptr<Config> config);

	// Defined in the .cpp, where Config is a complete type: a unique_ptr member
	// to a forward-declared class cannot be destroyed from a translation unit
	// that has only seen the declaration.
	~App();

	App(const App&) = delete;
	App& operator=(const App&) = delete;

	// Blocks until shutdown. Returns the process exit code.
	int Run();

private:
	void OnMessage(std::string_view payload);
	void OnSessionClosed(beast::error_code ec);
	void OnSignal(int signal_number);
	void ScheduleFlush();
	void Flush();
	void BeginShutdown(int exit_code);
	void FinishShutdown();
	std::string BuildTarget() const;

	static int64_t NowMs();

	std::unique_ptr<Config> config_;
	net::io_context ioc_;
	ssl::context ssl_ctx_;
	net::steady_timer flush_timer_;
	net::signal_set signals_;
	Aggregator aggregator_;
	FileWriter writer_;
	std::shared_ptr<WSSession> session_;

	uint64_t received_{ 0 };
	uint64_t rejected_{ 0 };
	uint64_t written_{ 0 };
	bool session_active_{ false };
	bool shutting_down_{ false };
	int exit_code_{ 0 };
};

} // namespace binagg
