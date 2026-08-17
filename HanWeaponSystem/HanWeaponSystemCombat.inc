// ============================================================================
// >> 战斗系统(伤害/后坐力/攻速/换弹速度/击退)
// ============================================================================

// ============================================================================
// >> 伤害加成
// ============================================================================
public Action OnTakeDamage(int victim, int &attacker, int &inflictor, float &damage, int &damagetype)
{
    if (attacker <= 0 || attacker > MaxClients || !IsClientInGame(attacker) || !IsPlayerAlive(attacker))
        return Plugin_Continue;

    if (victim <= 0 || victim > MaxClients || !IsClientInGame(victim) || !IsPlayerAlive(victim))
        return Plugin_Continue;

    int weaponEnt = GetEntPropEnt(attacker, Prop_Send, "m_hActiveWeapon");
    if (weaponEnt == -1)
        return Plugin_Continue;

    char damageStr[32];
    if (GetWeaponConfigValue(weaponEnt, "damage", damageStr, sizeof(damageStr)))
    {
        float delta = 0.0;
        if (damageStr[0] == '*')
        {
            delta = StringToFloat(damageStr[1]);
            if (delta > 0.0)
            {
                damage *= delta;
                return Plugin_Changed;
            }
            else
            {
                damage = 0.0;
                return Plugin_Changed;
            }
        }
        else if (damageStr[0] == '/')
        {
            delta = StringToFloat(damageStr[1]);
            if (delta > 0.0)
            {
                damage /= delta;
                return Plugin_Changed;
            }
            else
            {
                damage = 0.0;
                return Plugin_Changed;
            }
        }
        else
        {
            if (damageStr[0] == '+' || damageStr[0] == '-')
            {
                delta = StringToFloat(damageStr);
            }
            else
            {
                Format(damageStr, sizeof(damageStr), "+%s", damageStr);
                delta = StringToFloat(damageStr);
            }
            damage += delta;
            if (damage < 0.0)
                damage = 0.0;
            return Plugin_Changed;
        }
    }

    return Plugin_Continue;
}

// ============================================================================
// >> 玩家指令(后坐力/换弹速度检测/机瞄触发)
// ============================================================================
public Action OnPlayerRunCmd(int client, int &buttons, int &impulse, float vel[3], float angles[3], int &weapon, int &subtype, int &cmdnum, int &tickcount, int &seed, int mouse[2])
{
    if (client <= 0 || client > MaxClients || !IsClientInGame(client) || !IsPlayerAlive(client))
        return Plugin_Continue;

    int WeaponIndex = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (WeaponIndex <= 0)
        return Plugin_Continue;

    // 后坐力
    char Recoil[64];
    if (GetWeaponConfigValue(WeaponIndex, "recoil", Recoil, sizeof(Recoil)))
    {
        float recoilFactor = StringToFloat(Recoil);
        float punch[3];
        GetEntPropVector(client, Prop_Send, "m_vecPunchAngle", punch);
        punch[0] *= recoilFactor;
        punch[1] *= recoilFactor;
        punch[2] *= recoilFactor;
        SetEntPropVector(client, Prop_Send, "m_vecPunchAngle", punch);
    }

    g_bPressingAttack2[client] = (buttons & IN_ATTACK2) != 0;

    // 换弹速度检测
    static bool s_ClientIsReloading[MAXPLAYERS + 1];
    char sWeapon[64];
    int iWeapon = Client_GetActiveWeaponName(client, sWeapon, sizeof(sWeapon));
    if (iWeapon == INVALID_ENT_REFERENCE)
        return Plugin_Continue;

    bool bIsReloading = Weapon_IsReloading(iWeapon);
    if (!bIsReloading && HasEntProp(iWeapon, Prop_Send, "m_reloadState") && GetEntProp(iWeapon, Prop_Send, "m_reloadState") > 0)
        bIsReloading = true;

    if (bIsReloading && !s_ClientIsReloading[client])
    {
        IncreaseReloadSpeed(client);
    }

    s_ClientIsReloading[client] = bIsReloading;

    // 机瞄触发
    Zoom_OnPlayerRunCmd(client, buttons, bIsReloading);

    // 检视触发(F键/impulse 100)
    Inspect_OnPlayerRunCmd(client, impulse, bIsReloading);

    // 奔跑触发(前进按住时点按Shift); 奔跑中剔除IN_SPEED/禁止换弹时返回 Plugin_Changed 透传按键修改
    Action runRet = Run_OnPlayerRunCmd(client, buttons, bIsReloading);
    if (runRet == Plugin_Changed)
    {
        return Plugin_Changed;
    }

    return Plugin_Continue;
}

