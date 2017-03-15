#pragma once
#include <cstdint>
#include <map>
#include <string>

#include "process_module.h"
#include "process_thread.h"

class process
{
public:
	using thread_map = std::map<std::uint32_t, process_thread>;
	using module_map = std::map<std::uint64_t, process_module>;

public:
	process(PEVENT_RECORD record);

	const std::wstring& get_path() const noexcept
	{
		return path_;
	}

	std::uint32_t get_pid() const noexcept
	{
		return pid_;
	}

	std::uint32_t get_parent_pid() const noexcept
	{
		return parent_pid_;
	}

	std::uint32_t get_session_id() const noexcept
	{
		return session_id_;
	}

	process_thread& add_thread(process_thread&& thread);
	void remove_thread(std::uint32_t tid);
	process_thread* get_thread(std::uint32_t tid);
	const thread_map& get_threads() const noexcept
	{
		return threads_;
	}

	process_module& add_module(process_module&& module);
	void remove_module(std::uint64_t image_base);
	process_module* get_module(std::uint64_t image_base);
	const module_map& get_modules() const noexcept
	{
		return modules_;
	}

private:
	std::wstring path_;
	std::uint32_t pid_ = 0;
	std::uint32_t parent_pid_ = 0;
	std::uint32_t session_id_ = 0;
	thread_map threads_;
	module_map modules_;
};
