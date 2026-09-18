#include "core/FileWriter.hpp"

#include <gtest/gtest.h>

namespace {

class TempPath {
public:
	TempPath()
		: path_(std::filesystem::temp_directory_path() /
			("binagg_writer_" + std::to_string(counter_++) + ".txt"))
	{
	}

	~TempPath() { std::error_code ec; std::filesystem::remove(path_, ec); }

	std::string String() const { return path_.string(); }

	std::string Contents() const
	{
		std::ifstream in(path_);
		std::ostringstream buffer;
		buffer << in.rdbuf();
		return buffer.str();
	}

private:
	static inline int counter_ = 0;
	std::filesystem::path path_;
};

} // namespace

TEST(FileWriter, WritesBlocksInSubmissionOrder)
{
	TempPath file;
	std::string expected;
	{
		binagg::FileWriter writer{ file.String() };
		for (int i = 0; i < 200; ++i) {
			const std::string block = "block " + std::to_string(i) + "\n";
			writer.Write(block);
			expected += block;
		}
		writer.Stop();
	}
	EXPECT_EQ(file.Contents(), expected);
}

// Stop() must write everything already submitted, which is what makes the
// shutdown sequence safe.
TEST(FileWriter, StopDrainsPendingWrites)
{
	TempPath file;
	constexpr int kBlocks = 500;

	binagg::FileWriter writer{ file.String() };
	for (int i = 0; i < kBlocks; ++i) {
		writer.Write("x\n");
	}
	writer.Stop();

	EXPECT_EQ(file.Contents().size(), static_cast<size_t>(kBlocks) * 2);
}

TEST(FileWriter, DestructorDrainsWithoutExplicitStop)
{
	TempPath file;
	{
		binagg::FileWriter writer{ file.String() };
		writer.Write("a\n");
		writer.Write("b\n");
	}
	EXPECT_EQ(file.Contents(), "a\nb\n");
}

TEST(FileWriter, StopIsIdempotent)
{
	TempPath file;
	binagg::FileWriter writer{ file.String() };
	writer.Write("only\n");
	writer.Stop();
	writer.Stop();
	EXPECT_EQ(file.Contents(), "only\n");
}

TEST(FileWriter, WriteAfterStopIsDiscarded)
{
	TempPath file;
	binagg::FileWriter writer{ file.String() };
	writer.Write("before\n");
	writer.Stop();
	writer.Write("after\n");
	EXPECT_EQ(file.Contents(), "before\n");
}

// FR-5: the output file is opened in append mode.
TEST(FileWriter, AppendsToAnExistingFile)
{
	TempPath file;
	{ std::ofstream(file.String()) << "existing\n"; }
	{
		binagg::FileWriter writer{ file.String() };
		writer.Write("appended\n");
	}
	EXPECT_EQ(file.Contents(), "existing\nappended\n");
}

TEST(FileWriter, ThrowsWhenTheFileCannotBeOpened)
{
	EXPECT_THROW(binagg::FileWriter{ "/no/such/directory/out.txt" }, std::runtime_error);
}

TEST(FileWriter, ToleratesConcurrentProducers)
{
	TempPath file;
	constexpr int kThreads = 4;
	constexpr int kPerThread = 250;
	{
		binagg::FileWriter writer{ file.String() };
		{
			std::vector<std::jthread> producers;
			for (int t = 0; t < kThreads; ++t) {
				producers.emplace_back([&writer] {
					for (int i = 0; i < kPerThread; ++i) {
						writer.Write("line\n");
					}
				});
			}
		}
		writer.Stop();
	}
	EXPECT_EQ(file.Contents().size(), static_cast<size_t>(kThreads) * kPerThread * 5);
}