// ============================================================================
// >> 攻速设置
// ============================================================================
public void Hook_OnPostThinkSpeed(int client)
{
    if (client <= 0 || client > MaxClients || !IsClientInGame(client) || !IsPlayerAlive(client))
        return;

    int buttons = GetClientButtons(client);
    if (!(buttons & IN_ATTACK) && !(buttons & IN_ATTACK2))
        return;

    int weaponEnt = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weaponEnt == INVALID_ENT_REFERENCE)
        return;

    char FireSpeed[64];
    if (!GetWeaponConfigValue(weaponEnt, "firespeed", FireSpeed, sizeof(FireSpeed)))
        return;

    char leftPart[32], rightPart[32];
    strcopy(leftPart, sizeof(leftPart), FireSpeed);

    SplitStrings(leftPart, ":", rightPart, sizeof(rightPart));

    if (rightPart[0] == '\0')
    {
        strcopy(rightPart, sizeof(rightPart), leftPart);
    }

    float speedLeft = NormalizeFireSpeed(ParseFireSpeed(leftPart));
    float speedRight = NormalizeFireSpeed(ParseFireSpeed(rightPart));

    if (buttons & IN_ATTACK)
    {
        float nextPrimary = GetEntPropFloat(weaponEnt, Prop_Send, "m_flNextPrimaryAttack");
        if (GetGameTime() >= nextPrimary)
        {
            g_fModifyNextAttack[client] = 1.0 / speedLeft;
        }
    }

    if (buttons & IN_ATTACK2)
    {
        float nextSecondary = GetEntPropFloat(weaponEnt, Prop_Send, "m_flNextSecondaryAttack");
        if (GetGameTime() >= nextSecondary)
        {
            g_fModifyNextAttack[client] = 1.0 / speedRight;
        }
    }
}

void SplitStrings(char[] input, const char[] delim, char[] outRight, int maxRight)
{
    int pos = FindCharInString(input, delim[0]);
    if (pos == -1)
    {
        outRight[0] = '\0';
    }
    else
    {
        strcopy(outRight, maxRight, input[pos + 1]);
        input[pos] = '\0';
    }
}

float NormalizeFireSpeed(float value)
{
    if (value < 0.0)
        return 1.0 / FloatAbs(value);
    if (value == 0.0)
        return 0.0001;
    return value;
}

float ParseFireSpeed(const char[] input)
{
    float base = 1.0;
    char deltaStr[32];

    if (input[0] == '*' || input[0] == '/')
    {
        SubStr(input, 1, deltaStr, sizeof(deltaStr));

        float delta = StringToFloat(deltaStr);
        if (delta <= 0.0)
            return 0.0;

        if (input[0] == '*')
            return base * delta;
        else
            return base / delta;
    }
    else
    {
        if (input[0] != '+' && input[0] != '-')
        {
            Format(deltaStr, sizeof(deltaStr), "+%s", input);
        }
        else
        {
            strcopy(deltaStr, sizeof(deltaStr), input);
        }
        float delta = StringToFloat(deltaStr);
        return base + delta;
    }
}

void SubStr(const char[] source, int start, char[] dest, int maxlen)
{
    int len = strlen(source);
    if (start >= len)
    {
        dest[0] = '\0';
        return;
    }
    int copylen = len - start;
    if (copylen >= maxlen)
        copylen = maxlen - 1;

    for (int i = 0; i < copylen; i++)
    {
        dest[i] = source[start + i];
    }
    dest[copylen] = '\0';
}

