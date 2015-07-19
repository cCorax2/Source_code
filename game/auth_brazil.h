/* vi: set sw=4 ts=8 cino=g0,\:0 : */
/*********************************************************************
 * date        : 2010.4.7
 * file        : auth_brazil.h
 * author      : mhh
 * description : 
 */
#ifndef __auth_brazil_h_1270647899__
#define __auth_brazil_h_1270647899__

#define AUTH_BRAZIL_SERVER_ERR		0
#define AUTH_BRAZIL_SUCC		1
#define AUTH_BRAZIL_NOID		2
#define AUTH_BRAZIL_WRONGPWD		3
#define AUTH_BRAZIL_FLASHUSER		4

int auth_brazil(const char *login, const char *pwd);


int auth_brazil_inc_query_count();
void auth_brazil_log();

#endif // __auth_brazil_h_1270647899__
