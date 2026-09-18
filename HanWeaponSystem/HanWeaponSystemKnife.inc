// CS:S knife support. No detours, game DLL changes or gamedata offsets.
// The TE hook traces synchronously; damage supplementation runs after ItemPostFrame.
// See docs/8.2.md for the deliberately separate TraceAttack compatibility boundary.

HanKnifeResult g_Knife[MAXPLAYERS + 1];
bool g_KnifeHostage[MAXPLAYERS + 1];
bool g_KnifeHostageBlood[MAXPLAYERS + 1];
int g_KnifeHostageHealth[MAXPLAYERS + 1];
int g_KnifeSerial;
int g_KnifeTick[MAXPLAYERS + 1];
int g_KnifePreWeapon[MAXPLAYERS + 1];
int g_KnifePreTick[MAXPLAYERS + 1];
bool g_KnifeFirstSwing[MAXPLAYERS + 1];
int g_KnifeOriginalRef[MAXPLAYERS + 1];
bool g_KnifePending[MAXPLAYERS + 1];
bool g_KnifeNativeSeen[MAXPLAYERS + 1];
bool g_KnifeDamageCalled[MAXPLAYERS + 1];
bool g_KnifeInForward[MAXPLAYERS + 1];
bool g_KnifeApplying[MAXPLAYERS + 1];
float g_KnifeBaseDamage[MAXPLAYERS + 1];
float g_KnifeAngles[MAXPLAYERS + 1][3];
float g_KnifeSoundUntil[MAXPLAYERS + 1];
int g_KnifeSoundWeapon[MAXPLAYERS + 1];
GlobalForward g_HanKnifeAttack, g_HanKnifeTrace, g_HanKnifeDamage, g_HanKnifeFinished;
ConVar g_KnifeDebug, g_KnifeDamageScale, g_KnifePushScale;
StringMap g_KnifeBadConfig;
int g_KnifeSlashEffect = -1, g_KnifeBloodEffect = -1;

void Knife_Reset(int client)
{
    g_KnifeHostage[client] = false;
    g_KnifeHostageBlood[client] = false;
    g_KnifeHostageHealth[client] = -1;
    HanKnifeResult empty;
    g_Knife[client] = empty;
    g_Knife[client].WeaponRef = INVALID_ENT_REFERENCE;
    g_Knife[client].EntityRef = INVALID_ENT_REFERENCE;
    g_KnifeOriginalRef[client] = INVALID_ENT_REFERENCE;
    g_KnifePending[client] = false;
    g_KnifeNativeSeen[client] = false;
    g_KnifeDamageCalled[client] = false;
    g_KnifeApplying[client] = false;
    g_KnifeInForward[client] = false;
    g_KnifeSoundUntil[client] = 0.0;
    g_KnifeSoundWeapon[client] = INVALID_ENT_REFERENCE;
}

void Knife_OnPluginStart()
{
    g_KnifeDebug = CreateConVar("han_knife_debug", "0", "8.2 knife event/trace/damage diagnostics", 0, true, 0.0, true, 1.0);
    g_KnifeDamageScale = FindConVar("knife_damage_scale");
    g_KnifePushScale = FindConVar("phys_pushscale");
    g_KnifeBadConfig = new StringMap();
    AddTempEntHook("PlayerAnimEvent", Knife_PlayerAnim);
    HookEvent("player_hurt", Knife_PlayerHurt, EventHookMode_Pre);
    for (int client = 1; client <= MaxClients; client++)
    {
        Knife_Reset(client);
        Han_ModelReset(client);
        if (IsClientInGame(client))
            Knife_HookClient(client);
    }
    for (int entity = MaxClients + 1; entity < GetMaxEntities(); entity++)
    {
        if (IsValidEntity(entity) && HasEntProp(entity, Prop_Data, "m_takedamage")
            && GetEntProp(entity, Prop_Data, "m_takedamage") != 0)
            Knife_EnsureTargetHooks(EntIndexToEntRef(entity));
    }
}

void Knife_EntityCreated(int entity)
{
    if (entity > MaxClients)
        SDKHookEx(entity, SDKHook_SpawnPost, Knife_EntitySpawned);
}

public void Knife_EntitySpawned(int entity)
{
    if (IsValidEntity(entity) && HasEntProp(entity, Prop_Data, "m_takedamage") && GetEntProp(entity, Prop_Data, "m_takedamage") != 0)
        Knife_EnsureTargetHooks(EntIndexToEntRef(entity));
}

void Knife_HookClient(int client)
{
    SDKHook(client, SDKHook_TraceAttack, Knife_TraceAttack);
    SDKHook(client, SDKHook_PostThink, Knife_PostThink);
    SDKHook(client, SDKHook_PostThinkPost, Knife_PostThinkPost);
    SDKHook(client, SDKHook_WeaponSwitchPost, Knife_WeaponSwitchPost);
}

