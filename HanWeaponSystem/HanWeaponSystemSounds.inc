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

    // 手雷投射物: 自定义音效文件夹同名文件替换(弹跳/爆炸)
    int clen = strlen(Classname);
    if (clen >= 11 && StrContains(Classname, "_projectile") == clen - 11)
    {
        return GrenadePrefixSound(entity, sample, channel, level, flags, volume);
    }

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
// >> 手雷拉保险音(OnPlayerRunCmd 上升沿: 按下 IN_ATTACK 瞬间播一次)
//    活动武器为自定义手雷且配了前缀时, 播 前缀文件夹/pinpull.wav
// ============================================================================
public void Sounds_OnPlayerRunCmd(int client, int &buttons)
{
    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    bool bIsGrenade = (weapon > 0 && IsValidEntity(weapon) && IsCustomGrenadeWeapon(weapon));

    bool bAttack = (buttons & IN_ATTACK) != 0;

    if (bIsGrenade && bAttack && !g_bGrenadePullPrev[client])
    {
        char classname[64];
        GetEdictClassname(weapon, classname, sizeof(classname));

        char prefix[64];
        if (GetClientTeam(client) == CS_TEAM_CT)
        {
            if (!GetCustomSoundPrefixct(classname, prefix, sizeof(prefix)))
                prefix[0] = '\0';
        }
        else
        {
            if (!GetCustomSoundPrefixt(classname, prefix, sizeof(prefix)))
                prefix[0] = '\0';
        }

        if (strlen(prefix) > 0)
        {
            char soundPath[PLATFORM_MAX_PATH];
            Format(soundPath, sizeof(soundPath), "weapons/%s/pinpull.wav", prefix);
            EmitSoundToAll(soundPath, client, SNDCHAN_WEAPON, SNDLEVEL_NORMAL);
        }
    }

    // 只有持自定义手雷且按下时才记录, 换武器/松手即重置 → 下次按下必触发
    g_bGrenadePullPrev[client] = (bIsGrenade && bAttack);
}

// ============================================================================
// >> 判断是否为自定义手雷武器(反查配置 classname 为基础手雷类型)
// ============================================================================
bool IsCustomGrenadeWeapon(int weapon)
{
    char base[64];
    if (!GetWeaponConfigValue(weapon, "classname", base, sizeof(base)))
        return false;

    return (StrEqual(base, "weapon_hegrenade", false) ||
            StrEqual(base, "weapon_flashbang", false) ||
            StrEqual(base, "weapon_smokegrenade", false));
}

// ============================================================================
// >> 手雷投射物音效文件夹替换(customsoundt/ct 前缀文件夹同名文件替换)
//    拦截 hegrenade/flashbang/smokegrenade 投射物默认音(弹跳/爆炸),
//    改写为 自定义文件夹 的同名文件; 原版雷/未配前缀 放行走默认
// ============================================================================
Action GrenadePrefixSound(int entity, const char[] sample, int channel, int level, int flags, float volume)
{
    if (g_GrenadePrefix[entity][0] == '\0')
        return Plugin_Continue;

    // 剥掉开头的 ^ 标记(引擎"直接播放已预缓存音"标记, 爆炸等音效会带)
    char cleanSample[PLATFORM_MAX_PATH];
    if (sample[0] == '^')
        strcopy(cleanSample, sizeof(cleanSample), sample[1]);
    else
        strcopy(cleanSample, sizeof(cleanSample), sample);

    if (StrContains(cleanSample, "weapons/hegrenade/") != 0 &&
        StrContains(cleanSample, "weapons/flashbang/") != 0 &&
        StrContains(cleanSample, "weapons/smokegrenade/") != 0)
        return Plugin_Continue;

    // 改写文件夹: weapons/hegrenade/xxx.wav -> weapons/<prefix>/xxx.wav
    char newSample[PLATFORM_MAX_PATH];
    strcopy(newSample, sizeof(newSample), cleanSample);

    int start = strlen("weapons/");
    int end = FindCharInString(newSample[start], '/');
    if (end == -1)
        return Plugin_Continue;

    char fileName[PLATFORM_MAX_PATH];
    strcopy(fileName, sizeof(fileName), newSample[start + end + 1]);

    Format(newSample, sizeof(newSample), "weapons/%s/%s", g_GrenadePrefix[entity], fileName);

    EmitSoundToAll(newSample, entity, channel, level, flags, volume);
    return Plugin_Handled;
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

    // 满仓判定: 弹夹容量来自预缓存的武器脚本 clip_size(GetWeaponMaxAmmo), 满弹夹(ammo>=maxAmmo)或备用弹匣为空不播换弹音效
    int maxAmmo = GetWeaponMaxAmmo(weapon);

    if (ammo < maxAmmo && magazine > 0)
    {
        char soundStr[256];
        if (SelectReloadSound(weapon, ammo, soundStr, sizeof(soundStr)))
            ScheduleReloadSounds(owner, weapon, soundStr);
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