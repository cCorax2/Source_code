// vim:ts=8 sw=4
#ifndef __INC_CLIENTMANAGER_H__
#define __INC_CLIENTMANAGER_H__

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include "../../common/stl.h"
#include "../../common/building.h"
#include "../../common/auction_table.h"

#include "Peer.h"
#include "DBManager.h"
#include "LoginData.h"

class CPlayerTableCache;
class CItemCache;
class CItemPriceListTableCache;

class CPacketInfo
{
    public:
	void Add(int header);
	void Reset();

	std::map<int, int> m_map_info;
};

size_t CreatePlayerSaveQuery(char * pszQuery, size_t querySize, TPlayerTable * pkTab);

class CClientManager : public CNetBase, public singleton<CClientManager>
{
    public:
	typedef std::list<CPeer *>			TPeerList;
	typedef boost::unordered_map<DWORD, CPlayerTableCache *> TPlayerTableCacheMap;
	typedef boost::unordered_map<DWORD, CItemCache *> TItemCacheMap;
	typedef boost::unordered_set<CItemCache *, boost::hash<CItemCache*> > TItemCacheSet;
	typedef boost::unordered_map<DWORD, TItemCacheSet *> TItemCacheSetPtrMap;
	typedef boost::unordered_map<DWORD, CItemPriceListTableCache*> TItemPriceListCacheMap;
	typedef boost::unordered_map<short, BYTE> TChannelStatusMap;

	// MYSHOP_PRICE_LIST
	/// 아이템 가격정보 리스트 요청 정보
	/**
	 * first: Peer handle
	 * second: 요청한 플레이어의 ID
	 */
	typedef std::pair< DWORD, DWORD >		TItemPricelistReqInfo;
	// END_OF_MYSHOP_PRICE_LIST

	class ClientHandleInfo
	{
	    public:
		DWORD	dwHandle;
		DWORD	account_id;
		DWORD	player_id;
		BYTE	account_index;
		char	login[LOGIN_MAX_LEN + 1];
		char	safebox_password[SAFEBOX_PASSWORD_MAX_LEN + 1];
		char	ip[MAX_HOST_LENGTH + 1];

		TAccountTable * pAccountTable;
		TSafeboxTable * pSafebox;

		ClientHandleInfo(DWORD argHandle, DWORD dwPID = 0)
		{
		    dwHandle = argHandle;
		    pSafebox = NULL;
		    pAccountTable = NULL;
		    player_id = dwPID;
		};
		//독일선물기능용 생성자
		ClientHandleInfo(DWORD argHandle, DWORD dwPID, DWORD accountId)
		{
		    dwHandle = argHandle;
		    pSafebox = NULL;
		    pAccountTable = NULL;
		    player_id = dwPID;
			account_id = accountId;
		};

		~ClientHandleInfo()
		{
		    if (pSafebox)
			{
				delete pSafebox;
				pSafebox = NULL;
			}
		}
	};

	public:
	CClientManager();
	~CClientManager();

	bool	Initialize();
	time_t	GetCurrentTime();

	void	MainLoop();
	void	Quit();

	void	SetTablePostfix(const char* c_pszTablePostfix);
	void	SetPlayerIDStart(int iIDStart);
	int	GetPlayerIDStart() { return m_iPlayerIDStart; }

	int	GetPlayerDeleteLevelLimit() { return m_iPlayerDeleteLevelLimit; }

	void	SetChinaEventServer(bool flag) { m_bChinaEventServer = flag; }
	bool	IsChinaEventServer() { return m_bChinaEventServer; }

	DWORD	GetUserCount();	// 접속된 사용자 수를 리턴 한다.

	void	SendAllGuildSkillRechargePacket();
	void	SendTime();

	CPlayerTableCache *	GetPlayerCache(DWORD id);
	void			PutPlayerCache(TPlayerTable * pNew);

	void			CreateItemCacheSet(DWORD dwID);
	TItemCacheSet *		GetItemCacheSet(DWORD dwID);
	void			FlushItemCacheSet(DWORD dwID);

