#include "pch.h"
#include <filesystem>

namespace winrt 
{
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Foundation::Collections;
    using namespace winrt::Windows::Devices::Enumeration;
    using namespace winrt::Windows::Devices::Usb;
    using namespace winrt::Windows::Graphics::Imaging;
    using namespace winrt::Windows::Graphics::DirectX;
    using namespace winrt::Windows::Devices::Display;
    using namespace winrt::Windows::Devices::Display::Core;
    using namespace winrt::Windows::Storage;
    using namespace winrt::Windows::Storage::Streams;
    using namespace winrt::MicrosoftDisplayCaptureTools::Framework;
}
using namespace IteIt68051Plugin;

namespace winrt::TanagerPlugin::implementation
{
const unsigned char it68051i2cAddress = 0x48;

TanagerDevice::TanagerDevice(winrt::param::hstring deviceId, winrt::ILogger const& logger) :
    m_logger(logger),
    m_usbDevice(nullptr),
    m_deviceId(deviceId),
    hdmiChip(
        [&](uint8_t address, uint8_t value) // I2C write
        { m_pDriver->writeRegisterByte(it68051i2cAddress, address, value); },
        [&](uint8_t address) // I2C read
        { return m_pDriver->readRegisterByte(it68051i2cAddress, address); })
	{
		m_usbDevice = UsbDevice::FromIdAsync(deviceId).get();
		if (m_usbDevice == nullptr)
		{
			throw_hresult(E_FAIL);
		}
		m_fpga.SetUsbDevice(m_usbDevice);

		m_pDriver = std::make_shared<I2cDriver>(m_usbDevice);

        m_fpga.SysReset(); // Blocks until FPGA is ready

		hdmiChip.Initialize();

        m_logger.LogNote(L"Initializing Tanager Device: " + m_deviceId);
	}

	TanagerDevice::~TanagerDevice()
	{
	}

	std::vector<MicrosoftDisplayCaptureTools::CaptureCard::IDisplayInput> TanagerDevice::EnumerateDisplayInputs()
	{
		return std::vector<MicrosoftDisplayCaptureTools::CaptureCard::IDisplayInput>
		{
			winrt::make<TanagerDisplayInput>(this->weak_from_this(), TanagerDisplayInputPort::hdmi, m_logger),
			winrt::make<TanagerDisplayInput>(this->weak_from_this(), TanagerDisplayInputPort::displayPort, m_logger),
		};
	}

	void TanagerDevice::TriggerHdmiCapture()
	{
        m_logger.LogAssert(L"TriggerHdmiCapture not currently implemented.");
	}

	void TanagerDevice::FpgaWrite(unsigned short address, std::vector<byte> data)
	{
		return m_fpga.Write(address, data);
	}

	std::vector<byte> TanagerDevice::FpgaRead(unsigned short address, UINT16 size)
	{
		return m_fpga.Read(address, size);
	}

	std::vector<byte> TanagerDevice::ReadEndPointData(UINT32 dataSize)
	{
		return m_fpga.ReadEndPointData(dataSize);
	}

	void TanagerDevice::FlashFpgaFirmware(winrt::hstring filePath)
	{
        m_fpga.FlashFpgaFirmware(filePath);
	}

	void TanagerDevice::FlashFx3Firmware(winrt::hstring filePath)
	{
        m_fpga.FlashFx3Firmware(filePath);
	}

	FirmwareVersionInfo TanagerDevice::GetFirmwareVersionInfo()
	{
		return m_fpga.GetFirmwareVersionInfo();
	}

    IteIt68051Plugin::VideoTiming TanagerDevice::GetVideoTiming()
    {
        return hdmiChip.GetVideoTiming();
    }

    winrt::hstring TanagerDevice::GetDeviceId()
    {
        return m_deviceId;
    }

    winrt::Windows::Foundation::IAsyncAction TanagerDevice::UpdateFirmwareAsync()
    {
        co_await winrt::resume_background();

        FlashFpgaFirmware(FpgaFirmwareFileName);
        FlashFx3Firmware(Fx3FirmwareFileName);
    }

    MicrosoftDisplayCaptureTools::CaptureCard::ControllerFirmwareState TanagerDevice::GetFirmwareState()
    {
        auto versionInfo = GetFirmwareVersionInfo();
        if (versionInfo.GetFpgaFirmwareVersion() < MinimumFpgaVersion || versionInfo.GetFx3FirmwareVersion() < MinimumFx3Version)
        {
            return MicrosoftDisplayCaptureTools::CaptureCard::ControllerFirmwareState::UpdateRequired;
        }
        else
        {
            return MicrosoftDisplayCaptureTools::CaptureCard::ControllerFirmwareState::UpToDate;
        }
    }

