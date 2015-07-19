#include "stdafx.h"

#include <math.h>
#include "ProtoReader.h"

#include "CsvReader.h"

#include <sstream>

using namespace std;

inline string trim_left(const string& str)
{
    string::size_type n = str.find_first_not_of(" \t\v\n\r");
    return n == string::npos ? str : str.substr(n, str.length());
}

inline string trim_right(const string& str)
{
    string::size_type n = str.find_last_not_of(" \t\v\n\r");
    return n == string::npos ? str : str.substr(0, n + 1);
}

string trim(const string& str){return trim_left(trim_right(str));}

static string* StringSplit(string strOrigin, string strTok)
{
    int     cutAt;                            //자르는위치
    int     index     = 0;                    //문자열인덱스
    string* strResult = new string[30];		  //결과return 할변수

    //strTok을찾을때까지반복
    while ((cutAt = strOrigin.find_first_of(strTok)) != strOrigin.npos)
    {
       if (cutAt > 0)  //자르는위치가0보다크면(성공시)
       {
            strResult[index++] = strOrigin.substr(0, cutAt);  //결과배열에추가
       }
       strOrigin = strOrigin.substr(cutAt+1);  //원본은자른부분제외한나머지
    }

    if(strOrigin.length() > 0)  //원본이아직남았으면
    {
        strResult[index++] = strOrigin.substr(0, cutAt);  //나머지를결과배열에추가
    }

	for( int i=0;i<index;i++)
	{
		strResult[i] = trim(strResult[i]);
	}

    return strResult;  //결과return
}



int get_Item_Type_Value(string inputString)
{
	string arType[] = {"ITEM_NONE", "ITEM_WEAPON",
		"ITEM_ARMOR", "ITEM_USE", 
		"ITEM_AUTOUSE", "ITEM_MATERIAL",
		"ITEM_SPECIAL", "ITEM_TOOL", 
		"ITEM_LOTTERY", "ITEM_ELK",					//10개

		"ITEM_METIN", "ITEM_CONTAINER", 
		"ITEM_FISH", "ITEM_ROD", 
		"ITEM_RESOURCE", "ITEM_CAMPFIRE",
		"ITEM_UNIQUE", "ITEM_SKILLBOOK", 
		"ITEM_QUEST", "ITEM_POLYMORPH",				//20개

		"ITEM_TREASURE_BOX", "ITEM_TREASURE_KEY",
		"ITEM_SKILLFORGET", "ITEM_GIFTBOX", 
		"ITEM_PICK", "ITEM_HAIR", 
		"ITEM_TOTEM", "ITEM_BLEND", 
		"ITEM_COSTUME", "ITEM_DS",					//30개
	
		"ITEM_SPECIAL_DS",	"ITEM_EXTRACT",
		"ITEM_SECONDARY_COIN",						//33개

		"ITEM_RING",
		"ITEM_BELT",								//35개 (EItemTypes 값으로 치면 34)
	};

	
	int retInt = -1;
	//cout << "Type : " << typeStr << " -> ";
	for (int j=0;j<sizeof(arType)/sizeof(arType[0]);j++) {
		string tempString = arType[j];
		if	(inputString.find(tempString)!=string::npos && tempString.find(inputString)!=string::npos) {
			//cout << j << " ";
			retInt =  j;
			break;
		}
	}
	//cout << endl;

	return retInt;

}

