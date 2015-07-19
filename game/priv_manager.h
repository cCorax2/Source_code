#ifndef __PRIV_MANAGER_H
#define __PRIV_MANAGER_H

/**
 * @version 05/06/08	Bang2ni - Guild privilege 관련 함수 지속 시간 추가
 * 			          RequestGiveGuildPriv, GiveGuildPriv 함수 프로토타잎 수정
 * 			          m_aPrivGuild 타잎 수정
 * 			          구조체 SPrivGuildData, 멤버 함수 GetPrivByGuildEx 추가
 */
class CPrivManager : public singleton<CPrivManager>
{
	public:
		CPrivManager();

		void RequestGiveGuildPriv(DWORD guild_id, BYTE type, int value, time_t dur_time_sec);
		void RequestGiveEmpirePriv(BYTE empire, BYTE type, int value, time_t dur_time_sec);
		void RequestGiveCharacterPriv(DWORD pid, BYTE type, int value);

		void GiveGuildPriv(DWORD guild_id, BYTE type, int value, BYTE bLog, time_t end_time_sec);
		void GiveEmpirePriv(BYTE empire, BYTE type, int value, BYTE bLog, time_t end_time_sec);
		void GiveCharacterPriv(DWORD pid, BYTE type, int value, BYTE bLog);

		void RemoveGuildPriv(DWORD guild_id, BYTE type);
		void RemoveEmpirePriv(BYTE empire, BYTE type);
		void RemoveCharacterPriv(DWORD pid, BYTE type);

		int GetPriv(LPCHARACTER ch, BYTE type);
		int GetPrivByEmpire(BYTE bEmpire, BYTE type);
		int GetPrivByGuild(DWORD guild_id, BYTE type);
		int GetPrivByCharacter(DWORD pid, BYTE type);

	public:
		struct SPrivEmpireData
		{
			int m_value;
			time_t m_end_time_sec;
		};

		SPrivEmpireData* GetPrivByEmpireEx(BYTE bEmpire, BYTE type);

		/// 길드 보너스 데이터
		struct SPrivGuildData
		{
			int		value;		///< 보너스 수치
			time_t	end_time_sec;	///< 지속 시간
		};

		/// 길드 보너스 데이터를 얻어온다.
		/**
		 * @param [in]	dwGuildID 얻어올 길드의 ID
		 * @param [in]	byType 보너스 타잎
		 * @return	대상 길드의 길드 보너스 데이터의 포인터, 해당하는 보너스 타잎과 길드의 ID 에 대해 보너스 데이터가 없을 경우 NULL
		 */
		const SPrivGuildData*	GetPrivByGuildEx( DWORD dwGuildID, BYTE byType ) const;

	private:
		SPrivEmpireData m_aakPrivEmpireData[MAX_PRIV_NUM][EMPIRE_MAX_NUM];
		std::map<DWORD, SPrivGuildData> m_aPrivGuild[MAX_PRIV_NUM];
		std::map<DWORD, int> m_aPrivChar[MAX_PRIV_NUM];
};
#endif
