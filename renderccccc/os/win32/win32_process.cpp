/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2015-2026 Baldur Karlsson
 * Copyright (c) 2014 Crytek
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

// must be separate so that it's included first and not sorted by clang-format
#include <windows.h>

#include <Psapi.h>
#include <tchar.h>
#include <tlhelp32.h>
#include "common/formatting.h"
#include "core/core.h"
#include "os/os_specific.h"
#include "strings/string_utils.h"

#include <string>
#include <vector>

static rdcarray<EnvironmentModification> &GetEnvModifications()
{
  static rdcarray<EnvironmentModification> envCallbacks;
  return envCallbacks;
}

struct InsensitiveComparison
{
  bool operator()(const rdcstr &a, const rdcstr &b) const { return strlower(a) < strlower(b); }
};

typedef std::map<rdcstr, rdcstr, InsensitiveComparison> EnvMap;

static EnvMap EnvStringToEnvMap(const wchar_t *envstring)
{
  EnvMap ret;

  const wchar_t *e = envstring;

  while(*e)
  {
    const wchar_t *equals = wcschr(e, L'=');

    rdcstr name = StringFormat::Wide2UTF8(rdcwstr(e, equals - e));
    rdcstr value = StringFormat::Wide2UTF8(equals + 1);

    ret[name] = value;

    // jump to \0 and past it
    e += wcslen(e) + 1;
  }

  return ret;
}

void Process::RegisterEnvironmentModification(const EnvironmentModification &modif)
{
  GetEnvModifications().push_back(modif);
}

static void ApplyEnvModifications(EnvMap &envValues,
                                  const rdcarray<EnvironmentModification> &modifications,
                                  bool setToSystem)
{
  for(size_t i = 0; i < modifications.size(); i++)
  {
    const EnvironmentModification &m = modifications[i];

    rdcstr value;

    auto it = envValues.find(m.name);
    if(it != envValues.end())
      value = it->second;

    switch(m.mod)
    {
      case EnvMod::Set: value = m.value.c_str(); break;
      case EnvMod::Append:
      {
        if(!value.empty())
        {
          if(m.sep == EnvSep::Platform || m.sep == EnvSep::SemiColon)
            value += ";";
          else if(m.sep == EnvSep::Colon)
            value += ":";
        }
        value += m.value.c_str();
        break;
      }
      case EnvMod::Prepend:
      {
        if(!value.empty())
        {
          rdcstr prep = m.value;
          if(m.sep == EnvSep::Platform || m.sep == EnvSep::SemiColon)
            prep += ";";
          else if(m.sep == EnvSep::Colon)
            prep += ":";
          value = prep + value;
        }
        else
        {
          value = m.value.c_str();
        }
        break;
      }
    }

    envValues[m.name] = value;

    if(setToSystem)
      SetEnvironmentVariableW(StringFormat::UTF82Wide(m.name).c_str(),
                              StringFormat::UTF82Wide(value).c_str());
  }
}

// on windows we apply environment changes here, after process initialisation
// but before any real work (in Renderccccc::Initialise) so that we support
// injecting the dll into processes we didn't launch (ie didn't control the
// starting environment for), or even the application loading the dll itself
// without any interaction with our replay app.
void Process::ApplyEnvironmentModification()
{
  // turn environment string to a UTF-8 map
  LPWCH envStrings = GetEnvironmentStringsW();
  EnvMap envValues = EnvStringToEnvMap(envStrings);
  FreeEnvironmentStringsW(envStrings);
  rdcarray<EnvironmentModification> &modifications = GetEnvModifications();

  ApplyEnvModifications(envValues, modifications, true);

  // these have been applied to the current process
  modifications.clear();
}

rdcstr Process::GetEnvVariable(const rdcstr &name)
{
  DWORD len = GetEnvironmentVariableA(name.c_str(), NULL, 0);
  if(len == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND)
    return rdcstr();

  rdcstr ret;
  ret.resize(len + 1);

  GetEnvironmentVariableA(name.c_str(), ret.data(), len);
  ret.trim();
  return ret;
}

uint64_t Process::GetMemoryUsage()
{
  HANDLE proc = GetCurrentProcess();

  if(proc == NULL)
  {
    RDCERR("Couldn't open process: %d", GetLastError());
    return 0;
  }

  PROCESS_MEMORY_COUNTERS memInfo = {};

  uint64_t ret = 0;

  if(GetProcessMemoryInfo(proc, &memInfo, sizeof(memInfo)))
  {
    ret = memInfo.WorkingSetSize;
  }
  else
  {
    RDCERR("Couldn't get process memory info: %d", GetLastError());
  }

  return ret;
}

// helpers for various shims and dlls etc, not part of the public API
extern "C" __declspec(dllexport) void __cdecl INTERNAL_GetTargetControlIdent(uint32_t *ident)
{
  if(ident)
    *ident = Renderccccc::Inst().GetTargetControlIdent();
}

extern "C" __declspec(dllexport) void __cdecl INTERNAL_SetCaptureOptions(CaptureOptions *opts)
{
  if(opts)
    Renderccccc::Inst().SetCaptureOptions(*opts);
}

extern "C" __declspec(dllexport) void __cdecl INTERNAL_SetCaptureFile(const char *capfile)
{
  if(capfile)
    Renderccccc::Inst().SetCaptureFileTemplate(capfile);
}

extern "C" __declspec(dllexport) void __cdecl INTERNAL_SetDebugLogFile(const char *logfile)
{
  RENDERCCCCC_SetDebugLogFile(logfile ? logfile : rdcstr());
}

static EnvironmentModification tempEnvMod;

extern "C" __declspec(dllexport) void __cdecl INTERNAL_EnvModName(const char *name)
{
  if(name)
    tempEnvMod.name = name;
}

extern "C" __declspec(dllexport) void __cdecl INTERNAL_EnvModValue(const char *value)
{
  if(value)
    tempEnvMod.value = value;
}

extern "C" __declspec(dllexport) void __cdecl INTERNAL_EnvSep(EnvSep *sep)
{
  if(sep)
    tempEnvMod.sep = *sep;
}

extern "C" __declspec(dllexport) void __cdecl INTERNAL_EnvMod(EnvMod *mod)
{
  if(mod)
  {
    tempEnvMod.mod = *mod;
    Process::RegisterEnvironmentModification(tempEnvMod);
  }
}

extern "C" __declspec(dllexport) void __cdecl INTERNAL_ApplyEnvMods(void *ignored)
{
  Process::ApplyEnvironmentModification();
}

void InjectDLL(HANDLE hProcess, rdcwstr libName)
{
  wchar_t dllPath[MAX_PATH + 1] = {0};
  if(wcscpy_s(dllPath, libName.c_str()) != 0)
  {
    RDCERR("DLL path is too long to inject: '%ls'", libName.c_str());
    return;
  }

  static HMODULE kernel32 = GetModuleHandleA("kernel32.dll");

  if(kernel32 == NULL)
  {
    RDCERR("Couldn't get handle for kernel32.dll");
    return;
  }

  FARPROC loadLibrary = GetProcAddress(kernel32, "LoadLibraryW");
  if(loadLibrary == NULL)
  {
    RDCERR("Couldn't find LoadLibraryW: %u", GetLastError());
    return;
  }

  void *remoteMem =
      VirtualAllocEx(hProcess, NULL, sizeof(dllPath), MEM_COMMIT | MEM_RESERVE,
                     PAGE_EXECUTE_READWRITE);
  if(remoteMem == NULL)
  {
    RDCERR("Couldn't allocate remote memory for DLL '%ls': %u", libName.c_str(), GetLastError());
    return;
  }

  SIZE_T bytesWritten = 0;
  bool safeToFree = true;
  if(!WriteProcessMemory(hProcess, remoteMem, dllPath, sizeof(dllPath), &bytesWritten) ||
     bytesWritten != sizeof(dllPath))
  {
    DWORD err = GetLastError();
    if(err == ERROR_SUCCESS)
      err = ERROR_PARTIAL_COPY;
    RDCERR("Couldn't write remote memory %p with dllPath '%ls': %u", remoteMem, dllPath, err);
  }
  else
  {
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 1024 * 1024U,
                                        (LPTHREAD_START_ROUTINE)loadLibrary, remoteMem, 0, NULL);
    if(hThread == NULL)
    {
      RDCERR("Couldn't create remote thread for LoadLibraryW: %u", GetLastError());
    }
    else
    {
      DWORD waitResult = WaitForSingleObject(hThread, INFINITE);
      if(waitResult != WAIT_OBJECT_0)
      {
        DWORD err = waitResult == WAIT_FAILED ? GetLastError() : ERROR_GEN_FAILURE;
        RDCERR("Couldn't wait for remote LoadLibraryW thread: %u", err);
        // The thread may still be reading the path.
        safeToFree = false;
      }

      if(!CloseHandle(hThread))
        RDCERR("Couldn't close remote LoadLibraryW thread handle: %u", GetLastError());
    }
  }

  if(safeToFree && !VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE))
  {
    RDCERR("Couldn't free remote memory for DLL '%ls': %u", libName.c_str(), GetLastError());
  }
}

uintptr_t FindRemoteDLL(DWORD pid, rdcstr libName)
{
  HANDLE hModuleSnap = INVALID_HANDLE_VALUE;

  rdcwstr wlibName = StringFormat::UTF82Wide(strlower(libName));

  // up to 10 retries
  for(int i = 0; i < 10; i++)
  {
    hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);

    if(hModuleSnap == INVALID_HANDLE_VALUE)
    {
      DWORD err = GetLastError();

      RDCWARN("CreateToolhelp32Snapshot(%u) -> 0x%08x", pid, err);

      // retry if error is ERROR_BAD_LENGTH
      if(err == ERROR_BAD_LENGTH)
        continue;
    }

    // didn't retry, or succeeded
    break;
  }

  if(hModuleSnap == INVALID_HANDLE_VALUE)
  {
    RDCERR("Couldn't create toolhelp dump of modules in process %u", pid);
    return 0;
  }

  MODULEENTRY32 me32;
  RDCEraseEl(me32);
  me32.dwSize = sizeof(MODULEENTRY32);

  BOOL success = Module32First(hModuleSnap, &me32);

  if(success == FALSE)
  {
    DWORD err = GetLastError();

    RDCERR("Couldn't get first module in process %u: 0x%08x", pid, err);
    CloseHandle(hModuleSnap);
    return 0;
  }

  uintptr_t ret = 0;

  int numModules = 0;

  do
  {
    wchar_t modnameLower[MAX_MODULE_NAME32 + 1];
    RDCEraseEl(modnameLower);
    wcsncpy_s(modnameLower, me32.szModule, MAX_MODULE_NAME32);

    wchar_t *wc = &modnameLower[0];
    while(*wc)
    {
      *wc = towlower(*wc);
      wc++;
    }

    numModules++;

    if(wcsstr(modnameLower, wlibName.c_str()) == modnameLower)
    {
      ret = (uintptr_t)me32.modBaseAddr;
    }
  } while(ret == 0 && Module32Next(hModuleSnap, &me32));

  if(ret == 0)
  {
    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);

    DWORD exitCode = 0;

    if(h)
      GetExitCodeProcess(h, &exitCode);

    if(h == NULL || exitCode != STILL_ACTIVE)
    {
      RDCERR(
          "Error injecting into remote process with PID %u which is no longer available.\n"
          "Possibly the process has crashed during early startup, or is missing DLLs to run?",
          pid);
    }
    else
    {
      RDCERR("Couldn't find module '%s' among %d modules", libName.c_str(), numModules);
    }

    if(h)
      CloseHandle(h);
  }

  CloseHandle(hModuleSnap);

  return ret;
}

