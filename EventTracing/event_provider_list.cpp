#include "event_tracing/event_provider_list.h"

#include <cstdint>
#include <vector>
#include <utility>

#include <Windows.h>
#include <tdh.h>

#include "event_tracing/event_trace_error.h"

namespace event_tracing
{
event_provider_list::event_provider_list()
{
	ULONG buf_size = 0u;
	std::vector<std::uint8_t> buf;
	TDHSTATUS status = ERROR_SUCCESS;

	do
	{
		buf.resize(buf_size);
		status = ::TdhEnumerateProviders(
			reinterpret_cast<PPROVIDER_ENUMERATION_INFO>(buf.data()), &buf_size);
	}
	while (status == ERROR_INSUFFICIENT_BUFFER && buf.size() != buf_size);
	if (ERROR_SUCCESS != status)
		throw event_trace_error("Unable to enumerate providers", status);

	auto provider_info = reinterpret_cast<const PROVIDER_ENUMERATION_INFO*>(buf.data());
	for (ULONG i = 0; i != provider_info->NumberOfProviders; ++i)
	{
		ms_guid guid(provider_info->TraceProviderInfoArray[i].ProviderGuid);
		std::wstring name(reinterpret_cast<const wchar_t*>(
			buf.data() + provider_info->TraceProviderInfoArray[i].ProviderNameOffset));
		guid_to_name_.emplace(guid, name);
		name_to_guid_.emplace(std::move(name), std::move(guid));
	}
}

const ms_guid& event_provider_list::get_guid(const std::wstring& name) const
{
	auto it = name_to_guid_.find(name);
	if (it == name_to_guid_.cend())
		throw event_trace_error("No provider found with specified name");

	return (*it).second;
}

const std::wstring& event_provider_list::get_name(const ms_guid& guid) const
{
	auto it = guid_to_name_.find(guid);
	if (it == guid_to_name_.cend())
		throw event_trace_error("No provider found with specified GUID");

	return (*it).second;
}

bool event_provider_list::has_name(const std::wstring& name) const
{
	return name_to_guid_.find(name) != name_to_guid_.cend();
}

bool event_provider_list::has_guid(const ms_guid& guid) const
{
	return guid_to_name_.find(guid) != guid_to_name_.cend();
}
} //namespace event_tracing
