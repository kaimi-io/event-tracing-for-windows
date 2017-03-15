#include "event_tracing/guid_helpers.h"

#include <cstring>
#include <memory>

namespace event_tracing
{
ms_guid::ms_guid(const std::wstring& str)
	: ms_guid(str.c_str())
{
}

ms_guid::ms_guid(const wchar_t* str)
	: guid_{}
{
	if (FAILED(::CLSIDFromString(str, &guid_)))
		throw std::runtime_error("Invalid GUID string");
}

ms_guid::ms_guid(const GUID& guid) noexcept
	: guid_(guid)
{
}

std::wstring ms_guid::to_wstring() const
{
	OLECHAR* str = nullptr;
	if(FAILED(::StringFromCLSID(guid_, &str)))
		throw std::runtime_error("Invalid GUID string");

	std::unique_ptr<OLECHAR, void(*)(OLECHAR*)> mem_ptr(str, [](OLECHAR* ptr)
	{
		::CoTaskMemFree(ptr);
	});

	return std::wstring(str);
}

bool operator<(const ms_guid& left, const ms_guid& right) noexcept
{
	return std::memcmp(left.native_ptr(), right.native_ptr(), ms_guid::size) < 0;
}

bool operator>(const ms_guid& left, const ms_guid& right) noexcept
{
	return right < left;
}

bool operator==(const ms_guid& left, const ms_guid& right) noexcept
{
	return std::memcmp(left.native_ptr(), right.native_ptr(), ms_guid::size) == 0;
}

bool operator!=(const ms_guid& left, const ms_guid& right) noexcept
{
	return !(left == right);
}
} //namespace event_tracing
