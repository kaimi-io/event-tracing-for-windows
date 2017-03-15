#include "event_tracing/elevated_check.h"

#include <Windows.h>

namespace event_tracing
{
bool is_running_elevated() noexcept
{
	bool result = false;
	HANDLE token = nullptr;
	if (::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token))
	{
		TOKEN_ELEVATION elevation;
		DWORD size = sizeof(TOKEN_ELEVATION);
		if (::GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size))
			result = !!elevation.TokenIsElevated;
	}

	if (token)
		::CloseHandle(token);

	return result;
}
} //namespace event_tracing
