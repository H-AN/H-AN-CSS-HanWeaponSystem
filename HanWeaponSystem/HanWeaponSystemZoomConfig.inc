// ============================================================================
// >> 机瞄配置加载(独立配置文件 HanWeaponZoomData.cfg)
// ============================================================================

void ZoomConfig_OnPluginStart()
{
    InitZoomConfigIfNeeded();
    SyncZoomConfig();
    ReadZoomConfig();
    PrecacheZoomModels();
}

void ZoomConfig_OnMapStart()
{
    InitZoomConfigIfNeeded();
    SyncZoomConfig();
    ReadZoomConfig();
    PrecacheZoomModels();
}

// ============================================================================
// >> 创建机瞄配置
// ============================================================================
void InitZoomConfigIfNeeded()
{
    char cfgDir[PLATFORM_MAX_PATH];
    char cfgFile[PLATFORM_MAX_PATH];

    BuildPath(Path_SM, cfgDir, sizeof(cfgDir), "configs/HanWeaponSystem");
    if (!DirExists(cfgDir))
    {
        CreateDirectory(cfgDir, 511);
    }

    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponZoomData.cfg");
    if (!FileExists(cfgFile))
    {
        WriteDefaultZoomConfig(cfgFile);
    }
}

void WriteDefaultZoomConfig(const char[] path)
{
    Handle file = OpenFile(path, "w");
    if (file == INVALID_HANDLE)
        return;

    WriteFileLine(file, "// HanWeaponSystem 机瞄/侧瞄配置文件");
    WriteFileLine(file, "// 此文件与 HanWeaponData.cfg 自动同步：主配置新增武器时会自动追加一个默认键组");
    WriteFileLine(file, "// 普通武器保持 zoom 0 即可，右键无任何效果；做了机瞄模型的武器再开启并填写参数");
    WriteFileLine(file, "// 字段说明：");
    WriteFileLine(file, "// useclassname   对应主配置的武器 useclassname(自动复制)");
    WriteFileLine(file, "// zoom           1=启用机瞄 0=关闭");
    WriteFileLine(file, "// zoommodel      机瞄时 VM 显示的模型(完整路径)");
    WriteFileLine(file, "// zoomanimmove   开镜衔接动画序列号(-1=无衔接直接换模型)");
    WriteFileLine(file, "// zoomanimtime   衔接动画时长(秒)");
    WriteFileLine(file, "// zoomanimfire   1=机瞄时不强制同步开火动画");
    WriteFileLine(file, "// zoomfov        镜内放大 FOV(<90 启用镜内放大, 需机瞄模型带 _rt_camera 镜片材质)");
    WriteFileLine(file, "// zoomaccuracy   机瞄射击精度 m_fAccuracyPenalty(0.015为原版, -1=不修改)");
    WriteFileLine(file, "// zoomspeed      机瞄时移动速度系数(1.0=不减速)");
    WriteFileLine(file, "// zoomcrosshair  1=机瞄时隐藏准心 0=不隐藏");
    WriteFileLine(file, "// zoompunch      机瞄时屏幕震动系数(0.5=减半, 1.0=不变)");
    WriteFileLine(file, "// zoomskinadd    不开镜内放大时的镜片皮肤索引增量(防镜片黑屏, 0=不使用)");
    WriteFileLine(file, "//");
    WriteFileLine(file, "// 侧瞄参数(与机瞄完全平行的独立一套, 全部 sideaim 前缀, 中键 IN_ATTACK3 触发):");
    WriteFileLine(file, "// sideaim         1=启用侧瞄 0=关闭(开启但未填模型也无效)");
    WriteFileLine(file, "// sideaimmodel    侧瞄时 VM 显示的模型(完整路径)");
    WriteFileLine(file, "// sideaimanimmove 进入侧瞄衔接动画序列号(-1=无衔接直接换模型)");
    WriteFileLine(file, "// sideaimanimtime 衔接动画时长(秒)");
    WriteFileLine(file, "// sideaimanimfire 1=侧瞄时不强制同步开火动画");
    WriteFileLine(file, "// sideaimfov      侧瞄镜内放大 FOV(<90 启用)");
    WriteFileLine(file, "// sideaimaccuracy 侧瞄射击精度");
    WriteFileLine(file, "// sideaimspeed    侧瞄时移动速度系数");
    WriteFileLine(file, "// sideaimcrosshair 1=侧瞄时隐藏准心 0=不隐藏");
    WriteFileLine(file, "// sideaimpunch    侧瞄时屏幕震动系数");
    WriteFileLine(file, "// sideaimskinadd  侧瞄镜片皮肤索引增量");
    WriteFileLine(file, "\"HanWeaponSystem\"");
    WriteFileLine(file, "{");
    WriteFileLine(file, "}");

    CloseHandle(file);
}

