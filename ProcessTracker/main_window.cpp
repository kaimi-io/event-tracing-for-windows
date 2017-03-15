#include "main_window.h"

#include <sstream>
#include <stdexcept>

#include "resource.h"

namespace
{
class initialization_error : public std::runtime_error
{
public:
	using std::runtime_error::runtime_error;
};

constexpr const UINT message_tracker_error = WM_APP + 1;
constexpr const UINT message_add_process = WM_APP + 2;
constexpr const UINT message_delete_process = WM_APP + 3;
constexpr const UINT message_add_thread = WM_APP + 4;
constexpr const UINT message_delete_thread = WM_APP + 5;
constexpr const UINT message_add_module = WM_APP + 6;
constexpr const UINT message_delete_module = WM_APP + 7;
constexpr const UINT message_trace_stopped = WM_APP + 8;

struct listview_sort_info
{
	int column_index;
	bool sort_ascending;
};

template<typename T>
int compare_values(const T& left, const T& right, bool sort_ascending)
{
	if (left == right)
		return 0;

	if (sort_ascending)
		return left < right ? -1 : 1;

	return left > right ? -1 : 1;
}

template <class T>
std::wstring to_wstring(T t, std::ios_base & (*f)(std::ios_base&))
{
	std::wostringstream ss;
	ss << f << t;
	return ss.str();
}
} //namespace

main_window::main_window(HINSTANCE instance)
	: tracker_(std::make_unique<process_list>())
{
	on_message_[WM_INITDIALOG] = std::bind(&main_window::on_init_window, this);
	on_message_[WM_CLOSE] = std::bind(&main_window::on_close_window, this);
	on_message_[WM_NCDESTROY] = std::bind(&main_window::on_destroy_window, this);
	on_message_[WM_NOTIFY] = std::bind(&main_window::on_notify,
		this, std::placeholders::_1, std::placeholders::_2);
	on_message_[WM_COMMAND] = std::bind(&main_window::on_command, this, std::placeholders::_1);
	on_message_[message_tracker_error] = std::bind(&main_window::on_tracker_error, this, std::placeholders::_1);
	on_message_[message_add_process] = std::bind(&main_window::add_new_process, this, std::placeholders::_2);
	on_message_[message_delete_process] = std::bind(&main_window::delete_process_from_list, this,
		std::placeholders::_1, std::placeholders::_2);
	on_message_[message_add_thread] = std::bind(&main_window::add_new_thread, this,
		std::placeholders::_1, std::placeholders::_2);
	on_message_[message_delete_thread] = std::bind(&main_window::delete_thread, this,
		std::placeholders::_1, std::placeholders::_2);
	on_message_[message_add_module] = std::bind(&main_window::add_new_module, this,
		std::placeholders::_1, std::placeholders::_2);
	on_message_[message_delete_module] = std::bind(&main_window::delete_module, this,
		std::placeholders::_1, std::placeholders::_2);
	on_message_[message_trace_stopped] = std::bind(&main_window::trace_stopped, this);
	
	result_code_ = ::DialogBoxParamW(instance, MAKEINTRESOURCEW(IDD_MAIN_WINDOW), nullptr,
		main_window_proc_static, reinterpret_cast<LPARAM>(this));
	if (result_code_ == -1)
		throw std::runtime_error("Unable to show main window");
}

bool main_window::trace_stopped() const noexcept
{
	::MessageBoxW(dialog_hwnd_, L"Trace session has been stopped externally",
		L"Error", MB_OK | MB_ICONWARNING);
	return true;
}

void main_window::add_process_list_column(const wchar_t* text, int width,
	process_list_column_id index) const
{
	LVCOLUMNW column{};
	column.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
	column.fmt = LVCFMT_LEFT;
	column.pszText = const_cast<wchar_t*>(text);
	column.cx = width;
	if (-1 == ::SendDlgItemMessageW(dialog_hwnd_, IDC_PROCESS_LIST, LVM_INSERTCOLUMNW,
		static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(&column)))
	{
		throw initialization_error("Unable to add column to listview");
	}
}

