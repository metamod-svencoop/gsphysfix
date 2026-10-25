#include <extdll.h> // always

#include <meta_api.h> // of course

#include "enginedef.h"
#include "gsphysfix.h"

#define REQUIRED_INTERFACE_MAJOR_VERSION 5
#define REQUIRED_INTERFACE_MINOR_VERSION 17

// Must provide at least one of these..
static META_FUNCTIONS gMetaFunctionTable = {
    NULL,          // pfnGetEntityAPI				HL SDK; called before game DLL
    NULL,          // pfnGetEntityAPI_Post			META; called after game DLL
    GetEntityAPI2, // pfnGetEntityAPI2				HL SDK2; called before game DLL
    NULL,          // pfnGetEntityAPI2_Post		META; called after game DLL
    NULL,          // pfnGetNewDLLFunctions		HL SDK2; called before game DLL
    NULL,          // pfnGetNewDLLFunctions_Post	META; called after game DLL
    NULL,          // pfnGetEngineFunctions	META; called before HL engine
    NULL,          // pfnGetEngineFunctions_Post	META; called after HL engine
    NULL,          // pfnGetStudioBlendingInterface 2022/07/02 Added by hzqst
    NULL,          // pfnGetStudioBlendingInterface_Post 2022/07/02 Added by hzqst
};

// Description of plugin
plugin_info_t Plugin_info = {
    META_INTERFACE_VERSION,                          // ifvers
    "GSPhysFix",                                     // name
    "1.0",                                           // version
    "2026",                                          // date
    "hzqst",                                         // author
    "https://github.com/metamod-svencoop/gsphysfix", // url
    "GSPHYSFIX",                                     // logtag, all caps please
    PT_ANYTIME,                                      // (when) loadable
    PT_NEVER,                                        // (when) unloadable
};

// Global vars from metamod:
meta_globals_t*  gpMetaGlobals;   // metamod globals
gamedll_funcs_t* gpGamedllFuncs;  // gameDLL function tables
mutil_funcs_t*   gpMetaUtilFuncs; // metamod utility functions

// Accept "<major>:<minor>" with the required major version and at least the
// required minor version. Trailing characters are rejected.
static bool IsCompatibleInterfaceVersion(const char* interfaceVersion)
{
    if (!interfaceVersion)
        return false;

    int  iMajorVersion = 0;
    int  iMinorVersion = 0;
    char trailing      = 0;

    if (sscanf(interfaceVersion, "%d:%d%c", &iMajorVersion, &iMinorVersion, &trailing) != 2)
        return false;

    return iMajorVersion == REQUIRED_INTERFACE_MAJOR_VERSION && iMinorVersion >= REQUIRED_INTERFACE_MINOR_VERSION;
}

// Look symbolName up in metamod's gamedata catalog, retrying
// alternativeSymbolName when the catalog has no entry for symbolName.
static meta_gamesymbol_status_t LookupEngineSymbol(void* engineBase, const char* symbolName, meta_gamesymbol_kind_t kind, void** pAddress, const char* alternativeSymbolName)
{
    auto status = gpMetaUtilFuncs->pfnResolveGameSymbol(engineBase, symbolName, kind, pAddress);

    if (status == META_GAMESYMBOL_SYMBOL_NOT_FOUND && alternativeSymbolName)
        status = gpMetaUtilFuncs->pfnResolveGameSymbol(engineBase, alternativeSymbolName, kind, pAddress);

    return status;
}

static bool ResolveEngineSymbol(void* engineBase, const char* symbolName, meta_gamesymbol_kind_t kind, void** pAddress, const char* alternativeSymbolName = NULL)
{
    auto status = LookupEngineSymbol(engineBase, symbolName, kind, pAddress, alternativeSymbolName);

    if (status == META_GAMESYMBOL_OK)
        return true;

    LOG_ERROR(PLID, "Cannot resolve %s: %s", symbolName, gpMetaUtilFuncs->pfnGetGameSymbolStatusString(status));
    return false;
}

#define RESOLVE_ENGINE_FUNCTION(name) ResolveEngineSymbol(engineBase, name##_GameSymbol, META_GAMESYMBOL_KIND_FUNCTION, (void**)&g_pfn_##name, name##_GameSymbolAlternative)
#define RESOLVE_ENGINE_VARIABLE(name) ResolveEngineSymbol(engineBase, name##_GameSymbol, META_GAMESYMBOL_KIND_GLOBAL, (void**)&name)

