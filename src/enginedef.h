#pragma once

#include "gamesymbols.h"

#include "physfix.h"

// Engine private types

// areanode_t is only passed through to the original walker.
typedef struct areanode_s areanode_t;

// Engine private functions

// SV_CopyEdictToPhysent, where the engine keeps it as a separate function
typedef void (*fnSV_CopyEdictToPhysent)(physent_t* pe, int e, edict_t* check);
void NewSV_CopyEdictToPhysent(physent_t* pe, int e, edict_t* check);
PRIVATE_FUNCTION_EXTERN(SV_CopyEdictToPhysent);

// SV_AddLinksToPM_, hooked where SV_CopyEdictToPhysent is inlined into it
typedef void (*fnSV_AddLinksToPM_)(areanode_t* node, float* pmove_mins, float* pmove_maxs);
void NewSV_AddLinksToPM_(areanode_t* node, float* pmove_mins, float* pmove_maxs);
PRIVATE_FUNCTION_EXTERN(SV_AddLinksToPM_);

// build_number
typedef int (*fnbuild_number)();
PRIVATE_FUNCTION_EXTERN(build_number);

// Engine private variables

// sv.models, an array indexed by modelindex
extern model_t** sv_models;
// The current playermove_t, or playermove_10152_t on Sven Co-op 5.16 and later
extern void** pmove;

extern bool g_bPlayerMove10152;

// Startup loads defer hook activation to the Metamod transaction commit;
// runtime loads must enable the hook immediately.
bool InstallEngineHooks(bool bTransaction);
bool UninstallEngineHooks();
