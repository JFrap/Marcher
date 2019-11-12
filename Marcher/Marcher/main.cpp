#include <SFML/Graphics.hpp>
#include <glad/glad.h>

#include "Engine/Graphics/Shader.h"
#include "Engine/Graphics/Camera.h"
#include "Engine/Timer.h"
#include "Engine/Graphics/VolumetricModel.h"

int main() {
	sf::ContextSettings settings;
	settings.depthBits = 24;
	settings.stencilBits = 8;
	settings.majorVersion = 4;
	settings.minorVersion = 3;

	sf::Window window(sf::VideoMode(1280, 720), "Marcher", sf::Style::Default);
	window.setVerticalSyncEnabled(true);

	int gladInitRes = gladLoadGL();
	if (!gladInitRes) {
		fprintf(stderr, "Unable to initialize glad\n");
		window.close();
		return -1;
	}

	glEnable(GL_TEXTURE_3D);

	printf("OpenGL version supported by this platform (%s): \n", glGetString(GL_VERSION));

	std::shared_ptr<marcher::Shader> mainShader = std::unique_ptr<marcher::Shader>(new marcher::Shader("main.vs", "main.fs"));

	float vertices[] = {
		-1.f, -1.f, 0.f,
		-1.f,  1.f, 0.f,
		 1.f,  1.f, 0.f,
		 1.f,  1.f, 0.f,
		 1.f, -1.f, 0.f,
		-1.f, -1.f, 0.f
	};

	unsigned int VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	marcher::VolumetricModel model("bunny.vol");

	glm::vec3 cameraRot = glm::vec3();
	marcher::Camera camera = marcher::Camera(glm::vec3(0, 0, 1), glm::vec3(0, 0, 0));

	marcher::Timer frameTimer, timer;
	unsigned int TotalFrames = 0;
	float totalTime = 0;

	bool active = true;
	while (window.isOpen()) {
		float dt = timer.Restart<float>();
		totalTime += dt;
		
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				window.close();
			else if (event.type == sf::Event::Resized) {
				glViewport(0, 0, window.getSize().x, window.getSize().y);
			}
			else if (event.type == sf::Event::KeyPressed) {
				if (event.key.code == sf::Keyboard::F1) {
					printf("Reloading Shader...\n");
					mainShader = std::unique_ptr<marcher::Shader>(new marcher::Shader("main.vs", "main.fs"));
				}
			}
			else if (event.type == sf::Event::LostFocus) {
				active = false;
			}
			else if (event.type == sf::Event::GainedFocus) {
				active = true;
			}
		}

		TotalFrames++;
		float currentTime = frameTimer.CurrentTime<float>();
		if (currentTime >= 1.f) {
			printf("%f FPS | %f MS \n", (float)TotalFrames / currentTime, (currentTime / (float)TotalFrames) * 1000.f);
			frameTimer.Restart();
			TotalFrames = 0;
		}

		sf::Vector2i windowCenter = sf::Vector2i(window.getSize().x / 2, window.getSize().y / 2);
		if (sf::Mouse::getPosition(window) != windowCenter && active) {
			cameraRot.y += ((float)sf::Mouse::getPosition(window).x - (float)windowCenter.x) / 1000;
			cameraRot.x += ((float)sf::Mouse::getPosition(window).y - (float)windowCenter.y) / 1000;
			sf::Mouse::setPosition(windowCenter, window);
		}

		auto camDir = glm::vec3(glm::vec4(0, 0, -1, 1) * glm::rotate(cameraRot.x, glm::vec3(1, 0, 0)) * glm::rotate(cameraRot.y, glm::vec3(0, 1, 0)));
		auto camRight = glm::vec3(glm::vec4(1, 0, 0, 1) * glm::rotate(cameraRot.y, glm::vec3(0, 1, 0)));

		float speed = 5.f;

		if (active) {
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
				speed = 20;
			}

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
				camera.Position += camDir * dt * speed;
			}

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
				camera.Position -= camDir * dt * speed;
			}

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
				camera.Position += camRight * dt * speed;
			}

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
				camera.Position -= camRight * dt * speed;
			}

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
				camera.Position += glm::vec3(0, 1, 0) * dt * speed;
			}

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
				camera.Position -= glm::vec3(0, 1, 0) * dt * speed;
			}
		}

		camera.Target = camera.Position + camDir;

		glClearColor(0.5f, 0.5f, 0.5f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		camera.Update(mainShader, (float)window.getSize().x / (float)window.getSize().y, glm::vec4(0, 0, window.getSize().x, window.getSize().y));
		mainShader->SendUniform("ScreenSize", glm::vec2(window.getSize().x, window.getSize().y));
		mainShader->SendUniform("Time", totalTime);

		mainShader->Bind();

		model.Bind(0);
		mainShader->SendUniform("Model", 0);

		glBindVertexArray(VAO); 
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindTexture(GL_TEXTURE_3D, 0);

		window.display();
	}
	
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	return 0;
}