// ============================================================================
// >> Bot 武器系统(Bot 出生自动发自定义武器 + 挡购)
// ============================================================================
// 触发: player_spawn(仅 IsFakeClient) → 预挡购 → 按槽位分延时给枪(主0.3/副0.4/刀0.5/雷0.6/C4 0.7秒,
//       每槽位独立定时器 +0.1 秒顺延, 刀更晚发避开创建实体瞬间的卡顿)
// 槽位: 0主 1副 2刀 3雷 4C4, 候选按注册表武器脚本 bucket 归类
// 筛选: team 匹配 + botchance>0 + (金钱模式) 价格<=bot当前金钱
// 加权随机: 概率 = 该botchance / 候选总和(多个100=均分)
// 替换: 槽位现有原生武器(0/1/3/4槽) → 金钱模式先返旧枪价(引擎原生价格, 刀=0) 再扣新枪价;
//       免费模式只删默认武器, 不返不扣
//       刀槽(2)特殊: 直接发自定义刀, 不删默认刀不清理(避免引擎补刀/双刀竞态)
// 槽位现有注册表自定义武器 → 跳过(不删不给)
// 挡购: 每 bot 独立(g_bBotBlockBuy), 持有一把以上新武器才挡 buy/autobuy/rebuy;
//       出生瞬间先预挡, 给枪循环算完后再按实际结果定(没分到新武器的 bot 正常买枪)
// Bot 行为(机瞄/侧瞄/奔跑/检视): 机瞄/侧瞄由 weapon_fire 事件触发(bot 开枪不走 runcmd 按键检测);
//       奔跑/检视由 Combat 的共享 OnPlayerRunCmd 末尾调用 Bot_OnPlayerRunCmd 触发。
//       仅 IsFakeClient 进入, 纯函数调用不修改按键/返回值, 不影响玩家主逻辑。
//       判定链: 武器本身有该功能(ZoomData.zoom/sideaim, RunData.run, InspectData.inspectseq)
//       → 本配置 botaim/botrun/botinspect 允许且几率>0 → 掷骰 → 独立 CD(botaimcd/botruncd/botinspectcd, 互不饿死)。
//       开火事件掷 botaim(机瞄/侧瞄合并一个几率, 两者都有随机选一) → StartZoom/StartSideAim,
//       开镜维持 botaimtime 秒后自动 StopZoom/StopSideAim; 前进掷 botrun → Run_Start,
//       触发后保持 botruncd 秒(bot 松开前进也持续冲刺), 开火/开镜等自然停止;
//       空闲(未开火/未前进)掷 botinspect → Inspect_Start。互斥由现有模块状态保证。
// ============================================================================

void Bot_OnPluginStart()
{
    AddCommandListener(Bot_OnBuyCommand, "buy");
    AddCommandListener(Bot_OnBuyCommand, "autobuy");
    AddCommandListener(Bot_OnBuyCommand, "rebuy");

    HookEvent("player_spawn", Bot_EventPlayerSpawn, EventHookMode_Post);
    HookEvent("weapon_fire", Bot_EventWeaponFire, EventHookMode_Post);
}

// ============================================================================
// >> 总开关判断(默认关)
// ============================================================================
bool Bot_IsSystemEnabled()
{
    char buf[8];
    if (GetBotConfigRootValue("botsystem", buf, sizeof(buf)))
        return StringToInt(buf) > 0;
    return false;
}

// ============================================================================
// >> 金钱模式判断(1=金钱模式 0=免费模式)
// ============================================================================
bool Bot_IsMoneyMode()
{
    char buf[8];
    if (GetBotConfigRootValue("botmoney", buf, sizeof(buf)))
        return StringToInt(buf) > 0;
    return false;
}

