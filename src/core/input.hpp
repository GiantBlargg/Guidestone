#pragma once

#include "types.hpp"
#include <array>
#include <mutex>
#include <variant>
#include <vector>

namespace Event {

struct Resize {
	u32 width, height;
};

struct Quit {};

using Event = std::variant<Resize, Quit>;

} // namespace Event

class Input {
  public:
	using Event = Event::Event;

  private:
	std::mutex queue_mutex;
	std::array<std::vector<Event>, 2> event_queue;
	size_t active_queue = 0;

  public:
	void push_event(const Event&);
	// queue is double buffered, returned queue will be erased next call to get_events
	const std::vector<Event>& get_events();
};
