#version 430 core

in vec2 UV;

out vec4 outColor;

uniform sampler2D tex;

uniform vec2 screenSize;

uniform vec4 bounds;

uniform int currentSample;

uniform int drawBox;

void main()
{
	vec4 color = texture2D(tex, UV);
	color.a = 1;

	vec2 pix = UV * screenSize;

	if (drawBox != 0)
	{
		if (pix.x - bounds.x <= 0.5 && pix.x - bounds.x >= -0.5 && pix.y >= bounds.y && pix.y <= bounds.w && (length(pix - bounds.xy) <= 8 || length(pix - bounds.xw) <= 8))
		{
			color = vec4(1, 0.6, 0.1, 0.8);
		}

		if (pix.x - bounds.z <= 0.5 && pix.x - bounds.z >= -0.5 && pix.y >= bounds.y && pix.y <= bounds.w && (length(pix - bounds.zy) <= 8 || length(pix - bounds.zw) <= 8))
		{
			color = vec4(1, 0.6, 0.1, 0.8);
		}

		if (pix.y - bounds.y <= 0.5 && pix.y - bounds.y >= -0.5 && pix.x >= bounds.x && pix.x <= bounds.z && (length(pix - bounds.xy) <= 8 || length(pix - bounds.zy) <= 8))
		{
			color = vec4(1, 0.6, 0.1, 0.8);
		}

		if (pix.y - bounds.w <= 0.5 && pix.y - bounds.w >= -0.5 && pix.x >= bounds.x && pix.x <= bounds.z && (length(pix - bounds.xw) <= 8 || length(pix - bounds.zw) <= 8))
		{
			color = vec4(1, 0.6, 0.1, 0.8);
		}
	}

	outColor = color;
}