public void Knife_PostThink(int client)
{
    g_KnifePreWeapon[client] = INVALID_ENT_REFERENCE;
    if (!IsPlayerAlive(client))
        return;
    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon <= MaxClients || !HasEntProp(weapon, Prop_Send, "m_flNextPrimaryAttack"))
        return;
    // Capture before TE callbacks can modify the cooldown for their animation.
    g_KnifePreWeapon[client] = EntIndexToEntRef(weapon);
    g_KnifePreTick[client] = GetGameTickCount();
    g_KnifeFirstSwing[client] = GetEntPropFloat(weapon, Prop_Send, "m_flNextPrimaryAttack") + 0.4 < GetGameTime();
}

public void Knife_WeaponSwitchPost(int client, int weapon)
{
    // SDKHook post also runs for unsuccessful/same-weapon switch attempts.
    // Invalidate only when the actual active weapon changed.
    int active = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (EntRefToEntIndex(g_Knife[client].WeaponRef) != active)
        Knife_Reset(client);
    if (EntRefToEntIndex(g_HanModelWeapon[client]) != active)
        Han_ModelReset(client);
}

int Knife_EffectIndex(const char[] name)
{
    int table = FindStringTable("EffectDispatch");
    if (table == INVALID_STRING_TABLE)
        return -1;
    int index = FindStringIndex(table, name);
    if (index != INVALID_STRING_INDEX)
        return index;
    bool locked = LockStringTables(false);
    AddToStringTable(table, name);
    LockStringTables(locked);
    return FindStringIndex(table, name);
}

void Knife_OnMapStart()
{
    for (int client = 1; client <= MaxClients; client++)
    {
        Knife_Reset(client);
        Han_ModelReset(client);
    }
    g_KnifeBadConfig.Clear();
    g_KnifeSlashEffect = Knife_EffectIndex("KnifeSlash");
    g_KnifeBloodEffect = Knife_EffectIndex("csblood");
    PrecacheScriptSound("Weapon_Knife.Slash");
    PrecacheScriptSound("Weapon_Knife.Hit");
    PrecacheScriptSound("Weapon_Knife.Stab");
    PrecacheScriptSound("Weapon_Knife.HitWall");
}

bool Knife_IsKnife(int weapon)
{
    if (weapon <= MaxClients || !IsValidEntity(weapon))
        return false;
    // The actual engine class survives classname/model replacement. This covers
    // stock weapon_knife and every knife-template entity without registration.
    char netclass[64];
    return GetEntityNetClass(weapon, netclass, sizeof(netclass)) && StrEqual(netclass, "CKnife");
}

bool Knife_IsHostage(int entity)
{
    if (entity <= MaxClients || !IsValidEntity(entity))
        return false;
    char netclass[64];
    return GetEntityNetClass(entity, netclass, sizeof(netclass)) && StrEqual(netclass, "CHostage");
}

float Knife_ReadRange(int weapon, bool stab)
{
    float fallback = stab ? 32.0 : 48.0;
    char key[32], value[64];
    strcopy(key, sizeof(key), stab ? "knife_range_secondary" : "knife_range_primary");
    if (!GetWeaponConfigValue(weapon, key, value, sizeof(value)))
        return fallback;
    TrimString(value);
    if (!value[0])
        return fallback;
    float range;
    int consumed = StringToFloatEx(value, range);
    if (consumed == strlen(value) && range == 0.0)
        return fallback;
    // NaN and infinity fail these ordered bounds too. 8192 is an explicit config limit.
    if (consumed == strlen(value) && range > 0.0 && range <= 8192.0)
        return range;
    char classname[64], warning[128];
    GetEntityClassname(weapon, classname, sizeof(classname));
    Format(warning, sizeof(warning), "%s:%s", classname, key);
    int unused;
    if (!g_KnifeBadConfig.GetValue(warning, unused))
    {
        LogError("[Han 8.2] %s='%s' invalid; using %.1f (valid: 0 or (0,8192]).", warning, value, fallback);
        g_KnifeBadConfig.SetValue(warning, 1);
    }
    return fallback;
}

// MASK_SOLID contains MONSTER, WINDOW and MOVEABLE: StandardFilterRules' content
// exclusions do not remove those entities. World/static props stay in the trace.
// These group exclusions are CGameRules::ShouldCollide(NONE, group) in CS:S.
// SDKTools does NOT invoke every entity's virtual ShouldCollide; see limitations.
public bool Knife_TraceFilter(int entity, int mask, any client)
{
    if (entity == client)
        return false;
    if (entity <= 0)
        return true;
    if (!IsValidEntity(entity))
        return false;
    if (HasEntProp(entity, Prop_Send, "m_hOwnerEntity") && GetEntPropEnt(entity, Prop_Send, "m_hOwnerEntity") == client)
        return false;
    if (GetEntPropEnt(client, Prop_Send, "m_hOwnerEntity") == entity)
        return false;
    if (HasEntProp(entity, Prop_Send, "m_CollisionGroup"))
    {
        int group = GetEntProp(entity, Prop_Send, "m_CollisionGroup");
        if ((group == 1 && !(mask & CONTENTS_DEBRIS)) || group == 10 || group == 12 || group == 14)
            return false;
    }
    return true;
}

bool Knife_IsBSP(int entity)
{
    if (entity == 0)
        return true;
    char model[PLATFORM_MAX_PATH];
    if (entity > MaxClients && HasEntProp(entity, Prop_Data, "m_ModelName"))
    {
        GetEntPropString(entity, Prop_Data, "m_ModelName", model, sizeof(model));
        return model[0] == '*';
    }
    return false;
}

