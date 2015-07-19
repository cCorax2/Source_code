#include "stdafx.h"
#include "constants.h"
#include "questmanager.h"
#include "packet.h"
#include "buffer_manager.h"
#include "char.h"
#include "desc_client.h"
#include "questevent.h"

extern int test_server;

namespace quest
{
	PC::PC() :
		m_bIsGivenReward(false),
		m_bShouldSendDone(false),
		m_dwID(0),
		m_RunningQuestState(0),
		m_iSendToClient(0),
		m_bLoaded(false),
		m_iLastState(0),
		m_dwWaitConfirmFromPID(0),
		m_bConfirmWait(false)
	{
	}

	PC::~PC()
	{
		Destroy();
	}

	void PC::Destroy()
	{
		ClearTimer();
	}

	void PC::SetID(DWORD dwID)
	{
		m_dwID = dwID;
		m_bShouldSendDone = false;
	}

	const string & PC::GetCurrentQuestName() const
	{
		return m_stCurQuest;
	}

	int	PC::GetCurrentQuestIndex()
	{
		return CQuestManager::instance().GetQuestIndexByName(GetCurrentQuestName());
	}

	void PC::SetFlag(const string& name, int value, bool bSkipSave)
	{
		if ( test_server )
			sys_log(0, "QUEST Setting flag %s %d", name.c_str(),value);
		else
			sys_log(1, "QUEST Setting flag %s %d", name.c_str(),value);

		if (value == 0)
		{
			DeleteFlag(name);
			return;
		}

		TFlagMap::iterator it = m_FlagMap.find(name);

		if (it == m_FlagMap.end())
			m_FlagMap.insert(make_pair(name, value));
		else if (it->second != value)
			it->second = value;
		else
			bSkipSave = true;

		if (!bSkipSave)
			SaveFlag(name, value);
	}

	bool PC::DeleteFlag(const string & name)
	{
		TFlagMap::iterator it = m_FlagMap.find(name);

		if (it != m_FlagMap.end())
		{
			m_FlagMap.erase(it);
			SaveFlag(name, 0);
			return true;
		}

		return false;
	}

	int PC::GetFlag(const string & name)
	{
		TFlagMap::iterator it = m_FlagMap.find(name);

		if (it != m_FlagMap.end())
		{
			sys_log(1, "QUEST getting flag %s %d", name.c_str(),it->second);
			return it->second;
		}
		return 0;
	}

	void PC::SaveFlag(const string & name, int value)
	{
		TFlagMap::iterator it = m_FlagSaveMap.find(name);

		if (it == m_FlagSaveMap.end())
			m_FlagSaveMap.insert(make_pair(name, value));
		else if (it->second != value)
			it->second = value;
	}

	// only from lua call
	void PC::SetCurrentQuestStateName(const string& state_name) 
	{
		SetFlag(m_stCurQuest + ".__status", CQuestManager::Instance().GetQuestStateIndex(m_stCurQuest,state_name));
	}

	void PC::SetQuestState(const string& quest_name, const string& state_name)
	{
		SetQuestState(quest_name, CQuestManager::Instance().GetQuestStateIndex(quest_name, state_name));
	}

	void PC::SetQuestState(const string& quest_name, int new_state_index)
	{
		int iNowState = GetFlag(quest_name + ".__status");

		if (iNowState != new_state_index)
			AddQuestStateChange(quest_name, iNowState, new_state_index);
	}

	void PC::AddQuestStateChange(const string& quest_name, int prev_state, int next_state)
	{
		DWORD dwQuestIndex = CQuestManager::instance().GetQuestIndexByName(quest_name);
		sys_log(0, "QUEST reserve Quest State Change quest %s[%u] from %d to %d", quest_name.c_str(), dwQuestIndex, prev_state, next_state);
		m_QuestStateChange.push_back(TQuestStateChangeInfo(dwQuestIndex, prev_state, next_state));
	}

