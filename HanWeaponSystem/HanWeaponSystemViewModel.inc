// ============================================================================
// >> ViewModel 系统(双VM显示 + 动画同步)
// ============================================================================

void ViewModel_OnPluginStart()
{
    for (int client = 1; client <= MaxClients; client++)
    {
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
// >> 实体创建(追踪 predicted_viewmodel / 霰弹枪换弹钩子)
// ============================================================================
public void OnEntityCreated(int entity, const char[] classname)
{
    if (StrEqual(classname, "predicted_viewmodel", false))
    {
        SDKHook(entity, SDKHook_Spawn, OnEntitySpawned);
    }

    if (HasEntProp(entity, Prop_Send, "m_reloadState"))
    {
        SDKHook(entity, SDKHook_ReloadPost, Hook_OnReloadPost);
    }
}

public void OnEntitySpawned(int entity)
{
    int Owner = GetEntPropEnt(entity, Prop_Send, "m_hOwner");
    if ((Owner > 0) && (Owner <= MaxClients))
    {
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

    int Sequence = GetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence");
    float Cycle = GetEntPropFloat(ClientVM[client][0], Prop_Data, "m_flCycle");
    float PlaybackRate = GetEntPropFloat(ClientVM[client][0], Prop_Send, "m_flPlaybackRate");

    if (WeaponIndex <= 0)
    {
        int EntEffects = GetEntProp(ClientVM[client][1], Prop_Send, "m_fEffects");
        EntEffects |= EF_NODRAW;
        SetEntProp(ClientVM[client][1], Prop_Send, "m_fEffects", EntEffects);

        IsCustom[client] = false;
        IsCustom2[client] = false;
        g_bVM0BlockHooked[client] = false;
        OldWeapon[client] = WeaponIndex;
        OldSequence[client] = Sequence;
        OldCycle[client] = Cycle;

        return;
    }

    // 换武器
    if (WeaponIndex != OldWeapon[client])
    {
        // 换武器强制退出机瞄/侧瞄
        if (g_bZooming[client] || g_iZoomInSwitch[client] != 0 || g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0)
            Aim_ForceExit(client);

        GetEdictClassname(WeaponIndex, ClassName, sizeof(ClassName));

        // 自动连发标记
        char AutoFire[32];
        if (GetWeaponConfigValue(WeaponIndex, "autofire", AutoFire, sizeof(AutoFire)))
        {
            g_bWeaponAutoFire[client] = (StringToInt(AutoFire) > 0);
        }
        else
        {
            g_bWeaponAutoFire[client] = false;
        }

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

            // armfix: 需要修复的武器, 遍历地图找现成 prop_physics_multiplayer 指向它
            char armfixStr[8];
            if (GetWeaponConfigValue(WeaponIndex, "armfix", armfixStr, sizeof(armfixStr)) && StringToInt(armfixStr) > 0)
            {
                int fixEnt = -1;
                int ent = -1;
                while ((ent = FindEntityByClassname(ent, "prop_physics_multiplayer")) != -1)
                {
                    if (IsValidEdict(ent) && IsValidEntity(ent))
                    {
                        fixEnt = ent;
                        break;
                    }
                }
                if (fixEnt > 0)
                    SetEntPropEnt(ClientVM[client][1], Prop_Send, "m_hWeapon", fixEnt);
                else
                    SetEntPropEnt(ClientVM[client][1], Prop_Send, "m_hWeapon", GetEntPropEnt(ClientVM[client][0], Prop_Send, "m_hWeapon"));
            }
            else
            {
                SetEntPropEnt(ClientVM[client][1], Prop_Send, "m_hWeapon", GetEntPropEnt(ClientVM[client][0], Prop_Send, "m_hWeapon"));
            }

            SetModelSequenceFlags(client, WeaponIndex);

            SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", GetEntPropFloat(ClientVM[client][0], Prop_Send, "m_flPlaybackRate"));

            if (GetWeaponConfigValue(WeaponIndex, "skincode", SkinCode, sizeof(SkinCode)))
            {
                int skin = StringToInt(SkinCode);
                SetEntProp(WeaponIndex, Prop_Send, "m_nSkin", skin);
                SetEntProp(ClientVM[client][1], Prop_Send, "m_nSkin", skin);
            }

            SDKHook(ClientVM[client][0], SDKHook_SetTransmit, OnTransmit);
            g_bVM0BlockHooked[client] = true;
        }
        else
        {
            int EntEffects = GetEntProp(ClientVM[client][1], Prop_Send, "m_fEffects");
            EntEffects |= EF_NODRAW;
            SetEntProp(ClientVM[client][1], Prop_Send, "m_fEffects", EntEffects);

            SDKUnhook(ClientVM[client][0], SDKHook_SetTransmit, OnTransmit);
            g_bVM0BlockHooked[client] = false;

            IsCustom[client] = false;
            IsCustom2[client] = false;
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
            SetEntProp(ClientVM[client][1], Prop_Send, "m_nSequence", GetEntProp(ClientVM[client][0], Prop_Send, "m_nSequence"));

            SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", GetEntPropFloat(ClientVM[client][0], Prop_Send, "m_flPlaybackRate"));

            SetEntPropFloat(ClientVM[client][1], Prop_Send, "m_flPlaybackRate", PlaybackRate);

            // 刀类动画修复: cycle 回绕时强制显示 0 号序列数帧
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

    OldWeapon[client] = WeaponIndex;
    OldSequence[client] = Sequence;
    OldCycle[client] = Cycle;

    // 机瞄后处理(屏震降低/衔接提前结束)
    Zoom_OnPostThinkPost(client);

    // 检视后处理(动画强制/结束条件/手电屏蔽)
    Inspect_OnPostThinkPost(client);

    // 奔跑后处理(状态推进/动画强制/抖动/结束条件)
    Run_OnPostThinkPost(client);

    // 空仓/战术换弹后处理(强制换弹序列/结束条件)
    EmptyReload_OnPostThinkPost(client);
}

// 隐藏死亡v模型
public void Event_PlayerDeath(Handle event, const char[] name, bool dontBroadcast)
{
    int UserId = GetEventInt(event, "userid");
    int client = GetClientOfUserId(UserId);

    // 强制退出机瞄/侧瞄
    if (g_bZooming[client] || g_iZoomInSwitch[client] != 0 || g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0)
        Aim_ForceExit(client);

    // 强制退出检视
    Inspect_End(client);

    // 强制退出奔跑(恢复速度与动画)
    Run_End(client);

    // 强制结束空仓/战术换弹接管(恢复VM[1])
    EmptyReload_End(client);

    IsCustom[client] = false;
    IsCustom2[client] = false;
    g_bVM0BlockHooked[client] = false;

    int EntEffects = GetEntProp(ClientVM[client][1], Prop_Send, "m_fEffects");
    EntEffects |= EF_NODRAW;
    SetEntProp(ClientVM[client][1], Prop_Send, "m_fEffects", EntEffects);
}

// 复活显示v模型
public void Event_PlayerSpawn(Handle event, const char[] name, bool dontBroadcast)
{
    int UserId = GetEventInt(event, "userid");
    int client = GetClientOfUserId(UserId);
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