void main_window::add_string_to_log(const std::wstring& str) const
{
	auto log_hwnd = ::GetDlgItem(dialog_hwnd_, IDC_LOG);

	auto index = ::SendMessageW(log_hwnd, LB_ADDSTRING, 0,
		reinterpret_cast<LPARAM>(str.c_str()));
	if (LB_ERR == index)
		throw std::runtime_error("Unable to add string to log");

	::SendMessageW(log_hwnd, LB_SETCARETINDEX, index, FALSE);
	
	auto dc = ::GetDC(log_hwnd);
	auto font = ::SendMessageW(log_hwnd, WM_GETFONT, 0, 0);
	if (!dc || !font)
		return;

	auto old_font = ::SelectObject(dc, reinterpret_cast<HFONT>(font));
	if (old_font == HGDI_ERROR)
		return;

	SIZE size{};
	if (::GetTextExtentPoint32W(dc, str.c_str(), str.length(), &size))
	{
		size.cx += 5;
		if (::SendMessageW(log_hwnd, LB_GETHORIZONTALEXTENT, 0, 0) < size.cx)
			::SendMessageW(log_hwnd, LB_SETHORIZONTALEXTENT, size.cx, 0);
	}

	::SelectObject(dc, old_font);
	::ReleaseDC(log_hwnd, dc);
}

void main_window::add_process_list_subitem(const std::wstring& text,
	const process* process_ptr, int sub_item) const
{
	LVITEMW item{};
	item.mask = LVIF_DI_SETITEM | LVIF_TEXT;
	item.iSubItem = sub_item;
	if (!sub_item)
	{
		item.mask |= LVIF_PARAM;
		item.lParam = reinterpret_cast<LPARAM>(process_ptr);
	}

	item.pszText = const_cast<LPWSTR>(text.c_str());
	auto result = ::SendDlgItemMessageW(dialog_hwnd_, IDC_PROCESS_LIST,
		sub_item ? LVM_SETITEMTEXTW : LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item));
	if ((sub_item && !result) || (!sub_item && result == -1))
		throw std::runtime_error("Unable to add process information to the list");
}

int CALLBACK main_window::listview_sort_proc(LPARAM lparam1, LPARAM lparam2,
	LPARAM lparam_sort) noexcept
{
	const listview_sort_info* info = reinterpret_cast<listview_sort_info*>(lparam_sort);
	auto left = reinterpret_cast<const process*>(lparam1);
	auto right = reinterpret_cast<const process*>(lparam2);
	switch (static_cast<process_list_column_id>(info->column_index))
	{
	case process_list_column_id::image_name:
		return compare_values(left->get_path(), right->get_path(), info->sort_ascending);

	case process_list_column_id::parent_pid:
		return compare_values(left->get_parent_pid(), right->get_parent_pid(), info->sort_ascending);

	case process_list_column_id::pid:
		return compare_values(left->get_pid(), right->get_pid(), info->sort_ascending);

	case process_list_column_id::session_id:
		return compare_values(left->get_session_id(), right->get_session_id(), info->sort_ascending);

	default:
		return 0;
	}
}

void main_window::sort_process_list() const noexcept
{
	listview_sort_info info{};
	info.column_index = static_cast<int>(sort_column_);
	info.sort_ascending = sort_ascending_;
	::SendDlgItemMessageW(dialog_hwnd_, IDC_PROCESS_LIST, LVM_SORTITEMS,
		reinterpret_cast<WPARAM>(&info), reinterpret_cast<LPARAM>(listview_sort_proc));
}

bool main_window::add_new_process(LPARAM lparam) const
{
	const process& new_process = *reinterpret_cast<const process*>(lparam);
	add_process_list_subitem(new_process.get_path(), &new_process, 0);
	add_process_list_subitem(std::to_wstring(new_process.get_pid()), &new_process, 1);
	add_process_list_subitem(std::to_wstring(new_process.get_parent_pid()), &new_process, 2);
	add_process_list_subitem(std::to_wstring(new_process.get_session_id()), &new_process, 3);
	sort_process_list();
	std::wostringstream ss;
	ss << L"Started process [PID = " << new_process.get_pid()
		<< L"; Parent PID = " << new_process.get_parent_pid()
		<< L"; Session ID = " << new_process.get_session_id()
		<< L"]; Image: " << new_process.get_path();
	add_string_to_log(ss.str());
	return true;
}

