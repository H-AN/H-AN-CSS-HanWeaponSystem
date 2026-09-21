// ============================================================================
// >> ViewModel 系统(双VM显示 + 动画同步)
// ============================================================================

void ViewModel_OnPluginStart()
{
    for (int client = 1; client <= MaxClients; client++)
    {
        ViewModel_ResetShootQcFix(client);
        if (IsClientInGame(client))
        {
            SDKHook(client, SDKHook_PostThinkPost, OnPostThinkPost);

            ClientVM[client][0] = GetEntPropEnt(client, Prop_Send, "m_hViewModel");

            int PVM = -1;
            while ((PVM = FindEntityByClassname(PVM, "predicted_viewmodel")) != -1)
            {
                if (GetEntPropEnt(PVM, Prop_Send, "m_hOwner") == client)
                {
                    if (GetEntProp(PVM, Prop_Send, "m_nViewModelIndex") == 1)
                    {
                        ClientVM[client][1] = PVM;
                        break;
                    }
                }
            }
        }
    }
}

// ============================================================================
// >> 玩家进服初始化(补取VM句柄, 防止连接时viewmodel已存在且不再spawn导致ClientVM残留-1)
// ============================================================================
void ViewModel_OnClientPutInServer(int client)
{
    ClientVM[client][0] = GetEntPropEnt(client, Prop_Send, "m_hViewModel");

    int PVM = -1;
    while ((PVM = FindEntityByClassname(PVM, "predicted_viewmodel")) != -1)
    {
        if (GetEntPropEnt(PVM, Prop_Send, "m_hOwner") == client)
        {
            if (GetEntProp(PVM, Prop_Send, "m_nViewModelIndex") == 1)
            {
                ClientVM[client][1] = PVM;
                break;
            }
        }
    }
}

// ============================================================================
// >> armfix 专用修复实体(weapon_ak47): 懒创建 + EntRef 复用
//    旧方案遍历地图找现成 prop_physics_multiplayer 指向 m_hWeapon 修左手模型,
//    在 SM1.11 32位旧版 CS:S 下崩溃且依赖地图实体; 改用插件自建 weapon_ak47,
//    停物理/关碰撞/传送地图外防拾取, 实测新旧环境均稳定不崩溃。
// ============================================================================
int ViewModel_EnsureArmFixEnt()
{
    int fixEnt = EntRefToEntIndex(g_iArmFixEntRef);
    if (fixEnt != INVALID_ENT_REFERENCE && IsValidEntity(fixEnt))
        return fixEnt;

    fixEnt = CreateEntityByName("weapon_ak47");
    if (fixEnt == -1)
        return -1;

    DispatchSpawn(fixEnt);

    // 停止物理模拟(防vphysics持续下坠刷"Ignoring unreasonable position") + 关闭碰撞
    SetEntityMoveType(fixEnt, MOVETYPE_NONE);
    SetEntProp(fixEnt, Prop_Data, "m_nSolidType", 0);
    SetEntProp(fixEnt, Prop_Send, "m_CollisionGroup", 0);

    // 挪到地图外, 防玩家触碰拾取
    TeleportEntity(fixEnt, view_as<float>({0.0, 0.0, -10000.0}), NULL_VECTOR, NULL_VECTOR);

    g_iArmFixEntRef = EntIndexToEntRef(fixEnt);
    return fixEnt;
}

// ============================================================================
// >> 实体创建(追踪 predicted_viewmodel / 霰弹枪换弹钩子 / 手雷投射物)
// ============================================================================
public void OnEntityCreated(int entity, const char[] classname)
{
    Knife_EntityCreated(entity);
    if (StrEqual(classname, "predicted_viewmodel", false))
    {
        SDKHook(entity, SDKHook_Spawn, OnEntitySpawned);
    }

    if (HasEntProp(entity, Prop_Send, "m_reloadState"))
    {
        SDKHook(entity, SDKHook_ReloadPost, Hook_OnReloadPost);
    }

    // 手雷投射物(hegrenade/flashbang/smokegrenade 等 *_projectile): 挂 SpawnPost 反查自定义手雷飞行模型
    int len = strlen(classname);
    if (len >= 11 && StrContains(classname, "_projectile") == len - 11)
    {
        SDKHook(entity, SDKHook_SpawnPost, GrenadeProjectileSpawned);
    }
}

