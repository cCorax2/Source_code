
#include "stdafx.h"

#include "HackShield.h"

#include "HackShield_Impl.h"
#include "config.h"

bool CHackShieldManager::Initialize()
{
	impl_ = M2_NEW CHackShieldImpl;

	if (NULL == impl_)
	{
		return false;
	}

	return impl_->Initialize();
}

void CHackShieldManager::Release()
{
	if (NULL != impl_)
	{
		impl_->Release();

		M2_DELETE(impl_);

		impl_ = NULL;
	}
}

bool CHackShieldManager::CreateClientHandle(DWORD dwPlayerID)
{
	return impl_->CreateClientHandle(dwPlayerID);
}

void CHackShieldManager::DeleteClientHandle(DWORD dwPlayerID)
{
	impl_->DeleteClientHandle(dwPlayerID);
}

bool CHackShieldManager::SendCheckPacket(LPCHARACTER ch)
{
	return impl_->SendCheckPacket(ch);
}

bool CHackShieldManager::VerifyAck(LPCHARACTER ch, const void* buf)
{
	TPacketGCHSCheck* p = reinterpret_cast<TPacketGCHSCheck*>(const_cast<void*>(buf));

	return impl_->VerifyAck(ch, p);
}

