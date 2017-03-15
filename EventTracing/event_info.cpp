#include "event_tracing/event_info.h"

#include <cstdint>
#include <utility>

#include "event_tracing/event_trace_error.h"

namespace event_tracing
{
event_info_structure::event_info_structure(LPCWSTR name,
	USHORT struct_start_index, USHORT member_count, ULONG top_level_index) noexcept
	: name_(name)
	, struct_start_index_(struct_start_index)
	, member_count_(member_count)
	, top_level_index_(top_level_index)
{
}

event_info::event_info(PEVENT_RECORD record)
	: data_(sizeof(TRACE_EVENT_INFO))
	, record_(record)
{
	auto buffer_size = static_cast<DWORD>(data_.size());
	auto status = ::TdhGetEventInformation(record, 0, nullptr, *this, &buffer_size);
	if (ERROR_INSUFFICIENT_BUFFER == status)
	{
		data_.resize(buffer_size);
		status = ::TdhGetEventInformation(record, 0, nullptr, *this, &buffer_size);
	}

	if (status != ERROR_SUCCESS)
		throw event_trace_error("Unable to get event information", status);
}

const EVENT_DESCRIPTOR& event_info::get_event_descriptor() const noexcept
{
	return static_cast<const TRACE_EVENT_INFO*>(*this)->EventDescriptor;
}

USHORT event_info::get_event_id() const noexcept
{
	return get_event_descriptor().Id;
}

ULONG event_info::get_top_level_property_count() const noexcept
{
	return static_cast<const TRACE_EVENT_INFO*>(*this)->TopLevelPropertyCount;
}

bool event_info::find_property_index(const std::wstring& name, ULONG& top_level_index) const
{
	check_if_has_properties();
	auto info = static_cast<const TRACE_EVENT_INFO*>(*this);
	for (ULONG i = 0; i != info->TopLevelPropertyCount; ++i)
	{
		if (name == get_property_name(i))
		{
			top_level_index = i;
			return true;
		}
	}

	return false;
}

bool event_info::find_property_index(const event_info_structure& structure,
	const std::wstring& name, ULONG& index) const
{
	check_if_has_properties();
	ULONG end = structure.get_struct_start_index() + structure.get_member_count();
	for (ULONG i = structure.get_struct_start_index(); i != end; ++i)
	{
		if (name == get_property_name(i))
		{
			index = i;
			return true;
		}
	}

	return false;
}

ULONG event_info::get_array_property_size(ULONG top_level_index) const
{
	check_if_has_properties();
	auto info = static_cast<const TRACE_EVENT_INFO*>(*this);
	if ((info->EventPropertyInfoArray[top_level_index].Flags & PropertyParamCount) == PropertyParamCount)
	{
		return get_plain_property_value<std::uint32_t>(
			info->EventPropertyInfoArray[top_level_index].countPropertyIndex);
	}

	return info->EventPropertyInfoArray[top_level_index].count;
}

ULONG event_info::get_array_property_size(const event_info_structure& structure,
	ULONG struct_member_index) const
{
	return get_array_property_size(structure.get_struct_start_index() + struct_member_index);
}

event_property event_info::get_plain_property_value(ULONG top_level_index) const
{
	return get_array_property_value(top_level_index, 0u, false);
}

event_property event_info::get_plain_property_value(const event_info_structure& structure,
	ULONG struct_member_index) const
{
	return get_array_property_value(structure, 0u, struct_member_index,
		0u, false, false);
}

event_property event_info::get_plain_property_value(const event_info_structure& structure,
	ULONG struct_index, ULONG struct_member_index) const
{
	return get_array_property_value(structure, struct_index, struct_member_index,
		0u, true, false);
}

event_property event_info::get_array_property_value(ULONG top_level_index, ULONG element_index) const
{
	return get_array_property_value(top_level_index, element_index, true);
}

event_property event_info::get_array_property_value(const event_info_structure& structure,
	ULONG struct_index, ULONG struct_member_index, ULONG struct_element_index) const
{
	return get_array_property_value(structure, struct_index, struct_member_index,
		struct_element_index, true, true);
}

bool event_info::is_property_struct(ULONG top_level_index) const
{
	check_if_has_properties();
	auto info = static_cast<const TRACE_EVENT_INFO*>(*this);
	return (info->EventPropertyInfoArray[top_level_index].Flags & PropertyStruct) == PropertyStruct;
}

event_info_structure event_info::get_structure(ULONG top_level_index) const
{
	if(!is_property_struct(top_level_index))
		throw event_trace_error("Expected structure, got plain value");

	auto info = static_cast<const TRACE_EVENT_INFO*>(*this);
	auto struct_start_index = info->EventPropertyInfoArray[top_level_index].structType.StructStartIndex;
	auto member_count = info->EventPropertyInfoArray[top_level_index].structType.NumOfStructMembers;
	return event_info_structure(get_property_name(top_level_index),
		struct_start_index, member_count, top_level_index);
}

event_property event_info::get_array_property_value(const event_info_structure& structure,
	ULONG struct_index, ULONG struct_member_index, ULONG struct_element_index,
	bool is_array, bool is_struct_member_array) const
{
	PROPERTY_DATA_DESCRIPTOR data_descriptors[2]{};
	data_descriptors[0].PropertyName = reinterpret_cast<ULONGLONG>(structure.get_name());
	data_descriptors[0].ArrayIndex = struct_index;
	data_descriptors[1].PropertyName = reinterpret_cast<ULONGLONG>(
		get_property_name(structure.get_struct_start_index() + struct_member_index));
	data_descriptors[1].ArrayIndex = struct_element_index;
	return get_array_property_value(structure.get_top_level_index(), struct_index,
		structure.get_struct_start_index() + struct_member_index, is_array,
		is_struct_member_array, data_descriptors,
		sizeof(data_descriptors) / sizeof(data_descriptors[0]));
}

event_property event_info::get_array_property_value(ULONG top_level_index,
	ULONG element_index, ULONG struct_member_index, bool is_array, bool is_struct_member_array,
	PROPERTY_DATA_DESCRIPTOR* data_descriptors, ULONG descriptor_count) const
{
	check_if_has_properties();
	auto array_size = get_array_property_size(top_level_index);
	if (!is_array && array_size != 1)
		throw event_trace_error("Expected single-value property, got array");

	if (element_index >= array_size)
		throw event_trace_error("Array index out of bounds");

	bool is_struct = is_property_struct(top_level_index);
	if (is_struct && descriptor_count == 1)
		throw event_trace_error("Expected single-value property, got struct");
	else if (!is_struct && descriptor_count == 2)
		throw event_trace_error("Expected struct property, got single-value property");

	auto property_index = top_level_index;
	if (descriptor_count == 2)
	{
		property_index = struct_member_index;
		auto struct_member_array_size = get_array_property_size(property_index);
		if (!is_struct_member_array && struct_member_array_size != 1)
			throw event_trace_error("Expected single-value struct member, got array");
	}

	ULONG property_size = 0;
	auto status = ::TdhGetPropertySize(record_, 0, nullptr, descriptor_count, data_descriptors, &property_size);
	if (ERROR_SUCCESS != status)
		throw event_trace_error("Unable to get property size", status);

	event_property::raw_value_type raw_value;
	raw_value.resize(property_size);
	status = ::TdhGetProperty(record_, 0, nullptr, descriptor_count, data_descriptors, property_size, raw_value.data());
	if (ERROR_SUCCESS != status)
		throw event_trace_error("Failed to get property value", status);

	auto info = static_cast<const TRACE_EVENT_INFO*>(*this);
	return event_property(info->EventPropertyInfoArray[property_index].nonStructType.InType,
		info->EventPropertyInfoArray[property_index].nonStructType.OutType,
		!(record_->EventHeader.Flags & EVENT_HEADER_FLAG_32_BIT_HEADER),
		std::move(raw_value), get_property_name(property_index));
}

event_property event_info::get_array_property_value(ULONG top_level_index,
	ULONG element_index, bool is_array) const
{
	PROPERTY_DATA_DESCRIPTOR data_descriptor;
	data_descriptor.PropertyName = reinterpret_cast<ULONGLONG>(get_property_name(top_level_index));
	data_descriptor.ArrayIndex = element_index;
	return get_array_property_value(top_level_index, element_index, 0u, is_array, false, &data_descriptor, 1);
}

bool event_info::has_string_only() const noexcept
{
	return (record_->EventHeader.Flags & EVENT_HEADER_FLAG_STRING_ONLY) == EVENT_HEADER_FLAG_STRING_ONLY;
}

void event_info::check_if_has_properties() const
{
	if (has_string_only())
		throw event_trace_error("Event has not properties, only unicode string. Use get_event_string() to get it");
}

event_property event_info::get_event_string() const
{
	event_property::raw_value_type raw_value(
		static_cast<event_property::raw_value_type::const_pointer>(record_->UserData),
		static_cast<event_property::raw_value_type::const_pointer>(record_->UserData) + record_->UserDataLength);
	return event_property(TDH_INTYPE_UNICODESTRING, TDH_INTYPE_UNICODESTRING, false,
		std::move(raw_value), L"<Unnamed String Only>");
}

const wchar_t* event_info::get_property_name(ULONG top_level_index) const
{
	check_if_has_properties();
	auto info = static_cast<const TRACE_EVENT_INFO*>(*this);
	return reinterpret_cast<const WCHAR*>(reinterpret_cast<const BYTE*>(info)
		+ info->EventPropertyInfoArray[top_level_index].NameOffset);
}

const wchar_t* event_info::get_event_message() const noexcept
{
	auto info = static_cast<const TRACE_EVENT_INFO*>(*this);
	if (!info->EventMessageOffset)
		return nullptr;

	return reinterpret_cast<const WCHAR*>(reinterpret_cast<const BYTE*>(info)
		+ info->EventMessageOffset);
}

std::wostream& operator<<(std::wostream& stream, const event_info& info)
{
	stream << "Event ID = " << info.get_event_id();

	{
		auto msg = info.get_event_message();
		if (msg)
			stream << L", Message = \"" << msg << L"\"";
	}

	stream << std::endl;

	auto prop_count = info.get_top_level_property_count();
	for (ULONG top_level_index = 0; top_level_index != prop_count; ++top_level_index)
	{
		auto top_level_element_count = info.get_array_property_size(top_level_index);
		bool is_array = top_level_element_count > 1;
		if (is_array)
			stream << L"Array[Size=" << top_level_element_count << L"]" << std::endl;

		if (info.is_property_struct(top_level_index))
		{
			auto structure = info.get_structure(top_level_index);
			auto struct_name = structure.get_name();
			if(struct_name)
				stream << L"Struct \"" << struct_name << L"\"" << std::endl;
			else
				stream << L"Struct" << std::endl;

			for (ULONG element_index = 0; element_index != top_level_element_count; ++element_index)
			{
				if (is_array)
				{
					stream << L"  [#" << element_index << L"]" << std::endl;
					stream << L"  {" << std::endl;
				}
				else
				{
					stream << L"{" << std::endl;
				}

				for (USHORT struct_member_index = 0;
					struct_member_index != structure.get_member_count(); ++struct_member_index)
				{
					auto struct_member_array_size = info.get_array_property_size(structure, struct_member_index);
					bool is_struct_member_array = struct_member_array_size > 1;
					if (is_struct_member_array)
					{
						if (is_array)
							stream << L"  ";

						stream << L"Array[Size=" << struct_member_array_size << L"]" << std::endl;
					}

					for (ULONG struct_element_index = 0;
						struct_element_index != struct_member_array_size; ++struct_element_index)
					{
						if (is_array)
							stream << L"  ";
						if (is_struct_member_array)
							stream << L"  ";

						stream << L"  " << info.get_array_property_value(structure,
							element_index, struct_member_index, struct_element_index) << std::endl;
					}
				}

				stream << (is_array ? L"  }" : L"}") << std::endl;
			}
		}
		else
		{
			for (ULONG element_index = 0; element_index != top_level_element_count; ++element_index)
			{
				if (is_array)
					stream << L"  [#" << element_index << L"] ";

				stream << info.get_array_property_value(top_level_index, element_index) << std::endl;
			}
		}
	}


	return stream;
}
} //namespace event_tracing
