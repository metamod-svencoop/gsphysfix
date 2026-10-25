#include <extdll.h>

#include <meta_api.h>

#include "enginedef.h"
#include "gsphysfix.h"

PRIVATE_FUNCTION_DEFINE(SV_CopyEdictToPhysent);
PRIVATE_FUNCTION_DEFINE(SV_AddLinksToPM_);
PRIVATE_FUNCTION_DEFINE(build_number);

model_t** sv_models = NULL;
void**    pmove     = NULL;

bool g_bPlayerMove10152 = false;

// SV_AddLinksToPM_ recursion depth of the current walk.
static int g_iAddLinksToPMDepth = 0;

void NewSV_CopyEdictToPhysent(physent_t* pe, int e, edict_t* check)
{
    g_call_original_SV_CopyEdictToPhysent(pe, e, check);

    if (GSPhysFix_IsEnabled())
        GSPhysFix_FixPhysent(pe, check, sv_models);
}

template <typename TPlayerMove>
static void RunAddLinksToPM(areanode_t* node, float* pmove_mins, float* pmove_maxs)
{
    GSPhysFix_RunAddLinksToPM((TPlayerMove*)*pmove, g_iAddLinksToPMDepth, sv_models, g_engfuncs.pfnPEntityOfEntIndex, [=]() {
        g_call_original_SV_AddLinksToPM_(node, pmove_mins, pmove_maxs);
    });
}

void NewSV_AddLinksToPM_(areanode_t* node, float* pmove_mins, float* pmove_maxs)
{
    if (!GSPhysFix_IsEnabled())
    {
        g_call_original_SV_AddLinksToPM_(node, pmove_mins, pmove_maxs);
        return;
    }

    if (g_bPlayerMove10152)
        RunAddLinksToPM<playermove_10152_t>(node, pmove_mins, pmove_maxs);
    else
        RunAddLinksToPM<playermove_t>(node, pmove_mins, pmove_maxs);
}

// Meta_Attach resolves exactly one of the two hook points.
static hook_t* g_phook_SV_CopyEdictToPhysent = NULL;
static hook_t* g_phook_SV_AddLinksToPM_      = NULL;

static hook_t* InstallInlineHook(void* target, void* replacement, void** original, bool bTransaction)
{
    hook_t* hook = gpMetaUtilFuncs->pfnInlineHook(target, replacement, original, bTransaction);

    if (hook && !*original)
    {
        gpMetaUtilFuncs->pfnUnHook(hook);
        hook = NULL;
    }

    return hook;
}

bool InstallEngineHooks(bool bTransaction)
{
    if (g_pfn_SV_AddLinksToPM_)
    {
        g_phook_SV_AddLinksToPM_ = InstallInlineHook((void*)g_pfn_SV_AddLinksToPM_, (void*)NewSV_AddLinksToPM_, (void**)&g_call_original_SV_AddLinksToPM_, bTransaction);
        return g_phook_SV_AddLinksToPM_ != NULL;
    }

    g_phook_SV_CopyEdictToPhysent = InstallInlineHook((void*)g_pfn_SV_CopyEdictToPhysent, (void*)NewSV_CopyEdictToPhysent, (void**)&g_call_original_SV_CopyEdictToPhysent, bTransaction);
    return g_phook_SV_CopyEdictToPhysent != NULL;
}

bool UninstallEngineHooks()
{
    if (g_phook_SV_AddLinksToPM_ && !gpMetaUtilFuncs->pfnUnHook(g_phook_SV_AddLinksToPM_))
        return false;

    g_phook_SV_AddLinksToPM_ = NULL;

    if (g_phook_SV_CopyEdictToPhysent && !gpMetaUtilFuncs->pfnUnHook(g_phook_SV_CopyEdictToPhysent))
        return false;

    g_phook_SV_CopyEdictToPhysent = NULL;
    return true;
}
