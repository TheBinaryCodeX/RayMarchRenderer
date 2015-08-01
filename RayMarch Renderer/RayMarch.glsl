#version 430 core

layout(binding = 0, rgba32f) uniform image2D framebuffer;

uniform vec3 eye;
uniform vec3 ray00;
uniform vec3 ray10;
uniform vec3 ray01;
uniform vec3 ray11;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

const float maxDist = 1000;
int maxSteps = 512;

const mat2 m = mat2(0.80, 0.60, -0.60, 0.80);

uniform int passes;

uniform vec4 bounds;

uniform float time;

uniform float tempFact;
uniform float tempFact2;

uniform sampler2D lightmap;

struct Shader
{

};

struct Material
{
	vec3 color;

	bool reflective;
	bool transmissive;
	bool emissive;

	float power;

	float rRoughness;
	float tRoughness;

	float ior;

	float mixFact;

	void Material(vec3 col, bool refl, bool trans, bool emit, float pow, float rRough, float tRough, float IOR, float mixFactor)
	{
		color = color;
		reflective = refl;
		transmissive = trans;
		emissive = emit;
		power = pow;
		rRoughness = rRough;
		tRoughness = tRough;
		ior = IOR;
		mixFact = mixFactor;
	}
};

struct MapData
{
	Material material;

	float t;

	void MapData(Material mat, float T)
	{
		material = mat;
		t = T;
	}
};

highp float rand(vec2 co)
{
	highp float a = 12.9898;
	highp float b = 78.233;
	highp float c = 43758.5453;
	highp float dt = dot(co.xy, vec2(a, b));
	highp float sn = mod(dt, 3.14);
	return fract(sin(sn) * c);
}

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
	//return vec3(1);
	//return vec3(0);
	return vec3(0.015);

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
	return length(q) - radius;
}

float mapBox(vec3 p, vec3 centre, vec3 radius)
{
	vec3 q = abs(p - centre) - radius;
	return min(max(q.x, max(q.y, q.z)), 0) + length(max(q, 0));
}

float mapTorus(vec3 p, vec3 centre, vec3 normal, vec2 radius)
{
	vec3 q = p - centre;

	vec3 locY = normal;

	vec3 locX;
	if (locY == vec3(0, 0, 1))
	{
		locX = normalize(cross(locY, vec3(0, 1, 0)));
	}
	else
	{
		locX = normalize(cross(locY, vec3(0, 0, 1)));
	}

	vec3 locZ = normalize(cross(locY, locX));

	vec2 q2 = vec2(length(vec2(dot(q, locX), dot(q, locZ))) - radius.x, dot(q, locY));
	return length(q2) - radius.y;
}

MapData opU(MapData a, MapData b)
{
	return a.t < b.t ? a : b;
}

MapData map(vec3 p)
{
	MapData d = MapData(Material(vec3(-1, -1, -1), false, false, false, -1, -1, -1, -1, -1), maxDist);

	//#MATINSERT

	//#OBJINSERT

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
			d.t = t;
			return d;
		}

		if (t >= maxDist)
		{
			return MapData(Material(skyColor(dir), false, false, true, 1, 1, 1, 1, 1), maxDist);
		}

		t += d.t;
	}

	return MapData(Material(skyColor(dir), false, false, true, 1, 1, 1, 1, 1), maxDist);
}

MapData marchT(vec3 origin, vec3 dir)
{
	float t = 0;

	for (int i = 0; i < maxSteps; i++)
	{
		MapData d = map(origin + t * dir);
		d.t *= -1;

		if (d.t < 0.001)
		{
			d.t = t;
			return d;
		}

		if (t >= maxDist)
		{
			return MapData(Material(skyColor(dir), false, false, true, 1, 1, 1, 1, 1), maxDist);
		}

		t += d.t;
	}

	return MapData(Material(skyColor(dir), false, false, true, 1, 1, 1, 1, 1), maxDist);
}

vec3 getNormal(vec3 p)
{
	vec3 n;

	n.x = map(p + vec3(0.001, 0, 0)).t - map(p - vec3(0.001, 0, 0)).t;
	n.y = map(p + vec3(0, 0.001, 0)).t - map(p - vec3(0, 0.001, 0)).t;
	n.z = map(p + vec3(0, 0, 0.001)).t - map(p - vec3(0, 0, 0.001)).t;

	return normalize(n);
}

vec3 calculateDir(vec4 randCoords, vec3 dir, vec3 normal, float rRoughness, float tRoughness, float eta, bool flipZ)
{
	float theta = 2 * 3.141592653 * rand(randCoords.xy);
	float phi = acos(2 * rand(randCoords.zw) - 1);

	vec3 bDir = normalize(vec3(sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta)));

	if (dot(bDir, vec3(0, 0, 1)) < 0)
	{
		bDir *= -1;
	}

	vec3 locZ = normal;
	if (flipZ)
	{
		locZ *= -1;
	}

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

	if (rRoughness != 1.0)
	{
		bDir = mix(bDir, reflect(dir, normal), 1.0 - clamp(rRoughness, 0.0, 1.0));
	}

	if (eta != 0.0)
	{
		bDir = mix(bDir, refract(dir, normal, eta), 1.0 - clamp(tRoughness, 0.0, 1.0));
	}

	return bDir;
}

