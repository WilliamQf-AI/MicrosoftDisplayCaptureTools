#include "pch.h"
#include <filesystem>
#include <DirectXPackedVector.h>

namespace winrt 
{
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Foundation::Collections;
    using namespace winrt::Windows::Devices::Enumeration;
    using namespace winrt::Windows::Devices::Usb;
    using namespace Windows::Graphics;
    using namespace winrt::Windows::Graphics::Imaging;
    using namespace winrt::Windows::Graphics::DirectX;
    using namespace winrt::Windows::Devices::Display;
    using namespace winrt::Windows::Devices::Display::Core;
    using namespace winrt::Windows::Storage;
    using namespace winrt::Windows::Storage::Streams;
    using namespace winrt::MicrosoftDisplayCaptureTools::Framework;
}

using namespace IteIt68051Plugin;

namespace Shaders
{
    #include "ComputeShaders/RGB_444_8bpc.h"
    #include "ComputeShaders/RGB_444_10bpc.h"
    #include "ComputeShaders/YCbCr_444_8bpc.h"
    #include "ComputeShaders/YCbCr_444_10bpc.h"

    #include "ComputeShaders/scRGB_Bucket_Sum.h"
}

namespace winrt::TanagerPlugin::implementation
{
const unsigned char it68051i2cAddress = 0x48;

TanagerDevice::TanagerDevice(winrt::hstring deviceId) :
    m_usbDevice(nullptr),
    m_deviceId(deviceId),
    hdmiChip(
        [&](uint8_t address, uint8_t value) // I2C write
        { m_pDriver->writeRegisterByte(it68051i2cAddress, address, value); },
        [&](uint8_t address) // I2C read
        { return m_pDriver->readRegisterByte(it68051i2cAddress, address); })
	{
		m_usbDevice = UsbDevice::FromIdAsync(deviceId).get();

		if (!m_usbDevice)
		{
			throw_hresult(E_FAIL);
		}

		m_fpga.SetUsbDevice(m_usbDevice);
		m_pDriver = std::make_shared<I2cDriver>(m_usbDevice);
        m_fpga.SysReset(); // Blocks until FPGA is ready
		hdmiChip.Initialize();

        Logger().LogNote(L"Initializing Tanager Device: " + m_deviceId);
	}

	TanagerDevice::~TanagerDevice()
    {
	}

