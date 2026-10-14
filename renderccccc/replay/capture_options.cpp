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

#include "api/replay/capture_options.h"
#include <float.h>
#include "api/app/renderccccc_app.h"
#include "common/common.h"
#include "core/core.h"

int RENDERCCCCC_CC SetCaptureOptionU32(RENDERCCCCC_CaptureOption opt, uint32_t val)
{
  CaptureOptions opts = Renderccccc::Inst().GetCaptureOptions();

  switch(opt)
  {
    case eRENDERCCCCC_Option_AllowVSync: opts.allowVSync = (val != 0); break;
    case eRENDERCCCCC_Option_AllowFullscreen: opts.allowFullscreen = (val != 0); break;
    case eRENDERCCCCC_Option_APIValidation: opts.apiValidation = (val != 0); break;
    case eRENDERCCCCC_Option_CaptureCallstacks: opts.captureCallstacks = (val != 0); break;
    case eRENDERCCCCC_Option_CaptureCallstacksOnlyDraws:
      opts.captureCallstacksOnlyActions = (val != 0);
      break;
    case eRENDERCCCCC_Option_DelayForDebugger: opts.delayForDebugger = val; break;
    case eRENDERCCCCC_Option_VerifyBufferAccess: opts.verifyBufferAccess = (val != 0); break;
    case eRENDERCCCCC_Option_HookIntoChildren: opts.hookIntoChildren = (val != 0); break;
    case eRENDERCCCCC_Option_RefAllResources: opts.refAllResources = (val != 0); break;
    case eRENDERCCCCC_Option_SaveAllInitials:
      // option is deprecated
      break;
    case eRENDERCCCCC_Option_CaptureAllCmdLists: opts.captureAllCmdLists = (val != 0); break;
    case eRENDERCCCCC_Option_DebugOutputMute: opts.debugOutputMute = (val != 0); break;
    case eRENDERCCCCC_Option_AllowUnsupportedVendorExtensions:
      if(val == 0x10DE)
        Renderccccc::Inst().EnableVendorExtensions(VendorExtensions::NvAPI);
      else
        RDCWARN("AllowUnsupportedVendorExtensions unexpected parameter %x", val);
      break;
    case eRENDERCCCCC_Option_SoftMemoryLimit: opts.softMemoryLimit = val; break;
    case eRENDERCCCCC_Option_HookMode: opts.hookMode = (val != 0); break;
    case eRENDERCCCCC_Option_DllMode: opts.dllMode = (val != 0); break;
    case eRENDERCCCCC_Option_DllModeWait: opts.dllModeWait = (val != 0); break;
    default: RDCLOG("Unrecognised capture option '%d'", opt); return 0;
  }

  Renderccccc::Inst().SetCaptureOptions(opts);
  return 1;
}

int RENDERCCCCC_CC SetCaptureOptionF32(RENDERCCCCC_CaptureOption opt, float val)
{
  CaptureOptions opts = Renderccccc::Inst().GetCaptureOptions();

  switch(opt)
  {
    case eRENDERCCCCC_Option_AllowVSync: opts.allowVSync = (val != 0.0f); break;
    case eRENDERCCCCC_Option_AllowFullscreen: opts.allowFullscreen = (val != 0.0f); break;
    case eRENDERCCCCC_Option_APIValidation: opts.apiValidation = (val != 0.0f); break;
    case eRENDERCCCCC_Option_CaptureCallstacks: opts.captureCallstacks = (val != 0.0f); break;
    case eRENDERCCCCC_Option_CaptureCallstacksOnlyDraws:
      opts.captureCallstacksOnlyActions = (val != 0.0f);
      break;
    case eRENDERCCCCC_Option_DelayForDebugger: opts.delayForDebugger = (uint32_t)val; break;
    case eRENDERCCCCC_Option_VerifyBufferAccess: opts.verifyBufferAccess = (val != 0.0f); break;
    case eRENDERCCCCC_Option_HookIntoChildren: opts.hookIntoChildren = (val != 0.0f); break;
    case eRENDERCCCCC_Option_RefAllResources: opts.refAllResources = (val != 0.0f); break;
    case eRENDERCCCCC_Option_SaveAllInitials:
      // option is deprecated
      break;
    case eRENDERCCCCC_Option_CaptureAllCmdLists: opts.captureAllCmdLists = (val != 0.0f); break;
    case eRENDERCCCCC_Option_DebugOutputMute: opts.debugOutputMute = (val != 0.0f); break;
    case eRENDERCCCCC_Option_AllowUnsupportedVendorExtensions:
      RDCWARN("AllowUnsupportedVendorExtensions unexpected parameter %f", val);
      break;
    case eRENDERCCCCC_Option_SoftMemoryLimit: opts.softMemoryLimit = (uint32_t)val; break;
    case eRENDERCCCCC_Option_HookMode: opts.hookMode = (val != 0.0f); break;
    case eRENDERCCCCC_Option_DllMode: opts.dllMode = (val != 0.0f); break;
    case eRENDERCCCCC_Option_DllModeWait: opts.dllModeWait = (val != 0.0f); break;
    default: RDCLOG("Unrecognised capture option '%d'", opt); return 0;
  }

  Renderccccc::Inst().SetCaptureOptions(opts);
  return 1;
}