int get_Item_SubType_Value(int type_value, string inputString) 
{
	static string arSub1[] = { "WEAPON_SWORD", "WEAPON_DAGGER", "WEAPON_BOW", "WEAPON_TWO_HANDED",
				"WEAPON_BELL", "WEAPON_FAN", "WEAPON_ARROW", "WEAPON_MOUNT_SPEAR"};
	static string arSub2[] = { "ARMOR_BODY", "ARMOR_HEAD", "ARMOR_SHIELD", "ARMOR_WRIST", "ARMOR_FOOTS",
				"ARMOR_NECK", "ARMOR_EAR", "ARMOR_NUM_TYPES"};
	static string arSub3[] = { "USE_POTION", "USE_TALISMAN", "USE_TUNING", "USE_MOVE", "USE_TREASURE_BOX", "USE_MONEYBAG", "USE_BAIT",
				"USE_ABILITY_UP", "USE_AFFECT", "USE_CREATE_STONE", "USE_SPECIAL", "USE_POTION_NODELAY", "USE_CLEAR",
				"USE_INVISIBILITY", "USE_DETACHMENT", "USE_BUCKET", "USE_POTION_CONTINUE", "USE_CLEAN_SOCKET",
				"USE_CHANGE_ATTRIBUTE", "USE_ADD_ATTRIBUTE", "USE_ADD_ACCESSORY_SOCKET", "USE_PUT_INTO_ACCESSORY_SOCKET",
				"USE_ADD_ATTRIBUTE2", "USE_RECIPE", "USE_CHANGE_ATTRIBUTE2", "USE_BIND", "USE_UNBIND", "USE_TIME_CHARGE_PER", "USE_TIME_CHARGE_FIX", "USE_PUT_INTO_BELT_SOCKET", "USE_PUT_INTO_RING_SOCKET"};
	static string arSub4[] = { "AUTOUSE_POTION", "AUTOUSE_ABILITY_UP", "AUTOUSE_BOMB", "AUTOUSE_GOLD", "AUTOUSE_MONEYBAG", "AUTOUSE_TREASURE_BOX"};
	static string arSub5[] = { "MATERIAL_LEATHER", "MATERIAL_BLOOD", "MATERIAL_ROOT", "MATERIAL_NEEDLE", "MATERIAL_JEWEL", 
		"MATERIAL_DS_REFINE_NORMAL", "MATERIAL_DS_REFINE_BLESSED", "MATERIAL_DS_REFINE_HOLLY"};
	static string arSub6[] = { "SPECIAL_MAP", "SPECIAL_KEY", "SPECIAL_DOC", "SPECIAL_SPIRIT"};
	static string arSub7[] = { "TOOL_FISHING_ROD" };
	static string arSub8[] = { "LOTTERY_TICKET", "LOTTERY_INSTANT" };
	static string arSub10[] = { "METIN_NORMAL", "METIN_GOLD" };
	static string arSub12[] = { "FISH_ALIVE", "FISH_DEAD"};
	static string arSub14[] = { "RESOURCE_FISHBONE", "RESOURCE_WATERSTONEPIECE", "RESOURCE_WATERSTONE", "RESOURCE_BLOOD_PEARL",
						"RESOURCE_BLUE_PEARL", "RESOURCE_WHITE_PEARL", "RESOURCE_BUCKET", "RESOURCE_CRYSTAL", "RESOURCE_GEM",
						"RESOURCE_STONE", "RESOURCE_METIN", "RESOURCE_ORE" };
	static string arSub16[] = { "UNIQUE_NONE", "UNIQUE_BOOK", "UNIQUE_SPECIAL_RIDE", "UNIQUE_3", "UNIQUE_4", "UNIQUE_5",
					"UNIQUE_6", "UNIQUE_7", "UNIQUE_8", "UNIQUE_9", "USE_SPECIAL"};
	static string arSub28[] = { "COSTUME_BODY", "COSTUME_HAIR" };
	static string arSub29[] = { "DS_SLOT1", "DS_SLOT2", "DS_SLOT3", "DS_SLOT4", "DS_SLOT5", "DS_SLOT6" };
	static string arSub31[] = { "EXTRACT_DRAGON_SOUL", "EXTRACT_DRAGON_HEART" };
	
	static string* arSubType[] = {0,	//0
		arSub1,		//1
		arSub2,	//2
		arSub3,	//3
		arSub4,	//4
		arSub5,	//5
		arSub6,	//6
		arSub7,	//7
		arSub8,	//8
		0,			//9
		arSub10,	//10
		0,			//11
		arSub12,	//12
		0,			//13
		arSub14,	//14
		0,			//15
		arSub16,	//16
		0,			//17
		0,			//18
		0,			//19
		0,			//20
		0,			//21
		0,			//22
		0,			//23
		0,			//24
		0,			//25
		0,			//26
		0,			//27
		arSub28,	//28
		arSub29,		//29
		arSub29,	//30
		arSub31,	//31
		0,			//32
		0,			//33 반지
		0,			//34 벨트
	};
	static int arNumberOfSubtype[_countof(arSubType)] = {
		0,
		sizeof(arSub1)/sizeof(arSub1[0]),
		sizeof(arSub2)/sizeof(arSub2[0]),
		sizeof(arSub3)/sizeof(arSub3[0]),
		sizeof(arSub4)/sizeof(arSub4[0]),
		sizeof(arSub5)/sizeof(arSub5[0]),
		sizeof(arSub6)/sizeof(arSub6[0]),
		sizeof(arSub7)/sizeof(arSub7[0]),
		sizeof(arSub8)/sizeof(arSub8[0]),
		0,
		sizeof(arSub10)/sizeof(arSub10[0]),
		0,
		sizeof(arSub12)/sizeof(arSub12[0]),
		0,
		sizeof(arSub14)/sizeof(arSub14[0]),
		0,
		sizeof(arSub16)/sizeof(arSub16[0]),
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		sizeof(arSub28)/sizeof(arSub28[0]),
		sizeof(arSub29)/sizeof(arSub29[0]),
		sizeof(arSub29)/sizeof(arSub29[0]),
		sizeof(arSub31)/sizeof(arSub31[0]),
		0, // 32
		0, // 33 반지
		0, // 34 벨트
	};
	

	assert(_countof(arSubType) > type_value && "Subtype rule: Out of range!!");

	// assert 안 먹히는 듯..
	if (_countof(arSubType) <= type_value)
	{
		sys_err("SubType : Out of range!! (type_value: %d, count of registered subtype: %d", type_value, _countof(arSubType));
		return -1;
	}

	//아이템 타입의 서브타입 어레이가 존재하는지 알아보고, 없으면 0 리턴
	if (arSubType[type_value]==0) {
		return 0;
	}
	//

	int retInt = -1;
	//cout << "SubType : " << subTypeStr << " -> ";
	for (int j=0;j<arNumberOfSubtype[type_value];j++) {
		string tempString = arSubType[type_value][j];
		string tempInputString = trim(inputString);
		if	(tempInputString.compare(tempString)==0)
		{
			//cout << j << " ";
			retInt =  j;
			break;
		}
	}
	//cout << endl;

	return retInt;
}





