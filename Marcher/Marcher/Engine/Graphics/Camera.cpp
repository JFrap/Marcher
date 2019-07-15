#include "Camera.h"

namespace marcher {
	Camera::Camera(glm::vec3 position, glm::vec3 target) 
		: Position(position), Target(target) {
		FOV = 80.f;
	}

	void Camera::Update(std::shared_ptr<Shader> shader, float aspectRatio, glm::vec4 viewport) {
		View = glm::lookAt(Position, Target, glm::vec3(0, 1, 0));
		Projection = glm::perspective(glm::radians(FOV), aspectRatio, 0.1f, 100.f);
		VP = View * Projection;

		glm::vec3 TopLeft = glm::unProject(glm::vec3(0, 0, 0), View, Projection, viewport);
		glm::vec3 TopRight = glm::unProject(glm::vec3(viewport.z, 0, 0), View, Projection, viewport);
		glm::vec3 BottomLeft = glm::unProject(glm::vec3(0, viewport.w, 0), View, Projection, viewport);
		glm::vec3 BottomRight = glm::unProject(glm::vec3(viewport.z, viewport.w, 0), View, Projection, viewport);

		shader->SendUniform("MainCamera.Position", Position);
		shader->SendUniform("MainCamera.Target", Target);
		shader->SendUniform("MainCamera.TopLeft", TopLeft);
		shader->SendUniform("MainCamera.TopRight", TopRight);
		shader->SendUniform("MainCamera.BottomLeft", BottomLeft);
		shader->SendUniform("MainCamera.BottomRight", BottomRight);

		/*printf("TL: %f %f %f \n", TopLeft.x, TopLeft.y, TopLeft.z);
		printf("TR: %f %f %f \n", TopRight.x, TopRight.y, TopRight.z);
		printf("BL: %f %f %f \n", BottomLeft.x, BottomLeft.y, BottomLeft.z);
		printf("BR: %f %f %f \n", BottomRight.x, BottomRight.y, BottomRight.z);*/
	}

	Camera::~Camera() {

	}
}