public void OnEntitySpawned(int entity)
{
    int Owner = GetEntPropEnt(entity, Prop_Send, "m_hOwner");
    if ((Owner > 0) && (Owner <= MaxClients))
    {
        Han_ModelReset(Owner);
        if (GetEntProp(entity, Prop_Send, "m_nViewModelIndex") == 0)
        {
            ClientVM[Owner][0] = entity;
            // 新VM实体出现, 传输屏蔽钩子需重新挂(保险)
            g_bVM0BlockHooked[Owner] = false;
        }
        else if (GetEntProp(entity, Prop_Send, "m_nViewModelIndex") == 1)
        {
            ClientVM[Owner][1] = entity;
        }
    }
}

// ============================================================================
// >> 消音器切换检测(边沿): 消音器装上/取下瞬间强制退出机瞄/侧瞄/检视/奔跑
//    并重挂世界模型(消音/未消音模型切换)。非变体武器直接跳过。
//    换弹途中引擎天然禁止消音操作, 无需处理。
// ============================================================================
void ViewModel_CheckSilencerToggle(int client, int weapon)
{
    if (!IsVariantWeapon(weapon))
    {
        g_bPrevSilencer[client] = false;
        return;
    }

    bool on = GetEntProp(weapon, Prop_Send, "m_bSilencerOn") == 1;
    if (on == g_bPrevSilencer[client])
        return;

    g_bPrevSilencer[client] = on;

    // 强制退出开镜(机瞄/侧瞄)
    if (g_bZooming[client] || g_iZoomInSwitch[client] != 0 || g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0)
        Aim_ForceExit(client);

    // 强制退出检视
    if (g_bInspecting[client])
        Inspect_End(client);

    // 强制退出奔跑
    if (g_iRunStage[client] != RUN_STAGE_NONE)
        Run_End(client);

    // 重挂世界模型(消音/未消音模型切换; 与引擎自身切模型可能共存, 待上服验证)
    ApplyWeaponWorldModel(weapon);
}

