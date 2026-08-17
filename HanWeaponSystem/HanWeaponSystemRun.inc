// ============================================================================
// >> 持枪奔跑系统(点按Shift触发 + 前进按住保持 + 每tick动画强制 + 抖动)
// ============================================================================
// >> 原作者声明 / Original Author Credit
// 本模块的奔跑逻辑移植自开源社区插件 "Run_with_weapon"。
// This module's run logic is ported from the open-source plugin
// "Run_with_weapon".
//
// 原作者 / Original Author: cjsrk
// 版本 / Version: 1.1
//
// 感谢原作者的付出！原插件仅供学习参考，本插件已在此基础上大幅重构改写。
// Special thanks to the original author for the work! The original plugin is
// used for reference only; this plugin has been heavily reworked on top of it.
// ============================================================================

// 奔跑阶段常量
#define RUN_STAGE_NONE 0
#define RUN_STAGE_IN 1      // Idle->奔跑过渡中
#define RUN_STAGE_RUN 2     // 奔跑循环中
#define RUN_STAGE_OUT 3     // 奔跑->Idle过渡中

// IN_SPEED 点按判定窗口(秒): 短按松开视为点按触发奔跑, 长按保持静步
#define RUN_SPEED_TAP_TIME 0.2

// 奔跑抖动参数(幅度取配置 runshake, 频率/时长固定)
#define RUN_SHAKE_FREQ 0.4
#define RUN_SHAKE_TIME 2.0
// 超大负值幅度: 引擎会clamp到0, 用于每次触发前强制清空累计幅度
#define RUN_SHAKE_RESET_AMP -9999.0

void Run_OnPluginStart()
{
    // 无独立命令/cvar，配置与状态机见 RunConfig / 主插件
}

// ============================================================================
// >> 判断武器是否开启奔跑
// ============================================================================
bool Run_IsWeaponRunable(int weapon)
{
    return Run_GetInt(weapon, "run", 0) > 0;
}

// ============================================================================
// >> 奔跑循环切换间隔(帧转秒, 非法值兜底)
// ============================================================================
float Run_GetCycleTime(int weapon)
{
    int runtime = Run_GetInt(weapon, "runtime", 15);
    if (runtime <= 0)
        runtime = 15;
    return runtime / 30.0;
}

// ============================================================================
// >> 奔跑速度写入/恢复
// ============================================================================
void Run_ApplySpeed(int client, int weapon)
{
    if (g_bRunSpeedApplied[client])
        return;

    float speed = Run_GetFloat(weapon, "runspeed", 0.0);
    if (speed > 0.0)
    {
        SetEntPropFloat(client, Prop_Data, "m_flLaggedMovementValue", speed);
        g_bRunSpeedApplied[client] = true;
    }
}

void Run_RestoreSpeed(int client)
{
    if (!g_bRunSpeedApplied[client])
        return;

    SetEntPropFloat(client, Prop_Data, "m_flLaggedMovementValue", 1.0);
    g_bRunSpeedApplied[client] = false;
}

