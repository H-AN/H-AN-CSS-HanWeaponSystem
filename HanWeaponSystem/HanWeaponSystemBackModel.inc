// ============================================================================
// >> 背部武器模型(可选功能, cvar han_wpsdisablebackweapon 控制)
// ============================================================================
// 原理: 引擎原生挂载模型(背部主武器/腿部手枪)由客户端按武器ID查本地原版
// 脚本库绘制, 自定义武器只会显示模板原版样子。本模块在 PostThinkPost
// (引擎 PostThink 末尾 UpdateAddonBits 重算之后)剥离 m_iAddonBits 的
// ADDON_PRIMARY/PISTOL/PISTOL2 位, 用配置 wmodel 的 prop_dynamic_override
// 挂到同名附件点替代; 雷/C4/拆弹器等其他位保留不动。
// 规则: 只处理主配置登记且有 wmodel 的武器段, 原版武器保持原生挂载;
//       当前激活的槽不挂(引擎本就不给激活武器设位), 切枪即时互换。
// ============================================================================

// cs_shareddefs.h 的 ADDON_* 位(cs:s 无 silencer 位)
#define BWM_ADDON_PRIMARY   0x040
#define BWM_ADDON_PISTOL    0x080
#define BWM_ADDON_PISTOL2   0x100

// ============================================================================
// >> 生命周期
// ============================================================================
void BackModel_OnPluginStart()
{
    g_hBackWeaponCvar = CreateConVar("han_wpsdisablebackweapon", "0", "背部武器模型: 0=启用假背模逻辑(默认) 1=禁用走引擎原生", FCVAR_NOTIFY, true, 0.0, true, 1.0);

    g_hBackWeaponCvar.AddChangeHook(BackModel_CvarChange);
}

public void BackModel_CvarChange(ConVar convar, const char[] oldValue, const char[] newValue)
{
    // 禁用时立即拆全部假模型并停止剥位(引擎下tick自动恢复原生挂载); 启用时自然重建
    if (convar.IntValue != 0)
    {
        BackModel_ClearAll();
    }
}

void BackModel_OnMapStart()
{
    // 地图切换实体全灭, 全量清引用防索引复用串扰(v7.8 同款纪律)
    BackModel_ClearAll();
    BackModel_ReadConfig();
}

void BackModel_PluginEnd()
{
    BackModel_ClearAll();
}

void BackModel_Reset(int client)
{
    BackModel_RemoveFake(client, 0);
    BackModel_RemoveFake(client, 1);
}

// ============================================================================
// >> 配置缓存(从主配置 trie 取每把登记武器的 wmodel)
// ============================================================================
void BackModel_ReadConfig()
{
    g_iBWCount = 0;

    for (int i = 0; i < g_WeaponCount && g_iBWCount < BWM_MAX_WEAPONS; i++)
    {
        Handle kvWeapon;
        if (!GetTrieValue(g_hWeaponTrie, g_WeaponUseClassname[i], kvWeapon))
            continue;

        char path[PLATFORM_MAX_PATH];
        KvGetString(kvWeapon, "wmodel", path, sizeof(path), "");
        NormalizePath(path);

        if (strlen(path) == 0)
            continue;

        int modelIndex = PrecacheModel(path);
        if (modelIndex <= 0)
            continue;

        strcopy(g_sBWUseClass[g_iBWCount], sizeof(g_sBWUseClass[]), g_WeaponUseClassname[i]);
        strcopy(g_sBWModelPath[g_iBWCount], sizeof(g_sBWModelPath[]), path);
        g_iBWModelIndex[g_iBWCount] = modelIndex;
        g_iBWCount++;
    }
}

int BackModel_FindEntry(const char[] useClass)
{
    for (int i = 0; i < g_iBWCount; i++)
    {
        if (StrEqual(useClass, g_sBWUseClass[i], false))
            return i;
    }

    return -1;
}

