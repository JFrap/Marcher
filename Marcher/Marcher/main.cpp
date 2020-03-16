#include <SFML/Graphics.hpp>
#include <glad/glad.h>

#include "Engine/Graphics/Shader.h"
#include "Engine/Graphics/Camera.h"
#include "Engine/Timer.h"
#include "Engine/Graphics/VolumetricModel.h"


#include "Engine/ImGUI/imgui.h"
#include "Engine/ImGUI/imgui-SFML.h"

// F E A R L E S S C O N C U R R E N C Y
float MouseSensitivity = 1.f;
float MovementSpeed = 5.f;
float FieldOfView = 90.f;

float Epsilon = 0.005f;
bool ShadowsEnabled = true;
float ShadowStrength = 1.f;
glm::vec3 AmbientColor = glm::vec3(0.05f);

bool AAEnabled = true;

bool running = true;
bool VSync = true;
float MainFPS = 0.f;
float MainMS = 0.f;

void UtiltiyWindow(sf::Window *mainWindow) {
	sf::RenderWindow window(sf::VideoMode(200, 720), "Settings");
	window.setPosition(mainWindow->getPosition() + sf::Vector2i(mainWindow->getSize().x, 0));
	window.setVerticalSyncEnabled(true);

	ImGui::SFML::Init(window);

	bool attached = true;

	sf::Clock clock;
	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			ImGui::SFML::ProcessEvent(event);
			if (!running) {
				window.close();
			}
		}
		if (attached && mainWindow) {
			window.setPosition(mainWindow->getPosition() + sf::Vector2i(mainWindow->getSize().x, 0));
		}

		sf::Time delta = clock.restart();
		ImGui::SFML::Update(window, delta);

		ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
		ImGui::SetWindowPos(ImVec2(0, 0));
		ImGui::SetWindowSize(ImVec2(window.getSize().x, window.getSize().y));

		ImGui::Text("Performance");
		ImGui::Text(("FPS: " + std::to_string(MainFPS)).c_str());
		ImGui::Text(("MS: " + std::to_string(MainMS)).c_str());
		ImGui::Checkbox("VSync", &VSync);

		if (ImGui::CollapsingHeader("Meta")) {ImGui::PushItemWidth(-100);
			ImGui::Checkbox("Attach To Main Window", &attached);
		}
		ImGui::PushItemWidth(-100);
		if (ImGui::CollapsingHeader("Camera")) {
			ImGui::SliderFloat("Sensitivity", &MouseSensitivity, 0.1f, 5.f, "%.1f");
			ImGui::Spacing();
			ImGui::SliderFloat("Speed", &MovementSpeed, 1.f, 20.f, "%.1f");
			ImGui::Spacing();
			ImGui::SliderFloat("FOV", &FieldOfView, 1.f, 120.f, "%.1f");
		}
		ImGui::PushItemWidth(-75);
		if (ImGui::CollapsingHeader("Rendering")) {
			ImGui::InputFloat("Epsilon", &Epsilon, 0.0005f, -0.0005f, "%.5f");
			if (ImGui::CollapsingHeader("Shadows")) {
				ImGui::Checkbox("Enable Shadows", &ShadowsEnabled);
				ImGui::Spacing();
				ImGui::PushItemWidth(-110);
				ImGui::SliderFloat("Shadow Strength", &ShadowStrength, 0.f, 1.f, "%.2f");
				ImGui::Spacing();
				ImGui::ColorPicker3("Ambient Light", &AmbientColor[0]);
			}
		}

		ImGui::PopItemWidth();
		ImGui::End();



		window.clear(sf::Color(128, 128, 128));
		ImGui::SFML::Render(window);
		window.display();
	}

	ImGui::SFML::Shutdown();
}

int main() {
	sf::ContextSettings settings;
	settings.depthBits = 24;
	settings.stencilBits = 8;
	settings.majorVersion = 4;
	settings.minorVersion = 3;

	sf::Window window(sf::VideoMode(1280, 720), "Marcher", sf::Style::Default);
	//window.setFramerateLimit(120);

	sf::Thread UtilityThread(UtiltiyWindow, &window);
	UtilityThread.launch();

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

	//marcher::VolumetricModel model("bunny.vol");

	glm::vec3 cameraRot = glm::vec3();
	marcher::Camera camera = marcher::Camera(glm::vec3(0, 0, 1), glm::vec3(0, 0, 0));

	marcher::Timer frameTimer, timer;
	unsigned int TotalFrames = 0;
	float totalTime = 0;

	bool active = true;
	while (window.isOpen()) {
		window.setVerticalSyncEnabled(VSync);
		float dt = timer.Restart<float>();
		totalTime += dt;
		
		
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) {
				window.close();
				running = false;
			}
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
				window.setMouseCursorVisible(true);
				active = false;
			}
			else if (event.type == sf::Event::GainedFocus) {
				active = true;
				window.setMouseCursorVisible(false);
			}
		}

		TotalFrames++;
		float currentTime = frameTimer.CurrentTime<float>();
		if (currentTime >= 0.5f) {
			printf("%f FPS | %f MS \n", (float)TotalFrames / currentTime, (currentTime / (float)TotalFrames) * 1000.f);
			MainFPS = (float)TotalFrames / currentTime;
			MainMS = (currentTime / (float)TotalFrames) * 1000.f;
			frameTimer.Restart();
			TotalFrames = 0;
		}

		sf::Vector2i windowCenter = sf::Vector2i(window.getSize().x / 2, window.getSize().y / 2);
		if (sf::Mouse::getPosition(window) != windowCenter && active) {
			cameraRot.y += (((float)sf::Mouse::getPosition(window).x - (float)windowCenter.x) / 1000) * MouseSensitivity;
			cameraRot.x += (((float)sf::Mouse::getPosition(window).y - (float)windowCenter.y) / 1000) * MouseSensitivity;
			sf::Mouse::setPosition(windowCenter, window);
		}

		auto camDir = glm::vec3(glm::vec4(0, 0, -1, 1) * glm::rotate(cameraRot.x, glm::vec3(1, 0, 0)) * glm::rotate(cameraRot.y, glm::vec3(0, 1, 0)));
		auto camRight = glm::vec3(glm::vec4(1, 0, 0, 1) * glm::rotate(cameraRot.y, glm::vec3(0, 1, 0)));

		float speed = MovementSpeed;

		if (active) {
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
				speed *= 2;
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

		camera.FOV = FieldOfView;
		camera.Update(mainShader, (float)window.getSize().x / (float)window.getSize().y, glm::vec4(0, 0, window.getSize().x, window.getSize().y));
		mainShader->SendUniform("ScreenSize", glm::vec2(window.getSize().x, window.getSize().y));
		mainShader->SendUniform("Time", totalTime);

		if (Epsilon <= 0) {
			Epsilon = 0.0005f;
		}
		mainShader->SendUniform("EPSILON", Epsilon);
		mainShader->SendUniform("ShadowsEnabled", (int)ShadowsEnabled);
		mainShader->SendUniform("ShadowStrength", ShadowStrength);
		mainShader->SendUniform("AmbientColor", AmbientColor);

		mainShader->Bind();

		/*model.Bind(0);
		mainShader->SendUniform("Model", 0);*/

		glBindVertexArray(VAO); 
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindTexture(GL_TEXTURE_3D, 0);

		window.display();
	}
	
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	UtilityThread.wait();

	return 0;
}