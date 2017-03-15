#pragma once

#include <ostream>
#include <vector>

#include <Windows.h>
#include <Evntcons.h>
#include <Evntrace.h>
#include <tdh.h>

#include "event_tracing/event_property.h"
#include "event_tracing/event_trace_error.h"

namespace event_tracing
{
class event_info_structure
{
public:
	event_info_structure(LPCWSTR name, USHORT struct_start_index,
		USHORT member_count, ULONG top_level_index) noexcept;

	const wchar_t* get_name() const noexcept
	{
		return name_;
	}

	ULONG get_top_level_index() const noexcept
	{
		return top_level_index_;
	}

	USHORT get_struct_start_index() const noexcept
	{
		return struct_start_index_;
	}

	USHORT get_member_count() const noexcept
	{
		return member_count_;
	}

private:
	LPCWSTR name_;
	USHORT struct_start_index_;
	USHORT member_count_;
	ULONG top_level_index_;
};

class event_info
{
public:
	explicit event_info(PEVENT_RECORD record);

	operator PTRACE_EVENT_INFO() noexcept
	{
		return reinterpret_cast<PTRACE_EVENT_INFO>(data_.data());
	}

	operator const TRACE_EVENT_INFO*() const noexcept
	{
		return reinterpret_cast<const TRACE_EVENT_INFO*>(data_.data());
	}

	operator PEVENT_RECORD() noexcept
	{
		return record_;
	}

	operator const EVENT_RECORD*() const noexcept
	{
		return record_;
	}

	bool has_string_only() const noexcept;
	event_property get_event_string() const;

	const wchar_t* get_property_name(ULONG top_level_index) const;
	const wchar_t* get_event_message() const noexcept;

	const EVENT_DESCRIPTOR& get_event_descriptor() const noexcept;
	USHORT get_event_id() const noexcept;

	ULONG get_top_level_property_count() const noexcept;

	bool is_property_struct(ULONG top_level_index) const;
	event_info_structure get_structure(ULONG top_level_index) const;

	bool find_property_index(const std::wstring& name, ULONG& top_level_index) const;
	ULONG get_array_property_size(ULONG top_level_index) const;
	event_property get_plain_property_value(ULONG top_level_index) const;
	event_property get_array_property_value(ULONG top_level_index, ULONG element_index) const;

	bool find_property_index(const event_info_structure& structure,
		const std::wstring& name, ULONG& index) const;
	ULONG get_array_property_size(const event_info_structure& structure,
		ULONG struct_member_index) const;
	event_property get_plain_property_value(const event_info_structure& structure,
		ULONG struct_member_index) const;
	event_property get_plain_property_value(const event_info_structure& structure,
		ULONG struct_index, ULONG struct_member_index) const;
	event_property get_array_property_value(const event_info_structure& structure,
		ULONG struct_index, ULONG struct_member_index, ULONG struct_element_index) const;

	template<typename PropertyType>
	bool get_plain_property_value(const std::wstring& name, PropertyType& value) const
	{
		ULONG top_level_index = 0u;
		if (!find_property_index(name, top_level_index))
			return false;

		value = event_property_converter<PropertyType>::convert(get_plain_property_value(top_level_index));
		return true;
	}

	template<typename PropertyType>
	auto get_plain_property_value(const std::wstring& name) const
	{
		ULONG top_level_index = 0u;
		if (!find_property_index(name, top_level_index))
			throw event_trace_error("Property was not found in event");

		return event_property_converter<PropertyType>::convert(get_plain_property_value(top_level_index));
	}

	template<typename PropertyType>
	auto get_plain_property_value(ULONG top_level_index) const
	{
		return event_property_converter<PropertyType>::convert(get_plain_property_value(top_level_index));
	}

	template<typename PropertyType>
	auto get_array_property_value(ULONG top_level_index, ULONG element_index) const
	{
		return event_property_converter<PropertyType>::convert(
			get_array_property_value(top_level_index, element_index));
	}

	template<typename PropertyType>
	auto get_plain_property_value(const event_info_structure& structure,
		ULONG struct_member_index) const
	{
		return event_property_converter<PropertyType>::convert(
			get_plain_property_value(structure, struct_member_index));
	}

	template<typename PropertyType>
	auto get_plain_property_value(const event_info_structure& structure,
		ULONG struct_index, ULONG struct_member_index) const
	{
		return event_property_converter<PropertyType>::convert(
			get_plain_property_value(structure, struct_index, struct_member_index));
	}

	template<typename PropertyType>
	auto get_array_property_value(const event_info_structure& structure,
		ULONG struct_index, ULONG struct_member_index, ULONG struct_element_index) const
	{
		return event_property_converter<PropertyType>::convert(
			get_array_property_value(structure, struct_index, struct_member_index, struct_element_index));
	}

private:
	event_property get_array_property_value(ULONG top_level_index,
		ULONG element_index, bool is_array) const;
	event_property get_array_property_value(const event_info_structure& structure,
		ULONG struct_index, ULONG struct_member_index, ULONG struct_element_index,
		bool is_array, bool is_struct_member_array) const;
	event_property get_array_property_value(ULONG top_level_index,
		ULONG element_index, ULONG struct_member_index, bool is_array, bool is_struct_member_array,
		PROPERTY_DATA_DESCRIPTOR* data_descriptors, ULONG descriptor_count) const;

	void check_if_has_properties() const;

private:
	std::vector<uint8_t> data_;
	PEVENT_RECORD record_;
};

std::wostream& operator<<(std::wostream& stream, const event_info& info);
} //namespace event_tracing
