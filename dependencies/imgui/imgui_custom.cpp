#pragma once
#include "imgui.h"
#include <cmath>
#include <unordered_map>
#include <fonts/IconsFontAwesome6.h>
#include <imgui_settings.hpp>
#include <imgui_internal.h>
#include <string>

namespace custom
{

    struct TabTheme
    {
        ImU32 active_fill;    // translucent accent fill when selected / hovered
        ImU32 active_border;  // accent outline
        ImU32 hover_fill;     // lighter tint on hover (idle state)
        ImU32 active_text;    // accent text / icon colour
        ImU32 idle_text;      // muted zinc text / icon colour
    };

    inline const TabTheme& tab_theme(int idx)
    {
        static const TabTheme themes[3] = {
            // 0 – Login  (blue)
            {
                IM_COL32(37,  99, 235,  18),
                IM_COL32(59, 130, 246,  51),
                IM_COL32(59, 130, 246,  26),
                IM_COL32(96, 165, 250, 255),
                IM_COL32(113, 113, 122, 220),
            },
            // 1 – Register  (green)
            {
                IM_COL32(22, 163,  74,  18),
                IM_COL32(34, 197,  94,  51),
                IM_COL32(74, 222, 128,  26),
                IM_COL32(74, 222, 128, 255),
                IM_COL32(113, 113, 122, 220),
            },
            // 2 – Extend  (amber)
            {
                IM_COL32(245, 158,  11,  18),
                IM_COL32(251, 191,  36,  51),
                IM_COL32(251, 191,  36,  26),
                IM_COL32(251, 191,  36, 255),
                IM_COL32(113, 113, 122, 220),
            },
        };
        return themes[(idx >= 0 && idx < 3) ? idx : 0];
    }

    inline const char* tab_icon(int idx)
    {
        static const char* icons[3] = {
            ICON_FA_RIGHT_TO_BRACKET,
            ICON_FA_USER_PLUS,
            ICON_FA_CLOCK_ROTATE_LEFT,
        };
        return icons[(idx >= 0 && idx < 3) ? idx : 0];
    }

    inline const char* tab_label(int idx)
    {
        static const char* labels[3] = { "Login", "Register", "Extend" };
        return labels[(idx >= 0 && idx < 3) ? idx : 0];
    }

    // =========================================================================
    // Helpers
    // =========================================================================

