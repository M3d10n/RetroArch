// A constant buffer that stores the three basic column-major matrices for composing geometry.
cbuffer DisplayMatrixConstantBuffer : register(b0)
{
   matrix display;
   float4 size;
   float4 params;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
   float4 pos : SV_POSITION;
   float2 uv0 : TEXCOORD0;
};

PixelShaderInput main( float4 pos : POSITION )
{
   PixelShaderInput output;

   //output.pos = float4((pos.xy * 2) - 1.0f, 0, 1);
   output.pos = float4(mul(pos, display).xy, 0.5, 1);

   output.uv0 = pos.xy;

   //return float4(mul(pos, display).xyz, 0);

   return output;
}