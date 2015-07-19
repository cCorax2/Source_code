#include "stdafx.h"
#include "locale_service.h"
#include "constants.h"
#include "banword.h"
#include "utils.h"
#include "mob_manager.h"
#include "empire_text_convert.h"
#include "config.h"
#include "skill_power.h"

using namespace std;

extern string		g_stQuestDir;
extern set<string> 	g_setQuestObjectDir;

string g_stServiceName;
string g_stServiceBasePath = ".";
string g_stServiceMapPath = "data/map";

string g_stLocale = "euckr";
string g_stLocaleFilename;

int PK_PROTECT_LEVEL = 30;

string 			g_stLocal = "";
eLocalization 	g_eLocalType = LC_NOSET;

int (*check_name) (const char * str) = NULL;
int (*is_twobyte) (const char * str) = NULL;
bool LC_InitLocalization( const std::string& szLocal );

int is_twobyte_euckr(const char * str)
{
	return ishan(*str);
}

int is_twobyte_gb2312(const char * str)
{
	if (!str || !*str)
		return 0;

	BYTE b1 = str[0];
	BYTE b2 = str[1];

	if (!(b1 & 0x80))
		return 0;

	if (b1 < 0xb0 || b1 > 0xf7 || b2 < 0xa1 || b2 > 0xfe)
		return 0;

	return 1;
}

int is_twobyte_big5(const char * str)
{
	if (!str || !*str)
		return 0;

	BYTE b1 = str[0];
	BYTE b2 = str[1];
			
	BYTE b[2];

	b[0] = b2;
	b[1] = b1;
	
	WORD b12 = 0;
	memcpy(&b12, b, 2);

	if (!(b1 & 0x80))
		return 0;

	if ((b12 < 0xa440 || b12 > 0xc67e) && (b12 < 0xc940 || b12 > 0xf9d5))  
	{
		if (test_server)
		{
			sys_log(0, "twobyte_big5 %x %x", b1, b2);
		}
		return 0;
	}

	return 1;
}

int check_name_independent(const char * str)
{
	if (CBanwordManager::instance().CheckString(str, strlen(str)))
		return 0;

	// 몬스터 이름으로는 만들 수 없다.
	char szTmp[256];
	str_lower(str, szTmp, sizeof(szTmp));

	if (CMobManager::instance().Get(szTmp, false))
		return 0;

	return 1;
}

int check_name_gb2312(const char * str)
{
	static const BYTE exceptions[5][2] =
	{
		{ 0xd7, 0xfa },
		{ 0xd7, 0xfb },
		{ 0xd7, 0xfc },
		{ 0xd7, 0xfd },
		{ 0xd7, 0xfe }
	};

	int         i, j;
	BYTE        b1, b2;

	if (!str || !*str)
		return 0;

	i = 0;

	size_t len = 0;

	while (str[i])
	{
		if (str[i] & 0x80)
		{
			if (!str[i + 1])
				return 0;

			b1 = str[i++];
			b2 = str[i++];

			// 중국 간체는 첫번째 바이트 범위가 b0 -> f7 까지고
			// 두번째 바이트 범위가 a1 -> fe 다.
			if (b1 < 0xb0 || b1 > 0xf7 || b2 < 0xa1 || b2 > 0xfe)
				return 0;

			// 예외가 있다.
			for (j = 0; j < 5; j++)
				if (b1 == exceptions[j][0] && b2 == exceptions[j][1])
					return 0;

			len++;
		}
		else
		{
			if (!isdigit(str[i]) && !isalpha(str[i]))
				return 0;

			i++;
			len++;
		}
	}

	if ( len > 6 ) return 0;

	return check_name_independent(str);
}

int check_name_big5(const char * str )
{
	int i;
	BYTE b1, b2;

	if (!str || !*str)
		return 0;

	i = 0;

	while (str[i])
	{
		if (str[i] & 0x80)
		{
			if (!str[i + 1])
				return 0;

			b1 = str[i++];
			b2 = str[i++];

			BYTE b[2];

			b[0] = b2;
			b[1] = b1;

			// 중국 번체 ( big5 : 홍콩 )
			// 범위는 다음과 같다.
			//  big5: 0xA140--0xF9D5
			//  extended big5: 0x40--0x7E and 0xA1--0xFE

			// 0xa440-0xC67E
			// 0xC940-0xF9D5
			//
			/*	
				Plan Code Range Description 
				1 A140H - A3E0H Symbol and Chinese Control Code 
				1 A440H - C67EH Commonly Used Characters 
				2 C940H - F9D5H Less Commonly Used Characters 
				UDF FA40H - FEFE User-Defined Characters 

				8E40H - A0FEH User-Defined Characters 

				8140H - 8DFEH User-Defined Characters 

				8181H - 8C82H User-Defined Characters 

				F9D6H - F9F1H User-Defined Characters 
			*/

			WORD b12 = 0;
			memcpy(&b12, b, 2);

			if ((b12 < 0xa440 || b12 > 0xc67e) && (b12 < 0xc940 || b12 > 0xf9d5)) 
			{
				if (test_server)
					sys_log(0, "check_name_big5[%d][%s] %x %x %x", i - 2, str, b1, b2, b12);

				return 0;
			}
		}
		else
		{
			if (!isdigit(str[i]) && !isalpha(str[i]))
				return 0;

			i++;
		}
	}

	return check_name_independent(str);
}

