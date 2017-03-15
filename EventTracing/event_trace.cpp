#include "event_tracing/event_trace.h"

#include <cassert>

#include "event_tracing/guid_helpers.h"
#include "event_tracing/event_trace_error.h"
#include "event_tracing/event_trace_session.h"

namespace event_tracing
{
event_trace::event_trace(const event_trace_session& session)
	: session_name_(session.get_name())
{
	open_trace();
}

event_trace::~event_trace()
{
	try
	{
		on_event_.disconnect_all_slots();
		for (auto& pair : on_provider_event_)
			pair.second.disconnect_all_slots();
		stop();
	}
	catch (...)
	{
		assert(false);
	}
}

void event_trace::run_async()
{
	if (started_.test_and_set())
		return;

	event_processor_ = std::thread([this]
	{
		start_monitoring(false);
	});
}

void event_trace::run()
{
	if (started_.test_and_set())
		return;

	start_monitoring(true);
}

void event_trace::stop()
{
	if (!trace_handle_.close())
	{
		assert(false);
		if(event_processor_.joinable())
			event_processor_.detach();
	}
	else
	{
		if (event_processor_.joinable())
			event_processor_.join();
	}

	started_.clear();
}

void event_trace::open_trace()
{
	EVENT_TRACE_LOGFILEW trace{};
	auto session_name = session_name_;
	trace.LoggerName = &session_name[0];
	trace.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
	trace.EventRecordCallback = static_process_trace_event;
	trace.Context = this;

	trace_handle_.reset(::OpenTraceW(&trace));
	if (!trace_handle_.is_valid())
		throw event_trace_error("Unable to open trace", ::GetLastError());
}

void event_trace::start_monitoring(bool throw_error)
{
	auto handle = trace_handle_.get();
	auto result = ::ProcessTrace(&handle, 1, 0, 0);
	if (ERROR_SUCCESS != result && ERROR_CANCELLED != result)
	{
		if (throw_error)
			throw event_trace_error("Unable to start trace monitoring", result);

		process_trace_event(nullptr, result);
	}
	else
	{
		if (throw_error)
		{
			on_stop_trace_();
			return;
		}

		try
		{
			on_stop_trace_();
		}
		catch (const std::exception&)
		{
			assert(false);
		}
	}
}

void __stdcall event_trace::static_process_trace_event(PEVENT_RECORD record)
{
	if (record->UserContext)
		static_cast<event_trace*>(record->UserContext)->process_trace_event(record, 0u);
}

void event_trace::process_trace_event(PEVENT_RECORD record, std::uint32_t error) noexcept
{
	try
	{
		if (error)
		{
			on_error_(error);
			return;
		}

		if (record->EventHeader.ProviderId == EventTraceGuid)
			return;

		on_event_(record);

		auto it = on_provider_event_.find({ record->EventHeader.ProviderId });
		if (it != on_provider_event_.cend())
			(*it).second(record);

		it = on_provider_event_.find({ record->EventHeader.ProviderId, record->EventHeader.EventDescriptor.Id });
		if (it != on_provider_event_.cend())
			(*it).second(record);
	}
	catch (...)
	{
		assert(false);
	}
}
} //namespace event_tracing