	std::vector<MicrosoftDisplayCaptureTools::CaptureCard::IDisplayInput> TanagerDevice::EnumerateDisplayInputs()
	{
		return std::vector<MicrosoftDisplayCaptureTools::CaptureCard::IDisplayInput>
		{
			winrt::make<TanagerDisplayInputHdmi>(this->weak_from_this()),
			winrt::make<TanagerDisplayInputDisplayPort>(this->weak_from_this())
		};
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

    void TanagerDevice::SelectDisplayPortEDID(USHORT value)
    {
        m_fpga.SelectDisplayPortEDID(value);
    }

    bool TanagerDevice::IsVideoLocked()
    {
        return hdmiChip.IsVideoLocked();
    }

    void TanagerDevice::I2cWriteData(uint16_t i2cAddress, uint8_t address, std::vector<byte> data)
    {
        // Send this down in chunks
        const uint8_t writeBlockSize = 0x20;
        for (uint8_t i = 0, remaining = data.size(); remaining > 0; i += writeBlockSize, remaining -= min(writeBlockSize, remaining))
        {
            uint8_t amountToWrite = min(writeBlockSize, remaining);
            m_pDriver->writeRegister(i2cAddress, address + i, static_cast<uint32_t>(amountToWrite), data.data() + i);
            Sleep(100);
        }
    }

    std::mutex& TanagerDevice::SelectHdmi()
    {
        hdmiChip.SelectHdmi();
        return m_changingPortsLocked;
    }

    std::mutex& TanagerDevice::SelectDisplayPort()
    {
        hdmiChip.SelectDisplayPort();
        return m_changingPortsLocked;
    }

    IteIt68051Plugin::VideoTiming TanagerDevice::GetVideoTiming()
    {
        return hdmiChip.GetVideoTiming();
    }

    IteIt68051Plugin::aviInfoframe TanagerDevice::GetAviInfoframe()
    {
        return hdmiChip.GetAviInfoframe();
    }

    IteIt68051Plugin::ColorInformation TanagerDevice::GetColorInformation()
    {
        IteIt68051Plugin::ColorInformation colorInformation{};
        hdmiChip.GetColorInformation(&colorInformation);
        return colorInformation;
    }

    std::shared_ptr<TanagerD3D> TanagerDevice::GetD3D()
    {
        if (!m_d3d)
        {
            winrt::com_ptr<ID3D11Device> device;
            winrt::com_ptr<ID3D11DeviceContext> context;
            bool useWarp = !RuntimeSettings().GetSettingValueAsBool(L"RenderOnHardware");
            D3D_DRIVER_TYPE driverType = useWarp ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_HARDWARE;

            UINT uCreationFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;
            D3D_FEATURE_LEVEL flOut;

            static const D3D_FEATURE_LEVEL flvl[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_11_1};
            winrt::check_hresult(D3D11CreateDevice(
                nullptr,
                driverType,
                nullptr,
                uCreationFlags,
                flvl,
                ARRAYSIZE(flvl),
                D3D11_SDK_VERSION,
                device.put(),
                &flOut,
                context.put()));

            m_d3d = std::make_shared<TanagerD3D>(device, context);
		}

        return m_d3d;
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

    TanagerDisplayCapture::TanagerDisplayCapture(
        std::shared_ptr<TanagerD3D> D3DInstance,
        std::vector<byte> rawCaptureData,
        winrt::Windows::Graphics::SizeInt32 resolution,
        winrt::IMap<winrt::hstring, winrt::IInspectable> extendedProps) :
        m_extendedProps(extendedProps), m_D3DInstance(D3DInstance)
    {
        if (rawCaptureData.size() == 0)
        {
            throw hresult_invalid_argument();
        }

        m_frames = winrt::single_threaded_vector<winrt::IRawFrame>();

        // Retrieve the infoframe from the extended properties
        auto infoframe = extendedProps.Lookup(L"infoframe").as<winrt::Buffer>();

        auto buffer = winrt::Windows::Storage::Streams::Buffer(rawCaptureData.size());
        buffer.Length(rawCaptureData.size());
        memcpy(buffer.data(), rawCaptureData.data(), rawCaptureData.size());

        HdmiRawCaptureFormats pixelFormat = GetFormatFromInfoFrame(infoframe, 24);

        // Populate the frame format description, which for this capture card can only be 24bpc SDR RGB444.
        auto wireFormat = winrt::DisplayWireFormat(
            winrt::DisplayWireFormatPixelEncoding::Rgb444,
            24, // bits per pixel (minus alpha)
            winrt::DisplayWireFormatColorSpace::BT709,
            winrt::DisplayWireFormatEotf::Sdr,
            winrt::DisplayWireFormatHdrMetadata::None);

        auto frame = winrt::make<Frame>(m_D3DInstance, pixelFormat, resolution, wireFormat, buffer);

        m_frames.Append(frame);
    }

    bool TanagerDisplayCapture::CompareCaptureToPrediction(winrt::hstring name, winrt::MicrosoftDisplayCaptureTools::Framework::IRawFrameSet prediction)
    {
        if (prediction.Frames().Size() != m_frames.Size())
        {
            Logger().LogError(
                winrt::hstring(L"Prediction frame count did not match capture frame count. Predicted ") +
                std::to_wstring(prediction.Frames().Size()) + L", Captured " + std::to_wstring(m_frames.Size()));

            return false;
        }

        for (uint32_t index = 0; index < prediction.Frames().Size(); index++)
        {
            auto predictedFrame = prediction.Frames().GetAt(index);
            auto capturedFrame = m_frames.GetAt(index);

            {
                bool formatMismatch = false;

                // Compare the frame resolutions
                auto predictedFrameRes = predictedFrame.Resolution();
                auto capturedFrameRes = capturedFrame.Resolution();
                if (predictedFrameRes.Width != capturedFrameRes.Width || predictedFrameRes.Height != capturedFrameRes.Height)
                {
                    Logger().LogError(
                        winrt::hstring(L"Capture resolution did not match prediction. Captured=") + std::to_wstring(capturedFrameRes.Width) +
                        L"x" + std::to_wstring(capturedFrameRes.Height) + L", Predicted=" +
                        std::to_wstring(predictedFrameRes.Width) + L"x" + std::to_wstring(predictedFrameRes.Height));
                    formatMismatch = true;
                }

                // Compare the frame formats
                auto predictedFrameFormat = predictedFrame.DataFormat();
                auto capturedFrameFormat = capturedFrame.DataFormat();

                if (predictedFrameFormat.PixelEncoding() != capturedFrameFormat.PixelEncoding())
                {
                    Logger().LogError(
                        winrt::hstring(L"Capture pixel format did not match prediction"));
                    formatMismatch = true;
                }

                if (predictedFrameFormat.ColorSpace() != capturedFrameFormat.ColorSpace())
                {
                    Logger().LogError(winrt::hstring(L"Capture color space did not match prediction"));
                    formatMismatch = true;
                }

                if (predictedFrameFormat.Eotf() != capturedFrameFormat.Eotf())
                {
                    Logger().LogError(winrt::hstring(L"Capture EOTF did not match prediction"));
                    formatMismatch = true;
                }

                if (predictedFrameFormat.HdrMetadata() != capturedFrameFormat.HdrMetadata())
                {
                    Logger().LogError(winrt::hstring(L"Capture HDR metadata format did not match prediction"));
                    formatMismatch = true;
                }

                if (predictedFrameFormat.BitsPerChannel() != capturedFrameFormat.BitsPerChannel())
                {
                    Logger().LogError(winrt::hstring(L"Capture bits per channel did not match prediction. Captured=") +
                        std::to_wstring(capturedFrameFormat.BitsPerChannel()) + L", Predicted=" + 
                        std::to_wstring(predictedFrameFormat.BitsPerChannel()));
                    formatMismatch = true;
                }

                if (formatMismatch)
                {
                    return false;
                }
            }

            // The frame formats match, so we can compare the actual pixel data.

            // TODO: Create the input textures

            // TODO: Create the output buffer

            // TODO: Create the views

            // TODO: Dispatch the shader

            // TODO: Clear the shader pipeline

            // TODO: Sum the output items

            // TODO: Compute the PSNR in dB

            // TODO: Log the PSNR and report success/failure
		}

        return true;
    }

    winrt::IRawFrameSet TanagerDisplayCapture::GetFrameData()
    {
        return this->get_strong().try_as<winrt::MicrosoftDisplayCaptureTools::Framework::IRawFrameSet>();
    }

    winrt::IVector<winrt::IRawFrame> TanagerDisplayCapture::Frames()
    {
		return m_frames;
	}

    winrt::IMapView<winrt::hstring, winrt::IInspectable> TanagerDisplayCapture::ExtendedProperties()
    {
        return m_extendedProps.GetView();
    }

    winrt::IMap<winrt::hstring, winrt::IInspectable> TanagerDisplayCapture::Properties()
    {
        return m_properties;
    }

    Frame::Frame(
        std::shared_ptr<TanagerD3D> D3DInstance,
        HdmiRawCaptureFormats const& type,
        winrt::Windows::Graphics::SizeInt32 const& resolution,
        winrt::Windows::Devices::Display::Core::DisplayWireFormat const& format,
        winrt::Windows::Storage::Streams::IBuffer const& data) :
        m_type(type), m_data(data), m_format(format), m_resolution(resolution), m_D3DInstance(D3DInstance)
    {
        m_properties = winrt::single_threaded_map<winrt::hstring, winrt::IInspectable>();

        // Get the compute shader out of the cache, or load it if it's not there
        auto computeShader = m_D3DInstance->GetRawDataTranslateShader(m_type);

        // Only one thread can be running the compute shader at a time
        auto lock = std::scoped_lock<std::mutex>(m_D3DInstance->RenderingMutex());

        winrt::SoftwareBitmap renderableApproximation{nullptr};

        winrt::com_ptr<ID3D11Buffer> inputBuffer;
        winrt::com_ptr<ID3D11ShaderResourceView> inputBufferView;
        winrt::com_ptr<ID3D11Texture2D> rgba8OutputTex;
        winrt::com_ptr<ID3D11Texture2D> rgbaFp16OutputTex;
        winrt::com_ptr<ID3D11UnorderedAccessView> rgba8OutputTexView, rgbaFp16OutputTexView;

        // Create the input buffer
        {
            D3D11_BUFFER_DESC desc = {};
            desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
            desc.ByteWidth = m_data.Length();
            desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            desc.StructureByteStride = sizeof(uint64_t);
            D3D11_SUBRESOURCE_DATA inputData;
            inputData.pSysMem = m_data.data();
            winrt::check_hresult(m_D3DInstance->GetDevice()->CreateBuffer(&desc, &inputData, inputBuffer.put()));

            // Create input buffer SRV
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
            srvDesc.BufferEx.FirstElement = 0;
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            srvDesc.BufferEx.NumElements = desc.ByteWidth / desc.StructureByteStride;

            winrt::check_hresult(
                m_D3DInstance->GetDevice()->CreateShaderResourceView(inputBuffer.get(), &srvDesc, inputBufferView.put()));
        }

        // Create the 'approximation' output buffer (rgb8)
        {
            D3D11_TEXTURE2D_DESC textureDesc = {};
            textureDesc.Width = m_resolution.Width;
            textureDesc.Height = m_resolution.Height;
            textureDesc.MipLevels = 1;
            textureDesc.ArraySize = 1;
            textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UINT;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Usage = D3D11_USAGE_DEFAULT;
            textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
            textureDesc.CPUAccessFlags = 0;
            textureDesc.MiscFlags = 0;
            winrt::check_hresult(m_D3DInstance->GetDevice()->CreateTexture2D(&textureDesc, 0, rgba8OutputTex.put()));

            // Create output buffer UAV
            D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UINT;
            uavDesc.Texture2D.MipSlice = 0;
            winrt::check_hresult(
                m_D3DInstance->GetDevice()->CreateUnorderedAccessView(rgba8OutputTex.get(), &uavDesc, rgba8OutputTexView.put()));
        }

        // Create the comparison output buffer (4-channel fp16, actual data can be either rgb or yuv (a is ignored))
        {
            D3D11_TEXTURE2D_DESC textureDesc = {};
            textureDesc.Width = m_resolution.Width;
            textureDesc.Height = m_resolution.Height;
            textureDesc.MipLevels = 1;
            textureDesc.ArraySize = 1;
            textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Usage = D3D11_USAGE_DEFAULT;
            textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
            textureDesc.CPUAccessFlags = 0;
            textureDesc.MiscFlags = 0;
            winrt::check_hresult(m_D3DInstance->GetDevice()->CreateTexture2D(&textureDesc, 0, rgbaFp16OutputTex.put()));

            // Create output buffer UAV
            D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            uavDesc.Texture2D.MipSlice = 0;
            winrt::check_hresult(m_D3DInstance->GetDevice()->CreateUnorderedAccessView(
                rgbaFp16OutputTex.get(), &uavDesc, rgbaFp16OutputTexView.put()));
        }

        // Run the comput shader
        m_D3DInstance->GetDeviceContext()->CSSetShader(computeShader.get(), nullptr, 0);
        ID3D11ShaderResourceView* ppSRV[1] = {inputBufferView.get()};
        m_D3DInstance->GetDeviceContext()->CSSetShaderResources(0, 1, ppSRV);
        ID3D11UnorderedAccessView* ppUAView[2] = {rgba8OutputTexView.get(), rgbaFp16OutputTexView.get()};
        m_D3DInstance->GetDeviceContext()->CSSetUnorderedAccessViews(0, 2, ppUAView, nullptr);
        m_D3DInstance->GetDeviceContext()->Dispatch(m_resolution.Width, m_resolution.Height, 1);

        // Clear the compute shader pipeline
        {
            m_D3DInstance->GetDeviceContext()->CSSetShader(nullptr, nullptr, 0);
            ID3D11UnorderedAccessView* ppUAViewnullptr[1] = {nullptr};
            m_D3DInstance->GetDeviceContext()->CSSetUnorderedAccessViews(0, 1, ppUAViewnullptr, nullptr);
            ID3D11ShaderResourceView* ppSRVnullptr[1] = {nullptr};
            m_D3DInstance->GetDeviceContext()->CSSetShaderResources(0, 1, ppSRVnullptr);
            ID3D11Buffer* ppCBnullptr[1] = {nullptr};
            m_D3DInstance->GetDeviceContext()->CSSetConstantBuffers(0, 1, ppCBnullptr);
        }

        // Copy the 'approximation' output buffer to a software bitmap
        {
            winrt::Buffer outputDataRgba8(m_resolution.Height * m_resolution.Width * sizeof(uint32_t));
            outputDataRgba8.Length(m_resolution.Height * m_resolution.Width * sizeof(uint32_t));

            winrt::com_ptr<ID3D11Texture2D> readBackTex;
            D3D11_TEXTURE2D_DESC desc = {};
            rgba8OutputTex->GetDesc(&desc);
            desc.BindFlags = 0;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            desc.Usage = D3D11_USAGE_STAGING;
            desc.MiscFlags = 0;
            winrt::check_hresult(m_D3DInstance->GetDevice()->CreateTexture2D(&desc, nullptr, readBackTex.put()));
            m_D3DInstance->GetDeviceContext()->CopyResource(readBackTex.get(), rgba8OutputTex.get());

            D3D11_MAPPED_SUBRESOURCE mappedResource;
            winrt::check_hresult(m_D3DInstance->GetDeviceContext()->Map(readBackTex.get(), 0, D3D11_MAP_READ, 0, &mappedResource));

            auto approxData = reinterpret_cast<uint32_t*>(mappedResource.pData);
            memcpy_s(outputDataRgba8.data(), outputDataRgba8.Capacity(), approxData, outputDataRgba8.Capacity());
            m_D3DInstance->GetDeviceContext()->Unmap(readBackTex.get(), 0);

            outputDataRgba8.Length(outputDataRgba8.Capacity());

            renderableApproximation = winrt::SoftwareBitmap::CreateCopyFromBuffer(
                outputDataRgba8, winrt::BitmapPixelFormat::Rgba8, m_resolution.Width, m_resolution.Height);
        }

        {
            D3D11_TEXTURE2D_DESC desc = {};
            rgbaFp16OutputTex->GetDesc(&desc);
            desc.BindFlags = 0;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            desc.Usage = D3D11_USAGE_STAGING;
            desc.MiscFlags = 0;
            winrt::check_hresult(m_D3DInstance->GetDevice()->CreateTexture2D(&desc, nullptr, m_comparisonTexture.put()));
            m_D3DInstance->GetDeviceContext()->CopyResource(m_comparisonTexture.get(), rgbaFp16OutputTex.get());
        }

        m_bitmap = renderableApproximation;
    }

    winrt::IMap<winrt::hstring, winrt::IInspectable> Frame::Properties()
    {
        return m_properties;
    }

    winrt::IBuffer Frame::Data()
    {
        return m_data;
    }

    winrt::DisplayWireFormat Frame::DataFormat()
    {
        return m_format;
    }

    winrt::SizeInt32 Frame::Resolution()
    {
        return m_resolution;
    }

    winrt::IAsyncOperation<winrt::SoftwareBitmap> Frame::GetRenderableApproximationAsync()
    {
        // In this implementation, this approximation is created as a side effect of rendering the prediction. So here the result
        // can just be returned.
        co_return m_bitmap;
    }

    winrt::hstring Frame::GetPixelInfo(uint32_t x, uint32_t y)
    {
        // TODO: implement this - the intent is that because the above returns only an approximation, this should return a
        // description in string form of what the original pixel values are for a given address.
        throw winrt::hresult_not_implemented();
    }

    HdmiRawCaptureFormats GetFormatFromInfoFrame(winrt::IBuffer infoFrame, uint32_t bitsPerPixel)
    {
        bool limitedRange = false;
        winrt::DisplayWireFormatPixelEncoding pixelEncoding = winrt::DisplayWireFormatPixelEncoding::Rgb444;

        switch (bitsPerPixel)
        {
            case 24:
            case 30:
                break;
            default:
                Logger().LogAssert(L"Unsupported bits per pixel.");
                throw winrt::hresult_invalid_argument();
        }

        if (infoFrame.Capacity() < 14)
        {
            Logger().LogAssert(L"Infoframe buffer is too small.");
			throw winrt::hresult_invalid_argument();
		}

        auto infoFrameData = infoFrame.data();

        bool Y1 = (infoFrameData[1] & 0x40) != 0;
        bool Y0 = (infoFrameData[1] & 0x20) != 0;

        if (Y1 && Y0)
        {
            Logger().LogAssert(L"Infoframe Y0=Y1=1 is reserved!");
            throw winrt::hresult_invalid_argument();
		}
        else if (Y1 && !Y0)
        {
			pixelEncoding = winrt::DisplayWireFormatPixelEncoding::Ycc444;
		}
        else if (!Y1 && Y0)
        {
			pixelEncoding = winrt::DisplayWireFormatPixelEncoding::Ycc422;
		}
        else
        {
			pixelEncoding = winrt::DisplayWireFormatPixelEncoding::Rgb444;
		}

		bool Q1 = (infoFrameData[3] & 0x08) != 0;
        bool Q0 = (infoFrameData[3] & 0x04) != 0;

        if (Q1 && Q0)
        {
			Logger().LogAssert(L"Infoframe Q0=Q1=1 is reserved!");
			throw winrt::hresult_invalid_argument();
        }
        else if (Q1 && !Q0)
        {
            limitedRange = false;
        }
        else if (!Q1 && Q0)
        {
            limitedRange = true;
		}
        else
        {
            // We have to determine the quantization range from the video format
            if (pixelEncoding != winrt::DisplayWireFormatPixelEncoding::Rgb444)
            {
                // YCC formats are always limited range
                limitedRange = true;
            }
            else
            {
                // TODO: limited range for RGB CE formats (480p, 480i, 576p, 576i, 240p, 288p) (section 5.1)
                //       limited range for 1080p, 1080i, 720p (section 5.2)
                //       full range for RGB IT formats (section 5.4)

                // for now assuming full range for RGB
                limitedRange = false;
            }
		}

        if (pixelEncoding == winrt::DisplayWireFormatPixelEncoding::Rgb444)
        {
            if (bitsPerPixel == 24)
            {
                return limitedRange ? HdmiRawCaptureFormats::RGB_444_8bpc_Limited : HdmiRawCaptureFormats::RGB_444_8bpc_Full;
			}
            else if (bitsPerPixel == 30)
            {
                return limitedRange ? HdmiRawCaptureFormats::RGB_444_10bpc_Limited : HdmiRawCaptureFormats::RGB_444_10bpc_Full;
			}
            else if (bitsPerPixel == 36)
            {
                return limitedRange ? HdmiRawCaptureFormats::RGB_444_12bpc_Limited : HdmiRawCaptureFormats::RGB_444_12bpc_Full;
			}
            else if (bitsPerPixel == 48)
            {
                return limitedRange ? HdmiRawCaptureFormats::RGB_444_16bpc_Limited : HdmiRawCaptureFormats::RGB_444_16bpc_Full;
			}
		}
        else if (pixelEncoding == winrt::DisplayWireFormatPixelEncoding::Ycc444)
        {
            if (bitsPerPixel == 24)
            {
				return HdmiRawCaptureFormats::YCbCr_444_8bpc_Limited;
			}
            else if (bitsPerPixel == 30)
            {
                return HdmiRawCaptureFormats::YCbCr_444_10bpc_Limited;
			}
            else if (bitsPerPixel == 36)
            {
                return HdmiRawCaptureFormats::YCbCr_444_12bpc_Limited;
			}
            else if (bitsPerPixel == 48)
            {
                return HdmiRawCaptureFormats::YCbCr_444_16bpc_Limited;
			}
		}
        else if (pixelEncoding == winrt::DisplayWireFormatPixelEncoding::Ycc422)
        {
            if (bitsPerPixel == 24)
            {
                return HdmiRawCaptureFormats::YCbCr_422_8bpc_Limited;
            }
            else if (bitsPerPixel == 30)
            {
                return HdmiRawCaptureFormats::YCbCr_422_10bpc_Limited;
            }
            else if (bitsPerPixel == 36)
            {
                return HdmiRawCaptureFormats::YCbCr_422_12bpc_Limited;
            }
            else if (bitsPerPixel == 48)
            {
                return HdmiRawCaptureFormats::YCbCr_422_16bpc_Limited;
            }
        }
        else if (pixelEncoding == winrt::DisplayWireFormatPixelEncoding::Ycc420)
        {
            if (bitsPerPixel == 24)
            {
                return HdmiRawCaptureFormats::YCbCr_422_8bpc_Limited;
            }
            else if (bitsPerPixel == 30)
            {
                return HdmiRawCaptureFormats::YCbCr_422_10bpc_Limited;
            }
            else if (bitsPerPixel == 36)
            {
                return HdmiRawCaptureFormats::YCbCr_422_12bpc_Limited;
            }
            else if (bitsPerPixel == 48)
            {
                return HdmiRawCaptureFormats::YCbCr_422_16bpc_Limited;
            }
        }

        Logger().LogAssert(L"Unsupported pixel encoding.");
        throw winrt::hresult_invalid_argument();
    }

    winrt::com_ptr<ID3D11ComputeShader> TanagerD3D::GetRawDataTranslateShader(HdmiRawCaptureFormats const& type)
    {
        // If we have already loaded the shader, just provide it
        if (m_computeShaderCache.find(type) != m_computeShaderCache.end())
        {
		    return m_computeShaderCache[type];
	    }

        // Otherwise, load the shader
        winrt::com_ptr<ID3D11ComputeShader> shader;

        switch (type)
        {
        case HdmiRawCaptureFormats::RGB_444_8bpc_Full:
        case HdmiRawCaptureFormats::RGB_444_8bpc_Limited:
            winrt::check_hresult(m_d3dDevice->CreateComputeShader(
                Shaders::RGB_444_8bpc, sizeof(Shaders::RGB_444_8bpc), nullptr, shader.put()));
            break;
        case HdmiRawCaptureFormats::RGB_444_10bpc_Full:
        case HdmiRawCaptureFormats::RGB_444_10bpc_Limited:
            winrt::check_hresult(m_d3dDevice->CreateComputeShader(
                Shaders::RGB_444_10bpc, sizeof(Shaders::RGB_444_10bpc), nullptr, shader.put()));
            break;
        case HdmiRawCaptureFormats::YCbCr_444_8bpc_Limited:
            winrt::check_hresult(m_d3dDevice->CreateComputeShader(
                Shaders::YCbCr_444_8bpc, sizeof(Shaders::YCbCr_444_8bpc), nullptr, shader.put()));
            Logger().LogNote(L"Using YCbCr 444 8bpc.");
            break;
        case HdmiRawCaptureFormats::YCbCr_444_10bpc_Limited:
            winrt::check_hresult(m_d3dDevice->CreateComputeShader(
                Shaders::YCbCr_444_10bpc, sizeof(Shaders::YCbCr_444_10bpc), nullptr, shader.put()));
            break;
        case HdmiRawCaptureFormats::YCbCr_422_8bpc_Limited:
        case HdmiRawCaptureFormats::YCbCr_420_8bpc_Limited:
        case HdmiRawCaptureFormats::YCbCr_422_10bpc_Limited:
        case HdmiRawCaptureFormats::YCbCr_420_10bpc_Limited:
        case HdmiRawCaptureFormats::RGB_444_12bpc_Full:
        case HdmiRawCaptureFormats::RGB_444_12bpc_Limited:
        case HdmiRawCaptureFormats::YCbCr_444_12bpc_Limited:
        case HdmiRawCaptureFormats::YCbCr_422_12bpc_Limited:
        case HdmiRawCaptureFormats::YCbCr_420_12bpc_Limited:
        case HdmiRawCaptureFormats::RGB_444_16bpc_Full:
        case HdmiRawCaptureFormats::RGB_444_16bpc_Limited:
        case HdmiRawCaptureFormats::YCbCr_444_16bpc_Limited:
        case HdmiRawCaptureFormats::YCbCr_422_16bpc_Limited:
        case HdmiRawCaptureFormats::YCbCr_420_16bpc_Limited:
        default:
			Logger().LogAssert(L"Unsupported raw capture format.");
			throw winrt::hresult_invalid_argument();
        }

        m_computeShaderCache[type] = shader;
        return shader;
    }

    winrt::com_ptr<ID3D11ComputeShader> TanagerD3D::GetDiffSumShader()
    {
        if (m_diffSumShader)
        {
            return m_diffSumShader;
        }

        winrt::check_hresult(m_d3dDevice->CreateComputeShader(
            Shaders::scRGB_Bucket_Sum, sizeof(Shaders::scRGB_Bucket_Sum), nullptr, m_diffSumShader.put()));

        return m_diffSumShader;
    }

} // namespace winrt::TanagerPlugin::implementation
