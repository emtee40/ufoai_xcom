/**
 * @file g_local.h
 * @brief Local definitions for game module
 */

/*
All original materal Copyright (C) 2002-2007 UFO: Alien Invasion team.

Original file from Quake 2 v3.21: quake2-2.31/game/g_local.h
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

#ifndef GAME_G_LOCAL_H
#define GAME_G_LOCAL_H

#include "q_shared.h"
#include "inv_shared.h"
#include "../shared/infostring.h"

/** @todo no gettext support for game lib - but we must be able to mark the strings */
# define _(String) String
# define ngettext(x, y, cnt) x

/* define GAME_INCLUDE so that game.h does not define the */
/* short, server-visible player_t and edict_t structures, */
/* because we define the full size ones in this file */
#define	GAME_INCLUDE
#include "game.h"

/* the "gameversion" client command will print this plus compile date */
#define	GAMEVERSION	"baseufo"

/*================================================================== */

#define MAX_SPOT_DIST	4096 /* 768 */

#define P_MASK(p)		((p)->num < game.sv_maxplayersperteam ? 1<<((p)->num) : 0)
#define PM_ALL			0xFFFFFFFF

#define NO_ACTIVE_TEAM -1

/* server is running at 10 fps */
#define	SERVER_FRAME_SECONDS		0.1

/* memory tags to allow dynamic memory to be cleaned up */
#define	TAG_GAME	765			/* clear when unloading the dll */
#define	TAG_LEVEL	766			/* clear when loading a new level */

/* Macros for faster access to the inventory-container. */
#define RIGHT(e) (e->i.c[gi.csi->idRight])
#define LEFT(e)  (e->i.c[gi.csi->idLeft])
#define EXTENSION(e)  (e->i.c[gi.csi->idExtension])
#define HEADGEAR(e)  (e->i.c[gi.csi->idHeadgear])
#define FLOOR(e) (e->i.c[gi.csi->idFloor])

/* Actor visiblitly constants */
#define ACTOR_VIS_100	1.0
#define ACTOR_VIS_50	0.5
#define ACTOR_VIS_10	0.1
#define ACTOR_VIS_0		0.0

/* this structure is left intact through an entire game */
/* it should be initialized at dll load time, and read/written to */
/* the server.ssv file for savegames */
typedef struct {
	player_t *players;			/* [maxplayers] */

	/* store latched cvars here that we want to get at often */
	int sv_maxplayersperteam;
	int sv_maxentities;
} game_locals_t;

/* this structure is cleared as each map is entered */
/* it is read/written to the level.sav file for savegames */
typedef struct {
	int framenum;		/**< the current frame (10fps) */
	float time;			/**< seconds the game is running already
						 * calculated through framenum * SERVER_FRAME_SECONDS */

	char mapname[MAX_QPATH];	/**< the server name (base1, etc) */
	char nextmap[MAX_QPATH];	/**< @todo Spawn the new map after the current one was ended */
	qboolean routed;
	int maxteams; /**< the max team amount for multiplayer games for the current map */

	/* intermission state */
	float intermissionTime;
	int winningTeam;
	float roundstartTime;		/**< the time the team started the round */

	/* round statistics */
	int numplayers;
	int activeTeam;
	int nextEndRound;

	int actualRound;

	byte num_alive[MAX_TEAMS];
	byte num_spawned[MAX_TEAMS];
	byte num_spawnpoints[MAX_TEAMS];
	byte num_2x2spawnpoints[MAX_TEAMS];
	byte num_kills[MAX_TEAMS][MAX_TEAMS];
	byte num_stuns[MAX_TEAMS][MAX_TEAMS];
} level_locals_t;


/* spawn_temp_t is only used to hold entity field values that */
/* can be set from the editor, but aren't actualy present */
/* in edict_t during gameplay */
typedef struct {
	/* world vars */
	char *nextmap;
} spawn_temp_t;

