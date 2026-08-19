// ============================================================================
// >> 武器买卖系统(购买/卖出, 使用游戏金钱 m_iAccount)
// ============================================================================
// 购买: 注册表自定义武器 + 团队匹配 + 非已持有 + 金钱充足 + 时间窗内
//       成功 → GiveWeaponByIndex 发枪 + 扣除购买价
// 卖出: 身上持有 + 配置 sellable=1 → 移除武器 + 返还 价格×比例
//       比例按团队分析: 本队武器→sellratio, 敌方/缴获武器→sellratio_other
// 时间窗: buymode=buytime 时仅 round_start 后 mp_buytime 秒内可买可卖
// 命令: sm_buy / sm_sell(无参数=菜单, 带参数=直接买/卖, 兼容单机HUD点按钮发命令)
// ============================================================================

void Buy_OnPluginStart()
{
    // 读取根字段注册命令(配置在 BuyConfig_OnPluginStart 已加载)
    char buyCmd[64];
    if (!GetBuyConfigRootValue("buycommand", buyCmd, sizeof(buyCmd)) || strlen(buyCmd) <= 0)
        strcopy(buyCmd, sizeof(buyCmd), "sm_buy");
    RegConsoleCmd(buyCmd, Cmd_Buy, "打开购买菜单/直接购买武器");

    char sellCmd[64];
    if (!GetBuyConfigRootValue("sellcommand", sellCmd, sizeof(sellCmd)) || strlen(sellCmd) <= 0)
        strcopy(sellCmd, sizeof(sellCmd), "sm_sell");
    RegConsoleCmd(sellCmd, Cmd_Sell, "打开卖出菜单/直接卖出武器");

    g_hBuyTime = FindConVar("mp_buytime");

    HookEvent("round_start", Buy_EventRoundStart, EventHookMode_Post);
}

void Buy_OnMapStart()
{
    g_fRoundStartTime = GetGameTime();
}

public void Buy_EventRoundStart(Event event, const char[] name, bool dontBroadcast)
{
    g_fRoundStartTime = GetGameTime();
}

// ============================================================================
// >> 总开关判断
// ============================================================================
bool Buy_IsSystemEnabled()
{
    char buf[8];
    if (GetBuyConfigRootValue("buysystem", buf, sizeof(buf)))
        return StringToInt(buf) > 0;
    return true;
}

// ============================================================================
// >> 购买时间窗判断(仅 buymode=buytime 时限制)
// ============================================================================
bool Buy_IsBuyTime()
{
    char mode[16];
    if (!GetBuyConfigRootValue("buymode", mode, sizeof(mode)))
        return true;

    if (!StrEqual(mode, "buytime", false))
        return true;

    float buytime = 30.0;
    if (g_hBuyTime != null)
    {
        float cv = GetConVarFloat(g_hBuyTime);
        if (cv > 0.0)
            buytime = cv;
    }

    return (GetGameTime() - g_fRoundStartTime) <= buytime;
}

// ============================================================================
// >> 通过 useclassname 查注册表索引(-1=非注册表/原生武器)
// ============================================================================
int Buy_FindWeaponIndex(const char[] useclassname)
{
    char target[64];
    strcopy(target, sizeof(target), useclassname);
    if (StrContains(target, "weapon_") == 0)
        strcopy(target, sizeof(target), target[7]);

    for (int i = 0; i < g_WeaponCount; i++)
    {
        char use[64];
        strcopy(use, sizeof(use), g_WeaponUseClassname[i]);
        if (StrContains(use, "weapon_") == 0)
            strcopy(use, sizeof(use), use[7]);

        if (StrEqual(target, use, false))
            return i;
    }

    return -1;
}

