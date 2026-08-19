#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <sdktools>
#include <HanWeaponSystem>

#define PLUGIN_VERSION "1.0.0"

// 触发键: E 键(IN_USE 上升沿)
#define TRIGGER_BUTTON IN_USE

char g_sCfgPath[PLATFORM_MAX_PATH];

public Plugin myinfo =
{
    name = "[H-AN]自定义动画API测试",
    author = "H-AN",
    description = "HanWeaponSystem v6.9 自定义动画 API 独立测试插件",
    version = PLUGIN_VERSION,
    url = ""
};

public void OnPluginStart()
{
    BuildPath(Path_SM, g_sCfgPath, sizeof(g_sCfgPath), "configs/HanWeaponSystemCustomAnimTest.cfg");

    if (!FileExists(g_sCfgPath))
        WriteDefaultConfig();

    RegConsoleCmd("sm_canim", Cmd_CustomAnim, "触发自定义动画: sm_canim [seq] [frames] [repeat] [interruptable] (无参数用配置)");
    RegConsoleCmd("sm_canimstatus", Cmd_CustomAnimStatus, "查看当前自定义动画状态");
    RegConsoleCmd("sm_canimstop", Cmd_CustomAnimStop, "停止当前自定义动画");
    RegConsoleCmd("sm_canimweapon", Cmd_CustomAnimWeapon, "显示当前武器 classname");
    RegConsoleCmd("sm_canimreload", Cmd_CustomAnimReload, "重新生成默认配置文件");

    if (!LibraryExists("HanWeaponSystem"))
    {
        PrintToServer("[H-AN-ANIMTEST] 警告: 未检测到 HanWeaponSystem 主插件, API 不可用, 请先加载主插件 v6.9");
    }
}

public void OnAllPluginsLoaded()
{
    if (LibraryExists("HanWeaponSystem"))
    {
        PrintToServer("[H-AN-ANIMTEST] HanWeaponSystem 已加载, API 就绪");
    }
}

// ============================================================================
// >> E 键触发(IN_USE 上升沿)
// ============================================================================
public Action OnPlayerRunCmd(int client, int &buttons, int &impulse, float vel[3], float angles[3], int &weapon, int &subtype, int &cmdnum, int &tickcount, int &seed, int mouse[2])
{
    if (client < 1 || client > MaxClients || !IsClientInGame(client))
        return Plugin_Continue;

    static bool s_bPrev[MAXPLAYERS + 1];
    bool down = (buttons & TRIGGER_BUTTON) != 0;

    if (down && !s_bPrev[client])
    {
        s_bPrev[client] = true;
        if (IsPlayerAlive(client))
            TryPlayByConfig(client);
    }
    else if (!down)
    {
        s_bPrev[client] = false;
    }

    return Plugin_Continue;
}

// ============================================================================
// >> 从配置读取武器参数并触发
// ============================================================================
void TryPlayByConfig(int client)
{
    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0)
    {
        PrintToChat(client, "[动画测试] 当前没有武器");
        return;
    }

    char className[64];
    GetEntityClassname(weapon, className, sizeof(className));

    int seq, frames;
    bool repeat, interruptable;
    if (!GetConfigForWeapon(className, seq, frames, repeat, interruptable))
    {
        PrintToChat(client, "[动画测试] 当前武器 classname=[%s] 未在配置中定义", className);
        PrintToChat(client, "[动画测试] 请编辑 %s 或输入 !canimweapon 查看当前 classname", g_sCfgPath);
        return;
    }

    if (seq < 0 || frames <= 0)
    {
        PrintToChat(client, "[动画测试] 配置不完整: seq=%d frames=%d (两者都必须填写)", seq, frames);
        return;
    }

    PlayAnim(client, className, seq, frames, repeat, interruptable);
}

// ============================================================================
// >> 实际调用 API(带结果反馈)
// ============================================================================
void PlayAnim(int client, const char[] className, int seq, int frames, bool repeat, bool interruptable)
{
    if (GetFeatureStatus(FeatureType_Native, "Han_SetClientCustomAnim") != FeatureStatus_Available)
    {
        PrintToChat(client, "[动画测试] 主插件 API 不可用(未加载 HanWeaponSystem v6.9?)");
        return;
    }

    bool ok = Han_SetClientCustomAnim(client, seq, frames, repeat, interruptable);
    PrintToChat(client, "[动画测试] Han_SetClientCustomAnim(seq=%d frames=%d repeat=%d interruptable=%d) -> %s",
        seq, frames, repeat ? 1 : 0, interruptable ? 1 : 0, ok ? "成功" : "失败(可能已在播放且不允许重播)");
    PrintToServer("[H-AN-ANIMTEST] client=%d weapon=%s seq=%d frames=%d repeat=%d interruptable=%d -> %s",
        client, className, seq, frames, repeat ? 1 : 0, interruptable ? 1 : 0, ok ? "true" : "false");
}

