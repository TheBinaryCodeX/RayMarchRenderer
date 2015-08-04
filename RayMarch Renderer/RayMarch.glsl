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
	Material material;

	float t;

	void MapData(Material mat, float T)
	{
		material = mat;
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
void mat_func_0(in RayData ray, out vec3 outColor, out vec3 outDir)
{
	vec3 diffDir;
	vec3 diff;
	shader_diffuse(ray, vec3(0.8, 0.1, 0.1), diff, diffDir);

	vec3 glossDir;
	vec3 gloss;
	shader_glossy(ray, vec3(0.8), 0.1, gloss, glossDir);

	vec3 mixedDir;
	vec3 mixedColor;
	shader_mix(ray, diff, diffDir, gloss, glossDir, 0.5, mixedColor, mixedDir);

	outColor = mixedColor;
	outDir = mixedDir;
}

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

		if (ray.t < maxDist && !v.material.emissive)
		{
			/*
			Node nodes[3];

			Node n0;
			n0.nodeID = 0;
			n0.inputs[0] = vec4(0, 0, 0, 0);
			n0.inputs[1] = vec4(1, 0, 0, 0);
			n0.inputs[2] = vec4(2, 0, 0, 0);
			n0.inputs[3] = vec4(3, 0, 0, 0);
			n0.inputs[4] = vec4(0.5, 0, 0, 1);
			n0.outputs[0] = vec4(0, 0, 0, 4);
			n0.outputs[1] = vec4(0, 0, 0, 5);
			n0.depth = 1;
			nodes[0] = n0;

			Node n1;
			n1.nodeID = 1;
			n1.inputs[0] = vec4(v.material.color, 3);
			n1.outputs[0] = vec4(0, 0, 0, 0);
			n1.outputs[1] = vec4(0, 0, 0, 1);
			n1.depth = 2;
			nodes[1] = n1;

			Node n2;
			n2.nodeID = 2;
			n2.inputs[0] = vec4(0.8, 0.8, 0.8, 3);
			n2.inputs[1] = vec4(0.02, 0, 0, 1);
			n2.outputs[0] = vec4(0, 0, 0, 2);
			n2.outputs[1] = vec4(0, 0, 0, 3);
			n2.depth = 2;
			nodes[2] = n2;

			vec3 inouts[8];

			int depth = getHeighestDepth(nodes);
			while (depth > 0)
			{
				for (int i = 0; i < 3; i++)
				{
					Node n = nodes[i];
					if (n.depth == depth)
					{
						if (n.nodeID == 0)
						{
							vec3 inputs[5];
							for (int i = 0; i < 5; i++)
							{
								if (n.inputs[i].w == 0)
								{
									inputs[i] = inouts[int(n.inputs[i].x)];
								}
								else
								{
									inputs[i] = n.inputs[i].xyz;
								}
							}
							shader_mix(ray, inputs[0], inputs[1], inputs[2], inputs[3], inputs[4].x, inouts[int(n.outputs[0].w)], inouts[int(n.outputs[1].w)]);
						}
						else if (n.nodeID == 1)
						{
							vec3 input;
							if (n.inputs[0].w == 0)
							{
								input = inouts[int(n.inputs[0].x)];
							}
							else
							{
								input = n.inputs[0].xyz;
							}
							shader_diffuse(ray, input, inouts[int(n.outputs[0].w)], inouts[int(n.outputs[1].w)]);
						}
						else if (n.nodeID == 2)
						{
							vec3 input;
							if (n.inputs[0].w == 0)
							{
								input = inouts[int(n.inputs[0].x)];
							}
							else
							{
								input = n.inputs[0].xyz;
							}
							vec3 input2;
							if (n.inputs[1].w == 0)
							{
								input2 = inouts[int(n.inputs[1].x)];
							}
							else
							{
								input2 = n.inputs[1].xyz;
							}
							shader_glossy(ray, input, input2.x, inouts[int(n.outputs[0].w)], inouts[int(n.outputs[1].w)]);
						}
						else if (n.nodeID == 3)
						{

						}
					}
				}
				depth--;
			}
			color *= inouts[4];
			d = inouts[5];

			o = ray.hit + getNormal(ray.hit) * 0.001;
			*/
			/*
			vec3 diffDir;
			vec3 diff;
			shader_diffuse(ray, v.material.color, diff, diffDir);

			vec3 glossDir;
			vec3 gloss;
			shader_glossy(ray, vec3(0.8), 0.1, gloss, glossDir);

			vec3 mixedDir;
			vec3 mixedColor;
			shader_mix(ray, diff, diffDir, gloss, glossDir, 0.5, mixedColor, mixedDir);

			color *= mixedColor;

			o = ray.hit + getNormal(ray.hit) * 0.001;
			d = mixedDir;
			*/
			vec3 outColor;
			vec3 outDir;
			mat_func_0(ray, outColor, outDir);
			color *= outColor;
			d = outDir;
			o = ray.hit + getNormal(ray.hit) * 0.001;;
		}
		else
		{
			vec3 emit;
			shader_emission(ray, v.material.color, v.material.power, emit);
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