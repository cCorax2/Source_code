#ifndef QUEST_NPC
#define QUEST_NPC

#include "questpc.h"

extern int test_server;

namespace quest
{
	using namespace std;

	enum
	{
		QUEST_START_STATE_INDEX = 0,
		QUEST_CHAT_STATE_INDEX = -1,
		QUEST_FISH_REFINE_STATE_INDEX = -2,
	};

	class PC;

	class NPC
	{
		public:
			// 인자가 없는 스크립트들
			// first: state number
			typedef map<int, AStateScriptType> AQuestScriptType;
			// first: quest number
			typedef map<unsigned int, AQuestScriptType> QuestMapType;

			// 인자가 있는 스크립트들
			// first: state number
			typedef map<int, vector<AArgScript> > AArgQuestScriptType;
			// first: quest number
			typedef map<unsigned int, AArgQuestScriptType> ArgQuestMapType;

			NPC();
			~NPC();

			void	Set(unsigned int vnum, const string & script_name);

			static bool HasStartState(const AQuestScriptType & q)
			{
				return q.find(0) != q.end();
			}

			static bool HasStartState(const AArgQuestScriptType& q)
			{
				return q.find(0) != q.end();
			}

			bool	OnServerTimer(PC& pc);

			bool	OnClick(PC& pc);
			bool	OnKill(PC& pc);
			bool	OnPartyKill(PC& pc);
			bool	OnTimer(PC& pc);
			bool	OnLevelUp(PC& pc);
			bool	OnLogin(PC& pc, const char * c_pszQuestName = NULL);
			bool	OnLogout(PC& pc);
			bool	OnButton(PC& pc, unsigned int quest_index);
			bool	OnInfo(PC& pc, unsigned int quest_index);
			bool	OnAttrIn(PC& pc);
			bool	OnAttrOut(PC& pc);
			bool	OnUseItem(PC& pc, bool bReceiveAll);
			bool	OnTakeItem(PC& pc);
			bool	OnEnterState(PC& pc, DWORD quest_index, int state);
			bool	OnLeaveState(PC& pc, DWORD quest_index, int state);
			bool	OnLetter(PC& pc, DWORD quest_index, int state);
			bool	OnChat(PC& pc);
			bool	HasChat();

			bool	OnItemInformer(PC& pc,unsigned int vnum);	// 독일 선물 기능 테스트

			bool	OnTarget(PC& pc, DWORD dwQuestIndex, const char * c_pszTargetName, const char * c_pszVerb, bool & bRet);
			bool	OnUnmount(PC& pc);

			// ITEM_PICK EVENT
			bool	OnPickupItem(PC& pc);
			// Special item group USE EVENT
			bool	OnSIGUse(PC& pc, bool bReceiveAll);


			bool	HandleEvent(PC& pc, int EventIndex);
			bool	HandleReceiveAllEvent(PC& pc, int EventIndex);
			bool	HandleReceiveAllNoWaitEvent(PC& pc, int EventIndex);

			bool	ExecuteEventScript(PC& pc, int EventIndex, DWORD dwQuestIndex, int iState);

			unsigned int GetVnum() { return m_vnum; }


		protected:
			template <typename TQuestMapType, typename FuncMatch, typename FuncMiss>
				void MatchingQuest(PC& pc, TQuestMapType& QuestMap, FuncMatch& fMatch, FuncMiss& fMiss);

			// true if quest still running, false if ended

			void LoadStateScript(int idx, const char* filename, const char* script_name);

			unsigned int m_vnum;
			QuestMapType m_mapOwnQuest[QUEST_EVENT_COUNT];
			ArgQuestMapType m_mapOwnArgQuest[QUEST_EVENT_COUNT];
			//map<string, AStartConditionScriptType> m_mapStartCondition[QUEST_EVENT_COUNT];
	};

	template <typename TQuestMapType, typename FuncMatch, typename FuncMiss>
		void NPC::MatchingQuest(PC& pc, TQuestMapType& QuestMap, FuncMatch& fMatch, FuncMiss& fMiss)
		{
			if (test_server)
				sys_log(0, "Click Quest : MatchingQuest");

			PC::QuestInfoIterator itPCQuest = pc.quest_begin();
			typename TQuestMapType::iterator itQuestMap = QuestMap.begin();

			while (itQuestMap != QuestMap.end())
			{
				if (itPCQuest == pc.quest_end() || itPCQuest->first > itQuestMap->first)
				{
					fMiss(itPCQuest, itQuestMap);
					++itQuestMap;
				}
				else if (itPCQuest->first < itQuestMap->first)
				{
					++itPCQuest;
				}
				else
				{
					fMatch(itPCQuest, itQuestMap);
					++itPCQuest;
					++itQuestMap;
				}
			}
		}
}
#endif