bool InjectFunctionCall(HANDLE hProcess, uintptr_t renderccccc_remote, const char *funcName,
                        void *data, const size_t dataLen, bool *remoteThreadMayBeRunning);

static uintptr_t GetRemoteFunctionAddress(DWORD pid, const char *moduleName,
                                          const char *functionName)
{
  HMODULE localModule = GetModuleHandleA(moduleName);
  if(localModule == NULL)
  {
    RDCERR("Couldn't get local module %s: %u", moduleName, GetLastError());
    return 0;
  }

  FARPROC localFunction = GetProcAddress(localModule, functionName);
  if(localFunction == NULL)
  {
    RDCERR("Couldn't find %s in %s: %u", functionName, moduleName, GetLastError());
    return 0;
  }

  // GetProcAddress can return an address in a forwarded-to module, so resolve the
  // module which actually owns the function and apply its remote relocation.
  HMODULE owner = NULL;
  if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                             GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         (LPCWSTR)localFunction, &owner))
  {
    RDCERR("Couldn't find the module containing %s: %u", functionName, GetLastError());
    return 0;
  }

  wchar_t ownerPath[MAX_PATH] = {};
  DWORD pathLength = GetModuleFileNameW(owner, ownerPath, MAX_PATH);
  if(pathLength == 0 || pathLength >= MAX_PATH)
  {
    RDCERR("Couldn't get the module path containing %s: %u", functionName, GetLastError());
    return 0;
  }

  const wchar_t *ownerName = wcsrchr(ownerPath, L'\\');
  ownerName = ownerName ? ownerName + 1 : ownerPath;
  uintptr_t remoteModule = FindRemoteDLL(pid, StringFormat::Wide2UTF8(ownerName));
  if(remoteModule == 0)
    return 0;

  uintptr_t localBase = (uintptr_t)owner;
  uintptr_t localAddress = (uintptr_t)localFunction;
  if(localAddress < localBase)
  {
    RDCERR("Resolved function %s is outside module %ls", functionName, ownerPath);
    return 0;
  }

  return remoteModule + (localAddress - localBase);
}

static void EmitByte(std::vector<uint8_t> &code, uint8_t byte)
{
  code.push_back(byte);
}

static void EmitAddress32(std::vector<uint8_t> &code, uintptr_t address)
{
  uint32_t value = (uint32_t)address;
  for(size_t i = 0; i < sizeof(value); i++)
    EmitByte(code, (uint8_t)(value >> (i * 8)));
}

#if defined(_WIN64)
static void EmitAddress64(std::vector<uint8_t> &code, uintptr_t address)
{
  uint64_t value = (uint64_t)address;
  for(size_t i = 0; i < sizeof(value); i++)
    EmitByte(code, (uint8_t)(value >> (i * 8)));
}
#endif

static bool PatchRelative8(std::vector<uint8_t> &code, size_t displacementOffset,
                           size_t targetOffset)
{
  intptr_t displacement = (intptr_t)targetOffset - (intptr_t)(displacementOffset + 1);
  if(displacement < -128 || displacement > 127)
    return false;

  code[displacementOffset] = (uint8_t)(int8_t)displacement;
  return true;
}

static bool BuildLoadLibraryTrampoline(uintptr_t pathAddress, uintptr_t statusAddress,
                                       uintptr_t stackAddress, uintptr_t loadLibraryAddress,
                                       uintptr_t sleepAddress, std::vector<uint8_t> &code)
{
  code.clear();

#if defined(_WIN64)
  // Use a private stack so the initial loader context's stack is never touched.
  // The caller's original CONTEXT is restored by the host after this thread parks.
  EmitByte(code, 0x48); EmitByte(code, 0xBC); EmitAddress64(code, stackAddress); // mov rsp, imm64
  EmitByte(code, 0x48); EmitByte(code, 0xB9); EmitAddress64(code, pathAddress); // mov rcx, imm64
  EmitByte(code, 0x48); EmitByte(code, 0xB8); EmitAddress64(code, loadLibraryAddress); // mov rax
  EmitByte(code, 0x48); EmitByte(code, 0x83); EmitByte(code, 0xEC); EmitByte(code, 0x28);
  EmitByte(code, 0xFF); EmitByte(code, 0xD0); // call rax
  EmitByte(code, 0x48); EmitByte(code, 0x83); EmitByte(code, 0xC4); EmitByte(code, 0x28);
  EmitByte(code, 0x48); EmitByte(code, 0x85); EmitByte(code, 0xC0); // test rax, rax
  EmitByte(code, 0x74); // jz failed
  size_t failedDisplacement = code.size();
  EmitByte(code, 0);

  EmitByte(code, 0x49); EmitByte(code, 0xBB); EmitAddress64(code, statusAddress); // mov r11
  EmitByte(code, 0x41); EmitByte(code, 0xC7); EmitByte(code, 0x03);
  EmitAddress32(code, 1); // mov dword ptr [r11], 1
  EmitByte(code, 0xEB); // jmp park
  size_t parkDisplacement = code.size();
  EmitByte(code, 0);

  size_t failedOffset = code.size();
  EmitByte(code, 0x49); EmitByte(code, 0xBB); EmitAddress64(code, statusAddress);
  EmitByte(code, 0x41); EmitByte(code, 0xC7); EmitByte(code, 0x03);
  EmitAddress32(code, 2); // LoadLibraryW failed

  size_t parkOffset = code.size();
  size_t sleepLoop = code.size();
  EmitByte(code, 0x48); EmitByte(code, 0xB8); EmitAddress64(code, sleepAddress);
  EmitByte(code, 0xB9); EmitAddress32(code, 1); // mov ecx, 1
  EmitByte(code, 0x48); EmitByte(code, 0x83); EmitByte(code, 0xEC); EmitByte(code, 0x28);
  EmitByte(code, 0xFF); EmitByte(code, 0xD0); // call rax
  EmitByte(code, 0x48); EmitByte(code, 0x83); EmitByte(code, 0xC4); EmitByte(code, 0x28);
  EmitByte(code, 0xEB);
  size_t loopDisplacement = code.size();
  EmitByte(code, 0);

  return PatchRelative8(code, failedDisplacement, failedOffset) &&
         PatchRelative8(code, parkDisplacement, parkOffset) &&
         PatchRelative8(code, loopDisplacement, sleepLoop);
#else
  // LoadLibraryW and Sleep are WINAPI/stdcall functions on x86.
  EmitByte(code, 0x68); EmitAddress32(code, pathAddress); // push path
  EmitByte(code, 0xB8); EmitAddress32(code, loadLibraryAddress); // mov eax, LoadLibraryW
  EmitByte(code, 0xFF); EmitByte(code, 0xD0); // call eax
  EmitByte(code, 0x85); EmitByte(code, 0xC0); // test eax, eax
  EmitByte(code, 0x74); // jz failed
  size_t failedDisplacement = code.size();
  EmitByte(code, 0);

  EmitByte(code, 0xC7); EmitByte(code, 0x05); EmitAddress32(code, statusAddress);
  EmitAddress32(code, 1); // mov dword ptr [status], 1
  EmitByte(code, 0xEB); // jmp park
  size_t parkDisplacement = code.size();
  EmitByte(code, 0);

  size_t failedOffset = code.size();
  EmitByte(code, 0xC7); EmitByte(code, 0x05); EmitAddress32(code, statusAddress);
  EmitAddress32(code, 2); // LoadLibraryW failed

  size_t parkOffset = code.size();
  size_t sleepLoop = code.size();
  EmitByte(code, 0x6A); EmitByte(code, 0x01); // push 1
  EmitByte(code, 0xB8); EmitAddress32(code, sleepAddress);
  EmitByte(code, 0xFF); EmitByte(code, 0xD0); // call eax
  EmitByte(code, 0xEB);
  size_t loopDisplacement = code.size();
  EmitByte(code, 0);

  return PatchRelative8(code, failedDisplacement, failedOffset) &&
         PatchRelative8(code, parkDisplacement, parkOffset) &&
         PatchRelative8(code, loopDisplacement, sleepLoop);
#endif
}

static rdcpair<RDResult, uint32_t> ConfigureInjectedProcess(
    HANDLE hProcess, DWORD pid, const rdcstr &capturefile, const CaptureOptions &opts,
    const rdcarray<EnvironmentModification> &env, bool reportConfigurationFailure,
    bool *remoteThreadMayBeRunning = NULL)
{
  if(remoteThreadMayBeRunning)
    *remoteThreadMayBeRunning = false;

  const char *rdoc_dll = STRINGIZE(RDOC_BASE_NAME);
  uintptr_t loc = FindRemoteDLL(pid, STRINGIZE(RDOC_BASE_NAME) ".dll");

  rdcpair<RDResult, uint32_t> result = {ResultCode::Succeeded, 0};

  if(loc == 0)
  {
    const char *largeAddrMsg =
#if ENABLED(RDOC_X64)
        ", and is not built with /LARGEADDRESSAWARE:NO";
#else
        "";
#endif

    SET_ERROR_RESULT(
        result.first, ResultCode::InjectionFailed,
        "Failed to inject %s.dll into process. Check that the process did not crash or exit "
        "early in initialisation, e.g. if the working directory is incorrectly set.\n"
        "Otherwise ensure the program does not have significant "
        "code that will run on global constructors/thread start and crash%s",
        rdoc_dll, largeAddrMsg);
    return result;
  }

  bool configurationSucceeded = true;
  bool configurationThreadMayBeRunning = false;
  auto InjectConfigurationCall = [&](const char *funcName, void *data, size_t dataLen) {
    if(reportConfigurationFailure && configurationThreadMayBeRunning)
    {
      configurationSucceeded = false;
      return;
    }

    bool callMayBeRunning = false;
    bool success = InjectFunctionCall(hProcess, loc, funcName, data, dataLen, &callMayBeRunning);
    configurationThreadMayBeRunning = configurationThreadMayBeRunning || callMayBeRunning;
    configurationSucceeded = configurationSucceeded && success;
  };

  // safe to cast away the const as we know these functions don't modify the parameters
  if(!capturefile.empty())
    InjectConfigurationCall("INTERNAL_SetCaptureFile", (void *)capturefile.c_str(),
                            capturefile.size() + 1);

  rdcstr debugLogfile = RDCGETLOGFILE();
  InjectConfigurationCall("INTERNAL_SetDebugLogFile", (void *)debugLogfile.c_str(),
                          debugLogfile.size() + 1);

  InjectConfigurationCall("INTERNAL_SetCaptureOptions", (CaptureOptions *)&opts,
                          sizeof(CaptureOptions));

  InjectConfigurationCall("INTERNAL_GetTargetControlIdent", &result.second,
                          sizeof(result.second));

  if(!env.empty())
  {
    for(const EnvironmentModification &e : env)
    {
      rdcstr name = e.name.trimmed();
      rdcstr value = e.value;
      EnvMod mod = e.mod;
      EnvSep sep = e.sep;

      if(name == "")
        break;

      InjectConfigurationCall("INTERNAL_EnvModName", (void *)name.c_str(), name.size() + 1);
      InjectConfigurationCall("INTERNAL_EnvModValue", (void *)value.c_str(), value.size() + 1);
      InjectConfigurationCall("INTERNAL_EnvSep", &sep, sizeof(sep));
      InjectConfigurationCall("INTERNAL_EnvMod", &mod, sizeof(mod));
    }

    // parameter is unused
    void *dummy = NULL;
    InjectConfigurationCall("INTERNAL_ApplyEnvMods", &dummy, sizeof(dummy));
  }

  // Attach callers historically reported success once the DLL was present, even if
  // one of these optional configuration calls failed. Keep that result contract.
  if(reportConfigurationFailure && !configurationSucceeded)
  {
    SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                     "Failed to configure %s.dll in process %u", rdoc_dll, pid);
  }

  if(remoteThreadMayBeRunning)
    *remoteThreadMayBeRunning = configurationThreadMayBeRunning;

  return result;
}

