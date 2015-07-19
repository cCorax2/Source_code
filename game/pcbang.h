
typedef unsigned long PCBang_IP;
typedef unsigned long PCBang_ID;

class CPCBangManager : public singleton<CPCBangManager>
{
	private:
		std::map<PCBang_IP, PCBang_ID> m_map_ip;

		time_t m_minSavablePlayTime;

		PCBang_ID __GetIDFromString(const char* c_szID);
		PCBang_IP __GetIPFromString(const char* c_szIP);

	public:
		CPCBangManager();

		void InsertIP(const char* c_szID, const char* c_szIP);

		bool Log(const char* c_szIP, DWORD pid, time_t playTime);

		void RequestUpdateIPList(PCBang_ID id);

		bool IsPCBangIP(const char* c_szIP);
};

