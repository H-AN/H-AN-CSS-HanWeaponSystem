// ============================================================================
// >> 奔跑配置加载(独立配置文件 HanWeaponRunData.cfg)
// ============================================================================
// >> 原作者声明 / Original Author Credit
// 本模块的奔跑逻辑移植自开源社区插件 "Run_with_weapon"。
// This module's run logic is ported from the open-source plugin
// "Run_with_weapon".
//
// 原作者 / Original Author: cjsrk
// 版本 / Version: 1.1
//
// 感谢原作者的付出！原插件仅供学习参考，本插件已在此基础上大幅重构改写。
// Special thanks to the original author for the work! The original plugin is
// used for reference only; this plugin has been heavily reworked on top of it.
// ============================================================================

void RunConfig_OnPluginStart()
{
    InitRunConfigIfNeeded();
    SyncRunConfig();
    ReadRunConfig();
}

void RunConfig_OnMapStart()
{
    InitRunConfigIfNeeded();
    SyncRunConfig();
    ReadRunConfig();
}

// ============================================================================
// >> 创建奔跑配置
// ============================================================================
void InitRunConfigIfNeeded()
{
    char cfgDir[PLATFORM_MAX_PATH];
    char cfgFile[PLATFORM_MAX_PATH];

    BuildPath(Path_SM, cfgDir, sizeof(cfgDir), "configs/HanWeaponSystem");
    if (!DirExists(cfgDir))
    {
        CreateDirectory(cfgDir, 511);
    }

    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponRunData.cfg");
    if (!FileExists(cfgFile))
    {
        WriteDefaultRunConfig(cfgFile);
    }
}

void WriteDefaultRunConfig(const char[] path)
{
    Handle file = OpenFile(path, "w");
    if (file == INVALID_HANDLE)
        return;

    WriteFileLine(file, "// HanWeaponSystem 武器奔跑配置文件");
    WriteFileLine(file, "// 此文件与 HanWeaponData.cfg 自动同步：主配置新增武器时会自动追加一个默认键组");
    WriteFileLine(file, "// 默认全部关闭奔跑，需要奔跑的武器再填写参数");
    WriteFileLine(file, "// 字段说明：");
    WriteFileLine(file, "// useclassname   对应主配置的武器 useclassname(自动复制)");
    WriteFileLine(file, "// run            1=启用奔跑 0=关闭");
    WriteFileLine(file, "// runspeed       奔跑启动推力(每帧面向方向叠加的速度增量, 默认 50); 缺省/0=不加力");
    WriteFileLine(file, "// runmaxspeed    奔跑总体速度上限(默认走路约250, 500≈2倍速, 达到后保持不再加力)");
    WriteFileLine(file, "// runseq         奔跑动作1动画序列号(动作2自动=+1)");
    WriteFileLine(file, "// runtime        奔跑动作时长(帧, 30fps)");
    WriteFileLine(file, "// runin          可选 Idle->奔跑过渡动作序列号");
    WriteFileLine(file, "// runinlength    可选 Idle->奔跑过渡时长(帧)");
    WriteFileLine(file, "// runout         可选 奔跑->Idle过渡动作序列号");
    WriteFileLine(file, "// runoutlength   可选 奔跑->Idle过渡时长(帧)");
    WriteFileLine(file, "// runshake       抖动幅度值; 0/缺省=不抖动, >0 按该值抖动");
    WriteFileLine(file, "// runseq_silencer/runtime_silencer/runin_silencer/runinlength_silencer/runout_silencer/runoutlength_silencer: 消音器变体(仅变体武器 m4a1/usp); 装上消音器时生效, 不填回落普通字段");
    WriteFileLine(file, "\"HanWeaponSystem\"");
    WriteFileLine(file, "{");
    WriteFileLine(file, "}");

    CloseHandle(file);
}

