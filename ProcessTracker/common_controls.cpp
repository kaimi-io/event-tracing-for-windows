#include "common_controls.h"

#include <stdexcept>

#include <Windows.h>
#include <CommCtrl.h>

void common_controls::init()
{
	INITCOMMONCONTROLSEX controls{};
	controls.dwSize = sizeof(controls);
	controls.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES | ICC_TREEVIEW_CLASSES;
	if (!::InitCommonControlsEx(&controls))
		throw std::runtime_error("Unable to initialize common controls");
}
