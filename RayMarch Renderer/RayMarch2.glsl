#version 430 core

const float PI = 3.1415926535897932384626433832795;

layout(binding = 0, rgba32f) uniform image2D framebuffer;

uniform vec3 eye;
uniform vec3 ray00;
uniform vec3 ray10;
uniform vec3 ray01;
uniform vec3 ray11;

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

uniform float maxDist;
uniform int maxSteps;
uniform int maxBounces;
uniform float stepMultiply;

uniform int currentSample;

uniform vec4 bounds;

uniform float time;

uniform sampler2D envTex;
uniform int useEnvTex;

uniform int separateChannels;
vec3 channels;

struct RayData
{
	vec3 origin;
	vec3 hit;
	vec3 dir;
	float t;
	bool inside;
};

float randChange = 0;
highp float rand(vec2 co)
{
	co += (gl_GlobalInvocationID.xy + vec2(time)) * randChange;

	highp float a = 12.9898;
	highp float b = 78.233;
	highp float c = 43758.5453;
	highp float dt = dot(co.xy, vec2(a, b));
	highp float sn = mod(dt, 3.14);

	randChange = fract(sin(sn) * c);

	return randChange;
}

mat3 makeViewMat(vec3 dir)
{
	vec3 locZ = dir;

	vec3 locX;
	if (locZ == vec3(0, 1, 0))
	{
		locX = normalize(cross(locZ, vec3(0, 0, 1)));
	}
	else
	{
		locX = normalize(cross(locZ, vec3(0, 1, 0)));
	}

	vec3 locY = normalize(cross(locZ, locX));

	return mat3(locX, locY, locZ);
}

vec3 skyColor(vec3 dir)
{
	//return vec3(1);
	if (useEnvTex != 0)
	{
		float PI = 3.141592653;

		float theta = dir.y;
		float phi = atan(dir.z, dir.x);

		if (phi < 0)
		{
			phi += 2 * PI;
		}

		vec2 uv;
		uv.x = phi / (2 * PI);
		uv.y = 1.0 - (dir.y * 0.5 + 0.5);

		return texture2D(envTex, uv).rgb;
	}

	return vec3(0.015);
}

float smin(float a, float b, float k)
{
	float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
	return mix(b, a, h) - k * h * (1.0 - h);
}

// Map Functions
float map_sphere(vec3 p, vec3 centre, float radius)
{
	vec3 q = p - centre;
	return length(q) - radius;
}

float map_box(vec3 p, vec3 centre, vec3 radius)
{
	vec3 q = abs(p - centre) - radius;
	return min(max(q.x, max(q.y, q.z)), 0) + length(max(q, 0));
}

vec2 opU(vec2 a, vec2 b)
{
	return a.x < b.x ? a : b;
}

vec2 map(vec3 p)
{
	vec2 d = vec2(maxDist, -1);

	d = opU(d, vec2(map_box(p, vec3(0, -0.025, 0), vec3(8, 0.05, 8)), 0));
	d = opU(d, vec2(map_sphere(p, vec3(-1, 1, 0), 1), 1));
	d = opU(d, vec2(map_box(p, vec3(1, 1, 0), vec3(1)), 2));

	return d;
}

vec2 march(vec3 origin, vec3 dir, float distMult)
{
	float t = 0;

	for (int i = 0; i < maxSteps; i++)
	{
		vec2 d = map(origin + t * dir);
		d.x *= distMult;

		if (d.x < 0.001)
		{
			d.x = t;
			return d;
		}

		if (t >= maxDist)
		{
			return vec2(maxDist, -1);
		}

		t += d.x * stepMultiply;
	}

	return vec2(maxDist, -1);
}

vec3 getNormal(vec3 p)
{
	vec3 n;

	n.x = map(p + vec3(0.001, 0, 0)).x - map(p - vec3(0.001, 0, 0)).x;
	n.y = map(p + vec3(0, 0.001, 0)).x - map(p - vec3(0, 0.001, 0)).x;
	n.z = map(p + vec3(0, 0, 0.001)).x - map(p - vec3(0, 0, 0.001)).x;

	return normalize(n);
}

vec3 randHemisphere(vec2 randSeed1, vec2 randSeed2, vec3 normal)
{
	float theta = 2 * 3.141592653 * rand(randSeed1);
	float phi = asin(sqrt(rand(randSeed1)));
	//phi = acos(2 * rand(randSeed2) - 1);

	vec3 bDir = normalize(vec3(sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta)));

	if (normal != vec3(0, 0, 0))
	{
		if (dot(bDir, vec3(0, 0, 1)) < 0)
		{
			bDir *= -1;
		}

		vec3 locZ = normal;

		vec3 locX;
		if (locZ == vec3(0, 1, 0))
		{
			locX = normalize(cross(locZ, vec3(0, 0, 1)));
		}
		else
		{
			locX = normalize(cross(locZ, vec3(0, 1, 0)));
		}

		vec3 locY = normalize(cross(locZ, locX));

		mat3 m = mat3(locX, locY, locZ);

		bDir = m * bDir;
	}

	return bDir;
}

