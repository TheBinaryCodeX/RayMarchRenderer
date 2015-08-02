#include "stdafx.h"
#include "Graphics.h"
#include "SFML\Window.hpp"
#include "SOIL.h"

std::vector<Graphics::Material> materials;
std::vector<Graphics::Object> objects;

std::vector<std::string> matLines;
std::vector<std::string> objLines;

GLuint Graphics::loadShader(GLenum type, std::string path)
{
	std::string content;
	std::ifstream fileStream(path, std::ios::in);

	if (!fileStream.is_open())
	{
		std::cerr << "Could not read file " << path << ". File does not exist." << std::endl;
	}

	std::vector<std::string> lines;

	std::string line = "";
	while (std::getline(fileStream, line))
	{
		lines.push_back(line);
	}

	fileStream.close();

	int maxDepth = 0;
	std::string functionName = "";

	for (int i = 0; i < lines.size(); i++)
	{
		line = lines[i];

		if (line.find("//##") != std::string::npos)
		{
			lines.erase(lines.begin() + i);

			functionName = lines[i].substr(lines[i].find_last_of(" ", lines[i].find("(")) + 1, lines[i].find("(") - lines[i].find_last_of(" ", lines[i].find("(")) - 1);

			size_t pos = line.find("//##") + 4;
			std::string num = line.substr(pos);
			maxDepth = std::stoi(num);

			for (int j = 0; j < maxDepth; j++)
			{
				auto first = lines.begin() + i;
				auto last = first;

				for (int k = i; k < lines.size(); k++)
				{
					if (lines[k] == "}")
					{
						last = lines.begin() + k + 1;
						break;
					}
				}

				lines.insert(lines.begin() + i, first, last);
			}
		}
		else if (line.find("//#MATINSERT") != std::string::npos)
		{
			lines.erase(lines.begin() + i);
			lines.insert(lines.begin() + i, matLines.begin(), matLines.end());
		}
		else if (line.find("//#OBJINSERT") != std::string::npos)
		{
			lines.erase(lines.begin() + i);
			lines.insert(lines.begin() + i, objLines.begin(), objLines.end());
		}
	}

	if (maxDepth != 0)
	{
		int id = maxDepth;
		for (int i = 0; i < lines.size(); i++)
		{
			line = lines[i];

			if (line.find(functionName) != std::string::npos && line.back() != ';')
			{
				size_t pos = line.find(functionName) + functionName.size();

				lines[i].insert(pos, std::to_string(id));
				id--;
			}
			else if (line.find("//#+") != std::string::npos)
			{
				lines.erase(lines.begin() + i);

				size_t pos = line.find("//#+") + 4;
				std::string num = line.substr(pos);
				int sid = std::stoi(num);

				if (id + sid + 1 <= maxDepth)
				{
					pos = lines[i].find(functionName) + functionName.size();
					lines[i].insert(pos, std::to_string(id + sid + 1));
				}
				else
				{
					pos = lines[i].find_last_of("\t") + 1;
					lines[i].erase(pos, lines[i].find("//#IFMAX:") + 9 - pos);
				}
			}
			else if (line.find("//#") != std::string::npos)
			{
				lines.erase(lines.begin() + i);

				size_t pos = line.find("//#") + 3;
				std::string num = line.substr(pos);
				int sid = std::stoi(num);

				pos = lines[i].find(functionName) + functionName.size();
				lines[i].insert(pos, std::to_string(sid));
			}
		}
	}

	for (int i = 0; i < lines.size(); i++)
	{
		content.append(lines[i] + "\n");
	}

	const char* src = content.c_str();

	//std::cout << src << std::endl;

	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (status == GL_FALSE)
	{
		std::cerr << "Compile error in shader: " << path << std::endl;
	}

	char buffer[512];
	glGetShaderInfoLog(shader, 512, NULL, buffer);
	std::cout << buffer << std::endl;

	return shader;
}

Graphics::Shader fullQuad;
Graphics::Shader lightTrace;
Graphics::Shader rayTrace;

Graphics::Framebuffer lightmap;
Graphics::Framebuffer framebuffer;