	TanagerDisplayInput::TanagerDisplayInput(std::weak_ptr<TanagerDevice> parent, TanagerDisplayInputPort port, winrt::ILogger const& logger) :
        m_parent(parent), m_port(port), m_logger(logger)
    {
    }

	hstring TanagerDisplayInput::Name()
	{
		switch (m_port)
		{
		case TanagerDisplayInputPort::hdmi:
			return L"HDMI";
		case TanagerDisplayInputPort::displayPort:
			return L"DisplayPort";
        default: 
            m_logger.LogAssert(L"Invalid port chosen.");
            return L"";
		}
    }

    void TanagerDisplayInput::SetDescriptor(MicrosoftDisplayCaptureTools::Framework::IMonitorDescriptor descriptor)
    {
        switch (m_port)
        {
        case TanagerDisplayInputPort::hdmi:
        {
            if (descriptor.Type() != MicrosoftDisplayCaptureTools::Framework::MonitorDescriptorType::EDID)
            {
                m_logger.LogAssert(L"Only EDID descriptors are currently supported.");
            }

            auto edidDataView = descriptor.Data();
            std::vector<byte> edid(edidDataView.begin(), edidDataView.end());

            SetEdid(edid);
            break;
        }
        case TanagerDisplayInputPort::displayPort:
        default:
        {
            m_logger.LogAssert(L"DisplayPort input does not currently support setting descriptors.");
        }
        }
    }

    MicrosoftDisplayCaptureTools::CaptureCard::ICaptureTrigger TanagerDisplayInput::GetCaptureTrigger()
    {
        return MicrosoftDisplayCaptureTools::CaptureCard::ICaptureTrigger();
    }

	MicrosoftDisplayCaptureTools::CaptureCard::ICaptureCapabilities TanagerDisplayInput::GetCapabilities()
	{
        auto caps = winrt::make<TanagerCaptureCapabilities>(m_port);
        return caps;
    }

    MicrosoftDisplayCaptureTools::CaptureCard::IDisplayCapture TanagerDisplayInput::CaptureFrame()
    {
        auto parent = m_parent.lock();
        if (!parent)
        {
            m_logger.LogAssert(L"Cannot obtain reference to Tanager device.");
        }

        // Capture frame in DRAM
        parent->FpgaWrite(0x20, std::vector<byte>({0}));

        // Give the Tanager time to capture a frame.
        Sleep(17);

        // put video capture logic back in reset
        parent->FpgaWrite(0x20, std::vector<byte>({1}));

        // query resolution
        auto timing = parent->GetVideoTiming();

        // compute size of buffer
        uint32_t bufferSizeInDWords = timing.hActive * timing.vActive; // For now, assume good sync and 4 bytes per pixel

        // FX3 requires the read size to be a multiple of 1024 DWORDs
        if(bufferSizeInDWords % 1024)
        {
            bufferSizeInDWords += 1024 - (bufferSizeInDWords % 1024);
        }

        // specify number of dwords to read
        // This is bufferSizeInDWords but in big-endian order in a vector<byte>
        parent->FpgaWrite(
            0x15,
            std::vector<byte>(
                {(uint8_t)((bufferSizeInDWords >> 24) & 0xff),
                 (uint8_t)((bufferSizeInDWords >> 16) & 0xff),
                 (uint8_t)((bufferSizeInDWords >> 8) & 0xff),
                 (uint8_t)(bufferSizeInDWords & 0xff)}));

        // prepare for reading
        parent->FpgaWrite(0x10, std::vector<byte>({3}));

        // initiate read sequencer
        parent->FpgaWrite(0x10, std::vector<byte>({2}));

        // read frame
        auto frameData = parent->ReadEndPointData(bufferSizeInDWords * 4);

        // turn off read sequencer
        parent->FpgaWrite(0x10, std::vector<byte>({3}));

        // Add any extended properties that aren't directly exposable in the IDisplayCapture* interfaces yet
        auto extendedProps = winrt::multi_threaded_observable_map<winrt::hstring, winrt::IInspectable>();
        extendedProps.Insert(L"pixelClock",  winrt::box_value(timing.pixelClock));
        extendedProps.Insert(L"hTotal",      winrt::box_value(timing.hTotal));
        extendedProps.Insert(L"hFrontPorch", winrt::box_value(timing.hFrontPorch));
        extendedProps.Insert(L"hSyncWidth",  winrt::box_value(timing.hSyncWidth));
        extendedProps.Insert(L"hBackPorch",  winrt::box_value(timing.hBackPorch));
        extendedProps.Insert(L"vTotal",      winrt::box_value(timing.vTotal));
        extendedProps.Insert(L"vFrontPorch", winrt::box_value(timing.vFrontPorch));
        extendedProps.Insert(L"vSyncWidth",  winrt::box_value(timing.vSyncWidth));
        extendedProps.Insert(L"vBackPorch",  winrt::box_value(timing.vBackPorch));
        
        auto resolution = winrt::Windows::Graphics::SizeInt32();
        resolution = { timing.hActive, timing.vActive };

        return winrt::make<TanagerDisplayCapture>(frameData, resolution, extendedProps, m_logger);
    }

