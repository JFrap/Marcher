#include "Color.h"

namespace marcher {

	Color::Color() : r(1.f), g(1.f), b(1.f), a(1.f) { }

	Color::Color(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) { }

	bool operator==(const Color& left, const Color& right) {
		return (left.r == right.r) && (left.g == right.g) && (left.b == right.b) && (left.a == right.a);
	}

	bool operator!=(const Color& left, const Color& right) {
		return !(left == right);
	}

	float Color::operator[](const size_t index) {
		switch (index) {
		case 0:
			return r;
		case 1:
			return g;
		case 2:
			return b;
		case 3:
			return a;
		default:
			return 0.0f;
		}

		return 0.0f;
	}
}