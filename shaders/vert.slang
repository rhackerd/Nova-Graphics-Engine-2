import common;

[[vk::binding(0, 0)]]
ConstantBuffer<CameraUBO> cam;

[[vk::push_constant]]
Transform transform;

float3x3 inverseMat3(float3x3 m) {
    float det = m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2])
              - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
              + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    float invDet = 1.0 / det;
    float3x3 inv;
    inv[0][0] =  (m[1][1] * m[2][2] - m[2][1] * m[1][2]) * invDet;
    inv[0][1] = -(m[0][1] * m[2][2] - m[0][2] * m[2][1]) * invDet;
    inv[0][2] =  (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * invDet;
    inv[1][0] = -(m[1][0] * m[2][2] - m[1][2] * m[2][0]) * invDet;
    inv[1][1] =  (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * invDet;
    inv[1][2] = -(m[0][0] * m[1][2] - m[1][2] * m[1][0]) * invDet;
    inv[2][0] =  (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * invDet;
    inv[2][1] = -(m[0][0] * m[2][1] - m[2][0] * m[0][1]) * invDet;
    inv[2][2] =  (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * invDet;
    return inv;
}

[shader("vertex")]
VertexOutput main(VertexInput v) {
    VertexOutput out;

    float3x3 model3 = (float3x3)transform.model;
    float3x3 normalMatrix = transpose(inverseMat3(model3));
    out.normal   = normalize(mul(normalMatrix, v.normal));
    out.uv       = v.uv;
    out.position = mul(cam.proj, mul(cam.view, mul(transform.model, float4(v.position, 1.0))));

    return out;
}