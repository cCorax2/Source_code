
#include "stdafx.h"
#include "desc_client.h"
#include "item_manager.h"

int touch(const char *path)
{
	FILE	*fp;

	if ( !(fp = fopen(path, "a")) )
	{
		sys_err("touch failed");
		return (-1);
	}

	fclose(fp);
	return 0;
}

DWORD ITEM_MANAGER::GetNewID()
{
	assert(m_dwCurrentID != 0);

	if ( m_dwCurrentID >= m_ItemIDRange.dwMax )
	{
		if ( m_ItemIDSpareRange.dwMin == 0 || m_ItemIDSpareRange.dwMax == 0 || m_ItemIDSpareRange.dwUsableItemIDMin == 0 )
		{
			for ( int i=0; i < 10; i++ ) sys_err("ItemIDRange: FATAL ERROR!!! no more item id");
			touch(".killscript");
			thecore_shutdown();
			return 0;
		}
		else
		{
			sys_log(0, "ItemIDRange: First Range is full. Change to SpareRange %u ~ %u %u",
					m_ItemIDSpareRange.dwMin, m_ItemIDSpareRange.dwMax, m_ItemIDSpareRange.dwUsableItemIDMin);

			db_clientdesc->DBPacket(HEADER_GD_REQ_SPARE_ITEM_ID_RANGE, 0, &m_ItemIDRange.dwMax, sizeof(DWORD));

			SetMaxItemID(m_ItemIDSpareRange);

			m_ItemIDSpareRange.dwMin = 0;
			m_ItemIDSpareRange.dwMax = 0;
			m_ItemIDSpareRange.dwUsableItemIDMin = 0;
		}
	}

	return (m_dwCurrentID++);
}

bool ITEM_MANAGER::SetMaxItemID(TItemIDRangeTable range)
{
	m_ItemIDRange = range;

	if ( m_ItemIDRange.dwMin == 0 || m_ItemIDRange.dwMax == 0 || m_ItemIDRange.dwUsableItemIDMin == 0 )
	{
		for ( int i=0; i < 10; i++ ) sys_err("ItemIDRange: FATAL ERROR!!! ITEM ID RANGE is not set.");
		touch(".killscript");
		thecore_shutdown();
		return false;
	}

	m_dwCurrentID = range.dwUsableItemIDMin;

	sys_log(0, "ItemIDRange: %u ~ %u %u", m_ItemIDRange.dwMin, m_ItemIDRange.dwMax, m_dwCurrentID);

	return true;
}

bool ITEM_MANAGER::SetMaxSpareItemID(TItemIDRangeTable range)
{
	if ( range.dwMin == 0 || range.dwMax == 0 || range.dwUsableItemIDMin == 0 )
	{
		for ( int i=0; i < 10; i++ ) sys_err("ItemIDRange: FATAL ERROR!!! Spare ITEM ID RANGE is not set");
		return false;
	}

	m_ItemIDSpareRange = range;

	sys_log(0, "ItemIDRange: New Spare ItemID Range Recv %u ~ %u %u",
			m_ItemIDSpareRange.dwMin, m_ItemIDSpareRange.dwMax, m_ItemIDSpareRange.dwUsableItemIDMin);

	return true;
}

