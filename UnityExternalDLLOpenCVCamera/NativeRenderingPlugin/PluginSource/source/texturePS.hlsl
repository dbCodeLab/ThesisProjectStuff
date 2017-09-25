Texture2D shaderTexture;
SamplerState sampleType;

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

float4 main(PixelInputType input, bool isFrontFacing : SV_IsFrontFace) : SV_TARGET
{
    float4 textureColor;
    
    // Sample the pixel color from the texture using the sampler at this texture coordinate location.
    if (isFrontFacing)
    {
        textureColor = shaderTexture.Sample(sampleType, input.tex);
    }
    else
    {
        textureColor = float4(0.4f, 0.4f, 0.4f, 1.0f);
    }

    return textureColor;
}