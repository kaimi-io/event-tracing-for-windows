#include "process.h"

#include "event_tracing/event_info.h"

process::process(PEVENT_RECORD record)
{
	event_tracing::event_info info(record);
	path_ = info.get_plain_property_value<std::wstring>(L"ImageName");
	pid_ = info.get_plain_property_value<std::uint32_t>(L"ProcessID");
	parent_pid_ = info.get_plain_property_value<std::uint32_t>(L"ParentProcessID");
	session_id_ = info.get_plain_property_value<std::uint32_t>(L"SessionID");
}

process_thread& process::add_thread(process_thread&& thread)
{
	return (*threads_.emplace(thread.get_tid(), std::move(thread)).first).second;
}

void process::remove_thread(std::uint32_t tid)
{
	threads_.erase(tid);
}

process_thread* process::get_thread(std::uint32_t tid)
{
	auto it = threads_.find(tid);
	if (it == threads_.cend())
		return nullptr;

	return &(*it).second;
}

process_module& process::add_module(process_module&& module)
{
	return (*modules_.emplace(module.get_image_base(), std::move(module)).first).second;
}

void process::remove_module(std::uint64_t image_base)
{
	modules_.erase(image_base);
}

process_module* process::get_module(std::uint64_t image_base)
{
	auto it = modules_.find(image_base);
	if (it == modules_.cend())
		return nullptr;

	return &(*it).second;
}
