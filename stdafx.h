// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
// Windows Header Files:
#include <windows.h>
#include <winerror.h>

#include <WindowsX.h>
#include <unknwn.h>

#include <propidl.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <new>
#include <memory>
#include <tuple>
#include <utility>
#include <algorithm>
#include <iterator>
#include <exception>
#include <unordered_map>

#define LIBRARIES_NAMESPACE libraries
#include <libraries.h>
namespace lib=LIBRARIES_NAMESPACE;

#include "WIAAdaptor.h"
#include "Direct2dText.h"
#include "XmlLiteAdaptor.h"
#include "MainWindow.h"

//
// for WPP tracing.
// Must use /Zi to generate the pdb's and add a PreBuild BuildEvent with the following:
//    "C:\WinDDK\7600.16385.1\bin\x86\tracewpp.exe" -v4 -dll -cfgdir:C:\WinDDK\7600.16385.1\bin\WppConfig\Rev1 -odir:$(ProjectDir) -scan:$(ProjectDir)stdafx.h $(ProjectDir)*.cpp
//
// and then the following definitions

#define SCANNER_TRACING_ID      L"Shoop\\Kirk\\Scanner"


// TODO: Define trace flag bits for your WPD driver
#define WPP_CONTROL_GUIDS \
	WPP_DEFINE_CONTROL_GUID( \
		ScannerWppCtrlGuid, \
		(CCDA22F0,D30D,432F,BD0E,CA34D1AD41EC), \
		WPP_DEFINE_BIT(TRACE_FLAG_ALL) \
		WPP_DEFINE_BIT(TRACE_FLAG_UI) \
		WPP_DEFINE_BIT(TRACE_FLAG_WIA) \
		WPP_DEFINE_BIT(TRACE_FLAG_TP) \
	)

#define WPP_FLAG_LEVEL_LOGGER(flags, lvl) \
           WPP_LEVEL_LOGGER(flags)

#define WPP_FLAG_LEVEL_ENABLED(flags, lvl) \
           (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//
// This comment block is scanned by the trace preprocessor to define our
// Trace and TraceEvents function.
//
// begin_wpp config
// FUNC Trace{FLAG=TRACE_FLAG_ALL}(LEVEL, MSG, ...);
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// end_wpp


//
// MACRO: TRACE_SCOPE
// Configuration block that defines trace macro. It uses the PRE/POST macros to include
// code as part of the trace macro expansion. TRACE_FUNCTION is equivalent to the code below:
//
// This is the code in the PRE macro
// 	auto uwfunc_ ## __LINE__ = [&] () -> void { 
//     DoTraceMessage(TRACE_FLAG_ALL,  "%!STDPREFIX!" MSG " hr= %!HRESULT!", ..., hrCheck)
// This is the code in the POST macro
//  };
//  uwfunc_ ## __LINE__();
//	UNWINDER_NAMESPACE::unwinder<decltype(uwfunc_ ## __LINE__)> UnwinderName(std::addressof(uwfunc_ ## __LINE__));
//                                 
// begin_wpp config
// USEPREFIX (TRACE_SCOPE,"%!STDPREFIX!");
// FUNC TRACE_SCOPE{FLAG=TRACE_FLAG_ALL, LEVEL=TRACE_LEVEL_VERBOSE}(MSG, ...);
// end_wpp
#define WPP_FLAG_LEVEL_PRE(FLAGS, LEVEL) auto MAKE_IDENTIFIER(trace_scope_func_) = [&] () -> void { 
#define WPP_FLAG_LEVEL_POST(FLAGS, LEVEL) ; }; MAKE_IDENTIFIER(trace_scope_func_)(); UNWINDER_NAMESPACE::unwinder<decltype(MAKE_IDENTIFIER(trace_scope_func_))> MAKE_IDENTIFIER(trace_scope_unwind_)(std::addressof(MAKE_IDENTIFIER(trace_scope_func_)));

