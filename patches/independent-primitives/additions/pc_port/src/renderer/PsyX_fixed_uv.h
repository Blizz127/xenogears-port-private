#pragma once

#include <stdint.h>


struct XenoFixedUvVertex
{
	int16_t x;
	int16_t y;
	uint8_t u;
	uint8_t v;
};

struct XenoFixedUvRow
{
	int32_t seedU;
	int32_t seedV;
	int32_t stepU;
	int32_t stepV;
	int32_t firstX;
	int32_t valid;
	int32_t lastX;
	int32_t reserved1;
};

bool XenoBuildFixedUvRows(const XenoFixedUvVertex vertices[3], int width,
	int height, XenoFixedUvRow* rows);
