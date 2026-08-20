// ============================================================================
// >> 空仓/战术换弹系统(换弹时按弹匣状态强制不同动画序列 + 一次性广播)
// ============================================================================
// >> 原作者声明 / Original Author Credit
// 本模块的换弹动画逻辑移植自开源社区插件 "Multiple Reloading Action"。
// This module's reload animation logic is ported from the open-source plugin
// "Multiple Reloading Action".
//
// 原作者 / Original Author: Srk (cjsrk)
// 版本 / Version: 0.1
//
// 感谢原作者的付出！原插件仅供学习参考，本插件已在此基础上大幅重构改写。
// Special thanks to the original author for the work! The original plugin is
// used for reference only; this plugin has been heavily reworked on top of it.
// ============================================================================

void EmptyReload_OnPluginStart()
{
    // 无独立命令/cvar，触发挂载在换弹音效钩子 Hook_WeaponReloadPost(Sounds.inc) 内
}

// ============================================================================
// >> 判断武器是否开启空仓换弹/战术换弹
// ============================================================================
bool EmptyReload_IsEmptyEnabled(int weapon)
{
    return EmptyReload_GetInt(weapon, "emptyreload", 0) > 0;
}

bool EmptyReload_IsTacticalEnabled(int weapon)
{
    return EmptyReload_GetInt(weapon, "tacticalreload", 0) > 0;
}

// ============================================================================
// >> 换弹触发(由 Sounds.inc 的 Hook_WeaponReloadPost 调用, 不额外挂钩子)
// 此刻 m_iClip1 仍是换弹前弹匣值, 正好判定空仓/战术; 返回是否已接管该换弹动画
// ============================================================================
bool EmptyReload_OnWeaponReloadPost(int weapon)
{
    if (weapon <= 0 || !IsValidEntity(weapon))
        return false;

    int client = GetEntPropEnt(weapon, Prop_Send, "m_hOwnerEntity");
    if (client <= 0 || client > MaxClients || !IsClientInGame(client) || !IsPlayerAlive(client))
        return false;

    // 备弹为0时引擎本就不会开始换弹, 防御性跳过
    int ammoType = GetEntProp(weapon, Prop_Data, "m_iPrimaryAmmoType");
    if (ammoType > -1)
    {
        int reserve = GetEntProp(client, Prop_Data, "m_iAmmo", 4, ammoType);
        if (reserve <= 0)
            return false;
    }

    int clip = GetEntProp(weapon, Prop_Send, "m_iClip1");
    bool empty = (clip == 0);

    // 霰弹枪(m_reloadState)不走时长同步, 走全局reloadspeed
    bool timing = !HasEntProp(weapon, Prop_Send, "m_reloadState");

    // 按弹匣状态选定序列 + 是否启用对应换弹动画
    int seq;
    int timeFrames;
    int fps = EmptyReload_GetInt(weapon, "reloadfps", 30);
    if (fps <= 0)
        fps = 30;

    if (empty)
    {
        if (!EmptyReload_IsEmptyEnabled(weapon))
            return false;

        seq = EmptyReload_GetSeq(weapon, "emptyreloadseq");
        if (seq < 0)
            return false;

        timeFrames = EmptyReload_GetIntVariant(weapon, "emptyreloadtime", 0);
        if (timing && timeFrames <= 0)
            timing = false;

        EmptyReload_Start(client, weapon, seq, timing, timeFrames, fps);
        EmptyReload_FireForward(g_ForwardClientEmptyReload, client);
        // PrintToServer("[HWS] 空仓换弹接管: seq=%d timing=%d frames=%d fps=%d", seq, timing, timeFrames, fps);
        return true;
    }
    else
    {
        if (!EmptyReload_IsTacticalEnabled(weapon))
            return false;

        seq = EmptyReload_GetSeq(weapon, "reloadseq");
        if (seq < 0)
            return false;

        timeFrames = EmptyReload_GetIntVariant(weapon, "reloadtime", 0);
        if (timing && timeFrames <= 0)
            timing = false;

        EmptyReload_Start(client, weapon, seq, timing, timeFrames, fps);
        EmptyReload_FireForward(g_ForwardClientTacticalReload, client);
        // PrintToServer("[HWS] 战术换弹接管: seq=%d timing=%d frames=%d fps=%d", seq, timing, timeFrames, fps);
        return true;
    }
}

// ============================================================================
// >> 获取换弹动画序列号(消音器武器且配置了变体序列时用变体, 否则回落普通字段)
// ============================================================================
int EmptyReload_GetSeq(int weapon, const char[] baseKey)
{
    return EmptyReload_GetIntVariant(weapon, baseKey, -1);
}

