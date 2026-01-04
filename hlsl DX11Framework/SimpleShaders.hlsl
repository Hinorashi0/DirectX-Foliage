Texture2D diffuseTex : register(t0);
SamplerState bilinearSampler : register(s0);

// per-frame constant buffer
cbuffer ConstantBuffer : register(b0)
{
    float4x4 Projection;
    float4x4 View;
    float4x4 World; // keep for non-instanced draws if needed
    float4 DiffuseLight;
    float4 DiffuseMaterial;
    float4 AmbientLight;
    float4 AmbientMaterial;
    float3 LightDir;
    float count;
    uint hasTexture;
}

// per-instance world matrices (bound to t1 from C++)
StructuredBuffer<float4x4> InstanceWorlds : register(t1);

struct VS_Out
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float3 PosW : POSITION0;
    float3 NormalW : NORMAL;
    float2 texCoord : TEXCOORD;
    uint Instance : TEXCOORD1; // carry instance index to PS with ordinary semantic
};

// Read the system-provided instance id
VS_Out VS_main(float3 Position : POSITION, float3 Normal : NORMAL, float2 TexCoord : TEXCOORD, uint InstanceID : SV_InstanceID)
{
    VS_Out output;

    // get per-instance world matrix supplied from C++
    float4x4 instWorld = InstanceWorlds[InstanceID];

    // Transform to world space using per-instance matrix
    float4 worldPos = mul(float4(Position, 1.0f), instWorld);
    output.PosW = worldPos.xyz;

    // Transform to clip space
    float4 viewPos = mul(worldPos, View);
    output.position = mul(viewPos, Projection);

    // Transform normal by instance world (w=0)
    output.NormalW = normalize(mul(float4(Normal, 0.0f), instWorld).xyz);
    output.texCoord = TexCoord;

    // forward instance id to pixel shader
    output.Instance = InstanceID;

    return output;
}

float4 PS_main(VS_Out input) : SV_TARGET
{
    float d = dot(input.NormalW, -LightDir);
    float DiffuseAmount = saturate(d);
    float4 diffuse = DiffuseAmount * (DiffuseMaterial * DiffuseLight);
    float4 ambient = AmbientMaterial * AmbientLight;
    float4 color = ambient + diffuse;

    float4 texColor = color + diffuseTex.Sample(bilinearSampler, input.texCoord);

    clip(texColor.a - 0.1f);

    return texColor;
}