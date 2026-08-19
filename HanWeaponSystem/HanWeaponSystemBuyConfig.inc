// ============================================================================
// >> 武器买卖配置加载(独立配置文件 HanWeaponBuyData.cfg)
// ============================================================================
// 配置结构:
//   根节点键(根字段, 不随武器同步):
//     buysystem     买卖系统总开关 1=开启 0=关闭
//     buymode       购买时间模式 always=随时随地 buytime=仅购买时间内
//     buycommand    购买命令名(默认 sm_buy)
//     sellcommand   卖出命令名(默认 sm_sell)
//   武器键组(与 HanWeaponData.cfg 自动同步 + 可手动添加原生武器段):
//     useclassname      对应主配置的武器 useclassname(同步自动复制; 手动添加的原生武器需自己填)
//     name              菜单显示名(空=显示 useclassname)
//     buyable           1=可购买 0=关闭(仅对注册表自定义武器生效)
//     price             购买价格(空=注册表武器自动回落武器脚本 WeaponPrice;
//                       原生武器自动回落引擎原生价格 CS_GetWeaponPrice, 均不填=0)
//     sellable          1=可卖出 0=关闭
//     sellratio         本队武器卖出比例(默认 0.5=半价)
//     sellratio_other   敌方/缴获武器卖出比例(空=回落 sellratio)
// 推论(已与需求方确认):
//   A. 原生武器(非注册表)价格: 手填 price 优先, 不填自动回落引擎原生价格(CS_GetWeaponPrice, 按需实时查询), 引擎不可用再回落内置小表
//   B. 买菜单只列注册表自定义武器; 手写的原生段只参与卖出
//   C. 缴获加价(sellratio_other)只对注册表武器生效, 原生武器按双方通用走 sellratio
// ============================================================================

void BuyConfig_OnPluginStart()
{
    InitBuyConfigIfNeeded();
    SyncBuyConfig();
    ReadBuyConfig();
}

void BuyConfig_OnMapStart()
{
    InitBuyConfigIfNeeded();
    SyncBuyConfig();
    ReadBuyConfig();
}

// ============================================================================
// >> 创建买卖配置
// ============================================================================
void InitBuyConfigIfNeeded()
{
    char cfgDir[PLATFORM_MAX_PATH];
    char cfgFile[PLATFORM_MAX_PATH];

    BuildPath(Path_SM, cfgDir, sizeof(cfgDir), "configs/HanWeaponSystem");
    if (!DirExists(cfgDir))
    {
        CreateDirectory(cfgDir, 511);
    }

    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponBuyData.cfg");
    if (!FileExists(cfgFile))
    {
        WriteDefaultBuyConfig(cfgFile);
    }
}

void WriteDefaultBuyConfig(const char[] path)
{
    Handle file = OpenFile(path, "w");
    if (file == INVALID_HANDLE)
        return;

    WriteFileLine(file, "// HanWeaponSystem 武器买卖配置文件");
    WriteFileLine(file, "// 此文件与 HanWeaponData.cfg 自动同步：主配置新增武器时会自动追加一个默认键组");
    WriteFileLine(file, "// 也可手动添加任意武器段(如卖出原生武器 weapon_usp), 自己填写 useclassname 与价格");
    WriteFileLine(file, "// 根字段说明：");
    WriteFileLine(file, "// buysystem      买卖系统总开关 1=开启 0=关闭");
    WriteFileLine(file, "// buymode        购买时间模式 always=随时随地 buytime=仅购买时间内");
    WriteFileLine(file, "// buycommand     购买命令名(默认 sm_buy)");
    WriteFileLine(file, "// sellcommand    卖出命令名(默认 sm_sell)");
    WriteFileLine(file, "// 武器段字段说明：");
    WriteFileLine(file, "// useclassname   对应主配置的武器 useclassname(同步段自动复制; 手动添加的原生武器需自己填)");
    WriteFileLine(file, "// name           菜单显示名(空=显示 useclassname)");
    WriteFileLine(file, "// buyable        1=可购买 0=关闭(仅对注册表自定义武器生效)");
    WriteFileLine(file, "// price          购买价格(空=注册表武器自动回落脚本 WeaponPrice; 原生武器自动回落引擎原生价格)");
    WriteFileLine(file, "// sellable       1=可卖出 0=关闭");
    WriteFileLine(file, "// sellratio      本队武器卖出比例(默认 0.5=半价)");
    WriteFileLine(file, "// sellratio_other 敌方/缴获武器卖出比例(空=回落 sellratio)");
    WriteFileLine(file, "\"HanWeaponSystem\"");
    WriteFileLine(file, "{");
    WriteFileLine(file, "    \"buysystem\"       \"1\"");
    WriteFileLine(file, "    \"buymode\"         \"always\"");
    WriteFileLine(file, "    \"buycommand\"      \"sm_buy\"");
    WriteFileLine(file, "    \"sellcommand\"     \"sm_sell\"");
    WriteFileLine(file, "}");

    CloseHandle(file);
}

