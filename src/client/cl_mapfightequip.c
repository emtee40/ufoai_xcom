/**
 * @file cl_mapfightequip.c
 * @brief contains everything related to equiping slots of aircraft or base
 * @note Base defense functions prefix: BDEF_
 * @note Aircraft items slots functions prefix: AIM_
 */

/*
Copyright (C) 2002-2007 UFO: Alien Invasion team.

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

#include "client.h"
#include "cl_global.h"
#include "cl_mapfightequip.h"

static int airequipID = -1;				/**< value of aircraftItemType_t that defines what item we are installing. */
static qboolean noparams = qfalse;			/**< true if AIM_AircraftEquipMenuUpdate_f or BDEF_BaseDefenseMenuUpdate_f don't need paramters */
static int airequipSelectedZone = ZONE_NONE;		/**< Selected zone in equip menu */
static int airequipSelectedSlot = ZONE_NONE;			/**< Selected slot in equip menu */
static technology_t *airequipSelectedTechnology = NULL;		/**< Selected technolgy in equip menu */

/**
 * @brief Returns craftitem weight based on size.
 * @param[in] od Pointer to objDef_t object being craftitem.
 * @return itemWeight_t
 * @sa AII_WeightToName
 */
itemWeight_t AII_GetItemWeightBySize (objDef_t *od)
{
	assert(od);
	assert(od->craftitem.type >= 0);

	if (od->size < 50)
		return ITEM_LIGHT;
	else if (od->size < 100)
		return ITEM_MEDIUM;
	else
		return ITEM_HEAVY;
}


/**
 * @brief Returns a list of craftitem technologies for the given type.
 * @note This list is terminated by a NULL pointer.
 * @param[in] type Type of the craft-items to return.
 */
static technology_t **AII_GetCraftitemTechsByType (int type, base_t* base)
{
	static technology_t *techList[MAX_TECHNOLOGIES];
	objDef_t *aircraftitem = NULL;
	int i, j = 0;

	assert(base);

	for (i = 0; i < csi.numODs; i++) {
		aircraftitem = &csi.ods[i];
		if (aircraftitem->craftitem.type == type) {
			assert(j < MAX_TECHNOLOGIES);
			techList[j] = aircraftitem->tech;
			j++;
		}
		/* j+1 because last item has to be NULL */
		if (j + 1 >= MAX_TECHNOLOGIES) {
			Com_Printf("AII_GetCraftitemTechsByType: MAX_TECHNOLOGIES limit hit.\n");
			break;
		}
	}
	/* terminate the list */
	techList[j] = NULL;
	return techList;
}

/**
 * @brief Check airequipID value and set the correct values for aircraft items
 * and base defense items
 */
static void AIM_CheckAirequipID (void)
{
	const menu_t *activeMenu = NULL;
	qboolean aircraftMenu;

	/* select menu */
	activeMenu = MN_GetActiveMenu();
	aircraftMenu = !Q_strncmp(activeMenu->name, "aircraft_equip", 14);

	if (aircraftMenu) {
		switch (airequipID) {
		case AC_ITEM_AMMO:
		case AC_ITEM_WEAPON:
		case AC_ITEM_SHIELD:
		case AC_ITEM_ELECTRONICS:
			return;
		default:
			airequipID = AC_ITEM_WEAPON;
		}
	} else {
		switch (airequipID) {
		case AC_ITEM_BASE_MISSILE:
		case AC_ITEM_BASE_LASER:
		case AC_ITEM_AMMO_MISSILE:
		case AC_ITEM_AMMO_LASER:
			return;
		default:
			airequipID = AC_ITEM_BASE_MISSILE;
		}
	}
}

/**
 * @brief Check that airequipSelectedZone is available
 * @sa slot Pointer to the slot
 */
static void AIM_CheckAirequipSelectedZone (aircraftSlot_t *slot)
{
	/* You can choose an ammo only if a weapon has already been selected */
	if (airequipID >= AC_ITEM_AMMO && !slot->item) {
		airequipSelectedZone = ZONE_MAIN;
		switch (airequipID) {
		case AC_ITEM_AMMO:
			airequipID = AC_ITEM_WEAPON;
			break;
		case AC_ITEM_AMMO_MISSILE:
			airequipID = AC_ITEM_BASE_MISSILE;
			break;
		case AC_ITEM_AMMO_LASER:
			airequipID = AC_ITEM_BASE_LASER;
			break;
		default:
			Com_Printf("AIM_CheckAirequipSelectedZone: aircraftItemType_t must end with ammos !!!\n");
			return;
		}
	}

	/* You can select an item to install after removing only if you're removing an item */
	if (airequipSelectedZone == ZONE_NEXT && (slot->installationTime >= 0 || !slot->item)) {
		airequipSelectedZone = ZONE_MAIN;
		switch (airequipID) {
		case AC_ITEM_AMMO:
			airequipID = AC_ITEM_WEAPON;
			break;
		case AC_ITEM_BASE_MISSILE:
			airequipID = AC_ITEM_AMMO_MISSILE;
			break;
		case AC_ITEM_AMMO_LASER:
			airequipID = AC_ITEM_BASE_LASER;
			break;
		/* no default */
		}
	}
}

/**
 * @brief Returns a pointer to the selected slot.
 * @param[in] aircraft Pointer to the aircraft
 * @return Pointer to the slot corresponding to airequipID
 */
static aircraftSlot_t *AII_SelectAircraftSlot (aircraft_t *aircraft)
{
	aircraftSlot_t *slot;

	switch (airequipID) {
	case AC_ITEM_SHIELD:
		slot = &aircraft->shield;
		break;
	case AC_ITEM_ELECTRONICS:
		slot = aircraft->electronics + airequipSelectedSlot;
		break;
	case AC_ITEM_AMMO:
	case AC_ITEM_WEAPON:
		slot = aircraft->weapons + airequipSelectedSlot;
		break;
	default:
		Com_Printf("AII_SelectAircraftSlot: Unknown airequipID: %i\n", airequipID);
		return NULL;
	}

	return slot;
}

/**
 * @brief Returns a pointer to the selected slot.
 * @param[in] base Pointer to the aircraft
 * @return Pointer to the slot corresponding to airequipID
 * @note There is always at least one slot, otherwise you can't enter base defense menu screen.
 * @sa BDEF_ListClick_f
 */
static aircraftSlot_t *BDEF_SelectBaseSlot (base_t *base)
{
	aircraftSlot_t *slot;
	menuNode_t *node;

	switch (airequipID) {
	case AC_ITEM_AMMO_MISSILE:
	case AC_ITEM_BASE_MISSILE:
		assert(base->maxBatteries > 0);
		if (airequipSelectedSlot >= base->maxBatteries) {
			airequipSelectedSlot = 0;
			/* update position of the arrow in front of the selected base defense */
			node = MN_GetNodeFromCurrentMenu("basedef_selected_slot");
			Vector2Set(node->pos, 25, 30);
		}
		slot = base->batteries + airequipSelectedSlot;
		break;
	case AC_ITEM_AMMO_LASER:
	case AC_ITEM_BASE_LASER:
		assert(base->maxLasers > 0);
		if (airequipSelectedSlot >= base->maxLasers) {
			airequipSelectedSlot = 0;
			/* update position of the arrow in front of the selected base defense */
			node = MN_GetNodeFromCurrentMenu("basedef_selected_slot");
			Vector2Set(node->pos, 25, 30);
		}
		slot = base->lasers + airequipSelectedSlot;
		break;
	default:
		Com_Printf("BDEF_SelectBaseSlot: Unknown airequipID: %i\n", airequipID);
		return NULL;
	}

	return slot;
}

/**
 * @brief Check if an aircraft item should or should not be displayed in airequip menu
 * @param[in] tech Pointer to the technology to test
 * @return qtrue if the aircraft item should be displayed, qfalse else
 */
static qboolean AIM_SelectableAircraftItem (base_t* base, aircraft_t *aircraft, technology_t *tech)
{
	objDef_t *item;
	aircraftSlot_t *slot;

	if (aircraft)
		slot = AII_SelectAircraftSlot(aircraft);
	else if (base)
		slot = BDEF_SelectBaseSlot(base);
	else {
		Com_Printf("AIM_SelectableAircraftItem: no aircraft and no base given\n");
		return qfalse;
	}

	/* item is researched? */
	if (!RS_IsResearched_ptr(tech))
		return qfalse;

	item = AII_GetAircraftItemByID(tech->provides);
	if (!item)
		return qfalse;

	/* you can choose an ammo only if it fits the weapons installed in this slot */
	if (airequipID >= AC_ITEM_AMMO) {
		/* FIXME: This only works for ammo that is useable in exactly one weapon
		 * check the weap_idx array and not only the first value */
		if (item->weapons[0] != slot->item)
			return qfalse;
	}

	/* you can install an item only if its weight is small enough for the slot */
	if (AII_GetItemWeightBySize(item) > slot->size)
		return qfalse;

	/* you can't install an item that you don't possess
	 * missiles does not need to be possessed */
	if (aircraft) {
		if (aircraft->homebase->storage.num[item->idx] <= 0)
			return qfalse;
	} else if (base->storage.num[item->idx] <= 0 && !item->notOnMarket)
			return qfalse;

	/* you can't install an item that does not have an installation time (alien item)
	 * except for ammo which does not have installation time */
	if (item->craftitem.installationTime == -1 && airequipID < AC_ITEM_AMMO)
		return qfalse;

	return qtrue;
}

