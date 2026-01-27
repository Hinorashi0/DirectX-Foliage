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



float3 RotateY(float3 v, float angle)
{
    float s = sin(angle);
    float c = cos(angle);

    return float3(
        v.x * c - v.z * s,
        v.y,
        v.x * s + v.z * c
    );
}

VS_Out VS_main(float3 Position : POSITION, float3 Normal : NORMAL, float2 TexCoord : TEXCOORD, float3 InstancePos : INSTANCEPOS, uint InstanceID : SV_InstanceID)
{
    VS_Out output;

    static const float HALF_PI = 1.57079632f;

    float3 pos = Position;

    if ((InstanceID & 1) == 1)
    {
        pos = RotateY(pos, HALF_PI);
    }

    pos += InstancePos;

    float4 worldPos = mul(float4(pos, 1.0f), World);
    output.position = mul(worldPos, View);
    output.position = mul(output.position, Projection);

    output.Normal = normalize(mul(float4(Normal, 0.0f), World).xyz);
    output.texCoord = TexCoord;

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
    
    clip(texColor.a - 0.9f);
    
    return texColor;
}