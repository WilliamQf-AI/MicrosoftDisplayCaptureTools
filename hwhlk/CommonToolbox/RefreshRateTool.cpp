#include "pch.h"
#include "winrt\MicrosoftDisplayCaptureTools.Display.h"
#include "winrt\MicrosoftDisplayCaptureTools.ConfigurationTools.h"
#include "RefreshRateTool.h"

namespace winrt
{
	using namespace MicrosoftDisplayCaptureTools::ConfigurationTools;
	using namespace MicrosoftDisplayCaptureTools::Display;
}

namespace winrt::DisplayConfiguration::implementation
{
	std::map<RefreshRateToolConfigurations, winrt::hstring> RefreshRateConfigurationMap
	{
		{ RefreshRateToolConfigurations::r60, L"60hz" },
		{ RefreshRateToolConfigurations::r50, L"50hz" }
	};

	RefreshRateTool::RefreshRateTool() : m_currentConfig(sc_defaultConfig)
	{
	}

	hstring RefreshRateTool::Name()
	{
		return L"RefreshRate";
	}

	ConfigurationToolCategory RefreshRateTool::Category()
	{
		return ConfigurationToolCategory::DisplaySetup;
	}

	ConfigurationToolRequirements RefreshRateTool::Requirements()
	{
		return nullptr;
	}

	com_array<hstring> RefreshRateTool::GetSupportedConfigurations()
	{
		std::vector<hstring> configs;
		for (auto config : RefreshRateConfigurationMap)
		{
			configs.push_back(config.second);
		}

		return com_array<hstring>(configs);
	}

	hstring RefreshRateTool::GetDefaultConfiguration()
	{
		return RefreshRateConfigurationMap[sc_defaultConfig];
	}

	void RefreshRateTool::SetConfiguration(hstring configuration)
	{
		for (auto config : RefreshRateConfigurationMap)
		{
			if (config.second == configuration)
			{
				m_currentConfig = config.first;
				return;
			}
		}

		// An invalid configuration was asked for
		// TODO: log this case
		throw winrt::hresult_invalid_argument();
	}

	void RefreshRateTool::Apply(IDisplayEngine reference)
	{
		auto displayProperties = reference.GetProperties();

		switch (m_currentConfig)
		{
		case RefreshRateToolConfigurations::r60:
			displayProperties.RefreshRate(60.);
			break;
		case RefreshRateToolConfigurations::r50:
			displayProperties.RefreshRate(50.);
			break;
		}
	}
}