int get_Item_AntiFlag_Value(string inputString) 
{

	string arAntiFlag[] = {"ANTI_FEMALE", "ANTI_MALE", "ANTI_MUSA", "ANTI_ASSASSIN", "ANTI_SURA", "ANTI_MUDANG",
							"ANTI_GET", "ANTI_DROP", "ANTI_SELL", "ANTI_EMPIRE_A", "ANTI_EMPIRE_B", "ANTI_EMPIRE_C",
							"ANTI_SAVE", "ANTI_GIVE", "ANTI_PKDROP", "ANTI_STACK", "ANTI_MYSHOP", "ANTI_SAFEBOX"};


	int retValue = 0;
	string* arInputString = StringSplit(inputString, "|");				//프로토 정보 내용을 단어별로 쪼갠 배열.
	for(int i =0;i<sizeof(arAntiFlag)/sizeof(arAntiFlag[0]);i++) {
		string tempString = arAntiFlag[i];
		for (int j=0; j<30 ; j++)		//최대 30개 단어까지. (하드코딩)
		{
			string tempString2 = arInputString[j];
			if (tempString2.compare(tempString)==0) {				//일치하는지 확인.
				retValue = retValue + pow((float)2,(float)i);
			}
			
			if(tempString2.compare("") == 0)
				break;
		}
	}
	delete []arInputString;
	//cout << "AntiFlag : " << antiFlagStr << " -> " << retValue << endl;

	return retValue;
}

int get_Item_Flag_Value(string inputString) 
{

	string arFlag[] = {"ITEM_TUNABLE", "ITEM_SAVE", "ITEM_STACKABLE", "COUNT_PER_1GOLD", "ITEM_SLOW_QUERY", "ITEM_UNIQUE",
			"ITEM_MAKECOUNT", "ITEM_IRREMOVABLE", "CONFIRM_WHEN_USE", "QUEST_USE", "QUEST_USE_MULTIPLE",
			"QUEST_GIVE", "ITEM_QUEST", "LOG", "STACKABLE", "SLOW_QUERY", "REFINEABLE", "IRREMOVABLE", "ITEM_APPLICABLE"};


	int retValue = 0;
	string* arInputString = StringSplit(inputString, "|");				//프로토 정보 내용을 단어별로 쪼갠 배열.
	for(int i =0;i<sizeof(arFlag)/sizeof(arFlag[0]);i++) {
		string tempString = arFlag[i];
		for (int j=0; j<30 ; j++)		//최대 30개 단어까지. (하드코딩)
		{
			string tempString2 = arInputString[j];
			if (tempString2.compare(tempString)==0) {				//일치하는지 확인.
				retValue = retValue + pow((float)2,(float)i);
			}
			
			if(tempString2.compare("") == 0)
				break;
		}
	}
	delete []arInputString;
	//cout << "Flag : " << flagStr << " -> " << retValue << endl;

	return retValue;
}