    void draw_rect_filled_with_outline(ImVec2 top_left, ImVec2 bottom_right,
        ImU32 outline_color, ImU32 fill_color,
        float rounding)
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(top_left, bottom_right, outline_color, rounding);
        top_left.x += 1.f; top_left.y += 1.f;
        bottom_right.x -= 1.f; bottom_right.y -= 1.f;
        draw->AddRectFilled(top_left, bottom_right, fill_color, rounding);
    }

    void draw_rect_3d_hover(
        ImVec2  top_left, ImVec2  bottom_right,
        ImU32   outline_color, ImU32 fill_color,
        float   rounding = 4.f,
        float   max_tilt_deg = 20.f,
        float   perspective_dist = 800.f,
        float   lerp_speed = 0.18f
    )
    {
        static float smooth_rx = 0.f, smooth_ry = 0.f;
        const float DEG2RAD = 3.14159265f / 180.f;

        ImVec2 center = { (top_left.x + bottom_right.x) * 0.5f,
                          (top_left.y + bottom_right.y) * 0.5f };
        float half_w = (bottom_right.x - top_left.x) * 0.5f;
        float half_h = (bottom_right.y - top_left.y) * 0.5f;

        ImVec2 mouse = ImGui::GetMousePos();
        bool   hovered = (mouse.x >= top_left.x && mouse.x <= bottom_right.x &&
            mouse.y >= top_left.y && mouse.y <= bottom_right.y);

        float target_rx = 0.f, target_ry = 0.f;
        if (hovered && half_w > 0.f && half_h > 0.f)
        {
            float nx = (mouse.x - center.x) / half_w;
            float ny = (mouse.y - center.y) / half_h;
            target_ry = -nx * max_tilt_deg;
            target_rx = -ny * max_tilt_deg;
        }
        smooth_rx += (target_rx - smooth_rx) * lerp_speed;
        smooth_ry += (target_ry - smooth_ry) * lerp_speed;

        auto project = [&](float lx, float ly) -> ImVec2 {
            float rx_rad = smooth_rx * DEG2RAD, ry_rad = smooth_ry * DEG2RAD;
            float x1 = lx * cosf(ry_rad), z1 = lx * sinf(ry_rad);
            float y2 = ly * cosf(rx_rad) - z1 * sinf(rx_rad);
            float z2 = ly * sinf(rx_rad) + z1 * cosf(rx_rad);
            float w = perspective_dist / (perspective_dist - z2);
            return { center.x + x1 * w, center.y + y2 * w };
            };

        ImVec2 tl = project(-half_w, -half_h), tr = project(half_w, -half_h);
        ImVec2 br = project(half_w, half_h), bl = project(-half_w, half_h);
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddQuadFilled(tl, tr, br, bl, outline_color);

        constexpr float B = 1.f;
        ImVec2 itl = project(-(half_w - B), -(half_h - B)), itr = project(half_w - B, -(half_h - B));
        ImVec2 ibr = project(half_w - B, half_h - B), ibl = project(-(half_w - B), half_h - B);
        draw->AddQuadFilled(itl, itr, ibr, ibl, fill_color);
        if (hovered) draw->AddQuadFilled(itl, itr, ibr, ibl, IM_COL32(255, 255, 255, 15));
        (void)rounding;
    }

    bool input_3d(
        const char* label,
        const char* imgui_id,
        char* buf,
        size_t               buf_size,
        float                width = 0.f,
        float                height = 28.f,
        ImU32                outline_color = IM_COL32(62, 62, 62, 255),
        ImU32                fill_color = IM_COL32(24, 24, 24, 255),
        float                rounding = 4.f,
        ImGuiInputTextFlags  flags = 0
    )
    {
        struct CharState { float alpha = 0.f; };
        struct WidgetState {
            float     focus_t = 0.f, smooth_rx = 0.f, smooth_ry = 0.f;
            int       prev_len = 0;
            CharState chars[256] = {};
        };
        static std::unordered_map<ImGuiID, WidgetState> s_states;

        ImGuiID      wid = ImGui::GetID(imgui_id);
        WidgetState& st = s_states[wid];

        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2      pos = ImGui::GetCursorScreenPos();
        float       w = (width > 0.f) ? width : ImGui::GetContentRegionAvail().x;

        ImVec2 box_min = { pos.x,     pos.y };
        ImVec2 box_max = { pos.x + w, pos.y + height };
        ImVec2 center = { (box_min.x + box_max.x) * 0.5f, (box_min.y + box_max.y) * 0.5f };
        float  half_w = w * 0.5f, half_h = height * 0.5f;

        const float D2R = 3.14159265f / 180.f, PDIST = 800.f;
        auto project = [&](float lx, float ly) -> ImVec2 {
            float rxr = st.smooth_rx * D2R, ryr = st.smooth_ry * D2R;
            float x1 = lx * cosf(ryr), z1 = lx * sinf(ryr);
            float y2 = ly * cosf(rxr) - z1 * sinf(rxr);
            float z2 = ly * sinf(rxr) + z1 * cosf(rxr);
            float ww = PDIST / (PDIST - z2);
            return { center.x + x1 * ww, center.y + y2 * ww };
            };

        auto lerp_col = [](ImU32 a, ImU32 b, float t) -> ImU32 {
            ImVec4 ca = ImGui::ColorConvertU32ToFloat4(a), cb = ImGui::ColorConvertU32ToFloat4(b);
            return ImGui::ColorConvertFloat4ToU32({
                ca.x + (cb.x - ca.x) * t, ca.y + (cb.y - ca.y) * t,
                ca.z + (cb.z - ca.z) * t, ca.w + (cb.w - ca.w) * t });
            };
        ImU32 animated_outline = lerp_col(outline_color, IM_COL32(160, 160, 170, 255), st.focus_t);

        ImVec2 tl = project(-half_w, -half_h), tr = project(half_w, -half_h);
        ImVec2 br = project(half_w, half_h), bl = project(-half_w, half_h);
        draw->AddQuadFilled(tl, tr, br, bl, animated_outline);

        const float B = 1.f;
        ImVec2 itl = project(-(half_w - B), -(half_h - B)), itr = project(half_w - B, -(half_h - B));
        ImVec2 ibr = project(half_w - B, half_h - B), ibl = project(-(half_w - B), half_h - B);
        draw->AddQuadFilled(itl, itr, ibr, ibl, fill_color);
        if (st.focus_t > 0.01f)
            draw->AddQuadFilled(itl, itr, ibr, ibl, IM_COL32(255, 255, 255, (int)(st.focus_t * 10.f)));

        const float text_pad = 8.f;
        const float pad_y = (height - ImGui::GetTextLineHeight()) * 0.5f;

        ImGui::SetCursorScreenPos({ box_min.x, box_min.y });
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(text_pad, pad_y));
        ImGui::PushItemWidth(w);
        bool changed = ImGui::InputText(imgui_id, buf, buf_size, flags);
        bool focused = ImGui::IsItemActive();
        bool hovered = ImGui::IsItemHovered();
        ImGui::PopItemWidth();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(5);

        ImVec2 mouse = ImGui::GetMousePos();
        float  target_rx = 0.f, target_ry = 0.f;
        if (hovered) {
            float nx = (mouse.x - center.x) / half_w;
            float ny = (mouse.y - center.y) / half_h;
            target_ry = -nx * 12.f; target_rx = -ny * 12.f;
        }
        float tilt_lerp = hovered ? 0.18f : 0.007f;
        st.smooth_rx += (target_rx - st.smooth_rx) * tilt_lerp;
        st.smooth_ry += (target_ry - st.smooth_ry) * tilt_lerp;

        float focus_target = focused ? 1.f : (hovered ? 0.35f : 0.f);
        st.focus_t += (focus_target - st.focus_t) * 0.12f;

        int cur_len = (int)strlen(buf);
        if (cur_len > st.prev_len)
            for (int i = st.prev_len; i < cur_len && i < 255; ++i) st.chars[i].alpha = 0.f;
        st.prev_len = cur_len;
        for (int i = 0; i < cur_len && i < 255; ++i)
            st.chars[i].alpha += (1.f - st.chars[i].alpha) * 0.22f;

        float  text_h = ImGui::GetTextLineHeight();
        ImVec2 text_origin = project(-(half_w - B - text_pad), 0.f);
        text_origin.y -= text_h * 0.5f;
        ImFont* font = ImGui::GetFont();
        bool    empty = (cur_len == 0);

        float total_text_width = 0.f;
        {
            float font_size = ImGui::GetFontSize();
            bool  is_pw = (flags & ImGuiInputTextFlags_Password) != 0;
            if (ImFontBaked* baked = font->GetFontBaked(font_size)) {
                for (int i = 0; i < cur_len && i < 255; ++i) {
                    char dc = is_pw ? '*' : buf[i];
                    const ImFontGlyph* g = baked->FindGlyph((ImWchar)(unsigned char)dc);
                    total_text_width += g ? g->AdvanceX : font_size * 0.5f;
                }
            }
            else total_text_width = cur_len * font_size * 0.5f;
        }
        float available_w = w - (text_pad + B) * 2.f;
        float scroll_offset = (total_text_width > available_w) ? total_text_width - available_w : 0.f;

        draw->PushClipRect({ box_min.x + text_pad, box_min.y }, { box_max.x - text_pad, box_max.y }, true);
        if (empty) {
            float ph_alpha = 180.f * (1.f - st.focus_t * 1.5f);
            if (ph_alpha > 0.f)
                draw->AddText(font, ImGui::GetFontSize(), text_origin,
                    IM_COL32(80, 80, 80, (int)ph_alpha), label);
        }
        else {
            bool   is_pw = (flags & ImGuiInputTextFlags_Password) != 0;
            float  local_start_x = -(half_w - B - text_pad) - scroll_offset;
            ImVec2 char_pos = project(local_start_x, 0.f);
            char_pos.y -= text_h * 0.5f;
            float font_size = ImGui::GetFontSize();
            for (int i = 0; i < cur_len && i < 255; ++i) {
                char dc = is_pw ? '*' : buf[i];
                ImU32 col = IM_COL32(200, 200, 200, (int)(st.chars[i].alpha * 255.f));
                float advance = font_size * 0.5f;
                if (ImFontBaked* baked = font->GetFontBaked(font_size)) {
                    const ImFontGlyph* g = baked->FindGlyph((ImWchar)(unsigned char)dc);
                    if (g) advance = g->AdvanceX;
                }
                char txt[2] = { dc, '\0' };
                draw->AddText(font, font_size, char_pos, col, txt);
                char_pos.x += advance;
            }
        }
        draw->PopClipRect();
        (void)rounding;
        return changed;
    }

    struct TabBarState {
        float hover_t[3] = {};
        float active_t[3] = {};
        float slide_x = -1.f;
    };

    void tab_bar(
        ImVec2  origin,
        float   width,
        float   height = 28.f,
        int* selected = nullptr,
        float   expand_t = 0.f,
        ImU32   pill_col = IM_COL32(24, 24, 24, 102),
        ImU32   pill_border = IM_COL32(63, 63, 65, 128)
    )
    {
        static std::unordered_map<ImGuiID, TabBarState> s_tabs;
        ImGuiID      gid = ImGui::GetID("##tabbar");
        TabBarState& st = s_tabs[gid];

        const char* full_labels[3] = { "Login", "Register", "Extend" };
        const char* compact_labels[3] = {
            ICON_FA_RIGHT_TO_BRACKET, ICON_FA_USER_PLUS, ICON_FA_CLOCK_ROTATE_LEFT
        };
        const int N = 3;

        const float outer_r = 8.f;
        const float inner_r = 6.f;
        const float pad = 4.f;
        const float cell_gap = 3.f;
        const float tab_h = height - pad * 2.f;

        const float icon_tab_w = 36.f;
        const float full_tab_w = (width - pad * 2.f) / (float)N;
        const float tab_w = icon_tab_w + (full_tab_w - icon_tab_w) * expand_t;
        const float total_w = tab_w * (float)N + pad * 2.f;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2      mouse = ImGui::GetMousePos();
        int         sel = (selected && *selected >= 0 && *selected < N) ? *selected : 0;

        ImVec2 pill_min = { origin.x + (width - total_w) * 0.5f * (1.f - expand_t), origin.y };
        ImVec2 pill_max = { pill_min.x + total_w, origin.y + height };
        dl->AddRectFilled(pill_min, pill_max, pill_col, outer_r);
        dl->AddRect(pill_min, pill_max, pill_border, outer_r, 0, 1.f);

        const float bar_offset_x = (width - total_w) * 0.5f * (1.f - expand_t);
        for (int i = 0; i < N; ++i)
        {
            const TabTheme& th = tab_theme(i);

            float  tx = origin.x + bar_offset_x + pad + i * tab_w;
            float  ty = origin.y + pad;
            ImVec2 tmin = { tx + cell_gap, ty };
            ImVec2 tmax = { tx + tab_w - cell_gap, ty + tab_h };
            ImVec2 tc = { tx + tab_w * 0.5f, ty + tab_h * 0.5f };

            bool hov = (mouse.x >= tmin.x && mouse.x <= tmax.x &&
                mouse.y >= tmin.y && mouse.y <= tmax.y);
            if (hov && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && selected)
                *selected = i;

            bool is_active = (i == sel);
            st.hover_t[i] += ((hov ? 1.f : 0.f) - st.hover_t[i]) * 0.20f;
            st.active_t[i] += ((is_active ? 1.f : 0.f) - st.active_t[i]) * 0.20f;

            auto scale_alpha = [](ImU32 col, float t) -> ImU32 {
                int a = (int)(((col >> IM_COL32_A_SHIFT) & 0xFF) * t);
                return (col & ~(0xFF << IM_COL32_A_SHIFT)) | (a << IM_COL32_A_SHIFT);
                };

            if (st.active_t[i] > 0.005f) {
                dl->AddRectFilled(tmin, tmax, scale_alpha(th.active_fill, st.active_t[i]), inner_r);
                dl->AddRect(tmin, tmax, scale_alpha(th.active_border, st.active_t[i]), inner_r, 0, 1.f);
            }
            if (!is_active && st.hover_t[i] > 0.005f)
                dl->AddRectFilled(tmin, tmax, scale_alpha(th.hover_fill, st.hover_t[i]), inner_r);

            ImU32 txt_col;
            if (is_active) {
                auto lc = [&](int s) -> int {
                    return (int)(((th.idle_text >> s) & 0xFF)
                        + (((th.active_text >> s) & 0xFF) - ((th.idle_text >> s) & 0xFF)) * st.active_t[i]);
                    };
                txt_col = IM_COL32(lc(IM_COL32_R_SHIFT), lc(IM_COL32_G_SHIFT), lc(IM_COL32_B_SHIFT), 255);
            }
            else {
                float brightness = 0.65f + st.hover_t[i] * 0.35f;
                float ht = st.hover_t[i] * 0.55f;
                auto  blc = [&](int s) -> int {
                    int idle = (th.idle_text >> s) & 0xFF, acc = (th.active_text >> s) & 0xFF;
                    return (int)((idle + (acc - idle) * ht) * brightness);
                    };
                txt_col = IM_COL32(blc(IM_COL32_R_SHIFT), blc(IM_COL32_G_SHIFT), blc(IM_COL32_B_SHIFT), 220);
            }

            auto with_alpha = [](ImU32 col, float a) -> ImU32 {
                int base_a = (col >> IM_COL32_A_SHIFT) & 0xFF;
                return (col & ~(0xFF << IM_COL32_A_SHIFT)) | ((int)(base_a * a) << IM_COL32_A_SHIFT);
                };

            float icon_alpha = ImClamp(1.f - expand_t / 0.5f, 0.f, 1.f);
            float text_alpha = ImClamp((expand_t - 0.5f) / 0.5f, 0.f, 1.f);

            if (icon_alpha > 0.01f) {
                const float icon_size = 14.f;
                ImVec2 icon_sz = f::font::icon_font->CalcTextSizeA(icon_size, FLT_MAX, 0.f, compact_labels[i]);
                ImVec2 icon_pos = { tc.x - icon_sz.x * 0.5f, tc.y - icon_sz.y * 0.5f };
                dl->AddText(f::font::icon_font, icon_size, icon_pos,
                    with_alpha(txt_col, icon_alpha), compact_labels[i]);
            }
            if (text_alpha > 0.01f) {
                ImVec2 text_sz = ImGui::CalcTextSize(full_labels[i]);
                ImVec2 text_pos = { tc.x - text_sz.x * 0.5f, tc.y - text_sz.y * 0.5f };
                dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(), text_pos,
                    with_alpha(txt_col, text_alpha), full_labels[i]);
            }
        }

        ImGui::SetCursorScreenPos({ origin.x, origin.y + height });
    }

    struct ActionBtnState
    {
        float expand_t = 0.f;
        float click_t = 0.f;
        float active_t = 0.f;
    };

    bool action_button(
        ImVec2  center,
        int     tab_index,
        float   base_height = 25.f,
        float   alpha = 1.f,
        float   rounding = 6.f
    )
    {
        const TabTheme& th = tab_theme(tab_index);

        const float H = base_height;
        const float R = rounding;

        // Measure content first
        ImFont* icon_fnt = f::font::icon_font ? f::font::icon_font : ImGui::GetFont();
        const float icon_size = 13.f;
        const char* icon_str = tab_icon(tab_index);
        const char* label_str = tab_label(tab_index);

        ImVec2 icon_sz = icon_fnt->CalcTextSizeA(icon_size, FLT_MAX, 0.f, icon_str);
        ImVec2 label_sz = ImGui::CalcTextSize(label_str);

        // padding: left_pad | icon | gap | label | right_pad
        const float left_pad = 12.f;
        const float right_pad = 12.f;
        const float icon_gap = 7.f;   // space between icon and text

        const float W = left_pad + icon_sz.x + icon_gap + label_sz.x + right_pad;

        static std::unordered_map<ImGuiID, ActionBtnState> s_states;
        ImGuiID         wid = ImGui::GetID(("##actbtn" + std::to_string(tab_index)).c_str());
        ActionBtnState& st = s_states[wid];

        const float dt = ImGui::GetIO().DeltaTime;

        float  nudge = st.click_t * 2.f;
        ImVec2 btn_min = { center.x - W * 0.5f + nudge, center.y - H * 0.5f + nudge };
        ImVec2 btn_max = { center.x + W * 0.5f + nudge, center.y + H * 0.5f + nudge };

        ImGui::SetCursorScreenPos({ center.x - W * 0.5f, center.y - H * 0.5f });
        ImGui::InvisibleButton(("##actbtn_hit" + std::to_string(tab_index)).c_str(), { W, H });

        ImVec2 mouse = ImGui::GetMousePos();
        bool   hovered = (mouse.x >= btn_min.x && mouse.x <= btn_max.x &&
            mouse.y >= btn_min.y && mouse.y <= btn_max.y);
        bool   clicked = hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        st.active_t += ((hovered ? 1.f : 0.f) - st.active_t) * 0.20f;
        if (clicked) st.click_t = 1.f;
        st.click_t -= st.click_t * dt * 15.f;
        if (st.click_t < 0.f) st.click_t = 0.f;

        ImDrawList* dl = ImGui::GetWindowDrawList();

        auto with_alpha_f = [&](ImU32 col, float a) -> ImU32 {
            int base = (col >> IM_COL32_A_SHIFT) & 0xFF;
            return (col & ~(0xFF << IM_COL32_A_SHIFT)) | ((int)(base * a) << IM_COL32_A_SHIFT);
            };

        // Background
        dl->AddRectFilled(btn_min, btn_max, with_alpha_f(IM_COL32(24, 24, 24, 255), alpha), R);
        dl->AddRect(btn_min, btn_max, with_alpha_f(IM_COL32(62, 62, 65, 200), alpha), R, 0, 1.f);

        // Hover accent
        if (st.active_t > 0.005f)
        {
            dl->AddRectFilled(btn_min, btn_max,
                with_alpha_f(IM_COL32(
                    (th.active_fill >> IM_COL32_R_SHIFT) & 0xFF,
                    (th.active_fill >> IM_COL32_G_SHIFT) & 0xFF,
                    (th.active_fill >> IM_COL32_B_SHIFT) & 0xFF, 18), alpha), R);
            dl->AddRect(btn_min, btn_max,
                with_alpha_f(IM_COL32(
                    (th.active_border >> IM_COL32_R_SHIFT) & 0xFF,
                    (th.active_border >> IM_COL32_G_SHIFT) & 0xFF,
                    (th.active_border >> IM_COL32_B_SHIFT) & 0xFF, 18), alpha), R, 0, 1.f);
        }

        auto lerp_col_rgb = [](ImU32 a, ImU32 b, float t) -> ImU32 {
            ImVec4 ca = ImGui::ColorConvertU32ToFloat4(a);
            ImVec4 cb = ImGui::ColorConvertU32ToFloat4(b);
            return ImGui::ColorConvertFloat4ToU32({
                ca.x + (cb.x - ca.x) * t,
                ca.y + (cb.y - ca.y) * t,
                ca.z + (cb.z - ca.z) * t,
                1.f   // ignore source alphas entirely
                });
            };

        ImU32 icon_col = with_alpha_f(lerp_col_rgb(IM_COL32(200, 200, 200, 255), th.active_text, st.active_t), alpha);

        //  Draw icon + label left-to-right, centered as a unit
        float content_w = icon_sz.x + icon_gap + label_sz.x;
        float content_x = center.x + nudge - content_w * 0.5f;
        float content_cy = center.y + nudge;

        // Icon
        dl->AddText(icon_fnt, icon_size,
            { content_x, content_cy - icon_sz.y * 0.5f },
            icon_col, icon_str);

        // Label
        dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
            { content_x + icon_sz.x + icon_gap, content_cy - label_sz.y * 0.5f },
            icon_col, label_str);

        ImGui::SetCursorScreenPos({ center.x - W * 0.5f, center.y + H * 0.5f });
        ImGui::Dummy({ W, 0.f });

        return clicked;
    }

} // namespace custom