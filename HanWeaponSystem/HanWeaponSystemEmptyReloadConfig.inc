// ============================================================================
// >> 空仓/战术换弹配置加载(独立配置文件 HanWeaponEmptyReloadData.cfg)
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

void EmptyReloadConfig_OnPluginStart()
{
    InitEmptyReloadConfigIfNeeded();
    SyncEmptyReloadConfig();
    ReadEmptyReloadConfig();
}

void EmptyReloadConfig_OnMapStart()
{
    InitEmptyReloadConfigIfNeeded();
    SyncEmptyReloadConfig();
    ReadEmptyReloadConfig();
}

// ============================================================================
// >> 创建空仓/战术换弹配置
// ============================================================================
void InitEmptyReloadConfigIfNeeded()
{
    char cfgDir[PLATFORM_MAX_PATH];
    char cfgFile[PLATFORM_MAX_PATH];

    BuildPath(Path_SM, cfgDir, sizeof(cfgDir), "configs/HanWeaponSystem");
    if (!DirExists(cfgDir))
    {
        CreateDirectory(cfgDir, 511);
    }

    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponEmptyReloadData.cfg");
    if (!FileExists(cfgFile))
    {
        WriteDefaultEmptyReloadConfig(cfgFile);
    }
}

void WriteDefaultEmptyReloadConfig(const char[] path)
{
    Handle file = OpenFile(path, "w");
    if (file == INVALID_HANDLE)
        return;

    WriteFileLine(file, "// HanWeaponSystem 武器空仓/战术换弹配置文件");
    WriteFileLine(file, "// 此文件与 HanWeaponData.cfg 自动同步：主配置新增武器时会自动追加一个默认键组");
    WriteFileLine(file, "// 默认全部关闭换弹动画，需要换弹动画的武器再填写参数");
    WriteFileLine(file, "// 字段说明：");
    WriteFileLine(file, "// useclassname     对应主配置的武器 useclassname(自动复制)");
    WriteFileLine(file, "// emptyreload      1=启用空仓换弹 0=关闭");
    WriteFileLine(file, "// emptyreloadseq   空仓换弹动画序列号(弹匣打空时)");
    WriteFileLine(file, "// emptyreloadseq2  空仓换弹·消音器变体序列号(无消音器武器不填)");
    WriteFileLine(file, "// emptyreloadtime  空仓换弹动画时长(帧, 按fps换算秒)");
    WriteFileLine(file, "// reloadfps        换弹动画帧率(默认30, 动画帧率不同的武器按需改24等)");
    WriteFileLine(file, "// tacticalreload   1=启用战术换弹 0=关闭");
    WriteFileLine(file, "// reloadseq        战术换弹动画序列号(弹匣还有子弹时)");
    WriteFileLine(file, "// reloadseq2       战术换弹·消音器变体序列号(无消音器武器不填)");
    WriteFileLine(file, "// reloadtime       战术换弹动画时长(帧, 按fps换算秒)");
    WriteFileLine(file, "\"HanWeaponSystem\"");
    WriteFileLine(file, "{");
    WriteFileLine(file, "}");

    CloseHandle(file);
}