// ============================================================================
// >> 读注册表武器脚本 bucket(0主/1副/2刀/3雷/4C4; 脚本缺失/无 bucket → -1 不参与)
// ============================================================================
int Bot_GetWeaponBucket(const char[] useclassname)
{
    char bucketStr[16];
    if (GetWeaponScriptValue(useclassname, "bucket", bucketStr, sizeof(bucketStr)))
    {
        int b = StringToInt(bucketStr);
        if (b >= 0 && b <= 4)
            return b;
    }
    return -1;
}

// ============================================================================
// >> 出生事件(仅 bot; 预挡购 + 按槽位分延时给枪)
//    槽位延时: 主0.3 / 副0.4 / 刀0.5 / 雷0.6 / C4 0.7 秒
//    (每槽位独立定时器顺序触发, 刀更晚发, 避开创建实体瞬间的卡顿)
// ============================================================================
public void Bot_EventPlayerSpawn(Event event, const char[] name, bool dontBroadcast)
{
    int client = GetClientOfUserId(event.GetInt("userid"));
    if (client <= 0 || client > MaxClients || !IsClientInGame(client) || !IsFakeClient(client))
        return;

    if (!Bot_IsSystemEnabled())
        return;

    if (!IsPlayerAlive(client))
        return;

    int team = GetClientTeam(client);
    if (team != CS_TEAM_T && team != CS_TEAM_CT)
        return;

    g_bBotBlockBuy[client] = true;
    g_iBotGiveSlot[client] = 0;
    g_iBotGiveGen[client]++;
    g_fBotAimCd[client] = 0.0;
    g_fBotRunCd[client] = 0.0;
    g_fBotInspectCd[client] = 0.0;
    g_fBotAimEnd[client] = 0.0;
    g_bBotRunHold[client] = false;
    g_fBotRunHoldEnd[client] = 0.0;

    DataPack pack = new DataPack();
    pack.WriteCell(GetClientUserId(client));
    pack.WriteCell(g_iBotGiveGen[client]);
    CreateTimer(0.3, Bot_GiveWeaponSlot, pack);
}

// ============================================================================
// >> 槽位顺序给枪(每个槽位一个定时器, 间隔 +0.1 秒)
// ============================================================================
public Action Bot_GiveWeaponSlot(Handle timer, DataPack pack)
{
    pack.Reset();
    int userId = pack.ReadCell();
    int gen = pack.ReadCell();
    delete pack;

    int client = GetClientOfUserId(userId);
    if (client <= 0 || client > MaxClients || !IsClientInGame(client) || !IsFakeClient(client))
        return Plugin_Stop;

    if (gen != g_iBotGiveGen[client])
        return Plugin_Stop;

    if (!IsPlayerAlive(client))
        return Plugin_Stop;

    if (!Bot_IsSystemEnabled())
        return Plugin_Stop;

    int team = GetClientTeam(client);
    if (team != CS_TEAM_T && team != CS_TEAM_CT)
        return Plugin_Stop;

    int slot = g_iBotGiveSlot[client];
    if (slot <= 4)
    {
        Bot_GiveSlotWeapon(client, slot, Bot_IsMoneyMode());
        g_iBotGiveSlot[client] = slot + 1;

        DataPack next = new DataPack();
        next.WriteCell(GetClientUserId(client));
        next.WriteCell(gen);
        CreateTimer(0.1, Bot_GiveWeaponSlot, next);
    }
    else
    {
        // 全部槽位给完 → 结算挡购: 持有一把以上新武器才挡, 否则放行(可正常买枪)
        g_bBotBlockBuy[client] = Bot_HasAnyCustomWeapon(client);

        // 强制切一次主武器, 刷新武器/动画状态, 消除刀 T-pose/卡住
        Bot_SwitchWeapon(client, GetPlayerWeaponSlot(client, 0));
    }

    return Plugin_Stop;
}

// ============================================================================
// >> 强制切换武器(发 use 命令刷新状态)
// ============================================================================
void Bot_SwitchWeapon(int client, int weapon)
{
    if (IsValidEntity(weapon))
    {
        char classname[32];
        GetEntityClassname(weapon, classname, sizeof(classname));
        FakeClientCommand(client, "use %s", classname);
    }
}

