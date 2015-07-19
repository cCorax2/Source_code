#include "stdafx.h"
#include <fstream>
#include "constants.h"
#include "buffer_manager.h"
#include "packet.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "questmanager.h"
#include "lzo_manager.h"
#include "item.h"
#include "config.h"
#include "xmas_event.h"
#include "target.h"
#include "party.h"
#include "locale_service.h"

#include "dungeon.h"

DWORD g_GoldDropTimeLimitValue = 0;
extern bool DropEvent_CharStone_SetValue(const std::string& name, int value);
extern bool DropEvent_RefineBox_SetValue (const std::string& name, int value);

namespace quest
{
	using namespace std;

	CQuestManager::CQuestManager()
		: m_pSelectedDungeon(NULL), m_dwServerTimerArg(0), m_iRunningEventIndex(0), L(NULL), m_bNoSend (false),
		m_CurrentRunningState(NULL), m_pCurrentCharacter(NULL), m_pCurrentNPCCharacter(NULL), m_pCurrentPartyMember(NULL),
		m_pCurrentPC(NULL),  m_iCurrentSkin(0), m_bError(false), m_pOtherPCBlockRootPC(NULL)
	{
	}

	CQuestManager::~CQuestManager()
	{
		Destroy();
	}

	void CQuestManager::Destroy()
	{
		if (L)
		{
			lua_close(L);
			L = NULL;
		}
	}	

