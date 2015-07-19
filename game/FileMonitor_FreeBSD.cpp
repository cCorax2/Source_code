#include "stdafx.h"
#include "FileMonitor_FreeBSD.h"
#include "../../libthecore/include/log.h"


#define INVALID_KERNEL_EVENT	-1

FileMonitorFreeBSD::FileMonitorFreeBSD()
{
	m_KernelEventQueue = INVALID_KERNEL_EVENT;
}

FileMonitorFreeBSD::~FileMonitorFreeBSD()
{
	if( m_KernelEventQueue != INVALID_KERNEL_EVENT )
	{
		close ( m_KernelEventQueue );
		m_KernelEventQueue = INVALID_KERNEL_EVENT;
	}

	TMonitorFileHashMap::iterator it;
	for( it = m_FileLists.begin(); it != m_FileLists.end(); ++it )
	{
		close(it->second.fhMonitor);
	}

	m_FileLists.clear();

	m_MonitoredEventLists.clear();
	m_TriggeredEventLists.clear();
}


void FileMonitorFreeBSD::Update(DWORD dwPulses)
{
	if( m_KernelEventQueue == INVALID_KERNEL_EVENT || m_FileLists.size() == 0 )
		return;

	int nEvent = kevent(m_KernelEventQueue, &m_TriggeredEventLists[0], (int)m_TriggeredEventLists.size(), &m_MonitoredEventLists[0], (int)m_MonitoredEventLists.size(), NULL );
	if( nEvent == INVALID_KERNEL_EVENT )
	{
		return;
	}
	else if( nEvent > 0 )
	{
		for( int i = 0; i < nEvent; ++i )
		{
			int nEventFlags = m_MonitoredEventLists[i].flags;
			eFileUpdatedOptions eUpdateOption = e_FileUpdate_None;

			if (nEventFlags & EV_ERROR)
				eUpdateOption = e_FileUpdate_Error;

			else if (nEventFlags & NOTE_DELETE)
				eUpdateOption = e_FileUpdate_Deleted;
				
			else if (nEventFlags & NOTE_EXTEND || nEventFlags & NOTE_WRITE)
				eUpdateOption = e_FileUpdate_Modified;

			else if (nEventFlags & NOTE_ATTRIB)
				eUpdateOption = e_FileUpdate_AttrModified;

			else if (nEventFlags & NOTE_LINK)
				eUpdateOption = e_FileUpdate_Linked;

			else if (nEventFlags & NOTE_RENAME)
				eUpdateOption = e_FileUpdate_Renamed;

			else if (nEventFlags & NOTE_REVOKE)
				eUpdateOption = e_FileUpdate_Revoked;

			if( eUpdateOption != e_FileUpdate_None )
			{
				TMonitorFileHashMap::iterator it;
				for( it = m_FileLists.begin(); it != m_FileLists.end(); ++it )
				{
					FileIOContext_FreeBSD& context = it->second;
					if( context.idxToEventList == i )
					{
						std::string strModifedFileName = it->first;
						context.pListenFunc( strModifedFileName, eUpdateOption );
						break;
					}
				}
			}
		}
	}
}

void FileMonitorFreeBSD::AddWatch(const std::string& strFileName, PFN_FileChangeListener pListenerFunc)
{
	int iFileHandle = -1;
	if( (iFileHandle = open(strFileName.c_str(), O_RDONLY)) == -1) 
	{
		sys_err("FileMonitorFreeBSD:AddWatch : can`t open file(%s).\n", strFileName.c_str());
		return; 
	}

	//create kqueue if not exists
	if( m_KernelEventQueue == INVALID_KERNEL_EVENT )
		m_KernelEventQueue = kqueue();

	if( m_KernelEventQueue == INVALID_KERNEL_EVENT )
	{
		sys_err("FileMonitorFreeBSD:AddWatch : failed to create kqueue.\n");
		return;
	}

	TMonitorFileHashMap::iterator it = m_FileLists.find( strFileName );
	if( it != m_FileLists.end() )
	{
		sys_log(0, "FileMonitorFreeBSD:AddWatch : trying to add duplicated watch on file(%s).\n", strFileName.c_str() );
		return;
	}

	//set file context
	FileIOContext_FreeBSD context;
	{
		context.fhMonitor	   = iFileHandle;
		context.idxToEventList = (int)m_MonitoredEventLists.size();
		context.pListenFunc	   = pListenerFunc;
	}

	m_FileLists[strFileName] = context;

	//set events
	struct kevent kTriggerEvent, kMonitorEvent;

   	EV_SET(&kTriggerEvent, iFileHandle, EVFILT_VNODE, 
										EV_ADD | EV_ENABLE | EV_ONESHOT,
										NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB | NOTE_LINK | NOTE_RENAME | NOTE_REVOKE,
										0, 0);

	m_TriggeredEventLists.push_back( kTriggerEvent );
	m_MonitoredEventLists.push_back( kMonitorEvent );
}