#include <SFML/Graphics.hpp>
#include <glad/glad.h>

#include "Engine/Graphics/Shader.h"
#include "Engine/Graphics/Camera.h"
#include "Engine/Timer.h"

int main() {
	sf::ContextSettings settings;
	settings.depthBits = 24;
	settings.stencilBits = 8;
	settings.majorVersion = 3;
	settings.minorVersion = 3;

	sf::Window window(sf::VideoMode(1280, 720), "Marcher", sf::Style::Default);

	int gladInitRes = gladLoadGL();
	if (!gladInitRes) {
		fprintf(stderr, "Unable to initialize glad\n");
		window.close();
		return -1;
	}

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

	marcher::Camera camera = marcher::Camera(glm::vec3(0, 4, 6), glm::vec3(0, 0, 0));

	marcher::Timer frameTimer, timer;
	unsigned int TotalFrames = 0;

	while (window.isOpen()) {
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
		}

		TotalFrames++;
		float currentTime = frameTimer.CurrentTime<float>();
		if (currentTime >= 1.f) {
			printf("%f FPS | %f MS \n", (float)TotalFrames / currentTime, (currentTime / (float)TotalFrames) * 1000);
			frameTimer.Restart();
			TotalFrames = 0;
		}

		camera.Position.x = 5 * sin(timer.CurrentTime<float>() * 0.5f);
		camera.Position.z = 5 * cos(timer.CurrentTime<float>() * 0.5f);

		glClearColor(0.5f, 0.5f, 0.5f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		camera.Update(mainShader, (float)window.getSize().x / (float)window.getSize().y, glm::vec4(0, 0, window.getSize().x, window.getSize().y));
		mainShader->SendUniform("ScreenSize", glm::vec2(window.getSize().x, window.getSize().y));

		mainShader->Bind();

		glBindVertexArray(VAO); 
		glDrawArrays(GL_TRIANGLES, 0, 6);

		window.display();
	}
	
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	return 0;
}