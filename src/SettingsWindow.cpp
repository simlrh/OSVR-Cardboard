#include "SettingsWindow.h"
#include "TrackingServer.h"
#include "je_nourish_cardboard_json.h"

#include <json/json.h>

#include <map>
#include <iostream>

#include <Shobjidl.h>
#include <atlbase.h>

namespace OSVRCardboard {

	TrackingServer *server = NULL;

	DWORD resolutions[2][3] = {
		{ 1920, 1080, 32 },
		{ 1920, 1080, 32 }
	};
	DEVMODE _devmode;

	SettingsWindow::SettingsWindow(OSVR_PluginRegContext ctx) : mContext(ctx)
	{
		m_ui_thread = new std::thread(SettingsWindow::ui_thread, std::ref(m_ui_thread_data));

		OSVR_DeviceInitOptions opts = osvrDeviceCreateInitOptions(ctx);

		osvrDeviceTrackerConfigure(opts, &mTracker);

		/// Create the device token with the options
		mDev.initAsync(ctx, "GoogleCardboard", opts);

		/// Send JSON descriptor
		mDev.sendJsonDescriptor(je_nourish_cardboard_json);

		/// Register update callback
		mDev.registerUpdateCallback(this);
	}

	SettingsWindow::~SettingsWindow()
	{
		m_ui_thread_data.end = true;
		m_ui_thread->join();
	}

	OSVR_ReturnCode SettingsWindow::update() {
		TimestampedQuaternion q;
		
		while (server && server->hasQuaternion())
		{
			q = server->quaternion();
			osvrDeviceTrackerSendOrientationTimestamped(mDev, mTracker, &q.quaternion, 0, &q.timestamp);
		}

		return OSVR_RETURN_SUCCESS;
	}