	bool CQuestManager::Initialize()
	{
		if (g_bAuthServer)
			return true;

		if (!InitializeLua())
			return false;

		m_pSelectedDungeon = NULL;

		m_mapEventName.insert(TEventNameMap::value_type("click", QUEST_CLICK_EVENT));		// NPC를 클릭
		m_mapEventName.insert(TEventNameMap::value_type("kill", QUEST_KILL_EVENT));		// Mob을 사냥
		m_mapEventName.insert(TEventNameMap::value_type("timer", QUEST_TIMER_EVENT));		// 미리 지정해둔 시간이 지남
		m_mapEventName.insert(TEventNameMap::value_type("levelup", QUEST_LEVELUP_EVENT));	// 레벨업을 함
		m_mapEventName.insert(TEventNameMap::value_type("login", QUEST_LOGIN_EVENT));		// 로그인 시
		m_mapEventName.insert(TEventNameMap::value_type("logout", QUEST_LOGOUT_EVENT));		// 로그아웃 시
		m_mapEventName.insert(TEventNameMap::value_type("button", QUEST_BUTTON_EVENT));		// 퀘스트 버튼을 누름
		m_mapEventName.insert(TEventNameMap::value_type("info", QUEST_INFO_EVENT));		// 퀘스트 정보창을 염
		m_mapEventName.insert(TEventNameMap::value_type("chat", QUEST_CHAT_EVENT));		// 특정 키워드로 대화를 함
		m_mapEventName.insert(TEventNameMap::value_type("in", QUEST_ATTR_IN_EVENT));		// 맵의 특정 속성에 들어감
		m_mapEventName.insert(TEventNameMap::value_type("out", QUEST_ATTR_OUT_EVENT));		// 맵의 특정 속성에서 나옴
		m_mapEventName.insert(TEventNameMap::value_type("use", QUEST_ITEM_USE_EVENT));		// 퀘스트 아이템을 사용
		m_mapEventName.insert(TEventNameMap::value_type("server_timer", QUEST_SERVER_TIMER_EVENT));	// 서버 타이머 (아직 테스트 안됐음)
		m_mapEventName.insert(TEventNameMap::value_type("enter", QUEST_ENTER_STATE_EVENT));	// 현재 스테이트가 됨
		m_mapEventName.insert(TEventNameMap::value_type("leave", QUEST_LEAVE_STATE_EVENT));	// 현재 스테이트에서 다른 스테이트로 바뀜
		m_mapEventName.insert(TEventNameMap::value_type("letter", QUEST_LETTER_EVENT));		// 로긴 하거나 스테이트가 바껴 새로 정보를 세팅해줘야함
		m_mapEventName.insert(TEventNameMap::value_type("take", QUEST_ITEM_TAKE_EVENT));	// 아이템을 받음
		m_mapEventName.insert(TEventNameMap::value_type("target", QUEST_TARGET_EVENT));		// 타겟
		m_mapEventName.insert(TEventNameMap::value_type("party_kill", QUEST_PARTY_KILL_EVENT));	// 파티 멤버가 몬스터를 사냥 (리더에게 옴)
		m_mapEventName.insert(TEventNameMap::value_type("unmount", QUEST_UNMOUNT_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("pick", QUEST_ITEM_PICK_EVENT));	// 떨어져있는 아이템을 습득함.
		m_mapEventName.insert(TEventNameMap::value_type("sig_use", QUEST_SIG_USE_EVENT));		// Special item group에 속한 아이템을 사용함.
		m_mapEventName.insert(TEventNameMap::value_type("item_informer", QUEST_ITEM_INFORMER_EVENT));	// 독일선물기능테스트

		m_bNoSend = false;

		m_iCurrentSkin = QUEST_SKIN_NORMAL;

		{
			ifstream inf((g_stQuestDir + "/questnpc.txt").c_str());
			int line = 0;

			if (!inf.is_open())
				sys_err( "QUEST Cannot open 'questnpc.txt'");
			else
				sys_log(0, "QUEST can open 'questnpc.txt' (%s)", g_stQuestDir.c_str() );

			while (1)
			{
				unsigned int vnum;

				inf >> vnum;

				line++;

				if (inf.fail())
					break;

				string s;
				getline(inf, s);
				unsigned int li = 0, ri = s.size()-1;
				while (li < s.size() && isspace(s[li])) li++;
				while (ri > 0 && isspace(s[ri])) ri--;

				if (ri < li) 
				{
					sys_err("QUEST questnpc.txt:%d:npc name error",line);
					continue;
				}

				s = s.substr(li, ri-li+1);

				int	n = 0;
				str_to_number(n, s.c_str());
				if (n)
					continue;

				//cout << '-' << s << '-' << endl;
				if ( test_server )
					sys_log(0, "QUEST reading script of %s(%d)", s.c_str(), vnum);
				m_mapNPC[vnum].Set(vnum, s);
				m_mapNPCNameID[s] = vnum;
			}

			// notarget quest
			m_mapNPC[0].Set(0, "notarget");
		}

		if (g_iUseLocale)
		{
			SetEventFlag("guild_withdraw_delay", 1);
			SetEventFlag("guild_disband_delay", 1);
		}
		else
		{
			SetEventFlag("guild_withdraw_delay", 3);
			SetEventFlag("guild_disband_delay", 7);
		}
		return true;
	}

	unsigned int CQuestManager::FindNPCIDByName(const string& name)
	{
		map<string, unsigned int>::iterator it = m_mapNPCNameID.find(name);
		return it != m_mapNPCNameID.end() ? it->second : 0;
	}

	void CQuestManager::SelectItem(unsigned int pc, unsigned int selection)
	{
		PC* pPC = GetPC(pc);
		if (pPC && pPC->IsRunning() && pPC->GetRunningQuestState()->suspend_state == SUSPEND_STATE_SELECT_ITEM)
		{
			pPC->SetSendDoneFlag();
			pPC->GetRunningQuestState()->args=1;
			lua_pushnumber(pPC->GetRunningQuestState()->co,selection);

			if (!RunState(*pPC->GetRunningQuestState()))
			{
				CloseState(*pPC->GetRunningQuestState());
				pPC->EndRunning();
			}
		}
	}

	void CQuestManager::Confirm(unsigned int pc, EQuestConfirmType confirm, unsigned int pc2)
	{
		PC* pPC = GetPC(pc);

		if (!pPC->IsRunning())
		{
			sys_err("no quest running for pc, cannot process input : %u", pc);
			return;
		}

		if (pPC->GetRunningQuestState()->suspend_state != SUSPEND_STATE_CONFIRM)
		{
			sys_err("not wait for a confirm : %u %d", pc, pPC->GetRunningQuestState()->suspend_state);
			return;
		}

		if (pc2 && !pPC->IsConfirmWait(pc2))
		{
			sys_err("not wait for a confirm : %u %d", pc, pPC->GetRunningQuestState()->suspend_state);
			return;
		}

		pPC->ClearConfirmWait();

		pPC->SetSendDoneFlag();

		pPC->GetRunningQuestState()->args=1;
		lua_pushnumber(pPC->GetRunningQuestState()->co, confirm);

		AddScript("[END_CONFIRM_WAIT]");
		SetSkinStyle(QUEST_SKIN_NOWINDOW);
		SendScript();

		if (!RunState(*pPC->GetRunningQuestState()))
		{
			CloseState(*pPC->GetRunningQuestState());
			pPC->EndRunning();
		}

	}

	void CQuestManager::Input(unsigned int pc, const char* msg)
	{
		PC* pPC = GetPC(pc);
		if (!pPC)
		{
			sys_err("no pc! : %u",pc);
			return;
		}

		if (!pPC->IsRunning())
		{
			sys_err("no quest running for pc, cannot process input : %u", pc);
			return;
		}

		if (pPC->GetRunningQuestState()->suspend_state != SUSPEND_STATE_INPUT)
		{
			sys_err("not wait for a input : %u %d", pc, pPC->GetRunningQuestState()->suspend_state);
			return;
		}

		pPC->SetSendDoneFlag();

		pPC->GetRunningQuestState()->args=1;
		lua_pushstring(pPC->GetRunningQuestState()->co,msg);

		if (!RunState(*pPC->GetRunningQuestState()))
		{
			CloseState(*pPC->GetRunningQuestState());
			pPC->EndRunning();
		}
	}

	void CQuestManager::Select(unsigned int pc, unsigned int selection)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)) && pPC->IsRunning() && pPC->GetRunningQuestState()->suspend_state==SUSPEND_STATE_SELECT)
		{
			pPC->SetSendDoneFlag();

			if (!pPC->GetRunningQuestState()->chat_scripts.empty())
			{
				// 채팅 이벤트인 경우
				// 현재 퀘스트는 어느 퀘스트를 실행할 것인가를 고르는 퀘스트 이므로
				// 끝내고 선택된 퀘스트를 실행한다.
				QuestState& old_qs = *pPC->GetRunningQuestState();
				CloseState(old_qs);

				if (selection >= pPC->GetRunningQuestState()->chat_scripts.size())
				{
					pPC->SetSendDoneFlag();
					GotoEndState(old_qs);
					pPC->EndRunning();
				}
				else
				{
					AArgScript* pas = pPC->GetRunningQuestState()->chat_scripts[selection];
					ExecuteQuestScript(*pPC, pas->quest_index, pas->state_index, pas->script.GetCode(), pas->script.GetSize());
				}
			}
			else
			{
				// on default 
				pPC->GetRunningQuestState()->args=1;
				lua_pushnumber(pPC->GetRunningQuestState()->co,selection+1);

				if (!RunState(*pPC->GetRunningQuestState()))
				{
					CloseState(*pPC->GetRunningQuestState());
					pPC->EndRunning();
				}
			}
		}
		else
		{
			sys_err("wrong QUEST_SELECT request! : %d",pc);
		}
	}

	void CQuestManager::Resume(unsigned int pc)
	{
		PC * pPC;

		if ((pPC = GetPC(pc)) && pPC->IsRunning() && pPC->GetRunningQuestState()->suspend_state == SUSPEND_STATE_PAUSE)
		{
			pPC->SetSendDoneFlag();
			pPC->GetRunningQuestState()->args = 0;

			if (!RunState(*pPC->GetRunningQuestState()))
			{
				CloseState(*pPC->GetRunningQuestState());
				pPC->EndRunning();
			}
		}
		else
		{
			//cerr << pPC << endl;
			//cerr << pPC->IsRunning() << endl;
			//cerr << pPC->GetRunningQuestState()->suspend_state;
			//cerr << SUSPEND_STATE_WAIT << endl;
			//cerr << "wrong QUEST_WAIT request! : " << pc << endl;
			sys_err("wrong QUEST_WAIT request! : %d",pc);
		}
	}

	void CQuestManager::EnterState(DWORD pc, DWORD quest_index, int state)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnEnterState(*pPC, quest_index, state);
		}
		else
			sys_err("QUEST no such pc id : %d", pc);
	}

	void CQuestManager::LeaveState(DWORD pc, DWORD quest_index, int state)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnLeaveState(*pPC, quest_index, state);
		}
		else
			sys_err("QUEST no such pc id : %d", pc);
	}

	void CQuestManager::Letter(DWORD pc, DWORD quest_index, int state)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnLetter(*pPC, quest_index, state);
		}
		else
			sys_err("QUEST no such pc id : %d", pc);
	}

	void CQuestManager::LogoutPC(LPCHARACTER ch)
	{
		PC * pPC = GetPC(ch->GetPlayerID());

		if (pPC && pPC->IsRunning())
		{
			CloseState(*pPC->GetRunningQuestState());
			pPC->CancelRunning();
		}

		// 지우기 전에 로그아웃 한다.
		Logout(ch->GetPlayerID());

		if (ch == m_pCurrentCharacter)
		{
			m_pCurrentCharacter = NULL;
			m_pCurrentPC = NULL;
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	//
	// Quest Event 관련
	//
	///////////////////////////////////////////////////////////////////////////////////////////
	void CQuestManager::Login(unsigned int pc, const char * c_pszQuest)
	{
		PC * pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnLogin(*pPC, c_pszQuest);
		}
		else
		{
			sys_err("QUEST no such pc id : %d", pc);
		}
	}

	void CQuestManager::Logout(unsigned int pc)
	{
		PC * pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnLogout(*pPC);
		}
		else
			sys_err("QUEST no such pc id : %d", pc);
	}

	void CQuestManager::Kill(unsigned int pc, unsigned int npc)
	{
		//m_CurrentNPCRace = npc;
		PC * pPC;

		sys_log(0, "CQuestManager::Kill QUEST_KILL_EVENT (pc=%d, npc=%d)", pc, npc);

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			/* [hyo] 몹 kill시 중복 카운팅 이슈 관련한 수정사항
			   quest script에 when 171.kill begin ... 등의 코드로 인하여 스크립트가 처리되었더라도
			   바로 return하지 않고 다른 검사도 수행하도록 변경함. (2011/07/21)
			*/   
			// call script
			if(npc > 0)
			{
				m_mapNPC[npc].OnKill(*pPC);
			}

			LPCHARACTER ch = GetCurrentCharacterPtr();
			LPPARTY pParty = ch->GetParty();
			LPCHARACTER leader = pParty ? pParty->GetLeaderCharacter() : ch;

			if (leader)
			{
				m_pCurrentPartyMember = ch;

				if (m_mapNPC[npc].OnPartyKill(*GetPC(leader->GetPlayerID())))
					return;

				pPC = GetPC(pc);
			}

			if (m_mapNPC[QUEST_NO_NPC].OnKill(*pPC))
				return;

			if (leader)
			{
				m_pCurrentPartyMember = ch;
				m_mapNPC[QUEST_NO_NPC].OnPartyKill(*GetPC(leader->GetPlayerID()));
			}
		}
		else
			sys_err("QUEST: no such pc id : %d", pc);
	}

	bool CQuestManager::ServerTimer(unsigned int npc, unsigned int arg)
	{
		SetServerTimerArg(arg);
		sys_log(0, "XXX ServerTimer Call NPC %p", GetPCForce(0));
		m_pCurrentPC = GetPCForce(0);
		m_pCurrentCharacter = NULL;
		m_pSelectedDungeon = NULL;
		return m_mapNPC[npc].OnServerTimer(*m_pCurrentPC);
	}

	bool CQuestManager::Timer(unsigned int pc, unsigned int npc)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
				return false;
			}
			// call script
			return m_mapNPC[npc].OnTimer(*pPC);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST TIMER_EVENT no such pc id : %d", pc);
			return false;
		}
		//cerr << "QUEST TIMER" << endl;
	}

	void CQuestManager::LevelUp(unsigned int pc)
	{
		PC * pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnLevelUp(*pPC);
		}
		else
		{
			sys_err("QUEST LEVELUP_EVENT no such pc id : %d", pc);
		}
	}

	void CQuestManager::AttrIn(unsigned int pc, LPCHARACTER ch, int attr)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			m_pCurrentPartyMember = ch;
			if (!CheckQuestLoaded(pPC))
				return;

			// call script
			m_mapNPC[attr+QUEST_ATTR_NPC_START].OnAttrIn(*pPC);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST no such pc id : %d", pc);
		}
	}

	void CQuestManager::AttrOut(unsigned int pc, LPCHARACTER ch, int attr)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			//m_pCurrentCharacter = ch;
			m_pCurrentPartyMember = ch;
			if (!CheckQuestLoaded(pPC))
				return;

			// call script
			m_mapNPC[attr+QUEST_ATTR_NPC_START].OnAttrOut(*pPC);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST no such pc id : %d", pc);
		}
	}

	bool CQuestManager::Target(unsigned int pc, DWORD dwQuestIndex, const char * c_pszTargetName, const char * c_pszVerb)
	{
		PC * pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return false;

			bool bRet;
			return m_mapNPC[QUEST_NO_NPC].OnTarget(*pPC, dwQuestIndex, c_pszTargetName, c_pszVerb, bRet);
		}

		return false;
	}

	void CQuestManager::QuestInfo(unsigned int pc, unsigned int quest_index)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			// call script
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pc);

				if (ch)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("퀘스트를 로드하는 중입니다. 잠시만 기다려 주십시오."));

				return;
			}

			m_mapNPC[QUEST_NO_NPC].OnInfo(*pPC, quest_index);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST INFO_EVENT no such pc id : %d", pc);
		}
	}

	void CQuestManager::QuestButton(unsigned int pc, unsigned int quest_index)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			// call script
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pc);
				if (ch)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("퀘스트를 로드하는 중입니다. 잠시만 기다려 주십시오."));
				}
				return;
			}
			m_mapNPC[QUEST_NO_NPC].OnButton(*pPC, quest_index);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST CLICK_EVENT no such pc id : %d", pc);
		}
	}

	bool CQuestManager::TakeItem(unsigned int pc, unsigned int npc, LPITEM item)
	{
		//m_CurrentNPCRace = npc;
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pc);
				if (ch)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("퀘스트를 로드하는 중입니다. 잠시만 기다려 주십시오."));
				}
				return false;
			}
			// call script
			SetCurrentItem(item);
			return m_mapNPC[npc].OnTakeItem(*pPC);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST USE_ITEM_EVENT no such pc id : %d", pc);
			return false;
		}
	}

	bool CQuestManager::UseItem(unsigned int pc, LPITEM item, bool bReceiveAll)
	{
		if (test_server)
			sys_log( 0, "questmanager::UseItem Start : itemVnum : %d PC : %d", item->GetOriginalVnum(), pc);
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pc);
				if (ch)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("퀘스트를 로드하는 중입니다. 잠시만 기다려 주십시오."));
				}
				return false;
			}
			// call script
			SetCurrentItem(item);
			/*
			if (test_server)
			{
				sys_log( 0, "Quest UseItem Start : itemVnum : %d PC : %d", item->GetOriginalVnum(), pc);
				itertype(m_mapNPC) it = m_mapNPC.begin();
				itertype(m_mapNPC) end = m_mapNPC.end();
				for( ; it != end ; ++it)
				{
					sys_log( 0, "Quest UseItem : vnum : %d item Vnum : %d", it->first, item->GetOriginalVnum());
				}
			}
			if(test_server)
			sys_log( 0, "questmanager:useItem: mapNPCVnum : %d\n", m_mapNPC[item->GetVnum()].GetVnum());
			*/

			return m_mapNPC[item->GetVnum()].OnUseItem(*pPC, bReceiveAll);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST USE_ITEM_EVENT no such pc id : %d", pc);
			return false;
		}
	}

	// Speical Item Group에 정의된 Group Use
	bool CQuestManager::SIGUse(unsigned int pc, DWORD sig_vnum, LPITEM item, bool bReceiveAll)
	{
		if (test_server)
			sys_log( 0, "questmanager::SIGUse Start : itemVnum : %d PC : %d", item->GetOriginalVnum(), pc);
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pc);
				if (ch)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("퀘스트를 로드하는 중입니다. 잠시만 기다려 주십시오."));
				}
				return false;
			}
			// call script
			SetCurrentItem(item);

			return m_mapNPC[sig_vnum].OnSIGUse(*pPC, bReceiveAll);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST USE_ITEM_EVENT no such pc id : %d", pc);
			return false;
		}
	}

	bool CQuestManager::GiveItemToPC(unsigned int pc, LPCHARACTER pkChr)
	{
		if (!pkChr->IsPC())
			return false;

		PC * pPC = GetPC(pc);

		if (pPC)
		{
			if (!CheckQuestLoaded(pPC))
				return false;

			TargetInfo * pInfo = CTargetManager::instance().GetTargetInfo(pc, TARGET_TYPE_VID, pkChr->GetVID());

			if (pInfo)
			{
				bool bRet;

				if (m_mapNPC[QUEST_NO_NPC].OnTarget(*pPC, pInfo->dwQuestIndex, pInfo->szTargetName, "click", bRet))
					return true;
			}
		}

		return false;
	}

	bool CQuestManager::Click(unsigned int pc, LPCHARACTER pkChrTarget)
	{
		PC * pPC = GetPC(pc);

		if (pPC)
		{
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pc);

				if (ch)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("퀘스트를 로드하는 중입니다. 잠시만 기다려 주십시오."));

				return false;
			}

			TargetInfo * pInfo = CTargetManager::instance().GetTargetInfo(pc, TARGET_TYPE_VID, pkChrTarget->GetVID());
			if (test_server)
			{
				sys_log(0, "CQuestManager::Click(pid=%d, npc_name=%s) - target_info(%x)", pc, pkChrTarget->GetName(), pInfo);
			}

			if (pInfo)
			{
				bool bRet;
				if (m_mapNPC[QUEST_NO_NPC].OnTarget(*pPC, pInfo->dwQuestIndex, pInfo->szTargetName, "click", bRet))
					return bRet;
			}

			DWORD dwCurrentNPCRace = pkChrTarget->GetRaceNum();

			if (pkChrTarget->IsNPC())
			{
				map<unsigned int, NPC>::iterator it = m_mapNPC.find(dwCurrentNPCRace);

				if (it == m_mapNPC.end())
				{
					sys_err("CQuestManager::Click(pid=%d, target_npc_name=%s) - NOT EXIST NPC RACE VNUM[%d]",
							pc, 
							pkChrTarget->GetName(), 
							dwCurrentNPCRace);
					return false;
				}

				// call script
				if (it->second.HasChat())
				{
					// if have chat, give chat
					if (test_server)
						sys_log(0, "CQuestManager::Click->OnChat");

					if (!it->second.OnChat(*pPC))
					{
						if (test_server)
							sys_log(0, "CQuestManager::Click->OnChat Failed");

						return it->second.OnClick(*pPC);
					}

					return true;
				}
				else
				{
					// else click
					return it->second.OnClick(*pPC);
				}
			}
			return false;
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST CLICK_EVENT no such pc id : %d", pc);
			return false;
		}
		//cerr << "QUEST CLICk" << endl;
	}

	void CQuestManager::Unmount(unsigned int pc)
	{
		PC * pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnUnmount(*pPC);
		}
		else
			sys_err("QUEST no such pc id : %d", pc);
	}
	//독일 선물 기능 테스트
	void CQuestManager::ItemInformer(unsigned int pc,unsigned int vnum)
	{
		
		PC* pPC;
		pPC = GetPC(pc);
		
		m_mapNPC[QUEST_NO_NPC].OnItemInformer(*pPC,vnum);
	}
	///////////////////////////////////////////////////////////////////////////////////////////
	// END OF 퀘스트 이벤트 처리
	///////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////
	void CQuestManager::LoadStartQuest(const string& quest_name, unsigned int idx)
	{
		for (itertype(g_setQuestObjectDir) it = g_setQuestObjectDir.begin(); it != g_setQuestObjectDir.end(); ++it)
		{
			const string& stQuestObjectDir = *it;
			string full_name = stQuestObjectDir + "/begin_condition/" + quest_name;
			ifstream inf(full_name.c_str());

			if (inf.is_open())
			{
				sys_log(0, "QUEST loading begin condition for %s", quest_name.c_str());

				istreambuf_iterator<char> ib(inf), ie;
				copy(ib, ie, back_inserter(m_hmQuestStartScript[idx]));
			}
		}
	}

	bool CQuestManager::CanStartQuest(unsigned int quest_index, const PC& pc)
	{
		return CanStartQuest(quest_index);
	}

	bool CQuestManager::CanStartQuest(unsigned int quest_index)
	{
		THashMapQuestStartScript::iterator it;

		if ((it = m_hmQuestStartScript.find(quest_index)) == m_hmQuestStartScript.end())
			return true;
		else
		{
			int x = lua_gettop(L);
			lua_dobuffer(L, &(it->second[0]), it->second.size(), "StartScript");
			int bStart = lua_toboolean(L, -1);
			lua_settop(L, x);
			return bStart != 0;
		}
	}

	bool CQuestManager::CanEndQuestAtState(const string& quest_name, const string& state_name)
	{
		return false;
	}

	void CQuestManager::DisconnectPC(LPCHARACTER ch)
	{
		m_mapPC.erase(ch->GetPlayerID());
	}

	PC * CQuestManager::GetPCForce(unsigned int pc)
	{
		PCMap::iterator it;

		if ((it = m_mapPC.find(pc)) == m_mapPC.end())
		{
			PC * pPC = &m_mapPC[pc];
			pPC->SetID(pc);
			return pPC;
		}

		return &it->second;
	}

	PC * CQuestManager::GetPC(unsigned int pc)
	{
		PCMap::iterator it;

		LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindByPID(pc);

		if (!pkChr)
			return NULL;

		m_pCurrentPC = GetPCForce(pc);
		m_pCurrentCharacter = pkChr;
		m_pSelectedDungeon = NULL;
		return (m_pCurrentPC);
	}

	void CQuestManager::ClearScript()
	{
		m_strScript.clear();
		m_iCurrentSkin = QUEST_SKIN_NORMAL;
	}

	void CQuestManager::AddScript(const string& str)
	{
		m_strScript+=str;
	}

	void CQuestManager::SendScript()
	{
		if (m_bNoSend)
		{
			m_bNoSend = false;
			ClearScript();
			return;
		}

		if (m_strScript=="[DONE]" || m_strScript == "[NEXT]")
		{
			if (m_pCurrentPC && !m_pCurrentPC->GetAndResetDoneFlag() && m_strScript=="[DONE]" && m_iCurrentSkin == QUEST_SKIN_NORMAL && !IsError())
			{
				ClearScript();
				return;
			}
			m_iCurrentSkin = QUEST_SKIN_NOWINDOW;
		}

		//sys_log(0, "Send Quest Script to %s", GetCurrentCharacterPtr()->GetName());
		//send -_-!
		struct ::packet_script packet_script;

		packet_script.header = HEADER_GC_SCRIPT;
		packet_script.skin = m_iCurrentSkin;
		packet_script.src_size = m_strScript.size();
		packet_script.size = packet_script.src_size + sizeof(struct packet_script);

		TEMP_BUFFER buf;
		buf.write(&packet_script, sizeof(struct packet_script));
		buf.write(&m_strScript[0], m_strScript.size());

		GetCurrentCharacterPtr()->GetDesc()->Packet(buf.read_peek(), buf.size());

		if (test_server)
			sys_log(0, "m_strScript %s size %d", m_strScript.c_str(), buf.size());

		ClearScript();
	}

	const char* CQuestManager::GetQuestStateName(const string& quest_name, const int state_index)
	{
		int x = lua_gettop(L);
		lua_getglobal(L, quest_name.c_str());
		if (lua_isnil(L,-1))
		{
			sys_err("QUEST wrong quest state file %s.%d", quest_name.c_str(), state_index);
			lua_settop(L,x);
			return "";
		}
		lua_pushnumber(L, state_index);
		lua_gettable(L, -2);

		const char* str = lua_tostring(L, -1);
		lua_settop(L, x);
		return str;
	}

	int CQuestManager::GetQuestStateIndex(const string& quest_name, const string& state_name)
	{
		int x = lua_gettop(L);
		lua_getglobal(L, quest_name.c_str());
		if (lua_isnil(L,-1))
		{
			sys_err("QUEST wrong quest state file %s.%s",quest_name.c_str(),state_name.c_str()  );
			lua_settop(L,x);
			return 0;
		}
		lua_pushstring(L, state_name.c_str());
		lua_gettable(L, -2);

		int v = (int)rint(lua_tonumber(L,-1));
		lua_settop(L, x);
		if ( test_server )
			sys_log( 0,"[QUESTMANAGER] GetQuestStateIndex x(%d) v(%d) %s %s", v,x, quest_name.c_str(), state_name.c_str() );
		return v;
	}

	void CQuestManager::SetSkinStyle(int iStyle)
	{
		if (iStyle<0 || iStyle >= QUEST_SKIN_COUNT)
		{
			m_iCurrentSkin = QUEST_SKIN_NORMAL;
		}
		else
			m_iCurrentSkin = iStyle;
	}

	unsigned int CQuestManager::LoadTimerScript(const string& name)
	{
		map<string, unsigned int>::iterator it;
		if ((it = m_mapTimerID.find(name)) != m_mapTimerID.end())
		{
			return it->second;
		}
		else
		{
			unsigned int new_id = UINT_MAX - m_mapTimerID.size();

			m_mapNPC[new_id].Set(new_id, name);
			m_mapTimerID.insert(make_pair(name, new_id));

			return new_id;
		}
	}

	unsigned int CQuestManager::GetCurrentNPCRace()
	{
		return GetCurrentNPCCharacterPtr() ? GetCurrentNPCCharacterPtr()->GetRaceNum() : 0;
	}

	LPITEM CQuestManager::GetCurrentItem()
	{
		return GetCurrentCharacterPtr() ? GetCurrentCharacterPtr()->GetQuestItemPtr() : NULL; 
	}

	void CQuestManager::ClearCurrentItem()
	{
		if (GetCurrentCharacterPtr())
			GetCurrentCharacterPtr()->ClearQuestItemPtr();
	}

	void CQuestManager::SetCurrentItem(LPITEM item)
	{
		if (GetCurrentCharacterPtr())
			GetCurrentCharacterPtr()->SetQuestItemPtr(item);
	}

	LPCHARACTER CQuestManager::GetCurrentNPCCharacterPtr()
	{ 
		return GetCurrentCharacterPtr() ? GetCurrentCharacterPtr()->GetQuestNPC() : NULL; 
	}

	const string & CQuestManager::GetCurrentQuestName()
	{
		return GetCurrentPC()->GetCurrentQuestName();
	}

	LPDUNGEON CQuestManager::GetCurrentDungeon()
	{
		LPCHARACTER ch = GetCurrentCharacterPtr();

		if (!ch)
		{
			if (m_pSelectedDungeon)
				return m_pSelectedDungeon;
			return NULL;
		}

		return ch->GetDungeonForce();
	}

	void CQuestManager::RegisterQuest(const string & stQuestName, unsigned int idx)
	{
		assert(idx > 0);

		itertype(m_hmQuestName) it;

		if ((it = m_hmQuestName.find(stQuestName)) != m_hmQuestName.end())
			return;

		m_hmQuestName.insert(make_pair(stQuestName, idx));
		LoadStartQuest(stQuestName, idx);
		m_mapQuestNameByIndex.insert(make_pair(idx, stQuestName));

		sys_log(0, "QUEST: Register %4u %s", idx, stQuestName.c_str());
	}

	unsigned int CQuestManager::GetQuestIndexByName(const string& name)
	{
		THashMapQuestName::iterator it = m_hmQuestName.find(name);

		if (it == m_hmQuestName.end())
			return 0; // RESERVED

		return it->second;
	}

	const string & CQuestManager::GetQuestNameByIndex(unsigned int idx)
	{
		itertype(m_mapQuestNameByIndex) it;

		if ((it = m_mapQuestNameByIndex.find(idx)) == m_mapQuestNameByIndex.end())
		{
			sys_err("cannot find quest name by index %u", idx);
			assert(!"cannot find quest name by index");

			static std::string st = "";
			return st;
		}

		return it->second;
	}

	void CQuestManager::SendEventFlagList(LPCHARACTER ch)
	{
		itertype(m_mapEventFlag) it;
		for (it = m_mapEventFlag.begin(); it != m_mapEventFlag.end(); ++it)
		{
			const string& flagname = it->first;
			int value = it->second;

			if (!test_server && value == 1 && flagname == "valentine_drop")
				ch->ChatPacket(CHAT_TYPE_INFO, "%s %d prob 800", flagname.c_str(), value);
			else if (!test_server && value == 1 && flagname == "newyear_wonso")
				ch->ChatPacket(CHAT_TYPE_INFO, "%s %d prob 500", flagname.c_str(), value);
			else if (!test_server && value == 1 && flagname == "newyear_fire")
				ch->ChatPacket(CHAT_TYPE_INFO, "%s %d prob 1000", flagname.c_str(), value);
			else
				ch->ChatPacket(CHAT_TYPE_INFO, "%s %d", flagname.c_str(), value);
		}
	}

	void CQuestManager::RequestSetEventFlag(const string& name, int value)
	{
		TPacketSetEventFlag p;
		strlcpy(p.szFlagName, name.c_str(), sizeof(p.szFlagName));
		p.lValue = value;
		db_clientdesc->DBPacket(HEADER_GD_SET_EVENT_FLAG, 0, &p, sizeof(TPacketSetEventFlag));
	}

	void CQuestManager::SetEventFlag(const string& name, int value)
	{
		static const char*	DROPEVENT_CHARTONE_NAME		= "drop_char_stone";
		static const int	DROPEVENT_CHARTONE_NAME_LEN = strlen(DROPEVENT_CHARTONE_NAME);

		int prev_value = m_mapEventFlag[name];

		sys_log(0, "QUEST eventflag %s %d prev_value %d", name.c_str(), value, m_mapEventFlag[name]);
		m_mapEventFlag[name] = value;

		if (name == "mob_item")
		{
			CHARACTER_MANAGER::instance().SetMobItemRate(value);
		}
		else if (name == "mob_dam")
		{
			CHARACTER_MANAGER::instance().SetMobDamageRate(value);
		}
		else if (name == "mob_gold")
		{
			CHARACTER_MANAGER::instance().SetMobGoldAmountRate(value);
		}
		else if (name == "mob_gold_pct")
		{
			CHARACTER_MANAGER::instance().SetMobGoldDropRate(value);
		}
		else if (name == "user_dam")
		{
			CHARACTER_MANAGER::instance().SetUserDamageRate(value);
		}
		else if (name == "user_dam_buyer")
		{
			CHARACTER_MANAGER::instance().SetUserDamageRatePremium(value);
		}
		else if (name == "mob_exp")
		{
			CHARACTER_MANAGER::instance().SetMobExpRate(value);
		}
		else if (name == "mob_item_buyer")
		{
			CHARACTER_MANAGER::instance().SetMobItemRatePremium(value);
		}
		else if (name == "mob_exp_buyer")
		{
			CHARACTER_MANAGER::instance().SetMobExpRatePremium(value);
		}
		else if (name == "mob_gold_buyer")
		{
			CHARACTER_MANAGER::instance().SetMobGoldAmountRatePremium(value);
		}
		else if (name == "mob_gold_pct_buyer")
		{
			CHARACTER_MANAGER::instance().SetMobGoldDropRatePremium(value);
		}
		else if (name == "crcdisconnect")
		{
			DESC_MANAGER::instance().SetDisconnectInvalidCRCMode(value != 0);
		}
		else if (!name.compare(0,5,"xmas_"))
		{
			xmas::ProcessEventFlag(name, prev_value, value);
		}
		else if (name == "newyear_boom")
		{
			const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();

			for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
			{
				LPCHARACTER ch = (*it)->GetCharacter();

				if (!ch)
					continue;

				ch->ChatPacket(CHAT_TYPE_COMMAND, "newyear_boom %d", value);
			}
		}
		else if ( name == "eclipse" )
		{
			std::string mode("");

			if ( value == 1 )
			{
				mode = "dark";
			}
			else
			{
				mode = "light";
			}
			
			const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();

			for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
			{
				LPCHARACTER ch = (*it)->GetCharacter();
				if (!ch)
					continue;

				ch->ChatPacket(CHAT_TYPE_COMMAND, "DayMode %s", mode.c_str());
			}
		}
		else if (name == "day")
		{
			const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();

			for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
			{
				LPCHARACTER ch = (*it)->GetCharacter();
				if (!ch)
					continue;
				if (value)
				{
					// 밤
					ch->ChatPacket(CHAT_TYPE_COMMAND, "DayMode dark");
				}
				else
				{
					// 낮
					ch->ChatPacket(CHAT_TYPE_COMMAND, "DayMode light");
				}
			}

			if (value && !prev_value)
			{
				// 없으면 만들어준다
				struct SNPCSellFireworkPosition
				{
					long lMapIndex;
					int x;
					int y;
				} positions[] = {
					{	1,	615,	618 },
					{	3,	500,	625 },
					{	21,	598,	665 },
					{	23,	476,	360 },
					{	41,	318,	629 },
					{	43,	478,	375 },
					{	0,	0,	0   },
				};

				SNPCSellFireworkPosition* p = positions;
				while (p->lMapIndex)
				{
					if (map_allow_find(p->lMapIndex))
					{
						PIXEL_POSITION posBase;
						if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(p->lMapIndex, posBase))
						{
							sys_err("cannot get map base position %d", p->lMapIndex);
							++p;
							continue;
						}

						CHARACTER_MANAGER::instance().SpawnMob(xmas::MOB_XMAS_FIRWORK_SELLER_VNUM, p->lMapIndex, posBase.x + p->x * 100, posBase.y + p->y * 100, 0, false, -1);
					}
					p++;
				}
			}
			else if (!value && prev_value)
			{
				// 있으면 지워준다
				CharacterVectorInteractor i;

				if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(xmas::MOB_XMAS_FIRWORK_SELLER_VNUM, i))
				{
					CharacterVectorInteractor::iterator it = i.begin();

					while (it != i.end()) {
						M2_DESTROY_CHARACTER(*it++);
					}
				}
			}
		}
		else if (name == "pre_event_hc" && true == LC_IsEurope())
		{
			const DWORD EventNPC = 20090;

			struct SEventNPCPosition
			{
				long lMapIndex;
				int x;
				int y;
			} positions[] = {
				{ 3, 588, 617 },
				{ 23, 397, 250 },
				{ 43, 567, 426 },
				{ 0, 0, 0 },
			};

			if (value && !prev_value)
			{
				SEventNPCPosition* pPosition = positions;

				while (pPosition->lMapIndex)
				{
					if (map_allow_find(pPosition->lMapIndex))
					{
						PIXEL_POSITION pos;

						if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(pPosition->lMapIndex, pos))
						{
							sys_err("cannot get map base position %d", pPosition->lMapIndex);
							++pPosition;
							continue;
						}

						CHARACTER_MANAGER::instance().SpawnMob(EventNPC, pPosition->lMapIndex, pos.x+pPosition->x*100, pos.y+pPosition->y*100, 0, false, -1);
					}
					pPosition++;
				}
			}
			else if (!value && prev_value)
			{
				CharacterVectorInteractor i;

				if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(EventNPC, i))
				{
					CharacterVectorInteractor::iterator it = i.begin();

					while (it != i.end())
					{
						LPCHARACTER ch = *it++;

						switch (ch->GetMapIndex())
						{
							case 3:
							case 23:
							case 43:
								M2_DESTROY_CHARACTER(ch);
								break;
						}
					}
				}
			}
		}
		else if (name.compare(0, DROPEVENT_CHARTONE_NAME_LEN, DROPEVENT_CHARTONE_NAME)== 0)
		{
			DropEvent_CharStone_SetValue(name, value);
		}
		else if (name.compare(0, strlen("refine_box"), "refine_box")== 0)
		{
			DropEvent_RefineBox_SetValue(name, value);
		}
		else if (name == "gold_drop_limit_time")
		{
			g_GoldDropTimeLimitValue = value * 1000;
		}
		else if (name == "new_xmas_event")
		{
			// 20126 new산타.
			static DWORD new_santa = 20126;
			if (value != 0)
			{
				CharacterVectorInteractor i;
				bool map1_santa_exist = false;
				bool map21_santa_exist = false;
				bool map41_santa_exist = false;
				
				if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(new_santa, i))
				{
					CharacterVectorInteractor::iterator it = i.begin();

					while (it != i.end())
					{
						LPCHARACTER tch = *(it++);

						if (tch->GetMapIndex() == 1)
						{
							map1_santa_exist = true;
						}
						else if (tch->GetMapIndex() == 21)
						{
							map21_santa_exist = true;
						}
						else if (tch->GetMapIndex() == 41)
						{
							map41_santa_exist = true;
						}
					}
				}

				if (map_allow_find(1) && !map1_santa_exist)
				{
					LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(1);
					CHARACTER_MANAGER::instance().SpawnMob(new_santa, 1, pkSectreeMap->m_setting.iBaseX + 60800, pkSectreeMap->m_setting.iBaseY + 61700, 0, false, 90, true);
				}
				if (map_allow_find(21) && !map21_santa_exist)
				{
					LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(21);
					CHARACTER_MANAGER::instance().SpawnMob(new_santa, 21, pkSectreeMap->m_setting.iBaseX + 59600, pkSectreeMap->m_setting.iBaseY + 61000, 0, false, 110, true);
				}
				if (map_allow_find(41) && !map41_santa_exist)
				{
					LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(41);
					CHARACTER_MANAGER::instance().SpawnMob(new_santa, 41, pkSectreeMap->m_setting.iBaseX + 35700, pkSectreeMap->m_setting.iBaseY + 74300, 0, false, 140, true);
				}
			}
			else
			{
				CharacterVectorInteractor i;
				CHARACTER_MANAGER::instance().GetCharactersByRaceNum(new_santa, i);
				
				for (CharacterVectorInteractor::iterator it = i.begin(); it != i.end(); it++)
				{
					M2_DESTROY_CHARACTER(*it);
				}
			}
		}
	}

	int	CQuestManager::GetEventFlag(const string& name)
	{
		map<string,int>::iterator it = m_mapEventFlag.find(name);

		if (it == m_mapEventFlag.end())
			return 0;

		return it->second;
	}

	void CQuestManager::BroadcastEventFlagOnLogin(LPCHARACTER ch)
	{
		int iEventFlagValue;

		if ((iEventFlagValue = quest::CQuestManager::instance().GetEventFlag("xmas_snow")))
		{
			ch->ChatPacket(CHAT_TYPE_COMMAND, "xmas_snow %d", iEventFlagValue);
		}

		if ((iEventFlagValue = quest::CQuestManager::instance().GetEventFlag("xmas_boom")))
		{
			ch->ChatPacket(CHAT_TYPE_COMMAND, "xmas_boom %d", iEventFlagValue);
		}

		if ((iEventFlagValue = quest::CQuestManager::instance().GetEventFlag("xmas_tree")))
		{
			ch->ChatPacket(CHAT_TYPE_COMMAND, "xmas_tree %d", iEventFlagValue);
		}

		if ((iEventFlagValue = quest::CQuestManager::instance().GetEventFlag("day")))
		{
			ch->ChatPacket(CHAT_TYPE_COMMAND, "DayMode dark");
		}

		if ((iEventFlagValue = quest::CQuestManager::instance().GetEventFlag("newyear_boom")))
		{
			ch->ChatPacket(CHAT_TYPE_COMMAND, "newyear_boom %d", iEventFlagValue);
		}

		if ( (iEventFlagValue = quest::CQuestManager::instance().GetEventFlag("eclipse")) )
		{
			std::string mode;

			if ( iEventFlagValue == 1 ) mode = "dark";
			else mode = "light";

			ch->ChatPacket(CHAT_TYPE_COMMAND, "DayMode %s", mode.c_str());
		}
	}

	void CQuestManager::Reload()
	{
		lua_close(L);
		m_mapNPC.clear();
		m_mapNPCNameID.clear();
		m_hmQuestName.clear();
		m_mapTimerID.clear();
		m_hmQuestStartScript.clear();
		m_mapEventName.clear();
		L = NULL;
		Initialize();

		for (itertype(m_registeredNPCVnum) it = m_registeredNPCVnum.begin(); it != m_registeredNPCVnum.end(); ++it)
		{
			char buf[256];
			DWORD dwVnum = *it;
			snprintf(buf, sizeof(buf), "%u", dwVnum);
			m_mapNPC[dwVnum].Set(dwVnum, buf);
		}
	}

	bool CQuestManager::ExecuteQuestScript(PC& pc, DWORD quest_index, const int state, const char* code, const int code_size, vector<AArgScript*>* pChatScripts, bool bUseCache)
	{
		return ExecuteQuestScript(pc, CQuestManager::instance().GetQuestNameByIndex(quest_index), state, code, code_size, pChatScripts, bUseCache);
	}

	bool CQuestManager::ExecuteQuestScript(PC& pc, const string& quest_name, const int state, const char* code, const int code_size, vector<AArgScript*>* pChatScripts, bool bUseCache)
	{
		// 실행공간을 생성
		QuestState qs = CQuestManager::instance().OpenState(quest_name, state);
		if (pChatScripts)
			qs.chat_scripts.swap(*pChatScripts);

		// 코드를 읽어들임
		if (bUseCache)
		{
			lua_getglobal(qs.co, "__codecache");
			// stack : __codecache
			lua_pushnumber(qs.co, (long)code);
			// stack : __codecache (codeptr)
			lua_rawget(qs.co, -2);
			// stack : __codecache (compiled-code)
			if (lua_isnil(qs.co, -1))
			{
				// cache miss

				// load code to lua,
				// save it to cache
				// and only function remain in stack
				lua_pop(qs.co, 1);
				// stack : __codecache
				luaL_loadbuffer(qs.co, code, code_size, quest_name.c_str());
				// stack : __codecache (compiled-code)
				lua_pushnumber(qs.co, (long)code);
				// stack : __codecache (compiled-code) (codeptr)
				lua_pushvalue(qs.co, -2);
				// stack : __codecache (compiled-code) (codeptr) (compiled_code)
				lua_rawset(qs.co, -4);
				// stack : __codecache (compiled-code)
				lua_remove(qs.co, -2);
				// stack : (compiled-code)
			}
			else
			{
				// cache hit
				lua_remove(qs.co, -2);
				// stack : (compiled-code)
			}
		}
		else
			luaL_loadbuffer(qs.co, code, code_size, quest_name.c_str());

		// 플레이어와 연결
		pc.SetQuest(quest_name, qs);

		// 실행
		QuestState& rqs = *pc.GetRunningQuestState();
		if (!CQuestManager::instance().RunState(rqs))
		{
			CQuestManager::instance().CloseState(rqs);
			pc.EndRunning();
			return false;
		}
		return true;
	}

	void CQuestManager::RegisterNPCVnum(DWORD dwVnum)
	{
		if (m_registeredNPCVnum.find(dwVnum) != m_registeredNPCVnum.end())
			return;

		m_registeredNPCVnum.insert(dwVnum);

		char buf[256];
		DIR* dir;

		for (itertype(g_setQuestObjectDir) it = g_setQuestObjectDir.begin(); it != g_setQuestObjectDir.end(); ++it)
		{
			const string& stQuestObjectDir = *it;
			snprintf(buf, sizeof(buf), "%s/%u", stQuestObjectDir.c_str(), dwVnum);
			sys_log(0, "%s", buf);

			if ((dir = opendir(buf)))
			{
				closedir(dir);
				snprintf(buf, sizeof(buf), "%u", dwVnum);
				sys_log(0, "%s", buf);

				m_mapNPC[dwVnum].Set(dwVnum, buf);
			}
		}
	}

	void CQuestManager::WriteRunningStateToSyserr()
	{
		const char * state_name = GetQuestStateName(GetCurrentQuestName(), GetCurrentState()->st);

		string event_index_name = "";
		for (itertype(m_mapEventName) it = m_mapEventName.begin(); it != m_mapEventName.end(); ++it)
		{
			if (it->second == m_iRunningEventIndex)
			{
				event_index_name = it->first;
				break;
			}
		}

		sys_err("LUA_ERROR: quest %s.%s %s", GetCurrentQuestName().c_str(), state_name, event_index_name.c_str() );
		if (GetCurrentCharacterPtr() && test_server)
			GetCurrentCharacterPtr()->ChatPacket(CHAT_TYPE_PARTY, "LUA_ERROR: quest %s.%s %s", GetCurrentQuestName().c_str(), state_name, event_index_name.c_str() );
	}

