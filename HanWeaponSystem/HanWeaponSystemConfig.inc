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
    WriteFileLine(file, "// wmodel_silencer/firesound_silencer: 消音器变体(仅变体武器 weapon_m4a1/weapon_usp 有效); 装上消音器时用变体, 不填回落普通字段");
    WriteFileLine(file, "// customsoundct/customsoundt: 替换刀音效路径文件夹 不同队伍");
    WriteFileLine(file, "// autofire:      速射连发(1/0) 按住右键强制转左键速射并清除后坐力; 配有机瞄或原生开镜的武器自动失效(机瞄优先)");
    WriteFileLine(file, "// Sqtype:        1=手动自定义动画序列(IsCustom2)");
    WriteFileLine(file, "// skincode:      皮肤索引");
    WriteFileLine(file, "// ammo:          给予弹夹子弹量(默认20000)");
    WriteFileLine(file, "// killicon:      击杀图标");
    WriteFileLine(file, "// team:          可用队伍(all/ct/t/both等)");
    WriteFileLine(file, "// reloadsound:   兜底换弹音效(帧号:路径,逗号分隔; 用HLMV逐帧播放换弹动画, 看第几帧该播哪个音效后回来填; 按reloadfps换算秒(默认30); 自动随reloadspeed对齐)。适用于: 未配空仓/战术的武器所有换弹; 已配单侧空仓/战术的武器另一侧换弹(走引擎普通换弹)");
    WriteFileLine(file, "// usedroppedmodel: 1=掉落时使用假W模型 0=直接改世界模型");
    WriteFileLine(file, "// armfix:        手臂修复 1=每帧对1号VM执行 m_hWeapon 自赋值修复左手模 0/缺省=不管");
    WriteFileLine(file, "// shootqcfixes:  开火动画修复 1=开火动画播完自动回正idle(修复引擎同seq不重播导致的开火动画丢失/usp类不自动归0) 0/缺省=不管");
    WriteFileLine(file, "// grenadescale:  手雷类武器飞行模型缩放(如 2.0 放大 0.5 缩小; 0/缺省=不缩放; 仅手雷实体生效其他武器无效)");
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
    WriteFileLine(file, "        \"killicon\"       \"weapon_ak47\"");
    WriteFileLine(file, "        \"team\"           \"all\"");
    WriteFileLine(file, "        \"vmodel\"         \"models/v_custom/v_ak47.mdl\"");
    WriteFileLine(file, "        \"wmodel\"         \"models/w_custom/w_ak47.mdl\"");
    WriteFileLine(file, "        \"firesound\"      \"weapons/ak47/fire.wav\"");
    WriteFileLine(file, "        \"switchsound\"    \"weapons/ak47/draw.wav\"");
    WriteFileLine(file, "        \"reloadsound\"    \"0:weapons/ak47/reload.wav,45:weapons/ak47/insert.wav,70:weapons/ak47/bolt.wav\"");
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

                    // 解析武器脚本到内存缓存(插件加载时只解析一遍, 脚本不支持热更新), 供任意根节点字段通用读取
                    g_hWeaponScript[g_WeaponCount] = ParseWeaponScript(g_WeaponUseClassname[g_WeaponCount]);
                    // 弹夹容量: 从缓存读脚本根节点 clip_size(无子弹/缺失=0, 换弹音效不服务)
                    g_WeaponMaxAmmo[g_WeaponCount] = GetScriptClipSize(g_WeaponUseClassname[g_WeaponCount]);

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
        KvGetString(kvWeapon, "customsoundct", prefix, maxlen, "");
        NormalizePath(prefix);
        return strlen(prefix) > 0;
    }
    return false;
}