// ============================================================================
// >> 给单个槽位武器
// ============================================================================
void Bot_GiveSlotWeapon(int client, int slot, bool moneyMode)
{
    int slotWeapon = GetPlayerWeaponSlot(client, slot);

    // 槽位现有注册表自定义武器 → 跳过(不删不给)
    if (slotWeapon > MaxClients && IsValidEntity(slotWeapon))
    {
        char slotClass[64];
        GetEntityClassname(slotWeapon, slotClass, sizeof(slotClass));
        if (Buy_FindWeaponIndex(slotClass) != -1)
            return;
    }

    int picked = Bot_PickWeaponForSlot(client, slot, moneyMode);
    if (picked == -1)
        return;

    // 替换槽位现有原生武器(刀槽除外: 直接发自定义刀, 不删默认刀不清理,
    // 避免引擎"必须有刀"自动补刀/双刀竞态导致刀卡没或掉成世界实体)
    if (slot != 2 && slotWeapon > MaxClients && IsValidEntity(slotWeapon))
    {
        if (moneyMode)
        {
            char slotClass[64];
            GetEntityClassname(slotWeapon, slotClass, sizeof(slotClass));
            int refund = GetNativeWeaponPrice(client, slotClass);
            if (refund > 0)
                Client_SetMoney(client, Client_GetMoney(client) + refund);
        }

        RemovePlayerItem(client, slotWeapon);
        AcceptEntityInput(slotWeapon, "Kill");
    }

    GiveWeaponByIndex(client, picked);

    if (moneyMode)
    {
        int price = Buy_GetPrice(client, g_WeaponUseClassname[picked]);
        if (price > 0)
            Client_SetMoney(client, Client_GetMoney(client) - price);
    }
}

// ============================================================================
// >> 判断 bot 是否持有一把以上自定义武器(结算挡购用)
// ============================================================================
bool Bot_HasAnyCustomWeapon(int client)
{
    for (int slot = 0; slot <= 4; slot++)
    {
        int w = GetPlayerWeaponSlot(client, slot);
        if (w > MaxClients && IsValidEntity(w))
        {
            char cls[64];
            GetEntityClassname(w, cls, sizeof(cls));
            if (Buy_FindWeaponIndex(cls) != -1)
                return true;
        }
    }
    return false;
}

// ============================================================================
// >> 为槽位挑选武器(team + bucket + botchance + 金钱过滤 + 加权随机)
//    -1 = 无候选
// ============================================================================
int Bot_PickWeaponForSlot(int client, int slot, bool moneyMode)
{
    ArrayList candIdx = new ArrayList();
    ArrayList candChance = new ArrayList();

    int team = GetClientTeam(client);
    int money = Client_GetMoney(client);

    for (int i = 0; i < g_WeaponCount; i++)
    {
        if (Bot_GetWeaponBucket(g_WeaponUseClassname[i]) != slot)
            continue;

        if (StrEqual(g_WeaponTeam[i], "t", false) && team != CS_TEAM_T)
            continue;
        if (StrEqual(g_WeaponTeam[i], "ct", false) && team != CS_TEAM_CT)
            continue;

        int chance = Bot_GetInt(g_WeaponUseClassname[i], "botchance", 0);
        if (chance <= 0)
            continue;

        if (moneyMode)
        {
            int price = Buy_GetPrice(client, g_WeaponUseClassname[i]);
            if (price > money)
                continue;
        }

        candIdx.Push(i);
        candChance.Push(chance);
    }

    if (candIdx.Length == 0)
    {
        delete candIdx;
        delete candChance;
        return -1;
    }

    int total = 0;
    for (int i = 0; i < candChance.Length; i++)
        total += candChance.Get(i);

    if (total <= 0)
    {
        delete candIdx;
        delete candChance;
        return -1;
    }

    int roll = GetRandomInt(1, total);
    int picked = -1;
    int accum = 0;
    for (int i = 0; i < candIdx.Length; i++)
    {
        accum += candChance.Get(i);
        if (roll <= accum)
        {
            picked = candIdx.Get(i);
            break;
        }
    }

    delete candIdx;
    delete candChance;
    return picked;
}

