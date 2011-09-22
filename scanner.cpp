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

	hr.reset(CoInitialize(NULL));
	if (!hr)
	{
		return FALSE;
	}
	ON_UNWIND_AUTO([&]{CoUninitialize();});

	unique_winerror winerror;
	std::wstring title;
	std::wstring className;
	lib::wr::unique_close_window window;

#if 1
	typedef
		lib::wr::unique_cotask_factory<WCHAR[]>::type
	unique_cotask_wstr;

	auto helloRange = lib::rng::make_range(L"hello");
	unique_cotask_wstr output;
	std::tie(winerror, output) = unique_cotask_wstr::make(helloRange.size());
	if (!winerror)
	{
		return FALSE;
	}
	stdext::checked_array_iterator< WCHAR* > checked_begin(output->begin(), output->size());
	std::copy(helloRange.begin(), helloRange.end(), checked_begin);

	typedef
		lib::wr::unique_local_factory<std::wstring[]>::type
	unique_local_file;
	unique_local_file outputFile;
	std::tie(winerror, outputFile) = unique_local_file::make(2);
	if (!winerror)
	{
		return FALSE;
	}

	unique_cotask_wstr cotaskTitle;
	std::tie(winerror, cotaskTitle) = lib::wr::LoadStdString<unique_cotask_wstr>(hInstance, IDS_APP_TITLE);
	if (!winerror)
	{
		return FALSE;
	}
#endif

	try
	{
		title = lib::wr::LoadStdString(hInstance, IDS_APP_TITLE);
		className = lib::wr::LoadStdString(hInstance, IDC_SCANNER);
	}
	catch(const std::bad_alloc&)
	{
		return FALSE;
	}
	catch(const unique_winerror::exception&)
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
