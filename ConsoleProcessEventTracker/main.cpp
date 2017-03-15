/* Process Tracker (c) DX, kaimi.io */

#include <iostream>
#include <stdexcept>

#include <Windows.h>

#include "event_tracing/elevated_check.h"
#include "event_tracing/event_provider_list.h"
#include "event_tracing/event_info.h"
#include "event_tracing/event_trace.h"
#include "event_tracing/event_trace_session.h"
#include "event_tracing/event_trace_error.h"

event_tracing::event_trace* global_trace = nullptr;
event_tracing::event_trace_session* global_session = nullptr;

BOOL WINAPI console_handler(DWORD signal)
{
	if ((signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT || signal == CTRL_CLOSE_EVENT)
		&& global_trace)
	{
		global_trace->stop();
		global_trace = nullptr;
	}

	if (signal == CTRL_CLOSE_EVENT && global_session)
		global_session->close_trace_session();

	return TRUE;
}

int main()
{
	::SetConsoleCtrlHandler(console_handler, TRUE);

	using namespace event_tracing;

	try
	{
		if (!is_running_elevated())
			throw std::runtime_error("You should run the program as administrator");

		auto process_provider_guid = event_provider_list().get_guid(L"Microsoft-Windows-Kernel-Process");

		event_trace_session session(L"Kaimi.io test session");
		global_session = &session;

		static constexpr const std::uint64_t keyword_process = 0x10;
		static constexpr const std::uint64_t keyword_thread = 0x20;
		static constexpr const std::uint64_t keyword_image = 0x40;
		session.enable_trace(process_provider_guid, event_trace_session::trace_level::verbose,
			keyword_process | keyword_thread | keyword_image);

		event_trace trace(session);
		trace.on_trace_event(process_provider_guid, [](auto record)
		{
			try
			{
				std::wcout << event_info(record) << std::endl;
			}
			catch (const std::exception& e)
			{
				std::cout << "Error parsing event: " << e.what() << std::endl;
			}
		});

		global_trace = &trace;
		trace.run();
	}
	catch (const event_tracing::event_trace_error& e)
	{
		std::cout << "Error: " << e.what() << "; error code: " << e.get_error_code() << std::endl;
		system("pause");
		return -1;
	}
	catch (const std::exception& e)
	{
		std::cout << "Error: " << e.what() << std::endl;
		system("pause");
		return -1;
	}

	return 0;
}
