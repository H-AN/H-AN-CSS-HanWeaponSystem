// ============================================================================
// >> 武器给予/装备/世界模型
// ============================================================================

// ============================================================================
// >> 武器生成命令
// ============================================================================
public Action WeaponCommandCallback(int client, int args)
{
    if (!IsClientInGame(client) || !IsPlayerAlive(client))
        return Plugin_Handled;

    char cmdName[64];
    GetCmdArg(0, cmdName, sizeof(cmdName));

    for (int i = 0; i < g_WeaponCount; i++)
    {
        if (StrEqual(cmdName, g_WeaponCommand[i], false))
        {
            int team = GetClientTeam(client);
            if (StrEqual(g_WeaponTeam[i], "t") && team != CS_TEAM_T)
                return Plugin_Handled;
            if (StrEqual(g_WeaponTeam[i], "ct") && team != CS_TEAM_CT)
                return Plugin_Handled;

            GiveWeaponByIndex(client, i);
            break;
        }
    }
    return Plugin_Handled;
}

// ============================================================================
// >> 给予武器
// ============================================================================
void GiveWeaponByIndex(int client, int index)
{
    int ent = CreateEntityByName(g_WeaponClassname[index]);
    if (ent == -1)
        return;

    DispatchKeyValue(ent, "classname", g_WeaponUseClassname[index]);
    DispatchKeyValue(ent, "ammo", g_WeaponAmmo[index]);

    if (!DispatchSpawn(ent))
        return;

    int active = GetPlayerWeaponSlot(client, 0);
    if (active != -1)
    {
        char sClass[64];
        GetEdictClassname(active, sClass, sizeof(sClass));

        if (!StrEqual(sClass, g_WeaponUseClassname[index], false))
            EquipPlayerWeapon(client, ent);
    }
    else
    {
        EquipPlayerWeapon(client, ent);
    }

    int WeaponAmmo = GetEntProp(ent, Prop_Data, "m_iPrimaryAmmoType");
    if (WeaponAmmo != -1)
    {
        int curAmmo = GetEntProp(client, Prop_Data, "m_iAmmo", 4, WeaponAmmo);
        int ammoVal = StringToInt(g_WeaponAmmo[index]);
        if (curAmmo < ammoVal)
        {
            SetEntProp(client, Prop_Data, "m_iAmmo", ammoVal, 4, WeaponAmmo);
        }

        SDKHook(ent, SDKHook_ReloadPost, Hook_WeaponReloadPost);
    }
}

// ============================================================================
// >> 武器切换音效
// ============================================================================
public Action WeaponHook(int client, int weapon)
{
    if (client <= 0 || client > MaxClients || !IsClientInGame(client) || !IsPlayerAlive(client))
        return Plugin_Continue;

    int weaponEnt = GetEntPropEnt(client, Prop_Data, "m_hActiveWeapon");
    if (weaponEnt == -1)
        return Plugin_Continue;

    CreateTimer(0.0, ChangeModelIndex, client);

    return Plugin_Continue;
}

public Action ChangeModelIndex(Handle timer, int client)
{
    if (client <= 0 || client > MaxClients || !IsClientInGame(client) || !IsPlayerAlive(client))
        return Plugin_Continue;

    int weaponEnt = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weaponEnt == -1)
        return Plugin_Continue;

    char classname[64];
    GetEdictClassname(weaponEnt, classname, sizeof(classname));

    PlayWeaponSound(client, classname, "switchsound");
    SDKHook(weaponEnt, SDKHook_SetTransmit, SetTransmit_CallBack);

    return Plugin_Continue;
}

public Action SetTransmit_CallBack(int entity, int viewer)
{
    int owner = GetEntPropEnt(entity, Prop_Send, "m_hOwnerEntity");
    if (viewer > 0 && viewer <= MaxClients)
    {
        int weapons = GetEntPropEnt(viewer, Prop_Send, "m_hActiveWeapon");
        if (owner == viewer && weapons == GetPlayerWeaponSlot(viewer, 2))
            return Plugin_Handled;
    }
    if (owner > 0 && owner <= MaxClients && (!IsClientInGame(owner)))
        SDKUnhook(entity, SDKHook_SetTransmit, SetTransmit_CallBack);
    return Plugin_Continue;
}

