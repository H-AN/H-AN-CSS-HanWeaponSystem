# 交接文档：HanWeaponSystem v6.1（检视系统完成）

> 作者/交接人：华仔 H-AN
> 日期：2026-08-17
> 目的：记录 v6.0 → v6.1 新增「武器检视」功能的完整现状、架构、配置与坑，为后续「检视音效 / 持枪奔跑 / 空仓换弹」等动画序列类功能的整合提供上下文。
> 说明：本版本在 v6.0 交接文档（`交接文档-HanWeaponSystem-v6-机瞄移植完成.md`）基础上追加，机瞄部分架构不变，请同时参考 v6.0 文档。

---

## 1. 插件总览

| 项 | 值 |
|---|---|
| 插件名 | `[华仔]武器系统` |
| 主文件 | `addons/sourcemod/scripting/[H-AN_CSS]HanWeaponSystem v.6.1.sp` |
| 版本宏 | `VERSION "6.1"`（`HanWeaponSystemGlobals.inc` 顶部） |
| 作者 | 华仔 H-AN，QQ 407866133，github https://github.com/H-AN |
| 产物 | `scripting/compiled/[H-AN_CSS]HanWeaponSystem v.6.1.smx` |
| 编译 | `scripting/compile.exe "[H-AN_CSS]HanWeaponSystem v.6.1.sp"`（当前零警告零报错） |
| 运行框架 | SourceMod 1.12 / Counter-Strike: Source（经典句柄式 SDKHooks + EntityOutput 全可用） |
| 仓库 | github `H-AN-CSS-HanWeaponSystem`（v6.1 已提交；开发在本地工作环境，改完才同步仓库） |

### 1.1 文件结构

主 .sp 只做组装，逻辑全部拆到 `HanWeaponSystem/` 目录下 **12 个 inc**，按依赖顺序 include：

```
[H-AN_CSS]HanWeaponSystem v.6.1.sp
├── HanWeaponSystemGlobals.inc      全局变量/宏/VERSION
├── HanWeaponSystemConfig.inc       主武器配置 HanWeaponData.cfg + 指令 + 击杀事件
├── HanWeaponSystemSounds.inc       武器音效 + 霰弹枪火光 TE
├── HanWeaponSystemCamera.inc       机瞄镜头 point_camera/info_camera_link
├── HanWeaponSystemZoomConfig.inc   机瞄独立配置 HanWeaponZoomData.cfg（主配置自动同步）
├── HanWeaponSystemZoom.inc         机瞄核心（触发/过渡/模型/特效）
├── HanWeaponSystemInspectConfig.inc 检视独立配置 HanWeaponInspectData.cfg（主配置自动同步）★新增
├── HanWeaponSystemInspect.inc      检视核心（F键/命令触发 + 动画强制 + 手电屏蔽）★新增
├── HanWeaponSystemWeapon.inc       武器给予 + 假世界模型 + 传输拦截
├── HanWeaponSystemCombat.inc       后坐力/攻速/换弹速度 + OnPlayerRunCmd 总入口
├── HanWeaponSystemViewModel.inc    双 VM 模型驱动（每 tick 心脏）
└── HanWeaponSystemAPI.inc          对外 API（native + GlobalForward + 插件库）
```

依赖顺序：`Globals → Config → Sounds → Camera → ZoomConfig → Zoom → InspectConfig → Inspect → Weapon → Combat → ViewModel → API`。

### 1.2 配置文件（运行时自动生成）

- `addons/sourcemod/configs/HanWeaponSystem/HanWeaponData.cfg`：武器主配置
- `addons/sourcemod/configs/HanWeaponSystem/HanWeaponZoomData.cfg`：机瞄配置（自动同步）
- `addons/sourcemod/configs/HanWeaponSystem/HanWeaponInspectData.cfg`：**检视配置（v6.1 新增）**，与主配置自动同步——主配置新增武器时自动追加默认检视键组（`InspectConfig_OnMapStart` → `SyncInspectConfig`）。**默认全部关闭检视**。

---

## 2. 检视功能完整实现记录（v6.1 新增）

来源：旧插件 `Run LookAt01 Action`（作者 cjsrk，v1.0）。已按惯例在两个检视 inc 顶部加双语 credit 注释。参考的 `Run Lookat01.sp` 仍在 `scripting/` 下，仅作对照。

