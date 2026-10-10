/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2015-2026 Baldur Karlsson
 * Copyright (c) 2014 Crytek
 * Copyright (c) 2026 CaptureProbe contributors
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

// The Windows test build exposes CaptureProbe-prefixed C exports. Keep source
// compatibility for first-party code that still uses the upstream identifiers,
// while ensuring the compiled DLL exports only the CaptureProbe names.
#if (defined(RENDERDOC_PLATFORM_WIN32) || defined(WIN32) || defined(_WIN32) || \
     defined(__WIN32__)) &&                                                     \
    !defined(SWIG) && !defined(RDOC_SELFCAPTURE_LIMITEDAPI)

#define RENDERDOC_AllocArrayMem CAPTUREPROBE_AllocArrayMem
#define RENDERDOC_BecomeRemoteServer CAPTUREPROBE_BecomeRemoteServer
#define RENDERDOC_BeginProfileRegion CAPTUREPROBE_BeginProfileRegion
#define RENDERDOC_CanGlobalHook CAPTUREPROBE_CanGlobalHook
#define RENDERDOC_CanSelfHostedCapture CAPTUREPROBE_CanSelfHostedCapture
#define RENDERDOC_CheckAndroidPackage CAPTUREPROBE_CheckAndroidPackage
#define RENDERDOC_CheckRemoteServerConnection CAPTUREPROBE_CheckRemoteServerConnection
#define RENDERDOC_CreateBugReport CAPTUREPROBE_CreateBugReport
#define RENDERDOC_CreateRemoteServerConnection CAPTUREPROBE_CreateRemoteServerConnection
#define RENDERDOC_CreateTargetControl CAPTUREPROBE_CreateTargetControl
#define RENDERDOC_EndProfileRegion CAPTUREPROBE_EndProfileRegion
#define RENDERDOC_EndSelfHostCapture CAPTUREPROBE_EndSelfHostCapture
#define RENDERDOC_EnumerateRemoteTargets CAPTUREPROBE_EnumerateRemoteTargets
#define RENDERDOC_ExecuteAndInject CAPTUREPROBE_ExecuteAndInject
#define RENDERDOC_FloatToHalf CAPTUREPROBE_FloatToHalf
#define RENDERDOC_FreeArrayMem CAPTUREPROBE_FreeArrayMem
#define RENDERDOC_GetAPI CAPTUREPROBE_GetAPI
#define RENDERDOC_GetCommitHash CAPTUREPROBE_GetCommitHash
#define RENDERDOC_GetConfigSetting CAPTUREPROBE_GetConfigSetting
#define RENDERDOC_GetCurrentProcessMemoryUsage CAPTUREPROBE_GetCurrentProcessMemoryUsage
#define RENDERDOC_GetDeviceProtocolController CAPTUREPROBE_GetDeviceProtocolController
#define RENDERDOC_GetDriverInformation CAPTUREPROBE_GetDriverInformation
#define RENDERDOC_GetLogFile CAPTUREPROBE_GetLogFile
#define RENDERDOC_GetLogFileContents CAPTUREPROBE_GetLogFileContents
#define RENDERDOC_GetSupportedDeviceProtocols CAPTUREPROBE_GetSupportedDeviceProtocols
#define RENDERDOC_GetVersionString CAPTUREPROBE_GetVersionString
#define RENDERDOC_HalfToFloat CAPTUREPROBE_HalfToFloat
#define RENDERDOC_InitCamera CAPTUREPROBE_InitCamera
#define RENDERDOC_InitialiseReplay CAPTUREPROBE_InitialiseReplay
#define RENDERDOC_InjectIntoProcess CAPTUREPROBE_InjectIntoProcess
#define RENDERDOC_IsGlobalHookActive CAPTUREPROBE_IsGlobalHookActive
#define RENDERDOC_IsReleaseBuild CAPTUREPROBE_IsReleaseBuild
#define RENDERDOC_LogMessage CAPTUREPROBE_LogMessage
#define RENDERDOC_NeedVulkanLayerRegistration CAPTUREPROBE_NeedVulkanLayerRegistration
#define RENDERDOC_NumVerticesPerPrimitive CAPTUREPROBE_NumVerticesPerPrimitive
#define RENDERDOC_OpenCaptureFile CAPTUREPROBE_OpenCaptureFile
#define RENDERDOC_RegisterMemoryRegion CAPTUREPROBE_RegisterMemoryRegion
#define RENDERDOC_ResourceFormatName CAPTUREPROBE_ResourceFormatName
#define RENDERDOC_RunFunctionalTests CAPTUREPROBE_RunFunctionalTests
#define RENDERDOC_RunUnitTests CAPTUREPROBE_RunUnitTests
#define RENDERDOC_SaveConfigSettings CAPTUREPROBE_SaveConfigSettings
#define RENDERDOC_SetColors CAPTUREPROBE_SetColors
#define RENDERDOC_SetConfigSetting CAPTUREPROBE_SetConfigSetting
#define RENDERDOC_SetDebugLogFile CAPTUREPROBE_SetDebugLogFile
#define RENDERDOC_ShutdownReplay CAPTUREPROBE_ShutdownReplay
#define RENDERDOC_StartGlobalHook CAPTUREPROBE_StartGlobalHook
#define RENDERDOC_StartSelfHostCapture CAPTUREPROBE_StartSelfHostCapture
#define RENDERDOC_StopGlobalHook CAPTUREPROBE_StopGlobalHook
#define RENDERDOC_UnregisterMemoryRegion CAPTUREPROBE_UnregisterMemoryRegion
#define RENDERDOC_UpdateInstalledVersionNumber CAPTUREPROBE_UpdateInstalledVersionNumber
#define RENDERDOC_UpdateVulkanLayerRegistration CAPTUREPROBE_UpdateVulkanLayerRegistration
#define RENDERDOC_VertexOffset CAPTUREPROBE_VertexOffset

#endif