Handle Knife_Trace(int client, const float start[3], const float direction[3], float range)
{
    float end[3];
    for (int i = 0; i < 3; i++)
        end[i] = start[i] + direction[i] * range;
    Handle trace = TR_TraceRayFilterEx(start, end, MASK_SOLID, RayType_EndPoint, Knife_TraceFilter, client);
    if (TR_GetFraction(trace) < 1.0)
        return trace;
    delete trace;
    trace = TR_TraceHullFilterEx(start, end, view_as<float>({-16.0, -16.0, -18.0}),
        view_as<float>({16.0, 16.0, 18.0}), MASK_SOLID, Knife_TraceFilter, client);
    if (TR_GetFraction(trace) >= 1.0 || !Knife_IsBSP(TR_GetEntityIndex(trace)))
        return trace;

    // FindHullIntersection: refine a BSP contact, not an extended player attack.
    float hullEnd[3];
    TR_GetEndPosition(hullEnd, trace);
    for (int i = 0; i < 3; i++)
        hullEnd[i] = start[i] + (hullEnd[i] - start[i]) * 2.0;
    Handle exact = TR_TraceRayFilterEx(start, hullEnd, MASK_SOLID, RayType_EndPoint, Knife_TraceFilter, client);
    if (TR_GetFraction(exact) < 1.0)
    {
        delete trace;
        return exact;
    }
    delete exact;
    float nearest = 999999999.0;
    for (int corner = 0; corner < 8; corner++)
    {
        end[0] = hullEnd[0] + ((corner & 4) ? 16.0 : -16.0);
        end[1] = hullEnd[1] + ((corner & 2) ? 16.0 : -16.0);
        end[2] = hullEnd[2] + ((corner & 1) ? 45.0 : 0.0);
        exact = TR_TraceRayFilterEx(start, end, MASK_SOLID, RayType_EndPoint, Knife_TraceFilter, client);
        if (TR_GetFraction(exact) < 1.0)
        {
            float point[3];
            TR_GetEndPosition(point, exact);
            float distance = GetVectorDistance(start, point, true);
            if (distance < nearest)
            {
                nearest = distance;
                delete trace;
                trace = exact;
                exact = null;
            }
        }
        delete exact;
    }
    return trace;
}

int Knife_TraceEntityRef(Handle trace)
{
    if (TR_GetFraction(trace) >= 1.0)
        return INVALID_ENT_REFERENCE;
    int entity = TR_GetEntityIndex(trace);
    return entity == 0 ? 0 : (entity > 0 ? EntIndexToEntRef(entity) : INVALID_ENT_REFERENCE);
}

void Knife_ReadTrace(int client, Handle trace)
{
    g_Knife[client].Hit = TR_GetFraction(trace) < 1.0;
    g_Knife[client].EntityRef = Knife_TraceEntityRef(trace);
    float point[3], normal[3];
    TR_GetEndPosition(point, trace);
    TR_GetPlaneNormal(trace, normal);
    for (int i = 0; i < 3; i++)
    {
        g_Knife[client].Position[i] = point[i];
        g_Knife[client].Normal[i] = normal[i];
    }
    g_Knife[client].SurfaceProps = TR_GetSurfaceProps(trace);
    g_Knife[client].SurfaceFlags = TR_GetSurfaceFlags(trace);
    g_Knife[client].HitBox = TR_GetHitBoxIndex(trace);
}

public bool Knife_HitboxFilter(int entity, int mask, any client)
{
    // Retain walls/props/teammates. MASK_SHOT may pass grates but never selects
    // another target: the final entity must match the MASK_SOLID result.
    return Knife_TraceFilter(entity, mask, client);
}

void Knife_Classify(int client, const float start[3], const float direction[3], float range)
{
    int victim = EntRefToEntIndex(g_Knife[client].EntityRef);
    if (victim < 1 || victim > MaxClients || !IsClientInGame(victim))
        return;
    float end[3];
    for (int i = 0; i < 3; i++)
        end[i] = start[i] + direction[i] * range;
    Handle trace = TR_TraceRayFilterEx(start, end, MASK_SHOT | CONTENTS_GRATE, RayType_EndPoint, Knife_HitboxFilter, client);
    int group = TR_GetHitGroup(trace);
    if (TR_GetFraction(trace) < 1.0 && TR_GetEntityIndex(trace) == victim && group > 0)
    {
        g_Knife[client].HitGroup = group;
        g_Knife[client].HitGroupValid = true;
    }
    delete trace;
}

void Knife_EnsureTargetHooks(int ref)
{
    int entity = EntRefToEntIndex(ref);
    if (entity <= MaxClients || !IsValidEntity(entity) || !HasEntProp(entity, Prop_Data, "m_takedamage"))
        return;
    // Re-register OUR callback only; no growing callback list across attacks.
    SDKUnhook(entity, SDKHook_TraceAttack, Knife_TraceAttack);
    SDKUnhook(entity, SDKHook_OnTakeDamage, Knife_ObjectDamage);
    SDKUnhook(entity, SDKHook_OnTakeDamagePost, Knife_ObjectDamagePost);
    SDKHookEx(entity, SDKHook_TraceAttack, Knife_TraceAttack);
    SDKHookEx(entity, SDKHook_OnTakeDamage, Knife_ObjectDamage);
    if (Knife_IsHostage(entity))
        SDKHookEx(entity, SDKHook_OnTakeDamagePost, Knife_ObjectDamagePost);
}

