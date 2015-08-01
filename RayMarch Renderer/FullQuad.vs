#version 430 core

in vec2 pos;
in vec2 uv;

out vec2 UV;

void main()
{
	UV = uv;

	gl_Position = vec4(pos, 0, 1);
}