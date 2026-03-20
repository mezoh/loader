#include "ui.h"
#include <core/crypt/skCrypter.h>
#include <dependencies/imgui/imgui_settings.hpp>
#include <core/protection/anti_patch.h>
#include <dependencies/imgui/imgui_custom.hpp>
#include <core/auth/auth.h>
using namespace ImGui;

UIState current_state = UIState::Loading;
float transition_alpha = 0.0f;
std::string error_message = "";
bool auth_complete = false;
bool auth_success = false;

void ui::render() {
    float dt = GetIO().DeltaTime;

    // Update state machine
    switch (current_state) {
    case UIState::Loading:
        
        if (auth_complete) {
            if (auth_success) {
                current_state = UIState::TransitionToForm;
            }
        }
        break;

    case UIState::TransitionToForm:
        transition_alpha += dt * 2.f;
        if (transition_alpha >= 1.0f) {
            transition_alpha = 1.0f;
            current_state = UIState::Form;
        }
        break;

    case UIState::Form:
        
        break;
    }

    if (current_state == UIState::Form) {
        ui::forms_page();
    }
    else {
        ui::loading_page();
    }
}


void ui::forms_page()
{

    SetNextWindowSize(f::bg::login_size, ImGuiCond_Once);
    Begin(_("##menu"), NULL,
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

    ImDrawList* draw = GetWindowDrawList();
    ImVec2      tl = GetWindowPos();
    ImVec2      br = { tl.x + f::bg::login_size.x, tl.y + f::bg::login_size.y };

    // layered border
    draw->AddRectFilled(tl, br, f::bg::outline, f::bg::rounding);  
    tl += 2.f; br -= 2.f;
    draw->AddRectFilled(tl, br, f::bg::stroke, f::bg::rounding);  
    tl += 2.f; br -= 2.f;
    draw->AddRectFilled(tl, br, f::bg::outline, f::bg::rounding);  
    tl += 1.f; br -= 1.f;
    draw->AddRectFilledMultiColor(tl, br,
        IM_COL32(30, 30, 30, 255), IM_COL32(24, 24, 24, 255),
        IM_COL32(30, 30, 30, 255), IM_COL32(24, 24, 24, 255));

    //title
    {
        const float  TITLE_PAD = 10.f;
        ImFont* font = GetFont();
        float        font_size = GetFontSize();
        const char* distortion_text = "distortion";
        const char* vip_text = ".vip";
        float        part1_w = CalcTextSize(distortion_text).x;
        ImVec2       title_pos = { tl.x + TITLE_PAD, tl.y + (20.f - font_size) * 0.5f };

        draw->AddText(font, font_size, title_pos,
            IM_COL32(255, 255, 255, 220), distortion_text);
        draw->AddText(font, font_size, { title_pos.x + part1_w, title_pos.y },
            IM_COL32(255, 140, 30, 220), vip_text);
    }

    // child panel — top pushed down by 20px for header
    const float HEADER_H = 11.f;
    ImVec2 child_tl = { tl.x + 10.f, tl.y + 10.f + HEADER_H };
    ImVec2 child_br = { br.x - 10.f, br.y - 10.f };
    custom::draw_rect_filled_with_outline(child_tl, child_br, f::child::outline, f::child::fill, f::bg::rounding);
    tl = child_tl; br = child_br;

    // layout
    const float PAD = 16.f, INPUT_H = 25.f, SPACING = 38.f;
    const float BTN_GAP = 8.f, BTN_H = 25.f, BAR_H = 28.f;

    const float left = tl.x + PAD;
    const float right = br.x - PAD;
    const float input_w = right - left;
    const float bar_y = br.y - BAR_H - 10.f;
    const float zone_h = bar_y - (tl.y + PAD);

    // state
    static char             username[16]{}, password[16]{}, license[16]{};
    static int              active_tab = 0;
    static custom::FormAnim anim;

    struct Input { const char* label, * id; char* buf; ImGuiInputTextFlags flags; };
    Input login_inputs[] = { {"Username","##u",username,0}, {"Password","##p",password,ImGuiInputTextFlags_Password} };
    Input register_inputs[] = { {"Username","##u",username,0}, {"Password","##p",password,ImGuiInputTextFlags_Password}, {"License","##l",license,0} };
    Input extend_inputs[] = { {"Username","##u",username,0}, {"License","##l",license,0} };

    Input* inputs; int count;
    switch (active_tab) {
    case 1:  inputs = register_inputs; count = 3; break;
    case 2:  inputs = extend_inputs;   count = 2; break;
    default: inputs = login_inputs;    count = 2; break;
    }

    const float dt = GetIO().DeltaTime;
    anim.tick(active_tab, dt);

    const float block_h = (count - 1) * SPACING + INPUT_H + BTN_GAP + BTN_H;
    const float first_y = tl.y + PAD + (zone_h - block_h) * 0.5f;

    auto fade = [](ImU32 col, float a) -> ImU32 {
        ImVec4 c = ImGui::ColorConvertU32ToFloat4(col); c.w *= a;
        return ImGui::ColorConvertFloat4ToU32(c);
        };

    // inputs
    const float INPUT_W_SCALE = 0.85f;
    for (int i = 0; i < count; ++i)
    {
        float a = anim.alpha(i), s = anim.scale(i);
        if (a <= 0.001f) continue;

        float  sw = (input_w * INPUT_W_SCALE) * s, sh = INPUT_H * s;
        ImVec2 pos = { left + (input_w - sw) * 0.5f,
                       first_y + SPACING * i + (INPUT_H - sh) * 0.5f };

        draw->PushClipRect({ left - 2.f, first_y + SPACING * i - 2.f },
            { right + 2.f, first_y + SPACING * i + INPUT_H + 2.f }, true);
        SetCursorScreenPos(pos);
        custom::input_3d(inputs[i].label, inputs[i].id, inputs[i].buf, 16,
            sw, sh, fade(IM_COL32(62, 62, 62, 255), a), fade(f::bg::fill, a), 0.f, inputs[i].flags);
        draw->PopClipRect();
        Dummy({ input_w, INPUT_H });
    }

    // button
    {
        float a = anim.alpha(3), s = anim.scale(3);
        if (a > 0.001f)
        {
            float  btn_top = first_y + (count - 1) * SPACING + INPUT_H + BTN_GAP;
            ImVec2 center = { left + input_w * 0.5f, btn_top + BTN_H * 0.5f };

            draw->PushClipRect({ left - 2.f, btn_top - 2.f }, { right + 2.f, btn_top + BTN_H + 2.f }, true);
            if (custom::action_button(center, active_tab, BTN_H * s, a, 6.f))
            {
                // Dispatch action based on active tab
                bool success = false;

                switch (active_tab) {
                case 0: // Login
                    if (strlen(username) > 0 && strlen(password) > 0) {
                        success = auth::login(std::string(username), std::string(password));
                        if (success) {
                            // TODO: transition to main_page or set a flag
                            current_state = UIState::Form; // or switch to main menu
                        }
                    }
                    break;

                case 1: // Register
                    if (strlen(username) > 0 && strlen(password) > 0 && strlen(license) > 0) {
                        success = auth::registr(std::string(username), std::string(password), std::string(license));
                        if (success) {
                            // Auto-login after registration or show success message
                            // Maybe clear the license field and switch to login tab
                            memset(license, 0, sizeof(license));
                            active_tab = 0;
                        }
                    }
                    break;

                case 2: // Extend
                    if (strlen(username) > 0 && strlen(license) > 0) {
                        success = auth::extend(std::string(username), std::string(license));
                        if (success) {
                            // Clear fields and switch to login
                            memset(license, 0, sizeof(license));
                            active_tab = 0;
                        }
                    }
                    break;
                }
            }
            draw->PopClipRect();
        }
        SetCursorScreenPos({ left, first_y + (count - 1) * SPACING + INPUT_H + BTN_GAP });
        Dummy({ input_w, BTN_H });
    }

    // tab bar
    {
        const float compact_w = 72.f;
        float sw_e = ImClamp(anim.expand_t, 0.f, 1.f);
        sw_e = sw_e * sw_e * (3.f - 2.f * sw_e);
        float  bar_w = compact_w + (input_w - compact_w) * sw_e;
        ImVec2 origin = { left + (input_w - bar_w) * 0.5f, bar_y };

        anim.tick_expand(IsMouseHoveringRect(origin, { origin.x + bar_w, origin.y + BAR_H }), dt);

        SetCursorScreenPos(origin);
        custom::tab_bar(origin, bar_w, BAR_H, &active_tab, anim.expand_t);
        Dummy({ input_w, BAR_H });
    }

    End();
}

void ui::loading_page() {
    SetNextWindowSize(f::bg::login_size, ImGuiCond_Once);
    Begin(_("##menu"), NULL,
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

    ImDrawList* draw = GetWindowDrawList();
    ImVec2 tl = GetWindowPos();
    ImVec2 br = { tl.x + f::bg::login_size.x, tl.y + f::bg::login_size.y };

    // Same background as forms_page
    draw->AddRectFilled(tl, br, f::bg::outline, f::bg::rounding); tl += 2.f; br -= 2.f;
    draw->AddRectFilled(tl, br, f::bg::stroke, f::bg::rounding); tl += 2.f; br -= 2.f;
    draw->AddRectFilled(tl, br, f::bg::outline, f::bg::rounding); tl += 1.f; br -= 1.f;
    draw->AddRectFilledMultiColor(tl, br,
        IM_COL32(30, 30, 30, 255), IM_COL32(24, 24, 24, 255),
        IM_COL32(30, 30, 30, 255), IM_COL32(24, 24, 24, 255));

    // Title
    {
        const float TITLE_PAD = 10.f;
        ImFont* font = GetFont();
        float font_size = GetFontSize();
        const char* part1 = "distortion";
        const char* part2 = ".vip";
        float part1_w = CalcTextSize(part1).x;
        ImVec2 title_pos = { tl.x + TITLE_PAD, tl.y + (20.f - font_size) * 0.5f };

        draw->AddText(font, font_size, title_pos,
            IM_COL32(255, 255, 255, 220), part1);
        draw->AddText(font, font_size, { title_pos.x + part1_w, title_pos.y },
            IM_COL32(255, 140, 30, 220), part2);
    }

    ImVec2 center = { (tl.x + br.x) * 0.5f, (tl.y + br.y) * 0.5f };

    // Determine alpha based on state
    float loading_alpha = 1.0f;
    float form_alpha = 0.0f;

    if (current_state == UIState::TransitionToForm) {
        loading_alpha = 1.0f - transition_alpha;
        form_alpha = transition_alpha;
    }

    // Draw loading animation
    if (loading_alpha > 0.01f) {
        static float rotation = 0.0f;
        rotation += GetIO().DeltaTime * 3.0f;

        const float radius = 25.f;
        const float thickness = 3.5f;

        ImU32 color = IM_COL32(255, 140, 30, (int)(220 * loading_alpha));

        // Spinning arc
        draw->PathArcTo(center, radius, rotation, rotation + IM_PI * 1.5f, 32);
        draw->PathStroke(color, 0, thickness);

        // Loading text
        const char* text = auth_complete ? "Authenticating..." : "Initializing...";
        ImVec2 text_size = CalcTextSize(text);
        draw->AddText({ center.x - text_size.x * 0.5f, center.y + radius + 20.f },
            IM_COL32(255, 255, 255, (int)(180 * loading_alpha)), text);
    }

    // Draw form preview fading in during transition
    if (form_alpha > 0.01f) {
        const float HEADER_H = 11.f;
        ImVec2 child_tl = { tl.x + 10.f, tl.y + 10.f + HEADER_H };
        ImVec2 child_br = { br.x - 10.f, br.y - 10.f };

        ImU32 outline_col = IM_COL32(62, 62, 62, (int)(255 * form_alpha));
        ImU32 fill_col = IM_COL32(20, 20, 20, (int)(255 * form_alpha));

        custom::draw_rect_filled_with_outline(child_tl, child_br, outline_col, fill_col, f::bg::rounding);
    }

    End();
}

void ui::main_page() {

}