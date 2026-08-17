// ============================================================================
// >> 主武器配置加载
// ============================================================================

void Config_OnPluginStart()
{
    InitWeaponConfigIfNeeded();
    LoadWeaponCommands();
    ReadWeaponConfig();
}

void Config_OnMapStart()
{
    InitWeaponConfigIfNeeded();
    ReadWeaponConfig();
}

// ============================================================================
// >> 创建配置文件
// ============================================================================
void InitWeaponConfigIfNeeded()
{
    if (g_bWeaponConfigReady)
        return;

    char cfgDir[PLATFORM_MAX_PATH];
    char cfgFile[PLATFORM_MAX_PATH];

    BuildPath(Path_SM, cfgDir, sizeof(cfgDir), "configs/HanWeaponSystem");
    if (!DirExists(cfgDir))
    {
        CreateDirectory(cfgDir, 511);
    }

    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponData.cfg");
    if (!FileExists(cfgFile))
    {
        WriteDefaultWeaponConfig(cfgFile);
    }

    g_bWeaponConfigReady = true;
}

// ============================================================================
// >> 创建生成默认配置文件
// ============================================================================
void WriteDefaultWeaponConfig(const char[] path)
{
    Handle file = OpenFile(path, "w");
    if (file == INVALID_HANDLE)
        return;

    WriteFileLine(file, "// HanWeaponSystem 武器配置文件");
    WriteFileLine(file, "// 每个武器一个子节点，可设置字段如下：");
    WriteFileLine(file, "// command:       执行命令（如 sm_ 开头）");
    WriteFileLine(file, "// classname:     给武器的原始 classname");
    WriteFileLine(file, "// useclassname:  玩家使用时赋予的 classname");
    WriteFileLine(file, "// damage:        可以是 +40 或 x1.5（加法或乘法）");
    WriteFileLine(file, "// recoil:        后坐力可选1.0为正常 越高越抖0为无后坐力");
    WriteFileLine(file, "// reloadspeed:   换子弹速度带动画修复可选1.0正常越高越慢越低越快");
    WriteFileLine(file, "// knowback:      武器击退值");
    WriteFileLine(file, "// firespeed:     武器攻速 +1.0:-0.5 可设置左右键");
    WriteFileLine(file, "// vmodel/wmodel: 模型路径");
    WriteFileLine(file, "// firesound/switchsound: 音效路径");
    WriteFileLine(file, "// customsoundct/customsoundt: 替换刀音效路径文件夹 不同队伍");
    WriteFileLine(file, "// autofire:      速射连发(1/0) 按住右键强制转左键速射并清除后坐力; 配有机瞄或原生开镜的武器自动失效(机瞄优先)");
    WriteFileLine(file, "// Sqtype:        1=手动自定义动画序列(IsCustom2)");
    WriteFileLine(file, "// skincode:      皮肤索引");
    WriteFileLine(file, "// ammo:          给予弹夹子弹量(默认20000)");
    WriteFileLine(file, "// maxammo:       弹夹容量(用于换弹音效播放判断)");
    WriteFileLine(file, "// killicon:      击杀图标");
    WriteFileLine(file, "// team:          可用队伍(all/ct/t/both等)");
    WriteFileLine(file, "// reloadsound:   换弹音效路径(支持 0:路径,逗号分隔)");
    WriteFileLine(file, "// usedroppedmodel: 1=掉落时使用假W模型 0=直接改世界模型");
    WriteFileLine(file, "// armfix:        手臂修复 1=每帧对1号VM执行 m_hWeapon 自赋值修复左手模 0/缺省=不管");
    WriteFileLine(file, "// 机瞄/侧瞄请到 HanWeaponZoomData.cfg 中配置");
    WriteFileLine(file, "// 检视请到 HanWeaponInspectData.cfg 中配置");
    WriteFileLine(file, "// 奔跑请到 HanWeaponRunData.cfg 中配置");
    WriteFileLine(file, "// 空仓/战术换弹请到 HanWeaponEmptyReloadData.cfg 中配置");
    WriteFileLine(file, "\"HanWeaponSystem\"");
    WriteFileLine(file, "{");
    WriteFileLine(file, "    \"weapon_example\"");
    WriteFileLine(file, "    {");
    WriteFileLine(file, "        \"command\"        \"sm_customweapon\"");
    WriteFileLine(file, "        \"classname\"      \"weapon_ak47\"");
    WriteFileLine(file, "        \"useclassname\"   \"weapon_ak47\"");
    WriteFileLine(file, "        \"damage\"         \"+40\"");
    WriteFileLine(file, "        \"firespeed\"      \"+0.5:+0.6\"");
    WriteFileLine(file, "        \"ammo\"           \"1000\"");
    WriteFileLine(file, "        \"maxammo\"        \"30\"");
    WriteFileLine(file, "        \"killicon\"       \"weapon_ak47\"");
    WriteFileLine(file, "        \"team\"           \"all\"");
    WriteFileLine(file, "        \"vmodel\"         \"models/v_custom/v_ak47.mdl\"");
    WriteFileLine(file, "        \"wmodel\"         \"models/w_custom/w_ak47.mdl\"");
    WriteFileLine(file, "        \"firesound\"      \"weapons/ak47/fire.wav\"");
    WriteFileLine(file, "        \"switchsound\"    \"weapons/ak47/draw.wav\"");
    WriteFileLine(file, "        \"reloadsound\"    \"0:weapons/ak47/reload.wav\"");
    WriteFileLine(file, "    }");
    WriteFileLine(file, "}");

    CloseHandle(file);
}