	CItemCache *		GetItemCache(DWORD id);
	void			PutItemCache(TPlayerItem * pNew, bool bSkipQuery = false);
	bool			DeleteItemCache(DWORD id);

	void			UpdatePlayerCache();
	void			UpdateItemCache();

	// MYSHOP_PRICE_LIST
	/// 가격정보 리스트 캐시를 가져온다.
	/**
	 * @param [in]	dwID 가격정보 리스트의 소유자.(플레이어 ID)
	 * @return	가격정보 리스트 캐시의 포인터
	 */
	CItemPriceListTableCache*	GetItemPriceListCache(DWORD dwID);

	/// 가격정보 리스트 캐시를 넣는다.
	/**
	 * @param [in]	pItemPriceList 캐시에 넣을 아이템 가격정보 리스트
	 *
	 * 캐시가 이미 있으면 Update 가 아닌 replace 한다.
	 */
	void			PutItemPriceListCache(const TItemPriceListTable* pItemPriceList);


	/// Flush 시간이 만료된 아이템 가격정보 리스트 캐시를 Flush 해주고 캐시에서 삭제한다.
	void			UpdateItemPriceListCache(void);
	// END_OF_MYSHOP_PRICE_LIST


	void			SendGuildSkillUsable(DWORD guild_id, DWORD dwSkillVnum, bool bUsable);

	void			SetCacheFlushCountLimit(int iLimit);

	template <class Func>
	Func		for_each_peer(Func f);

	CPeer *		GetAnyPeer();

	void			ForwardPacket(BYTE header, const void* data, int size, BYTE bChannel = 0, CPeer * except = NULL);

	void			SendNotice(const char * c_pszFormat, ...);

	char*			GetCommand(char* str);					//독일선물기능에서 명령어 얻는 함수
	void			ItemAward(CPeer * peer, char* login);	//독일 선물 기능

    protected:
	void	Destroy();

    private:
	bool		InitializeTables();
	bool		InitializeShopTable();
	bool		InitializeMobTable();
	bool		InitializeItemTable();
	bool		InitializeQuestItemTable();
	bool		InitializeSkillTable();
	bool		InitializeRefineTable();
	bool		InitializeBanwordTable();
	bool		InitializeItemAttrTable();
	bool		InitializeItemRareTable();
	bool		InitializeLandTable();
	bool		InitializeObjectProto();
	bool		InitializeObjectTable();
	bool		InitializeMonarch();

	// mob_proto.txt, item_proto.txt에서 읽은 mob_proto, item_proto를 real db에 반영.
	//	item_proto, mob_proto를 db에 반영하지 않아도, 게임 돌아가는데는 문제가 없지만,
	//	운영툴 등에서 db의 item_proto, mob_proto를 읽어 쓰기 때문에 문제가 발생한다.
	bool		MirrorMobTableIntoDB();
	bool		MirrorItemTableIntoDB();

	void		AddPeer(socket_t fd);
	void		RemovePeer(CPeer * pPeer);
	CPeer *		GetPeer(IDENT ident);

	int		AnalyzeQueryResult(SQLMsg * msg);
	int		AnalyzeErrorMsg(CPeer * peer, SQLMsg * msg);

	int		Process();

        void            ProcessPackets(CPeer * peer);

	CLoginData *	GetLoginData(DWORD dwKey);
	CLoginData *	GetLoginDataByLogin(const char * c_pszLogin);
	CLoginData *	GetLoginDataByAID(DWORD dwAID);

	void		InsertLoginData(CLoginData * pkLD);
	void		DeleteLoginData(CLoginData * pkLD);

	bool		InsertLogonAccount(const char * c_pszLogin, DWORD dwHandle, const char * c_pszIP);
	bool		DeleteLogonAccount(const char * c_pszLogin, DWORD dwHandle);
	bool		FindLogonAccount(const char * c_pszLogin);

