#pragma once
#include "Screen.h"
#include <GL/glew.h>
#include <glm.hpp>
#include <GL/GL.h>
#include <fstream>
#include <string>
#include <vector>
#include "Vector.h"
using namespace Vector;
class Graphics
{
public:
	static GLuint loadShader(GLenum type, std::string path);
	static GLuint createProgram(GLuint vShader, GLuint fShader)
	{
		GLuint newProgram = glCreateProgram();
		glAttachShader(newProgram, vShader);
		glAttachShader(newProgram, fShader);
		glBindFragDataLocation(newProgram, 0, "outColor");
		glLinkProgram(newProgram);
		glUseProgram(newProgram);
		return newProgram;
	}

	struct Shader
	{
		GLuint vertex;
		GLuint fragment;
		GLuint compute;
		GLuint program;

		void Create(std::string fileName)
		{
			vertex = loadShader(GL_VERTEX_SHADER, (fileName + ".vs").c_str());
			fragment = loadShader(GL_FRAGMENT_SHADER, (fileName + ".fs").c_str());
			program = createProgram(vertex, fragment);
		}

		void Create(std::string vertName, std::string fragName)
		{
			vertex = loadShader(GL_VERTEX_SHADER, (vertName + ".vs").c_str());
			fragment = loadShader(GL_FRAGMENT_SHADER, (fragName + ".fs").c_str());
			program = createProgram(vertex, fragment);
		}

		void CreateCompute(std::string fileName)
		{
			compute = loadShader(GL_COMPUTE_SHADER, (fileName + ".glsl").c_str());

			program = glCreateProgram();
			glAttachShader(program, compute);
			glLinkProgram(program);
			glUseProgram(program);
		}

		void Delete()
		{
			glDeleteShader(vertex);
			glDeleteShader(fragment);
			glDeleteProgram(program);
		}

		void DeleteCompute()
		{
			glDeleteShader(compute);
			glDeleteProgram(program);
		}
	};

	struct Framebuffer
	{
		GLuint buffer;

		GLuint color;

		void Create()
		{
			glGenFramebuffers(1, &buffer);
			glBindFramebuffer(GL_FRAMEBUFFER, buffer);

			glGenTextures(1, &color);
			glBindTexture(GL_TEXTURE_2D, color);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, Screen::getScreenSize().x, Screen::getScreenSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
		}

		void CreateLM()
		{
			glGenFramebuffers(1, &buffer);
			glBindFramebuffer(GL_FRAMEBUFFER, buffer);

			glGenTextures(1, &color);
			glBindTexture(GL_TEXTURE_2D, color);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
		}
	};

	struct Material
	{
		GLuint id;

		Vector3 color;

		GLboolean reflective;
		GLboolean transmissive;
		GLboolean emissive;

		GLfloat power;

		GLfloat rRoughness;
		GLfloat tRoughness;

		GLfloat ior;

		GLfloat mixFact;
	};

	struct Object
	{
		GLuint matID;
		GLuint type;

		Vector3 centre;
		Vector3 radius;
		Vector3 normal;
	};

public:
	static void Init();
	static void Render(GLfloat currentTime, Vector2 min, Vector2 max, GLuint passes, GLuint currentPass, Vector2 chunkSize);
	static void Reload();

	static void SaveImage(std::string path);

	static void addMaterial(Material material);
	static void addObject(Object object);

	static void setView(Vector3 eye, Vector3 ray00, Vector3 ray01, Vector3 ray10, Vector3 ray11);
};