// ============================================================================
// >> 每帧处理(核心: 模型切换 + 动画同步 + 机瞄)
// ============================================================================
public void OnPostThinkPost(int client)
{
    if (client <= 0 || client > MaxClients || !IsClientInGame(client))
        return;

    static int OldWeapon[MAXPLAYERS + 1];
    static int OldSequence[MAXPLAYERS + 1];
    static float OldCycle[MAXPLAYERS + 1];
    static int Data[MAXPLAYERS + 1];

    char ClassName[64];
    int WeaponIndex;

    char vmodelpath[PLATFORM_MAX_PATH];
    char SkinCode[32];

    // 观战者显示目标武器模型
    if (!IsPlayerAlive(client))
    {
        int spec = GetEntPropEnt(client, Prop_Send, "m_hObserverTarget");
        if (spec != -1)
        {
            WeaponIndex = GetEntPropEnt(spec, Prop_Send, "m_hActiveWeapon");
            if (WeaponIndex <= 0)
                return;

            GetEntityClassname(WeaponIndex, ClassName, sizeof(ClassName));
            if (GetWeaponConfigValue(WeaponIndex, "vmodel", vmodelpath, sizeof(vmodelpath)))
            {
                int modelIndex = PrecacheModel(vmodelpath, true);
                SetEntProp(ClientVM[client][1], Prop_Send, "m_nModelIndex", modelIndex); 
            }
        }

        return;
    }

    WeaponIndex = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");

    if (ClientVM[client][0] <= MaxClients || !IsValidEntity(ClientVM[client][0])
        || ClientVM[client][1] <= MaxClients || !IsValidEntity(ClientVM[client][1]))
    {
        Han_ModelReset(client);
        return;
    }

    int Sequence = GetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence");
    float Cycle = GetEntPropFloat(ClientVM[client][0], Prop_Data, "m_flCycle");
    float PlaybackRate = GetEntPropFloat(ClientVM[client][0], Prop_Send, "m_flPlaybackRate");

    if (WeaponIndex <= 0)
    {
        Han_ModelReset(client);
        int EntEffects = GetEntProp(ClientVM[client][1], Prop_Send, "m_fEffects");
        EntEffects |= EF_NODRAW;
        SetEntProp(ClientVM[client][1], Prop_Send, "m_fEffects", EntEffects);

        IsCustom[client] = false;
        IsCustom2[client] = false;
        g_bVM0BlockHooked[client] = false;
        g_bShootQcFix[client] = false;
        g_bQcFixShot[client] = false;
        g_iQcFixRestore[client] = -1;
        Data[client] = 0;

        // 摘除 0号VM 传输屏蔽钩子(无武器时保险, 防死亡/换武器后钩子残留)
        if (IsValidEntity(ClientVM[client][0]))
            SDKUnhook(ClientVM[client][0], SDKHook_SetTransmit, OnTransmit);

        OldWeapon[client] = WeaponIndex;
        OldSequence[client] = Sequence;
        OldCycle[client] = Cycle;

        return;
    }

    // 换武器
    if (WeaponIndex != OldWeapon[client] || EntRefToEntIndex(g_HanModelWeapon[client]) != WeaponIndex)
    {
        // 活动武器已变化，直接保存重置前确认的 VM 模式，避免查询 API 返回 NotReady。
        HanViewModelMode previousMode = g_HanModelMode[client];
        Han_ModelReset(client);
        // 换武器强制退出机瞄/侧瞄
        if (g_bZooming[client] || g_iZoomInSwitch[client] != 0 || g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0)
            Aim_ForceExit(client);

        // 换武器强制结束自定义动画(防旧序列残留到新武器, 广播结束事件)
        if (g_bCustomAnim[client])
            CustomAnim_End(client);

        GetEdictClassname(WeaponIndex, ClassName, sizeof(ClassName));

        // 自动连发标记(有效值: 配置开启且非原生开镜且无插件机瞄; 右键被机瞄/原生开镜占用则自动失效)
        char AutoFire[32];
        g_bWeaponAutoFire[client] = false;
        if (GetWeaponConfigValue(WeaponIndex, "autofire", AutoFire, sizeof(AutoFire)) && StringToInt(AutoFire) > 0)
        {
            if (!Zoom_IsNativeScopeWeapon(WeaponIndex) && !Zoom_IsWeaponZoomable(WeaponIndex))
            {
                g_bWeaponAutoFire[client] = true;
            }
        }

        // 开火动画修复标记(shootqcfixes: 换武器时缓存, 每tick修复块只查内存不走KV树)
        g_bShootQcFix[client] = Weapon_IsShootQcFix(WeaponIndex);
        g_bQcFixShot[client] = false;
        g_iQcFixRestore[client] = -1;

        // 机瞄/侧瞄开火动画同步开关(zoomanimfire/sideaimanimfire, 默认0=同步; 换武器时缓存)
        char fireSyncStr[8];
        g_bZoomAnimFire[client] = false;
        g_bSideAimAnimFire[client] = false;
        if (GetZoomConfigValue(WeaponIndex, "zoomanimfire", fireSyncStr, sizeof(fireSyncStr)) && StringToInt(fireSyncStr) > 0)
            g_bZoomAnimFire[client] = true;
        if (GetZoomConfigValue(WeaponIndex, "sideaimanimfire", fireSyncStr, sizeof(fireSyncStr)) && StringToInt(fireSyncStr) > 0)
            g_bSideAimAnimFire[client] = true;

        if (GetWeaponConfigValue(WeaponIndex, "vmodel", vmodelpath, sizeof(vmodelpath)) && strlen(vmodelpath) > 0)
        {
            int weaponTeam = GetWeaponTeam(WeaponIndex);
            if (weaponTeam != 0 && weaponTeam != GetClientTeam(client))
                return;

            int modelIndex = PrecacheModel(vmodelpath, true);

            int EntEffects = GetEntProp(ClientVM[client][0], Prop_Send, "m_fEffects");
            EntEffects |= EF_NODRAW;
            SetEntProp(ClientVM[client][0], Prop_Send, "m_fEffects", EntEffects);

            EntEffects = GetEntProp(ClientVM[client][1], Prop_Send, "m_fEffects");
            EntEffects &= ~EF_NODRAW;
            SetEntProp(ClientVM[client][1], Prop_Send, "m_fEffects", EntEffects);

            SetEntProp(ClientVM[client][1], Prop_Send, "m_nModelIndex", modelIndex); 

            // armfix: 需要修复的武器, 用插件自建专用修复实体(weapon_ak47)指向它
            char armfixStr[8];
            if (GetWeaponConfigValue(WeaponIndex, "armfix", armfixStr, sizeof(armfixStr)) && StringToInt(armfixStr) > 0)
            {
                int fixEnt = ViewModel_EnsureArmFixEnt();
                if (fixEnt != -1)
                    SetEntPropEnt(ClientVM[client][1], Prop_Send, "m_hWeapon", fixEnt);
                else
                    SetEntPropEnt(ClientVM[client][1], Prop_Send, "m_hWeapon", GetEntPropEnt(ClientVM[client][0], Prop_Send, "m_hWeapon"));
            }
            else
            {
                SetEntPropEnt(ClientVM[client][1], Prop_Send, "m_hWeapon", GetEntPropEnt(ClientVM[client][0], Prop_Send, "m_hWeapon"));
            }

            SetModelSequenceFlags(client, WeaponIndex);
            
            // 仅修复已确认的 VM0 -> VM1 切换；VM1 -> VM1 和旧模式未知时不插入临时序列。
            if (!OldWeaponFixes && previousMode == HanViewModel_VM0)
            {
                // 若 VM1 现存 seq 恰好等于 VM0 当前 seq(快切碰撞),
                // 先踢到不同值, 下tick常规同步拷贝 draw seq 时必产生 seq 跳变 → 客户端必定从第0帧重播
                int vmSeq = GetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence");
                if (GetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence") == vmSeq)
                    SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", (vmSeq == 0) ? 1 : 0);
            }

            // 切入自定义模型时单独归零动画进度，VM1 -> VM1 也执行，但不插入临时序列。
            SetEntPropFloat(ClientVM[client][1], Prop_Data, "m_flCycle", 0.0);
            
            SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", GetEntPropFloat(ClientVM[client][0], Prop_Send, "m_flPlaybackRate"));

            if (GetWeaponConfigValue(WeaponIndex, "skincode", SkinCode, sizeof(SkinCode)))
            {
                int skin = StringToInt(SkinCode);
                SetEntProp(WeaponIndex, Prop_Send, "m_nSkin", skin);
                SetEntProp(ClientVM[client][1], Prop_Send, "m_nSkin", skin);
            }

            SDKHook(ClientVM[client][0], SDKHook_SetTransmit, OnTransmit);
            g_bVM0BlockHooked[client] = true;
            Han_ModelConfirm(client, WeaponIndex, HanViewModel_VM1);
        }
        else
        {
            if(OldWeaponFixes)
            {
                Data[client] = 0;
                OldWeapon_HandleSwitch(client, WeaponIndex);
            }
            else
            {
                int EntEffects = GetEntProp(ClientVM[client][1], Prop_Send, "m_fEffects");
                EntEffects |= EF_NODRAW;
                SetEntProp(ClientVM[client][1], Prop_Send, "m_fEffects", EntEffects);

                SDKUnhook(ClientVM[client][0], SDKHook_SetTransmit, OnTransmit);
                g_bVM0BlockHooked[client] = false;

                // 切到原版武器: 清seq0技巧计数器残留(该块停摆期间Data会冻结, 防下次切入误吃draw)
                Data[client] = 0;

                // 恢复 VM0 可见(非自定义武器必须显示, 清死亡/切枪残留的隐藏状态)
                if (IsValidEntity(ClientVM[client][0]))
                {
                    EntEffects = GetEntProp(ClientVM[client][0], Prop_Send, "m_fEffects");
                    EntEffects &= ~EF_NODRAW;
                    SetEntProp(ClientVM[client][0], Prop_Send, "m_fEffects", EntEffects);
                }

                IsCustom[client] = false;
                IsCustom2[client] = false;
                g_bShootQcFix[client] = false;
                Han_ModelConfirm(client, WeaponIndex, HanViewModel_VM0);
            }
        }
    }
    else
    {
        if (IsCustom[client])
        {
            // 保险: 新武器每tick确保 0号VM 传输屏蔽钩子已挂(防切枪/重建后漏勾穿帮)
            if (!g_bVM0BlockHooked[client] && ClientVM[client][0] > 0 && IsValidEntity(ClientVM[client][0]))
            {
                SDKHook(ClientVM[client][0], SDKHook_SetTransmit, OnTransmit);
                g_bVM0BlockHooked[client] = true;
            }

            // 复制动画数据到 1号VM
            // zoomanimfire/sideaimanimfire=1 时, 完全瞄准状态下不把开火动画同步给镜内模型
            // (过渡期 InSwitch 不抑制, 保证衔接动画正常; 默认0=照常同步)
            bool bSuppressFireSync = false;
            if (g_iZoomInSwitch[client] == 0 && g_iSideAimInSwitch[client] == 0)
            {
                if (g_bZooming[client])
                    bSuppressFireSync = g_bZoomAnimFire[client];
                else if (g_bSideAiming[client])
                    bSuppressFireSync = g_bSideAimAnimFire[client];
            }

            if (!bSuppressFireSync)
                SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", GetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence"));

            SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", GetEntPropFloat(ClientVM[client][0], Prop_Send, "m_flPlaybackRate"));

            SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", PlaybackRate);

            // 刀类动画修复: cycle 回绕时强制显示 0 号序列数帧(消音器变体武器 m4a1/usp模板 跳过, 避免强制显示有消音器 idle=seq0)
            if (!IsVariantWeapon(WeaponIndex))
            {
                if ((Cycle < OldCycle[client]) && (Sequence == OldSequence[client]))
                {
                    Data[client] = 5;
                    SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", 0);
                }
                else if (Sequence == OldSequence[client])
                {
                    if (Data[client] > 0)
                    {
                        Data[client] -= 1;
                        SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", 0);
                    }
                    else if (Data[client])
                        Data[client] = 0;
                }
                else if (Data[client])
                    Data[client] = 0;
            }
        }
        else if (IsCustom2[client])
        {
            // 保险: 新武器每tick确保 0号VM 传输屏蔽钩子已挂(防切枪/重建后漏勾穿帮)
            if (!g_bVM0BlockHooked[client] && ClientVM[client][0] > 0 && IsValidEntity(ClientVM[client][0]))
            {
                SDKHook(ClientVM[client][0], SDKHook_SetTransmit, OnTransmit);
                g_bVM0BlockHooked[client] = true;
            }

            SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", GetEntPropFloat(ClientVM[client][0], Prop_Send, "m_flPlaybackRate"));

            SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", PlaybackRate);

            // 消音器变体武器 m4a1/usp模板 跳过 seq0 回绕, 避免强制显示有消音器 idle=seq0
            if (!IsVariantWeapon(WeaponIndex))
            {
                if ((Cycle < OldCycle[client]) && (Sequence == OldSequence[client]))
                {
                    Data[client] = 5;
                    SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", 0);
                }
                else if (Sequence == OldSequence[client])
                {
                    if (Data[client] > 0)
                    {
                        Data[client] -= 1;
                        SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", 0);
                    }
                    else if (Data[client])
                        Data[client] = 0;
                }
                else if (Data[client])
                    Data[client] = 0;
            }
        }
    }

    // 隐藏 0号VM 一帧后生效
    if (SpawnCheck[client])
    {
        SpawnCheck[client] = false;
        if (IsCustom[client] || IsCustom2[client])
        {
            int EntEffects = GetEntProp(ClientVM[client][0], Prop_Send, "m_fEffects");
            EntEffects |= EF_NODRAW;
            SetEntProp(ClientVM[client][0], Prop_Send, "m_fEffects", EntEffects);
        }
    }

    // 开火动画修复(shootqcfixes): 须在 Old* 更新前调用(需上一tick seq/cycle 做回绕判定)
    ViewModel_ShootQcFix(client, WeaponIndex, Sequence, Cycle, OldSequence[client], OldCycle[client]);

    OldWeapon[client] = WeaponIndex;
    OldSequence[client] = Sequence;
    OldCycle[client] = Cycle;

    // 消音器切换检测(边沿): 切换瞬间强制退出机瞄/侧瞄/检视/奔跑 + 重挂W模型
    // 换弹途中引擎天然禁止消音操作, 无需处理
    ViewModel_CheckSilencerToggle(client, WeaponIndex);

    // 机瞄后处理(屏震降低/衔接提前结束)
    Zoom_OnPostThinkPost(client);

    // 检视后处理(动画强制/结束条件/手电屏蔽)
    Inspect_OnPostThinkPost(client);

    // 奔跑后处理(状态推进/动画强制/抖动/结束条件)
    Run_OnPostThinkPost(client);

    // 空仓/战术换弹后处理(强制换弹序列/结束条件)
    EmptyReload_OnPostThinkPost(client);

    // 自定义动画后处理(最高优先级, 覆盖一切其他模块动画)
    CustomAnim_OnPostThinkPost(client);
}