### 2.1 配置字段（HanWeaponInspectData.cfg 每个武器键组）

| 键 | 含义 |
|---|---|
| `inspect` | 0=关闭检视（默认） 1=启用 |
| `inspectseq` | 检视动作动画序列号 |
| `inspecttime` | 检视动作时长，**单位帧（30fps）**，与原 Run Lookat01 配置兼容（如 230 ≈ 7.7 秒） |
| `inspectrepeat` | 0=检视一次播完为止（默认） 1=允许反复按 F 从动画开头重播（好玩向） |
| `inspectflashlight` | 0=检视时手电筒正常发光（默认） 1=检视时屏蔽手电筒光 |

> **单位决策（重要）**：`inspecttime` 用**帧**而非秒，因为原插件配置直接是帧（`RLWeaponsActTime[num]/30.0` 换算），用帧可让老玩家原样迁移配置（`<230>` 直接填 `230`），无需换算。

### 2.2 触发方式（两种）

1. **命令**：`sm_inspect`（`Cmd_Inspect`，Inspect.inc:49）。已确认用 `sm_inspect` 而非原插件的 `sm_lookat`，方便玩家绑定按键。
2. **F 键**：`impulse 100` 检测（`Inspect_OnPlayerRunCmd`，Inspect.inc:87）。

### 2.3 F 键检测原理（重要坑，务必记住）

- `impulse` 是客户端本地命令，`AddCommandListener("impulse")` **抓不到**（不发服务器命令）。
- 但 usercmd 的 `impulse` 字段**会随 OnPlayerRunCmd 上行**，所以必须用 `SDKHook_OnPlayerRunCmd` 的 `int &impulse` 参数检测 `impulse == 100` 的**上升沿**（`static s_bPrevImpulse100`）。
- 实测：按 F 时引擎切换玩家 `m_fEffects` 的 `0x0 <-> 0x4`（`EF_FLASHLIGHT`，手电筒开关）。手电状态不影响检视触发，只在 `inspectflashlight 1` 时屏蔽其发光。

### 2.4 触发门槛（与机瞄一致）

`Inspect_Start`（Inspect.inc:136）之前的一系列拦截（命令与 F 键共用逻辑）：

- 机瞄中/过渡中（`g_bZooming || g_iZoomInSwitch != 0`）→ 禁止检视（互斥）。
- 单次模式检视中重复触发 → 忽略；重复模式 → 允许重播。
- 换弹/换枪（`m_flNextAttack`，`bReloading`）期间 → 禁止；**无弹药特殊武器豁免**（`Zoom_IsNoAmmoWeapon`，与机瞄门槛一致）。
- 非重复模式有 0.5s 防抖冷却 `g_fInspectBlock`；重复模式不限冷却支持快速连按。

### 2.5 检视状态机与结束条件

全局（Globals.inc）：

- `g_bInspecting[MAXPLAYERS+1]`：是否检视中
- `g_iInspectWeapon[MAXPLAYERS+1]`：检视开始时武器实体
- `g_fInspectEnd[MAXPLAYERS+1]`：结束时间（`GetGameTime() + dur`）
- `g_fInspectBlock[MAXPLAYERS+1]`：触发防抖时间
- `g_iInspectRestart[MAXPLAYERS+1]`：重复检视重启标记（2=播一帧原生序列 → 1=下帧切回检视序列 → 0）

`Inspect_OnPostThinkPost`（Inspect.inc:205，由 ViewModel 每 tick 调用）结束条件：

1. 死亡 / 无武器 / 武器变化（`weapon != g_iInspectWeapon`）
2. 机瞄开启（`g_bZooming || g_iZoomInSwitch`）→ 机瞄优先强制退检视
3. 开火（`IN_ATTACK`）→ 取消（否则看不到射击动画）
4. 换弹（`Weapon_IsReloading` / `m_reloadState`）
5. 时长到（`GetGameTime() >= g_fInspectEnd`）

满足任一即 `Inspect_End`。

### 2.6 动画强制与两个起源引擎坑

**坑 A：起源引擎设置同一序列不会重播**。检视结束后 VM[1] 若停在检视序列最后一帧，下次设同一序列号不会从头播。解决：