int check_name_euckr(const char * str)
{
	int		code;
	const char*	tmp;

	if (!str || !*str)
		return 0;

	if ( strlen(str) < 2 || strlen(str) > 12 )
		return 0;

	for (tmp = str; *tmp; ++tmp)
	{
		// 한글이 아니고 빈칸이면 잘못된 것
		if (isnhspace(*tmp))
			return 0;

		// 한글이 아니고 숫자라면 적합하다.
		if (isnhdigit(*tmp))
			continue;

		// 한글이 아니고 영문이라면 적합하다.   
		if (!ishan(*tmp) && isalpha(*tmp))
			continue;

		code = *tmp;
		code += 256;

		if (code < 176 || code > 200)
			return 0;

		++tmp;

		if (!*tmp)
			break;
	}

	return check_name_independent(str);
}

int check_name_latin1(const char * str)
{
	int		code;
	const char*	tmp;

	if (!str || !*str)
		return 0;

	if (strlen(str) < 2)
		return 0;

	for (tmp = str; *tmp; ++tmp)
	{
		// 한글이 아니고 빈칸이면 잘못된 것
		if (isnhspace(*tmp))
			return 0;

		// 한글이 아니고 숫자라면 적합하다.
		if (isnhdigit(*tmp))
			continue;

		// 한글이 아니고 영문이라면 적합하다.   
		if (!ishan(*tmp) && isalpha(*tmp))
			continue;

		unsigned char uc_tmp = *tmp;

		if (uc_tmp == 145 || uc_tmp == 146 || uc_tmp == 196
				|| uc_tmp == 214 || uc_tmp == 220 || uc_tmp == 223 
				|| uc_tmp == 228 || uc_tmp == 246 || uc_tmp == 252 ) 
			continue;
		code = *tmp;
		code += 256;

		if (code < 176 || code > 200)
			return 0;

		++tmp;

		if (!*tmp)
			break;
	}

	return check_name_independent(str);
}

int check_name_alphabet(const char * str)
{
	const char*	tmp;

	if (!str || !*str)
		return 0;

	if (strlen(str) < 2)
		return 0;

	for (tmp = str; *tmp; ++tmp)
	{
		// 알파벳과 수자만 허용
		if (isdigit(*tmp) || isalpha(*tmp))
			continue;
		else
			return 0;
	}

	return check_name_independent(str);
}
// DISABLE_SPECIAL_CHAR_NAMING
bool sjis_is_disable_name_char(const char* src)
{
	static const char* sjis_symbols = "?";

	if (strncmp(src, sjis_symbols, 2) == 0)
		return true;

	return false;
}
// END_OF_DISABLE_SPECIAL_CHAR_NAMING



//-----------------------------------------------
// CHECK SJIS STRING
//-----------------------------------------------
#define issjishead(c) ((0x81<=(c) && (c)<=0x9f) || \
		                               ((0xe0<=(c)) && (c)<=0xfc))
#define issjistail(c) ((0x40<=(c) && (c)<=0x7e) || \
		                               (0x80<=(c) && (c)<=0xfc))

static int is_char_sjis(const char *p, const char *e)
{
	return (issjishead((BYTE) *p) && (e-p)>1 && issjistail((BYTE)p[1]) ? true : false);
}

int is_twobyte_sjis(const char *str)
{
	if (str && str[0] && str[1])
		return issjishead((BYTE)str[0]) && issjistail((BYTE)str[1]);
	else
		return 0;
}

