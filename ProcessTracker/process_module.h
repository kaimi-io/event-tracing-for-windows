#pragma once
#include <cstdint>
#include <string>

#include <Windows.h>
#include <Evntrace.h>

class process_module
{
public:
	explicit process_module(PEVENT_RECORD record);

	std::uint32_t get_pid() const noexcept
	{
		return pid_;
	}

	std::uint64_t get_image_base() const noexcept
	{
		return image_base_;
	}

	const std::wstring& get_image_name() const noexcept
	{
		return image_name_;
	}

private:
	std::uint32_t pid_ = 0;
	std::uint64_t image_base_ = 0;
	std::wstring image_name_;
};