public void Hook_OnPostThinkPostSpeed(int client)
{
    if (g_fModifyNextAttack[client] <= 0.0)
        return;

    if (!IsClientInGame(client) || !IsPlayerAlive(client))
    {
        g_fModifyNextAttack[client] = 0.0;
        return;
    }

    int iWeapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (iWeapon != INVALID_ENT_REFERENCE)
    {
        float flNextPrimaryAttack = GetEntPropFloat(iWeapon, Prop_Send, "m_flNextPrimaryAttack");

        flNextPrimaryAttack -= GetGameTime();
        flNextPrimaryAttack *= g_fModifyNextAttack[client];
        flNextPrimaryAttack += GetGameTime();

        float flNextSecondaryAttack = GetEntPropFloat(iWeapon, Prop_Send, "m_flNextSecondaryAttack");

        flNextSecondaryAttack -= GetGameTime();
        flNextSecondaryAttack *= g_fModifyNextAttack[client];
        flNextSecondaryAttack += GetGameTime();

        SetEntPropFloat(iWeapon, Prop_Send, "m_flNextPrimaryAttack", flNextPrimaryAttack);
        SetEntPropFloat(iWeapon, Prop_Send, "m_flNextSecondaryAttack", flNextSecondaryAttack);
    }

    g_fModifyNextAttack[client] = 0.0;
}

// ============================================================================
// >> 换子弹速度
// ============================================================================
void IncreaseReloadSpeed(int client)
{
    if (client <= 0 || client > MaxClients || !IsClientInGame(client) || !IsPlayerAlive(client))
        return;

    if (IsFakeClient(client))
        return;

    // 空仓/战术换弹时长同步接管中: 由 EmptyReload 模块按实测时长/兜底帧数控制换弹节奏
    if (g_bEmptyReloadTiming[client])
        return;

    char sWeapon[64];
    int iWeapon = Client_GetActiveWeaponName(client, sWeapon, sizeof(sWeapon));

    if (iWeapon == INVALID_ENT_REFERENCE)
        return;

    bool bIsShotgun = HasEntProp(iWeapon, Prop_Send, "m_reloadState");
    if (bIsShotgun)
    {
        int iReloadState = GetEntProp(iWeapon, Prop_Send, "m_reloadState");
        if (iReloadState == 0)
            return;
    }

    float fNextAttack = GetEntPropFloat(iWeapon, Prop_Send, "m_flNextPrimaryAttack");
    float fGameTime = GetGameTime();

    char ReloadSpeed[64];
    if (!GetWeaponConfigValue(iWeapon, "reloadspeed", ReloadSpeed, sizeof(ReloadSpeed)))
        return;

    float reloadSpeed = StringToFloat(ReloadSpeed);
    if (reloadSpeed <= 0.0)
        reloadSpeed = 1.0;

    float fReloadIncrease = reloadSpeed;

    SetEntPropFloat(iWeapon, Prop_Send, "m_flPlaybackRate", 1.0 / fReloadIncrease);

    int iViewModel = GetEntPropEnt(client, Prop_Send, "m_hViewModel");
    if (iViewModel != INVALID_ENT_REFERENCE)
        SetEntPropFloat(iViewModel, Prop_Send, "m_flPlaybackRate", 1.0 / fReloadIncrease);

    float fNextAttackNew = (fNextAttack - fGameTime) * fReloadIncrease;

    if (bIsShotgun)
    {
        DataPack hData;
        CreateDataTimer(0.01, Timer_CheckShotgunEnd, hData, TIMER_FLAG_NO_MAPCHANGE | TIMER_REPEAT);
        hData.WriteCell(EntIndexToEntRef(iWeapon));
        hData.WriteCell(GetClientUserId(client));
    }
    else
    {
        DataPack hData;
        CreateDataTimer(fNextAttackNew, Timer_ResetPlaybackRate, hData, TIMER_FLAG_NO_MAPCHANGE);
        hData.WriteCell(EntIndexToEntRef(iWeapon));
        hData.WriteCell(GetClientUserId(client));
    }

    fNextAttackNew += fGameTime;
    SetEntPropFloat(iWeapon, Prop_Send, "m_flTimeWeaponIdle", fNextAttackNew);
    SetEntPropFloat(iWeapon, Prop_Send, "m_flNextPrimaryAttack", fNextAttackNew);
    SetEntPropFloat(client, Prop_Send, "m_flNextAttack", fNextAttackNew);
    SetEntPropFloat(iWeapon, Prop_Send, "m_flNextSecondaryAttack", fNextAttackNew);
}

