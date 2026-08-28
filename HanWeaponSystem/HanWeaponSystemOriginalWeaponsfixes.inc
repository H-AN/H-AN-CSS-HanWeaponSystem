// ============================================================================
// >> 原生武器走1号模型修复 (han_oldweaponfix)
//    开启后所有原生武器(未在配置中注册useclassname的weapon_*)也走VM1显示,
//    避免0号/1号VM切换导致的动画丢失
//    同时支持快速近战等快速切换类武器的动画
// ============================================================================

void OldWeapon_OnPluginStart()
{
    g_hOldWeaponCvar = CreateConVar("han_oldweaponfix", "0", "开启后所有原始武器都走1号模型", FCVAR_NOTIFY, true, 0.0, true, 1.0);
    g_hOldWeaponCvar.AddChangeHook(OldWeapon_CvarChange);
}

// 判断是否为原版武器: 遍历配置useclassname列表, 未匹配到的即为原版武器
bool IsOriginalWeapon(int weapon)
{
    if (weapon <= 0 || !IsValidEntity(weapon))
        return false;

    char cls[64];
    GetEdictClassname(weapon, cls, sizeof(cls));

    for (int i = 0; i < g_WeaponCount; i++)
    {
        if (StrEqual(cls, g_WeaponUseClassname[i], false))
            return false;
    }

    return true;
}

public void OldWeapon_CvarChange(ConVar convar, const char[] oldValue, const char[] newValue)
{
    OldWeaponFixes = view_as<bool>(convar.IntValue);

    if (!OldWeaponFixes)
    {
        // 关闭: 恢复引擎原生VM0显示, 隐藏VM1
        for (int client = 1; client <= MaxClients; client++)
        {
            if (!IsClientInGame(client) || !IsPlayerAlive(client))
                continue;

            int EntEffects = GetEntProp(ClientVM[client][0], Prop_Send, "m_fEffects");
            EntEffects &= ~EF_NODRAW;
            SetEntProp(ClientVM[client][0], Prop_Send, "m_fEffects", EntEffects);

            EntEffects = GetEntProp(ClientVM[client][1], Prop_Send, "m_fEffects");
            EntEffects |= EF_NODRAW;
            SetEntProp(ClientVM[client][1], Prop_Send, "m_fEffects", EntEffects);

            SDKUnhook(ClientVM[client][0], SDKHook_SetTransmit, OnTransmit);
            g_bVM0BlockHooked[client] = false;

            IsCustom[client] = false;
            IsCustom2[client] = false;
            g_bShootQcFix[client] = false;
        }
    }
    else
    {
        // 开启: 强制所有存活玩家立即刷新当前武器的VM状态
        for (int client = 1; client <= MaxClients; client++)
        {
            if (!IsClientInGame(client) || !IsPlayerAlive(client))
                continue;

            int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
            if (weapon <= 0)
                continue;

            if (IsOriginalWeapon(weapon))
            {
                OldWeapon_HandleSwitch(client, weapon);
            }
        }
    }
}

// 原版武器走1号模型: 隐藏VM0, 显示VM1并设置模型路径, 同时调用左手模型修复
public void OldWeapon_HandleSwitch(int client, int weapon)
{
    // 隐藏VM0
    int EntEffects = GetEntProp(ClientVM[client][0], Prop_Send, "m_fEffects");
    EntEffects |= EF_NODRAW;
    SetEntProp(ClientVM[client][0], Prop_Send, "m_fEffects", EntEffects);

    // 显示VM1并设置模型
    EntEffects = GetEntProp(ClientVM[client][1], Prop_Send, "m_fEffects");
    EntEffects &= ~EF_NODRAW;
    SetEntProp(ClientVM[client][1], Prop_Send, "m_fEffects", EntEffects);

    SetEntProp(ClientVM[client][1], Prop_Send, "m_nModelIndex", GetEntProp(ClientVM[client][0], Prop_Send, "m_nModelIndex"));

    SetEntPropEnt(ClientVM[client][1], Prop_Send, "m_hWeapon", GetEntPropEnt(ClientVM[client][0], Prop_Send, "m_hWeapon"));

    // 保持VM0传输屏蔽挂钩
    SDKHook(ClientVM[client][0], SDKHook_SetTransmit, OnTransmit);
    g_bVM0BlockHooked[client] = true;

    IsCustom[client] = true;
    IsCustom2[client] = false;
    g_bShootQcFix[client] = false;
}