// ============================================================================
// >> 触发/停止检测(由 Combat 的 OnPlayerRunCmd 调用)
// 奔跑触发方式: 前进按住时 点按Shift(短按松开)进入奔跑; 长按Shift保持静步;
// 奔跑中松开前进/停止即结束, 再次点按Shift才可再奔跑
// 返回 Plugin_Changed 表示已修改按键(奔跑中剔除IN_SPEED/禁止换弹)
// ============================================================================
public Action Run_OnPlayerRunCmd(int client, int &buttons, bool bReloading)
{
    int stage = g_iRunStage[client];
    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");

    // ===== 奔跑流程中 =====
    if (stage != RUN_STAGE_NONE)
    {
        bool bChanged = false;

        // 奔跑中剔除 IN_SPEED(防止按住shift被引擎静步减速干扰奔跑速度)
        if ((stage == RUN_STAGE_IN || stage == RUN_STAGE_RUN) && (buttons & IN_SPEED))
        {
            buttons &= ~IN_SPEED;
            bChanged = true;
        }

        // 奔跑中(实际冲刺阶段)禁止换弹
        if ((stage == RUN_STAGE_IN || stage == RUN_STAGE_RUN) && (buttons & IN_RELOAD))
        {
            buttons &= ~IN_RELOAD;
            bChanged = true;
        }

        bool bStop = false;
        if (weapon <= 0)
            bStop = true;
        else if (g_bZooming[client] || g_iZoomInSwitch[client] != 0 || g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0 || g_bInspecting[client])
            bStop = true;
        else if (bReloading)
            bStop = true;
        else if (stage == RUN_STAGE_IN || stage == RUN_STAGE_RUN)
        {
            // 松开前进/按其他方向或跳跃下蹲/开火即结束奔跑
            bStop = ((buttons & IN_ATTACK) || (buttons & IN_ATTACK2) || (buttons & IN_ATTACK3) || (buttons & IN_ALT1) || (buttons & IN_ALT2) ||
                     !(buttons & IN_FORWARD) ||
                     (buttons & IN_BACK) || (buttons & IN_MOVELEFT) || (buttons & IN_MOVERIGHT) ||
                     (buttons & IN_JUMP) || (buttons & IN_DUCK));
        }
        else // RUN_STAGE_OUT: 过渡中按攻击/侧瞄立即截断
        {
            bStop = ((buttons & IN_ATTACK) || (buttons & IN_ATTACK2) || (buttons & IN_ATTACK3) || (buttons & IN_ALT1) || (buttons & IN_ALT2));
        }

        if (bStop)
        {
            if (stage == RUN_STAGE_OUT)
                Run_End(client);
            else
                Run_Stop(client);
        }

        // 同步 IN_SPEED 状态(奔跑期间不做点按触发)
        g_bSpeedDown[client] = (buttons & IN_SPEED) != 0;

        if (bChanged)
            return Plugin_Changed;
        return Plugin_Continue;
    }

    // ===== 未奔跑: IN_SPEED 点按检测 =====
    bool speedDown = (buttons & IN_SPEED) != 0;

    if (speedDown && !g_bSpeedDown[client])
    {
        // 上升沿: 记录按下时刻(长按保持静步不触发奔跑)
        g_bSpeedDown[client] = true;
        g_fSpeedPressTime[client] = GetGameTime();
    }
    else if (!speedDown && g_bSpeedDown[client])
    {
        // 下降沿(松开): 短按判定为点按, 尝试触发奔跑
        g_bSpeedDown[client] = false;

        if (GetGameTime() - g_fSpeedPressTime[client] <= RUN_SPEED_TAP_TIME)
        {
            Run_TryStart(client, buttons, bReloading);
        }
    }

    return Plugin_Continue;
}

// ============================================================================
// >> 奔跑触发尝试(点按Shift松开时判定; 前进必须仍按住且无干扰键)
// ============================================================================
void Run_TryStart(int client, int buttons, bool bReloading)
{
    int stage = g_iRunStage[client];
    if (stage != RUN_STAGE_NONE)
        return;

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0)
        return;

    // 必须按住前进, 且无其他方向/跳跃/下蹲/开火键
    if (!(buttons & IN_FORWARD) || (buttons & IN_BACK) || (buttons & IN_MOVELEFT) || (buttons & IN_MOVERIGHT) ||
        (buttons & IN_JUMP) || (buttons & IN_DUCK) || (buttons & IN_ATTACK))
        return;

    // 互斥门槛: 检视期间优先奔跑(先结束检视再继续判定); 机瞄/侧瞄期间不可奔跑
    if (g_bInspecting[client])
        Inspect_End(client);

    if (g_bZooming[client] || g_iZoomInSwitch[client] != 0 || g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0)
        return;

    // 换弹/换枪(deploy)期间不允许奔跑
    if (bReloading || GetEntPropFloat(client, Prop_Send, "m_flNextAttack") > GetGameTime())
        return;

    if (!Run_IsWeaponRunable(weapon))
        return;

    Run_Start(client);
}