uint32_t RENDERCCCCC_CC GetCaptureOptionU32(RENDERCCCCC_CaptureOption opt)
{
  switch(opt)
  {
    case eRENDERCCCCC_Option_AllowVSync:
      return (Renderccccc::Inst().GetCaptureOptions().allowVSync ? 1 : 0);
    case eRENDERCCCCC_Option_AllowFullscreen:
      return (Renderccccc::Inst().GetCaptureOptions().allowFullscreen ? 1 : 0);
    case eRENDERCCCCC_Option_APIValidation:
      return (Renderccccc::Inst().GetCaptureOptions().apiValidation ? 1 : 0);
    case eRENDERCCCCC_Option_CaptureCallstacks:
      return (Renderccccc::Inst().GetCaptureOptions().captureCallstacks ? 1 : 0);
    case eRENDERCCCCC_Option_CaptureCallstacksOnlyDraws:
      return (Renderccccc::Inst().GetCaptureOptions().captureCallstacksOnlyActions ? 1 : 0);
    case eRENDERCCCCC_Option_DelayForDebugger:
      return (Renderccccc::Inst().GetCaptureOptions().delayForDebugger);
    case eRENDERCCCCC_Option_VerifyBufferAccess:
      return (Renderccccc::Inst().GetCaptureOptions().verifyBufferAccess ? 1 : 0);
    case eRENDERCCCCC_Option_HookIntoChildren:
      return (Renderccccc::Inst().GetCaptureOptions().hookIntoChildren ? 1 : 0);
    case eRENDERCCCCC_Option_RefAllResources:
      return (Renderccccc::Inst().GetCaptureOptions().refAllResources ? 1 : 0);
    case eRENDERCCCCC_Option_SaveAllInitials:
      // option is deprecated - always enabled
      return 1;
    case eRENDERCCCCC_Option_CaptureAllCmdLists:
      return (Renderccccc::Inst().GetCaptureOptions().captureAllCmdLists ? 1 : 0);
    case eRENDERCCCCC_Option_DebugOutputMute:
      return (Renderccccc::Inst().GetCaptureOptions().debugOutputMute ? 1 : 0);
    case eRENDERCCCCC_Option_AllowUnsupportedVendorExtensions: return 0;
    case eRENDERCCCCC_Option_SoftMemoryLimit:
      return (Renderccccc::Inst().GetCaptureOptions().softMemoryLimit);
    case eRENDERCCCCC_Option_HookMode:
      return (Renderccccc::Inst().GetCaptureOptions().hookMode ? 1 : 0);
    case eRENDERCCCCC_Option_DllMode:
      return (Renderccccc::Inst().GetCaptureOptions().dllMode ? 1 : 0);
    case eRENDERCCCCC_Option_DllModeWait:
      return (Renderccccc::Inst().GetCaptureOptions().dllModeWait ? 1 : 0);
    default: break;
  }

  RDCLOG("Unrecognised capture option '%d'", opt);
  return 0xffffffff;
}

