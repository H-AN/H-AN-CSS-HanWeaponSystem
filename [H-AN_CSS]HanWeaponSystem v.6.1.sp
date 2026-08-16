#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <sdktools>
#include <sdkhooks>
#include <cstrike>
#include <smlib>
#include <clientprefs>
#include <HanWeaponSystem>

#include "HanWeaponSystem/HanWeaponSystemGlobals"
#include "HanWeaponSystem/HanWeaponSystemConfig"
#include "HanWeaponSystem/HanWeaponSystemSounds"
#include "HanWeaponSystem/HanWeaponSystemCamera"
#include "HanWeaponSystem/HanWeaponSystemZoomConfig"
#include "HanWeaponSystem/HanWeaponSystemZoom"
#include "HanWeaponSystem/HanWeaponSystemInspectConfig"
#include "HanWeaponSystem/HanWeaponSystemInspect"
#include "HanWeaponSystem/HanWeaponSystemWeapon"
#include "HanWeaponSystem/HanWeaponSystemCombat"
#include "HanWeaponSystem/HanWeaponSystemViewModel"
#include "HanWeaponSystem/HanWeaponSystemAPI"

public Plugin myinfo =
{
    name = "[华仔]武器系统",
    author = "华仔 H-AN",
    description = "华仔 H-AN 武器整合系统",
    version = VERSION,
    url = "[华仔]武器系统, QQ群107866133, github https://github.com/H-AN"
};

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
    RegisterWeaponAPI();
    return APLRes_Success;
}

public void OnPluginStart()
{
    offsCollision = FindSendPropInfo("CBaseEntity", "m_CollisionGroup");

    Config_OnPluginStart();
    ZoomConfig_OnPluginStart();
    Zoom_OnPluginStart();
    InspectConfig_OnPluginStart();
    Inspect_OnPluginStart();
    ViewModel_OnPluginStart();

    HookEvent("player_death", Event_PlayerDeathPre, EventHookMode_Pre);
    HookEvent("player_death", Event_PlayerDeath);
    HookEvent("player_spawn", Event_PlayerSpawn);

    AddNormalSoundHook(SoundCallBackHook);
    AddTempEntHook("Shotgun Shot", WeaponFireBullets);

    Hookshotguns();
}

public void OnMapStart()
{
    Config_OnMapStart();
    ZoomConfig_OnMapStart();
    Zoom_OnMapStart();
    InspectConfig_OnMapStart();
    Camera_OnMapStart();
}

public void OnClientPutInServer(int client)
{
    SDKHook(client, SDKHook_WeaponSwitch, WeaponHook);
    SDKHook(client, SDKHook_WeaponEquip, WeaponHook);

    SDKHook(client, SDKHook_OnTakeDamage, OnTakeDamage);

    SDKHook(client, SDKHook_PostThinkPost, OnPostThinkPost);

    SDKHook(client, SDKHook_WeaponSwitchPost, WeaponSwitchPost);
    SDKHook(client, SDKHook_WeaponDrop, OnWeaponDrop);

    SDKHook(client, SDKHook_PostThink, Hook_OnPostThinkSpeed);
    SDKHook(client, SDKHook_PostThinkPost, Hook_OnPostThinkPostSpeed);

    SDKHook(client, SDKHook_OnTakeDamagePost, BlockKnowBack);

    SDKHook(client, SDKHook_WeaponSwitch, HideFakeWModel);
    SDKHook(client, SDKHook_WeaponEquip, HideFakeWModel);
}

public void OnClientDisconnect(int client)
{
    g_fModifyNextAttack[client] = 0.0;

    SpawnCheck[client] = false;
    IsCustom[client] = false;
    IsCustom2[client] = false;
    g_bPressingAttack2[client] = false;
    g_bWeaponAutoFire[client] = false;

    g_bZooming[client] = false;
    g_iZoomInSwitch[client] = 0;
    g_fZoomBlock[client] = 0.0;

    Inspect_Reset(client);

    ClientVM[client][0] = -1;
    ClientVM[client][1] = -1;
}