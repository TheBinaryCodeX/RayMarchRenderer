#version 430 core

layout(binding = 0, rgba32f) uniform image2D framebuffer;

uniform vec3 eye;
uniform vec3 ray00;
uniform vec3 ray10;
uniform vec3 ray01;
uniform vec3 ray11;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

uniform float maxDist;
uniform int maxSteps;
uniform int maxBounces;
uniform float stepMultiply;

uniform int currentSample;

uniform vec4 bounds;

uniform float time;

uniform sampler2D envTex;
uniform sampler2D envTexPower;
uniform int useEnvTex;

uniform float tempFact;
uniform float tempFact2;

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
	//co += vec2(time);

	highp float a = 12.9898;
	highp float b = 78.233;
	highp float c = 43758.5453;
	highp float dt = dot(co.xy, vec2(a, b));
	highp float sn = mod(dt, 3.14);

	randChange = fract(sin(sn) * c);

	return randChange;
}

vec3 skyColor(vec3 dir)
{
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

		return texture2D(envTex, uv).rgb;// *texture2D(envTexPower, uv).x * 100;
	}

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

// Misc Functions
void misc_getX(in vec3 v, out vec3 s)
{
	s = vec3(v.x);
}

void misc_getY(in vec3 v, out vec3 s)
{
	s = vec3(v.y);
}

void misc_getZ(in vec3 v, out vec3 s)
{
	s = vec3(v.z);
}

// Math Functions
void math_add(in vec3 x, in vec3 n, out vec3 y)
{
	y = x + n;
}

void math_subtract(in vec3 x, in vec3 n, out vec3 y)
{
	y = x - n;
}

void math_multiply(in vec3 x, in vec3 n, out vec3 y)
{
	y = x * n;
}


void math_divide(in vec3 x, in vec3 n, out vec3 y)
{
	y = x / n;
}

void math_sine(in vec3 x, out vec3 y)
{
	y = sin(x);
}

void math_cosine(in vec3 x, out vec3 y)
{
	y = cos(x);
}

// Map Functions
void map_sphere(in vec3 p, in vec3 centre, in vec3 radius, out vec3 d)
{
	vec3 q = p - centre;
	d = vec3(length(q) - radius.x);
}

void map_box(in vec3 p, in vec3 centre, in vec3 radius, out vec3 d)
{
	vec3 q = abs(p - centre) - radius;
	d = vec3(min(max(q.x, max(q.y, q.z)), 0) + length(max(q, 0)));
}

// Operator Functions
void op_union(in vec3 a, in vec3 b, out vec3 c)
{
	c = min(a, b);
}

void op_subtract(in vec3 a, in vec3 b, out vec3 c)
{
	c = max(a, -b);
}

void op_intersect(in vec3 a, in vec3 b, out vec3 c)
{
	c = max(a, b);
}

// Domain Functions
void domain_repeat(in vec3 p, in vec3 m, out vec3 q)
{
	q = p;

	if (m.x != 0)
	{
		q.x = mod(p.x, m.x) - m.x * 0.5;
	}
	if (m.y != 0)
	{
		q.y = mod(p.y, m.y) - m.y * 0.5;
	}
	if (m.z != 0)
	{
		q.z = mod(p.z, m.z) - m.z * 0.5;
	}
}

//#OBJFUNCINSERT

vec2 opU(vec2 a, vec2 b)
{
	return a.x < b.x ? a : b;
}

vec2 map(vec3 p)
{
	vec2 d = vec2(maxDist, -1);

	//#OBJINSERT

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
	return (color.r + color.g + color.b) / (channels.r + channels.g + channels.b);
}

// Misc Functions
void misc_facing(in RayData ray, out vec3 outFactor)
{
	if (!ray.inside)
	{
		outFactor = vec3(clamp(dot(-ray.dir, getNormal(ray.hit)), 0.0, 1.0));
	}
	else
	{
		outFactor = vec3(clamp(dot(ray.dir, getNormal(ray.hit)), 0.0, 1.0));
	}
}

void misc_inside(in RayData ray, out vec3 outFactor)
{
	if (ray.inside)
	{
		outFactor = vec3(1);
	}
	else
	{
		outFactor = vec3(0);
	}
}

// Math Functions
void math_add(in RayData ray, in vec3 x, in vec3 n, out vec3 y)
{
	y = n + x;
}

void math_subtract(in RayData ray, in vec3 x, in vec3 n, out vec3 y)
{
	y = n - x;
}

void math_multiply(in RayData ray, in vec3 x, in vec3 n, out vec3 y)
{
	y = n * x;
}

void math_divide(in RayData ray, in vec3 x, in vec3 n, out vec3 y)
{
	y = n / x;
}

// Shader Functions
void shader_mix(in RayData ray, in vec3 inColor1, in vec3 inDir1, in vec3 inInside1, in vec3 inColor2, in vec3 inDir2, in vec3 inInside2, in vec3 inFactor, out vec3 outColor, out vec3 outDir, out vec3 outInside)
{
	float r = rand(ray.origin.zx);
	if (r < clamp(grayscale(inFactor * channels), 0.0, 1.0))
	{
		outColor = inColor2;
		outDir = inDir2;
		outInside = inInside2;
	}
	else
	{
		outColor = inColor1;
		outDir = inDir1;
		outInside = inInside1;
	}
	if (clamp(grayscale(inFactor * channels), 0.0, 1.0) == 0)
	{
		outColor = inColor1;
		outDir = inDir1;
		outInside = inInside1;
	}
	else if (clamp(grayscale(inFactor * channels), 0.0, 1.0) == 1)
	{
		outColor = inColor2;
		outDir = inDir2;
		outInside = inInside2;
	}
}

