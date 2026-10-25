#include "physfix.h"

#include <cstddef>

// Layouts verified against the engine binaries by GoldSrc_VibeSignatures PR #406.
static_assert(sizeof(physent_t) == 224, "physent_t layout");
static_assert(offsetof(playermove_t, numphysent) == 588, "playermove_t layout");
static_assert(offsetof(playermove_t, physents) == 592, "playermove_t layout");
static_assert(offsetof(playermove_t, numvisent) == 149332, "playermove_t layout");
static_assert(offsetof(playermove_t, visents) == 149336, "playermove_t layout");
static_assert(offsetof(playermove_10152_t, numphysent) == 588, "playermove_10152_t layout");
static_assert(offsetof(playermove_10152_t, physents) == 592, "playermove_10152_t layout");
static_assert(offsetof(playermove_10152_t, numvisent) == 244308, "playermove_10152_t layout");
static_assert(offsetof(playermove_10152_t, visents) == 244312, "playermove_10152_t layout");

model_t* GSPhysFix_GetSlideBoxHitboxModel(const edict_t* check, model_t* const* models)
{
    if (!check || !models || check->v.solid != SOLID_SLIDEBOX)
        return NULL;

    // The engine indexes sv.models without an upper bound in its other branches.
    if (check->v.modelindex <= 0)
        return NULL;

    model_t* model = models[check->v.modelindex];

    if (!model || model->type != mod_studio || !(model->flags & FMODEL_TRACE_HITBOX))
        return NULL;

    return model;
}

void GSPhysFix_FixPhysent(physent_t* pe, const edict_t* check, model_t* const* models)
{
    if (!pe)
        return;

    model_t* model = GSPhysFix_GetSlideBoxHitboxModel(check, models);

    if (model)
        pe->studiomodel = model;
}

void GSPhysFix_FixPhysents(physent_t* physents, int first, int count, int capacity, model_t* const* models, fnEdictOfIndex edictOfIndex)
{
    if (!physents || !edictOfIndex)
        return;

    // windows.h min/max macros rule out std::min/std::max here.
    const int begin = first > 0 ? first : 0;
    const int end   = count < capacity ? count : capacity;

    for (int i = begin; i < end; i++)
    {
        physent_t* pe = &physents[i];

        if (pe->solid != SOLID_SLIDEBOX)
            continue;

        GSPhysFix_FixPhysent(pe, edictOfIndex(pe->info), models);
    }
}