// ============================================================================
// >> 开始奔跑
// ============================================================================
void Run_Start(int client)
{
    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0)
        return;

    int runin = Run_GetInt(weapon, "runin", -1);
    int runinlen = Run_GetInt(weapon, "runinlength", 0);

    g_iRunWeapon[client] = weapon;
    g_bRunFrame[client] = false;
    g_bRunSpeedApplied[client] = false;

    if (runin >= 0 && runinlen > 0)
    {
        g_iRunStage[client] = RUN_STAGE_IN;
        g_fRunSwitch[client] = GetGameTime() + (runinlen / 30.0);
    }
    else
    {
        g_iRunStage[client] = RUN_STAGE_RUN;
        g_fRunSwitch[client] = GetGameTime() + Run_GetCycleTime(weapon);
    }

    g_bRunning[client] = true;

    // 奔跑开始即触发首次抖动
    g_fRunShakeNext[client] = 0.0;

    Run_ApplySpeed(client, weapon);

    Run_FireForward(client, true);
}

// ============================================================================
// >> 停止奔跑(有 runout 则播放过渡, 否则直接结束)
// ============================================================================
void Run_Stop(int client)
{
    if (g_iRunStage[client] == RUN_STAGE_NONE)
        return;

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");

    // 恢复奔跑速度
    Run_RestoreSpeed(client);

    // 已在 runout 中
    if (g_iRunStage[client] == RUN_STAGE_OUT)
        return;

    int runout = -1, runoutlen = 0;
    if (weapon > 0)
    {
        runout = Run_GetInt(weapon, "runout", -1);
        runoutlen = Run_GetInt(weapon, "runoutlength", 0);
    }

    if (runout >= 0 && runoutlen > 0)
    {
        g_iRunStage[client] = RUN_STAGE_OUT;
        g_fRunSwitch[client] = GetGameTime() + (runoutlen / 30.0);
        return;
    }

    Run_End(client);
}

// ============================================================================
// >> 完全结束奔跑(恢复空闲)
// ============================================================================
void Run_End(int client)
{
    if (g_iRunStage[client] == RUN_STAGE_NONE)
        return;

    g_bRunning[client] = false;
    g_iRunStage[client] = RUN_STAGE_NONE;
    g_bRunFrame[client] = false;
    g_iRunWeapon[client] = -1;
    g_fRunShakeNext[client] = 0.0;

    // 立即停止屏幕抖动, 消除奔跑结束后的抖动尾巴
    Run_StopShake(client);

    // 恢复奔跑速度
    Run_RestoreSpeed(client);

    // 恢复 VM[1] 到引擎原生序列(idle)
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

    Run_FireForward(client, false);
}

