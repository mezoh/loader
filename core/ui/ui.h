#pragma once
#include <imgui.h>
#include <string>

enum class UIState {
    Loading,
    TransitionToForm,
    Form
};

extern UIState current_state;
extern float transition_alpha;
extern std::string error_message;
extern bool auth_complete;
extern bool auth_success;

namespace ui
{
	void render();
	void forms_page();
    void loading_page();
	void main_page();
}