int check_name_sjis(const char *str)
{
	const char	*p = str;
	const char	*e = str + strlen(str);	// NULL position

	// 일본은 캐릭터 이름길이 16byte 까지
	if ( strlen(str) < 2 || strlen(str) > 16 )
		return 0;

	while (*p)
	{
		if (is_char_sjis(p, e))
		{
			// DISABLE_SPECIAL_CHAR_NAMING
			if (sjis_is_disable_name_char(p))
				return false;

			// END_OF_DISABLE_SPECIAL_CHAR_NAMING
			// 이문자는 허용되지 않는다.
			if ((BYTE)p[0]==0x81 && (BYTE)p[1]==0x40) return false;

			p += 2;
			continue;
		}
		else
		{
			// 영문이나 수자는 허용한다.
			if (isalpha(*p) || isdigit(*p))
			{
				p += 1;
				continue;
			}
			else
			{
				return 0;
			}
		}
	}


	return check_name_independent(str);
}
//-----------------------------------------------
// END OF CHECK SJIS STRING
//-----------------------------------------------
void LocaleService_LoadLocaleStringFile()
{
	if (g_stLocaleFilename.empty())
		return;

	if (g_bAuthServer)
		return;

	fprintf(stderr, "LocaleService %s\n", g_stLocaleFilename.c_str());

	locale_init(g_stLocaleFilename.c_str());
}

void LocaleService_LoadEmpireTextConvertTables()
{
	char szFileName[256];

	for (int iEmpire = 1; iEmpire <= 3; ++iEmpire)
	{
		snprintf(szFileName, sizeof(szFileName), "%s/lang%d.cvt", LocaleService_GetBasePath().c_str(), iEmpire);
		sys_log(0, "Load %s", szFileName);

		LoadEmpireTextConvertTable(iEmpire, szFileName);
	}
}

static void __LocaleService_Init_DEFAULT()
{
	g_stLocaleFilename = "";

	g_stServiceBasePath = "locale/" + g_stServiceName;
	g_stServiceMapPath = g_stServiceBasePath + "/map";
	g_stQuestDir = g_stServiceBasePath + "/quest";

	g_setQuestObjectDir.clear();	
	g_setQuestObjectDir.insert(g_stQuestDir + "/object");
}

static void __LocaleService_Init_JAPAN()
{
	g_stLocale = "sjis";
	g_stServiceBasePath = "locale/japan";
	g_stQuestDir = "locale/japan/quest";
	g_stServiceMapPath = "locale/japan/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/japan/quest/object");
	g_stLocaleFilename = "locale/japan/sjis_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_sjis;
	is_twobyte = is_twobyte_sjis;
	exp_table = exp_table_euckr;
}

static void __LocaleService_Init_English()
{
	g_stLocale = "";
	g_stServiceBasePath = "locale/english";
	g_stQuestDir = "locale/english/quest";
	g_stServiceMapPath = "locale/english/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/english/quest/object");
	g_stLocaleFilename = "locale/english/eng_string.txt";

	g_iUseLocale = TRUE;
	check_name = check_name_alphabet;
}

static void __LocaleService_Init_HongKong()
{
	g_stLocale = "big5";
	g_stServiceBasePath = "locale/hongkong";
	g_stQuestDir = "locale/hongkong/quest";
	g_stServiceMapPath = "locale/hongkong/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/hongkong/quest/object");
	g_stLocaleFilename = "locale/hongkong/big5_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_big5;
	is_twobyte = is_twobyte_big5;
}

static void __LocaleService_Init_NewCIBN()
{
	g_stLocale = "gb2312";
	g_stServiceBasePath = "locale/newcibn";
	g_stQuestDir = "locale/newcibn/quest";
	g_stServiceMapPath = "locale/newcibn/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/newcibn/quest/object");
	g_stLocaleFilename = "locale/newcibn/locale_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_gb2312;
	is_twobyte = is_twobyte_gb2312;
	//exp_table = exp_table_newcibn;
}

static void __LocaleService_Init_Germany()
{
	g_stLocale="latin1";
	g_stServiceBasePath = "locale/germany";
	g_stQuestDir = "locale/germany/quest";
	g_stServiceMapPath = "locale/germany/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/germany/quest/object");
	g_stLocaleFilename = "locale/germany/locale_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_alphabet;
	
	PK_PROTECT_LEVEL = 15;
}

static void __LocaleService_Init_Korea()
{
	g_stLocale="euckr";
	g_stServiceBasePath = "locale/korea";
	g_stQuestDir = "locale/korea/quest";
	g_stServiceMapPath = "locale/korea/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/korea/quest/object");

	g_iUseLocale = TRUE;
	exp_table = exp_table_euckr;
}

static void __LocaleService_Init_France()
{
	g_stLocale="latin1";
	g_stServiceBasePath = "locale/france";
	g_stQuestDir = "locale/france/quest";
	g_stServiceMapPath = "locale/france/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/france/quest/object");
	g_stLocaleFilename = "locale/france/locale_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_alphabet;
	
	PK_PROTECT_LEVEL = 15;
}

