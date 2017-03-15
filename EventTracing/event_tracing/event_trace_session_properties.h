#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include <Windows.h>
#include <Evntrace.h>

namespace event_tracing
{
class event_trace_session_properties
{
public:
	explicit event_trace_session_properties(const std::wstring& session_name);

	operator PEVENT_TRACE_PROPERTIES() noexcept
	{
		return reinterpret_cast<PEVENT_TRACE_PROPERTIES>(data_.data());
	}

	operator const EVENT_TRACE_PROPERTIES*() const noexcept
	{
		return reinterpret_cast<const EVENT_TRACE_PROPERTIES*>(data_.data());
	}

private:
	using props_type = std::vector<std::uint8_t>;
	props_type data_;
};
} //namespace event_tracing
