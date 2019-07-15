#pragma once

#include "../Maths.h"

namespace marcher {

	class Color {
	public:
		Color();
		Color(float r, float g, float b, float a = 1.f);

		float r;
		float g;
		float b;
		float a;

		float operator[](const size_t index);
	};

	bool operator==(const Color& left, const Color& right);
	bool operator!=(const Color& left, const Color& right);
}