static void __LocaleService_Init_Italy()
{
	g_stLocale="latin1";
	g_stServiceBasePath = "locale/italy";
	g_stQuestDir = "locale/italy/quest";
	g_stServiceMapPath = "locale/italy/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/italy/quest/object");
	g_stLocaleFilename = "locale/italy/locale_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_alphabet;
	
	PK_PROTECT_LEVEL = 15;
}

static void __LocaleService_Init_spain()
{
	g_stLocale="latin1";
	g_stServiceBasePath = "locale/spain";
	g_stQuestDir = "locale/spain/quest";
	g_stServiceMapPath = "locale/spain/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/spain/quest/object");
	g_stLocaleFilename = "locale/spain/locale_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_alphabet;
	
	PK_PROTECT_LEVEL = 15;
}

static void __LocaleService_Init_greek()
{
	g_stLocale="greek";
	g_stServiceBasePath = "locale/greek";
	g_stQuestDir = "locale/greek/quest";
	g_stServiceMapPath = "locale/greek/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/greek/quest/object");
	g_stLocaleFilename = "locale/greek/locale_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_alphabet;
	
	PK_PROTECT_LEVEL = 15;
}

static void __LocaleService_Init_UK()
{
	g_stLocale="latin1";
	g_stServiceBasePath = "locale/uk";
	g_stQuestDir = "locale/uk/quest";
	g_stServiceMapPath = "locale/uk/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/uk/quest/object");
	g_stLocaleFilename = "locale/uk/locale_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_alphabet;
	
	PK_PROTECT_LEVEL = 15;
}

static void __LocaleService_Init_Turkey()
{
	g_stLocale="latin5";
	g_stServiceBasePath = "locale/turkey";
	g_stQuestDir = "locale/turkey/quest";
	g_stServiceMapPath = "locale/turkey/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/turkey/quest/object");
	g_stLocaleFilename = "locale/turkey/locale_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_alphabet;
	
	PK_PROTECT_LEVEL = 15;
}

static void __LocaleService_Init_Poland()
{
	g_stLocale="latin2";
	g_stServiceBasePath = "locale/poland";
	g_stQuestDir = "locale/poland/quest";
	g_stServiceMapPath = "locale/poland/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/poland/quest/object");
	g_stLocaleFilename = "locale/poland/locale_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_alphabet;
	
	PK_PROTECT_LEVEL = 15;
}

static void __LocaleService_Init_Portugal() 
{
	g_stLocale="latin1";
	g_stServiceBasePath = "locale/portugal";
	g_stQuestDir = "locale/portugal/quest";
	g_stServiceMapPath = "locale/portugal/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/portugal/quest/object");
	g_stLocaleFilename = "locale/portugal/locale_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_alphabet;
	
	PK_PROTECT_LEVEL = 15;
}

static void __LocaleService_Init_Canada()
{
	g_stLocale = "latin1";
	g_stServiceBasePath = "locale/canada";
	g_stQuestDir		= "locale/canada/quest";
	g_stServiceMapPath	= "locale/canada/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/canada/quest/object");
	g_stLocaleFilename = "locale/canada/locale_string.txt";

	check_name	= check_name_alphabet;

	g_iUseLocale = TRUE;
}

static void __LocaleService_Init_Brazil()
{
	g_stLocale = "latin1";
	g_stServiceBasePath = "locale/brazil";
	g_stQuestDir		= "locale/brazil/quest";
	g_stServiceMapPath	= "locale/brazil/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/brazil/quest/object");
	g_stLocaleFilename = "locale/brazil/locale_string.txt";

	check_name = check_name_alphabet;

	g_iUseLocale = TRUE;
}

static void __LocaleService_Init_YMIR()
{
	g_stLocaleFilename = "";

	g_stServiceBasePath = "locale/" + g_stServiceName;
	g_stServiceMapPath = g_stServiceBasePath + "/map";
	g_stQuestDir = g_stServiceBasePath + "/quest";

	g_setQuestObjectDir.clear();	
	g_setQuestObjectDir.insert(g_stQuestDir + "/object");

	PK_PROTECT_LEVEL = 30;

	exp_table = exp_table_euckr;
}

static void __LocaleService_Init_Russia() 
{
	g_stLocale="cp1251";
	g_stServiceBasePath = "locale/russia";
	g_stQuestDir = "locale/russia/quest";
	g_stServiceMapPath = "locale/russia/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/russia/quest/object");
	g_stLocaleFilename = "locale/russia/locale_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_alphabet;
	
	PK_PROTECT_LEVEL = 15;
}

static void __LocaleService_Init_Denmark() 
{
	g_stLocale="latin1";
	g_stServiceBasePath = "locale/denmark";
	g_stQuestDir = "locale/denmark/quest";
	g_stServiceMapPath = "locale/denmark/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/denmark/quest/object");
	g_stLocaleFilename = "locale/denmark/locale_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_alphabet;
	
	PK_PROTECT_LEVEL = 15;
}