	void PC::SetQuest(const string& quest_name, QuestState& qs)
	{
		//sys_log(0, "PC SetQuest %s", quest_name.c_str());
		unsigned int qi = CQuestManager::instance().GetQuestIndexByName(quest_name);
		QuestInfo::iterator it = m_QuestInfo.find(qi);

		if (it == m_QuestInfo.end())
			m_QuestInfo.insert(make_pair(qi, qs));
		else
			it->second = qs;

		m_stCurQuest = quest_name;
		m_RunningQuestState = &m_QuestInfo[qi];
		m_iSendToClient = 0;

		m_iLastState = qs.st;
		SetFlag(quest_name + ".__status", qs.st);

		//m_RunningQuestState->iIndex = GetCurrentQuestBeginFlag();
		m_RunningQuestState->iIndex = qi;
		m_bShouldSendDone = false;
		//if (GetCurrentQuestBeginFlag())
		//{
		//m_bSendToClient = true;
		//}
	}

	void PC::AddTimer(const string & name, LPEVENT pEvent)
	{
		RemoveTimer(name);
		m_TimerMap.insert(make_pair(name, pEvent));
		sys_log(0, "QUEST add timer %p %d", get_pointer(pEvent), m_TimerMap.size());
	}

	void PC::RemoveTimerNotCancel(const string & name)
	{
		TTimerMap::iterator it = m_TimerMap.find(name);

		if (it != m_TimerMap.end())
		{
			sys_log(0, "QUEST remove with no cancel %p", get_pointer(it->second));
			m_TimerMap.erase(it);
		}

		sys_log(1, "QUEST timer map size %d by RemoveTimerNotCancel", m_TimerMap.size());
	}

	void PC::RemoveTimer(const string & name)
	{
		TTimerMap::iterator it = m_TimerMap.find(name);

		if (it != m_TimerMap.end())
		{
			sys_log(0, "QUEST remove timer %p", get_pointer(it->second));
			CancelTimerEvent(&it->second);
			m_TimerMap.erase(it);
		}

		sys_log(1, "QUEST timer map size %d by RemoveTimer", m_TimerMap.size());
	}

	void PC::ClearTimer()
	{
		sys_log(0, "QUEST clear timer %d", m_TimerMap.size());
		TTimerMap::iterator it = m_TimerMap.begin();

		while (it != m_TimerMap.end())
		{
			CancelTimerEvent(&it->second);
			++it;
		}

		m_TimerMap.clear();
	}

	void PC::SetCurrentQuestStartFlag()
	{
		if (!GetCurrentQuestBeginFlag())
		{
			SetCurrentQuestBeginFlag();
		}
	}

	void PC::SetCurrentQuestDoneFlag()
	{
		if (GetCurrentQuestBeginFlag())
		{
			ClearCurrentQuestBeginFlag();
		}
	}

