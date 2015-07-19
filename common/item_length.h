#ifndef __INC_METIN2_ITEM_LENGTH_H__
#define __INC_METIN2_ITEM_LENGTH_H__

enum EItemMisc
{
	ITEM_NAME_MAX_LEN			= 24,
	ITEM_VALUES_MAX_NUM			= 6,
	ITEM_SMALL_DESCR_MAX_LEN	= 256,
	ITEM_LIMIT_MAX_NUM			= 2,
	ITEM_APPLY_MAX_NUM			= 3,
	ITEM_SOCKET_MAX_NUM			= 3,
	ITEM_MAX_COUNT				= 200,
	ITEM_ATTRIBUTE_MAX_NUM		= 7,
	ITEM_ATTRIBUTE_MAX_LEVEL	= 5,
	ITEM_AWARD_WHY_MAX_LEN		= 50,

	REFINE_MATERIAL_MAX_NUM		= 5,

	ITEM_ELK_VNUM				= 50026,
};

const BYTE ITEM_SOCKET_REMAIN_SEC = 0;
enum EItemValueIdice
{
	ITEM_VALUE_DRAGON_SOUL_POLL_OUT_BONUS_IDX = 0,
	ITEM_VALUE_CHARGING_AMOUNT_IDX = 0,
	ITEM_VALUE_SECONDARY_COIN_UNIT_IDX = 0,
};
enum EItemDragonSoulSockets
{
	ITEM_SOCKET_DRAGON_SOUL_ACTIVE_IDX = 2,
	ITEM_SOCKET_CHARGING_AMOUNT_IDX = 2,
};
// 헐 이거 미친거 아니야?
// 나중에 소켓 확장하면 어쩌려고 이지랄 -_-;;;
enum EItemUniqueSockets
{
	ITEM_SOCKET_UNIQUE_SAVE_TIME = ITEM_SOCKET_MAX_NUM - 2,
	ITEM_SOCKET_UNIQUE_REMAIN_TIME = ITEM_SOCKET_MAX_NUM - 1
};

enum EItemTypes
{
    ITEM_NONE,              //0
    ITEM_WEAPON,            //1//무기
    ITEM_ARMOR,             //2//갑옷
    ITEM_USE,               //3//아이템 사용
    ITEM_AUTOUSE,           //4
    ITEM_MATERIAL,          //5
    ITEM_SPECIAL,           //6 //스페셜 아이템
    ITEM_TOOL,              //7
    ITEM_LOTTERY,           //8//복권
    ITEM_ELK,               //9//돈
    ITEM_METIN,             //10
    ITEM_CONTAINER,         //11
    ITEM_FISH,              //12//낚시
    ITEM_ROD,               //13
    ITEM_RESOURCE,          //14
    ITEM_CAMPFIRE,          //15
    ITEM_UNIQUE,            //16
    ITEM_SKILLBOOK,         //17
    ITEM_QUEST,             //18
    ITEM_POLYMORPH,         //19
    ITEM_TREASURE_BOX,      //20//보물상자
    ITEM_TREASURE_KEY,      //21//보물상자 열쇠
    ITEM_SKILLFORGET,       //22
    ITEM_GIFTBOX,           //23
    ITEM_PICK,              //24
    ITEM_HAIR,              //25//머리
    ITEM_TOTEM,             //26//토템
	ITEM_BLEND,				//27//생성될때 랜덤하게 속성이 붙는 약물
	ITEM_COSTUME,			//28//코스츔 아이템 (2011년 8월 추가된 코스츔 시스템용 아이템)
	ITEM_DS,				//29 //용혼석
	ITEM_SPECIAL_DS,		//30 // 특수한 용혼석 (DS_SLOT에 착용하는 UNIQUE 아이템이라 생각하면 됨)
	ITEM_EXTRACT,			//31 추출도구.
	ITEM_SECONDARY_COIN,	//32 ?? 명도전??
	ITEM_RING,				//33 반지
	ITEM_BELT,				//34 벨트
};

