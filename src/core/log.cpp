#include "log.hpp"

#include <iostream>

namespace Log {

struct Format {
	struct Level {
		const char *type_start, *msg_start, *msg_end;
	};
	const Level fatal, error, warning, info, trace;
};
static constexpr Format ANSI{
	.fatal = {"\033[1;31m", "\n\033[0m", "\n"},
	.error = {"\033[1;31m", "\n\033[0m", "\n"},
	.warning = {"\033[1;33m", "\n\033[0m", "\n"},
	.info = {"\033[1;36m", "\n\033[0m", "\n"},
	.trace = {"\033[1;32m", "\n\033[0m", "\n"},
};
static constexpr Format Plain{
	.fatal = {"", "\n", "\n"},
	.error = {"", "\n", "\n"},
	.warning = {"", "\n", "\n"},
	.info = {"", "\n", "\n"},
	.trace = {"", "\n", "\n"},
};

void log(const Msg& msg) {
	const Format& format = ANSI;
	Format::Level level_fmt;

	switch (msg.severity) {
	case Log::Severity::Fatal:
		level_fmt = format.fatal;
		break;
	case Log::Severity::Error:
		level_fmt = format.error;
		break;
	case Log::Severity::Warning:
		level_fmt = format.warning;
		break;
	case Log::Severity::Info:
		level_fmt = format.info;
		break;
	case Log::Severity::Trace:
		level_fmt = format.trace;
		break;
	}

	std::clog << level_fmt.type_start << msg.type << level_fmt.msg_start;
	if (msg.msg != "")
		std::clog << msg.msg << level_fmt.msg_end;
	std::clog << std::flush;
}

} // namespace Log
