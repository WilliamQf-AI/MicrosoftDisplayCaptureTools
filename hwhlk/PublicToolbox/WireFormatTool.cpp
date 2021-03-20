#include "pch.h"
#include "WireFormatTool.h"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

namespace winrt::ConfigurationTools::implementation
{
    WireFormatTool::WireFormatTool()
    {
        // Define the default configuration
        m_currentConfig = WireFormatToolConfigurations::RGB8;
    }

    hstring WireFormatTool::Name()
    {
        return L"WireFormatTool";
    }

    ConfigurationTools::ConfigurationToolCategory WireFormatTool::Category()
    {
        return ConfigurationTools::ConfigurationToolCategory::DisplaySetup;
    }

    ConfigurationTools::ConfigurationToolRequirements WireFormatTool::Requirements()
    {
        auto reqs = ConfigurationTools::ConfigurationToolRequirements();
        reqs.ContributedComparisonTolerance = 0.f;
        reqs.MaxComparisonTolerance = 0.f;

        return reqs;
    }

    std::map<std::wstring, WireFormatToolConfigurations> MapNameToConfiguration =
    {
        {L"RGB8", WireFormatToolConfigurations::RGB8}
    };

    com_array<hstring> WireFormatTool::GetSupportedConfigurations()
    {
        auto configNames = std::vector<hstring>();
        for (auto tool : MapNameToConfiguration)
        {
            configNames.push_back(hstring(tool.first));
        }

        return com_array<hstring>(configNames);
    }

    void WireFormatTool::SetConfiguration(hstring const& configuration)
    {
        m_currentConfig = MapNameToConfiguration[std::wstring(configuration)];
    }

    void WireFormatTool::ApplyToHardware(Windows::Devices::Display::Core::DisplayTarget const& target)
    {
        throw hresult_not_implemented();
    }

    void WireFormatTool::ApplyToSoftwareReference(DisplayStateReference::IStaticReference const& reference)
    {
        auto frameInfo = reference.FrameInfo();

        switch (m_currentConfig)
        {
        case WireFormatToolConfigurations::RGB8:
            frameInfo.pixelFormat = DisplayStateReference::FramePixelFormat::R8G8B8;
            frameInfo.pixelStride = 3; // 24bpp
            break;
        default:
            Log::Error(L"An unsupported resolution has been chosen");
        }

        reference.FrameInfo(frameInfo);
    }
}