// ============================================================================
// >> 每帧后处理(动画强制 + 状态推进 + 抖动 + 结束条件)
// ============================================================================
void Run_OnPostThinkPost(int client)
{
    int stage = g_iRunStage[client];
    if (stage == RUN_STAGE_NONE)
        return;

    if (!IsPlayerAlive(client))
    {
        Run_End(client);
        return;
    }

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0 || weapon != g_iRunWeapon[client])
    {
        Run_End(client);
        return;
    }

    // 机瞄/侧瞄/检视/换弹优先: 强制退出奔跑(安全网, 正常由 runcmd 触发 runout)
    if (g_bZooming[client] || g_iZoomInSwitch[client] != 0 || g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0 || g_bInspecting[client])
    {
        Run_End(client);
        return;
    }

    if (Weapon_IsReloading(weapon) || (HasEntProp(weapon, Prop_Send, "m_reloadState") && GetEntProp(weapon, Prop_Send, "m_reloadState") > 0))
    {
        Run_End(client);
        return;
    }

    float now = GetGameTime();

    // 状态推进 + 动画强制(作用在 1号VM, 在动画复制之后覆盖)
    switch (stage)
    {
        case RUN_STAGE_IN:
        {
            int runin = Run_GetInt(weapon, "runin", -1);
            if (runin >= 0)
            {
                SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", runin);
                SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", 1.0);
            }

            if (now >= g_fRunSwitch[client])
            {
                g_iRunStage[client] = RUN_STAGE_RUN;
                g_fRunSwitch[client] = now + Run_GetCycleTime(weapon);
            }
        }

        case RUN_STAGE_RUN:
        {
            // 起源引擎设置同一序列不会重播, 用两个序列交替强制重播
            if (now >= g_fRunSwitch[client])
            {
                g_fRunSwitch[client] = now + Run_GetCycleTime(weapon);
                g_bRunFrame[client] = !g_bRunFrame[client];
            }

            int runseq = Run_GetInt(weapon, "runseq", -1);
            if (runseq >= 0)
            {
                int seq = runseq + (g_bRunFrame[client] ? 1 : 0);
                SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", seq);
                SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", 1.0);
            }
        }

        case RUN_STAGE_OUT:
        {
            int runout = Run_GetInt(weapon, "runout", -1);
            if (runout >= 0)
            {
                SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", runout);
                SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", 1.0);
            }

            if (now >= g_fRunSwitch[client])
            {
                Run_End(client);
                return;
            }
        }
    }

    // 奔跑期间压住武器空闲动画, 避免中途插回 idle 打断
    SetEntPropFloat(weapon, Prop_Send, "m_flTimeWeaponIdle", GetGameTime() + 1.0);

    // 抖动(仅冲刺阶段; 每 RUN_SHAKE_TIME 秒触发)
    if (stage == RUN_STAGE_IN || stage == RUN_STAGE_RUN)
    {
        float shake = Run_GetFloat(weapon, "runshake", 0.0);
        if (shake > 0.0 && now >= g_fRunShakeNext[client])
        {
            // 先发超大负值(引擎clamp到0)清空累计幅度, 再发正值, 每次幅度精确=x且不累加
            Run_CreateShake(client, RUN_SHAKE_RESET_AMP, RUN_SHAKE_TIME);
            Run_CreateShake(client, shake, RUN_SHAKE_TIME);
            g_fRunShakeNext[client] = now + RUN_SHAKE_TIME;
        }
    }
}

// ============================================================================
// >> 屏幕抖动(幅度取配置 runshake, 频率/时长固定)
// ============================================================================
void Run_CreateShake(int client, float amplitude, float duration)
{
    if (IsFakeClient(client))
        return;

    Handle bf = StartMessageOne("Shake", client);
    if (bf == INVALID_HANDLE)
        return;

    BfWriteByte(bf, 0);            // 抖动类型: 开始
    BfWriteFloat(bf, amplitude);   // 幅度(配置 runshake, 可为负用于短脉冲重置)
    BfWriteFloat(bf, RUN_SHAKE_FREQ);
    BfWriteFloat(bf, duration);    // 持续时间(秒)
    EndMessage();
}

// ============================================================================
// >> 停止屏幕抖动(奔跑结束立即消除残留, 防止时长尾巴)
// ============================================================================
void Run_StopShake(int client)
{
    if (IsFakeClient(client))
        return;

    Handle bf = StartMessageOne("Shake", client);
    if (bf == INVALID_HANDLE)
        return;

    BfWriteByte(bf, 1);            // 抖动类型: 停止
    BfWriteFloat(bf, 0.0);
    BfWriteFloat(bf, 0.0);
    BfWriteFloat(bf, 0.0);
    EndMessage();
}

// ============================================================================
// >> 事件转发(开始/结束奔跑广播给其他插件)
// ============================================================================
void Run_FireForward(int client, bool running)
{
    if (g_ForwardClientRun == INVALID_HANDLE)
        return;

    Call_StartForward(g_ForwardClientRun);
    Call_PushCell(client);
    Call_PushCell(running);
    Call_Finish();
}

// ============================================================================
// >> 重置(掉线)
// ============================================================================
void Run_Reset(int client)
{
    g_bRunning[client] = false;
    g_iRunStage[client] = RUN_STAGE_NONE;
    g_bRunFrame[client] = false;
    g_bRunSpeedApplied[client] = false;
    g_iRunWeapon[client] = -1;
    g_fRunSwitch[client] = 0.0;
    g_fSpeedPressTime[client] = 0.0;
    g_bSpeedDown[client] = false;
    g_fRunShakeNext[client] = 0.0;
}