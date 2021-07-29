#pragma once
#include "CaptureCard.Controller.g.h"
#include "CaptureCard.Controller.h"

namespace winrt::CaptureCard::implementation
{
    class IMicrosoftCaptureBoard;

    struct SampleDisplayCapture : implements<SampleDisplayCapture, IDisplayCapture>
    {
        SampleDisplayCapture(std::shared_ptr<IMicrosoftCaptureBoard> singleCapture);

        void CompareCaptureToReference(hstring name, DisplayStateReference::IStaticReference reference);
        void SaveCaptureToDisk(hstring path);

    private:
        hstring ComputeHashedFileName(hstring testName);
        winrt::Windows::Graphics::Imaging::SoftwareBitmap LoadComparisonImage(hstring name);
        void SaveMemoryToBitmap(hstring name);
        void SaveMismatchedImage(hstring name, winrt::Windows::Graphics::Imaging::SoftwareBitmap bitmap);
        static winrt::Windows::Storage::StorageFolder LoadFolder(hstring subFolder);

    private:
        // The folder in which to look for test data
        winrt::Windows::Storage::StorageFolder m_testDataFolder, m_mismatchFolder;
        std::shared_ptr<IMicrosoftCaptureBoard> m_captureBoard;
    };
}