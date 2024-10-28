#include "RootSignature.hlsl"

[RootSignature(ROOTSIG)]
void main(
    // == IN == 
    in float2 pos : Position,
    in float2 uv : Texcoord, 
    // == OUT == 
    out float2 o_uv : Texcoord, 
    out float4 o_pos : SV_Position

)
{
    o_uv = uv;
    o_pos = float4(pos.xy, 0.0f, 1.0f);
}