	void PC::SendQuestInfoPakcet()
	{
		assert(m_iSendToClient);
		assert(m_RunningQuestState);

		packet_quest_info qi;

		qi.header = HEADER_GC_QUEST_INFO;
		qi.size = sizeof(struct packet_quest_info);
		qi.index = m_RunningQuestState->iIndex;
		qi.flag = m_iSendToClient;

		TEMP_BUFFER buf;
		buf.write(&qi, sizeof(qi));

		if (m_iSendToClient & QUEST_SEND_ISBEGIN)
		{
			BYTE temp = m_RunningQuestState->bStart?1:0;
			buf.write(&temp,1);		
			qi.size+=1;

			sys_log(1, "QUEST BeginFlag %d", (int)temp);
		}
		if (m_iSendToClient & QUEST_SEND_TITLE)
		{
			m_RunningQuestState->_title.reserve(30+1);
			buf.write(m_RunningQuestState->_title.c_str(), 30+1);
			qi.size+=30+1;

			sys_log(1, "QUEST Title %s", m_RunningQuestState->_title.c_str());
		}
		if (m_iSendToClient & QUEST_SEND_CLOCK_NAME)
		{
			m_RunningQuestState->_clock_name.reserve(16+1);
			buf.write(m_RunningQuestState->_clock_name.c_str(), 16+1);
			qi.size+=16+1;

			sys_log(1, "QUEST Clock Name %s", m_RunningQuestState->_clock_name.c_str());
		}
		if (m_iSendToClient & QUEST_SEND_CLOCK_VALUE)
		{
			buf.write(&m_RunningQuestState->_clock_value, sizeof(int));
			qi.size+=4;

			sys_log(1, "QUEST Clock Value %d", m_RunningQuestState->_clock_value);
		}
		if (m_iSendToClient & QUEST_SEND_COUNTER_NAME)
		{
			m_RunningQuestState->_counter_name.reserve(16+1);
			buf.write(m_RunningQuestState->_counter_name.c_str(), 16+1);
			qi.size+=16+1;

			sys_log(1, "QUEST Counter Name %s", m_RunningQuestState->_counter_name.c_str());
		}
		if (m_iSendToClient & QUEST_SEND_COUNTER_VALUE)
		{
			buf.write(&m_RunningQuestState->_counter_value, sizeof(int));
			qi.size+=4;

			sys_log(1, "QUEST Counter Value %d", m_RunningQuestState->_counter_value);
		}
		if (m_iSendToClient & QUEST_SEND_ICON_FILE)
		{
			m_RunningQuestState->_icon_file.reserve(24+1);
			buf.write(m_RunningQuestState->_icon_file.c_str(), 24+1);
			qi.size+=24+1;

			sys_log(1, "QUEST Icon File %s", m_RunningQuestState->_icon_file.c_str());
		}

		CQuestManager::instance().GetCurrentCharacterPtr()->GetDesc()->Packet(buf.read_peek(),buf.size());

		m_iSendToClient = 0;

	}

	void PC::EndRunning()
	{
		// unlocked locked npc
		{
			LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
			LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
			// npc 있었던 경우
			if (npc && !npc->IsPC())
			{
				// 그 엔피씨가 나에게 락인 경우
				if (ch->GetPlayerID() == npc->GetQuestNPCID())
				{
					npc->SetQuestNPCID(0);
					sys_log(0, "QUEST NPC lock isn't unlocked : pid %u", ch->GetPlayerID());
					CQuestManager::instance().WriteRunningStateToSyserr();
				}
			}
		}

		// commit data
		if (HasReward())
		{
			Save();

			LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
			if (ch != NULL) {
				Reward(ch);
				ch->Save();
			}
		}
		m_bIsGivenReward = false;

		if (m_iSendToClient)
		{
			sys_log(1, "QUEST end running %d", m_iSendToClient);
			SendQuestInfoPakcet();
		}

		if (m_RunningQuestState == NULL) {
			sys_log(0, "Entered PC::EndRunning() with invalid running quest state");
			return;
		}
		QuestState * pOldState = m_RunningQuestState;
		int iNowState = m_RunningQuestState->st;

		m_RunningQuestState = 0;

		if (m_iLastState != iNowState)
		{
			LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
			DWORD dwQuestIndex = CQuestManager::instance().GetQuestIndexByName(m_stCurQuest);
			if (ch)
			{
				SetFlag(m_stCurQuest + ".__status", m_iLastState);
				CQuestManager::instance().LeaveState(ch->GetPlayerID(), dwQuestIndex, m_iLastState);
				pOldState->st = iNowState;
				SetFlag(m_stCurQuest + ".__status", iNowState);
				CQuestManager::instance().EnterState(ch->GetPlayerID(), dwQuestIndex, iNowState);
				if (GetFlag(m_stCurQuest + ".__status") == iNowState)
					CQuestManager::instance().Letter(ch->GetPlayerID(), dwQuestIndex, iNowState);
			}
		}


		DoQuestStateChange();
	}