bool InjectFunctionCall(HANDLE hProcess, uintptr_t renderccccc_remote, const char *funcName,
                        void *data, const size_t dataLen, bool *remoteThreadMayBeRunning)
{
  if(remoteThreadMayBeRunning)
    *remoteThreadMayBeRunning = false;

  if(dataLen == 0)
  {
    RDCERR("Invalid function call injection attempt");
    return false;
  }

  RDCDEBUG("Injecting call to %s", funcName);

  HMODULE renderccccc_local = GetModuleHandleA(STRINGIZE(RDOC_BASE_NAME) ".dll");

  if(renderccccc_local == NULL)
  {
    RDCERR("Couldn't get handle for " STRINGIZE(RDOC_BASE_NAME) ".dll: %u", GetLastError());
    return false;
  }

  uintptr_t func_local = (uintptr_t)GetProcAddress(renderccccc_local, funcName);

  if(func_local == 0)
  {
    RDCERR("Couldn't find export %s in " STRINGIZE(RDOC_BASE_NAME) ".dll: %u", funcName,
           GetLastError());
    return false;
  }

  // we've found SetCaptureOptions in our local instance of the module, now calculate the offset and
  // so get the function
  // in the remote module (which might be loaded at a different base address
  uintptr_t func_remote = func_local + renderccccc_remote - (uintptr_t)renderccccc_local;

  void *remoteMem = VirtualAllocEx(hProcess, NULL, dataLen, MEM_COMMIT | MEM_RESERVE,
                                   PAGE_EXECUTE_READWRITE);
  if(remoteMem == NULL)
  {
    RDCERR("Couldn't allocate remote memory for %s: %u", funcName, GetLastError());
    return false;
  }

  SIZE_T numWritten = 0;
  if(!WriteProcessMemory(hProcess, remoteMem, data, dataLen, &numWritten) || numWritten != dataLen)
  {
    DWORD err = GetLastError();
    RDCERR("Couldn't write remote memory for %s: %u", funcName, err);
    if(!VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE))
      RDCERR("Couldn't free remote memory for %s: %u", funcName, GetLastError());
    return false;
  }

  HANDLE hThread =
      CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)func_remote, remoteMem, 0, NULL);
  if(hThread == NULL)
  {
    RDCERR("Couldn't create remote thread for %s: %u", funcName, GetLastError());
    if(!VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE))
      RDCERR("Couldn't free remote memory for %s: %u", funcName, GetLastError());
    return false;
  }

  DWORD waitResult = WaitForSingleObject(hThread, INFINITE);
  if(waitResult != WAIT_OBJECT_0)
  {
    DWORD err = waitResult == WAIT_FAILED ? GetLastError() : ERROR_GEN_FAILURE;
    RDCERR("Couldn't wait for remote call to %s: %u", funcName, err);
    if(remoteThreadMayBeRunning)
      *remoteThreadMayBeRunning = true;
    if(!CloseHandle(hThread))
      RDCERR("Couldn't close remote thread handle for %s: %u", funcName, GetLastError());
    // The remote thread may still be using this allocation.
    return false;
  }

  SIZE_T numRead = 0;
  bool success = true;
  if(!ReadProcessMemory(hProcess, remoteMem, data, dataLen, &numRead) || numRead != dataLen)
  {
    DWORD err = GetLastError();
    RDCERR("Couldn't read remote memory after %s: %u", funcName, err);
    success = false;
  }

  if(!CloseHandle(hThread))
  {
    RDCERR("Couldn't close remote thread handle for %s: %u", funcName, GetLastError());
    success = false;
  }

  if(!VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE))
  {
    RDCERR("Couldn't free remote memory after %s: %u", funcName, GetLastError());
    success = false;
  }

  return success;
}

static void TerminateLaunchedProcess(HANDLE hProcess, const char *reason)
{
  RDCERR("Terminating newly launched process because %s", reason);

  if(!TerminateProcess(hProcess, ERROR_PROCESS_ABORTED))
  {
    DWORD err = GetLastError();
    DWORD waitResult = WaitForSingleObject(hProcess, 0);
    if(waitResult != WAIT_OBJECT_0)
    {
      RDCERR("Couldn't terminate newly launched process after %s: %u", reason, err);
      if(waitResult == WAIT_FAILED)
        RDCERR("Couldn't check whether the process exited after termination failed: %u",
               GetLastError());
    }
    return;
  }

  DWORD waitResult = WaitForSingleObject(hProcess, INFINITE);
  if(waitResult != WAIT_OBJECT_0)
  {
    DWORD err = waitResult == WAIT_FAILED ? GetLastError() : ERROR_GEN_FAILURE;
    RDCERR("Couldn't wait for terminated process after %s: %u", reason, err);
  }
}

static rdcpair<RDResult, uint32_t> InjectIntoSuspendedPrimaryThread(
    HANDLE hProcess, HANDLE hThread, DWORD pid, rdcwstr dllPath, const rdcstr &capturefile,
    const CaptureOptions &opts, const rdcarray<EnvironmentModification> &env)
{
  rdcpair<RDResult, uint32_t> result = {ResultCode::Succeeded, 0};

  CONTEXT original = {};
  original.ContextFlags = CONTEXT_ALL;
  if(!GetThreadContext(hThread, &original))
  {
    DWORD err = GetLastError();
    SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                     "GetThreadContext failed for process %u (Win32 error 0x%08x)", pid, err);

    DWORD resumeCount = ResumeThread(hThread);
    if(resumeCount != 1)
    {
      RDCERR("Couldn't resume process %u after GetThreadContext failed: %u", pid,
             resumeCount == DWORD(-1) ? GetLastError() : ERROR_INVALID_STATE);
      TerminateLaunchedProcess(hProcess, "the initial thread context could not be read or resumed");
    }

    return result;
  }

  uintptr_t loadLibraryAddress = GetRemoteFunctionAddress(pid, "kernel32.dll", "LoadLibraryW");
  uintptr_t sleepAddress = GetRemoteFunctionAddress(pid, "kernel32.dll", "Sleep");
  if(loadLibraryAddress == 0 || sleepAddress == 0)
  {
    SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                     "Couldn't resolve remote loader functions for process %u", pid);
    if(!SetThreadContext(hThread, &original))
    {
      RDCERR("Couldn't restore initial thread context for process %u: %u", pid, GetLastError());
      TerminateLaunchedProcess(hProcess, "the initial thread context could not be restored");
      return result;
    }

    DWORD resumeCount = ResumeThread(hThread);
    if(resumeCount != 1)
    {
      RDCERR("Couldn't resume process %u after loader function resolution failed: %u", pid,
             resumeCount == DWORD(-1) ? GetLastError() : ERROR_INVALID_STATE);
      TerminateLaunchedProcess(hProcess, "the initial thread could not be resumed");
    }
    return result;
  }

  constexpr SIZE_T codeOffset = 0;
  constexpr SIZE_T codeCapacity = 512;
  constexpr SIZE_T statusOffset = codeCapacity;
  constexpr SIZE_T pathOffset = statusOffset + sizeof(LONG);
  constexpr SIZE_T stackSize = 64 * 1024;
  const SIZE_T pathBytes = (wcslen(dllPath.c_str()) + 1) * sizeof(wchar_t);
  const SIZE_T stackOffset = (pathOffset + pathBytes + 15) & ~SIZE_T(15);
  const SIZE_T allocationSize = stackOffset + stackSize;

  void *remoteMem = VirtualAllocEx(hProcess, NULL, allocationSize, MEM_COMMIT | MEM_RESERVE,
                                   PAGE_EXECUTE_READWRITE);
  if(remoteMem == NULL)
  {
    DWORD err = GetLastError();
    SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                     "VirtualAllocEx failed for process %u (Win32 error 0x%08x)", pid, err);

    if(!SetThreadContext(hThread, &original))
    {
      RDCERR("Couldn't restore initial thread context for process %u: %u", pid, GetLastError());
      TerminateLaunchedProcess(hProcess, "the initial thread context could not be restored");
      return result;
    }

    DWORD resumeCount = ResumeThread(hThread);
    if(resumeCount != 1)
    {
      RDCERR("Couldn't resume process %u after remote allocation failed: %u", pid,
             resumeCount == DWORD(-1) ? GetLastError() : ERROR_INVALID_STATE);
      TerminateLaunchedProcess(hProcess, "the initial thread could not be resumed");
    }
    return result;
  }

  uintptr_t remoteBase = (uintptr_t)remoteMem;
  uintptr_t remoteStatus = remoteBase + statusOffset;
  uintptr_t remotePath = remoteBase + pathOffset;
#if defined(_WIN64)
  uintptr_t remoteStack = remoteBase + allocationSize - 8;
#else
  uintptr_t remoteStack = remoteBase + allocationSize - 16;
#endif

  std::vector<uint8_t> trampoline;
  bool trampolineBuilt = BuildLoadLibraryTrampoline(remotePath, remoteStatus, remoteStack,
                                                   loadLibraryAddress, sleepAddress, trampoline) &&
                         trampoline.size() <= codeCapacity;
  if(!trampolineBuilt)
  {
    SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                     "Couldn't build the remote loader trampoline for process %u", pid);
  }

  LONG initialStatus = 0;
  SIZE_T bytesWritten = 0;
  bool writeSucceeded = trampolineBuilt;
  if(writeSucceeded)
  {
    writeSucceeded = WriteProcessMemory(hProcess, (void *)(remoteBase + codeOffset),
                                       trampoline.data(), trampoline.size(), &bytesWritten) &&
                     bytesWritten == trampoline.size();
  }
  if(writeSucceeded)
  {
    bytesWritten = 0;
    writeSucceeded = WriteProcessMemory(hProcess, (void *)remoteStatus, &initialStatus,
                                       sizeof(initialStatus), &bytesWritten) &&
                     bytesWritten == sizeof(initialStatus);
  }
  if(writeSucceeded)
  {
    bytesWritten = 0;
    writeSucceeded = WriteProcessMemory(hProcess, (void *)remotePath, dllPath.c_str(), pathBytes,
                                       &bytesWritten) &&
                     bytesWritten == pathBytes;
  }

  if(writeSucceeded && !FlushInstructionCache(hProcess, remoteMem, trampoline.size()))
  {
    DWORD err = GetLastError();
    SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                     "FlushInstructionCache failed for process %u (Win32 error 0x%08x)", pid,
                     err);
    writeSucceeded = false;
  }

  if(!writeSucceeded)
  {
    DWORD err = GetLastError();
    if(err == ERROR_SUCCESS)
      err = ERROR_PARTIAL_COPY;
    if(result.first == ResultCode::Succeeded)
      SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                       "WriteProcessMemory failed for process %u (Win32 error 0x%08x)", pid,
                       err);

    if(!SetThreadContext(hThread, &original))
    {
      RDCERR("Couldn't restore initial thread context for process %u: %u", pid, GetLastError());
      TerminateLaunchedProcess(hProcess, "the initial thread context could not be restored");
    }
    else
    {
      if(!VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE))
        RDCERR("Couldn't free remote trampoline for process %u: %u", pid, GetLastError());

      DWORD resumeCount = ResumeThread(hThread);
      if(resumeCount != 1)
      {
        RDCERR("Couldn't resume process %u after remote write failed: %u", pid,
               resumeCount == DWORD(-1) ? GetLastError() : ERROR_INVALID_STATE);
        TerminateLaunchedProcess(hProcess, "the initial thread could not be resumed");
      }
    }
    return result;
  }

  CONTEXT bootstrap = original;