// ============================================================================
// >> 同步追加缺失武器键组(只追加，绝不重写已有内容)
// ============================================================================
void SyncRunConfig()
{
    if (g_hWeaponConfig == INVALID_HANDLE)
        return;

    char cfgFile[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponRunData.cfg");

    if (!FileExists(cfgFile))
    {
        WriteDefaultRunConfig(cfgFile);
    }

    // 收集主配置中的武器键名 + useclassname + classname(变体判定用)
    char weaponKeys[MAX_WEAPON][64];
    char weaponUse[MAX_WEAPON][64];
    char weaponOrig[MAX_WEAPON][64];
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
            char origclass[64];
            KvGetString(g_hWeaponConfig, "classname", origclass, sizeof(origclass), "");

            if (strlen(name) > 0 && total < MAX_WEAPON)
            {
                strcopy(weaponKeys[total], sizeof(weaponKeys[]), name);
                strcopy(weaponUse[total], sizeof(weaponUse[]), useclass);
                strcopy(weaponOrig[total], sizeof(weaponOrig[]), origclass);
                total++;
            }
        } while (KvGotoNextKey(g_hWeaponConfig));
    }
    KvRewind(g_hWeaponConfig);

    // 找出奔跑配置中缺失的键
    Handle runKv = CreateKeyValues("HanWeaponSystem");
    if (!FileToKeyValues(runKv, cfgFile))
    {
        CloseHandle(runKv);
        return;
    }

    ArrayList missingKeys = new ArrayList(64);
    ArrayList missingUse = new ArrayList(64);
    ArrayList missingOrig = new ArrayList(64);

    for (int i = 0; i < total; i++)
    {
        KvRewind(runKv);
        bool found = false;
        if (KvGotoFirstSubKey(runKv))
        {
            do
            {
                char sec[64];
                KvGetSectionName(runKv, sec, sizeof(sec));
                if (StrEqual(sec, weaponKeys[i], false))
                {
                    found = true;
                    break;
                }
            } while (KvGotoNextKey(runKv));
        }

        if (!found)
        {
            missingKeys.PushString(weaponKeys[i]);
            missingUse.PushString(weaponUse[i]);
            missingOrig.PushString(weaponOrig[i]);
        }
    }
    CloseHandle(runKv);

    int synced = missingKeys.Length;
    if (synced == 0)
    {
        delete missingKeys;
        delete missingUse;
        delete missingOrig;
        return;
    }

    // 逐行读取现有文件
    ArrayList lines = new ArrayList(512);
    Handle inFile = OpenFile(cfgFile, "r");
    if (inFile == INVALID_HANDLE)
    {
        delete missingKeys;
        delete missingUse;
        delete missingOrig;
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
        delete missingOrig;
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
                char key[64], use[64], orig[64];
                missingKeys.GetString(m, key, sizeof(key));
                missingUse.GetString(m, use, sizeof(use));
                missingOrig.GetString(m, orig, sizeof(orig));

                WriteFileLine(outFile, "");
                WriteFileLine(outFile, "    \"%s\"", key);
                WriteFileLine(outFile, "    {");
                WriteFileLine(outFile, "        \"useclassname\"    \"%s\"", use);
                WriteFileLine(outFile, "        \"run\"             \"0\"");
                WriteFileLine(outFile, "        // 奔跑参数(仅 run 1 时生效):");
                WriteFileLine(outFile, "        // \"runspeed\"        \"50.0\"");
                WriteFileLine(outFile, "        // \"runmaxspeed\"     \"500.0\"");
                WriteFileLine(outFile, "        // \"runseq\"          \"5\"");
                WriteFileLine(outFile, "        // \"runtime\"         \"15\"");
                WriteFileLine(outFile, "        // \"runin\"           \"4\"");
                WriteFileLine(outFile, "        // \"runinlength\"     \"5\"");
                WriteFileLine(outFile, "        // \"runout\"          \"6\"");
                WriteFileLine(outFile, "        // \"runoutlength\"    \"5\"");
                WriteFileLine(outFile, "        // \"runshake\"        \"0.6\"");

                if (IsVariantWeaponClass(orig))
                {
                    WriteFileLine(outFile, "        // 消音器变体(仅变体武器 m4a1/usp, 装上消音器时生效; 不填=回落普通字段):");
                    WriteFileLine(outFile, "        // \"runseq_silencer\"       \"7\"");
                    WriteFileLine(outFile, "        // \"runtime_silencer\"      \"15\"");
                    WriteFileLine(outFile, "        // \"runin_silencer\"        \"6\"");
                    WriteFileLine(outFile, "        // \"runinlength_silencer\"  \"5\"");
                    WriteFileLine(outFile, "        // \"runout_silencer\"       \"8\"");
                    WriteFileLine(outFile, "        // \"runoutlength_silencer\" \"5\"");
                }

                WriteFileLine(outFile, "    }");
            }
        }

        WriteFileLine(outFile, "%s", out);
    }
    CloseHandle(outFile);

    delete missingKeys;
    delete missingUse;
    delete missingOrig;
    delete lines;

    PrintToServer("[HWS] 奔跑配置已同步 %d 个缺失武器", synced);
}