static void __LocaleService_Init_Bulgaria() 
{
	g_stLocale="cp1251";
	g_stServiceBasePath = "locale/bulgaria";
	g_stQuestDir = "locale/bulgaria/quest";
	g_stServiceMapPath = "locale/bulgaria/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/bulgaria/quest/object");
	g_stLocaleFilename = "locale/bulgaria/locale_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_alphabet;
	
	PK_PROTECT_LEVEL = 15;
}

static void __LocaleService_Init_Croatia() 
{
	g_stLocale="cp1251";
	g_stServiceBasePath = "locale/croatia";
	g_stQuestDir = "locale/croatia/quest";
	g_stServiceMapPath = "locale/croatia/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/croatia/quest/object");
	g_stLocaleFilename = "locale/croatia/locale_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_alphabet;
	
	PK_PROTECT_LEVEL = 15;
}

static void __LocaleService_Init_Mexico() 
{
	g_stLocale="latin1";
	g_stServiceBasePath = "locale/mexico";
	g_stQuestDir = "locale/mexico/quest";
	g_stServiceMapPath = "locale/mexico/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/mexico/quest/object");
	g_stLocaleFilename = "locale/mexico/locale_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_alphabet;
	
	PK_PROTECT_LEVEL = 15;
}

static void __LocaleService_Init_Arabia() 
{
	g_stLocale="cp1256";
	g_stServiceBasePath = "locale/arabia";
	g_stQuestDir = "locale/arabia/quest";
	g_stServiceMapPath = "locale/arabia/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/arabia/quest/object");
	g_stLocaleFilename = "locale/arabia/locale_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_alphabet;
	
	PK_PROTECT_LEVEL = 15;
}

static void __LocaleService_Init_Czech()
{
	g_stLocale="latin2";
	g_stServiceBasePath = "locale/czech";
	g_stQuestDir = "locale/czech/quest";
	g_stServiceMapPath = "locale/czech/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/czech/quest/object");
	g_stLocaleFilename = "locale/czech/locale_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_alphabet;
	
	PK_PROTECT_LEVEL = 15;
}

static void __LocaleService_Init_Hungary()
{
	g_stLocale="latin2";
	g_stServiceBasePath = "locale/hungary";
	g_stQuestDir = "locale/hungary/quest";
	g_stServiceMapPath = "locale/hungary/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/hungary/quest/object");
	g_stLocaleFilename = "locale/hungary/locale_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_alphabet;
	
	PK_PROTECT_LEVEL = 15;
}

static void __LocaleService_Init_Romania()
{
	g_stLocale="latin2";
	g_stServiceBasePath = "locale/romania";
	g_stQuestDir = "locale/romania/quest";
	g_stServiceMapPath = "locale/romania/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/romania/quest/object");
	g_stLocaleFilename = "locale/romania/locale_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_alphabet;
	
	PK_PROTECT_LEVEL = 15;
}

static void __LocaleService_Init_Netherlands()
{
	g_stLocale="latin1";
	g_stServiceBasePath = "locale/netherlands";
	g_stQuestDir = "locale/netherlands/quest";
	g_stServiceMapPath = "locale/netherlands/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/netherlands/quest/object");
	g_stLocaleFilename = "locale/netherlands/locale_string.txt";

	g_iUseLocale = TRUE;

	check_name = check_name_alphabet;
	
	PK_PROTECT_LEVEL = 15;
}

static void __LocaleService_Init_Singapore()
{
	g_stLocale = "latin1";
	g_stServiceBasePath = "locale/singapore";
	g_stQuestDir		= "locale/singapore/quest";
	g_stServiceMapPath	= "locale/singapore/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/singapore/quest/object");
	g_stLocaleFilename = "locale/singapore/locale_string.txt";

	check_name	= check_name_alphabet;

	g_iUseLocale = TRUE;
	exp_table = exp_table_newcibn;
}

static void __LocaleService_Init_Vietnam()
{
	g_stLocale 			= "latin1";
	g_stServiceBasePath = "locale/vietnam";
	g_stQuestDir		= "locale/vietnam/quest";
	g_stServiceMapPath	= "locale/vietnam/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/vietnam/quest/object");
	g_stLocaleFilename = "locale/vietnam/locale_string.txt";

	check_name	= check_name_alphabet;

	g_iUseLocale = TRUE;
	exp_table = exp_table_newcibn;

}

static void __LocaleService_Init_Thailand()
{
	g_stLocale 			= "latin1";
	g_stServiceBasePath = "locale/thailand";
	g_stQuestDir		= "locale/thailand/quest";
	g_stServiceMapPath	= "locale/thailand/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/thailand/quest/object");
	g_stLocaleFilename = "locale/thailand/locale_string.txt";

	check_name	= check_name_alphabet;

	g_iUseLocale = TRUE;
}

