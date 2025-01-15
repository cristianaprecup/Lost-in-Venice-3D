#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "GLEW/glew.h"
#include "GLFW\include\glfw3.h"
#include "Graphics\window.h"
#include "Camera\camera.h"
#include "Shaders\shader.h"
#include "Model Loading\mesh.h"
#include "Model Loading\texture.h"
#include "Model Loading\meshLoaderObj.h"
//---------
#include <random>
//--------

#include <chrono>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION 
#include "../Dependencies/stb/stb_image.h"

enum ObjectiveState {
	FIND_MAP,
	FIND_BOAT,
	FIND_CABINET,
	FIND_KEY,
	FIND_FUEL,
	ESCAPE
};
ObjectiveState currentObjective = FIND_MAP;


// ----------------- for gui -----------------
enum GameState {
	START_SCREEN,
	GAME_RUNNING,
	GAME_PAUSED,
	GAME_OVER
};
GameState gameState = START_SCREEN;

// ----------------- for gui -----------------


float deltaTime = 0.0f;
float lastFrame = 0.0f;
float rotationAngle = 0.5f;
bool keyTouched = false;
static bool bensTouched = false;
 
std::chrono::time_point<std::chrono::high_resolution_clock> previousTime = std::chrono::high_resolution_clock::now();

// Function to calculate the elapsed time since the last frame
float getDeltaTime() {
	auto currentTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> elapsedTime = currentTime - previousTime;
	previousTime = currentTime;
	return elapsedTime.count();
}

Window window("Game Engine", 800, 800);
Camera camera;

glm::vec3 lightColor = glm::vec3(1.0f);
glm::vec3 lightPos = glm::vec3(-180.0f, 100.0f, -200.0f);

std::vector<std::string> faces
{
  "Resources/Skybox/px.png",
  "Resources/Skybox/nx.png",
  "Resources/Skybox/py.png",
  "Resources/Skybox/ny.png",
  "Resources/Skybox/pz.png",
  "Resources/Skybox/nz.png"
};


GLuint loadCubemap(std::vector<std::string> faces)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);

		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

GLuint cubemapTexture = loadCubemap(faces);

//-------------

glm::vec3 mapPosition;


float randomFloat(float min, float max) {
	static std::random_device rd;
	static std::mt19937 generator(rd());
	std::uniform_real_distribution<float> distribution(min, max);
	return distribution(generator);
}

void initGame() {
	float randomX = randomFloat(-50.0f, -50.0);
	float randomY = -17.0f;
	float randomZ = randomFloat(-50.0f, 50.0f);

	mapPosition = glm::vec3(randomX, randomY, randomZ);

	std::cout << "Map spawned at position: ("
		<< mapPosition.x << ", "
		<< mapPosition.y << ", "
		<< mapPosition.z << ")" << std::endl;
}

//-----------------

// ---------------- for ghost ----------------

glm::vec3 phantomPosition = glm::vec3(0.5f, -10.0f, 0.1f);
float phantomSpeed = 4.0f;
float phantomRotation = 0.0f;

void updatePhantom(float deltaTime, const glm::vec3& cameraPosition) {
	if (deltaTime <= 0.0f) return;

	glm::vec3 direction = cameraPosition - phantomPosition;

	direction = glm::normalize(direction);
	phantomPosition += direction * phantomSpeed * deltaTime;
	phantomPosition.y = cameraPosition.y;
	phantomRotation = glm::degrees(atan2(direction.z, direction.x));
}




GLuint LoadTextureFromFile(const char* filename) {
	int width, height, channels;
	unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);
	if (!data) {
		std::cerr << "Failed to load texture: " << filename << std::endl;
		return 0;
	}

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(data);

	return textureID;
}

GLuint backgroundTexture = LoadTextureFromFile("path/to/your/background/image.png");

void processKeyboardInput();

//glm::vec3 signPosition = glm::vec3(-35.0f, -20.0f, 125.0f);
float calculateDistance(glm::vec3 from, glm::vec3 to) {
	return glm::length(to - from);
}