/** @brief used in shot probability calculations (pseudo shots) */
typedef struct {
	int enemy;				/**< shot would hit that much enemies */
	int friend;				/**< shot would hit that much friends */
	int civilian;			/**< shot would hit that much civilians */
	int self;				/**< @todo incorrect actor facing or shotOrg, or bug in trace code? */
	int damage;
	qboolean allow_self;
} shot_mock_t;

extern game_locals_t game;
extern level_locals_t level;
extern game_import_t gi;
extern game_export_t globals;

extern edict_t *g_edicts;

#define random()	((rand() & 0x7fff) / ((float)0x7fff))
#define crandom()	(2.0 * (random() - 0.5))

extern cvar_t *sv_maxentities;
extern cvar_t *password;
extern cvar_t *sv_needpass;
extern cvar_t *sv_dedicated;

extern cvar_t *logstats;
extern FILE *logstatsfile;

extern cvar_t *sv_filterban;

extern cvar_t *sv_maxvelocity;

extern cvar_t *sv_cheats;
extern cvar_t *sv_maxclients;
extern cvar_t *sv_reaction_leftover;
extern cvar_t *sv_shot_origin;
extern cvar_t *sv_send_edicts;
extern cvar_t *sv_maxplayersperteam;
extern cvar_t *sv_maxsoldiersperteam;
extern cvar_t *sv_maxsoldiersperplayer;
extern cvar_t *sv_enablemorale;
extern cvar_t *sv_roundtimelimit;

extern cvar_t *sv_maxteams;

extern cvar_t *sv_ai;
extern cvar_t *sv_teamplay;

extern cvar_t *ai_alien;
extern cvar_t *ai_civilian;
extern cvar_t *ai_equipment;
extern cvar_t *ai_numaliens;
extern cvar_t *ai_numcivilians;
extern cvar_t *ai_numactors;
extern cvar_t *ai_autojoin;

extern cvar_t *mob_death;
extern cvar_t *mob_wound;
extern cvar_t *mof_watching;
extern cvar_t *mof_teamkill;
extern cvar_t *mof_civilian;
extern cvar_t *mof_enemy;
extern cvar_t *mor_pain;

/*everyone gets this times morale damage */
extern cvar_t *mor_default;

/*at this distance the following two get halfed (exponential scale) */
extern cvar_t *mor_distance;

/*at this distance the following two get halfed (exponential scale) */
extern cvar_t *mor_victim;

/*at this distance the following two get halfed (exponential scale) */
extern cvar_t *mor_attacker;

/* how much the morale depends on the size of the damaged team */
extern cvar_t *mon_teamfactor;

extern cvar_t *mor_regeneration;
extern cvar_t *mor_shaken;
extern cvar_t *mor_panic;

extern cvar_t *m_sanity;
extern cvar_t *m_rage;
extern cvar_t *m_rage_stop;
extern cvar_t *m_panic_stop;

extern cvar_t *g_aidebug;
extern cvar_t *g_nodamage;
extern cvar_t *g_notu;

extern cvar_t *flood_msgs;
extern cvar_t *flood_persecond;
extern cvar_t *flood_waitdelay;

extern cvar_t *difficulty;

/* fields are needed for spawning from the entity string */
/* and saving / loading games */
#define FFL_SPAWNTEMP		1
#define FFL_NOSPAWN			2

/* g_phys.c */
void G_PhysicsRun(void);
void G_PhysicsStep(edict_t *ent);

/* g_utils.c */
edict_t *G_Find(edict_t *from, int fieldofs, char *match);
edict_t *G_FindRadius(edict_t *from, vec3_t org, float rad, entity_type_t type);
edict_t *G_FindTargetEntity(const char *target);
const char* G_GetPlayerName(int pnum);
int G_GetActiveTeam(void);
const char* G_GetWeaponNameForFiredef(const fireDef_t *fd);
void G_PrintActorStats(const edict_t *victim, const edict_t *attacker, const fireDef_t *fd);
void G_PrintStats(const char *buffer);
int G_TouchTriggers(edict_t *ent);
edict_t *G_Spawn(void);
void G_FreeEdict(edict_t *e);
qboolean G_UseEdict(edict_t *ent);