/**
 * @brief Update the list of item you can choose
 */
static void AIM_UpdateAircraftItemList (base_t* base, aircraft_t* aircraft)
{
	static char buffer[1024];
	technology_t **list;
	int i;

	/* Delete list */
	buffer[0] = '\0';

	assert(base || aircraft);

	if (airequipID == -1) {
		Com_Printf("AIM_UpdateAircraftItemList: airequipID is -1\n");
		return;
	}

	/* Add all items corresponding to airequipID to list */
	list = AII_GetCraftitemTechsByType(airequipID, base ? base : baseCurrent);

	/* Copy only those which are researched to buffer */
	i = 0;
	while (*list) {
		if (AIM_SelectableAircraftItem(base, aircraft, *list)) {
			Q_strcat(buffer, va("%s\n", _((*list)->name)), sizeof(buffer));
			i++;
		}
		list++;
	}

	/* copy buffer to mn.menuText to display it on screen */
	mn.menuText[TEXT_LIST] = buffer;

	/* if there is at least one element, select the first one */
	if (i)
		Cmd_ExecuteString("airequip_list_click 0");
	else {
		/* no element in list, no tech selected */
		airequipSelectedTechnology = NULL;
		UP_AircraftItemDescription(NONE);
	}
}

/**
 * @brief Highlight selected zone
 */
static void AIM_DrawSelectedZone (void)
{
	menuNode_t *node;

	node = MN_GetNodeFromCurrentMenu("airequip_zone_select1");
	if (airequipSelectedZone == ZONE_MAIN)
		MN_HideNode(node);
	else
		MN_UnHideNode(node);

	node = MN_GetNodeFromCurrentMenu("airequip_zone_select2");
	if (airequipSelectedZone == ZONE_NEXT)
		MN_HideNode(node);
	else
		MN_UnHideNode(node);

	node = MN_GetNodeFromCurrentMenu("airequip_zone_select3");
	if (airequipSelectedZone == ZONE_AMMO)
		MN_HideNode(node);
	else
		MN_UnHideNode(node);
}

/**
 * @brief Adds a defense system to base.
 * @param[in] basedefType Base defense type (see basedefenseType_t)
 * @param[in] base Pointer to the base in which the battery will be added
 */
static void BDEF_AddBattery (basedefenseType_t basedefType, base_t* base)
{
	switch (basedefType) {
	case BASEDEF_MISSILE:
		if (base->maxBatteries >= MAX_BASE_SLOT) {
			Com_Printf("BDEF_AddBattery: too many missile batteries in base\n");
			return;
		}

		base->maxBatteries++;
		break;
	case BASEDEF_LASER:
		if (base->maxLasers >= MAX_BASE_SLOT) {
			Com_Printf("BDEF_AddBattery: too many laser batteries in base\n");
			return;
		}
		/* slots has a lot of ammo for now */
		/* FIXME: it should be unlimited, no ? check that when we'll know how laser battery work */
		base->lasers[base->maxLasers].ammoLeft = 9999;

		base->maxLasers++;
		break;
	default:
		Com_Printf("BDEF_AddBattery: unknown type of base defense system.\n");
	}
}

/**
 * @brief Reload the battery of every bases
 * @todo we should define the number of ammo to reload and the period of reloading in the .ufo file
 */
void BDEF_ReloadBattery (void)
{
	int i, j;

	/* Reload all ammos of aircraft */
	for (i = 0; i < gd.numBases; i++) {
		for (j = 0; j < gd.bases[i].maxBatteries; j++) {
			if (gd.bases[i].batteries[j].ammoLeft >= 0 && gd.bases[i].batteries[j].ammoLeft < 20)
				gd.bases[i].batteries[j].ammoLeft++;
		}
	}
}

/**
 * @brief Adds a defense system to base.
 */
void BDEF_AddBattery_f (void)
{
	int basedefType, baseIdx;

	if (Cmd_Argc() < 3)
		return;
	else {
		basedefType = atoi(Cmd_Argv(1));
		baseIdx = atoi(Cmd_Argv(2));
	}

	/* Check that the baseIdx exists */
	if (baseIdx < 0 || baseIdx >= gd.numBases) {
		Com_Printf("BDEF_AddBattery_f: baseIdx %i doesn't exists: there is only %i bases in game.\n", baseIdx, gd.numBases);
		return;
	}

	/* Check that the basedefType exists */
	if (basedefType != BASEDEF_MISSILE && basedefType != BASEDEF_LASER) {
		Com_Printf("BDEF_AddBattery_f: base defense type %i doesn't exists.\n", basedefType);
		return;
	}

	BDEF_AddBattery(basedefType, B_GetBase(baseIdx));
}

/**
 * @brief Remove a base defense sytem from base.
 * @param[in] basedefType (see basedefenseType_t)
 * @param[in] idx idx of the battery to destroy (-1 if this is random)
 */
void BDEF_RemoveBattery (base_t *base, basedefenseType_t basedefType, int idx)
{
	assert(base);

	/* Select the type of base defense system to destroy */
	switch (basedefType) {
	case BASEDEF_MISSILE: /* this is a missile battery */
		/* we must have at least one missile battery to remove it */
		assert(base->maxBatteries > 0);
		if (idx < 0)
			idx = rand() % base->maxBatteries;
		if (idx < base->maxBatteries - 1)
			memmove(base->batteries + idx, base->batteries + idx + 1, sizeof(aircraftSlot_t) * (base->maxBatteries - idx - 1));
		base->maxBatteries--;
		/* just for security */
		AII_InitialiseSlot(base->batteries + base->maxBatteries, NULL, base, AC_ITEM_BASE_MISSILE);
		break;
	case BASEDEF_LASER: /* this is a laser battery */
		/* we must have at least one laser battery to remove it */
		assert(base->maxLasers > 0);
		if (idx < 0)
			idx = rand() % base->maxLasers;
		if (idx < base->maxLasers - 1)
			memmove(base->lasers + idx, base->lasers + idx + 1, sizeof(aircraftSlot_t) * (base->maxLasers - idx - 1));
		base->maxLasers--;
		/* just for security */
		AII_InitialiseSlot(base->lasers + base->maxLasers, NULL, base, AC_ITEM_BASE_LASER);
		break;
	default:
		Com_Printf("BDEF_RemoveBattery_f: unknown type of base defense system.\n");
	}
}

/**
 * @brief Remove a defense system from base.
 * @note 1st argument is the basedefense system type to destroy (sa basedefenseType_t).
 * @note 2nd argument is the idx of the base in which you want the battery to be destroyed.
 * @note if the first argument is BASEDEF_RANDOM, the type of the battery to destroy is randomly selected
 * @note the building must already be removed from gd.buildings[baseIdx][]
 */
void BDEF_RemoveBattery_f (void)
{
	int basedefType, baseIdx;
	base_t *base;

	if (Cmd_Argc() < 3)
		return;
	else {
		basedefType = atoi(Cmd_Argv(1));
		baseIdx = atoi(Cmd_Argv(2));
	}

	/* Check that the baseIdx exists */
	if (baseIdx < 0 || baseIdx >= gd.numBases) {
		Com_Printf("BDEF_RemoveBattery_f: baseIdx %i doesn't exists: there is only %i bases in game.\n", baseIdx, gd.numBases);
		return;
	}

	base = B_GetBase(baseIdx);

	if (basedefType == BASEDEF_RANDOM) {
		/* Type of base defense to destroy is randomly selected */
		if (base->maxBatteries <= 0 && base->maxLasers <= 0) {
			Com_Printf("No base defense to destroy\n");
			return;
		} else if (base->maxBatteries <= 0) {
			/* only laser battery is possible */
			basedefType = BASEDEF_LASER;
		} else if (base->maxLasers <= 0) {
			/* only missile battery is possible */
			basedefType = BASEDEF_MISSILE;
		} else {
			/* both type are possible, choose one randomly */
			basedefType = rand() % 2 + BASEDEF_MISSILE;
		}
	} else {
		/* Check if the removed building was under construction */
		int i, type, max;
		int workingNum = 0;

		switch (basedefType) {
		case BASEDEF_MISSILE:
			type = B_DEFENSE_MISSILE;
			max = base->maxBatteries;
			break;
		case BASEDEF_LASER:
			type = B_DEFENSE_MISSILE;
			max = base->maxLasers;
			break;
		default:
			Com_Printf("BDEF_RemoveBattery_f: base defense type %i doesn't exists.\n", basedefType);
			return;
		}

		for (i = 0; i < gd.numBuildings[baseIdx]; i++) {
			if (gd.buildings[baseIdx][i].buildingType == type
				&& gd.buildings[baseIdx][i].buildingStatus == B_STATUS_WORKING)
				workingNum++;
		}

		if (workingNum == max) {
			/* Removed building was under construction, do nothing */
			return;
		} else if ((workingNum != max - 1)) {
			/* Should never happen, we only remove building one by one */
			Com_Printf("BDEF_RemoveBattery_f: Error while checking number of batteries (%i instead of %i).\n", workingNum, max);
			return;
		}

		/* If we reached this point, that means we are removing a working building: continue */
	}

	BDEF_RemoveBattery(base, basedefType, -1);
}

