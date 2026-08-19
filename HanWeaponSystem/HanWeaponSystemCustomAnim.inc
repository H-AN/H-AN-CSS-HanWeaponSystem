// ============================================================================
// >> 自定义动画系统(最高优先级动画播放 API)
// 供外部插件播放任意 VM 动画序列(如枪托攻击), 由 OnPostThinkPost 每tick强制,
// 优先级高于 机瞄/侧瞄/奔跑/检视/换弹接管。
// - 触发时: 强制退出机瞄/侧瞄/奔跑/检视/空仓换弹接管
// - 播放中: 机瞄/侧瞄/奔跑可打断(interruptable=true 时); 检视不打断
// - 结束: 只回 idle, 不恢复被中断状态
// - 帧基准: 30fps(与检视/奔跑一致), 时长 = 帧数/30
// - 纯 API, 系统不内置任何技能/伤害逻辑
// ============================================================================

// ============================================================================
// >> 设置自定义动画(外部插件通过 Han_SetClientCustomAnim 调用)
//    seq         动画序列号
//    frames      动画时长(帧, 30fps 基准)
//    repeat      已在播放中时: true=允许重播打断自己, false=拒绝(返回 false)
//    interruptable 播放中是否可被 机瞄/侧瞄/奔跑 打断: true=可打断(默认), false=绝对霸体
//    返回 true=已开始播放, false=参数非法/未在游戏/不可重播
// ============================================================================
bool CustomAnim_Set(int client, int seq, int frames, bool repeat, bool interruptable)
{
    if (client <= 0 || client > MaxClients || !IsClientInGame(client) || !IsPlayerAlive(client))
        return false;

    if (seq < 0)
        return false;

    if (frames <= 0)
        return false;

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0)
        return false;

    bool replay = g_bCustomAnim[client];
    if (replay && !repeat)
        return false;

    // 触发即强制退出其余模块(自定义动画优先级最高)
    Aim_ForceExit(client);
    Run_End(client);
    Inspect_End(client);
    EmptyReload_End(client);

    g_bCustomAnim[client] = true;
    g_iCustomAnimSeq[client] = seq;
    g_fCustomAnimEnd[client] = GetGameTime() + (float(frames) / 30.0);
    g_iCustomAnimWeapon[client] = weapon;
    g_bCustomAnimInterruptable[client] = interruptable;

    // 已在本动画中重播: 起源引擎设置同一序列不会重播, 需先播一帧引擎原生序列再切回
    // (2=播一帧引擎原生序列, 1=下帧切回动画序列, 0=无; 由 OnPostThinkPost 执行)
    g_iCustomAnimRestart[client] = replay ? 2 : 0;

    // 首次播放: 直接强制 1号VM 动画序列
    if (g_iCustomAnimRestart[client] == 0)
    {
        if (ClientVM[client][1] > 0 && IsValidEntity(ClientVM[client][1]))
        {
            SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", seq);
            SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", 1.0);
        }
    }

    // 压住武器空闲动画, 避免中途插回 idle 打断自定义动画
    SetEntPropFloat(weapon, Prop_Send, "m_flTimeWeaponIdle", GetGameTime() + (float(frames) / 30.0));

    CustomAnim_FireForwardStart(client, seq, frames);

    return true;
}

// ============================================================================
// >> 结束自定义动画(自然结束/被打断/切枪/死亡, 只回 idle)
// ============================================================================
void CustomAnim_End(int client)
{
    if (!g_bCustomAnim[client])
        return;

    g_bCustomAnim[client] = false;
    g_iCustomAnimSeq[client] = -1;
    g_fCustomAnimEnd[client] = 0.0;
    g_iCustomAnimWeapon[client] = -1;
    g_bCustomAnimInterruptable[client] = true;
    g_iCustomAnimRestart[client] = 0;

    // 广播结束事件(含被打断/切枪/死亡, 供外部插件停止伤害/效果逻辑)
    CustomAnim_FireForwardEnd(client);

    // 恢复 VM[1] 到引擎原生序列(idle)
    if (ClientVM[client][0] > 0 && IsValidEntity(ClientVM[client][0]) &&
        ClientVM[client][1] > 0 && IsValidEntity(ClientVM[client][1]))
    {
        SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", GetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence"));
        SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", 1.0);
    }

    // 换弹/换枪期间不重置武器空闲计时, 交给引擎节奏
    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon > 0 &&
        !Weapon_IsReloading(weapon) &&
        !(HasEntProp(weapon, Prop_Send, "m_reloadState") && GetEntProp(weapon, Prop_Send, "m_reloadState") > 0))
    {
        SetEntPropFloat(weapon, Prop_Send, "m_flTimeWeaponIdle", GetGameTime());
    }
}

