// ============================================================================
// >> Bot 武器配置加载(独立配置文件 HanWeaponBotData.cfg)
// ============================================================================
// 配置结构:
//   根节点键(根字段, 不随武器同步):
//     botsystem     Bot 武器系统总开关 1=开启 0=关闭(默认 0=关闭)
//     botmoney      1=金钱模式(按 bot 当前金钱过滤候选 + 替换原生武器返差价)
//                   0=免费模式(删默认武器直接随机给, 不返还也不扣钱)
//     botaimcd      Bot 开镜独立 CD(秒, 同类行为间冷却, 与奔跑/检视互不饿死)
//     botruncd      Bot 奔跑独立 CD(秒)
//     botinspectcd  Bot 检视独立 CD(秒)
//     botaimtime    Bot 机瞄/侧瞄开镜维持时长(秒, 到时自动退镜)
//   武器键组(与 HanWeaponData.cfg 自动同步):
//     useclassname      对应主配置的武器 useclassname(同步自动复制)
//     botchance         Bot 随机给予权重 0-100(0=不给予; 权重制: 概率=该值/候选总和, 多个100=均分)
//     botaim            Bot 使用机瞄/侧瞄几率 0-100(0=禁用; 开火时掷骰, 机瞄/侧瞄合并一个几率随机选一)
//     botrun            Bot 奔跑几率 0-100(0=禁用; 按住前进时掷骰, 跑或不跑)
//     botinspect        Bot 检视几率 0-100(0=禁用; 空闲时掷骰)
//   行为判定链: 先查武器本身是否有该功能(机瞄=ZoomData.zoom>0, 侧瞄=ZoomData.sideaim 1且有模型,
//               奔跑=RunData.run>0, 检视=InspectData.inspectseq>=0), 再查本配置允许且几率>0, 最后掷骰+同类行为独立CD
// 槽位判定: 读注册表武器脚本 bucket(0主/1副/2刀/3雷/4C4), 脚本无 bucket 的武器不参与 bot 给予
// 触发: player_spawn(仅 IsFakeClient) + 短延时确保默认武器已生成
// 挡购: 每 bot 独立判定(持有一把以上新武器才挡 buy/autobuy/rebuy), 未分到新武器的 bot 正常买枪
// 原生武器价格: 不建缓存, 使用时按需查 CS_AliasToWeaponID + CS_GetWeaponPrice(引擎原生)
// ============================================================================

void BotConfig_OnPluginStart()
{
    InitBotConfigIfNeeded();
    SyncBotConfig();
    ReadBotConfig();
}

void BotConfig_OnMapStart()
{
    InitBotConfigIfNeeded();
    SyncBotConfig();
    ReadBotConfig();
}

// ============================================================================
// >> 创建 Bot 配置
// ============================================================================
void InitBotConfigIfNeeded()
{
    char cfgDir[PLATFORM_MAX_PATH];
    char cfgFile[PLATFORM_MAX_PATH];

    BuildPath(Path_SM, cfgDir, sizeof(cfgDir), "configs/HanWeaponSystem");
    if (!DirExists(cfgDir))
    {
        CreateDirectory(cfgDir, 511);
    }

    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponBotData.cfg");
    if (!FileExists(cfgFile))
    {
        WriteDefaultBotConfig(cfgFile);
    }
}

