// frag.slang
import common;

[[vk::binding(0, 1)]]
SamplerState u_sampler;

[[vk::binding(1, 1)]]
Texture2D u_texture;

[shader("fragment")]
float4 main(VertexOutput v) : SV_Target0 {
    return u_texture.Sample(u_sampler, v.uv);
}