// ============================================================================
// >> 每帧后处理(动画强制 + 重播机制 + 结束条件检测)
//    必须挂在 ViewModel OnPostThinkPost 最末, 覆盖一切其他模块动画
// ============================================================================
void CustomAnim_OnPostThinkPost(int client)
{
    if (!g_bCustomAnim[client])
        return;

    if (!IsPlayerAlive(client))
    {
        CustomAnim_End(client);
        return;
    }

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0 || weapon != g_iCustomAnimWeapon[client])
    {
        CustomAnim_End(client);
        return;
    }

    // 重播机制: 先播一帧引擎原生序列再切回动画序列强制重播
    if (g_iCustomAnimRestart[client] == 2)
    {
        g_iCustomAnimRestart[client] = 1;
        SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", GetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence"));
        SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", 1.0);
        return;
    }
    else if (g_iCustomAnimRestart[client] == 1)
    {
        g_iCustomAnimRestart[client] = 0;
    }

    // 时长到自动结束(只回 idle)
    if (GetGameTime() >= g_fCustomAnimEnd[client])
    {
        CustomAnim_End(client);
        return;
    }

    // 持续强制 1号VM 动画序列(每tick在动画复制之后覆盖)
    if (ClientVM[client][1] > 0 && IsValidEntity(ClientVM[client][1]))
    {
        SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", g_iCustomAnimSeq[client]);
        SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", 1.0);
    }

    // 压住武器空闲动画, 避免中途插回 idle 打断自定义动画
    SetEntPropFloat(weapon, Prop_Send, "m_flTimeWeaponIdle", GetGameTime() + 1.0);
}

// ============================================================================
// >> 玩家指令(由 Combat 的 OnPlayerRunCmd 调用; 返回 true 表示已修改按键)
//    自定义动画中剔除 IN_RELOAD, 禁止开始换弹(防换弹逻辑/动画与自定义动画冲突)
// ============================================================================
bool CustomAnim_OnPlayerRunCmd(int client, int &buttons)
{
    if (!g_bCustomAnim[client])
        return false;

    if (buttons & IN_RELOAD)
    {
        buttons &= ~IN_RELOAD;
        return true;
    }

    return false;
}

// ============================================================================
// >> 事件转发(开始/结束自定义动画广播给其他插件)
// ============================================================================
void CustomAnim_FireForwardStart(int client, int seq, int frames)
{
    if (g_ForwardClientCustomAnimStart == INVALID_HANDLE)
        return;

    Call_StartForward(g_ForwardClientCustomAnimStart);
    Call_PushCell(client);
    Call_PushCell(seq);
    Call_PushCell(frames);
    Call_Finish();
}

void CustomAnim_FireForwardEnd(int client)
{
    if (g_ForwardClientCustomAnimEnd == INVALID_HANDLE)
        return;

    Call_StartForward(g_ForwardClientCustomAnimEnd);
    Call_PushCell(client);
    Call_Finish();
}

// ============================================================================
// >> 重置(掉线)
// ============================================================================
void CustomAnim_Reset(int client)
{
    g_bCustomAnim[client] = false;
    g_iCustomAnimSeq[client] = -1;
    g_fCustomAnimEnd[client] = 0.0;
    g_iCustomAnimWeapon[client] = -1;
    g_bCustomAnimInterruptable[client] = true;
    g_iCustomAnimRestart[client] = 0;
}