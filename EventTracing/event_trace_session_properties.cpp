#include "event_tracing/event_trace_session_properties.h"

namespace event_tracing
{
event_trace_session_properties::event_trace_session_properties(const std::wstring& session_name)
	: data_(sizeof(EVENT_TRACE_PROPERTIES) + (session_name.size() + 1) * sizeof(std::wstring::value_type))
{
	auto& sessionProperties = *static_cast<PEVENT_TRACE_PROPERTIES>(*this);
	constexpr const ULONG QueryPerformanceCounterClientContext = 1;
	sessionProperties.Wnode.ClientContext = QueryPerformanceCounterClientContext;
	sessionProperties.Wnode.BufferSize = data_.size();
	sessionProperties.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
	sessionProperties.LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
}
} //namespace event_tracing