// The catalog publishes pmove only for builds whose SV_AddLinksToPM_ inlines the
// physent conversion; those builds hook the walker. Every other build hooks
// SV_CopyEdictToPhysent, which a build may keep without calling it.
static bool ResolveHookPoint(void* engineBase)
{
    if (LookupEngineSymbol(engineBase, SV_AddLinksToPM_GameSymbol, META_GAMESYMBOL_KIND_FUNCTION, (void**)&g_pfn_SV_AddLinksToPM_, SV_AddLinksToPM_GameSymbolAlternative) == META_GAMESYMBOL_OK &&
        LookupEngineSymbol(engineBase, pmove_GameSymbol, META_GAMESYMBOL_KIND_GLOBAL, (void**)&pmove, NULL) == META_GAMESYMBOL_OK)
    {
        if (!ResolveEngineSymbol(engineBase, build_number_GameSymbol, META_GAMESYMBOL_KIND_FUNCTION, (void**)&g_pfn_build_number))
            return false;

        // Half-Life builds beyond 10152 keep the original layout.
        g_bPlayerMove10152 = g_bIsSvenCoop && g_pfn_build_number() >= SVENCOOP_PLAYERMOVE_10152_BUILD;

        LOG_MESSAGE(PLID, "Hooking SV_AddLinksToPM_ (%s playermove layout)", g_bPlayerMove10152 ? "Sven Co-op 5.16" : "original");
        return true;
    }

    g_pfn_SV_AddLinksToPM_ = NULL;
    pmove                  = NULL;

    if (!RESOLVE_ENGINE_FUNCTION(SV_CopyEdictToPhysent))
        return false;

    LOG_MESSAGE(PLID, "Hooking SV_CopyEdictToPhysent");
    return true;
}

// Metamod requesting info about this plugin:
//  ifvers			(given) interface_version metamod is using
//  pPlugInfo		(requested) struct with info about plugin
//  pMetaUtilFuncs	(given) table of utility functions provided by metamod
C_DLLEXPORT int Meta_Query(const char* interfaceVersion, plugin_info_t** pPlugInfo, mutil_funcs_t* pMetaUtilFuncs)
{
    if (!IsCompatibleInterfaceVersion(interfaceVersion))
    {
        pMetaUtilFuncs->pfnLogError(PLID, "Metamod interface 5:17 or newer compatible 5.x is required");
        return FALSE;
    }

    // Give metamod our plugin_info struct
    *pPlugInfo = &Plugin_info;
    // Get metamod utility function table.
    gpMetaUtilFuncs = pMetaUtilFuncs;
    return TRUE;
}

C_DLLEXPORT int Meta_Attach(PLUG_LOADTIME    now,
                            META_FUNCTIONS*  pFunctionTable,
                            meta_globals_t*  pMGlobals,
                            gamedll_funcs_t* pGamedllFuncs)
{
    if (!pFunctionTable || !pMGlobals || !gpGlobals)
        return FALSE;

    gpMetaGlobals  = pMGlobals;
    gpGamedllFuncs = pGamedllFuncs;

    auto engineBase = gpMetaUtilFuncs->pfnGetEngineBase();

    if (!engineBase)
        return FALSE;

    char gameDirectory[1024] = {};
    g_engfuncs.pfnGetGameDir(gameDirectory);

    GSPhysFix_Init(gameDirectory);

    if (!RESOLVE_ENGINE_VARIABLE(sv_models))
        return FALSE;

    if (!ResolveHookPoint(engineBase))
        return FALSE;

    // Only startup has a later Metamod transaction commit.
    if (!InstallEngineHooks(now == PT_STARTUP))
    {
        LOG_ERROR(PLID, "Unable to install the physent conversion hook");
        return FALSE;
    }

    GSPhysFix_RegisterCvars();

    memcpy(pFunctionTable, &gMetaFunctionTable, sizeof(META_FUNCTIONS));
    return TRUE;
}

// Metamod detaching plugin from the server.
// now		(given) current phase, ie during map, etc
// reason	(given) why detaching (refresh, console unload, forced unload, etc)
C_DLLEXPORT int Meta_Detach(PLUG_LOADTIME /* now */,
                            PL_UNLOAD_REASON /* reason */)
{
    return UninstallEngineHooks() ? TRUE : FALSE;
}