// 隐藏死亡v模型
public void Event_PlayerDeath(Handle event, const char[] name, bool dontBroadcast)
{
    int UserId = GetEventInt(event, "userid");
    int client = GetClientOfUserId(UserId);
    Knife_Reset(client);
    Han_ModelReset(client);

    // 强制退出机瞄/侧瞄
    if (g_bZooming[client] || g_iZoomInSwitch[client] != 0 || g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0)
        Aim_ForceExit(client);

    // 强制退出检视
    Inspect_End(client);

    // 强制退出奔跑(恢复速度与动画)
    Run_End(client);

    // 强制结束空仓/战术换弹接管(恢复VM[1])
    EmptyReload_End(client);

    // 强制结束自定义动画(广播结束事件)
    CustomAnim_End(client);

    IsCustom[client] = false;
    IsCustom2[client] = false;
    g_bVM0BlockHooked[client] = false;

    // 摘除 0号VM 传输屏蔽钩子: 手雷死亡时武器实体释放索引被复活武器复用, 换武器分支可能跳过,
    // 钩子残留会导致复活后手枪/默认武器隐身, 必须在死亡清理里显式摘除
    if (IsValidEntity(ClientVM[client][0]))
        SDKUnhook(ClientVM[client][0], SDKHook_SetTransmit, OnTransmit);

    int EntEffects = GetEntProp(ClientVM[client][1], Prop_Send, "m_fEffects");
    EntEffects |= EF_NODRAW;
    SetEntProp(ClientVM[client][1], Prop_Send, "m_fEffects", EntEffects);

    // 清 VM0 隐藏残留(死亡时若正持自定义武器 VM0 被隐藏, 复活发默认武器后可避免透明枪)
    if (IsValidEntity(ClientVM[client][0]))
    {
        EntEffects = GetEntProp(ClientVM[client][0], Prop_Send, "m_fEffects");
        EntEffects &= ~EF_NODRAW;
        SetEntProp(ClientVM[client][0], Prop_Send, "m_fEffects", EntEffects);
    }
}

