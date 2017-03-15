#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "event_tracing/guid_helpers.h"

namespace event_tracing
{
class event_property
{
public:
	using raw_value_type = std::vector<std::uint8_t>;

	event_property(std::uint16_t in_type, std::uint16_t out_type, bool wide_pointer,
		raw_value_type&& value, std::wstring&& name) noexcept;

	std::uint16_t get_in_type() const noexcept
	{
		return in_type_;
	}

	std::uint16_t get_out_type() const noexcept
	{
		return out_type_;
	}

	const std::wstring& get_name() const noexcept
	{
		return name_;
	}

	const raw_value_type& get_raw_value() const noexcept
	{
		return value_;
	}

	bool is_wide_pointer() const noexcept
	{
		return wide_pointer_;
	}

	std::wstring to_wstring() const;

private:
	std::uint16_t in_type_;
	std::uint16_t out_type_;
	bool wide_pointer_;
	raw_value_type value_;
	std::wstring name_;
};

template<typename Stream>
Stream& operator<<(Stream& stream, const event_property& prop)
{
	stream << prop.to_wstring();
	return stream;
}

template<typename T>
class event_property_converter
{
};

template<>
class event_property_converter<bool>
{
public:
	static bool convert(const event_property& prop);
};

template<>
class event_property_converter<std::uint64_t>
{
public:
	static std::uint64_t convert(const event_property& prop);
};

template<>
class event_property_converter<std::uint32_t>
{
public:
	static std::uint32_t convert(const event_property& prop);
};

template<>
class event_property_converter<std::uint16_t>
{
public:
	static std::uint16_t convert(const event_property& prop);
};

template<>
class event_property_converter<std::uint8_t>
{
public:
	static std::uint8_t convert(const event_property& prop);
};

template<>
class event_property_converter<std::int64_t>
{
public:
	static std::int64_t convert(const event_property& prop);
};

template<>
class event_property_converter<std::int32_t>
{
public:
	static std::int32_t convert(const event_property& prop);
};

template<>
class event_property_converter<std::int16_t>
{
public:
	static std::int16_t convert(const event_property& prop);
};

template<>
class event_property_converter<std::int8_t>
{
public:
	static std::int8_t convert(const event_property& prop);
};

template<>
class event_property_converter<float>
{
public:
	static float convert(const event_property& prop);
};

template<>
class event_property_converter<double>
{
public:
	static double convert(const event_property& prop);
};

struct event_type_size_t {};
struct event_type_pointer {};

template<>
class event_property_converter<event_type_size_t>
{
public:
	static std::uint64_t convert(const event_property& prop);
};

template<>
class event_property_converter<event_type_pointer>
{
public:
	static std::uint64_t convert(const event_property& prop);
};

template<>
class event_property_converter<wchar_t>
{
public:
	static wchar_t convert(const event_property& prop);
};

template<>
class event_property_converter<char>
{
public:
	static char convert(const event_property& prop);
};

template<>
class event_property_converter<std::wstring>
{
public:
	static std::wstring convert(const event_property& prop);
};

template<>
class event_property_converter<std::string>
{
public:
	static std::string convert(const event_property& prop);
};

template<>
class event_property_converter<std::chrono::system_clock::time_point>
{
public:
	static std::chrono::system_clock::time_point convert(const event_property& prop);
};

template<>
class event_property_converter<ms_guid>
{
public:
	static ms_guid convert(const event_property& prop);
};
} //namespace event_tracing