void WriteDefaultBotConfig(const char[] path)
{
    Handle file = OpenFile(path, "w");
    if (file == INVALID_HANDLE)
        return;

    WriteFileLine(file, "// HanWeaponSystem Bot 武器配置文件");
    WriteFileLine(file, "// 此文件与 HanWeaponData.cfg 自动同步：主配置新增武器时会自动追加一个默认键组(botchance=0 不给予)");
    WriteFileLine(file, "// 根字段说明：");
    WriteFileLine(file, "// botsystem      Bot 武器系统总开关 1=开启 0=关闭(默认关闭)");
    WriteFileLine(file, "// botmoney       1=金钱模式(按bot当前金钱过滤候选+替换原生武器返差价)");
    WriteFileLine(file, "//                0=免费模式(删默认武器直接随机给, 不返还也不扣钱)");
    WriteFileLine(file, "// botaimcd       Bot开镜独立CD(秒, 同类行为间冷却, 与奔跑/检视互不饿死)");
    WriteFileLine(file, "// botruncd       Bot奔跑独立CD(秒)");
    WriteFileLine(file, "// botinspectcd   Bot检视独立CD(秒)");
    WriteFileLine(file, "// botaimtime     Bot机瞄/侧瞄开镜维持时长(秒, 到时自动退镜)");
    WriteFileLine(file, "// 武器段字段说明：");
    WriteFileLine(file, "// useclassname   对应主配置的武器 useclassname(同步段自动复制)");
    WriteFileLine(file, "// botchance      Bot随机给予权重 0-100(0=不给予; 权重制: 概率=该值/候选总和, 多个100=均分)");
    WriteFileLine(file, "// botaim         Bot使用机瞄/侧瞄几率 0-100(0=禁用; 开火时掷骰, 机瞄/侧瞄合并一个几率随机选一)");
    WriteFileLine(file, "// botrun         Bot奔跑几率 0-100(0=禁用; 按住前进时掷骰, 跑或不跑)");
    WriteFileLine(file, "// botinspect     Bot检视几率 0-100(0=禁用; 空闲时掷骰)");
    WriteFileLine(file, "// 行为判定链: 先查武器本身是否有该功能, 再查本配置允许且几率>0, 最后掷骰+同类行为独立CD");
    WriteFileLine(file, "// 槽位判定: 读注册表武器脚本 bucket(0主/1副/2刀/3雷/4C4), 脚本无 bucket 的武器不参与bot给予");
    WriteFileLine(file, "\"HanWeaponSystem\"");
    WriteFileLine(file, "{");
    WriteFileLine(file, "    \"botsystem\"    \"0\"");
    WriteFileLine(file, "    \"botmoney\"     \"0\"");
    WriteFileLine(file, "    \"botaimcd\"     \"5\"");
    WriteFileLine(file, "    \"botruncd\"     \"5\"");
    WriteFileLine(file, "    \"botinspectcd\" \"5\"");
    WriteFileLine(file, "    \"botaimtime\"   \"3\"");
    WriteFileLine(file, "}");

    CloseHandle(file);
}