	void		GuildCreate(CPeer * peer, DWORD dwGuildID);
	void		GuildSkillUpdate(CPeer * peer, TPacketGuildSkillUpdate* p);
	void		GuildExpUpdate(CPeer * peer, TPacketGuildExpUpdate* p);
	void		GuildAddMember(CPeer * peer, TPacketGDGuildAddMember* p);
	void		GuildChangeGrade(CPeer * peer, TPacketGuild* p);
	void		GuildRemoveMember(CPeer * peer, TPacketGuild* p);
	void		GuildChangeMemberData(CPeer * peer, TPacketGuildChangeMemberData* p);
	void		GuildDisband(CPeer * peer, TPacketGuild * p);
	void		GuildWar(CPeer * peer, TPacketGuildWar * p);
	void		GuildWarScore(CPeer * peer, TPacketGuildWarScore * p);
	void		GuildChangeLadderPoint(TPacketGuildLadderPoint* p);
	void		GuildUseSkill(TPacketGuildUseSkill* p);
	void		GuildDepositMoney(TPacketGDGuildMoney* p);
	void		GuildWithdrawMoney(CPeer* peer, TPacketGDGuildMoney* p);
	void		GuildWithdrawMoneyGiveReply(TPacketGDGuildMoneyWithdrawGiveReply* p);
	void		GuildWarBet(TPacketGDGuildWarBet * p);
	void		GuildChangeMaster(TPacketChangeGuildMaster* p);

	void		SetGuildWarEndTime(DWORD guild_id1, DWORD guild_id2, time_t tEndTime);

	void		QUERY_BOOT(CPeer * peer, TPacketGDBoot * p);

	void		QUERY_LOGIN(CPeer * peer, DWORD dwHandle, SLoginPacket* data);
	void		QUERY_LOGOUT(CPeer * peer, DWORD dwHandle, const char *);

	void		RESULT_LOGIN(CPeer * peer, SQLMsg *msg);

	void		QUERY_PLAYER_LOAD(CPeer * peer, DWORD dwHandle, TPlayerLoadPacket*);
	void		RESULT_COMPOSITE_PLAYER(CPeer * peer, SQLMsg * pMsg, DWORD dwQID);
	void		RESULT_PLAYER_LOAD(CPeer * peer, MYSQL_RES * pRes, ClientHandleInfo * pkInfo);
	void		RESULT_ITEM_LOAD(CPeer * peer, MYSQL_RES * pRes, DWORD dwHandle, DWORD dwPID);
	void		RESULT_QUEST_LOAD(CPeer * pkPeer, MYSQL_RES * pRes, DWORD dwHandle, DWORD dwPID);
	void		RESULT_AFFECT_LOAD(CPeer * pkPeer, MYSQL_RES * pRes, DWORD dwHandle);

	// PLAYER_INDEX_CREATE_BUG_FIX
	void		RESULT_PLAYER_INDEX_CREATE(CPeer *pkPeer, SQLMsg *msg);
	// END_PLAYER_INDEX_CREATE_BUG_FIX
 
	// MYSHOP_PRICE_LIST
	/// 가격정보 로드 쿼리에 대한 Result 처리
	/**
	 * @param	peer 가격정보를 요청한 Game server 의 peer 객체 포인터
	 * @param	pMsg 쿼리의 Result 로 받은 객체의 포인터
	 *
	 * 로드된 가격정보 리스트를 캐시에 저장하고 peer 에게 리스트를 보내준다.
	 */
	void		RESULT_PRICELIST_LOAD(CPeer* peer, SQLMsg* pMsg);

	/// 가격정보 업데이트를 위한 로드 쿼리에 대한 Result 처리
	/**
	 * @param	pMsg 쿼리의 Result 로 받은 객체의 포인터
	 *
	 * 로드된 정보로 가격정보 리스트 캐시를 만들고 업데이트 받은 가격정보로 업데이트 한다.
	 */
	void		RESULT_PRICELIST_LOAD_FOR_UPDATE(SQLMsg* pMsg);
	// END_OF_MYSHOP_PRICE_LIST