bool main_window::delete_process_from_list(WPARAM wparam, LPARAM lparam)
{
	const process* process_ptr = reinterpret_cast<const process*>(lparam);
	LVFINDINFOW find_info{};
	find_info.flags = LVFI_PARAM;
	find_info.lParam = reinterpret_cast<LPARAM>(process_ptr);
	auto root_index = ::SendDlgItemMessageW(dialog_hwnd_,
		IDC_PROCESS_LIST, LVM_FINDITEMW, -1, reinterpret_cast<LPARAM>(&find_info));
	if (root_index != -1)
	{
		if (!::SendDlgItemMessageW(dialog_hwnd_, IDC_PROCESS_LIST, LVM_DELETEITEM, root_index, 0))
			throw std::runtime_error("Unable to remove process information from the list");
	}

	if (selected_process_ == process_ptr)
		clear_process_info();

	std::wostringstream ss;
	ss << L"Stopped process [PID = " << process_ptr->get_pid()
		<< L"; Exit Code = " << wparam
		<< L"]";
	add_string_to_log(ss.str());
	return true;
}

void main_window::add_process_info_item(HTREEITEM root, const std::wstring& text,
	LPARAM param, const std::function<bool(LPARAM)>& insert_before) const
{
	auto parent = root;
	TVITEMEXW item{};
	item.mask = TVIF_PARAM;
	HTREEITEM prev_item{};
	auto get_mode = TVGN_CHILD;
	while (root = reinterpret_cast<HTREEITEM>(::SendDlgItemMessageW(dialog_hwnd_, IDC_PROCESS_INFO,
		TVM_GETNEXTITEM, get_mode, reinterpret_cast<LPARAM>(root))))
	{
		item.hItem = root;
		get_mode = TVGN_NEXT;
		if (!::SendDlgItemMessageW(dialog_hwnd_, IDC_PROCESS_INFO,
			TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&item)))
		{
			throw std::runtime_error("Unable to add process object to process info");
		}

		if (insert_before(item.lParam))
			break;

		prev_item = root;
	}

	TV_INSERTSTRUCTW insert_item{};
	insert_item.hInsertAfter = prev_item ? prev_item : TVI_FIRST;
	insert_item.hParent = parent;
	insert_item.itemex.mask = TVIF_TEXT | TVIF_PARAM;
	insert_item.itemex.lParam = param;
	insert_item.itemex.pszText = const_cast<LPWSTR>(text.c_str());
	if (!::SendDlgItemMessageW(dialog_hwnd_, IDC_PROCESS_INFO,
		TVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&insert_item)))
	{
		throw std::runtime_error("Unable to add process object to process info");
	}
}

void main_window::delete_process_info_item(HTREEITEM root, LPARAM param) const
{
	TVITEMEXW item{};
	item.mask = TVIF_PARAM;
	auto get_mode = TVGN_CHILD;
	while (root = reinterpret_cast<HTREEITEM>(::SendDlgItemMessageW(dialog_hwnd_, IDC_PROCESS_INFO,
		TVM_GETNEXTITEM, get_mode, reinterpret_cast<LPARAM>(root))))
	{
		item.hItem = root;
		get_mode = TVGN_NEXT;
		if (!::SendDlgItemMessageW(dialog_hwnd_, IDC_PROCESS_INFO,
			TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&item)))
		{
			root = nullptr;
			break;
		}

		if (static_cast<std::uint32_t>(item.lParam) == param)
			break;
	}

	if (!root || !::SendDlgItemMessageW(dialog_hwnd_, IDC_PROCESS_INFO,
		TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(root)))
	{
		throw std::runtime_error("Unable to remove process object from process info");
	}
}

bool main_window::delete_thread(WPARAM wparam, LPARAM lparam)
{
	const auto& target_process = *reinterpret_cast<const process*>(wparam);
	const auto& stopped_thread = *reinterpret_cast<const process_thread*>(lparam);
	if (selected_process_ == &target_process)
		delete_process_info_item(threads_node_, stopped_thread.get_tid());

	std::wostringstream ss;
	ss << L"Process thread stopped [PID = " << target_process.get_pid()
		<< L"; TID = " << stopped_thread.get_tid()
		<< L"]";
	add_string_to_log(ss.str());
	return true;
}

