#version 430 core

const float PI = 3.1415926535897932384626433832795;

layout(binding = 0, rgba32f) uniform image2D framebuffer;
layout(binding = 1, rgba32f) uniform image2D materialData;

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

struct PointData
{
	vec3 pos;
	vec3 dir;
	vec3 normal;
	mat3 tbn;
};

struct ColorRange
{
	uint minWave;
	uint maxWave;
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

	//return vec3(0.985);
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

	d = opU(d, vec2(map_box(p, vec3(0, -0.025, 0), vec3(32, 0.05, 32)), 1));

	d = opU(d, vec2(map_sphere(p, vec3(0, 1, 0), 1), 2));

	d = opU(d, vec2(map_sphere(p, vec3(6, 8, -4), 4), 0));

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

mat3 makeTBN(vec3 p)
{
	vec3 normal = getNormal(p);
	vec3 tangent;
	vec3 bitangent;

	if (normal.x == 0)
	{
		tangent = vec3(1, 0, 0);
	}
	else
	{
		tangent = normalize(cross(vec3(0, 1, 0), normal));
	}

	bitangent = normalize(cross(tangent, normal));

	return mat3(bitangent, normal, tangent);
}

vec3 randHemisphere(vec2 randSeed1, vec2 randSeed2, vec3 normal)
{
	float theta = 2 * 3.141592653 * rand(randSeed1);
	float phi = acos(2 * rand(randSeed2) - 1);

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

ColorRange multColor(ColorRange a, ColorRange b)
{
	ColorRange r;
	r.minWave = max(a.minWave, b.minWave);
	r.maxWave = min(a.maxWave, b.maxWave);
	return r;
}

void mat_func_0(in vec2 randSeed, inout uint color, inout float power, out bool willBreak)
{
	willBreak = false;

	ColorRange matColor;
	matColor.minWave = 380;
	matColor.maxWave = 780;

	float matPower = 8;

	if (color == 0)
	{
		float r = rand(randSeed);
		r *= (matColor.maxWave - matColor.minWave) / 5;
		r = floor(r) * 5;
		color = int(r) + matColor.minWave;
		power *= matPower;
	}
	else
	{
		if (color < matColor.minWave || color > matColor.maxWave)
		{
			color = 0;
			willBreak = true;
		}
		else
		{
			power *= matPower;
		}
	}
}

void mat_func_1(in vec2 randSeed, inout uint color, inout float power, out bool willBreak)
{
	willBreak = false;

	ColorRange matColor;
	matColor.minWave = 380;
	matColor.maxWave = 780;

	float matPower = 0.8;

	if (color == 0)
	{
		float r = rand(randSeed);
		r *= (matColor.maxWave - matColor.minWave) / 5;
		r = floor(r) * 5;
		color = int(r) + matColor.minWave;
		power *= matPower;
	}
	else
	{
		if (color < matColor.minWave || color > matColor.maxWave)
		{
			color = 0;
			willBreak = true;
		}
		else
		{
			power *= matPower;
		}
	}
}

void mat_func_2(in vec2 randSeed, inout uint color, inout float power, out bool willBreak)
{
	willBreak = false;

	ColorRange matColor;
	matColor.minWave = 490;
	matColor.maxWave = 590;

	float matPower = 0.8;

	if (color == 0)
	{
		float r = rand(randSeed);
		r *= (matColor.maxWave - matColor.minWave) / 5;
		r = floor(r) * 5;
		color = int(r) + matColor.minWave;
		power *= matPower;
	}
	else
	{
		if (color < matColor.minWave || color > matColor.maxWave)
		{
			color = 0;
			willBreak = true;
		}
		else
		{
			power *= matPower;
		}
	}
}

uint trace(vec3 origin, vec3 dir, out float lightPower)
{
	uint color = 0;
	float power = 1.0;

	vec3 o = origin;
	vec3 d = dir;

	int bounces = 0;
	while (bounces < maxBounces)
	{
		bounces++;

		vec2 v = march(o, d, 1);

		PointData point;
		point.pos = o + v.x * d;
		point.dir = -d;
		point.normal = getNormal(point.pos);
		point.tbn = makeTBN(point.pos);

		if (v.x < maxDist)
		{
			vec3 newDir;
			if (int(v.y) == 0)
			{
				bool willBreak = false;
				mat_func_0(point.pos.yx, color, power, willBreak);
				if (willBreak)
				{
					break;
				}

				break;
			}
			else if (int(v.y) == 1)
			{
				bool willBreak = false;
				mat_func_1(point.pos.yx, color, power, willBreak);
				if (willBreak)
				{
					break;
				}

				newDir = randHemisphere(point.pos.xy, point.pos.zy, point.normal);
			}
			else if (int(v.y) == 2)
			{
				bool willBreak = false;
				mat_func_2(point.pos.yx, color, power, willBreak);
				if (willBreak)
				{
					break;
				}

				newDir = randHemisphere(point.pos.xy, point.pos.zy, point.normal);
			}

			o = point.pos + point.normal * 0.002;
			d = newDir;
		}
		else
		{
			ColorRange matColor;
			matColor.minWave = 390;
			matColor.maxWave = 830;

			float matPower = 0.015;

			if (color == 0)
			{
				float r = rand(point.pos.yx);
				r *= (matColor.maxWave - matColor.minWave) / 5;
				r = floor(r) * 5;
				color = int(r) + matColor.minWave;
				power *= matPower;
			}
			else
			{
				if (color < matColor.minWave || color > matColor.maxWave)
				{
					color = 0;
					break;
				}
				else
				{
					power *= matPower;
				}
			}

			break;
		}
	}

	lightPower = power;

	return color;
}

// This function is directly copied from: http://scienceprimer.com/javascript-code-convert-light-wavelength-color
vec3 wavelengthToColor(uint wavelength) 
{
	float r,
		g,
		b,
		alpha,
		wl = wavelength,
		gamma = 1,
		R,
		G,
		B;


	if (wl >= 380 && wl < 440) 
	{
		R = -1 * (wl - 440) / (440 - 380);
		G = 0;
		B = 1;
	}
	else if (wl >= 440 && wl < 490) 
	{
		R = 0;
		G = (wl - 440) / (490 - 440);
		B = 1;
	}
	else if (wl >= 490 && wl < 510) 
	{
		R = 0;
		G = 1;
		B = -1 * (wl - 510) / (510 - 490);
	}
	else if (wl >= 510 && wl < 580) 
	{
		R = (wl - 510) / (580 - 510);
		G = 1;
		B = 0;
	}
	else if (wl >= 580 && wl < 645) 
	{
		R = 1;
		G = -1 * (wl - 645) / (645 - 580);
		B = 0.0;
	}
	else if (wl >= 645 && wl <= 780) 
	{
		R = 1;
		G = 0;
		B = 0;
	}
	else 
	{
		R = 0;
		G = 0;
		B = 0;
	}

	// intensty is lower at the edges of the visible spectrum.
	if (wl > 780 || wl < 380) 
	{
		alpha = 0;
	}
	else if (wl > 700) 
	{
		alpha = (780 - wl) / (780 - 700);
	}
	else if (wl < 420) 
	{
		alpha = (wl - 380) / (420 - 380);
	}
	else 
	{
		alpha = 1;
	}

	return vec3(R, G, B) * alpha;
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

	float power;
	uint range = trace(eye, normalize(dir), power);

	vec3 color = wavelengthToColor(range) * power;

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