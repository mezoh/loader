#include <imgui.h>
#include <imgui_internal.h>

namespace custom
{
	void draw_rect_filled_with_outline(ImVec2 top_left, ImVec2 bottom_right, ImU32 outline_color, ImU32 fill_color, float rounding = 0.f);
    void draw_rect_3d_hover(
        ImVec2       top_left,
        ImVec2       bottom_right,
        ImU32        outline_color,
        ImU32        fill_color,
        float        rounding = 0.f,
        float        max_tilt_deg = 12.f,
        float        perspective_dist = 800.f,
        float        lerp_speed = 0.05f
    );

    bool input_3d(
        const char* label,
        const char* imgui_id,
        char* buf,
        size_t               buf_size,
        float                width = 0.f,
        float                height = 28.f,
        ImU32                outline_color = IM_COL32(62, 62, 65, 255),
        ImU32                fill_color = IM_COL32(24, 24, 24, 255),
        float                rounding = 4.f,
        ImGuiInputTextFlags  flags = 0
    );
    void tab_bar(
        ImVec2  origin,
        float   width,
        float   height = 28.f,
        int* selected = nullptr,
        float   expand_t = 0.f,          // ADD THIS
        ImU32   pill_col = IM_COL32(24, 24, 24, 180),
        ImU32   pill_border = IM_COL32(62, 62, 65, 200)
    );

    bool action_button(
        ImVec2  center,
        int     tab_index,
        float   base_height = 25.f,
        float   alpha = 1.f,   // spring fade-in multiplier from ui.cpp
        float   rounding = 6.f    // matches tab_bar inner_r
    );

    // ?????????????????????????????????????????????????????????????????????????
// FormAnim  –  spring system for the login/register/extend form
// ?????????????????????????????????????????????????????????????????????????
    struct FormAnim
    {
        struct Spring { float pos = 0.f, vel = 0.f; };

        Spring  springs[4];          // [0..2] inputs, [3] button
        float   trigger_t[4] = {};
        float   tab_switch_t = 1.f;
        float   expand_t = 0.f;
        int     last_tab = -1;

        static constexpr float STIFFNESS[4] = { 280.f, 240.f, 260.f, 240.f };
        static constexpr float DAMPING = 18.f;
        static constexpr float STAGGER = 0.18f;

        void reset()
        {
            for (int i = 0; i < 4; ++i)
                springs[i] = {}, trigger_t[i] = 0.f;
            tab_switch_t = 0.f;
        }

        // Call once per frame before drawing
        void tick(int active_tab, float dt)
        {
            if (last_tab != active_tab) { reset(); last_tab = active_tab; }

            tab_switch_t = ImMin(tab_switch_t + dt * 1.2f, 1.f);

            for (int i = 0; i < 4; ++i)
            {
                if (tab_switch_t > 0.f) trigger_t[i] += dt;
                if (trigger_t[i] < STAGGER * i) continue;

                Spring& s = springs[i];
                float force = (1.f - s.pos) * STIFFNESS[i] - s.vel * DAMPING;
                s.vel += force * dt;
                s.pos = ImClamp(s.pos + s.vel * dt, 0.f, 1.02f);
            }
        }

        // Animate the tab bar expand on hover
        void tick_expand(bool hovered, float dt)
        {
            expand_t += ((hovered ? 1.f : 0.f) - expand_t) * dt * 6.f;
        }

        float alpha(int i) const { return ImClamp(springs[i].pos, 0.f, 1.f); }
        float scale(int i) const { return 0.92f + 0.08f * alpha(i); }
    };

}