// ============================================================================
// >> 挡购回调(buy/autobuy/rebuy): 系统开 + 该bot持有一把以上新武器才拦
// ============================================================================
public Action Bot_OnBuyCommand(int client, const char[] command, int argc)
{
    if (client <= 0 || client > MaxClients || !IsClientInGame(client) || !IsFakeClient(client))
        return Plugin_Continue;

    if (!Bot_IsSystemEnabled())
        return Plugin_Continue;

    if (!g_bBotBlockBuy[client])
        return Plugin_Continue;

    return Plugin_Handled;
}

// ============================================================================
// >> Bot 行为触发(机瞄/侧瞄/奔跑/检视)
//    机瞄/侧瞄: 由 weapon_fire 事件触发(bot 开枪不走 runcmd 按键检测), 开镜维持 botaimtime 秒自动退镜
//    奔跑/检视: 由 Combat 共享 OnPlayerRunCmd 末尾调用本函数(仅 bot + 系统开启进入)
//    判定链: 武器本身有该功能 → 本配置允许且几率>0 → 掷骰 → 独立 CD(botaimcd/botruncd/botinspectcd, 互不饿死)
//    奔跑触发后保持 botruncd 秒(bot 松开前进也持续冲刺), 开火/开镜等自然停止
// ============================================================================
void Bot_OnPlayerRunCmd(int client, int buttons, bool bReloading)
{
    if (client <= 0 || client > MaxClients || !IsClientInGame(client) || !IsFakeClient(client))
        return;

    if (!Bot_IsSystemEnabled())
        return;

    if (!IsPlayerAlive(client))
        return;

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0)
        return;

    // 机瞄/侧瞄维持时间到 → 自动退镜(独立于 CD, 每帧检查; 仅 bot 触发的瞄准受控)
    if (g_fBotAimEnd[client] > 0.0 && (g_bZooming[client] || g_iZoomInSwitch[client] != 0) && GetGameTime() >= g_fBotAimEnd[client])
    {
        StopZoom(client, weapon);
    }
    else if (g_fBotAimEnd[client] > 0.0 && (g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0) && GetGameTime() >= g_fBotAimEnd[client])
    {
        StopSideAim(client, weapon);
    }

    // 换弹/换枪(deploy)期间不触发(与各模块门槛一致)
    if (bReloading || GetEntPropFloat(client, Prop_Send, "m_flNextAttack") > GetGameTime())
        return;

    // 互斥: 已在瞄准/奔跑/检视中不叠加触发
    if (g_bZooming[client] || g_iZoomInSwitch[client] != 0 ||
        g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0 ||
        g_bRunning[client] || g_iRunStage[client] != 0 ||
        g_bInspecting[client])
        return;

    // 武器本身功能检测(机瞄/侧瞄改由 weapon_fire 事件触发)
    bool hasRun = Run_IsWeaponRunable(weapon);
    bool hasInspect = (Inspect_GetInt(weapon, "inspectseq", -1) >= 0 && Inspect_GetInt(weapon, "inspecttime", 0) > 0);

    char useClass[64];
    GetEntityClassname(weapon, useClass, sizeof(useClass));

    // 开火中不触发奔跑/检视
    if (buttons & IN_ATTACK)
        return;

    // 前进 → 奔跑(跑或不跑按几率; 触发后保持 botruncd 秒, 开火/开镜等自然停止)
    if ((buttons & IN_FORWARD) && hasRun)
    {
        int chance = Bot_GetInt(useClass, "botrun", 0);
        if (chance > 0 && GetGameTime() >= g_fBotRunCd[client] && GetRandomInt(1, 100) <= chance)
        {
            float runCd = Bot_GetRootFloat("botruncd", 5.0);
            Run_Start(client);
            g_bBotRunHold[client] = true;
            g_fBotRunHoldEnd[client] = GetGameTime() + runCd;
            g_fBotRunCd[client] = GetGameTime() + runCd;
        }
        return;
    }

    // 空闲(未开火/未前进) → 检视
    if (hasInspect && !(buttons & (IN_FORWARD | IN_ATTACK2 | IN_ATTACK3)))
    {
        int chance = Bot_GetInt(useClass, "botinspect", 0);
        if (chance > 0 && GetGameTime() >= g_fBotInspectCd[client] && GetRandomInt(1, 100) <= chance)
        {
            float inspectCd = Bot_GetRootFloat("botinspectcd", 5.0);
            Inspect_Start(client);
            g_fBotInspectCd[client] = GetGameTime() + inspectCd;
        }
    }
}

