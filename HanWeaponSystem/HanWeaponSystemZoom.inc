// ============================================================================
// >> 机瞄系统(集成 IronSight 思路，全盘基于 1号VM)
// ============================================================================
// >> 原作者声明 / Original Author Credit
// 本模块的机瞄逻辑移植自开源社区插件 "IronSight Plugin"。
// This module's iron-sight/zoom logic is ported from the open-source plugin
// "IronSight Plugin".
//
// 原作者 / Original Authors: boss, cjsrk
// 版本 / Version: 0.8
//
// 感谢原作者的付出！原插件仅供学习参考，本插件已在此基础上大幅重构改写。
// Special thanks to the original authors for their work! The original plugin is
// used for reference only; this plugin has been heavily reworked on top of it.
//
// 按键说明：机瞄=右键(IN_ATTACK2)，侧瞄=中键(IN_ATTACK3)。狙击枪(awp/scout)右键为引擎原生开镜，
// 机瞄配置无效，仅侧瞄生效(中键触发)。两种瞄准互斥，机瞄中按中键瞬间切侧瞄，反之亦然。
// Key note: iron sight = right click (IN_ATTACK2), side aim = middle mouse (IN_ATTACK3). For
// awp/scout, right-click is the engine's native scope (zoom config ignored, side aim only via
// middle mouse). The two aim modes are mutually exclusive and instantly switchable.
// ============================================================================

void Zoom_OnPluginStart()
{
    HookUserMessage(GetUserMessageId("ReloadEffect"), Zoom_OnReloadEffect);
}

void Zoom_OnMapStart()
{
    // 仅预缓存不下载(下载统一由 Download.cfg 管理, 默认已含这两行)
    PrecacheDecal("materials/zoom/zoomin.vmt");
    PrecacheDecal("materials/zoom/zoomin.vtf");

    UnlockConsoleCommandAndConvar("r_screenoverlay");

    for (int i = 1; i <= MaxClients; i++)
    {
        if (IsClientInGame(i))
            Zoom_Reset(i);
    }
}

// ============================================================================
// >> 重置客户端机瞄/侧瞄状态(换图/断线等场景，恢复模型与相机残留)
// ============================================================================
void Zoom_Reset(int client)
{
    g_bZooming[client] = false;
    g_iZoomInSwitch[client] = 0;
    g_fZoomBlock[client] = 0.0;

    g_bSideAiming[client] = false;
    g_iSideAimInSwitch[client] = 0;
    g_fSideAimBlock[client] = 0.0;

    SetEntPropFloat(client, Prop_Data, "m_flLaggedMovementValue", 1.0);

    if (!IsFakeClient(client))
    {
        if (ClientVM[client][1] > 0 && IsValidEntity(ClientVM[client][1]))
            Camera_RemoveLink(ClientVM[client][1], client, false);
        ClientCommand(client, "r_screenoverlay 0");
    }
}

// ============================================================================
// >> 判断武器是否可机瞄
// ============================================================================
bool Zoom_IsWeaponZoomable(int weapon)
{
    char val[8];
    if (GetZoomConfigValue(weapon, "zoom", val, sizeof(val)))
    {
        return StringToInt(val) > 0;
    }
    return false;
}

// ============================================================================
// >> 判断武器是否可侧瞄(sideaim 1 且填写了侧瞄模型才有效)
// ============================================================================
bool Zoom_IsWeaponSideAimable(int weapon)
{
    char val[8];
    if (!GetZoomConfigValue(weapon, "sideaim", val, sizeof(val)))
        return false;

    if (StringToInt(val) <= 0)
        return false;

    char model[PLATFORM_MAX_PATH];
    if (!GetZoomConfigValue(weapon, "sideaimmodel", model, sizeof(model)) || strlen(model) == 0)
        return false;

    return true;
}

// ============================================================================
// >> 判断是否为无弹药特殊武器(脚本 primary_ammo None → ammo type -1)
//    特殊武器不执行"没子弹/换弹强制退镜"逻辑
// ============================================================================
bool Zoom_IsNoAmmoWeapon(int weapon)
{
    if (weapon <= 0 || !IsValidEdict(weapon))
        return false;

    return GetEntProp(weapon, Prop_Data, "m_iPrimaryAmmoType") == -1;
}

