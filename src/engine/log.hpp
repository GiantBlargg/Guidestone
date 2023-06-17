#pragma once

#include <memory>
#include <vector>

namespace Log {

enum class Severity { Fatal, Error, Warning, Info, Verbose };
struct Msg {
	Severity severity;
	std::string type;
	std::string msg;
};
void log(const Msg&);

} // namespace Log