#if defined(_WIN64)
  bootstrap.Rip = (DWORD64)remoteBase;
  bootstrap.Rsp = (DWORD64)remoteStack;
#else
  bootstrap.Eip = (DWORD)remoteBase;
  bootstrap.Esp = (DWORD)remoteStack;
#endif

  if(!SetThreadContext(hThread, &bootstrap))
  {
    DWORD err = GetLastError();
    SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                     "SetThreadContext failed for process %u (Win32 error 0x%08x)", pid, err);

    if(!SetThreadContext(hThread, &original))
    {
      RDCERR("Couldn't restore initial thread context for process %u: %u", pid, GetLastError());
      TerminateLaunchedProcess(hProcess, "the initial thread context could not be restored");
    }
    else
    {
      if(!VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE))
        RDCERR("Couldn't free remote trampoline for process %u: %u", pid, GetLastError());

      DWORD resumeCount = ResumeThread(hThread);
      if(resumeCount != 1)
      {
        RDCERR("Couldn't resume process %u after SetThreadContext failed: %u", pid,
               resumeCount == DWORD(-1) ? GetLastError() : ERROR_INVALID_STATE);
        TerminateLaunchedProcess(hProcess, "the initial thread could not be resumed");
      }
    }
    return result;
  }

  DWORD resumeCount = ResumeThread(hThread);
  if(resumeCount != 1)
  {
    DWORD err = resumeCount == DWORD(-1) ? GetLastError() : ERROR_INVALID_STATE;
    SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                     "ResumeThread failed while starting injection into process %u "
                     "(Win32 error 0x%08x)",
                     pid, err);

    if(resumeCount == 0)
    {
      // The thread was already running when the initial resume was attempted.
      // Its state is no longer known to this launcher, so terminate the child.
      TerminateLaunchedProcess(hProcess, "the primary thread was already running before bootstrap");
      return result;
    }

    if(!SetThreadContext(hThread, &original))
    {
      RDCERR("Couldn't restore initial thread context for process %u: %u", pid, GetLastError());
      TerminateLaunchedProcess(hProcess, "the initial thread context could not be restored");
    }
    else
    {
      if(!VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE))
        RDCERR("Couldn't free remote trampoline for process %u: %u", pid, GetLastError());

      // ResumeThread failed without changing the suspend count, so the original
      // CREATE_SUSPENDED hold is still present.
      DWORD restoreResumeCount = ResumeThread(hThread);
      if(restoreResumeCount != 1)
      {
        RDCERR("Couldn't resume process %u after initial ResumeThread failed: %u", pid,
               restoreResumeCount == DWORD(-1) ? GetLastError() : ERROR_INVALID_STATE);
        TerminateLaunchedProcess(hProcess, "the initial thread could not be resumed");
      }
    }
    return result;
  }

  auto restoreRunningPrimaryThread = [&]() {
    DWORD suspendCount = SuspendThread(hThread);
    if(suspendCount == DWORD(-1))
    {
      DWORD err = GetLastError();
      RDCERR("SuspendThread failed for process %u: %u", pid, err);
      TerminateLaunchedProcess(hProcess, "the bootstrap thread could not be suspended for restore");
      return false;
    }

    if(suspendCount != 0)
      RDCWARN("Unexpected suspend count %u while restoring process %u", suspendCount, pid);

    if(!SetThreadContext(hThread, &original))
    {
      DWORD err = GetLastError();
      RDCERR("SetThreadContext could not restore process %u: %u", pid, err);
      TerminateLaunchedProcess(hProcess, "the original primary thread context could not be restored");
      return false;
    }

    bool cleanupSucceeded = true;
    if(!VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE))
    {
      DWORD err = GetLastError();
      RDCERR("Couldn't free remote trampoline for process %u: %u", pid, err);
      cleanupSucceeded = false;
    }
    else
    {
      remoteMem = NULL;
    }

    DWORD restoreResumeCount = ResumeThread(hThread);
    if(restoreResumeCount != 1)
    {
      DWORD err = restoreResumeCount == DWORD(-1) ? GetLastError() : ERROR_INVALID_STATE;
      RDCERR("Couldn't resume restored primary thread in process %u: %u", pid, err);
      TerminateLaunchedProcess(hProcess, "the restored primary thread could not be resumed");
      return false;
    }

    return cleanupSucceeded;
  };

  LONG loadStatus = 0;
  while(true)
  {
    DWORD processWait = WaitForSingleObject(hProcess, 0);
    if(processWait == WAIT_OBJECT_0)
    {
      SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                       "Process %u exited before the injected DLL finished loading", pid);
      // The operating system has released the remote allocation with the process.
      return result;
    }
    if(processWait == WAIT_FAILED)
    {
      DWORD err = GetLastError();
      SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                       "Couldn't check process %u while waiting for DLL load (Win32 error 0x%08x)",
                       pid, err);
      restoreRunningPrimaryThread();
      return result;
    }

    DWORD threadWait = WaitForSingleObject(hThread, 0);
    if(threadWait == WAIT_OBJECT_0)
    {
      SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                       "Primary thread in process %u exited before DLL load completed", pid);
      TerminateLaunchedProcess(hProcess, "the bootstrap primary thread exited before signaling load completion");
      return result;
    }
    if(threadWait == WAIT_FAILED)
    {
      DWORD err = GetLastError();
      SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                       "Couldn't check primary thread in process %u while waiting for DLL load "
                       "(Win32 error 0x%08x)",
                       pid, err);
      TerminateLaunchedProcess(hProcess, "the bootstrap thread state could not be checked");
      return result;
    }

    SIZE_T bytesRead = 0;
    if(!ReadProcessMemory(hProcess, (void *)remoteStatus, &loadStatus, sizeof(loadStatus),
                          &bytesRead) ||
       bytesRead != sizeof(loadStatus))
    {
      DWORD err = GetLastError();
      SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                       "ReadProcessMemory failed while waiting for DLL load in process %u "
                       "(Win32 error 0x%08x)",
                       pid, err);
      restoreRunningPrimaryThread();
      return result;
    }

    if(loadStatus != 0)
      break;

    Sleep(10);
  }

  if(loadStatus != 1)
  {
    SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                     "LoadLibraryW failed to load %ls into process %u", dllPath.c_str(), pid);
    restoreRunningPrimaryThread();
    return result;
  }

  bool configurationThreadMayBeRunning = false;
  result = ConfigureInjectedProcess(hProcess, pid, capturefile, opts, env, true,
                                    &configurationThreadMayBeRunning);
  if(configurationThreadMayBeRunning)
  {
    TerminateLaunchedProcess(hProcess, "a remote configuration call did not finish");
    return result;
  }

  if(!restoreRunningPrimaryThread() && result.first == ResultCode::Succeeded)
  {
    SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                     "Couldn't restore the primary thread in process %u after injection", pid);
  }

  return result;
}

static PROCESS_INFORMATION RunProcess(const rdcstr &app, const rdcstr &workingDir,
                                      const rdcstr &cmdLine,
                                      const rdcarray<EnvironmentModification> &env, bool internal,
                                      HANDLE *phChildStdOutput_Rd, HANDLE *phChildStdError_Rd)
{
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  SECURITY_ATTRIBUTES pSec;
  SECURITY_ATTRIBUTES tSec;

  RDCEraseEl(pi);
  RDCEraseEl(si);
  RDCEraseEl(pSec);
  RDCEraseEl(tSec);

  si.cb = sizeof(si);

  pSec.nLength = sizeof(pSec);
  tSec.nLength = sizeof(tSec);

  rdcwstr workdir = L"";

  if(!workingDir.empty())
    workdir = StringFormat::UTF82Wide(workingDir);
  else
    workdir = StringFormat::UTF82Wide(get_dirname(app));

  wchar_t *paramsAlloc = NULL;

  rdcwstr wapp = StringFormat::UTF82Wide(app);

  // CreateProcessW can modify the params, need space.
  size_t len = wapp.length() + 10;

  rdcwstr wcmd = L"";

  if(!cmdLine.empty())
  {
    wcmd = StringFormat::UTF82Wide(cmdLine);
    len += wcmd.length();
  }

  paramsAlloc = new wchar_t[len];

  RDCEraseMem(paramsAlloc, len * sizeof(wchar_t));

  wcscpy_s(paramsAlloc, len, L"\"");
  wcscat_s(paramsAlloc, len, wapp.c_str());
  wcscat_s(paramsAlloc, len, L"\"");

  if(!cmdLine.empty())
  {
    wcscat_s(paramsAlloc, len, L" ");
    wcscat_s(paramsAlloc, len, wcmd.c_str());
  }

  bool inheritHandles = false;

  HANDLE hChildStdOutput_Wr = 0, hChildStdError_Wr = 0;
  if(phChildStdOutput_Rd)
  {
    RDCASSERT(phChildStdError_Rd);

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if(!CreatePipe(phChildStdOutput_Rd, &hChildStdOutput_Wr, &sa, 0))
      RDCERR("Could not create pipe to read stdout");
    if(!SetHandleInformation(*phChildStdOutput_Rd, HANDLE_FLAG_INHERIT, 0))
      RDCERR("Could not set pipe handle information");

    if(!CreatePipe(phChildStdError_Rd, &hChildStdError_Wr, &sa, 0))
      RDCERR("Could not create pipe to read stdout");
    if(!SetHandleInformation(*phChildStdError_Rd, HANDLE_FLAG_INHERIT, 0))
      RDCERR("Could not set pipe handle information");

    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = hChildStdOutput_Wr;
    si.hStdError = hChildStdError_Wr;

    // Need to inherit handles in CreateProcess for ReadFile to read stdout
    inheritHandles = true;
  }

  // if it's a utility launch, hide the command prompt window from showing
  if(phChildStdOutput_Rd || internal)
    si.dwFlags |= STARTF_USESHOWWINDOW;

  if(!internal)
    RDCLOG("Running process %s", app.c_str());

  // turn environment string to a UTF-8 map
  std::wstring envString;

  if(!env.empty())
  {
    LPWCH envStrings = GetEnvironmentStringsW();
    EnvMap envValues = EnvStringToEnvMap(envStrings);
    FreeEnvironmentStringsW(envStrings);

    ApplyEnvModifications(envValues, env, false);

    for(auto it = envValues.begin(); it != envValues.end(); ++it)
    {
      envString += StringFormat::UTF82Wide(it->first).c_str();
      envString += L"=";
      envString += StringFormat::UTF82Wide(it->second).c_str();
      envString.push_back(0);
    }
  }

  BOOL retValue = CreateProcessW(
      NULL, paramsAlloc, &pSec, &tSec, inheritHandles, CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT,
      envString.empty() ? NULL : (void *)envString.data(), workdir.c_str(), &si, &pi);

  DWORD err = GetLastError();

  if(phChildStdOutput_Rd)
  {
    CloseHandle(hChildStdOutput_Wr);
    CloseHandle(hChildStdError_Wr);
  }

  SAFE_DELETE_ARRAY(paramsAlloc);

  if(!retValue)
  {
    if(!internal)
      RDCWARN("Process %s could not be loaded (error %d).", app.c_str(), err);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    RDCEraseEl(pi);
  }

  return pi;
}

