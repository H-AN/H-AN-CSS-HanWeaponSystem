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
// ============================================================================

void Zoom_OnPluginStart()
{
    HookUserMessage(GetUserMessageId("ReloadEffect"), Zoom_OnReloadEffect);
}

void Zoom_OnMapStart()
{
    AddFileToDownloadsTable("materials/zoom/zoomin.vmt");
    AddFileToDownloadsTable("materials/zoom/zoomin.vtf");
    PrecacheDecal("materials/zoom/zoomin.vmt");
    PrecacheDecal("materials/zoom/zoomin.vtf");

    UnlockConsoleCommandAndConvar("r_screenoverlay");
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
// >> 判断是否为无弹药特殊武器(脚本 primary_ammo None → ammo type -1)
//    特殊武器不执行"没子弹/换弹强制退镜"逻辑
// ============================================================================
bool Zoom_IsNoAmmoWeapon(int weapon)
{
    if (weapon <= 0 || !IsValidEdict(weapon))
        return false;

    return GetEntProp(weapon, Prop_Data, "m_iPrimaryAmmoType") == -1;
}

int Zoom_GetAnimMove(int weapon)
{
    return Zoom_GetInt(weapon, "zoomanimmove", -1);
}

float Zoom_GetAnimTime(int weapon)
{
    return Zoom_GetFloat(weapon, "zoomanimtime", 0.0);
}

// ============================================================================
// >> 触发检测(由 Combat 的 OnPlayerRunCmd 调用)
// ============================================================================
void Zoom_OnPlayerRunCmd(int client, int buttons, bool bReloading)
{
    bool pressing = (buttons & IN_ATTACK2) != 0;

    if (!IsPlayerAlive(client))
    {
        if (g_bZooming[client] || g_iZoomInSwitch[client] != 0)
            Zoom_ForceExit(client);
        return;
    }

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0)
        return;

    // 换弹/换枪(deploy)期间不允许机瞄，正在机瞄则强制退出
    // 无弹药特殊武器(primary_ammo None)不受此限制
    if (!Zoom_IsNoAmmoWeapon(weapon) && (bReloading || GetEntPropFloat(client, Prop_Send, "m_flNextAttack") > GetGameTime()))
    {
        if (g_bZooming[client])
            Zoom_ForceExit(client);
        return;
    }

    if (!Zoom_IsWeaponZoomable(weapon))
        return;

    // 检视中禁止开镜(互斥)
    if (g_bInspecting[client])
        return;

    static bool s_bPrevAttack2[MAXPLAYERS + 1];
    bool risingEdge = pressing && !s_bPrevAttack2[client];
    s_bPrevAttack2[client] = pressing;

    // 点按开镜，再按解除；冷却防抖
    bool wantZoom = g_bZooming[client];
    if (risingEdge && GetGameTime() >= g_fZoomBlock[client])
    {
        wantZoom = !g_bZooming[client];
        g_fZoomBlock[client] = GetGameTime() + 0.5;
    }

    if (wantZoom != g_bZooming[client])
    {
        if (wantZoom)
            StartZoom(client, weapon);
        else
            StopZoom(client, weapon);
    }
}

// ============================================================================
// >> 进入机瞄
// ============================================================================
void StartZoom(int client, int weapon)
{
    g_bZooming[client] = true;

    int animMove = Zoom_GetAnimMove(weapon);
    float animTime = Zoom_GetAnimTime(weapon);

    if (animMove > 0 && animTime > 0.0)
    {
        g_iZoomInSwitch[client] = 1;
        SetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence", animMove);
        SetEntPropFloat(ClientVM[client][0], Prop_Send, "m_flPlaybackRate", 1.0);
        SetEntPropFloat(weapon, Prop_Send, "m_flTimeWeaponIdle", GetGameTime() + animTime);
        SetEntPropFloat(weapon, Prop_Send, "m_flNextPrimaryAttack", GetGameTime() + animTime);
        SetEntPropFloat(weapon, Prop_Send, "m_flNextSecondaryAttack", GetGameTime() + animTime);
        CreateTimer(animTime, Timer_ZoomTransition, client);
    }
    else
    {
        ApplyZoomModel(client, weapon);
    }

    ApplyZoomEffects(client, weapon, true);
    Zoom_FireForward(client, true);
}

// ============================================================================
// >> 退出机瞄
// ============================================================================
void StopZoom(int client, int weapon)
{
    g_bZooming[client] = false;

    int animMove = Zoom_GetAnimMove(weapon);
    float animTime = Zoom_GetAnimTime(weapon);

    // 正在开镜过渡中则直接回退，不再播放关镜衔接
    if (animMove > 0 && animTime > 0.0 && g_iZoomInSwitch[client] != 1)
    {
        g_iZoomInSwitch[client] = 2;
        SetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence", animMove);
        SetEntPropFloat(ClientVM[client][0], Prop_Send, "m_flPlaybackRate", 1.0);
        SetEntPropFloat(weapon, Prop_Send, "m_flTimeWeaponIdle", GetGameTime() + animTime);
        SetEntPropFloat(weapon, Prop_Send, "m_flNextPrimaryAttack", GetGameTime() + animTime);
        SetEntPropFloat(weapon, Prop_Send, "m_flNextSecondaryAttack", GetGameTime() + animTime);
        CreateTimer(animTime, Timer_ZoomTransition, client);
    }
    else
    {
        ApplyNormalModel(client, weapon);
    }

    ApplyZoomEffects(client, weapon, false);
    Zoom_FireForward(client, false);
}

