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

cbuffer InstanceBuffer : register(b1)
{
    float4x4 InstanceWorld[128];
};

struct VS_Out
{
    float4 position : POSITION;
    float3 Normal : NORMAL;
    float2 texCoord : TEXCOORD;
    uint instanceID : SV_InstanceID;
};



VS_Out VS_main(float3 Position : POSITION, float3 Normal : NORMAL, float2 TexCoord : TEXCOORD, uint instanceID : SV_InstanceID)
{
    VS_Out output = (VS_Out) 0;
    
    float4 worldPos = mul(float4(output.position),InstanceWorld[instanceID]);

    worldPos = mul(worldPos, World);
    worldPos = mul(worldPos, View);
    worldPos = mul(worldPos, Projection);

    output.position = worldPos;
    output.texCoord = TexCoord;
    output.Normal = mul(output.Normal, (float3x3) World);

    return output;
}
    
float4 PS_main(VS_Out input) : SV_TARGET
{
    float d = dot(input.Normal, -LightDir);

    float DiffuseAmount = saturate(d);
    float4 diffuse = DiffuseAmount * (DiffuseMaterial * DiffuseLight);

    float4 ambient = AmbientMaterial * AmbientLight;

    float4 color = ambient + diffuse;
    
    
    float4 totalColor = float4(0, 0, 0, 0);
    float4 texColor = color + diffuseTex.Sample(bilinearSampler, input.texCoord);
    
    clip(texColor.a - 0.1f);
    
    return texColor;
}