	void SettingsWindow::ui_thread(ui_thread_data& data)
	{
		MSG msg;
		BOOL ret;
		HWND hDlg;
		HINSTANCE hInst;

		server = new TrackingServer();

		bool wasReady = true;
		bool isReady = false;

		hInst = GetModuleHandle("je_nourish_cardboard.dll");
		hDlg = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_DIALOG1), 0, DialogProc, 0);
		ShowWindow(hDlg, SW_RESTORE);
		CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
		HWND hConnButton = GetDlgItem(hDlg, IDC_DISCONNECT);

		EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &_devmode);
		resolutions[0][0] = _devmode.dmPelsWidth;
		resolutions[0][1] = _devmode.dmPelsHeight;
		resolutions[0][2] = _devmode.dmBitsPerPel;
		_devmode.dmSize = sizeof(_devmode);

		std::string viewerResolution = "System resolution (" + std::to_string(resolutions[0][0]) + "x" + std::to_string(resolutions[0][1]) + ")";
		SetDlgItemText(hDlg, IDC_RADIO1, viewerResolution.c_str());

		viewerResolution = "Viewer resolution (" + std::to_string(resolutions[1][0]) + "x" + std::to_string(resolutions[1][1]) + ")";
		SetDlgItemText(hDlg, IDC_RADIO2, viewerResolution.c_str());

		UpdateWindow(hDlg);


		do {
			if (ret = PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
				if (!IsDialogMessage(hDlg, &msg)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}

			isReady = server->isReady();
			if (isReady && !wasReady) {
				ShowWindow(hConnButton, SW_SHOW);
				SetDlgItemText(hDlg, IDC_CONNECTION_STATUS, server->getStatus());
				UpdateWindow(hConnButton);
			}
			else if (!isReady && wasReady) {
				SetDlgItemText(hDlg, IDC_CONNECTION_STATUS, server->getStatus());
				ShowWindow(hConnButton, SW_HIDE);
				UpdateWindow(hConnButton);
			}
			wasReady = isReady;

			if (isReady) {
				if (server->configChanged()) {
					Viewer viewer = server->config();
					if (viewer.hasMagnet()) {
						MessageBox(hDlg, "Warning: This viewer device contains magnets which may interfere with orientation tracking.",
							"OSVR Cardboard", MB_OK);
					}
					std::string viewerName = viewer.vendor() + " " + viewer.model();
					if (viewerName.length() == 1) {
						viewerName = "No viewer - scan QR code";
					}
					else {
						viewerName.append(" - " + viewer.name());
					}
					SetDlgItemText(hDlg, IDC_CONFIGURATION_STATUS, viewerName.c_str());
					EnableWindow(GetDlgItem(hDlg, IDC_SAVE_CONFIGURATION), true);

					viewer.resolution(resolutions[1]);
					viewerResolution = "Viewer resolution (" + std::to_string(resolutions[1][0]) + "x" + std::to_string(resolutions[1][1]) + ")";
					SetDlgItemText(hDlg, IDC_RADIO2, viewerResolution.c_str());
				}
			}

			if (data.end) break;
		} while (true);

		DestroyWindow(hDlg);
	}

	INT_PTR CALLBACK SettingsWindow::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_DISCONNECT:
				server->disconnect();
				break;
			case IDC_RADIO1:
			case IDC_RADIO2:
				if (BST_CHECKED == Button_GetCheck(GetDlgItem(hDlg, LOWORD(wParam)))) {

					_devmode.dmPelsWidth = resolutions[LOWORD(wParam) - IDC_RADIO1][0];
					_devmode.dmPelsHeight = resolutions[LOWORD(wParam) - IDC_RADIO1][1];
					_devmode.dmBitsPerPel = resolutions[LOWORD(wParam) - IDC_RADIO1][2];
					_devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

					ChangeDisplaySettings(&_devmode, CDS_FULLSCREEN);
				}
				break;
			case IDC_SAVE_CONFIGURATION:
				saveConfig(hDlg);
				break;
			}
			break;

		case WM_CLOSE:
			return FALSE;

		case WM_DESTROY:
			PostQuitMessage(0);
			return TRUE;
		}

		return FALSE;
	}

	void SettingsWindow::saveConfig(HWND hDlg)
	{
		Viewer viewer = server->config();

		HRESULT hr;
		CComPtr<IFileSaveDialog> pDlg;
		COMDLG_FILTERSPEC aFileTypes[] = {
			{ L"Json files", L"*.json" },
			{ L"All files", L"*.*" }
		};

		hr = pDlg.CoCreateInstance(__uuidof(FileSaveDialog));

		if (FAILED(hr))
			return;

		std::string filename = viewer.model().append("-" + viewer.name()).append(".json");
		std::string contents = viewer.displayConfig();

		pDlg->SetFileTypes(_countof(aFileTypes), aFileTypes);
		pDlg->SetTitle(L"Save OSVR display config");
		pDlg->SetFileName(std::wstring(filename.begin(), filename.end()).c_str());
		pDlg->SetDefaultExtension(L"json");

		// Show the dialog.
		hr = pDlg->Show(hDlg);

		// If the user chose a file, save the user's data to that file.
		if (SUCCEEDED(hr))
		{
			CComPtr<IShellItem> pItem;
			hr = pDlg->GetResult(&pItem);
			if (SUCCEEDED(hr))
			{
				PWSTR pwsz = NULL;
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pwsz);
				if (SUCCEEDED(hr))
				{
					std::wstring wname(pwsz);
					std::string filepath(wname.begin(), wname.end());

					HANDLE hFile = CreateFile(filepath.c_str(),
						GENERIC_WRITE,
						0,
						NULL,
						CREATE_ALWAYS,
						FILE_ATTRIBUTE_NORMAL,
						NULL);

					if (hFile != INVALID_HANDLE_VALUE) {

						DWORD dwBytesWritten = 0;
						BOOL bErrorFlag = WriteFile(
							hFile,
							contents.c_str(),
							contents.length(),
							&dwBytesWritten,
							NULL);

						CloseHandle(hFile);

					}

					CoTaskMemFree(pwsz);
				}
			}
		}
	}
}