int get_Item_WearFlag_Value(string inputString) 
{

	string arWearrFlag[] = {"WEAR_BODY", "WEAR_HEAD", "WEAR_FOOTS", "WEAR_WRIST", "WEAR_WEAPON", "WEAR_NECK", "WEAR_EAR", "WEAR_SHIELD", "WEAR_UNIQUE",
					"WEAR_ARROW", "WEAR_HAIR", "WEAR_ABILITY"};


	int retValue = 0;
	string* arInputString = StringSplit(inputString, "|");				//프로토 정보 내용을 단어별로 쪼갠 배열.
	for(int i =0;i<sizeof(arWearrFlag)/sizeof(arWearrFlag[0]);i++) {
		string tempString = arWearrFlag[i];
		for (int j=0; j<30 ; j++)		//최대 30개 단어까지. (하드코딩)
		{
			string tempString2 = arInputString[j];
			if (tempString2.compare(tempString)==0) {				//일치하는지 확인.
				retValue = retValue + pow((float)2,(float)i);
			}
			
			if(tempString2.compare("") == 0)
				break;
		}
	}
	delete []arInputString;
	//cout << "WearFlag : " << wearFlagStr << " -> " << retValue << endl;

	return retValue;
}

int get_Item_Immune_Value(string inputString) 
{

	string arImmune[] = {"PARA","CURSE","STUN","SLEEP","SLOW","POISON","TERROR"};

	int retValue = 0;
	string* arInputString = StringSplit(inputString, "|");				//프로토 정보 내용을 단어별로 쪼갠 배열.
	for(int i =0;i<sizeof(arImmune)/sizeof(arImmune[0]);i++) {
		string tempString = arImmune[i];
		for (int j=0; j<30 ; j++)		//최대 30개 단어까지. (하드코딩)
		{
			string tempString2 = arInputString[j];
			if (tempString2.compare(tempString)==0) {				//일치하는지 확인.
				retValue = retValue + pow((float)2,(float)i);
			}
			
			if(tempString2.compare("") == 0)
				break;
		}
	}
	delete []arInputString;
	//cout << "Immune : " << immuneStr << " -> " << retValue << endl;

	return retValue;
}




int get_Item_LimitType_Value(string inputString)
{
	string arLimitType[] = {"LIMIT_NONE", "LEVEL", "STR", "DEX", "INT", "CON", "PC_BANG", "REAL_TIME", "REAL_TIME_FIRST_USE", "TIMER_BASED_ON_WEAR"};
	
	int retInt = -1;
	//cout << "LimitType : " << limitTypeStr << " -> ";
	for (int j=0;j<sizeof(arLimitType)/sizeof(arLimitType[0]);j++) {
		string tempString = arLimitType[j];
		string tempInputString = trim(inputString);
		if	(tempInputString.compare(tempString)==0)
		{
			//cout << j << " ";
			retInt =  j;
			break;
		}
	}
	//cout << endl;

	return retInt;
}


