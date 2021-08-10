#pragma once

namespace winrt::Toolbox::implementation
{
    enum class PatternToolConfigurations
    {
        Black,
        White,
        Red,
        Green,
        Blue
    };

    struct PatternTool : implements<PatternTool, MicrosoftDisplayCaptureTools::ConfigurationTools::IConfigurationTool>
    {
        PatternTool();

        hstring Name();
        MicrosoftDisplayCaptureTools::ConfigurationTools::ConfigurationToolCategory Category();
        MicrosoftDisplayCaptureTools::ConfigurationTools::ConfigurationToolRequirements Requirements();
        com_array<hstring> GetSupportedConfigurations();
        void SetConfiguration(hstring const& configuration);
        void ApplyToHardware(Windows::Devices::Display::Core::DisplayTarget const& target);
        void ApplyToReference(MicrosoftDisplayCaptureTools::DisplayStateReference::IStaticReference const& reference);

    private:
        void ApplyToSoftwareReferenceFallback(MicrosoftDisplayCaptureTools::DisplayStateReference::IStaticReference const& reference);
        PatternToolConfigurations m_currentConfig;
    };
}