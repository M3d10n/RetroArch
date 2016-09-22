// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
   float4 pos : SV_POSITION;
   float2 uv0 : TEXCOORD0;
};

// A constant buffer that stores the three basic column-major matrices for composing geometry.
cbuffer DisplayMatrixConstantBuffer : register(b0)
{
	matrix display;
	float4 size;
	float4 params;
};

Texture2D frame : register(t0);
SamplerState samplerState : register(s0);

float4 main(PixelShaderInput input) : SV_TARGET
{
	float sharpness = params.x;

	float2 px = input.uv0 * size.xy;
	float2 f = frac(px);
	float2 uv = (floor(px) - 0.5f + pow(f, sharpness)) * size.zw;

	return float4(frame.Sample(samplerState, uv).xyz, 1.0f);
}