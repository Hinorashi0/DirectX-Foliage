cbuffer ConstantBuffer : register(b0)
{
    float4x4 Projection;
    float4x4 View;
    float4x4 World;
    float4 DiffuseLight;
    float4 DiffuseMaterial;
    float4 AmbientMaterial;
    float4 AmbientLight;
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
struct PS_Input
{
    float3 Norm : TEXCOORD0;
};

VS_Out VS_main(float3 Position : POSITION, float3 Normal : NORMAL)
{
    //World Pos 
    float4 Pos4 = float4(Position, 1.0f);
    
    VS_Out output = (VS_Out) 0;
    PS_Input input = (PS_Input) 0;
    output.position = mul(Pos4, World);
    output.PosW = output.position;
    output.PosW = normalize(output.PosW);
    output.position = mul(output.position, View);
    output.position = mul(output.position, Projection);
    output.NormalW = normalize(mul(float4(input.Norm, 1), World)).xyz;
    
    float4 ambient = AmbientLight + AmbientMaterial;
    float3 lightPos = (0.0f, 0.0f, 0.0f, 0.0f);
    float3 lightDir = normalize(lightPos - output.position.xyz);
    float d = dot(output.NormalW, lightDir);
    float DiffuseAmount = saturate(d);
    
    output.color = DiffuseAmount * (DiffuseMaterial * DiffuseLight);
    
    return output;
}
    
float4 PS_main(VS_Out input) : SV_TARGET
{
    return input.color;
}