bool GetCustomSoundPrefixt(const char[] classname, char[] prefix, int maxlen)
{
    Handle kvWeapon = INVALID_HANDLE;
    if (GetTrieValue(g_hWeaponTrie, classname, kvWeapon))
    {
        KvGetString(kvWeapon, "customsoundt", prefix, maxlen, "");
        NormalizePath(prefix);
        return strlen(prefix) > 0;
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
// >> 预缓存音效列表(多路径逗号分隔)
// ============================================================================
void PrecacheSoundList(const char[] soundStr)
{
    if (strlen(soundStr) == 0)
        return;

    char parts[8][PLATFORM_MAX_PATH];
    int count = ExplodeString(soundStr, ",", parts, sizeof(parts), sizeof(parts[]));

    for (int i = 0; i < count; i++)
    {
        TrimString(parts[i]);
        NormalizePath(parts[i]);
        if (strlen(parts[i]) > 0)
            PrecacheSound(parts[i]);
    }
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
        char wmodelSilencer[PLATFORM_MAX_PATH];
        char fireSoundSilencer[PLATFORM_MAX_PATH];

        KvGetString(kvRoot, "Vmodels", vmodel, sizeof(vmodel), "");
        KvGetString(kvRoot, "Wmodels", wmodel, sizeof(wmodel), "");
        KvGetString(kvRoot, "FireSound", fireSound, sizeof(fireSound), "");
        KvGetString(kvRoot, "SwitchSound", switchSound, sizeof(switchSound), "");
        KvGetString(kvRoot, "reloadsound", reloadSoundStr, sizeof(reloadSoundStr), "");
        KvGetString(kvRoot, "wmodel_silencer", wmodelSilencer, sizeof(wmodelSilencer), "");
        KvGetString(kvRoot, "firesound_silencer", fireSoundSilencer, sizeof(fireSoundSilencer), "");

        NormalizePath(vmodel);
        NormalizePath(wmodel);
        NormalizePath(fireSound);
        NormalizePath(switchSound);
        NormalizePath(reloadSoundStr);
        NormalizePath(wmodelSilencer);
        NormalizePath(fireSoundSilencer);

        if (strlen(vmodel) > 0)
            PrecacheModel(vmodel);

        if (strlen(wmodel) > 0)
            PrecacheModel(wmodel);

        if (strlen(fireSound) > 0)
            PrecacheSoundList(fireSound);

        if (strlen(switchSound) > 0)
            PrecacheSoundList(switchSound);

        // 消音器变体资源: 填了才预缓存, 缺失不预缓存(回落普通字段)
        if (strlen(wmodelSilencer) > 0)
            PrecacheModel(wmodelSilencer);

        if (strlen(fireSoundSilencer) > 0)
            PrecacheSoundList(fireSoundSilencer);

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
                    NormalizePath(pair[1]);
                    PrecacheSound(pair[1]);
                }
            }
        }

        char customPrefixt[64];
        KvGetString(kvRoot, "customsoundt", customPrefixt, sizeof(customPrefixt), "");
        NormalizePath(customPrefixt);
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

                PrecacheSound(fullPath, true);
            }

            // 手雷音效同名文件(弹跳/爆炸/拉保险, 与默认路径一致的文件名)
            static const char grenadeSoundNames[][] = {
                "he_bounce-1.wav",
                "explode3.wav",
                "explode4.wav",
                "explode5.wav",
                "grenade_hit1.wav",
                "flashbang_explode1.wav",
                "flashbang_explode2.wav",
                "sg_explode.wav",
                "pinpull.wav"
            };

            for (int i = 0; i < sizeof(grenadeSoundNames); i++)
            {
                char fullPath[PLATFORM_MAX_PATH];
                Format(fullPath, sizeof(fullPath), "weapons/%s/%s", customPrefixt, grenadeSoundNames[i]);

                PrecacheSound(fullPath, true);
            }
        }

        char customPrefixct[64];
        KvGetString(kvRoot, "customsoundct", customPrefixct, sizeof(customPrefixct), "");
        NormalizePath(customPrefixct);
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

                PrecacheSound(fullPath, true);
            }

            // 手雷音效同名文件(弹跳/爆炸/拉保险, 与默认路径一致的文件名)
            static const char grenadeSoundNamesct[][] = {
                "he_bounce-1.wav",
                "explode3.wav",
                "explode4.wav",
                "explode5.wav",
                "grenade_hit1.wav",
                "flashbang_explode1.wav",
                "flashbang_explode2.wav",
                "sg_explode.wav",
                "pinpull.wav"
            };

            for (int i = 0; i < sizeof(grenadeSoundNamesct); i++)
            {
                char fullPath[PLATFORM_MAX_PATH];
                Format(fullPath, sizeof(fullPath), "weapons/%s/%s", customPrefixct, grenadeSoundNamesct[i]);

                PrecacheSound(fullPath, true);
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
// >> 路径统一: 反斜杠转正斜杠(引擎路径只接受 /, 用户填 \ 也兼容)
// ============================================================================
void NormalizePath(char[] path)
{
    for (int i = 0; i < strlen(path); i++)
    {
        if (path[i] == '\\')
            path[i] = '/';
    }
}

// ============================================================================
// >> 通过配置获取武器值
// ============================================================================
// ============================================================================
// >> 解析服务器武器脚本 scripts/<useclassname>.txt(插件加载时只解析一遍)
//    返回 KV 句柄(根节点), 脚本缺失/读取失败返回 INVALID_HANDLE
//    注意: 句柄由插件持有, 插件卸载时 SourceMod 自动回收, 无需手动关闭
// ============================================================================
Handle ParseWeaponScript(const char[] useclassname)
{
    char path[PLATFORM_MAX_PATH];
    Format(path, sizeof(path), "scripts/%s.txt", useclassname);
    if (!FileExists(path, true, "GAME"))
        return INVALID_HANDLE;

    Handle kv = CreateKeyValues("WeaponData");
    if (!FileToKeyValues(kv, path))
    {
        CloseHandle(kv);
        return INVALID_HANDLE;
    }

    KvRewind(kv);
    return kv;
}

// ============================================================================
// >> 通用: 从缓存的武器脚本根节点读取任意字段(字符串)
//    脚本未缓存/键缺失 → false + 空串
//    新字段(如 WeaponPrice/自定义键)直接复用, 无需新建函数
//    扫描含当前正在注册的槽位(i<=g_WeaponCount), 兼容插件加载时半注册状态
// ============================================================================
bool GetWeaponScriptValue(const char[] useclassname, const char[] key, char[] value, int maxlen)
{
    if (strlen(useclassname) <= 0)
        return false;

    for (int i = 0; i <= g_WeaponCount; i++)
    {
        if (StrEqual(useclassname, g_WeaponUseClassname[i], false))
        {
            if (g_hWeaponScript[i] == INVALID_HANDLE)
                return false;

            KvRewind(g_hWeaponScript[i]);
            KvGetString(g_hWeaponScript[i], key, value, maxlen, "");
            return (strlen(value) > 0);
        }
    }

    return false;
}

// ============================================================================
// >> clip_size 专用薄封装(弹夹容量)
//    键缺失 → 0; 负值(无弹武器如刀/雷/火箭筒)钳制为 0
//    0 = 无子弹 = 换弹音效不服务
// ============================================================================
int GetScriptClipSize(const char[] useclassname)
{
    char clipStr[16];
    if (!GetWeaponScriptValue(useclassname, "clip_size", clipStr, sizeof(clipStr)))
        return 0;

    int clip = StringToInt(clipStr);
    return (clip < 0) ? 0 : clip;
}

// ============================================================================
// >> 获取武器弹夹容量(预缓存的脚本 clip_size, 0=未知)
// ============================================================================
int GetWeaponMaxAmmo(int weaponEnt)
{
    if (weaponEnt <= 0 || !IsValidEntity(weaponEnt))
        return 0;

    char className[64];
    GetEntityClassname(weaponEnt, className, sizeof(className));

    for (int i = 0; i < g_WeaponCount; i++)
    {
        if (StrEqual(className, g_WeaponUseClassname[i], false))
            return g_WeaponMaxAmmo[i];
    }

    return 0;
}

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
            NormalizePath(value);
            return (strlen(value) > 0);
        }

    } while (KvGotoNextKey(g_hWeaponConfig));

    return false;
}