int get_Item_ApplyType_Value(string inputString)
{
	string arApplyType[] = {"APPLY_NONE", "APPLY_MAX_HP", "APPLY_MAX_SP", "APPLY_CON", "APPLY_INT", "APPLY_STR", "APPLY_DEX", "APPLY_ATT_SPEED",
			"APPLY_MOV_SPEED", "APPLY_CAST_SPEED", "APPLY_HP_REGEN", "APPLY_SP_REGEN", "APPLY_POISON_PCT", "APPLY_STUN_PCT",
			"APPLY_SLOW_PCT", "APPLY_CRITICAL_PCT", "APPLY_PENETRATE_PCT", "APPLY_ATTBONUS_HUMAN", "APPLY_ATTBONUS_ANIMAL",
			"APPLY_ATTBONUS_ORC", "APPLY_ATTBONUS_MILGYO", "APPLY_ATTBONUS_UNDEAD", "APPLY_ATTBONUS_DEVIL", "APPLY_STEAL_HP",
			"APPLY_STEAL_SP", "APPLY_MANA_BURN_PCT", "APPLY_DAMAGE_SP_RECOVER", "APPLY_BLOCK", "APPLY_DODGE", "APPLY_RESIST_SWORD",
			"APPLY_RESIST_TWOHAND", "APPLY_RESIST_DAGGER", "APPLY_RESIST_BELL", "APPLY_RESIST_FAN", "APPLY_RESIST_BOW", "APPLY_RESIST_FIRE",
			"APPLY_RESIST_ELEC", "APPLY_RESIST_MAGIC", "APPLY_RESIST_WIND", "APPLY_REFLECT_MELEE", "APPLY_REFLECT_CURSE", "APPLY_POISON_REDUCE",
			"APPLY_KILL_SP_RECOVER", "APPLY_EXP_DOUBLE_BONUS", "APPLY_GOLD_DOUBLE_BONUS", "APPLY_ITEM_DROP_BONUS", "APPLY_POTION_BONUS",
			"APPLY_KILL_HP_RECOVER", "APPLY_IMMUNE_STUN", "APPLY_IMMUNE_SLOW", "APPLY_IMMUNE_FALL", "APPLY_SKILL", "APPLY_BOW_DISTANCE",
			"APPLY_ATT_GRADE_BONUS", "APPLY_DEF_GRADE_BONUS", "APPLY_MAGIC_ATT_GRADE", "APPLY_MAGIC_DEF_GRADE", "APPLY_CURSE_PCT",
			"APPLY_MAX_STAMINA", "APPLY_ATTBONUS_WARRIOR", "APPLY_ATTBONUS_ASSASSIN", "APPLY_ATTBONUS_SURA", "APPLY_ATTBONUS_SHAMAN",
			"APPLY_ATTBONUS_MONSTER", "APPLY_MALL_ATTBONUS", "APPLY_MALL_DEFBONUS", "APPLY_MALL_EXPBONUS", "APPLY_MALL_ITEMBONUS",
			"APPLY_MALL_GOLDBONUS", "APPLY_MAX_HP_PCT", "APPLY_MAX_SP_PCT", "APPLY_SKILL_DAMAGE_BONUS", "APPLY_NORMAL_HIT_DAMAGE_BONUS",
			"APPLY_SKILL_DEFEND_BONUS", "APPLY_NORMAL_HIT_DEFEND_BONUS", "APPLY_PC_BANG_EXP_BONUS", "APPLY_PC_BANG_DROP_BONUS",
			"APPLY_EXTRACT_HP_PCT", "APPLY_RESIST_WARRIOR", "APPLY_RESIST_ASSASSIN", "APPLY_RESIST_SURA", "APPLY_RESIST_SHAMAN",
			"APPLY_ENERGY",	"APPLY_DEF_GRADE", "APPLY_COSTUME_ATTR_BONUS", "APPLY_MAGIC_ATTBONUS_PER", "APPLY_MELEE_MAGIC_ATTBONUS_PER",
			"APPLY_RESIST_ICE", "APPLY_RESIST_EARTH", "APPLY_RESIST_DARK", "APPLY_ANTI_CRITICAL_PCT", "APPLY_ANTI_PENETRATE_PCT",
	};

	int retInt = -1;
	//cout << "ApplyType : " << applyTypeStr << " -> ";
	for (int j=0;j<sizeof(arApplyType)/sizeof(arApplyType[0]);j++) {
		string tempString = arApplyType[j];
		string tempInputString = trim(inputString);
		if	(tempInputString.compare(tempString)==0)
		{ 
			//cout << j << " ";
			retInt =  j;
			break;
		}
	}
	//cout << endl;

	return retInt;

}


//몬스터 프로토도 읽는다.


int get_Mob_Rank_Value(string inputString) 
{
	string arRank[] = {"PAWN", "S_PAWN", "KNIGHT", "S_KNIGHT", "BOSS", "KING"};

	int retInt = -1;
	//cout << "Rank : " << rankStr << " -> ";
	for (int j=0;j<sizeof(arRank)/sizeof(arRank[0]);j++) {
		string tempString = arRank[j];
		string tempInputString = trim(inputString);
		if	(tempInputString.compare(tempString)==0) 
		{
			//cout << j << " ";
			retInt =  j;
			break;
		}
	}
	//cout << endl;

	return retInt;
}


int get_Mob_Type_Value(string inputString)
{
	string arType[] = { "MONSTER", "NPC", "STONE", "WARP", "DOOR", "BUILDING", "PC", "POLYMORPH_PC", "HORSE", "GOTO"};

	int retInt = -1;
	//cout << "Type : " << typeStr << " -> ";
	for (int j=0;j<sizeof(arType)/sizeof(arType[0]);j++) {
		string tempString = arType[j];
		string tempInputString = trim(inputString);
		if	(tempInputString.compare(tempString)==0) 
		{
			//cout << j << " ";
			retInt =  j;
			break;
		}
	}
	//cout << endl;

	return retInt;
}

int get_Mob_BattleType_Value(string inputString) 
{
	string arBattleType[] = { "MELEE", "RANGE", "MAGIC", "SPECIAL", "POWER", "TANKER", "SUPER_POWER", "SUPER_TANKER"};

	int retInt = -1;
	//cout << "Battle Type : " << battleTypeStr << " -> ";
	for (int j=0;j<sizeof(arBattleType)/sizeof(arBattleType[0]);j++) {
		string tempString = arBattleType[j];
		string tempInputString = trim(inputString);
		if	(tempInputString.compare(tempString)==0) 
		{ 
			//cout << j << " ";
			retInt =  j;
			break;
		}
	}
	//cout << endl;

	return retInt;
}