/**
 * @brief Initialise all values of base slot defense.
 * @param[in] base Pointer to the base which needs initalisation of its slots.
 */
void BDEF_InitialiseBaseSlots (base_t *base)
{
	int i;

	for (i = 0; i < MAX_BASE_SLOT; i++) {
		AII_InitialiseSlot(base->batteries + i, NULL, base, AC_ITEM_BASE_MISSILE);
		AII_InitialiseSlot(base->lasers + i, NULL, base, AC_ITEM_BASE_LASER);
		base->targetMissileIdx[i] = AIRFIGHT_BASE_CAN_T_FIRE;
		base->targetLaserIdx[i] = AIRFIGHT_BASE_CAN_T_FIRE;
	}
}

/**
 * @brief Script command to init the base defense menu.
 * @note this function is only called when the menu launches
 * @sa BDEF_BaseDefenseMenuUpdate_f
 */
void BDEF_MenuInit_f (void)
{
	menuNode_t *node;

	/* initialize selected item */
	Cvar_Set("basedef_item_name", "main");
	airequipSelectedTechnology = NULL;

	/* initialize different variables */
	airequipID = -1;
	noparams = qfalse;
	airequipSelectedZone = ZONE_NONE;
	airequipSelectedSlot = ZONE_NONE;

	/* initialize selected slot */
	airequipSelectedSlot = 0;
	/* update position of the arrow in front of the selected base defense */
	node = MN_GetNodeFromCurrentMenu("basedef_selected_slot");
	Vector2Set(node->pos, 25, 30);
}

/**
 * @brief Fills the battery list, descriptions, and weapons in slots
 * of the basedefense equip menu
 * @sa BDEF_MenuInit_f
 */
void BDEF_BaseDefenseMenuUpdate_f (void)
{
	static char defBuffer[1024];
	static char smallbuffer1[128];
	static char smallbuffer2[128];
	static char smallbuffer3[128];
	objDef_t *item;
	aircraftSlot_t *slot;
	int i;
	int type;
	menuNode_t *node;

	/* don't let old links appear on this menu */
	MN_MenuTextReset(TEXT_BASEDEFENSE_LIST);
	MN_MenuTextReset(TEXT_AIREQUIP_1);
	MN_MenuTextReset(TEXT_AIREQUIP_2);
	MN_MenuTextReset(TEXT_AIREQUIP_3);
	MN_MenuTextReset(TEXT_STANDARD);

	/* baseCurrent should be non NULL because we are in the menu of this base */
	if (!baseCurrent)
		return;

	/* Check that the base has at least 1 battery */
	if (baseCurrent->maxBatteries + baseCurrent->maxLasers < 1) {
		Com_Printf("BDEF_BaseDefenseMenuUpdate_f: there is no defense battery in this base: you shouldn't be in this function.\n");
		return;
	}

	if (Cmd_Argc() != 2 || noparams) {
		if (airequipID == -1) {
			Com_Printf("Usage: %s <num>\n", Cmd_Argv(0));
			return;
		}
		AIM_CheckAirequipID();
	} else {
		type = atoi(Cmd_Argv(1));
		switch (type) {
		case 0:
			/* missiles */
			airequipID = AC_ITEM_BASE_MISSILE;
			break;
		case 1:
			/* lasers */
			airequipID = AC_ITEM_BASE_LASER;
			break;
		case 2:
			/* ammo */
			if (airequipID == AC_ITEM_BASE_MISSILE)
				airequipID = AC_ITEM_AMMO_MISSILE;
			else if (airequipID == AC_ITEM_BASE_LASER)
				airequipID = AC_ITEM_AMMO_LASER;
			break;
		default:
			Com_Printf("BDEF_BaseDefenseMenuUpdate_f: Unvalid type %i.\n", type);
			return;
		}
	}

	/* Check if we can change to laser or missile */
	if (baseCurrent->maxBatteries > 0 && baseCurrent->maxLasers > 0) {
		node = MN_GetNodeFromCurrentMenu("basedef_button_missile");
		MN_UnHideNode(node);
		node = MN_GetNodeFromCurrentMenu("basedef_button_missile_str");
		MN_UnHideNode(node);
		node = MN_GetNodeFromCurrentMenu("basedef_button_laser");
		MN_UnHideNode(node);
		node = MN_GetNodeFromCurrentMenu("basedef_button_laser_str");
		MN_UnHideNode(node);
	}

	/* Select slot */
	slot = BDEF_SelectBaseSlot(baseCurrent);

	/* Check that the selected zone is OK */
	AIM_CheckAirequipSelectedZone(slot);

	/* Fill the list of item you can equip your aircraft with */
	AIM_UpdateAircraftItemList(baseCurrent, NULL);

	/* Delete list */
	defBuffer[0] = '\0';

	if (airequipID == AC_ITEM_BASE_MISSILE || airequipID == AC_ITEM_AMMO_MISSILE) {
		/* we are in the base defense menu for missile */
		if (baseCurrent->maxBatteries == 0)
			Q_strcat(defBuffer, _("No defense of this type in this base\n"), sizeof(defBuffer));
		else {
			for (i = 0; i < baseCurrent->maxBatteries; i++) {
				if (!baseCurrent->batteries[i].item)
					Q_strcat(defBuffer, va(_("Slot %i:\tempty\n"), i), sizeof(defBuffer));
				else {
					item = baseCurrent->batteries[i].item;
					Q_strcat(defBuffer, va(_("Slot %i:\t%s\n"), i, _(item->tech->name)), sizeof(defBuffer));
				}
			}
		}
	} else if (airequipID == AC_ITEM_BASE_LASER || airequipID == AC_ITEM_AMMO_LASER) {
		/* we are in the base defense menu for laser */
		if (baseCurrent->maxLasers == 0)
			Q_strcat(defBuffer, _("No defense of this type in this base\n"), sizeof(defBuffer));
		else {
			for (i = 0; i < baseCurrent->maxLasers; i++) {
				if (!baseCurrent->lasers[i].item)
					Q_strcat(defBuffer, va(_("Slot %i:\tempty\n"), i), sizeof(defBuffer));
				else {
					item = baseCurrent->lasers[i].item;
					Q_strcat(defBuffer, va(_("Slot %i:\t%s\n"), i, _(item->tech->name)), sizeof(defBuffer));
				}
			}
		}
	} else {
		Com_Printf("BDEF_BaseDefenseMenuUpdate_f: unknown airequipId.\n");
		return;
	}
	mn.menuText[TEXT_BASEDEFENSE_LIST] = defBuffer;

	/* Fill the texts of each zone */
	/* First slot: item currently assigned */
	if (!slot->item) {
		Com_sprintf(smallbuffer1, sizeof(smallbuffer1), _("No defense system assigned.\n"));
		/* Weight are not used for base defense atm
		Q_strcat(smallbuffer1, va(_("This slot is for %s or smaller items."), AII_WeightToName(slot->size)), sizeof(smallbuffer1)); */
	} else {
		Com_sprintf(smallbuffer1, sizeof(smallbuffer1), _(slot->item->tech->name));
		Q_strcat(smallbuffer1, "\n", sizeof(smallbuffer1));
		if (!slot->installationTime)
			Q_strcat(smallbuffer1, _("This defense system is functional.\n"), sizeof(smallbuffer1));
		else if (slot->installationTime > 0)
			Q_strcat(smallbuffer1, va(_("This defense system will be installed in %i hours.\n"), slot->installationTime), sizeof(smallbuffer1));
		else
			Q_strcat(smallbuffer1, va(_("This defense system will be removed in %i hours.\n"), -slot->installationTime), sizeof(smallbuffer1));
	}
	mn.menuText[TEXT_AIREQUIP_1] = smallbuffer1;

	/* Second slot: next item to install when the first one will be removed */
	if (slot->item && slot->installationTime < 0) {
		if (!slot->nextItem)
			Com_sprintf(smallbuffer2, sizeof(smallbuffer2), _("No defense system assigned."));
		else {
			Com_sprintf(smallbuffer2, sizeof(smallbuffer2), _(slot->nextItem->tech->name));
			Q_strcat(smallbuffer2, "\n", sizeof(smallbuffer2));
			Q_strcat(smallbuffer2, va(_("This defense system will be operational in %i hours.\n"), slot->nextItem->craftitem.installationTime - slot->installationTime), sizeof(smallbuffer2));
		}
	} else {
		*smallbuffer2 = '\0';
	}
	mn.menuText[TEXT_AIREQUIP_2] = smallbuffer2;

	/* Third slot: ammo slot (only used for weapons) */
	if ((airequipID < AC_ITEM_WEAPON || airequipID > AC_ITEM_AMMO) && slot->item) {
		if (!slot->ammo)
			Com_sprintf(smallbuffer3, sizeof(smallbuffer3), _("No ammo assigned to this defense system."));
		else
			Com_sprintf(smallbuffer3, sizeof(smallbuffer3), _(slot->ammo->tech->name));
		/* show remaining missile in battery for missiles */
		if ((airequipID == AC_ITEM_AMMO_MISSILE) || (airequipID == AC_ITEM_BASE_MISSILE))
			Q_strcat(smallbuffer3, va(ngettext(" (%i missile left)", " (%i missiles left)", slot->ammoLeft), slot->ammoLeft), sizeof(smallbuffer3));
	} else {
		*smallbuffer3 = '\0';
	}
	mn.menuText[TEXT_AIREQUIP_3] = smallbuffer3;

	/* Draw selected zone */
	AIM_DrawSelectedZone();

	noparams = qfalse;
}