// ============================================================================
// >> 判断武器是否基于引擎原生右键开镜的脚本(awp/scout)
//    实体 classname 已被改写为 useclassname(如 weapon_awm)，须按 useclassname
//    反查主配置的 classname 键，原始脚本为 weapon_awp/weapon_scout 才算。
//    这两把武器右键自带瞄准镜，机瞄不生效(填写zoom直接无效)，侧瞄由中键(IN_ATTACK3)触发
// ============================================================================
bool Zoom_IsNativeScopeWeapon(int weapon)
{
    if (weapon <= 0 || !IsValidEntity(weapon) || g_hWeaponConfig == INVALID_HANDLE)
        return false;

    char useClass[64];
    GetEntityClassname(weapon, useClass, sizeof(useClass));

    if (StrContains(useClass, "weapon_") == 0)
        strcopy(useClass, sizeof(useClass), useClass[7]);

    KvRewind(g_hWeaponConfig);
    if (!KvGotoFirstSubKey(g_hWeaponConfig))
        return false;

    do
    {
        char cfgUse[64];
        KvGetString(g_hWeaponConfig, "useclassname", cfgUse, sizeof(cfgUse), "");

        if (StrContains(cfgUse, "weapon_") == 0)
            strcopy(cfgUse, sizeof(cfgUse), cfgUse[7]);

        if (StrEqual(useClass, cfgUse, false))
        {
            char origClass[64];
            KvGetString(g_hWeaponConfig, "classname", origClass, sizeof(origClass), "");

            if (StrContains(origClass, "weapon_") == 0)
                strcopy(origClass, sizeof(origClass), origClass[7]);

            return (StrEqual(origClass, "awp", false) || StrEqual(origClass, "scout", false));
        }

    } while (KvGotoNextKey(g_hWeaponConfig));

    return false;
}

// ============================================================================
// >> 触发检测(由 Combat 的 OnPlayerRunCmd 调用)
// ============================================================================
void Zoom_OnPlayerRunCmd(int client, int buttons, bool bReloading)
{
    if (!IsPlayerAlive(client))
    {
        if (g_bZooming[client] || g_iZoomInSwitch[client] != 0 || g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0)
            Aim_ForceExit(client);
        return;
    }

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0)
        return;

    bool isNativeScope = Zoom_IsNativeScopeWeapon(weapon);

    // 换弹/换枪(deploy)期间不允许机瞄/侧瞄，正在瞄准则强制退出
    // 无弹药特殊武器(primary_ammo None)不受此限制
    if (!Zoom_IsNoAmmoWeapon(weapon) && (bReloading || GetEntPropFloat(client, Prop_Send, "m_flNextAttack") > GetGameTime()))
    {
        if (g_bZooming[client] || g_iZoomInSwitch[client] != 0 || g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0)
            Aim_ForceExit(client);
        return;
    }

    bool canZoom = !isNativeScope && Zoom_IsWeaponZoomable(weapon);
    bool canSideAim = Zoom_IsWeaponSideAimable(weapon);

    // 检视中按下瞄准键(机瞄=右键/侧瞄=中键) → 先打断检视, 下一帧瞄准逻辑接管(检视优先级最低)
    if (g_bInspecting[client] || g_bRunning[client])
    {
        if (g_bInspecting[client] && ((canZoom && (buttons & IN_ATTACK2)) || (canSideAim && (buttons & IN_ATTACK3))))
        {
            Inspect_End(client);
        }
        return;
    }

    static bool s_bPrevAttack2[MAXPLAYERS + 1];
    static bool s_bPrevAttack3[MAXPLAYERS + 1];
    bool pressAttack2 = (buttons & IN_ATTACK2) != 0;
    bool pressAttack3 = (buttons & IN_ATTACK3) != 0;
    bool risingAttack2 = pressAttack2 && !s_bPrevAttack2[client];
    bool risingAttack3 = pressAttack3 && !s_bPrevAttack3[client];
    s_bPrevAttack2[client] = pressAttack2;
    s_bPrevAttack3[client] = pressAttack3;

    // 机瞄(右键)触发
    if (canZoom && risingAttack2 && GetGameTime() >= g_fZoomBlock[client])
    {
        g_fZoomBlock[client] = GetGameTime() + 0.5;

        if (g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0)
        {
            // 侧瞄中按右键 → 瞬间切机瞄
            Aim_ForceExit(client);
            StartZoom(client, weapon);
        }
        else if (g_bZooming[client] || g_iZoomInSwitch[client] != 0)
        {
            StopZoom(client, weapon);
        }
        else
        {
            StartZoom(client, weapon);
        }
    }

    // 侧瞄(中键)触发
    if (canSideAim && risingAttack3 && GetGameTime() >= g_fSideAimBlock[client])
    {
        g_fSideAimBlock[client] = GetGameTime() + 0.5;

        if (g_bZooming[client] || g_iZoomInSwitch[client] != 0)
        {
            // 机瞄中按中键 → 瞬间切侧瞄
            Aim_ForceExit(client);
            StartSideAim(client, weapon);
        }
        else if (g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0)
        {
            StopSideAim(client, weapon);
        }
        else
        {
            StartSideAim(client, weapon);
        }
    }
}

