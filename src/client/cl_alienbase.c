/**
 * @file cl_alienbase.c
 * @brief
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
#include "cl_alienbase.h"
#include "cl_map.h"

static alienBase_t alienBases[MAX_ALIEN_BASES];		/**< Alien bases spawned in game */
static int numAlienBases;							/**< Number of alien bases in game */

/**
 * @brief Reset global array and number of base
 * @note Use me when stating a new game
 */
void AB_ResetAlienBases (void)
{
	memset(&alienBases[MAX_ALIEN_BASES], 0, sizeof(alienBases[MAX_ALIEN_BASES]));
	numAlienBases = 0;
}

/**
 * @brief Build a new alien base
 * @param[in] pos Position of the new base.
 * @return Pointer to the base that has been built.
 */
alienBase_t* AB_BuildBase (vec2_t pos)
{
	alienBase_t *base;
	const float initialStealthValue = 50.0f;				/**< How hard PHALANX will find the base */

	if (numAlienBases >= MAX_ALIEN_BASES) {
		Com_Printf("AB_BuildBase: Too many alien bases build\n");
		return NULL;
	}

	base = &alienBases[numAlienBases];
	memset(base, 0, sizeof(base));

	Vector2Copy(pos, base->pos);
	base->stealth = initialStealthValue;
	base->idx = numAlienBases++;

	return base;
}

/**
 * @brief Destroy an alien base.
 * @param[in] base Pointer to the alien base.
 */
void AB_DestroyBase (alienBase_t *base)
{
	int i;
	int idx;

	assert(base);

	idx = base->idx;
	numAlienBases--;
	assert(numAlienBases >= idx);
	memmove(base, base + 1, (numAlienBases - idx) * sizeof(*base));
	/* wipe the now vacant last slot */
	memset(&alienBases[numAlienBases], 0, sizeof(alienBases[numAlienBases]));

	for (i = idx; i < numAlienBases; i++)
		alienBases[i].idx--;

	/* Alien loose all their interest in supply if there's no base to send the supply */
	if (!numAlienBases)
		ccs.interest[INTERESTCATEGORY_SUPPLY] = 0;
}

/**
 * @brief Get Alien Base per Idx.
 * @param[in] baseIDX IDX of the alien Base in alienBases[].
 * @param[in] checkIdx True if you want to check if baseIdx is lower than number of base.
 * @return Pointer to the base.
 */
alienBase_t* AB_GetBase (int baseIDX, qboolean checkIdx)
{
	if (baseIDX < 0 || baseIDX >= MAX_ALIEN_BASES)
		return NULL;

	if (checkIdx && baseIDX >= numAlienBases)
		return NULL;

	return &alienBases[baseIDX];
}

/**
 * @brief Update stealth value of every base after aircraft moved.
 * @param[in] aircraft Pointer to the aircraft_t.
 * @param[in] dt Elapsed time since last check.
 * @param[in] base Pointer to the alien base.
 * @note base stealth decreases if it is inside an aircraft radar range, and even more if it's
 * 	inside @c radarratio times radar range.
 * @sa UFO_UpdateAlienInterestForOneBase
 */
static void AB_UpdateStealthForOneBase (const aircraft_t *aircraft, int dt, alienBase_t *base)
{
	float distance;
	float probability = 0.0001f;			/**< base probability, will be modified below */
	const float radarratio = 0.4f;			/**< stealth decreases faster if base is inside radarratio times radar range */
	const float decreasingFactor = 5.0f;	/**< factor applied when outside @c radarratio times radar range */

	/* base is already discovered */
	if (base->stealth < 0)
		return;

	/* aircraft can't find base if it's too far */
	distance = MAP_GetDistance(aircraft->pos, base->pos);
	if (distance > aircraft->radar.range)
		return;

	/* the bigger the base, the higher the probability to find it */
	probability *= base->supply;

	/* decrease probability if the base is far from aircraft */
	if (distance > aircraft->radar.range * radarratio)
		probability /= decreasingFactor;

	/* probability must depend on time scale */
	probability *= dt;

	base->stealth -= probability;

	/* base discovered ? */
	if (base->stealth < 0) {
		base->stealth = -10.0f;		/* just to avoid rounding errors */
		CP_SpawnAlienBaseMission(base);
	}
}