bool main_window::add_new_thread(WPARAM wparam, LPARAM lparam) const
{
	const auto& target_process = *reinterpret_cast<const process*>(wparam);
	const auto& new_thread = *reinterpret_cast<const process_thread*>(lparam);
	if (selected_process_ == &target_process)
	{
		add_process_info_item(threads_node_,
			L"Thread #" + std::to_wstring(new_thread.get_tid()) + L" [EP = "
			+ to_wstring(new_thread.get_ep(), std::hex) + L"]", new_thread.get_tid(),
			[&new_thread](LPARAM lparam)
		{
			return static_cast<std::uint32_t>(lparam) > new_thread.get_tid();
		});
	}

	std::wostringstream ss;
	ss << L"Process thread started [PID = " << target_process.get_pid()
		<< L"; TID = " << new_thread.get_tid()
		<< L"; EP = " << std::hex << new_thread.get_ep()
		<< L"; Usr stack base = " << new_thread.get_user_stack_base()
		<< L"; Usr stack limit = " << new_thread.get_user_stack_limit()
		<< L"]";
	add_string_to_log(ss.str());
	return true;
}

bool main_window::delete_module(WPARAM wparam, LPARAM lparam)
{
	const auto& target_process = *reinterpret_cast<const process*>(wparam);
	const auto& unloaded_module = *reinterpret_cast<const process_module*>(lparam);
	if (selected_process_ == &target_process)
		delete_process_info_item(modules_node_, reinterpret_cast<LPARAM>(&unloaded_module));

	std::wostringstream ss;
	ss << L"Process [PID = " << target_process.get_pid()
		<< L"] unloaded module [Path = " << unloaded_module.get_image_name()
		<< L"]";
	add_string_to_log(ss.str());
	return true;
}

bool main_window::add_new_module(WPARAM wparam, LPARAM lparam) const
{
	const auto& target_process = *reinterpret_cast<const process*>(wparam);
	const auto& new_module = *reinterpret_cast<const process_module*>(lparam);
	if (selected_process_ == &target_process)
	{
		add_process_info_item(modules_node_,
			std::wstring(L"[") + to_wstring(new_module.get_image_base(), std::hex) + L"] "
			+ new_module.get_image_name(),
			reinterpret_cast<LPARAM>(&new_module),
			[&new_module](LPARAM lparam)
		{
			return reinterpret_cast<const process_module*>(lparam)
				->get_image_base() > new_module.get_image_base();
		});
	}

	std::wostringstream ss;
	ss << L"Process [PID = " << target_process.get_pid()
		<< L"] loaded module [Path = " << new_module.get_image_name()
		<< L"; ImageBase = " << std::hex << new_module.get_image_base()
		<< L"]";
	add_string_to_log(ss.str());
	return true;
}

void main_window::on_new_process(const process& new_process) const noexcept
{
	::SendMessageW(dialog_hwnd_, message_add_process, 0, reinterpret_cast<LPARAM>(&new_process));
}

void main_window::on_stopped_process(const process& stopped_process,
	std::uint32_t exit_code) const noexcept
{
	::SendMessageW(dialog_hwnd_, message_delete_process, exit_code,
		reinterpret_cast<LPARAM>(&stopped_process));
}

void main_window::on_new_thread(const process& target_process,
	const process_thread& new_thread) const noexcept
{
	::SendMessageW(dialog_hwnd_, message_add_thread,
		reinterpret_cast<WPARAM>(&target_process), reinterpret_cast<LPARAM>(&new_thread));
}

void main_window::on_stopped_thread(const process& target_process,
	const process_thread& stopped_thread) const noexcept
{
	::SendMessageW(dialog_hwnd_, message_delete_thread,
		reinterpret_cast<WPARAM>(&target_process), reinterpret_cast<LPARAM>(&stopped_thread));
}

void main_window::on_loaded_module(const process& target_process,
	const process_module& new_module) const noexcept
{
	::SendMessageW(dialog_hwnd_, message_add_module,
		reinterpret_cast<WPARAM>(&target_process), reinterpret_cast<LPARAM>(&new_module));
}

void main_window::on_unloaded_module(const process& target_process,
	const process_module& unloaded_module) const noexcept
{
	::SendMessageW(dialog_hwnd_, message_delete_module,
		reinterpret_cast<WPARAM>(&target_process), reinterpret_cast<LPARAM>(&unloaded_module));
}

void main_window::on_error(std::uint32_t error_code) const noexcept
{
	::PostMessageW(dialog_hwnd_, message_tracker_error, error_code, 0);
}

