#include "stdafx.h"
#include "../../libgame/include/attribute.h"
#include "../../libgame/include/targa.h"
#include "../../common/d3dtype.h"
#include "utils.h"
#include "minilzo.h"
#include "sectree.h"
#include "sectree_manager.h"
#include "vector.h"
#include "lzo_manager.h"
#include "char_manager.h"
#include "desc_manager.h"
#include "questmanager.h"
#include "arena.h"
#include "pvp.h"
#include "cmd.h"

enum
{
	COLLISION_TYPE_PLANE,
	COLLISION_TYPE_BOX,
	COLLISION_TYPE_SPHERE,
	COLLISION_TYPE_CYLINDER,
};  

typedef struct SSphereData
{
	D3DXVECTOR3 v3Position;
	float fRadius;
} TSphereData;

typedef struct SPlaneData
{
	D3DXVECTOR3 v3Position;
	D3DXVECTOR3 v3Normal;

	D3DXVECTOR3 v3QuadPosition[4];
	D3DXVECTOR3 v3InsideVector[4];
} TPlaneData;

typedef struct SCylinderData
{
	D3DXVECTOR3 v3Position;
	float	fRadius;
	float	fHeight;
} TCylinderData;

LPSECTREE_MAP g_pkMapSectree;

int	max_bytes_written = 0;
int	total_bytes_written = 0;
int	current_bytes_written = 0;

int g_bx;
int g_by;

LPFDWATCH   main_fdw = NULL;
socket_t	udp_socket = 0;

bool g_bShutdown = false;

void	ConvertAttribute(const char * c_pszCollisionDataFileName, const char * c_pszMapDirectory);
void	ReadColorMapRecursive(const char * c_pszFileName);
void	ConvertAttribute2(const char * filename);

void ContinueOnFatalError()
{
}

void ShutdownOnFatalError()
{
}

void heartbeat(LPHEART heart, int pulse)
{
}

bool Metin2Server_IsInvalid()
{
	return false;
}

void SectreeTest(LPSECTREE pSec)
{
	sys_log(0, " %d %d section attribute ------------------------------", pSec->GetID().coord.x, pSec->GetID().coord.y);

	DWORD * line = M2_NEW DWORD[SECTREE_SIZE / CELL_SIZE];

	for (int y = 0; y < SECTREE_SIZE / CELL_SIZE; y++)
	{
		pSec->GetAttributePtr()->CopyRow(y, line);

		for (int x = 0; x < SECTREE_SIZE / CELL_SIZE; x++)
			printf("%c", line[x] ? 'X' : ' ');

		puts("");
	}

	M2_DELETE_ARRAY(line);
}

int start(int argc, char ** argv)
{
	extern const char * _malloc_options;
	_malloc_options = "A";

	if (lzo_init() != LZO_E_OK)
	{
		sys_err("lzo_init() failed");
		return 0;
	}

	thecore_init(25, heartbeat);
	signal_timer_disable();
	return 1;
}

