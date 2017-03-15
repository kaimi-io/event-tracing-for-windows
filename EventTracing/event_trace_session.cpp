#include "event_tracing/event_trace_session.h"

#include <cassert>
#include <limits>

#include "event_tracing/guid_helpers.h"
#include "event_tracing/event_trace_error.h"
#include "event_tracing/event_trace_session_properties.h"

namespace event_tracing
{
event_trace_session::~event_trace_session()
{
	try
	{
		close_trace_session();
	}
	catch (...)
	{
		assert(false);
	}
}

void event_trace_session::enable_trace(const ms_guid& provider_guid,
	trace_level level, std::uint64_t match_any_keywords)
{
	ENABLE_TRACE_PARAMETERS params{};
	params.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
	auto result = ::EnableTraceEx2(handle_, provider_guid.native_ptr(), EVENT_CONTROL_CODE_ENABLE_PROVIDER,
		static_cast<UCHAR>(level), match_any_keywords, 0, 0, &params);
	if (ERROR_SUCCESS != result)
		throw event_trace_error("Unable to enable trace for provider", result);
}

void event_trace_session::enable_trace(const ms_guid& provider_guid, trace_level level)
{
	return enable_trace(provider_guid, level, (std::numeric_limits<std::uint64_t>::max)());
}

void event_trace_session::create_or_replace_trace_session()
{
	event_trace_session_properties props(session_name_);
	ULONG result = 0;
	bool closed = false;
	for (int i = 0; i < 2; ++i)
	{
		switch (result = ::StartTraceW(&handle_, session_name_.c_str(), props))
		{
		case ERROR_SUCCESS:
			return;

		case ERROR_ALREADY_EXISTS:
			if (i)
				break;

			close_trace_session();
			continue;

		default:
			break;
		}
	}

	throw event_trace_error("Unable to start trace session", result);
}

void event_trace_session::close_trace_session()
{
	event_trace_session_properties props(session_name_);
	auto result = ::ControlTraceW(0, session_name_.c_str(), props, EVENT_TRACE_CONTROL_STOP);
	if (ERROR_SUCCESS != result && ERROR_WMI_INSTANCE_NOT_FOUND != result)
		throw event_trace_error("Unable to stop trace session", result);
}
} //namespace event_tracing
