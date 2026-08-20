#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <sdktools>
#include <sdkhooks>

#define PLUGIN_VERSION "1.0.2"

// 状态
bool g_bEnabled = false;                 // 总开关: 开了才打日志/拦截
bool g_bWatchMode = false;               // 观察模式: 把观察者听到被观察者的音效打印到其游戏聊天
char g_sFilter[128];                     // 路径过滤子串 (空 = 全部)
int g_iTarget = 0;                       // 聊天显示目标 (0 = 仅服务器控制台)
bool g_bBlock = false;                   // 静音: 命中音返回 Plugin_Handled
char g_sReplace[PLATFORM_MAX_PATH];      // 替换路径 (空 = 不替换)
bool g_bGodMode = false;                 // 无敌模式: CT/T 互射不掉血
bool g_bGodHooked[MAXPLAYERS + 1];       // 每人是否已挂 OnTakeDamage 钩子

public Plugin myinfo =
{
    name = "[H-AN]换弹音效测试",
    author = "H-AN",
    description = "武器换弹原版音效拦截/替换可行性测试插件 (AddNormalSoundHook)",
    version = PLUGIN_VERSION,
    url = ""
};

public void OnPluginStart()
{
    RegConsoleCmd("sm_rsound", Cmd_Toggle, "日志总开关: sm_rsound [1/0]");
    RegConsoleCmd("sm_rsound_watch", Cmd_Watch, "观察模式: sm_rsound_watch [1/0] (观察者听到被观察者的音效时打印到其聊天)");
    RegConsoleCmd("sm_rsound_filter", Cmd_Filter, "设置路径过滤子串: sm_rsound_filter <子串> (空=全部)");
    RegConsoleCmd("sm_rsound_target", Cmd_Target, "设置聊天目标玩家: sm_rsound_target <玩家ID> (0=仅控制台)");
    RegConsoleCmd("sm_rsound_block", Cmd_Block, "静音开关: sm_rsound_block [1/0] (命中音返回 Plugin_Handled)");
    RegConsoleCmd("sm_rsound_replace", Cmd_Replace, "替换开关: sm_rsound_replace <路径> (空=关)");
    RegConsoleCmd("sm_rsound_god", Cmd_God, "无敌模式: sm_rsound_god [1/0] (CT/T 互射不掉血)");
    RegConsoleCmd("sm_rsound_status", Cmd_Status, "显示当前状态");

    AddNormalSoundHook(TestSoundHook);
    PrintToServer("[RSTEST] 已挂载 AddNormalSoundHook, 输入 sm_rsound 1 开始记录");
}

public void OnClientPutInServer(int client)
{
    HookGod(client);
}

public void OnClientDisconnect(int client)
{
    g_bGodHooked[client] = false;
}

// ============================================================================
// >> 声音钩子 (所有经过服务端 IEngineSound 的音效都会进这里)
// ============================================================================
public Action TestSoundHook(int clients[64], int &numClients, char sample[PLATFORM_MAX_PATH], int &entity, int &channel, float &volume, int &level, int &pitch, int &flags)
{
    bool bMatch = (g_sFilter[0] == '\0' || StrContains(sample, g_sFilter, false) != -1);

    if (g_bEnabled && bMatch)
    {
        char classname[64];
        if (entity > 0 && IsValidEntity(entity))
            GetEntityClassname(entity, classname, sizeof(classname));
        else
            strcopy(classname, sizeof(classname), "(无实体)");

        char action[16] = "观察";
        if (g_bBlock)
            strcopy(action, sizeof(action), "静音");
        else if (g_sReplace[0] != '\0')
            strcopy(action, sizeof(action), "替换");

        LogBoth("[RSTEST][%s] sample=[%s] entity=%d(%s) channel=%d vol=%.2f level=%d pitch=%d flags=0x%X clients=%d",
            action, sample, entity, classname, channel, volume, level, pitch, flags, numClients);

        if (g_bBlock)
            return Plugin_Handled;

        if (g_sReplace[0] != '\0')
        {
            strcopy(sample, sizeof(sample), g_sReplace);
            return Plugin_Changed;
        }
    }

    // 观察模式: 独立于日志开关, 把"观察者正在听被观察者发出的音效"打印到观察者游戏聊天
    if (g_bWatchMode && bMatch)
        WatchEmit(clients, numClients, sample, entity, channel, volume, level, pitch);

    return Plugin_Continue;
}

