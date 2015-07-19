#ifndef __Item_CSV_READER_H__
#define __Item_CSV_READER_H__

#include <iostream>
#include <map>

#include "CsvReader.h"

//csv 파일을 읽어와서 아이템 테이블에 넣어준다.
void putItemIntoTable(); //(테이블, 테스트여부)

int get_Item_Type_Value(std::string inputString);
int get_Item_SubType_Value(int type_value, std::string inputString);
int get_Item_AntiFlag_Value(std::string inputString);
int get_Item_Flag_Value(std::string inputString);
int get_Item_WearFlag_Value(std::string inputString);
int get_Item_Immune_Value(std::string inputString);
int get_Item_LimitType_Value(std::string inputString);
int get_Item_ApplyType_Value(std::string inputString);


//몬스터 프로토도 읽을 수 있다.
int get_Mob_Rank_Value(std::string inputString);
int get_Mob_Type_Value(std::string inputString);
int get_Mob_BattleType_Value(std::string inputString);

int get_Mob_Size_Value(std::string inputString);
int get_Mob_AIFlag_Value(std::string inputString);
int get_Mob_RaceFlag_Value(std::string inputString);
int get_Mob_ImmuneFlag_Value(std::string inputString);

//
bool Set_Proto_Mob_Table(TMobTable *mobTable, cCsvTable &csvTable, std::map<int,const char*> &nameMap);
bool Set_Proto_Item_Table(TItemTable *itemTable, cCsvTable &csvTable,std::map<int,const char*> &nameMap);

#endif