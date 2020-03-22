#include <SFML/Graphics.hpp>
#include <glad/glad.h>

#include "Engine/Graphics/Shader.h"
#include "Engine/Graphics/Camera.h"
#include "Engine/Timer.h"
#include "Engine/Graphics/VolumetricModel.h"


#include "Engine/ImGUI/imgui.h"
#include "Engine/ImGUI/imgui-SFML.h"

std::string HeaderFS = "#version 330 core\nout vec4 FragColor;\n\nstruct Camera {\n    vec3 Position, Target;\n    vec3 TopLeft, TopRight, BottomLeft, BottomRight;\n};\n\nstruct Ray {\n    vec3 Origin, Direction;\n};\n\nuniform float EPSILON;\nuniform float MAX_DISTANCE;\nuniform int MAX_MARCHING_STEPS;\n\nuniform bool ShadowsEnabled;\nuniform float ShadowStrength;\nuniform float AOStrength;\n\nuniform vec3 AmbientColor;\nuniform vec3 LightColor;\nuniform vec3 LightDir;\n\nuniform vec2 ScreenSize;\nuniform Camera MainCamera;\nuniform float Time;\n\n//uniform sampler3D Model;\n\nRay CalculateFragRay() {\n    vec2 RelScreenPos = gl_FragCoord.xy / ScreenSize;\n\n    vec3 TopPos = mix(MainCamera.TopLeft, MainCamera.TopRight, RelScreenPos.x);\n    vec3 BottomPos = mix(MainCamera.BottomLeft, MainCamera.BottomRight, RelScreenPos.x);\n    vec3 FinalPos = mix(TopPos, BottomPos, RelScreenPos.y);\n    return Ray(FinalPos, normalize(FinalPos-MainCamera.Position));\n}\n\nfloat SceneSDF(in vec3 p);\n\n// float s = 100000;\n// if (sdBox(p - vec3(1), vec3(1)) <= EPSILON) {\n//     s = texture(Model, p * vec3(1.f/2)).r;\n// }\n// else {\n//     s = sdBox(p - vec3(1), vec3(1));\n// }\n// float f = sphereSDF(SDFRepitition(p, vec3(6,0,6)), vec3(0));\n// if (f <= EPSILON) {\n//     //f = max(f, cnoise(p+vec3(Time)));\n// }\n\n// return min(min(s, f), p.y);\n\n/*float SceneSDFAO(in vec3 p) {\n    // float s = 100000;\n    // if (sdBox(p - vec3(1), vec3(1)) <= EPSILON) {\n    //     s = texture(Model, p * vec3(1.f/2)).r;\n    // }\n    // else if (sdBox(p - vec3(1), vec3(1)) <= 0.5) {\n    //     s = sdBox(p - vec3(1), vec3(1)) + texture(Model, p * vec3(1.f/2)).r;\n    // }\n    // float f = sphereSDF(SDFRepitition(p, vec3(6,0,6)), vec3(0));\n    // if (f <= EPSILON) {\n    //     //f = max(f, cnoise(p+vec3(Time)));\n    // }\n\n    // return min(min(s, f), p.y);\n    return min(sphereSDF(SDFRepitition(p, vec3(6,6,6)), vec3(0)), p.y);\n}*/\n\nvec3 EstimateNormal(in vec3 p) {\n    vec3 small_step = vec3(EPSILON, 0.0, 0.0);\n\n    float gradient_x = SceneSDF(p + small_step.xyy) - SceneSDF(p - small_step.xyy);\n    float gradient_y = SceneSDF(p + small_step.yxy) - SceneSDF(p - small_step.yxy);\n    float gradient_z = SceneSDF(p + small_step.yyx) - SceneSDF(p - small_step.yyx);\n\n    vec3 normal = vec3(gradient_x, gradient_y, gradient_z);\n\n    return normalize(normal);\n}\n\nstruct MarchInfo {\n    bool Hit;\n    float Depth, MinDistance;\n    vec3 Position, Normal;\n    int Steps;\n};\n\nMarchInfo March(in Ray ray) {\n    float depth = 0.f;\n    float dist, minDist = MAX_DISTANCE;\n    int i = 0;\n    for (; i < MAX_MARCHING_STEPS; i++) {\n        dist = SceneSDF(ray.Origin + (ray.Direction * depth));\n        minDist = min(dist, minDist);\n        if (dist < EPSILON) {\n            if (dist < 0) {\n                depth += dist; depth += dist;\n            }\n            return MarchInfo(true, depth, dist, ray.Origin + (ray.Direction * depth), EstimateNormal(ray.Origin + (ray.Direction * depth)), i);\n        }\n        depth += dist;\n        if (depth >= MAX_DISTANCE) {\n            return MarchInfo(false, depth, minDist, ray.Origin + (ray.Direction * depth), vec3(0), i);\n        }\n    }\n    return MarchInfo(false, depth, minDist, ray.Origin + (ray.Direction * depth), vec3(0), i);\n}\n\nfloat Shadow(in Ray ray) {\n    for(float t=EPSILON; t<MAX_DISTANCE;) {\n        float h = SceneSDF(ray.Origin + ray.Direction*t);\n        if(h<EPSILON)\n            return 0;\n        t += h;\n    }\n    return 1;\n}\n\nfloat genAmbientOcclusion(vec3 ro, vec3 rd) {\n    vec4 totao = vec4(0.0);\n    float sca = 1.0;\n\n    for (int aoi = 0; aoi < 5; aoi++) {\n        float hr = 0.01 + 0.02 * float(aoi * aoi);\n        vec3 aopos = ro + rd * hr;\n        float dd = SceneSDF(aopos);\n        float ao = clamp(-(dd - hr), 0.0, 1.0);\n        totao += ao * sca * vec4(1.0, 1.0, 1.0, 1.0);\n        sca *= 0.75;\n    }\n    \n    return 1.0 - clamp(AOStrength * totao.w, 0.0, 1.0);\n}\n\nvec3 Render(in Ray ray) {\n    MarchInfo info = March(ray);\n\n    if (info.Hit) {\n        float shadow = 0.f;\n        if (ShadowsEnabled) {\n            shadow = 1.f-Shadow(Ray(info.Position + info.Normal * EPSILON*2, normalize(-LightDir)));\n        }\n        \n        vec3 ret = LightColor * max(dot(info.Normal, normalize(-LightDir)), 0.f);\n        ret -= vec3(shadow * ShadowStrength) * ret;\n        ret += AmbientColor * (vec3(1)-ret);\n\n        ret *= genAmbientOcclusion(info.Position + info.Normal * EPSILON, info.Normal);\n        return mix(ret * vec3(1.f), AmbientColor, distance(ray.Origin, info.Position)/MAX_DISTANCE);\n    }\n    return AmbientColor;\n}\n\nvoid SetDiffuse(in vec3 col) {\n\n}\n\nvoid main() {\n    Ray CamRay = CalculateFragRay();\n\n    if (SceneSDF(CamRay.Origin) < EPSILON) {\n        FragColor = vec4(0,0,0,1);\n    }\n    else {\n        FragColor = vec4(Render(CamRay), 1.f);\n    }\n}";
std::string VertexShader = "#version 330 core\nlayout (location = 0) in vec3 aPos;\n\nvoid main() {\n    gl_Position = vec4(aPos, 1.0);\n}";
std::string DefaultShader = "float SceneSDF(in vec3 p) {\n    float sphere = distance(p + vec3(sin(p.x*20)*0.01), vec3(0, 1, 0)) - 1;\n    float plane = p.y;\n\n    return min(sphere, plane);\n}";