#ifndef __WIN32__
	void CQuestManager::QuestError(const char* func, int line, const char* fmt, ...)
	{
		char szMsg[4096];
		va_list args;

		va_start(args, fmt);
		vsnprintf(szMsg, sizeof(szMsg), fmt, args);
		va_end(args);

		_sys_err(func, line, "%s", szMsg);
		if (test_server)
		{
			LPCHARACTER ch = GetCurrentCharacterPtr();
			if (ch)
			{
				ch->ChatPacket(CHAT_TYPE_PARTY, "error occurred on [%s:%d]", func,line);
				ch->ChatPacket(CHAT_TYPE_PARTY, "%s", szMsg);
			}
		}
	}
#else
	void CQuestManager::QuestError(const char* func, int line, const char* fmt, ...)
	{
		char szMsg[4096];
		va_list args;

		va_start(args, fmt);
		vsnprintf(szMsg, sizeof(szMsg), fmt, args);
		va_end(args);

		_sys_err(func, line, "%s", szMsg);
		if (test_server)
		{
			LPCHARACTER ch = GetCurrentCharacterPtr();
			if (ch)
			{
				ch->ChatPacket(CHAT_TYPE_PARTY, "error occurred on [%s:%d]", func,line);
				ch->ChatPacket(CHAT_TYPE_PARTY, "%s", szMsg);
			}
		}
	}
