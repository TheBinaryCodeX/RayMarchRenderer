#version 430 core

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 uv;

out vec2 Pos;
out vec2 UV;

uniform vec2 screenSize;

void main()
{
	Pos = pos;
	UV = uv;

	vec2 pos2 = pos;
	pos2.y = screenSize.y - pos.y;
	gl_Position = vec4((pos2 - screenSize / 2) / (screenSize / 2), 0, 1);
}