// ============================================================================
// >> 通用配置读取(按前缀拼接键名: zoom/sideaim)
// ============================================================================
int Aim_GetInt(int weapon, const char[] prefix, const char[] suffix, int def)
{
    char key[64];
    Format(key, sizeof(key), "%s%s", prefix, suffix);
    return Zoom_GetInt(weapon, key, def);
}

float Aim_GetFloat(int weapon, const char[] prefix, const char[] suffix, float def)
{
    char key[64];
    Format(key, sizeof(key), "%s%s", prefix, suffix);
    return Zoom_GetFloat(weapon, key, def);
}

int Aim_GetAnimMove(int weapon, const char[] prefix)
{
    return Aim_GetInt(weapon, prefix, "animmove", -1);
}

float Aim_GetAnimTime(int weapon, const char[] prefix)
{
    return Aim_GetFloat(weapon, prefix, "animtime", 0.0);
}

// ============================================================================
// >> 进入瞄准(机瞄/侧瞄通用, prefix 区分)
// ============================================================================
void StartAim(int client, int weapon, const char[] prefix, bool isSideAim)
{
    if (isSideAim)
        g_bSideAiming[client] = true;
    else
        g_bZooming[client] = true;

    int animMove = Aim_GetAnimMove(weapon, prefix);
    float animTime = Aim_GetAnimTime(weapon, prefix);

    if (animMove > 0 && animTime > 0.0)
    {
        if (isSideAim)
            g_iSideAimInSwitch[client] = 1;
        else
            g_iZoomInSwitch[client] = 1;

        SetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence", animMove);
        SetEntPropFloat(ClientVM[client][0], Prop_Send, "m_flPlaybackRate", 1.0);
        SetEntPropFloat(weapon, Prop_Send, "m_flTimeWeaponIdle", GetGameTime() + animTime);
        SetEntPropFloat(weapon, Prop_Send, "m_flNextPrimaryAttack", GetGameTime() + animTime);
        SetEntPropFloat(weapon, Prop_Send, "m_flNextSecondaryAttack", GetGameTime() + animTime);
        CreateTimer(animTime, isSideAim ? Timer_SideAimTransition : Timer_ZoomTransition, client);
    }
    else
    {
        ApplyAimModel(client, weapon, prefix, isSideAim);
    }

    ApplyAimEffects(client, weapon, prefix, true);
    if (isSideAim)
        SideAim_FireForward(client, true);
    else
        Zoom_FireForward(client, true);
}

