// 由实际模型切换路径确认状态，以 EntRef 防止实体槽位复用。
int g_HanModelWeapon[MAXPLAYERS + 1];
HanViewModelMode g_HanModelMode[MAXPLAYERS + 1];

void Han_ModelReset(int client)
{
    ViewModel_ResetShootQcFix(client);
    g_HanModelWeapon[client] = INVALID_ENT_REFERENCE;
    g_HanModelMode[client] = HanViewModel_NotReady;
}

void Han_ModelConfirm(int client, int weapon, HanViewModelMode mode)
{
    g_HanModelWeapon[client] = EntIndexToEntRef(weapon);
    g_HanModelMode[client] = mode;
}

HanViewModelMode Han_ModelMode(int client)
{
    if (client < 1 || client > MaxClients || !IsClientInGame(client) || !IsPlayerAlive(client))
        return HanViewModel_NotReady;
    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= MaxClients || EntRefToEntIndex(g_HanModelWeapon[client]) != weapon)
        return HanViewModel_NotReady;
    int index = view_as<int>(g_HanModelMode[client]);
    if (index < 0 || ClientVM[client][index] <= MaxClients || !IsValidEntity(ClientVM[client][index]))
        return HanViewModel_NotReady;
    return g_HanModelMode[client];
}

bool Han_WeaponBase(int weapon, char[] buffer, int maxlen)
{
    buffer[0] = '\0';
    if (weapon <= MaxClients || !IsValidEntity(weapon))
        return false;
    return GetWeaponConfigValue(weapon, "classname", buffer, maxlen);
}
