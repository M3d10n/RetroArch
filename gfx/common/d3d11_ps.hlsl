// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
   float4 pos : SV_POSITION;
   float2 uv0 : TEXCOORD0;
};

Texture2D frame : register(t0);
SamplerState samplerState : register(s0);

float4 main(PixelShaderInput input) : SV_TARGET
{
   return float4(frame.Sample(samplerState, input.uv0).xyz, 1.0f);
}