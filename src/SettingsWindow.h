#pragma once

#include "TrackingServer.h"
#include "Viewer.h"

#include <thread>
#include <mutex>

#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>
#include "resource.h"

#include <osvr/PluginKit/PluginKit.h>
#include <osvr/PluginKit/TrackerInterfaceC.h>

namespace OSVRCardboard {
	struct ui_thread_data
	{
		bool end = false;
	};

	class SettingsWindow {
	public:
		SettingsWindow(OSVR_PluginRegContext ctx);
		~SettingsWindow();

		OSVR_ReturnCode update();

		static void ui_thread(ui_thread_data& data);
		static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	private:
		std::thread *m_ui_thread;
		ui_thread_data m_ui_thread_data;
		osvr::pluginkit::PluginContext mContext;
		osvr::pluginkit::DeviceToken mDev;
		OSVR_TrackerDeviceInterface mTracker;

		static void saveConfig(HWND hDlg);
	};
}