// ============================================================================
// >> Bot 开火事件(机瞄/侧瞄触发) - bot 开枪不走 runcmd 按键检测, 用 weapon_fire 事件检测
//    开镜独立 CD(botaimcd), 与奔跑/检视互不饿死; 开火打断奔跑/检视后接上开镜
// ============================================================================
public void Bot_EventWeaponFire(Event event, const char[] name, bool dontBroadcast)
{
    int client = GetClientOfUserId(event.GetInt("userid"));
    if (client <= 0 || client > MaxClients || !IsClientInGame(client) || !IsFakeClient(client))
        return;

    if (!Bot_IsSystemEnabled())
        return;

    if (!IsPlayerAlive(client))
        return;

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0)
        return;

    // 开镜独立 CD(奔跑/检视不再饿死开镜)
    if (GetGameTime() < g_fBotAimCd[client])
        return;

    // 已在瞄准中不叠加触发
    if (g_bZooming[client] || g_iZoomInSwitch[client] != 0 ||
        g_bSideAiming[client] || g_iSideAimInSwitch[client] != 0)
        return;

    // 开火打断奔跑/检视后接上开镜(开枪本就会自然停跑/停检视)
    if (g_bRunning[client] || g_iRunStage[client] != 0)
        Run_End(client);
    if (g_bInspecting[client])
        Inspect_End(client);

    // 武器本身功能检测
    bool hasZoom = Zoom_IsWeaponZoomable(weapon);
    bool hasSideAim = Zoom_IsWeaponSideAimable(weapon);
    if (!hasZoom && !hasSideAim)
        return;

    char useClass[64];
    GetEntityClassname(weapon, useClass, sizeof(useClass));

    int chance = Bot_GetInt(useClass, "botaim", 0);
    if (chance <= 0 || GetRandomInt(1, 100) > chance)
        return;

    float aimCd = Bot_GetRootFloat("botaimcd", 5.0);
    float aimTime = Bot_GetRootFloat("botaimtime", 3.0);

    // 机瞄/侧瞄合并一个几率, 两者都有随机选一
    bool useSide = false;
    if (hasZoom && hasSideAim)
        useSide = (GetRandomInt(0, 1) == 1);
    else if (hasSideAim)
        useSide = true;

    if (useSide)
        StartSideAim(client, weapon);
    else
        StartZoom(client, weapon);

    g_fBotAimEnd[client] = GetGameTime() + aimTime;
    g_fBotAimCd[client] = GetGameTime() + aimCd;
}

// ============================================================================
// >> 读取 Bot 配置根浮点字段
// ============================================================================
float Bot_GetRootFloat(const char[] key, float def)
{
    char buf[16];
    if (GetBotConfigRootValue(key, buf, sizeof(buf)))
        return StringToFloat(buf);
    return def;
}