GLuint fqVAO;
void createFQ()
{
	std::vector<GLfloat> vertexData = std::vector<GLfloat>
	{
		-1.0f, 1.0f, 0.0f, 0.0f,
			1.0f, 1.0f, 1.0f, 0.0f,
			1.0f, -1.0f, 1.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 1.0f
	};

	glGenVertexArrays(1, &fqVAO);
	glBindVertexArray(fqVAO);

	GLuint fqVBO;
	glGenBuffers(1, &fqVBO);
	glBindBuffer(GL_ARRAY_BUFFER, fqVBO);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(GLfloat), &vertexData[0], GL_STATIC_DRAW);

	GLint posAttrib = glGetAttribLocation(fullQuad.program, "pos");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

	GLint uvAttrib = glGetAttribLocation(fullQuad.program, "uv");
	glEnableVertexAttribArray(uvAttrib);
	glVertexAttribPointer(uvAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(float)));
}

void Graphics::Init()
{
	glewExperimental = GL_TRUE;
	glewInit();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	matLines.push_back("Material m0 = Material(vec3(0.5, 0.5, 0.5), false, false, false, 0, 0.6, 1, 0, 1);");
	matLines.push_back("Material m1 = Material(vec3(1.0, 0.1, 0.1), false, false, false, 0, 0.1, 1, 0, 1);");
	matLines.push_back("Material m2 = Material(vec3(0.1, 0.1, 1.0), false, false, false, 0, 0.05, 1, 0, 0.2);");
	matLines.push_back("Material m3 = Material(vec3(0.1, 1.0, 0.1), false, false, false, 0, 1, 1, 0, 1);");
	matLines.push_back("Material m4 = Material(vec3(1.0, 1.0, 1.0), false, false, true, 4, 1, 1, 0, 1);");

	objLines.push_back("d = opU(d, MapData(m0, mapBox(p, vec3(0, -1.025, 0), vec3(32, 0.05, 32))));");
	objLines.push_back("d = opU(d, MapData(m1, mapSphere(p, vec3(-1, 0, 0), 1)));");
	objLines.push_back("d = opU(d, MapData(m2, mapSphere(p, vec3(1, 0, 0), 1)));");
	objLines.push_back("d = opU(d, MapData(m3, mapBox(p, vec3(-4, 1, 0), vec3(0.05, 2, 2))));");
	objLines.push_back("d = opU(d, MapData(m4, mapSphere(p, vec3(8, 8, -4), 4)));");

	fullQuad.Create("FullQuad");
	rayTrace.CreateCompute("RayMarch");

	framebuffer.Create();

	createFQ();

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.buffer);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	/*
	Material m0;
	m0.id = 0;
	m0.color = Vector3(0.8, 0.8, 0.8);
	m0.reflective = false;
	m0.transmissive = false;
	m0.emissive = false;
	m0.power = 0;
	m0.rRoughness = 1;
	m0.tRoughness = 1;
	m0.ior = 1;
	m0.mixFact = 1;
	//materials.push_back(m0);

	Material m1;
	m1.id = 1;
	m1.color = Vector3(1.0, 0.1, 0.1);
	m1.reflective = false;
	m1.transmissive = false;
	m1.emissive = false;
	m1.power = 0;
	m1.rRoughness = 1;
	m1.tRoughness = 1;
	m1.ior = 1;
	m1.mixFact = 1;
	//materials.push_back(m1);

	Material m2;
	m2.id = 2;
	m2.color = Vector3(0.1, 0.1, 1.0);
	m2.reflective = true;
	m2.transmissive = false;
	m2.emissive = false;
	m2.power = 0;
	m2.rRoughness = 0.1;
	m2.tRoughness = 1;
	m2.ior = 1;
	m2.mixFact = 0.2;
	//materials.push_back(m2);

	Material m3;
	m3.id = 3;
	m3.color = Vector3(0.1, 1.0, 0.1);
	m3.reflective = false;
	m3.transmissive = false;
	m3.emissive = false;
	m3.power = 0;
	m3.rRoughness = 1;
	m3.tRoughness = 1;
	m3.ior = 1;
	m3.mixFact = 1;
	//materials.push_back(m3);

	Material m4;
	m4.id = 4;
	m4.color = Vector3(1.0, 1.0, 1.0);
	m4.reflective = false;
	m4.transmissive = false;
	m4.emissive = true;
	m4.power = 512;
	m4.rRoughness = 1;
	m4.tRoughness = 1;
	m4.ior = 1;
	m4.mixFact = 1;
	//materials.push_back(m4);

	Material m5;
	m5.id = 5;
	m5.color = Vector3(0.2, 0.2, 0.2);
	m5.reflective = false;
	m5.transmissive = false;
	m5.emissive = false;
	m5.power = 0;
	m5.rRoughness = 1;
	m5.tRoughness = 1;
	m5.ior = 1;
	m5.mixFact = 1;
	//materials.push_back(m5);

	Object o0;
	o0.matID = 0;
	o0.type = 1;
	o0.centre = Vector3(0, -1.025, 0);
	o0.radius = Vector3(32, 0.05, 32);
	//objects.push_back(o0);
	*/

	/*
	Object o1;
	o1.matID = 5;
	o1.type = 1;
	o1.centre = Vector3(-1.1, 0, 0);
	o1.radius = Vector3(1, 1, 1);
	objects.push_back(o1);

	Object o2;
	o2.matID = 5;
	o2.type = 1;
	o2.centre = Vector3(1.1, 0, 0);
	o2.radius = Vector3(1, 1, 1);
	objects.push_back(o2);
	*/

	/*
	Object o1;
	o1.matID = 1;
	o1.type = 0;
	o1.centre = Vector3(-1, 0, 0);
	o1.radius = Vector3(1, 1, 1);
	objects.push_back(o1);

	Object o2;
	o2.matID = 2;
	o2.type = 0;
	o2.centre = Vector3(1, 0, 0);
	o2.radius = Vector3(1, 1, 1);
	objects.push_back(o2);

	Object o3;
	o3.matID = 3;
	o3.type = 1;
	o3.centre = Vector3(-4, 1, 0);
	o3.radius = Vector3(0.05, 2, 2);
	objects.push_back(o3);

	Object o4;
	o4.matID = 4;
	o4.type = 0;
	o4.centre = Vector3(8, 8, -4);
	o4.radius = Vector3(4, 4, 4);
	objects.push_back(o4);
	*/
}

