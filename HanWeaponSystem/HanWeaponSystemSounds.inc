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
    if (strlen(soundPath) == 0)
        return;

    // 多路径逗号分隔 → 每次随机选一个播放
    char parts[8][PLATFORM_MAX_PATH];
    int count = ExplodeString(soundPath, ",", parts, sizeof(parts), sizeof(parts[]));

    int valid[8];
    int validCount = 0;
    for (int i = 0; i < count; i++)
    {
        TrimString(parts[i]);
        if (strlen(parts[i]) > 0)
        {
            valid[validCount] = i;
            validCount++;
        }
    }

    if (validCount == 0)
        return;

    strcopy(soundPath, sizeof(soundPath), parts[valid[GetRandomInt(0, validCount - 1)]]);
    NormalizePath(soundPath);

    EmitSoundToAll(soundPath, client, SNDCHAN_WEAPON, SNDLEVEL_ROCKET);
    EmitSoundToAll(soundPath, client, SNDCHAN_STATIC, SNDLEVEL_NORMAL);
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
            char soundStr[256];
            if (SelectReloadSound(weapon, ammo, soundStr, sizeof(soundStr)))
                ScheduleReloadSounds(owner, weapon, soundStr);
        }
    }
}

// ============================================================================
// >> 选择换弹音效来源(按弹匣状态匹配 空仓/战术/兜底)
//    空仓(clip==0): 该武器启用空仓换弹 → 播 emptyreloadsound(没填则不播); 否则兜底主配置 reloadsound
//    战术(clip>0):  该武器启用战术换弹 → 播 tacticalreloadsound(没填则不播); 否则兜底主配置 reloadsound
// ============================================================================
bool SelectReloadSound(int weapon, int clip, char[] out, int outLen)
{
    if (clip == 0)
    {
        if (EmptyReload_IsEmptyEnabled(weapon))
        {
            GetEmptyReloadConfigValue(weapon, "emptyreloadsound", out, outLen);
            return (strlen(out) > 0);
        }
        return GetWeaponConfigValue(weapon, "reloadsound", out, outLen);
    }

    if (EmptyReload_IsTacticalEnabled(weapon))
    {
        GetEmptyReloadConfigValue(weapon, "tacticalreloadsound", out, outLen);
        return (strlen(out) > 0);
    }
    return GetWeaponConfigValue(weapon, "reloadsound", out, outLen);
}

// ============================================================================
// >> 调度换弹音效(帧号:路径,逗号分隔; delay = 帧号 × reloadspeed / reloadfps)
// ============================================================================
void ScheduleReloadSounds(int owner, int weapon, const char[] soundStr)
{
    char sounds[8][64];
    int count = ExplodeString(soundStr, ",", sounds, sizeof(sounds), sizeof(sounds[]));

    // 帧号 → 秒: delay = 帧号 × reloadspeed / reloadfps (reloadfps 默认 30, 见空仓换弹配置)
    float reloadSpeed = 1.0;
    char reloadSpeedStr[16];
    if (GetWeaponConfigValue(weapon, "reloadspeed", reloadSpeedStr, sizeof(reloadSpeedStr)))
    {
        float v = StringToFloat(reloadSpeedStr);
        if (v > 0.0)
            reloadSpeed = v;
    }

    int reloadFps = EmptyReload_GetInt(weapon, "reloadfps", 30);
    if (reloadFps <= 0)
        reloadFps = 30;

    for (int i = 0; i < count; i++)
    {
        char pair[2][64];
        if (ExplodeString(sounds[i], ":", pair, sizeof(pair), sizeof(pair[])) == 2)
        {
            int frame = StringToInt(pair[0]);
            float delay = (float(frame) * reloadSpeed) / float(reloadFps);
            char soundPath[PLATFORM_MAX_PATH];
            strcopy(soundPath, sizeof(soundPath), pair[1]);
            NormalizePath(soundPath);

            DataPack dp;
            CreateDataTimer(delay, Timer_PlayReloadSound, dp);
            dp.WriteCell(owner);
            dp.WriteCell(EntIndexToEntRef(weapon));
            dp.WriteString(soundPath);
        }
    }
}

public Action Timer_PlayReloadSound(Handle timer, DataPack dp)
{
    dp.Reset();
    int owner = dp.ReadCell();
    int weaponRef = dp.ReadCell();
    char soundPath[PLATFORM_MAX_PATH];
    dp.ReadString(soundPath, sizeof(soundPath));

    if (!IsClientInGame(owner))
        return Plugin_Handled;

    // 换弹被打断/切枪: 武器不在手上则不再播放剩余音效(无需 kill timer, 零管理开销)
    int weapon = EntRefToEntIndex(weaponRef);
    if (weapon == -1 || GetEntPropEnt(owner, Prop_Send, "m_hActiveWeapon") != weapon)
        return Plugin_Handled;

    EmitSoundToAll(soundPath, owner, SNDCHAN_WEAPON, SNDLEVEL_ROCKET);
    EmitSoundToAll(soundPath, owner, SNDCHAN_STATIC, SNDLEVEL_NORMAL);
    return Plugin_Handled;
}