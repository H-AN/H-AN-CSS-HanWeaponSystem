#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <sdktools>
#include <sdkhooks>
#include <cstrike>
#include <HanWeaponSystem>

#include "HanWeaponSystem/HanWeaponSystemGlobals"
#include "HanWeaponSystem/HanWeaponSystemConfig"
#include "HanWeaponSystem/HanWeaponSystemDownload"
#include "HanWeaponSystem/HanWeaponSystemEmptyReloadConfig"
#include "HanWeaponSystem/HanWeaponSystemEmptyReload"
#include "HanWeaponSystem/HanWeaponSystemSounds"
#include "HanWeaponSystem/HanWeaponSystemCamera"
#include "HanWeaponSystem/HanWeaponSystemZoomConfig"
#include "HanWeaponSystem/HanWeaponSystemZoom"
#include "HanWeaponSystem/HanWeaponSystemInspectConfig"
#include "HanWeaponSystem/HanWeaponSystemInspect"
#include "HanWeaponSystem/HanWeaponSystemRunConfig"
#include "HanWeaponSystem/HanWeaponSystemRun"
#include "HanWeaponSystem/HanWeaponSystemBuyConfig"
#include "HanWeaponSystem/HanWeaponSystemBotConfig"
#include "HanWeaponSystem/HanWeaponSystemWeapon"
#include "HanWeaponSystem/HanWeaponSystemBuy"
#include "HanWeaponSystem/HanWeaponSystemBot"
#include "HanWeaponSystem/HanWeaponSystemCustomAnim"
#include "HanWeaponSystem/HanWeaponSystemCombat"
#include "HanWeaponSystem/HanWeaponSystemViewModel"
#include "HanWeaponSystem/HanWeaponSystemBackModel"
#include "HanWeaponSystem/HanWeaponSystemOriginalWeaponsfixes"
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
    Download_OnPluginStart();
    EmptyReloadConfig_OnPluginStart();
    EmptyReload_OnPluginStart();
    ZoomConfig_OnPluginStart();
    Zoom_OnPluginStart();
    InspectConfig_OnPluginStart();
    Inspect_OnPluginStart();
    RunConfig_OnPluginStart();
    Run_OnPluginStart();
    BuyConfig_OnPluginStart();
    Buy_OnPluginStart();
    BotConfig_OnPluginStart();
    Bot_OnPluginStart();
    ViewModel_OnPluginStart();
    BackModel_OnPluginStart();
    OldWeapon_OnPluginStart();

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
    Download_OnMapStart();
    EmptyReloadConfig_OnMapStart();
    ZoomConfig_OnMapStart();
    Zoom_OnMapStart();
    InspectConfig_OnMapStart();
    RunConfig_OnMapStart();
    BuyConfig_OnMapStart();
    BotConfig_OnMapStart();
    Buy_OnMapStart();
    Camera_OnMapStart();
    Weapon_OnMapStart();
    BackModel_OnMapStart();
}

public void OnClientPutInServer(int client)
{
    ViewModel_OnClientPutInServer(client);

    SDKHook(client, SDKHook_WeaponSwitch, WeaponHook);
    SDKHook(client, SDKHook_WeaponEquip, WeaponHook);

    SDKHook(client, SDKHook_OnTakeDamage, OnTakeDamage);

    SDKHook(client, SDKHook_PostThinkPost, OnPostThinkPost);
    SDKHook(client, SDKHook_PostThinkPost, BackModel_PostThinkPost);

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
    g_bPrevSilencer[client] = false;

    g_bZooming[client] = false;
    g_iZoomInSwitch[client] = 0;
    g_fZoomBlock[client] = 0.0;

    g_bSideAiming[client] = false;
    g_iSideAimInSwitch[client] = 0;
    g_fSideAimBlock[client] = 0.0;

    Inspect_Reset(client);
    Run_Reset(client);
    EmptyReload_Reset(client);
    CustomAnim_Reset(client);

    g_bBotBlockBuy[client] = false;
    g_bBotRunHold[client] = false;
    g_fBotRunHoldEnd[client] = 0.0;
    g_iBotGiveSlot[client] = 0;

    g_bShootQcFix[client] = false;
    g_bQcFixShot[client] = false;
    g_iQcFixRestore[client] = -1;
    g_bGrenadePullPrev[client] = false;
    g_bZoomAnimFire[client] = false;
    g_bSideAimAnimFire[client] = false;

    ClientVM[client][0] = -1;
    ClientVM[client][1] = -1;

    BackModel_Reset(client);
}

public void OnPluginEnd()
{
    BackModel_PluginEnd();
}