enum EMetinSubTypes
{
	METIN_NORMAL,
	METIN_GOLD,
};

enum EWeaponSubTypes
{
	WEAPON_SWORD,
	WEAPON_DAGGER,
	WEAPON_BOW,
	WEAPON_TWO_HANDED,
	WEAPON_BELL,
	WEAPON_FAN,
	WEAPON_ARROW,
	WEAPON_MOUNT_SPEAR,
	WEAPON_NUM_TYPES,
};

enum EArmorSubTypes
{
	ARMOR_BODY,
	ARMOR_HEAD,
	ARMOR_SHIELD,
	ARMOR_WRIST,
	ARMOR_FOOTS,
	ARMOR_NECK,
	ARMOR_EAR,
	ARMOR_NUM_TYPES
};

enum ECostumeSubTypes
{
	COSTUME_BODY = ARMOR_BODY,			// [중요!!] ECostumeSubTypes enum value는  종류별로 EArmorSubTypes의 그것과 같아야 함.
	COSTUME_HAIR = ARMOR_HEAD,			// 이는 코스츔 아이템에 추가 속성을 붙이겠다는 사업부의 요청에 따라서 기존 로직을 활용하기 위함임.
	COSTUME_NUM_TYPES,
};

enum EDragonSoulSubType
{
	DS_SLOT1,
	DS_SLOT2,
	DS_SLOT3,
	DS_SLOT4,
	DS_SLOT5,
	DS_SLOT6,
	DS_SLOT_MAX,
};

enum EDragonSoulGradeTypes
{
	DRAGON_SOUL_GRADE_NORMAL,
	DRAGON_SOUL_GRADE_BRILLIANT,
	DRAGON_SOUL_GRADE_RARE,
	DRAGON_SOUL_GRADE_ANCIENT,
	DRAGON_SOUL_GRADE_LEGENDARY,
	DRAGON_SOUL_GRADE_MAX,

};

enum EDragonSoulStepTypes
{
	DRAGON_SOUL_STEP_LOWEST,
	DRAGON_SOUL_STEP_LOW,
	DRAGON_SOUL_STEP_MID,
	DRAGON_SOUL_STEP_HIGH,
	DRAGON_SOUL_STEP_HIGHEST,
	DRAGON_SOUL_STEP_MAX,
};
#define DRAGON_SOUL_STRENGTH_MAX 7

enum EDSInventoryMaxNum
{
	DRAGON_SOUL_INVENTORY_MAX_NUM = DS_SLOT_MAX * DRAGON_SOUL_GRADE_MAX * DRAGON_SOUL_BOX_SIZE,
};

enum EFishSubTypes
{
	FISH_ALIVE,
	FISH_DEAD,
};

enum EResourceSubTypes
{
	RESOURCE_FISHBONE,
	RESOURCE_WATERSTONEPIECE,
	RESOURCE_WATERSTONE,
	RESOURCE_BLOOD_PEARL,
	RESOURCE_BLUE_PEARL,
	RESOURCE_WHITE_PEARL,
	RESOURCE_BUCKET,
	RESOURCE_CRYSTAL,
	RESOURCE_GEM,
	RESOURCE_STONE,
	RESOURCE_METIN,
	RESOURCE_ORE,
};

enum EUniqueSubTypes
{
	UNIQUE_NONE,
	UNIQUE_BOOK,
	UNIQUE_SPECIAL_RIDE,
	UNIQUE_SPECIAL_MOUNT_RIDE,
};

