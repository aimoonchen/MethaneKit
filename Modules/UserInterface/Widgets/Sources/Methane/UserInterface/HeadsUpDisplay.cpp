/******************************************************************************

Copyright 2020-2021 Evgeny Gorodetskiy

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

FILE: Methane/UserInterface/HeadsUpDisplay.cpp
Heads-Up-Display widget for displaying runtime rendering parameters.
 --------------------------------------------------
 | F1 - Help     | GPU Adapter Name               |
 |-------------- |--------------------------------|
 | Frame Time ms |                                |
 |-------------- | 123 FPS (Major Font)           |
 | CPU Time %    |                                |
 |-------------- |--------------------------------|
 | VSync ON/OFF  | W x H       N FB      GFX API  |
 --------------------------------------------------

******************************************************************************/

#include <Methane/UserInterface/HeadsUpDisplay.h>
#include <Methane/UserInterface/FontLibrary.h>
#include <Methane/UserInterface/Text.h>
#include <Methane/UserInterface/Context.h>

#include <Methane/Graphics/RHI/RenderContext.h>
#include <Methane/Graphics/RHI/System.h>
#include <Methane/Data/IFpsCounter.h>
#include <Methane/Graphics/RHI/CommandListDebugGroup.h>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

#include <magic_enum/magic_enum.hpp>