public Action Knife_PlayerAnim(const char[] name, const int[] clients, int numClients, float delay)
{
    // CS:S stores m_hPlayer as a CBaseHandle. TE_ReadNum reads its raw cells;
    // mark it as a SourceMod EntRef so resolution checks the serial as well.
    // Do not mask to an entity index: a stale handle must remain invalid.
    int client = EntRefToEntIndex(TE_ReadNum("m_hPlayer") | (1 << 31));
    int event = TE_ReadNum("m_iEvent");
    if ((event != 0 && event != 1) || client < 1 || client > MaxClients || !IsClientInGame(client) || !IsPlayerAlive(client))
        return Plugin_Continue;
    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (!Knife_IsKnife(weapon))
        return Plugin_Continue;
    // An API consumer must not recursively generate a new attack inside a forward.
    if (g_KnifeInForward[client] || g_KnifeApplying[client])
        return Plugin_Continue;
    Knife_Reset(client);
    bool stab = event == 0;
    float range = Knife_ReadRange(weapon, stab);
    float originalRange = stab ? 32.0 : 48.0;
    if (++g_KnifeSerial <= 0)
        g_KnifeSerial = 1;
    g_Knife[client].AttackId = g_KnifeSerial;
    g_Knife[client].WeaponRef = EntIndexToEntRef(weapon);
    g_Knife[client].Type = stab ? HanKnife_Secondary : HanKnife_Primary;
    g_Knife[client].Time = GetGameTime();
    g_Knife[client].Range = range;
    g_Knife[client].RangeChanged = range != originalRange;
    g_KnifeTick[client] = GetGameTickCount();
    g_KnifePending[client] = true;
    float start[3], angles[3], direction[3];
    GetClientEyePosition(client, start);
    GetClientEyeAngles(client, angles);
    GetAngleVectors(angles, direction, NULL_VECTOR, NULL_VECTOR);
    GetEntPropVector(client, Prop_Data, "m_angAbsRotation", g_KnifeAngles[client]);
    for (int i = 0; i < 3; i++)
    {
        g_Knife[client].Start[i] = start[i];
        g_Knife[client].Direction[i] = direction[i];
    }
    Handle trace = Knife_Trace(client, start, direction, range);
    Knife_ReadTrace(client, trace);
    delete trace;
    g_KnifeHostage[client] = Knife_IsHostage(EntRefToEntIndex(g_Knife[client].EntityRef));
    g_KnifeOriginalRef[client] = g_Knife[client].EntityRef;
    if (g_Knife[client].RangeChanged)
    {
        // Reference only; actual TraceAttack is still the authority for whether
        // the native path attempted damage (including blocked/friendly damage).
        trace = Knife_Trace(client, start, direction, originalRange);
        g_KnifeOriginalRef[client] = Knife_TraceEntityRef(trace);
        delete trace;
        g_KnifeSoundWeapon[client] = EntIndexToEntRef(weapon);
        g_KnifeSoundUntil[client] = GetGameTime() + (stab ? 0.25 : 0.15);
    }
    if (g_KnifeHostage[client])
    {
        g_KnifeSoundWeapon[client] = EntIndexToEntRef(weapon);
        g_KnifeSoundUntil[client] = GetGameTime() + (stab ? 0.25 : 0.15);
    }
    Knife_Classify(client, start, direction, range);
    bool firstSwing = GetEntPropFloat(weapon, Prop_Send, "m_flNextPrimaryAttack") + 0.4 < GetGameTime();
    if (g_KnifePreWeapon[client] == g_Knife[client].WeaponRef && g_KnifePreTick[client] == GetGameTickCount())
        firstSwing = g_KnifeFirstSwing[client];
    g_KnifeBaseDamage[client] = stab ? 65.0 : (firstSwing ? 20.0 : 15.0);
    int victim = EntRefToEntIndex(g_Knife[client].EntityRef);
    if (stab && victim > 0 && victim <= MaxClients)
    {
        float attackerPos[3], victimPos[3], victimAngles[3], targetForward[3], los[3];
        GetClientAbsOrigin(client, attackerPos);
        GetClientAbsOrigin(victim, victimPos);
        GetClientAbsAngles(victim, victimAngles);
        GetAngleVectors(victimAngles, targetForward, NULL_VECTOR, NULL_VECTOR);
        MakeVectorFromPoints(attackerPos, victimPos, los);
        los[2] = 0.0;
        NormalizeVector(los, los);
        if (los[0] * targetForward[0] + los[1] * targetForward[1] > 0.8)
            g_KnifeBaseDamage[client] *= 3.0;
    }
    if (g_KnifeDamageScale != null)
        g_KnifeBaseDamage[client] *= float(g_KnifeDamageScale.IntValue) / 100.0;
    Knife_EnsureTargetHooks(g_KnifeOriginalRef[client]);
    Knife_EnsureTargetHooks(g_Knife[client].EntityRef);
    int attackId = g_Knife[client].AttackId;
    g_KnifeInForward[client] = true;
    Call_StartForward(g_HanKnifeAttack);
    Call_PushCell(client);
    Call_PushCell(weapon);
    Call_PushCell(g_Knife[client].AttackId);
    Call_PushCell(g_Knife[client].Type);
    Call_Finish();
    if (!Knife_RecordValid(client, attackId))
    {
        g_KnifeInForward[client] = false;
        TE_Start(name);
        return Plugin_Continue;
    }
    Call_StartForward(g_HanKnifeTrace);
    Call_PushCell(client);
    Call_PushCell(weapon);
    Call_PushCell(g_Knife[client].AttackId);
    Call_Finish();
    g_KnifeInForward[client] = false;
    // A consumer may send a different TE in a forward; restore the current TE
    // so subsequent PlayerAnimEvent hooks still read their own fields.
    TE_Start(name);
    if (g_KnifeDebug.BoolValue)
        LogMessage("[Han knife] trace tick=%d client=%d id=%d type=%d range=%.3f target=%d native-estimate=%d group=%d valid=%d", g_KnifeTick[client], client,
            g_Knife[client].AttackId, event, range, victim, EntRefToEntIndex(g_KnifeOriginalRef[client]),
            g_Knife[client].HitGroup, g_Knife[client].HitGroupValid);
    return Plugin_Continue;
}

