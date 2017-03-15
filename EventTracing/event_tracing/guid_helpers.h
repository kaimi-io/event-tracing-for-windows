#pragma once

#include <cstddef>
#include <string>

#include <Windows.h>

namespace event_tracing
{
class ms_guid
{
public:
	static constexpr const std::size_t size = sizeof(GUID);

public:
	ms_guid(const std::wstring& str);
	ms_guid(const wchar_t* str);
	ms_guid(const GUID& guid) noexcept;

	const GUID& native() const noexcept
	{
		return guid_;
	}

	const GUID* native_ptr() const noexcept
	{
		return &guid_;
	}

	std::wstring to_wstring() const;

private:
	GUID guid_;
};

bool operator<(const ms_guid& left, const ms_guid& right) noexcept;
bool operator>(const ms_guid& left, const ms_guid& right) noexcept;
bool operator==(const ms_guid& left, const ms_guid& right) noexcept;
bool operator!=(const ms_guid& left, const ms_guid& right) noexcept;
} //namespace event_tracing