int main(int argc, char ** argv)
{
	SECTREE_MANAGER	sectree_manager;
	DESC_MANAGER desc_manager;
	CHARACTER_MANAGER char_manager;
	quest::CQuestManager quest_manager; // CHARACTER::Intiailize에서 필요함
	CArenaManager arena_manager;
	CPVPManager pvp_manager;
	LZOManager lzo;

	if (!start(argc, argv))
		return 0;

	signal_timer_disable();

	char buf[256];
	char last_cmd[256];
	char * p;

	bool bEnd = false;

	while (!bEnd && fgets(buf, 256, stdin))
	{
		while ((p = strrchr(buf, '\r'))) *p = '\0';
		while ((p = strrchr(buf, '\n'))) *p = '\0';

		if (buf[0] == '!')
			strlcpy(buf, last_cmd, sizeof(buf));

		strlcpy(last_cmd, buf, sizeof(last_cmd));

		char arg1[64], arg2[64];//, arg3[64], arg4[64];
		const char * line = one_argument(buf, arg1, sizeof(arg1));

		switch (arg1[0])
		{
			case 'a':
				{
					two_arguments(line, arg1, sizeof(arg1), arg2, sizeof(arg2));

					if (!*arg1 || !*arg2)
					{
						printf("Syntax: a <collision data filename> <map directory>\n");
						break;
					}

					ConvertAttribute(arg1, arg2);
					puts("build server_attr done");
				}
				break;

			case 'c':
				{
					one_argument(line, arg1, sizeof(arg1));

					if (!*arg1)
					{
						printf("Syntax: c <filename>\n");
						break;
					}

					ConvertAttribute2(arg1);
				}
				//ReadColorMapRecursive(line);
				break;

			case 'b':
				{
					// Buffer overflow test (must use with valgrind or gdb at least)
                    LPCHARACTER ch = CHARACTER_MANAGER::instance().CreateCharacter("test");

					// 스택에 할당하면 valgrind가 제대로 오류를 잡지 못함
					size_t bufsize = 512 + 1;
					size_t linesize = 1024 + 1;

					if (bufsize > 0 && linesize > 0)
					{
						char *buf = (char *) malloc(bufsize);
						char *line = (char *) malloc(linesize);

						memset(buf, 0, bufsize);
						memset(line, 0, linesize);

						for (size_t i = 0; i < bufsize - 1; ++i)
						{
							buf[i] = '$';
							int linelen = snprintf(line, linesize, "pvp %s", buf);

							if (linelen < 0 || linelen >= (int) linesize)
								linelen = linesize - 1;

							printf("%d %s\n", i, line);
							interpret_command(ch, line, linelen);
						}

						free(buf);
						free(line);
					}
					else
					{
						printf("size error!\n");
						abort();
					}

					printf("Buffer overflow test finished\n");
				}
				break;

			case 'q':
				bEnd = true;
				break;
		}
	}

	thecore_destroy();
	event_destroy();
	return 0;
}

const int region[8][2] =
{
	{   0,  64 },
	{  64, 128 },
	{ 128, 192 },
	{ 192, 256 }
	/*{   0,  32 },
	  {  32,  64 },
	  {  64,  96 },
	  {  96, 128 },
	  { 128, 160 },
	  { 160, 192 },
	  { 192, 224 },
	  { 224, 256 },*/
};

bool ReadMapAttribute(DWORD dwAreaX, DWORD dwAreaY, const char * c_pszFileName)
{
	FILE * fp = fopen(c_pszFileName, "rb");

	if (!fp)
	{
		sys_err("cannot open %s", c_pszFileName);
		return false;
	}

	WORD wCC, wWidth, wHeight;
	fread(&wCC,		sizeof(WORD), 1, fp);

	if (wCC != 2634)
	{
		sys_err("not a attribute file: %s cc: %d", c_pszFileName, wCC);
		fclose(fp);
		return false;
	}

	fread(&wWidth,	sizeof(WORD), 1, fp);
	fread(&wHeight,	sizeof(WORD), 1, fp);

	sys_log(0, "Reading attribute on %d %d (w %d h %d)", dwAreaX, dwAreaY, wWidth, wHeight);
	//printf("Reading attribute on %d %d (w %d h %d)", dwAreaX, dwAreaY, wWidth, wHeight);

	BYTE * pbAttr = M2_NEW BYTE [wWidth * wHeight];

	if (fread(pbAttr, sizeof(BYTE), wWidth * wHeight, fp) != (DWORD) (wWidth * wHeight))
	{
		sys_err("cannot read %s", c_pszFileName);
		M2_DELETE_ARRAY(pbAttr);
		fclose(fp);
		return false;
	}

	fclose(fp);	// close file

	// 
	// 하나의 SECTREE는 6400 PIXEL을 차지 100픽셀이 1미터 이므로 
	// 1 SECTREE = 64m = 6400px
	//
	// 클라이언트의 하나의 Area는 256m를 차지하므로 SECTREE 네개를 처리해야 
	// 한다.
	//
	DWORD dwStartX, dwStartY, dwEndX, dwEndY;

	dwEndX = (dwAreaX + 1) * (25600 / SECTREE_SIZE);
	dwEndY = (dwAreaY + 1) * (25600 / SECTREE_SIZE);
	dwStartX = dwEndX - (25600 / SECTREE_SIZE);
	dwStartY = dwEndY - (25600 / SECTREE_SIZE);

	for (DWORD y = dwStartY; y < dwEndY; ++y)
	{
		for (DWORD x = dwStartX; x < dwEndX; ++x)
		{
			SECTREEID id;

			id.coord.x = x+g_bx;
			id.coord.y = y+g_by;

			LPSECTREE pSec;

			if (!(pSec = g_pkMapSectree->Find(id.package)))
			{
				sys_err("cannot get sectree (id %d %d)", id.coord.x, id.coord.y);
				break;
			}

			// 64 128 192 256
			//  0   1   2   3
			for (int ay = region[y - dwStartY][0], py = 0; ay < region[y - dwStartY][1]; ++ay, py += 2)
			{
				for (int ax = region[x - dwStartX][0], px = 0; ax < region[x - dwStartX][1]; ++ax, px += 2)
				{
					// AREA의 경우 속성 해상도가 1m 이므로
					// CELL 1개가 50x50cm인 SECTREE에서는
					// AREA의 속성 한개가 2x2개를 차지 한다.
					BYTE bAttr = pbAttr[ay * wHeight + ax];

					if (bAttr & ATTR_OBJECT)
					{
						printf("8");
						REMOVE_BIT(bAttr, ATTR_OBJECT);
					}

					printf("%u", bAttr);

					pSec->SetAttribute(px   , py   , bAttr);
					pSec->SetAttribute(px   , py + 1, bAttr);
					pSec->SetAttribute(px + 1, py   , bAttr);
					pSec->SetAttribute(px + 1, py + 1, bAttr);
				}
			}
		}
	}

	M2_DELETE_ARRAY(pbAttr);
	return true;
}

