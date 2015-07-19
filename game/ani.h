/*********************************************************************
 * date        : 2007.11.16
 * file        : ani.h
 * author      : mhh
 * description : 
 */

#ifndef _ani_h_
#define _ani_h_


void ani_init();
DWORD ani_attack_speed(LPCHARACTER ch);
void ani_print_attack_speed();
DWORD ani_combo_speed(LPCHARACTER ch, BYTE combo);

#endif	/* _ani_h_ */