// 复活显示v模型
public void Event_PlayerSpawn(Handle event, const char[] name, bool dontBroadcast)
{
    int UserId = GetEventInt(event, "userid");
    int client = GetClientOfUserId(UserId);
    Knife_Reset(client);
    Han_ModelReset(client);
    SpawnCheck[client] = true;
}

// ============================================================================
// >> 序列同步方式
// ============================================================================
void SetModelSequenceFlags(int client, int weapon)
{
    IsCustom[client] = true;
    IsCustom2[client] = false;

    if (weapon <= 0)
        return;

    char value[8];
    if (GetWeaponConfigValue(weapon, "Sqtype", value, sizeof(value)))
    {
        int val = StringToInt(value);
        if (val >= 1)
        {
            IsCustom[client] = false;
            IsCustom2[client] = true;
        }
    }

    // 机瞄/侧瞄中强制自动同步
    if ((g_bZooming[client] || g_iZoomInSwitch[client] != 0) && Zoom_IsWeaponZoomable(weapon))
    {
        IsCustom[client] = true;
        IsCustom2[client] = false;
    }
    else if ((g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0) && Zoom_IsWeaponSideAimable(weapon))
    {
        IsCustom[client] = true;
        IsCustom2[client] = false;
    }

    if (IsCustom[client])
    {
        int seq = GetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence");
        SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", seq);
    }
}

