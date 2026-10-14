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

// Proxy dxgi.dll used by CaptureProbe's "dll mode".
//
// It is copied next to the application's executable, so that the loader finds it
// before the system copy: dxgi.dll is not in the KnownDLLs list, and the
// application directory is searched first. Applications that resolve dxgi
// dynamically (UE5 does: FPlatformProcess::GetDllHandle("dxgi.dll")) therefore
// load this module instead of C:\Windows\System32\dxgi.dll.
//
// In DllMain - ie. before the application can call any of our exports - it:
//   1. loads the real system dxgi.dll and d3d12.dll by absolute path,
//   2. caches the address of every real export *before* any hook can be active,
//   3. replaces the address of each of our exports with a jump thunk to the real
//      function, so this module is a transparent pass-through for any caller
//      that does get our export (the capture dll normally intercepts
//      GetProcAddress/IAT lookups first and returns its own hook),
//   4. loads the capture dll (captureprobe.dll) so its hooks are registered
//      before the application makes any D3D call,
//   5. applies the configuration the launcher published in shared memory and
//      reports back the target control ident, so the launcher can connect.
//
// Everything in step 4/5 is deliberately done during DllMain: doing it on a
// worker thread would let the application reach D3D before the hooks exist.

#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>

#include "../../renderccccc/os/win32/global_hook_data.h"

#define PROXY_ARRAY_COUNT(a) (sizeof(a) / sizeof(a[0]))

