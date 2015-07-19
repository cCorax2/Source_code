#ifndef IFILEMONITOR_INCLUDED
#define IFILEMONITOR_INCLUDED

//#include <boost/function.hpp>
#include <boost/unordered_map.hpp>

enum eFileUpdatedOptions
{
	e_FileUpdate_None = -1,
	e_FileUpdate_Error,
	e_FileUpdate_Deleted,
	e_FileUpdate_Modified,
	e_FileUpdate_AttrModified,
	e_FileUpdate_Linked,
	e_FileUpdate_Renamed,
	e_FileUpdate_Revoked,
};

// TODO : in FreeBSD boost function doesn`t work with boost bind
// so currently we only support for static function ptr only
//typedef boost::function< void ( const std::string&, eFileUpdatedOptions ) > PFN_FileChangeListener;      
typedef void (* PFN_FileChangeListener )(const std::string&, eFileUpdatedOptions);

struct IFileMonitor
{
	virtual void Update		(DWORD dwPulses) = 0;
	virtual void AddWatch	(const std::string& strFileName, PFN_FileChangeListener pListenerFunc) = 0; 
};

#endif // IFILEMONITOR_INCLUDED
