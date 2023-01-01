/**
 * @file
 */

/*
Copyright (C) 2002-2023 UFO: Alien Invasion.

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

#pragma once

#include "../ui_nodes.h"

class uiLineChartNode : public uiLocatedNode {
	void draw(uiNode_t* node) override;
	void deleteNode(uiNode_t* node) override;
};

/**
 * @brief extradata for the linechart node
 * @todo add info about axes min-max...
 */
typedef struct lineChartExtraData_s {
	linkedList_t* lines;		/**< list of lines to draw */
	bool displayAxes;			/**< If true the node display axes */
	vec4_t axesColor;			/**< color of the axes */
} lineChartExtraData_t;

void UI_RegisterLineChartNode(uiBehaviour_t* behaviour);
bool UI_ClearLineChart(uiNode_t* node);
bool UI_AddLineChartLine(uiNode_t* node, const char* id, bool visible, const vec4_t color, bool dots, int numPoints);
bool UI_AddLineChartCoord(uiNode_t* node, const char* id, int x, int y);
bool UI_ShowChartLine(uiNode_t* node, const char* id, bool visible);
bool UI_ShowChartDots(uiNode_t* node, const char* id, bool visible);
