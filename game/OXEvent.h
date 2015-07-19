
#define OXEVENT_MAP_INDEX 113

struct tag_Quiz
{
	char level;
	char Quiz[256];
	bool answer;
};

enum OXEventStatus
{
	OXEVENT_FINISH = 0, // OX이벤트가 완전히 끝난 상태
	OXEVENT_OPEN = 1,	// OX이벤트가 시작됨. 을두지(20012)를 통해서 입장가능
	OXEVENT_CLOSE = 2,	// OX이벤트의 참가가 끝남. 을두지(20012)를 통한 입장이 차단됨
	OXEVENT_QUIZ = 3,	// 퀴즈를 출제함.

	OXEVENT_ERR = 0xff
};

class COXEventManager : public singleton<COXEventManager>
{
	private :
		std::map<DWORD, DWORD> m_map_char;
		std::map<DWORD, DWORD> m_map_attender;
		std::map<DWORD, DWORD> m_map_miss;

		std::vector<std::vector<tag_Quiz> > m_vec_quiz;

		LPEVENT m_timedEvent;

	protected :
		bool CheckAnswer();

		bool EnterAudience(LPCHARACTER pChar);
		bool EnterAttender(LPCHARACTER pChar);

	public :
		bool Initialize();
		void Destroy();

		OXEventStatus GetStatus();
		void SetStatus(OXEventStatus status);

		bool LoadQuizScript(const char* szFileName);

		bool Enter(LPCHARACTER pChar);
		
		bool CloseEvent();

		void ClearQuiz();
		bool AddQuiz(unsigned char level, const char* pszQuestion, bool answer);
		bool ShowQuizList(LPCHARACTER pChar);

		bool Quiz(unsigned char level, int timelimit);
		bool GiveItemToAttender(DWORD dwItemVnum, BYTE count);

		bool CheckAnswer(bool answer);
		void WarpToAudience();

		bool LogWinner();

		DWORD GetAttenderCount() { return m_map_attender.size(); }
};