/* g_reaction.c */
qboolean G_ResolveReactionFire(edict_t *target, qboolean force, qboolean endTurn, qboolean doShoot);
void G_ReactToPreFire(edict_t *target);
void G_ReactToPostFire(edict_t *target);
/* uncomment this to enable debugging the reaction fire */
/*#define DEBUG_REACTION*/


void G_CompleteRecalcRouting(void);
void G_RecalcRouting(const edict_t * ent);
void G_GenerateEntList(const char *entList[MAX_EDICTS]);

/* g_client.c */
/* the visibile changed - if it was visible - it's (the edict) now invisible */
#define VIS_CHANGE	1
/* actor visible? */
#define VIS_YES		2
/* stop the current action if actor appears */
#define VIS_STOP	4

/** check whether edict is still visible - it maybe is currently visible but this
 * might have changed due to some action */
#define VT_PERISH		1
/** don't perform a frustum vis check via G_FrustumVis in G_Vis */
#define VT_NOFRUSTUM	2

#define MORALE_RANDOM( mod )	( (mod) * (1.0 + 0.3*crand()) )

#define MAX_DVTAB 32

void G_FlushSteps(void);
qboolean G_ClientUseEdict(player_t *player, edict_t *actor, edict_t *door);
qboolean G_ActionCheck(player_t *player, edict_t *ent, int TU, qboolean quiet);
void G_SendStats(edict_t *ent);
edict_t *G_SpawnFloor(pos3_t pos);
int G_CheckVisTeam(int team, edict_t *check, qboolean perish);
edict_t *G_GetFloorItems(edict_t *ent);

qboolean G_IsLivingActor(const edict_t *ent);
void G_ForceEndRound(void);
void G_ActorDie(edict_t *ent, int state, edict_t *attacker);
int G_ClientAction(player_t * player);
void G_ClientEndRound(player_t * player, qboolean quiet);
void G_ClientTeamInfo(player_t * player);
int G_ClientGetTeamNum(const player_t * player);
int G_ClientGetTeamNumPref(const player_t * player);
void G_ResetClientData(void);

void G_ClientCommand(player_t * player);
void G_ClientUserinfoChanged(player_t * player, char *userinfo);
void G_ClientBegin(player_t * player);
qboolean G_ClientSpawn(player_t * player);
qboolean G_ClientConnect(player_t * player, char *userinfo);
void G_ClientDisconnect(player_t * player);

int G_TestVis(int team, edict_t * check, int flags);
void G_ClientReload(player_t *player, int entnum, shoot_types_t st, qboolean quiet);
qboolean G_ClientCanReload(player_t *player, int entnum, shoot_types_t st);
void G_ClientGetWeaponFromInventory(player_t *player, int entnum, qboolean quiet);
void G_ClientMove(player_t * player, int team, int num, pos3_t to, qboolean stop, qboolean quiet);
void G_MoveCalc(int team, pos3_t from, int size, int distance);
void G_ClientInvMove(player_t * player, int num, const invDef_t * from, int fx, int fy, const invDef_t * to, int tx, int ty, qboolean checkaction, qboolean quiet);
void G_ClientStateChange(player_t * player, int num, int reqState, qboolean checkaction);
int G_DoTurn(edict_t * ent, byte toDV);

qboolean G_FrustumVis(const edict_t *from, const vec3_t point);
float G_ActorVis(const vec3_t from, const edict_t *check, qboolean full);
void G_ClearVisFlags(int team);
int G_CheckVis(edict_t *check, qboolean perish);
void G_SendInvisible(player_t *player);
void G_GiveTimeUnits(int team);

void G_AppearPerishEvent(int player_mask, int appear, edict_t * check);
int G_VisToPM(int vis_mask);
void G_SendInventory(int player_mask, edict_t * ent);
int G_TeamToPM(int team);

void G_SpawnEntities(const char *mapname, const char *entities);
qboolean G_RunFrame(void);