void ReadMapRecursive(const char * c_pszFileName)
{
	struct stat s;
	char buf[512 + 1];

	//printf("%s\n",c_pszFileName);

	if (stat(c_pszFileName, &s) != 0)
	{
		sys_log(0, "No map directory: %s", c_pszFileName);
		return;
	}

	if (S_ISDIR(s.st_mode))
	{
		struct dirent ** namelist;
		int n;

		n = scandir(c_pszFileName, &namelist, 0, alphasort);

		if (n < 0)
			perror("scandir");
		else
		{
			while (n-- > 0)
			{
				if (namelist[n]->d_name[0] == '.')
					continue;

				snprintf(buf, sizeof(buf), "%s/%s", c_pszFileName, namelist[n]->d_name);
				ReadMapRecursive(buf);
			}
		}

		free(namelist);
		return;
	}

	const char * p;

	if ((p = strrchr(c_pszFileName, '.')))
	{
		++p;

		if (!strncmp(p, "atr", 3))
		{
			int count = 0;
			int i;

			for (i = strlen(c_pszFileName); i >= 0; --i)
				if (c_pszFileName[i] == '/' && ++count == 2)
					break;

			if (i > 0)
				p = &c_pszFileName[i + 1];
			else
				return;

			char szCoordX[4];
			char szCoordY[4];
			strlcpy(szCoordX, p, sizeof(szCoordX));
			strlcpy(szCoordY, p + 3, sizeof(szCoordY));

			ReadMapAttribute(atoi(szCoordX), atoi(szCoordY), c_pszFileName);
		}
	}
}

void ReadColorMap(const char * c_pszFileName)
{
	FILE * fp = fopen(c_pszFileName, "rb");

	WORD wMagic, wWidth, wHeight;
	WORD awTileMap[256 * 256];

	fread(&wMagic, sizeof(WORD), 1, fp);
	fread(&wWidth, sizeof(WORD), 1, fp);
	fread(&wHeight, sizeof(WORD), 1, fp);
	fread(&awTileMap, sizeof(WORD), 256 * 256, fp);

	bool abUsed[32];

	memset(&abUsed, 0, sizeof(abUsed));

	for (int y = 0; y < 256; ++y)
		for (int x = 0; x < 256; ++x)
		{
			BYTE t = awTileMap[y * 256 + x] >> 8;

			if (t >= 32)
			{
				printf("ERROR! %d %dx%d\n", t, x, y);
				continue;
			}
			else
				abUsed[t] = true;
		}

	int iUsed = 0;

	for (int i = 0; i < 32; ++i)
		if (abUsed[i])
			++iUsed;

	printf("%s %d\n", c_pszFileName, iUsed);
	fclose(fp);
}