// ============================================================================
// >> 同步追加缺失武器键组(只追加，绝不重写已有内容)
// ============================================================================
void SyncZoomConfig()
{
    if (g_hWeaponConfig == INVALID_HANDLE)
        return;

    char cfgFile[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponZoomData.cfg");

    if (!FileExists(cfgFile))
    {
        WriteDefaultZoomConfig(cfgFile);
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

    // 找出机瞄配置中缺失的键
    Handle zoomKv = CreateKeyValues("HanWeaponSystem");
    if (!FileToKeyValues(zoomKv, cfgFile))
    {
        CloseHandle(zoomKv);
        return;
    }

    ArrayList missingKeys = new ArrayList(64);
    ArrayList missingUse = new ArrayList(64);

    for (int i = 0; i < total; i++)
    {
        KvRewind(zoomKv);
        bool found = false;
        if (KvGotoFirstSubKey(zoomKv))
        {
            do
            {
                char sec[64];
                KvGetSectionName(zoomKv, sec, sizeof(sec));
                if (StrEqual(sec, weaponKeys[i], false))
                {
                    found = true;
                    break;
                }
            } while (KvGotoNextKey(zoomKv));
        }

        if (!found)
        {
            missingKeys.PushString(weaponKeys[i]);
            missingUse.PushString(weaponUse[i]);
        }
    }
    CloseHandle(zoomKv);

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
                WriteFileLine(outFile, "        \"zoom\"            \"0\"");
                WriteFileLine(outFile, "        // 机瞄参数(仅 zoom 1 时生效):");
                WriteFileLine(outFile, "        // \"zoommodel\"       \"models/v_custom/v_ak47_aim.mdl\"");
                WriteFileLine(outFile, "        // \"zoomanimmove\"    \"6\"");
                WriteFileLine(outFile, "        // \"zoomanimtime\"    \"0.4\"");
                WriteFileLine(outFile, "        // \"zoomanimfire\"    \"0\"");
                WriteFileLine(outFile, "        // \"zoomfov\"         \"70\"");
                WriteFileLine(outFile, "        // \"zoomaccuracy\"    \"0.015\"");
                WriteFileLine(outFile, "        // \"zoomspeed\"       \"0.66\"");
                WriteFileLine(outFile, "        // \"zoomcrosshair\"   \"1\"");
                WriteFileLine(outFile, "        // \"zoompunch\"       \"1.0\"");
                WriteFileLine(outFile, "        // \"zoomskinadd\"     \"0\"");
                WriteFileLine(outFile, "        \"sideaim\"         \"0\"");
                WriteFileLine(outFile, "        // 侧瞄参数(仅 sideaim 1 时生效, 中键触发):");
                WriteFileLine(outFile, "        // \"sideaimmodel\"    \"models/v_custom/v_ak47_side.mdl\"");
                WriteFileLine(outFile, "        // \"sideaimanimmove\" \"8\"");
                WriteFileLine(outFile, "        // \"sideaimanimtime\" \"0.3\"");
                WriteFileLine(outFile, "        // \"sideaimanimfire\" \"0\"");
                WriteFileLine(outFile, "        // \"sideaimfov\"      \"80\"");
                WriteFileLine(outFile, "        // \"sideaimaccuracy\" \"0.015\"");
                WriteFileLine(outFile, "        // \"sideaimspeed\"    \"0.8\"");
                WriteFileLine(outFile, "        // \"sideaimcrosshair\" \"1\"");
                WriteFileLine(outFile, "        // \"sideaimpunch\"    \"1.0\"");
                WriteFileLine(outFile, "        // \"sideaimskinadd\"  \"0\"");
                WriteFileLine(outFile, "    }");
            }
        }

        WriteFileLine(outFile, "%s", out);
    }
    CloseHandle(outFile);

    delete missingKeys;
    delete missingUse;
    delete lines;

    PrintToServer("[HWS] 机瞄配置已同步 %d 个缺失武器", synced);
}

