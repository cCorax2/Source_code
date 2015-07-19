/*********************************************************************
 * date        : 2007.05.31
 * file        : block_country.h
 * author      : mhh
 * description : 
 */

#ifndef _block_country_h_
#define _block_country_h_


void add_blocked_country_ip(TPacketBlockCountryIp *data);
void block_exception(TPacketBlockException *data);
bool is_blocked_country_ip(const char *user_ip);
bool is_block_exception(const char *login);

#endif	// _block_country_h_

