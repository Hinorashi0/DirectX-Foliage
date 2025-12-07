Texture2D diffuseTex : register(t0);
SamplerState bilinearSampler : register(s0);

cbuffer ConstantBuffer : register(b0)
{
    float4x4 Projection;
    float4x4 View;
    float4x4 World;
    float4 DiffuseLight;
    float4 DiffuseMaterial;
    float4 AmbientLight;
    float4 AmbientMaterial;
    float3 LightDir;
    float count;
    uint hasTexture;
}
struct VS_Out
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float3 PosW : POSITION0;
    float3 NormalW : NORMAL;
    float2 texCoord : TEXCOORD;
};


VS_Out VS_main(float3 Position : POSITION, float3 Normal : NORMAL)
{
    VS_Out output;

    // Transform to world space
    output.position = mul(float4(Position, 1.0f), World);
    
    output.PosW = output.position.xyz;

    // Transform to clip space
    output.position = mul(output.position, View);
    output.position = mul(output.position, Projection);
    
    // Transform normal to world space
    output.NormalW = normalize(mul(float4(Normal, 0.0f), World).xyz);

    return output;
}
    
float4 PS_main(VS_Out input) : SV_TARGET
{
    float d = dot(input.NormalW, -LightDir);

    float DiffuseAmount = saturate(d);
    float4 diffuse = DiffuseAmount * (DiffuseMaterial * DiffuseLight);

    float4 ambient = AmbientMaterial * AmbientLight;

    float4 color = ambient + diffuse;
    
    float4 texColor = diffuseTex.Sample(bilinearSampler, input.texCoord);
    
    return texColor;
}