#pragma once

#include <glad/glad.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "../Maths.h"
#include "Color.h"

namespace marcher {
	class VolumetricModel {
	public:
		VolumetricModel(std::string path);

		void LoadModel(std::string path);
		void Bind(int unit = -1);

		~VolumetricModel();

	private:

		GLuint m_handle;
	};
}