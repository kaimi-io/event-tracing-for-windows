#pragma once

#include <cstdint>
#include <memory>

#include <Windows.h>
#include <CommCtrl.h>

#include <boost/signals2.hpp>

#include "event_tracing/event_trace.h"
#include "event_tracing/event_trace_session.h"

#include "process.h"
#include "process_module.h"
#include "process_thread.h"

class process_list
{
public:
	using new_process_handler = void(const process&);
	using new_process_signal = boost::signals2::signal<new_process_handler>;
	using stopped_process_handler = void(const process&, std::uint32_t exit_code);
	using stopped_process_signal = boost::signals2::signal<stopped_process_handler>;
	using new_thread_handler = void(const process&, const process_thread&);
	using new_thread_signal = boost::signals2::signal<new_thread_handler>;
	using stopped_thread_handler = void(const process&, const process_thread&);
	using stopped_thread_signal = boost::signals2::signal<new_thread_handler>;
	using loaded_module_handler = void(const process&, const process_module&);
	using loaded_module_signal = boost::signals2::signal<loaded_module_handler>;
	using unloaded_module_handler = void(const process&, const process_module&);
	using unloaded_module_signal = boost::signals2::signal<unloaded_module_handler>;

	using error_handler = void(std::uint32_t error_code);
	using error_signal = boost::signals2::signal<error_handler>;
	using stop_handler = void();
	using stop_signal = boost::signals2::signal<stop_handler>;

public:
	void start_tracking();

	template<typename Handler>
	boost::signals2::connection on_new_process(Handler&& handler)
	{
		return on_new_process_.connect(std::forward<Handler>(handler));
	}

	template<typename Handler>
	boost::signals2::connection on_stopped_process(Handler&& handler)
	{
		return on_stopped_process_.connect(std::forward<Handler>(handler));
	}

	template<typename Handler>
	boost::signals2::connection on_new_thread(Handler&& handler)
	{
		return on_new_thread_.connect(std::forward<Handler>(handler));
	}

	template<typename Handler>
	boost::signals2::connection on_stopped_thread(Handler&& handler)
	{
		return on_stopped_thread_.connect(std::forward<Handler>(handler));
	}

	template<typename Handler>
	boost::signals2::connection on_loaded_module(Handler&& handler)
	{
		return on_loaded_module_.connect(std::forward<Handler>(handler));
	}

	template<typename Handler>
	boost::signals2::connection on_unloaded_module(Handler&& handler)
	{
		return on_unloaded_module_.connect(std::forward<Handler>(handler));
	}

	template<typename Handler>
	boost::signals2::connection on_error(Handler&& handler)
	{
		return on_error_.connect(std::forward<Handler>(handler));
	}

	template<typename Handler>
	boost::signals2::connection on_stop_trace(Handler&& handler)
	{
		return on_stop_trace_.connect(std::forward<Handler>(handler));
	}

private:
	void on_process_started(PEVENT_RECORD record);
	void on_process_stopped(PEVENT_RECORD record);
	void on_thread_started(PEVENT_RECORD record);
	void on_thread_stopped(PEVENT_RECORD record);
	void on_image_loaded(PEVENT_RECORD record);
	void on_image_unloaded(PEVENT_RECORD record);

private:
	std::map<std::uint32_t, process> processes_;
	new_process_signal on_new_process_;
	stopped_process_signal on_stopped_process_;
	new_thread_signal on_new_thread_;
	stopped_thread_signal on_stopped_thread_;
	loaded_module_signal on_loaded_module_;
	unloaded_module_signal on_unloaded_module_;
	error_signal on_error_;
	stop_signal on_stop_trace_;
	std::unique_ptr<event_tracing::event_trace_session> sess_;
	std::unique_ptr<event_tracing::event_trace> trace_;
};