- 非重复模式：`Inspect_End`（Inspect.inc:178）把 VM[1] 序列恢复为 VM[0] 的引擎原生序列（idle），下次切到检视序列即可重播。
- 重复模式：`Inspect_Start` 设 `g_iInspectRestart=2`，两帧切换强制重播——先播一帧 VM[0] 原生序列，下帧切回检视序列。

**坑 B：`m_flCycle` 是 `Prop_Data`**，不是 `Prop_Send`（`GetEntPropFloat(ClientVM[0], Prop_Data, "m_flCycle")`，ViewModel.inc:108），否则报 `Property "m_flCycle" not found`。

动画强制：`Inspect_OnPostThinkPost` 在 ViewModel 的动画复制**之后**每 tick 覆盖 VM[1] 为 `inspectseq`、`m_flPlaybackRate=1.0`。同时 `SetEntPropFloat(weapon, "m_flTimeWeaponIdle", GetGameTime() + dur)` 压住武器空闲动画，避免中途插回 idle 打断检视。

### 2.7 手电屏蔽

`inspectflashlight 1` 时，每 tick 检测玩家 `m_fEffects` 的 `EF_FLASHLIGHT`（0x4）位并清除（Inspect.inc:277）。注意这是**检视期间**屏蔽发光；检视结束恢复后手电状态位回到原值（按 F 时引擎已切换，若屏蔽则解除检视后仍为关）。

### 2.8 0号VM 传输屏蔽保险（v6.1 加固）

原 v6.0 已用 `SDKHook(ClientVM[0], SetTransmit, OnTransmit)` 屏蔽 0 号 VM 消灭残留枪口火光。v6.1 加固为**每 tick 保险**：

- 新增全局 `g_bVM0BlockHooked[MAXPLAYERS+1]`（Globals.inc:21）标记钩子是否已挂。
- 换武器有 vmodel → 挂钩置 true；无 vmodel → unhook 置 false。
- `IsCustom` / `IsCustom2` 稳态分支**每 tick 检查**：未挂且 VM[0] 有效则补挂（ViewModel.inc:199 / :234），防切枪/VM 实体重建后漏勾穿帮。
- VM[0] 实体重建（`OnEntitySpawned`）、死亡、无武器时重置标记，下一 tick 自动补挂。

---

## 3. 调用链（检视）

```
SDKHook_OnPlayerRunCmd (Combat.inc:76)
  ├─ 后坐力 recoil
  ├─ g_bPressingAttack2 记录
  ├─ 换弹检测 bIsReloading → IncreaseReloadSpeed
  ├─ Zoom_OnPlayerRunCmd(client, buttons, bIsReloading)      ← 机瞄触发
  └─ Inspect_OnPlayerRunCmd(client, impulse, bIsReloading)   ← 检视触发(F键/impulse 100)

SDKHook_PostThinkPost (ViewModel.inc:68)
  ├─ 双 VM 驱动/动画复制
  ├─ Zoom_OnPostThinkPost(client)                            ← 机瞄屏震/过渡
  └─ Inspect_OnPostThinkPost(client)                         ← 检视动画强制/结束条件/手电屏蔽

事件: Event_PlayerDeath (ViewModel.inc:288)
  └─ Zoom_ForceExit + Inspect_End + IsCustom/IsCustom2 清理 + g_bVM0BlockHooked 重置

主 .sp: OnClientDisconnect
  └─ Inspect_Reset(client)（并清理 VM、机瞄、检视状态）
```

---

## 4. 新增/修改文件清单（v6.0 → v6.1）

### 新增

- `HanWeaponSystem/HanWeaponSystemInspectConfig.inc`：检视配置初始化/同步/读取（含双语 credit）
- `HanWeaponSystem/HanWeaponSystemInspect.inc`：检视核心逻辑（含双语 credit）

### 修改

