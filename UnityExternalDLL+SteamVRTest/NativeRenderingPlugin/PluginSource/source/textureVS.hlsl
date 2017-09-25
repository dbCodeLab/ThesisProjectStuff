cbuffer MatrixBuffer
{
    float4x4 mvp;
};

struct VertexInputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

PixelInputType main(VertexInputType input)
{
    PixelInputType output;
    
    output.position = mul(mvp, float4(input.position, 1.0f));
    output.tex = input.tex;
    
    return output;
}