#ifndef __INC_METIN_II_SERVER_LOGINSIM__
#define __INC_METIN_II_SERVER_LOGINSIM__

#include "desc_client.h"

class CLoginSim
{
	public:
		CLoginSim()
		{
			memset(&auth, 0, sizeof(auth));
			memset(&login, 0, sizeof(login));
			memset(&load, 0, sizeof(load));
			memset(&logout, 0, sizeof(logout));
			vecIdx = 0;
			bCheck = false;
		}

		void AddPlayer(DWORD dwID)
		{
			vecID.push_back(dwID);
			sys_log(0, "AddPlayer %lu", dwID);
		}

		bool IsCheck()
		{
			return bCheck;
		}

		void SendLogin()
		{
			bCheck = true;

			if (IsDone())
				return;

			if (vecIdx == 0)
				db_clientdesc->DBPacket(HEADER_GD_AUTH_LOGIN, 0, &auth, sizeof(TPacketGDAuthLogin));

			load.player_id = vecID[vecIdx++];
			db_clientdesc->DBPacket(HEADER_GD_LOGIN_BY_KEY, 0, &login, sizeof(TPacketGDLoginByKey));
		}

		void SendLoad()
		{
			db_clientdesc->DBPacket(HEADER_GD_PLAYER_LOAD, 0, &load, sizeof(TPlayerLoadPacket));
		}

		void SendLogout()
		{
			db_clientdesc->DBPacket(HEADER_GD_LOGOUT, 0, &logout, sizeof(TLogoutPacket));
			SendLogin();
		}

		bool IsDone()
		{
			if (vecIdx >= vecID.size())
				return true;

			return false;
		}

		TPacketGDAuthLogin auth;
		TPacketGDLoginByKey login;
		TPlayerLoadPacket load;
		TLogoutPacket logout;

		std::vector<DWORD> vecID;
		unsigned int vecIdx;
		bool bCheck;
};

#endif