- `[H-AN_CSS]HanWeaponSystem v.6.0.sp` → `[H-AN_CSS]HanWeaponSystem v.6.1.sp`：include 顺序插入 InspectConfig/Inspect；`OnPluginStart`/`OnMapStart` 调 `InspectConfig_OnPluginStart`/`Inspect_OnPluginStart`/`InspectConfig_OnMapStart`；`OnClientDisconnect` 调 `Inspect_Reset`
- `HanWeaponSystemGlobals.inc`：`VERSION "6.1"`；新增 `g_hInspectConfig`、`g_bInspecting`、`g_iInspectWeapon`、`g_fInspectEnd`、`g_fInspectBlock`、`g_iInspectRestart`、`g_bVM0BlockHooked`
- `HanWeaponSystemCombat.inc`：`OnPlayerRunCmd` 增加 `Inspect_OnPlayerRunCmd(client, impulse, bIsReloading)` 调用
- `HanWeaponSystemViewModel.inc`：末尾调 `Inspect_OnPostThinkPost`；`Event_PlayerDeath` 调 `Inspect_End`；`OnEntitySpawned` 重置 `g_bVM0BlockHooked`；`IsCustom`/`IsCustom2` 分支每 tick 保险挂 `SetTransmit` 钩子
- `HanWeaponSystemZoom.inc`：无逻辑改动（互斥判断复用在 Inspect 侧）

---

## 5. 已知坑与决策记录（务必记住）

1. **F 键检测**：必须用 `OnPlayerRunCmd` 的 `impulse` 参数 + 上升沿，`AddCommandListener("impulse")` 无效（客户端本地命令不上行）。
2. **`inspecttime` 单位是帧（30fps）**，不是秒。内部 `frame / 30.0` 换算。配置迁移直接填原插件数字（如 230）。
3. **起源同序列不重播**：非重复检视结束必须恢复 VM[1] 为 VM[0] 序列（idle）；重复检视靠两帧切换强制重播。
4. **`m_flCycle` 是 `Prop_Data`**，不是 `Prop_Send`。
5. **0号VM 传输屏蔽保险**：`g_bVM0BlockHooked` 每 tick 补挂 `SetTransmit`，防切枪/VM 重建漏勾火光穿帮。`OnTransmit` 只拦 `viewer>0`，本地监听服务器无效属正常。
6. **检视与机瞄互斥**：检视中开镜 → 强制退检视；机瞄中/过渡中按 F / sm_inspect → 忽略。
7. **检视期间压 idle**：`m_flTimeWeaponIdle` 在检视期间被压到检视结束，结束后设回 `GetGameTime()`，避免检视被武器空闲动画打断。
8. **命令名**：`sm_inspect`（不是原插件的 `sm_lookat`）。
9. **配置同步**：新增检视键要写进 `WriteDefaultInspectConfig` 模板 + `SyncInspectConfig` 追加默认键组（照抄 ZoomConfig 套路）。默认 `inspect 0` 全关。

---

## 6. 遗留事项 / 待办（下一个窗口）

- [ ] **检视音效**（v6.1 规划，未实现）：检视应有音效选项。计划仿照 Sounds 模块增加配置键（如 `inspectsound` 音效路径 + 可选 `inspectsoundvolume`），检视开始时播放；需在 `Inspect_Start` 调用、考虑预缓存与音效文件管理，详见 v6.0 文档 Sounds 模块规范。
- [ ] 持枪奔跑、空仓换弹（见 v6.0 文档第 5 节指南）
- [ ] 检视 API（`Han_IsClientInspecting` native + `Han_OnClientInspect` forward，仿照 `Han_IsClientZooming`/`Han_OnClientZoom` 模式）——已规划未实现
- [ ] 检视无转动/无视角偏移（当前纯动画，VM[1] 播 inspectseq），若需转动反馈在下一版本评估

## 7. 快速索引（关键文件:行号）

| 功能 | 位置 |
|---|---|
| 每 tick 心脏 | `HanWeaponSystemViewModel.inc:68` |
| 屏蔽 0 号 VM（保险补挂） | `HanWeaponSystemViewModel.inc:199 / :234` |
| 检视触发入口（F键） | `HanWeaponSystemInspect.inc:87` |
| 检视命令 | `HanWeaponSystemInspect.inc:49` |
| 检视开始 | `HanWeaponSystemInspect.inc:136` |
| 检视结束（恢复idle） | `HanWeaponSystemInspect.inc:178` |
| 检视每 tick 后处理 | `HanWeaponSystemInspect.inc:205` |
| 检视重置 | `HanWeaponSystemInspect.inc:290` |
| 检视配置同步 | `HanWeaponSystemInspectConfig.inc:78` |
| 检视配置读取 | `HanWeaponSystemInspectConfig.inc:241` |
| Combat 调用链 | `HanWeaponSystemCombat.inc:76 / :122` |
| 死亡清理 | `HanWeaponSystemViewModel.inc:288` |