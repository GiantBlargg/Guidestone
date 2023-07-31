#include "input.hpp"

void Input::push_event(const Event& event) {
	queue_mutex.lock();
	event_queue[active_queue].push_back(event);
	queue_mutex.unlock();
}

const std::vector<Input::Event>& Input::get_events() {
	queue_mutex.lock();
	const std::vector<Event>& ret = event_queue[active_queue];
	active_queue = (active_queue + 1) % event_queue.size();
	event_queue[active_queue].clear();
	queue_mutex.unlock();
	return ret;
}