// ============================================================================
// >> 按武器 classname 查找配置段
// ============================================================================
bool GetConfigForWeapon(const char[] className, int &seq, int &frames, bool &repeat, bool &interruptable)
{
    KeyValues kv = new KeyValues("HanWeaponSystemCustomAnimTest");
    if (!kv.ImportFromFile(g_sCfgPath))
    {
        delete kv;
        return false;
    }

    char search[64];
    strcopy(search, sizeof(search), className);
    if (StrContains(search, "weapon_") == 0)
        strcopy(search, sizeof(search), search[7]);

    char section[64];
    bool found = false;

    if (kv.GotoFirstSubKey())
    {
        do
        {
            kv.GetSectionName(section, sizeof(section));

            char secSearch[64];
            strcopy(secSearch, sizeof(secSearch), section);
            if (StrContains(secSearch, "weapon_") == 0)
                strcopy(secSearch, sizeof(secSearch), secSearch[7]);

            if (StrEqual(search, secSearch, false))
            {
                seq = kv.GetNum("seq", -1);
                frames = kv.GetNum("frames", 0);
                repeat = kv.GetNum("repeat", 0) != 0;
                interruptable = kv.GetNum("interruptable", 1) != 0;
                found = true;
                break;
            }
        } while (kv.GotoNextKey());
    }

    delete kv;
    return found;
}

// ============================================================================
// >> 命令: sm_canim [seq] [frames] [repeat] [interruptable]
// ============================================================================
public Action Cmd_CustomAnim(int client, int args)
{
    if (client < 1 || client > MaxClients || !IsClientInGame(client) || !IsPlayerAlive(client))
    {
        ReplyToCommand(client, "本命令只能由存活玩家使用");
        return Plugin_Handled;
    }

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0)
    {
        ReplyToCommand(client, "当前没有武器");
        return Plugin_Handled;
    }

    char className[64];
    GetEntityClassname(weapon, className, sizeof(className));

    int seq, frames;
    bool repeat, interruptable;

    if (args >= 1)
    {
        char arg[16];
        GetCmdArg(1, arg, sizeof(arg));
        seq = StringToInt(arg);
    }
    else
    {
        if (!GetConfigForWeapon(className, seq, frames, repeat, interruptable))
        {
            ReplyToCommand(client, "当前武器 [%s] 未配置, 请用 sm_canim <seq> <frames> [repeat] [interruptable] 手动指定", className);
            return Plugin_Handled;
        }
    }

    if (args >= 2)
    {
        char arg[16];
        GetCmdArg(2, arg, sizeof(arg));
        frames = StringToInt(arg);
    }
    if (args >= 3)
    {
        char arg[16];
        GetCmdArg(3, arg, sizeof(arg));
        repeat = StringToInt(arg) != 0;
    }
    if (args >= 4)
    {
        char arg[16];
        GetCmdArg(4, arg, sizeof(arg));
        interruptable = StringToInt(arg) != 0;
    }

    if (seq < 0 || frames <= 0)
    {
        ReplyToCommand(client, "参数非法: seq=%d frames=%d (frames 必须大于0)", seq, frames);
        return Plugin_Handled;
    }

    PlayAnim(client, className, seq, frames, repeat, interruptable);
    return Plugin_Handled;
}

// ============================================================================
// >> 命令: 状态/停止/查武器/重载配置
// ============================================================================
public Action Cmd_CustomAnimStatus(int client, int args)
{
    if (client < 1 || client > MaxClients || !IsClientInGame(client))
    {
        ReplyToCommand(client, "本命令只能由玩家使用");
        return Plugin_Handled;
    }

    if (GetFeatureStatus(FeatureType_Native, "Han_IsClientCustomAnim") != FeatureStatus_Available)
    {
        ReplyToCommand(client, "主插件 API 不可用(未加载 HanWeaponSystem v6.9?)");
        return Plugin_Handled;
    }

    bool active = Han_IsClientCustomAnim(client);
    ReplyToCommand(client, "Han_IsClientCustomAnim -> %s", active ? "true (播放中)" : "false (未播放)");
    return Plugin_Handled;
}

