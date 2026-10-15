/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2015-2026 Baldur Karlsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#pragma once

// Shared between:
//  * the launcher (captureprobecmd/UI) which publishes the data,
//  * captureprobeshimNN.dll which picks it up when global hooking is enabled,
//  * the proxy dxgi.dll (dll mode) which picks it up when it is loaded from the
//    application's directory.
//
// This header must stay free of any CRT dependencies, as it is also included by
// the shim which deliberately links against nothing but kernel32.

#include <stdint.h>

struct ShimData
{
  wchar_t pathmatchstring[2048];
  wchar_t rdocpath[2048];
  char debuglog[2048];
  char capfile[2048];

  unsigned char opts[512];

  // Set to 1 by the launcher when this data was published for dll mode, so the
  // proxy knows it should also report back through this structure.
  uint32_t dllMode;
  // Progress reported by the target: 0 nothing, 1 capture dll loaded,
  // 2 configuration applied, 3 ident published below.
  uint32_t targetStatus;
  // Target control ident (listening port) of the target, so the launcher can
  // connect to it exactly as it does after a normal injection.
  uint32_t ident;
  uint32_t reserved;

  // Name the dll mode proxy disguises itself as once it has loaded everything:
  // its entry in the loader's module list (BaseDllName/FullDllName) is renamed to
  // this. Never a path, only a file name - the launcher sanitises it and the proxy
  // refuses anything that is not a plain file name. Empty means the proxy's own
  // default.
  wchar_t proxyname[64];
  // Where the disguised full path points, one of the DLL_MODE_PATH_* values below.
  uint32_t proxypath;
};

// Where the disguised full path of the proxy points. The system directory is the
// default: the disguised path then names a real, signed system module, so a check for
// the file or its signature finds an ordinary system file. The proxy's own directory is
// the conservative choice - the named file deliberately does not exist, so nothing can
// load a module from it - but a check for the file will not find it.
//
// Zero is the default so that a zero-initialised CaptureOptions gets this choice.
#define DLL_MODE_PATH_SYSTEM_DIR 0
#define DLL_MODE_PATH_PROXY_DIR 1

// The name the proxy disguises itself as when nothing else is configured: a real
// system component that is a plausible thing for a game process to have loaded, but
// that games rarely load themselves (a name that is already in the process would
// appear twice in the module list once the proxy disguises itself as it).
//
// Other names that fit the same description, and the ones to avoid:
//  * good: msdmo.dll, dsdmo.dll, ksuser.dll, mf.dll, mfreadwrite.dll, pdh.dll
//  * avoid because games commonly load them: mfplat.dll, d3dcompiler_47.dll,
//    xinput1_4.dll, dinput8.dll, hid.dll, dwrite.dll, d2d1.dll, dcomp.dll,
//    dxcore.dll, d3d12core.dll, vulkan-1.dll, nvapi64.dll, winmm.dll, version.dll,
//    wintrust.dll, msvcp140.dll, ucrtbase.dll
//  * avoid because they are always in the process: KnownDLLs (kernel32, user32,
//    ole32, combase, shcore, shell32, ...) and ntdll, kernelbase, win32u
#define DLL_MODE_DEFAULT_PROXY_NAME L"mfreadwrite.dll"

// _WIN64 is what the compiler defines, WIN64 is what the build system defines -
// accept either so the name matches no matter how a module was built
#if defined(_WIN64) || defined(WIN64)
#define GLOBAL_HOOK_DATA_NAME "CaptureProbeGlobalHookData64"
#define DLL_MODE_DATA_NAME "CaptureProbeDllMode64"
#define SHIM_DLL_NAME "captureprobeshim64.dll"
#else
#define GLOBAL_HOOK_DATA_NAME "CaptureProbeGlobalHookData32"
#define DLL_MODE_DATA_NAME "CaptureProbeDllMode32"
#define SHIM_DLL_NAME "captureprobeshim32.dll"
#endif