static void __LocaleService_Init_USA()
{
	g_stLocale = "latin1";
	g_stServiceBasePath = "locale/usa";
	g_stQuestDir = "locale/usa/quest";
	g_stServiceMapPath = "locale/usa/map";

	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("locale/usa/quest/object");
	g_stLocaleFilename = "locale/usa/locale_string.txt";

	g_iUseLocale = TRUE;
	check_name = check_name_alphabet;
}

// World Edition version for korea
static void __LocaleService_Init_WE_Korea()
{
	g_stLocale = "euckr";

//	g_stLocaleFilename = "locale/we_korea/locale_string.txt";

	g_stServiceBasePath = "locale/" + g_stServiceName;
	g_stServiceMapPath = g_stServiceBasePath + "/map";
	g_stQuestDir = g_stServiceBasePath + "/quest";

	g_setQuestObjectDir.clear();	
	g_setQuestObjectDir.insert(g_stQuestDir + "/object");

	g_iUseLocale = TRUE;

	PK_PROTECT_LEVEL = 15;
}

static void __LocaleService_Init_Taiwan()
{
	g_stLocale = "big5";
	g_stServiceBasePath = "locale/" + g_stServiceName;
	g_stServiceMapPath = g_stServiceBasePath + "/map";
	g_stQuestDir = g_stServiceBasePath + "/quest";

	g_setQuestObjectDir.clear();	
	g_setQuestObjectDir.insert(g_stQuestDir + "/object");
	g_stLocaleFilename = "locale/taiwan/locale_string.txt";

	check_name = check_name_big5;
	is_twobyte = is_twobyte_big5;

	g_iUseLocale = TRUE;
	
	PK_PROTECT_LEVEL = 15;
}

static void __CheckPlayerSlot(const std::string& service_name)
{
	if (PLAYER_PER_ACCOUNT != 4)
	{
		printf("<ERROR> PLAYER_PER_ACCOUNT = %d\n", PLAYER_PER_ACCOUNT);
		exit(0);
	}
}

bool LocaleService_Init(const std::string& c_rstServiceName)
{
	if (!g_stServiceName.empty())
	{
		sys_err("ALREADY exist service");
		return false;
	}

	g_stServiceName = c_rstServiceName;

	if ( "japan" == g_stServiceName)
	{
		__LocaleService_Init_JAPAN();
	}
	else if ( "english" == g_stServiceName)
	{
		__LocaleService_Init_English();
	}
	else if ( "hongkong" == g_stServiceName)
	{
		__LocaleService_Init_HongKong();
	}
	else if ( "newcibn" == g_stServiceName)
	{
		__LocaleService_Init_NewCIBN();
	}
	else if ( "germany" == g_stServiceName)
	{
		__LocaleService_Init_Germany();
	}
	else if ( "korea" == g_stServiceName)
	{
		__LocaleService_Init_Korea();
	}
	else if ( "france" == g_stServiceName)
	{
		__LocaleService_Init_France();
	}
	else if ( "italy" == g_stServiceName)
	{
		__LocaleService_Init_Italy();
	}
	else if ( "spain" == g_stServiceName)
	{
		__LocaleService_Init_spain();
	}
	else if ( "greek" == g_stServiceName)
	{
		__LocaleService_Init_greek();
	}
	else if ( "uk" == g_stServiceName)
	{
		__LocaleService_Init_UK();
	}
	else if ( "turkey" == g_stServiceName)
	{
		__LocaleService_Init_Turkey();
	}
	else if ( "poland" == g_stServiceName)
	{
		__LocaleService_Init_Poland();
	}
	else if ( "portugal" == g_stServiceName)
	{
		__LocaleService_Init_Portugal();
	}
	else if ( "canada" == g_stServiceName)
	{
		__LocaleService_Init_Canada();
	}
	else if ( "brazil" == g_stServiceName)
	{
		__LocaleService_Init_Brazil();
	}
	else if ( "ymir" == g_stServiceName)
	{
		__LocaleService_Init_YMIR();
	}
	else if ( "russia" == g_stServiceName)
	{
		__LocaleService_Init_Russia();
	}
	else if ( "denmark" == g_stServiceName)
	{
		__LocaleService_Init_Denmark();
	}
	else if ( "bulgaria" == g_stServiceName)
	{
		__LocaleService_Init_Bulgaria();
	}
	else if ( "croatia" == g_stServiceName)
	{
		__LocaleService_Init_Croatia();
	}
	else if ( "mexico" == g_stServiceName)
	{
		__LocaleService_Init_Mexico();
	}
	else if ( "arabia" == g_stServiceName)
	{
		__LocaleService_Init_Arabia();
	}
	else if ( "czech" == g_stServiceName)
	{
		__LocaleService_Init_Czech();
	}
	else if ( "romania" == g_stServiceName)
	{
		__LocaleService_Init_Romania();
	}
	else if ( "hungary" == g_stServiceName)
	{
		__LocaleService_Init_Hungary();
	}
	else if ( "netherlands" == g_stServiceName)
	{
		__LocaleService_Init_Netherlands();
	}
	else if ( "singapore" == g_stServiceName)
	{
		__LocaleService_Init_Singapore();
	}
	else if ( "vietnam" == g_stServiceName)
	{
		__LocaleService_Init_Vietnam();
	}
	else if ( "thailand" == g_stServiceName)
	{
		__LocaleService_Init_Thailand();
	}
	else if ("usa" == g_stServiceName)
	{
		__LocaleService_Init_USA();
	}
	else if ("we_korea" == g_stServiceName)
	{
		__LocaleService_Init_WE_Korea(); // ver.World Edition for korea
	}
	else if ("taiwan" == g_stServiceName)
	{
		__LocaleService_Init_Taiwan();
	}
	else
	{
		__LocaleService_Init_DEFAULT();
	}

	fprintf(stdout, "Setting Locale \"%s\" (Path: %s)\n", g_stServiceName.c_str(), g_stServiceBasePath.c_str());

	__CheckPlayerSlot(g_stServiceName);

	if (false == LC_InitLocalization(c_rstServiceName))
		return false;
	
	return true;
}

