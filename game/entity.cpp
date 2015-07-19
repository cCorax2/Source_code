#include "stdafx.h"
#include "char.h"
#include "desc.h"
#include "sectree_manager.h"

CEntity::CEntity()
{
	Initialize();
}

CEntity::~CEntity()
{
	if (!m_bIsDestroyed)
		assert(!"You must call CEntity::destroy() method in your derived class destructor");
}

void CEntity::Initialize(int type)
{
	m_bIsDestroyed = false;

	m_iType = type;
	m_iViewAge = 0;
	m_pos.x = m_pos.y = m_pos.z = 0;
	m_map_view.clear();

	m_pSectree = NULL;
	m_lpDesc = NULL;
	m_lMapIndex = 0;
	m_bIsObserver = false;
	m_bObserverModeChange = false;
}

void CEntity::Destroy()
{
	if (m_bIsDestroyed) {
		return;
	}
	ViewCleanup();
	m_bIsDestroyed = true;
}

void CEntity::SetType(int type)
{
	m_iType = type;
}

int CEntity::GetType() const
{
	return m_iType;
}

bool CEntity::IsType(int type) const
{
	return (m_iType == type ? true : false);
}

struct FuncPacketAround
{
	const void *        m_data;
	int                 m_bytes;
	LPENTITY            m_except;

	FuncPacketAround(const void * data, int bytes, LPENTITY except = NULL) :m_data(data), m_bytes(bytes), m_except(except)
	{
	}

	void operator () (LPENTITY ent)
	{
		if (ent == m_except)
			return;

		if (ent->GetDesc())
			ent->GetDesc()->Packet(m_data, m_bytes);
	}
};

struct FuncPacketView : public FuncPacketAround
{
	FuncPacketView(const void * data, int bytes, LPENTITY except = NULL) : FuncPacketAround(data, bytes, except)
	{}

	void operator() (const CEntity::ENTITY_MAP::value_type& v)
	{
		FuncPacketAround::operator() (v.first);
	}
};

void CEntity::PacketAround(const void * data, int bytes, LPENTITY except)
{
	PacketView(data, bytes, except);
}

void CEntity::PacketView(const void * data, int bytes, LPENTITY except)
{
	if (!GetSectree())
		return;

	FuncPacketView f(data, bytes, except);

	// 옵저버 상태에선 내 패킷은 나만 받는다.
	if (!m_bIsObserver)
		for_each(m_map_view.begin(), m_map_view.end(), f);

	f(std::make_pair(this, 0));
}

void CEntity::SetObserverMode(bool bFlag)
{
	if (m_bIsObserver == bFlag)
		return;

	m_bIsObserver = bFlag;
	m_bObserverModeChange = true;
	UpdateSectree();

	if (IsType(ENTITY_CHARACTER))
	{
		LPCHARACTER ch = (LPCHARACTER) this;
		ch->ChatPacket(CHAT_TYPE_COMMAND, "ObserverMode %d", m_bIsObserver ? 1 : 0);
	}
}

