#pragma once

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

	int Run();

private:
	std::unique_ptr<Config> config_;
};

} // namespace binagg