int get_Mob_Size_Value(string inputString)
{
	string arSize[] = { "SAMLL", "MEDIUM", "BIG"};

	int retInt = 0;
	//cout << "Size : " << sizeStr << " -> ";
	for (int j=0;j<sizeof(arSize)/sizeof(arSize[0]);j++) {
		string tempString = arSize[j];
		string tempInputString = trim(inputString);
		if	(tempInputString.compare(tempString)==0) 
		{
			//cout << j << " ";
			retInt =  j + 1;
			break;
		}
	}
	//cout << endl;

	return retInt;
}

int get_Mob_AIFlag_Value(string inputString)
{
	string arAIFlag[] = {"AGGR","NOMOVE","COWARD","NOATTSHINSU","NOATTCHUNJO","NOATTJINNO","ATTMOB","BERSERK","STONESKIN","GODSPEED","DEATHBLOW","REVIVE"};


	int retValue = 0;
	string* arInputString = StringSplit(inputString, ",");				//프로토 정보 내용을 단어별로 쪼갠 배열.
	for(int i =0;i<sizeof(arAIFlag)/sizeof(arAIFlag[0]);i++) {
		string tempString = arAIFlag[i];
		for (int j=0; j<30 ; j++)		//최대 30개 단어까지. (하드코딩)
		{
			string tempString2 = arInputString[j];
			if (tempString2.compare(tempString)==0) {				//일치하는지 확인.
				retValue = retValue + pow((float)2,(float)i);
			}
			
			if(tempString2.compare("") == 0)
				break;
		}
	}
	delete []arInputString;
	//cout << "AIFlag : " << aiFlagStr << " -> " << retValue << endl;

	return retValue;
}
int get_Mob_RaceFlag_Value(string inputString)
{
	string arRaceFlag[] = {"ANIMAL","UNDEAD","DEVIL","HUMAN","ORC","MILGYO","INSECT","FIRE","ICE","DESERT","TREE",
		"ATT_ELEC","ATT_FIRE","ATT_ICE","ATT_WIND","ATT_EARTH","ATT_DARK"};

	int retValue = 0;
	string* arInputString = StringSplit(inputString, ",");				//프로토 정보 내용을 단어별로 쪼갠 배열.
	for(int i =0;i<sizeof(arRaceFlag)/sizeof(arRaceFlag[0]);i++) {
		string tempString = arRaceFlag[i];
		for (int j=0; j<30 ; j++)		//최대 30개 단어까지. (하드코딩)
		{
			string tempString2 = arInputString[j];
			if (tempString2.compare(tempString)==0) {				//일치하는지 확인.
				retValue = retValue + pow((float)2,(float)i);
			}
			
			if(tempString2.compare("") == 0)
				break;
		}
	}
	delete []arInputString;
	//cout << "Race Flag : " << raceFlagStr << " -> " << retValue << endl;

	return retValue;
}
int get_Mob_ImmuneFlag_Value(string inputString)
{
	string arImmuneFlag[] = {"STUN","SLOW","FALL","CURSE","POISON","TERROR", "REFLECT"};

	int retValue = 0;
	string* arInputString = StringSplit(inputString, ",");				//프로토 정보 내용을 단어별로 쪼갠 배열.
	for(int i =0;i<sizeof(arImmuneFlag)/sizeof(arImmuneFlag[0]);i++) {
		string tempString = arImmuneFlag[i];
		for (int j=0; j<30 ; j++)		//최대 30개 단어까지. (하드코딩)
		{
			string tempString2 = arInputString[j];
			if (tempString2.compare(tempString)==0) {				//일치하는지 확인.
				retValue = retValue + pow((float)2,(float)i);
			}
			
			if(tempString2.compare("") == 0)
				break;
		}
	}
	delete []arInputString;
	//cout << "Immune Flag : " << immuneFlagStr << " -> " << retValue << endl;


	return retValue;
}


#ifndef __DUMP_PROTO__