	void		QUERY_PLAYER_SAVE(CPeer * peer, DWORD dwHandle, TPlayerTable*);

	void		__QUERY_PLAYER_CREATE(CPeer * peer, DWORD dwHandle, TPlayerCreatePacket *);
	void		__QUERY_PLAYER_DELETE(CPeer * peer, DWORD dwHandle, TPlayerDeletePacket *);
	void		__RESULT_PLAYER_DELETE(CPeer * peer, SQLMsg* msg);

	void		QUERY_PLAYER_COUNT(CPeer * pkPeer, TPlayerCountPacket *);

	void		QUERY_ITEM_SAVE(CPeer * pkPeer, const char * c_pData);
	void		QUERY_ITEM_DESTROY(CPeer * pkPeer, const char * c_pData);
	void		QUERY_ITEM_FLUSH(CPeer * pkPeer, const char * c_pData);


	void		QUERY_QUEST_SAVE(CPeer * pkPeer, TQuestTable *, DWORD dwLen);
	void		QUERY_ADD_AFFECT(CPeer * pkPeer, TPacketGDAddAffect * p);
	void		QUERY_REMOVE_AFFECT(CPeer * pkPeer, TPacketGDRemoveAffect * p);

	void		QUERY_SAFEBOX_LOAD(CPeer * pkPeer, DWORD dwHandle, TSafeboxLoadPacket *, bool bMall);
	void		QUERY_SAFEBOX_SAVE(CPeer * pkPeer, TSafeboxTable * pTable);
	void		QUERY_SAFEBOX_CHANGE_SIZE(CPeer * pkPeer, DWORD dwHandle, TSafeboxChangeSizePacket * p);
	void		QUERY_SAFEBOX_CHANGE_PASSWORD(CPeer * pkPeer, DWORD dwHandle, TSafeboxChangePasswordPacket * p);

	void		RESULT_SAFEBOX_LOAD(CPeer * pkPeer, SQLMsg * msg);
	void		RESULT_SAFEBOX_CHANGE_SIZE(CPeer * pkPeer, SQLMsg * msg);
	void		RESULT_SAFEBOX_CHANGE_PASSWORD(CPeer * pkPeer, SQLMsg * msg);
	void		RESULT_SAFEBOX_CHANGE_PASSWORD_SECOND(CPeer * pkPeer, SQLMsg * msg);

	void		QUERY_EMPIRE_SELECT(CPeer * pkPeer, DWORD dwHandle, TEmpireSelectPacket * p);
	void		QUERY_SETUP(CPeer * pkPeer, DWORD dwHandle, const char * c_pData);

	void		SendPartyOnSetup(CPeer * peer);

	void		QUERY_HIGHSCORE_REGISTER(CPeer * peer, TPacketGDHighscore* data);
	void		RESULT_HIGHSCORE_REGISTER(CPeer * pkPeer, SQLMsg * msg);

	void		QUERY_FLUSH_CACHE(CPeer * pkPeer, const char * c_pData);

	void		QUERY_PARTY_CREATE(CPeer * peer, TPacketPartyCreate* p);
	void		QUERY_PARTY_DELETE(CPeer * peer, TPacketPartyDelete* p);
	void		QUERY_PARTY_ADD(CPeer * peer, TPacketPartyAdd* p);
	void		QUERY_PARTY_REMOVE(CPeer * peer, TPacketPartyRemove* p);
	void		QUERY_PARTY_STATE_CHANGE(CPeer * peer, TPacketPartyStateChange* p);
	void		QUERY_PARTY_SET_MEMBER_LEVEL(CPeer * peer, TPacketPartySetMemberLevel* p);

	void		QUERY_RELOAD_PROTO();

	void		QUERY_CHANGE_NAME(CPeer * peer, DWORD dwHandle, TPacketGDChangeName * p);
	void		GetPlayerFromRes(TPlayerTable * player_table, MYSQL_RES* res);

