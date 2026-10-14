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
};

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
