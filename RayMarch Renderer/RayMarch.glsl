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

/*
Inputs:
If w is 0: x is a reference to an inout variable
If w is 1-3: the input is the first 1-3 numbers

Outputs:
xyz is the output itself, w is the inout variable to output to.
*/
struct Node
{
	int nodeID;
	vec4 inputs[8];
	vec4 outputs[8];

	int depth;
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
	int matID;
	float t;

	void MapData(int mat, float T)
	{
		matID = mat;
		t = T;
	}
};

struct RayData
{
	vec3 origin;
	vec3 hit;
	vec3 dir;
	float t;
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

vec3 skyColor(vec3 dir)
{
	//return vec3(0.5);
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

MapData opU(MapData a, MapData b)
{
	return a.t < b.t ? a : b;
}

MapData map(vec3 p)
{
	MapData d = MapData(-1, maxDist);

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
			return MapData(0, maxDist);
		}

		t += d.t;
	}

	return MapData(0, maxDist);
}

vec3 getNormal(vec3 p)
{
	vec3 n;

	n.x = map(p + vec3(0.001, 0, 0)).t - map(p - vec3(0.001, 0, 0)).t;
	n.y = map(p + vec3(0, 0.001, 0)).t - map(p - vec3(0, 0.001, 0)).t;
	n.z = map(p + vec3(0, 0, 0.001)).t - map(p - vec3(0, 0, 0.001)).t;

	return normalize(n);
}

vec3 randHemisphere(vec2 randSeed1, vec2 randSeed2, vec3 dir, vec3 normal)
{
	float theta = 2 * 3.141592653 * rand(randSeed1);
	float phi = acos(2 * rand(randSeed2) - 1);

	vec3 bDir = normalize(vec3(sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta)));

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

	return bDir;
}

// Shader Functions
void shader_mix(in RayData ray, in vec3 inColor1, in vec3 inDir1, in vec3 inColor2, in vec3 inDir2, in float inFactor, out vec3 outColor, out vec3 outDir)
{
	float r = rand(ray.origin.zx + vec2(time));
	if (r < inFactor)
	{
		outColor = inColor2;
		outDir = inDir2;
	}
	else
	{
		outColor = inColor1;
		outDir = inDir1;
	}
}

void shader_diffuse(in RayData ray, in vec3 inColor, out vec3 outColor, out vec3 outDir)
{
	// Color
	outColor = inColor;

	// Direction
	vec2 rs1 = ray.hit.xy + vec2(time);
	vec2 rs2 = ray.hit.zx + vec2(time);
	outDir = randHemisphere(rs1, rs2, ray.dir, getNormal(ray.hit));
}

void shader_glossy(in RayData ray, in vec3 inColor, in float inRoughness, out vec3 outColor, out vec3 outDir)
{
	// Color
	outColor = inColor;

	// Direction
	vec2 rs1 = ray.hit.yx + vec2(time);
	vec2 rs2 = ray.hit.xz + vec2(time);
	outDir = mix(randHemisphere(rs1, rs2, ray.dir, getNormal(ray.hit)), reflect(ray.dir, getNormal(ray.hit)), 1.0 - inRoughness);
}

void shader_emission(in RayData ray, in vec3 inColor, in float inPower, out vec3 outColor)
{
	outColor = inColor * inPower;
}

int getHeighestDepth(Node nodes[3])
{
	int d = 0;
	for (int i = 0; i < 3; i++)
	{
		d = max(d, nodes[i].depth);
	}
	return d;
}

//#MATFUNCINSERT

vec3 trace(vec3 origin, vec3 dir)
{
	vec3 color = vec3(1, 1, 1);

	vec3 o = origin;
	vec3 d = dir;

	int bounces = 0;
	while (bounces < 512)
	{
		bounces++;

		MapData v = march(o, d);

		RayData ray;
		ray.origin = o;
		ray.dir = d;
		ray.t = v.t;
		ray.hit = o + v.t * d;

		if (ray.t < maxDist)
		{
			vec3 newColor;
			vec3 newDir;

			switch (v.matID)
			{
			//#CASEINSERT
			}
			
			color *= newColor;

			if (newDir == vec3(0, 0, 0))
			{
				break;
			}
			else
			{
				d = newDir;
				o = ray.hit + getNormal(ray.hit) * 0.001;
			}
		}
		else
		{
			vec3 emit;
			shader_emission(ray, skyColor(ray.dir), 1, emit);
			color *= emit;
			break;
		}
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
	vec3 dir = mix(mix(ray00, ray01, pos.x + 0.5 / size.x), mix(ray10, ray11, pos.x), pos.y + 0.5 / size.y);

	vec3 color = trace(eye, normalize(dir));

	vec4 old = imageLoad(framebuffer, pix);

	float fact = 1.0 / float(passes);
	imageStore(framebuffer, pix, vec4(color * fact + old.rgb, 1));
}