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

| Action | Key / Command |
|---|---|
| Buy weapon | `sm_buy` (configurable in BuyData.cfg) |
| Sell weapon | `sm_sell` (configurable in BuyData.cfg) |
| Iron sight | Right mouse button |
| Side sight | Middle mouse button |
| Inspect | F key |
| Sprint | Shift |

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

## FAQ

**Q: What is the minimum config to add a new weapon?**
A: Just `classname` + `useclassname` + `vmodel` + `wmodel` in the main config. Everything else is optional.

**Q: Why do I hear two overlapping gun sounds?**
A: Make sure `firesound` is filled. Weapons with a configured fire sound automatically block the original engine sound so only the custom sound plays.

**Q: Config changes are not taking effect?**
A: Reload with `sm plugins reload`, then have players re-acquire the weapon (caches refresh on weapon switch).

---

## Support the Author

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/Z8Z31PY52N)