vec3 trace(vec3 origin, vec3 dir)
{
	MapData v = march(origin, dir);

	vec3 color = v.material.color;
	bool reflective = v.material.reflective;
	bool transmissive = v.material.transmissive;
	bool emissive = v.material.emissive;
	float rRoughness = v.material.rRoughness;
	float tRoughness = v.material.tRoughness;
	float ior = v.material.ior;
	float mixFact = v.material.mixFact;
	float t = v.t;

	if (t < maxDist && !emissive)
	{
		vec3 point = origin + t * dir;
		vec3 normal = getNormal(point);
		point += normal * 0.001;

		vec3 diff = color;

		vec3 point2 = point;
		vec3 normal2 = normal;
		vec3 dir2 = dir;

		int bounces = 0;
		while (bounces < 512)
		{
			bounces++;

			vec2 s1 = (point2.xy * point2.yz) + vec2(time / 100);
			vec2 s2 = (point2.xy * point2.yz) + vec2(-time / 100);

			vec3 bDir;

			if (mixFact == -1)
			{
				mixFact = 1.0 - pow(1.0 - dot(-dir2, normal2), 1.0);
			}

			if (reflective)
			{
				float r = rand((point2.yx * point2.zy) + vec2(time / 50));
				if (r <= mixFact)
				{
					bDir = calculateDir(vec4(s1, s2), dir2, normal2, 1.0, 1.0, 0.0, false);
				}
				else
				{
					bDir = calculateDir(vec4(s1, s2), dir2, normal2, rRoughness, 1.0, 0.0, false);
				}
			}
			else
			{
				bDir = calculateDir(vec4(s1, s2), dir2, normal2, 1.0, 1.0, 0.0, false);
			}

			if (transmissive)
			{
				if (reflective)
				{
					float r = rand((point2.yx * point2.zy) + vec2(time / 50));
					if (r > mixFact)
					{
						bDir = calculateDir(vec4(s1, s2), dir2, normal2, rRoughness, 1.0, 0.0, false);
					}
					else
					{
						bDir = calculateDir(vec4(s1, s2), dir2, normal2, 1.0, tRoughness, 1.0 / ior, true);
						MapData mdt = marchT(point2 - normal2 * 0.003, bDir);

						point2 = point2 + mdt.t * bDir;
						normal2 = getNormal(point2);
						point2 += normal2 * 0.005;

						bDir = calculateDir(vec4(s2, s1), bDir, -normal2, 1.0, tRoughness, ior, true);
					}
				}
				else
				{
					bDir = calculateDir(vec4(s1, s2), dir2, normal2, 1.0, tRoughness, 1.0 / ior, true);
					MapData mdt = marchT(point2 - normal2 * 0.003, bDir);

					point2 = point2 + mdt.t * bDir;
					normal2 = getNormal(point2);
					point2 += normal2 * 0.005;

					bDir = calculateDir(vec4(s1, s2), bDir, -normal2, 1.0, tRoughness, ior, true);
				}
			}

			MapData md = march(point2, bDir);

			if (md.material.emissive)
			{
				if (md.t >= maxDist)
				{
					diff *= clamp(md.material.color * md.material.power, 0.0, 1.0);
				}
				else
				{
					diff *= md.material.color * md.material.power * (1.0 / pow(md.t, 2));
				}
				break;
			}
			else
			{
				diff *= md.material.color * clamp((1.0 / pow(md.t, 2)), 0.0, 1.0);
			}

			point2 = point2 + md.t * bDir;
			normal2 = getNormal(point2);
			point2 += normal2 * 0.001;

			dir2 = bDir;

			color = md.material.color;
			reflective = md.material.reflective;
			transmissive = md.material.transmissive;
			emissive = md.material.emissive;
			rRoughness = md.material.rRoughness;
			tRoughness = md.material.tRoughness;
			ior = md.material.ior;
			mixFact = md.material.mixFact;
		}

		color = diff;
	}

	return color;
}

float clength(vec3 v)
{
	return (abs(v.r) + abs(v.g) + abs(v.b)) / 3;
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
	vec3 dir = mix(mix(ray00, ray01, pos.x + 0.5 / size.x), mix(ray10, ray11, pos.x), pos.y + 0.5 / size.y);

	vec3 color = trace(eye, normalize(dir));
	//color = pow(color, vec3(0.4545));

	/*
	vec2 pos2 = pos + vec2(0.5, 0.5) / vec2(size.x, size.y);

	vec3 dir1 = mix(mix(ray00, ray01, pos2.x - 0.25 / size.x), mix(ray10, ray11, pos.x), pos2.y - 0.25 / size.y);
	vec3 dir2 = mix(mix(ray00, ray01, pos2.x + 0.25 / size.x), mix(ray10, ray11, pos.x), pos2.y - 0.25 / size.y);
	vec3 dir3 = mix(mix(ray00, ray01, pos2.x - 0.25 / size.x), mix(ray10, ray11, pos.x), pos2.y + 0.25 / size.y);
	vec3 dir4 = mix(mix(ray00, ray01, pos2.x + 0.25 / size.x), mix(ray10, ray11, pos.x), pos2.y + 0.25 / size.y);

	vec3 color1 = trace(eye, normalize(dir1));
	vec3 color2 = trace(eye, normalize(dir2));
	vec3 color3 = trace(eye, normalize(dir3));
	vec3 color4 = trace(eye, normalize(dir4));

	color = (color1 + color2 + color3 + color4) / 4;
	*/

	vec4 old = imageLoad(framebuffer, pix);

	float fact = 1.0 / float(passes);
	imageStore(framebuffer, pix, vec4(color * fact + old.rgb, 1));
}