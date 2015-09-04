#include "stdafx.h"
#include "Graphics.h"
#include "SOIL.h"

Vector2 imageSize = Vector2(800, 600);

std::vector<Json::Value> materials;
std::vector<Json::Value> objects;

std::vector<std::string> matLines;
std::vector<std::string> objLines;

GLuint Graphics::loadShader(GLenum type, std::string path)
{
	clock_t t = clock();

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
		else if (line.find("//#MATFUNCINSERT") != std::string::npos)
		{
			lines.erase(lines.begin() + i);
			lines.insert(lines.begin() + i, matLines.begin(), matLines.end());
		}
		else if (line.find("//#CASEINSERT") != std::string::npos)
		{
			lines.erase(lines.begin() + i);

			int matNum = 0;
			for (int j = 0; j < matLines.size(); j++)
			{
				if (matLines[j] == "}")
				{
					matNum++;
				}
			}

			for (int j = matNum - 1; j >= 0; j--)
			{
				lines.insert(lines.begin() + i, std::string("				break;"));
				lines.insert(lines.begin() + i, std::string("				mat_func_") + std::to_string(j) + "(ray, newColor, newDir);");
				lines.insert(lines.begin() + i, std::string("			case ") + std::to_string(j) + ":");
			}
		}
		else if (line.find("//#OBJFUNCINSERT") != std::string::npos)
		{
			lines.erase(lines.begin() + i);
			lines.insert(lines.begin() + i, objLines.begin(), objLines.end());
		}
		else if (line.find("//#OBJINSERT") != std::string::npos)
		{
			lines.erase(lines.begin() + i);

			int objNum = 0;
			for (int j = 0; j < objLines.size(); j++)
			{
				if (objLines[j] == "}")
				{
					objNum++;
				}
			}

			for (int j = objNum - 1; j >= 0; j--)
			{
				lines.insert(lines.begin() + i, std::string("	d = opU(d, vec2(d") + std::to_string(j) + ".x, " + std::to_string(objects[j]["matID"].asInt()) + "));");
				lines.insert(lines.begin() + i, std::string("	obj_func_") + std::to_string(j) + "(p, " + "d" + std::to_string(j) + ");");
				lines.insert(lines.begin() + i, std::string("	vec3 d") + std::to_string(j) + ";");
			}
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

	/*
	for (int i = 0; i < lines.size(); i++)
	{
		std::cout << std::to_string(i + 1) << " " << lines[i] << std::endl;
	}
	*/

	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	float time = (double)(clock() - t) / (double)CLOCKS_PER_SEC;
	//std::cout << "Compile Time: " << time << std::endl;

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

GLuint envTex;

GLuint fqVAO;
void createFQ(Vector2 centre, GLfloat zoom)
{
	float halfWidth = (imageSize.x / 2) * zoom;
	float halfHeight = (imageSize.y / 2) * zoom;

	std::vector<GLfloat> vertexData = std::vector<GLfloat>
	{
		(float)centre.x - halfWidth, (float)centre.y - halfHeight, 0.0f, 0.0f,
		(float)centre.x + halfWidth, (float)centre.y - halfHeight, 1.0f, 0.0f,
		(float)centre.x + halfWidth, (float)centre.y + halfHeight, 1.0f, 1.0f,
		(float)centre.x - halfWidth, (float)centre.y + halfHeight, 0.0f, 1.0f
	};

	glGenVertexArrays(1, &fqVAO);
	glBindVertexArray(fqVAO);

	GLuint fqVBO;
	glGenBuffers(1, &fqVBO);
	glBindBuffer(GL_ARRAY_BUFFER, fqVBO);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(GLfloat), &vertexData[0], GL_STATIC_DRAW);

	GLint posAttrib = 0;// glGetAttribLocation(fullQuad.program, "pos");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

	GLint uvAttrib = 1;// glGetAttribLocation(fullQuad.program, "uv");
	glEnableVertexAttribArray(uvAttrib);
	glVertexAttribPointer(uvAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(float)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Graphics::Init()
{
	glewExperimental = GL_TRUE;
	glewInit();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	fullQuad.Create("FullQuad");
	rayTrace.CreateCompute("RayMarch");

	framebuffer.Create(imageSize);

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.buffer);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	envTex = SOIL_load_OGL_texture("data\\textures\\skybox.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, 0);
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

void Graphics::Render(GLfloat currentTime, Vector2 min, Vector2 max, GLuint currentSample)
{
	// Compute
	glUseProgram(rayTrace.program);

	glUniform1i(glGetUniformLocation(rayTrace.program, "currentSample"), currentSample);

	glUniform4f(glGetUniformLocation(rayTrace.program, "bounds"), min.x, min.y, max.x, max.y);

	glUniform1f(glGetUniformLocation(rayTrace.program, "time"), currentTime);

	glUniform1i(glGetUniformLocation(rayTrace.program, "useEnvTex"), 1);
	glUniform1i(glGetUniformLocation(rayTrace.program, "envTex"), 0); 

	glUniform1i(glGetUniformLocation(rayTrace.program, "separateChannels"), 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, envTex);

	glBindImageTexture(0, framebuffer.color, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);

	glDispatchCompute(nextPowerOfTwo(ceil(min.x / 8 + max.x / 8)), nextPowerOfTwo(ceil(min.y / 8 + max.y / 8)), 1);

	glBindTexture(GL_TEXTURE_2D, 0);

	glUseProgram(0);
}

void Graphics::Display(Vector2 centre, GLfloat zoom, Vector2 min, Vector2 max)
{
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	//glClear(GL_COLOR_BUFFER_BIT);

	// Draw
	glEnable(GL_FRAMEBUFFER_SRGB);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(fullQuad.program);

	glUniform1i(glGetUniformLocation(fullQuad.program, "tex"), 0);

	glUniform2f(glGetUniformLocation(fullQuad.program, "screenSize"), Screen::getScreenSize().x, Screen::getScreenSize().y);

	glUniform4f(glGetUniformLocation(fullQuad.program, "bounds"), min.x, min.y, max.x, max.y);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, framebuffer.color);

	createFQ(centre, zoom);

	glBindVertexArray(fqVAO);
	glDrawArrays(GL_QUADS, 0, 4);

	glBindVertexArray(0);
	glDeleteVertexArrays(1, &fqVAO);

	glBindTexture(GL_TEXTURE_2D, 0);

	glUseProgram(0);

	sf::Texture::bind(0);

	glDisable(GL_FRAMEBUFFER_SRGB);
}

void Graphics::Reload()
{
	matLines.clear();
	for (int i = 0; i < materials.size(); i++)
	{
		Json::Value mat = materials[i];

		matLines.push_back(std::string("void mat_func_") + std::to_string(mat["id"].asInt()) + "(inout RayData ray, out vec3 outColor, out vec3 outDir)");
		matLines.push_back("{");

		matLines.push_back(std::string("vec3 vars[") + mat["total_vars"].asString() + "];");

		for (int j = 0; j < mat["nodes"].size(); j++)
		{
			std::string func;

			Json::Value node = mat["nodes"][j];

			func += node["name"].asString() + "(ray, ";

			for (int k = 0; k < node["inputs"].size(); k++)
			{
				if (node["inputs"][k].isArray())
				{
					Json::Value input = node["inputs"][k];
					func += "vec3(" + std::to_string(input[0].asFloat()) + ", " + std::to_string(input[1].asFloat()) + ", " + std::to_string(input[2].asFloat()) + "), ";
				}
				else
				{
					func += "vars[" + std::to_string(node["inputs"][k].asInt()) + "], ";
				}
			}

			for (int k = 0; k < node["outputs"].size(); k++)
			{
				func += "vars[" + std::to_string(node["outputs"][k].asInt()) + "]";
				if (k < node["outputs"].size() - 1)
				{
					func += ", ";
				}
			}

			func += ");";

			matLines.push_back(func);
		}

		if (mat["color"].asInt() != -1)
		{
			matLines.push_back(std::string("outColor = vars[") + std::to_string(mat["color"].asInt()) + "];");
		}

		if (mat["dir"].asInt() != -1)
		{
			matLines.push_back(std::string("outDir = vars[") + std::to_string(mat["dir"].asInt()) + "];");
		}

		matLines.push_back("}");
	}

	objLines.clear();
	for (int i = 0; i < objects.size(); i++)
	{
		Json::Value obj = objects[i];

		objLines.push_back(std::string("void obj_func_") + std::to_string(i) + "(in vec3 p, out vec3 d)");
		objLines.push_back("{");

		objLines.push_back(std::string("vec3 vars[") + obj["total_vars"].asString() + "];");

		for (int j = 0; j < obj["nodes"].size(); j++)
		{
			std::string func;

			Json::Value node = obj["nodes"][j];

			func += node["name"].asString() + "(";

			for (int k = 0; k < node["inputs"].size(); k++)
			{
				if (node["inputs"][k].isArray())
				{
					Json::Value input = node["inputs"][k];
					func += "vec3(" + std::to_string(input[0].asFloat()) + ", " + std::to_string(input[1].asFloat()) + ", " + std::to_string(input[2].asFloat()) + "), ";
				}
				else
				{
					if (node["inputs"][k].asInt() == -1)
					{
						func += "p, ";
					}
					else
					{
						func += "vars[" + std::to_string(node["inputs"][k].asInt()) + "], ";
					}
				}
			}

			for (int k = 0; k < node["outputs"].size(); k++)
			{
				func += "vars[" + std::to_string(node["outputs"][k].asInt()) + "]";
				if (k < node["outputs"].size() - 1)
				{
					func += ", ";
				}
			}

			func += ");";

			objLines.push_back(func);
		}

		objLines.push_back(std::string("d = vars[") + std::to_string(obj["distance"].asInt()) + "];");

		objLines.push_back("}");
	}

	rayTrace.DeleteCompute();
	rayTrace.CreateCompute("RayMarch");

	framebuffer.Delete();
	framebuffer.Create(imageSize);

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.buffer);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

void Graphics::addMaterial(Json::Value material)
{
	materials.push_back(material);
}

void Graphics::addObject(Json::Value object)
{
	objects.push_back(object);
}

void Graphics::clearScene()
{
	materials.clear();
	objects.clear();
}

void Graphics::setImageSize(Vector2 size)
{
	imageSize = size;
}

Vector2 Graphics::getImageSize()
{
	return imageSize;
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