int nextPowerOfTwo(int x) 
{
	x--;
	x |= x >> 1; // handle 2 bit numbers
	x |= x >> 2; // handle 4 bit numbers
	x |= x >> 4; // handle 8 bit numbers
	x |= x >> 8; // handle 16 bit numbers
	x |= x >> 16; // handle 32 bit numbers
	x++;
	return x;
}

void Graphics::Render(GLfloat currentTime, Vector2 min, Vector2 max, GLuint passes, GLuint currentPass, Vector2 chunkSize)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Compute
	glUseProgram(rayTrace.program);

	glUniform1i(glGetUniformLocation(rayTrace.program, "passes"), passes);

	glUniform4f(glGetUniformLocation(rayTrace.program, "bounds"), min.x, min.y, max.x, max.y);

	glUniform1f(glGetUniformLocation(rayTrace.program, "time"), currentTime);

	glBindImageTexture(0, framebuffer.color, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);

	glDispatchCompute(nextPowerOfTwo(ceil(min.x / 8 + max.x / 8)), nextPowerOfTwo(ceil(min.y / 8 + max.y / 8)), 1);

	glEnable(GL_FRAMEBUFFER_SRGB);

	// Draw
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(fullQuad.program);

	glUniform1i(glGetUniformLocation(fullQuad.program, "tex"), 0);

	glUniform2f(glGetUniformLocation(fullQuad.program, "screenSize"), Screen::getScreenSize().x, Screen::getScreenSize().y);

	glUniform4f(glGetUniformLocation(fullQuad.program, "bounds"), min.x, min.y, max.x, max.y);
	glUniform2f(glGetUniformLocation(fullQuad.program, "chunkSize"), chunkSize.x, chunkSize.y);

	glUniform1i(glGetUniformLocation(fullQuad.program, "passes"), passes);
	glUniform1i(glGetUniformLocation(fullQuad.program, "currentPass"), currentPass);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, framebuffer.color);

	glBindVertexArray(fqVAO);
	glDrawArrays(GL_QUADS, 0, 4);

	glDisable(GL_FRAMEBUFFER_SRGB);
}

