# dxgiproxy — CaptureProbe "DLL mode" proxy

`dxgiproxy` builds a proxy `dxgi.dll` that CaptureProbe's **DLL mode** copies next to
the target executable.

## Why

The Windows loader searches the application directory before `System32`, and
`dxgi.dll` is not in the KnownDLLs list. Applications that resolve dxgi
dynamically (`LoadLibrary("dxgi.dll")` + `GetProcAddress` — UE5 does this in
`D3D12Adapter.cpp`) therefore load this proxy instead of the system copy.

The proxy loads the real system `dxgi.dll` **and** `d3d12.dll`, then loads
`captureprobe.dll`, all from `DllMain` — i.e. before the application can call any
D3D function. CaptureProbe's hooks (IAT patching of every module plus the
`GetProcAddress` hook) are therefore in place before the application resolves
`CreateDXGIFactory*` / `D3D12CreateDevice`, so the DXGI factory and the D3D12
device get wrapped and a capture can be taken.

## How it works

1. `LoadLibraryExW("%SystemRoot%\System32\dxgi.dll")` and `...\d3d12.dll` by
   absolute path, so the system copies are used even though this module is also
   called `dxgi.dll`.
2. Every export of the system dxgi is resolved with `GetProcAddress` *before*
   CaptureProbe is loaded, so the pointers are the real code, not CaptureProbe's
   hooks.
3. Each of our exports (see `dxgiproxy.def`, which mirrors the 19 exports of the
   system dxgi) gets its export table address rewritten to a jump thunk onto the
   real function, so callers that do not go through CaptureProbe's interception
   still reach the real implementation. The thunks live in the module's own
   `.proxythnk` section: an export table entry stores a 32-bit RVA, so a thunk
   allocated elsewhere cannot be addressed by it.
4. `captureprobe.dll` is loaded (path published by the launcher in shared memory,
   otherwise a sibling of this module) and configured through
   `INTERNAL_SetCaptureFile` / `INTERNAL_SetDebugLogFile` /
   `INTERNAL_SetCaptureOptions`.
5. The target control ident is published back into the shared memory, so the
   launcher can connect to the process exactly as it does after an injection.

Diagnostics are written to `%TEMP%\CaptureProbe\dllmode_proxy.log`.

## Building

Standalone project, independent of the CMake build:

```
msbuild util\dxgiproxy\dxgiproxy.vcxproj /p:Configuration=Release /p:Platform=x64
```

Output goes to `<repo>\x64\Release\proxy\dxgi.dll` (or
`<repo>\Win32\Release\proxy\dxgi.dll`), which is where the launcher looks for it:
same bitness as the launcher in `proxy\`, and the other bitness in
`Win32\<config>\proxy\` (or `x86\proxy\` in an installed layout).
