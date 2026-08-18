// ============================================================================
// >> 检视配置加载(独立配置文件 HanWeaponInspectData.cfg)
// ============================================================================
// >> 原作者声明 / Original Author Credit
// 本模块的检视逻辑移植自开源社区插件 "Run LookAt01 Action"。
// This module's inspect logic is ported from the open-source plugin
// "Run LookAt01 Action".
//
// 原作者 / Original Author: cjsrk
// 版本 / Version: 1.0
//
// 感谢原作者的付出！原插件仅供学习参考，本插件已在此基础上大幅重构改写。
// Special thanks to the original author for the work! The original plugin is
// used for reference only; this plugin has been heavily reworked on top of it.
// ============================================================================

void InspectConfig_OnPluginStart()
{
    InitInspectConfigIfNeeded();
    SyncInspectConfig();
    ReadInspectConfig();
}

void InspectConfig_OnMapStart()
{
    InitInspectConfigIfNeeded();
    SyncInspectConfig();
    ReadInspectConfig();
}

// ============================================================================
// >> 创建检视配置
// ============================================================================
void InitInspectConfigIfNeeded()
{
    char cfgDir[PLATFORM_MAX_PATH];
    char cfgFile[PLATFORM_MAX_PATH];

    BuildPath(Path_SM, cfgDir, sizeof(cfgDir), "configs/HanWeaponSystem");
    if (!DirExists(cfgDir))
    {
        CreateDirectory(cfgDir, 511);
    }

    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponInspectData.cfg");
    if (!FileExists(cfgFile))
    {
        WriteDefaultInspectConfig(cfgFile);
    }
}

void WriteDefaultInspectConfig(const char[] path)
{
    Handle file = OpenFile(path, "w");
    if (file == INVALID_HANDLE)
        return;

    WriteFileLine(file, "// HanWeaponSystem 武器检视配置文件");
    WriteFileLine(file, "// 此文件与 HanWeaponData.cfg 自动同步：主配置新增武器时会自动追加一个默认键组");
    WriteFileLine(file, "// 默认全部关闭检视，需要检视的武器再填写参数");
    WriteFileLine(file, "// 字段说明：");
    WriteFileLine(file, "// useclassname       对应主配置的武器 useclassname(自动复制)");
    WriteFileLine(file, "// inspect            1=启用检视 0=关闭");
    WriteFileLine(file, "// inspectseq         检视动作动画序列号");
    WriteFileLine(file, "// inspecttime        检视动作时长(帧, 30fps, 与原插件兼容, 如 230=约7.7秒)");
    WriteFileLine(file, "// inspectrepeat      0=检视一次播完为止 1=允许反复按F从动画开头重播(好玩向)");
    WriteFileLine(file, "// inspectflashlight  0=检视时手电筒正常发光 1=检视时屏蔽手电筒光");
    WriteFileLine(file, "// inspectsound       检视音效(可缺省不填=不播放); 多个用英文逗号隔开随机播其中一个; 只自己听得见");
    WriteFileLine(file, "\"HanWeaponSystem\"");
    WriteFileLine(file, "{");
    WriteFileLine(file, "}");

    CloseHandle(file);
}

// ============================================================================
// >> 同步追加缺失武器键组(只追加，绝不重写已有内容)
// ============================================================================
void SyncInspectConfig()
{
    if (g_hWeaponConfig == INVALID_HANDLE)
        return;

    char cfgFile[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponInspectData.cfg");

    if (!FileExists(cfgFile))
    {
        WriteDefaultInspectConfig(cfgFile);
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

    // 找出检视配置中缺失的键
    Handle inspectKv = CreateKeyValues("HanWeaponSystem");
    if (!FileToKeyValues(inspectKv, cfgFile))
    {
        CloseHandle(inspectKv);
        return;
    }

    ArrayList missingKeys = new ArrayList(64);
    ArrayList missingUse = new ArrayList(64);

    for (int i = 0; i < total; i++)
    {
        KvRewind(inspectKv);
        bool found = false;
        if (KvGotoFirstSubKey(inspectKv))
        {
            do
            {
                char sec[64];
                KvGetSectionName(inspectKv, sec, sizeof(sec));
                if (StrEqual(sec, weaponKeys[i], false))
                {
                    found = true;
                    break;
                }
            } while (KvGotoNextKey(inspectKv));
        }

        if (!found)
        {
            missingKeys.PushString(weaponKeys[i]);
            missingUse.PushString(weaponUse[i]);
        }
    }
    CloseHandle(inspectKv);

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
                WriteFileLine(outFile, "        \"inspect\"         \"0\"");
                WriteFileLine(outFile, "        // 检视参数(仅 inspect 1 时生效):");
                WriteFileLine(outFile, "        // \"inspectseq\"        \"6\"");
                WriteFileLine(outFile, "        // \"inspecttime\"       \"230\"");
                WriteFileLine(outFile, "        // \"inspectrepeat\"     \"0\"");
                WriteFileLine(outFile, "        // \"inspectflashlight\" \"1\"");
                WriteFileLine(outFile, "        // \"inspectsound\"      \"weapons/inspect/inspect1.wav,weapons/inspect/inspect2.wav\"");
                WriteFileLine(outFile, "    }");
            }
        }

        WriteFileLine(outFile, "%s", out);
    }
    CloseHandle(outFile);

    delete missingKeys;
    delete missingUse;
    delete lines;

    PrintToServer("[HWS] 检视配置已同步 %d 个缺失武器", synced);
}

