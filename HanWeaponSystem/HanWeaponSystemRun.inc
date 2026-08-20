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

// Bot 奔跑保持: 水平速度(runcmd vel 的 x/y 分量)低于此值视为已停下(被挡/站定), 结束奔跑
#define BOT_RUN_STOP_SPEED 25.0

// 奔跑抖动参数(弹簧阻尼器: 幅度取配置 runshake 作强度, 间隔固定无需步伐检测)
#define RUN_SHAKE_INTERVAL 0.3            // 晃动脉冲间隔(秒)
#define RUN_SHAKE_DAMPING 9.0             // 弹簧阻尼(速度衰减, Beatrun 原值)
#define RUN_SHAKE_SPRING 120.0            // 弹簧常数(拉回零点, Beatrun 原值)
#define RUN_SHAKE_PITCH_GAIN 30.0         // 俯仰(点头)注入增益(度/秒; runshake=0.6 时峰值约 1.6°)
#define RUN_SHAKE_YAW_GAIN 40.0           // 偏航(左右摆头)注入增益(度/秒; runshake=0.6 时峰值约 2.2°, 替代CS:S不存在的m_flRollAngle)

void Run_OnPluginStart()
{
    // 死亡/复活时清零抖动弹簧(防止死亡瞬间积分停止导致角度冻结)
    HookEvent("player_death", Run_EventPlayerDeath, EventHookMode_Post);
    HookEvent("player_spawn", Run_EventPlayerSpawn, EventHookMode_Post);
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
    int runtime = Run_GetIntVariant(weapon, "runtime", 15);
    if (runtime <= 0)
        runtime = 15;
    return runtime / 30.0;
}

// ============================================================================
// >> 奔跑力推动(模式同 滑铲/滑索 插件; 已取代旧版 m_flLaggedMovementValue 倍率)
//    面向角取 forward -> 贴地归一化 -> 按 runspeed(启动推力) 叠加到当前速度(不覆盖),
//    保留垂直速度; runmaxspeed 为总体速度上限(默认走路约250, 500≈2倍速), 达到后不再加力。
// ============================================================================
void Run_ApplyForce(int client, int weapon)
{
    float force = Run_GetFloat(weapon, "runspeed", 0.0);
    if (force <= 0.0)
        return;

    float maxSpeed = Run_GetFloat(weapon, "runmaxspeed", 0.0);

    float vAng[3];
    float vDir[3];
    GetClientEyeAngles(client, vAng);
    GetAngleVectors(vAng, vDir, NULL_VECTOR, NULL_VECTOR);

    vDir[2] = 0.0;
    NormalizeVector(vDir, vDir);

    float curVel[3];
    GetEntPropVector(client, Prop_Data, "m_vecVelocity", curVel);

    float horizontal = SquareRoot(curVel[0] * curVel[0] + curVel[1] * curVel[1]);
    if (maxSpeed > 0.0 && horizontal >= maxSpeed)
        return;

    ScaleVector(vDir, force);

    float vVel[3];
    AddVectors(curVel, vDir, vVel);
    vVel[2] = curVel[2];

    TeleportEntity(client, NULL_VECTOR, NULL_VECTOR, vVel);
}

