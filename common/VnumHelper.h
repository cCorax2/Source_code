#ifndef __HEADER_VNUM_HELPER__
#define	__HEADER_VNUM_HELPER__

/**
	이미 존재하거나 앞으로 추가될 아이템, 몹 등을 소스에서 식별할 때 현재는 모두
	식별자(숫자=VNum)를 하드코딩하는 방식으로 되어있어서 가독성이 매우 떨어지는데

	앞으로는 소스만 봐도 어떤 아이템(혹은 몹)인지 알 수 있게 하자는 승철님의 제안으로 추가.

		* 이 파일은 변경이 잦을것으로 예상되는데 PCH에 넣으면 바뀔 때마다 전체 컴파일 해야하니 
		일단은 필요한 cpp파일에서 include 해서 쓰도록 했음.

		* cpp에서 구현하면 컴파일 ~ 링크해야하니 그냥 common에 헤더만 넣었음. (game, db프로젝트 둘 다 사용 예정)

	@date	2011. 8. 29.
*/


class CItemVnumHelper
{
public:
	/// 독일 DVD용 불사조 소환권
	static	const bool	IsPhoenix(DWORD vnum)				{ return 53001 == vnum; }		// NOTE: 불사조 소환 아이템은 53001 이지만 mob-vnum은 34001 입니다.

	/// 라마단 이벤트 초승달의 반지 (원래는 라마단 이벤트용 특수 아이템이었으나 앞으로 여러 방향으로 재활용해서 계속 쓴다고 함)
	static	const bool	IsRamadanMoonRing(DWORD vnum)		{ return 71135 == vnum; }

	/// 할로윈 사탕 (스펙은 초승달의 반지와 동일)
	static	const bool	IsHalloweenCandy(DWORD vnum)		{ return 71136 == vnum; }

	/// 크리스마스 행복의 반지
	static	const bool	IsHappinessRing(DWORD vnum)		{ return 71143 == vnum; }

	/// 발렌타인 사랑의 팬던트 
	static	const bool	IsLovePendant(DWORD vnum)		{ return 71145 == vnum; }
};

class CMobVnumHelper
{
public:
	/// 독일 DVD용 불사조 몹 번호
	static	bool	IsPhoenix(DWORD vnum)				{ return 34001 == vnum; }
	static	bool	IsIcePhoenix(DWORD vnum)				{ return 34003 == vnum; }
	/// PetSystem이 관리하는 펫인가?
	static	bool	IsPetUsingPetSystem(DWORD vnum)	{ return (IsPhoenix(vnum) || IsReindeerYoung(vnum)) || IsIcePhoenix(vnum); }

	/// 2011년 크리스마스 이벤트용 펫 (아기 순록)
	static	bool	IsReindeerYoung(DWORD vnum)	{ return 34002 == vnum; }

	/// 라마단 이벤트 보상용 흑마(20119) .. 할로윈 이벤트용 라마단 흑마 클론(스펙은 같음, 20219)
	static	bool	IsRamadanBlackHorse(DWORD vnum)		{ return 20119 == vnum || 20219 == vnum || 22022 == vnum; }
};

class CVnumHelper
{
};


#endif	//__HEADER_VNUM_HELPER__