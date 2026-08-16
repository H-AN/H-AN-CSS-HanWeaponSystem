// ============================================================================
// >> 武器检视系统(F键/命令触发 + 每tick动画强制 + 手电屏蔽)
// ============================================================================
// >> 原作者声明 / Original Author Credit
// 本模块的检视逻辑移植自开源社区插件 "Run LookAt01 Action"。
// This module's inspect logic is ported from the open-source plugin
// "Run LookAt01 Action".
//
// 原作者 / Original Author: cjsrk
// 版本 / Version: 1.0
//
// 感谢原作者的付出！原插件仅供学习参考，本插件已在此基础上大幅重构改写。
// Special thanks to the original author for the work! The original plugin is
// used for reference only; this plugin has been heavily reworked on top of it.
// ============================================================================

// 手电筒状态位(实测 F键/impulse 100 会使玩家 m_fEffects 在 0x0 <-> 0x4 切换)
#define EF_FLASHLIGHT 4

void Inspect_OnPluginStart()
{
    RegConsoleCmd("sm_inspect", Cmd_Inspect, "检视当前武器");
}

// ============================================================================
// >> 判断武器是否开启检视
// ============================================================================
bool Inspect_IsWeaponInspectable(int weapon)
{
    char val[8];
    if (GetInspectConfigValue(weapon, "inspect", val, sizeof(val)))
    {
        return StringToInt(val) > 0;
    }
    return false;
}

// ============================================================================
// >> 判断武器是否允许重复检视(反复按F重播动画)
// ============================================================================
bool Inspect_IsRepeatable(int weapon)
{
    return Inspect_GetInt(weapon, "inspectrepeat", 0) > 0;
}

// ============================================================================
// >> 命令触发(sm_inspect)
// ============================================================================
public Action Cmd_Inspect(int client, int args)
{
    if (client < 1 || client > MaxClients || !IsClientInGame(client) || !IsPlayerAlive(client))
        return Plugin_Handled;

    if (IsFakeClient(client))
        return Plugin_Handled;

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0)
        return Plugin_Handled;

    // 机瞄中/过渡中禁止检视
    if (g_bZooming[client] || g_iZoomInSwitch[client] != 0)
        return Plugin_Handled;

    // 单次模式: 检视中忽略重复触发; 重复模式: 允许重播
    if (!Inspect_IsRepeatable(weapon) && g_bInspecting[client])
        return Plugin_Handled;

    if (!Inspect_IsWeaponInspectable(weapon))
        return Plugin_Handled;

    // 换弹/换枪(deploy)期间不允许检视(无弹药特殊武器豁免, 与机瞄门槛一致)
    if (!Zoom_IsNoAmmoWeapon(weapon) && GetEntPropFloat(client, Prop_Send, "m_flNextAttack") > GetGameTime())
        return Plugin_Handled;

    // 单次模式冷却防抖; 重复模式不限制以支持快速连按
    if (!Inspect_IsRepeatable(weapon) && GetGameTime() < g_fInspectBlock[client])
        return Plugin_Handled;

    Inspect_Start(client);
    return Plugin_Handled;
}

// ============================================================================
// >> 触发检测(F键/impulse 100, 由 Combat 的 OnPlayerRunCmd 调用)
// ============================================================================
void Inspect_OnPlayerRunCmd(int client, int impulse, bool bReloading)
{
    static bool s_bPrevImpulse100[MAXPLAYERS + 1];

    if (impulse == 100)
    {
        // 上升沿: impulse 100 首次出现
        if (s_bPrevImpulse100[client])
            return;

        s_bPrevImpulse100[client] = true;

        if (!IsPlayerAlive(client))
            return;

        int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
        if (weapon <= 0)
            return;

        // 机瞄中/过渡中禁止检视
        if (g_bZooming[client] || g_iZoomInSwitch[client] != 0)
            return;

        // 单次模式: 检视中忽略重复触发; 重复模式: 允许反复按F重播
        if (!Inspect_IsRepeatable(weapon) && g_bInspecting[client])
            return;

        // 换弹/换枪(deploy)期间不允许检视(无弹药特殊武器豁免, 与机瞄门槛一致)
        if (!Zoom_IsNoAmmoWeapon(weapon) && (bReloading || GetEntPropFloat(client, Prop_Send, "m_flNextAttack") > GetGameTime()))
            return;

        if (!Inspect_IsWeaponInspectable(weapon))
            return;

        // 冷却防抖(防止按键抖动重复触发); 重复模式不限制冷却以支持快速连按
        if (!Inspect_IsRepeatable(weapon) && GetGameTime() < g_fInspectBlock[client])
            return;

        Inspect_Start(client);
    }
    else
    {
        s_bPrevImpulse100[client] = false;
    }
}