// ============================================================================
// >> 应用/还原模型
// ============================================================================
void ApplyZoomModel(int client, int weapon)
{
    char zoomModel[PLATFORM_MAX_PATH];
    if (!GetZoomConfigValue(weapon, "zoommodel", zoomModel, sizeof(zoomModel)) || strlen(zoomModel) == 0)
    {
        Zoom_ForceExit(client);
        return;
    }

    int modelIndex = PrecacheModel(zoomModel, true);
    SetEntProp(ClientVM[client][1], Prop_Send, "m_nModelIndex", modelIndex);

    char skinStr[32];
    if (GetZoomConfigValue(weapon, "skincode", skinStr, sizeof(skinStr)))
    {
        int skin = StringToInt(skinStr);
        SetEntProp(weapon, Prop_Send, "m_nSkin", skin);
        SetEntProp(ClientVM[client][1], Prop_Send, "m_nSkin", skin);
    }

    g_iZoomInSwitch[client] = 0;
}

void ApplyNormalModel(int client, int weapon)
{
    char vmodel[PLATFORM_MAX_PATH];
    if (GetWeaponConfigValue(weapon, "vmodel", vmodel, sizeof(vmodel)) && strlen(vmodel) > 0)
    {
        int modelIndex = PrecacheModel(vmodel, true);
        SetEntProp(ClientVM[client][1], Prop_Send, "m_nModelIndex", modelIndex);
    }

    g_iZoomInSwitch[client] = 0;
}

// ============================================================================
// >> 衔接动画结束/提前结束
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

void FinishZoomTransition(int client, int weapon)
{
    if (g_iZoomInSwitch[client] == 1)
        ApplyZoomModel(client, weapon);
    else if (g_iZoomInSwitch[client] == 2)
        ApplyNormalModel(client, weapon);

    g_iZoomInSwitch[client] = 0;
}

// ============================================================================
// >> 机瞄附带效果
// ============================================================================
void ApplyZoomEffects(int client, int weapon, bool zooming)
{
    // 准心
    if (Zoom_GetInt(weapon, "zoomcrosshair", 1) > 0)
    {
        ClientCommand(client, zooming ? "crosshair 0" : "crosshair 1");
    }

    // 移动速度
    float zoomSpeed = Zoom_GetFloat(weapon, "zoomspeed", 1.0);
    if (zooming && zoomSpeed > 0.0 && zoomSpeed < 1.0)
    {
        SetEntPropFloat(client, Prop_Data, "m_flLaggedMovementValue", zoomSpeed);
    }
    else if (!zooming)
    {
        SetEntPropFloat(client, Prop_Data, "m_flLaggedMovementValue", 1.0);
    }

    // 精度
    float zoomAccuracy = Zoom_GetFloat(weapon, "zoomaccuracy", -1.0);
    if (zoomAccuracy > 0.0 && zoomAccuracy < 1.0)
    {
        SetEntPropFloat(weapon, Prop_Send, "m_fAccuracyPenalty", zooming ? zoomAccuracy : 0.015);
    }

    // 关闭三连发
    if (HasEntProp(weapon, Prop_Send, "m_bBurstMode"))
    {
        SetEntProp(weapon, Prop_Send, "m_bBurstMode", 0);
    }

    // 镜内放大
    int zoomFov = Zoom_GetInt(weapon, "zoomfov", 90);
    if (zoomFov > 0 && zoomFov < 90)
    {
        if (zooming)
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
                    IntToString(zoomFov, fovStr, sizeof(fovStr));
                    DispatchKeyValue(cam, "FOV", fovStr);
                }

                int skinAdd = Zoom_GetInt(weapon, "zoomskinadd", 0);
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

                int skinAdd = Zoom_GetInt(weapon, "zoomskinadd", 0);
                if (skinAdd > 0)
                {
                    int skin = GetEntProp(ClientVM[client][1], Prop_Send, "m_nSkin");
                    SetEntProp(ClientVM[client][1], Prop_Send, "m_nSkin", skin - skinAdd);
                }
            }
        }
    }
    else if (!zooming)
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
    if (!g_bZooming[client])
        return;

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0)
        return;

    // 衔接动画被其他动作打断时提前完成切换
    if (g_iZoomInSwitch[client] > 0)
    {
        int animMove = Zoom_GetAnimMove(weapon);
        if (animMove > 0)
        {
            int seq = GetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence");
            if (seq != animMove)
            {
                FinishZoomTransition(client, weapon);
            }
        }
    }

    // 屏震降低
    float punchFactor = Zoom_GetFloat(weapon, "zoompunch", 1.0);
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
// >> 强制退出机瞄(死亡/换弹/换武器等)
// ============================================================================
void Zoom_ForceExit(int client)
{
    bool wasZooming = g_bZooming[client];
    if (!wasZooming && g_iZoomInSwitch[client] == 0)
        return;

    g_bZooming[client] = false;
    g_iZoomInSwitch[client] = 0;

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon > 0)
    {
        ApplyNormalModel(client, weapon);
        ApplyZoomEffects(client, weapon, false);
    }

    if (wasZooming)
    {
        Zoom_FireForward(client, false);
    }
}

// ============================================================================
// >> 换弹时退出机瞄
// ============================================================================
public Action Zoom_OnReloadEffect(UserMsg msg_id, Handle bf, const int[] players, int playersNum, bool reliable, bool init)
{
    int client = BfReadByte(bf);
    if (client > 0 && client <= MaxClients && (g_bZooming[client] || g_iZoomInSwitch[client] != 0))
    {
        // 无弹药特殊武器不因换弹提示退镜
        int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
        if (!Zoom_IsNoAmmoWeapon(weapon))
        {
            Zoom_ForceExit(client);
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