// ============================================================================
// >> W模型设置
// ============================================================================
public void WeaponSwitchPost(int client, int iActiveWeapon)
{
    if (iActiveWeapon <= 0)
        return;

    char ClassName[64];
    GetEdictClassname(iActiveWeapon, ClassName, sizeof(ClassName));

    char wmodelpath[PLATFORM_MAX_PATH];
    if (GetWeaponConfigValue(iActiveWeapon, "wmodel", wmodelpath, sizeof(wmodelpath)))
    {
        int modelIndex = PrecacheModel(wmodelpath, true);
        if (!StrEqual("weapon_knife", ClassName, false))
        {
            SetEntProp(iActiveWeapon, Prop_Send, "m_iWorldModelIndex", modelIndex);
        }
    }
}

public void OnWeaponDrop(int client, int weapon)
{
    if (!IsValidEntity(weapon))
        return;

    CreateTimer(0.0, ChangeWmodel, EntIndexToEntRef(weapon));
}

public Action ChangeWmodel(Handle timer, int entityRef)
{
    int weapon = EntRefToEntIndex(entityRef);
    if (!IsValidEntity(weapon))
        return Plugin_Continue;

    char ClassName[64];
    GetEdictClassname(weapon, ClassName, sizeof(ClassName));

    char useDropped[8];

    char wmodelpath[PLATFORM_MAX_PATH];
    if (GetWeaponConfigValue(weapon, "wmodel", wmodelpath, sizeof(wmodelpath)))
    {
        if (!StrEqual("weapon_knife", ClassName, false))
        {
            int modelIndex = PrecacheModel(wmodelpath, true);
            if (GetWeaponConfigValue(weapon, "usedroppedmodel", useDropped, sizeof(useDropped)))
            {
                int mode = StringToInt(useDropped);
                if (mode > 0)
                {
                    CreateFakeWorldmod(weapon);
                }
                else
                {
                    SetEntProp(weapon, Prop_Send, "m_iWorldModelIndex", modelIndex);
                }
            }
            else
            {
                SetEntProp(weapon, Prop_Send, "m_iWorldModelIndex", modelIndex);
            }
        }
    }
    return Plugin_Continue;
}

// ============================================================================
// >> 掉落伪世界模型
// ============================================================================
void CreateFakeWorldmod(int entity)
{
    if (entity <= 0 || !IsValidEntity(entity))
        return;

    if (IsValidEntity(g_FakeModels[entity]) && g_FakeModels[entity] > 0)
    {
        AcceptEntityInput(g_FakeModels[entity], "Kill");
        g_FakeModels[entity] = 0;
    }

    char ClassName[64];
    GetEdictClassname(entity, ClassName, sizeof(ClassName));

    if (StrEqual(ClassName, "weapon_knife", false))
        return;

    char wmodelpath[PLATFORM_MAX_PATH];
    char SkinCode[32];
    int skin = 0;

    if (GetWeaponConfigValue(entity, "wmodel", wmodelpath, sizeof(wmodelpath)))
    {
        if (GetWeaponConfigValue(entity, "skincode", SkinCode, sizeof(SkinCode)))
        {
            skin = StringToInt(SkinCode);
        }
        else
        {
            skin = 0;
        }

        int fakeEnt = CreateEntityByName("prop_dynamic_override");
        if (!IsValidEntity(fakeEnt))
            return;

        DispatchKeyValue(fakeEnt, "model", wmodelpath);
        DispatchKeyValue(fakeEnt, "Solid", "6");
        DispatchSpawn(fakeEnt);

        SetEntProp(fakeEnt, Prop_Send, "m_nSkin", skin);

        SetEntData(fakeEnt, offsCollision, 2, 1, true);

        float pos[3];
        GetEntPropVector(entity, Prop_Data, "m_vecOrigin", pos);
        TeleportEntity(fakeEnt, pos, NULL_VECTOR, NULL_VECTOR);

        SetVariantString("!activator");
        AcceptEntityInput(fakeEnt, "SetParent", entity);

        int EntEffects = GetEntProp(entity, Prop_Send, "m_fEffects");
        EntEffects |= EF_NODRAW;
        SetEntProp(entity, Prop_Send, "m_fEffects", EntEffects);

        g_FakeModels[entity] = fakeEnt;
    }
}

public Action HideFakeWModel(int client, int weapon)
{
    if (weapon <= 0 || weapon >= 2048 || !IsValidEntity(weapon))
        return Plugin_Continue;

    int fake = g_FakeModels[weapon];

    if (fake > 0 && IsValidEntity(fake))
    {
        AcceptEntityInput(fake, "Kill");
        g_FakeModels[weapon] = 0;
    }

    return Plugin_Continue;
}