int main()
{

	//--------------------IMGUI--------------------
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window.getWindow(), true);
	ImGui_ImplOpenGL3_Init("#version 330");

	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameRounding = 10.0f;
	style.Colors[ImGuiCol_Button] = ImVec4(0.4f, 0.2f, 0.1f, 1.0f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.5f, 0.3f, 0.2f, 1.0f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.3f, 0.1f, 0.0f, 1.0f);

	io.FontGlobalScale = 1.5f;

	//---------------------------------------------

	glClearColor(0.2f, 0.8f, 1.0f, 1.0f);

	initGame();



	float skyboxVertices[] = {


		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};


	GLuint skyboxVAO, skyboxVBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glBindVertexArray(0);


	Shader skyboxShader("Shaders/skybox_vertex.glsl", "Shaders/skybox_fragment.glsl");
	Shader shader("Shaders/vertex_shader.glsl", "Shaders/fragment_shader.glsl");
	Shader sunShader("Shaders/sun_vertex_shader.glsl", "Shaders/sun_fragment_shader.glsl");

	GLuint tex = loadBMP("Resources/Textures/wood.bmp");
	GLuint tex2 = loadBMP("Resources/Textures/rock.bmp");
	GLuint tex3 = loadBMP("Resources/Textures/orange.bmp");
	GLuint tex4 = loadBMP("Resources/Textures/wood.bmp");
	GLuint dirtTexture = loadBMP("Resources/Textures/dirt.bmp");
	GLuint boatTexture = loadBMP("Resources/Textures/boat.bmp");
	GLuint mapTexture = loadBMP("Resources/Textures/map.bmp");
	GLuint backgroundTexture = loadBMP("Resources/Textures/background.bmp");
	GLuint skin = loadBMP("Resources/Textures/skin.bmp");
	GLuint signTexture = loadBMP("Resources/Textures/sign.bmp");
	GLuint ghostTexture = loadBMP("Resources/Textures/ghost.bmp");
	GLuint leafTexture = loadBMP("Resources/Textures/leaf.bmp");

	glEnable(GL_DEPTH_TEST);

	std::vector<Vertex> vert;
	vert.push_back(Vertex());
	vert[0].pos = glm::vec3(10.5f, 10.5f, 0.0f);
	vert[0].textureCoords = glm::vec2(1.0f, 1.0f);

	vert.push_back(Vertex());
	vert[1].pos = glm::vec3(10.5f, -10.5f, 0.0f);
	vert[1].textureCoords = glm::vec2(1.0f, 0.0f);

	vert.push_back(Vertex());
	vert[2].pos = glm::vec3(-10.5f, -10.5f, 0.0f);
	vert[2].textureCoords = glm::vec2(0.0f, 0.0f);

	vert.push_back(Vertex());
	vert[3].pos = glm::vec3(-10.5f, 10.5f, 0.0f);
	vert[3].textureCoords = glm::vec2(0.0f, 1.0f);

	vert[0].normals = glm::normalize(glm::cross(vert[1].pos - vert[0].pos, vert[3].pos - vert[0].pos));
	vert[1].normals = glm::normalize(glm::cross(vert[2].pos - vert[1].pos, vert[0].pos - vert[1].pos));
	vert[2].normals = glm::normalize(glm::cross(vert[3].pos - vert[2].pos, vert[1].pos - vert[2].pos));
	vert[3].normals = glm::normalize(glm::cross(vert[0].pos - vert[3].pos, vert[2].pos - vert[3].pos));

	std::vector<int> ind = { 0, 1, 3,
		1, 2, 3 };

	std::vector<Texture> textures;
	textures.push_back(Texture());
	textures[0].id = tex;
	textures[0].type = "texture_diffuse";

	std::vector<Texture> textures2;
	textures2.push_back(Texture());
	textures2[0].id = tex2;
	textures2[0].type = "texture_diffuse";

	std::vector<Texture> textures3;
	textures3.push_back(Texture());
	textures3[0].id = tex3;
	textures3[0].type = "texture_diffuse";

	std::vector<Texture> textures4;
	textures4.push_back(Texture());
	textures4[0].id = tex4;
	textures4[0].type = "texture_diffuse";

	std::vector<Texture> textureDirt;
	textureDirt.push_back(Texture());
	textureDirt[0].id = dirtTexture;
	textureDirt[0].type = "texture_diffuse";

	std::vector<Texture> textureBoat;
	textureBoat.push_back(Texture());
	textureBoat[0].id = boatTexture;
	textureBoat[0].type = "texture_diffuse";

	std::vector<Texture> textureMap;
	textureMap.push_back(Texture());
	textureMap[0].id = mapTexture;
	textureMap[0].type = "texture_diffuse";

	std::vector<Texture> textureSign;
	textureSign.push_back(Texture());
	textureSign[0].id = signTexture;
	textureSign[0].type = "texture_diffuse";
	//----------

	//---------
	std::vector<Texture> textureGhost;
	textureGhost.push_back(Texture());
	textureGhost[0].id = ghostTexture;
	textureGhost[0].type = "texture_diffuse";
	//----------

	std::vector<Texture> textureLeaf;
	textureLeaf.push_back(Texture());
	textureLeaf[0].id = leafTexture;
	textureLeaf[0].type = "texture_diffuse";


	Mesh mesh(vert, ind, textures3);


	MeshLoaderObj loader;
	Mesh sun = loader.loadObj("Resources/Models/sphere.obj");
	Mesh box = loader.loadObj("Resources/Models/cube.obj", textures);
	Mesh plane = loader.loadObj("Resources/Models/plane.obj", textureDirt);
	Mesh tree = loader.loadObj("Resources/Models/Tree.obj", textureLeaf);
	Mesh watchtowe = loader.loadObj("Resources/Models/watchtower.obj", textures4);
	Mesh wood = loader.loadObj("Resources/Models/Wood.obj", textures4);
	Mesh woodHouse = loader.loadObj("Resources/Models/woodHouse.obj", textures4);
	Mesh house2 = loader.loadObj("Resources/Models/house2.obj", textures4);
	Mesh house3 = loader.loadObj("Resources/Models/house3.obj", textures4);
	Mesh house4 = loader.loadObj("Resources/Models/house4.obj", textures4);
	Mesh house5 = loader.loadObj("Resources/Models/house5.obj", textures4);
	Mesh bridge = loader.loadObj("Resources/Models/bridge.obj", textures2);
	Mesh plane2 = loader.loadObj("Resources/Models/plane.obj", textureDirt);
	Mesh woodHouse2 = loader.loadObj("Resources/Models/woodHouse.obj", textures4);
	Mesh boat = loader.loadObj("Resources/Models/boat.obj", textureBoat);
	Mesh arms = loader.loadObj("Resources/Models/arms.obj", textures);
	Mesh key = loader.loadObj("Resources/Models/key.obj", textures3);
	Mesh dulap = loader.loadObj("Resources/Models/dulap.obj", textures4);
	Mesh bens = loader.loadObj("Resources/Models/bens.obj", textures3);
	Mesh phantom = loader.loadObj("Resources/Models/ghost.obj", textureGhost);


	//--------------------
	Mesh map = loader.loadObj("Resources/Models/map.obj", textureMap);
	Mesh sign = loader.loadObj("Resources/Models/sign.obj", textureSign);

	bool showStartMessage = true;
	static float startTime = 0.0f;
	glm::vec3 signPosition = glm::vec3(-35.0f, -20.0f, 125.0f);


	while (!window.isPressed(GLFW_KEY_ESCAPE) &&
		glfwWindowShouldClose(window.getWindow()) == 0)
	{
		window.clear();

		if (gameState == GAME_RUNNING) {
			float currentFrame = glfwGetTime();
			deltaTime = currentFrame - lastFrame;
			lastFrame = currentFrame;
		}

		processKeyboardInput();


		glDepthFunc(GL_LEQUAL);
		skyboxShader.use();
		glm::mat4 view = glm::mat4(glm::mat3(camera.getViewMatrix())); // Remove translation from the view matrix
		glm::mat4 projection = glm::perspective(glm::radians(90.0f), (float)window.getWidth() / (float)window.getHeight(), 0.1f, 100.0f);
		GLuint viewLoc = glGetUniformLocation(skyboxShader.getId(), "view");
		GLuint projectionLoc = glGetUniformLocation(skyboxShader.getId(), "projection");
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &projection[0][0]);
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();


		ImVec2 buttonSize = ImVec2(io.DisplaySize.x * 0.2f, io.DisplaySize.y * 0.1f);
		float centerX = (io.DisplaySize.x - buttonSize.x) / 2.0f;
		float centerY = (io.DisplaySize.y - buttonSize.y * 2 - 20) / 2.0f;

		if (gameState == START_SCREEN) { // sart page
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSize(io.DisplaySize);
			ImGui::Begin("##Background", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar);
			ImGui::Image((ImTextureID)(uintptr_t)backgroundTexture, io.DisplaySize);
			ImGui::End();
			ImGui::SetCursorPos(ImVec2(centerX, centerY));
			if (ImGui::Button("Start Game", buttonSize)) {
				gameState = GAME_RUNNING;
			}
			ImGui::SetCursorPos(ImVec2(centerX, centerY + buttonSize.y + 100));
			if (ImGui::Button("Exit", buttonSize)) {
				glfwSetWindowShouldClose(window.getWindow(), true);
			}
		}
		else if (gameState == GAME_OVER) {
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSize(io.DisplaySize);
			ImGui::Begin("##Background", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar);

			ImGui::Image((ImTextureID)(uintptr_t)backgroundTexture, io.DisplaySize);
			ImGui::End();

			ImGui::SetCursorPos(ImVec2((io.DisplaySize.x - ImGui::CalcTextSize("Game Over!").x) / 2, io.DisplaySize.y));
			ImGui::Text("Game Over! Better luck next time.");

			ImVec2 buttonSize = ImVec2(io.DisplaySize.x * 0.2f, io.DisplaySize.y * 0.1f);
			float centerX = (io.DisplaySize.x - buttonSize.x) / 2.0f;

			ImGui::SetCursorPos(ImVec2(centerX, io.DisplaySize.y * 0.6f + buttonSize.y + 20));
			if (ImGui::Button("Exit", buttonSize)) {
				glfwSetWindowShouldClose(window.getWindow(), true);
			}
		}

		if (gameState == GAME_RUNNING) {
			// aici se intampla chestii in joc, daca se pune in afara o sa fie si in pagina de start sau pause


			glDepthFunc(GL_LEQUAL);
			skyboxShader.use();
			glm::mat4 view = glm::mat4(glm::mat3(camera.getViewMatrix())); // Remove translation from the view matrix
			glm::mat4 projection = glm::perspective(glm::radians(90.0f), (float)window.getWidth() / (float)window.getHeight(), 0.1f, 100.0f);
			GLuint viewLoc = glGetUniformLocation(skyboxShader.getId(), "view");
			GLuint projectionLoc = glGetUniformLocation(skyboxShader.getId(), "projection");
			glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
			glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &projection[0][0]);

			glBindVertexArray(skyboxVAO);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glBindVertexArray(0);
			glDepthFunc(GL_LESS);

			//Message from the start
			if (showStartMessage) {
				static float startMessageStartTime = 0.0f;
				if (startMessageStartTime == 0.0f) {
					startMessageStartTime = glfwGetTime();
				}
				float elapsedTime = glfwGetTime() - startMessageStartTime;
				float bounceHeight = 20.0f;
				float bounceSpeed = 3.0f;
				float offsetY = abs(sin(elapsedTime * bounceSpeed)) * bounceHeight;
				ImVec2 windowSize = io.DisplaySize;
				ImVec2 messagePos = ImVec2(windowSize.x / 2.0f, 100.0f + offsetY);
				ImGui::SetNextWindowPos(messagePos, ImGuiCond_Always, ImVec2(0.5f, 0.0f));
				ImGui::Begin("StartMessage", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
				ImGui::Text("Try to escape? There is a map with clues around.");
				ImGui::End();

				if (elapsedTime > 4.0f) {
					showStartMessage = false;
					startMessageStartTime = 0.0f;
				}
			}

			// Message from map
			static float mapMessageStartTime = 0.0f;
			float distanceToMap = calculateDistance(camera.getCameraPosition(), mapPosition);
			if (distanceToMap < 10.0f) {
				if (mapMessageStartTime == 0.0f) {
					mapMessageStartTime = glfwGetTime();
				}
				float elapsedTime = glfwGetTime() - mapMessageStartTime;
				float bounceHeight = 20.0f;
				float bounceSpeed = 3.0f;
				float offsetY = abs(sin(elapsedTime * bounceSpeed)) * bounceHeight;
				ImVec2 windowSize = io.DisplaySize;
				ImVec2 messagePos = ImVec2(windowSize.x / 2.0f, 100.0f + offsetY);
				ImGui::SetNextWindowPos(messagePos, ImGuiCond_Always, ImVec2(0.5f, 0.0f));
				ImGui::Begin("Message", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
				ImGui::Text("Boat is your way out. Find it!");
				ImGui::End();
				if (elapsedTime > 10.0f) {
					mapMessageStartTime = glfwGetTime();
				}
			}
			else {
				mapMessageStartTime = 0.0f;
			}

			// Message from sign
			glm::vec3 signPosition = glm::vec3(-35.0f, -20.0f, 120.0f);
			static float signMessageStartTime = 0.0f;

			float signProximityThreshold = 20.0f;
			float distanceToSign = glm::distance(camera.getCameraPosition(), signPosition);
			std::cout << "Distance to sign: " << distanceToSign << std::endl;
			bool displaySignMessage = distanceToSign < signProximityThreshold;

			if (displaySignMessage) {
				if (signMessageStartTime == 0.0f) {
					signMessageStartTime = glfwGetTime();
				}
				float elapsedTime = glfwGetTime() - signMessageStartTime;
				float bounceHeight = 20.0f;
				float bounceSpeed = 3.0f;
				float offsetY = abs(sin(elapsedTime * bounceSpeed)) * bounceHeight;
				ImVec2 windowSize = io.DisplaySize;
				ImVec2 messagePos = ImVec2(windowSize.x / 2.0f, 100.0f + offsetY);
				ImGui::SetNextWindowPos(messagePos, ImGuiCond_Always, ImVec2(0.5f, 0.0f));
				ImGui::Begin("Notification", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
				ImGui::Text("Boat needs fuel!");
				ImGui::End();
				if (elapsedTime > 10.0f) {
					signMessageStartTime = glfwGetTime();
				}
			}
			else {
				signMessageStartTime = 0.0f;
			}

			// Message from dulap
			glm::vec3 dulapPosition = glm::vec3(-14.0f, -18.0f, -70.0f);
			float dulapProximityThreshold = 18.0f;
			static float dulapMessageStartTime = 0.0f;

			float distanceToDulap = glm::distance(camera.getCameraPosition(), dulapPosition);
			std::cout << "Distance to dulap: " << distanceToDulap << std::endl;
			bool displayDulapMessage = distanceToDulap < dulapProximityThreshold;

			if (displayDulapMessage) {
				if (dulapMessageStartTime == 0.0f) {
					dulapMessageStartTime = glfwGetTime();
				}
				float elapsedTime = glfwGetTime() - dulapMessageStartTime;
				float bounceHeight = 20.0f;
				float bounceSpeed = 3.0f;
				float offsetY = abs(sin(elapsedTime * bounceSpeed)) * bounceHeight;
				ImVec2 windowSize = io.DisplaySize;
				ImVec2 messagePos = ImVec2(windowSize.x / 2.0f, 100.0f + offsetY);
				ImGui::SetNextWindowPos(messagePos, ImGuiCond_Always, ImVec2(0.5f, 0.0f));
				ImGui::Begin("Notification", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
				ImGui::Text("You found the fuel! But you may need a key to open the cabinet...");
				ImGui::End();
				if (elapsedTime > 10.0f) {
					dulapMessageStartTime = glfwGetTime();
				}
			}
			else {
				dulapMessageStartTime = 0.0f;
			}

			// Position of the key
			glm::vec3 keyPosition = glm::vec3(18.0f, -10.0f, 30.0f);
			float keyProximityThreshold = 20.0f;
			static float keyMessageStartTime = 0.0f;

			float distanceToKey = glm::distance(camera.getCameraPosition(), keyPosition);
			std::cout << "Distance to key: " << distanceToKey << std::endl;
			bool displayKeyMessage = distanceToKey < keyProximityThreshold;

			if (displayKeyMessage) {
				if (keyMessageStartTime == 0.0f) {
					keyMessageStartTime = glfwGetTime();
				}
				float elapsedTime = glfwGetTime() - keyMessageStartTime;
				float bounceHeight = 20.0f;
				float bounceSpeed = 3.0f;
				float offsetY = abs(sin(elapsedTime * bounceSpeed)) * bounceHeight;
				ImVec2 windowSize = io.DisplaySize;
				ImVec2 messagePos = ImVec2(windowSize.x / 2.0f, 100.0f + offsetY);
				ImGui::SetNextWindowPos(messagePos, ImGuiCond_Always, ImVec2(0.5f, 0.0f));
				ImGui::Begin("Notification", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
				ImGui::Text("You found it! Now go back, get the fuel and run!");
				ImGui::End();
				if (elapsedTime > 10.0f) {
					keyMessageStartTime = glfwGetTime();
				}
			}
			else {
				keyMessageStartTime = 0.0f;
			}

			// Position of Bens
			glm::vec3 bensPosition = glm::vec3(-14.0f, -11.0f, -68.0f);
			float bensProximityThreshold = 3.0f;
			static float bensMessageStartTime = 0.0f;

			float distanceToBens = glm::distance(camera.getCameraPosition(), bensPosition);
			std::cout << "Distance to bens: " << distanceToBens << std::endl;
			bool displayBensMessage = distanceToBens < bensProximityThreshold;

			if (displayBensMessage) {
				if (bensMessageStartTime == 0.0f) {
					bensMessageStartTime = glfwGetTime();
				}
				float elapsedTime = glfwGetTime() - bensMessageStartTime;
				float bounceHeight = 20.0f;
				float bounceSpeed = 3.0f;
				float offsetY = abs(sin(elapsedTime * bounceSpeed)) * bounceHeight;
				ImVec2 windowSize = io.DisplaySize;
				ImVec2 messagePos = ImVec2(windowSize.x / 2.0f, 100.0f + offsetY);
				ImGui::SetNextWindowPos(messagePos, ImGuiCond_Always, ImVec2(0.5f, 0.0f));
				ImGui::Begin("Notification", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
				ImGui::Text("RUN! RUN! RUN!");
				ImGui::End();
				if (elapsedTime > 10.0f) {
					bensMessageStartTime = glfwGetTime();
				}
			}
			else {
				bensMessageStartTime = 0.0f;
			}


			// Position of the boat
			glm::vec3 boatPosition = glm::vec3(-15.0f, -20.0f, 125.0f);
			float boatProximityThreshold = 5.0f;
			static float boatMessageStartTime = 0.0f;

			float distanceToBoat = glm::distance(camera.getCameraPosition(), boatPosition);
			std::cout << "Distance to boat: " << distanceToBoat << std::endl;
			bool displayBoatMessage = distanceToBoat < boatProximityThreshold;

			if (displayBoatMessage) {
				if (boatMessageStartTime == 0.0f) {
					boatMessageStartTime = glfwGetTime();
				}
				float elapsedTime = glfwGetTime() - boatMessageStartTime;
				float bounceHeight = 20.0f;
				float bounceSpeed = 3.0f;
				float offsetY = abs(sin(elapsedTime * bounceSpeed)) * bounceHeight;
				ImVec2 windowSize = io.DisplaySize;
				ImVec2 messagePos = ImVec2(windowSize.x / 2.0f, 100.0f + offsetY);
				ImGui::SetNextWindowPos(messagePos, ImGuiCond_Always, ImVec2(0.5f, 0.0f));
				ImGui::Begin("Notification", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
				ImGui::Text("You won!");
				ImGui::End();
				if (elapsedTime > 10.0f) {
					boatMessageStartTime = glfwGetTime();
				}
			}
			else {
				boatMessageStartTime = 0.0f;
			}

			


			sunShader.use();
			glm::mat4 ProjectionMatrix = glm::perspective(90.0f, window.getWidth() * 1.0f / window.getHeight(), 0.1f, 10000.0f);
			glm::mat4 ViewMatrix = glm::lookAt(camera.getCameraPosition(), camera.getCameraPosition() + camera.getCameraViewDirection(), camera.getCameraUp());

			GLuint MatrixID = glGetUniformLocation(sunShader.getId(), "MVP");

			glm::mat4 ModelMatrix = glm::mat4(1.0);
			ModelMatrix = glm::translate(ModelMatrix, lightPos);
			glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

			sun.draw(sunShader);

			shader.use();

			GLuint MatrixID2 = glGetUniformLocation(shader.getId(), "MVP");
			GLuint ModelMatrixID = glGetUniformLocation(shader.getId(), "model");

			ModelMatrix = glm::mat4(1.0);
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniform3f(glGetUniformLocation(shader.getId(), "lightColor"), lightColor.x, lightColor.y, lightColor.z);
			glUniform3f(glGetUniformLocation(shader.getId(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
			glUniform3f(glGetUniformLocation(shader.getId(), "viewPos"), camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);

			box.draw(shader);

			ModelMatrix = glm::mat4(1.0);
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-10.0f, -20.0f, 0.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.8f, 0.8f, 0.8f));
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			plane.draw(shader);

			ModelMatrix = glm::mat4(1.0);
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(40.0f, -20.0f, 20.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(5.0f, 5.0f, 5.0f));
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			tree.draw(shader);

			ModelMatrix = glm::mat4(1.0);
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(20.0f, -25.0f, 30.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(5.0f, 5.0f, 5.0f));
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			watchtowe.draw(shader);

			ModelMatrix = glm::mat4(1.0);
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-10.0f, -20.0f, 30.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(7.0f, 12.5f, 8.0f));
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			woodHouse.draw(shader);

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-50.0f, -18.0f, -60.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.05f, 0.05f, 0.05f));
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			house2.draw(shader);

			ModelMatrix = glm::mat4(1.0);
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, -18.0f, -60.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.05f, 0.05f, 0.05f));
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			house2.draw(shader);

			ModelMatrix = glm::mat4(1.0);
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(30.0f, -18.0f, -60.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.05f, 0.05f, 0.05f));
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			house2.draw(shader);

			ModelMatrix = glm::mat4(1.0);
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, -18.0f, -40.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.05f, 0.05f, 0.05f));
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			house3.draw(shader);

			ModelMatrix = glm::mat4(1.0);
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-20.0f, -15.0f, -20.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.12f, 0.12f, 0.12f));
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			house4.draw(shader);

			ModelMatrix = glm::mat4(1.0);
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-70.0f, -20.0f, 30.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.3f, 0.3f, 0.3f));
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			house5.draw(shader);

			ModelMatrix = glm::mat4(1.0);
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-50.0f, -20.0f, 70.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.0f, 1.0f, 1.0f));
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			bridge.draw(shader);

			ModelMatrix = glm::mat4(1.0);
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-50.0f, -20.0f, 115.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.3f, 0.3f, 0.3f));
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			plane.draw(shader);

			ModelMatrix = glm::mat4(1.0);
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-60.0f, -20.0f, 120.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(7.0f, 12.5f, 8.0f));
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			woodHouse.draw(shader);

			ModelMatrix = glm::mat4(1.0);
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-35.0f, -20.0f, 125.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(5.0f, 5.0f, 5.0f));
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			tree.draw(shader);

			
			if (currentObjective >= FIND_BOAT) {
				ModelMatrix = glm::mat4(1.0);
				ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-15.0f, -20.0f, 125.0f));
				ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.1f, 0.1f, 0.1f));
				MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
				boat.draw(shader);
			}

			if (currentObjective >= FIND_MAP) {
				ModelMatrix = glm::mat4(1.0);
				ModelMatrix = glm::translate(ModelMatrix, mapPosition);
				ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.4f, 0.4f, 0.4f));
				ModelMatrix = glm::rotate(ModelMatrix, 50.0f * (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
				MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
				map.draw(shader);
			}

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::translate(ModelMatrix, signPosition); // Updated translation
			ModelMatrix = glm::rotate(ModelMatrix, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotation to face forward
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(5.0f, 5.0f, 5.0f));
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			sign.draw(shader);




			//arms 

			ModelMatrix = glm::mat4(1.0f);
			glm::vec3 armsOffset = glm::vec3(0.0f, -2.0f, 0.0f); // Adjust as needed
			glm::vec3 cameraPos = camera.getCameraPosition();
			glm::vec3 cameraViewDir = camera.getCameraViewDirection();
			glm::vec3 cameraUp = camera.getCameraUp();
			glm::vec3 armsPos = cameraPos + cameraViewDir * 0.5f + armsOffset;
			ModelMatrix = glm::translate(glm::mat4(1.0f), armsPos);
			glm::vec3 forward = glm::normalize(cameraViewDir);
			glm::vec3 right = glm::normalize(glm::cross(forward, cameraUp));
			glm::vec3 up = glm::normalize(glm::cross(right, forward));
			glm::mat4 rotationMatrix = glm::mat4(
				glm::vec4(right, 0.0f),
				glm::vec4(up, 0.0f),
				glm::vec4(-forward, 0.0f),
				glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
			);

		ModelMatrix *= rotationMatrix;
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		arms.draw(shader);
		
		//key
		if (currentObjective >= FIND_KEY) {
		if (!keyTouched) {
			glm::vec3 keyPosition = glm::vec3(18.0f, -10.0f, 30.0f);
			glm::vec3 distanceVector = cameraPos - keyPosition;
			float distance = glm::length(distanceVector);
			float distanceThreshold = 1.0f;

			if (distance < distanceThreshold) {
				keyTouched = true;
			}

			if (!keyTouched) {

				ModelMatrix = glm::mat4(1.0f);
				ModelMatrix = glm::translate(ModelMatrix, keyPosition);
				ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.2f, 0.2f, 0.2f));
				ModelMatrix = glm::rotate(ModelMatrix, 50.0f * (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
				MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
				key.draw(shader);
			}
		}
		key.draw(shader);
		}


			//dulap
			if (currentObjective >= FIND_CABINET) {
				ModelMatrix = glm::mat4(1.0f);
				ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-14.0f, -18.0f, -70.0f));
				ModelMatrix = glm::scale(ModelMatrix, glm::vec3(5.0f, 5.0f, 9.0f));
				MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
				dulap.draw(shader);
			}
			if (currentObjective >= FIND_FUEL) {
				ModelMatrix = glm::mat4(1.0f);
				ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-14.0f, -11.0f, -68.0f));
				ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.8f, 0.8f, 0.8f));
				MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
				bens.draw(shader);
			}

			glm::vec3 cameraPosition = camera.getCameraPosition();
			updatePhantom(deltaTime, cameraPosition);

			ModelMatrix = glm::mat4(1.0f);
			ModelMatrix = glm::translate(ModelMatrix, phantomPosition);
			ModelMatrix = glm::rotate(ModelMatrix, glm::radians(phantomRotation), glm::vec3(0.0f, 1.0f, 0.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(5.0f, 5.0f, 5.0f));

			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

			phantom.draw(shader);

			//------------

			if (glm::length(cameraPosition - phantomPosition) < 0.5f) {
				gameState = GAME_OVER;
			}

			float distanceToMap2 = calculateDistance(camera.getCameraPosition(), mapPosition);
			if (currentObjective == FIND_MAP && distanceToMap2 < 10.0f) {
				currentObjective = FIND_BOAT;
			}

			float distanceToBoat2 = calculateDistance(camera.getCameraPosition(), glm::vec3(-15.0f, -20.0f, 125.0f));
			if (currentObjective == FIND_BOAT && distanceToBoat2 < 30.0f) {
				currentObjective = FIND_CABINET;
			}

			float distanceToCabinet = calculateDistance(camera.getCameraPosition(), glm::vec3(-14.0f, -18.0f, -70.0f));
			if (currentObjective == FIND_CABINET && distanceToCabinet < 10.0f) {
				currentObjective = FIND_KEY;
			}

			float distanceToKey2 = calculateDistance(camera.getCameraPosition(), glm::vec3(18.0f, -10.0f, 30.0f));
			if (currentObjective == FIND_KEY && distanceToKey2 < 10.0f) {
				currentObjective = FIND_FUEL;
			}

			float distanceToFuel = calculateDistance(camera.getCameraPosition(), glm::vec3(-14.0f, -11.0f, -68.0f));
			if (currentObjective == FIND_FUEL && distanceToFuel < 20.0f) {
				currentObjective = ESCAPE;
			}

			float distanceToBoatAfterFuel = calculateDistance(camera.getCameraPosition(), glm::vec3(-15.0f, -20.0f, 125.0f));
			if (currentObjective == ESCAPE && distanceToBoatAfterFuel < 30.0f) {
				std::cout << "You won the game!" << std::endl;
				gameState = GAME_OVER;
			}

		}
		// pauza:
		else if (gameState == GAME_PAUSED) {
			ImVec2 windowSize = io.DisplaySize;
			float centerX = (windowSize.x - buttonSize.x) / 2.0f;
			float centerY = (windowSize.y - buttonSize.y * 3 - 40) / 2.0f;

			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSize(io.DisplaySize);
			ImGui::Begin("##Background", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar);

			if (backgroundTexture != 0) {
				ImGui::Image((ImTextureID)(uintptr_t)backgroundTexture, io.DisplaySize);
			}
			else {
				ImGui::Text("Failed to load background texture.");
			}

			ImGui::End();

			ImGui::SetCursorPos(ImVec2((windowSize.x - ImGui::CalcTextSize("Paused").x) / 2, centerY - 100));
			ImGui::Text("Paused");

			ImGui::SetCursorPos(ImVec2(centerX, centerY));
			if (ImGui::Button("Resume", buttonSize)) {
				gameState = GAME_RUNNING;
			}

			ImGui::SetCursorPos(ImVec2(centerX, centerY + buttonSize.y + 20));
			if (ImGui::Button("Exit to Main Menu", buttonSize)) {
				gameState = START_SCREEN;
			}

			ImGui::SetCursorPos(ImVec2(centerX, centerY + 2 * (buttonSize.y + 20)));
			if (ImGui::Button("Exit Game", buttonSize)) {
				glfwSetWindowShouldClose(window.getWindow(), true);
			}
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		window.update();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void processKeyboardInput() {
	static bool escPressed = false;

	if (window.isPressed(GLFW_KEY_ESCAPE)) {
		if (!escPressed) {
			escPressed = true;
			if (gameState == GAME_RUNNING) {
				gameState = GAME_PAUSED;
			}
			else if (gameState == GAME_PAUSED) {
				gameState = GAME_RUNNING;
			}
		}
	}
	else {
		escPressed = false;
	}

	if (gameState == GAME_RUNNING) {
		float cameraSpeed = 20.0f * deltaTime;
		float rotationSpeed = glm::radians(90.0f * deltaTime * 20);

		if (window.isPressed(GLFW_KEY_W))
			camera.keyboardMoveFront(cameraSpeed);
		if (window.isPressed(GLFW_KEY_S))
			camera.keyboardMoveBack(cameraSpeed);
		if (window.isPressed(GLFW_KEY_A))
			camera.keyboardMoveLeft(cameraSpeed);
		if (window.isPressed(GLFW_KEY_D))
			camera.keyboardMoveRight(cameraSpeed);

		if (window.isPressed(GLFW_KEY_R))
			camera.keyboardMoveUp(cameraSpeed);
		if (window.isPressed(GLFW_KEY_F))
			camera.keyboardMoveDown(cameraSpeed);

		if (window.isPressed(GLFW_KEY_LEFT))
			camera.rotateOy(rotationSpeed);
		if (window.isPressed(GLFW_KEY_RIGHT))
			camera.rotateOy(-rotationSpeed);
	}
}
