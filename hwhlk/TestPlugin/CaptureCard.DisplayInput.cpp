#include "pch.h"
#include "CaptureCard.DisplayInput.h"
#include "CaptureCard.DisplayInput.g.cpp"

namespace winrt::CaptureCard::implementation
{
    hstring DisplayInput::Name()
    {
        throw hresult_not_implemented();
    }
    void DisplayInput::Name(hstring const& value)
    {
        throw hresult_not_implemented();
    }
    Windows::Devices::Display::Core::DisplayTarget DisplayInput::MapCaptureInputToDisplayPath()
    {
        throw hresult_not_implemented();
    }
    CaptureCard::CaptureCapabilities DisplayInput::GetCapabilities()
    {
        throw hresult_not_implemented();
    }
    CaptureCard::DisplayCapture DisplayInput::CaptureFrame(CaptureCard::CaptureTrigger const& trigger)
    {
        throw hresult_not_implemented();
    }
}