enum EUseSubTypes
{
	USE_POTION,					// 0
	USE_TALISMAN,
	USE_TUNING,
	USE_MOVE,
	USE_TREASURE_BOX,
	USE_MONEYBAG,
	USE_BAIT,
	USE_ABILITY_UP,
	USE_AFFECT,
	USE_CREATE_STONE,
	USE_SPECIAL,				// 10
	USE_POTION_NODELAY,
	USE_CLEAR,
	USE_INVISIBILITY,
	USE_DETACHMENT,
	USE_BUCKET,
	USE_POTION_CONTINUE,
	USE_CLEAN_SOCKET,
	USE_CHANGE_ATTRIBUTE,
	USE_ADD_ATTRIBUTE,
	USE_ADD_ACCESSORY_SOCKET,	// 20
	USE_PUT_INTO_ACCESSORY_SOCKET,
	USE_ADD_ATTRIBUTE2,
	USE_RECIPE,
	USE_CHANGE_ATTRIBUTE2,
	USE_BIND,
	USE_UNBIND,
	USE_TIME_CHARGE_PER,
	USE_TIME_CHARGE_FIX,				// 28
	USE_PUT_INTO_BELT_SOCKET,			// 29 벨트 소켓에 사용할 수 있는 아이템 
	USE_PUT_INTO_RING_SOCKET,			// 30 반지 소켓에 사용할 수 있는 아이템 (유니크 반지 말고, 새로 추가된 반지 슬롯)
};

enum EExtractSubTypes
{
	EXTRACT_DRAGON_SOUL,
	EXTRACT_DRAGON_HEART,
};

enum EAutoUseSubTypes
{
	AUTOUSE_POTION,
	AUTOUSE_ABILITY_UP,
	AUTOUSE_BOMB,
	AUTOUSE_GOLD,
	AUTOUSE_MONEYBAG,
	AUTOUSE_TREASURE_BOX
};

enum EMaterialSubTypes
{
	MATERIAL_LEATHER,
	MATERIAL_BLOOD,
	MATERIAL_ROOT,
	MATERIAL_NEEDLE,
	MATERIAL_JEWEL,
	MATERIAL_DS_REFINE_NORMAL, 
	MATERIAL_DS_REFINE_BLESSED, 
	MATERIAL_DS_REFINE_HOLLY,
};

enum ESpecialSubTypes
{
	SPECIAL_MAP,
	SPECIAL_KEY,
	SPECIAL_DOC,
	SPECIAL_SPIRIT,
};

enum EToolSubTypes
{
	TOOL_FISHING_ROD
};

enum ELotterySubTypes
{
	LOTTERY_TICKET,
	LOTTERY_INSTANT
};

enum EItemFlag
{
	ITEM_FLAG_REFINEABLE		= (1 << 0),
	ITEM_FLAG_SAVE			= (1 << 1),
	ITEM_FLAG_STACKABLE		= (1 << 2),	// 여러개 합칠 수 있음
	ITEM_FLAG_COUNT_PER_1GOLD	= (1 << 3),
	ITEM_FLAG_SLOW_QUERY		= (1 << 4),
	ITEM_FLAG_UNUSED01		= (1 << 5),	// UNUSED
	ITEM_FLAG_UNIQUE		= (1 << 6),
	ITEM_FLAG_MAKECOUNT		= (1 << 7),
	ITEM_FLAG_IRREMOVABLE		= (1 << 8),
	ITEM_FLAG_CONFIRM_WHEN_USE	= (1 << 9),
	ITEM_FLAG_QUEST_USE		= (1 << 10),
	ITEM_FLAG_QUEST_USE_MULTIPLE	= (1 << 11),
	ITEM_FLAG_QUEST_GIVE		= (1 << 12),
	ITEM_FLAG_LOG			= (1 << 13),
	ITEM_FLAG_APPLICABLE		= (1 << 14),
};

