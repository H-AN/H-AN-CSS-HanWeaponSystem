// ============================================================================
// >> 音效系统
// ============================================================================

// ============================================================================
// >> 射击音效
// ============================================================================
public Action WeaponFireBullets(const char[] te_name, const int[] Players, int numClients, float delay)
{
    int client = TE_ReadNum("m_iPlayer") + 1;
    if (!IsClientInGame(client))
        return Plugin_Continue;

    int weaponEnt = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weaponEnt == -1)
        return Plugin_Continue;

    char classname[64];
    GetEdictClassname(weaponEnt, classname, sizeof(classname));

    PlayWeaponSound(client, classname, "firesound");

    return Plugin_Continue;
}

// ============================================================================
// >> 音效播放
// ============================================================================
void PlayWeaponSound(int client, const char[] weaponClassname, const char[] key)
{
    Handle kvEntry;
    if (!GetTrieValue(g_hWeaponTrie, weaponClassname, kvEntry))
        return;

    char soundPath[PLATFORM_MAX_PATH];
    KvRewind(kvEntry);
    KvGetString(kvEntry, key, soundPath, sizeof(soundPath), "");
    NormalizePath(soundPath);
    if (strlen(soundPath) > 0)
    {
        EmitSoundToAll(soundPath, client, SNDCHAN_WEAPON, SNDLEVEL_ROCKET);
        EmitSoundToAll(soundPath, client, SNDCHAN_STATIC, SNDLEVEL_NORMAL);
    }
}

// ============================================================================
// >> 刀音效替换
// ============================================================================
public Action SoundCallBackHook(int clients[64], int &numClients, char sample[PLATFORM_MAX_PATH], int &entity, int &channel, float &volume, int &level, int &pitch, int &flags)
{
    if (entity <= 0)
        return Plugin_Continue;

    char Classname[64];
    if (!GetEntityClassname(entity, Classname, sizeof(Classname)))
        return Plugin_Continue;

    if (StrContains(Classname, "weapon_") != 0)
        return Plugin_Continue;

    int owner = GetEntPropEnt(entity, Prop_Send, "m_hOwnerEntity");

    if (owner <= 0 || owner > MaxClients || !IsClientInGame(owner) || !IsPlayerAlive(owner))
        return Plugin_Continue;

    int weapon = GetEntPropEnt(owner, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0)
        return Plugin_Continue;

    char classname[32];
    GetEdictClassname(weapon, classname, sizeof(classname));

    if (GetClientTeam(owner) == CS_TEAM_CT)
    {
        char prefix[64];
        if (!GetCustomSoundPrefixct(classname, prefix, sizeof(prefix)))
            return Plugin_Continue;

        if (StrContains(sample, "weapons/knife/") == 0)
        {
            ReplaceString(sample, sizeof(sample), "knife", prefix);
            EmitSoundToAll(sample, owner, channel, level, flags, volume);
            return Plugin_Handled;
        }
    }
    else if (GetClientTeam(owner) == CS_TEAM_T)
    {
        char prefix[64];
        if (!GetCustomSoundPrefixt(classname, prefix, sizeof(prefix)))
            return Plugin_Continue;

        if (StrContains(sample, "weapons/knife/") == 0)
        {
            ReplaceString(sample, sizeof(sample), "knife", prefix);
            EmitSoundToAll(sample, owner, channel, level, flags, volume);
            return Plugin_Handled;
        }
    }

    return Plugin_Continue;
}

// ============================================================================
// >> 换子弹音效
// ============================================================================
public void Hook_WeaponReloadPost(int weapon, bool bSuccessful)
{
    int owner = GetEntPropEnt(weapon, Prop_Send, "m_hOwnerEntity");
    if (owner <= 0 || !IsClientInGame(owner))
        return;

    // 空仓/战术换弹动画接管(此处 m_iClip1 仍是换弹前值, 正好判定空仓/战术)
    EmptyReload_OnWeaponReloadPost(weapon);

    int ammo = GetEntProp(weapon, Prop_Data, "m_iClip1");

    int WeaponAmmo = GetEntProp(weapon, Prop_Data, "m_iPrimaryAmmoType");
    int magazine;
    if (WeaponAmmo != -1)
    {
        magazine = GetEntProp(owner, Prop_Data, "m_iAmmo", 4, WeaponAmmo);
    }

    int maxAmmo = 0;
    char maxAmmoStr[32];
    if (GetWeaponConfigValue(weapon, "maxammo", maxAmmoStr, sizeof(maxAmmoStr)))
    {
        maxAmmo = StringToInt(maxAmmoStr);

        if (ammo < maxAmmo && magazine > 0)
        {
            char reloadStr[256];
            if (GetWeaponConfigValue(weapon, "reloadsound", reloadStr, sizeof(reloadStr)))
            {
                char sounds[8][64];
                int count = ExplodeString(reloadStr, ",", sounds, sizeof(sounds), sizeof(sounds[]));

                for (int i = 0; i < count; i++)
                {
                    char pair[2][64];
                    if (ExplodeString(sounds[i], ":", pair, sizeof(pair), sizeof(pair[])) == 2)
                    {
                        float delay = StringToFloat(pair[0]);
                        char soundPath[PLATFORM_MAX_PATH];
                        strcopy(soundPath, sizeof(soundPath), pair[1]);

                        DataPack dp;
                        CreateDataTimer(delay, Timer_PlayReloadSound, dp);
                        dp.WriteCell(owner);
                        dp.WriteString(soundPath);
                    }
                }
            }
        }
    }
}

public Action Timer_PlayReloadSound(Handle timer, DataPack dp)
{
    dp.Reset();
    int owner = dp.ReadCell();
    char soundPath[PLATFORM_MAX_PATH];
    dp.ReadString(soundPath, sizeof(soundPath));

    if (IsClientInGame(owner))
    {
        EmitSoundToAll(soundPath, owner, SNDCHAN_WEAPON, SNDLEVEL_ROCKET);
        EmitSoundToAll(soundPath, owner, SNDCHAN_STATIC, SNDLEVEL_NORMAL);
    }
    return Plugin_Handled;
}