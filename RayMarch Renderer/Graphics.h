#pragma once
#include "Screen.h"
#include "json\json.h"
#include "SFML\Window.hpp"
#include "SFML/Graphics.hpp"
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

		void Create(Vector2 size)
		{
			glGenFramebuffers(1, &buffer);
			glBindFramebuffer(GL_FRAMEBUFFER, buffer);

			glGenTextures(1, &color);
			glBindTexture(GL_TEXTURE_2D, color);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
		}

		void Delete()
		{
			glDeleteTextures(1, &color);
			glDeleteFramebuffers(1, &buffer);
		}
	};

	struct Material
	{
		GLuint id;
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
	static void Render(GLfloat currentTime, Vector2 min, Vector2 max, GLuint currentSample);
	static void Display(Vector2 centre, GLfloat zoom, Vector2 min, Vector2 max);
	static void Reload();

	static void SaveImage(std::string path);

	static void addMaterial(Json::Value material);
	static void addObject(Json::Value object);
	static void clearScene();

	static void setImageSize(Vector2 size);

	static void setView(Vector3 eye, Vector3 ray00, Vector3 ray01, Vector3 ray10, Vector3 ray11);
};

