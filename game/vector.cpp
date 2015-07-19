#include "stdafx.h"

#include "vector.h"

#define _PI    ((float) 3.141592654f)
#define _1BYPI ((float) 0.318309886f)

#define DegreeToRadian( degree ) ((degree) * (_PI / 180.0f))
#define RadianToDegree( radian ) ((radian) * (180.0f / _PI))

void Normalize(VECTOR * pV1, VECTOR * pV2)
{
	float l = sqrtf(pV1->x * pV1->x + pV1->y * pV1->y + pV1->z * pV1->z + 1.0e-12);

	pV2->x = pV1->x / l;
	pV2->y = pV1->y / l;
	pV2->z = pV1->z / l;
}

float DotProduct(VECTOR * pV1, VECTOR * pV2)
{
	return pV1->x * pV2->x + pV1->y * pV2->y + pV1->z * pV2->z;
}

float GetDegreeFromPosition(float x, float y)
{   
	VECTOR	vtDir;
	VECTOR	vtStan;
	float	ret;

	vtDir.x = x;
	vtDir.y = y; 
	vtDir.z = 0.0f;

	Normalize(&vtDir, &vtDir); 

	vtStan.x = 0.0f;
	vtStan.y = 1.0f;
	vtStan.z = 0.0f;

	ret = RadianToDegree(acosf(DotProduct(&vtDir, &vtStan)));

	if (vtDir.x < 0.0f)
		ret = 360.0f - ret;

	return (ret);
}   

float GetDegreeFromPositionXY(long sx, long sy, long ex, long ey)
{
	return GetDegreeFromPosition(ex - sx, ey - sy);
}

void GetDeltaByDegree(float fDegree, float fDistance, float *x, float *y)
{
	float fRadian = DegreeToRadian(fDegree);

	*x = fDistance * sin(fRadian);
	*y = fDistance * cos(fRadian);
}

float GetDegreeDelta(float iDegree, float iDegree2)
{
	if (iDegree > 180.0f)
		iDegree = iDegree - 360.0f;

	if (iDegree2 > 180.0f)
		iDegree2 = iDegree2 - 360.0f;

	return fabs(iDegree - iDegree2);
}