// ============================================================================
// >> 团队匹配(注册表武器按 team 过滤; 原生武器双方通用)
// ============================================================================
bool Buy_WeaponTeamMatches(int client, int idx)
{
    int team = GetClientTeam(client);

    if (StrEqual(g_WeaponTeam[idx], "t", false) && team != CS_TEAM_T)
        return false;
    if (StrEqual(g_WeaponTeam[idx], "ct", false) && team != CS_TEAM_CT)
        return false;

    return true;
}

// ============================================================================
// >> 在玩家武器列表里按类名找武器实体(-1=没有)
// ============================================================================
int Buy_FindPlayerWeapon(int client, const char[] useclassname)
{
    char target[64];
    strcopy(target, sizeof(target), useclassname);
    if (StrContains(target, "weapon_") == 0)
        strcopy(target, sizeof(target), target[7]);

    for (int i = 0; i < 48; i++)
    {
        int weapon = GetEntPropEnt(client, Prop_Send, "m_hMyWeapons", i);
        if (weapon <= MaxClients || !IsValidEntity(weapon))
            continue;

        char cls[64];
        GetEntityClassname(weapon, cls, sizeof(cls));
        if (StrContains(cls, "weapon_") == 0)
            strcopy(cls, sizeof(cls), cls[7]);

        if (StrEqual(target, cls, false))
            return weapon;
    }

    return -1;
}

// ============================================================================
// >> 菜单显示名(配置 name, 空=useclassname 去 weapon_ 前缀)
// ============================================================================
void Buy_GetDisplayName(const char[] useclassname, char[] out, int maxlen)
{
    char name[64];
    if (GetBuyConfigValue(useclassname, "name", name, sizeof(name)) && strlen(name) > 0)
    {
        strcopy(out, maxlen, name);
        return;
    }

    strcopy(out, maxlen, useclassname);
    if (StrContains(out, "weapon_") == 0)
        strcopy(out, maxlen, out[7]);
}

// ============================================================================
// >> 购买价格(配置 price → 注册表武器回落脚本 WeaponPrice → 0)
// ============================================================================
int Buy_GetPrice(const char[] useclassname)
{
    char buf[16];
    if (GetBuyConfigValue(useclassname, "price", buf, sizeof(buf)))
        return StringToInt(buf);

    if (Buy_FindWeaponIndex(useclassname) != -1)
    {
        if (GetWeaponScriptValue(useclassname, "WeaponPrice", buf, sizeof(buf)))
            return StringToInt(buf);
    }

    return 0;
}

// ============================================================================
// >> 卖出比例(本队→sellratio; 敌方/缴获→sellratio_other, 空回落 sellratio)
//    原生武器按双方通用(推论 C)
// ============================================================================
float Buy_GetSellRatio(const char[] useclassname, int client)
{
    float ownRatio = Buy_GetFloat(useclassname, "sellratio", 0.5);

    int idx = Buy_FindWeaponIndex(useclassname);
    if (idx == -1)
        return ownRatio;

    int team = GetClientTeam(client);
    bool ownTeam = true;

    if (StrEqual(g_WeaponTeam[idx], "t", false))
        ownTeam = (team == CS_TEAM_T);
    else if (StrEqual(g_WeaponTeam[idx], "ct", false))
        ownTeam = (team == CS_TEAM_CT);

    if (ownTeam)
        return ownRatio;

    float otherRatio = Buy_GetFloat(useclassname, "sellratio_other", 0.0);
    if (otherRatio > 0.0)
        return otherRatio;

    return ownRatio;
}

// ============================================================================
// >> 卖出返还金额(价格×比例, 四舍五入)
// ============================================================================
int Buy_GetSellRefund(const char[] useclassname, int client)
{
    int price = Buy_GetPrice(useclassname);
    float ratio = Buy_GetSellRatio(useclassname, client);
    return RoundToNearest(price * ratio);
}

