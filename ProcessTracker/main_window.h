#pragma once

/* Process Tracker (c) DX, kaimi.io */

#include <cstdint>
#include <functional>
#include <memory>

#include <Windows.h>

#include "process_list.h"

class main_window
{
public:
	explicit main_window(HINSTANCE instance);

	INT_PTR get_result_code() const noexcept
	{
		return result_code_;
	}

private:
	enum class process_list_column_id : std::uint8_t
	{
		image_name,
		pid,
		parent_pid,
		session_id,
		max_value
	};
	
	using message_callback = std::function<bool(WPARAM wparam, LPARAM lparam)>;

private:
	static INT_PTR CALLBACK main_window_proc_static(HWND hwnd, UINT msg,
		WPARAM wparam, LPARAM lparam);
	INT_PTR main_window_proc(HWND hwnd, UINT msg,
		WPARAM wparam, LPARAM lparam);

	bool on_init_window();
	bool on_close_window() const noexcept;
	bool on_destroy_window();
	bool on_notify(WPARAM wparam, LPARAM lparam);
	bool on_tracker_error(WPARAM error_code) const;
	bool on_command(WPARAM wparam) const noexcept;
	bool add_new_process(LPARAM lparam) const;
	bool delete_process_from_list(WPARAM wparam, LPARAM lparam);
	bool add_new_thread(WPARAM wparam, LPARAM lparam) const;
	bool delete_thread(WPARAM wparam, LPARAM lparam);
	bool add_new_module(WPARAM wparam, LPARAM lparam) const;
	bool delete_module(WPARAM wparam, LPARAM lparam);
	bool trace_stopped() const noexcept;

	void add_process_list_column(const wchar_t* text, int width,
		process_list_column_id index) const;
	void add_process_list_subitem(const std::wstring& text,
		const process* process_ptr, int sub_item) const;
	void add_string_to_log(const std::wstring& str) const;
	void sort_process_list() const noexcept;
	void set_process_info(const process& info);
	void clear_process_info();
	void add_process_info_item(HTREEITEM root, const std::wstring& text, LPARAM param,
		const std::function<bool(LPARAM)>& insert_before) const;
	void delete_process_info_item(HTREEITEM root, LPARAM param) const;
	HTREEITEM insert_process_info_node(HTREEITEM root, LPARAM param, const std::wstring& text) const;
	static int CALLBACK listview_sort_proc(LPARAM lparam1, LPARAM lparam2, LPARAM lparam_sort) noexcept;

	void on_new_process(const process& new_process) const noexcept;
	void on_stopped_process(const process& stopped_process,
		std::uint32_t exit_code) const noexcept;
	void on_new_thread(const process& target_process,
		const process_thread& new_thread) const noexcept;
	void on_stopped_thread(const process& target_process,
		const process_thread& stopped_thread) const noexcept;
	void on_loaded_module(const process& target_process,
		const process_module& new_module) const noexcept;
	void on_unloaded_module(const process& target_process,
		const process_module& unloaded_module) const noexcept;
	void on_error(std::uint32_t error_code) const noexcept;
	void on_stop_trace() const noexcept;

private:
	std::map<UINT, message_callback> on_message_;
	INT_PTR result_code_ = 0;
	std::vector<boost::signals2::connection> connections_;
	HWND dialog_hwnd_ = nullptr;
	HTREEITEM threads_node_ = nullptr;
	HTREEITEM modules_node_ = nullptr;
	process_list_column_id sort_column_ = process_list_column_id::image_name;
	bool sort_ascending_ = true;
	const process* selected_process_ = nullptr;
	std::unique_ptr<process_list> tracker_;
};
