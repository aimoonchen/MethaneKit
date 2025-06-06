/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: MethaneKit/Modules/Graphics/Extensions/Shaders/SkyBox.hlsl
Shaders for sky-box rendering from cube-map texture on a sphere mesh without lighting

******************************************************************************/

#include "SkyBoxUniforms.h"

struct VSInput
{
    float3 position : POSITION;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 uvw      : UVFACE;
};

ConstantBuffer<SkyBoxUniforms> g_skybox_uniforms : register(b0, META_ARG_FRAME_CONSTANT);
TextureCube                    g_skybox_texture  : register(t0, META_ARG_CONSTANT);
SamplerState                   g_texture_sampler : register(s0, META_ARG_CONSTANT);

PSInput SkyboxVS(VSInput input)
{
    PSInput output;

    output.position   = mul(float4(input.position, 0.0f), g_skybox_uniforms.mvp_matrix);
    output.position.z = 0.0f;
    output.uvw        = input.position;

    return output;
}

float4 SkyboxPS(PSInput input) : SV_TARGET
{
    const float4 texel_color = g_skybox_texture.Sample(g_texture_sampler, input.uvw);
    return float4(texel_color.xyz, 1.f);
}