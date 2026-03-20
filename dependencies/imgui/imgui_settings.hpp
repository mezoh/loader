#pragma once

namespace f
{

	namespace font
	{
		inline ImFont* segoe_black = nullptr;
		inline ImFont* segoe_black_italic = nullptr;
		inline ImFont* segoe_bold = nullptr;
		inline ImFont* segoe_bold_italic = nullptr;
		inline ImFont* segoe_italic = nullptr;
		inline ImFont* segoe_light = nullptr;
		inline ImFont* segoe_light_italic = nullptr;
		inline ImFont* segoe_regular = nullptr;
		inline ImFont* segoe_semibold = nullptr;
		inline ImFont* segoe_semibold_italic = nullptr;
		inline ImFont* segoe_semilight = nullptr;
		inline ImFont* segoe_semilight_italic = nullptr;
		inline ImFont* icon_font = nullptr;
	}

	namespace bg
	{
		inline ImU32 fill			= IM_COL32(26, 26, 26, 255);
		inline ImU32 stroke			= IM_COL32(57, 57, 57, 255);
		inline ImU32 outline		= IM_COL32(10, 10, 10, 255);

		inline ImVec2 login_size	= ImVec2(300, 240);
		inline ImVec2 main_size		= ImVec2(550, 550);

		inline float rounding = 0.f;
	}
	
	namespace child
	{
		inline ImU32 fill = IM_COL32(35, 35, 35, 255);
		inline ImU32 outline = IM_COL32(62, 62, 62, 255);
	}

	namespace tab_bar
	{
		inline ImU32 fill = IM_COL32(35, 35, 35, 255);
		inline ImU32 stroke = IM_COL32(57, 57, 57, 255);
		inline float rounding = 0.f;
	}
}