void ReadColorMapRecursive(const char * c_pszFileName)
{
	struct stat	s;
	char	buf[512 + 1];

	if (stat(c_pszFileName, &s) != 0)
	{
		sys_log(0, "No map directory: %s", c_pszFileName);
		return;
	}

	if (S_ISDIR(s.st_mode))
	{
		struct dirent ** namelist;
		int		 n;

		n = scandir(c_pszFileName, &namelist, 0, alphasort);

		if (n < 0)
			perror("scandir");
		else
		{
			while (n-- > 0)
			{
				if (namelist[n]->d_name[0] == '.')
					continue;

				snprintf(buf, sizeof(buf), "%s/%s", c_pszFileName, namelist[n]->d_name);
				ReadColorMapRecursive(buf);
			}
		}

		free(namelist);
		return;
	}

	char * p;

	if ((p = strrchr(c_pszFileName, '.')))
	{
		++p;

		if (!strncmp(p, "cmp", 3))
			ReadColorMap(c_pszFileName);
	}
}

	template <typename _f>
void ProcessLine(int sx, int sy, int ex, int ey, _f f)
{
	int         mode = 0;
	int         w, h;
	int         x = 0, y = 0;
	int         d;
	int         px, py;

	// 0: dx > 0 && dy > 0
	// 1: dx < 0 && dy > 0
	// 2: dx > 0 && dy < 0
	// 3: dx < 0 && dy < 0
	if (sx > ex)
	{
		mode |= 1;
		w = sx - ex;
	}
	else
		w = ex - sx;

	if (sy > ey)
	{
		mode |= 2;
		h = sy - ey;
	}
	else
		h = ey - sy;

	// distance = (w * w) + (h * h); // 거리
	if (w > h)
	{
		d = (h << 1) - w;

		while (x < w)
		{
			if (d < 0)
			{
				d += (h << 1);
				x++;
			}
			else
			{
				d += ((h - w) << 1);

				x++;
				y++;
			}

			switch (mode)
			{
				case 0: px = sx + x; py = sy + y; break;
				case 1: px = sx - x; py = sy + y; break;
				case 2: px = sx + x; py = sy - y; break;
				case 3: px = sx - x; py = sy - y; break;
				default: return;
			}

			if (!f(px, py))	// false 가 리턴 되면 중단 한다.
			{
				printf("putting error %d %d", px, py);
				return;
			}
		}
	}
	else
	{
		d = h - (w << 1);

		while (y < h)
		{
			if (d > 0)
			{
				d += -(w << 1);
				y++;
			}
			else
			{
				d += ((h - w) << 1);
				x++;
				y++; 
			}   

			switch (mode)
			{
				case 0: px = sx + x; py = sy + y; break;
				case 1: px = sx - x; py = sy + y; break;
				case 2: px = sx + x; py = sy - y; break;
				case 3: px = sx - x; py = sy - y; break;
				default: return;
			}

			if (!f(px, py))	// false 가 리턴 되면 중단 한다.
			{
				printf("putting error %d %d", px, py);
				return;
			}
		}
	}
}

	template <typename _f>
void ProcessSphere(float x, float y, float fRadius, _f f)
{
	//printf("%f %f %f\n", x, y, fRadius);
	int r = (int)fRadius + 1;
	int ix = (int)x;
	int iy = (int)y;
	//int ex = (int) (ix + r);
	//int ey = (int) (iy + r);
	//printf("%d %d %d\n", ix, iy, r);

	for (int rx = - r; rx < + r; rx+=25)
		for (int ry = - r; ry < + r; ry+=25)
		{
			//printf("%d %d %d %d %d %d\n", rx, ry, ix-rx, iy-ry, (ix-rx)*(ix-rx)+(iy-ry)*(iy-ry), r*r);
			//if ((ix-rx)*(ix-rx)+(iy-ry)*(iy-ry)<r*r)
			if (rx*rx + ry*ry < r*r)
			{
				//printf("#%d %d\n", rx+ix, ry+iy);
				if (!f(rx+ix, ry+iy))
				{
					printf("putting error");
					return;
				}
			}
		}
}

