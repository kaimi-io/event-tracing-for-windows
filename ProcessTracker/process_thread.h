#pragma once
#include <cstdint>

#include <Windows.h>
#include <Evntrace.h>

class process_thread
{
public:
	explicit process_thread(PEVENT_RECORD record);

	std::uint32_t get_tid() const noexcept
	{
		return tid_;
	}

	std::uint32_t get_pid() const noexcept
	{
		return pid_;
	}

	std::uint64_t get_ep() const noexcept
	{
		return ep_;
	}

	std::uint64_t get_user_stack_base() const noexcept
	{
		return pid_;
	}

	std::uint64_t get_user_stack_limit() const noexcept
	{
		return user_stack_limit_;
	}

private:
	std::uint32_t pid_ = 0;
	std::uint32_t tid_ = 0;
	std::uint64_t ep_ = 0;
	std::uint64_t user_stack_base_ = 0;
	std::uint64_t user_stack_limit_ = 0;
};