// ============================================================================
// >> 读取奔跑配置
// ============================================================================
void ReadRunConfig()
{
    if (g_hRunConfig != INVALID_HANDLE)
    {
        CloseHandle(g_hRunConfig);
        g_hRunConfig = INVALID_HANDLE;
    }

    char cfgFile[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponRunData.cfg");

    g_hRunConfig = CreateKeyValues("HanWeaponSystem");
    if (!FileToKeyValues(g_hRunConfig, cfgFile))
    {
        CloseHandle(g_hRunConfig);
        g_hRunConfig = INVALID_HANDLE;
        return;
    }

    KvRewind(g_hRunConfig);
}

// ============================================================================
// >> 通过奔跑配置获取武器值
// ============================================================================
bool GetRunConfigValue(int weaponEnt, const char[] key, char[] value, int valueLen)
{
    if (weaponEnt <= 0 || !IsValidEntity(weaponEnt) || g_hRunConfig == INVALID_HANDLE)
        return false;

    char className[64];
    GetEntityClassname(weaponEnt, className, sizeof(className));

    if (StrContains(className, "weapon_") == 0)
        strcopy(className, sizeof(className), className[7]);

    KvRewind(g_hRunConfig);
    if (!KvGotoFirstSubKey(g_hRunConfig))
        return false;

    do
    {
        char useClass[64];
        KvGetString(g_hRunConfig, "useclassname", useClass, sizeof(useClass), "");

        if (StrContains(useClass, "weapon_") == 0)
            strcopy(useClass, sizeof(useClass), useClass[7]);

        if (StrEqual(className, useClass, false))
        {
            KvGetString(g_hRunConfig, key, value, valueLen, "");
            return (strlen(value) > 0);
        }

    } while (KvGotoNextKey(g_hRunConfig));

    return false;
}

int Run_GetInt(int weapon, const char[] key, int def)
{
    char buf[16];
    if (GetRunConfigValue(weapon, key, buf, sizeof(buf)))
        return StringToInt(buf);
    return def;
}

float Run_GetFloat(int weapon, const char[] key, float def)
{
    char buf[16];
    if (GetRunConfigValue(weapon, key, buf, sizeof(buf)))
        return StringToFloat(buf);
    return def;
}

// ============================================================================
// >> 变体感知读取奔跑配置整型值(消音器装上且变体字段填了 → 变体; 否则回落普通字段)
// ============================================================================
int Run_GetIntVariant(int weapon, const char[] key, int def)
{
    char variantKey[64];
    Format(variantKey, sizeof(variantKey), "%s_silencer", key);

    int variantVal = Run_GetInt(weapon, variantKey, -1);
    if (IsWeaponSilencerOn(weapon) && variantVal >= 0)
        return variantVal;

    return Run_GetInt(weapon, key, def);
}