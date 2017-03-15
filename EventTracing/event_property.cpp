#include "event_tracing/event_property.h"

#include <codecvt>
#include <ctime>
#include <iomanip>
#include <locale>
#include <sstream>
#include <utility>

#include <Windows.h>
#include <tdh.h>

#include <boost/endian/conversion.hpp>

#include "event_tracing/event_trace_error.h"

namespace event_tracing
{
event_property::event_property(std::uint16_t in_type, std::uint16_t out_type, bool wide_pointer,
	raw_value_type&& value, std::wstring&& name) noexcept
	: in_type_(in_type)
	, out_type_(out_type)
	, wide_pointer_(wide_pointer)
	, value_(std::move(value))
	, name_(std::move(name))
{
}

namespace
{
const wchar_t* get_type_name(std::uint16_t type_id) noexcept
{
	switch (type_id)
	{
	case TDH_INTYPE_BOOLEAN:
		return L"[BOOL]   ";

	case TDH_INTYPE_UINT64:
		return L"[UINT64] ";

	case TDH_INTYPE_UINT32:
		return L"[UINT32] ";

	case TDH_INTYPE_UINT16:
		return L"[UINT16] ";

	case TDH_INTYPE_UINT8:
		return L"[UINT8]  ";

	case TDH_INTYPE_INT64:
		return L"[INT64]  ";

	case TDH_INTYPE_HEXINT64:
		return L"[HEX64]  ";

	case TDH_INTYPE_INT32:
		return L"[INT32]  ";

	case TDH_INTYPE_HEXINT32:
		return L"[HEX32]  ";

	case TDH_INTYPE_INT16:
		return L"[INT16]  ";

	case TDH_INTYPE_INT8:
		return L"[INT8]   ";

	case TDH_INTYPE_DOUBLE:
		return L"[DOUBLE] ";

	case TDH_INTYPE_FLOAT:
		return L"[FLOAT]  ";

	case TDH_INTYPE_UNICODESTRING:
		return L"[USTR]   ";

	case TDH_INTYPE_COUNTEDSTRING:
		return L"[CUSTR]  ";

	case TDH_INTYPE_REVERSEDCOUNTEDSTRING:
		return L"[RCUSTR] ";

	case TDH_INTYPE_NONNULLTERMINATEDSTRING:
		return L"[NUSTR]  ";

	case TDH_INTYPE_ANSISTRING:
		return L"[ASTR]   ";

	case TDH_INTYPE_COUNTEDANSISTRING:
		return L"[CASTR]  ";

	case TDH_INTYPE_REVERSEDCOUNTEDANSISTRING:
		return L"[RCASTR] ";

	case TDH_INTYPE_NONNULLTERMINATEDANSISTRING:
		return L"[NASTR]  ";

	case TDH_INTYPE_UNICODECHAR:
		return L"[UCHAR]  ";

	case TDH_INTYPE_ANSICHAR:
		return L"[CHAR]   ";

	case TDH_INTYPE_NULL:
		return L"[NULL]   ";

	case TDH_INTYPE_SIZET:
		return L"[SIZET]  ";

	case TDH_INTYPE_POINTER:
		return L"[PTR]    ";

	case TDH_INTYPE_SYSTEMTIME:
		return L"[STIME]  ";

	case TDH_INTYPE_FILETIME:
		return L"[FTIME]  ";

	case TDH_INTYPE_GUID:
		return L"[GUID]   ";

	default:
		break;
	}

	return nullptr;
}
} //namespace

std::wstring event_property::to_wstring() const
{
	auto type_name = get_type_name(in_type_);
	std::wstring result;
	if (type_name)
		result += type_name;
	result += name_ + L" = ";
	switch (in_type_)
	{
	case TDH_INTYPE_BOOLEAN:
		result += (event_property_converter<bool>::convert(*this) ? L"true" : L"false");
		break;

	case TDH_INTYPE_UINT64:
	case TDH_INTYPE_UINT32:
	case TDH_INTYPE_UINT16:
	case TDH_INTYPE_UINT8:
		result += std::to_wstring(event_property_converter<std::uint64_t>::convert(*this));
		break;

	case TDH_INTYPE_INT64:
	case TDH_INTYPE_HEXINT64:
	case TDH_INTYPE_INT32:
	case TDH_INTYPE_HEXINT32:
	case TDH_INTYPE_INT16:
	case TDH_INTYPE_INT8:
		result += std::to_wstring(event_property_converter<std::int64_t>::convert(*this));
		break;

	case TDH_INTYPE_DOUBLE:
		result += std::to_wstring(event_property_converter<double>::convert(*this));
		break;

	case TDH_INTYPE_FLOAT:
		result += std::to_wstring(event_property_converter<float>::convert(*this));
		break;

	case TDH_INTYPE_UNICODESTRING:
	case TDH_INTYPE_COUNTEDSTRING:
	case TDH_INTYPE_REVERSEDCOUNTEDSTRING:
	case TDH_INTYPE_NONNULLTERMINATEDSTRING:
		result += event_property_converter<std::wstring>::convert(*this);
		break;

	case TDH_INTYPE_ANSISTRING:
	case TDH_INTYPE_COUNTEDANSISTRING:
	case TDH_INTYPE_REVERSEDCOUNTEDANSISTRING:
	case TDH_INTYPE_NONNULLTERMINATEDANSISTRING:
		{
			auto prop_string = event_property_converter<std::string>::convert(*this);
			result += std::wstring(prop_string.cbegin(), prop_string.cend());
		}
		break;

	case TDH_INTYPE_UNICODECHAR:
		result.push_back(event_property_converter<wchar_t>::convert(*this));
		break;

	case TDH_INTYPE_ANSICHAR:
		result.push_back(event_property_converter<char>::convert(*this));
		break;

	case TDH_INTYPE_NULL:
		break;

	case TDH_INTYPE_SIZET:
		result += std::to_wstring(event_property_converter<event_type_size_t>::convert(*this));
		break;

	case TDH_INTYPE_POINTER:
		result += std::to_wstring(event_property_converter<event_type_pointer>::convert(*this));
		break;

	case TDH_INTYPE_SYSTEMTIME:
	case TDH_INTYPE_FILETIME:
		{
			auto value = std::chrono::system_clock::to_time_t(
				event_property_converter<std::chrono::system_clock::time_point>::convert(*this));
			std::tm tm_value;
			gmtime_s(&tm_value, &value);
			std::wstringstream ss;
			ss << std::put_time(&tm_value, L"%F %T UTC");
			result += ss.str();
		}
		break;

	case TDH_INTYPE_GUID:
		result += event_property_converter<ms_guid>::convert(*this).to_wstring();
		break;

	default:
		result += L"[unsupported type " + std::to_wstring(in_type_) + L"]";
		break;
	}

	return result;
}

namespace
{
template<typename Type, std::uint16_t TdhType>
struct EventTypeInfo
{
	using type = Type;
	static constexpr const std::uint16_t tdh_type = TdhType;
};

template<typename ResultType>
[[noreturn]] ResultType convert_property_data(const event_property&)
{
	throw event_trace_error("Incorrect property type");
}

template<typename ResultType, typename EventType, typename... Args>
ResultType convert_property_data(const event_property& prop)
{
	if (prop.get_in_type() == EventType::tdh_type)
	{
		using SourceType = EventType::type;
		if (prop.get_raw_value().size() != sizeof(SourceType))
			throw event_trace_error("Invalid property value size");

		return static_cast<ResultType>(*reinterpret_cast<const SourceType*>(prop.get_raw_value().data()));
	}

	return convert_property_data<ResultType, Args...>(prop);
}
} //namespace

bool event_property_converter<bool>::convert(const event_property& prop)
{
	return !!convert_property_data<std::uint32_t,
		EventTypeInfo<std::uint32_t, TDH_INTYPE_BOOLEAN>>(prop);
}

std::uint64_t event_property_converter<std::uint64_t>::convert(const event_property& prop)
{
	return convert_property_data<std::uint64_t,
		EventTypeInfo<std::uint64_t, TDH_INTYPE_UINT64>,
		EventTypeInfo<std::uint32_t, TDH_INTYPE_UINT32>,
		EventTypeInfo<std::uint16_t, TDH_INTYPE_UINT16>,
		EventTypeInfo<std::uint8_t, TDH_INTYPE_UINT8>>(prop);
}

std::uint32_t event_property_converter<std::uint32_t>::convert(const event_property& prop)
{
	return convert_property_data<std::uint32_t,
		EventTypeInfo<std::uint32_t, TDH_INTYPE_UINT32>,
		EventTypeInfo<std::uint16_t, TDH_INTYPE_UINT16>,
		EventTypeInfo<std::uint8_t, TDH_INTYPE_UINT8>>(prop);
}

std::uint16_t event_property_converter<std::uint16_t>::convert(const event_property& prop)
{
	return convert_property_data<std::uint16_t,
		EventTypeInfo<std::uint16_t, TDH_INTYPE_UINT16>,
		EventTypeInfo<std::uint8_t, TDH_INTYPE_UINT8>>(prop);
}

std::uint8_t event_property_converter<std::uint8_t>::convert(const event_property& prop)
{
	return convert_property_data<std::uint8_t,
		EventTypeInfo<std::uint8_t, TDH_INTYPE_UINT8>>(prop);
}

std::int64_t event_property_converter<std::int64_t>::convert(const event_property& prop)
{
	return convert_property_data<std::int64_t,
		EventTypeInfo<std::int64_t, TDH_INTYPE_INT64>,
		EventTypeInfo<std::int64_t, TDH_INTYPE_HEXINT64>,
		EventTypeInfo<std::int32_t, TDH_INTYPE_INT32>,
		EventTypeInfo<std::int32_t, TDH_INTYPE_HEXINT32>,
		EventTypeInfo<std::int16_t, TDH_INTYPE_INT16>,
		EventTypeInfo<std::int8_t, TDH_INTYPE_INT8>>(prop);
}

std::int32_t event_property_converter<std::int32_t>::convert(const event_property& prop)
{
	return convert_property_data<std::int32_t,
		EventTypeInfo<std::int32_t, TDH_INTYPE_INT32>,
		EventTypeInfo<std::int32_t, TDH_INTYPE_HEXINT32>,
		EventTypeInfo<std::int16_t, TDH_INTYPE_INT16>,
		EventTypeInfo<std::int8_t, TDH_INTYPE_INT8>>(prop);
}

std::int16_t event_property_converter<std::int16_t>::convert(const event_property& prop)
{
	return convert_property_data<std::int16_t,
		EventTypeInfo<std::int16_t, TDH_INTYPE_INT16>,
		EventTypeInfo<std::int8_t, TDH_INTYPE_INT8>>(prop);
}

std::int8_t event_property_converter<std::int8_t>::convert(const event_property& prop)
{
	return convert_property_data<std::int8_t,
		EventTypeInfo<std::int8_t, TDH_INTYPE_INT8>>(prop);
}

float event_property_converter<float>::convert(const event_property& prop)
{
	return convert_property_data<float,
		EventTypeInfo<FLOAT, TDH_INTYPE_FLOAT>>(prop);
}

double event_property_converter<double>::convert(const event_property& prop)
{
	return convert_property_data<double,
		EventTypeInfo<DOUBLE, TDH_INTYPE_DOUBLE>,
		EventTypeInfo<FLOAT, TDH_INTYPE_FLOAT>>(prop);
}

wchar_t event_property_converter<wchar_t>::convert(const event_property& prop)
{
	return convert_property_data<wchar_t,
		EventTypeInfo<WCHAR, TDH_INTYPE_UNICODECHAR>>(prop);
}

char event_property_converter<char>::convert(const event_property& prop)
{
	return convert_property_data<char,
		EventTypeInfo<CHAR, TDH_INTYPE_ANSICHAR>>(prop);
}

namespace
{
template<typename StringType, std::uint16_t UnicodeString,
	std::uint16_t CountedString, std::uint16_t ReversedCountedString,
	std::uint16_t NonNullTerminatedString>
StringType convert_to_string(const event_property& prop)
{
	auto string_start = reinterpret_cast<typename StringType::const_pointer>(prop.get_raw_value().data());
	switch (prop.get_in_type())
	{
	case UnicodeString:
		{
			if (prop.get_raw_value().empty())
				return StringType();

			if (prop.get_raw_value().size() % sizeof(typename StringType::value_type))
				throw event_trace_error("Invalid property value size");

			auto end = string_start + prop.get_raw_value().size() / sizeof(typename StringType::value_type);
			if (std::find(string_start, end, static_cast<typename StringType::value_type>(0)) == end)
				return StringType(string_start, end);

			return StringType(string_start);
		}
		break;

	case CountedString:
	case ReversedCountedString:
		{
			if (!prop.get_raw_value().size())
				throw event_trace_error("Invalid property value size");

			if ((prop.get_raw_value().size() - 1) % sizeof(typename StringType::value_type))
				throw event_trace_error("Invalid property value size");

			auto size = static_cast<std::uint16_t>(*string_start);
			if (prop.get_in_type() == ReversedCountedString)
				boost::endian::big_to_native_inplace(size);

			if (size * sizeof(typename StringType::value_type) >= prop.get_raw_value().size())
				throw event_trace_error("Invalid property value size");

			return StringType(string_start + 1, size);
		}
		break;

	case NonNullTerminatedString:
		if (prop.get_raw_value().size() % sizeof(typename StringType::value_type))
			throw event_trace_error("Invalid property value size");

		return StringType(string_start, prop.get_raw_value().size()
			/ sizeof(typename StringType::value_type));
		break;

	default:
		break;
	}

	throw event_trace_error("Incorrect property type");
}
} //namespace

std::wstring event_property_converter<std::wstring>::convert(const event_property& prop)
{
	return convert_to_string<std::wstring, TDH_INTYPE_UNICODESTRING,
		TDH_INTYPE_COUNTEDSTRING, TDH_INTYPE_REVERSEDCOUNTEDSTRING,
		TDH_INTYPE_NONNULLTERMINATEDSTRING>(prop);
}

std::string event_property_converter<std::string>::convert(const event_property& prop)
{
	return convert_to_string<std::string, TDH_INTYPE_ANSISTRING,
		TDH_INTYPE_COUNTEDANSISTRING, TDH_INTYPE_REVERSEDCOUNTEDANSISTRING,
		TDH_INTYPE_NONNULLTERMINATEDANSISTRING>(prop);
}

std::uint64_t event_property_converter<event_type_size_t>::convert(const event_property& prop)
{
	if (prop.is_wide_pointer())
	{
		return convert_property_data<std::uint64_t,
			EventTypeInfo<std::uint64_t, TDH_INTYPE_SIZET>>(prop);
	}

	return convert_property_data<std::uint32_t,
		EventTypeInfo<std::uint32_t, TDH_INTYPE_SIZET>>(prop);
}

std::uint64_t event_property_converter<event_type_pointer>::convert(const event_property& prop)
{
	if (prop.is_wide_pointer())
	{
		return convert_property_data<std::uint64_t,
			EventTypeInfo<std::uint64_t, TDH_INTYPE_POINTER>>(prop);
	}

	return convert_property_data<std::uint32_t,
		EventTypeInfo<std::uint32_t, TDH_INTYPE_POINTER>>(prop);
}

std::chrono::system_clock::time_point event_property_converter<
	std::chrono::system_clock::time_point>::convert(const event_property& prop)
{
	SYSTEMTIME value;
	switch (prop.get_in_type())
	{
	case TDH_INTYPE_SYSTEMTIME:
		if(prop.get_raw_value().size() != sizeof(SYSTEMTIME))
			throw event_trace_error("Invalid property value size");

		value = *reinterpret_cast<const SYSTEMTIME*>(prop.get_raw_value().data());
		break;

	case TDH_INTYPE_FILETIME:
		if (prop.get_raw_value().size() != sizeof(FILETIME))
			throw event_trace_error("Invalid property value size");

		if(!::FileTimeToSystemTime(reinterpret_cast<const FILETIME*>(prop.get_raw_value().data()), &value))
			throw event_trace_error("Invalid filetime property value", ::GetLastError());
		break;

	default:
		throw event_trace_error("Incorrect property type");
		break;
	}

	std::tm tm_value{};
	tm_value.tm_hour = value.wHour;
	tm_value.tm_mday = value.wDay;
	tm_value.tm_min = value.wMinute;
	tm_value.tm_mon = value.wMonth - 1;
	tm_value.tm_sec = value.wSecond;
	tm_value.tm_year = value.wYear - 1900;
	return std::chrono::system_clock::from_time_t(_mkgmtime64(&tm_value));
}

ms_guid event_property_converter<ms_guid>::convert(const event_property& prop)
{
	return convert_property_data<ms_guid,
		EventTypeInfo<GUID, TDH_INTYPE_GUID>>(prop);
}
} //namespace event_tracing