	void PC::DoQuestStateChange()
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		std::vector<TQuestStateChangeInfo> vecQuestStateChange;
		m_QuestStateChange.swap(vecQuestStateChange);

		for (DWORD i=0; i<vecQuestStateChange.size(); ++i)
		{
			const TQuestStateChangeInfo& rInfo = vecQuestStateChange[i];
			if (rInfo.quest_idx == 0)
				continue;

			DWORD dwQuestIdx = rInfo.quest_idx;
			QuestInfoIterator it = quest_find(dwQuestIdx);
			const string stQuestName = CQuestManager::instance().GetQuestNameByIndex(dwQuestIdx);

			if (it == quest_end())
			{
				QuestState qs;
				qs.st = 0;

				m_QuestInfo.insert(make_pair(dwQuestIdx, qs));
				SetFlag(stQuestName + ".__status", 0);

				it = quest_find(dwQuestIdx);
			}

			sys_log(0, "QUEST change reserved Quest State Change quest %u from %d to %d (%d %d)", dwQuestIdx, rInfo.prev_state, rInfo.next_state, it->second.st, rInfo.prev_state );

			assert(it->second.st == rInfo.prev_state);

			CQuestManager::instance().LeaveState(ch->GetPlayerID(), dwQuestIdx, rInfo.prev_state);
			it->second.st = rInfo.next_state;
			SetFlag(stQuestName + ".__status", rInfo.next_state);

			CQuestManager::instance().EnterState(ch->GetPlayerID(), dwQuestIdx, rInfo.next_state);

			if (GetFlag(stQuestName + ".__status")==rInfo.next_state)
				CQuestManager::instance().Letter(ch->GetPlayerID(), dwQuestIdx, rInfo.next_state);
		}
	}

	void PC::CancelRunning()
	{
		// cancel data
		m_RunningQuestState = 0;
		m_iSendToClient = 0;
		m_bShouldSendDone = false;
	}

	void PC::SetSendFlag(int idx)
	{
		m_iSendToClient |= idx;
	}

	void PC::ClearCurrentQuestBeginFlag()
	{
		//cerr << "iIndex " << m_RunningQuestState->iIndex << endl;
		SetSendFlag(QUEST_SEND_ISBEGIN);
		m_RunningQuestState->bStart = false;
		//SetFlag(m_stCurQuest+".__isbegin", 0);
	}

	void PC::SetCurrentQuestBeginFlag()
	{
		CQuestManager& q = CQuestManager::instance();
		int iQuestIndex = q.GetQuestIndexByName(m_stCurQuest);
		m_RunningQuestState->bStart = true;
		m_RunningQuestState->iIndex = iQuestIndex;

		SetSendFlag(QUEST_SEND_ISBEGIN);
		//SetFlag(m_stCurQuest+".__isbegin", iQuestIndex);
	}

	int PC::GetCurrentQuestBeginFlag()
	{
		return m_RunningQuestState?m_RunningQuestState->iIndex:0;
		//return GetFlag(m_stCurQuest+".__isbegin");
	}

	void PC::SetCurrentQuestTitle(const string& title)
	{
		SetSendFlag(QUEST_SEND_TITLE);
		m_RunningQuestState->_title = title;
	}

	void PC::SetQuestTitle(const string& quest, const string& title)
	{
		//SetSendFlag(QUEST_SEND_TITLE);
		QuestInfo::iterator it = m_QuestInfo.find(CQuestManager::instance().GetQuestIndexByName(quest));

		if (it != m_QuestInfo.end()) 
		{
			//(*it)->_title = title;
			QuestState* old = m_RunningQuestState;
			int old2 = m_iSendToClient;
			std::string oldquestname = m_stCurQuest;
			m_stCurQuest = quest;
			m_RunningQuestState = &it->second;
			m_iSendToClient = QUEST_SEND_TITLE;
			m_RunningQuestState->iIndex = GetCurrentQuestBeginFlag();

			SetCurrentQuestTitle(title);

			SendQuestInfoPakcet();

			m_stCurQuest = oldquestname;
			m_RunningQuestState = old;
			m_iSendToClient = old2;
		}
	}

	void PC::SetCurrentQuestClockName(const string& name)
	{
		SetSendFlag(QUEST_SEND_CLOCK_NAME);
		m_RunningQuestState->_clock_name = name;
	}

	void PC::SetCurrentQuestClockValue(int value)
	{
		SetSendFlag(QUEST_SEND_CLOCK_VALUE);
		m_RunningQuestState->_clock_value = value;
	}

	void PC::SetCurrentQuestCounterName(const string& name)
	{
		SetSendFlag(QUEST_SEND_COUNTER_NAME);
		m_RunningQuestState->_counter_name = name;
	}

	void PC::SetCurrentQuestCounterValue(int value)
	{
		SetSendFlag(QUEST_SEND_COUNTER_VALUE);
		m_RunningQuestState->_counter_value = value;
	}

	void PC::SetCurrentQuestIconFile(const string& icon_file)
	{
		SetSendFlag(QUEST_SEND_ICON_FILE);
		m_RunningQuestState->_icon_file = icon_file;
	}

	void PC::Save()
	{
		if (m_FlagSaveMap.empty())
			return;

		static std::vector<TQuestTable> s_table;
		s_table.resize(m_FlagSaveMap.size());

		int i = 0;

		TFlagMap::iterator it = m_FlagSaveMap.begin();

		while (it != m_FlagSaveMap.end())
		{
			const std::string & stComp = it->first;
			long lValue = it->second;

			++it;

			int iPos = stComp.find(".");

			if (iPos < 0)
			{
				sys_err("quest::PC::Save : cannot find . in FlagMap");
				continue;
			}

			string stName;
			stName.assign(stComp, 0, iPos);

			string stState;
			stState.assign(stComp, iPos + 1, stComp.length());

			if (stName.length() == 0 || stState.length() == 0)
			{
				sys_err("quest::PC::Save : invalid quest data: %s", stComp.c_str());
				continue;
			}

			sys_log(1, "QUEST Save Flag %s, %s %d (%d)", stName.c_str(), stState.c_str(), lValue, i);

			if (stName.length() >= QUEST_NAME_MAX_LEN)
			{
				sys_err("quest::PC::Save : quest name overflow");
				continue;
			}

			if (stState.length() >= QUEST_STATE_MAX_LEN)
			{
				sys_err("quest::PC::Save : quest state overflow");
				continue;
			}

			TQuestTable & r = s_table[i++];

			r.dwPID = m_dwID;
			strlcpy(r.szName, stName.c_str(), sizeof(r.szName));
			strlcpy(r.szState, stState.c_str(), sizeof(r.szState));
			r.lValue = lValue;
		}

		if (i > 0)
		{
			sys_log(1, "QuestPC::Save %d", i);
			db_clientdesc->DBPacketHeader(HEADER_GD_QUEST_SAVE, 0, sizeof(TQuestTable) * i);
			db_clientdesc->Packet(&s_table[0], sizeof(TQuestTable) * i);
		}

		m_FlagSaveMap.clear();
	}

	bool PC::HasQuest(const string & quest_name)
	{
		unsigned int qi = CQuestManager::instance().GetQuestIndexByName(quest_name);
		return m_QuestInfo.find(qi)!=m_QuestInfo.end();
	}

	QuestState & PC::GetQuest(const string & quest_name)
	{
		unsigned int qi = CQuestManager::instance().GetQuestIndexByName(quest_name);
		return m_QuestInfo[qi];
	}

	void PC::GiveItem(const string& label, DWORD dwVnum, int count)
	{
		sys_log(1, "QUEST GiveItem %s %d %d", label.c_str(),dwVnum,count);
		if (!GetFlag(m_stCurQuest+"."+label))
		{
			m_vRewardData.push_back(RewardData(RewardData::REWARD_TYPE_ITEM, dwVnum, count));
			//SetFlag(m_stCurQuest+"."+label,1);
		}
		else
			m_bIsGivenReward = true;
	}

	void PC::GiveExp(const string& label, DWORD exp)
	{
		sys_log(1, "QUEST GiveExp %s %d", label.c_str(),exp);

		if (!GetFlag(m_stCurQuest+"."+label))
		{
			m_vRewardData.push_back(RewardData(RewardData::REWARD_TYPE_EXP, exp));
			//SetFlag(m_stCurQuest+"."+label,1);
		}
		else
			m_bIsGivenReward = true;
	}

	void PC::Reward(LPCHARACTER ch)
	{
		if (m_bIsGivenReward)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<퀘스트> 이전에 같은 보상을 받은 적이 있어 다시 받지 않습니다."));
			m_bIsGivenReward = false;
		}

		for (vector<RewardData>::iterator it = m_vRewardData.begin(); it != m_vRewardData.end(); ++it)
		{
			switch (it->type)
			{
				case RewardData::REWARD_TYPE_EXP:
					sys_log(0, "EXP cur %d add %d next %d",ch->GetExp(), it->value1, ch->GetNextExp());

					if (ch->GetExp() + it->value1 > ch->GetNextExp())
						ch->PointChange(POINT_EXP, ch->GetNextExp() - 1 - ch->GetExp());
					else
						ch->PointChange(POINT_EXP, it->value1);

					break;

				case RewardData::REWARD_TYPE_ITEM:
					ch->AutoGiveItem(it->value1, it->value2);
					break;

				case RewardData::REWARD_TYPE_NONE:
				default:
					sys_err("Invalid RewardData type");
					break;
			}
		}

		m_vRewardData.clear();
	}

	void PC::Build()
	{
		itertype(m_FlagMap) it;
		for (it = m_FlagMap.begin(); it != m_FlagMap.end(); ++it)
		{
			if (it->first.size()>9 && it->first.compare(it->first.size()-9,9, ".__status") == 0)
			{
				DWORD dwQuestIndex = CQuestManager::instance().GetQuestIndexByName(it->first.substr(0, it->first.size()-9));
				int state = it->second;
				QuestState qs;
				qs.st = state;

				m_QuestInfo.insert(make_pair(dwQuestIndex, qs));
			}
		}
	}

	void PC::ClearQuest(const string& quest_name)
	{
		string quest_name_with_dot = quest_name + '.';
		for (itertype(m_FlagMap) it = m_FlagMap.begin(); it!= m_FlagMap.end();)
		{
			itertype(m_FlagMap) itNow = it++;
			if (itNow->second != 0 && itNow->first.compare(0, quest_name_with_dot.size(), quest_name_with_dot) == 0)
			{
				//m_FlagMap.erase(itNow);
				SetFlag(itNow->first, 0);
			}
		}

		ClearTimer();

		quest::PC::QuestInfoIterator it = quest_begin();
		unsigned int questindex = quest::CQuestManager::instance().GetQuestIndexByName(quest_name);

		while (it!= quest_end())
		{
			if (it->first == questindex)
			{
				it->second.st = 0;
				break;
			}

			++it;
		}
	}

	void PC::SendFlagList(LPCHARACTER ch)
	{
		for (itertype(m_FlagMap) it = m_FlagMap.begin(); it!= m_FlagMap.end(); ++it)
		{
			if (it->first.size()>9 && it->first.compare(it->first.size()-9,9, ".__status") == 0)
			{
				const string quest_name = it->first.substr(0, it->first.size()-9);
				const char* state_name = CQuestManager::instance().GetQuestStateName(quest_name, it->second);
				ch->ChatPacket(CHAT_TYPE_INFO, "%s %s (%d)", quest_name.c_str(), state_name, it->second);
			}
			else
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "%s %d", it->first.c_str(), it->second);
			}
		}
	}
}

