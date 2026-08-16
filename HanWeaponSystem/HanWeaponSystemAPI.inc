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

    g_ForwardClientZoom = new GlobalForward("Han_OnClientZoom", ET_Ignore, Param_Cell, Param_Cell);

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