// ============================================================================
// >> 读取检视配置
// ============================================================================
void ReadInspectConfig()
{
    if (g_hInspectConfig != INVALID_HANDLE)
    {
        CloseHandle(g_hInspectConfig);
        g_hInspectConfig = INVALID_HANDLE;
    }

    char cfgFile[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponInspectData.cfg");

    g_hInspectConfig = CreateKeyValues("HanWeaponSystem");
    if (!FileToKeyValues(g_hInspectConfig, cfgFile))
    {
        CloseHandle(g_hInspectConfig);
        g_hInspectConfig = INVALID_HANDLE;
        return;
    }

    KvRewind(g_hInspectConfig);

    PrecacheInspectSounds();
}

// ============================================================================
// >> 预缓存所有检视音效(逗号分隔多路径逐一预缓存; 下载统一由 Download.cfg 管理)
// ============================================================================
void PrecacheInspectSounds()
{
    if (g_hInspectConfig == INVALID_HANDLE)
        return;

    KvRewind(g_hInspectConfig);
    if (!KvGotoFirstSubKey(g_hInspectConfig))
        return;

    do
    {
        char soundStr[512];
        KvGetString(g_hInspectConfig, "inspectsound", soundStr, sizeof(soundStr), "");
        if (strlen(soundStr) == 0)
            continue;

        char parts[8][PLATFORM_MAX_PATH];
        int count = ExplodeString(soundStr, ",", parts, sizeof(parts), sizeof(parts[]));

        for (int i = 0; i < count; i++)
        {
            TrimString(parts[i]);
            if (strlen(parts[i]) == 0)
                continue;

            NormalizePath(parts[i]);
            PrecacheSound(parts[i]);
        }

    } while (KvGotoNextKey(g_hInspectConfig));

    KvRewind(g_hInspectConfig);
}

// ============================================================================
// >> 通过检视配置获取武器值
// ============================================================================
bool GetInspectConfigValue(int weaponEnt, const char[] key, char[] value, int valueLen)
{
    if (weaponEnt <= 0 || !IsValidEntity(weaponEnt) || g_hInspectConfig == INVALID_HANDLE)
        return false;

    char className[64];
    GetEntityClassname(weaponEnt, className, sizeof(className));

    if (StrContains(className, "weapon_") == 0)
        strcopy(className, sizeof(className), className[7]);

    KvRewind(g_hInspectConfig);
    if (!KvGotoFirstSubKey(g_hInspectConfig))
        return false;

    do
    {
        char useClass[64];
        KvGetString(g_hInspectConfig, "useclassname", useClass, sizeof(useClass), "");

        if (StrContains(useClass, "weapon_") == 0)
            strcopy(useClass, sizeof(useClass), useClass[7]);

        if (StrEqual(className, useClass, false))
        {
            KvGetString(g_hInspectConfig, key, value, valueLen, "");
            NormalizePath(value);
            return (strlen(value) > 0);
        }

    } while (KvGotoNextKey(g_hInspectConfig));

    return false;
}

int Inspect_GetInt(int weapon, const char[] key, int def)
{
    char buf[16];
    if (GetInspectConfigValue(weapon, key, buf, sizeof(buf)))
        return StringToInt(buf);
    return def;
}