/**
 * @brief Click function for base defense menu list.
 * @sa AIM_AircraftEquipMenuUpdate_f
 */
void BDEF_ListClick_f (void)
{
	int num, height;
	menuNode_t *node;

	if (!baseCurrent)
		return;

	if (Cmd_Argc() < 2)
		return;
	num = atoi(Cmd_Argv(1));

	if (num < baseCurrent->maxBatteries)
		airequipSelectedSlot = num;

	/* draw an arrow in front of the selected base defense */
	node = MN_GetNodeFromCurrentMenu("basedef_slot_list");
	height = node->texh[0];
	node = MN_GetNodeFromCurrentMenu("basedef_selected_slot");
	Vector2Set(node->pos, 25, 30 + height * airequipSelectedSlot);

	noparams = qtrue;
	BDEF_BaseDefenseMenuUpdate_f();
}

/**
 * @brief Update the installation delay of one slot.
 * @param[in] base Pointer to the base to update the storage and capacity for
 * @param[in] aircraft Pointer to the aircraft (NULL if a base is updated)
 * @param[in] slot Pointer to the slot to update
 * @sa AII_AddItemToSlot
 */
static void AII_UpdateOneInstallationDelay (base_t* base, aircraft_t *aircraft, aircraftSlot_t *slot)
{
	assert(base);

	/* if the item is already installed, nothing to do */
	if (slot->installationTime == 0)
		return;
	else if (slot->installationTime > 0) {
		/* the item is being installed */
		slot->installationTime--;
		/* check if installation is over */
		if (slot->installationTime <= 0) {
			/* Update stats values */
			if (aircraft) {
				AII_UpdateAircraftStats(aircraft);
				MN_AddNewMessage(_("Notice"), _("Aircraft item was successfully installed."), qfalse, MSG_STANDARD, NULL);
			} else {
				MN_AddNewMessage(_("Notice"), _("Base defense item was successfully installed."), qfalse, MSG_STANDARD, NULL);
			}
		}
	} else if (slot->installationTime < 0) {
		/* the item is being removed */
		slot->installationTime++;
		if (slot->installationTime >= 0) {
#ifdef DEBUG
			if (aircraft && aircraft->homebase != base)
				Sys_Error("AII_UpdateOneInstallationDelay: aircraft->homebase and base pointers are out of sync\n");
#endif
			AII_RemoveItemFromSlot(base, slot, qfalse);
			if (aircraft) {
				AII_UpdateAircraftStats(aircraft);
				/* Only stop time and post a notice, if no new item to install is assigned */
				if (!slot->item) {
					MN_AddNewMessage(_("Notice"), _("Aircraft item was successfully removed."), qfalse, MSG_STANDARD, NULL);
					CL_GameTimeStop();
				}
			} else if (!slot->item) {
				MN_AddNewMessage(_("Notice"), _("Base defense item was successfully removed."), qfalse, MSG_STANDARD, NULL);
				CL_GameTimeStop();
			}
		}
	}
}

/**
 * @brief Update the installation delay of all slots of a given aircraft.
 * @note hourly called
 * @sa CL_CampaignRun
 * @sa AII_UpdateOneInstallationDelay
 * @param[in] aircraft Pointer to the aircraft
 */
void AII_UpdateInstallationDelay (void)
{
	int i, j, k;
	base_t *base;
	aircraft_t *aircraft;

	for (j = 0, base = gd.bases; j < gd.numBases; j++, base++) {
		if (!base->founded)
			continue;

		/* Update base */
		for (k = 0; k < base->maxBatteries; k++)
			AII_UpdateOneInstallationDelay(base, NULL, base->batteries + k);
		for (k = 0; k < base->maxLasers; k++)
			AII_UpdateOneInstallationDelay(base, NULL, base->lasers + k);

		/* Update each aircraft */
		for (i = 0, aircraft = (aircraft_t *) base->aircraft; i < base->numAircraftInBase; i++, aircraft++)
			if (aircraft->homebase) {
				assert(aircraft->homebase == base);
				if (AIR_IsAircraftInBase(aircraft)) {
					/* Update electronics delay */
					for (k = 0; k < aircraft->maxElectronics; k++)
						AII_UpdateOneInstallationDelay(base, aircraft, aircraft->electronics + k);

					/* Update weapons delay */
					for (k = 0; k < aircraft->maxWeapons; k++)
						AII_UpdateOneInstallationDelay(base, aircraft, aircraft->weapons + k);

					/* Update shield delay */
					AII_UpdateOneInstallationDelay(base, aircraft, &aircraft->shield);
				}
			}
	}
}

/**
 * @brief Check that airequipSelectedSlot is the indice of an existing slot in the aircraft
 * @note airequipSelectedSlot concerns only weapons and electronics
 * @sa aircraft Pointer to the aircraft
 */
static void AIM_CheckAirequipSelectedSlot (const aircraft_t *aircraft)
{
	switch (airequipID) {
	case AC_ITEM_AMMO:
	case AC_ITEM_WEAPON:
		if (airequipSelectedSlot >= aircraft->maxWeapons) {
			airequipSelectedSlot = 0;
			return;
		}
		break;
	case AC_ITEM_ELECTRONICS:
		if (airequipSelectedSlot >= aircraft->maxElectronics) {
			airequipSelectedSlot = 0;
			return;
		}
		break;
	}
}

/**
 * @brief Draw only slots existing for this aircraft, and emphases selected one
 * @return[out] aircraft Pointer to the aircraft
 */
static void AIM_DrawAircraftSlots (const aircraft_t *aircraft)
{
	menuNode_t *node;
	int i, j;
	const aircraftSlot_t *slot;
	int max;

	/* initialise models Cvar */
	for (i = 0; i < 8; i++)
		Cvar_Set(va("mn_aircraft_item_model_slot%i", i), "");

	node = MN_GetNodeFromCurrentMenu("airequip_slot0");
	for (i = 0; node && i < AIR_POSITIONS_MAX; node = node->next) {
		if (!Q_strncmp(node->name, "airequip_slot", 13)) {
			/* Default value */
			MN_HideNode(node);
			/* Draw available slots */
			switch (airequipID) {
			case AC_ITEM_AMMO:
			case AC_ITEM_WEAPON:
				max = aircraft->maxWeapons;
				slot = aircraft->weapons;
				break;
			case AC_ITEM_ELECTRONICS:
				max = aircraft->maxElectronics;
				slot = aircraft->electronics;
				break;
			/* do nothing for shield: there is only one slot */
			default:
				continue;
			}
			for (j = 0; j < max; j++, slot++) {
				/* check if one of the aircraft slots is at this position */
				if (slot->pos == i) {
					MN_UnHideNode(node);
					/* draw in white if this is the selected slot */
					if (j == airequipSelectedSlot) {
						Vector2Set(node->texl, 64, 0);
						Vector2Set(node->texh, 128, 64);
					} else {
						Vector2Set(node->texl, 0, 0);
						Vector2Set(node->texh, 64, 64);
					}
					if (slot->item) {
						assert(slot->item->tech);
						Cvar_Set(va("mn_aircraft_item_model_slot%i", i), slot->item->tech->mdl_top);
					} else
						Cvar_Set(va("mn_aircraft_item_model_slot%i", i), "");
				}
			}
			i++;
		}
	}
}

/**
 * @brief Write in Red the text in zone ammo (zone 3)
 * @sa AIM_NoEmphazeAmmoSlotText
 * @note This is intended to show the player that there is no ammo in his aircraft
 */
static void AIM_EmphazeAmmoSlotText (void)
{
	menuNode_t *node;

	node = MN_GetNodeFromCurrentMenu("airequip_text_zone3");
	VectorSet(node->color, 1.0f, .0f, .0f);
}

/**
 * @brief Write in White the text in zone ammo (zone 3)
 * @sa AIM_EmphazeAmmoSlotText
 * @note This is intended to revert effects of AIM_EmphazeAmmoSlotText()
 */
static void AIM_NoEmphazeAmmoSlotText (void)
{
	menuNode_t *node;

	node = MN_GetNodeFromCurrentMenu("airequip_text_zone3");
	VectorSet(node->color, 1.0f, 1.0f, 1.0f);
}

