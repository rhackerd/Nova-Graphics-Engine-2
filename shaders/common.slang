module common;

public struct CameraUBO {
    public float4x4 view;
    public float4x4 proj;
};

public struct Transform {
    public float4x4 model;
};

public struct VertexInput {
    public float3 position : POSITION;
    public float3 normal   : NORMAL;
    public float2 uv       : TEXCOORD0;
};

public struct VertexOutput {
    public float4 position : SV_Position;
    public float3 normal   : NORMAL;
    public float2 uv       : TEXCOORD0;
};