	void		QUERY_SMS(CPeer * pkPeer, TPacketGDSMS * p);
	void		QUERY_LOGIN_KEY(CPeer * pkPeer, TPacketGDLoginKey * p);

	void		AddGuildPriv(TPacketGiveGuildPriv* p);
	void		AddEmpirePriv(TPacketGiveEmpirePriv* p);
	void		AddCharacterPriv(TPacketGiveCharacterPriv* p);

	void		MoneyLog(TPacketMoneyLog* p);

	void		QUERY_AUTH_LOGIN(CPeer * pkPeer, DWORD dwHandle, TPacketGDAuthLogin * p);

	void		QUERY_LOGIN_BY_KEY(CPeer * pkPeer, DWORD dwHandle, TPacketGDLoginByKey * p);
	void		RESULT_LOGIN_BY_KEY(CPeer * peer, SQLMsg * msg);

	void		ChargeCash(const TRequestChargeCash * p);

	void		LoadEventFlag();
	void		SetEventFlag(TPacketSetEventFlag* p);
	void		SendEventFlagsOnSetup(CPeer* peer);

	void		BillingExpire(TPacketBillingExpire * p);
	void		BillingCheck(const char * data);

	void		SendAllLoginToBilling();
	void		SendLoginToBilling(CLoginData * pkLD, bool bLogin);

	// 결혼
	void		MarriageAdd(TPacketMarriageAdd * p);
	void		MarriageUpdate(TPacketMarriageUpdate * p);
	void		MarriageRemove(TPacketMarriageRemove * p);

	void		WeddingRequest(TPacketWeddingRequest * p);
	void		WeddingReady(TPacketWeddingReady * p);
	void		WeddingEnd(TPacketWeddingEnd * p);

	// MYSHOP_PRICE_LIST
	// 개인상점 가격정보

	/// 아이템 가격정보 리스트 업데이트 패킷(HEADER_GD_MYSHOP_PRICELIST_UPDATE) 처리함수
	/**
	 * @param [in]	pPacket 패킷 데이터의 포인터
	 */
	void		MyshopPricelistUpdate(const TPacketMyshopPricelistHeader* pPacket);

	/// 아이템 가격정보 리스트 요청 패킷(HEADER_GD_MYSHOP_PRICELIST_REQ) 처리함수
	/**
	 * @param	peer 패킷을 보낸 Game server 의 peer 객체의 포인터
	 * @param [in]	dwHandle 가격정보를 요청한 peer 의 핸들
	 * @param [in]	dwPlayerID 가격정보 리스트를 요청한 플레이어의 ID
	 */
	void		MyshopPricelistRequest(CPeer* peer, DWORD dwHandle, DWORD dwPlayerID);
	// END_OF_MYSHOP_PRICE_LIST

	// Building
	void		CreateObject(TPacketGDCreateObject * p);
	void		DeleteObject(DWORD dwID);
	void		UpdateLand(DWORD * pdw);

	// VCard
	void 		VCard(TPacketGDVCard * p);
	void		VCardProcess();

	// BLOCK_CHAT
	void		BlockChat(TPacketBlockChat * p);
	// END_OF_BLOCK_CHAT
   
    private:
	int					m_looping;
	socket_t				m_fdAccept;	// 접속 받는 소켓
	TPeerList				m_peerList;

	CPeer *					m_pkAuthPeer;

	// LoginKey, LoginData pair
	typedef boost::unordered_map<DWORD, CLoginData *> TLoginDataByLoginKey;
	TLoginDataByLoginKey			m_map_pkLoginData;

	// Login LoginData pair
	typedef boost::unordered_map<std::string, CLoginData *> TLoginDataByLogin;
	TLoginDataByLogin			m_map_pkLoginDataByLogin;
	