// ============================================================================
// >> 退出瞄准(机瞄/侧瞄通用)
// ============================================================================
void StopAim(int client, int weapon, const char[] prefix, bool isSideAim)
{
    if (isSideAim)
        g_bSideAiming[client] = false;
    else
        g_bZooming[client] = false;

    int animMove = Aim_GetAnimMove(weapon, prefix);
    float animTime = Aim_GetAnimTime(weapon, prefix);

    // 正在进入过渡中则直接回退，不再播放退出衔接
    int inSwitch = isSideAim ? g_iSideAimInSwitch[client] : g_iZoomInSwitch[client];
    if (animMove > 0 && animTime > 0.0 && inSwitch != 1)
    {
        if (isSideAim)
            g_iSideAimInSwitch[client] = 2;
        else
            g_iZoomInSwitch[client] = 2;

        SetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence", animMove);
        SetEntPropFloat(ClientVM[client][0], Prop_Send, "m_flPlaybackRate", 1.0);
        SetEntPropFloat(weapon, Prop_Send, "m_flTimeWeaponIdle", GetGameTime() + animTime);
        SetEntPropFloat(weapon, Prop_Send, "m_flNextPrimaryAttack", GetGameTime() + animTime);
        SetEntPropFloat(weapon, Prop_Send, "m_flNextSecondaryAttack", GetGameTime() + animTime);
        CreateTimer(animTime, isSideAim ? Timer_SideAimTransition : Timer_ZoomTransition, client);
    }
    else
    {
        ApplyNormalModel(client, weapon, isSideAim);
    }

    ApplyAimEffects(client, weapon, prefix, false);
    if (isSideAim)
        SideAim_FireForward(client, false);
    else
        Zoom_FireForward(client, false);
}

// ============================================================================
// >> 应用/还原模型(机瞄/侧瞄通用)
// ============================================================================
void ApplyAimModel(int client, int weapon, const char[] prefix, bool isSideAim)
{
    char aimModel[PLATFORM_MAX_PATH];
    char key[64];
    Format(key, sizeof(key), "%smodel", prefix);

    if (!GetZoomConfigValue(weapon, key, aimModel, sizeof(aimModel)) || strlen(aimModel) == 0)
    {
        ForceExitAimMode(client, isSideAim);
        return;
    }

    int modelIndex = PrecacheModel(aimModel, true);
    SetEntProp(ClientVM[client][1], Prop_Send, "m_nModelIndex", modelIndex);

    char skinStr[32];
    if (GetZoomConfigValue(weapon, "skincode", skinStr, sizeof(skinStr)))
    {
        int skin = StringToInt(skinStr);
        SetEntProp(weapon, Prop_Send, "m_nSkin", skin);
        SetEntProp(ClientVM[client][1], Prop_Send, "m_nSkin", skin);
    }

    if (isSideAim)
        g_iSideAimInSwitch[client] = 0;
    else
        g_iZoomInSwitch[client] = 0;
}

void ApplyNormalModel(int client, int weapon, bool isSideAim)
{
    char vmodel[PLATFORM_MAX_PATH];
    if (GetWeaponConfigValue(weapon, "vmodel", vmodel, sizeof(vmodel)) && strlen(vmodel) > 0)
    {
        int modelIndex = PrecacheModel(vmodel, true);
        SetEntProp(ClientVM[client][1], Prop_Send, "m_nModelIndex", modelIndex);
    }

    if (isSideAim)
        g_iSideAimInSwitch[client] = 0;
    else
        g_iZoomInSwitch[client] = 0;
}

// ============================================================================
// >> 衔接动画结束/提前结束(机瞄/侧瞄通用)
// ============================================================================
public Action Timer_ZoomTransition(Handle timer, int client)
{
    if (!IsClientInGame(client))
        return Plugin_Stop;

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0 || g_iZoomInSwitch[client] == 0)
        return Plugin_Stop;

    FinishZoomTransition(client, weapon);
    return Plugin_Stop;
}

public Action Timer_SideAimTransition(Handle timer, int client)
{
    if (!IsClientInGame(client))
        return Plugin_Stop;

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0 || g_iSideAimInSwitch[client] == 0)
        return Plugin_Stop;

    FinishSideAimTransition(client, weapon);
    return Plugin_Stop;
}

void FinishZoomTransition(int client, int weapon)
{
    if (g_iZoomInSwitch[client] == 1)
        ApplyAimModel(client, weapon, "zoom", false);
    else if (g_iZoomInSwitch[client] == 2)
        ApplyNormalModel(client, weapon, false);

    g_iZoomInSwitch[client] = 0;
}

void FinishSideAimTransition(int client, int weapon)
{
    if (g_iSideAimInSwitch[client] == 1)
        ApplyAimModel(client, weapon, "sideaim", true);
    else if (g_iSideAimInSwitch[client] == 2)
        ApplyNormalModel(client, weapon, true);

    g_iSideAimInSwitch[client] = 0;
}