bool Knife_CurrentDamage(int attacker, int inflictor, int type)
{
    if (attacker < 1 || attacker > MaxClients || inflictor != attacker || !IsClientInGame(attacker))
        return false;
    if (!g_KnifePending[attacker] || g_KnifeTick[attacker] != GetGameTickCount())
        return false;
    if ((type & (DMG_BULLET | DMG_NEVERGIB)) != (DMG_BULLET | DMG_NEVERGIB) || (type & DMG_BLAST))
        return false;
    return GetEntPropEnt(attacker, Prop_Send, "m_hActiveWeapon") == EntRefToEntIndex(g_Knife[attacker].WeaponRef);
}

public Action Knife_TraceAttack(int victim, int &attacker, int &inflictor, float &damage, int &type, int &ammo, int hitbox, int hitgroup)
{
    if (!Knife_CurrentDamage(attacker, inflictor, type) || g_KnifeApplying[attacker])
        return Plugin_Continue;
    if (g_KnifeInForward[attacker])
    {
        damage = 0.0;
        return Plugin_Handled;
    }
    bool same = EntIndexToEntRef(victim) == g_Knife[attacker].EntityRef;
    if (same)
        g_KnifeNativeSeen[attacker] = true; // An attempted native hit is enough; never retry blocked damage.
    if (g_Knife[attacker].RangeChanged && !same)
    {
        damage = 0.0;
        return Plugin_Handled; // before TraceAttack blood / hitgroup / multi-damage
    }
    return Plugin_Continue;
}

// Called at the top of the existing player damage hook and the new object hook.
Action Knife_Damage(int victim, int attacker, int inflictor, float &damage, int type)
{
    if (!Knife_CurrentDamage(attacker, inflictor, type))
        return Plugin_Continue;
    if (g_KnifeInForward[attacker])
    {
        damage = 0.0;
        return Plugin_Handled;
    }
    bool same = EntIndexToEntRef(victim) == g_Knife[attacker].EntityRef;
    if (!same)
    {
        if (!g_Knife[attacker].RangeChanged)
            return Plugin_Continue;
        damage = 0.0;
        return Plugin_Handled;
    }
    if (!g_KnifeApplying[attacker])
        g_KnifeNativeSeen[attacker] = true;
    if (g_KnifeDamageCalled[attacker])
    {
        damage = 0.0;
        return Plugin_Handled;
    }
    if (g_KnifeHostage[attacker] && HasEntProp(victim, Prop_Data, "m_iHealth"))
        g_KnifeHostageHealth[attacker] = GetEntProp(victim, Prop_Data, "m_iHealth");
    g_KnifeDamageCalled[attacker] = true;
    g_Knife[attacker].DamageEntered = true;
    Action action;
    float adjusted = damage;
    int attackId = g_Knife[attacker].AttackId;
    g_KnifeInForward[attacker] = true;
    Call_StartForward(g_HanKnifeDamage);
    Call_PushCell(attacker);
    Call_PushCell(EntRefToEntIndex(g_Knife[attacker].WeaponRef));
    Call_PushCell(g_Knife[attacker].AttackId);
    Call_PushCell(victim);
    Call_PushCell(g_KnifeApplying[attacker]);
    Call_PushFloatRef(adjusted);
    Call_Finish(action);
    g_KnifeInForward[attacker] = false;
    if (!Knife_RecordValid(attacker, attackId))
    {
        damage = 0.0;
        return Plugin_Handled;
    }
    if (g_KnifeDebug.BoolValue)
        LogMessage("[Han knife] damage tick=%d id=%d victim=%d supplemental=%d damage=%.2f action=%d", GetGameTickCount(),
            g_Knife[attacker].AttackId, victim, g_KnifeApplying[attacker], adjusted, action);
    if (action >= Plugin_Handled)
    {
        damage = 0.0;
        return Plugin_Handled;
    }
    if (action == Plugin_Changed)
    {
        damage = (adjusted >= 0.0 && adjusted < 1000000.0) ? adjusted : 0.0;
        return Plugin_Changed;
    }
    return Plugin_Continue;
}