bool BlockAttribute(int x, int y)
{
	int bx = x / SECTREE_SIZE;
	int by = y / SECTREE_SIZE;

	SECTREEID id;

	id.coord.x = bx;
	id.coord.y = by;

	LPSECTREE pSec = g_pkMapSectree->Find(id.package);

	if (!pSec)
	{
		printf("BlockAttribute: no sectree on %d %d", bx, by);
		return false;
	}

	pSec->SetAttribute((x % SECTREE_SIZE) / CELL_SIZE, (y % SECTREE_SIZE) / CELL_SIZE, 1);
	//printf("BlockAttribute: put on %d %d", x, y);
	return true;
}

void ReadCollisionData(const char * c_pszFileName, int iBaseX, int iBaseY)
{
	FILE * fp = fopen(c_pszFileName, "rb");

	printf("Reading collision data file: %s\n", c_pszFileName);

	if (!fp)
	{
		printf("No collision data file: %s\n", c_pszFileName);
		return;
	}

	DWORD dwFourCC;
	fread(&dwFourCC, sizeof(DWORD), 1, fp);

	if (dwFourCC != MAKEFOURCC('M', '2', 'C', 'D'))
	{
		printf("Not a collision data file: %s", c_pszFileName);
		fclose(fp);
		return;
	}

	WORD wxSize, wySize;
	fread(&wxSize, sizeof(WORD), 1, fp);
	fread(&wySize, sizeof(WORD), 1, fp);

	printf("collision area size: %d %d", wxSize, wySize);

	for (DWORD x = 0; x < wxSize; ++x)
	{
		for (DWORD y = 0; y < wySize; ++y)
		{
			DWORD dwCount;
			fread(&dwCount, sizeof(DWORD), 1, fp);

			printf("\n%ux%u %u ", x, y, dwCount);

			for (DWORD j = 0; j < dwCount; ++j)
			{
				BYTE bType;
				fread(&bType, sizeof(BYTE), 1, fp);

				switch (bType)
				{
					case COLLISION_TYPE_PLANE:
						printf("P");
						{       
							TPlaneData PlaneData;
							fread(&PlaneData, sizeof(TPlaneData), 1, fp);

							int pairs[6][2] =
							{
								{ 0, 3 },
								{ 0, 1 },
								{ 0, 2 },
								{ 1, 2 },
								{ 1, 3 },
								{ 2, 3 },
							};

							for (int iPairIndex = 0; iPairIndex < 6; iPairIndex++)
							{
								int iFrom = pairs[iPairIndex][0];
								int iTo = pairs[iPairIndex][1];

								if (fabs(PlaneData.v3QuadPosition[iFrom].x - PlaneData.v3QuadPosition[iTo].x) > 100000.0f ||
										fabs(PlaneData.v3QuadPosition[iFrom].y - PlaneData.v3QuadPosition[iTo].y) > 100000.0f)
								{
									sys_log(0, "Blcok too big: %d %d %d %d", 
											iBaseX + (int)  PlaneData.v3QuadPosition[iFrom].x,
											iBaseY + (int) -PlaneData.v3QuadPosition[iFrom].y,
											iBaseX + (int)  PlaneData.v3QuadPosition[iTo].x,
											iBaseY + (int) -PlaneData.v3QuadPosition[iTo].y);
								}
								else
								{
									printf("Block %d %d %d %d\n", 
											iBaseX + (int)  PlaneData.v3QuadPosition[iFrom].x,
											iBaseY + (int) -PlaneData.v3QuadPosition[iFrom].y,
											iBaseX + (int)  PlaneData.v3QuadPosition[iTo].x,
											iBaseY + (int) -PlaneData.v3QuadPosition[iTo].y);


									ProcessLine(
											iBaseX + (int)  PlaneData.v3QuadPosition[iFrom].x,
											iBaseY + (int) -PlaneData.v3QuadPosition[iFrom].y,
											iBaseX + (int)  PlaneData.v3QuadPosition[iTo].x,
											iBaseY + (int) -PlaneData.v3QuadPosition[iTo].y,
											BlockAttribute);

								}
							}
							// 사각형 채우기도 합시다
							// 0 1 2 3 순서대로
							// -- +- -+ ++
							// 근데 y 는 마이너스 취하는거 확인하시길
							// TODO
						}
						break;

					case COLLISION_TYPE_SPHERE:
						printf("S");
						{
							TSphereData SphereData;
							fread(&SphereData, sizeof(TSphereData), 1, fp);
							ProcessSphere(
									iBaseX + SphereData.v3Position.x, 
									iBaseY - SphereData.v3Position.y, 
									SphereData.fRadius, 
									BlockAttribute);
						}
						break;

					case COLLISION_TYPE_CYLINDER:
						printf("C");
						{       
							TCylinderData CylinderData;
							fread(&CylinderData, sizeof(TCylinderData), 1, fp);
							ProcessSphere(
									iBaseX + CylinderData.v3Position.x, 
									iBaseY - CylinderData.v3Position.y, 
									CylinderData.fRadius, 
									BlockAttribute);
						}
						break;

					default:
						printf("invalid type!");
						break;
				}
			}
		}
	}

	printf("%d %d\n", iBaseX, iBaseY);

	fclose(fp);
}