/**
 * @brief Update stealth value of every base after aircraft moved
 * @param[in] aircraft Pointer to the aircraft_t
 * @param[in] dt Elapsed time since last check
 * @param[in] base Pointer to the base
 * @sa UFO_UpdateAlienInterestForOneBase
 */
void AB_UpdateStealthForAllBase (const aircraft_t *aircraft, int dt)
{
	alienBase_t* base;

	assert(aircraft);

	for (base = alienBases; base < alienBases + numAlienBases; base++) {
		AB_UpdateStealthForOneBase(aircraft, dt, base);
	}
}

/**
 * @brief Check if a supply mission is possible.
 * @return True if there is at least one base to supply.
 */
qboolean AB_CheckSupplyMissionPossible (void)
{
	return numAlienBases;
}

/**
 * @brief Choose Alien Base that should be supplied.
 * @param[out] pos Position of the base.
 * @return Pointer to the base.
 */
alienBase_t* AB_ChooseBaseToSupply (vec2_t pos)
{
	const int baseIDX = rand() % numAlienBases;

	Vector2Copy(alienBases[baseIDX].pos, pos);

	return &alienBases[baseIDX];
}

/**
 * @brief Supply a base.
 * @param[in] base Pointer to the supplied base.
 */
void AB_SupplyBase (alienBase_t *base, qboolean decreaseStealth)
{
	const float decreasedStealthValue = 5.0f;				/**< How much stealth is reduced because Supply UFO was seen */

	assert(base);

	base->supply++;
	if (decreaseStealth && base->stealth >= 0.0f)
		base->stealth -= decreasedStealthValue;
}

#ifdef DEBUG
/**
 * @brief Print Alien Bases information to game console
 */
static void AB_AlienBaseDiscovered_f (void)
{
	int i;

	for (i = 0; i < numAlienBases; i++) {
		alienBases[i].stealth = -10.0f;
		CP_SpawnAlienBaseMission(&alienBases[i]);
	}
}

/**
 * @brief Print Alien Bases information to game console
 */
static void AB_AlienBaseList_f (void)
{
	int i;

	if (numAlienBases == 0) {
		Com_Printf("No alien base founded\n");
		return;
	}

	for (i = 0; i < numAlienBases; i++) {
		Com_Printf("Alien Base: %i\n", i);
		if (i != alienBases[i].idx)
			Com_Printf("Warning: bad idx (%i instead of %i)\n", alienBases[i].idx, i);
		Com_Printf("...pos: (%f, %f)\n", alienBases[i].pos[0], alienBases[i].pos[1]);
		Com_Printf("...supply: %i\n", alienBases[i].supply);
		if (alienBases[i].stealth < 0)
			Com_Printf("...base discovered\n");
		else
			Com_Printf("...stealth: %f\n", alienBases[i].stealth);
	}
}
#endif

/**
 * @sa MN_ResetMenus
 */
void AB_Reset (void)
{
#ifdef DEBUG
	Cmd_AddCommand("debug_alienbaselist", AB_AlienBaseList_f, "Print Alien Bases information to game console");
	Cmd_AddCommand("debug_alienbasevisible", AB_AlienBaseDiscovered_f, "Set all alien bases to discovered");
#endif
}

/**
 * @brief Load callback for alien base data
 * @sa AB_Save
 */
qboolean AB_Load (sizebuf_t *sb, void *data)
{
	int i;

	numAlienBases = MSG_ReadShort(sb);
	for (i = 0; i < presaveArray[PRE_MAXALB]; i++) {
		alienBase_t *base = &alienBases[i];
		base->idx = MSG_ReadShort(sb);
		MSG_Read2Pos(sb, base->pos);
		base->supply = MSG_ReadShort(sb);
		base->stealth = MSG_ReadFloat(sb);
	}
	return qtrue;
}

/**
 * @brief Save callback for alien base data
 * @sa AB_Load
 */
qboolean AB_Save (sizebuf_t *sb, void *data)
{
	int i;

	MSG_WriteShort(sb, numAlienBases);
	for (i = 0; i < presaveArray[PRE_MAXALB]; i++) {
		const alienBase_t *base = &alienBases[i];
		MSG_WriteShort(sb, base->idx);
		MSG_Write2Pos(sb, base->pos);
		MSG_WriteShort(sb, base->supply);
		MSG_WriteFloat(sb, base->stealth);
	}

	return qtrue;
}
