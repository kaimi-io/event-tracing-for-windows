#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>

namespace event_tracing
{
class event_trace_error : public std::runtime_error
{
public:
	event_trace_error(const std::string& text, std::uint32_t error_code);
	explicit event_trace_error(const std::string& text);

	std::uint32_t get_error_code() const noexcept
	{
		return error_code_;
	}

private:
	std::uint32_t error_code_;
};
} //namespace event_tracing