void TrimTrailingNewline(char[] str)
{
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r'))
    {
        str[--len] = '\0';
    }
}

// ============================================================================
// >> 读取机瞄配置
// ============================================================================
void ReadZoomConfig()
{
    if (g_hZoomConfig != INVALID_HANDLE)
    {
        CloseHandle(g_hZoomConfig);
        g_hZoomConfig = INVALID_HANDLE;
    }

    char cfgFile[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponZoomData.cfg");

    g_hZoomConfig = CreateKeyValues("HanWeaponSystem");
    if (!FileToKeyValues(g_hZoomConfig, cfgFile))
    {
        CloseHandle(g_hZoomConfig);
        g_hZoomConfig = INVALID_HANDLE;
        return;
    }

    KvRewind(g_hZoomConfig);
}

// ============================================================================
// >> 预缓存机瞄模型
// ============================================================================
void PrecacheZoomModels()
{
    if (g_hZoomConfig == INVALID_HANDLE)
        return;

    KvRewind(g_hZoomConfig);
    if (!KvGotoFirstSubKey(g_hZoomConfig))
        return;

    do
    {
        char model[PLATFORM_MAX_PATH];
        KvGetString(g_hZoomConfig, "zoommodel", model, sizeof(model), "");
        if (strlen(model) > 0)
        {
            if (PrecacheModel(model))
            {
                AddFileToDownloadsTable(model);
            }
        }

        KvGetString(g_hZoomConfig, "sideaimmodel", model, sizeof(model), "");
        if (strlen(model) > 0)
        {
            if (PrecacheModel(model))
            {
                AddFileToDownloadsTable(model);
            }
        }
    } while (KvGotoNextKey(g_hZoomConfig));

    KvRewind(g_hZoomConfig);
}

// ============================================================================
// >> 通过机瞄配置获取武器值
// ============================================================================
bool GetZoomConfigValue(int weaponEnt, const char[] key, char[] value, int valueLen)
{
    if (weaponEnt <= 0 || !IsValidEntity(weaponEnt) || g_hZoomConfig == INVALID_HANDLE)
        return false;

    char className[64];
    GetEntityClassname(weaponEnt, className, sizeof(className));

    if (StrContains(className, "weapon_") == 0)
        strcopy(className, sizeof(className), className[7]);

    KvRewind(g_hZoomConfig);
    if (!KvGotoFirstSubKey(g_hZoomConfig))
        return false;

    do
    {
        char useClass[64];
        KvGetString(g_hZoomConfig, "useclassname", useClass, sizeof(useClass), "");

        if (StrContains(useClass, "weapon_") == 0)
            strcopy(useClass, sizeof(useClass), useClass[7]);

        if (StrEqual(className, useClass, false))
        {
            KvGetString(g_hZoomConfig, key, value, valueLen, "");
            return (strlen(value) > 0);
        }

    } while (KvGotoNextKey(g_hZoomConfig));

    return false;
}

int Zoom_GetInt(int weapon, const char[] key, int def)
{
    char buf[16];
    if (GetZoomConfigValue(weapon, key, buf, sizeof(buf)))
        return StringToInt(buf);
    return def;
}

float Zoom_GetFloat(int weapon, const char[] key, float def)
{
    char buf[16];
    if (GetZoomConfigValue(weapon, key, buf, sizeof(buf)))
        return StringToFloat(buf);
    return def;
}