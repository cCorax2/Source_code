#ifndef __INC_METIN_II_D3DTYPE_H__
#define __INC_METIN_II_D3DTYPE_H__

typedef struct D3DXVECTOR2
{
	float x, y;
} D3DXVECTOR2, *LPD3DXVECTOR2;

typedef struct D3DXVECTOR3
{
	float x, y, z;
} D3DXVECTOR3, *LPD3DXVECTOR3;

typedef struct D3DXVECTOR4
{
	float x, y, z, w;
} D3DXVECTOR4, *LPD3DXVECTOR4;

typedef struct D3DXQUATERNION
{
	float x, y, z, w;
} D3DXQUATERNION, *LPD3DXQUATERNION;

typedef struct D3DXCOLOR
{
	float r, g, b, a;
} D3DXCOLOR, *LPD3DXCOLOR;

typedef struct _D3DCOLORVALUE
{
	float r;
	float g;
	float b;
	float a;
} D3DCOLORVALUE;

typedef D3DXVECTOR3 D3DVECTOR;

#endif