rdcpair<RDResult, uint32_t> Process::InjectIntoProcess(uint32_t pid,
                                                       const rdcarray<EnvironmentModification> &env,
                                                       const rdcstr &capturefile,
                                                       const CaptureOptions &opts, bool waitForExit)
{
  rdcwstr wcapturefile = StringFormat::UTF82Wide(capturefile);

  HANDLE hProcess =
      OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
                      PROCESS_VM_WRITE | PROCESS_VM_READ | SYNCHRONIZE,
                  FALSE, pid);

  if(hProcess == NULL)
  {
    DWORD err = GetLastError();
    RDResult result;
    SET_ERROR_RESULT(result, ResultCode::InjectionFailed,
                     "OpenProcess failed for process %u (Win32 error 0x%08x)", pid, err);
    return {result, 0};
  }

  if(opts.delayForDebugger > 0)
  {
    RDCDEBUG("Waiting for debugger attach to %lu", pid);
    uint32_t timeout = 0;

    BOOL debuggerAttached = FALSE;

    while(!debuggerAttached)
    {
      if(!CheckRemoteDebuggerPresent(hProcess, &debuggerAttached))
      {
        DWORD err = GetLastError();
        CloseHandle(hProcess);
        RDResult result;
        SET_ERROR_RESULT(result, ResultCode::InjectionFailed,
                         "CheckRemoteDebuggerPresent failed for process %u (Win32 error 0x%08x)",
                         pid, err);
        return {result, 0};
      }

      Sleep(10);
      timeout += 10;

      if(timeout > opts.delayForDebugger * 1000)
        break;
    }

    if(debuggerAttached)
      RDCDEBUG("Debugger attach detected after %.2f s", float(timeout) / 1000.0f);
    else
      RDCDEBUG("Timed out waiting for debugger, gave up after %u s", opts.delayForDebugger);
  }

  RDCLOG("Injecting CaptureProbe into process %lu", pid);

  wchar_t rendercccccPath[MAX_PATH] = {0};
  GetModuleFileNameW(GetModuleHandleA(STRINGIZE(RDOC_BASE_NAME) ".dll"), &rendercccccPath[0],
                                      MAX_PATH - 1);

  wchar_t rendercccccPathLower[MAX_PATH] = {0};
  memcpy(rendercccccPathLower, rendercccccPath, MAX_PATH * sizeof(wchar_t));
  for(size_t i = 0; i < MAX_PATH && rendercccccPathLower[i]; i++)
  {
    // lowercase
    if(rendercccccPathLower[i] >= 'A' && rendercccccPathLower[i] <= 'Z')
      rendercccccPathLower[i] = 'a' + char(rendercccccPathLower[i] - 'A');

    // normalise paths
    if(rendercccccPathLower[i] == '/')
      rendercccccPathLower[i] = '\\';
  }

  BOOL isWow64 = FALSE;
  BOOL success = IsWow64Process(hProcess, &isWow64);

  if(!success)
  {
    DWORD err = GetLastError();
    RDResult result;
    SET_ERROR_RESULT(result, ResultCode::IncompatibleProcess,
                     "Couldn't determine bitness of process, err: %08x", err);
    CloseHandle(hProcess);
    return {result, 0};
  }

  bool capalt = false;

#if DISABLED(RDOC_X64)
  BOOL selfWow64 = FALSE;

  HANDLE hSelfProcess = GetCurrentProcess();

  // check to see if we're a WoW64 process
  success = IsWow64Process(hSelfProcess, &selfWow64);

  CloseHandle(hSelfProcess);

  if(!success)
  {
    DWORD err = GetLastError();
    RDResult result;
    SET_ERROR_RESULT(result, ResultCode::IncompatibleProcess,
                     "Couldn't determine bitness of self, err: %08x", err);
    CloseHandle(hProcess);
    return {result, 0};
  }

  // we know we're 32-bit, so if the target process is not wow64
  // and we are, it's 64-bit. If we're both not wow64 then we're
  // running on 32-bit windows, and if we're both wow64 then we're
  // both 32-bit on 64-bit windows.
  //
  // We don't support capturing 64-bit programs from a 32-bit install
  // because it's pointless - a 64-bit install will work for all in
  // that case. But we do want to handle the case of:
  // 64-bit renderccccc -> 32-bit program (via 32-bit renderccccccmd)
  //    -> 64-bit program (going back to 64-bit renderccccccmd).
  // so we try to see if we're an x86 invoked renderccccccmd in an
  // otherwise 64-bit install, and 'promote' back to 64-bit.
  if(selfWow64 && !isWow64)
  {
    wchar_t *slash = wcsrchr(rendercccccPath, L'\\');

    if(slash && slash > rendercccccPath + 4)
    {
      slash -= 4;

      if(slash && !wcsncmp(slash, L"\\x86", 4))
      {
        RDCDEBUG("Promoting back to 64-bit");
        capalt = true;
      }
    }

    // if it looks like we're in the development environment, look for the alternate bitness in the
    // corresponding folder
    if(!capalt)
    {
      const wchar_t *devLocation = wcsstr(rendercccccPathLower, L"\\win32\\development\\");
      if(!devLocation)
        devLocation = wcsstr(rendercccccPathLower, L"\\win32\\release\\");

      if(devLocation)
      {
        RDCDEBUG("Promoting back to 64-bit");
        capalt = true;
      }
    }

    // if we couldn't promote, then bail out.
    if(!capalt)
    {
      RDCDEBUG("Running from %ls", rendercccccPathLower);

      CloseHandle(hProcess);
      RDResult result;
      SET_ERROR_RESULT(result, ResultCode::IncompatibleProcess,
                       "Can't capture 64-bit program with 32-bit build of CaptureProbe. Please run a "
                       "64-bit build of CaptureProbe");
      return {result, 0};
    }
  }
#else
  // farm off to alternate bitness captureprobecmd.exe

  // if the target process is 'wow64' that means it's 32-bit.
  capalt = (isWow64 == TRUE);
