/**
 *
 * @file	TrafficProfiler.h
 * @brief	TrafficProfiler class definition file
 * @author	Bang2ni
 * @version	05/07/07 Bang2ni - First release.
 * 
 */

#ifndef _METIN_II_TRAFFICPROFILER_H_
#define _METIN_II_TRAFFICPROFILER_H_

/**
 * @class	TrafficProfiler
 * @brief	Network I/O traffic 을 패킷 단위로 측정하는 profiler.
 * @author	Bang2ni
 * @version	05/07/07 Bang2ni - First release.
 *
 * 시간대 별로 Network I/O 의 traffic 을 패킷 단위로 측정하고, Text file 형태로 보고서를 작성한다.
 */
class TrafficProfiler : public singleton< TrafficProfiler >
{
	public:

		/// I/O 방향
		enum IODirection {
			IODIR_INPUT	= 0,	///< Input
			IODIR_OUTPUT,	///< Output
			IODIR_MAX
		};

	public:

		/// Constructor
		TrafficProfiler( void );

		/// Destructor
		~TrafficProfiler( void );

		/// Profiling 에 필요한 초기화를 한다.
		/**
		 * @param [in]	dwFlushCycle Flush 주기. 초 단위이다.
		 * @param [in]	pszLogFileName Profiling log file 의 이름
		 * @return	false 일 경우 profiling log file 을 open 하지 못했다.
		 *
		 * profiling log file 을 open(생성) 한다.
		 */
		bool	Initialize( DWORD dwFlushCycle, const char* pszLogFileName );

		/// Profiling 을 위해 전송됐거나 전송 할 Packet 을 Report 한다.
		/**
		 * @param [in]	dir Profiling 할 Packet 의 방향
		 * @param [in]	byHeader Packet 헤더
		 * @param [in]	dwSize Packet 의 총 size
		 * @return	Initialize 되지 않았다면 false 를 반환한다.
		 *
		 * Packet 에 해당하는 size 를 누적시킨다.
		 * Initialize 이후나 최근 Flush 된 이후에 Flush 주기 만큼 시간이 흐른 후 호출된다면 Report 이후 Flush 한다.
		 */
		bool	Report( IODirection dir, BYTE byHeader, DWORD dwSize )
		{
			ComputeTraffic( dir, byHeader, dwSize );
			if ( (DWORD)(time( NULL ) - m_tmProfileStartTime) >= m_dwFlushCycle )
				return Flush();
			return true;
		}

		/// 현재까지 Report 된 내용을 파일에 쓴다.
		/**
		 * @return	Initialize 되지 않았다.
		 */
		bool	Flush( void );

	private:

		/// Profling 에 관련된 variables 를 초기화 한다.
		void	InitializeProfiling( void );

		/// Report 된 Packet 의 traffic 를 계산한다.
		/**
		 * @param [in]	dir Profiling 할 Packet 의 방향
		 * @param [in]	byHeader Packet 헤더
		 * @param [in]	dwSize Packet 의 총 size
		 */
		void	ComputeTraffic( IODirection dir, BYTE byHeader, DWORD dwSize )
		{

			TrafficInfo& rTrafficInfo = m_aTrafficVec[ dir ][ byHeader ];

			m_dwTotalTraffic += dwSize;
			m_dwTotalPacket += !rTrafficInfo.second;

			rTrafficInfo.first += dwSize;
			rTrafficInfo.second++;
		}

		/// Traffic info type.
		/**
		 * first: 누적된 총 size
		 * second: 이 packet 이 전송된 횟수
		 */
		typedef std::pair< DWORD, DWORD >	TrafficInfo;

		/// Traffic info vector.
		typedef std::vector< TrafficInfo >	TrafficVec;

		FILE*		m_pfProfileLogFile;	///< Profile log file pointer
		DWORD		m_dwFlushCycle;		///< Flush 주기
		time_t		m_tmProfileStartTime;	///< 프로파일을 시작한 시간. Flush 될 때마다 Update 된다.
		DWORD		m_dwTotalTraffic;	///< Report 된 총 Traffic 용량
		DWORD		m_dwTotalPacket;	///< Report 된 총 Packet 수
		TrafficVec	m_aTrafficVec[ IODIR_MAX ];	///< Report 된 Traffic 을 저장할 vector의 배열. 각 방향마다 vector 를 가진다.
};

#endif // _METIN_II_TRAFFICPROFILER_H_
