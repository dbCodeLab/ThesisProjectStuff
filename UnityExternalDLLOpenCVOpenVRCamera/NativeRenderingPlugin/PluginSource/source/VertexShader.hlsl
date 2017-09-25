cbuffer MatrixBuffer : register(b0)
{
    float4x4 mvp;
    //float4x4 w;
    //float4x4 v;
    //float4x4 p;
};

struct VertexInputType
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PixelInputType main(VertexInputType input)
{
    PixelInputType output;
    

    // Change the position vector to be 4 units for proper matrix calculations.
    float4 inputPosition = float4(input.position, 1.0f);

    // Calculate the position of the vertex against the world, view, and projection matrices.
    //mul(mul(mul(p, v), w), inputPosition); 
    output.position = mul(mvp, inputPosition); //mul(mvp, inputPosition); //mul(mul(p, v), inputPosition); //mul(mvp, inputPosition); //mul(inputPosition, mvp);
    // Store the input color for the pixel shader to use.
  
    output.color = input.color;
    
    return output;
}