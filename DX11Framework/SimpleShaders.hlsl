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
    float3 Normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 instancePos : INSTANCEPOS;
};


VS_Out VS_main(float3 Position : POSITION, float3 Normal : NORMAL, float2 TexCoord : TEXCOORD, float3 Instance : INSTANCEPOS)
{
    VS_Out output;

    // Transform to world space
    float3 worldPos = mul(float4(Position + Instance, 1.0f), World).xyz;
    output.position = float4(worldPos, 1.0f);

    // Transform to clip space
    output.position = mul(output.position, View);
    output.position = mul(output.position, Projection);

    // Transform normal to world space
    output.Normal = normalize(mul(float4(Normal, 0.0f), World).xyz);
    output.texCoord = TexCoord;
    output.instancePos = Instance;

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