#ifndef __WEDDING_H
#define __WEDDING_H

#include "marriage.h"

namespace marriage
{
	const DWORD WEDDING_MAP_INDEX = 81;
	typedef CHARACTER_SET charset_t;

	class WeddingMap
	{
		public:
			WeddingMap(DWORD dwMapIndex, DWORD dwPID1, DWORD dwPID2);
			~WeddingMap();

			DWORD GetMapIndex() { return m_dwMapIndex; }

			void WarpAll();
			void DestroyAll();
			void Notice(const char* psz);
			void SetEnded();

			void IncMember(LPCHARACTER ch);
			void DecMember(LPCHARACTER ch);
			bool IsMember(LPCHARACTER ch );

			void SetDark(bool bSet);
			void SetSnow(bool bSet);
			void SetMusic(bool bSet, const char* szMusicFileName);

			bool IsPlayingMusic();

			void SendLocalEvent(LPCHARACTER ch);

			void ShoutInMap(BYTE type, const char* szMsg);
		private:

			const char* __BuildCommandPlayMusic(char* szCommand, size_t nCmdLen, BYTE bSet, const char* c_szMusicFileName); 

		private:
			DWORD m_dwMapIndex;
			LPEVENT m_pEndEvent;
			charset_t m_set_pkChr;

			bool m_isDark;
			bool m_isSnow;
			bool m_isMusic;

			DWORD dwPID1;
			DWORD dwPID2;

			std::string m_stMusicFileName;
	};

	class WeddingManager : public singleton<WeddingManager>
	{
		public:
			WeddingManager();
			virtual ~WeddingManager();

			bool IsWeddingMap(DWORD dwMapIndex);

			void Request(DWORD dwPID1, DWORD dwPID2);
			bool End(DWORD dwMapIndex);

			void DestroyWeddingMap(WeddingMap* pMap);

			WeddingMap* Find(DWORD dwMapIndex);

		private:
			DWORD __CreateWeddingMap(DWORD dwPID1, DWORD dwPID2);

		private:

			std::map<DWORD, WeddingMap*> m_mapWedding;
	};
}
#endif
