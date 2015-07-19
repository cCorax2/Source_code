
#ifndef HACK_SHIELD_IMPL_H_
#define HACK_SHIELD_IMPL_H_

#include <boost/unordered_map.hpp>

#ifdef __FreeBSD__
// Live build only
#define UNIX
#include <AntiCpXSvr.h>
#undef UNIX
#endif

#pragma pack(1)

typedef struct SPacketGCHSCheck
{
	BYTE	bHeader;
#ifdef __FreeBSD__
	AHNHS_TRANS_BUFFER Req;
#endif
} TPacketGCHSCheck;

#pragma pack()

class CHackShieldImpl
{
	public:
		bool Initialize ();
		void Release ();

		bool CreateClientHandle (DWORD dwPlayerID);
		void DeleteClientHandle (DWORD dwPlayerID);

		bool SendCheckPacket (LPCHARACTER ch);
		bool VerifyAck (LPCHARACTER ch, TPacketGCHSCheck* buf);

	private:
#ifdef __FreeBSD__
		AHNHS_SERVER_HANDLE handle_;

		typedef boost::unordered_map<DWORD, AHNHS_CLIENT_HANDLE> ClientHandleContainer;
		ClientHandleContainer CliehtHandleMap_;

		typedef boost::unordered_map<DWORD, bool> ClientCheckContainer;
		ClientCheckContainer ClientCheckMap_;
#endif
};

#endif /* HACK_SHIELD_IMPL_H_ */

