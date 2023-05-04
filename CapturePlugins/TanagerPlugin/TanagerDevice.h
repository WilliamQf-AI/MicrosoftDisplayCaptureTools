#pragma once

namespace winrt::TanagerPlugin::implementation
{
    // This is a temporary limit while we're bringing up some of the hardware on board.
    constexpr uint32_t MaxDescriptorByteSize = 512;

    class TanagerDevice :
        public IMicrosoftCaptureBoard,
        public std::enable_shared_from_this<TanagerDevice>
    {
        inline static constinit const PCWSTR FpgaFirmwareFileName = L"TanagerFpgaFirmware.bin";
        inline static constinit const PCWSTR Fx3FirmwareFileName = L"TanagerFx3Firmware.bin";
        inline static constinit const std::tuple<uint8_t, uint8_t, uint8_t> MinimumFpgaVersion{(uint8_t)0, (uint8_t)0, (uint8_t)0};
        inline static constinit const std::tuple<uint8_t, uint8_t, uint8_t> MinimumFx3Version{(uint8_t)0, (uint8_t)0, (uint8_t)0};

    public:
        TanagerDevice(winrt::param::hstring deviceId, winrt::MicrosoftDisplayCaptureTools::Framework::ILogger const& logger);
        ~TanagerDevice();

        winrt::hstring GetDeviceId() override;
        std::vector<MicrosoftDisplayCaptureTools::CaptureCard::IDisplayInput> EnumerateDisplayInputs() override;
        void TriggerHdmiCapture() override;
        void FpgaWrite(unsigned short address, std::vector<byte> data) override;
        std::vector<byte> FpgaRead(unsigned short address, UINT16 data) override;
        std::vector<byte> ReadEndPointData(UINT32 dataSize) override;

        // Firmware updates
        void FlashFpgaFirmware(winrt::hstring filePath) override;
        void FlashFx3Firmware(winrt::hstring filePath) override;
        FirmwareVersionInfo GetFirmwareVersionInfo() override;
        winrt::Windows::Foundation::IAsyncAction UpdateFirmwareAsync() override;
        MicrosoftDisplayCaptureTools::CaptureCard::ControllerFirmwareState GetFirmwareState() override;

        IteIt68051Plugin::VideoTiming GetVideoTiming();
        IteIt68051Plugin::aviInfoframe GetAviInfoframe();

        bool IsVideoLocked();

    private:
        winrt::hstring m_deviceId;
        winrt::Windows::Devices::Usb::UsbDevice m_usbDevice;
        std::shared_ptr<I2cDriver> m_pDriver;
        IteIt68051Plugin::IteIt68051 hdmiChip;
        Fx3FpgaInterface m_fpga;

    public:
        // Adding logger as public member as classes use a weak_from_this pattern
        const winrt::MicrosoftDisplayCaptureTools::Framework::ILogger m_logger{nullptr};
    };

    enum class TanagerDisplayInputPort
    {
        hdmi,
        displayPort,
    };

    struct TanagerDisplayInput : implements<TanagerDisplayInput, MicrosoftDisplayCaptureTools::CaptureCard::IDisplayInput>
    {
    public:
        TanagerDisplayInput(
            std::weak_ptr<TanagerDevice> parent,
            TanagerDisplayInputPort port,
            winrt::MicrosoftDisplayCaptureTools::Framework::ILogger const& logger);

        ~TanagerDisplayInput();

        hstring Name();
        void SetDescriptor(MicrosoftDisplayCaptureTools::Framework::IMonitorDescriptor descriptor);
        MicrosoftDisplayCaptureTools::CaptureCard::ICaptureTrigger GetCaptureTrigger();
        MicrosoftDisplayCaptureTools::CaptureCard::ICaptureCapabilities GetCapabilities();
        MicrosoftDisplayCaptureTools::CaptureCard::IDisplayCapture CaptureFrame();
        void FinalizeDisplayState();
        void SetEdid(std::vector<byte> edid);

    private:
        // Members for detecting changes to the display stack - do not take ownership of any devices with this DisplayManager
        winrt::Windows::Foundation::IAsyncAction WaitForDisplayDevicesChange();
        winrt::Windows::Devices::Display::Core::DisplayManager m_displayManager{nullptr};

    private:
        std::weak_ptr<TanagerDevice> m_parent;
        std::shared_ptr<TanagerDevice> m_strongParent;
        TanagerDisplayInputPort m_port;
        std::atomic_bool m_hasDescriptorChanged = false;
        const winrt::MicrosoftDisplayCaptureTools::Framework::ILogger m_logger{nullptr};
    };

    struct CaptureTrigger : implements<CaptureTrigger, winrt::MicrosoftDisplayCaptureTools::CaptureCard::ICaptureTrigger>
    {
    private:
        winrt::MicrosoftDisplayCaptureTools::CaptureCard::CaptureTriggerType m_type{
            winrt::MicrosoftDisplayCaptureTools::CaptureCard::CaptureTriggerType::Immediate};

        uint32_t m_time{0};

        const winrt::MicrosoftDisplayCaptureTools::Framework::ILogger m_logger{nullptr};

    public:
        CaptureTrigger() = default;

        winrt::MicrosoftDisplayCaptureTools::CaptureCard::CaptureTriggerType Type()
        {
            return m_type;
        }
        void Type(winrt::MicrosoftDisplayCaptureTools::CaptureCard::CaptureTriggerType type)
        {
            m_type = type;
        }

        uint32_t TimeToCapture() 
        {
            return m_time;
        }
        void TimeToCapture(uint32_t time)
        {
            m_time = time;
        }
    };

    struct TanagerDisplayCapture : implements<TanagerDisplayCapture, winrt::MicrosoftDisplayCaptureTools::CaptureCard::IDisplayCapture>
    {
        TanagerDisplayCapture(
            std::vector<byte> pixels,
            winrt::Windows::Graphics::SizeInt32 resolution,
            winrt::Windows::Foundation::Collections::IMap<winrt::hstring, winrt::Windows::Foundation::IInspectable> extendedProps,
            winrt::MicrosoftDisplayCaptureTools::Framework::ILogger const& logger);

        bool CompareCaptureToPrediction(winrt::hstring name, winrt::MicrosoftDisplayCaptureTools::Display::IDisplayPredictionData prediction);
        winrt::MicrosoftDisplayCaptureTools::Framework::IFrameData GetFrameData();
        winrt::Windows::Foundation::Collections::IMapView<winrt::hstring, winrt::Windows::Foundation::IInspectable> ExtendedProperties();

    private:
        winrt::MicrosoftDisplayCaptureTools::Framework::FrameData m_frameData{nullptr};
        winrt::Windows::Foundation::Collections::IMap<winrt::hstring, winrt::Windows::Foundation::IInspectable> m_extendedProps{nullptr};

        const winrt::MicrosoftDisplayCaptureTools::Framework::ILogger m_logger{nullptr};
    };

    struct TanagerCaptureCapabilities : implements<TanagerCaptureCapabilities, winrt::MicrosoftDisplayCaptureTools::CaptureCard::ICaptureCapabilities>
    {
        TanagerCaptureCapabilities(TanagerDisplayInputPort Port) : m_port(Port)
        {
        }

        bool CanReturnRawFramesToHost();
        bool CanReturnFramesToHost();
        bool CanCaptureFrameSeries();
        bool CanHotPlug();
        bool CanConfigureEDID();
        bool CanConfigureDisplayID();
        uint32_t GetMaxDescriptorSize();

        TanagerDisplayInputPort m_port;
    };
}
