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
    //World Pos
    float4 Pos4 = float4(Position, 1.0f);
    
    VS_Out output = (VS_Out)0;
    
    output.position = mul(Pos4, World);
    
    output.PosW = output.position;

    output.position = mul(output.position, View);
    output.position = mul(output.position, Projection);

    output.PosW = normalize(output.PosW);
    
    output.NormalW = normalize(mul((float3x3) World, Normal));
    
    float3 normalizedDir = normalize(-LightDir);
    
    float3 reflectedDir = saturate(reflect(normalizedDir, output.NormalW));
    
    float DiffuseAmount = reflectedDir;
    
    //float4(0.5f + 0.5f * output.NormalW, 1)
    //
    output.color = DiffuseAmount * (DiffuseMaterial * DiffuseLight);
    
    return output;
}

float4 PS_main(VS_Out input) : SV_TARGET
{ 
    return input.color; 
}