#endif

  if(capalt)
  {
#if ENABLED(RDOC_X64)
    // if it looks like we're in the development environment, look for the alternate bitness in the
    // corresponding folder
    const wchar_t *devLocation = wcsstr(rendercccccPathLower, L"\\x64\\development\\");
    if(devLocation)
    {
      size_t idx = devLocation - rendercccccPathLower;

      rendercccccPath[idx] = 0;

      wcscat_s(rendercccccPath, L"\\Win32\\Development\\captureprobecmd.exe");
    }

    if(!devLocation)
    {
      devLocation = wcsstr(rendercccccPathLower, L"\\x64\\release\\");

      if(devLocation)
      {
        size_t idx = devLocation - rendercccccPathLower;

        rendercccccPath[idx] = 0;

        wcscat_s(rendercccccPath, L"\\Win32\\Release\\captureprobecmd.exe");
      }
    }

    if(!devLocation)
    {
      // look in a subfolder for x86.

      // remove the filename from the path
      wchar_t *slash = wcsrchr(rendercccccPath, L'\\');

      if(slash)
        *slash = 0;

      // append path
      wcscat_s(rendercccccPath, L"\\x86\\captureprobecmd.exe");
    }
#else
    // if it looks like we're in the development environment, look for the alternate bitness in the
    // corresponding folder
    const wchar_t *devLocation = wcsstr(rendercccccPathLower, L"\\win32\\development\\");
    if(devLocation)
    {
      size_t idx = devLocation - rendercccccPathLower;

      rendercccccPath[idx] = 0;

      wcscat_s(rendercccccPath, L"\\x64\\Development\\captureprobecmd.exe");
    }

    if(!devLocation)
    {
      devLocation = wcsstr(rendercccccPathLower, L"\\win32\\release\\");

      if(devLocation)
      {
        size_t idx = devLocation - rendercccccPathLower;

        rendercccccPath[idx] = 0;

        wcscat_s(rendercccccPath, L"\\x64\\Release\\captureprobecmd.exe");
      }
    }

    if(!devLocation)
    {
      // look upwards on 32-bit to find the parent renderccccccmd.
      wchar_t *slash = wcsrchr(rendercccccPath, L'\\');

      // remove the filename
      if(slash)
        *slash = 0;

      // remove the \\x86
      slash = wcsrchr(rendercccccPath, L'\\');

      if(slash)
        *slash = 0;

      // append path
      wcscat_s(rendercccccPath, L"\\captureprobecmd.exe");
    }
#endif

    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    SECURITY_ATTRIBUTES pSec;
    SECURITY_ATTRIBUTES tSec;

    RDCEraseEl(pi);
    RDCEraseEl(si);
    RDCEraseEl(pSec);
    RDCEraseEl(tSec);

    // hide the console window
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    pSec.nLength = sizeof(pSec);
    tSec.nLength = sizeof(tSec);

    // serialise to string with two chars per byte
    rdcstr optstr = opts.EncodeAsString();

    wchar_t *paramsAlloc = new wchar_t[2048];

    rdcstr debugLogfile = RDCGETLOGFILE();
    rdcwstr wdebugLogfile = StringFormat::UTF82Wide(debugLogfile);

    _snwprintf_s(
        paramsAlloc, 2047, 2047,
        L"\"%ls\" capaltbit --pid=%u --capfile=\"%ls\" --debuglog=\"%ls\" --capopts=\"%hs\"",
        rendercccccPath, pid, wcapturefile.c_str(), wdebugLogfile.c_str(), optstr.c_str());

    RDCDEBUG("params %ls", paramsAlloc);

    paramsAlloc[2047] = 0;

    wchar_t *commandLine = paramsAlloc;

    std::wstring cmdWithEnv;

    if(!env.empty())
    {
      cmdWithEnv = paramsAlloc;

      for(const EnvironmentModification &e : env)
      {
        rdcstr name = e.name.trimmed();
        rdcstr value = e.value;

        if(name == "")
          break;

        cmdWithEnv += L" +env-";
        switch(e.mod)
        {
          case EnvMod::Set: cmdWithEnv += L"replace"; break;
          case EnvMod::Append: cmdWithEnv += L"append"; break;
          case EnvMod::Prepend: cmdWithEnv += L"prepend"; break;
        }

        if(e.mod != EnvMod::Set)
        {
          switch(e.sep)
          {
            case EnvSep::Platform: cmdWithEnv += L"-platform"; break;
            case EnvSep::SemiColon: cmdWithEnv += L"-semicolon"; break;
            case EnvSep::Colon: cmdWithEnv += L"-colon"; break;
            case EnvSep::NoSep: break;
          }
        }

        cmdWithEnv += L" ";

        // escape the parameters
        for(size_t it = 0; it < name.size(); it++)
        {
          if(name[it] == '"')
          {
            name.insert(it, '\\');
            it++;
          }
        }

        for(size_t it = 0; it < value.size(); it++)
        {
          if(value[it] == '"')
          {
            value.insert(it, '\\');
            it++;
          }
        }

        if(name.back() == '\\')
          name += "\\";

        if(value.back() == '\\')
          value += "\\";

        cmdWithEnv += L"\"" + std::wstring(StringFormat::UTF82Wide(name).c_str()) + L"\" ";
        cmdWithEnv += L"\"" + std::wstring(StringFormat::UTF82Wide(value).c_str()) + L"\" ";
      }

      commandLine = (wchar_t *)cmdWithEnv.c_str();
    }

    BOOL retValue = CreateProcessW(NULL, commandLine, &pSec, &tSec, false,
                                   CREATE_NEW_CONSOLE | CREATE_SUSPENDED, NULL, NULL, &si, &pi);

    SAFE_DELETE_ARRAY(paramsAlloc);

    if(!retValue)
    {
      RDResult result;
#if RENDERCCCCC_OFFICIAL_BUILD
      SET_ERROR_RESULT(result, ResultCode::InternalError,
                       "Can't run 32-bit captureprobecmd to capture 32-bit program.");
#else
      SET_ERROR_RESULT(
          result, ResultCode::InternalError,
          "Can't run 32-bit captureprobecmd to capture 32-bit program."
          "If this is a locally built CaptureProbe you must build both 32-bit and 64-bit versions.");
#endif
      CloseHandle(hProcess);
      return {result, 0};
    }

    DWORD helperResumeCount = ResumeThread(pi.hThread);
    if(helperResumeCount != 1)
    {
      DWORD err = helperResumeCount == DWORD(-1) ? GetLastError() : ERROR_INVALID_STATE;
      TerminateLaunchedProcess(pi.hProcess, "the cross-bitness injector helper could not be resumed");
      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);
      CloseHandle(hProcess);
      RDResult result;
      SET_ERROR_RESULT(result, ResultCode::InjectionFailed,
                       "Couldn't resume cross-bitness injector for process %u "
                       "(Win32 error 0x%08x)",
                       pid, err);
      return {result, 0};
    }

    DWORD helperWait = WaitForSingleObject(pi.hThread, INFINITE);
    if(helperWait != WAIT_OBJECT_0)
    {
      DWORD err = helperWait == WAIT_FAILED ? GetLastError() : ERROR_GEN_FAILURE;
      TerminateLaunchedProcess(pi.hProcess, "the cross-bitness injector helper did not finish");
      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);
      CloseHandle(hProcess);
      RDResult result;
      SET_ERROR_RESULT(result, ResultCode::InjectionFailed,
                       "Couldn't wait for cross-bitness injector for process %u "
                       "(Win32 error 0x%08x)",
                       pid, err);
      return {result, 0};
    }
    CloseHandle(pi.hThread);

    DWORD exitCode = 0;
    if(!GetExitCodeProcess(pi.hProcess, &exitCode))
    {
      DWORD err = GetLastError();
      CloseHandle(pi.hProcess);
      CloseHandle(hProcess);
      RDResult result;
      SET_ERROR_RESULT(result, ResultCode::InjectionFailed,
                       "Couldn't get cross-bitness injector result for process %u "
                       "(Win32 error 0x%08x)",
                       pid, err);
      return {result, 0};
    }
    CloseHandle(pi.hProcess);

    if(waitForExit)
    {
      DWORD targetWait = WaitForSingleObject(hProcess, INFINITE);
      if(targetWait != WAIT_OBJECT_0)
      {
        DWORD err = targetWait == WAIT_FAILED ? GetLastError() : ERROR_GEN_FAILURE;
        CloseHandle(hProcess);
        RDResult result;
        SET_ERROR_RESULT(result, ResultCode::InjectionFailed,
                         "Couldn't wait for target process %u (Win32 error 0x%08x)", pid, err);
        return {result, 0};
      }
    }

    CloseHandle(hProcess);

    if(exitCode == 0)
    {
      RDResult result;
      SET_ERROR_RESULT(result, ResultCode::UnknownError,
                       "Encountered error while launching target 32-bit program.");
      return {result, 0};
    }

    if(exitCode < Renderccccc_FirstTargetControlPort)
    {
      ResultCode code = (ResultCode)exitCode;

      RDResult result;
      SET_ERROR_RESULT(result, code, "32-bit captureprobecmd returned '%s'", ToStr(code).c_str());
      return {code, 0};
    }

    return {ResultCode::Succeeded, (uint32_t)exitCode};
  }

  InjectDLL(hProcess, rendercccccPath);

  rdcpair<RDResult, uint32_t> result =
      ConfigureInjectedProcess(hProcess, pid, capturefile, opts, env, false);

  if(waitForExit)
  {
    DWORD targetWait = WaitForSingleObject(hProcess, INFINITE);
    if(targetWait != WAIT_OBJECT_0)
    {
      DWORD err = targetWait == WAIT_FAILED ? GetLastError() : ERROR_GEN_FAILURE;
      SET_ERROR_RESULT(result.first, ResultCode::InjectionFailed,
                       "Couldn't wait for target process %u (Win32 error 0x%08x)", pid, err);
    }
  }

  CloseHandle(hProcess);

  return result;
}

uint32_t Process::LaunchProcess(const rdcstr &app, const rdcstr &workingDir, const rdcstr &cmdLine,
                                bool internal, ProcessResult *result)
{
  HANDLE hChildStdOutput_Rd = NULL, hChildStdError_Rd = NULL;

  rdcstr appPath = app;
  size_t len = appPath.length();
  rdcstr ext;
  if(len > 4)
    ext = strlower(appPath.substr(len - 4));
  if(ext != ".exe")
    appPath += ".exe";

  PROCESS_INFORMATION pi =
      RunProcess(appPath, workingDir, cmdLine, {}, internal, result ? &hChildStdOutput_Rd : NULL,
                 result ? &hChildStdError_Rd : NULL);

  if(pi.dwProcessId == 0)
  {
    if(!internal)
      RDCWARN("Couldn't launch process '%s'", appPath.c_str());

    if(hChildStdError_Rd != NULL)
      CloseHandle(hChildStdError_Rd);
    if(hChildStdOutput_Rd != NULL)
      CloseHandle(hChildStdOutput_Rd);

    return 0;
  }

  if(!internal)
    RDCLOG("Launched process '%s' with '%s'", appPath.c_str(), cmdLine.c_str());

  ResumeThread(pi.hThread);

  if(result)
  {
    result->strStdout = "";
    result->strStderror = "";

    char chBuf[4096];
    DWORD dwOutputRead, dwErrorRead;
    BOOL success = FALSE;
    rdcstr s;
    for(;;)
    {
      success = ReadFile(hChildStdOutput_Rd, chBuf, sizeof(chBuf), &dwOutputRead, NULL);
      s = rdcstr(chBuf, dwOutputRead);
      result->strStdout += s;

      if(!success && !dwOutputRead)
        break;
    }

    for(;;)
    {
      success = ReadFile(hChildStdError_Rd, chBuf, sizeof(chBuf), &dwErrorRead, NULL);
      s = rdcstr(chBuf, dwErrorRead);
      result->strStderror += s;

      if(!success && !dwErrorRead)
        break;
    }

    CloseHandle(hChildStdOutput_Rd);
    CloseHandle(hChildStdError_Rd);

    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, (LPDWORD)&result->retCode);
  }

  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);

  return pi.dwProcessId;
}

uint32_t Process::LaunchScript(const rdcstr &script, const rdcstr &workingDir,
                               const rdcstr &argList, bool internal, ProcessResult *result)
{
  // Change parameters to invoke command interpreter
  rdcstr args = "/C " + script + " " + argList;

  return LaunchProcess("cmd.exe", workingDir, args, internal, result);
}

rdcpair<RDResult, uint32_t> Process::LaunchAndInjectIntoProcess(
    const rdcstr &app, const rdcstr &workingDir, const rdcstr &cmdLine,
    const rdcarray<EnvironmentModification> &env, const rdcstr &capturefile,
    const CaptureOptions &opts, bool waitForExit)
{
  HMODULE selfModule = GetModuleHandleA(STRINGIZE(RDOC_BASE_NAME) ".dll");
  void *func = selfModule ? GetProcAddress(selfModule, "INTERNAL_SetCaptureFile") : NULL;

  wchar_t dllPath[MAX_PATH] = {};
  DWORD dllPathLength = selfModule ? GetModuleFileNameW(selfModule, dllPath, MAX_PATH) : 0;

  if(func == NULL || dllPathLength == 0 || dllPathLength >= MAX_PATH)
  {
    const char *rdoc_dll = STRINGIZE(RDOC_BASE_NAME);
    RDResult result;
    SET_ERROR_RESULT(result, ResultCode::InternalError,
                     "Can't find required export function in %s.dll - corrupted/missing file?",
                     rdoc_dll);
    return {result, 0};
  }

  if(get_basename(app) == "explorer.exe" || get_basename(app) == "dllhost.exe")
  {
    RDResult result;
    SET_ERROR_RESULT(
        result, ResultCode::InjectionFailed,
        "For safety reasons CaptureProbe does not support capturing executables with a "
        "reserved system filename such as '%s'. Please rename your executable to capture.",
        get_basename(app).c_str());
    return {result, 0};
  }

  PROCESS_INFORMATION pi = RunProcess(app, workingDir, cmdLine, env, false, NULL, NULL);

  if(pi.dwProcessId == 0)
  {
    RDResult result;
    SET_ERROR_RESULT(result, ResultCode::InjectionFailed, "Failed to launch process.");
    return {result, 0};
  }

  if(pi.hProcess == NULL || pi.hThread == NULL)
  {
    RDResult result;
    SET_ERROR_RESULT(result, ResultCode::InjectionFailed,
                     "Failed to get process handles after launching process %u", pi.dwProcessId);
    if(pi.hProcess)
      TerminateLaunchedProcess(pi.hProcess, "the process handles returned by CreateProcess were invalid");
    if(pi.hThread)
      CloseHandle(pi.hThread);
    if(pi.hProcess)
      CloseHandle(pi.hProcess);
    return {result, 0};
  }

  BOOL selfWow64 = FALSE;
  BOOL targetWow64 = FALSE;
  bool sameBitness = IsWow64Process(GetCurrentProcess(), &selfWow64) &&
                     IsWow64Process(pi.hProcess, &targetWow64) && (selfWow64 == targetWow64);

  rdcpair<RDResult, uint32_t> ret;
  if(sameBitness)
  {
    // RunProcess created this thread suspended. The trampoline loads the DLL on
    // this thread, parks it while the host configures the DLL, then restores the
    // exact startup CONTEXT before the application's initial loader code runs.
    ret = InjectIntoSuspendedPrimaryThread(pi.hProcess, pi.hThread, pi.dwProcessId, dllPath,
                                           capturefile, opts, {});
  }
  else
  {
    // Keep the existing cross-bitness helper path: this process cannot interpret
    // the target's CONTEXT layout or call into its loader directly.
    ret = InjectIntoProcess(pi.dwProcessId, {}, capturefile, opts, false);

    DWORD resumeCount = ResumeThread(pi.hThread);
    if(resumeCount != 1)
    {
      DWORD err = resumeCount == DWORD(-1) ? GetLastError() : ERROR_INVALID_STATE;
      RDCERR("Couldn't resume cross-bitness target process %u: %u", pi.dwProcessId, err);
      TerminateLaunchedProcess(pi.hProcess, "the cross-bitness primary thread could not be resumed");
      if(ret.first == ResultCode::Succeeded)
      {
        SET_ERROR_RESULT(ret.first, ResultCode::InjectionFailed,
                         "Couldn't resume process %u after cross-bitness injection "
                         "(Win32 error 0x%08x)",
                         pi.dwProcessId, err);
      }
    }
  }

  if(ret.second == 0 || ret.first != ResultCode::Succeeded)
  {
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return ret;
  }

  if(waitForExit)
  {
    DWORD waitResult = WaitForSingleObject(pi.hThread, INFINITE);
    if(waitResult != WAIT_OBJECT_0)
    {
      DWORD err = waitResult == WAIT_FAILED ? GetLastError() : ERROR_GEN_FAILURE;
      SET_ERROR_RESULT(ret.first, ResultCode::InjectionFailed,
                       "Couldn't wait for process %u primary thread (Win32 error 0x%08x)",
                       pi.dwProcessId, err);
    }
  }

  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);

  return ret;
}

