RWTexture2D<float4> inputTexture : register(u0); // 16-bit float per channel texture
RWTexture2D<uint4> outputTextureRgba8 : register(u1); // RGBA8 texture
RWTexture2D<float4> outputTextureFp16 : register(u2); // 16-bit float per channel texture

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // Copy the input buffer to the output, for 2020 we use the following primaries:
    // R x = 0.708,  y = 0.292
    // G x = 0.170,  y = 0.797
    // B x = 0.131,  y = 0.046
    // W x = 0.3127, y = 0.3290
    float4x4 mat = float4x4( 1.660362656, -0.587539997, -0.072822659, 0.0,
                            -0.124563549,  1.132911375, -0.008347826, 0.0,
                            -0.018156606, -0.100601732,  1.118758338, 0.0,
                             0.0,          0.0,          0.0,         1.0);
    float4 input = mul(mat, inputTexture[DTid.xy]);
    outputTextureFp16[DTid.xy] = input;
    
    // Convert the input buffer to sRGB 8bpc
    float4 sRGB = input;
    
    sRGB.r = sRGB.r <= 0.0031308 ? sRGB.r * 12.92 : 1.055 * pow(sRGB.r, 1.0 / 2.4) - 0.055;
    sRGB.g = sRGB.g <= 0.0031308 ? sRGB.g * 12.92 : 1.055 * pow(sRGB.g, 1.0 / 2.4) - 0.055;
    sRGB.b = sRGB.b <= 0.0031308 ? sRGB.b * 12.92 : 1.055 * pow(sRGB.b, 1.0 / 2.4) - 0.055;

    outputTextureRgba8[DTid.xy] = (uint4) (sRGB * 255.0f);
}
