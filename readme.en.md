# HanWeaponSystem [H-AN]

![Language](https://img.shields.io/badge/Language-English-blue) ![语言](https://img.shields.io/badge/语言-中文-red) ![Game](https://img.shields.io/badge/Game-CS%3A%20Source-yellow) ![Platform](https://img.shields.io/badge/SourceMod-1.12-orange)

[ 简体中文 ](README.md) | **[ English ]**

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