enum EItemAntiFlag
{
	ITEM_ANTIFLAG_FEMALE	= (1 << 0), // 여성 사용 불가
	ITEM_ANTIFLAG_MALE		= (1 << 1), // 남성 사용 불가
	ITEM_ANTIFLAG_WARRIOR	= (1 << 2), // 무사 사용 불가
	ITEM_ANTIFLAG_ASSASSIN	= (1 << 3), // 자객 사용 불가
	ITEM_ANTIFLAG_SURA		= (1 << 4), // 수라 사용 불가 
	ITEM_ANTIFLAG_SHAMAN	= (1 << 5), // 무당 사용 불가
	ITEM_ANTIFLAG_GET		= (1 << 6), // 집을 수 없음
	ITEM_ANTIFLAG_DROP		= (1 << 7), // 버릴 수 없음
	ITEM_ANTIFLAG_SELL		= (1 << 8), // 팔 수 없음
	ITEM_ANTIFLAG_EMPIRE_A	= (1 << 9), // A 제국 사용 불가
	ITEM_ANTIFLAG_EMPIRE_B	= (1 << 10), // B 제국 사용 불가
	ITEM_ANTIFLAG_EMPIRE_C	= (1 << 11), // C 제국 사용 불가
	ITEM_ANTIFLAG_SAVE		= (1 << 12), // 저장되지 않음
	ITEM_ANTIFLAG_GIVE		= (1 << 13), // 거래 불가
	ITEM_ANTIFLAG_PKDROP	= (1 << 14), // PK시 떨어지지 않음
	ITEM_ANTIFLAG_STACK		= (1 << 15), // 합칠 수 없음
	ITEM_ANTIFLAG_MYSHOP	= (1 << 16), // 개인 상점에 올릴 수 없음
	ITEM_ANTIFLAG_SAFEBOX	= (1 << 17), // 창고에 넣을 수 없음
};

enum EItemWearableFlag
{
	WEARABLE_BODY	= (1 << 0),
	WEARABLE_HEAD	= (1 << 1),
	WEARABLE_FOOTS	= (1 << 2),
	WEARABLE_WRIST	= (1 << 3),
	WEARABLE_WEAPON	= (1 << 4),
	WEARABLE_NECK	= (1 << 5),
	WEARABLE_EAR	= (1 << 6),
	WEARABLE_UNIQUE	= (1 << 7),
	WEARABLE_SHIELD	= (1 << 8),
	WEARABLE_ARROW	= (1 << 9),
	WEARABLE_HAIR	= (1 << 10),
	WEARABLE_ABILITY		= (1 << 11),
	WEARABLE_COSTUME_BODY	= (1 << 12),
};

enum ELimitTypes
{
	LIMIT_NONE,

	LIMIT_LEVEL,
	LIMIT_STR,
	LIMIT_DEX,
	LIMIT_INT,
	LIMIT_CON,
	LIMIT_PCBANG,

	/// 착용 여부와 상관 없이 실시간으로 시간 차감 (socket0에 소멸 시간이 박힘: unix_timestamp 타입)
	LIMIT_REAL_TIME,						

	/// 아이템을 맨 처음 사용(혹은 착용) 한 순간부터 리얼타임 타이머 시작 
	/// 최초 사용 전에는 socket0에 사용가능시간(초단위, 0이면 프로토의 limit value값 사용) 값이 쓰여있다가 
	/// 아이템 사용시 socket1에 사용 횟수가 박히고 socket0에 unix_timestamp 타입의 소멸시간이 박힘.
	LIMIT_REAL_TIME_START_FIRST_USE,

	/// 아이템을 착용 중일 때만 사용 시간이 차감되는 아이템
	/// socket0에 남은 시간이 초단위로 박힘. (아이템 최초 사용시 해당 값이 0이면 프로토의 limit value값을 socket0에 복사)
	LIMIT_TIMER_BASED_ON_WEAR,

	LIMIT_MAX_NUM
};

enum EAttrAddonTypes
{
	ATTR_ADDON_NONE,
	// positive values are reserved for set
	ATTR_DAMAGE_ADDON = -1,
};

enum ERefineType
{
	REFINE_TYPE_NORMAL,
	REFINE_TYPE_NOT_USED1,
	REFINE_TYPE_SCROLL,
	REFINE_TYPE_HYUNIRON,
	REFINE_TYPE_MONEY_ONLY,
	REFINE_TYPE_MUSIN,
	REFINE_TYPE_BDRAGON,
};

#endif