// ============================================================================
// >> 瞄准附带效果(机瞄/侧瞄通用)
// ============================================================================
void ApplyAimEffects(int client, int weapon, const char[] prefix, bool aiming)
{
    // 准心
    if (Aim_GetInt(weapon, prefix, "crosshair", 1) > 0)
    {
        ClientCommand(client, aiming ? "crosshair 0" : "crosshair 1");
    }

    // 移动速度
    float aimSpeed = Aim_GetFloat(weapon, prefix, "speed", 1.0);
    if (aiming && aimSpeed > 0.0 && aimSpeed < 1.0)
    {
        SetEntPropFloat(client, Prop_Data, "m_flLaggedMovementValue", aimSpeed);
    }
    else if (!aiming)
    {
        SetEntPropFloat(client, Prop_Data, "m_flLaggedMovementValue", 1.0);
    }

    // 精度
    float aimAccuracy = Aim_GetFloat(weapon, prefix, "accuracy", -1.0);
    if (aimAccuracy > 0.0 && aimAccuracy < 1.0)
    {
        SetEntPropFloat(weapon, Prop_Send, "m_fAccuracyPenalty", aiming ? aimAccuracy : 0.015);
    }

    // 关闭三连发
    if (HasEntProp(weapon, Prop_Send, "m_bBurstMode"))
    {
        SetEntProp(weapon, Prop_Send, "m_bBurstMode", 0);
    }

    // 镜内放大
    int aimFov = Aim_GetInt(weapon, prefix, "fov", 90);
    if (aimFov > 0 && aimFov < 90)
    {
        if (aiming)
        {
            if (!IsFakeClient(client))
            {
                if (!Camera_HasLink(ClientVM[client][1], client))
                {
                    Camera_CreateLink(ClientVM[client][1], client, 90, true);
                }
                else
                {
                    Camera_ResetCamera(client);
                }

                int cam = Camera_GetCamera(client);
                if (cam > 0)
                {
                    char fovStr[8];
                    IntToString(aimFov, fovStr, sizeof(fovStr));
                    DispatchKeyValue(cam, "FOV", fovStr);
                }

                int skinAdd = Aim_GetInt(weapon, prefix, "skinadd", 0);
                if (skinAdd > 0)
                {
                    int skin = GetEntProp(ClientVM[client][1], Prop_Send, "m_nSkin");
                    SetEntProp(ClientVM[client][1], Prop_Send, "m_nSkin", skin + skinAdd);
                }

                ClientCommand(client, "r_screenoverlay materials/zoom/zoomin.vmt");
            }
        }
        else
        {
            if (!IsFakeClient(client))
            {
                Camera_RemoveLink(ClientVM[client][1], client, false);
                ClientCommand(client, "r_screenoverlay 0");

                int skinAdd = Aim_GetInt(weapon, prefix, "skinadd", 0);
                if (skinAdd > 0)
                {
                    int skin = GetEntProp(ClientVM[client][1], Prop_Send, "m_nSkin");
                    SetEntProp(ClientVM[client][1], Prop_Send, "m_nSkin", skin - skinAdd);
                }
            }
        }
    }
    else if (!aiming)
    {
        if (!IsFakeClient(client))
        {
            Camera_RemoveLink(ClientVM[client][1], client, false);
            ClientCommand(client, "r_screenoverlay 0");
        }
    }
}

// ============================================================================
// >> 每帧后处理(屏震降低 + 衔接动画提前结束)
// ============================================================================
void Zoom_OnPostThinkPost(int client)
{
    bool aiming = g_bZooming[client] || g_iZoomInSwitch[client] != 0 ||
        g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0;
    if (!aiming)
        return;

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0)
        return;

    bool isSideAim = (g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0);
    char prefix[16];
    strcopy(prefix, sizeof(prefix), isSideAim ? "sideaim" : "zoom");

    // 衔接动画被其他动作打断时提前完成切换
    int inSwitch = isSideAim ? g_iSideAimInSwitch[client] : g_iZoomInSwitch[client];
    if (inSwitch > 0)
    {
        int animMove = Aim_GetAnimMove(weapon, prefix);
        if (animMove > 0)
        {
            int seq = GetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence");
            if (seq != animMove)
            {
                if (isSideAim)
                    FinishSideAimTransition(client, weapon);
                else
                    FinishZoomTransition(client, weapon);
            }
        }
    }

    // 屏震降低
    float punchFactor = Aim_GetFloat(weapon, prefix, "punch", 1.0);
    if (punchFactor >= 0.0 && punchFactor != 1.0)
    {
        float punch[3];
        GetEntPropVector(client, Prop_Send, "m_vecPunchAngle", punch);
        punch[0] *= punchFactor;
        punch[1] *= punchFactor;
        punch[2] *= punchFactor;
        SetEntPropVector(client, Prop_Send, "m_vecPunchAngle", punch);
    }
}

