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
        TanagerDevice(winrt::hstring deviceId);
        ~TanagerDevice();

        winrt::hstring GetDeviceId() override;
        std::vector<MicrosoftDisplayCaptureTools::CaptureCard::IDisplayInput> EnumerateDisplayInputs() override;
        void FpgaWrite(unsigned short address, std::vector<byte> data) override;
        std::vector<byte> FpgaRead(unsigned short address, UINT16 data) override;
        std::vector<byte> ReadEndPointData(UINT32 dataSize) override;
        void SelectDisplayPortEDID(USHORT value);
        void I2cWriteData(uint16_t i2cAddress, uint8_t address, std::vector<byte> data);

        // Firmware updates
        void FlashFpgaFirmware(winrt::hstring filePath) override;
        void FlashFx3Firmware(winrt::hstring filePath) override;
        FirmwareVersionInfo GetFirmwareVersionInfo() override;
        winrt::Windows::Foundation::IAsyncAction UpdateFirmwareAsync() override;
        MicrosoftDisplayCaptureTools::CaptureCard::ControllerFirmwareState GetFirmwareState() override;

        bool IsVideoLocked();
        std::mutex& SelectHdmi();
        std::mutex& SelectDisplayPort();
        IteIt68051Plugin::VideoTiming GetVideoTiming();

        IteIt68051Plugin::aviInfoframe GetAviInfoframe();

    private:
        winrt::hstring m_deviceId;
        winrt::Windows::Devices::Usb::UsbDevice m_usbDevice;
        std::shared_ptr<I2cDriver> m_pDriver;
        IteIt68051Plugin::IteIt68051 hdmiChip;
        Fx3FpgaInterface m_fpga;
        std::mutex m_changingPortsLocked;
    };

    struct TanagerDisplayInputHdmi : implements<TanagerDisplayInputHdmi, MicrosoftDisplayCaptureTools::CaptureCard::IDisplayInput>
    {
    public:
        TanagerDisplayInputHdmi(std::weak_ptr<TanagerDevice> parent);

        ~TanagerDisplayInputHdmi();

        hstring Name();
        void SetDescriptor(MicrosoftDisplayCaptureTools::Framework::IMonitorDescriptor descriptor);
        MicrosoftDisplayCaptureTools::CaptureCard::ICaptureTrigger GetCaptureTrigger();
        MicrosoftDisplayCaptureTools::CaptureCard::ICaptureCapabilities GetCapabilities();
        MicrosoftDisplayCaptureTools::CaptureCard::IDisplayCapture CaptureFrame();
        void FinalizeDisplayState();
        void SetEdid(std::vector<byte> edid);

    private:
        std::weak_ptr<TanagerDevice> m_parent;
        std::shared_ptr<TanagerDevice> m_strongParent;
        std::atomic_bool m_hasDescriptorChanged = false;
    };

    struct TanagerDisplayInputDisplayPort : implements<TanagerDisplayInputDisplayPort, MicrosoftDisplayCaptureTools::CaptureCard::IDisplayInput>
    {
    public:
        TanagerDisplayInputDisplayPort(std::weak_ptr<TanagerDevice> parent);

        ~TanagerDisplayInputDisplayPort();

        hstring Name();
        void SetDescriptor(MicrosoftDisplayCaptureTools::Framework::IMonitorDescriptor descriptor);
        MicrosoftDisplayCaptureTools::CaptureCard::ICaptureTrigger GetCaptureTrigger();
        MicrosoftDisplayCaptureTools::CaptureCard::ICaptureCapabilities GetCapabilities();
        MicrosoftDisplayCaptureTools::CaptureCard::IDisplayCapture CaptureFrame();
        void FinalizeDisplayState();
        void SetEdid(std::vector<byte> edid);

    private:
        std::weak_ptr<TanagerDevice> m_parent;
        std::shared_ptr<TanagerDevice> m_strongParent;
        std::atomic_bool m_hasDescriptorChanged = false;
    };

    struct CaptureTrigger : implements<CaptureTrigger, winrt::MicrosoftDisplayCaptureTools::CaptureCard::ICaptureTrigger>
    {
    private:
        winrt::MicrosoftDisplayCaptureTools::CaptureCard::CaptureTriggerType m_type{
            winrt::MicrosoftDisplayCaptureTools::CaptureCard::CaptureTriggerType::Immediate};

        uint32_t m_time{0};

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
            winrt::Windows::Foundation::Collections::IMap<winrt::hstring, winrt::Windows::Foundation::IInspectable> extendedProps);

        bool CompareCaptureToPrediction(winrt::hstring name, winrt::MicrosoftDisplayCaptureTools::Framework::IRawFrameSet prediction);
        winrt::MicrosoftDisplayCaptureTools::Framework::IRawFrameSet GetFrameData();
        winrt::Windows::Foundation::Collections::IMapView<winrt::hstring, winrt::Windows::Foundation::IInspectable> ExtendedProperties();

    private:
        winrt::MicrosoftDisplayCaptureTools::Framework::IRawFrameSet m_frameSet{nullptr};
        winrt::Windows::Foundation::Collections::IMap<winrt::hstring, winrt::Windows::Foundation::IInspectable> m_extendedProps{nullptr};
    };

    // TODO: move this to a module
    struct Frame
        : winrt::implements<Frame, winrt::MicrosoftDisplayCaptureTools::Framework::IRawFrame, winrt::MicrosoftDisplayCaptureTools::Framework::IRawFrameRenderable>
    {
        Frame();

        // Functions from IRawFrame
        winrt::Windows::Storage::Streams::IBuffer Data();
        winrt::Windows::Devices::Display::Core::DisplayWireFormat DataFormat();
        winrt::Windows::Foundation::Collections::IMap<winrt::hstring, winrt::Windows::Foundation::IInspectable> Properties();
        winrt::Windows::Graphics::SizeInt32 Resolution();

        // Functions from IRawFrameRenderable
        winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Graphics::Imaging::SoftwareBitmap> GetRenderableApproximationAsync();
        winrt::hstring GetPixelInfo(uint32_t x, uint32_t y);

        // Local-only members
        void SetBuffer(winrt::Windows::Storage::Streams::IBuffer data);
        void DataFormat(winrt::Windows::Devices::Display::Core::DisplayWireFormat const& description);
        void Resolution(winrt::Windows::Graphics::SizeInt32 const& resolution);
        void SetImageApproximation(winrt::Windows::Graphics::Imaging::SoftwareBitmap bitmap);

    private:
        winrt::Windows::Storage::Streams::IBuffer m_data{nullptr};
        winrt::Windows::Devices::Display::Core::DisplayWireFormat m_format{nullptr};
        winrt::Windows::Foundation::Collections::IMap<winrt::hstring, winrt::Windows::Foundation::IInspectable> m_properties;
        winrt::Windows::Graphics::SizeInt32 m_resolution{0, 0};

        winrt::Windows::Graphics::Imaging::SoftwareBitmap m_bitmap{nullptr};
    };

    struct FrameSet : winrt::implements<FrameSet, winrt::MicrosoftDisplayCaptureTools::Framework::IRawFrameSet>
    {
        FrameSet();

        winrt::Windows::Foundation::Collections::IVector<winrt::MicrosoftDisplayCaptureTools::Framework::IRawFrame> Frames();
        winrt::Windows::Foundation::Collections::IMap<winrt::hstring, winrt::Windows::Foundation::IInspectable> Properties();

    private:
        winrt::Windows::Foundation::Collections::IVector<winrt::MicrosoftDisplayCaptureTools::Framework::IRawFrame> m_frames;
        winrt::Windows::Foundation::Collections::IMap<winrt::hstring, winrt::Windows::Foundation::IInspectable> m_properties;
    };
}