void Graphics::Reload()
{
	matLines.clear();
	for (int i = 0; i < materials.size(); i++)
	{
		Material m = materials[i];

		std::string line;

		line += std::string("Material m") + std::to_string(m.id) + " = Material(";

		line += "vec3(" + std::to_string(m.color.x) + ", " + std::to_string(m.color.y) + ", " + std::to_string(m.color.z) + "), ";

		line += m.reflective ? "true, " : "false, ";
		line += m.transmissive ? "true, " : "false, ";
		line += m.emissive ? "true, " : "false, ";

		line += std::to_string(m.power) + ", ";

		line += std::to_string(m.rRoughness) + ", ";
		line += std::to_string(m.tRoughness) + ", ";

		line += std::to_string(m.ior) + ", ";

		line += std::to_string(m.mixFact);

		line += ");";
		
		matLines.push_back(line);
	}

	objLines.clear();
	for (int i = 0; i < objects.size(); i++)
	{
		Object o = objects[i];

		std::string line;

		line += "d = opU(d, MapData(m" + std::to_string(o.matID) + ", ";

		if (o.type == 0)
		{
			line += "mapSphere(p, ";
			line += "vec3(" + std::to_string(o.centre.x) + ", " + std::to_string(o.centre.y) + ", " + std::to_string(o.centre.z) + "), ";
			line += std::to_string(o.radius.x);
		}
		else if (o.type == 1)
		{
			line += "mapBox(p, ";
			line += "vec3(" + std::to_string(o.centre.x) + ", " + std::to_string(o.centre.y) + ", " + std::to_string(o.centre.z) + "), ";
			line += "vec3(" + std::to_string(o.radius.x) + ", " + std::to_string(o.radius.y) + ", " + std::to_string(o.radius.z) + ")";
		}

		line += ")));";

		objLines.push_back(line);
	}
	
	///*
	matLines.push_back("Material m0 = Material(vec3(0.8, 0.8, 0.8), false, false, false, 0, 0.6, 1, 0, 1);");
	matLines.push_back("Material m1 = Material(vec3(1.0, 0.1, 0.1), false, false, false, 0, 0.1, 1, 0, 1);");
	matLines.push_back("Material m2 = Material(vec3(0.1, 0.1, 1.0), false, false, false, 0, 0.05, 1, 0, 0.2);");
	matLines.push_back("Material m3 = Material(vec3(0.1, 1.0, 0.1), false, false, false, 0, 1, 1, 0, 1);");
	matLines.push_back("Material m4 = Material(vec3(1.0, 1.0, 1.0), false, false, true, 512, 1, 1, 0, 1);");

	objLines.push_back("d = opU(d, MapData(m0, mapBox(p, vec3(0, -1.025, 0), vec3(32, 0.05, 32))));");
	objLines.push_back("d = opU(d, MapData(m1, mapSphere(p, vec3(-1, 0, 0), 1)));");
	objLines.push_back("d = opU(d, MapData(m2, mapSphere(p, vec3(1, 0, 0), 1)));");
	objLines.push_back("d = opU(d, MapData(m3, mapBox(p, vec3(-4, 1, 0), vec3(0.05, 2, 2))));");
	objLines.push_back("d = opU(d, MapData(m4, mapSphere(p, vec3(8, 8, -4), 4)));");
	//*/

	rayTrace.DeleteCompute();
	rayTrace.CreateCompute("RayMarch");

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.buffer);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void Graphics::SaveImage(std::string path)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	std::vector<unsigned char> data(Screen::getScreenSize().x * Screen::getScreenSize().y * 4);
	glReadPixels(0, 0, Screen::getScreenSize().x, Screen::getScreenSize().y, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);

	int r = SOIL_save_image
	(
		path.c_str(),
		SOIL_SAVE_TYPE_BMP,
		Screen::getScreenSize().x,
		Screen::getScreenSize().y,
		4,
		data.data()
	);
}

void Graphics::addMaterial(Material material)
{
	materials.push_back(material);
}

void Graphics::addObject(Object object)
{
	objects.push_back(object);
}

void Graphics::setView(Vector3 eye, Vector3 ray00, Vector3 ray01, Vector3 ray10, Vector3 ray11)
{
	glUseProgram(rayTrace.program);
	glUniform3f(glGetUniformLocation(rayTrace.program, "eye"), eye.x, eye.y, eye.z);
	glUniform3f(glGetUniformLocation(rayTrace.program, "ray00"), ray00.x, ray00.y, ray00.z);
	glUniform3f(glGetUniformLocation(rayTrace.program, "ray10"), ray10.x, ray10.y, ray10.z);
	glUniform3f(glGetUniformLocation(rayTrace.program, "ray01"), ray01.x, ray01.y, ray01.z);
	glUniform3f(glGetUniformLocation(rayTrace.program, "ray11"), ray11.x, ray11.y, ray11.z);
}