/**
 * @brief Fills the weapon and shield list of the aircraft equip menu
 * @sa AIM_AircraftEquipMenuClick_f
 */
void AIM_AircraftEquipMenuUpdate_f (void)
{
	static char smallbuffer1[128];
	static char smallbuffer2[128];
	static char smallbuffer3[128];
	int type;
	menuNode_t *node;
	aircraft_t *aircraft;
	aircraftSlot_t *slot;

	if (!baseCurrent)
		return;

	/* don't let old links appear on this menu */
	MN_MenuTextReset(TEXT_STANDARD);
	MN_MenuTextReset(TEXT_AIREQUIP_1);
	MN_MenuTextReset(TEXT_AIREQUIP_2);
	MN_MenuTextReset(TEXT_AIREQUIP_3);
	MN_MenuTextReset(TEXT_LIST);

	if (Cmd_Argc() != 2 || noparams) {
		if (airequipID == -1) {
			Com_Printf("Usage: %s <num>\n", Cmd_Argv(0));
			return;
		}
		AIM_CheckAirequipID();
	} else {
		type = atoi(Cmd_Argv(1));
		switch (type) {
		case 1:
			/* shield/armour */
			airequipID = AC_ITEM_SHIELD;
			break;
		case 2:
			/* items */
			airequipID = AC_ITEM_ELECTRONICS;
			break;
		case 3:
			/* ammo - only available from weapons */
			if (airequipID == AC_ITEM_WEAPON)
				airequipID = AC_ITEM_AMMO;
			break;
		default:
			airequipID = AC_ITEM_WEAPON;
			break;
		}
	}

	/* Reset value of noparams */
	noparams = qfalse;

	node = MN_GetNodeFromCurrentMenu("aircraftequip");

	/* we are not in the aircraft menu */
	if (!node) {
		Com_DPrintf(DEBUG_CLIENT, "AIM_AircraftEquipMenuUpdate_f: Error - node aircraftequip not found\n");
		return;
	}

	aircraft = &baseCurrent->aircraft[baseCurrent->aircraftCurrent];

	assert(aircraft);
	assert(node);

	/* Check that airequipSelectedSlot corresponds to an existing slot for this aircraft */
	AIM_CheckAirequipSelectedSlot(aircraft);

	/* Select slot */
	slot = AII_SelectAircraftSlot(aircraft);

	/* Check that the selected zone is OK */
	AIM_CheckAirequipSelectedZone(slot);

	/* Fill the list of item you can equip your aircraft with */
	AIM_UpdateAircraftItemList(NULL, aircraft);

	/* Fill the texts of each zone */
	/* First slot: item currently assigned */
	if (!slot->item) {
		Com_sprintf(smallbuffer1, sizeof(smallbuffer1), _("No item assigned.\n"));
		Q_strcat(smallbuffer1, va(_("This slot is for %s or smaller items."), AII_WeightToName(slot->size)), sizeof(smallbuffer1));
	} else {
		Com_sprintf(smallbuffer1, sizeof(smallbuffer1), _(slot->item->tech->name));
		Q_strcat(smallbuffer1, "\n", sizeof(smallbuffer1));
		if (!slot->installationTime)
			Q_strcat(smallbuffer1, _("This item is functional.\n"), sizeof(smallbuffer1));
		else if (slot->installationTime > 0)
			Q_strcat(smallbuffer1, va(_("This item will be installed in %i hours.\n"),slot->installationTime), sizeof(smallbuffer1));
		else
			Q_strcat(smallbuffer1, va(_("This item will be removed in %i hours.\n"),-slot->installationTime), sizeof(smallbuffer1));
	}
	mn.menuText[TEXT_AIREQUIP_1] = smallbuffer1;

	/* Second slot: next item to install when the first one will be removed */
	if (slot->item && slot->installationTime < 0) {
		if (!slot->nextItem)
			Com_sprintf(smallbuffer2, sizeof(smallbuffer2), _("No item assigned."));
		else {
			Com_sprintf(smallbuffer2, sizeof(smallbuffer2), _(slot->nextItem->tech->name));
			Q_strcat(smallbuffer2, "\n", sizeof(smallbuffer2));
			Q_strcat(smallbuffer2, va(_("This item will be operational in %i hours.\n"), slot->nextItem->craftitem.installationTime - slot->installationTime), sizeof(smallbuffer2));
		}
	} else
		*smallbuffer2 = '\0';
	mn.menuText[TEXT_AIREQUIP_2] = smallbuffer2;

	/* Third slot: ammo slot (only used for weapons) */
	if ((airequipID == AC_ITEM_WEAPON || airequipID == AC_ITEM_AMMO) && slot->item) {
		if (!slot->ammo) {
			AIM_EmphazeAmmoSlotText();
			Com_sprintf(smallbuffer3, sizeof(smallbuffer3), _("No ammo assigned to this weapon."));
		} else {
			AIM_NoEmphazeAmmoSlotText();
			Com_sprintf(smallbuffer3, sizeof(smallbuffer3), _(slot->ammo->tech->name));
		}
	} else {
		*smallbuffer3 = '\0';
	}
	mn.menuText[TEXT_AIREQUIP_3] = smallbuffer3;

	/* Draw existing slots for this aircraft */
	AIM_DrawAircraftSlots(aircraft);

	/* Draw selected zone */
	AIM_DrawSelectedZone();
}

/**
 * @brief Select the current slot you want to assign the item to.
 * @note This function is only for aircraft and not far bases.
 */
void AIM_AircraftEquipSlotSelect_f (void)
{
	int i, pos;
	aircraft_t *aircraft;

	if (!baseCurrent)
		return;

	if (Cmd_Argc() < 2) {
		Com_Printf("Usage: %s <arg>\n", Cmd_Argv(0));
		return;
	}

	aircraft = &baseCurrent->aircraft[baseCurrent->aircraftCurrent];
	assert(aircraft);

	pos = atoi(Cmd_Argv(1));

	/* select the slot corresponding to pos, and set airequipSelectedSlot to this slot */
	switch (airequipID) {
	case AC_ITEM_ELECTRONICS:
		/* electronics selected */
		for (i = 0; i < aircraft->maxElectronics; i++) {
			if (aircraft->electronics[i].pos == pos) {
				airequipSelectedSlot = i;
				break;
			}
		}
		if (i == aircraft->maxElectronics)
			Com_Printf("this slot hasn't been found in aircraft electronics slots\n");
		break;
	case AC_ITEM_AMMO:
	case AC_ITEM_WEAPON:
		/* weapon selected */
		for (i = 0; i < aircraft->maxWeapons; i++) {
			if (aircraft->weapons[i].pos == pos) {
				airequipSelectedSlot = i;
				break;
			}
		}
		if (i == aircraft->maxWeapons)
			Com_Printf("this slot hasn't been found in aircraft weapon slots\n");
		break;
	default:
		Com_Printf("AIM_AircraftEquipSlotSelect_f : only weapons and electronics have several slots\n");
	}

	/* Update menu after changing slot */
	noparams = qtrue; /* used for AIM_AircraftEquipMenuUpdate_f */
	AIM_AircraftEquipMenuUpdate_f();
}

/**
 * @brief Select the current zone you want to assign the item to.
 */
void AIM_AircraftEquipZoneSelect_f (void)
{
	int zone;
	aircraft_t *aircraft;
	aircraftSlot_t *slot;
	const menu_t *activeMenu;
	qboolean aircraftMenu;

	if (!baseCurrent)
		return;

	if (Cmd_Argc() < 2) {
		Com_Printf("Usage: %s <arg>\n", Cmd_Argv(0));
		return;
	}

	/* select menu */
	activeMenu = MN_GetActiveMenu();
	aircraftMenu = !Q_strncmp(activeMenu->name, "aircraft_equip", 14);

	zone = atoi(Cmd_Argv(1));

	if (aircraftMenu) {
		assert(baseCurrent->aircraftCurrent >= 0);
		aircraft = &baseCurrent->aircraft[baseCurrent->aircraftCurrent];
		assert(aircraft);
		/* Select slot */
		slot = AII_SelectAircraftSlot(aircraft);
	} else {
		/* Select slot */
		slot = BDEF_SelectBaseSlot(baseCurrent);
		aircraft = NULL;
	}

	/* ammos are only available for weapons */
	switch (airequipID) {
	/* a weapon was selected - select ammo type corresponding to this weapon */
	case AC_ITEM_WEAPON:
		if (zone == ZONE_AMMO) {
			if (slot->item)
				airequipID = AC_ITEM_AMMO;
		}
		break;
	case AC_ITEM_BASE_MISSILE:
		if (zone == ZONE_AMMO) {
			if (slot->item)
				airequipID = AC_ITEM_AMMO_MISSILE;
		}
		break;
	case AC_ITEM_BASE_LASER:
		if (zone == ZONE_AMMO) {
			if (slot->item)
				airequipID = AC_ITEM_AMMO_LASER;
		}
		break;
	/* an ammo was selected - select weapon type corresponding to this ammo */
	case AC_ITEM_AMMO:
		if (zone != ZONE_AMMO)
			airequipID = AC_ITEM_WEAPON;
		break;
	case AC_ITEM_AMMO_MISSILE:
		if (zone != ZONE_AMMO)
			airequipID = AC_ITEM_BASE_MISSILE;
		break;
	case AC_ITEM_AMMO_LASER:
		if (zone != ZONE_AMMO)
			airequipID = AC_ITEM_BASE_LASER;
		break;
	default :
		/* ZONE_AMMO is not available for electronics and shields */
		if (zone == ZONE_AMMO)
			return;
	}
	airequipSelectedZone = zone;

	/* Fill the list of item you can equip your aircraft with */
	AIM_UpdateAircraftItemList(aircraftMenu ? NULL : baseCurrent, aircraft);

	/* Check that the selected zone is OK */
	AIM_CheckAirequipSelectedZone(slot);

	/* Draw selected zone */
	AIM_DrawSelectedZone();
}