void LocaleService_TransferDefaultSetting()
{
	if (!check_name)
		check_name = check_name_euckr;

	if (!is_twobyte)
		is_twobyte = is_twobyte_euckr;

	if (!exp_table)
		exp_table = exp_table_common;

	if (!CTableBySkill::instance().Check())
		exit(1);

	if (!aiPercentByDeltaLevForBoss)
		aiPercentByDeltaLevForBoss = aiPercentByDeltaLevForBoss_euckr;

	if (!aiPercentByDeltaLev)
		aiPercentByDeltaLev = aiPercentByDeltaLev_euckr;

	if (!aiChainLightningCountBySkillLevel)
		aiChainLightningCountBySkillLevel = aiChainLightningCountBySkillLevel_euckr;
}

const std::string& LocaleService_GetBasePath()
{
	return g_stServiceBasePath;
}

const std::string& LocaleService_GetMapPath()
{
	return g_stServiceMapPath;
}

const std::string& LocaleService_GetQuestPath()
{
	return g_stQuestDir;
}

bool LC_InitLocalization( const std::string& szLocal )
{
	g_stLocal = szLocal;

	if ( !g_stLocal.compare("ymir") )
		g_eLocalType = LC_YMIR;
	else if ( !g_stLocal.compare("japan") )
		g_eLocalType = LC_JAPAN;
	else if ( !g_stLocal.compare("english") )
		g_eLocalType = LC_ENGLISH;
	else if ( !g_stLocal.compare("hongkong") )
		g_eLocalType = LC_HONGKONG;
	else if (!g_stLocal.compare("newcibn") )
		g_eLocalType = LC_NEWCIBN;
	else if ( !g_stLocal.compare("germany") )
		g_eLocalType = LC_GERMANY;
	else if ( !g_stLocal.compare("korea") )
		g_eLocalType = LC_KOREA;
	else if ( !g_stLocal.compare("france") )
		g_eLocalType = LC_FRANCE;
	else if ( !g_stLocal.compare("italy") )
		g_eLocalType = LC_ITALY;
	else if ( !g_stLocal.compare("spain") )
		g_eLocalType = LC_SPAIN;
	else if ( !g_stLocal.compare("greek") )
		g_eLocalType = LC_GREEK;
	else if ( !g_stLocal.compare("uk") )
		g_eLocalType = LC_UK;
	else if ( !g_stLocal.compare("turkey") )
		g_eLocalType = LC_TURKEY;
	else if ( !g_stLocal.compare("poland") )
		g_eLocalType = LC_POLAND;
	else if ( !g_stLocal.compare("portugal") )
		g_eLocalType = LC_PORTUGAL;
	else if ( !g_stLocal.compare("canada") )
		g_eLocalType = LC_CANADA;
	else if ( !g_stLocal.compare("brazil") )
		g_eLocalType = LC_BRAZIL;
	else if ( !g_stLocal.compare("russia") )
		g_eLocalType = LC_RUSSIA;
	else if ( !g_stLocal.compare("denmark") )
		g_eLocalType = LC_DENMARK;
	else if ( !g_stLocal.compare("bulgaria") )
		g_eLocalType = LC_BULGARIA;
	else if ( !g_stLocal.compare("croatia") )
		g_eLocalType = LC_CROATIA;
	else if ( !g_stLocal.compare("mexico") )
		g_eLocalType = LC_MEXICO;
	else if ( !g_stLocal.compare("arabia") )
		g_eLocalType = LC_ARABIA;
	else if ( !g_stLocal.compare("czech") )
		g_eLocalType = LC_CZECH;
	else if ( !g_stLocal.compare("romania") )
		g_eLocalType = LC_ROMANIA;
	else if ( !g_stLocal.compare("hungary") )
		g_eLocalType = LC_HUNGARY;
	else if ( !g_stLocal.compare("netherlands") )
		g_eLocalType = LC_NETHERLANDS;
	else if ( !g_stLocal.compare("singapore") )
		g_eLocalType = LC_SINGAPORE;
	else if ( !g_stLocal.compare("vietnam") )
		g_eLocalType = LC_VIETNAM;
	else if ( !g_stLocal.compare("thailand") )
		g_eLocalType = LC_THAILAND;
	else if ( !g_stLocal.compare("usa") )
		g_eLocalType = LC_USA;
	else if ( !g_stLocal.compare("we_korea") ) // ver.WorldEdition for korea
		g_eLocalType = LC_WE_KOREA;
	else if ( !g_stLocal.compare("taiwan") ) 
		g_eLocalType = LC_TAIWAN;
	else
		return false;

	return true;
}

