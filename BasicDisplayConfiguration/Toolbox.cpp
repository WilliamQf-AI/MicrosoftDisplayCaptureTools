#include "pch.h"
#include "Toolbox.h"
#include "Toolbox.g.cpp"
#include "ToolboxFactory.g.cpp"

#include "PatternTool.h"
#include "ResolutionTool.h"
#include "RefreshRateTool.h"
#include "PixelFormatTool.h"

namespace winrt
{
    using namespace winrt::Windows::Data::Json;
    using namespace winrt::MicrosoftDisplayCaptureTools::ConfigurationTools;
    using namespace winrt::MicrosoftDisplayCaptureTools::Framework;
}

namespace winrt::BasicDisplayConfiguration::implementation 
{
    winrt::IConfigurationToolbox ToolboxFactory::CreateConfigurationToolbox(winrt::ILogger const& logger)
    {
        return winrt::make<Toolbox>(logger);
    }

    Toolbox::Toolbox()
    {
        // Throw - callers should explicitly instantiate through the factory
        throw winrt::hresult_illegal_method_call();
    }

    Toolbox::Toolbox(winrt::ILogger const& logger) : m_logger(logger)
    {
    }

    enum class Tools
    {
        Pattern,
        Resolution,
        RefreshRate,
        PixelFormat
    };

    std::map<hstring, Tools> MapNameToTool =
    {
        {L"Pattern", Tools::Pattern},
        {L"Resolution", Tools::Resolution},
        {L"RefreshRate", Tools::RefreshRate},
        {L"PixelFormat", Tools::PixelFormat}
    };

    hstring Toolbox::Name()
    {
        return L"BasicDisplayConfiguration";
    }
    com_array<hstring> Toolbox::GetSupportedTools()
    {
        auto toolNames = std::vector<hstring>();
        for (auto tool : MapNameToTool)
        {
            toolNames.push_back(tool.first);
        }

        return com_array<hstring>(toolNames);
    }
    IConfigurationTool Toolbox::GetTool(hstring const& toolName)
    {
        switch (MapNameToTool[toolName])
        {
        case Tools::Pattern:
            return winrt::make<PatternTool>(m_logger);
        case Tools::RefreshRate: 
            return winrt::make<RefreshRateTool>(m_logger);
        case Tools::Resolution: 
            return winrt::make<ResolutionTool>(m_logger);
        case Tools::PixelFormat:
            return winrt::make<PixelFormatTool>(m_logger);
        }

        // The caller has asked for a tool that is not exposed from this toolbox
        m_logger.LogAssert(Name() + L"::GetTool was called with invalid tool name: " + toolName);
        throw winrt::hresult_invalid_argument();
    }

    void Toolbox::SetConfigData(IJsonValue data)
    {
    }
}