void ConvertAttribute(const char * c_pszCollisionDataFileName, const char * c_pszMapDirectory)
{
	char szFilename[256];
	snprintf(szFilename, sizeof(szFilename), "%s/Setting.txt", c_pszMapDirectory);
	TMapSetting setting;
	SECTREE_MANAGER::instance().LoadSettingFile(0, szFilename, setting);
	g_pkMapSectree = SECTREE_MANAGER::instance().BuildSectreeFromSetting(setting);
	int iMapHeight = setting.iHeight / 128 / 200;
	int iMapWidth = setting.iWidth / 128 / 200;

	if (iMapHeight < 0 || iMapWidth < 0)
	{
		printf("SYSERR: setting.w/h %d %d, base %d %d\n", setting.iWidth, setting.iHeight, setting.iBaseX, setting.iBaseY);
		return;
	}

	int bx = setting.iBaseX;
	int by = setting.iBaseY;

	printf("w %d h %d\n", iMapWidth, iMapHeight);
	bx /= SECTREE_SIZE;
	by /= SECTREE_SIZE;
	g_bx = bx;
	g_by = by;

	printf("bx by %d %d\n", bx, by);
	int x, y;

	// 일단 속성 초기화
	for (y = 0; y < iMapHeight * (25600 / SECTREE_SIZE); ++y)
	{
		for (x = 0; x < iMapWidth * (25600 / SECTREE_SIZE); ++x)
		{
			SECTREEID id;

			id.coord.x = x + bx;
			id.coord.y = y + by;

			g_pkMapSectree->Find(id.package)->BindAttribute(M2_NEW CAttribute(SECTREE_SIZE / CELL_SIZE, SECTREE_SIZE / CELL_SIZE));
		}
	}

	ReadMapRecursive(c_pszMapDirectory);

	printf("iMapWidth %d iMapHeight %d\n", iMapWidth, iMapHeight);

	ReadCollisionData(c_pszCollisionDataFileName, setting.iBaseX, setting.iBaseY);

	SECTREE_MANAGER::instance().SaveAttributeToImage(0, "test.tga", g_pkMapSectree);

	snprintf(szFilename, sizeof(szFilename), "%s/server_attr", c_pszMapDirectory);

	FILE * fp = fopen(szFilename, "wb");

	if (!fp)
	{
		sys_err("cannot open %s for writing", szFilename);
		return;
	}

	iMapWidth *= 4;
	iMapHeight *= 4;

	fwrite(&iMapWidth, sizeof(int), 1, fp);
	fwrite(&iMapHeight, sizeof(int), 1, fp);

	unsigned int uiBlockSize = sizeof(DWORD) * (SECTREE_SIZE / CELL_SIZE) * (SECTREE_SIZE / CELL_SIZE);
	unsigned int uiMemSize = iMapWidth * iMapHeight;
	unsigned int uiCompSize;

	printf("Attribute size: %u block %u\n", uiMemSize, uiBlockSize);
	char * pAttr = (char *) malloc(LZOManager::instance().GetMaxCompressedSize(sizeof(DWORD) * (SECTREE_SIZE / CELL_SIZE) * (SECTREE_SIZE / CELL_SIZE)));

	for (y = 0; y < iMapHeight; ++y)
		for (x = 0; x < iMapWidth; ++x)
		{
			SECTREEID id;

			id.coord.x = x+bx;
			id.coord.y = y+by;

			LPSECTREE pSec = g_pkMapSectree->Find(id.package);

			LZOManager::instance().Compress((unsigned char *) pSec->GetAttributePtr()->GetDataPtr(),
					uiBlockSize,
					(unsigned char *) pAttr,
					&uiCompSize);

			fwrite(&uiCompSize, sizeof(int), 1, fp);
			fwrite(pAttr, sizeof(char), uiCompSize, fp);
		}

	fclose(fp);
	free(pAttr);
}

