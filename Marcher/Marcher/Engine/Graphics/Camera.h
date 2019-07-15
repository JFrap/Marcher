#pragma once

#include <iostream>

#include "../Maths.h"
#include "Shader.h"

namespace marcher {
	class Camera {
	public:
		Camera(glm::vec3 position = glm::vec3(0, 0, 5), glm::vec3 target = glm::vec3());
		~Camera();

		void Update(std::shared_ptr<Shader> shader, float aspectRatio, glm::vec4 viewport);

		float FOV;

		glm::mat4 View, Projection, VP;
		glm::vec3 Position, Target;
	};
}