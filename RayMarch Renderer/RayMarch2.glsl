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
	d = opU(d, vec2(map_box(p, vec3(0, 16.025, 0), vec3(8, 0.05, 8)), 0));

	d = opU(d, vec2(map_box(p, vec3(-8.025, 8, 0), vec3(0.05, 8, 8)), 2));
	d = opU(d, vec2(map_box(p, vec3(8.025, 8, 0), vec3(0.05, 8, 8)), 1));

	d = opU(d, vec2(map_box(p, vec3(0, 8, -8.025), vec3(8, 8, 0.05)), 0));
	d = opU(d, vec2(map_box(p, vec3(0, 8, 8.025), vec3(8, 8, 0.05)), 3));

	d = opU(d, vec2(map_sphere(p, vec3(-1, 1, 0), 1), 4));
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

struct DiffuseMaterial
{
	vec3 albedo;

	vec3 brdf(vec3 wi, vec3 wo)
	{
		return albedo / PI;
	}

	vec3 samplePDF(vec3 wo)
	{
		float sin2_theta = rand(albedo.rg);
		float cos2_theta = 1.0 - sin2_theta;

		float sin_theta = sqrt(sin2_theta);
		float cos_theta = sqrt(cos2_theta);

		float o = rand(albedo.bg) * 2 * PI;

		return normalize(vec3(sin_theta * cos(o), cos_theta, sin_theta * sin(o)));
	}

	vec3 weightPDF(vec3 wo)
	{
		return albedo;
	}
};

struct GlossyMaterial // All the math in this struct is probably wrong
{
	vec3 color;
	float roughness;

	vec3 brdf(vec3 wi, vec3 wo, vec3 normal)
	{
		vec3 h = normalize(wi + wo);

		float a = pow(roughness, 2);

		float dt = clamp(dot(h, normal), 0.0, 1.0);

		float k = a * sqrt(2.0 / PI);

		float d = ((a*a) / (PI * pow(pow(dt, 2) * (a*a - 1) + 1, 2)));
		float f = 1;// pow(1.0 - clamp(dot(h, normal), 0.0, 1.0), 5) * 0.96 + 0.04;
		float g = (2 * clamp(dot(wo, normal), 0.0, 1.0)) / (clamp(dot(wo, normal), 0.0, 1.0) + sqrt(a*a + (1.0 - a*a) * pow(clamp(dot(wo, normal), 0.0, 1.0), 2)));// clamp(dot(wo, normal), 0.0, 1.0) / (clamp(dot(wo, normal), 0.0, 1.0) * (1.0 - k) + k);// (clamp(dot(wo, normal), 0.0, 1.0) * clamp(dot(wi, normal), 0.0, 1.0)) * pow(clamp(dot(h, wo), 0.0, 1.0), 2);
		g *= (2 * clamp(dot(wi, normal), 0.0, 1.0)) / (clamp(dot(wi, normal), 0.0, 1.0) + sqrt(a*a + (1.0 - a*a) * pow(clamp(dot(wi, normal), 0.0, 1.0), 2)));
		//g *= clamp(dot(wi, normal), 0.0, 1.0) / (clamp(dot(wi, normal), 0.0, 1.0) * (1.0 - k) + k);;

		return color * clamp(((d * f * g) / (4 * clamp(dot(wi, normal), 0.0, 1.0) * clamp(dot(wo, normal), 0.0, 1.0))), 0.0, 1.0);
	}

	vec3 samplePDF(vec3 wo, vec3 normal)
	{
		float o = rand(color.bg) * 2 * PI;

		float a = pow(roughness, 2);
		float r = rand(color.bg);
		float theta = acos(sqrt((1.0f - r) / ((a*a - 1.0f) * r + 1.0f)));

		return normalize(vec3(sin(theta) * cos(o), cos(theta), sin(theta) * sin(o)));
	}

	vec3 weightPDF(vec3 wo)
	{
		return color;
	}
};

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
			mat3 tbn = makeTBN(point);

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
			case 3:
				mat.albedo = vec3(0.2, 0.8, 0.2);
				break;
			case 4:
				mat.albedo = vec3(0.8, 0.2, 0.2);
				break;
			}

			vec3 lightPos = vec3(2, 6, -2);
			float lightPower = 50.0;

			vec3 lightDir = normalize(lightPos - point);

			float f = pow(1.0 - clamp(dot(-d, normal), 0.0, 1.0), 5) * 0.96 + 0.04;
			f = 1;

			float r = rand(point.xy);
			if (int(v.y) == 4 && r <= f)
			{
				GlossyMaterial mat2;
				mat2.color = vec3(1);
				mat2.roughness = 0.2;

				float sd = march(point + normal * 0.002, lightDir, 1).x;
				if (sd >= length(lightPos - point))
				{
					endColors[nextOpen] = color * mat2.brdf(lightDir, -d, normal) * clamp(dot(lightDir, normal), 0.0, 1.0) * (lightPower / pow(length(lightPos - point), 2.0));
					nextOpen++;
				}
			}
			else
			{
				float sd = march(point + normal * 0.002, lightDir, 1).x;
				if (sd >= length(lightPos - point))
				{
					endColors[nextOpen] = color * mat.brdf(lightDir, -d) * clamp(dot(lightDir, normal), 0.0, 1.0) * (lightPower / pow(length(lightPos - point), 2.0));
					nextOpen++;
				}
			}

			vec3 newDir;
			if (int(v.y) == 4 && r <= f)
			{
				GlossyMaterial mat2;
				mat2.color = vec3(1);
				mat2.roughness = 0.2;

				newDir = tbn * mat2.samplePDF(-d, normal);

				vec3 reflectance = mat2.weightPDF(-d);
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
			}
			else
			{
				newDir = tbn * mat.samplePDF(-d);

				vec3 reflectance = mat.weightPDF(-d);
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

	if (bounces == maxBounces)
	{
		color = vec3(0);
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