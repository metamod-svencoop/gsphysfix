#include <extdll.h> // always

#include <meta_api.h> // of course

#include <filesystem>
#include <string>

#include "gsphysfix.h"

// cvar_t holds non-const strings, so keep the initial values in writable storage.
static char g_szEnableCvarName[]  = "sv_gsphysfix";
static char g_szEnableCvarValue[] = "1";

cvar_t g_sv_gsphysfix = {g_szEnableCvarName, g_szEnableCvarValue, FCVAR_SERVER};

bool g_bIsSvenCoop = false;

// Metamod registers its own copy of plugin cvars; this is the engine-owned one.
static cvar_t* g_pEngineEnableCvar = NULL;

void GSPhysFix_Init(const char* gameDirectory)
{
    std::string gameName = std::filesystem::path(gameDirectory).filename().string();

    for (char& c : gameName)
    {
        if (c >= 'A' && c <= 'Z')
            c = char(c + ('a' - 'A'));
    }

    g_bIsSvenCoop = gameName == "svencoop";
}

void GSPhysFix_RegisterCvars()
{
    g_engfuncs.pfnCVarRegister(&g_sv_gsphysfix);
    g_pEngineEnableCvar = g_engfuncs.pfnCVarGetPointer(g_sv_gsphysfix.name);
}

bool GSPhysFix_IsEnabled()
{
    // Queried for every converted entity, so avoid a name lookup per call.
    if (g_pEngineEnableCvar)
        return g_pEngineEnableCvar->value != 0;

    return g_engfuncs.pfnCVarGetFloat(g_sv_gsphysfix.name) != 0;
}
