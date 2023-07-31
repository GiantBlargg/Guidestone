#pragma once

#include <memory>
#include <vector>

namespace Log {

enum class Severity { Fatal, Error, Warning, Info, Trace };
struct Msg {
	Severity severity;
	std::string type;
	std::string msg;
};
void log(const Msg&);

inline void fatal(std::string type, std::string msg = "") {
	return log(Msg{.severity = Severity::Fatal, .type = type, .msg = msg});
}
inline void error(std::string type, std::string msg = "") {
	return log(Msg{.severity = Severity::Error, .type = type, .msg = msg});
}
inline void warn(std::string type, std::string msg = "") {
	return log(Msg{.severity = Severity::Warning, .type = type, .msg = msg});
}
inline void info(std::string type, std::string msg = "") {
	return log(Msg{.severity = Severity::Info, .type = type, .msg = msg});
}
inline void trace(std::string type, std::string msg = "") {
	return log(Msg{.severity = Severity::Trace, .type = type, .msg = msg});
}

} // namespace Log