// ============================================================================
// >> 观察模式: 音效源 = entity 本身是玩家/bot, 或 entity 的持有者(武器→玩家/bot)
//    遍历所有听者, 找出正在观察该源的观察者(可多个) → 打印到其游戏聊天
// ============================================================================
void WatchEmit(int clients[64], int numClients, const char[] sample, int entity, int channel, float volume, int level, int pitch)
{
    int source = 0;
    if (entity >= 1 && entity <= MaxClients)
    {
        source = entity;
    }
    else if (entity > 0 && IsValidEntity(entity))
    {
        int owner = GetEntPropEnt(entity, Prop_Send, "m_hOwnerEntity");
        if (owner >= 1 && owner <= MaxClients)
            source = owner;
    }

    if (source <= 0)
        return;

    char classname[64];
    if (entity > 0 && IsValidEntity(entity))
        GetEntityClassname(entity, classname, sizeof(classname));
    else
        strcopy(classname, sizeof(classname), "?");

    char srcName[32];
    GetClientName(source, srcName, sizeof(srcName));

    int obsCount = 0;
    for (int i = 0; i < numClients; i++)
    {
        int listener = clients[i];
        if (listener < 1 || listener > MaxClients || !IsClientInGame(listener))
            continue;

        if (GetEntPropEnt(listener, Prop_Send, "m_hObserverTarget") != source)
            continue;

        obsCount++;
        PrintToChat(listener, "[RSTEST][观察] %s(%d) 的音效: [%s] ent=%d(%s) ch=%d vol=%.2f lvl=%d pitch=%d (观战模式=%d)",
            srcName, source, sample, entity, classname, channel, volume, level, pitch,
            GetEntProp(listener, Prop_Send, "m_iObserverMode"));
    }

    if (obsCount > 0)
        PrintToServer("[RSTEST][观察] %s(%d) -> %d 个观察者收到: [%s] ent=%d(%s) ch=%d",
            srcName, source, obsCount, sample, entity, classname, channel);
}

// ============================================================================
// >> 同时输出到服务器控制台 + 目标玩家聊天
// ============================================================================
void LogBoth(const char[] format, any ...)
{
    char buffer[512];
    VFormat(buffer, sizeof(buffer), format, 2);

    PrintToServer("%s", buffer);

    if (g_iTarget > 0 && g_iTarget <= MaxClients && IsClientInGame(g_iTarget))
        PrintToChat(g_iTarget, "%s", buffer);
}

// ============================================================================
// >> 无敌模式: 屏蔽所有伤害(含枪伤/爆炸/掉落), 方便持续测试
// ============================================================================
public Action OnGodTakeDamage(int victim, int &attacker, int &inflictor, float &damage, int &damagetype, int &weapon, float damageForce[3], float damagePosition[3])
{
    if (g_bGodMode)
        return Plugin_Handled;
    return Plugin_Continue;
}

void HookGod(int client)
{
    if (g_bGodHooked[client])
        return;
    SDKHook(client, SDKHook_OnTakeDamage, OnGodTakeDamage);
    g_bGodHooked[client] = true;
}

public Action Cmd_God(int client, int args)
{
    if (args >= 1)
    {
        char arg[8];
        GetCmdArg(1, arg, sizeof(arg));
        g_bGodMode = StringToInt(arg) != 0;
    }
    else
    {
        g_bGodMode = !g_bGodMode;
    }

    if (g_bGodMode)
    {
        for (int i = 1; i <= MaxClients; i++)
            if (IsClientInGame(i))
                HookGod(i);
    }

    ReplyToCommand(client, "[RSTEST] 无敌模式: %s (CT/T 互射不掉血)", g_bGodMode ? "开" : "关");
    PrintToServer("[RSTEST] 无敌模式 -> %s", g_bGodMode ? "on" : "off");
    return Plugin_Handled;
}