public Action Knife_ObjectDamage(int victim, int &attacker, int &inflictor, float &damage, int &type)
{
    return Knife_Damage(victim, attacker, inflictor, damage, type);
}

// Both native and synthetic damage reach this hook (bypassHooks=false).
// Compare health immediately after application, including lethal damage.
public void Knife_ObjectDamagePost(int victim, int attacker, int inflictor, float damage, int type)
{
    if (Knife_CurrentDamage(attacker, inflictor, type) && !g_KnifeInForward[attacker]
        && Knife_RecordValid(attacker, g_Knife[attacker].AttackId))
        Knife_HostageBlood(attacker, victim);
}

void Knife_HostageBlood(int client, int victim)
{
    if (!g_KnifeHostage[client] || g_KnifeHostageBlood[client]
        || !g_KnifeDamageCalled[client] || g_KnifeHostageHealth[client] <= 0
        || !Knife_IsHostage(victim) || EntIndexToEntRef(victim) != g_Knife[client].EntityRef)
        return;
    int lost = g_KnifeHostageHealth[client] - GetEntProp(victim, Prop_Data, "m_iHealth");
    if (lost <= 0)
        return;
    g_KnifeHostageBlood[client] = true;
    float point[3], start[3], normal[3];
    for (int i = 0; i < 3; i++)
    {
        point[i] = g_Knife[client].Position[i];
        start[i] = g_Knife[client].Start[i];
        normal[i] = -g_Knife[client].Direction[i];
    }
    Knife_Dispatch(g_KnifeBloodEffect, victim, 0, 0, start, point, normal, g_KnifeAngles[client], float(lost), true);
}

public void Knife_PlayerHurt(Event event, const char[] name, bool dontBroadcast)
{
    int attacker = GetClientOfUserId(event.GetInt("attacker"));
    int victim = GetClientOfUserId(event.GetInt("userid"));
    if (attacker < 1 || victim < 1 || !g_KnifePending[attacker] || !g_KnifeDamageCalled[attacker]
        || g_KnifeTick[attacker] != GetGameTickCount() || EntIndexToEntRef(victim) != g_Knife[attacker].EntityRef)
        return;
    char weapon[64];
    event.GetString("weapon", weapon, sizeof(weapon));
    // CS:S emits the custom classname too; identity is established by the damage context.
    int active = EntRefToEntIndex(g_Knife[attacker].WeaponRef);
    char classname[64];
    if (active <= 0 || !IsValidEntity(active))
        return;
    GetEntityClassname(active, classname, sizeof(classname));
    if (!StrEqual(weapon, "knife") && !StrEqual(weapon, classname) && !StrEqual(weapon, classname[7]))
        return;
    g_Knife[attacker].HealthDamage += event.GetInt("dmg_health");
}

public void Knife_PostThinkPost(int client)
{
    if (!g_KnifePending[client])
        return;
    if (!Knife_RecordValid(client, g_Knife[client].AttackId) || g_KnifeTick[client] != GetGameTickCount())
    {
        Knife_Reset(client);
        return;
    }
    int weapon = EntRefToEntIndex(g_Knife[client].WeaponRef);
    int victim = EntRefToEntIndex(g_Knife[client].EntityRef);
    int attackId = g_Knife[client].AttackId;
    // The reference trace is an extra conservative check when a third-party hook
    // consumes the native trace before our callback: same target is NEVER retried.
    if (g_Knife[client].RangeChanged && !g_KnifeNativeSeen[client]
        && g_KnifeOriginalRef[client] != g_Knife[client].EntityRef
        && victim > 0 && IsValidEntity(victim) && HasEntProp(victim, Prop_Data, "m_takedamage")
        && GetEntProp(victim, Prop_Data, "m_takedamage") != 0)
    {
        if (victim > MaxClients || (IsClientInGame(victim) && IsPlayerAlive(victim)))
        {
            float force[3], point[3];
            for (int i = 0; i < 3; i++)
            {
                force[i] = g_Knife[client].Direction[i] * 300.0 * (g_KnifePushScale != null ? g_KnifePushScale.FloatValue : 1.0);
                point[i] = g_Knife[client].Position[i];
            }
            // TakeDamage skips TraceAttack. Clear its stale armor/hitgroup input;
            // precision hitgroups stay in our API and do not add engine head multipliers.
            if (HasEntProp(victim, Prop_Data, "m_LastHitGroup"))
                SetEntProp(victim, Prop_Data, "m_LastHitGroup", 0);
            g_KnifeApplying[client] = true;
            g_Knife[client].Supplemental = true;
            SDKHooks_TakeDamage(victim, client, client, g_KnifeBaseDamage[client], DMG_BULLET | DMG_NEVERGIB,
                weapon, force, point, false);
            g_KnifeApplying[client] = false;
            if (!Knife_RecordValid(client, attackId))
                return;
            if (g_Knife[client].HealthDamage > 0)
                Knife_Blood(client);
        }
    }
    // No new cooldown writer: the engine and existing firespeed retain ownership.
    // Smack's saved native contact may now be outside the configured range.
    // This networked field schedules feedback only; damage has already happened.
    if ((g_Knife[client].RangeChanged || g_KnifeHostage[client]) && HasEntProp(weapon, Prop_Send, "m_flSmackTime"))
        SetEntPropFloat(weapon, Prop_Send, "m_flSmackTime", -1.0);
    Knife_ScheduleFeedback(client);
    g_Knife[client].Finished = true;
    g_KnifePending[client] = false;
    g_KnifeInForward[client] = true;
    Call_StartForward(g_HanKnifeFinished);
    Call_PushCell(client);
    Call_PushCell(weapon);
    Call_PushCell(g_Knife[client].AttackId);
    Call_Finish();
    g_KnifeInForward[client] = false;
    if (g_KnifeDebug.BoolValue)
        LogMessage("[Han knife] finish tick=%d id=%d native=%d supplemental=%d health=%d", GetGameTickCount(),
            g_Knife[client].AttackId, g_KnifeNativeSeen[client], g_Knife[client].Supplemental, g_Knife[client].HealthDamage);
}