// ============================================================================
// >> 开始接管换弹动画(记录状态, 由 OnPostThinkPost 每tick强制序列)
// ============================================================================
void EmptyReload_Start(int client, int weapon, int seq, bool timing, int timeFrames, int fps)
{
    g_bEmptyReloading[client] = true;
    g_iEmptyReloadWeapon[client] = weapon;
    g_iEmptyReloadSeq[client] = seq;

    g_bEmptyReloadTiming[client] = timing;
    g_iEmptyReloadTimeFrames[client] = timeFrames;
    g_iEmptyReloadFps[client] = fps;
    g_fEmptyReloadStart[client] = GetGameTime();
}

// ============================================================================
// >> 结束换弹动画接管(恢复空闲)
// ============================================================================
void EmptyReload_End(int client)
{
    if (!g_bEmptyReloading[client])
        return;

    int weapon = g_iEmptyReloadWeapon[client];

    g_bEmptyReloading[client] = false;
    g_iEmptyReloadWeapon[client] = -1;
    g_iEmptyReloadSeq[client] = -1;

    g_bEmptyReloadTiming[client] = false;
    g_iEmptyReloadTimeFrames[client] = 0;
    g_iEmptyReloadFps[client] = 0;
    g_fEmptyReloadStart[client] = 0.0;

    // 恢复 idle; usp 模板武器引擎不回正, 主动写一次正确 idle(有消音=0/无消音=8)
    ViewModel_RestoreIdle(client, weapon);
}

// ============================================================================
// >> 每帧后处理(强制换弹序列 + 结束条件检测)
// ============================================================================
void EmptyReload_OnPostThinkPost(int client)
{
    if (!g_bEmptyReloading[client])
        return;

    if (!IsPlayerAlive(client))
    {
        EmptyReload_End(client);
        return;
    }

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0 || weapon != g_iEmptyReloadWeapon[client])
    {
        EmptyReload_End(client);
        return;
    }

    // 换弹已结束/被打断(切枪/死亡) → 结束接管
    bool reloading = Weapon_IsReloading(weapon) ||
        (HasEntProp(weapon, Prop_Send, "m_reloadState") && GetEntProp(weapon, Prop_Send, "m_reloadState") > 0);
    if (!reloading)
    {
        EmptyReload_End(client);
        return;
    }

    // 持续强制 1号VM 换弹序列(每tick在动画复制之后覆盖)
    if (ClientVM[client][1] > 0 && IsValidEntity(ClientVM[client][1]))
    {
        SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", g_iEmptyReloadSeq[client]);
    }

    // 时长同步模式: 播放速率固定1.0(动画按原始速度播放), 换弹完成时刻 = 开始 + 帧数/fps
    if (g_bEmptyReloadTiming[client])
    {
        SetEntPropFloat(weapon, Prop_Send, "m_flPlaybackRate", 1.0);
        if (ClientVM[client][0] > 0 && IsValidEntity(ClientVM[client][0]))
            SetEntPropFloat(ClientVM[client][0], Prop_Send, "m_flPlaybackRate", 1.0);
        if (ClientVM[client][1] > 0 && IsValidEntity(ClientVM[client][1]))
            SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", 1.0);

        EmptyReload_ApplyTiming(client, weapon);
    }
}

// ============================================================================
// >> 应用换弹时长(同步计时)
//    结束时刻 = 接管起始 + 配置帧数 / fps(默认30, 按需配置24)
// ============================================================================
void EmptyReload_ApplyTiming(int client, int weapon)
{
    if (g_iEmptyReloadTimeFrames[client] <= 0)
        return;

    int fps = g_iEmptyReloadFps[client];
    if (fps <= 0)
        fps = 30;

    float endTime = g_fEmptyReloadStart[client] + (float(g_iEmptyReloadTimeFrames[client]) / float(fps));
    if (endTime <= GetGameTime())
        return;

    SetEntPropFloat(weapon, Prop_Send, "m_flTimeWeaponIdle", endTime);

    SetEntPropFloat(weapon, Prop_Send, "m_flNextPrimaryAttack", endTime);
    SetEntPropFloat(weapon, Prop_Send, "m_flNextSecondaryAttack", endTime);
    SetEntPropFloat(client, Prop_Send, "m_flNextAttack", endTime);
}

// ============================================================================
// >> 事件转发(开始空仓/战术换弹一次性广播给其他插件)
// ============================================================================
void EmptyReload_FireForward(Handle fwd, int client)
{
    if (fwd == INVALID_HANDLE)
        return;

    Call_StartForward(fwd);
    Call_PushCell(client);
    Call_Finish();
}

// ============================================================================
// >> 重置(死亡/掉线)
// ============================================================================
void EmptyReload_Reset(int client)
{
    g_bEmptyReloading[client] = false;
    g_iEmptyReloadWeapon[client] = -1;
    g_iEmptyReloadSeq[client] = -1;

    g_bEmptyReloadTiming[client] = false;
    g_iEmptyReloadTimeFrames[client] = 0;
    g_iEmptyReloadFps[client] = 0;
    g_fEmptyReloadStart[client] = 0.0;
}