public Action Timer_CheckShotgunEnd(Handle timer, DataPack data)
{
    data.Reset();

    int iWeapon = EntRefToEntIndex(data.ReadCell());
    int client = GetClientOfUserId(data.ReadCell());

    if (iWeapon == INVALID_ENT_REFERENCE)
    {
        if (client > 0)
            ResetClientViewModel(client);
        return Plugin_Stop;
    }

    int iOwner = Weapon_GetOwner(iWeapon);
    if (iOwner <= 0)
    {
        if (client > 0)
            ResetClientViewModel(client);

        SetEntPropFloat(iWeapon, Prop_Send, "m_flPlaybackRate", 1.0);

        return Plugin_Stop;
    }

    int iReloadState = GetEntProp(iWeapon, Prop_Send, "m_reloadState");

    if (iReloadState > 0)
        return Plugin_Continue;

    SetEntPropFloat(iWeapon, Prop_Send, "m_flPlaybackRate", 1.0);

    if (client > 0)
        ResetClientViewModel(client);

    return Plugin_Stop;
}

// 霰弹枪换弹加速
public void Hook_OnReloadPost(int weapon, bool bSuccessful)
{
    int client = Weapon_GetOwner(weapon);
    if (client <= 0)
        return;

    if (GetEntProp(weapon, Prop_Send, "m_reloadState") != 2)
        return;

    if (IsFakeClient(client))
        return;

    char ReloadSpeed[64];
    if (!GetWeaponConfigValue(weapon, "reloadspeed", ReloadSpeed, sizeof(ReloadSpeed)))
        return;

    float reloadSpeed = StringToFloat(ReloadSpeed);
    if (reloadSpeed <= 0.0)
        reloadSpeed = 1.0;

    float fReloadIncrease = reloadSpeed;

    float fIdleTime = GetEntPropFloat(weapon, Prop_Send, "m_flTimeWeaponIdle");
    float fGameTime = GetGameTime();
    float fIdleTimeNew = (fIdleTime - fGameTime) * fReloadIncrease + fGameTime;
    SetEntPropFloat(weapon, Prop_Send, "m_flTimeWeaponIdle", fIdleTimeNew);
}

void ResetClientViewModel(int client)
{
    int iViewModel = GetEntPropEnt(client, Prop_Send, "m_hViewModel");
    if (iViewModel != INVALID_ENT_REFERENCE)
        SetEntPropFloat(iViewModel, Prop_Send, "m_flPlaybackRate", 1.0);
}

public Action Timer_ResetPlaybackRate(Handle timer, DataPack data)
{
    data.Reset();

    int iWeapon = EntRefToEntIndex(data.ReadCell());
    int client = GetClientOfUserId(data.ReadCell());

    if (iWeapon != INVALID_ENT_REFERENCE)
        SetEntPropFloat(iWeapon, Prop_Send, "m_flPlaybackRate", 1.0);

    if (client > 0)
        ResetClientViewModel(client);

    return Plugin_Stop;
}

void Hookshotguns()
{
    int iEntities = GetMaxEntities();
    for (int i = MaxClients + 1; i <= iEntities; i++)
    {
        if (IsValidEntity(i) && HasEntProp(i, Prop_Send, "m_reloadState"))
        {
            SDKHook(i, SDKHook_ReloadPost, Hook_OnReloadPost);
        }
    }
}

// ============================================================================
// >> 击退效果
// ============================================================================
public void BlockKnowBack(int victim, int attacker, int inflictor, float damage, int damagetype, int weapon, const float damageForce[3], const float damagePosition[3])
{
    if (attacker <= 0 || attacker > MaxClients || !IsClientInGame(attacker) || !IsPlayerAlive(attacker))
        return;

    if (victim <= 0 || victim > MaxClients || !IsClientInGame(victim) || !IsPlayerAlive(victim))
        return;

    int weaponEnt = GetEntPropEnt(attacker, Prop_Send, "m_hActiveWeapon");
    if (weaponEnt == -1)
        return;

    if (GetClientTeam(attacker) == GetClientTeam(victim) || attacker == victim)
        return;

    char KnowBack[32];
    if (!GetWeaponConfigValue(weaponEnt, "knowback", KnowBack, sizeof(KnowBack)))
        return;

    float knowback = StringToFloat(KnowBack);

    float fVictimOrigin[3], fAttackerOrigin[3], fDirection[3];
    GetClientEyePosition(victim, fVictimOrigin);
    GetClientEyePosition(attacker, fAttackerOrigin);
    MakeVectorFromPoints(fAttackerOrigin, fVictimOrigin, fDirection);
    NormalizeVector(fDirection, fDirection);
    float fForce = knowback;
    ScaleVector(fDirection, fForce);
    TeleportEntity(victim, NULL_VECTOR, NULL_VECTOR, fDirection);
}