// diagnostics: go to the debug output and to a log next to the capture dll's
// logs, since a proxy dll that fails quietly is impossible to diagnose.
static void ProxyLog(const char *fmt, ...)
{
  char buf[1024] = {};

  va_list args;
  va_start(args, fmt);
  _vsnprintf_s(buf, sizeof(buf) - 1, _TRUNCATE, fmt, args);
  va_end(args);

  OutputDebugStringA(buf);
  OutputDebugStringA("\n");

  wchar_t tempPath[MAX_PATH] = {};
  DWORD len = GetTempPathW(MAX_PATH - 1, tempPath);
  if(len == 0 || len >= MAX_PATH - 1)
    return;

  wchar_t dir[MAX_PATH] = {};
  wcscpy_s(dir, tempPath);
  wcscat_s(dir, L"CaptureProbe");
  CreateDirectoryW(dir, NULL);

  wchar_t file[MAX_PATH] = {};
  wcscpy_s(file, dir);
  wcscat_s(file, L"\\dllmode_proxy.log");

  HANDLE h = CreateFileW(file, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                         OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if(h == INVALID_HANDLE_VALUE)
    return;

  DWORD ignored = 0;
  WriteFile(h, buf, (DWORD)strlen(buf), &ignored, NULL);
  WriteFile(h, "\r\n", 2, &ignored, NULL);
  CloseHandle(h);
}

#define PROXY_LOG(...) ProxyLog(__VA_ARGS__)

// names of the exports of the system dxgi.dll, in the order they appear in its
// export table. Our module exports all of them (see dxgiproxy.def) and patches
// each address at runtime.
static const char *const kExportNames[] = {
    "ApplyCompatResolutionQuirking",
    "CompatString",
    "CompatValue",
    "CreateDXGIFactory",
    "CreateDXGIFactory1",
    "CreateDXGIFactory2",
    "DXGID3D10CreateDevice",
    "DXGID3D10CreateLayeredDevice",
    "DXGID3D10GetLayeredDeviceSize",
    "DXGID3D10RegisterLayers",
    "DXGIDeclareAdapterRemovalSupport",
    "DXGIDumpJournal",
    "DXGIGetDebugInterface1",
    "DXGIReportAdapterConfiguration",
    "PIXBeginCapture",
    "PIXEndCapture",
    "PIXGetCaptureState",
    "SetAppCompatStringPointer",
    "UpdateHMDEmulationStatus",
};

static HMODULE gSelfModule = NULL;
static HMODULE gSystemDxgi = NULL;
static HMODULE gSystemD3D12 = NULL;
static HMODULE gCaptureDll = NULL;
static bool gInitialised = false;

// The jump thunks live in our own image: an export table entry holds a 32-bit
// RVA, so a thunk allocated anywhere else (VirtualAlloc typically returns an
// address far from a module loaded high) cannot be addressed by it.
#define PROXY_THUNK_SIZE 16

#pragma section(".proxythnk", execute, read)
// the thunks are filled in at runtime, so the section has to end up writable
#pragma comment(linker, "/SECTION:.proxythnk,RWE")
__declspec(allocate(".proxythnk")) unsigned char
    g_thunkSpace[PROXY_ARRAY_COUNT(kExportNames) * PROXY_THUNK_SIZE] = {};

// exported by dxgiproxy.def under every name in kExportNames. Never called -
// the export table addresses are redirected to the real functions - but it has
// to exist so the module has the exports at all.
extern "C" void ProxyExportStub()
{
}

// Builds a thunk that jumps to target preserving every register and argument, so
// no export signature has to be known here.
static void *MakeJumpThunk(size_t index, void *target)
{
  BYTE *mem = g_thunkSpace + index * PROXY_THUNK_SIZE;

#if defined(_WIN64)
  // jmp qword ptr [rip+1] ; <pad> ; <target>
  // the instruction is 7 bytes (REX + FF + modrm + disp32), so rip after it is
  // mem+7 and disp 1 points at the slot at mem+8
  mem[0] = 0x48;
  mem[1] = 0xFF;
  mem[2] = 0x25;
  *(uint32_t *)(mem + 3) = 1;
  mem[7] = 0xCC;
  *(void **)(mem + 8) = target;
#else
  // jmp dword ptr [slot]
  mem[0] = 0xFF;
  mem[1] = 0x25;
  *(uint32_t *)(mem + 2) = (uint32_t)(uintptr_t)(mem + 8);
  *(void **)(mem + 8) = target;
#endif

  FlushInstructionCache(GetCurrentProcess(), mem, PROXY_THUNK_SIZE);
  return mem;
}

// rewrites the export address of `name` in `module` so that it points at
// newAddress instead of the stub in this module's image
static bool PatchExportAddress(HMODULE module, const char *name, void *newAddress)
{
  if(module == NULL || newAddress == NULL)
    return false;

  BYTE *base = (BYTE *)module;

  PIMAGE_DOS_HEADER dosheader = (PIMAGE_DOS_HEADER)base;
  if(dosheader->e_magic != 0x5a4d)
    return false;

  PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(base + dosheader->e_lfanew);
  if(ntHeaders->Signature != 0x00004550)
    return false;

  IMAGE_DATA_DIRECTORY &exportDir =
      ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
  if(exportDir.VirtualAddress == 0)
    return false;

  PIMAGE_EXPORT_DIRECTORY exports = (PIMAGE_EXPORT_DIRECTORY)(base + exportDir.VirtualAddress);

  DWORD *names = (DWORD *)(base + exports->AddressOfNames);
  WORD *ordinals = (WORD *)(base + exports->AddressOfNameOrdinals);
  DWORD *functions = (DWORD *)(base + exports->AddressOfFunctions);

  for(DWORD i = 0; i < exports->NumberOfNames; i++)
  {
    if(strcmp((const char *)(base + names[i]), name) != 0)
      continue;

    WORD index = ordinals[i];
    if(index >= exports->NumberOfFunctions)
      return false;

    DWORD oldProtection = 0;
    if(!VirtualProtect(&functions[index], sizeof(DWORD), PAGE_READWRITE, &oldProtection))
      return false;

    functions[index] = (DWORD)((BYTE *)newAddress - base);

    DWORD ignored = 0;
    VirtualProtect(&functions[index], sizeof(DWORD), oldProtection, &ignored);
    return true;
  }

  return false;
}

static HMODULE LoadSystemModule(const wchar_t *name)
{
  wchar_t systemDir[MAX_PATH] = {};
  UINT len = GetSystemDirectoryW(systemDir, MAX_PATH - 1);
  if(len == 0 || len >= MAX_PATH - 1)
    return NULL;

  wchar_t path[MAX_PATH] = {};
  if(wcslen(systemDir) + wcslen(name) + 2 >= MAX_PATH)
    return NULL;

  wcscpy_s(path, systemDir);
  wcscat_s(path, L"\\");
  wcscat_s(path, name);

  // full path: this loads the system copy, not this module
  return LoadLibraryExW(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
}

struct SharedConfig
{
  HANDLE mapping = NULL;
  ShimData *data = NULL;
  bool writable = false;
};

// opens the shared data the launcher publishes, preferring the dll mode mapping.
// the view is kept open while the capture dll is loaded so we can report back.
static SharedConfig OpenSharedConfig()
{
  SharedConfig ret = {};

  const char *names[2] = {DLL_MODE_DATA_NAME, GLOBAL_HOOK_DATA_NAME};

  for(size_t i = 0; i < PROXY_ARRAY_COUNT(names); i++)
  {
    HANDLE mapping = OpenFileMappingA(FILE_MAP_WRITE, FALSE, names[i]);
    bool writable = mapping != NULL;

    if(mapping == NULL)
      mapping = OpenFileMappingA(FILE_MAP_READ, FALSE, names[i]);

    if(mapping == NULL)
      continue;

    ShimData *data =
        (ShimData *)MapViewOfFile(mapping, writable ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0,
                                  sizeof(ShimData));

    if(data == NULL)
    {
      CloseHandle(mapping);
      continue;
    }

    ret.mapping = mapping;
    ret.data = data;
    ret.writable = writable;
    PROXY_LOG("using shared config '%s'", names[i]);
    return ret;
  }

  return ret;
}

static void CloseSharedConfig(SharedConfig &config)
{
  if(config.data)
    UnmapViewOfFile(config.data);
  if(config.mapping)
    CloseHandle(config.mapping);
  config.data = NULL;
  config.mapping = NULL;
}

// resolves the capture dll: the launcher tells us where it is, otherwise look
// next to this module (and let the loader search PATH as a last resort).
static HMODULE LoadCaptureDll(const ShimData *data)
{
  if(data && data->rdocpath[0])
  {
    HMODULE mod = LoadLibraryExW(data->rdocpath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if(mod)
      return mod;

    PROXY_LOG("couldn't load the capture dll from the published path '%ls' (%u)", data->rdocpath,
              GetLastError());
  }
  else
  {
    PROXY_LOG("no shared configuration found, falling back to the proxy's own directory");
  }

  wchar_t selfPath[MAX_PATH] = {};
  if(GetModuleFileNameW(gSelfModule, selfPath, MAX_PATH - 1) != 0)
  {
    wchar_t *slash = wcsrchr(selfPath, L'\\');
    if(slash)
    {
      *(slash + 1) = 0;

      wchar_t sibling[MAX_PATH] = {};
      if(wcslen(selfPath) + 16 < MAX_PATH)
      {
        wcscpy_s(sibling, selfPath);
        wcscat_s(sibling, L"captureprobe.dll");

        HMODULE mod = LoadLibraryExW(sibling, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if(mod)
          return mod;

        PROXY_LOG("couldn't load '%ls' (%u)", sibling, GetLastError());
      }
    }
  }

  HMODULE mod = LoadLibraryW(L"captureprobe.dll");
  if(mod == NULL)
    PROXY_LOG("couldn't load captureprobe.dll from the search path (%u)", GetLastError());

  return mod;
}

typedef void(__cdecl *pSetCaptureOptions)(const void *opts);
typedef void(__cdecl *pSetCaptureFile)(const char *capfile);
typedef void(__cdecl *pSetDebugLogFile)(const char *logfile);
typedef void(__cdecl *pGetTargetControlIdent)(uint32_t *ident);

static void ConfigureCaptureDll(HMODULE capture, ShimData *data, bool writable)
{
  if(capture == NULL)
    return;

  if(data == NULL)
    return;

  if(data->capfile[0])
  {
    pSetCaptureFile setcapfile = (pSetCaptureFile)GetProcAddress(capture, "INTERNAL_SetCaptureFile");
    if(setcapfile)
      setcapfile(data->capfile);
  }

  if(data->debuglog[0])
  {
    pSetDebugLogFile setdebuglog =
        (pSetDebugLogFile)GetProcAddress(capture, "INTERNAL_SetDebugLogFile");
    if(setdebuglog)
      setdebuglog(data->debuglog);
  }

  {
    pSetCaptureOptions setopts =
        (pSetCaptureOptions)GetProcAddress(capture, "INTERNAL_SetCaptureOptions");
    if(setopts)
      setopts(data->opts);
  }

  if(!writable)
    return;

  uint32_t ident = 0;
  pGetTargetControlIdent getident =
      (pGetTargetControlIdent)GetProcAddress(capture, "INTERNAL_GetTargetControlIdent");
  if(getident)
    getident(&ident);

  data->ident = ident;
  MemoryBarrier();
  data->targetStatus = ident != 0 ? 3 : 2;
}

static void InitProxy()
{
  if(gInitialised)
    return;

  gInitialised = true;

  SharedConfig config = OpenSharedConfig();
  ShimData *data = config.data;
  bool armed = (data != NULL && data->dllMode != 0 && data->rdocpath[0] != 0);

  // 1. the real system dxgi is always needed: our exports forward to it.
  gSystemDxgi = LoadSystemModule(L"dxgi.dll");

  // d3d12 is only preloaded when a session is actually armed. Preloading it is
  // what lets the capture dll hook the D3D12 entry points before the application
  // resolves them, but it changes module load order, so a proxy that is left in
  // place without anything armed stays as inert as possible.
  if(armed)
    gSystemD3D12 = LoadSystemModule(L"d3d12.dll");

  if(gSystemDxgi == NULL)
    PROXY_LOG("couldn't load the system dxgi.dll (%u)", GetLastError());
  if(gSystemD3D12 == NULL)
    PROXY_LOG("couldn't load the system d3d12.dll (%u)", GetLastError());

  if(gSystemDxgi)
  {
    wchar_t loadedPath[MAX_PATH] = {};
    GetModuleFileNameW(gSystemDxgi, loadedPath, MAX_PATH - 1);
    PROXY_LOG("system dxgi module: %p '%ls'%s", (void *)gSystemDxgi, loadedPath,
              gSystemDxgi == gSelfModule ? " (THIS MODULE - shadowed by name!)" : "");
  }

  // 2./3. cache the real functions and turn our exports into pass-throughs.
  // This has to happen before the capture dll is loaded so that these pointers
  // are the real code and not the capture dll's hooks.
  if(gSystemDxgi)
  {
    // the thunk space has to be writable before we fill it in
    DWORD oldProtection = 0;
    if(!VirtualProtect(g_thunkSpace, sizeof(g_thunkSpace), PAGE_EXECUTE_READWRITE, &oldProtection))
      PROXY_LOG("couldn't make the thunk space writable (%u)", GetLastError());

    for(size_t i = 0; i < PROXY_ARRAY_COUNT(kExportNames); i++)
    {
      void *realFunc = (void *)GetProcAddress(gSystemDxgi, kExportNames[i]);
      if(realFunc == NULL)
      {
        PROXY_LOG("export %s not found in the system dxgi", kExportNames[i]);
        continue;
      }

      void *thunk = MakeJumpThunk(i, realFunc);
      if(thunk == NULL)
        continue;

      bool patched = PatchExportAddress(gSelfModule, kExportNames[i], thunk);
      PROXY_LOG("export %s: real %p thunk %p patched %d now %p", kExportNames[i], realFunc, thunk,
                patched ? 1 : 0, (void *)GetProcAddress(gSelfModule, kExportNames[i]));

      if(!patched)
        PROXY_LOG("couldn't redirect export %s", kExportNames[i]);
    }
  }

  // 4. load and configure the capture dll, installing its hooks now - before the
  // application has had a chance to call any D3D function.
  gCaptureDll = LoadCaptureDll(data);

  if(gCaptureDll == NULL)
  {
    PROXY_LOG("couldn't load the capture dll (%u)", GetLastError());
    CloseSharedConfig(config);
    return;
  }

  if(data && config.writable)
  {
    data->targetStatus = 1;
    MemoryBarrier();
  }

  ConfigureCaptureDll(gCaptureDll, data, config.writable);

  CloseSharedConfig(config);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
  if(reason == DLL_PROCESS_ATTACH)
  {
    gSelfModule = hModule;
    DisableThreadLibraryCalls(hModule);
    InitProxy();
  }

  return TRUE;
}