void ConvertAttribute2(const char * filename)
{
	std::string newFileName = std::string(filename) + ".new";
	FILE * fp = fopen(filename, "rb");
	FILE * wfp = fopen(newFileName.c_str(), "wb");

	if (!fp)
	{
		printf("cannot open %s\n", filename);
		return;
	}

	if (!wfp)
	{
		printf("cannot open %s\n", newFileName.c_str());
		return;
	}

	unsigned int uiSize;
	unsigned int uiDestSize;

	size_t maxMemSize = LZOManager::instance().GetMaxCompressedSize(sizeof(DWORD) * (SECTREE_SIZE / CELL_SIZE) * (SECTREE_SIZE / CELL_SIZE));

	BYTE abComp[maxMemSize];
	DWORD * attr = M2_NEW DWORD[maxMemSize];

	int iWidth, iHeight;

	fread(&iWidth, sizeof(int), 1, fp);
	fread(&iHeight, sizeof(int), 1, fp);

	fwrite(&iWidth, sizeof(int), 1, wfp);
	fwrite(&iHeight, sizeof(int), 1, wfp);

	printf("width %d height %d\n", iWidth, iHeight);
	sleep(1);

	for (int y = 0; y < iHeight; ++y)
		for (int x = 0; x < iWidth; ++x)
		{
			fread(&uiSize, sizeof(int), 1, fp);
			fread(abComp, sizeof(char), uiSize, fp);

			printf("orig_size %d ", uiSize);
			uiDestSize = sizeof(DWORD) * maxMemSize;
			LZOManager::instance().Decompress(abComp, uiSize, (BYTE *) attr, &uiDestSize);

			if (uiDestSize != sizeof(DWORD) * (SECTREE_SIZE / CELL_SIZE) * (SECTREE_SIZE / CELL_SIZE))
			{
				printf("SECTREE_MANAGER::LoadAttribte: %s: size mismatch! %d\n", filename, uiDestSize);
				fclose(fp);

				M2_DELETE_ARRAY(attr);
				continue;
			}

			bool found = false;

			for (int i = 0; i < (SECTREE_SIZE / CELL_SIZE) * (SECTREE_SIZE / CELL_SIZE); ++i)
				if (IS_SET(attr[i], 0xFFFFFFF8))
				{
					if (!found)
					{
						printf("!!! ");
						found = true;
					}

					REMOVE_BIT(attr[i], 0xFFFFFFF8);
				}

			LZOManager::instance().Compress((unsigned char *) attr, uiDestSize, abComp, &uiDestSize);
			printf("new_size %d\n", uiDestSize);

			fwrite(&uiDestSize, sizeof(int), 1, wfp);
			fwrite(abComp, uiDestSize, 1, wfp);
		}

	fclose(fp);
	fclose(wfp);
}

