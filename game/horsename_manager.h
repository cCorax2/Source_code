
class CHorseNameManager : public singleton<CHorseNameManager>
{
	private :
		std::map<DWORD, std::string> m_mapHorseNames;

		void BroadcastHorseName(DWORD dwPlayerID, const char* szHorseName);

	public :
		CHorseNameManager();

		const char* GetHorseName(DWORD dwPlayerID);

		void UpdateHorseName(DWORD dwPlayerID, const char* szHorseName, bool broadcast=false);

		void Validate(LPCHARACTER pChar);
};

