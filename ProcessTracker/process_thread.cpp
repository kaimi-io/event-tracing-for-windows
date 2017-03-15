#include "process_thread.h"

#include "event_tracing/event_info.h"

process_thread::process_thread(PEVENT_RECORD record)
{
	event_tracing::event_info info(record);
	pid_ = info.get_plain_property_value<std::uint32_t>(L"ProcessID");
	tid_ = info.get_plain_property_value<std::uint32_t>(L"ThreadID");

	ep_ = info.get_plain_property_value<event_tracing::event_type_pointer>(L"StartAddr");
	user_stack_base_ = info.get_plain_property_value<event_tracing::event_type_pointer>(L"UserStackBase");
	user_stack_limit_ = info.get_plain_property_value<event_tracing::event_type_pointer>(L"UserStackLimit");
}
