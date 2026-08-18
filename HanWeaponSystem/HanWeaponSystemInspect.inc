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

// 检视音效固定通道(同一路径同一通道才能被 StopSound 正确停止)
#define SNDCHAN_INSPECT SNDCHAN_STATIC

// 检视音效最多支持逗号分隔的数量
#define MAX_INSPECT_SOUNDS 8

// 判断字符串是否全为数字(用于识别"帧号:路径"格式)
bool IsNumericString(const char[] s)
{
    if (s[0] == '\0')
        return false;

    for (int i = 0; s[i] != '\0'; i++)
    {
        if (!IsCharNumeric(s[i]))
            return false;
    }
    return true;
}

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

    // 机瞄/侧瞄中/过渡中/奔跑中禁止检视
    if (g_bZooming[client] || g_iZoomInSwitch[client] != 0 || g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0 || g_bRunning[client])
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

        // 机瞄/侧瞄中/过渡中/奔跑中禁止检视
        if (g_bZooming[client] || g_iZoomInSwitch[client] != 0 || g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0 || g_bRunning[client])
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
// >> 停止检视音效(只停自己的固定通道音效, 无音效则为空操作)
// ============================================================================
void Inspect_StopSound(int client)
{
    if (g_sInspectSound[client][0] == '\0')
        return;

    if (client < 1 || client > MaxClients || !IsClientInGame(client) || !IsValidEdict(client))
    {
        g_sInspectSound[client][0] = '\0';
        return;
    }

    StopSound(client, SNDCHAN_INSPECT, g_sInspectSound[client]);
    g_sInspectSound[client][0] = '\0';
}

// ============================================================================
// >> 调度检视帧音效(delay = 帧号 / 30, 同检视动画 30fps 基准)
// ============================================================================
void ScheduleInspectSound(int client, int weapon, int gen, int frame, const char[] path)
{
    DataPack dp;
    CreateDataTimer(frame / 30.0, Timer_PlayInspectSound, dp);
    dp.WriteCell(client);
    dp.WriteCell(EntIndexToEntRef(weapon));
    dp.WriteCell(gen);
    dp.WriteString(path);
}

public Action Timer_PlayInspectSound(Handle timer, DataPack dp)
{
    dp.Reset();
    int client = dp.ReadCell();
    int weaponRef = dp.ReadCell();
    int gen = dp.ReadCell();
    char path[PLATFORM_MAX_PATH];
    dp.ReadString(path, sizeof(path));

    // 检视已结束/切枪/重播(代次不符) → 剩余帧音效不再播放
    if (client < 1 || client > MaxClients || !IsClientInGame(client))
        return Plugin_Handled;
    if (!g_bInspecting[client] || gen != g_iInspectSoundGen[client])
        return Plugin_Handled;

    int weapon = EntRefToEntIndex(weaponRef);
    if (weapon == -1 || g_iInspectWeapon[client] != weapon)
        return Plugin_Handled;

    EmitSoundToClient(client, path, client, SNDCHAN_INSPECT, SNDLEVEL_NORMAL);
    return Plugin_Handled;
}

// ============================================================================
// >> 开始检视
// ============================================================================
void Inspect_Start(int client)
{
    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0)
        return;

    // 记录进入检视前状态(重复模式重播时已在检视中, 不重复广播开始)
    bool bWasInspecting = g_bInspecting[client];

    // 重复模式重播时先停掉上一段检视音效, 避免音效重叠
    Inspect_StopSound(client);

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
    g_iInspectSoundGen[client]++;

    // 检视音效: 缺省不填不播放; 纯路径条目(无帧号)逗号分隔随机选一个检视一开始播;
    // 带"帧号:路径"条目按帧号定时多段播放(同换弹逻辑, 30fps 换算秒); 只自己听得见
    char soundStr[512];
    if (GetInspectConfigValue(weapon, "inspectsound", soundStr, sizeof(soundStr)))
    {
        char parts[MAX_INSPECT_SOUNDS][PLATFORM_MAX_PATH];
        int count = ExplodeString(soundStr, ",", parts, sizeof(parts), sizeof(parts[]));

        int valid[MAX_INSPECT_SOUNDS];
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

        int gen = g_iInspectSoundGen[client];

        // 纯路径条目(开场随机播一个)与帧号条目(定时多段)分开处理
        int plain[MAX_INSPECT_SOUNDS];
        int plainCount = 0;
        for (int i = 0; i < validCount; i++)
        {
            char pair[2][PLATFORM_MAX_PATH];
            if (ExplodeString(parts[valid[i]], ":", pair, sizeof(pair), sizeof(pair[])) == 2 && IsNumericString(pair[0]))
            {
                int frameNum = StringToInt(pair[0]);
                char path[PLATFORM_MAX_PATH];
                strcopy(path, sizeof(path), pair[1]);
                NormalizePath(path);
                ScheduleInspectSound(client, weapon, gen, frameNum, path);
            }
            else
            {
                plain[plainCount] = valid[i];
                plainCount++;
            }
        }

        if (plainCount > 0)
        {
            int pick = plain[GetRandomInt(0, plainCount - 1)];
            strcopy(g_sInspectSound[client], sizeof(g_sInspectSound[]), parts[pick]);
            NormalizePath(g_sInspectSound[client]);
            EmitSoundToClient(client, g_sInspectSound[client], client, SNDCHAN_INSPECT, SNDLEVEL_NORMAL);
        }
    }

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

    // 首次进入检视才广播开始事件(重复模式重播不重复广播)
    if (!bWasInspecting)
    {
        Inspect_FireForward(client, true);
    }
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

    // 结束检视即停止检视音效 + 广播结束事件
    Inspect_StopSound(client);
    Inspect_FireForward(client, false);

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

    // 机瞄/侧瞄/奔跑优先: 开启时强制退出检视
    if (g_bZooming[client] || g_iZoomInSwitch[client] != 0 || g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0 || g_bRunning[client])
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

    // 右键取消检视: 刀 / 配置了 firespeed 右键连发的枪械 / autofire 速射武器
    // (带机瞄/侧瞄的武器已由 Zoom 模块在按下瞄准键时打断检视, 此处不重复处理)
    if ((GetClientButtons(client) & IN_ATTACK2) && (IsKnifeWeapon(weapon) || HasRightFireSpeed(weapon) || g_bWeaponAutoFire[client]))
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
// >> 事件转发(开始/结束检视广播给其他插件)
// ============================================================================
void Inspect_FireForward(int client, bool inspecting)
{
    if (g_ForwardClientInspect == INVALID_HANDLE)
        return;

    Call_StartForward(g_ForwardClientInspect);
    Call_PushCell(client);
    Call_PushCell(inspecting);
    Call_Finish();
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

    // 掉线/死亡时停止检视音效
    Inspect_StopSound(client);
}