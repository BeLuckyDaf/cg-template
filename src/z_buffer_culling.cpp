#include "z_buffer_culling.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <iostream>


ZCulling::ZCulling(unsigned short width, unsigned short height, std::string obj_file) : TriangleRasterization(width, height, obj_file)
{
}

ZCulling::~ZCulling()
{
}

void ZCulling::DrawScene()
{
	parser->Parse();

	float rotangle = 0.0f * M_PI / 180.0;
	float4x4 rotate{
		{1, 0, 0, 0},
		{0, cos(rotangle), sin(rotangle), 0},
		{0, -sin(rotangle), cos(rotangle), 0},
		{0, 0, 0, 1},
	};

	float3 eye{ 0, 0, 0 };
	float3 look_at{ 0, 0, 1 };
	float3 up{ 0, 1, 0 };

	float3 zaxis = normalize(eye - look_at);
	float3 xaxis = normalize(cross(up, zaxis));
	float3 yaxis = cross(zaxis, xaxis);

	float aspect = static_cast<float>(width) / static_cast<float>(height);
	float fovy = 60.0 * M_PI / 180.0;
	float yscale = 1.0 / tan(fovy / 2.0);
	float xscale = yscale / aspect;
	float z_far = 10;
	float z_near = 1;

	float4x4 view{
		{xaxis.x, xaxis.y, xaxis.z, -dot(xaxis, eye)},
		{yaxis.x, yaxis.y, yaxis.z, -dot(yaxis, eye)},
		{zaxis.x, zaxis.y, zaxis.z, -dot(zaxis, eye)},
		{0, 0, 0, 1},
	};

	float4x4 translate{
		{1, 0, 0, 0},
		{0, 1, 0, 0},
		{0, 0, 1, 0},
		{0, 0, 2, 1},
	}; // казалось бы, строки, но это столбцы (A^T)

	float4x4 projection{
		{xscale, 0, 0, 0},
		{0, yscale, 0, 0},
		{0, 0, z_far / (z_far - z_near), -z_near * z_far / (z_far - z_near)},
		{0, 0, 1, 0},
	};

	float4x4 world = mul(translate, rotate);

	float4x4 translate_matrix = mul(projection, view, world);

	for (auto face : parser->GetFaces()) {
		float4 translated[3];
		for (int i = 0; i < 3; i++) {
			translated[i] = mul(translate_matrix, face.vertexes[i]);
			translated[i] /= translated[i].w;
			translated[i].x = x_center + x_center * translated[i].x;
			translated[i].y = y_center - y_center * translated[i].y;
		}
		DrawTriangle(translated);
	}
}

void ZCulling::Clear()
{

}

void ZCulling::DrawTriangle(float4 triangle[3])
{
	float2 bb_begin = max(float2{ 0,0 }, min(min(triangle[0].xy(), triangle[1].xy()), triangle[2].xy()));
	float2 bb_end = min(float2{ static_cast<float>(width - 1), static_cast<float>(height - 1) }, max(max(triangle[0].xy(), triangle[1].xy()), triangle[2].xy()));

	float area = EdgeFunction(triangle[0].xy(), triangle[1].xy(), triangle[2].xy());
	bool drawing = false;
	for (float x = bb_begin.x; x <= bb_end.x; x++) {
		for (float y = bb_begin.y; y <= bb_end.y; y++) {
			float area1 = EdgeFunction(triangle[0].xy(), triangle[1].xy(), float2{ x, y });
			float area0 = EdgeFunction(triangle[1].xy(), triangle[2].xy(), float2{ x, y });
			float area2 = EdgeFunction(triangle[2].xy(), triangle[0].xy(), float2{ x, y });
			if (area0 >= 0.0 && area1 >= 0.0 && area2 >= 0.0) {
				float u = area0 / area;
				float v = area1 / area;
				float w = area2 / area;
				drawing = true;
				float z = u * (-triangle[0].z) + v * (-triangle[1].z) + w * (-triangle[2].z);
				SetPixel(x, y, color(255 * u, 255 * v, 255 * w), z);
			}
		}
	}

	if (drawing) {
		DrawLine(static_cast<unsigned short>(triangle[0].x),
			static_cast<unsigned short>(triangle[0].y),
			static_cast<unsigned short>(triangle[1].x),
			static_cast<unsigned short>(triangle[1].y), color(255, 255, 255));

		DrawLine(static_cast<unsigned short>(triangle[1].x),
			static_cast<unsigned short>(triangle[1].y),
			static_cast<unsigned short>(triangle[2].x),
			static_cast<unsigned short>(triangle[2].y), color(255, 255, 255));

		DrawLine(static_cast<unsigned short>(triangle[2].x),
			static_cast<unsigned short>(triangle[2].y),
			static_cast<unsigned short>(triangle[0].x),
			static_cast<unsigned short>(triangle[0].y), color(255, 255, 255));
	}
}

void ZCulling::SetPixel(unsigned short x, unsigned short y, color color, float depth)
{
	int index = y * width + x;
	if (depth_buffer[index] < depth)
	{
		depth_buffer[index] = depth;
		frame_buffer[index] = color;
	}
}