namespace Methane::UserInterface
{

static constexpr uint32_t g_first_line_height_decrement = 5;

inline uint32_t GetTextHeightInDots(const Context& ui_context, const Font& font)
{
    return ui_context.ConvertPixelsToDots(font.GetMaxGlyphSize().GetHeight());
}

inline uint32_t GetFpsTextHeightInDots(const Context& ui_context, const Font& major_font, const Font& minor_font, const UnitSize& text_margins)
{
    return std::max(GetTextHeightInDots(ui_context, major_font), GetTextHeightInDots(ui_context, minor_font) * 2U + ui_context.ConvertTo<Units::Dots>(text_margins).GetHeight());
}

inline uint32_t GetTimingTextHeightInDots(const Context& ui_context, const Font& major_font, const Font& minor_font, const UnitSize& text_margins)
{
    return (GetFpsTextHeightInDots(ui_context, major_font, minor_font, text_margins) - ui_context.ConvertTo<Units::Dots>(text_margins).GetHeight()) / 2U;
}

HeadsUpDisplay::Settings& HeadsUpDisplay::Settings::SetMajorFont(const Font::Description& new_major_font) noexcept
{
    META_FUNCTION_TASK();
    major_font = new_major_font;
    return *this;
}

HeadsUpDisplay::Settings& HeadsUpDisplay::Settings::SetMinorFont(const Font::Description& new_minor_font) noexcept
{
    META_FUNCTION_TASK();
    minor_font = new_minor_font;
    return *this;
}

HeadsUpDisplay::Settings& HeadsUpDisplay::Settings::SetPosition(const UnitPoint& new_position) noexcept
{
    META_FUNCTION_TASK();
    position = new_position;
    return *this;
}

HeadsUpDisplay::Settings& HeadsUpDisplay::Settings::SetTextMargins(const UnitSize& new_text_margins) noexcept
{
    META_FUNCTION_TASK();
    text_margins = new_text_margins;
    return *this;
}

HeadsUpDisplay::Settings& HeadsUpDisplay::Settings::SetTextColor(const Color4F& new_text_color) noexcept
{
    META_FUNCTION_TASK();
    text_color = new_text_color;
    return *this;
}

HeadsUpDisplay::Settings& HeadsUpDisplay::Settings::SetOnColor(const Color4F& new_on_color) noexcept
{
    META_FUNCTION_TASK();
    on_color = new_on_color;
    return *this;
}

HeadsUpDisplay::Settings& HeadsUpDisplay::Settings::SetOffColor(const Color4F& new_off_color) noexcept
{
    META_FUNCTION_TASK();
    off_color = new_off_color;
    return *this;
}

HeadsUpDisplay::Settings& HeadsUpDisplay::Settings::SetHelpColor(const Color4F& new_help_color) noexcept
{
    META_FUNCTION_TASK();
    help_color = new_help_color;
    return *this;
}

HeadsUpDisplay::Settings& HeadsUpDisplay::Settings::SetBackgroundColor(const Color4F& new_background_color) noexcept
{
    META_FUNCTION_TASK();
    background_color = new_background_color;
    return *this;
}

HeadsUpDisplay::Settings& HeadsUpDisplay::Settings::SetHelpShortcut(const pin::Keyboard::State& new_help_shortcut) noexcept
{
    META_FUNCTION_TASK();
    help_shortcut = new_help_shortcut;
    return *this;
}

HeadsUpDisplay::Settings& HeadsUpDisplay::Settings::SetUpdateIntervalSec(double new_update_interval_sec) noexcept
{
    META_FUNCTION_TASK();
    update_interval_sec = new_update_interval_sec;
    return *this;
}

HeadsUpDisplay::HeadsUpDisplay(Context& ui_context, const FontContext& font_context, const Settings& settings)
    : Panel(ui_context, { }, { "Heads Up Display" })
    , m_settings(settings)
    , m_major_font(
        font_context.GetFont(Font::Settings {
                m_settings.major_font,
                GetUIContext().GetFontResolutionDpi(),
                U"FPS0123456789",
            }
        )
    )
    , m_minor_font(
        font_context.GetFont(Font::Settings
            {
                m_settings.minor_font,
                GetUIContext().GetFontResolutionDpi(),
                Font::GetAlphabetDefault()
            }
        )
    )
    , m_text_blocks({
        std::make_shared<TextItem>(ui_context, m_major_font,
            Text::SettingsUtf8
            {
                "FPS",
                "000 FPS",
                UnitRect{ Units::Dots, gfx::Point2I{ }, gfx::FrameSize{ 0U, GetFpsTextHeightInDots(ui_context, m_major_font, m_minor_font, m_settings.text_margins) } },
                Text::Layout{ Text::Wrap::None, Text::HorizontalAlignment::Left, Text::VerticalAlignment::Center },
                m_settings.text_color
            }
        ),
        std::make_shared<TextItem>(ui_context, m_minor_font,
            Text::SettingsUtf8
            {
                "Frame Time",
                "00.00 ms",
                UnitRect{ Units::Dots, gfx::Point2I{ }, gfx::FrameSize{ 0U, GetTimingTextHeightInDots(ui_context, m_major_font, m_minor_font, m_settings.text_margins) } },
                Text::Layout{ Text::Wrap::None, Text::HorizontalAlignment::Left, Text::VerticalAlignment::Center },
                m_settings.text_color
            }
        ),
        std::make_shared<TextItem>(ui_context, m_minor_font,
            Text::SettingsUtf8
            {
                "CPU Time",
                "00.00% cpu",
                UnitRect{ Units::Dots, gfx::Point2I{ }, gfx::FrameSize{ 0U, GetTimingTextHeightInDots(ui_context, m_major_font, m_minor_font, m_settings.text_margins) } },
                Text::Layout{ Text::Wrap::None, Text::HorizontalAlignment::Left, Text::VerticalAlignment::Center },
                m_settings.text_color
            }
        ),
        std::make_shared<TextItem>(ui_context, m_minor_font,
            Text::SettingsUtf8
            {
                "GPU",
                "Graphics Adapter",
                UnitRect{ Units::Dots, gfx::Point2I{ }, gfx::FrameSize{ 0U, GetTextHeightInDots(ui_context, m_minor_font) - g_first_line_height_decrement } },
                Text::Layout{ Text::Wrap::None, Text::HorizontalAlignment::Left, Text::VerticalAlignment::Top },
                m_settings.text_color
            }
        ),
        std::make_shared<TextItem>(ui_context, m_minor_font,
            Text::SettingsUtf8
            {
                "Help",
                m_settings.help_shortcut ? m_settings.help_shortcut.ToString() + " - Help" : "",
                UnitRect{ Units::Dots, gfx::Point2I{ }, gfx::FrameSize{ 0U, GetTextHeightInDots(ui_context, m_minor_font) - g_first_line_height_decrement } },
                Text::Layout{ Text::Wrap::None, Text::HorizontalAlignment::Left, Text::VerticalAlignment::Top },
                m_settings.help_color
            }
        ),
        std::make_shared<TextItem>(ui_context, m_minor_font,
            Text::SettingsUtf8
            {
                "Frame Buffers",
                "0000 x 0000   3 FB   DirectX",
                UnitRect{ Units::Dots, gfx::Point2I{ }, gfx::FrameSize{ 0U, GetTextHeightInDots(ui_context, m_minor_font) } },
                Text::Layout{ Text::Wrap::None, Text::HorizontalAlignment::Justify, Text::VerticalAlignment::Top },
                m_settings.text_color
            }
        ),
        std::make_shared<TextItem>(ui_context, m_minor_font,
            Text::SettingsUtf8
            {
                "VSync",
                "VSync ON",
                UnitRect{ Units::Dots, gfx::Point2I{ }, gfx::FrameSize{ 0U, GetTextHeightInDots(ui_context, m_minor_font) } },
                Text::Layout{ Text::Wrap::None, Text::HorizontalAlignment::Left, Text::VerticalAlignment::Top },
                m_settings.on_color
            }
        )
    })
{
    META_FUNCTION_TASK();

    // Add HUD text blocks as children to the base panel container
    for(const Ptr<TextItem>& text_item_ptr : m_text_blocks)
    {
        AddChild(*text_item_ptr); // NOSONAR - method is not overridable in final class
    }

    // Reset timer behind so that HUD is filled with actual values on first update
    m_update_timer.ResetToSeconds(m_settings.update_interval_sec);
}

void HeadsUpDisplay::SetTextColor(const gfx::Color4F& text_color)
{
    META_FUNCTION_TASK();
    if (m_settings.text_color == text_color)
        return;

    m_settings.text_color = text_color;

    for(const Ptr<TextItem>& text_ptr : m_text_blocks)
    {
        text_ptr->SetColor(text_color);
    }
}

void HeadsUpDisplay::SetUpdateInterval(double update_interval_sec)
{
    META_FUNCTION_TASK();
    m_settings.update_interval_sec = update_interval_sec;
}

void HeadsUpDisplay::Update(const FrameSize& render_attachment_size)
{
    META_FUNCTION_TASK();
    if (m_update_timer.GetElapsedSecondsD() < m_settings.update_interval_sec)
    {
        UpdateAllTextBlocks(render_attachment_size);
        return;
    }

    const Data::IFpsCounter&          fps_counter      = GetUIContext().GetRenderContext().GetFpsCounter();
    const rhi::RenderContextSettings& context_settings = GetUIContext().GetRenderContext().GetSettings();

    using enum TextBlock;
    GetTextBlock(Fps).SetText(fmt::format("{:d} FPS", fps_counter.GetFramesPerSecond()));
    GetTextBlock(FrameTime).SetText(fmt::format("{:.2f} ms", fps_counter.GetAverageFrameTiming().GetTotalTimeMSec()));
    GetTextBlock(CpuTime).SetText(fmt::format("{:.2f}% cpu", fps_counter.GetAverageFrameTiming().GetCpuTimePercent()));
    GetTextBlock(GpuName).SetText(GetUIContext().GetRenderContext().GetDevice().GetAdapterName());
    GetTextBlock(FrameBuffersAndApi).SetText(fmt::format("{:d} x {:d}  {:d} FB  {:s}", // NOSONAR - string contains invisible NBSP symbols
                                                         context_settings.frame_size.GetWidth(),
                                                         context_settings.frame_size.GetHeight(),
                                                         context_settings.frame_buffers_count,
                                                         magic_enum::enum_name(rhi::ISystem::GetNativeApi())));
    GetTextBlock(VSync).SetText(context_settings.vsync_enabled ? "VSync ON" : "VSync OFF");
    GetTextBlock(VSync).SetColor(context_settings.vsync_enabled ? m_settings.on_color : m_settings.off_color);

    LayoutTextBlocks();
    UpdateAllTextBlocks(render_attachment_size);
    m_update_timer.Reset();
}

void HeadsUpDisplay::Draw(const rhi::RenderCommandList& cmd_list, const rhi::CommandListDebugGroup* debug_group_ptr) const
{
    META_FUNCTION_TASK();
    Panel::Draw(cmd_list, debug_group_ptr);

    for(const Ptr<TextItem>& text_ptr : m_text_blocks)
    {
        text_ptr->Draw(cmd_list, debug_group_ptr);
    }
}

TextItem& HeadsUpDisplay::GetTextBlock(TextBlock block) const
{
    META_FUNCTION_TASK();
    const Ptr<TextItem>& text_item_ptr = m_text_blocks[static_cast<size_t>(block)];
    META_CHECK_NOT_NULL(text_item_ptr);
    return *text_item_ptr;
}

void HeadsUpDisplay::LayoutTextBlocks()
{
    META_FUNCTION_TASK();
    const UnitSize text_margins_in_dots = GetUIContext().ConvertTo<Units::Dots>(m_settings.text_margins);

    // Layout left column text blocks
    using enum TextBlock;
    const FrameSize help_size          = GetTextBlock(HelpKey).GetRectInDots().size;
    const FrameSize frame_time_size    = GetTextBlock(FrameTime).GetRectInDots().size;
    const FrameSize cpu_time_size      = GetTextBlock(CpuTime).GetRectInDots().size;
    const FrameSize vsync_size         = GetTextBlock(VSync).GetRectInDots().size;
    const uint32_t  left_column_width  = std::max({ help_size.GetWidth(), frame_time_size.GetWidth(), cpu_time_size.GetWidth(), vsync_size.GetWidth() });

    UnitPoint position(Units::Dots, text_margins_in_dots.GetWidth(), text_margins_in_dots.GetHeight());
    GetTextBlock(HelpKey).SetRelOrigin(position);

    position.SetY(position.GetY() + help_size.GetHeight() + text_margins_in_dots.GetHeight());
    GetTextBlock(FrameTime).SetRelOrigin(position);

    position.SetY(position.GetY() + frame_time_size.GetHeight() + text_margins_in_dots.GetHeight());
    GetTextBlock(CpuTime).SetRelOrigin(position);

    position.SetY(position.GetY() + cpu_time_size.GetHeight() + text_margins_in_dots.GetHeight());
    GetTextBlock(VSync).SetRelOrigin(position);

    // Layout right column text blocks
    const FrameSize gpu_name_size      = GetTextBlock(GpuName).GetRectInDots().size;
    const FrameSize fps_size           = GetTextBlock(Fps).GetRectInDots().size;
    const FrameSize frame_buffers_size = GetTextBlock(FrameBuffersAndApi).GetRectInDots().size;
    const uint32_t  right_column_width = std::max({ gpu_name_size.GetWidth(), fps_size.GetWidth(), frame_buffers_size.GetWidth() });

    position.SetX(left_column_width + 2 * text_margins_in_dots.GetWidth());
    GetTextBlock(FrameBuffersAndApi).SetRelOrigin(position);
    GetTextBlock(FrameBuffersAndApi).SetSize(UnitSize(Units::Dots, right_column_width, frame_buffers_size.GetHeight()));

    const UnitPoint right_bottom_position = position;

    position.SetY(text_margins_in_dots.GetHeight());
    GetTextBlock(GpuName).SetRelOrigin(position);

    position.SetY(position.GetY() + gpu_name_size.GetHeight() + text_margins_in_dots.GetHeight());
    GetTextBlock(Fps).SetRelOrigin(position);

    Panel::SetRect(UnitRect{
        Units::Dots,
        m_settings.position,
        gfx::FrameSize
        {
            right_bottom_position.GetX() + right_column_width + text_margins_in_dots.GetWidth(),
            right_bottom_position.GetY() + vsync_size.GetHeight() + text_margins_in_dots.GetHeight()
        }
    });
}

void HeadsUpDisplay::UpdateAllTextBlocks(const FrameSize& render_attachment_size) const
{
    META_FUNCTION_TASK();
    for(const Ptr<TextItem>& text_ptr : m_text_blocks)
    {
        text_ptr->Update(render_attachment_size);
    }
}

} // namespace Methane::UserInterface