// ============================================================================
// >> 每 tick 核心(PostThinkPost: 引擎 UpdateAddonBits 重算之后执行)
// ============================================================================
public void BackModel_PostThinkPost(int client)
{
    if (g_iBWCount == 0 || g_hBackWeaponCvar.IntValue != 0)
        return;

    if (!IsClientInGame(client) || !IsPlayerAlive(client))
    {
        BackModel_Reset(client);
        return;
    }

    int active = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    int bits = GetEntProp(client, Prop_Send, "m_iAddonBits");
    int strip = 0;

    // 主武器槽(背部附件)
    int prim = GetPlayerWeaponSlot(client, 0);
    if (BackModel_ShouldFake(prim, active))
    {
        strip |= BWM_ADDON_PRIMARY;
        BackModel_EnsureFake(client, 0, prim, "primary");
    }
    else
    {
        BackModel_RemoveFake(client, 0);
    }

    // 手枪槽(右腿附件)
    int pist = GetPlayerWeaponSlot(client, 1);
    if (BackModel_ShouldFake(pist, active))
    {
        strip |= BWM_ADDON_PISTOL | BWM_ADDON_PISTOL2;
        BackModel_EnsureFake(client, 1, pist, "pistol");
    }
    else
    {
        BackModel_RemoveFake(client, 1);
    }

    if (strip != 0)
    {
        int newBits = bits & ~strip;
        if (newBits != bits)
            SetEntProp(client, Prop_Send, "m_iAddonBits", newBits);
    }
}

bool BackModel_ShouldFake(int weapon, int active)
{
    if (weapon <= 0 || weapon == active || !IsValidEntity(weapon))
        return false;

    char cls[64];
    GetEntityClassname(weapon, cls, sizeof(cls));

    return (BackModel_FindEntry(cls) != -1);
}

// ============================================================================
// >> 假模型创建/销毁(来源+模型双校验, 无变更即复用, 不反复生成)
// ============================================================================
void BackModel_EnsureFake(int client, int slot, int weapon, const char[] attach)
{
    char cls[64];
    GetEntityClassname(weapon, cls, sizeof(cls));

    int idx = BackModel_FindEntry(cls);
    if (idx == -1)
        return;

    int srcRef = EntIndexToEntRef(weapon);
    int modelIdx = g_iBWModelIndex[idx];

    int cur = EntRefToEntIndex(g_iBMFakeRef[client][slot]);
    if (cur > 0 && IsValidEntity(cur) &&
        g_iBMFakeSrcRef[client][slot] == srcRef &&
        g_iBMFakeModelIdx[client][slot] == modelIdx)
        return;

    BackModel_RemoveFake(client, slot);

    int prop = CreateEntityByName("prop_dynamic_override");
    if (prop == -1 || !IsValidEntity(prop))
        return;

    DispatchKeyValue(prop, "model", g_sBWModelPath[idx]);
    DispatchKeyValue(prop, "solid", "0");
    DispatchSpawn(prop);

    SetEntProp(prop, Prop_Data, "m_nSolidType", 0);
    SetEntData(prop, offsCollision, 2, 1, true);   // COLLISION_GROUP_DEBRIS, 与掉落假模型同款

    SetVariantString("!activator");
    AcceptEntityInput(prop, "SetParent", client);
    SetVariantString(attach);
    AcceptEntityInput(prop, "SetParentAttachment");

    g_iBMFakeRef[client][slot] = EntIndexToEntRef(prop);
    g_iBMFakeSrcRef[client][slot] = srcRef;
    g_iBMFakeModelIdx[client][slot] = modelIdx;
}

void BackModel_RemoveFake(int client, int slot)
{
    int old = EntRefToEntIndex(g_iBMFakeRef[client][slot]);
    if (old > 0 && IsValidEntity(old))
        AcceptEntityInput(old, "Kill");

    g_iBMFakeRef[client][slot] = INVALID_ENT_REFERENCE;
    g_iBMFakeSrcRef[client][slot] = INVALID_ENT_REFERENCE;
    g_iBMFakeModelIdx[client][slot] = 0;
}

void BackModel_ClearAll()
{
    for (int i = 1; i <= MaxClients; i++)
    {
        BackModel_Reset(i);
    }
}
