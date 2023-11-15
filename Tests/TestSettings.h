#pragma once

namespace MicrosoftDisplayCaptureTools::Tests {
	inline static const wchar_t ConfigFileRuntimeParameter[] = L"DisplayCaptureConfig";
    inline static const wchar_t DisableFirmwareUpdateRuntimeParameter[] = L"DisableFirmwareUpdate";
    inline static const wchar_t RunPredictionOnlyRuntimeParameter[] = L"OnlyPredictions";
    inline static const wchar_t SynchronizeSavingPredictionToDisk[] = L"SynchronizeSavingPredictionToDisk";

    inline static const wchar_t CaptureBoardInputSourceTableName[] = L"InputName";
} // namespace MicrosoftDisplayCaptureTools::Tests

extern winrt::MicrosoftDisplayCaptureTools::Framework::Core g_framework;
extern winrt::Windows::Foundation::Collections::IVector<winrt::MicrosoftDisplayCaptureTools::Framework::ISourceToSinkMapping> g_displayMap;