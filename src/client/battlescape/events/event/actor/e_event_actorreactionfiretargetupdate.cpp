/**
 * @file
 */

/*
Copyright (C) 2002-2022 UFO: Alien Invasion.

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

#include "../../../../client.h"
#include "../../../cl_localentity.h"
#include "../../../cl_actor.h"
#include "../../../cl_hud.h"
#include "../../../../ui/ui_main.h"
#include "e_event_actorreactionfiretargetupdate.h"

int CL_ActorReactionFireTargetUpdateTime (const eventRegister_t* self, dbuffer* msg, eventTiming_t* eventTiming)
{
	int targetEntNum;
	int unused;
	int step;

	NET_ReadFormat(msg, self->formatString, &unused, &targetEntNum, &unused, &step);

	const le_t* target = LE_Get(targetEntNum);
	if (!target)
		LE_NotFoundError(targetEntNum);
	if (step >= MAX_ROUTE)
		return eventTiming->nextTime;
	int stepTime = CL_GetStepTime(eventTiming, target, step);
	if (eventTiming->shootTime > stepTime)
		return eventTiming->impactTime;
	return stepTime;
}

/**
 * @brief Network event function for reaction fire target handling. Responsible for updating
 * the HUD with the information that were received from the server
 * @param self The event pointer
 * @param msg The network message to parse the event data from
 */
void CL_ActorReactionFireTargetUpdate (const eventRegister_t* self, dbuffer* msg)
{
	int shooterEntNum;
	int targetEntNum;
	// if these TUs have arrived at 0, the reaction fire can be triggered
	int tusUntilTriggered;
	int unused;

	NET_ReadFormat(msg, self->formatString, &shooterEntNum, &targetEntNum, &tusUntilTriggered, &unused);

	const le_t* shooter = LE_Get(shooterEntNum);
	if (!shooter)
		LE_NotFoundError(shooterEntNum);

	const le_t* target = LE_Get(targetEntNum);
	if (!target)
		LE_NotFoundError(targetEntNum);

	const bool outOfRange = CL_ActorIsReactionFireOutOfRange(shooter, target);
	UI_ExecuteConfunc("reactionfire_updatetarget %i %i %i %i", shooterEntNum, target->entnum, tusUntilTriggered, outOfRange);
}