#ifdef DEBUG
void Cmd_InvList(player_t *player);
void G_KillTeam(void);
void G_StunTeam(void);
void G_ListMissionScore_f(void);
#endif

extern int turnTeam;

/* g_combat.c */
qboolean G_ClientShoot(player_t *player, int num, pos3_t at, int type, int firemode, shot_mock_t *mock, qboolean allowReaction, int z_align);
void G_ResetReactionFire(int team);
qboolean G_ReactToMove(edict_t *target, qboolean mock);
void G_ReactToEndTurn(void);

/* g_ai.c */
extern edict_t *ai_waypointList;
void G_AddToWayPointList(edict_t *ent);
void AI_Run(void);
void AI_ActorThink(player_t *player, edict_t *ent);
player_t *AI_CreatePlayer(int team);
qboolean AI_CheckUsingDoor(const edict_t *ent, const edict_t *door);

/* g_svcmds.c */
void ServerCommand(void);
qboolean SV_FilterPacket(const char *from);

/* g_main.c */
qboolean G_GameRunning(void);
void G_EndGame(int team);
void G_CheckEndGame(void);

/* g_trigger.c */
edict_t* G_TriggerSpawn(edict_t *owner);
void SP_trigger_hurt(edict_t *ent);
void SP_trigger_touch(edict_t *ent);

/* g_func.c */
void SP_func_rotating(edict_t *ent);
void SP_func_door(edict_t *ent);
void SP_func_breakable(edict_t *ent);

/*============================================================================ */

/** @brief e.g. used for breakable objects */
typedef enum {
	MAT_GLASS,		/* default */
	MAT_METAL,
	MAT_ELECTRICAL,
	MAT_WOOD,

	MAT_MAX
} edictMaterial_t;

typedef struct {
	/* actor movement */
	int			contentFlags[MAX_DVTAB];
	int			visflags[MAX_DVTAB];
	byte		steps;
	int			currentStep;

	int			state;
} moveinfo_t;

/* client_t->anim_priority */
#define	ANIM_BASIC		0		/* stand / run */
#define	ANIM_WAVE		1
#define	ANIM_JUMP		2
#define	ANIM_PAIN		3
#define	ANIM_ATTACK		4
#define	ANIM_DEATH		5
#define	ANIM_REVERSE	6

/* client data that stays across multiple level loads */
typedef struct {
	char userinfo[MAX_INFO_STRING];
	char netname[16];

	/** the number of the team for this player
	 * 0 is reserved for civilians and critters */
	int team;
	qboolean ai;				/**< client controlled by ai */

	/** ai specific data */
	edict_t *last;

	float		flood_locktill;		/**< locked from talking */
	float		flood_when[10];		/**< when messages were said */
	int			flood_whenhead;		/**< head pointer for when said */
} client_persistant_t;

/* this structure is cleared on each PutClientInServer(),
 * except for 'client->pers'
 */
struct player_s {
	/* known to server */
	qboolean inuse;
	int num;
	int ping;

	/* private to game */
	qboolean spawned;			/**< already spawned? */
	qboolean began;				/**< the player sent his 'begin' already */
	qboolean ready;				/**< ready to end his round */

	qboolean autostand;			/**< autostand for long walks */

	client_persistant_t pers;
};

/**
 * @brief not the first on the team
 * @sa groupMaster and groupChain
 */
#define FL_GROUPSLAVE	0x00000008
/**
 * @brief If an edict is destroyable (like ET_BREAKABLE, ET_DOOR [if health set]
 * or maybe a ET_MISSION [if health set])
 * @note e.g. misc_mission, func_breakable, func_door
 */
#define FL_DESTROYABLE	0x00000004
/**
 * @brief Trigger the edict at spawn.
 */
#define FL_TRIGGERED	0x00000100

struct edict_s {
	qboolean inuse;
	int linkcount;

	int number;

	vec3_t origin;
	vec3_t angles;

	/** @todo move these fields to a server private sv_entity_t */
	link_t area;				/**< linked to a division node or leaf */

	/* tracing info SOLID_BSP, SOLID_BBOX, ... */
	solid_t solid;