bool Knife_RecordValid(int client, int id)
{
    if (client < 1 || client > MaxClients || id <= 0 || !IsClientInGame(client) || !IsPlayerAlive(client))
        return false;
    if (g_Knife[client].AttackId != id || GetGameTime() - g_Knife[client].Time > 2.0)
        return false;
    int weapon = EntRefToEntIndex(g_Knife[client].WeaponRef);
    return weapon > MaxClients && GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon") == weapon;
}

// EffectDispatch uses the stock client material logic, including static-prop hitbox IDs.
void Knife_Dispatch(int effect, int entity, int surface, int hitbox, const float start[3], const float point[3],
    const float normal[3], const float angles[3], float magnitude, bool blood)
{
    if (effect < 0)
        return;
    TE_Start("EffectDispatch");
    TE_WriteFloat("m_vOrigin[0]", point[0]);
    TE_WriteFloat("m_vOrigin[1]", point[1]);
    TE_WriteFloat("m_vOrigin[2]", point[2]);
    TE_WriteFloat("m_vStart[0]", start[0]);
    TE_WriteFloat("m_vStart[1]", start[1]);
    TE_WriteFloat("m_vStart[2]", start[2]);
    TE_WriteAngles("m_vAngles", angles);
    TE_WriteVector("m_vNormal", normal);
    TE_WriteNum("m_fFlags", blood ? 0 : 1);
    TE_WriteFloat("m_flMagnitude", magnitude);
    TE_WriteFloat("m_flScale", 1.0);
    TE_WriteNum("m_nAttachmentIndex", 0);
    TE_WriteNum("m_nSurfaceProp", surface);
    TE_WriteNum("m_iEffectName", effect);
    TE_WriteNum("m_nMaterial", 0);
    TE_WriteNum("m_nDamageType", DMG_SLASH);
    TE_WriteNum("m_nHitBox", hitbox);
    TE_WriteNum("entindex", entity);
    TE_WriteNum("m_nColor", 0);
    TE_WriteFloat("m_flRadius", 0.0);
    if (TE_IsValidProp("m_bCustomColors"))
        TE_WriteNum("m_bCustomColors", 0);
    if (TE_IsValidProp("m_bControlPoint1"))
        TE_WriteNum("m_bControlPoint1", 0);
    TE_SendToAllInRange(point, RangeType_Audibility);
}

void Knife_Blood(int client)
{
    int victim = EntRefToEntIndex(g_Knife[client].EntityRef);
    if (victim < 1 || victim > MaxClients || !IsClientInGame(victim))
        return;
    float point[3], start[3], normal[3];
    for (int i = 0; i < 3; i++)
    {
        point[i] = g_Knife[client].Position[i];
        start[i] = g_Knife[client].Start[i];
        normal[i] = -g_Knife[client].Direction[i];
    }
    Knife_Dispatch(g_KnifeBloodEffect, victim, 0, 0, start, point, normal, g_KnifeAngles[client],
        float(g_Knife[client].HealthDamage), true);
}

bool Knife_SuppressSound(int weapon, const char[] sample)
{
    if (weapon <= MaxClients || !IsValidEntity(weapon) || StrContains(sample, "weapons/knife/") != 0)
        return false;
    if (StrContains(sample, "knife_hit") == -1 && StrContains(sample, "knife_stab") == -1 && StrContains(sample, "knife_slash") == -1)
        return false;
    if (!HasEntProp(weapon, Prop_Send, "m_hOwnerEntity"))
        return false;
    int client = GetEntPropEnt(weapon, Prop_Send, "m_hOwnerEntity");
    return client > 0 && client <= MaxClients && EntRefToEntIndex(g_KnifeSoundWeapon[client]) == weapon
        && GetGameTime() <= g_KnifeSoundUntil[client];
}