// ============================================================================
// >> 消音器变体武器判断(硬编码 weapon_m4a1/weapon_usp 两类)
//    按主配置 classname 键判定(实体 useclassname 可能被改写)
// ============================================================================
bool IsVariantWeaponClass(const char[] origClass)
{
    char cls[64];
    strcopy(cls, sizeof(cls), origClass);

    if (StrContains(cls, "weapon_") == 0)
        strcopy(cls, sizeof(cls), cls[7]);

    return StrEqual(cls, "m4a1", false) || StrEqual(cls, "usp", false);
}

// ============================================================================
// >> 运行时判断武器是否为消音器变体武器(实体存在 m_bSilencerOn 联网属性即算,
//    该属性仅 m4a1/usp 类实体存在, 与主配置 classname 硬编码判定一致)
// ============================================================================
bool IsVariantWeapon(int weapon)
{
    return weapon > 0 && IsValidEntity(weapon) && HasEntProp(weapon, Prop_Send, "m_bSilencerOn");
}

// ============================================================================
// >> 消音器是否装上(m_bSilencerOn==1; 仅变体武器有意义)
// ============================================================================
bool IsWeaponSilencerOn(int weapon)
{
    return IsVariantWeapon(weapon) && GetEntProp(weapon, Prop_Send, "m_bSilencerOn") == 1;
}

