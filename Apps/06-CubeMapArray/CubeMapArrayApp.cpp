/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: CubeMapArrayApp.cpp
Tutorial demonstrating textured cube rendering with Methane graphics API

******************************************************************************/

#include "CubeMapArrayApp.h"

#include <Methane/Tutorials/TextureLabeler.h>
#include <Methane/Tutorials/AppSettings.h>
#include <Methane/Graphics/CubeMesh.hpp>
#include <Methane/Data/TimeAnimation.hpp>

#include <cmath>
#include <numbers>

namespace Methane::Tutorials
{

namespace gui = Methane::UserInterface;

struct CubeVertex
{
    gfx::Mesh::Position position;

    inline static const gfx::Mesh::VertexLayout layout{
        gfx::Mesh::VertexField::Position,
    };
};

constexpr uint32_t g_cube_texture_size = 320U;
constexpr float    g_model_scale = 6.F;

CubeMapArrayApp::CubeMapArrayApp()
    : UserInterfaceApp(
        []() {
            Graphics::CombinedAppSettings settings = GetGraphicsTutorialAppSettings("Methane Cube Map Array", AppOptions::GetDefaultWithColorDepthAndAnim());
            settings.graphics_app.device_capabilities.features.SetBitOn(rhi::DeviceFeature::ImageCubeArray);
            settings.render_context.clear_depth_stencil = gfx::DepthStencilValues(0.F, {}); // Clear depth with 0.F to support reversed depth rendering
            settings.render_context.clear_color = {}; // Disable color clearing, use sky-box instead
            return settings;
        }(),
        GetUserInterfaceTutorialAppSettings(AppOptions::GetDefaultWithColorDepthAndAnim()),
        "Methane tutorial of cube-map array texturing")
    , m_model_matrix(hlslpp::mul(hlslpp::float4x4::scale(g_model_scale), // NOSONAR - do not use in-class initializer
                                 hlslpp::float4x4::rotation_z(std::numbers::pi_v<float>)))
{
    // NOTE: Near and Far values are swapped in camera parameters (1st value is near = max depth, 2nd value is far = min depth)
    // for Reversed-Z buffer values range [ near: 1, far 0], instead of [ near 0, far 1]
    // which is used for "from near to far" drawing order for reducing pixels overdraw
    m_camera.ResetOrientation({ { 13.F, 13.F, -13.F }, { 0.F, 0.F, 0.F }, { 0.F, 1.F, 0.F } });
    m_camera.SetParameters({ 600.F /* near = max depth */, 0.01F /*far = min depth*/, 90.F /* FOV */ });

    // Setup animations
    GetAnimations().emplace_back(Data::MakeTimeAnimationPtr([this](double elapsed_seconds, double delta_seconds)
    {
        return Animate(elapsed_seconds, delta_seconds);
    }));
}

CubeMapArrayApp::~CubeMapArrayApp()
{
    // Wait for GPU rendering is completed to release resources
    WaitForRenderComplete();
}

void CubeMapArrayApp::Init()
{
    UserInterfaceApp::Init();

    const rhi::CommandQueue render_cmd_queue = GetRenderContext().GetRenderCommandKit().GetQueue();
    m_camera.Resize(GetRenderContext().GetSettings().frame_size);

    // Create cube mesh
    gfx::CubeMesh<CubeVertex> cube_mesh(CubeVertex::layout);

    // Create render state with program
    rhi::RenderState::Settings render_state_settings
    {
        .program = GetRenderContext().CreateProgram(
            rhi::Program::Settings
            {
                .shader_set = rhi::Program::ShaderSet
                {
                    { rhi::ShaderType::Vertex, { Data::ShaderProvider::Get(), { "CubeMapArray", "CubeVS" } } },
                    { rhi::ShaderType::Pixel,  { Data::ShaderProvider::Get(), { "CubeMapArray", "CubePS" } } },
                },
                .input_buffer_layouts = rhi::ProgramInputBufferLayouts
                {
                    rhi::IProgram::InputBufferLayout
                    {
                        rhi::IProgram::InputBufferLayout::ArgumentSemantics { cube_mesh.GetVertexLayout().GetSemantics() }
                    }
                },
                .argument_accessors = rhi::ProgramArgumentAccessors
                {
                    META_PROGRAM_ARG_ROOT_BUFFER_FRAME_CONSTANT(rhi::ShaderType::Vertex, "g_uniforms")
                },
                .attachment_formats = GetScreenRenderPattern().GetAttachmentFormats()
            }),
        .render_pattern = GetScreenRenderPattern()
    };
    render_state_settings.program.SetName("Render Pipeline State");
    render_state_settings.depth.enabled = true;
    render_state_settings.depth.compare = gfx::Compare::GreaterEqual; // Reversed depth rendering
    m_render_state = GetRenderContext().CreateRenderState( render_state_settings);

    // Create cube mesh buffer resources
    m_cube_buffers_ptr = std::make_unique<TexturedMeshBuffers>(render_cmd_queue, std::move(cube_mesh), "Cube");

    // Create cube-map render target texture
    m_cube_buffers_ptr->SetTexture(
        rhi::Texture(
            GetRenderContext(),
            rhi::ITexture::Settings::ForCubeImage(
                g_cube_texture_size, CUBE_MAP_ARRAY_SIZE, gfx::PixelFormat::RGBA8Unorm, false,
                rhi::ResourceUsageMask({ rhi::ResourceUsage::RenderTarget, rhi::ResourceUsage::ShaderRead })
            )));

    // Create sampler for image texture
    m_texture_sampler = GetRenderContext().CreateSampler(
        rhi::Sampler::Settings
        {
            rhi::Sampler::Filter  { rhi::Sampler::Filter::MinMag::Linear },
            rhi::Sampler::Address { rhi::Sampler::Address::Mode::ClampToEdge }
        }
    );

    // Load cube-map texture images for Sky-box
    const rhi::Texture sky_box_texture = GetImageLoader().LoadImagesToTextureCube(render_cmd_queue,
        gfx::ImageLoader::CubeFaceResources
        {
            "SkyBox/Clouds/PositiveX.jpg",
            "SkyBox/Clouds/NegativeX.jpg",
            "SkyBox/Clouds/PositiveY.jpg",
            "SkyBox/Clouds/NegativeY.jpg",
            "SkyBox/Clouds/PositiveZ.jpg",
            "SkyBox/Clouds/NegativeZ.jpg"
        },
        gfx::ImageOptionMask(gfx::ImageOption::Mipmapped),
        "Sky-Box Texture"
    );

    // Create sky-box
    m_sky_box = gfx::SkyBox(render_cmd_queue, GetScreenRenderPattern(), sky_box_texture,
        gfx::SkyBox::Settings
        {
            .view_camera = m_camera,
            .scale = g_model_scale * 100.F,
            .render_options = gfx::SkyBox::OptionMask({ gfx::SkyBox::Option::DepthEnabled, gfx::SkyBox::Option::DepthReversed })
        });

    // Create frame buffer resources
    for(CubeMapArrayFrame& frame : GetFrames())
    {
        // Configure program resource bindings
        frame.cube.program_bindings = m_render_state.GetProgram().CreateBindings({
            { { rhi::ShaderType::Pixel,  "g_texture_array" }, m_cube_buffers_ptr->GetTexture().GetResourceView() },
            { { rhi::ShaderType::Pixel,  "g_sampler"       }, m_texture_sampler.GetResourceView()                },
        }, frame.index);
        frame.cube.uniforms_argument_binding_ptr = &frame.cube.program_bindings.Get({ rhi::ShaderType::Vertex, "g_uniforms" });
        frame.cube.program_bindings.SetName(fmt::format("Cube Bindings {}", frame.index));

        // Resource bindings for Sky-Box rendering
        std::tie(frame.sky_box.program_bindings, frame.sky_box.uniforms_argument_binding_ptr) = m_sky_box.CreateProgramBindings(frame.index);
        frame.sky_box.program_bindings.SetName(fmt::format("Space Sky-Box Bindings {}", frame.index));
        
        // Create command list for rendering
        frame.render_cmd_list = render_cmd_queue.CreateRenderCommandList(frame.screen_pass);
        frame.render_cmd_list.SetName(fmt::format("Cube Rendering {}", frame.index));
        frame.execute_cmd_list_set = rhi::CommandListSet({ frame.render_cmd_list.GetInterface() }, frame.index);
    }
    
    // Create all resources for texture labels rendering before resources upload in UserInterfaceApp::CompleteInitialization()
    TextureLabeler cube_texture_labeler(GetUIContext(), GetFontContext(), m_cube_buffers_ptr->GetTexture(),
                                        rhi::ResourceState::Undefined, { g_cube_texture_size / 4U, 10U });

    // Upload all resources, including font texture and text mesh buffers required for rendering
    UserInterfaceApp::CompleteInitialization();
    
    // Encode and execute texture labels rendering commands when all resources are uploaded and ready on GPU
    cube_texture_labeler.Render();

    GetRenderContext().WaitForGpu(rhi::IContext::WaitFor::RenderComplete);
}

bool CubeMapArrayApp::Animate(double, double delta_seconds)
{
    m_camera.Rotate(m_camera.GetOrientation().up, static_cast<float>(delta_seconds * 360.0 / 16.0));
    m_model_matrix = hlslpp::mul(m_model_matrix,
                                 hlslpp::mul(hlslpp::float4x4::rotation_z(static_cast<float>(delta_seconds * std::numbers::pi / 2.0)),
                                             hlslpp::float4x4::rotation_y(static_cast<float>(delta_seconds * std::numbers::pi / 4.0))));
    return true;
}

bool CubeMapArrayApp::Resize(const gfx::FrameSize& frame_size, bool is_minimized)
{
    // Resize screen color and depth textures
    if (!UserInterfaceApp::Resize(frame_size, is_minimized))
        return false;

    m_camera.Resize(frame_size);
    return true;
}

bool CubeMapArrayApp::Update()
{
    if (!UserInterfaceApp::Update())
        return false;

    static const auto   s_cbrt_count      = static_cast<size_t>(std::floor(std::cbrt(float(CUBE_MAP_ARRAY_SIZE))));
    static const size_t s_cbrt_count_sqr  = s_cbrt_count * s_cbrt_count;
    static const float  s_cbrt_count_half = static_cast<float>(s_cbrt_count - 1) / 2.f;
    const float ts = g_model_scale * 1.7F;

    // Update MVP-matrices for all cube instances so that they are positioned in a cube grid
    hlslpp::Uniforms uniforms{};
    for(size_t i = 0; i < CUBE_MAP_ARRAY_SIZE; ++i)
    {
        const float tx = static_cast<float>(i % s_cbrt_count) - s_cbrt_count_half;
        const float ty = static_cast<float>(i % s_cbrt_count_sqr / s_cbrt_count) - s_cbrt_count_half;
        const float tz = static_cast<float>(i / s_cbrt_count_sqr) - s_cbrt_count_half;
        const hlslpp::float4x4 translation_matrix = hlslpp::float4x4::translation(tx * ts, ty * ts, tz * ts);
        uniforms.mvp_matrix_per_instance[i] = hlslpp::transpose(hlslpp::mul(hlslpp::mul(m_model_matrix, translation_matrix), m_camera.GetViewProjMatrix()));
    }

    const CubeMapArrayFrame& frame = GetCurrentFrame();
    frame.cube.uniforms_argument_binding_ptr->SetRootConstant(rhi::RootConstant(uniforms));
    m_sky_box.Update(*frame.sky_box.uniforms_argument_binding_ptr);

    return true;
}

bool CubeMapArrayApp::Render()
{
    if (!UserInterfaceApp::Render())
        return false;

    const CubeMapArrayFrame& frame = GetCurrentFrame();

    // 1) Render cube instances of 'CUBE_MAP_ARRAY_SIZE' count
    META_DEBUG_GROUP_VAR(s_debug_group, "Cube Instances Rendering");
    frame.render_cmd_list.ResetWithState(m_render_state, &s_debug_group);
    frame.render_cmd_list.SetViewState(GetViewState());
    m_cube_buffers_ptr->Draw(frame.render_cmd_list, frame.cube.program_bindings, 0U, CUBE_MAP_ARRAY_SIZE);

    // 2) Render sky-box after cubes to minimize overdraw
    m_sky_box.Draw(frame.render_cmd_list, frame.sky_box.program_bindings, GetViewState());

    RenderOverlay(frame.render_cmd_list);

    frame.render_cmd_list.Commit();

    // Execute command list on render queue and present frame to screen
    GetRenderContext().GetRenderCommandKit().GetQueue().Execute(frame.execute_cmd_list_set);
    GetRenderContext().Present();

    return true;
}

void CubeMapArrayApp::OnContextReleased(rhi::IContext& context)
{
    m_sky_box = {};
    m_cube_buffers_ptr.reset();
    m_texture_sampler = {};
    m_render_state = {};

    UserInterfaceApp::OnContextReleased(context);
}

} // namespace Methane::Tutorials

int main(int argc, const char* argv[])
{
    return Methane::Tutorials::CubeMapArrayApp().Run({ argc, argv });
}
