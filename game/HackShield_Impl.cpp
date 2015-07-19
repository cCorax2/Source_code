#include "stdafx.h"

#include "HackShield_Impl.h"

#ifdef __FreeBSD__

#include "char.h"
#include "packet.h"
#include "desc.h"
#include "log.h"

bool CHackShieldImpl::Initialize()
{
	handle_ = _AhnHS_CreateServerObject("metin2client.bin.hsb");

	if (ANTICPX_INVALID_HANDLE_VALUE == handle_)
	{
		return false;
	}

	sys_log(0, "HShield: Success to CreateServerObject");

	return true;
}

void CHackShieldImpl::Release()
{
	_AhnHS_CloseServerHandle(handle_);

	sys_log(0, "HShield: Server Handle Closed");
}

bool CHackShieldImpl::CreateClientHandle(DWORD dwPlayerID)
{
	ClientHandleContainer::const_iterator iter = CliehtHandleMap_.find( dwPlayerID );

	if (iter != CliehtHandleMap_.end())
	{
		sys_log(0, "HShield: Client Handle is already created for Player(%u)", dwPlayerID);
		
		return false;
	}

	AHNHS_CLIENT_HANDLE handle = _AhnHS_CreateClientObject(handle_);

	if (ANTICPX_INVALID_HANDLE_VALUE == handle)
	{
		sys_log(0, "HShield: Failed to create client handle for Player(%u)", dwPlayerID);

		return false;
	}

	CliehtHandleMap_.insert( std::make_pair(dwPlayerID, handle) );

	sys_log(0, "HShield: Success to create client handle for Player(%u)", dwPlayerID);

	return true;
}

void CHackShieldImpl::DeleteClientHandle(DWORD dwPlayerID)
{
	ClientHandleContainer::iterator iter = CliehtHandleMap_.find( dwPlayerID );

	if (iter == CliehtHandleMap_.end())
	{
		sys_log(0, "HShield: there is no client handle for Player(%u)", dwPlayerID);

		return;
	}

	_AhnHS_CloseClientHandle(iter->second);

	CliehtHandleMap_.erase(iter);

	sys_log(0, "HShield: client handle deleted for Player(%u)", dwPlayerID);
}

bool CHackShieldImpl::SendCheckPacket(LPCHARACTER ch)
{
	if (NULL == ch)
	{
		return false;
	}

	ClientHandleContainer::const_iterator iter = CliehtHandleMap_.find( ch->GetPlayerID() );

	if (iter == CliehtHandleMap_.end())
	{
		sys_log(0, "HShield: Client Handle not create for Player(%u)", ch->GetPlayerID());
		return false;
	}

	TPacketGCHSCheck pack;
	pack.bHeader = HEADER_GC_HS_REQUEST;

	memset( &pack.Req, 0, sizeof(pack.Req));
	unsigned long ret = _AhnHS_MakeRequest( iter->second, &(pack.Req) );

	if (0 != ret)
	{
		sys_log(0, "HShield: _AhnHS_MakeRequest return error(%u) for Player(%u)", ret, ch->GetPlayerID());
		return false;
	}
	else
	{
		sys_log(0, "HShield: _AhnHS_MakeRequest success ret(%d)", ret);
	}

	if (NULL != ch->GetDesc())
	{
		ch->GetDesc()->Packet( &pack, sizeof(pack) );
		sys_log(0, "HShield: Send Check Request for Player(%u)", ch->GetPlayerID());

		return true;
	}

	sys_log(0, "HShield: Failed to get DESC for Player(%u)", ch->GetPlayerID());

	return false;
}

bool CHackShieldImpl::VerifyAck(LPCHARACTER ch, TPacketGCHSCheck* buf)
{
	if (NULL == ch)
	{
		return false;
	}

	bool NeedDisconnect = false;

	ClientHandleContainer::const_iterator iter = CliehtHandleMap_.find( ch->GetPlayerID() );

	if (iter == CliehtHandleMap_.end())
	{
		sys_log(0, "HShield: Cannot Find ClientHandle For Verify");

		NeedDisconnect = true;
	}

	unsigned long dwError = 0;

	unsigned long ret = _AhnHS_VerifyResponseEx( iter->second, buf->Req.byBuffer, buf->Req.nLength, &dwError );

	if (ANTICPX_RECOMMAND_CLOSE_SESSION == ret)
	{
		sys_log(0, "HShield: not a valid ack ret(%u) error(%u) from Player(%u)", ret, dwError, ch->GetPlayerID());
		NeedDisconnect = true;

		ch->StopHackShieldCheckCycle();
	}

	if (NULL != ch->GetDesc())
	{
		if (true == NeedDisconnect)
		{
			ch->GetDesc()->SetPhase(PHASE_CLOSE);
			LogManager::instance().HackShieldLog(dwError, ch);
			return false;
		}
		else
		{
			ch->SetHackShieldCheckMode(false);
		}
	}

	sys_log(0, "HShield: Valid Ack from Player(%u)", ch->GetPlayerID());

	return true;
}

#else

bool CHackShieldImpl::Initialize()
{
	return true;
}

void CHackShieldImpl::Release()
{
}

bool CHackShieldImpl::CreateClientHandle(DWORD dwPlayerID)
{
	return true;
}

void CHackShieldImpl::DeleteClientHandle(DWORD dwPlayerID)
{
}

bool CHackShieldImpl::SendCheckPacket(LPCHARACTER ch)
{
	return true;
}

bool CHackShieldImpl::VerifyAck(LPCHARACTER ch, TPacketGCHSCheck* buf)
{
	return true;
}

#endif

