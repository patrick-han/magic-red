
cbuffer Constants
{
    float4x4 g_Model;
    float4x4 g_View;
    float4x4 g_Proj;
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
    float3 Normal : NORMAL;
    float4 Color  : COLOR0;
    float4 FragWorldPos : FRAG_WORLD_POS;
};

void main(in  VSInput VSIn, out PSInput PSIn)
{
    float4x4 g_ModelViewProj = mul(mul(g_Proj, g_View), g_Model);
    PSIn.Pos = mul(g_ModelViewProj, float4(VSIn.Pos, 1.0));
    PSIn.Normal = VSIn.Normal;
    PSIn.Color = VSIn.Color;
    PSIn.FragWorldPos = mul(g_Model, float4(VSIn.Pos, 1.0));
    // PSIn.Color = float4(VSIn.Normal, 1.0);
}