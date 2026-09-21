# HanWeaponSystem [H-AN]

![Language](https://img.shields.io/badge/Language-English-blue) ![语言](https://img.shields.io/badge/语言-中文-red) ![Game](https://img.shields.io/badge/Game-CS%3A%20Source-yellow) ![Platform](https://img.shields.io/badge/SourceMod-1.12-orange)

[ 简体中文 ](README.md) | **[ English ]**

**8.2:** Configurable primary/secondary knife reach, line/hull tracing, stock material effects, cached hitgroup APIs, and managed-weapon/viewmodel-state APIs. 

Material feedback and hitgroup classification now run for every actual `CKnife` entity, including the unregistered stock knife.


**8.3:** Adjusted the refresh range for weapon-switching animations and implemented independent handling to reset animation progress 

when switching to custom models; refactored the trigger and cleanup logic for firing animation fixes to minimize unintended interference with draw, 

reload, and other animation sequences.

Added a standalone plugin: [HanVMCrossFix](https://github.com/H-AN/HanVMCrossFix).

This fixes animation issues caused by switching between v0 and v1 models; 

installing it alongside the main mod is recommended for a better experience.


If you like this plugin, you can support me in the following ways. Thank you!

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/Z8Z31PY52N)

---

## Introduction

HanWeaponSystem is a large-scale weapon system plugin for Counter-Strike: Source. It allows servers to **add fully custom weapons** — new models, animations and sounds — with a complete feature set: iron sights, side sights, weapon inspection, sprinting, empty/tactical reloads, a buy/sell menu and Bot support.

- Requirements: SourceMod 1.12 / Counter-Strike: Source
- No third-party library dependencies; compiling only requires the files in this repository

---

## Features

| Feature | Description |
|---|---|
| 🔫 Custom weapons | New V/W models, adjustable damage / recoil / fire rate / knockback |
| 🔊 Custom sounds | Fire / deploy / reload / inspect sounds, multi-sound random playback |
| 🎯 Iron sights | Right-click aiming with true FOV zoom (point_camera lens) |
| 🎯 Side sights | Middle-mouse triggered, an independent parameter set parallel to iron sights |
| 🔍 Inspection | F key inspect; sound audible only to the inspector and first-person spectators |
| 🏃 Sprinting | Shift triggered, with start boost, speed cap and screen shake |
| 🔄 Empty/tactical reload | Reload animation & duration takeover, frame-based sound config |
| 💣 Grenade system | Pin-pull sound, bounce/explode sound replacement, projectile model scaling |
| 🛒 Buy system | `sm_buy` / `sm_sell` to purchase and sell custom weapons |
| 🤖 Bot support | Bots auto-buy custom weapons, randomly aim / sprint / inspect |
| 🧩 Public API | Natives and GlobalForwards for other plugins |

---

## Installation

1. Put the compiled `.smx` from `plugins` into `addons/sourcemod/plugins/`
2. On first load, all default configs are generated in `addons/sourcemod/configs/HanWeaponSystem/`
3. Put custom weapon **weapon scripts** into the server's `cstrike/scripts/` folder
4. Model/sound downloads are managed automatically by the plugin

---

## Quick Start

1. Edit `configs/HanWeaponSystem/HanWeaponData.cfg` and add a weapon node following the in-file comments
2. Enable features as needed in ZoomData / InspectData / RunData / EmptyReloadData
3. Reload with `sm plugins reload` (configs are synced to sub-config files automatically)
4. Use the buy command in game to receive the weapon

### Default Controls

| Action | Key / Command | Notes |
|---|---|---|
| Buy weapon | `sm_buy` (configurable in BuyData.cfg) | Opens the buy menu |
| Sell weapon | `sm_sell` (configurable in BuyData.cfg) | Opens the sell menu |
| Iron sight | Right mouse button (IN_ATTACK2) | Requires `zoom 1`; press to toggle |
| Side sight | **Middle mouse button (IN_ATTACK3)** | Requires `sideaim 1`; **not bound by default — bind it manually** (see guide below) |
| Inspect | F key or `sm_inspect` | Requires `inspect 1`; F was originally the flashlight key (impulse 100), now taken over |
| Sprint | **Tap Shift** | Requires `run 1`: tapping triggers the sprint animation (with start/end transitions), then **keep holding forward to keep sprinting**; releasing forward, firing, switching or aiming ends it |
| Silent walk | **Hold Shift down** | Original silent-walk logic is preserved, independent from sprinting |

> Note: iron sights / side sights / inspection / sprinting are configured **per weapon**. Keys do nothing on weapons without the matching fields.

### Key Binding Guide

Side sights use middle mouse button (IN_ATTACK3), which CS:S does not bind by default. Bind it in the console:

```text
bind mouse3 +attack3
```

- Binding is permanent (saved to config.cfg);
- F key is bound to `impulse 100` (flashlight) by default; with this plugin it becomes inspect automatically, no extra setup;
- If a key seems dead, check in order:
  1. Does this weapon have the matching field enabled (zoom / sideaim / inspect / run);
  2. Did you run `bind mouse3 +attack3` for side sights;
  3. Are you mid-reload or mid-switch (some actions are temporarily suppressed).

---

## Configuration

All configs live in `addons/sourcemod/configs/HanWeaponSystem/`. Every file has full field comments at the top.

### HanWeaponData.cfg (main config, one node per weapon)

| Key | Description |
|---|---|
| `command` | Console command that gives this weapon |
| `classname` | Base entity classname used to create the weapon (e.g. weapon_ak47) |
| `useclassname` | Weapon script name this entity points to (same name as the script in scripts/) |
| `damage` | Damage adjust: `+40` additive or `x1.5` multiplicative |
| `recoil` | Recoil factor, 1.0 normal, higher = stronger, 0 = none |
| `firespeed` | Fire rate, format `left:right` (e.g. `+0.5:+0.6`) |
| `reloadspeed` | Reload speed multiplier, 1.0 normal |
| `knowback` | Hit knockback strength |
| `vmodel` / `wmodel` | View model / world model path |
| `firesound` | Fire sound(s), comma separated, random pick |
| `switchsound` | Deploy sound, same format |
| `wmodel_silencer` / `firesound_silencer` | Silencer variants (m4a1/usp templates only) |
| `customsoundct` / `customsoundt` | Knife sound folder replacement per team |
| `autofire` | 1 = hold right mouse for full-auto burst |
| `Sqtype` | Animation sequence sync mode (1 = manual) |
| `skincode` | Model skin index |
| `ammo` | Magazine ammo amount |
| `killicon` | Kill icon |
| `team` | Allowed team (all/ct/t) |
| `reloadsound` | Fallback reload sound (frame:path, comma separated segments) |
| `usedroppedmodel` | 1 = use fake W model when dropped |
| `armfix` | 1 = fix left arm model |
| `shootqcfixes` | 1 = fix missing fire animation (engine same-sequence no-restart issue) |
| `grenadescale` | Grenade projectile model scale (grenades only) |

### HanWeaponZoomData.cfg (iron sights / side sights)

| Key | Description |
|---|---|
| `zoom` | 1 = enable iron sight |
| `zoommodel` | Model shown while aiming |
| `zoomanimmove` / `zoomanimtime` | Transition animation index and duration (seconds) |
| `zoomanimfire` | 1 = do not force-sync fire animation while aiming (default 0 = sync) |
| `zoomfov` | Zoomed FOV (<90 enables, model needs _rt_camera lens material) |
| `zoomaccuracy` / `zoomspeed` / `zoomcrosshair` / `zoompunch` / `zoomskinadd` | Accuracy / speed / crosshair / screen punch / skin offset |
| `sideaim...` | Full parallel set of side-sight fields (sideaim prefix) |
| `*_silencer` | model/animmove/animtime/animfire support silencer variants |

### HanWeaponInspectData.cfg (inspection)

| Key | Description |
|---|---|
| `inspect` | 1 = enable inspection |
| `inspectseq` / `inspecttime` | Animation index and duration (frames, 30fps) |
| `inspectrepeat` | 1 = allow repeated F presses to restart from the beginning |
| `inspectflashlight` | 1 = suppress flashlight while inspecting |
| `inspectsound` | Inspect sound(s), comma separated random pick |
| `*_silencer` | seq/time/sound silencer variants |

### HanWeaponRunData.cfg (sprinting)

| Key | Description |
|---|---|
| `run` | 1 = enable sprinting |
| `runspeed` | Start boost (velocity added per frame, default 50) |
| `runmaxspeed` | Speed cap (500 ≈ 2x walk speed) |
| `runseq` / `runtime` | Sprint animation index and duration (anim 2 = index+1 automatically) |
| `runin` / `runinlength` | Optional idle→sprint transition |
| `runout` / `runoutlength` | Optional sprint→idle transition |
| `runshake` | Screen shake amplitude, 0 = off |
| Sequence keys `_silencer` | Silencer variants |

### HanWeaponEmptyReloadData.cfg (empty / tactical reload)

| Key | Description |
|---|---|
| `emptyreload` | 1 = enable empty reload (magazine empty) |
| `emptyreloadseq` / `emptyreloadtime` | Animation index and duration (frames) |
| `emptyreloadsound` | Sounds (frame:path, comma separated segments) |
| `tacticalreload` | 1 = enable tactical reload (magazine not empty) |
| `reloadseq` / `reloadtime` / `tacticalreloadsound` | Tactical reload fields |
| `reloadfps` | Animation FPS (default 30, use 24 etc. as needed) |
| All of the above support `_silencer` variants | m4a1/usp templates only, active while silencer is on |

#### Reload sound setup (HLMV frame alignment — important for servers)

The recommended way to add custom reload sounds is **frame alignment in HLMV** (the model viewer):

1. Open the weapon model in HLMV, play the reload animation and note the **frame numbers** of key actions (e.g. mag out at frame 10, new mag in at 45, bolt rack at 70);
2. Fill the config as `frame:sound path`, comma separated; the plugin plays each sound exactly at its frame during the reload takeover:

```text
"emptyreloadsound"   "10:weapons/mygun/magout.wav,45:weapons/mygun/magin.wav,70:weapons/mygun/bolt.wav"
```

3. `reloadfps` must match the animation's real FPS seen in HLMV (converted at 30fps by default), otherwise sounds drift out of sync.

**Why is this mandatory on servers?**

- **Local / single-player use**: the client automatically plays the custom sound events embedded in the model — no config needed;
- **Dedicated servers**: the engine never fires the model's embedded sound events (client/animation driven, unhookable server-side). You must use the **frame + sound** config so the plugin plays them server-side with precise timing.

The main config fallback `reloadsound` and the empty/tactical reload `emptyreloadsound` / `tacticalreloadsound` fields follow the same idea.

### HanWeaponBuyData.cfg (buy system)

| Key | Description |
|---|---|
| `buysystem` | 1 = enable buy system |
| `buymode` | always = buy anytime / buytime = limited to buy time |
| `buycommand` / `sellcommand` | Buy/sell commands (default sm_buy / sm_sell) |
| `name` | Menu display name |
| `buyable` / `price` | Purchasable flag and price (auto-read from weapon script if empty) |
| `sellable` / `sellratio` / `sellratio_other` | Sell flag and refund ratios |

### HanWeaponBotData.cfg (Bot system)

| Key | Description |
|---|---|
| `botsystem` | 1 = enable Bot weapon system (off by default) |
| `botmoney` | 1 = real money mode (deducts bot money via account selection) |
| `botaimcd` / `botruncd` / `botinspectcd` | Independent cooldowns per behavior (seconds) |
| `botaimtime` | Aim hold duration (seconds) |
| `botchance` | Bot purchase weight 0-100 |
| `botaim` / `botrun` / `botinspect` | Behavior trigger chance 0-100 |

---

## Server Commands

| Command | Description |
|---|---|
| `sm_buy` | Opens the buy menu (name configurable via BuyData.cfg `buycommand`) |
| `sm_sell` | Opens the sell menu (name configurable via BuyData.cfg `sellcommand`) |
| `sm_inspect` | Inspects the current weapon |
| Per-weapon give commands | Console command registered from each weapon's `command` field; typing it grants that weapon |

---

## Server ConVars

| ConVar | Default | Description |
|---|---|---|
| `han_wpsdisablebackweapon` | `0` | Back weapon models: `0` = enable fake back models (unequipped custom weapons shown on the back / leg); `1` = disabled, use the engine's native logic |
| `han_oldweaponfix` | `0` |Native weapons that are not custom weapons also use the 1st model (v): `0` = disabled by default; `1` = enabled. When enabled, the animation for switching between 1st and 1st models will never be lost, and it supports quick-switch logic from plugins such as fast melee attacks.|

Put it in `server.cfg` to persist, or switch it live from the console as an admin (changes apply instantly: disabling removes fake models and restores native ones on the next tick, re-enabling rebuilds automatically).

---

## Examples

### Example 1: Adding a special weapon

```text
"weapon_mygun"
{
    "command"        "sm_mygun"
    "classname"      "weapon_ak47"
    "useclassname"   "weapon_mygun"
    "team"           "all"
    "damage"         "+40"
    "vmodel"         "models/weapons/mygun/v_mygun.mdl"
    "wmodel"         "models/weapons/mygun/w_mygun.mdl"
    "firesound"      "weapons/mygun/fire1.wav,weapons/mygun/fire2.wav"
    "ammo"           "1000"
    "killicon"       "weapon_ak47"
}
```

Steps: put the weapon script into the server's `cstrike/scripts/weapon_mygun.txt`, add models/sounds to the download table — players then type `sm_mygun` to receive it.

### Example 2: Secret command anti-abuse (random command name + server-side granting)

A memorable name like `sm_mygun` lets anyone who learns it grab the weapon for free. Solution: **make the command a random string and only call it from the server side**.

1. Generate a random string on a site such as [suijimimashengcheng.bmcx.com](https://suijimimashengcheng.bmcx.com/), e.g. `hrhipN2bNeVW0PBz`;
2. Put it in the main config:

```text
"command"   "sm_hrhipN2bNeVW0PBz"
```

3. The command is never published, so players cannot guess it; normal acquisition still goes through the buy menu (the menu shows the configured display name, unaffected);
4. Other systems (supply crate pickups, quest reward plugins, etc.) grant the weapon server-side:

```sourcepawn
// Inside your pickup / reward trigger:
FakeClientCommand(client, "sm_hrhipN2bNeVW0PBz");
```

> Tip: `command` registers a real console command — always use a long enough random string to prevent brute-forcing.

### Example 3: Admin manual grant

Admins simply execute the weapon's give command from the console or chat (e.g. `sm_mygun`). The main config's `team` field restrictions still apply.

---

## Developer API

Other plugins can `#include <HanWeaponSystem>` to access all interfaces. Check the dependency before using:

```sourcepawn
public void OnAllPluginsLoaded()
{
    if (!LibraryExists("HanWeaponSystem"))
        SetFailState("Requires the HanWeaponSystem main plugin");
}
```

Full signatures are in `include/HanWeaponSystem.inc`. Overview:

**Natives**

| Interface | Purpose |
|---|---|
| `GetClientViewModel(client, index)` | Get a player's view model entity |
| `IsClientPressingAttack2(client)` | Whether the player holds right mouse (correct even if buttons were rewritten) |
| `IsWeaponAutoFire(client)` | Whether the current weapon supports auto burst |
| `Han_IsClientZooming(client)` / `Han_IsWeaponZoomable(client)` | Iron-sight state / capability query |
| `Han_IsClientSideAiming(client)` / `Han_IsWeaponSideAimable(client)` | Side-sight state / capability query |
| `Han_IsClientInspecting(client)` / `Han_IsWeaponInspectable(client)` | Inspection state / capability query |
| `Han_IsClientRunning(client)` | Whether the player is sprinting |
| `Han_SetClientCustomAnim(client, seq, frames, repeat, interruptable)` | Play a custom animation (highest priority; interruptible / unstoppable modes) |
| `Han_IsClientCustomAnim(client)` | Whether a custom animation is playing |
| `Han_StopClientCustomAnim(client)` | Stop the custom animation (broadcasts like a natural end) |

**Forwards**

| Interface | Fired when |
|---|---|
| `Han_OnClientZoom(client, bool zooming)` | Iron-sight state changes |
| `Han_OnClientSideAim(client, bool aiming)` | Side-sight state changes |
| `Han_OnClientInspect(client, bool inspecting)` | Inspection starts / ends |
| `Han_OnClientRun(client, bool running)` | Sprint starts / ends (incl. transitions) |
| `Han_OnClientEmptyReload(client)` | Empty reload starts (one-shot) |
| `Han_OnClientTacticalReload(client)` | Tactical reload starts (one-shot) |
| `Han_OnClientCustomAnimStart(client, seq, frames)` | Custom animation starts (incl. self-restart) |
| `Han_OnClientCustomAnimEnd(client)` | Custom animation ends (natural / interrupted / weapon switch / death) |

---

## v8.2 Features and API Usage

Hostage feedback is included in v8.2 without a version bump or additional configuration. Stock and custom knives use flesh-hit sounds for primary attacks and stab sounds for secondary attacks, retaining CT/T custom sound prefixes. A confirmed health decrease emits one additional blood effect at the cached contact point, while the plugin skips its wall-material effect for hostages. Hostage damage, money penalties, and the player hitgroup API are unchanged.

### New features

- **Knife reach:** `knife_range_primary` / `knife_range_secondary` control each attack independently. Detection starts at the eyes along the view direction, using a line followed by a hull on a miss. Native damage is reconciled with the configured trace: out-of-range hits are blocked and additional reach can deal supplemental damage without doubling overlapping hits.
- **Unified material feedback:** every actual knife-template entity (`CKnife`), including stock `weapon_knife`, receives stock `KnifeSlash` feedback. Supplemental hits that cause player health damage receive blood effects. Material selection uses surface properties, not model filenames.
- **Shared hitgroup results:** targets and hitgroups are captured during the attack and exposed for reuse. Precise classification only applies to the player already selected by the main trace; unavailable classification is explicitly unknown. No automatic headshot damage bonus is added.
- **Weapon identity and viewmodel state:** query registration, registered base class, and the confirmed display mode without maintaining a stock-weapon list.
- Existing animation, `firespeed`, `damage`, and older APIs remain available. No DHooks or additional gamedata dependency is introduced.

Add reach fields to an existing knife entry; keep its model and sound settings:

```text
"classname"             "weapon_knife"
"damage"                ""
"firespeed"             "0"
"knife_range_primary"   "80.0"
"knife_range_secondary" "60.0"
```

Omitted, empty or `0` reach values use stock **48 / 32**. Fractional values are supported within `(0, 8192]`; invalid values fall back and log an error. Reach is trace-path length, not the distance between player centers; stock hull tolerance still applies. Unregistered stock knives also expose hitgroup APIs and use stock reach.

`knife_effects` and `knife_hitgroups` are **retired and ignored, including values of 0**. Material feedback and hitgroup classification run without these settings. Companion plugins decide whether unknown hitgroups count as headshots and choose headshot/Bot multipliers.

### New natives: identity, display mode and attack snapshots

Compile with this repository's `include/HanWeaponSystem.inc` and run with HanWeaponSystem 8.2.

```sourcepawn
#include <sourcemod>
#include <sdktools>
#include <HanWeaponSystem>

native bool Han_IsManagedWeapon(int weapon);
native bool Han_GetWeaponBaseClass(int weapon, char[] buffer, int maxlen);
native HanViewModelMode Han_GetClientViewModelMode(int client);
native int Han_GetLatestKnifeAttackId(int client);
native bool Han_GetKnifeAttackResult(int client, int attackId,
    HanKnifeResult result, int size = sizeof(HanKnifeResult));
```

The block above is a signature reference. Do not redeclare these natives in your plugin after including the header.

| Native | Purpose and return contract |
|---|---|
| `Han_IsManagedWeapon` | Whether the entity matches a configured `useclassname`; an unregistered stock knife may return false while still emitting knife API notifications |
| `Han_GetWeaponBaseClass` | Reads the registered `classname`, such as `weapon_knife`; returns false and clears the string when unregistered, without guessing third-party templates |
| `Han_GetClientViewModelMode` | `HanViewModel_VM0=0`, `HanViewModel_VM1=1`, or `HanViewModel_NotReady=-1`; incomplete switches, death or missing models return NotReady |
| `Han_GetLatestKnifeAttackId` | Current valid attack ID, or 0 when unavailable; useful for explicit queries |
| `Han_GetKnifeAttackResult` | Copies the snapshot for a specific ID without tracing again; failure clears the output. Normally omit the final size argument |

Registration and display mode are independent: a stock weapon can use VM1 through `han_oldweaponfix` while remaining unregistered. Queries have no visibility side effects. Do not interpret NotReady as VM0 or use it to force VM1 visible.

### New forwards: consume results at the right stage

```sourcepawn
forward void Han_OnKnifeAttack(int client, int weapon, int attackId,
    HanKnifeAttackType type);
forward void Han_OnKnifeTraceResult(int client, int weapon, int attackId);
forward Action Han_OnKnifeDamage(int client, int weapon, int attackId,
    int victim, bool supplemental, float &damage);
forward void Han_OnKnifeAttackFinished(int client, int weapon, int attackId);
```

| Forward | Timing and purpose |
|---|---|
| `Han_OnKnifeAttack` | The snapshot is ready; initialize or advance combos. `HanKnife_Primary=0` means left click, `HanKnife_Secondary=1` means right click; these are not the raw PlayerAnimEvent values |
| `Han_OnKnifeTraceResult` | Publishes geometry after Attack, including misses, world, props and players; a geometric hit does not prove damage |
| `Han_OnKnifeDamage` | Before health deduction and the main configuration's damage adjustment; shared by native and supplemental damage, with `supplemental=true` identifying the latter |
| `Han_OnKnifeAttackFinished` | Settlement has finished in the same PostThinkPost; read outcomes here, not modify damage that already occurred |

```text
Trace and cache → Attack → TraceResult
                        → Eligible damage: Damage → configured damage → game damage processing
                        → Finished (misses or blocked attacks may have no Damage notification)
```

These are synchronous plugin callbacks. Querying the native inside `Han_OnKnifeDamage` returns the current snapshot immediately; no Timer or `player_hurt` wait is needed. Return `Plugin_Continue` to retain damage, `Plugin_Changed` to apply an edit, or `Plugin_Handled` / `Plugin_Stop` to block it. Do not deal a second hit or recursively invoke damage from this callback.

### Result fields and lifetime

Receive data in `HanKnifeResult result;`:

| Fields | Meaning |
|---|---|
| `AttackId`, `WeaponRef`, `Type`, `Time`, `Range` | Attack ID, weapon reference, attack type, game time and path length |
| `Hit`, `EntityRef` | Geometric hit and target reference; world is 0, a miss is -1 |
| `Start[3]`, `Direction[3]`, `Position[3]`, `Normal[3]` | Start, direction, contact point and surface normal; surface fields are not a valid contact on a miss |
| `SurfaceProps`, `SurfaceFlags`, `HitBox` | Main trace surface properties, flags and hitbox information; static props may use world plus a hitbox ID |
| `HitGroup`, `HitGroupValid` | Precise classification and validity; valid group 1 is a confirmed head hit. Unknown is 0 and does not automatically mean headshot |
| `RangeChanged`, `DamageEntered`, `Supplemental` | Changed reach, entered damage callback, attempted supplemental damage; none alone proves health loss |
| `HealthDamage`, `Finished` | Player health damage reported by `player_hurt`, and completed settlement; this field does not report prop health changes |

`WeaponRef` / `EntityRef` are EntRefs. Resolve them with `EntRefToEntIndex` and validate before operating on entities. A new attack, switch, attacker death, disconnect or map change invalidates the old record; records last at most 2 seconds. Victim death does not erase the attacker's snapshot. An expired record is different from a valid hit with an unknown hitgroup; never reuse stale data after a failed query.

### Complete example: inspect state and adjust damage before deduction

v8.2 api example.

```sourcepawn

public Action ShowWeaponState(int client, int args)
{
    if (client < 1 || !IsClientInGame(client) || !IsPlayerAlive(client))
        return Plugin_Handled;
    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    char base[64];
    bool hasBase = Han_GetWeaponBaseClass(weapon, base, sizeof(base));
    ReplyToCommand(client, "managed=%d baseKnown=%d base=%s VM=%d",
        Han_IsManagedWeapon(weapon), hasBase, base, Han_GetClientViewModelMode(client));

    HanKnifeResult result;
    int id = Han_GetLatestKnifeAttackId(client);
    if (id > 0 && Han_GetKnifeAttackResult(client, id, result))
        ReplyToCommand(client, "attack=%d hit=%d group=%d valid=%d",
            id, result.Hit, result.HitGroup, result.HitGroupValid);
    return Plugin_Handled;
}

public void Han_OnKnifeAttack(int client, int weapon, int attackId, HanKnifeAttackType type)
{
    // Advance your configured combo here; do not also consume PlayerAnimEvent.
    PrintToServer("knife attack=%d client=%d secondary=%d",
        attackId, client, type == HanKnife_Secondary);
}

public void Han_OnKnifeTraceResult(int client, int weapon, int attackId)
{
    HanKnifeResult result;
    if (!Han_GetKnifeAttackResult(client, attackId, result))
        return;
    int target = EntRefToEntIndex(result.EntityRef);
    PrintToServer("trace attack=%d hit=%d target=%d", attackId, result.Hit, target);
}

public Action Han_OnKnifeDamage(int client, int weapon, int attackId,
    int victim, bool supplemental, float &damage)
{
    if (victim < 1 || victim > MaxClients || !IsClientInGame(victim))
        return Plugin_Continue;
    if (weapon <= MaxClients || !IsValidEntity(weapon))
        return Plugin_Continue;
    char classname[64];
    GetEntityClassname(weapon, classname, sizeof(classname));
    if (!StrEqual(classname, "weapon_example_knife"))
        return Plugin_Continue;

    HanKnifeResult result;
    if (!Han_GetKnifeAttackResult(client, attackId, result)
        || EntRefToEntIndex(result.WeaponRef) != weapon
        || EntRefToEntIndex(result.EntityRef) != victim)
        return Plugin_Continue;

    if (result.HitGroupValid && result.HitGroup == 1)
    {
        damage *= 2.0; // Applies to native AND supplemental hits before health is deducted.
        return Plugin_Changed;
    }
    return Plugin_Continue;
}

public void Han_OnKnifeAttackFinished(int client, int weapon, int attackId)
{
    HanKnifeResult result;
    if (Han_GetKnifeAttackResult(client, attackId, result))
        PrintToServer("finished attack=%d healthDamage=%d supplemental=%d",
            attackId, result.HealthDamage, result.Supplemental);
}
```

### Migration and limits

- Replace combo-driving `PlayerAnimEvent` handling with `Han_OnKnifeAttack`; do not consume both.
- Move special damage calculation to `Han_OnKnifeDamage`. Supplemental damage does not invoke legacy `SDKHook_TraceAttack`. Remove the old damage modifier after fully migrating to avoid applying multipliers twice.
- A companion intentionally treating unknown hitgroups as headshots may use `!result.HitGroupValid || result.HitGroup == 1`. This is its own gameplay policy, not the main system's definition.
- Bounded classification retains obstacles and may differ from an old infinite victim-only ray.
- Empty `damage` adds no main-system adjustment; populated values still apply after the damage forward. The engine and existing `firespeed` retain cooldown control; a changed custom hit/miss result does not introduce a separate cooldown writer.
- Use `han_knife_debug 1` for attack/damage/settlement logs, then restore 0. Duplicate observer decals are allowed. Special collision behavior, glass-trigger damage before the animation event, and supplemental TraceBleed .

## FAQ

**Q: What is the minimum config to add a new weapon?**
A: Just `classname` + `useclassname` + `vmodel` + `wmodel` in the main config. Everything else is optional.

**Q: Why do I hear two overlapping gun sounds?**
A: Make sure `firesound` is filled. Weapons with a configured fire sound automatically block the original engine sound so only the custom sound plays.

**Q: Config changes are not taking effect?**
A: Reload with `sm plugins reload`, then have players re-acquire the weapon (caches refresh on weapon switch).

**Q: Side sight does nothing when I press middle mouse?**
A: Middle mouse is unbound by default — run `bind mouse3 +attack3` in the console first, and make sure this weapon has `sideaim 1` plus a side-sight model configured.

**Q: Pressing F inspects instead of toggling the flashlight?**
A: That is intended. The F key (impulse 100) is taken over by inspection; inspection animation and sounds only apply to weapons with `inspect 1`, other weapons keep the original flashlight behavior.

**Q: No custom weapon model shown on my back / leg?**
A: Make sure `han_wpsdisablebackweapon` is `0` (default) and the weapon's `wmodel` is configured; the weapon you are currently holding never shows a back model.

---

## Support the Author

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/Z8Z31PY52N)