// ============================================================================
// >> 禁止枪口火光(0号VM 不再传输)
// ============================================================================
public Action OnTransmit(int entity, int viewer)
{
    if (viewer > 0 && viewer <= MaxClients)
    {
        return Plugin_Handled;
    }
    return Plugin_Continue;
}

// ============================================================================
// >> 开火动画修复(shootqcfixes 配置)
//    根因: 引擎 SendWeaponAnim 对相同 seq 不重置重播 + usp 类 VM 不自动归 idle
//    → 开火随机 seq 命中上次残留 seq 时该次开火动画丢失
//    两条修复路径:
//    A. 已跟踪的开火动画播完(非开火帧): 回正 idle，避免影响拔枪等其他动画
//       播完判定: cycle>=1.0(非循环钳在1.0, v7.3实测) 或 同seq下cycle回绕(循环动画)
//    B. 开火瞬间(TE钩子信号)引擎同seq跳过重设且动画已冻结: 两步强制重播 —
//       本tick写idle、下tick写回开火seq(cycle=0), 跨tick两次更新客户端必看到seq变化
//    只跟踪真实开火信号对应的动画；切枪、换弹或其他动画接管时取消跟踪。
// ============================================================================
void ViewModel_ResetShootQcFix(int client)
{
    g_bQcFixShot[client] = false;
    g_iQcFixRestore[client] = -1;
    g_iQcFixWeapon[client] = INVALID_ENT_REFERENCE;
    g_bQcFixTracking[client] = false;
    g_iQcFixSequence[client] = -1;
    g_iQcFixParity[client] = -1;
}

