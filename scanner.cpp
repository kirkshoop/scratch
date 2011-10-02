// scanner.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "scanner.h"

#include <Ntsecapi.h> 

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

// enable UNICODE_STRING <-> range

PWSTR range_begin( const UNICODE_STRING& string )
{
	return string.Buffer;
}

PWSTR range_end( const UNICODE_STRING& string )
{
	return string.Buffer + (string.Length / 2);
}

template< class RangeFrom >
UNICODE_STRING range_copy( RangeFrom&& r, UNICODE_STRING* )
{
	UNICODE_STRING result = {};

	result.Buffer = const_cast<PWSTR>(lib::rng::begin(std::forward<RangeFrom>(r)));
	result.Length = lib::rng::size_cast<USHORT>(lib::rng::size(std::forward<RangeFrom>(r)) * 2);
	result.MaximumLength = lib::rng::size_cast<USHORT>(lib::rng::size(std::forward<RangeFrom>(r)) * 2);

	return result;
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

	auto result = function_contract_hresult(
		[&] () -> unique_hresult
		{
			WPP_INIT_TRACING(SCANNER_TRACING_ID);
			ON_UNWIND_AUTO([&]{
				WPP_CLEANUP();
			});

			TRACE_SCOPE("commandline: %ls", lpCmdLine);

			unique_winerror winerror;
			unique_hresult hr;

			hr.reset(CoInitialize(NULL)).throw_if();
			ON_UNWIND_AUTO([&]{CoUninitialize();});

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
			winerror.throw_if();
			stdext::checked_array_iterator< WCHAR* > checked_begin(output->begin(), output->size());
			std::copy(helloRange.begin(), helloRange.end(), checked_begin);

			typedef
				lib::wr::unique_local_factory<std::wstring[]>::type
			unique_local_strings;
			unique_local_strings strings;
			std::tie(winerror, strings) = unique_local_strings::make(2);
			winerror.throw_if();

			unique_cotask_wstr cotaskTitle;
			std::tie(winerror, cotaskTitle) = lib::wr::LoadStdString<unique_cotask_wstr>(hInstance, IDS_APP_TITLE);
			winerror.throw_if();

			auto string = lib::rng::copy<UNICODE_STRING>(helloRange);
			auto stringRange = lib::rng::make_range(string);
			auto unicodeTitle = lib::rng::copy<UNICODE_STRING>(lib::rng::make_range_raw(title));
			stringRange = lib::rng::make_range(unicodeTitle);
#endif

			title = lib::wr::LoadStdString(hInstance, IDS_APP_TITLE);
			className = lib::wr::LoadStdString(hInstance, IDC_SCANNER);

			// Perform application initialization:
			std::tie(winerror, window) = CreateMainWindow(hInstance, className.c_str(), title.c_str(), nCmdShow);
			winerror.throw_if();

			if (!window)
			{
				return hresult_cast(E_UNEXPECTED);
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

			return hr;
		}
	);
	return SUCCEEDED(result) ? (int) msg.wParam : FALSE;
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