// ============================================================================
// >> 是否为 usp 模板武器(按主配置 classname 字段 == weapon_usp 判定)
//    用于 usp 特例回正: usp 引擎不会自动把 0号VM 回 idle, 需要主动写正确 idle
// ============================================================================
bool Weapon_IsUspTemplate(int weapon)
{
    if (weapon <= 0 || !IsValidEntity(weapon))
        return false;

    char className[64];
    GetEntityClassname(weapon, className, sizeof(className));

    for (int i = 0; i < g_WeaponCount; i++)
    {
        if (StrEqual(className, g_WeaponUseClassname[i], false))
            return StrEqual(g_WeaponClassname[i], "weapon_usp", false);
    }

    return false;
}

// ============================================================================
// >> 开火动画修复开关(主配置 shootqcfixes 键, 模式同 armfix/autofire)
//    配置了的武器: 非idle动画播完(cycle>=1.0)自动回正 idle,
//    保证下次开火随机 seq 必然 != 当前 seq → 引擎必定重播开火动画
// ============================================================================
bool Weapon_IsShootQcFix(int weaponEnt)
{
    if (weaponEnt <= 0 || !IsValidEntity(weaponEnt))
        return false;

    char value[8];
    return GetWeaponConfigValue(weaponEnt, "shootqcfixes", value, sizeof(value)) && StringToInt(value) > 0;
}