	vec3_t mins, maxs; /**< position of min and max points - relative to origin */
	vec3_t absmin, absmax; /**< position of min and max points - relative to world's origin */
	vec3_t size;

	edict_t *child;	/**< e.g. the trigger for this edict */
	edict_t *owner;	/**< e.g. the door model in case of func_door */
	/*
	 * type of objects the entity will not pass through
	 * can be any of MASK_* or CONTENTS_*
	 */
	int clipmask;
	int modelindex;	 /**< inline model index */

	/*================================ */
	/* don't change anything above here - the server expects the fields in that order */
	/*================================ */

	int mapNum;		/**< entity number in the map file */
	const char *model;	/**< inline model (start with * and is followed by the model numberer)
						 * or misc_model model name (e.g. md2) */

	/** only used locally in game, not by server */

	int type;
	int visflags;

	int contentFlags;			/**< contents flags of the brush the actor is walking in */

	pos3_t pos;
	byte dir;					/**< direction the player looks at */

	int TU;						/**< remaining timeunits for actors or timeunits needed to 'use' this entity */
	int HP;						/**< remaining healthpoints */
	int STUN;					/**< The stun damage received in a mission.
							 * @sa g_combat.c:G_Damage
							 * @todo How is this handled after mission-end?
							 * @todo How is this checked to determine the stun-state? (I've found HP<=STUN in g_combat.c:G_Damage)
							 */
	int morale;					/**< the current morale value */

	int state;					/**< the player state - dead, shaken.... */

	int reaction_minhit;		/**< acceptable odds for reaction shots */

	int team;					/**< player of which team? */
	int pnum;					/**< the actual player slot */
	/** the models (hud) */
	int body;
	int head;
	int skin;

	char *group;				/**< this can be used to trigger a group of entities
								 * e.g. for two-part-doors - set the group to the same
								 * string for each door part and they will open both
								 * if you open one */

	/** delayed reaction fire */
	edict_t *reactionTarget;
	float reactionTUs;
	qboolean reactionNoDraw;

	/** client actions - interact with the world */
	edict_t *client_action;

	/** only set this for the attacked edict - to know who's shooting */
	edict_t *reactionAttacker;

	int	reactionFired;	/**< A simple counter that tells us how many times an actor has fired as a reaction (int he current turn). */

	/** here are the character values */
	character_t chr;

	/** this is the inventory */
	inventory_t i;

	int spawnflags;	/**< set via mapeditor */
	const char *classname;

	float angle;	/**< entity yaw - set via mapeditor - -1=up; -2=down */

	int speed;	/**< speed of entities - e.g. rotating or actors */
	const char *target;	/**< name of the entity to trigger or move towards */
	const char *targetname;	/**< name pointed to by target */
	const char *item;	/**< the item id that must be placed to e.g. the func_mission to activate the use function */
	const char *particle;
	edictMaterial_t material;	/**< material value (e.g. for func_breakable) */
	int count;		/**< general purpose 'amount' variable - set via mapeditor often */
	int time;		/**< general purpose 'rounds' variable - set via mapeditor often */
	int sounds;		/**< type of sounds to play - e.g. doors */
	int dmg;		/**< damage done by entity */
	/** @sa memcpy in Grid_CheckForbidden */
	int fieldSize;	/* ACTOR_SIZE_* */
	qboolean hiding;		/**< for ai actors - when they try to hide after the performed their action */

	/** function to call when triggered - this function should only return true when there is
	 * a client action assosiated with it */
	qboolean (*touch)(edict_t * self, edict_t * activator);
	float nextthink;
	void (*think)(edict_t *self);
	/** general use function that is called when the triggered client action is executed
	 * or when the server has to 'use' the entity */
	qboolean (*use)(edict_t *self);
	qboolean (*destroy)(edict_t *self);

	/** e.g. doors */
	moveinfo_t		moveinfo;

	edict_t *groupChain;
	edict_t *groupMaster;
	int flags;		/**< FL_* */
};

#endif /* GAME_G_LOCAL_H */