	void TanagerDisplayInput::FinalizeDisplayState()
    {
        if (auto parent = m_parent.lock())
        {
            parent->FpgaWrite(0x4, std::vector<byte>({0x34})); // HPD high
        }
        else
        {
            m_logger.LogAssert(L"Cannot obtain reference to Tanager object.");
        }

        Sleep(1000);
    }

    void TanagerDisplayInput::SetEdid(std::vector<byte> edid)
    {
        // EDIDs are made of a series of 128-byte blocks
        if (edid.empty() || edid.size() % 128 !=0)
        {
            m_logger.LogError(L"SetEdid provided edid of invalid size=" + to_hstring(edid.size()));
        }

        unsigned short writeAddress;
        switch (m_port)
        {
            case TanagerDisplayInputPort::hdmi:
                writeAddress = 0x400;
                break;
            default:
            case TanagerDisplayInputPort::displayPort:
                m_logger.LogAssert(L"DisplayPort input of Tanager does not currently support setting EDIDs.");
                return;
        }

        if (auto parent = m_parent.lock())
        {
            parent->FpgaWrite(writeAddress, edid);
        }
        else
        {
            m_logger.LogAssert(L"Cannot obtain reference to Tanager object.");
        }
    }


	bool TanagerCaptureCapabilities::CanReturnRawFramesToHost()
    {
        return true;
    }

    bool TanagerCaptureCapabilities::CanReturnFramesToHost()
    {
        return true;
    }

    bool TanagerCaptureCapabilities::CanCaptureFrameSeries()
    {
        return false;
    }

    bool TanagerCaptureCapabilities::CanHotPlug()
    {
        return true;
    }

    bool TanagerCaptureCapabilities::CanConfigureEDID()
    {
        switch (m_port)
		{
		case TanagerDisplayInputPort::hdmi:
			return true;
		case TanagerDisplayInputPort::displayPort:
			return false;
        }

        return false;
    }

    bool TanagerCaptureCapabilities::CanConfigureDisplayID()
    {
        return false;
    }

    uint32_t TanagerCaptureCapabilities::GetMaxDescriptorSize()
    {
        switch (m_port)
		{
		case TanagerDisplayInputPort::hdmi:
			return 1024;
		case TanagerDisplayInputPort::displayPort:
        default:
			throw winrt::hresult_not_implemented();
		}
    }

    TanagerDisplayCapture::TanagerDisplayCapture(
        std::vector<byte> rawCaptureData,
        winrt::Windows::Graphics::SizeInt32 resolution,
        winrt::IMap<winrt::hstring, winrt::IInspectable> extendedProps,
        winrt::ILogger const& logger) :
        m_extendedProps(extendedProps), m_logger(logger)
    {
        if (rawCaptureData.size() == 0)
        {
            throw hresult_invalid_argument();
        }

        m_frameData = FrameData(m_logger);

        // TODO: isolate this into a header supporting different masks
        typedef struct
        {
            uint64_t pad1 : 2;
            uint64_t red1 : 8;
            uint64_t pad2 : 2;
            uint64_t green1 : 8;
            uint64_t pad3 : 2;
            uint64_t blue1 : 8;
            uint64_t pad4 : 2;
            uint64_t red2 : 8;
            uint64_t pad5 : 2;
            uint64_t green2 : 8;
            uint64_t pad6 : 2;
            uint64_t blue2 : 8;
            uint64_t rsvd : 4;
        } rgbDataType;

        auto pixelDataWriter = DataWriter();
        rgbDataType* rgbData = (rgbDataType*)rawCaptureData.data();
        while ((void*)rgbData < (void*)(rawCaptureData.data() + rawCaptureData.size()))
        {
            pixelDataWriter.WriteByte(rgbData->red1);
            pixelDataWriter.WriteByte(rgbData->green1);
            pixelDataWriter.WriteByte(rgbData->blue1);
            pixelDataWriter.WriteByte(0xFF); // Alpha padding
            pixelDataWriter.WriteByte(rgbData->red2);
            pixelDataWriter.WriteByte(rgbData->green2);
            pixelDataWriter.WriteByte(rgbData->blue2);
            pixelDataWriter.WriteByte(0xFF); // Alpha padding
            rgbData++;
        }

        m_frameData.Data(pixelDataWriter.DetachBuffer());
        m_frameData.Resolution(resolution);

        // This is only supporting 8bpc RGB444 currently
        FrameDataDescription desc{0};
        desc.BitsPerPixel = 24;
        desc.Stride = resolution.Width * 3; // There is no padding with this capture
        desc.PixelFormat = DirectXPixelFormat::Unknown; // Specify that we don't have an exact match to the input DirectX formats
        desc.PixelEncoding = DisplayWireFormatPixelEncoding::Rgb444;
        desc.Eotf = DisplayWireFormatEotf::Sdr;

        m_frameData.FormatDescription(desc);
    }

