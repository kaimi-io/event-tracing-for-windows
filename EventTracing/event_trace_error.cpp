#include "event_tracing/event_trace_error.h"

namespace event_tracing
{
event_trace_error::event_trace_error(const std::string& text, std::uint32_t error_code)
	: std::runtime_error(text)
	, error_code_(error_code)
{
}

event_trace_error::event_trace_error(const std::string& text)
	: event_trace_error(text, 0u)
{
}
} //namespace event_tracing
