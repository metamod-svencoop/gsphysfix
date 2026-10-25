#pragma once

extern cvar_t g_sv_gsphysfix;

extern bool g_bIsSvenCoop;

void GSPhysFix_Init(const char* gameDirectory);
void GSPhysFix_RegisterCvars();
bool GSPhysFix_IsEnabled();