// ============================================================================
// >> 同步追加缺失武器键组(只追加，绝不重写已有内容)
// ============================================================================
void SyncBuyConfig()
{
    if (g_hWeaponConfig == INVALID_HANDLE)
        return;

    char cfgFile[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponBuyData.cfg");

    if (!FileExists(cfgFile))
    {
        WriteDefaultBuyConfig(cfgFile);
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

    // 找出买卖配置中缺失的键
    Handle buyKv = CreateKeyValues("HanWeaponSystem");
    if (!FileToKeyValues(buyKv, cfgFile))
    {
        CloseHandle(buyKv);
        return;
    }

    ArrayList missingKeys = new ArrayList(64);
    ArrayList missingUse = new ArrayList(64);

    for (int i = 0; i < total; i++)
    {
        KvRewind(buyKv);
        bool found = false;
        if (KvGotoFirstSubKey(buyKv))
        {
            do
            {
                char sec[64];
                KvGetSectionName(buyKv, sec, sizeof(sec));
                if (StrEqual(sec, weaponKeys[i], false))
                {
                    found = true;
                    break;
                }
            } while (KvGotoNextKey(buyKv));
        }

        if (!found)
        {
            missingKeys.PushString(weaponKeys[i]);
            missingUse.PushString(weaponUse[i]);
        }
    }
    CloseHandle(buyKv);

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
                WriteFileLine(outFile, "        \"name\"            \"\"");
                WriteFileLine(outFile, "        \"buyable\"         \"1\"");
                WriteFileLine(outFile, "        \"price\"           \"\"");
                WriteFileLine(outFile, "        \"sellable\"        \"1\"");
                WriteFileLine(outFile, "        \"sellratio\"       \"0.5\"");
                WriteFileLine(outFile, "        // \"sellratio_other\"   \"0.2\"");
                WriteFileLine(outFile, "    }");
            }
        }

        WriteFileLine(outFile, "%s", out);
    }
    CloseHandle(outFile);

    delete missingKeys;
    delete missingUse;
    delete lines;

    PrintToServer("[HWS] 买卖配置已同步 %d 个缺失武器", synced);
}

// ============================================================================
// >> 读取买卖配置
// ============================================================================
void ReadBuyConfig()
{
    if (g_hBuyConfig != INVALID_HANDLE)
    {
        CloseHandle(g_hBuyConfig);
        g_hBuyConfig = INVALID_HANDLE;
    }

    char cfgFile[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponBuyData.cfg");

    g_hBuyConfig = CreateKeyValues("HanWeaponSystem");
    if (!FileToKeyValues(g_hBuyConfig, cfgFile))
    {
        CloseHandle(g_hBuyConfig);
        g_hBuyConfig = INVALID_HANDLE;
        return;
    }

    KvRewind(g_hBuyConfig);
}

// ============================================================================
// >> 读取买卖配置根字段(总开关/模式/命令名)
// ============================================================================
bool GetBuyConfigRootValue(const char[] key, char[] value, int valueLen)
{
    if (g_hBuyConfig == INVALID_HANDLE)
        return false;

    KvRewind(g_hBuyConfig);
    KvGetString(g_hBuyConfig, key, value, valueLen, "");
    return (strlen(value) > 0);
}

// ============================================================================
// >> 通过 useclassname 读取买卖配置武器段值
// ============================================================================
bool GetBuyConfigValue(const char[] useclassname, const char[] key, char[] value, int valueLen)
{
    if (g_hBuyConfig == INVALID_HANDLE || strlen(useclassname) <= 0)
        return false;

    KvRewind(g_hBuyConfig);
    if (!KvGotoFirstSubKey(g_hBuyConfig))
        return false;

    do
    {
        char useClass[64];
        KvGetString(g_hBuyConfig, "useclassname", useClass, sizeof(useClass), "");

        if (StrContains(useClass, "weapon_") == 0)
            strcopy(useClass, sizeof(useClass), useClass[7]);

        char target[64];
        strcopy(target, sizeof(target), useclassname);
        if (StrContains(target, "weapon_") == 0)
            strcopy(target, sizeof(target), target[7]);

        if (StrEqual(target, useClass, false))
        {
            KvGetString(g_hBuyConfig, key, value, valueLen, "");
            return (strlen(value) > 0);
        }

    } while (KvGotoNextKey(g_hBuyConfig));

    return false;
}

int Buy_GetInt(const char[] useclassname, const char[] key, int def)
{
    char buf[16];
    if (GetBuyConfigValue(useclassname, key, buf, sizeof(buf)))
        return StringToInt(buf);
    return def;
}

float Buy_GetFloat(const char[] useclassname, const char[] key, float def)
{
    char buf[16];
    if (GetBuyConfigValue(useclassname, key, buf, sizeof(buf)))
        return StringToFloat(buf);
    return def;
}