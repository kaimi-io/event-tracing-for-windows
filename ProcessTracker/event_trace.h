#pragma once

#include <atomic>
#include <cstdint>
#include <map>
#include <string>
#include <thread>
#include <utility>

#include <boost/signals2.hpp>

#define INITGUID
#include <Windows.h>
#include <Evntcons.h>
#include <Evntrace.h>

#include "guid_helpers.h"
#include "event_trace_handle.h"

class event_trace_session;
class event_trace
{
public:
	using event_processor = void(PEVENT_RECORD record, std::uint32_t error);
	using event_processor_signal = boost::signals2::signal<event_processor>;

public:
	explicit event_trace(const event_trace_session& session);

	event_trace(const event_trace&) = delete;
	event_trace& operator=(const event_trace&) = delete;

	~event_trace();

	template<typename Handler>
	boost::signals2::connection on_trace_event(Handler&& handler)
	{
		return on_event_.connect(std::forward<Handler>(handler));
	}

	template<typename Handler>
	boost::signals2::connection on_trace_event(const ms_guid& trace_provider, Handler&& handler)
	{
		return on_provider_event_[trace_provider].connect(std::forward<Handler>(handler));
	}

	void run_async();
	void run();
	void stop();

private:
	void open_trace();
	void start_monitoring(bool throw_error);

	static void __stdcall static_process_trace_event(PEVENT_RECORD record);
	void process_trace_event(PEVENT_RECORD record, std::uint32_t error) noexcept;

	std::wstring session_name_;
	event_processor_signal on_event_;
	std::map<ms_guid, event_processor_signal> on_provider_event_;
	event_trace_handle trace_handle_;
	std::thread event_processor_;
	std::atomic_flag started_ = ATOMIC_FLAG_INIT;
};
