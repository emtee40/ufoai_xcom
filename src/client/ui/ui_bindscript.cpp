/**
 * @file ui_bindscript.cpp
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

#include "../../common/angelscript/angelscript.h"
#include "../../common/angelscript/addons/scriptbuilder.h"
#include "../../common/angelscript/addons/scriptstdstring.h"

extern "C" {

#include "ui_main.h"
#include "ui_internal.h"
#include "ui_bindscript.h"
#include "ui_parse.h"

#include "../client.h"

}/* extern "C" */


static asIScriptEngine *engine;


extern "C" void UI_ParseActionScript (uiNode_t *node, const char *script)
{
	int r;

	CScriptBuilder builder;
	r = builder.StartNewModule(engine, UI_GetPath(node));
	if( r < 0 ) return;

	r = builder.AddSectionFromMemory(script);
	if( r < 0 ) return;

	r = builder.BuildModule();
	if( r < 0 )	{
		Com_Printf("UI_ParseActionScript: No way to build script module\n");
		return;
	}

	Com_Printf("UI_ParseActionScript: Script successfully built\n");
	asIScriptModule *module = engine->GetModule(UI_GetPath(node));

	/* link all node event to relative scripts */
	const uiBehaviour_t *behaviour = node->behaviour;
	while (behaviour) {
		const value_t *property = behaviour->properties;
		while(property && property->string != NULL) {
			if (property->type == V_UI_ACTION) {
				const int functionId = module->GetFunctionIdByName(property->string);
				if (functionId >= 0) {
					asIScriptFunction *function = module->GetFunctionDescriptorById(functionId);
					if (function != NULL) {
						Com_Printf("UI_ParseActionScript: Found %s\n", property->string);
						uiAction_t *action = UI_AllocStaticAction();
						action->type = EA_SCRIPT;
						action->d.terminal.d1.data = function;
						if (node->onClick) {
							Com_Printf("UI_ParseActionScript: %s function not linked\n", property->string);
						} else {
							node->onClick = action;
						}
					}
				}
			}
			property++;
		}
		behaviour = behaviour->super;
	}
}

extern "C" void UI_ExecuteScriptAction (uiAction_t *action, uiCallContext_t *context)
{
	int r;
	asIScriptFunction *function = (asIScriptFunction*) action->d.terminal.d1.data;

	asIScriptContext *ctx = engine->CreateContext();
	if (ctx == NULL) {
		Com_Printf("Failed to create the context.\n");
		return;
	}

	r = ctx->Prepare(function->GetId());
	if (r < 0) {
		Com_Printf("Failed to prepare the context.\n");
		ctx->Release();
		return;
	}

	r = ctx->Execute();
	if (r != asEXECUTION_FINISHED) {
		// The execution didn't finish as we had planned. Determine why.
		if (r == asEXECUTION_ABORTED) {
			Com_Printf("The script was aborted before it could finish. Probably it timed out.");
		} else if (r == asEXECUTION_EXCEPTION) {
			Com_Printf("The script ended with an exception.");

			// Write some information about the script exception
			/*
			cout << "func: " << function->GetDeclaration() << endl;
			cout << "modl: " << function->GetModuleName() << endl;
			cout << "sect: " << function->GetScriptSectionName() << endl;
			cout << "line: " << ctx->GetExceptionLineNumber() << endl;
			cout << "desc: " << ctx->GetExceptionString() << endl;
			*/
		} else {
			Com_Printf("The script ended for some unforeseen reason (%i)", r);
		}
	}

	// We must release the contexts when no longer using them
	ctx->Release();
}

static const std::string CvarRef_GetString(cvar_t *cvar) {
	return cvar->string;
}
static void CvarRef_SetString(cvar_t *cvar, std::string value) {
	Cvar_Set(cvar->name, value.c_str());
}
static int CvarRef_GetInteger(cvar_t *cvar) {
	return cvar->integer;
}
static float CvarRef_GetNumber(cvar_t *cvar) {
	return cvar->value;
}
static void CvarRef_SetInteger(cvar_t *cvar, int value) {
	Cvar_SetValue(cvar->name, value);
}
static void CvarRef_SetNumber(cvar_t *cvar, float value) {
	Cvar_SetValue(cvar->name, value);
}
static cvar_t *CvarRef_Get(std::string name) {
	return Cvar_Get(name.c_str(), "", 0, "Script generated");
}
static void CvarRef_AddRef(cvar_t *cvar) {
	/** @todo count number of ref */
}

static void CvarRef_Release(cvar_t *cvar) {
	/** @todo count number of ref */
}

static void MessageCallback(const asSMessageInfo *msg, void *param)
{
	const char *type = "ERR ";
	if( msg->type == asMSGTYPE_WARNING )
		type = "WARN";
	else if( msg->type == asMSGTYPE_INFORMATION )
		type = "INFO";
	Com_Printf("AngelScript: %s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
}

extern "C" void UI_InitBindScript (void)
{
	int r;
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);
	RegisterStdString(engine);


	/* CVAR */

	r = engine->RegisterObjectType("cvar", sizeof((cvar_t*)NULL), asOBJ_REF);
	assert(r >= 0);
	r = engine->RegisterObjectBehaviour("cvar", asBEHAVE_ADDREF, "void f()", asFUNCTION(CvarRef_AddRef), asCALL_CDECL_OBJFIRST);
	assert(r >= 0);
	r = engine->RegisterObjectBehaviour("cvar", asBEHAVE_RELEASE, "void f()", asFUNCTION(CvarRef_Release), asCALL_CDECL_OBJFIRST);
	assert(r >= 0);

	r = engine->RegisterObjectMethod("cvar", "const string get_string()", asFUNCTION(CvarRef_GetString), asCALL_CDECL_OBJFIRST);
	assert(r >= 0);
	r = engine->RegisterObjectMethod("cvar", "void set_string(string)", asFUNCTION(CvarRef_SetString), asCALL_CDECL_OBJFIRST);
	assert(r >= 0);
	r = engine->RegisterObjectMethod("cvar", "int get_integer()", asFUNCTION(CvarRef_GetInteger), asCALL_CDECL_OBJFIRST);
	assert(r >= 0);
	r = engine->RegisterObjectMethod("cvar", "float get_number()", asFUNCTION(CvarRef_GetNumber), asCALL_CDECL_OBJFIRST);
	assert(r >= 0);
	r = engine->RegisterObjectMethod("cvar", "void set_integer(int)", asFUNCTION(CvarRef_SetInteger), asCALL_CDECL_OBJFIRST);
	assert(r >= 0);
	r = engine->RegisterObjectMethod("cvar", "void set_number(float)", asFUNCTION(CvarRef_SetNumber), asCALL_CDECL_OBJFIRST);
	assert(r >= 0);

	r = engine->RegisterGlobalFunction("cvar@ getCvar(string)", asFUNCTION(CvarRef_Get), asCALL_CDECL);
	assert(r >= 0);
}

extern "C" void UI_ShutdownBindScript (void)
{
	engine->Release();
}
