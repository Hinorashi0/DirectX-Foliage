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
    VS_Out output;


    float4 worldPos = mul(float4(Position, 1.0f), World);
    output.position = mul(mul(worldPos, View), Projection);

    float3x3 normalMatrix = (float3x3) World;
    float3 NormalW = normalize(mul(Normal, normalMatrix));

    float3 L = normalize(-LightDir);

    float DiffuseAmount = saturate(dot(NormalW, L));
    
    output.color = DiffuseAmount * (DiffuseMaterial * DiffuseLight);

    return output;
}

float4 PS_main(VS_Out input) : SV_TARGET
{ 
    return input;
   
}