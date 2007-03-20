/**
 * @file cl_hospital.h
 * @brief Header file for hospital related stuff
 */

/*
Copyright (C) 2002-2006 UFO: Alien Invasion team.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

extern void HOS_Reset(void);
extern qboolean HOS_HealCharacter(character_t* chr, qboolean hospital);
extern qboolean HOS_HealEmployee(employee_t* employee);
extern void HOS_HealAll(const base_t* const base);
extern void HOS_HospitalRun(void);
extern void HOS_GameSave(sizebuf_t *sb);
extern void HOS_GameLoad(sizebuf_t *sb);
