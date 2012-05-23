/**
 * @file
 */

/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#ifndef _SCRIPTLIB
#define _SCRIPTLIB

extern char parsedToken[MAX_TOKEN_CHARS];

void LoadScriptFile(const char *filename);
void ParseFromMemory(char *buffer, int size);

bool GetToken(bool crossline);
bool TokenAvailable(void);
int GetScriptLine(void);

#endif /* _SCRIPTLIB */