// ============================================================================
// >> 强制退出指定瞄准模式(机瞄/侧瞄通用)
// ============================================================================
void ForceExitAimMode(int client, bool isSideAim)
{
    bool wasAiming;
    if (isSideAim)
        wasAiming = g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0;
    else
        wasAiming = g_bZooming[client] || g_iZoomInSwitch[client] != 0;

    if (!wasAiming)
        return;

    if (isSideAim)
    {
        g_bSideAiming[client] = false;
        g_iSideAimInSwitch[client] = 0;
    }
    else
    {
        g_bZooming[client] = false;
        g_iZoomInSwitch[client] = 0;
    }

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon > 0)
    {
        ApplyNormalModel(client, weapon, isSideAim);
        ApplyAimEffects(client, weapon, isSideAim ? "sideaim" : "zoom", false);
    }

    if (wasAiming)
    {
        if (isSideAim)
            SideAim_FireForward(client, false);
        else
            Zoom_FireForward(client, false);
    }
}

// ============================================================================
// >> 强制退出全部瞄准(死亡/换弹/换武器/模式切换等)
// ============================================================================
void Aim_ForceExit(int client)
{
    if (g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0)
        ForceExitAimMode(client, true);
    if (g_bZooming[client] || g_iZoomInSwitch[client] != 0)
        ForceExitAimMode(client, false);
}

// ============================================================================
// >> 模式切换包装(触发逻辑调用)
// ============================================================================
void StartZoom(int client, int weapon)
{
    StartAim(client, weapon, "zoom", false);
}

void StopZoom(int client, int weapon)
{
    StopAim(client, weapon, "zoom", false);
}

void StartSideAim(int client, int weapon)
{
    StartAim(client, weapon, "sideaim", true);
}

void StopSideAim(int client, int weapon)
{
    StopAim(client, weapon, "sideaim", true);
}

// ============================================================================
// >> 换弹时退出瞄准
// ============================================================================
public Action Zoom_OnReloadEffect(UserMsg msg_id, Handle bf, const int[] players, int playersNum, bool reliable, bool init)
{
    int client = BfReadByte(bf);
    if (client > 0 && client <= MaxClients &&
        (g_bZooming[client] || g_iZoomInSwitch[client] != 0 || g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0))
    {
        // 无弹药特殊武器不因换弹提示退镜
        int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
        if (!Zoom_IsNoAmmoWeapon(weapon))
        {
            Aim_ForceExit(client);
        }
    }
    return Plugin_Continue;
}

// ============================================================================
// >> 事件转发
// ============================================================================
void Zoom_FireForward(int client, bool zooming)
{
    if (g_ForwardClientZoom == INVALID_HANDLE)
        return;

    Call_StartForward(g_ForwardClientZoom);
    Call_PushCell(client);
    Call_PushCell(zooming);
    Call_Finish();
}

void SideAim_FireForward(int client, bool aiming)
{
    if (g_ForwardClientSideAim == INVALID_HANDLE)
        return;

    Call_StartForward(g_ForwardClientSideAim);
    Call_PushCell(client);
    Call_PushCell(aiming);
    Call_Finish();
}

// ============================================================================
// >> 解除作弊标记命令(用于 r_screenoverlay)
// ============================================================================
void UnlockConsoleCommandAndConvar(const char[] command)
{
    int flags = GetCommandFlags(command);
    if (flags != INVALID_FCVAR_FLAGS)
    {
        SetCommandFlags(command, flags & ~FCVAR_CHEAT);
    }

    Handle cvar = FindConVar(command);
    if (cvar != INVALID_HANDLE)
    {
        flags = GetConVarFlags(cvar);
        SetConVarFlags(cvar, flags & ~FCVAR_CHEAT);
    }
}