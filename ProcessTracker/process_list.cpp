#include "process_list.h"

#include "event_tracing/event_info.h"
#include "event_tracing/event_provider_list.h"

void process_list::start_tracking()
{
	using namespace event_tracing;

	auto process_provider_guid = event_provider_list().get_guid(L"Microsoft-Windows-Kernel-Process");
	sess_ = std::make_unique<event_trace_session>(L"Kaimi.io test session");
	static constexpr const std::uint64_t keyword_process = 0x10;
	static constexpr const std::uint64_t keyword_thread = 0x20;
	static constexpr const std::uint64_t keyword_image = 0x40;
	sess_->enable_trace(process_provider_guid, event_trace_session::trace_level::verbose,
		keyword_process | keyword_thread | keyword_image);
	trace_ = std::make_unique<event_trace>(*sess_);

	static constexpr const USHORT event_process_started = 1;
	static constexpr const USHORT event_process_stopped = 2;
	static constexpr const USHORT event_thread_started = 3;
	static constexpr const USHORT event_thread_stopped = 4;
	static constexpr const USHORT image_loaded = 5;
	static constexpr const USHORT image_unloaded = 6;
	trace_->on_trace_event(process_provider_guid, event_process_started, [this](auto event_record)
	{
		on_process_started(event_record);
	});
	trace_->on_trace_event(process_provider_guid, event_process_stopped, [this](auto event_record)
	{
		on_process_stopped(event_record);
	});
	trace_->on_trace_event(process_provider_guid, event_thread_started, [this](auto event_record)
	{
		on_thread_started(event_record);
	});
	trace_->on_trace_event(process_provider_guid, event_thread_stopped, [this](auto event_record)
	{
		on_thread_stopped(event_record);
	});
	trace_->on_trace_event(process_provider_guid, image_loaded, [this](auto event_record)
	{
		on_image_loaded(event_record);
	});
	trace_->on_trace_event(process_provider_guid, image_unloaded, [this](auto event_record)
	{
		on_image_unloaded(event_record);
	});
	trace_->on_error([this](std::uint32_t error)
	{
		on_error_(error);
	});
	trace_->on_stop_trace([this]()
	{
		on_stop_trace_();
	});

	trace_->run_async();
}

void process_list::on_process_started(PEVENT_RECORD record)
{
	process new_process(record);
	auto it = processes_.emplace(new_process.get_pid(), std::move(new_process)).first;
	on_new_process_((*it).second);
}

void process_list::on_process_stopped(PEVENT_RECORD record)
{
	event_tracing::event_info info(record);
	auto pid = info.get_plain_property_value<std::uint32_t>(L"ProcessID");
	auto exit_code = info.get_plain_property_value<std::uint32_t>(L"ExitCode");
	auto it = processes_.find(pid);
	if (it != processes_.cend())
	{
		on_stopped_process_((*it).second, exit_code);
		processes_.erase(it);
	}
}

void process_list::on_thread_started(PEVENT_RECORD record)
{
	auto thread = process_thread(record);
	auto it = processes_.find(thread.get_pid());
	if (it != processes_.cend())
		on_new_thread_((*it).second, (*it).second.add_thread(std::move(thread)));
}

void process_list::on_thread_stopped(PEVENT_RECORD record)
{
	auto thread = process_thread(record);
	auto it = processes_.find(thread.get_pid());
	if (it != processes_.cend())
	{
		auto thread_ptr = (*it).second.get_thread(thread.get_tid());
		if (thread_ptr)
		{
			on_stopped_thread_((*it).second, *thread_ptr);
			(*it).second.remove_thread(thread.get_tid());
		}
	}
}

void process_list::on_image_loaded(PEVENT_RECORD record)
{
	auto module = process_module(record);
	auto it = processes_.find(module.get_pid());
	if (it != processes_.cend())
		on_loaded_module_((*it).second, (*it).second.add_module(std::move(module)));
}

void process_list::on_image_unloaded(PEVENT_RECORD record)
{
	auto module = process_module(record);
	auto it = processes_.find(module.get_pid());
	if (it != processes_.cend())
	{
		auto module_ptr = (*it).second.get_module(module.get_image_base());
		if (module_ptr)
		{
			on_unloaded_module_((*it).second, *module_ptr);
			(*it).second.remove_module(module.get_image_base());
		}
	}
}