// ============================================================================
// >> 同步追加缺失武器键组(只追加，绝不重写已有内容)
// ============================================================================
void SyncBotConfig()
{
    if (g_hWeaponConfig == INVALID_HANDLE)
        return;

    char cfgFile[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponBotData.cfg");

    if (!FileExists(cfgFile))
    {
        WriteDefaultBotConfig(cfgFile);
    }

    // 收集主配置中的武器键名 + useclassname
    char weaponKeys[MAX_WEAPON][64];
    char weaponUse[MAX_WEAPON][64];
    int total = 0;

    KvRewind(g_hWeaponConfig);
    if (KvGotoFirstSubKey(g_hWeaponConfig))
    {
        do
        {
            char name[64];
            KvGetSectionName(g_hWeaponConfig, name, sizeof(name));
            char useclass[64];
            KvGetString(g_hWeaponConfig, "useclassname", useclass, sizeof(useclass), "");

            if (strlen(name) > 0 && total < MAX_WEAPON)
            {
                strcopy(weaponKeys[total], sizeof(weaponKeys[]), name);
                strcopy(weaponUse[total], sizeof(weaponUse[]), useclass);
                total++;
            }
        } while (KvGotoNextKey(g_hWeaponConfig));
    }
    KvRewind(g_hWeaponConfig);

    // 找出 Bot 配置中缺失的键
    Handle botKv = CreateKeyValues("HanWeaponSystem");
    if (!FileToKeyValues(botKv, cfgFile))
    {
        CloseHandle(botKv);
        return;
    }

    ArrayList missingKeys = new ArrayList(64);
    ArrayList missingUse = new ArrayList(64);

    for (int i = 0; i < total; i++)
    {
        KvRewind(botKv);
        bool found = false;
        if (KvGotoFirstSubKey(botKv))
        {
            do
            {
                char sec[64];
                KvGetSectionName(botKv, sec, sizeof(sec));
                if (StrEqual(sec, weaponKeys[i], false))
                {
                    found = true;
                    break;
                }
            } while (KvGotoNextKey(botKv));
        }

        if (!found)
        {
            missingKeys.PushString(weaponKeys[i]);
            missingUse.PushString(weaponUse[i]);
        }
    }
    CloseHandle(botKv);

    int synced = missingKeys.Length;
    if (synced == 0)
    {
        delete missingKeys;
        delete missingUse;
        return;
    }

    // 逐行读取现有文件
    ArrayList lines = new ArrayList(512);
    Handle inFile = OpenFile(cfgFile, "r");
    if (inFile == INVALID_HANDLE)
    {
        delete missingKeys;
        delete missingUse;
        delete lines;
        return;
    }

    char line[512];
    while (!IsEndOfFile(inFile) && ReadFileLine(inFile, line, sizeof(line)))
    {
        lines.PushString(line);
    }
    CloseHandle(inFile);

    // 找到 root 的最后一个右花括号行
    int lastClose = -1;
    for (int i = 0; i < lines.Length; i++)
    {
        lines.GetString(i, line, sizeof(line));
        TrimString(line);
        if (StrEqual(line, "}"))
            lastClose = i;
    }

    // 重写文件并在 root 结尾前插入缺失键组
    Handle outFile = OpenFile(cfgFile, "w");
    if (outFile == INVALID_HANDLE)
    {
        delete missingKeys;
        delete missingUse;
        delete lines;
        return;
    }

    char out[512];
    for (int i = 0; i < lines.Length; i++)
    {
        lines.GetString(i, out, sizeof(out));
        TrimTrailingNewline(out);

        if (i == lastClose)
        {
            for (int m = 0; m < missingKeys.Length; m++)
            {
                char key[64], use[64];
                missingKeys.GetString(m, key, sizeof(key));
                missingUse.GetString(m, use, sizeof(use));

                WriteFileLine(outFile, "");
                WriteFileLine(outFile, "    \"%s\"", key);
                WriteFileLine(outFile, "    {");
                WriteFileLine(outFile, "        \"useclassname\"    \"%s\"", use);
                WriteFileLine(outFile, "        \"botchance\"       \"0\"");
                WriteFileLine(outFile, "        \"botaim\"          \"0\"");
                WriteFileLine(outFile, "        \"botrun\"          \"0\"");
                WriteFileLine(outFile, "        \"botinspect\"      \"0\"");
                WriteFileLine(outFile, "    }");
            }
        }

        WriteFileLine(outFile, "%s", out);
    }
    CloseHandle(outFile);

    delete missingKeys;
    delete missingUse;
    delete lines;

    PrintToServer("[HWS] Bot 配置已同步 %d 个缺失武器", synced);
}

// ============================================================================
// >> 读取 Bot 配置
// ============================================================================
void ReadBotConfig()
{
    if (g_hBotConfig != INVALID_HANDLE)
    {
        CloseHandle(g_hBotConfig);
        g_hBotConfig = INVALID_HANDLE;
    }

    char cfgFile[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponBotData.cfg");

    g_hBotConfig = CreateKeyValues("HanWeaponSystem");
    if (!FileToKeyValues(g_hBotConfig, cfgFile))
    {
        CloseHandle(g_hBotConfig);
        g_hBotConfig = INVALID_HANDLE;
        return;
    }

    KvRewind(g_hBotConfig);
}

// ============================================================================
// >> 读取 Bot 配置根字段(总开关/金钱模式)
// ============================================================================
bool GetBotConfigRootValue(const char[] key, char[] value, int valueLen)
{
    if (g_hBotConfig == INVALID_HANDLE)
        return false;

    KvRewind(g_hBotConfig);
    KvGetString(g_hBotConfig, key, value, valueLen, "");
    return (strlen(value) > 0);
}

// ============================================================================
// >> 通过 useclassname 读取 Bot 配置武器段值
// ============================================================================
bool GetBotConfigValue(const char[] useclassname, const char[] key, char[] value, int valueLen)
{
    if (g_hBotConfig == INVALID_HANDLE || strlen(useclassname) <= 0)
        return false;

    KvRewind(g_hBotConfig);
    if (!KvGotoFirstSubKey(g_hBotConfig))
        return false;

    do
    {
        char useClass[64];
        KvGetString(g_hBotConfig, "useclassname", useClass, sizeof(useClass), "");

        if (StrContains(useClass, "weapon_") == 0)
            strcopy(useClass, sizeof(useClass), useClass[7]);

        char target[64];
        strcopy(target, sizeof(target), useclassname);
        if (StrContains(target, "weapon_") == 0)
            strcopy(target, sizeof(target), target[7]);

        if (StrEqual(target, useClass, false))
        {
            KvGetString(g_hBotConfig, key, value, valueLen, "");
            return (strlen(value) > 0);
        }

    } while (KvGotoNextKey(g_hBotConfig));

    return false;
}

int Bot_GetInt(const char[] useclassname, const char[] key, int def)
{
    char buf[16];
    if (GetBotConfigValue(useclassname, key, buf, sizeof(buf)))
        return StringToInt(buf);
    return def;
}