void shader_diffuse(in RayData ray, in vec3 inColor, out vec3 outColor, out vec3 outDir)
{
	// Color
	outColor = inColor;

	// Direction
	vec2 rs1 = ray.hit.xy;// +vec2(time);
	vec2 rs2 = ray.hit.zx;// +vec2(time);
	outDir = randHemisphere(rs1, rs2, getNormal(ray.hit));
}

void shader_glossy(in RayData ray, in vec3 inColor, in vec3 inRoughness, out vec3 outColor, out vec3 outDir)
{
	// Color
	outColor = inColor;

	// Direction
	vec2 rs1 = ray.hit.yx;// +vec2(time);
	vec2 rs2 = ray.hit.xz;// +vec2(time);
	outDir = mix(randHemisphere(rs1, rs2, getNormal(ray.hit)), reflect(ray.dir, getNormal(ray.hit) * -(int(ray.inside) * 2 - 1)), 1.0 - grayscale(inRoughness * channels));
}

void shader_refraction(inout RayData ray, in vec3 inColor, in vec3 inIOR, in vec3 inRoughness, out vec3 outColor, out vec3 outDir, out vec3 outInside)
{
	// Color
	if (ray.inside)
	{
		outColor = inColor;
	}
	else
	{
		outColor = vec3(1, 1, 1);
	}

	// Direction
	if (!ray.inside)
	{
		outDir = normalize(refract(ray.dir, getNormal(ray.hit), 1.0 / grayscale(inIOR * channels)));
		outInside = vec3(1);
	}
	else
	{
		vec3 rDir = normalize(refract(ray.dir, -getNormal(ray.hit), grayscale(inIOR * channels)));
		vec3 dDir = randHemisphere(ray.hit.zy, ray.hit.yz, getNormal(ray.hit));

		outDir = mix(dDir, rDir, 1.0 - grayscale(inRoughness * channels));

		outInside = vec3(0);
	}
}

void shader_volumeScatter(inout RayData ray, in vec3 inColor, in vec3 inDensity, out vec3 outColor, out vec3 outDir, out vec3 outInside, out vec3 outHit)
{
	if (ray.inside)
	{
		float t = ray.t;

		float den = grayscale(inDensity * channels) / 20.0;

		int numPoints = int(floor(t * 100.0));

		vec3 hitPos = vec3(0);

		for (int i = 0; i < numPoints; i++)
		{
			float r = rand(ray.hit.xy);
			if (r < den)
			{
				r = rand(ray.hit.zy) * t;
				hitPos = ray.origin + ray.dir * r;
				break;
			}
		}

		if (hitPos != vec3(0))
		{
			outColor = inColor;
			outDir = randHemisphere(hitPos.xy, hitPos.zy, vec3(0));
			outInside = vec3(1);
			outHit = hitPos;
		}
		else
		{
			outColor = vec3(1, 1, 1);
			outDir = ray.dir;
			outInside = vec3(0);
			outHit = vec3(0);
		}
	}
	else
	{
		outColor = vec3(1, 1, 1);
		outDir = ray.dir;
		outInside = vec3(1);
		outHit = vec3(0);
	}
}

void shader_emission(in RayData ray, in vec3 inColor, in vec3 inPower, out vec3 outColor)
{
	outColor = inColor * grayscale(inPower * channels);
}

//#MATFUNCINSERT

vec3 trace(vec3 origin, vec3 dir)
{
	vec3 color = channels;

	vec3 o = origin;
	vec3 d = dir;

	bool inside = false;

	int bounces = 0;
	while (bounces < maxBounces)
	{
		bounces++;

		vec2 v;
		if (!inside)
		{
			v = march(o, d, 1);
		}
		else
		{
			v = march(o, d, -1);
		}

		RayData ray;
		ray.origin = o;
		ray.dir = d;
		ray.t = v.x;
		ray.hit = o + v.x * d;
		ray.inside = inside;

		if (ray.t < maxDist)
		{
			vec3 newColor = vec3(0);
			vec3 newDir = vec3(0);
			vec3 newInside = vec3(0);
			vec3 newHit = vec3(0);

			switch (int(v.y))
			{
			//#CASEINSERT
			}
			
			color *= newColor;

			inside = bool(newInside.x);

			if (newDir == vec3(0, 0, 0))
			{
				break;
			}
			else
			{
				d = newDir;

				if (newHit == vec3(0, 0, 0))
				{
					if (!inside)
					{
						o = ray.hit + getNormal(ray.hit) * 0.003;
					}
					else
					{
						o = ray.hit + getNormal(ray.hit) * -0.002;
					}
				}
				else
				{
					o = newHit;
				}
			}
		}
		else
		{
			vec3 emit;
			shader_emission(ray, skyColor(ray.dir), vec3(1, 1, 1), emit);
			color *= emit;
			break;
		}
	}

	return color;
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

void main()
{
	ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(framebuffer);

	if (pix.x < bounds.x || pix.y < bounds.y || pix.x >= bounds.z || pix.y >= bounds.w)
	{
		return;
	}

	vec2 pos = vec2(pix) / vec2(size.x, size.y);
	//vec3 dir = mix(mix(ray00, ray01, pos.x + 0.5 / size.x), mix(ray10, ray11, pos.x + 0.5 / size.x), pos.y + 0.5 / size.y);
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