bool Process::CanGlobalHook()
{
  // all we need is admin rights and it's the caller's responsibility to ensure that.
  return true;
}

// to simplify the below code, rather than splitting by 32-bit/64-bit we split by native and Wow32.
// This means that for 32-bit code (whether it's on 32-bit OS or not) we just have native, and the
// Wow32 stuff is empty/unused. For 64-bit we use both. Thus the native registry key is always the
// same path regardless of the bitness we're running as and we don't have to move things around or
// have conditionals all over

struct GlobalHookData
{
  struct
  {
    HANDLE pipe = NULL;
    DWORD appinitEnabled = 0;
    rdcwstr appinitDLLs;
  } dataNative, dataWow32;

  int32_t finished = 0;
  Threading::ThreadHandle pipeThread = 0;
};

// utility function to close the registry keys, print an error, and quit
static RDResult HandleRegError(HKEY keyNative, HKEY keyWow32, LSTATUS ret, const char *msg)
{
  if(keyNative)
    RegCloseKey(keyNative);

  if(keyWow32)
    RegCloseKey(keyWow32);

  RDCLOG("Error with AppInit registry keys - %s (%d)", msg, ret);

  RETURN_ERROR_RESULT(ResultCode::InjectionFailed,
                      "Error updating registry to enable global hook.\n"
                      "Check that CaptureProbe is correctly running as administrator.");
}

#define REG_CHECK(msg)                                    \
  if(ret != ERROR_SUCCESS)                                \
  {                                                       \
    return HandleRegError(keyNative, keyWow32, ret, msg); \
  }

// function to backup the previous settings for AppInit, then enable it and write our own paths.
RDResult BackupAndChangeRegistry(GlobalHookData &hookdata, const rdcstr &shimpathWow32,
                                 const rdcstr &shimpathNative)
{
  HKEY keyNative = NULL;
  HKEY keyWow32 = NULL;

  // AppInit_DLLs requires short paths, but short paths can be disabled globally or on a per-volume
  // level. If short paths are disabled we'll get the long path back, we *always* expect the path to
  // get shorter because the shim filename is bigger than 8.3.

  DWORD nativeShortSize = GetShortPathNameW(StringFormat::UTF82Wide(shimpathNative).c_str(), NULL,
                                            (DWORD)shimpathNative.length());
  if(nativeShortSize == (DWORD)shimpathNative.length() + 1)
  {
    RETURN_ERROR_RESULT(
        ResultCode::FileIOFailed,
        "CaptureProbe is installed on a volume or system that has short paths disabled.\n"
        "For the global hook, short paths must be enabled where CaptureProbe is installed.");
  }

  if(!shimpathWow32.empty())
  {
    DWORD wow32ShortSize = GetShortPathNameW(StringFormat::UTF82Wide(shimpathWow32).c_str(), NULL,
                                             (DWORD)shimpathWow32.length());

    if(wow32ShortSize == (DWORD)shimpathWow32.length() + 1)
    {
      RETURN_ERROR_RESULT(
          ResultCode::FileIOFailed,
          "CaptureProbe is installed on a volume or system that has short paths disabled.\n"
          "For the global hook, short paths must be enabled where CaptureProbe is installed.");
    }
  }

  // open the native key
  LSTATUS ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE,
                                "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows", 0, NULL,
                                0, KEY_READ | KEY_WRITE, NULL, &keyNative, NULL);

  REG_CHECK("Could not open AppInit key");

  // if we are doing Wow32, open that key as well
  if(!shimpathWow32.empty())
  {
    ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE,
                          "SOFTWARE\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\Windows",
                          0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &keyWow32, NULL);

    REG_CHECK("Could not open AppInit key");
  }

  const DWORD one = 1;

  // fetch the previous data for LoadAppInit_DLLs and AppInit_DLLs
  DWORD sz = 4;
  ret = RegGetValueA(keyNative, NULL, "LoadAppInit_DLLs", RRF_RT_REG_DWORD, NULL,
                     (void *)&hookdata.dataNative.appinitEnabled, &sz);
  REG_CHECK("Could not fetch LoadAppInit_DLLs");

  sz = 0;
  ret = RegGetValueW(keyNative, NULL, L"AppInit_DLLs", RRF_RT_ANY, NULL, NULL, &sz);
  if(ret == ERROR_MORE_DATA || ret == ERROR_SUCCESS)
  {
    hookdata.dataNative.appinitDLLs = rdcwstr(sz / sizeof(wchar_t));
    ret = RegGetValueW(keyNative, NULL, L"AppInit_DLLs", RRF_RT_ANY, NULL,
                       hookdata.dataNative.appinitDLLs.data(), &sz);
  }
  REG_CHECK("Could not fetch AppInit_DLLs");

  // set DWORD:1 for LoadAppInit_DLLs and convert our path to a short path then set it
  ret = RegSetValueExA(keyNative, "LoadAppInit_DLLs", 0, REG_DWORD, (const BYTE *)&one, sizeof(one));
  REG_CHECK("Could not set LoadAppInit_DLLs");

  rdcwstr shortpath(shimpathNative.size());
  GetShortPathNameW(StringFormat::UTF82Wide(shimpathNative).c_str(), shortpath.data(),
                    (DWORD)shortpath.length());

  ret = RegSetValueExW(keyNative, L"AppInit_DLLs", 0, REG_SZ, (const BYTE *)shortpath.data(),
                       DWORD(shortpath.length() * sizeof(wchar_t)));
  REG_CHECK("Could not set AppInit_DLLs");

  // if we're doing Wow32, repeat the process for those keys
  if(keyWow32)
  {
    sz = 4;
    ret = RegGetValueA(keyWow32, NULL, "LoadAppInit_DLLs", RRF_RT_REG_DWORD, NULL,
                       (void *)&hookdata.dataWow32.appinitEnabled, &sz);
    REG_CHECK("Could not fetch LoadAppInit_DLLs");

    sz = 0;
    ret = RegGetValueW(keyWow32, NULL, L"AppInit_DLLs", RRF_RT_ANY, NULL, NULL, &sz);
    if(ret == ERROR_MORE_DATA || ret == ERROR_SUCCESS)
    {
      hookdata.dataWow32.appinitDLLs = rdcwstr(sz / sizeof(wchar_t));
      ret = RegGetValueW(keyWow32, NULL, L"AppInit_DLLs", RRF_RT_ANY, NULL,
                         hookdata.dataWow32.appinitDLLs.data(), &sz);
    }
    REG_CHECK("Could not fetch AppInit_DLLs");

    ret = RegSetValueExA(keyWow32, "LoadAppInit_DLLs", 0, REG_DWORD, (const BYTE *)&one, sizeof(one));
    REG_CHECK("Could not set LoadAppInit_DLLs");

    shortpath = rdcwstr(shimpathWow32.size());
    GetShortPathNameW(StringFormat::UTF82Wide(shimpathWow32).c_str(), shortpath.data(),
                      (DWORD)shortpath.length());

    ret = RegSetValueExW(keyWow32, L"AppInit_DLLs", 0, REG_SZ, (const BYTE *)shortpath.data(),
                         DWORD(shortpath.length() * sizeof(wchar_t)));
    REG_CHECK("Could not set AppInit_DLLs");
  }

  std::wstring backup;

  // write a .reg file that contains the previous settings, so that if all else fails the user can
  // manually insert it back into the registry to restore everything.
  backup += L"Windows Registry Editor Version 5.00\n";
  backup += L"\n";
  backup += L"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows]\n";
  backup += L"\"LoadAppInit_DLLs\"=dword:0000000";
  backup += (hookdata.dataNative.appinitEnabled ? L"1\n" : L"0\n");
  backup += L"\"AppInit_DLLs\"=\"";
  // we append with the C string so we don't add trailing NULLs into the text.
  backup += hookdata.dataNative.appinitDLLs.c_str();
  backup += L"\"\n";
  if(keyWow32)
  {
    backup += L"\n";
    backup +=
        L"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\"
        L"Windows NT\\CurrentVersion\\Windows]\n";
    backup += L"\"LoadAppInit_DLLs\"=dword:0000000";
    backup += (hookdata.dataWow32.appinitEnabled ? L"1\n" : L"0\n");
    backup += L"\"AppInit_DLLs\"=\"";
    backup += hookdata.dataWow32.appinitDLLs.c_str();
    backup += L"\"\n";
  }

  if(keyNative)
    RegCloseKey(keyNative);

  if(keyWow32)
    RegCloseKey(keyWow32);

  keyNative = keyWow32 = NULL;

  // write it to disk but don't fail if we can't, just print it to the log and keep going.
  wchar_t reg_backup[MAX_PATH];
  GetTempPathW(MAX_PATH, reg_backup);
  wcscat_s(reg_backup, L"CaptureProbe_RestoreGlobalHook.reg");

  FILE *f = NULL;
  _wfopen_s(&f, reg_backup, L"w");
  if(f)
  {
    fputws(backup.c_str(), f);
    fclose(f);
  }
  else
  {
    RDCERR("Error opening registry backup file %ls", reg_backup);
    RDCERR("Backup registry data is:\n\n%ls\n\n", backup.c_str());
  }

  return RDResult();
}

// switch error-handling to print-and-continue, as we can't really do anything about it at this
// point and we want to continue restoring in case only one thing failed.
#undef REG_CHECK
#define REG_CHECK(msg)                                                      \
  if(ret != ERROR_SUCCESS)                                                  \
  {                                                                         \
    HandleRegError(keyNative, keyWow32, ret, "Could not open AppInit key"); \
  }

void RestoreRegistry(const GlobalHookData &hookdata)
{
  HKEY keyNative = NULL;
  HKEY keyWow32 = NULL;
  LSTATUS ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE,
                                "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows", 0, NULL,
                                0, KEY_READ | KEY_WRITE, NULL, &keyNative, NULL);

  REG_CHECK("Could not open AppInit key");

#if ENABLED(RDOC_X64)
  ret = RegCreateKeyExA(HKEY_LOCAL_MACHINE,
                        "SOFTWARE\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\Windows", 0,
                        NULL, 0, KEY_READ | KEY_WRITE, NULL, &keyWow32, NULL);

  REG_CHECK("Could not open AppInit key");