	// AccountID LoginData pair
	typedef boost::unordered_map<DWORD, CLoginData *> TLoginDataByAID;
	TLoginDataByAID				m_map_pkLoginDataByAID;

	// Login LoginData pair (실제 로그인 되어있는 계정)
	typedef boost::unordered_map<std::string, CLoginData *> TLogonAccountMap;
	TLogonAccountMap			m_map_kLogonAccount;

	int					m_iPlayerIDStart;
	int					m_iPlayerDeleteLevelLimit;
	int					m_iPlayerDeleteLevelLimitLower;
	bool					m_bChinaEventServer;

	std::vector<TMobTable>			m_vec_mobTable;
	std::vector<TItemTable>			m_vec_itemTable;
	std::map<DWORD, TItemTable *>		m_map_itemTableByVnum;

	int					m_iShopTableSize;
	TShopTable *				m_pShopTable;

	int					m_iRefineTableSize;
	TRefineTable*				m_pRefineTable;

	std::vector<TSkillTable>		m_vec_skillTable;
	std::vector<TBanwordTable>		m_vec_banwordTable;
	std::vector<TItemAttrTable>		m_vec_itemAttrTable;
	std::vector<TItemAttrTable>		m_vec_itemRareTable;

	std::vector<building::TLand>		m_vec_kLandTable;
	std::vector<building::TObjectProto>	m_vec_kObjectProto;
	std::map<DWORD, building::TObject *>	m_map_pkObjectTable;

	std::queue<TPacketGDVCard>		m_queue_vcard;

	bool					m_bShutdowned;

	TPlayerTableCacheMap			m_map_playerCache;  // 플레이어 id가 key

	TItemCacheMap				m_map_itemCache;  // 아이템 id가 key
	TItemCacheSetPtrMap			m_map_pkItemCacheSetPtr;  // 플레이어 id가 key, 이 플레이어가 어떤 아이템 캐쉬를 가지고 있나?

	// MYSHOP_PRICE_LIST
	/// 플레이어별 아이템 가격정보 리스트 map. key: 플레이어 ID, value: 가격정보 리스트 캐시
	TItemPriceListCacheMap m_mapItemPriceListCache;  ///< 플레이어별 아이템 가격정보 리스트
	// END_OF_MYSHOP_PRICE_LIST

	TChannelStatusMap m_mChannelStatus;

	struct TPartyInfo
	{
	    BYTE bRole;
	    int bLevel;

		TPartyInfo() :bRole(0), bLevel(0)
		{
		}
	};

	typedef std::map<DWORD, TPartyInfo>	TPartyMember;
	typedef std::map<DWORD, TPartyMember>	TPartyMap;
	typedef std::map<BYTE, TPartyMap>	TPartyChannelMap;
	TPartyChannelMap m_map_pkChannelParty;

	typedef std::map<std::string, long>	TEventFlagMap;
	TEventFlagMap m_map_lEventFlag;

	BYTE					m_bLastHeader;
	int					m_iCacheFlushCount;
	int					m_iCacheFlushCountLimit;

    private :
	TItemIDRangeTable m_itemRange;

    public :
	bool InitializeNowItemID();
	DWORD GetItemID();
	DWORD GainItemID();
	TItemIDRangeTable GetItemRange() { return m_itemRange; }

	//BOOT_LOCALIZATION
    public:
	/* 로컬 정보 초기화 
	 **/
	bool InitializeLocalization(); 

    private:
	std::vector<tLocale> m_vec_Locale;
	//END_BOOT_LOCALIZATION
	//ADMIN_MANAGER

	bool __GetAdminInfo(const char *szIP, std::vector<tAdminInfo> & rAdminVec);
	bool __GetHostInfo(std::vector<std::string> & rIPVec);
	//END_ADMIN_MANAGER

		
	//RELOAD_ADMIN
	void ReloadAdmin(CPeer * peer, TPacketReloadAdmin * p);
	//END_RELOAD_ADMIN
	void BreakMarriage(CPeer * peer, const char * data);

