#pragma once

#include <map>
#include <string>

#include "event_tracing/guid_helpers.h"

namespace event_tracing
{
class event_provider_list
{
public:
	event_provider_list();

	const ms_guid& get_guid(const std::wstring& name) const;
	const std::wstring& get_name(const ms_guid& guid) const;
	bool has_name(const std::wstring& name) const;
	bool has_guid(const ms_guid& guid) const;

private:
	std::map<ms_guid, std::wstring> guid_to_name_;
	std::map<std::wstring, ms_guid> name_to_guid_;
};
} //namespace event_tracing