public Action Cmd_CustomAnimStop(int client, int args)
{
    if (client < 1 || client > MaxClients || !IsClientInGame(client))
    {
        ReplyToCommand(client, "本命令只能由玩家使用");
        return Plugin_Handled;
    }

    if (GetFeatureStatus(FeatureType_Native, "Han_StopClientCustomAnim") != FeatureStatus_Available)
    {
        ReplyToCommand(client, "主插件 API 不可用(未加载 HanWeaponSystem v6.9?)");
        return Plugin_Handled;
    }

    Han_StopClientCustomAnim(client);
    ReplyToCommand(client, "已调用 Han_StopClientCustomAnim (应收到 Forward 结束事件)");
    return Plugin_Handled;
}

public Action Cmd_CustomAnimWeapon(int client, int args)
{
    if (client < 1 || client > MaxClients || !IsClientInGame(client))
    {
        ReplyToCommand(client, "本命令只能由玩家使用");
        return Plugin_Handled;
    }

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0)
    {
        ReplyToCommand(client, "当前没有武器");
        return Plugin_Handled;
    }

    char className[64];
    GetEntityClassname(weapon, className, sizeof(className));
    ReplyToCommand(client, "当前武器 classname = [%s] (配置文件中请使用该名称)", className);
    return Plugin_Handled;
}

public Action Cmd_CustomAnimReload(int client, int args)
{
    WriteDefaultConfig();
    ReplyToCommand(client, "已重新生成默认配置: %s (已有文件会被覆盖)", g_sCfgPath);
    return Plugin_Handled;
}

// ============================================================================
// >> Forward 监听(验证广播)
// ============================================================================
public void Han_OnClientCustomAnimStart(int client, int seq, int frames)
{
    PrintToChat(client, "[动画测试] >> Forward Han_OnClientCustomAnimStart: seq=%d frames=%d", seq, frames);
    PrintToServer("[H-AN-ANIMTEST] Forward Start: client=%d seq=%d frames=%d", client, seq, frames);
}

public void Han_OnClientCustomAnimEnd(int client)
{
    PrintToChat(client, "[动画测试] >> Forward Han_OnClientCustomAnimEnd");
    PrintToServer("[H-AN-ANIMTEST] Forward End: client=%d", client);
}

// ============================================================================
// >> 生成默认配置
// ============================================================================
void WriteDefaultConfig()
{
    Handle file = OpenFile(g_sCfgPath, "w");
    if (file == INVALID_HANDLE)
    {
        PrintToServer("[H-AN-ANIMTEST] 无法创建配置文件: %s", g_sCfgPath);
        return;
    }

    WriteFileLine(file, "// HanWeaponSystem v6.9 自定义动画 API 测试配置");
    WriteFileLine(file, "// 段名 = 武器 classname (游戏内 !canimweapon 查看当前武器 classname; 自动忽略 weapon_ 前缀)");
    WriteFileLine(file, "// 字段:");
    WriteFileLine(file, "//   seq           动画序列号(QC 里的编号, 必填, 大于等于0)");
    WriteFileLine(file, "//   frames        动画时长(帧, 30fps 基准, 必填, 大于0)");
    WriteFileLine(file, "//   repeat        0=播放中再次触发被拒绝  1=允许重播打断自己");
    WriteFileLine(file, "//   interruptable 0=绝对霸体播完  1=可被机瞄/侧瞄/奔跑打断(默认)");
    WriteFileLine(file, "// 触发方式:");
    WriteFileLine(file, "//   E 键(IN_USE 上升沿) 或控制台命令 sm_canim [seq] [frames] [repeat] [interruptable]");
    WriteFileLine(file, "\"HanWeaponSystemCustomAnimTest\"");
    WriteFileLine(file, "{");
    WriteFileLine(file, "    \"weapon_knife_snake\"");
    WriteFileLine(file, "    {");
    WriteFileLine(file, "        \"seq\"            \"40\"");
    WriteFileLine(file, "        \"frames\"         \"36\"");
    WriteFileLine(file, "        \"repeat\"         \"0\"");
    WriteFileLine(file, "        \"interruptable\"  \"1\"");
    WriteFileLine(file, "    }");
    WriteFileLine(file, "}");

    CloseHandle(file);
    PrintToServer("[H-AN-ANIMTEST] 已生成默认配置: %s", g_sCfgPath);
}