#endif

  // set the native values back to where they were
  ret = RegSetValueExA(keyNative, "LoadAppInit_DLLs", 0, REG_DWORD,
                       (const BYTE *)&hookdata.dataNative.appinitEnabled,
                       sizeof(hookdata.dataNative.appinitEnabled));
  REG_CHECK("Could not set LoadAppInit_DLLs");

  ret = RegSetValueExW(keyNative, L"AppInit_DLLs", 0, REG_SZ,
                       (const BYTE *)hookdata.dataNative.appinitDLLs.c_str(),
                       DWORD(hookdata.dataNative.appinitDLLs.length() * sizeof(wchar_t)));
  REG_CHECK("Could not set AppInit_DLLs");

  // if we opened it, restore the Wow32 values as well
  if(keyWow32)
  {
    ret = RegSetValueExA(keyWow32, "LoadAppInit_DLLs", 0, REG_DWORD,
                         (const BYTE *)&hookdata.dataWow32.appinitEnabled,
                         sizeof(hookdata.dataWow32.appinitEnabled));
    REG_CHECK("Could not set LoadAppInit_DLLs");

    ret = RegSetValueExW(keyWow32, L"AppInit_DLLs", 0, REG_SZ,
                         (const BYTE *)hookdata.dataWow32.appinitDLLs.c_str(),
                         DWORD(hookdata.dataWow32.appinitDLLs.length() * sizeof(wchar_t)));
    REG_CHECK("Could not set AppInit_DLLs");
  }
}

static GlobalHookData *globalHook = NULL;

// a thread we run in the background just to keep the pipes open and wait until we're ready to stop
// the global hook.
static void GlobalHookThread()
{
  Threading::SetCurrentThreadName("GlobalHookThread");

  // keep looping doing an atomic compare-exchange to check that finished is still 0
  while(Atomic::CmpExch32(&globalHook->finished, 0, 0) == 0)
  {
    // wake every quarter of a second to test again
    Threading::Sleep(250);
  }

  char exitData[32] = "exit";

  // write some data into the pipe and close it. The data is (currently) unimportant, just that it
  // causes the blocking read on the other end to succeed and close the program.
  DWORD dummy = 0;
  if(globalHook->dataNative.pipe)
  {
    WriteFile(globalHook->dataNative.pipe, exitData, (DWORD)sizeof(exitData), &dummy, NULL);
    CloseHandle(globalHook->dataNative.pipe);
  }

  if(globalHook->dataWow32.pipe)
  {
    WriteFile(globalHook->dataWow32.pipe, exitData, (DWORD)sizeof(exitData), &dummy, NULL);
    CloseHandle(globalHook->dataWow32.pipe);
  }
}

RDResult Process::StartGlobalHook(const rdcstr &pathmatch, const rdcstr &capturefile,
                                  const CaptureOptions &opts)
{
  if(pathmatch.empty())
  {
    RETURN_ERROR_RESULT(ResultCode::InvalidParameter,
                        "Invalid global hook parameter, empty path to match");
  }

  rdcstr rendercccccPath;
  FileIO::GetLibraryFilename(rendercccccPath);

  rendercccccPath = get_dirname(rendercccccPath);

  // the native captureprobecmd.exe is always next to the dll. Wow32 will be somewhere else
  rdcstr cmdpathNative = rendercccccPath + "\\captureprobecmd.exe";
  rdcstr cmdpathWow32;

  rdcstr shimpathNative = rendercccccPath;
  rdcstr shimpathWow32;

#if ENABLED(RDOC_X64)

  // native shim is just captureprobeshim64.dll
  shimpathNative = rendercccccPath + "\\captureprobeshim64.dll";

  // if it looks like we're in the development environment, look for the alternate bitness in the
  // corresponding folder
  int devLocation = rendercccccPath.find("\\x64\\Development");
  if(devLocation >= 0)
  {
    rendercccccPath.erase(devLocation, ~0U);

    shimpathWow32 = rendercccccPath + "\\Win32\\Development\\captureprobeshim32.dll";
    cmdpathWow32 = rendercccccPath + "\\Win32\\Development\\captureprobecmd.exe";
  }
  else
  {
    devLocation = rendercccccPath.find("\\x64\\Release");

    if(devLocation >= 0)
    {
      rendercccccPath.erase(devLocation, ~0U);

      shimpathWow32 = rendercccccPath + "\\Win32\\Release\\captureprobeshim32.dll";
      cmdpathWow32 = rendercccccPath + "\\Win32\\Release\\captureprobecmd.exe";
    }
  }

  // if we're not in the dev environment, assume it's under a x86\ subfolder
  if(devLocation < 0)
  {
    shimpathWow32 = rendercccccPath + "\\x86\\captureprobeshim32.dll";
    cmdpathWow32 = rendercccccPath + "\\x86\\captureprobecmd.exe";
  }

#else

  // nothing fancy to do here for 32-bit, just point the shim next to our dll.
  shimpathNative = rendercccccPath + "\\captureprobeshim32.dll";

#endif

  GlobalHookData hookdata;

  // try to backup and change the registry settings to start loading our shim dlls. If that fails,
  // we bail out immediately
  RDResult regStatus = BackupAndChangeRegistry(hookdata, shimpathWow32, shimpathNative);
  if(regStatus != ResultCode::Succeeded)
    return regStatus;

  PROCESS_INFORMATION pi = {0};
  STARTUPINFO si = {0};
  SECURITY_ATTRIBUTES pSec = {0};
  SECURITY_ATTRIBUTES tSec = {0};
  pSec.nLength = sizeof(pSec);
  tSec.nLength = sizeof(tSec);

  si.cb = sizeof(si);

  // serialise to string with two chars per byte
  rdcstr optstr = opts.EncodeAsString();
  rdcstr debugLogfile = RDCGETLOGFILE();

  rdcstr params = StringFormat::Fmt(
      "\"%s\" globalhook --match \"%s\" --capfile \"%s\" --debuglog \"%s\" --capopts \"%s\"",
      cmdpathNative.c_str(), pathmatch.c_str(), capturefile.c_str(), debugLogfile.c_str(),
      optstr.c_str());

  rdcwstr paramsAlloc = StringFormat::UTF82Wide(params);

  // we'll be setting stdin
  si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

  // hide the console window
  si.wShowWindow = SW_HIDE;

  // this is the end of the pipe that the child will inherit and use as stdin
  HANDLE childEnd = NULL;

  DWORD err;

  // create a pipe with the writing end for us, and the reading end as the child process's stdin
  {
    SECURITY_ATTRIBUTES pipeSec;
    pipeSec.nLength = sizeof(SECURITY_ATTRIBUTES);
    pipeSec.bInheritHandle = TRUE;
    pipeSec.lpSecurityDescriptor = NULL;

    BOOL res;
    res = CreatePipe(&childEnd, &hookdata.dataNative.pipe, &pipeSec, 0);

    if(!res)
    {
      err = GetLastError();
      RestoreRegistry(hookdata);
      RETURN_ERROR_RESULT(ResultCode::InternalError, "Could not create 32-bit stdin pipe (err %u)",
                          err);
    }

    // we don't want the child process to inherit our end
    res = SetHandleInformation(hookdata.dataNative.pipe, HANDLE_FLAG_INHERIT, 0);

    if(!res)
    {
      err = GetLastError();
      RestoreRegistry(hookdata);
      RETURN_ERROR_RESULT(ResultCode::InternalError,
                          "Could not make 32-bit stdin pipe inheritable (err %u)", err);
    }

    si.hStdInput = childEnd;
  }

  // launch the process
  BOOL retValue = CreateProcessW(NULL, &paramsAlloc[0], &pSec, &tSec, true, CREATE_NEW_CONSOLE,
                                 NULL, NULL, &si, &pi);

  err = GetLastError();

  // we don't need this end anymore, the child has it
  CloseHandle(childEnd);

  if(retValue == FALSE)
  {
    CloseHandle(hookdata.dataNative.pipe);
    RestoreRegistry(hookdata);
    RETURN_ERROR_RESULT(ResultCode::InternalError, "Can't launch captureprobecmd from '%s' (err %u)",
                        cmdpathNative.c_str(), err);
  }

  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);

  RDCEraseEl(pi);

// repeat the process for the Wow32 renderccccccmd
#if ENABLED(RDOC_X64)
  params = StringFormat::Fmt(
      "\"%s\" globalhook --match \"%s\" --capfile \"%s\" --debuglog \"%s\" --capopts \"%s\"",
      cmdpathWow32.c_str(), pathmatch.c_str(), capturefile.c_str(), debugLogfile.c_str(),
      optstr.c_str());

  paramsAlloc = StringFormat::UTF82Wide(params);

  {
    SECURITY_ATTRIBUTES pipeSec;
    pipeSec.nLength = sizeof(SECURITY_ATTRIBUTES);
    pipeSec.bInheritHandle = TRUE;
    pipeSec.lpSecurityDescriptor = NULL;

    BOOL res;
    res = CreatePipe(&childEnd, &hookdata.dataWow32.pipe, &pipeSec, 0);

    if(!res)
    {
      err = GetLastError();
      RestoreRegistry(hookdata);
      RETURN_ERROR_RESULT(ResultCode::InternalError, "Could not create 64-bit stdin pipe (err %u)",
                          err);
    }

    res = SetHandleInformation(hookdata.dataWow32.pipe, HANDLE_FLAG_INHERIT, 0);

    if(!res)
    {
      err = GetLastError();
      RestoreRegistry(hookdata);
      RETURN_ERROR_RESULT(ResultCode::InternalError,
                          "Could not make 64-bit stdin pipe inheritable (err %u)", err);
    }

    si.hStdInput = childEnd;
  }

  retValue = CreateProcessW(NULL, &paramsAlloc[0], &pSec, &tSec, true, CREATE_NEW_CONSOLE, NULL,
                            NULL, &si, &pi);

  err = GetLastError();

  // we don't need this end anymore
  CloseHandle(childEnd);

  if(retValue == FALSE)
  {
    CloseHandle(hookdata.dataNative.pipe);
    CloseHandle(hookdata.dataWow32.pipe);
    RestoreRegistry(hookdata);
    RETURN_ERROR_RESULT(ResultCode::InternalError, "Can't launch captureprobecmd from '%s' (err %u)",
                        cmdpathWow32.c_str(), err);
  }

  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
#endif

  // set static global pointer with our data, and launch the thread
  globalHook = new GlobalHookData;
  *globalHook = hookdata;

  globalHook->pipeThread = Threading::CreateThread(&GlobalHookThread);

  return RDResult();
}

bool Process::IsGlobalHookActive()
{
  return globalHook != NULL;
}
void Process::StopGlobalHook()
{
  if(!globalHook)
    return;

  // set the finished flag and join to the thread so it closes the pipes (and so the child
  // processes)
  Atomic::Inc32(&globalHook->finished);

  Threading::JoinThread(globalHook->pipeThread);
  Threading::CloseThread(globalHook->pipeThread);

  // restore the registry settings from before we started
  RestoreRegistry(*globalHook);

  delete globalHook;
  globalHook = NULL;
}

bool Process::IsModuleLoaded(const rdcstr &module)
{
  return GetModuleHandleA(module.c_str()) != NULL;
}

void *Process::LoadModule(const rdcstr &module)
{
  HMODULE mod = GetModuleHandleA(module.c_str());
  if(mod != NULL)
    return mod;

  return LoadLibraryA(module.c_str());
}

void *Process::GetFunctionAddress(void *module, const rdcstr &function)
{
  if(module == NULL)
    return NULL;

  return (void *)GetProcAddress((HMODULE)module, function.c_str());
}

uint32_t Process::GetCurrentPID()
{
  return (uint32_t)GetCurrentProcessId();
}

void Process::Shutdown()
{
  // nothing to do
}