void Knife_PlaySound(int client, int weapon, int kind)
{
    char script[32], sample[PLATFORM_MAX_PATH], prefix[64], classname[64];
    switch (kind)
    {
        case 0: strcopy(script, sizeof(script), "Weapon_Knife.Slash");
        case 1: strcopy(script, sizeof(script), "Weapon_Knife.Hit");
        case 2: strcopy(script, sizeof(script), "Weapon_Knife.Stab");
        default: strcopy(script, sizeof(script), "Weapon_Knife.HitWall");
    }
    int channel, level, pitch;
    float volume;
    if (!GetGameSoundParams(script, channel, level, volume, pitch, sample, sizeof(sample), weapon))
        return;
    GetEntityClassname(weapon, classname, sizeof(classname));
    if (GetClientTeam(client) == CS_TEAM_CT)
        GetCustomSoundPrefixct(classname, prefix, sizeof(prefix));
    else if (GetClientTeam(client) == CS_TEAM_T)
        GetCustomSoundPrefixt(classname, prefix, sizeof(prefix));
    if (prefix[0])
        ReplaceString(sample, sizeof(sample), "knife", prefix);
    PrecacheSound(sample, true);
    float origin[3];
    GetClientAbsOrigin(client, origin);
    int recipients[MAXPLAYERS], count = GetClientsInRange(origin, RangeType_Audibility, recipients, sizeof(recipients));
    // Emit from the player, as the existing prefix replacement does; do not re-enter weapon sound replacement.
    if (count > 0)
        EmitSound(recipients, count, sample, client, channel, level, SND_NOFLAGS, volume, pitch);
}

void Knife_ScheduleFeedback(int client)
{
    int weapon = EntRefToEntIndex(g_Knife[client].WeaponRef);
    bool replaceSound = g_Knife[client].RangeChanged || g_KnifeHostage[client];
    if (!g_Knife[client].Hit)
    {
        if (replaceSound)
            Knife_PlaySound(client, weapon, 0);
        return;
    }
    if (g_Knife[client].SurfaceFlags & SURF_SKY)
        return;
    bool stab = g_Knife[client].Type == HanKnife_Secondary;
    // Stock weapon_knife already supplies its own impact effect. Snapshot only
    // this visual decision; stock knives still receive all other knife support.
    char classname[64];
    GetEntityClassname(weapon, classname, sizeof(classname));
    bool supplementImpact = !StrEqual(classname, "weapon_knife");
    DataPack pack;
    CreateDataTimer(stab ? 0.2 : 0.1, Knife_FeedbackTimer, pack, TIMER_FLAG_NO_MAPCHANGE);
    pack.WriteCell(GetClientUserId(client));
    pack.WriteCell(g_Knife[client].WeaponRef);
    pack.WriteCell(g_Knife[client].EntityRef);
    pack.WriteCell(stab);
    pack.WriteCell(replaceSound);
    pack.WriteCell(g_KnifeHostage[client]);
    pack.WriteCell(supplementImpact);
    pack.WriteCell(g_Knife[client].SurfaceProps);
    pack.WriteCell(g_Knife[client].SurfaceFlags);
    pack.WriteCell(g_Knife[client].HitBox);
    for (int i = 0; i < 3; i++)
    {
        pack.WriteFloat(g_Knife[client].Start[i]);
        pack.WriteFloat(g_Knife[client].Position[i]);
        pack.WriteFloat(g_Knife[client].Normal[i]);
        pack.WriteFloat(g_KnifeAngles[client][i]);
    }
}

public Action Knife_FeedbackTimer(Handle timer, DataPack pack)
{
    pack.Reset();
    int client = GetClientOfUserId(pack.ReadCell());
    int weapon = EntRefToEntIndex(pack.ReadCell());
    int target = EntRefToEntIndex(pack.ReadCell());
    bool stab = view_as<bool>(pack.ReadCell());
    bool sound = view_as<bool>(pack.ReadCell());
    bool hostage = view_as<bool>(pack.ReadCell());
    bool supplementImpact = view_as<bool>(pack.ReadCell());
    int surface = pack.ReadCell(), flags = pack.ReadCell(), hitbox = pack.ReadCell();
    if (client < 1 || !IsPlayerAlive(client) || weapon <= MaxClients || target < 0
        || GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon") != weapon)
        return Plugin_Stop;
    float start[3], point[3], normal[3], angles[3];
    for (int i = 0; i < 3; i++)
    {
        start[i] = pack.ReadFloat();
        point[i] = pack.ReadFloat();
        normal[i] = pack.ReadFloat();
        angles[i] = pack.ReadFloat();
    }
    if (sound)
        Knife_PlaySound(client, weapon, (hostage || (target > 0 && target <= MaxClients)) ? (stab ? 2 : 1) : 3);
    if (supplementImpact && !hostage && !(flags & (SURF_SKY | SURF_NODRAW | SURF_HINT | SURF_SKIP)) && surface >= 0)
        Knife_Dispatch(g_KnifeSlashEffect, target, surface, hitbox, start, point, normal, angles, 0.0, false);
    return Plugin_Stop;
}