	struct TLogoutPlayer
	{
	    DWORD	pid;
	    time_t	time;

	    bool operator < (const TLogoutPlayer & r) 
	    {
		return (pid < r.pid);
	    }
	};

	typedef boost::unordered_map<DWORD, TLogoutPlayer*> TLogoutPlayerMap;
	TLogoutPlayerMap m_map_logout;
	
	void InsertLogoutPlayer(DWORD pid);
	void DeleteLogoutPlayer(DWORD pid);
	void UpdateLogoutPlayer();
	void UpdateItemCacheSet(DWORD pid);

	void FlushPlayerCacheSet(DWORD pid);

	//MONARCH
	void Election(CPeer * peer, DWORD dwHandle, const char * p);
	void Candidacy(CPeer * peer, DWORD dwHandle, const char * p);
	void AddMonarchMoney(CPeer * peer, DWORD dwHandle, const char * p);
	void TakeMonarchMoney(CPeer * peer, DWORD dwHandle, const char * p);
	void ComeToVote(CPeer * peer, DWORD dwHandle, const char * p);
	void RMCandidacy(CPeer * peer, DWORD dwHandle, const char * p);
	void SetMonarch(CPeer * peer, DWORD dwHandle, const char * p);
	void RMMonarch(CPeer * peer, DWORD dwHandle, const char * p);
								

	void DecMonarchMoney(CPeer * peer, DWORD dwHandle, const char * p);
	//END_MONARCH

	void ChangeMonarchLord(CPeer* peer, DWORD dwHandle, TPacketChangeMonarchLord* info);
	void BlockException(TPacketBlockException *data);

	void SendSpareItemIDRange(CPeer* peer);

	void UpdateHorseName(TPacketUpdateHorseName* data, CPeer* peer);
	void AckHorseName(DWORD dwPID, CPeer* peer);
	void DeleteLoginKey(TPacketDC *data);
	void ResetLastPlayerID(const TPacketNeedLoginLogInfo* data);
	//delete gift notify icon
	void DeleteAwardId(TPacketDeleteAwardID* data);
	void UpdateChannelStatus(TChannelStatus* pData);
	void RequestChannelStatus(CPeer* peer, DWORD dwHandle);
#ifdef __AUCTION__
	void EnrollInAuction (CPeer * peer, DWORD owner_id, AuctionEnrollProductInfo* data);
	void EnrollInSale (CPeer * peer, DWORD owner_id, AuctionEnrollSaleInfo* data);
	void EnrollInWish (CPeer * peer, DWORD wisher_id, AuctionEnrollWishInfo* data);
	void AuctionBid (CPeer * peer, DWORD bidder_id, AuctionBidInfo* data);
	void AuctionImpur (CPeer * peer, DWORD purchaser_id, AuctionImpurInfo* data);
	void AuctionGetAuctionedItem (CPeer * peer, DWORD actor_id, DWORD item_id);
	void AuctionBuySoldItem (CPeer * peer, DWORD actor_id, DWORD item_id);
	void AuctionCancelAuction (CPeer * peer, DWORD actor_id, DWORD item_id);
	void AuctionCancelWish (CPeer * peer, DWORD actor_id, DWORD item_num);
	void AuctionCancelSale (CPeer * peer, DWORD actor_id, DWORD item_id);
	void AuctionDeleteAuctionItem (CPeer * peer, DWORD actor_id, DWORD item_id);
	void AuctionDeleteSaleItem (CPeer * peer, DWORD actor_id, DWORD item_id);
	void AuctionReBid (CPeer * peer, DWORD bidder_id, AuctionBidInfo* data);
	void AuctionBidCancel (CPeer * peer, DWORD bidder_id, DWORD item_id);
#endif
};

template<class Func>	
Func CClientManager::for_each_peer(Func f)
{
    TPeerList::iterator it;
    for (it = m_peerList.begin(); it!=m_peerList.end();++it)
    {
	f(*it);
    }
    return f;
}
#endif
