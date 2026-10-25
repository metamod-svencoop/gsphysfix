#pragma once

#include <extdll.h>

#include <com_model.h>
#include <pm_defs.h>

#include <iterator>

// Studio models compiled with "$flags 512" request hitbox traces.
#ifndef FMODEL_TRACE_HITBOX
#    define FMODEL_TRACE_HITBOX 0x200
#endif

// Sven Co-op 5.16 (build 10152) enlarged the playermove entity lists.
#define SVENCOOP_PLAYERMOVE_10152_BUILD 10152

typedef edict_t* (*fnEdictOfIndex)(int iEntIndex);

// The studio model a SOLID_SLIDEBOX entity should be traced against, as client
// prediction already does, or NULL to keep its bounding box. models is sv.models.
model_t* GSPhysFix_GetSlideBoxHitboxModel(const edict_t* check, model_t* const* models);

// Applies the rule to one physent converted from check by SV_CopyEdictToPhysent.
// Only studiomodel is written; every other field keeps the engine's conversion.
void GSPhysFix_FixPhysent(physent_t* pe, const edict_t* check, model_t* const* models);

// Applies the rule to entries [first, count) of a list holding capacity entries.
void GSPhysFix_FixPhysents(physent_t* physents, int first, int count, int capacity, model_t* const* models, fnEdictOfIndex edictOfIndex);

// Runs one SV_AddLinksToPM_ call where the engine inlined SV_CopyEdictToPhysent.
// The walker recurses through its own hooked entry, so only the outermost call
// fixes the visents and physents appended by the whole walk. The walker copies
// each visent into physents without reading studiomodel, so fixing both lists
// afterwards matches converting them with the rule applied.
template <typename TPlayerMove, typename TCallOriginal>
void GSPhysFix_RunAddLinksToPM(TPlayerMove* pmove, int& depth, model_t* const* models, fnEdictOfIndex edictOfIndex, TCallOriginal callOriginal)
{
    if (depth > 0 || !pmove)
    {
        callOriginal();
        return;
    }

    const int firstVisent  = pmove->numvisent;
    const int firstPhysent = pmove->numphysent;

    ++depth;
    callOriginal();
    --depth;

    GSPhysFix_FixPhysents(pmove->visents, firstVisent, pmove->numvisent, int(std::size(pmove->visents)), models, edictOfIndex);
    GSPhysFix_FixPhysents(pmove->physents, firstPhysent, pmove->numphysent, int(std::size(pmove->physents)), models, edictOfIndex);
}
