#pragma once

#include <atomic>
#include <cstdint>
#include <map>
#include <string>
#include <thread>
#include <utility>

#include <boost/optional.hpp>
#include <boost/signals2.hpp>

#define INITGUID
#include <Windows.h>
#include <Evntcons.h>
#include <Evntrace.h>

#include "event_tracing/guid_helpers.h"
#include "event_tracing/event_trace_handle.h"

namespace event_tracing
{
class event_trace_session;
class event_trace
{
public:
	using event_processor = void(PEVENT_RECORD record);
	using event_processor_signal = boost::signals2::signal<event_processor>;
	using error_processor = void(std::uint32_t error);
	using error_processor_signal = boost::signals2::signal<error_processor>;
	using stop_processor = void();
	using stop_processor_signal = boost::signals2::signal<stop_processor>;

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
	boost::signals2::connection on_error(Handler&& handler)
	{
		return on_error_.connect(std::forward<Handler>(handler));
	}

	template<typename Handler>
	boost::signals2::connection on_trace_event(const ms_guid& trace_provider, Handler&& handler)
	{
		return on_provider_event_[trace_provider].connect(std::forward<Handler>(handler));
	}

	template<typename Handler>
	boost::signals2::connection on_trace_event(const ms_guid& trace_provider,
		USHORT event_id, Handler&& handler)
	{
		return on_provider_event_[{ trace_provider, event_id }].connect(std::forward<Handler>(handler));
	}

	template<typename Handler>
	boost::signals2::connection on_stop_trace(Handler&& handler)
	{
		return on_stop_trace_.connect(std::forward<Handler>(handler));
	}

	void run_async();
	void run();
	void stop();

private:
	void open_trace();
	void start_monitoring(bool throw_error);

	static void __stdcall static_process_trace_event(PEVENT_RECORD record);
	void process_trace_event(PEVENT_RECORD record, std::uint32_t error) noexcept;

private:
	struct event_key
	{
		event_key(const ms_guid& guid)
			: guid(guid)
		{
		}

		event_key(const ms_guid& guid, USHORT event_id)
			: guid(guid)
			, event_id(event_id)
		{
		}

		friend bool operator<(const event_key& left, const event_key& right) noexcept
		{
			return left.guid < right.guid
				|| (left.guid == right.guid && left.event_id < right.event_id);
		}

		ms_guid guid;
		boost::optional<USHORT> event_id;
	};

	std::wstring session_name_;
	event_processor_signal on_event_;
	error_processor_signal on_error_;
	stop_processor_signal on_stop_trace_;
	std::map<event_key, event_processor_signal> on_provider_event_;
	event_trace_handle trace_handle_;
	std::thread event_processor_;
	std::atomic_flag started_ = ATOMIC_FLAG_INIT;
};
} //namespace event_tracing