void ViewModel_ShootQcFix(int client, int weapon, int sequence, float cycle, int prevSequence, float prevCycle)
{
    if (!g_bShootQcFix[client] || EntRefToEntIndex(g_iQcFixWeapon[client]) != weapon)
    {
        ViewModel_ResetShootQcFix(client);
        return;
    }

    if (g_bZooming[client] || g_iZoomInSwitch[client] != 0 ||
        g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0 ||
        g_bInspecting[client] || g_iRunStage[client] != RUN_STAGE_NONE ||
        g_bEmptyReloading[client] || g_bCustomAnim[client] || Weapon_IsReloading(weapon) ||
        (HasEntProp(weapon, Prop_Send, "m_reloadState") && GetEntProp(weapon, Prop_Send, "m_reloadState") > 0))
    {
        ViewModel_ResetShootQcFix(client);
        return;
    }

    if (ClientVM[client][0] <= 0 || !IsValidEntity(ClientVM[client][0]) ||
        ClientVM[client][1] <= 0 || !IsValidEntity(ClientVM[client][1]))
    {
        ViewModel_ResetShootQcFix(client);
        return;
    }

    int idle = ViewModel_GetIdleSeq(weapon);
    int parity = GetEntProp(ClientVM[client][0], Prop_Send, "m_nAnimationParity");
    bool shot = g_bQcFixShot[client];
    g_bQcFixShot[client] = false;

    // 两步强制重播第二步: 写回开火seq(上一步已写idle, 客户端已看到 seq 变化)
    if (g_iQcFixRestore[client] != -1)
    {
        // 新动作或新一枪优先，不能用旧的延迟恢复覆盖它。
        if (shot || sequence != idle || parity != g_iQcFixParity[client])
        {
            g_iQcFixRestore[client] = -1;
            g_bQcFixTracking[client] = false;
            if (!shot)
            {
                ViewModel_ResetShootQcFix(client);
                return;
            }
        }
        else
        {
            SetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence", g_iQcFixRestore[client]);
            SetEntPropFloat(ClientVM[client][0], Prop_Data, "m_flCycle", 0.0);
            SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", g_iQcFixRestore[client]);
            SetEntPropFloat(ClientVM[client][1], Prop_Data, "m_flCycle", 0.0);
            g_iQcFixRestore[client] = -1;
            return;
        }
    }

    // 已是 idle 无事可做(顺手清掉过期开火信号)
    if (sequence == idle)
    {
        ViewModel_ResetShootQcFix(client);
        return;
    }

    bool sameShotAnim = g_bQcFixTracking[client] && sequence == g_iQcFixSequence[client]
        && parity == g_iQcFixParity[client];
    if (shot)
    {
        g_bQcFixTracking[client] = true;
        g_iQcFixSequence[client] = sequence;
        g_iQcFixParity[client] = parity;
    }
    else if (!sameShotAnim)
    {
        ViewModel_ResetShootQcFix(client);
        return;
    }

    // 动画是否已播完: 非循环钳在 1.0 / 循环同seq下cycle回绕
    // 开火当帧 cycle 回落表示正常重播，不应再强制重播；跨动画的旧 cycle 不参与判定。
    bool bFinished = (cycle >= 1.0) || (!shot && sameShotAnim && sequence == prevSequence && cycle < prevCycle);

    if (!bFinished)
    {
        // 动画播放中: 引擎正常重设了新seq或动画未结束, 无需干预
        return;
    }

    if (shot)
    {
        if (!sameShotAnim)
            return;
        // 路径B: 开火瞬间但动画冻结在尾帧 → 引擎同seq跳过了重设, 两步强制重播
        SetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence", idle);
        SetEntPropFloat(ClientVM[client][0], Prop_Data, "m_flCycle", 0.0);
        SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", idle);
        SetEntPropFloat(ClientVM[client][1], Prop_Data, "m_flCycle", 0.0);
        g_iQcFixRestore[client] = sequence;
        return;
    }

    // 路径A: 非开火帧且动画已播完 → 回正 idle
    SetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence", idle);
    SetEntPropFloat(ClientVM[client][0], Prop_Data, "m_flCycle", 0.0);
    SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", idle);
    SetEntPropFloat(ClientVM[client][1], Prop_Data, "m_flCycle", 0.0);
    ViewModel_ResetShootQcFix(client);
}