// ============================================================================
// >> 命令
// ============================================================================
public Action Cmd_Toggle(int client, int args)
{
    if (args >= 1)
    {
        char arg[8];
        GetCmdArg(1, arg, sizeof(arg));
        g_bEnabled = StringToInt(arg) != 0;
    }
    else
    {
        g_bEnabled = !g_bEnabled;
    }

    ReplyToCommand(client, "[RSTEST] 日志总开关: %s (所有经过服务端的音效将被打日志/处理)",
        g_bEnabled ? "开" : "关");
    PrintToServer("[RSTEST] 日志总开关 -> %s", g_bEnabled ? "on" : "off");
    return Plugin_Handled;
}

public Action Cmd_Watch(int client, int args)
{
    if (args >= 1)
    {
        char arg[8];
        GetCmdArg(1, arg, sizeof(arg));
        g_bWatchMode = StringToInt(arg) != 0;
    }
    else
    {
        g_bWatchMode = !g_bWatchMode;
    }

    ReplyToCommand(client, "[RSTEST] 观察模式: %s (观察者听到被观察者的音效时会打印到其游戏聊天)",
        g_bWatchMode ? "开" : "关");
    PrintToServer("[RSTEST] 观察模式 -> %s", g_bWatchMode ? "on" : "off");
    return Plugin_Handled;
}

public Action Cmd_Filter(int client, int args)
{
    if (args >= 1)
    {
        GetCmdArg(1, g_sFilter, sizeof(g_sFilter));
    }
    else
    {
        g_sFilter[0] = '\0';
    }

    ReplyToCommand(client, "[RSTEST] 过滤子串: [%s]%s", g_sFilter,
        g_sFilter[0] == '\0' ? " (空=全部音效)" : "");
    return Plugin_Handled;
}

public Action Cmd_Target(int client, int args)
{
    if (args >= 1)
    {
        char arg[16];
        GetCmdArg(1, arg, sizeof(arg));
        g_iTarget = StringToInt(arg);
    }
    else
    {
        g_iTarget = client;
    }

    if (g_iTarget > 0 && g_iTarget <= MaxClients && IsClientInGame(g_iTarget))
        ReplyToCommand(client, "[RSTEST] 聊天目标 -> %N (id %d)", g_iTarget, g_iTarget);
    else
    {
        g_iTarget = 0;
        ReplyToCommand(client, "[RSTEST] 聊天目标 -> 无 (仅服务器控制台打印)");
    }
    return Plugin_Handled;
}

public Action Cmd_Block(int client, int args)
{
    if (args >= 1)
    {
        char arg[8];
        GetCmdArg(1, arg, sizeof(arg));
        g_bBlock = StringToInt(arg) != 0;
    }
    else
    {
        g_bBlock = !g_bBlock;
    }

    ReplyToCommand(client, "[RSTEST] 静音开关: %s%s",
        g_bBlock ? "开" : "关",
        g_bBlock && g_sFilter[0] == '\0' ? " (警告: 过滤为空将静音所有音效!)" : "");
    return Plugin_Handled;
}

public Action Cmd_Replace(int client, int args)
{
    if (args >= 1)
    {
        GetCmdArgString(g_sReplace, sizeof(g_sReplace));
    }
    else
    {
        g_sReplace[0] = '\0';
    }

    ReplyToCommand(client, "[RSTEST] 替换路径: [%s]%s", g_sReplace,
        g_sReplace[0] == '\0' ? " (空=关闭替换)" : " (命中音将被改为该路径, 需为服务端已存在的音效)");
    return Plugin_Handled;
}

public Action Cmd_Status(int client, int args)
{
    ReplyToCommand(client, "[RSTEST] 日志=%s | 观察=%s | 过滤=[%s] | 目标=%d | 静音=%s | 替换=[%s] | 无敌=%s",
        g_bEnabled ? "开" : "关", g_bWatchMode ? "开" : "关", g_sFilter, g_iTarget,
        g_bBlock ? "开" : "关", g_sReplace, g_bGodMode ? "开" : "关");
    return Plugin_Handled;
}