void main_window::on_stop_trace() const noexcept
{
	::PostMessageW(dialog_hwnd_, message_trace_stopped, 0, 0);
}

void main_window::clear_process_info()
{
	selected_process_ = nullptr;
	if (!::SendDlgItemMessageW(dialog_hwnd_, IDC_PROCESS_INFO,
		TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(TVI_ROOT)))
	{
		throw std::runtime_error("Unable to remove current process information");
	}
}

HTREEITEM main_window::insert_process_info_node(HTREEITEM root,
	LPARAM param, const std::wstring& text) const
{
	TV_INSERTSTRUCTW item{};
	item.hInsertAfter = TVI_LAST;
	item.hParent = root;
	item.itemex.mask = TVIF_TEXT;
	if (root != TVI_ROOT)
	{
		item.itemex.mask |= TVIF_PARAM;
		item.itemex.lParam = param;
	}

	item.itemex.pszText = const_cast<LPWSTR>(text.c_str());
	auto result = ::SendDlgItemMessageW(dialog_hwnd_, IDC_PROCESS_INFO,
			TVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item));
	if(!result)
		throw std::runtime_error("Unable to add node to process info tree view");

	return reinterpret_cast<HTREEITEM>(result);
}

void main_window::set_process_info(const process& info)
{
	clear_process_info();
	selected_process_ = &info;

	threads_node_ = insert_process_info_node(TVI_ROOT, 0,
		L"Process #" + std::to_wstring(info.get_pid()) + L" threads");
	modules_node_ = insert_process_info_node(TVI_ROOT, 0,
		L"Process #" + std::to_wstring(info.get_pid()) + L" modules");

	for (const auto& pair : info.get_threads())
	{
		insert_process_info_node(threads_node_, pair.second.get_tid(),
			L"Thread #" + std::to_wstring(pair.second.get_tid()) + L" [EP = "
			+ to_wstring(pair.second.get_ep(), std::hex) + L"]");
	}

	for (const auto& pair : info.get_modules())
	{
		insert_process_info_node(modules_node_, reinterpret_cast<LPARAM>(&pair.second),
			std::wstring(L"[") + to_wstring(pair.second.get_image_base(), std::hex)
			+ L"] " + pair.second.get_image_name());
	}
}

bool main_window::on_notify(WPARAM wparam, LPARAM lparam)
{
	const NMHDR* header = reinterpret_cast<const NMHDR*>(lparam);
	if(!header)
		return false;

	if (header->code == LVN_COLUMNCLICK)
	{
		auto list_view = reinterpret_cast<const NMLISTVIEW*>(lparam);
		if (!sort_ascending_ || static_cast<int>(sort_column_) != list_view->iSubItem)
			sort_ascending_ = true;
		else
			sort_ascending_ = false;

		sort_column_ = static_cast<process_list_column_id>(list_view->iSubItem);
		sort_process_list();
	}
	else if (header->code == LVN_ITEMCHANGED)
	{
		auto info = reinterpret_cast<const NMLISTVIEW*>(lparam);
		if ((info->uNewState & LVIS_SELECTED) == LVIS_SELECTED)
		{
			LVITEM item_info{};
			item_info.mask = LVIF_PARAM;
			item_info.iItem = info->iItem;
			if (!::SendDlgItemMessageW(dialog_hwnd_, IDC_PROCESS_LIST, LVM_GETITEMW,
				0, reinterpret_cast<LPARAM>(&item_info)))
			{
				return false;
			}

			auto target_process = reinterpret_cast<const process*>(item_info.lParam);
			if (selected_process_ != target_process)
				set_process_info(*target_process);
		}
	}

	return false;
}

bool main_window::on_command(WPARAM wparam) const noexcept
{
	switch (LOWORD(wparam))
	{
	case IDC_CLEAR:
		::SendDlgItemMessageW(dialog_hwnd_, IDC_LOG, LB_RESETCONTENT, 0, 0);
		::SendDlgItemMessageW(dialog_hwnd_, IDC_LOG, LB_SETHORIZONTALEXTENT, 0, 0);
		break;

	case IDC_AUTOSCROLL:
		::SendDlgItemMessageW(dialog_hwnd_, IDC_LOG, LB_SETCURSEL, -1, 0);
		break;

	default:
		break;
	}

	return false;
}

