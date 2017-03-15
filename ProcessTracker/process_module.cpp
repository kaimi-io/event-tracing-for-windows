#include "process_module.h"

#include "event_tracing/event_info.h"

process_module::process_module(PEVENT_RECORD record)
{
	event_tracing::event_info info(record);
	pid_ = info.get_plain_property_value<std::uint32_t>(L"ProcessID");
	image_base_ = info.get_plain_property_value<event_tracing::event_type_pointer>(L"ImageBase");
	image_name_ = info.get_plain_property_value<std::wstring>(L"ImageName");
}
