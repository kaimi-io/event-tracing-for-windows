#include <stdexcept>

#include <Windows.h>

#include "event_tracing/elevated_check.h"

#include "common_controls.h"
#include "main_window.h"

int CALLBACK wWinMain(HINSTANCE instance, HINSTANCE /*prev_instance*/, LPWSTR /*cmd_line*/, int /*cmd_show*/)
{
	try
	{
		if (!event_tracing::is_running_elevated())
			throw std::runtime_error("You should run the program with administrative privileges");

		common_controls::init();

		return main_window(instance).get_result_code();
	}
	catch (const std::exception& e)
	{
		::MessageBoxA(nullptr, e.what(), "Error", MB_ICONERROR | MB_OK);
		return -1;
	}
}
