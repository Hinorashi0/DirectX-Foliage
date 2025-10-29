cbuffer ConstantBuffer : register(b0)
{
    float4x4 Projection;
    float4x4 View;
    float4x4 World;
    float4 DiffuseLight;
    float4 DiffuseMaterial;
    float3 LightDir;
    float count;
}

struct VS_Out
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float3 PosW : POSITION0;
    float3 NormalW : NORMAL;
};

VS_Out VS_main(float3 Position : POSITION, float3 Normal : NORMAL)
{   
    VS_Out output = (VS_Out)0;

    output.NormalW = Normal;
    output.PosW = Position;
    float4 Pos4 = float4(Position, 1.0f);
    output.position = output.PosW.y += sin(count);
    output.position = mul(Pos4, World);
    output.position = mul(output.position, View);
    output.position = mul(output.position, Projection);
    
    
    return output;
}

float4 PS_main(VS_Out input) : SV_TARGET
{ 

        return input.color;
   
}