// ============================================================================
// >> 开始检视
// ============================================================================
void Inspect_Start(int client)
{
    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0)
        return;

    int seq = Inspect_GetInt(weapon, "inspectseq", -1);
    if (seq < 0)
        return;

    // 时长单位为帧(30fps, 与原插件配置兼容), 换算成秒
    int frame = Inspect_GetInt(weapon, "inspecttime", 0);
    if (frame <= 0)
        return;

    float dur = frame / 30.0;

    g_bInspecting[client] = true;
    g_iInspectWeapon[client] = weapon;
    g_fInspectEnd[client] = GetGameTime() + dur;
    g_fInspectBlock[client] = GetGameTime() + 0.5;

    // 单次模式: 直接强制 1号VM 检视序列
    // 重复模式: 起源引擎设置同一序列不会重播, 需先切其他序列再切回
    // 故标记重启(2=播一帧引擎原生序列, 1=下帧切回检视序列), 由 OnPostThinkPost 执行
    if (Inspect_IsRepeatable(weapon))
    {
        g_iInspectRestart[client] = 2;
    }
    else
    {
        SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", seq);
        SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", 1.0);
    }

    // 压住武器空闲动画, 避免中途插回 idle 打断检视
    SetEntPropFloat(weapon, Prop_Send, "m_flTimeWeaponIdle", GetGameTime() + dur);
}

// ============================================================================
// >> 结束检视(恢复空闲)
// ============================================================================
void Inspect_End(int client)
{
    if (!g_bInspecting[client])
        return;

    g_bInspecting[client] = false;
    g_iInspectWeapon[client] = -1;
    g_iInspectRestart[client] = 0;

    // 恢复 VM[1] 到引擎原生序列(idle), 避免残留检视序列最后一帧导致下次设同序列无法重播
    if (ClientVM[client][0] > 0 && IsValidEntity(ClientVM[client][0]) &&
        ClientVM[client][1] > 0 && IsValidEntity(ClientVM[client][1]))
    {
        SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", GetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence"));
        SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", 1.0);
    }

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon > 0)
    {
        SetEntPropFloat(weapon, Prop_Send, "m_flTimeWeaponIdle", GetGameTime());
    }
}

// ============================================================================
// >> 每帧后处理(动画强制 + 结束条件检测 + 手电屏蔽)
// ============================================================================
void Inspect_OnPostThinkPost(int client)
{
    if (!g_bInspecting[client])
        return;

    if (!IsPlayerAlive(client))
    {
        Inspect_End(client);
        return;
    }

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0 || weapon != g_iInspectWeapon[client])
    {
        Inspect_End(client);
        return;
    }

    // 机瞄优先: 机瞄开启时强制退出检视
    if (g_bZooming[client] || g_iZoomInSwitch[client] != 0)
    {
        Inspect_End(client);
        return;
    }

    // 开火取消检视(否则看不到射击动画)
    if (GetClientButtons(client) & IN_ATTACK)
    {
        Inspect_End(client);
        return;
    }

    // 换弹取消检视
    if (Weapon_IsReloading(weapon) || (HasEntProp(weapon, Prop_Send, "m_reloadState") && GetEntProp(weapon, Prop_Send, "m_reloadState") > 0))
    {
        Inspect_End(client);
        return;
    }

    // 时长到自动结束
    if (GetGameTime() >= g_fInspectEnd[client])
    {
        Inspect_End(client);
        return;
    }

    // 重复模式重启: 两帧切换强制重播
    // 起源引擎设置同一序列不会重播, 需先播一帧其他序列(引擎原生序列)再切回检视序列
    if (g_iInspectRestart[client] == 2)
    {
        g_iInspectRestart[client] = 1;
        SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", GetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence"));
        SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", 1.0);
        return;
    }
    else if (g_iInspectRestart[client] == 1)
    {
        g_iInspectRestart[client] = 0;
    }

    // 持续强制 1号VM 检视序列(每tick在动画复制之后覆盖)
    int seq = Inspect_GetInt(weapon, "inspectseq", -1);
    if (seq < 0)
    {
        Inspect_End(client);
        return;
    }

    SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", seq);
    SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", 1.0);

    // 屏蔽手电筒光(配置 inspectflashlight 1 时)
    if (Inspect_GetInt(weapon, "inspectflashlight", 0) > 0)
    {
        int fx = GetEntProp(client, Prop_Send, "m_fEffects");
        if (fx & EF_FLASHLIGHT)
        {
            SetEntProp(client, Prop_Send, "m_fEffects", fx & ~EF_FLASHLIGHT);
        }
    }
}

// ============================================================================
// >> 重置(死亡/掉线)
// ============================================================================
void Inspect_Reset(int client)
{
    g_bInspecting[client] = false;
    g_iInspectWeapon[client] = -1;
    g_fInspectEnd[client] = 0.0;
    g_fInspectBlock[client] = 0.0;
    g_iInspectRestart[client] = 0;
}