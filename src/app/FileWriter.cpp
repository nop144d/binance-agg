#include "FileWriter.hpp"

namespace binagg
{

FileWriter::FileWriter(const std::string& output_file) :
	output_(output_file, std::ios::out | std::ios::app),
	thread_([this](std::stop_token stop_requested) { Run(std::move(stop_requested)); })
{
}

FileWriter::~FileWriter() {
	try {
		Stop();
	}
	catch (...) { }
}

void FileWriter::Stop() {
	{
		std::lock_guard lock(mutex_);
		if (stop_requested_) {
			return;
		}
		stop_requested_ = true;
	}

	thread_.request_stop();

	if (thread_.joinable()) {
		thread_.join();
	}
	output_.close();
}

void FileWriter::Write(std::string data)
{
	{
		std::lock_guard lock(mutex_);
		if (stop_requested_) {
			spdlog::error("write after stop: {} bytes discarded", data.size());
			return;
		}
		queue_.push_back(std::move(data));
	}
	queue_ready_.notify_one();
}

void FileWriter::Run(std::stop_token stop_requested) {
    while (true) {
        std::deque<std::string> batch;
        {
            std::unique_lock lock(mutex_);
            queue_ready_.wait(lock, stop_requested, [this] { return !queue_.empty(); });
            if (queue_.empty()) {
                return;
            }
            batch.swap(queue_);
        }

        std::size_t total_size = 0;
        for (const std::string& block : batch) {
            total_size += block.size();
        }
        std::string combined;
        combined.reserve(total_size);
        for (const std::string& block : batch) {
            combined += block;
        }

		spdlog::debug("writing {} bytes to output file", combined.size());

		output_ << combined;
		output_.flush();
	}
}

} // namespace binagg
