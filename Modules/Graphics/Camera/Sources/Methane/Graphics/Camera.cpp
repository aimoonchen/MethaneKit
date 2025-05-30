/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Camera.cpp
Camera helper implementation allowing to generate view and projection matrices.

******************************************************************************/

#include <Methane/Graphics/Camera.h>
#include <Methane/Graphics/TypeFormatters.hpp>
#include <Methane/Data/Math.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

Camera::Camera() noexcept
{
    META_FUNCTION_TASK();
    ResetOrientation();
}

void Camera::Resize(const Data::FloatSize& screen_size)
{
    META_FUNCTION_TASK();
    m_screen_size = screen_size;
    m_aspect_ratio = screen_size.GetWidth()  / screen_size.GetHeight();
    m_is_current_proj_matrix_dirty = true;
    UpdateProjectionSettings();
}

void Camera::SetProjection(Projection projection)
{
    META_FUNCTION_TASK();
    m_projection = projection;
    m_is_current_proj_matrix_dirty = true;
}

void Camera::SetParameters(const Parameters& parameters)
{
    META_FUNCTION_TASK();
    m_parameters = parameters;
    m_is_current_proj_matrix_dirty = true;
    UpdateProjectionSettings();
}

hlslpp::frustum Camera::CreateFrustum() const
{
    META_FUNCTION_TASK();
    switch (m_projection)
    {
    case Projection::Perspective:
        return hlslpp::frustum::field_of_view_y(GetFovAngleY(), m_aspect_ratio, m_parameters.near_depth, m_parameters.far_depth);

    case Projection::Orthogonal:
        return hlslpp::frustum(m_screen_size.GetWidth(), m_screen_size.GetHeight(), m_parameters.near_depth, m_parameters.far_depth);

    default:
        META_UNEXPECTED_RETURN(m_projection, hlslpp::frustum(0.F, 0.F, 0.F, 0.F));
    }
}

void Camera::UpdateProjectionSettings()
{
    META_FUNCTION_TASK();
    m_projection_settings = hlslpp::projection(CreateFrustum(), hlslpp::zclip::zero);
}

void Camera::Rotate(const hlslpp::float3& axis, float angle_deg) noexcept
{
    META_FUNCTION_TASK();
    const hlslpp::float3x3 rotation_matrix = hlslpp::float3x3::rotation_axis(axis, Data::DegreeToRadians(angle_deg));
    const hlslpp::float3   new_look_dir    = hlslpp::mul(GetLookDirection(), rotation_matrix);
    SetOrientationEye(GetOrientation().aim - new_look_dir);
}

hlslpp::float4x4 Camera::CreateViewMatrix(const Orientation& orientation) const noexcept
{
    META_FUNCTION_TASK();
    return hlslpp::float4x4::look_at(orientation.eye, orientation.aim, orientation.up);
}

hlslpp::float4x4 Camera::CreateProjMatrix() const
{
    META_FUNCTION_TASK();
    META_CHECK_NOT_NULL_DESCR(m_projection_settings, "can not create projection matrix until parameters, projection and size are initialized");

    switch (m_projection)
    {
    case Projection::Perspective:
        return hlslpp::float4x4::perspective(*m_projection_settings);

    case Projection::Orthogonal:
        return hlslpp::float4x4::orthographic(*m_projection_settings);

    default:
        META_UNEXPECTED_RETURN(m_projection, hlslpp::float4x4{});
    }
}

const hlslpp::float4x4& Camera::GetViewMatrix() const noexcept
{
    META_FUNCTION_TASK();
    if (!m_is_current_view_matrix_dirty)
        return m_current_view_matrix;

    m_current_view_matrix = CreateViewMatrix(m_current_orientation);
    m_is_current_view_matrix_dirty = false;
    return m_current_view_matrix;
}

const hlslpp::float4x4& Camera::GetProjMatrix() const
{
    META_FUNCTION_TASK();
    if (!m_is_current_proj_matrix_dirty)
        return m_current_proj_matrix;

    m_current_proj_matrix = CreateProjMatrix();
    m_is_current_proj_matrix_dirty = false;
    return m_current_proj_matrix;
}

const hlslpp::float4x4& Camera::GetViewProjMatrix() const noexcept
{
    META_FUNCTION_TASK();
    if (!m_is_current_view_matrix_dirty && !m_is_current_proj_matrix_dirty)
        return m_current_view_proj_matrix;

    m_current_view_proj_matrix = hlslpp::mul(GetViewMatrix(), GetProjMatrix());
    return m_current_view_proj_matrix;
}

hlslpp::float2 Camera::TransformScreenToProj(const Data::Point2I& screen_pos) const noexcept
{
    META_FUNCTION_TASK();
    return { 2.F * static_cast<float>(screen_pos.GetX()) / m_screen_size.GetWidth()  - 1.F,
           -(2.F * static_cast<float>(screen_pos.GetY()) / m_screen_size.GetHeight() - 1.F) };
}

hlslpp::float3 Camera::TransformScreenToView(const Data::Point2I& screen_pos) const noexcept
{
    META_FUNCTION_TASK();
    return hlslpp::mul(hlslpp::inverse(GetProjMatrix()), hlslpp::float4(TransformScreenToProj(screen_pos), 0.F, 1.F)).xyz;
}

hlslpp::float3 Camera::TransformScreenToWorld(const Data::Point2I& screen_pos) const noexcept
{
    META_FUNCTION_TASK();
    return TransformViewToWorld(TransformScreenToView(screen_pos));
}

hlslpp::float4 Camera::TransformWorldToView(const hlslpp::float4& world_pos, const Orientation& orientation) const noexcept
{
    META_FUNCTION_TASK();
    return hlslpp::mul(hlslpp::inverse(CreateViewMatrix(orientation)), world_pos);
}

hlslpp::float4 Camera::TransformViewToWorld(const hlslpp::float4& view_pos, const Orientation& orientation) const noexcept
{
    META_FUNCTION_TASK();
    return hlslpp::mul(CreateViewMatrix(orientation), view_pos);
}

float Camera::GetFovAngleY() const noexcept
{
    META_FUNCTION_TASK();
    float fov_angle_y = Data::DegreeToRadians(m_parameters.fov_deg);
    if (m_aspect_ratio != 0.F && m_aspect_ratio < 1.0F)
    {
        fov_angle_y /= (0.5F + m_aspect_ratio / 2.F);
    }
    return fov_angle_y;
}

std::string Camera::GetOrientationString() const
{
    return fmt::format("Camera orientation:\n  - eye: {}\n  - aim: {}\n  - up:  {}",
                       m_current_orientation.eye, m_current_orientation.aim, m_current_orientation.up);
}

void Camera::LogOrientation() const
{
    META_LOG("Camera orientation:\n  - eye: {}\n  - aim: {}\n  - up:  {}",
             m_current_orientation.eye, m_current_orientation.aim, m_current_orientation.up);
}

} // namespace Methane::Graphics
