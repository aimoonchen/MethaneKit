/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License"),
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: MethaneKit/Apps/Tutorials/03-TexturedCube/Shaders/TexturedCube.hlsl
Shaders for textured cube rendering with Phong lighting model

******************************************************************************/

#include "TexturedCubeUniforms.h"
#include "../../Common/Shaders/Primitives.hlsl"

struct VSInput
{
    float3 position         : POSITION;
    float3 normal           : NORMAL;
    float2 texcoord         : TEXCOORD;
};

struct PSInput
{
    float4 position         : SV_POSITION;
    float3 world_position   : POSITION;
    float3 world_normal     : NORMAL;
    float2 texcoord         : TEXCOORD;
};

ConstantBuffer<Constants> g_constants : register(b0, META_ARG_CONSTANT);
ConstantBuffer<Uniforms>  g_uniforms  : register(b1, META_ARG_FRAME_CONSTANT);
Texture2D                 g_texture   : register(t0, META_ARG_CONSTANT);
SamplerState              g_sampler   : register(s0, META_ARG_CONSTANT);

PSInput CubeVS(VSInput input)
{
    const float4 position = float4(input.position, 1.F);

    PSInput output;
    output.position       = mul(position, g_uniforms.mvp_matrix);
    output.world_position = mul(position, g_uniforms.model_matrix).xyz;
    output.world_normal   = normalize(mul(float4(input.normal, 0.F), g_uniforms.model_matrix).xyz);
    output.texcoord       = input.texcoord;

    return output;
}

float4 CubePS(PSInput input) : SV_TARGET
{
    const float3 fragment_to_light  = normalize(g_uniforms.light_position - input.world_position);
    const float3 fragment_to_eye    = normalize(g_uniforms.eye_position.xyz - input.world_position);
    const float3 light_reflected_from_fragment = reflect(-fragment_to_light, input.world_normal);

    const float4 texel_color    = g_texture.Sample(g_sampler, input.texcoord);
    const float4 ambient_color  = texel_color * g_constants.light_ambient_factor;
    const float4 base_color     = texel_color * g_constants.light_color * g_constants.light_power;

    const float  distance       = length(g_uniforms.light_position - input.world_position);
    const float  diffuse_part   = clamp(dot(fragment_to_light, input.world_normal), 0.0, 1.0);
    const float4 diffuse_color  = base_color * diffuse_part / (distance * distance);

    const float  specular_part  = pow(clamp(dot(fragment_to_eye, light_reflected_from_fragment), 0.0, 1.0), g_constants.light_specular_factor);
    const float4 specular_color = base_color * specular_part / (distance * distance);;

    return ColorLinearToSrgb(ambient_color + diffuse_color + specular_color);
}