// ============================================================================
// >> 变体感知读取主配置值(消音器装上且变体字段填了 → 变体; 否则回落普通字段)
// ============================================================================
bool GetWeaponConfigValueVariant(int weapon, const char[] baseKey, char[] value, int valueLen)
{
    if (IsWeaponSilencerOn(weapon))
    {
        char variantKey[64];
        Format(variantKey, sizeof(variantKey), "%s_silencer", baseKey);

        if (GetWeaponConfigValue(weapon, variantKey, value, valueLen))
            return true;
    }

    return GetWeaponConfigValue(weapon, baseKey, value, valueLen);
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

// ============================================================================
// >> 原生武器价格(运行时按需查询引擎, 不建缓存; 刀/C4/盾=0)
// ============================================================================
int GetNativeWeaponPrice(int client, const char[] classname)
{
    if (client <= 0 || client > MaxClients || !IsClientInGame(client))
        return 0;

    if (!g_bNativePriceChecked)
    {
        g_bNativePriceAvailable = (GetFeatureStatus(FeatureType_Native, "CS_AliasToWeaponID") == FeatureStatus_Available
                                && GetFeatureStatus(FeatureType_Native, "CS_GetWeaponPrice") == FeatureStatus_Available);
        g_bNativePriceChecked = true;
    }

    if (!g_bNativePriceAvailable)
        return GetFallbackNativePrice(classname);

    char alias[32];
    strcopy(alias, sizeof(alias), classname);
    if (StrContains(alias, "weapon_") == 0)
        strcopy(alias, sizeof(alias), alias[7]);

    CSWeaponID wid = CS_AliasToWeaponID(alias);
    if (wid == CSWeapon_NONE)
        return 0;

    return CS_GetWeaponPrice(client, wid);
}

// ============================================================================
// >> 原生武器价格内置小表(仅当 cstrike 原生不可用时的兜底; CS:S 默认售价)
//    未知/未列出的原生武器返回 0(如刀/C4 本就无价格)
// ============================================================================
int GetFallbackNativePrice(const char[] classname)
{
    if (StrEqual(classname, "weapon_p228")) return 600;
    if (StrEqual(classname, "weapon_glock")) return 400;
    if (StrEqual(classname, "weapon_scout")) return 2750;
    if (StrEqual(classname, "weapon_hegrenade")) return 300;
    if (StrEqual(classname, "weapon_xm1014")) return 3000;
    if (StrEqual(classname, "weapon_mac10")) return 1400;
    if (StrEqual(classname, "weapon_aug")) return 3500;
    if (StrEqual(classname, "weapon_smokegrenade")) return 300;
    if (StrEqual(classname, "weapon_elite")) return 800;
    if (StrEqual(classname, "weapon_fiveseven")) return 750;
    if (StrEqual(classname, "weapon_ump45")) return 1700;
    if (StrEqual(classname, "weapon_sg550")) return 4200;
    if (StrEqual(classname, "weapon_galil")) return 2000;
    if (StrEqual(classname, "weapon_famas")) return 2250;
    if (StrEqual(classname, "weapon_usp")) return 500;
    if (StrEqual(classname, "weapon_awp")) return 4750;
    if (StrEqual(classname, "weapon_mp5navy")) return 1500;
    if (StrEqual(classname, "weapon_m249")) return 5750;
    if (StrEqual(classname, "weapon_m3")) return 1700;
    if (StrEqual(classname, "weapon_m4a1")) return 3100;
    if (StrEqual(classname, "weapon_tmp")) return 1250;
    if (StrEqual(classname, "weapon_g3sg1")) return 5000;
    if (StrEqual(classname, "weapon_flashbang")) return 200;
    if (StrEqual(classname, "weapon_deagle")) return 650;
    if (StrEqual(classname, "weapon_sg552")) return 3500;
    if (StrEqual(classname, "weapon_ak47")) return 2500;
    if (StrEqual(classname, "weapon_p90")) return 2350;
    if (StrEqual(classname, "weapon_shield")) return 2200;
    if (StrEqual(classname, "weapon_kevlar")) return 650;
    if (StrEqual(classname, "weapon_assaultsuit")) return 1000;
    if (StrEqual(classname, "weapon_nightvision")) return 1500;
    return 0;
}

// ============================================================================
// >> 通用工具函数(原 smlib 内联, 摆脱外部库依赖; 实现与 smlib 等价)
//    Config.inc 为第2个被 include 的模块, 早于全部使用点(最早 EmptyReload.inc)
// ============================================================================

// 玩家金钱读取(CS:S m_iAccount)
int Client_GetMoney(int client)
{
    return GetEntProp(client, Prop_Send, "m_iAccount");
}

// 玩家金钱写入
void Client_SetMoney(int client, int value)
{
    SetEntProp(client, Prop_Send, "m_iAccount", value);
}

// 武器归属者(Prop_Data m_hOwner)
int Weapon_GetOwner(int weapon)
{
    return GetEntPropEnt(weapon, Prop_Data, "m_hOwner");
}

// 武器是否正在换弹(Prop_Data m_bInReload)
bool Weapon_IsReloading(int weapon)
{
    return GetEntProp(weapon, Prop_Data, "m_bInReload") != 0;
}

// 取当前活动武器实体 + classname(无有效武器时 buffer 置空并返回 INVALID_ENT_REFERENCE)
int Client_GetActiveWeaponName(int client, char[] buffer, int size)
{
    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= 0 || !IsValidEntity(weapon))
    {
        buffer[0] = '\0';
        return INVALID_ENT_REFERENCE;
    }

    GetEdictClassname(weapon, buffer, size);
    return weapon;
}