//몹 테이블을 셋팅해준다.
bool Set_Proto_Mob_Table(TMobTable *mobTable, cCsvTable &csvTable,std::map<int,const char*> &nameMap)
{
	int col = 0;
	str_to_number(mobTable->dwVnum, csvTable.AsStringByIndex(col++));
	strlcpy(mobTable->szName, csvTable.AsStringByIndex(col++), sizeof(mobTable->szName));

	//3. 지역별 이름 넣어주기.
	map<int,const char*>::iterator it;
	it = nameMap.find(mobTable->dwVnum);
	if (it != nameMap.end()) {
		const char * localeName = it->second;
		strlcpy(mobTable->szLocaleName, localeName, sizeof (mobTable->szLocaleName));
	} else {
		strlcpy(mobTable->szLocaleName, mobTable->szName, sizeof (mobTable->szLocaleName));
	}

	//RANK
	int rankValue = get_Mob_Rank_Value(csvTable.AsStringByIndex(col++));
	mobTable->bRank = rankValue;
	//TYPE
	int typeValue = get_Mob_Type_Value(csvTable.AsStringByIndex(col++));
	mobTable->bType = typeValue;
	//BATTLE_TYPE
	int battleTypeValue = get_Mob_BattleType_Value(csvTable.AsStringByIndex(col++));
	mobTable->bBattleType = battleTypeValue;

	str_to_number(mobTable->bLevel, csvTable.AsStringByIndex(col++));
	//SIZE
	int sizeValue = get_Mob_Size_Value(csvTable.AsStringByIndex(col++));
	mobTable->bSize = sizeValue;
	//AI_FLAG
	int aiFlagValue = get_Mob_AIFlag_Value(csvTable.AsStringByIndex(col++));
	mobTable->dwAIFlag = aiFlagValue;
	//mount_capacity;
	col++;
	//RACE_FLAG
	int raceFlagValue = get_Mob_RaceFlag_Value(csvTable.AsStringByIndex(col++));
	mobTable->dwRaceFlag = raceFlagValue;
	//IMMUNE_FLAG
	int immuneFlagValue = get_Mob_ImmuneFlag_Value(csvTable.AsStringByIndex(col++));
	mobTable->dwImmuneFlag = immuneFlagValue;

	str_to_number(mobTable->bEmpire, csvTable.AsStringByIndex(col++));  //col = 11

	strlcpy(mobTable->szFolder, csvTable.AsStringByIndex(col++), sizeof(mobTable->szFolder));

	str_to_number(mobTable->bOnClickType, csvTable.AsStringByIndex(col++));	

	str_to_number(mobTable->bStr, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->bDex, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->bCon, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->bInt, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->dwDamageRange[0], csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->dwDamageRange[1], csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->dwMaxHP, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->bRegenCycle, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->bRegenPercent,	csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->dwGoldMin, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->dwGoldMax, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->dwExp,	csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->wDef, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->sAttackSpeed, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->sMovingSpeed, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->bAggresiveHPPct, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->wAggressiveSight, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->wAttackRange, csvTable.AsStringByIndex(col++));

	str_to_number(mobTable->dwDropItemVnum, csvTable.AsStringByIndex(col++));	//32
	str_to_number(mobTable->dwResurrectionVnum, csvTable.AsStringByIndex(col++));
	for (int i = 0; i < MOB_ENCHANTS_MAX_NUM; ++i)
		str_to_number(mobTable->cEnchants[i], csvTable.AsStringByIndex(col++));

	for (int i = 0; i < MOB_RESISTS_MAX_NUM; ++i)
		str_to_number(mobTable->cResists[i], csvTable.AsStringByIndex(col++));

	str_to_number(mobTable->fDamMultiply, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->dwSummonVnum, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->dwDrainSP, csvTable.AsStringByIndex(col++));

	//Mob_Color
	++col;

	str_to_number(mobTable->dwPolymorphItemVnum, csvTable.AsStringByIndex(col++));

	str_to_number(mobTable->Skills[0].bLevel, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->Skills[0].dwVnum, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->Skills[1].bLevel, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->Skills[1].dwVnum, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->Skills[2].bLevel, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->Skills[2].dwVnum, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->Skills[3].bLevel, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->Skills[3].dwVnum, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->Skills[4].bLevel, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->Skills[4].dwVnum, csvTable.AsStringByIndex(col++));

	str_to_number(mobTable->bBerserkPoint, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->bStoneSkinPoint, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->bGodSpeedPoint, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->bDeathBlowPoint, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->bRevivePoint, csvTable.AsStringByIndex(col++));

	sys_log(0, "MOB #%-5d %-24s level: %-3u rank: %u empire: %d", mobTable->dwVnum, mobTable->szLocaleName, mobTable->bLevel, mobTable->bRank, mobTable->bEmpire);

	return true;
}