    bool TanagerDisplayCapture::CompareCaptureToPrediction(winrt::hstring name, winrt::MicrosoftDisplayCaptureTools::Display::IDisplayPredictionData prediction)
    {
        auto predictedFrameData = prediction.FrameData();

        if (predictedFrameData.Resolution().Height != m_frameData.Resolution().Height || 
            predictedFrameData.Resolution().Width != m_frameData.Resolution().Width)
        {
            m_logger.LogError(winrt::hstring(L"Predicted resolution (") + 
                to_hstring(predictedFrameData.Resolution().Width) + L"," +
                to_hstring(predictedFrameData.Resolution().Height) + L"), Captured Resolution(" + 
                to_hstring(m_frameData.Resolution().Width) + L"," +
                to_hstring(m_frameData.Resolution().Height) + L")");
        }

        auto captureBuffer = m_frameData.Data();
        auto predictBuffer = predictedFrameData.Data();

        if (captureBuffer.Length() < predictBuffer.Length())
        {
            m_logger.LogError(
                winrt::hstring(L"Capture should be at least as large as prediction") + std::to_wstring(captureBuffer.Length()) +
                L", Predicted=" + std::to_wstring(predictBuffer.Length()));
        }
        else if (0 == memcmp(captureBuffer.data(), predictBuffer.data(), predictBuffer.Length()))
        {
            m_logger.LogNote(L"Capture and Prediction perfectly match!");
        }
        else
        {
            m_logger.LogWarning(L"Capture did not exactly match prediction! Attempting comparison with tolerance.");
            {
                auto filename = name + L"_Captured.bin";
                auto folder = winrt::StorageFolder::GetFolderFromPathAsync(std::filesystem::current_path().c_str()).get();
                auto file = folder.CreateFileAsync(filename, winrt::CreationCollisionOption::GenerateUniqueName).get();
                auto stream = file.OpenAsync(winrt::FileAccessMode::ReadWrite).get();
                stream.WriteAsync(captureBuffer).get();
                stream.FlushAsync().get();
                stream.Close();

                m_logger.LogNote(L"Dumping captured data here: " + filename);
            }
            {
                auto filename = name + L"_Predicted.bin";
                auto folder = winrt::StorageFolder::GetFolderFromPathAsync(std::filesystem::current_path().c_str()).get();
                auto file = folder.CreateFileAsync(filename, winrt::CreationCollisionOption::GenerateUniqueName).get();
                auto stream = file.OpenAsync(winrt::FileAccessMode::ReadWrite).get();
                stream.WriteAsync(predictBuffer).get();
                stream.FlushAsync().get();
                stream.Close();

                m_logger.LogNote(L"Dumping captured data here: " + filename);
            }

            struct PixelStruct
            {
                uint8_t r, g, b, a;
            };

            auto differenceCount = 0;

            PixelStruct* cap = reinterpret_cast<PixelStruct*>(captureBuffer.data());
            PixelStruct* pre = reinterpret_cast<PixelStruct*>(predictBuffer.data());

            // Comparing pixel for pixel takes a very long time at the moment - so let's compare stochastically
            const int samples = 10000;
            const int pixelCount = m_frameData.Resolution().Width * m_frameData.Resolution().Height;
            for (auto i = 0; i < samples; i++)
            {
                auto index = rand() % pixelCount;

                if (cap[index].r != pre[index].r ||
                    cap[index].g != pre[index].g ||
                    cap[index].b != pre[index].b)
                {
                    differenceCount++;
                }
            }

            float diff = (float)differenceCount / (float)pixelCount;
            if (diff > 0.10f)
            {
                std::wstring msg;
                std::format_to(std::back_inserter(msg), "\n\tMatch = %2.2f\n\n", diff);
                m_logger.LogError(msg);

                return false;
            }
        }

        return true;
    }

    winrt::MicrosoftDisplayCaptureTools::Framework::IFrameData TanagerDisplayCapture::GetFrameData()
    {
        return m_frameData;
    }

    winrt::Windows::Foundation::Collections::IMapView<winrt::hstring, winrt::Windows::Foundation::IInspectable> TanagerDisplayCapture::ExtendedProperties()
    {
        return m_extendedProps.GetView();
    }

} // namespace winrt::TanagerPlugin::implementation