eLocalization LC_GetLocalType() 
{
	return g_eLocalType;
}

bool LC_IsLocale( const eLocalization t ) 
{
	return LC_GetLocalType() == t ? true : false;
}

bool LC_IsYMIR()		{ return LC_GetLocalType() == LC_YMIR ? true : false; }
bool LC_IsJapan()		{ return LC_GetLocalType() == LC_JAPAN ? true : false; }
bool LC_IsEnglish()		{ return LC_GetLocalType() == LC_ENGLISH ? true : false; } 
bool LC_IsHongKong()	{ return LC_GetLocalType() == LC_HONGKONG ? true : false; }
bool LC_IsNewCIBN()		{ return LC_GetLocalType() == LC_NEWCIBN ? true : false; }
bool LC_IsGermany()		{ return LC_GetLocalType() == LC_GERMANY ? true : false; }
bool LC_IsKorea()		{ return LC_GetLocalType() == LC_KOREA ? true : false; }
bool LC_IsCanada()		{ return LC_GetLocalType() == LC_CANADA ? false : false; }
bool LC_IsBrazil()		{ return LC_GetLocalType() == LC_BRAZIL ? true : false; }
bool LC_IsSingapore()	{ return LC_GetLocalType() == LC_SINGAPORE ? true : false; }
bool LC_IsVietnam()		{ return LC_GetLocalType() == LC_VIETNAM ? true : false; }
bool LC_IsThailand()	{ return LC_GetLocalType() == LC_THAILAND ? true : false; }
bool LC_IsWE_Korea()	{ return LC_GetLocalType() == LC_WE_KOREA ? true : false; }
bool LC_IsTaiwan()	{ return LC_GetLocalType() == LC_TAIWAN ? true : false; }

bool LC_IsWorldEdition()
{
	return LC_IsWE_Korea() || LC_IsEurope();
}

bool LC_IsEurope()
{
	eLocalization val = LC_GetLocalType();

	switch ((int) val)
	{
		case LC_GERMANY:
		case LC_FRANCE:
		case LC_ITALY:
		case LC_TURKEY:
		case LC_POLAND:
		case LC_UK:
		case LC_SPAIN:
		case LC_PORTUGAL:
		case LC_GREEK:
		case LC_RUSSIA:
		case LC_DENMARK:
		case LC_BULGARIA:
		case LC_CROATIA:
		case LC_MEXICO: // 남미지만 GF에서 서비스 하므로 여기 넣음
		case LC_ARABIA: // 중동이지만 GF에서 서비스 하므로 여기 넣음
		case LC_CZECH:
		case LC_ROMANIA:
		case LC_HUNGARY:
		case LC_NETHERLANDS:
		case LC_USA:
		case LC_WE_KOREA:	// 한국이지만 UK 버전 기반이므로 여기 넣음
		case LC_TAIWAN:		// 대만이지만 WE_KOREA 버전 기반이므로 여기 넣음
		case LC_JAPAN:		// 일본이지만 WE(World Edition -_-) 버전이므로 여기 넣음
		case LC_NEWCIBN:
		case LC_CANADA:	// 캐나다 GF에서 서비스 시작
			return true;
	}

	return false;
}