// ============================================================================
// >> 购买命令
// ============================================================================
public Action Cmd_Buy(int client, int args)
{
    if (!IsClientInGame(client))
        return Plugin_Handled;

    if (!Buy_IsSystemEnabled())
    {
        PrintToChat(client, "[HWS] 买卖系统未开启");
        return Plugin_Handled;
    }

    if (!IsPlayerAlive(client))
    {
        PrintToChat(client, "[HWS] 死亡状态下无法购买武器");
        return Plugin_Handled;
    }

    if (args > 0)
    {
        char useclass[64];
        GetCmdArg(1, useclass, sizeof(useclass));
        Buy_TryBuy(client, useclass);
        return Plugin_Handled;
    }

    Buy_OpenBuyMenu(client);
    return Plugin_Handled;
}

// ============================================================================
// >> 卖出命令
// ============================================================================
public Action Cmd_Sell(int client, int args)
{
    if (!IsClientInGame(client))
        return Plugin_Handled;

    if (!Buy_IsSystemEnabled())
    {
        PrintToChat(client, "[HWS] 买卖系统未开启");
        return Plugin_Handled;
    }

    if (!IsPlayerAlive(client))
    {
        PrintToChat(client, "[HWS] 死亡状态下无法卖出武器");
        return Plugin_Handled;
    }

    if (args > 0)
    {
        char useclass[64];
        GetCmdArg(1, useclass, sizeof(useclass));
        Buy_TrySell(client, useclass);
        return Plugin_Handled;
    }

    Buy_OpenSellMenu(client);
    return Plugin_Handled;
}

// ============================================================================
// >> 购买菜单(只列注册表自定义武器, 推论 B)
// ============================================================================
void Buy_OpenBuyMenu(int client)
{
    if (!Buy_IsBuyTime())
    {
        PrintToChat(client, "[HWS] 当前不在购买时间内");
        return;
    }

    int money = Client_GetMoney(client);

    Menu menu = new Menu(Buy_MenuHandler);
    char title[96];
    Format(title, sizeof(title), "购买武器  余额: $%d\n ", money);
    menu.SetTitle(title);

    for (int i = 0; i < g_WeaponCount; i++)
    {
        int idx = Buy_FindWeaponIndex(g_WeaponUseClassname[i]);
        if (idx == -1)
            continue;

        if (Buy_GetInt(g_WeaponUseClassname[i], "buyable", 1) <= 0)
            continue;

        if (!Buy_WeaponTeamMatches(client, idx))
            continue;

        int price = Buy_GetPrice(g_WeaponUseClassname[i]);
        char name[64];
        Buy_GetDisplayName(g_WeaponUseClassname[i], name, sizeof(name));

        char display[96];
        Format(display, sizeof(display), "%s - $%d", name, price);
        menu.AddItem(g_WeaponUseClassname[i], display);
    }

    if (menu.ItemCount == 0)
    {
        delete menu;
        PrintToChat(client, "[HWS] 没有可购买的武器");
        return;
    }

    menu.Display(client, MENU_TIME_FOREVER);
}

public int Buy_MenuHandler(Menu menu, MenuAction action, int client, int item)
{
    if (action == MenuAction_Select)
    {
        char useclass[64];
        GetMenuItem(menu, item, useclass, sizeof(useclass));
        Buy_TryBuy(client, useclass);
    }
    else if (action == MenuAction_End)
    {
        delete menu;
    }
    return 0;
}

// ============================================================================
// >> 卖出菜单(只列身上持有且 sellable=1 的武器)
// ============================================================================
void Buy_OpenSellMenu(int client)
{
    if (!Buy_IsBuyTime())
    {
        PrintToChat(client, "[HWS] 当前不在购买时间内");
        return;
    }

    Menu menu = new Menu(Sell_MenuHandler);
    char title[96];
    Format(title, sizeof(title), "卖出武器  余额: $%d\n ", Client_GetMoney(client));
    menu.SetTitle(title);

    for (int i = 0; i < 48; i++)
    {
        int weapon = GetEntPropEnt(client, Prop_Send, "m_hMyWeapons", i);
        if (weapon <= MaxClients || !IsValidEntity(weapon))
            continue;

        char cls[64];
        GetEntityClassname(weapon, cls, sizeof(cls));

        if (Buy_GetInt(cls, "sellable", 0) <= 0)
            continue;

        int refund = Buy_GetSellRefund(cls, client);
        char name[64];
        Buy_GetDisplayName(cls, name, sizeof(name));

        char display[96];
        Format(display, sizeof(display), "%s - 卖出 $%d", name, refund);
        menu.AddItem(cls, display);
    }

    if (menu.ItemCount == 0)
    {
        delete menu;
        PrintToChat(client, "[HWS] 没有可出售的武器");
        return;
    }

    menu.Display(client, MENU_TIME_FOREVER);
}

