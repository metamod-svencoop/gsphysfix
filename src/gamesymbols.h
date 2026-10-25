#pragma once

#include <signatures_template.h>

// XXXX_GameSymbol is the symbolName of XXXX in the GoldSrc_VibeSignatures
// gamedata (scripts/manifests/gsphysfix_windows.json and gsphysfix_linux.json).
// XXXX_GameSymbolAlternative is tried when the catalog has no XXXX_GameSymbol
// entry, since some Linux builds only export the mangled name.

#define SV_CopyEdictToPhysent_GameSymbol            "SV_CopyEdictToPhysent"
#define SV_CopyEdictToPhysent_GameSymbolAlternative "_Z21SV_CopyEdictToPhysentP9physent_siP7edict_s"
#define SV_AddLinksToPM_GameSymbol                  "SV_AddLinksToPM_"
#define SV_AddLinksToPM_GameSymbolAlternative       "_Z16SV_AddLinksToPM_P10areanode_sPfS1_"
#define build_number_GameSymbol                     "build_number"

#define sv_models_GameSymbol "sv_models"
#define pmove_GameSymbol     "pmove"