// ============================================================================
// >> 触发/停止检测(由 Combat 的 OnPlayerRunCmd 调用)
// 奔跑触发方式: 前进按住时 点按Shift(短按松开)进入奔跑; 长按Shift保持静步;
// 奔跑中松开前进/停止即结束, 再次点按Shift才可再奔跑
// Bot 奔跑保持: botruncd 秒内松开前进/转向不结束奔跑, 以 runcmd 实际速度判定是否停下
// 返回 Plugin_Changed 表示已修改按键(奔跑中剔除IN_SPEED/禁止换弹)
// ============================================================================
public Action Run_OnPlayerRunCmd(int client, int &buttons, bool bReloading, const float vel[3])
{
    int stage = g_iRunStage[client];
    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");

    // 抖动弹簧积分+脉冲注入(每tick执行, 停止奔跑后弹簧自然衰减归零) ★新版
    Run_ShakeTick(client, weapon);

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
            // Bot 奔跑保持: botruncd 秒内松开前进/转向不结束奔跑, 以实际速度判定是否停下
            bool botHold = g_bBotRunHold[client] && GetGameTime() < g_fBotRunHoldEnd[client];
            float hSpeed = SquareRoot(vel[0] * vel[0] + vel[1] * vel[1]);

            bStop = (buttons & IN_ATTACK) || (buttons & IN_ATTACK2) || (buttons & IN_ATTACK3) || (buttons & IN_ALT1) || (buttons & IN_ALT2);

            if (!bStop)
            {
                if (botHold)
                {
                    // 保持期: 转向/松开前进不停止, 仅后退/跳跃/下蹲/速度近0(被挡或停下)才停
                    bStop = (buttons & IN_BACK) || (buttons & IN_JUMP) || (buttons & IN_DUCK) ||
                            (hSpeed < BOT_RUN_STOP_SPEED);
                }
                else
                {
                    // 常规: 松开前进/转向/后退/跳跃/下蹲即结束奔跑
                    bStop = !(buttons & IN_FORWARD) || (buttons & IN_BACK) || (buttons & IN_MOVELEFT) || (buttons & IN_MOVERIGHT) ||
                            (buttons & IN_JUMP) || (buttons & IN_DUCK);
                }
            }
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

    // 自定义动画中: 不可打断则不触发奔跑; 可打断则先结束自定义动画再判定
    if (g_bCustomAnim[client])
    {
        if (!g_bCustomAnimInterruptable[client])
            return;
        CustomAnim_End(client);
    }

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

    int runin = Run_GetIntVariant(weapon, "runin", -1);
    int runinlen = Run_GetIntVariant(weapon, "runinlength", 0);

    g_iRunWeapon[client] = weapon;
    g_bRunFrame[client] = false;

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

    // 速度推力由 Run_ApplyForce 每帧在 Run_OnPostThinkPost 施加

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

    // 推力为叠加式, 停止后引擎自然减速, 无需恢复

    // 已在 runout 中
    if (g_iRunStage[client] == RUN_STAGE_OUT)
        return;

    int runout = -1, runoutlen = 0;
    if (weapon > 0)
    {
        runout = Run_GetIntVariant(weapon, "runout", -1);
        runoutlen = Run_GetIntVariant(weapon, "runoutlength", 0);
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
    g_bBotRunHold[client] = false;

    // 死亡或已无武器时直接清零抖动弹簧(否则无法积分导致角度冻结); 正常停止交给弹簧自然衰减
    if (!IsPlayerAlive(client) || GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon") <= 0)
        Run_ShakeReset(client);

    // 推力为叠加式, 停止后引擎自然减速, 无需恢复

    // 恢复 idle; usp 模板武器引擎不回正, 主动写一次正确 idle(有消音=0/无消音=8)
    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    ViewModel_RestoreIdle(client, weapon);

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

    // 奔跑力推动(仅实际冲刺阶段施加; OUT 阶段不推, 自然减速过渡)
    if (stage == RUN_STAGE_IN || stage == RUN_STAGE_RUN)
        Run_ApplyForce(client, weapon);

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

            int runseq = Run_GetIntVariant(weapon, "runseq", -1);
            if (runseq >= 0)
            {
                int seq = runseq + (g_bRunFrame[client] ? 1 : 0);
                SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", seq);
                SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", 1.0);
            }
        }

        case RUN_STAGE_OUT:
        {
            int runout = Run_GetIntVariant(weapon, "runout", -1);
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
}

// ============================================================================
// >> 新版抖动: 弹簧阻尼器 + 固定间隔脉冲(Beatrun 移植, 无需步伐检测)
//    奔跑中每 RUN_SHAKE_INTERVAL 秒注入一次脉冲, 弹簧把每脚平滑成连续摆动并
//    自然衰减; 停止奔跑后弹簧自动归零, 无抖动尾巴。
//    CS:S 玩家无 m_flRollAngle(引擎内算侧倾, 服务端不可写), 故用 m_vecPunchAngle
//    的 x(俯仰点头)/y(偏航左右摆头) 驱动。z(roll) 保持不变。
// ============================================================================
void Run_ShakeTick(int client, int weapon)
{
    float FT = GetTickInterval();
    if (FT <= 0.0)
        FT = 0.015;

    // 弹簧-阻尼积分(偏航 yaw, 对应 Beatrun 阻尼9/弹簧120)
    g_fRunShakeYawVel[client] += (-g_fRunShakeYaw[client] * RUN_SHAKE_SPRING - g_fRunShakeYawVel[client] * RUN_SHAKE_DAMPING) * FT;
    g_fRunShakeYaw[client] += g_fRunShakeYawVel[client] * FT;

    // 弹簧-阻尼积分(俯仰 pitch)
    g_fRunShakePitchVel[client] += (-g_fRunShakePitch[client] * RUN_SHAKE_SPRING - g_fRunShakePitchVel[client] * RUN_SHAKE_DAMPING) * FT;
    g_fRunShakePitch[client] += g_fRunShakePitchVel[client] * FT;

    // 奔跑中(冲刺阶段)每 RUN_SHAKE_INTERVAL 注入一次脉冲
    if (weapon > 0 && (g_iRunStage[client] == RUN_STAGE_IN || g_iRunStage[client] == RUN_STAGE_RUN))
    {
        float shake = Run_GetFloat(weapon, "runshake", 0.0);
        if (shake > 0.0 && GetGameTime() >= g_fRunShakeNext[client])
        {
            // 交替符号(模拟左右脚交替摆头, 无需步伐检测)
            g_bRunShakeSign[client] = !g_bRunShakeSign[client];
            float sign = g_bRunShakeSign[client] ? 1.0 : -1.0;

            g_fRunShakeYawVel[client] += sign * shake * RUN_SHAKE_YAW_GAIN;
            g_fRunShakePitchVel[client] += shake * RUN_SHAKE_PITCH_GAIN;

            g_fRunShakeNext[client] = GetGameTime() + RUN_SHAKE_INTERVAL;
        }
    }

    // 有晃动能量才写回引擎(避免覆盖受击/伤害 punch)
    if (g_iRunStage[client] != RUN_STAGE_NONE ||
        FloatAbs(g_fRunShakeYaw[client]) > 0.01 || FloatAbs(g_fRunShakeYawVel[client]) > 0.01 ||
        FloatAbs(g_fRunShakePitch[client]) > 0.01 || FloatAbs(g_fRunShakePitchVel[client]) > 0.01)
    {
        float punch[3];
        GetEntPropVector(client, Prop_Send, "m_vecPunchAngle", punch);
        punch[0] = g_fRunShakePitch[client];
        punch[1] = g_fRunShakeYaw[client];
        SetEntPropVector(client, Prop_Send, "m_vecPunchAngle", punch);
    }
}

// ============================================================================
// >> 清零抖动弹簧状态(死亡/掉线/换武器时防角度冻结)
// ============================================================================
void Run_ShakeReset(int client)
{
    g_fRunShakeYaw[client] = 0.0;
    g_fRunShakeYawVel[client] = 0.0;
    g_fRunShakePitch[client] = 0.0;
    g_fRunShakePitchVel[client] = 0.0;
    g_bRunShakeSign[client] = false;
    g_fRunShakeNext[client] = 0.0;
}

// ============================================================================
// >> 死亡/复活: 清零抖动弹簧(防止死亡瞬间积分停止导致角度冻结)
// ============================================================================
public void Run_EventPlayerDeath(Event event, const char[] name, bool dontBroadcast)
{
    int client = GetClientOfUserId(GetEventInt(event, "userid"));
    if (client > 0 && client <= MaxClients)
        Run_ShakeReset(client);
}

public void Run_EventPlayerSpawn(Event event, const char[] name, bool dontBroadcast)
{
    int client = GetClientOfUserId(GetEventInt(event, "userid"));
    if (client > 0 && client <= MaxClients)
        Run_ShakeReset(client);
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
    g_iRunWeapon[client] = -1;
    g_fRunSwitch[client] = 0.0;
    g_fSpeedPressTime[client] = 0.0;
    g_bSpeedDown[client] = false;
    g_fRunShakeNext[client] = 0.0;
    g_fRunShakeYaw[client] = 0.0;
    g_fRunShakeYawVel[client] = 0.0;
    g_fRunShakePitch[client] = 0.0;
    g_fRunShakePitchVel[client] = 0.0;
    g_bRunShakeSign[client] = false;
}