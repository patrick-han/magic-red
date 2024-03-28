cbuffer Constants
{
    float4x4 g_WorldViewProj;
};

struct VSInput
{
    float3 Pos    : ATTRIB0;
    float3 Normal : ATTRIB1;
    float4 Color  : ATTRIB2;
};

struct PSInput 
{ 
    float4 Pos    : SV_POSITION;
    float4 Color  : COLOR0; 
};

void main(in  VSInput VSIn, out PSInput PSIn)
{
    PSIn.Pos = mul(g_WorldViewProj, float4(VSIn.Pos, 1.0));
    PSIn.Color = VSIn.Color;
}