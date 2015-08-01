#version 430 core

layout(binding = 0, rgba32f) uniform image2D framebuffer;

uniform vec3 eye;
uniform vec3 ray00;
uniform vec3 ray10;
uniform vec3 ray01;
uniform vec3 ray11;

layout(local_size_x = 8, local_size_y = 8) in;

const float maxDist = 1000;
int maxSteps = 512;

const mat2 m = mat2(0.80, 0.60, -0.60, 0.80);

uniform float time;

uniform float tempFact;
uniform float tempFact2;

struct MapData
{
	vec3 color;
	bool reflective;

	float t;

	void MapData(vec3 c, bool b, float T)
	{
		color = c;
		reflective = b;
		t = T;
	}
};

float noise(in vec2 x)
{
	return sin(1.5*x.x)*sin(1.5*x.y);
}

float fbm4(vec2 p)
{
	float f = 0.0;
	f += 0.5000*noise(p); p = m*p*2.02;
	f += 0.2500*noise(p); p = m*p*2.03;
	f += 0.1250*noise(p); p = m*p*2.01;
	f += 0.0625*noise(p);
	return f / 0.9375;
}

float fbm6(vec2 p)
{
	float f = 0.0;
	f += 0.500000*(0.5 + 0.5*noise(p)); p = m*p*2.02;
	f += 0.250000*(0.5 + 0.5*noise(p)); p = m*p*2.03;
	f += 0.125000*(0.5 + 0.5*noise(p)); p = m*p*2.01;
	f += 0.062500*(0.5 + 0.5*noise(p)); p = m*p*2.04;
	f += 0.031250*(0.5 + 0.5*noise(p)); p = m*p*2.01;
	f += 0.015625*(0.5 + 0.5*noise(p));
	return f / 0.96875;
}

vec3 skyColor(vec3 dir)
{
	float d = dot(dir, vec3(0, 1, 0));

	if (d >= 0)
	{
		return mix(vec3(0.6484, 0.8828, 1.0), vec3(0.5, 0.582, 1.0), d);
	}
	else
	{
		return mix(vec3(0.6484, 0.8828, 1.0), vec3(0, 0, 0), -d * 4);
	}
}

float smin(float a, float b, float k)
{
	float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
	return mix(b, a, h) - k * h * (1.0 - h);
}

float mapSphere(vec3 p, vec3 centre, float radius)
{
	vec3 q = p - centre;

	//vec2 uv;
	//uv.x = atan(q.z, q.x);
	//uv.y = acos(q.y / length(q));

	return length(q) - radius;// +sin(uv.y * 16 * tempFact) * sin(uv.x * 16 * tempFact) * 0.05;//fbm6(uv * tempFact * 3) * (1.0 - fbm6(uv * tempFact * 3));
}

float mapPlane(vec3 p, vec3 point, vec3 normal)
{
	vec3 q = p - point;
	//float f = mix(fbm4(q.xz / 32 + vec2(2, 4)), fbm6(q.xz / 48), 0.5);
	return dot(q, normal);// +(f - 0.5) * 8;
}

float mapBox(vec3 p, vec3 centre, vec3 radius)
{
	vec3 q = abs(p - centre) - radius;

	/*
	vec2 uv;
	if (max(q.x, max(q.y, q.z)) == q.x)
	{
	uv = (p - centre).zy;
	}
	else if (max(q.x, max(q.y, q.z)) == q.y)
	{
	uv = (p - centre).xz;
	}
	else if (max(q.x, max(q.y, q.z)) == q.z)
	{
	uv = (p - centre).xy;
	}
	*/

	return min(max(q.x, max(q.y, q.z)), 0) + length(max(q, 0));// +sin(uv.y * 16 * tempFact) * sin(uv.x * 16 * tempFact) * 0.05;//fbm6(uv * tempFact * 3) * (1.0 - fbm6(uv * tempFact * 3));
}

MapData opU(MapData a, MapData b)
{
	return a.t < b.t ? a : b;
}

float getHueFact(float t)
{
	if (t < 2)
	{
		return 0;
	}
	else if (t >= 2 && t < 3)
	{
		return fract(t);
	}
	else if (t >= 3 && t < 5)
	{
		return 1;
	}
	else if (t >= 5 && t <= 6)
	{
		return 1.0 - fract(t);
	}
}

MapData map(vec3 p)
{
	MapData d = MapData(vec3(-1, -1, -1), false, maxDist);

	vec3 q = p - vec3(-1, 0, 0);

	vec3 color = vec3(0);

	float timeDiv = 2;

	color.r = getHueFact(mod(time + 4 * timeDiv, 12) / timeDiv);
	color.g = getHueFact(mod(time + 2 * timeDiv, 12) / timeDiv);
	color.b = getHueFact(mod(time + 0 * timeDiv, 12) / timeDiv);

	d = opU(d, MapData(vec3(0.8, 0.8, 0.8), false, mapPlane(p, vec3(0, -2, 0), vec3(0, 1, 0))));
	d = opU(d, MapData(color, true, smin(mapSphere(p, vec3(1.0 - tempFact - 0.5, 0, 1.0 - tempFact2) * 2, 1), mapSphere(p, vec3(tempFact - 0.5, 0, tempFact2) * 2, 1), 1)));
	//d = opU(d, MapData(vec3(0.1, 0.1, 1), true, mapSphere(p, vec3(1, 0, 0), 1)));

	return d;
}

MapData march(vec3 origin, vec3 dir)
{
	float t = 0;

	for (int i = 0; i < maxSteps; i++)
	{
		MapData d = map(origin + t * dir);

		if (d.t < 0.001)
		{
			return MapData(d.color, d.reflective, t);
		}

		if (t >= maxDist)
		{
			return MapData(skyColor(dir), false, maxDist);
		}

		t += d.t;
	}

	return MapData(skyColor(dir), false, maxDist);
}

vec3 getNormal(vec3 p)
{
	vec3 n;

	n.x = map(p + vec3(0.001, 0, 0)).t - map(p - vec3(0.001, 0, 0)).t;
	n.y = map(p + vec3(0, 0.001, 0)).t - map(p - vec3(0, 0.001, 0)).t;
	n.z = map(p + vec3(0, 0, 0.001)).t - map(p - vec3(0, 0, 0.001)).t;

	return normalize(n);
}

vec3 trace(vec3 origin, vec3 dir)
{
	MapData v = march(origin, dir);

	vec3 color = vec3(0, 0, 0);
	float t = v.t;

	if (t < maxDist)
	{
		vec3 point = origin + t * dir;
		vec3 normal = getNormal(point);
		point += normal * 0.001;

		color = vec3(clamp(dot(-dir, normal), 0.0, 1.0) * (1.0 / t) * 100);
	}

	return color;
}

void main()
{
	ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(framebuffer);

	if (pix.x >= size.x || pix.y >= size.y)
	{
		return;
	}

	vec2 pos = vec2(pix) / vec2(size.x, size.y);
	pos = pos * 2 - vec2(1);
	pos *= 1;

	vec3 locZ = normalize(-vec3(-2, 8, -4));
	vec3 locX = cross(locZ, vec3(0, 1, 0));
	vec3 locY = cross(locZ, locX);

	vec3 color = trace(vec3(-2, 8, -4) * 32 + pos.x * locX + pos.y * locY, normalize(-vec3(-2, 8, -4)));

	imageStore(framebuffer, pix, vec4(color, 1));
}