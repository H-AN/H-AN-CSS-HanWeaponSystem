// ============================================================================
// >> API 注册(natives/forwards)
// ============================================================================

void RegisterWeaponAPI()
{
    CreateNative("GetClientViewModel", Native_GetClientViewModel);
    CreateNative("IsClientPressingAttack2", Native_IsClientPressingAttack2);
    CreateNative("IsWeaponAutoFire", Native_IsWeaponAutoFire);
    CreateNative("Han_IsClientZooming", Native_Han_IsClientZooming);
    CreateNative("Han_IsWeaponZoomable", Native_Han_IsWeaponZoomable);
    CreateNative("Han_IsClientSideAiming", Native_Han_IsClientSideAiming);
    CreateNative("Han_IsWeaponSideAimable", Native_Han_IsWeaponSideAimable);
    CreateNative("Han_IsClientInspecting", Native_Han_IsClientInspecting);
    CreateNative("Han_IsWeaponInspectable", Native_Han_IsWeaponInspectable);
    CreateNative("Han_IsClientRunning", Native_Han_IsClientRunning);
    CreateNative("Han_SetClientCustomAnim", Native_Han_SetClientCustomAnim);
    CreateNative("Han_IsClientCustomAnim", Native_Han_IsClientCustomAnim);
    CreateNative("Han_StopClientCustomAnim", Native_Han_StopClientCustomAnim);

    g_ForwardClientZoom = new GlobalForward("Han_OnClientZoom", ET_Ignore, Param_Cell, Param_Cell);
    g_ForwardClientSideAim = new GlobalForward("Han_OnClientSideAim", ET_Ignore, Param_Cell, Param_Cell);
    g_ForwardClientInspect = new GlobalForward("Han_OnClientInspect", ET_Ignore, Param_Cell, Param_Cell);
    g_ForwardClientRun = new GlobalForward("Han_OnClientRun", ET_Ignore, Param_Cell, Param_Cell);
    g_ForwardClientEmptyReload = new GlobalForward("Han_OnClientEmptyReload", ET_Ignore, Param_Cell);
    g_ForwardClientTacticalReload = new GlobalForward("Han_OnClientTacticalReload", ET_Ignore, Param_Cell);
    g_ForwardClientCustomAnimStart = new GlobalForward("Han_OnClientCustomAnimStart", ET_Ignore, Param_Cell, Param_Cell, Param_Cell);
    g_ForwardClientCustomAnimEnd = new GlobalForward("Han_OnClientCustomAnimEnd", ET_Ignore, Param_Cell);

    RegPluginLibrary("HanWeaponSystem");
}

public any Native_GetClientViewModel(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    int index = GetNativeCell(2);

    if (client < 1 || client > MaxClients || index < 0 || index > 1)
    {
        return -1;
    }

    return ClientVM[client][index];
}

public any Native_IsClientPressingAttack2(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    if (client < 1 || client > MaxClients)
        return false;
    return g_bPressingAttack2[client];
}

public any Native_IsWeaponAutoFire(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    if (client < 1 || client > MaxClients)
        return false;

    return g_bWeaponAutoFire[client];
}

public any Native_Han_IsClientZooming(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    if (client < 1 || client > MaxClients)
        return false;
    return g_bZooming[client];
}

public any Native_Han_IsWeaponZoomable(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    if (client < 1 || client > MaxClients || !IsClientInGame(client) || !IsPlayerAlive(client))
        return false;

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0)
        return false;

    return Zoom_IsWeaponZoomable(weapon);
}

public any Native_Han_IsClientSideAiming(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    if (client < 1 || client > MaxClients)
        return false;
    return g_bSideAiming[client];
}

public any Native_Han_IsWeaponSideAimable(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    if (client < 1 || client > MaxClients || !IsClientInGame(client) || !IsPlayerAlive(client))
        return false;

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0)
        return false;

    return Zoom_IsWeaponSideAimable(weapon);
}

public any Native_Han_IsClientInspecting(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    if (client < 1 || client > MaxClients)
        return false;
    return g_bInspecting[client];
}

public any Native_Han_IsWeaponInspectable(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    if (client < 1 || client > MaxClients || !IsClientInGame(client) || !IsPlayerAlive(client))
        return false;

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0)
        return false;

    return Inspect_IsWeaponInspectable(weapon);
}

public any Native_Han_IsClientRunning(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    if (client < 1 || client > MaxClients)
        return false;
    return g_bRunning[client];
}

public any Native_Han_SetClientCustomAnim(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    int seq = GetNativeCell(2);
    int frames = GetNativeCell(3);
    bool repeat = numParams >= 4 ? GetNativeCell(4) : false;
    bool interruptable = numParams >= 5 ? GetNativeCell(5) : true;

    return CustomAnim_Set(client, seq, frames, repeat, interruptable);
}

public any Native_Han_IsClientCustomAnim(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    if (client < 1 || client > MaxClients)
        return false;
    return g_bCustomAnim[client];
}

public any Native_Han_StopClientCustomAnim(Handle plugin, int numParams)
{
    int client = GetNativeCell(1);
    if (client < 1 || client > MaxClients)
        return 0;
    CustomAnim_End(client);
    return 0;
}