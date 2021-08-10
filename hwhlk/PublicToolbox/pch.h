﻿#pragma once

#include <MemoryBuffer.h>

#include <unknwn.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Devices.Display.h>
#include <winrt/Windows.Devices.Display.Core.h>

#include "ConfigurationToolbox.h"
#include "winrt/MicrosoftDisplayCaptureTools.ConfigurationTools.h"
#include "winrt/MicrosoftDisplayCaptureTools.DisplayStateReference.h"

#include <WexTestClass.h>
#include <Wex.Common.h>
#include <Wex.Logger.h>
#include <WexString.h>

// Include the various tools
#include "PatternTool.h"
#include "WireFormatTool.h"
#include "ResolutionTool.h"
