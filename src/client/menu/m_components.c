/**
 * @file m_components.c
 */

/*
Copyright (C) 2002-2010 UFO: Alien Invasion.

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

#include "m_internal.h"
#include "m_components.h"

/**
 * @brief Searches all components for the specified one
 * @param[in] name Name of the component we search
 * @return The component found, else NULL
 * @note Use dichotomic search
 */
uiNode_t *UI_GetComponent (const char *name)
{
	unsigned char min = 0;
	unsigned char max = uiGlobal.numComponents;

	while (min != max) {
		const int mid = (min + max) >> 1;
		const char diff = strcmp(uiGlobal.components[mid]->name, name);
		assert(mid < max);
		assert(mid >= min);

		if (diff == 0)
			return uiGlobal.components[mid];

		if (diff > 0)
			max = mid;
		else
			min = mid + 1;
	}

	return NULL;
}

/**
 * @brief Add a new component to the list of all components
 * @note Sort components by alphabet
 */
void UI_InsertComponent(uiNode_t* component)
{
	int pos = 0;
	int i;

	if (uiGlobal.numComponents >= MAX_COMPONENTS)
		Com_Error(ERR_FATAL, "UI_InsertComponent: hit MAX_COMPONENTS");

	/* search the insertion position */
	for (pos = 0; pos < uiGlobal.numComponents; pos++) {
		const uiNode_t* node = uiGlobal.components[pos];
		if (strcmp(component->name, node->name) < 0)
			break;
	}

	/* create the space */
	for (i = uiGlobal.numComponents - 1; i >= pos; i--)
		uiGlobal.components[i + 1] = uiGlobal.components[i];

	/* insert */
	uiGlobal.components[pos] = component;
	uiGlobal.numComponents++;
}