bool Set_Proto_Item_Table(TItemTable *itemTable, cCsvTable &csvTable,std::map<int,const char*> &nameMap)
{
	int col = 0;

	int dataArray[33];
	for (int i=0; i<sizeof(dataArray)/sizeof(dataArray[0]);i++) {
		int validCheck = 0;
		if (i==2) {
			dataArray[i] = get_Item_Type_Value(csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else if (i==3) {
			dataArray[i] = get_Item_SubType_Value(dataArray[i-1], csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else if (i==5) {
			dataArray[i] = get_Item_AntiFlag_Value(csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else if (i==6) {
			dataArray[i] = get_Item_Flag_Value(csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else if (i==7) {
			dataArray[i] = get_Item_WearFlag_Value(csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else if (i==8) {
			dataArray[i] = get_Item_Immune_Value(csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else if (i==14) {
			dataArray[i] = get_Item_LimitType_Value(csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else if (i==16) {
			dataArray[i] = get_Item_LimitType_Value(csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else if (i==18) {
			dataArray[i] = get_Item_ApplyType_Value(csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else if (i==20) {
			dataArray[i] = get_Item_ApplyType_Value(csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else if (i==22) {
			dataArray[i] = get_Item_ApplyType_Value(csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else {
			str_to_number(dataArray[i], csvTable.AsStringByIndex(col));
		}

		if (validCheck == -1)
		{
			std::ostringstream dataStream;

			for (int j = 0; j < i; ++j)
				dataStream << dataArray[j] << ",";

			//fprintf(stderr, "ItemProto Reading Failed : Invalid value.\n");
			sys_err("ItemProto Reading Failed : Invalid value. (index: %d, col: %d, value: %s)", i, col, csvTable.AsStringByIndex(col));
			sys_err("\t%d ~ %d Values: %s", 0, i, dataStream.str().c_str());

			exit(0);
		}
		
		col = col + 1;
	}

	// vnum 및 vnum range 읽기.
	{
		std::string s(csvTable.AsStringByIndex(0));
		int pos = s.find("~");
		// vnum 필드에 '~'가 없다면 패스
		if (std::string::npos == pos)
		{
			itemTable->dwVnum = dataArray[0];
			itemTable->dwVnumRange = 0;
		}
		else
		{
			std::string s_start_vnum (s.substr(0, pos));
			std::string s_end_vnum (s.substr(pos +1 ));

			int start_vnum = atoi(s_start_vnum.c_str());
			int end_vnum = atoi(s_end_vnum.c_str());
			if (0 == start_vnum || (0 != end_vnum && end_vnum < start_vnum))
			{
				sys_err ("INVALID VNUM %s", s.c_str());
				return false;
			}
			itemTable->dwVnum = start_vnum;
			itemTable->dwVnumRange = end_vnum - start_vnum;
		}
	}

	strlcpy(itemTable->szName, csvTable.AsStringByIndex(1), sizeof(itemTable->szName));
	//지역별 이름 넣어주기.
	map<int,const char*>::iterator it;
	it = nameMap.find(itemTable->dwVnum);
	if (it != nameMap.end()) {
		const char * localeName = it->second;
		strlcpy(itemTable->szLocaleName, localeName, sizeof (itemTable->szLocaleName));
	} else {
		strlcpy(itemTable->szLocaleName, itemTable->szName, sizeof (itemTable->szLocaleName));
	}
	itemTable->bType = dataArray[2];
	itemTable->bSubType = dataArray[3];
	itemTable->bSize = dataArray[4];
	itemTable->dwAntiFlags = dataArray[5];
	itemTable->dwFlags = dataArray[6];
	itemTable->dwWearFlags = dataArray[7];
	itemTable->dwImmuneFlag = dataArray[8];
	itemTable->dwGold = dataArray[9];
	itemTable->dwShopBuyPrice = dataArray[10];
	itemTable->dwRefinedVnum = dataArray[11];
	itemTable->wRefineSet = dataArray[12];
	itemTable->bAlterToMagicItemPct = dataArray[13];
	itemTable->cLimitRealTimeFirstUseIndex = -1;
	itemTable->cLimitTimerBasedOnWearIndex = -1;

	int i;

	for (i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		itemTable->aLimits[i].bType = dataArray[14+i*2];
		itemTable->aLimits[i].lValue = dataArray[15+i*2];

		if (LIMIT_REAL_TIME_START_FIRST_USE == itemTable->aLimits[i].bType)
			itemTable->cLimitRealTimeFirstUseIndex = (char)i;

		if (LIMIT_TIMER_BASED_ON_WEAR == itemTable->aLimits[i].bType)
			itemTable->cLimitTimerBasedOnWearIndex = (char)i;

	}

	for (i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
	{
		itemTable->aApplies[i].bType = dataArray[18+i*2];
		itemTable->aApplies[i].lValue = dataArray[19+i*2];
	}

	for (i = 0; i < ITEM_VALUES_MAX_NUM; ++i)
		itemTable->alValues[i] = dataArray[24+i];

	//column for 'Specular'
	itemTable->bGainSocketPct = dataArray[31];
	itemTable->sAddonType = dataArray[32];

	//test
	str_to_number(itemTable->bWeight, "0");
			
	return true;
}

#endif
