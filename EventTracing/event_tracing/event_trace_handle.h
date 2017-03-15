#pragma once

#include <Windows.h>
#include <Evntrace.h>

namespace event_tracing
{
class event_trace_handle
{
public:
	event_trace_handle() noexcept
		: handle_(INVALID_PROCESSTRACE_HANDLE)
	{
	}

	explicit event_trace_handle(TRACEHANDLE handle) noexcept
		: handle_(handle)
	{
	}

	event_trace_handle(const event_trace_handle&) = delete;
	event_trace_handle& operator=(const event_trace_handle&) = delete;

	~event_trace_handle()
	{
		close();
	}

	event_trace_handle(event_trace_handle&& other) noexcept
		: handle_(other.handle_)
	{
		other.handle_ = INVALID_PROCESSTRACE_HANDLE;
	}

	event_trace_handle& operator=(event_trace_handle&& other) noexcept
	{
		close();
		handle_ = other.handle_;
		other.handle_ = INVALID_PROCESSTRACE_HANDLE;
		return *this;
	}

	void reset(TRACEHANDLE handle) noexcept
	{
		close();
		handle_ = handle;
	}

	bool close() noexcept
	{
		if (is_valid())
		{
			auto result = ::CloseTrace(handle_);
			handle_ = INVALID_PROCESSTRACE_HANDLE;
			return result == ERROR_SUCCESS || result == ERROR_CTX_CLOSE_PENDING;
		}

		return true;
	}

	TRACEHANDLE get() const noexcept
	{
		return handle_;
	}

	bool is_valid() const noexcept
	{
		return handle_ != INVALID_PROCESSTRACE_HANDLE;
	}

private:
	TRACEHANDLE handle_;
};
} //namespace event_tracing