// ============================================================================
// >> 同步追加缺失武器键组(只追加，绝不重写已有内容)
// ============================================================================
void SyncEmptyReloadConfig()
{
    if (g_hWeaponConfig == INVALID_HANDLE)
        return;

    char cfgFile[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponEmptyReloadData.cfg");

    if (!FileExists(cfgFile))
    {
        WriteDefaultEmptyReloadConfig(cfgFile);
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

    // 找出空仓换弹配置中缺失的键
    Handle erKv = CreateKeyValues("HanWeaponSystem");
    if (!FileToKeyValues(erKv, cfgFile))
    {
        CloseHandle(erKv);
        return;
    }

    ArrayList missingKeys = new ArrayList(64);
    ArrayList missingUse = new ArrayList(64);

    for (int i = 0; i < total; i++)
    {
        KvRewind(erKv);
        bool found = false;
        if (KvGotoFirstSubKey(erKv))
        {
            do
            {
                char sec[64];
                KvGetSectionName(erKv, sec, sizeof(sec));
                if (StrEqual(sec, weaponKeys[i], false))
                {
                    found = true;
                    break;
                }
            } while (KvGotoNextKey(erKv));
        }

        if (!found)
        {
            missingKeys.PushString(weaponKeys[i]);
            missingUse.PushString(weaponUse[i]);
        }
    }
    CloseHandle(erKv);

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
                WriteFileLine(outFile, "        \"emptyreload\"     \"0\"");
                WriteFileLine(outFile, "        // 空仓换弹参数(仅 emptyreload 1 时生效):");
                WriteFileLine(outFile, "        // \"emptyreloadseq\"   \"5\"");
                WriteFileLine(outFile, "        // \"emptyreloadseq2\"  \"6\"  (消音器变体)");
                WriteFileLine(outFile, "        // \"emptyreloadtime\"  \"280\"  (动画时长, 帧)");
                WriteFileLine(outFile, "        // \"reloadfps\"        \"30\"  (动画帧率, 按需改24)");
                WriteFileLine(outFile, "        \"tacticalreload\"   \"0\"");
                WriteFileLine(outFile, "        // 战术换弹参数(仅 tacticalreload 1 时生效):");
                WriteFileLine(outFile, "        // \"reloadseq\"        \"7\"");
                WriteFileLine(outFile, "        // \"reloadseq2\"       \"8\"  (消音器变体)");
                WriteFileLine(outFile, "        // \"reloadtime\"       \"240\"  (动画时长, 帧)");
                WriteFileLine(outFile, "        // \"reloadfps\"        \"30\"  (动画帧率, 按需改24)");
                WriteFileLine(outFile, "    }");
            }
        }

        WriteFileLine(outFile, "%s", out);
    }
    CloseHandle(outFile);

    delete missingKeys;
    delete missingUse;
    delete lines;

    PrintToServer("[HWS] 空仓/战术换弹配置已同步 %d 个缺失武器", synced);
}

// ============================================================================
// >> 读取空仓/战术换弹配置
// ============================================================================
void ReadEmptyReloadConfig()
{
    if (g_hEmptyReloadConfig != INVALID_HANDLE)
    {
        CloseHandle(g_hEmptyReloadConfig);
        g_hEmptyReloadConfig = INVALID_HANDLE;
    }

    char cfgFile[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponEmptyReloadData.cfg");

    g_hEmptyReloadConfig = CreateKeyValues("HanWeaponSystem");
    if (!FileToKeyValues(g_hEmptyReloadConfig, cfgFile))
    {
        CloseHandle(g_hEmptyReloadConfig);
        g_hEmptyReloadConfig = INVALID_HANDLE;
        return;
    }

    KvRewind(g_hEmptyReloadConfig);
}

// ============================================================================
// >> 通过空仓换弹配置获取武器值
// ============================================================================
bool GetEmptyReloadConfigValue(int weaponEnt, const char[] key, char[] value, int valueLen)
{
    if (weaponEnt <= 0 || !IsValidEntity(weaponEnt) || g_hEmptyReloadConfig == INVALID_HANDLE)
        return false;

    char className[64];
    GetEntityClassname(weaponEnt, className, sizeof(className));

    if (StrContains(className, "weapon_") == 0)
        strcopy(className, sizeof(className), className[7]);

    KvRewind(g_hEmptyReloadConfig);
    if (!KvGotoFirstSubKey(g_hEmptyReloadConfig))
        return false;

    do
    {
        char useClass[64];
        KvGetString(g_hEmptyReloadConfig, "useclassname", useClass, sizeof(useClass), "");

        if (StrContains(useClass, "weapon_") == 0)
            strcopy(useClass, sizeof(useClass), useClass[7]);

        if (StrEqual(className, useClass, false))
        {
            KvGetString(g_hEmptyReloadConfig, key, value, valueLen, "");
            return (strlen(value) > 0);
        }

    } while (KvGotoNextKey(g_hEmptyReloadConfig));

    return false;
}

int EmptyReload_GetInt(int weapon, const char[] key, int def)
{
    char buf[16];
    if (GetEmptyReloadConfigValue(weapon, key, buf, sizeof(buf)))
        return StringToInt(buf);
    return def;
}