// ============================================================================
// >> 注册武器给予命令
// ============================================================================
void LoadWeaponCommands()
{
    char cfgFile[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponData.cfg");
    Handle kv = CreateKeyValues("HanWeaponSystem");
    if (FileToKeyValues(kv, cfgFile))
    {
        KvRewind(kv);

        if (KvGotoFirstSubKey(kv))
        {
            do
            {
                if (g_WeaponCount >= MAX_WEAPON)
                    break;

                KvGetString(kv, "command", g_WeaponCommand[g_WeaponCount], sizeof(g_WeaponCommand[]));
                KvGetString(kv, "classname", g_WeaponClassname[g_WeaponCount], sizeof(g_WeaponClassname[]));
                KvGetString(kv, "useclassname", g_WeaponUseClassname[g_WeaponCount], sizeof(g_WeaponUseClassname[]));
                KvGetString(kv, "team", g_WeaponTeam[g_WeaponCount], sizeof(g_WeaponTeam[]));
                KvGetString(kv, "ammo", g_WeaponAmmo[g_WeaponCount], sizeof(g_WeaponAmmo[]), "20000");

                if (strlen(g_WeaponCommand[g_WeaponCount]) > 0)
                {
                    RegConsoleCmd(g_WeaponCommand[g_WeaponCount], WeaponCommandCallback, "");
                    g_WeaponCount++;
                }
                else
                {
                    PrintToServer("[HWS] 未找到 command 字段");
                }
            } while (KvGotoNextKey(kv));
        }
        else
        {
            PrintToServer("[HWS] 配置中无任何子键");
        }
    }
    else
    {
        PrintToServer("[HWS] 读取配置失败！");
    }
    CloseHandle(kv);
}

// ============================================================================
// >> 读取配置文件
// ============================================================================
void ReadWeaponConfig()
{
    if (g_hWeaponConfig != INVALID_HANDLE)
    {
        CloseHandle(g_hWeaponConfig);
        g_hWeaponConfig = INVALID_HANDLE;
    }

    if (g_hWeaponTrie != INVALID_HANDLE)
    {
        ClearTrie(g_hWeaponTrie);
        CloseHandle(g_hWeaponTrie);
    }
    g_hWeaponTrie = CreateTrie();

    char cfgFile[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, cfgFile, sizeof(cfgFile), "configs/HanWeaponSystem/HanWeaponData.cfg");

    g_hWeaponConfig = CreateKeyValues("HanWeaponSystem");
    if (!FileToKeyValues(g_hWeaponConfig, cfgFile))
    {
        CloseHandle(g_hWeaponConfig);
        g_hWeaponConfig = INVALID_HANDLE;
        return;
    }

    KvRewind(g_hWeaponConfig);

    if (KvGotoFirstSubKey(g_hWeaponConfig))
    {
        do
        {
            char useclassname[64];
            KvGetString(g_hWeaponConfig, "useclassname", useclassname, sizeof(useclassname), "");
            if (strlen(useclassname) > 0)
            {
                Handle kvClone = CreateKeyValues("WeaponEntry");
                KvCopySubkeys(g_hWeaponConfig, kvClone);
                SetTrieValue(g_hWeaponTrie, useclassname, kvClone);
            }
        } while (KvGotoNextKey(g_hWeaponConfig));
    }

    KvRewind(g_hWeaponConfig);

    PrecacheWeaponResources(g_hWeaponConfig);
}

bool GetCustomSoundPrefixct(const char[] classname, char[] prefix, int maxlen)
{
    Handle kvWeapon = INVALID_HANDLE;
    if (GetTrieValue(g_hWeaponTrie, classname, kvWeapon))
    {
        return KvGetString(kvWeapon, "customsoundct", prefix, maxlen, "") && strlen(prefix) > 0;
    }
    return false;
}

bool GetCustomSoundPrefixt(const char[] classname, char[] prefix, int maxlen)
{
    Handle kvWeapon = INVALID_HANDLE;
    if (GetTrieValue(g_hWeaponTrie, classname, kvWeapon))
    {
        return KvGetString(kvWeapon, "customsoundt", prefix, maxlen, "") && strlen(prefix) > 0;
    }
    return false;
}

// ============================================================================
// >> 判断是否为刀(实体 classname 为 weapon_knife, 或反查主配置 classname 键为 weapon_knife)
// ============================================================================
bool IsKnifeWeapon(int weapon)
{
    if (weapon <= 0 || !IsValidEntity(weapon))
        return false;

    char cls[64];
    GetEntityClassname(weapon, cls, sizeof(cls));

    if (StrEqual(cls, "weapon_knife", false))
        return true;

    if (g_hWeaponConfig == INVALID_HANDLE)
        return false;

    if (StrContains(cls, "weapon_") == 0)
        strcopy(cls, sizeof(cls), cls[7]);

    KvRewind(g_hWeaponConfig);
    if (!KvGotoFirstSubKey(g_hWeaponConfig))
        return false;

    do
    {
        char cfgUse[64];
        KvGetString(g_hWeaponConfig, "useclassname", cfgUse, sizeof(cfgUse), "");

        if (StrContains(cfgUse, "weapon_") == 0)
            strcopy(cfgUse, sizeof(cfgUse), cfgUse[7]);

        if (StrEqual(cls, cfgUse, false))
        {
            char origClass[64];
            KvGetString(g_hWeaponConfig, "classname", origClass, sizeof(origClass), "");

            if (StrContains(origClass, "weapon_") == 0)
                strcopy(origClass, sizeof(origClass), origClass[7]);

            return (StrEqual(origClass, "knife", false));
        }

    } while (KvGotoNextKey(g_hWeaponConfig));

    return false;
}

// ============================================================================
// >> 判断武器是否配置了右键连发(firespeed 右值非空)
// ============================================================================
bool HasRightFireSpeed(int weapon)
{
    char fireSpeed[64];
    if (!GetWeaponConfigValue(weapon, "firespeed", fireSpeed, sizeof(fireSpeed)))
        return false;

    int pos = FindCharInString(fireSpeed, ':');
    if (pos == -1)
        return false;

    for (int i = pos + 1; i < sizeof(fireSpeed); i++)
    {
        if (fireSpeed[i] != '\0' && fireSpeed[i] != ' ')
            return true;
    }

    return false;
}

// ============================================================================
// >> 预缓存武器资源
// ============================================================================
void PrecacheWeaponResources(Handle kvRoot)
{
    if (kvRoot == INVALID_HANDLE)
        return;

    KvRewind(kvRoot);

    if (!KvGotoFirstSubKey(kvRoot))
        return;

    do
    {
        char weaponName[64];
        KvGetSectionName(kvRoot, weaponName, sizeof(weaponName));

        char vmodel[PLATFORM_MAX_PATH];
        char wmodel[PLATFORM_MAX_PATH];
        char fireSound[PLATFORM_MAX_PATH];
        char switchSound[PLATFORM_MAX_PATH];
        char reloadSoundStr[256];

        KvGetString(kvRoot, "Vmodels", vmodel, sizeof(vmodel), "");
        KvGetString(kvRoot, "Wmodels", wmodel, sizeof(wmodel), "");
        KvGetString(kvRoot, "FireSound", fireSound, sizeof(fireSound), "");
        KvGetString(kvRoot, "SwitchSound", switchSound, sizeof(switchSound), "");
        KvGetString(kvRoot, "reloadsound", reloadSoundStr, sizeof(reloadSoundStr), "");

        if (strlen(vmodel) > 0)
        {
            if (PrecacheModel(vmodel))
            {
                AddFileToDownloadsTable(vmodel);
            }
        }

        if (strlen(wmodel) > 0)
        {
            if (PrecacheModel(wmodel))
            {
                AddFileToDownloadsTable(wmodel);
            }
        }

        char soundPath[PLATFORM_MAX_PATH];

        if (strlen(fireSound) > 0)
        {
            if (PrecacheSound(fireSound))
            {
                Format(soundPath, sizeof(soundPath), "sound/%s", fireSound);
                AddFileToDownloadsTable(soundPath);
            }
        }

        if (strlen(switchSound) > 0)
        {
            if (PrecacheSound(switchSound))
            {
                Format(soundPath, sizeof(soundPath), "sound/%s", switchSound);
                AddFileToDownloadsTable(soundPath);
            }
        }
        if (strlen(reloadSoundStr) > 0)
        {
            char parts[8][64];
            int count = ExplodeString(reloadSoundStr, ",", parts, sizeof(parts), sizeof(parts[]));

            for (int i = 0; i < count; i++)
            {
                char pair[2][64];
                if (ExplodeString(parts[i], ":", pair, sizeof(pair), sizeof(pair[])) == 2)
                {
                    TrimString(pair[1]);

                    if (PrecacheSound(pair[1]))
                    {
                        Format(soundPath, sizeof(soundPath), "sound/%s", pair[1]);
                        AddFileToDownloadsTable(soundPath);
                    }
                }
            }
        }

        char customPrefixt[64];
        KvGetString(kvRoot, "customsoundt", customPrefixt, sizeof(customPrefixt), "");
        if (strlen(customPrefixt) > 0)
        {
            static const char soundNames[][] = {
                "hit1.wav",
                "hit2.wav",
                "hit3.wav",
                "hit4.wav",
                "stab.wav",
                "slash1.wav",
                "slash2.wav",
                "hitwall1.wav",
                "deploy1.wav"
            };

            for (int i = 0; i < sizeof(soundNames); i++)
            {
                char fullPath[PLATFORM_MAX_PATH];
                Format(fullPath, sizeof(fullPath), "weapons/%s/%s_%s", customPrefixt, customPrefixt, soundNames[i]);

                if (PrecacheSound(fullPath, true))
                {
                    Format(soundPath, sizeof(soundPath), "sound/%s", fullPath);
                    AddFileToDownloadsTable(soundPath);
                }
            }
        }

        char customPrefixct[64];
        KvGetString(kvRoot, "customsoundct", customPrefixct, sizeof(customPrefixct), "");
        if (strlen(customPrefixct) > 0)
        {
            static const char soundNames[][] = {
                "hit1.wav",
                "hit2.wav",
                "hit3.wav",
                "hit4.wav",
                "stab.wav",
                "slash1.wav",
                "slash2.wav",
                "hitwall1.wav",
                "deploy1.wav"
            };

            for (int i = 0; i < sizeof(soundNames); i++)
            {
                char fullPath[PLATFORM_MAX_PATH];
                Format(fullPath, sizeof(fullPath), "weapons/%s/%s_%s", customPrefixct, customPrefixct, soundNames[i]);

                if (PrecacheSound(fullPath, true))
                {
                    Format(soundPath, sizeof(soundPath), "sound/%s", fullPath);
                    AddFileToDownloadsTable(soundPath);
                }
            }
        }

    } while (KvGotoNextKey(kvRoot));

    KvRewind(kvRoot);
}

// ============================================================================
// >> 击杀图标
// ============================================================================
public Action Event_PlayerDeathPre(Handle event, const char[] name, bool dontBroadcast)
{
    if (g_hWeaponConfig == INVALID_HANDLE)
        return Plugin_Continue;

    char getweapon[64];
    GetEventString(event, "weapon", getweapon, sizeof(getweapon));

    KvRewind(g_hWeaponConfig);
    if (!KvGotoFirstSubKey(g_hWeaponConfig))
        return Plugin_Continue;

    do
    {
        char useclassname[64];
        KvGetString(g_hWeaponConfig, "useclassname", useclassname, sizeof(useclassname), "");

        if (StrContains(useclassname, "weapon_") == 0)
        {
            strcopy(useclassname, sizeof(useclassname), useclassname[7]);
        }

        if (StrEqual(getweapon, useclassname, false))
        {
            char killicon[64];
            KvGetString(g_hWeaponConfig, "killicon", killicon, sizeof(killicon), "");

            if (StrContains(killicon, "weapon_") == 0)
            {
                strcopy(killicon, sizeof(killicon), killicon[7]);
            }

            if (killicon[0] != '\0')
            {
                SetEventString(event, "weapon", killicon);
            }
            break;
        }

    } while (KvGotoNextKey(g_hWeaponConfig));

    return Plugin_Continue;
}

// ============================================================================
// >> 通过配置获取武器值
// ============================================================================
bool GetWeaponConfigValue(int weaponEnt, const char[] key, char[] value, int valueLen)
{
    if (weaponEnt <= 0 || !IsValidEntity(weaponEnt) || g_hWeaponConfig == INVALID_HANDLE)
        return false;

    char className[64];
    GetEntityClassname(weaponEnt, className, sizeof(className));

    if (StrContains(className, "weapon_") == 0)
        strcopy(className, sizeof(className), className[7]);

    KvRewind(g_hWeaponConfig);
    if (!KvGotoFirstSubKey(g_hWeaponConfig))
        return false;

    do
    {
        char useClass[64];
        KvGetString(g_hWeaponConfig, "useclassname", useClass, sizeof(useClass), "");

        if (StrContains(useClass, "weapon_") == 0)
            strcopy(useClass, sizeof(useClass), useClass[7]);

        if (StrEqual(className, useClass, false))
        {
            KvGetString(g_hWeaponConfig, key, value, valueLen, "");
            return (strlen(value) > 0);
        }

    } while (KvGotoNextKey(g_hWeaponConfig));

    return false;
}

int GetWeaponTeam(int weaponEnt)
{
    if (weaponEnt <= 0 || !IsValidEntity(weaponEnt) || g_hWeaponConfig == INVALID_HANDLE)
        return 0;

    char className[64];
    GetEntityClassname(weaponEnt, className, sizeof(className));

    if (StrContains(className, "weapon_") == 0)
        strcopy(className, sizeof(className), className[7]);

    KvRewind(g_hWeaponConfig);
    if (!KvGotoFirstSubKey(g_hWeaponConfig))
        return 0;

    do
    {
        char useClass[64];
        KvGetString(g_hWeaponConfig, "useclassname", useClass, sizeof(useClass), "");

        if (StrContains(useClass, "weapon_") == 0)
            strcopy(useClass, sizeof(useClass), useClass[7]);

        if (StrEqual(className, useClass, false))
        {
            char teamStr[8];
            KvGetString(g_hWeaponConfig, "team", teamStr, sizeof(teamStr), "");

            if (StrEqual(teamStr, "ct", false))
                return CS_TEAM_CT;
            else if (StrEqual(teamStr, "t", false))
                return CS_TEAM_T;
            else
                return 0;
        }

    } while (KvGotoNextKey(g_hWeaponConfig));

    return 0;
}