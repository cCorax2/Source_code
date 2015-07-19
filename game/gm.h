extern void gm_insert(const char * name, BYTE level);
extern BYTE gm_get_level(const char * name, const char * host = NULL, const char * account = NULL);
extern void gm_host_insert(const char * host);
extern void gm_new_clear();
extern void gm_new_insert( const tAdminInfo & c_rInfo );
extern void gm_new_host_inert( const char * host );