/**
 * @brief Auto add ammo corresponding to weapon in main zone, if there is enough in storage.
 * @param[in] base Pointer to the base where you want to add/remove ammo
 * @param[in] aircraft Pointer to the aircraft (NULL if we are changing base defense system)
 * @param[in] slot Pointer to the slot where you want to add ammo
 * @sa AIM_AircraftEquipAddItem_f
 * @sa AII_RemoveItemFromSlot
 */
static void AIM_AutoAddAmmo (base_t *base, aircraft_t *aircraft, aircraftSlot_t *slot)
{
	int k;
	technology_t *ammo_tech;
	objDef_t *ammo;
	objDef_t *item;

	assert(slot);

	/* Get the weapon */
	item = slot->item;

	if (!item)
		return;

	if (item->craftitem.type > AC_ITEM_WEAPON)
		return;

	/* don't try to add ammo to a slot that already has ammo */
	if (slot->ammo)
		return;

	/* Try every ammo usable with this weapon until we find one we have in storage */
	for (k = 0; k < item->numAmmos; k++) {
		ammo = item->ammos[k];
		if (ammo) {
			ammo_tech = ammo->tech;
			if (ammo_tech && AIM_SelectableAircraftItem(base, aircraft, ammo_tech)) {
				AII_AddAmmoToSlot(ammo->notOnMarket ? NULL : base, ammo_tech, slot);
				/* base missile are free, you have 20 when you build a new base defense */
				if (item->craftitem.type == AC_ITEM_BASE_MISSILE && (slot->ammoLeft < 0)) {
					/* we use < 0 here, and not <= 0, because we give missiles only on first build
					 * (not when player removes base defense and re-add it)
					 * sa AII_InitialiseSlot: ammoLeft is initialized to -1 */
					slot->ammoLeft = 20;
				} else if (aircraft)
					AII_ReloadWeapon(aircraft);
				break;
			}
		}
	}
}

/**
 * @brief Move the item in the slot (or optionally its ammo only) to the base storage.
 * @note if there is another item to install after removal, begin this installation.
 * @param[in] base The base to add the item to (may be NULL if item shouldn't be removed from any base).
 * @param[in] slot The slot to remove the item from.
 * @param[in] ammo qtrue if we want to remove only ammo. qfalse if the whole item should be removed.
 * @sa AII_AddItemToSlot
 * @sa AII_AddAmmoToSlot
 */
void AII_RemoveItemFromSlot (base_t* base, aircraftSlot_t *slot, qboolean ammo)
{
	assert(slot);

	if (ammo) {
		/* only remove the ammo */
		if (slot->ammo) {
			if (base)
				B_UpdateStorageAndCapacity(base, slot->ammo->idx, 1, qfalse, qfalse);
			slot->ammo = NULL;
		}
	} else if (slot->item) {
		if (base)
			B_UpdateStorageAndCapacity(base, slot->item->idx, 1, qfalse, qfalse);
		/* the removal is over */
		if (slot->nextItem) {
			/* there is anoter item to install after this one */
			slot->item = slot->nextItem;
			if (base) {
				/* remove next item from storage (maybe we don't have anymore item ?) */
				if (B_UpdateStorageAndCapacity(base, slot->nextItem->idx, -1, qfalse, qfalse)) {
					slot->item = slot->nextItem;
					slot->installationTime = slot->item->craftitem.installationTime;
				} else {
					slot->item = NULL;
					slot->installationTime = 0;
				}
			} else {
				slot->item = slot->nextItem;
				slot->installationTime = slot->item->craftitem.installationTime;
			}
			slot->nextItem = NULL;
		} else {
			slot->item = NULL;
			slot->installationTime = 0;
		}
		/* also remove ammo */
		if (slot->ammo) {
			if (base)
				B_UpdateStorageAndCapacity(base, slot->ammo->idx, 1, qfalse, qfalse);
			slot->ammo = NULL;
		}
	}
}

/**
 * @brief Add an ammo to an aircraft weapon slot
 * @note No check for the _type_ of item is done here, so it must be done before.
 * @param[in] base Pointer to the base which provides items (NULL if items shouldn't be removed of storage)
 * @param[in] tech Pointer to the tech to add to slot
 * @param[in] slot Pointer to the slot where you want to add ammos
 * @sa AII_AddItemToSlot
 */
qboolean AII_AddAmmoToSlot (base_t* base, const technology_t *tech, aircraftSlot_t *slot)
{
	objDef_t *ammo;

	assert(slot);
	assert(tech);

	ammo = AII_GetAircraftItemByID(tech->provides);
	if (!ammo) {
		Com_Printf("AII_AddAmmoToSlot: Could not add item (%s) to slot\n", tech->provides);
		return qfalse;
	}

	/* the base pointer can be null here - e.g. in case you are equipping a UFO
	 * and base ammo defense are not stored in storage */
	if (base && ammo->craftitem.type <= AC_ITEM_AMMO) {
		if (base->storage.num[ammo->idx] <= 0) {
			Com_Printf("AII_AddAmmoToSlot: No more ammo of this type to equip (%s)\n", ammo->id);
			return qfalse;
		}
	}

	/* remove any applied ammo in the current slot */
	AII_RemoveItemFromSlot(base, slot, qtrue);
	slot->ammo = ammo;

	/* the base pointer can be null here - e.g. in case you are equipping a UFO */
	if (base && ammo->craftitem.type <= AC_ITEM_AMMO)
		B_UpdateStorageAndCapacity(base, ammo->idx, -1, qfalse, qfalse);

	return qtrue;
}

/**
 * @brief Add an item to an aircraft slot
 * @note No check for the _type_ of item is done here.
 * @sa AII_UpdateOneInstallationDelay
 * @sa AII_AddAmmoToSlot
 */