// F E A R L E S S C O N C U R R E N C Y
// G L O B A L V A R I A B L E S

namespace globals {
	float MouseSensitivity = 1.f;
	float MovementSpeed = 5.f;
	float FieldOfView = 90.f;

	float Epsilon = 0.005f;

	float MarchDistance = 200.f;
	int MarchSteps = 1024;

	bool ShadowsEnabled = true;
	float ShadowStrength = 1.f;
	float AOStrength = 1.f;
	glm::vec3 AmbientColor = glm::vec3(0.05f);
	glm::vec3 LightColor = glm::vec3(1.f);
	glm::vec3 LightDirection = glm::vec3(-1.f);

	bool AAEnabled = true;

	bool running = true;
	bool VSync = true;
	float MainFPS = 0.f;
	float MainMS = 0.f;
}

void UtiltiyWindow(sf::Window *mainWindow) {
	sf::RenderWindow window(sf::VideoMode(200, 720), "Settings");
	window.setPosition(mainWindow->getPosition() + sf::Vector2i(mainWindow->getSize().x, 0));
	window.setVerticalSyncEnabled(true);

	ImGui::SFML::Init(window);
	//globals::MouseSensitivity = ImGui::GetStateStorage()->GetFloat(ImGui::GetID(&globals::MouseSensitivity), 1.f);

	bool attached = true;

	sf::Clock clock;
	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			ImGui::SFML::ProcessEvent(event);
			if (!globals::running) {
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
		ImGui::Text(("FPS: " + std::to_string(globals::MainFPS)).c_str());
		ImGui::Text(("MS: " + std::to_string(globals::MainMS)).c_str());
		ImGui::Checkbox("VSync", &globals::VSync);
		ImGui::Text("Controls:\nF1 - Reload Shader\nF - Toggle Mouselook\nW,A,S,D,LCtrl,\nLShift & Space - Movement");

		if (ImGui::CollapsingHeader("Meta")) {ImGui::PushItemWidth(-100);
			ImGui::Checkbox("Attach To Main Window", &attached);
		}
		ImGui::PushItemWidth(-100);
		if (ImGui::CollapsingHeader("Camera")) {
			ImGui::SliderFloat("Sensitivity", &globals::MouseSensitivity, 0.1f, 5.f, "%.1f");
			ImGui::Spacing();
			ImGui::SliderFloat("Speed", &globals::MovementSpeed, 1.f, 20.f, "%.1f");
			ImGui::Spacing();
			ImGui::SliderFloat("FOV", &globals::FieldOfView, 1.f, 120.f, "%.1f");
		}
		ImGui::PushItemWidth(-75);
		if (ImGui::CollapsingHeader("Rendering")) {
			ImGui::InputFloat("Epsilon", &globals::Epsilon, 0.0005f, -0.0005f, "%.5f");
			ImGui::Spacing();
			ImGui::PushItemWidth(-110);
			ImGui::SliderFloat("Marching Distance", &globals::MarchDistance, 1.f, 2000.f, "%.1f");
			ImGui::Spacing();
			ImGui::SliderInt("Marching Steps", &globals::MarchSteps, 1, 2048);
			ImGui::Spacing();
			if (ImGui::CollapsingHeader("Lighting")) {
				ImGui::Checkbox("Enable Shadows", &globals::ShadowsEnabled);
				ImGui::Spacing();
				ImGui::SliderFloat("AO Strength", &globals::AOStrength, 0.f, 5.f, "%.2f");
				ImGui::Spacing();
				ImGui::SliderFloat("Shadow Strength", &globals::ShadowStrength, 0.f, 1.f, "%.2f");
				ImGui::Spacing();
				ImGui::ColorPicker3("Ambient Light", &globals::AmbientColor[0]);
				ImGui::Spacing();
				ImGui::ColorPicker3("Light Color", &globals::LightColor[0]);
				if (ImGui::CollapsingHeader("Light Direction")) {
					ImGui::SliderFloat("X", &globals::LightDirection.x, -1, 1);
					ImGui::SliderFloat("Y", &globals::LightDirection.y, -1, 1);
					ImGui::SliderFloat("Z", &globals::LightDirection.z, -1, 1);
				}
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

bool isFile(const std::string& name) {
	std::ifstream f(name.c_str());
	return f.good();
}

const std::string fileName = "shader.fs";

std::string LoadShader() {
	if (isFile(fileName)) {
		std::string line;
		std::string content;
		std::ifstream myfile(fileName, std::ifstream::in);

		while (std::getline(myfile, line)) { content.append(line + "\n"); }
		myfile.close();
		return HeaderFS + "\n" + content;
	}
	else {
		std::ofstream mfile(fileName, std::ofstream::out);
		mfile << DefaultShader;
		mfile.close();
		return HeaderFS + "\n" + DefaultShader;
	}
}

int main() {
	sf::ContextSettings settings;
	settings.depthBits = 24;
	settings.stencilBits = 8;
	settings.majorVersion = 4;
	settings.minorVersion = 3;

	sf::Window window(sf::VideoMode(1280, 720), "Marchtool", sf::Style::Default);
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

	std::shared_ptr<marcher::Shader> mainShader = std::unique_ptr<marcher::Shader>(new marcher::Shader());/*std::unique_ptr<marcher::Shader>(new marcher::Shader("main.vs", "main.fs"));*/
	mainShader->AddShaderString(VertexShader, marcher::ShaderType::VERTEX_SHADER);
	mainShader->AddShaderString(LoadShader(), marcher::ShaderType::FRAGMENT_SHADER);
	mainShader->Compile();

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

	bool active = true, mouselook = false;
	while (window.isOpen()) {
		window.setVerticalSyncEnabled(globals::VSync);
		float dt = timer.Restart<float>();
		totalTime += dt;
		
		
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) {
				window.close();
				globals::running = false;
				glDeleteVertexArrays(1, &VAO);
				glDeleteBuffers(1, &VBO);
				return 0;
			}
			else if (event.type == sf::Event::Resized) {
				glViewport(0, 0, window.getSize().x, window.getSize().y);
			}
			else if (event.type == sf::Event::KeyPressed) {
				if (event.key.code == sf::Keyboard::F1) {
					printf("Reloading Shader...\n");
					mainShader = std::unique_ptr<marcher::Shader>(new marcher::Shader("main.vs", "main.fs"));
				}
				else if (event.key.code == sf::Keyboard::F) {
					mouselook = !mouselook;
					window.setMouseCursorVisible(!mouselook);
				}
			}
			else if (event.type == sf::Event::LostFocus) {
				active = false;
				mouselook = false;
				window.setMouseCursorVisible(!mouselook);
			}
			else if (event.type == sf::Event::GainedFocus) {
				active = true;
			}
		}			

		TotalFrames++;
		float currentTime = frameTimer.CurrentTime<float>();
		if (currentTime >= 0.5f) {
			printf("%f FPS | %f MS \n", (float)TotalFrames / currentTime, (currentTime / (float)TotalFrames) * 1000.f);
			globals::MainFPS = (float)TotalFrames / currentTime;
			globals::MainMS = (currentTime / (float)TotalFrames) * 1000.f;
			frameTimer.Restart();
			TotalFrames = 0;
		}

		if (mouselook) {
			sf::Vector2i windowCenter = sf::Vector2i(window.getSize().x / 2, window.getSize().y / 2);
			if (sf::Mouse::getPosition(window) != windowCenter && active) {
				cameraRot.y += (((float)sf::Mouse::getPosition(window).x - (float)windowCenter.x) / 1000) * globals::MouseSensitivity;
				cameraRot.x += (((float)sf::Mouse::getPosition(window).y - (float)windowCenter.y) / 1000) * globals::MouseSensitivity;
				sf::Mouse::setPosition(windowCenter, window);
			}
		}

		auto camDir = glm::vec3(glm::vec4(0, 0, -1, 1) * glm::rotate(cameraRot.x, glm::vec3(1, 0, 0)) * glm::rotate(cameraRot.y, glm::vec3(0, 1, 0)));
		auto camRight = glm::vec3(glm::vec4(1, 0, 0, 1) * glm::rotate(cameraRot.y, glm::vec3(0, 1, 0)));

		float speed = globals::MovementSpeed;

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
		glClear(GL_COLOR_BUFFER_BIT);

		camera.FOV = globals::FieldOfView;
		camera.Update(mainShader, (float)window.getSize().x / (float)window.getSize().y, glm::vec4(0, 0, window.getSize().x, window.getSize().y));
		mainShader->SendUniform("ScreenSize", glm::vec2(window.getSize().x, window.getSize().y));
		mainShader->SendUniform("Time", totalTime);

		if (globals::Epsilon <= 0) {
			globals::Epsilon = 0.0005f;
		}
		mainShader->SendUniform("EPSILON", globals::Epsilon);
		mainShader->SendUniform("MAX_DISTANCE", globals::MarchDistance);
		mainShader->SendUniform("MAX_MARCHING_STEPS", globals::MarchSteps);

		mainShader->SendUniform("ShadowsEnabled", (int)globals::ShadowsEnabled);
		mainShader->SendUniform("ShadowStrength", globals::ShadowStrength);
		mainShader->SendUniform("AOStrength", globals::AOStrength);
		mainShader->SendUniform("AmbientColor", globals::AmbientColor);
		mainShader->SendUniform("LightColor", globals::LightColor);
		mainShader->SendUniform("LightDir", globals::LightDirection);

		mainShader->Bind();

		/*model.Bind(0);
		mainShader->SendUniform("Model", 0);*/

		glBindVertexArray(VAO); 
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindTexture(GL_TEXTURE_3D, 0);

		window.display();
	}
	
	

	UtilityThread.wait();

	return 0;
}