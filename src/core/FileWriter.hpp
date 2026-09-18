#pragma once

namespace binagg
{

class FileWriter
{
public:
	FileWriter(const std::string& output_file);
	~FileWriter();

	FileWriter(const FileWriter&) = delete;
	FileWriter& operator=(const FileWriter&) = delete;

	void Write(std::string data);

	void Stop();

private:
	void Run(std::stop_token stop_requested);

	std::ofstream output_;
	std::deque<std::string> queue_;
	std::condition_variable_any queue_ready_;
	std::mutex mutex_;
	bool stop_requested_{ false };
	std::jthread thread_;
};

} // namespace binagg