qboolean AII_AddItemToSlot (base_t* base, const technology_t *tech, aircraftSlot_t *slot)
{
	objDef_t *item;

	assert(slot);
	assert(tech);

	item = AII_GetAircraftItemByID(tech->provides);
	if (!item) {
		Com_Printf("AII_AddItemToSlot: Could not add item (%s) to slot\n", tech->provides);
		return qfalse;
	}

	/* Sanity check : the type of the item should be the same than the slot type */
	if (slot->type != item->craftitem.type) {
		Com_Printf("AII_AddItemToSlot: Type of the item to install (%s -- %i) doesn't match type of the slot (%i)\n", item->id, item->craftitem.type, slot->type);
		return qfalse;
	}

#ifdef DEBUG
	/* Sanity check : the type of the item cannot be an ammo */
	/* note that this should never be reached because a slot type should never be an ammo
	 * , so the test just before should be wrong */
	if (item->craftitem.type >= AC_ITEM_AMMO) {
		Com_Printf("AII_AddItemToSlot: Type of the item to install (%s) should be a weapon, a shield, or electronics (no ammo)\n", item->id);
		return qfalse;
	}
#endif

	/* the base pointer can be null here - e.g. in case you are equipping a UFO */
	if (base) {
		if (base->storage.num[item->idx] <= 0) {
			Com_Printf("AII_AddItemToSlot: No more item of this type to equip (%s)\n", item->id);
			return qfalse;
		}
	}

	if (slot->size >= AII_GetItemWeightBySize(item)) {
		slot->item = item;
		slot->installationTime = item->craftitem.installationTime;
		/* the base pointer can be null here - e.g. in case you are equipping a UFO */
		if (base)
			B_UpdateStorageAndCapacity(base, item->idx, -1, qfalse, qfalse);
	} else {
		Com_Printf("AII_AddItemToSlot: Could not add item '%s' to slot %i (slot-size: %i - item-weight: %i)\n",
			item->id, slot->idx, slot->size, item->size);
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Add selected item to current zone.
 * @note May be called from airequip menu or basedefense menu
 * @sa aircraftItemType_t
 */
void AIM_AircraftEquipAddItem_f (void)
{
	int zone;
	aircraftSlot_t *slot;
	aircraft_t *aircraft;
	const menu_t *activeMenu;
	qboolean aircraftMenu;
	base_t* base;

	if (!baseCurrent)
		return;

	if (Cmd_Argc() < 2) {
		Com_Printf("Usage: %s <arg>\n", Cmd_Argv(0));
		return;
	}

	zone = atoi(Cmd_Argv(1));

	/* select menu */
	activeMenu = MN_GetActiveMenu();
	aircraftMenu = !Q_strncmp(activeMenu->name, "aircraft_equip", 14);

	/* proceed only if an item has been selected */
	if (!airequipSelectedTechnology)
		return;

	/* check in which menu we are */
	if (aircraftMenu) {
		aircraft = &baseCurrent->aircraft[baseCurrent->aircraftCurrent];
		assert(aircraft);
		base = aircraft->homebase;
		slot = AII_SelectAircraftSlot(aircraft);
	} else {
		slot = BDEF_SelectBaseSlot(baseCurrent);
		base = baseCurrent;
		aircraft = NULL;
	}

	/* the clicked button doesn't correspond to the selected zone */
	if (zone != airequipSelectedZone)
		return;

	/* there's no item in ZONE_MAIN: you can't use zone ZONE_NEXT */
	if (zone == ZONE_NEXT && !slot->item)
		return;

	/* check if the zone exists */
	if (zone >= ZONE_MAX)
		return;

	/* select the slot we are changing */
	/* update the new item to slot */

	switch (zone) {
	case ZONE_MAIN:
		/* we add the weapon, shield, item, or base defense if slot is free or the installation of
			current item just began */
		if (!slot->item || slot->installationTime == slot->item->craftitem.installationTime) {
			AII_RemoveItemFromSlot(base, slot, qfalse);
			AII_AddItemToSlot(base, airequipSelectedTechnology, slot); /* Aircraft stats are updated below */
			AIM_AutoAddAmmo(base, aircraft, slot);
			break;
		} else if (slot->item == AII_GetAircraftItemByID(airequipSelectedTechnology->provides) &&
			slot->installationTime == -slot->item->craftitem.installationTime) {
			/* player changed he's mind: he just want to readd the item he just removed */
			slot->installationTime = 0;
			slot->nextItem = NULL;
			AIM_AutoAddAmmo(base, aircraft, slot);
		} else {
			AII_RemoveItemFromSlot(base, slot, qtrue); /* remove ammo */
			/* We start removing current item in slot, and the selected item will be installed afterwards */
			slot->installationTime = -slot->item->craftitem.installationTime;
			/* more below (don't use break) */
			}
	case ZONE_NEXT:
		/* we change the weapon, shield, item, or base defense that will be installed AFTER the removal
		 * of the one in the slot atm */
		slot->nextItem = AII_GetAircraftItemByID(airequipSelectedTechnology->provides);
		/* do not remove item from storage now, this will be done in AII_RemoveItemFromSlot */
		break;
	case ZONE_AMMO:
		/* we can change ammo only if the selected item is an ammo (for weapon or base defense system) */
		if (airequipID >= AC_ITEM_AMMO) {
			AII_AddAmmoToSlot(base, airequipSelectedTechnology, slot);
			/* reload its ammunition */
			if (aircraft)
				AII_ReloadWeapon(aircraft);
		}
		break;
	default:
		/* Zone higher than ZONE_AMMO shouldn't exist */
		return;
	}

	if (aircraftMenu) {
		/* Update the values of aircraft stats (just in case an item has an installationTime of 0) */
		AII_UpdateAircraftStats(aircraft);

		noparams = qtrue; /* used for AIM_AircraftEquipMenuUpdate_f */
		AIM_AircraftEquipMenuUpdate_f();
	} else {
		noparams = qtrue; /* used for BDEF_BaseDefenseMenuUpdate_f */
		BDEF_BaseDefenseMenuUpdate_f();
	}
}

/**
 * @brief Delete an object from a zone.
 */
void AIM_AircraftEquipDeleteItem_f (void)
{
	int zone;
	aircraftSlot_t *slot;
	aircraft_t *aircraft;
	const menu_t *activeMenu;
	qboolean aircraftMenu;

	if (!baseCurrent)
		return;

	if (Cmd_Argc() < 2) {
		Com_Printf("Usage: %s <arg>\n", Cmd_Argv(0));
		return;
	}
	zone = atoi(Cmd_Argv(1));

	/* select menu */
	activeMenu = MN_GetActiveMenu();
	aircraftMenu = !Q_strncmp(activeMenu->name, "aircraft_equip", 14);

	/* check in which menu we are */
	if (aircraftMenu) {
		aircraft = &baseCurrent->aircraft[baseCurrent->aircraftCurrent];
		slot = AII_SelectAircraftSlot(aircraft);
	} else {
		slot = BDEF_SelectBaseSlot(baseCurrent);
		aircraft = NULL;
	}

	/* select the slot we are changing */
	/* update the new item to slot */

	switch (zone) {
	case ZONE_MAIN:
		/* we change the weapon, shield, item, or base defense that is already in the slot */
		/* if the item has been installed since less than 1 hour, you don't need time to remove it */
		if (slot->installationTime < slot->item->craftitem.installationTime) {
			slot->installationTime = -slot->item->craftitem.installationTime;
			AII_RemoveItemFromSlot(baseCurrent, slot, qtrue); /* we remove only ammo, not item */
		} else {
			AII_RemoveItemFromSlot(baseCurrent, slot, qfalse);
		}
		/* aircraft stats are updated below */
		break;
	case ZONE_NEXT:
		/* we change the weapon, shield, item, or base defense that will be installed AFTER the removal
		 * of the one in the slot atm */
		slot->nextItem = NULL;
		break;
	case ZONE_AMMO:
		/* we can change ammo only if the selected item is an ammo (for weapon or base defense system) */
		if (airequipID >= AC_ITEM_AMMO)
			AII_RemoveItemFromSlot(baseCurrent, slot, qtrue);
		break;
	default:
		/* Zone higher than ZONE_AMMO shouldn't exist */
		return;
	}

	if (aircraftMenu) {
		/* Update the values of aircraft stats */
		AII_UpdateAircraftStats(aircraft);

		noparams = qtrue; /* used for AIM_AircraftEquipMenuUpdate_f */
		AIM_AircraftEquipMenuUpdate_f();
	} else {
		noparams = qtrue; /* used for BDEF_MenuUpdate_f */
		BDEF_BaseDefenseMenuUpdate_f();
	}
}

/**
 * @brief Set airequipSelectedTechnology to the technology of current selected aircraft item.
 * @sa AIM_AircraftEquipMenuUpdate_f
 */
void AIM_AircraftEquipMenuClick_f (void)
{
	aircraft_t *aircraft;
	base_t *base;
	int num;
	technology_t **list;
	const menu_t *activeMenu;

	if (!baseCurrent || airequipID == -1)
		return;

	if (Cmd_Argc() < 2) {
		Com_Printf("Usage: %s <num>\n", Cmd_Argv(0));
		return;
	}

	/* select menu */
	activeMenu = MN_GetActiveMenu();
	/* check in which menu we are */
	if (!Q_strncmp(activeMenu->name, "aircraft_equip", 14)) {
		if (baseCurrent->aircraftCurrent < 0)
			return;
		aircraft = &baseCurrent->aircraft[baseCurrent->aircraftCurrent];
		base = NULL;
	} else if (!Q_strncmp(activeMenu->name, "basedefense", 11)) {
		base = baseCurrent;
		aircraft = NULL;
	} else
		return;

	/* Which entry in the list? */
	num = atoi(Cmd_Argv(1));

	/* make sure that airequipSelectedTechnology is NULL if no tech is found */
	airequipSelectedTechnology = NULL;

	/* build the list of all aircraft items of type airequipID - null terminated */
	list = AII_GetCraftitemTechsByType(airequipID, base ? base : baseCurrent);
	/* to prevent overflows we go through the list instead of address it directly */
	while (*list) {
		if (AIM_SelectableAircraftItem(base, aircraft, *list)) {
			/* found it */
			if (num <= 0) {
				airequipSelectedTechnology = *list;
				UP_AircraftItemDescription(AII_GetAircraftItemByID(airequipSelectedTechnology->provides)->idx);
				break;
			}
			num--;
		}
		/* next item in the tech pointer list */
		list++;
	}
}

/**
 * @brief Auto Add weapon and ammo to an aircraft.
 * @param[in] aircraft Pointer to the aircraft
 * @note This is used to auto equip interceptor of first base
 * @sa B_SetUpBase
 */
void AIM_AutoEquipAircraft (aircraft_t *aircraft)
{
	int i;
	aircraftSlot_t *slot;
	objDef_t *item;
	const technology_t *tech = RS_GetTechByID("rs_craft_weapon_sparrowhawk");

	if (!tech)
		Sys_Error("Could not get tech rs_craft_weapon_sparrowhawk");

	assert(aircraft);
	assert(aircraft->homebase);

	airequipID = AC_ITEM_WEAPON;

	item = AII_GetAircraftItemByID(tech->provides);
	if (!item)
		return;

	for (i = 0; i < aircraft->maxWeapons; i++) {
		slot = &aircraft->weapons[i];
		if (slot->size < AII_GetItemWeightBySize(item))
			continue;
		if (aircraft->homebase->storage.num[item->idx] <= 0)
			continue;
		AII_AddItemToSlot(aircraft->homebase, tech, slot);
		AIM_AutoAddAmmo(aircraft->homebase, aircraft, slot);
		slot->installationTime = 0;
	}

	AII_UpdateAircraftStats(aircraft);
}

/**
 * @brief Initialise values of one slot of an aircraft or basedefense common to all types of items.
 * @param[in] slot Pointer to the slot to initialize
 * @param[in] aircraft	Pointer to aircraft.
 * @param[in] base	Pointer to base.
 * @param[in] type
 */
void AII_InitialiseSlot (aircraftSlot_t *slot, aircraft_t *aircraft, base_t *base, aircraftItemType_t type)
{
	assert ((!base && aircraft) ||(base && !aircraft));	/* Only one of them is allowed. */

	memset(slot, 0, sizeof(slot)); /* all values to 0 */
	slot->aircraft = aircraft;
	slot->base = base;
	slot->item = NULL;
	slot->ammo = NULL;
	slot->size = ITEM_HEAVY;
	slot->nextItem = NULL;
	slot->type = type;
	slot->ammoLeft = -1; /** sa BDEF_AddBattery: it needs to be -1 and not 0 @sa B_SaveItemSlots */
}

/**
 * @brief Check if item in given slot should change one aircraft stat.
 * @param[in] slot Pointer to the slot containing the item
 * @param[in] stat the stat that should be checked
 * @return qtrue if the item should change the stat.
 */
static qboolean AII_CheckUpdateAircraftStats (const aircraftSlot_t *slot, int stat)
{
	const objDef_t *item;

	assert(slot);

	/* there's no item */
	if (!slot->item)
		return qfalse;

	/* you can not have advantages from items if it is being installed or removed, but only disavantages */
	if (slot->installationTime != 0) {
		item = slot->item;
		if (item->craftitem.stats[stat] > 1.0f) /* advandages for relative and absolute values */
			return qfalse;
	}

	return qtrue;
}

/**
 * @brief Update the value of stats array of an aircraft.
 * @param[in] aircraft Pointer to the aircraft
 * @note This should be called when an item starts to be added/removed and when addition/removal is over.
 */
void AII_UpdateAircraftStats (aircraft_t *aircraft)
{
	int i, currentStat;
	const aircraft_t *source;
	const objDef_t *item;

	assert(aircraft);

	source = &aircraft_samples[aircraft->idx_sample];

	/* we scan all the stats except AIR_STATS_WRANGE (see below) */
	for (currentStat = 0; currentStat < AIR_STATS_MAX - 1; currentStat++) {
		/* initialise value */
		aircraft->stats[currentStat] = source->stats[currentStat];

		/* modify by electronics (do nothing if the value of stat is 0) */
		for (i = 0; i < aircraft->maxElectronics; i++) {
			if (!AII_CheckUpdateAircraftStats (&aircraft->electronics[i], currentStat))
				continue;
			item = aircraft->electronics[i].item;
			if (fabs(item->craftitem.stats[currentStat]) > 2.0f)
				aircraft->stats[currentStat] += (int) item->craftitem.stats[currentStat];
			else if (item->craftitem.stats[currentStat] > UFO_EPSILON)
				aircraft->stats[currentStat] *= item->craftitem.stats[currentStat];
		}

		/* modify by weapons (do nothing if the value of stat is 0)
		 * note that stats are not modified by ammos */
		for (i = 0; i < aircraft->maxWeapons; i++) {
			if (!AII_CheckUpdateAircraftStats (&aircraft->weapons[i], currentStat))
				continue;
			item = aircraft->weapons[i].item;
			if (fabs(item->craftitem.stats[currentStat]) > 2.0f)
				aircraft->stats[currentStat] += item->craftitem.stats[currentStat];
			else if (item->craftitem.stats[currentStat] > UFO_EPSILON)
				aircraft->stats[currentStat] *= item->craftitem.stats[currentStat];
		}

		/* modify by shield (do nothing if the value of stat is 0) */
		if (AII_CheckUpdateAircraftStats (&aircraft->shield, currentStat)) {
			item = aircraft->shield.item;
			if (fabs(item->craftitem.stats[currentStat]) > 2.0f)
				aircraft->stats[currentStat] += item->craftitem.stats[currentStat];
			else if (item->craftitem.stats[currentStat] > UFO_EPSILON)
				aircraft->stats[currentStat] *= item->craftitem.stats[currentStat];
		}
	}

	/* now we update AIR_STATS_WRANGE (this one is the biggest value of every ammo) */
	aircraft->stats[AIR_STATS_WRANGE] = 0;
	for (i = 0; i < aircraft->maxWeapons; i++) {
		if (!AII_CheckUpdateAircraftStats (&aircraft->weapons[i], AIR_STATS_WRANGE))
			continue;
		item = aircraft->weapons[i].ammo;
		if (item && item->craftitem.stats[AIR_STATS_WRANGE] > aircraft->stats[AIR_STATS_WRANGE])	/***@todo Check if the "ammo" is _supposed_ to be set here! */
			aircraft->stats[AIR_STATS_WRANGE] = item->craftitem.stats[AIR_STATS_WRANGE];
	}

	/* check that iracraft hasn't too much fuel (caused by removal of fuel pod) */
	if (aircraft->fuel > aircraft->stats[AIR_STATS_FUELSIZE])
		aircraft->fuel = aircraft->stats[AIR_STATS_FUELSIZE];

	/* check that speed of the aircraft is positive */
	if (aircraft->stats[AIR_STATS_SPEED] < 1)
		aircraft->stats[AIR_STATS_SPEED] = 1;

	/* Update aircraft state if needed */
	if (aircraft->status == AIR_HOME && aircraft->fuel < aircraft->stats[AIR_STATS_FUELSIZE])
		aircraft->status = AIR_REFUEL;
}

/**
 * @brief Returns the amount of assigned items for a given slot of a given aircraft
 * @param[in] type This is the slot type to get the amount of assigned items for
 * @param[in] aircraft The aircraft to count the items for (may not be NULL)
 * @return The amount of assigned items for the given slot
 */
int AII_GetSlotItems (aircraftItemType_t type, const aircraft_t *aircraft)
{
	int i, max, cnt = 0;
	const aircraftSlot_t *slot;

	assert(aircraft);

	switch (type) {
	case AC_ITEM_SHIELD:
		if (aircraft->shield.item)
			return 1;
		else
			return 0;
		break;
	case AC_ITEM_WEAPON:
		slot = aircraft->weapons;
		max = MAX_AIRCRAFTSLOT;
		break;
	case AC_ITEM_ELECTRONICS:
		slot = aircraft->electronics;
		max = MAX_AIRCRAFTSLOT;
		break;
	default:
		Com_Printf("AIR_GetSlotItems: Unknow type of slot : %i", type);
		return 0;
	}

	for (i = 0; i < max; i++)
		if (slot[i].item)
			cnt++;

	return cnt;
}

/**
 * @brief Check if the aircraft has weapon and ammo
 * @param[in] aircraft The aircraft to count the items for (may not be NULL)
 * @return qtrue if the aircraft can fight, qflase else
 * @sa AII_BaseCanShoot
 */
int AII_AircraftCanShoot (const aircraft_t *aircraft)
{
	int i;

	assert(aircraft);

	for (i = 0; i < aircraft->maxWeapons; i++)
		if (aircraft->weapons[i].item
		 && aircraft->weapons[i].ammo && aircraft->weapons[i].ammoLeft > 0)
			return qtrue;

	return qfalse;
}

/**
 * @brief Check if the base has weapon and ammo
 * @param[in] base Pointer to the base you want to check (may not be NULL)
 * @return qtrue if the base can shoot, qflase else
 * @sa AII_AircraftCanShoot
 */
int AII_BaseCanShoot (const base_t *base)
{
	int i;

	assert(base);

	if (base->hasBuilding[B_DEFENSE_MISSILE]) {
	/* base has missile battery and any needed building */
		for (i = 0; i < base->maxBatteries; i++)
			if (base->batteries[i].item
			 && base->batteries[i].ammo && base->batteries[i].ammoLeft > 0
			 && base->batteries[i].installationTime == 0)
				return qtrue;
	}

	if (base->hasBuilding[B_DEFENSE_LASER]) {
	/* base has laser battery and any needed building */
		for (i = 0; i < base->maxLasers; i++)
			if (base->lasers[i].item
			 && base->lasers[i].ammo && base->lasers[i].ammoLeft > 0
			 && base->lasers[i].installationTime == 0)
				return qtrue;
	}

	return qfalse;
}

/**
 * @brief Translate a weight int to a translated string
 * @sa itemWeight_t
 * @sa AII_GetItemWeightBySize
 */
const char* AII_WeightToName (itemWeight_t weight)
{
	switch (weight) {
	case ITEM_LIGHT:
		return _("Light weight");
		break;
	case ITEM_MEDIUM:
		return _("Medium weight");
		break;
	case ITEM_HEAVY:
		return _("Heavy weight");
		break;
	default:
		return _("Unknown weight");
		break;
	}
}