// ============================================================================
// >> 计算武器正确 idle 序列(v7.3 消音器序列约定)
//    变体武器(m4a1/usp模板): 有消音=0 / 无消音= usp 8、m4 7
//    其他武器: idle = 0
// ============================================================================
int ViewModel_GetIdleSeq(int weapon)
{
    if (IsVariantWeapon(weapon))
    {
        if (IsWeaponSilencerOn(weapon))
            return 0;

        return Weapon_IsUspTemplate(weapon) ? 8 : 7;
    }

    return 0;
}

// ============================================================================
// >> 接管结束恢复 idle(同步 1号VM)
//    usp 模板武器: 引擎不会自动把 0号VM 回正 idle, 主动写一次正确 idle 再同步 1号VM
//    (usp 模型序列布局 0~7=有消音器块 / 8~15=无消音器块, idle 有=0 无=8)
//    其他武器: 镜像 0号VM 引擎序列(引擎会自动回正 idle)
// ============================================================================
void ViewModel_RestoreIdle(int client, int weapon)
{
    if (ClientVM[client][0] <= 0 || !IsValidEntity(ClientVM[client][0]) ||
        ClientVM[client][1] <= 0 || !IsValidEntity(ClientVM[client][1]))
        return;

    if (weapon > 0 && IsValidEntity(weapon) && Weapon_IsUspTemplate(weapon))
    {
        int idle = IsWeaponSilencerOn(weapon) ? 0 : 8;

        SetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence", idle);
        SetEntPropFloat(ClientVM[client][0], Prop_Data, "m_flCycle", 0.0);
        SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", idle);
        SetEntPropFloat(ClientVM[client][1], Prop_Data, "m_flCycle", 0.0);
        SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", 1.0);
        return;
    }

    // 普通武器: 镜像 0号VM 引擎序列(引擎会自动回正 idle)
    SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", GetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence"));
    SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", 1.0);
}