float grayscale(vec3 color)
{
	return (color.r + color.g + color.b) / 3.0;// (channels.r + channels.g + channels.b);
}

struct DiffuseMaterial
{
	vec3 albedo;

	vec3 brdf(vec3 wi, vec3 wo)
	{
		return albedo / PI;
	}

	vec3 samplePDF(vec3 wo)
	{
		float theta = 2 * 3.141592653 * rand(albedo.rg);
		float phi = asin(sqrt(rand(albedo.bg)));

		return normalize(vec3(sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta)));
	}

	vec3 weightPDF(vec3 wo)
	{
		return albedo;
	}
};

vec3 brdf(vec3 wi, vec3 wo, int matID)
{
	switch (matID)
	{
	case 0:
		return vec3(0.8, 0.8, 0.8) / PI;
	case 1:
		return vec3(0.8, 0.2, 0.2) / PI;
	case 2:
		return vec3(0.2, 0.2, 0.8) / PI;
	}
}

vec3 refl(vec3 wo, int matID)
{
	switch (matID)
	{
	case 0:
		return vec3(0.8, 0.8, 0.8);
	case 1:
		return vec3(0.8, 0.2, 0.2);
	case 2:
		return vec3(0.2, 0.2, 0.8);
	}
}

vec3 trace(vec3 origin, vec3 dir)
{
	vec3 endColors[64];
	int nextOpen = 0;

	vec3 color = vec3(1);

	vec3 o = origin;
	vec3 d = dir;

	int bounces = 0;
	while (bounces < maxBounces)
	{
		bounces++;

		vec2 v = march(o, d, 1);

		if (v.x < maxDist)
		{
			vec3 point = o + v.x * d;
			vec3 normal = getNormal(point);

			vec3 lightPos = vec3(4, 6, -2);
			float lightPower = 200.0;

			vec3 lightDir = normalize(lightPos - point);

			float sd = march(point + normal * 0.002, lightDir, 1).x;
			if (sd >= length(lightPos - point))
			{
				endColors[nextOpen] = color * brdf(d, lightDir, int(v.y)) * clamp(dot(lightDir,  normal), 0.0, 1.0) * (lightPower / pow(length(lightPos - point), 2.0));
				nextOpen++;
			}

			DiffuseMaterial mat;
			switch (int(v.y))
			{
			case 0:
				mat.albedo = vec3(0.8, 0.8, 0.8);
				break;
			case 1:
				mat.albedo = vec3(0.8, 0.2, 0.2);
				break;
			case 2:
				mat.albedo = vec3(0.2, 0.2, 0.8);
				break;
			}

			mat3 normMat = makeViewMat(normal);
			vec3 newDir = mat.samplePDF(d);
			if (dot(newDir, vec3(0, 0, 1)) < 0)
			{
				newDir *= -1;
			}
			newDir = normMat * newDir;

			vec3 reflectance = mat.weightPDF(d);
			float probability = max(reflectance.r, max(reflectance.g, reflectance.b));
			if (rand(point.zx) <= probability)
			{
				color *= reflectance / probability;
			}
			else
			{
				color = vec3(0);
				break;
			}

			o = point + normal * 0.002;
			d = newDir;
		}
		else
		{
			color *= skyColor(d);
			break;
		}
	}

	for (int i = 0; i < nextOpen; i++)
	{
		color += endColors[i];
	}

	return color;
}

void main()
{
	ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(framebuffer);

	if (pix.x < bounds.x || pix.y < bounds.y || pix.x >= bounds.z || pix.y >= bounds.w)
	{
		return;
	}

	vec2 pos = vec2(pix) / vec2(size.x, size.y);
	vec3 dir = mix(mix(ray00, ray01, pos.x + rand(pix.xy + vec2(time)) / size.x), mix(ray10, ray11, pos.x + rand(pix.xy + vec2(time)) / size.x), pos.y + rand(pix.yx + vec2(time)) / size.y);

	vec3 color;
	if (separateChannels == 0)
	{
		channels = vec3(1, 1, 1);
		color = trace(eye, normalize(dir));
	}
	else
	{
		channels = vec3(1, 0, 0);
		vec3 r = trace(eye, normalize(dir));

		channels = vec3(0, 1, 0);
		vec3 g = trace(eye, normalize(dir));

		channels = vec3(0, 0, 1);
		vec3 b = trace(eye, normalize(dir));

		color = r + g + b;
	}

	if (currentSample != 0)
	{
		vec4 old = imageLoad(framebuffer, pix);

		float f1 = 1.0 / float(currentSample + 1);
		float f2 = float(currentSample) / float(currentSample + 1);

		imageStore(framebuffer, pix, vec4(color * f1 + old.rgb * f2, 1));
	}
	else
	{
		imageStore(framebuffer, pix, vec4(color, 1));
	}
}