#endif

	void CQuestManager::AddServerTimer(const std::string& name, DWORD arg, LPEVENT event)
	{
		sys_log(0, "XXX AddServerTimer %s %d %p", name.c_str(), arg, get_pointer(event));
		if (m_mapServerTimer.find(make_pair(name, arg)) != m_mapServerTimer.end())
		{
			sys_err("already registered server timer name:%s arg:%u", name.c_str(), arg);
			return;
		}
		m_mapServerTimer.insert(make_pair(make_pair(name, arg), event));
	}

	void CQuestManager::ClearServerTimerNotCancel(const std::string& name, DWORD arg)
	{
		m_mapServerTimer.erase(make_pair(name, arg));
	}

	void CQuestManager::ClearServerTimer(const std::string& name, DWORD arg)
	{
		itertype(m_mapServerTimer) it = m_mapServerTimer.find(make_pair(name, arg));
		if (it != m_mapServerTimer.end())
		{
			LPEVENT event = it->second;
			event_cancel(&event);
			m_mapServerTimer.erase(it);
		}
	}

	void CQuestManager::CancelServerTimers(DWORD arg)
	{
		itertype(m_mapServerTimer) it = m_mapServerTimer.begin();
		for ( ; it != m_mapServerTimer.end(); ++it) {
			if (it->first.second == arg) {
				LPEVENT event = it->second;
				event_cancel(&event);
				m_mapServerTimer.erase(it);
			}
		}
	}

	void CQuestManager::SetServerTimerArg(DWORD dwArg)
	{
		m_dwServerTimerArg = dwArg;
	}

	DWORD CQuestManager::GetServerTimerArg()
	{
		return m_dwServerTimerArg;
	}

	void CQuestManager::SelectDungeon(LPDUNGEON pDungeon)
	{
		m_pSelectedDungeon = pDungeon;
	}
	
	bool CQuestManager::PickupItem(unsigned int pc, LPITEM item)
	{
		if (test_server)
			sys_log( 0, "questmanager::PickupItem Start : itemVnum : %d PC : %d", item->GetOriginalVnum(), pc);
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pc);
				if (ch)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("퀘스트를 로드하는 중입니다. 잠시만 기다려 주십시오."));
				}
				return false;
			}
			// call script
			SetCurrentItem(item);

			return m_mapNPC[item->GetVnum()].OnPickupItem(*pPC);
		}
		else
		{
			sys_err("QUEST PICK_ITEM_EVENT no such pc id : %d", pc);
			return false;
		}
	}
	void CQuestManager::BeginOtherPCBlock(DWORD pid)
	{
		LPCHARACTER ch = GetCurrentCharacterPtr();
		if (NULL == ch)
		{
			sys_err("NULL?");
			return;
		}
		/*
		# 1. current pid = pid0 <- It will be m_pOtherPCBlockRootPC.
		begin_other_pc_block(pid1)
			# 2. current pid = pid1
			begin_other_pc_block(pid2)
				# 3. current_pid = pid2
			end_other_pc_block()
		end_other_pc_block()
		*/
		// when begin_other_pc_block(pid1)
		if (m_vecPCStack.empty())
		{
			m_pOtherPCBlockRootPC = GetCurrentPC();
		}
		m_vecPCStack.push_back(GetCurrentCharacterPtr()->GetPlayerID());
		GetPC(pid);
	}

	void CQuestManager::EndOtherPCBlock()
	{
		if (m_vecPCStack.size() == 0)
		{
			sys_err("m_vecPCStack is alread empty. CurrentQuest{Name(%s), State(%s)}", GetCurrentQuestName().c_str(), GetCurrentState()->_title.c_str());
			return;
		}
		DWORD pc = m_vecPCStack.back();
		m_vecPCStack.pop_back();
		GetPC(pc);

		if (m_vecPCStack.empty())
		{
			m_pOtherPCBlockRootPC = NULL;
		}
	}

	bool CQuestManager::IsInOtherPCBlock()
	{
		return !m_vecPCStack.empty();
	}

	PC*	CQuestManager::GetOtherPCBlockRootPC()
	{
		return m_pOtherPCBlockRootPC;
	}
}

