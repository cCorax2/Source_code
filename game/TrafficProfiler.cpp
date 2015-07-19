/**
 *
 * @file	TrafficProfiler.cpp
 * @brief	TrafficProfiler class implementation file
 * @author	Bang2ni
 * @version	05/07/07 Bang2ni - First release.
 *
 */

#include "stdafx.h"
#include "TrafficProfiler.h"

TrafficProfiler::TrafficProfiler()
	: m_pfProfileLogFile(NULL), m_dwFlushCycle(0), m_tmProfileStartTime(0), m_dwTotalTraffic(0), m_dwTotalPacket(0)
{
	m_aTrafficVec[ 0 ].resize( 256 );
	m_aTrafficVec[ 1 ].resize( 256 );
}

TrafficProfiler::~TrafficProfiler()
{
	if ( m_pfProfileLogFile )
		fclose( m_pfProfileLogFile );
}

bool TrafficProfiler::Initialize( DWORD dwFlushCycle, const char* pszFileName )
{
	m_pfProfileLogFile = fopen( pszFileName, "w" );
	if ( !m_pfProfileLogFile )
		return false;

	m_dwFlushCycle = dwFlushCycle;
	InitializeProfiling();

	return true;
}

bool TrafficProfiler::Flush()
{
	if ( !m_pfProfileLogFile )
		return false;

	//
	// Profling result write to file
	//

	fprintf( m_pfProfileLogFile, "# Profile Start: %s", ctime( &m_tmProfileStartTime ) );
	fprintf( m_pfProfileLogFile, "Total traffic: %u bytes\n", m_dwTotalTraffic );
	fprintf( m_pfProfileLogFile, "Total used packet: %u\n", m_dwTotalPacket );

	fprintf( m_pfProfileLogFile, "------------------ Input ------------------\n" );

	for ( int idx = 0; idx < (int)IODIR_MAX; idx++ ) 
	{
		fprintf( m_pfProfileLogFile, "Packet\tCount\tTotal Size\tAverage\n" );

		BYTE byHeader = 0;
		for ( TrafficVec::iterator it = m_aTrafficVec[ idx ].begin(); it != m_aTrafficVec[ idx ].end(); ++it, byHeader++ ) 
		{
			if ( it->second )
				fprintf( m_pfProfileLogFile, "%d\t%u\t%u\t\t%u\n", byHeader, it->second, it->first, it->first / it->second );
		}

		fprintf( m_pfProfileLogFile, "------------------ Output -----------------\n" );
	}

	time_t cur = time( NULL );
	fprintf( m_pfProfileLogFile, "# Profile End(Flush): %s", ctime( &cur ) );
	fflush( m_pfProfileLogFile );

	//
	// Initialization
	//

	InitializeProfiling();

	return true;
}

void TrafficProfiler::InitializeProfiling()
{
	m_tmProfileStartTime = time( NULL );
	m_dwTotalPacket = 0;
	m_dwTotalTraffic = 0;

	TrafficInfo empty( 0, 0 );
	for ( int idx = 0; idx < (int)IODIR_MAX; idx++ ) 
	{
		for ( TrafficVec::iterator it = m_aTrafficVec[ idx ].begin(); it != m_aTrafficVec[ idx ].end(); ++it )
			*it = empty;
	}
}
