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
#if (defined(RENDERCCCCC_PLATFORM_WIN32) || defined(WIN32) || defined(_WIN32) || \
     defined(__WIN32__)) &&                                                     \
    !defined(SWIG) && !defined(RDOC_SELFCAPTURE_LIMITEDAPI)

#define RENDERCCCCC_AllocArrayMem CAPTUREPROBE_AllocArrayMem
#define RENDERCCCCC_BecomeRemoteServer CAPTUREPROBE_BecomeRemoteServer
#define RENDERCCCCC_BeginProfileRegion CAPTUREPROBE_BeginProfileRegion
#define RENDERCCCCC_CanGlobalHook CAPTUREPROBE_CanGlobalHook
#define RENDERCCCCC_CanSelfHostedCapture CAPTUREPROBE_CanSelfHostedCapture
#define RENDERCCCCC_CheckAndroidPackage CAPTUREPROBE_CheckAndroidPackage
#define RENDERCCCCC_CheckRemoteServerConnection CAPTUREPROBE_CheckRemoteServerConnection
#define RENDERCCCCC_CreateBugReport CAPTUREPROBE_CreateBugReport
#define RENDERCCCCC_CreateRemoteServerConnection CAPTUREPROBE_CreateRemoteServerConnection
#define RENDERCCCCC_CreateTargetControl CAPTUREPROBE_CreateTargetControl
#define RENDERCCCCC_EndProfileRegion CAPTUREPROBE_EndProfileRegion
#define RENDERCCCCC_EndSelfHostCapture CAPTUREPROBE_EndSelfHostCapture
#define RENDERCCCCC_EnumerateRemoteTargets CAPTUREPROBE_EnumerateRemoteTargets
#define RENDERCCCCC_ExecuteAndInject CAPTUREPROBE_ExecuteAndInject
#define RENDERCCCCC_FloatToHalf CAPTUREPROBE_FloatToHalf
#define RENDERCCCCC_FreeArrayMem CAPTUREPROBE_FreeArrayMem
#define RENDERCCCCC_GetAPI CAPTUREPROBE_GetAPI
#define RENDERCCCCC_GetCommitHash CAPTUREPROBE_GetCommitHash
#define RENDERCCCCC_GetConfigSetting CAPTUREPROBE_GetConfigSetting
#define RENDERCCCCC_GetCurrentProcessMemoryUsage CAPTUREPROBE_GetCurrentProcessMemoryUsage
#define RENDERCCCCC_GetDeviceProtocolController CAPTUREPROBE_GetDeviceProtocolController
#define RENDERCCCCC_GetDriverInformation CAPTUREPROBE_GetDriverInformation
#define RENDERCCCCC_GetLogFile CAPTUREPROBE_GetLogFile
#define RENDERCCCCC_GetLogFileContents CAPTUREPROBE_GetLogFileContents
#define RENDERCCCCC_GetSupportedDeviceProtocols CAPTUREPROBE_GetSupportedDeviceProtocols
#define RENDERCCCCC_GetVersionString CAPTUREPROBE_GetVersionString
#define RENDERCCCCC_HalfToFloat CAPTUREPROBE_HalfToFloat
#define RENDERCCCCC_InitCamera CAPTUREPROBE_InitCamera
#define RENDERCCCCC_InitialiseReplay CAPTUREPROBE_InitialiseReplay
#define RENDERCCCCC_InjectIntoProcess CAPTUREPROBE_InjectIntoProcess
#define RENDERCCCCC_IsGlobalHookActive CAPTUREPROBE_IsGlobalHookActive
#define RENDERCCCCC_IsReleaseBuild CAPTUREPROBE_IsReleaseBuild
#define RENDERCCCCC_LogMessage CAPTUREPROBE_LogMessage
#define RENDERCCCCC_NeedVulkanLayerRegistration CAPTUREPROBE_NeedVulkanLayerRegistration
#define RENDERCCCCC_NumVerticesPerPrimitive CAPTUREPROBE_NumVerticesPerPrimitive
#define RENDERCCCCC_OpenCaptureFile CAPTUREPROBE_OpenCaptureFile
#define RENDERCCCCC_RegisterMemoryRegion CAPTUREPROBE_RegisterMemoryRegion
#define RENDERCCCCC_ResourceFormatName CAPTUREPROBE_ResourceFormatName
#define RENDERCCCCC_RunFunctionalTests CAPTUREPROBE_RunFunctionalTests
#define RENDERCCCCC_RunUnitTests CAPTUREPROBE_RunUnitTests
#define RENDERCCCCC_SaveConfigSettings CAPTUREPROBE_SaveConfigSettings
#define RENDERCCCCC_SetColors CAPTUREPROBE_SetColors
#define RENDERCCCCC_SetConfigSetting CAPTUREPROBE_SetConfigSetting
#define RENDERCCCCC_SetDebugLogFile CAPTUREPROBE_SetDebugLogFile
#define RENDERCCCCC_ShutdownReplay CAPTUREPROBE_ShutdownReplay
#define RENDERCCCCC_StartGlobalHook CAPTUREPROBE_StartGlobalHook
#define RENDERCCCCC_StartSelfHostCapture CAPTUREPROBE_StartSelfHostCapture
#define RENDERCCCCC_StopGlobalHook CAPTUREPROBE_StopGlobalHook
#define RENDERCCCCC_UnregisterMemoryRegion CAPTUREPROBE_UnregisterMemoryRegion
#define RENDERCCCCC_UpdateInstalledVersionNumber CAPTUREPROBE_UpdateInstalledVersionNumber
#define RENDERCCCCC_UpdateVulkanLayerRegistration CAPTUREPROBE_UpdateVulkanLayerRegistration
#define RENDERCCCCC_VertexOffset CAPTUREPROBE_VertexOffset

#endif