bool main_window::on_tracker_error(WPARAM error_code) const
{
	LPWSTR buf = nullptr;
	auto size = ::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, static_cast<DWORD>(error_code), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPWSTR>(&buf), 0, nullptr);
	std::unique_ptr<WCHAR, void(*)(LPWSTR)> deleter(buf, [](LPWSTR str)
	{
		::LocalFree(str);
	});

	std::wstring message(buf, size);

	::MessageBoxW(dialog_hwnd_, (message + L" (code: " + std::to_wstring(error_code) + L")").c_str(),
		L"Tracker Error", MB_OK | MB_ICONERROR);
	return true;
}

bool main_window::on_init_window()
{
	::SendDlgItemMessageW(dialog_hwnd_, IDC_PROCESS_LIST, LVM_SETEXTENDEDLISTVIEWSTYLE,
		0, LVS_EX_FULLROWSELECT);

	add_process_list_column(L"Image", 300, process_list_column_id::image_name);
	add_process_list_column(L"PID", 70, process_list_column_id::pid);
	add_process_list_column(L"Parent PID", 70, process_list_column_id::parent_pid);
	add_process_list_column(L"Session ID", 70, process_list_column_id::session_id);

	try
	{
		connections_.emplace_back(tracker_->on_new_process(
			std::bind(&main_window::on_new_process, this, std::placeholders::_1)));
		connections_.emplace_back(tracker_->on_stopped_process(
			std::bind(&main_window::on_stopped_process, this,
				std::placeholders::_1, std::placeholders::_2)));

		connections_.emplace_back(tracker_->on_new_thread(
			std::bind(&main_window::on_new_thread, this,
				std::placeholders::_1, std::placeholders::_2)));
		connections_.emplace_back(tracker_->on_stopped_thread(
			std::bind(&main_window::on_stopped_thread, this,
				std::placeholders::_1, std::placeholders::_2)));

		connections_.emplace_back(tracker_->on_loaded_module(
			std::bind(&main_window::on_loaded_module, this,
				std::placeholders::_1, std::placeholders::_2)));
		connections_.emplace_back(tracker_->on_unloaded_module(
			std::bind(&main_window::on_unloaded_module, this,
				std::placeholders::_1, std::placeholders::_2)));

		connections_.emplace_back(tracker_->on_error(
			std::bind(&main_window::on_error, this, std::placeholders::_1)));
		connections_.emplace_back(tracker_->on_stop_trace(
			std::bind(&main_window::on_stop_trace, this)));

		tracker_->start_tracking();
	}
	catch (const std::exception& e)
	{
		throw initialization_error(e.what());
	}

	return true;
}

bool main_window::on_close_window() const noexcept
{
	::EndDialog(dialog_hwnd_, 0);
	return true;
}

bool main_window::on_destroy_window()
{
	for (const auto& connection : connections_)
		connection.disconnect();
	
	on_message_.clear();

	return false;
}

INT_PTR CALLBACK main_window::main_window_proc_static(HWND hwnd, UINT msg,
	WPARAM wparam, LPARAM lparam)
{
	LONG_PTR user_data{};
	if (msg == WM_INITDIALOG)
	{
		user_data = lparam;
		::SetWindowLongPtrW(hwnd, GWLP_USERDATA, lparam);
		reinterpret_cast<main_window*>(user_data)->dialog_hwnd_ = hwnd;
	}
	else
	{
		user_data = ::GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	}

	if (user_data)
	{
		try
		{
			return reinterpret_cast<main_window*>(user_data)
				->main_window_proc(hwnd, msg, wparam, lparam);
		}
		catch (const initialization_error& e)
		{
			::MessageBoxA(hwnd, e.what(), "Initialization error", MB_OK | MB_ICONERROR);
			::EndDialog(hwnd, -1);
		}
		catch (const std::exception& e)
		{
			::MessageBoxA(hwnd, e.what(), "Error", MB_OK | MB_ICONERROR);
		}
	}

	return FALSE;
}

INT_PTR main_window::main_window_proc(HWND hwnd, UINT msg,
	WPARAM wparam, LPARAM lparam)
{
	auto it = on_message_.find(msg);
	if (it == on_message_.cend())
		return FALSE;
	
	return (*it).second(wparam, lparam) ? TRUE : FALSE;
}