public int Sell_MenuHandler(Menu menu, MenuAction action, int client, int item)
{
    if (action == MenuAction_Select)
    {
        char useclass[64];
        GetMenuItem(menu, item, useclass, sizeof(useclass));
        Buy_TrySell(client, useclass);
    }
    else if (action == MenuAction_End)
    {
        delete menu;
    }
    return 0;
}

// ============================================================================
// >> 执行购买
// ============================================================================
void Buy_TryBuy(int client, const char[] useclass)
{
    if (!IsPlayerAlive(client))
    {
        PrintToChat(client, "[HWS] 死亡状态下无法购买武器");
        return;
    }

    if (!Buy_IsBuyTime())
    {
        PrintToChat(client, "[HWS] 当前不在购买时间内");
        return;
    }

    int idx = Buy_FindWeaponIndex(useclass);
    if (idx == -1)
    {
        PrintToChat(client, "[HWS] 该武器不在购买系统中");
        return;
    }

    if (!Buy_WeaponTeamMatches(client, idx))
    {
        PrintToChat(client, "[HWS] 你的队伍无法购买该武器");
        return;
    }

    if (Buy_FindPlayerWeapon(client, g_WeaponUseClassname[idx]) != -1)
    {
        PrintToChat(client, "[HWS] 你已经拥有该武器, 想再买请先丢出去");
        return;
    }

    int price = Buy_GetPrice(g_WeaponUseClassname[idx]);
    int money = Client_GetMoney(client);
    if (money < price)
    {
        PrintToChat(client, "[HWS] 金钱不足: 需要 $%d, 当前 $%d", price, money);
        return;
    }

    char name[64];
    Buy_GetDisplayName(g_WeaponUseClassname[idx], name, sizeof(name));

    GiveWeaponByIndex(client, idx);
    Client_SetMoney(client, money - price);

    PrintToChat(client, "[HWS] 购买成功: %s 花费 $%d, 余额 $%d", name, price, money - price);
}

// ============================================================================
// >> 执行卖出
// ============================================================================
void Buy_TrySell(int client, const char[] useclass)
{
    if (!IsPlayerAlive(client))
    {
        PrintToChat(client, "[HWS] 死亡状态下无法卖出武器");
        return;
    }

    if (!Buy_IsBuyTime())
    {
        PrintToChat(client, "[HWS] 当前不在购买时间内");
        return;
    }

    int weapon = Buy_FindPlayerWeapon(client, useclass);
    if (weapon == -1)
    {
        PrintToChat(client, "[HWS] 你身上没有该武器");
        return;
    }

    if (Buy_GetInt(useclass, "sellable", 0) <= 0)
    {
        PrintToChat(client, "[HWS] 该武器无法出售");
        return;
    }

    int refund = Buy_GetSellRefund(useclass, client);
    int money = Client_GetMoney(client);

    char name[64];
    Buy_GetDisplayName(useclass, name, sizeof(name));

    RemovePlayerItem(client, weapon);
    AcceptEntityInput(weapon, "Kill");

    Client_SetMoney(client, money + refund);

    PrintToChat(client, "[HWS] 卖出成功: %s 获得 $%d, 余额 $%d", name, refund, money + refund);
}