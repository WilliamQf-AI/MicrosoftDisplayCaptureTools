#include "pch.h"
#include "SampleDisplayCapture.h"
#include "MethodAccess.h"

#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Security.Cryptography.Core.h>

#include <wincodec.h>
#include <MemoryBuffer.h>


using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

namespace winrt::CaptureCard::implementation
{
    SampleDisplayCapture::SampleDisplayCapture() : 
        m_testDataFolder(LoadFolder(L"TestData")),
        m_mismatchFolder(LoadFolder(L"Mismatches"))
    {
        
    }

    void SampleDisplayCapture::CompareCaptureToReference(hstring name, DisplayStateReference::IStaticReference reference)
    {
        // 
        // This software-only plugin doesn't compare against a hardware-captured image, instead it compares against 
        // an image loaded from disk.
        //

        Log::Comment(L"Comparison For: " + String(name.c_str()));

        // Hash the name string to compute the on-disk file name.
        auto hashedName = ComputeHashedFileName(name);

        // Retrieve serialized data from the current reference image
        auto referenceMetadata = reference.GetSerializedMetadata();

        auto frame = reference.GetFrame();

        boolean comparisonSucceeded = false;

        try
        {
            auto comparison = LoadComparisonImage(hashedName);

            // TODO: Actually do the comparison


            comparisonSucceeded = true;
        }
        catch (winrt::hresult_error const& ex)
        {
            if (ex.code() == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                Log::Comment(L"Comparison image could not be located");
            }
            else throw;
        }

        if (!comparisonSucceeded)
        {
            SaveMismatchedImage(hashedName, frame);

            Log::Error(L"Comparison to image-on-disk failed! Generated image saved");
        }
    }

    // TODO: re-implement shader-based comparisons once I fix the hardware systems

    void SampleDisplayCapture::SaveCaptureToDisk(hstring path)
    {
    }

    hstring SampleDisplayCapture::ComputeHashedFileName(hstring testName)
    {
        auto hash = winrt::Windows::Security::Cryptography::Core::HashAlgorithmProvider::OpenAlgorithm(
            winrt::Windows::Security::Cryptography::Core::HashAlgorithmNames::Sha1());

        auto stringBuffer = winrt::Windows::Security::Cryptography::CryptographicBuffer::ConvertStringToBinary(
            testName, winrt::Windows::Security::Cryptography::BinaryStringEncoding::Utf16LE);

        auto buffer = hash.HashData(stringBuffer);
        auto hashedName = winrt::Windows::Security::Cryptography::CryptographicBuffer::EncodeToHexString(buffer);

        hashedName = hashedName + L".bmp";

        return hashedName;
    }

    winrt::Windows::Graphics::Imaging::SoftwareBitmap SampleDisplayCapture::LoadComparisonImage(hstring name)
    {
        auto file = m_testDataFolder.GetFileAsync(name).get();
        if (!file) winrt::throw_hresult(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

        auto read = file.OpenReadAsync().get();
        auto decoder = winrt::Windows::Graphics::Imaging::BitmapDecoder::CreateAsync(read).get();
        auto bitmap = decoder.GetSoftwareBitmapAsync().get();

        return bitmap;
    }
    
    //winrt::Windows::Graphics::Imaging::SoftwareBitmap SampleDisplayCapture::SaveMemoryToBitmap(hstring name)
    void SampleDisplayCapture:: SaveMemoryToBitmap (hstring name)
    {
        auto file = m_testDataFolder.GetFileAsync(name).get();
        //if (!file) winrt::throw_hresult(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
        byte data = 0x0;
        std::vector<byte> dataBuff;
        dataBuff.push_back(data);
        auto readBuffer= CaptureCard::FpgaRead (0x20,dataBuff);
        auto read = readBuffer.get();
        auto decoder = winrt::Windows::Graphics::Imaging::BitmapDecoder::CreateAsync(read).get();
        auto bitmap = decoder.GetSoftwareBitmapAsync().get();
        // width, height and pixel format info retrieved from FPGA
        int bWidth= 644; 
        int bHeight= 300;
        winrt::com_ptr<IWICBitmap> m_pEmbeddedBitmap ; //COM pointer 
        int bitsPerPixel =32;
        hr = pIWICFactory -> CreateBitmapFromMemory (
            bHeight,
            bWidth,
            GUID_WICPixelFormat32bppRGB, 
            (bWidth*bitsPerPixel+7)/8, 
            bHeight*bWidth,
            readBuffer,
            m_pEmbeddedBitmap );
        if (!SUCCEEDED (hr)) {
            char *buffer ="Error in Creating Bitmap \n"; }

        return hr;
    }

    void SampleDisplayCapture::SaveMismatchedImage(hstring name, winrt::Windows::Graphics::Imaging::SoftwareBitmap bitmap)
    {
        auto file = m_mismatchFolder.CreateFileAsync(name, winrt::Windows::Storage::CreationCollisionOption::FailIfExists).get();
        auto stream = file.OpenAsync(winrt::Windows::Storage::FileAccessMode::ReadWrite).get();
        auto encoder = winrt::Windows::Graphics::Imaging::BitmapEncoder::CreateAsync(winrt::Windows::Graphics::Imaging::BitmapEncoder::BmpEncoderId(), stream).get();
        encoder.SetSoftwareBitmap(bitmap);

        encoder.FlushAsync().get();
    }

    winrt::Windows::Storage::StorageFolder SampleDisplayCapture::LoadFolder(hstring subFolder)
    {
        DWORD directoryLength = MAX_PATH;
        wchar_t directoryBuffer[MAX_PATH];

        auto err = GetCurrentDirectoryW(directoryLength, directoryBuffer);

        if (err == 0 || err > MAX_PATH) winrt::check_win32(GetLastError());

        hstring directoryName(directoryBuffer);

        auto testFolder = winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(directoryName).get();
        auto folder = testFolder.CreateFolderAsync(subFolder, winrt::Windows::Storage::CreationCollisionOption::OpenIfExists).get();

        return folder;
    }
}