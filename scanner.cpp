// scanner.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "scanner.h"

#include "scanner.tmh"

void unique_error_report_initiated(DWORD value, unique_winerror::tag&&)
{
	static DWORD anchor;
	anchor = value;
}

void unique_error_report_reset(DWORD value, unique_winerror::tag&&)
{
	static DWORD anchor;
	anchor = value;
}

void unique_error_report_initiated(HRESULT value, unique_hresult::tag&&)
{
	static HRESULT anchor;
	anchor = value;
}

void unique_error_report_reset(HRESULT value, unique_hresult::tag&&)
{
	static HRESULT anchor;
	anchor = value;
}

int APIENTRY wWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPWSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg = {};
	HACCEL hAccelTable = NULL;

	WPP_INIT_TRACING(SCANNER_TRACING_ID);
	ON_UNWIND_AUTO([&]{
		WPP_CLEANUP();
	});

	TRACE_SCOPE("commandline: %ls", lpCmdLine);

	unique_hresult hr;
#if 1
	hr.reset(CoInitialize(NULL));
	if (!hr)
	{
		return FALSE;
	}
	ON_UNWIND_AUTO([&]{CoUninitialize();});
#endif

	unique_winerror winerror;
	std::wstring title;
	std::wstring className;
	lib::unique_close_window window;

	std::tie(winerror, title) = lib::LoadStdString(hInstance, IDS_APP_TITLE);
	if (!winerror)
	{
		return FALSE;
	}

	std::tie(winerror, className) = lib::LoadStdString(hInstance, IDC_SCANNER);
	if (!winerror)
	{
		return FALSE;
	}

	// Perform application initialization:
	std::tie(winerror, window) = CreateMainWindow(hInstance, className.c_str(), title.c_str(), nCmdShow);

	if (!winerror || !window)
	{
		return winerror.get();
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SCANNER));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
