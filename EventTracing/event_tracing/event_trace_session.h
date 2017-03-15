#pragma once
#include <cstdint>
#include <string>
#include <utility>

#include <Windows.h>
#include <Evntrace.h>

#include "event_tracing/guid_helpers.h"

namespace event_tracing
{
class event_trace_session
{
public:
	enum class trace_level : std::uint8_t
	{
		none = TRACE_LEVEL_NONE,
		critical = TRACE_LEVEL_CRITICAL,
		fatal = TRACE_LEVEL_FATAL,
		error = TRACE_LEVEL_ERROR,
		warning = TRACE_LEVEL_WARNING,
		information = TRACE_LEVEL_INFORMATION,
		verbose = TRACE_LEVEL_VERBOSE
	};

public:
	template<typename String>
	explicit event_trace_session(String&& session_name)
		: session_name_(std::forward<String>(session_name))
	{
		create_or_replace_trace_session();
	}

	event_trace_session(const event_trace_session&) = delete;
	event_trace_session& operator=(const event_trace_session&) = delete;

	~event_trace_session();

	void close_trace_session();
	void enable_trace(const ms_guid& provider_guid, trace_level level);
	void enable_trace(const ms_guid& provider_guid, trace_level level, std::uint64_t match_any_keywords);

	const std::wstring& get_name() const noexcept
	{
		return session_name_;
	}

private:
	void create_or_replace_trace_session();

	TRACEHANDLE handle_ = 0;
	std::wstring session_name_;
};
} //namespace event_tracing