float RENDERCCCCC_CC GetCaptureOptionF32(RENDERCCCCC_CaptureOption opt)
{
  switch(opt)
  {
    case eRENDERCCCCC_Option_AllowVSync:
      return (Renderccccc::Inst().GetCaptureOptions().allowVSync ? 1.0f : 0.0f);
    case eRENDERCCCCC_Option_AllowFullscreen:
      return (Renderccccc::Inst().GetCaptureOptions().allowFullscreen ? 1.0f : 0.0f);
    case eRENDERCCCCC_Option_APIValidation:
      return (Renderccccc::Inst().GetCaptureOptions().apiValidation ? 1.0f : 0.0f);
    case eRENDERCCCCC_Option_CaptureCallstacks:
      return (Renderccccc::Inst().GetCaptureOptions().captureCallstacks ? 1.0f : 0.0f);
    case eRENDERCCCCC_Option_CaptureCallstacksOnlyDraws:
      return (Renderccccc::Inst().GetCaptureOptions().captureCallstacksOnlyActions ? 1.0f : 0.0f);
    case eRENDERCCCCC_Option_DelayForDebugger:
      return (Renderccccc::Inst().GetCaptureOptions().delayForDebugger * 1.0f);
    case eRENDERCCCCC_Option_VerifyBufferAccess:
      return (Renderccccc::Inst().GetCaptureOptions().verifyBufferAccess ? 1.0f : 0.0f);
    case eRENDERCCCCC_Option_HookIntoChildren:
      return (Renderccccc::Inst().GetCaptureOptions().hookIntoChildren ? 1.0f : 0.0f);
    case eRENDERCCCCC_Option_RefAllResources:
      return (Renderccccc::Inst().GetCaptureOptions().refAllResources ? 1.0f : 0.0f);
    case eRENDERCCCCC_Option_SaveAllInitials:
      // option is deprecated - always enabled
      return 1.0f;
    case eRENDERCCCCC_Option_CaptureAllCmdLists:
      return (Renderccccc::Inst().GetCaptureOptions().captureAllCmdLists ? 1.0f : 0.0f);
    case eRENDERCCCCC_Option_DebugOutputMute:
      return (Renderccccc::Inst().GetCaptureOptions().debugOutputMute ? 1.0f : 0.0f);
    case eRENDERCCCCC_Option_AllowUnsupportedVendorExtensions: return 0.0f;
    case eRENDERCCCCC_Option_SoftMemoryLimit:
      return (Renderccccc::Inst().GetCaptureOptions().softMemoryLimit * 1.0f);
    case eRENDERCCCCC_Option_HookMode:
      return (Renderccccc::Inst().GetCaptureOptions().hookMode ? 1.0f : 0.0f);
    case eRENDERCCCCC_Option_DllMode:
      return (Renderccccc::Inst().GetCaptureOptions().dllMode ? 1.0f : 0.0f);
    case eRENDERCCCCC_Option_DllModeWait:
      return (Renderccccc::Inst().GetCaptureOptions().dllModeWait ? 1.0f : 0.0f);
    default: break;
  }

  RDCLOG("Unrecognised capture option '%d'", opt);
  return -FLT_MAX;
}

#if ENABLED(ENABLE_UNIT_TESTS)

#undef None
#undef Always

#include "catch/catch.hpp"

TEST_CASE("Check CaptureOptions de/serialise to string", "[serialise]")
{
  CaptureOptions opts;

  bool *boolOpts[] = {
      &opts.allowVSync,
      &opts.allowFullscreen,
      &opts.apiValidation,
      &opts.captureCallstacks,
      &opts.captureCallstacksOnlyActions,
      &opts.verifyBufferAccess,
      &opts.hookIntoChildren,
      &opts.refAllResources,
      &opts.captureAllCmdLists,
      &opts.debugOutputMute,
      &opts.hookMode,
      &opts.dllMode,
      &opts.dllModeWait,
  };

  for(uint32_t delay = 0; delay < 1000; delay++)
  {
    for(uint32_t variant = 0; variant < (1 << ARRAY_COUNT(boolOpts)); variant++)
    {
      opts.delayForDebugger = delay;
      for(size_t o = 0; o < ARRAY_COUNT(boolOpts); o++)
      {
        *boolOpts[o] = (variant & (1 << o)) != 0;
      }

      rdcstr s = opts.EncodeAsString();
      CaptureOptions decoded;
      decoded.DecodeFromString(s);

      CHECK(memcmp(&opts, &decoded, sizeof(decoded)) == 0);
    }
  }

  // check that nothing explodes here
  CaptureOptions a;
  a.DecodeFromString("");
}

#endif
