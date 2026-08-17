# 交接文档：HanWeaponSystem v6.2（奔跑系统完成）

> 作者/交接人：华仔 H-AN
> 日期：2026-08-17
> 目的：记录 v6.1 → v6.2 新增「持枪奔跑」功能的完整现状、架构、配置与坑，并确认 v6.1 遗留的「检视音效」「检视 API」两项已补完。为后续「空仓换弹」等动画序列类功能的整合提供上下文。
> 说明：本版本在 v6.1 交接文档基础上追加，机瞄/检视部分架构不变，请同时参考 v6.0/v6.1 文档。

---

## 1. 插件总览

| 项 | 值 |
|---|---|
| 插件名 | `[华仔]武器系统` |
| 主文件 | `addons/sourcemod/scripting/[H-AN_CSS]HanWeaponSystem v.6.2.sp` |
| 版本宏 | `VERSION "6.2"`（`HanWeaponSystemGlobals.inc:1`） |
| 作者 | 华仔 H-AN，QQ 407866133，github https://github.com/H-AN |
| 产物 | `scripting/compiled/[H-AN_CSS]HanWeaponSystem v.6.2.smx` |
| 编译 | `scripting/compile.exe "[H-AN_CSS]HanWeaponSystem v.6.2.sp"`（当前零警告零报错） |
| 运行框架 | SourceMod 1.12 / Counter-Strike: Source（经典句柄式 SDKHooks + EntityOutput 全可用） |
| 仓库 | github `H-AN-CSS-HanWeaponSystem`（**v6.2 尚未同步仓库**，开发在本地工作环境，改完才全量复制） |

### 1.1 文件结构

主 .sp 只做组装，逻辑全部拆到 `HanWeaponSystem/` 目录下 **14 个 inc**，按依赖顺序 include：

```
[H-AN_CSS]HanWeaponSystem v.6.2.sp
├── HanWeaponSystemGlobals.inc       全局变量/宏/VERSION
├── HanWeaponSystemConfig.inc        主武器配置 HanWeaponData.cfg + 指令 + 击杀事件
├── HanWeaponSystemSounds.inc        武器音效 + 霰弹枪火光 TE
├── HanWeaponSystemCamera.inc        机瞄镜头 point_camera/info_camera_link
├── HanWeaponSystemZoomConfig.inc    机瞄独立配置 HanWeaponZoomData.cfg（主配置自动同步）
├── HanWeaponSystemZoom.inc          机瞄核心（触发/过渡/模型/特效）
├── HanWeaponSystemInspectConfig.inc 检视独立配置 HanWeaponInspectData.cfg（主配置自动同步）
├── HanWeaponSystemInspect.inc       检视核心（F键/命令触发 + 动画强制 + 手电屏蔽 + 音效）
├── HanWeaponSystemRunConfig.inc     奔跑独立配置 HanWeaponRunData.cfg（主配置自动同步）★v6.2新增
├── HanWeaponSystemRun.inc           奔跑核心（点按Shift触发 + 状态机 + 动画强制 + 抖动）★v6.2新增
├── HanWeaponSystemWeapon.inc        武器给予 + 假世界模型 + 传输拦截
├── HanWeaponSystemCombat.inc        后坐力/攻速/换弹速度 + OnPlayerRunCmd 总入口
├── HanWeaponSystemViewModel.inc     双 VM 模型驱动（每 tick 心脏）
└── HanWeaponSystemAPI.inc           对外 API（native + GlobalForward + 插件库）
```

依赖顺序：`Globals → Config → Sounds → Camera → ZoomConfig → Zoom → InspectConfig → Inspect → RunConfig → Run → Weapon → Combat → ViewModel → API`。

### 1.2 配置文件（运行时自动生成）

- `addons/sourcemod/configs/HanWeaponSystem/HanWeaponData.cfg`：武器主配置
- `addons/sourcemod/configs/HanWeaponSystem/HanWeaponZoomData.cfg`：机瞄配置（自动同步）
- `addons/sourcemod/configs/HanWeaponSystem/HanWeaponInspectData.cfg`：检视配置（自动同步）
- `addons/sourcemod/configs/HanWeaponSystem/HanWeaponRunData.cfg`：**奔跑配置（v6.2 新增）**，与主配置自动同步——主配置新增武器时自动追加默认键组（`RunConfig_OnMapStart` → `SyncRunConfig`）。**默认 `run 0` 全部关闭奔跑**。

---

## 2. 奔跑功能完整实现记录（v6.2 新增）

来源：旧插件 `Run_with_weapon`（作者 cjsrk，v1.1）。已按惯例在两个奔跑 inc 顶部加双语 credit 注释。参考的 `Run_with_weapon.sp` 仍在 `scripting/` 下，仅作对照。

### 2.1 配置字段（HanWeaponRunData.cfg 每个武器键组）

| 键 | 含义 |
|---|---|
| `useclassname` | 对应主配置武器的 useclassname（自动复制，匹配用） |
| `run` | 0=关闭奔跑（默认） 1=启用 |
| `runspeed` | 奔跑移速倍率（如 1.5）；缺省/0=不加速 |
| `runseq` | 奔跑动作1动画序列号（**动作2自动 = runseq+1**，见坑 3） |
| `runtime` | 奔跑动作时长，**单位帧（30fps）**，与原 Run_with_weapon 配置兼容（如 15 ≈ 0.5 秒） |
| `runin` | 可选 Idle→奔跑过渡动作序列号（-1/缺省 = 无过渡） |
| `runinlength` | 可选 Idle→奔跑过渡时长（帧） |
| `runout` | 可选 奔跑→Idle过渡动作序列号（-1/缺省 = 无过渡，直接结束） |
| `runoutlength` | 可选 奔跑→Idle过渡时长（帧） |
| `runshake` | 抖动幅度值；0/缺省=不抖动，>0 按该值抖动（频率/时长固定） |

> **单位决策（沿用惯例）**：`runtime`/`runinlength`/`runoutlength` 用**帧**（`/30.0` 换算秒），与原插件配置一致，可原样迁移。

### 2.2 触发方式：点按 Shift（重要设计决策）

最终采用的触发设计（经过多次返工确认）：

- **长按 Shift（IN_SPEED）+ 前进** = 引擎原生静步，奔跑系统完全不碰。
- **点按 Shift（短按松开）+ 前进按住** = 开始奔跑。
- **松开前进 / 停下**（按方向/跳跃/下蹲/开火/侧瞄/机瞄/检视/换弹/切枪/死亡）→ 奔跑结束。
- **再次点按 Shift** 才能再次奔跑（不会在松开时自动续跑）。

**点按判定原理**（`Run_OnPlayerRunCmd`，Run.inc:87）：

- 用 `SDKHook_OnPlayerRunCmd` 的 `buttons` 检测 `IN_SPEED` 的**边沿**，而不是按键状态本身：
  - 上升沿（`speedDown && !g_bSpeedDown`）：记录按下时刻 `g_fSpeedPressTime = GetGameTime()`。
  - 下降沿（`!speedDown && g_bSpeedDown`）：若 `GetGameTime() - g_fSpeedPressTime <= RUN_SPEED_TAP_TIME(0.2)` 判定为"点按"，走 `Run_TryStart` 触发；长按松开（>0.2s）不触发，保留静步。
- `Run_TryStart`（Run.inc:173）门槛：未奔跑、有武器、前进按住、无 BACK/MOVELEFT/MOVERIGHT/JUMP/DUCK/ATTACK、非机瞄/检视、非换弹/deploy（`m_flNextAttack`）、`run 1` 启用 → `Run_Start`。
- 奔跑中每 tick **剔除 IN_SPEED 位**（`buttons &= ~IN_SPEED`，返回 `Plugin_Changed` 透传）：防止按住 Shift 被引擎静步减速干扰奔跑速度。

### 2.3 奔跑状态机

全局（Globals.inc:55-70）：

- `g_bRunning[MAXPLAYERS+1]`：奔跑流程中（stage != 0），供互斥/API 使用
- `g_iRunStage[MAXPLAYERS+1]`：0=无 1=runin过渡 2=奔跑循环 3=runout过渡
- `g_iRunWeapon[MAXPLAYERS+1]`：奔跑开始时武器实体
- `g_fRunSwitch[MAXPLAYERS+1]`：循环切换/过渡结束时间戳
- `g_bRunFrame[MAXPLAYERS+1]`：循环动画两帧切换标记
- `g_bRunSpeedApplied[MAXPLAYERS+1]`：奔跑速度是否已写入（防每 tick 重复写）
- `g_fSpeedPressTime` / `g_bSpeedDown`：点按判定用（见 2.2）
- `g_fRunShakeNext[MAXPLAYERS+1]`：下次抖动触发时间

流程：`Run_Start`（Run.inc:205）→ 有 runin 则 `RUN_STAGE_IN`，播完进 `RUN_STAGE_RUN`；无 runin 直接 `RUN_STAGE_RUN`。停止时 `Run_Stop`（Run.inc:242）→ 有 runout 则 `RUN_STAGE_OUT`，播完 `Run_End`；无 runout 直接 `Run_End`（Run.inc:276）。

### 2.4 动画强制与"同序列不重播"坑

- 每 tick 在 ViewModel 动画复制**之后**调用 `Run_OnPostThinkPost`（ViewModel.inc:287），覆盖 VM[1] 为对应阶段的 `m_nSequence` + `m_flPlaybackRate = 1.0`。
- **坑：起源引擎设置同一序列不会重播**。奔跑循环用 `runseq` 和 `runseq+1` 两个序列交替（`g_bRunFrame` 每 `runtime` 翻转），配置只需填动作1，动作2自动 +1。
- 奔跑期间 `SetEntPropFloat(weapon, "m_flTimeWeaponIdle", GetGameTime()+1.0)` 压住武器空闲动画，避免中途插回 idle 打断。
- `Run_End` 恢复 VM[1] 为 VM[0] 的引擎原生序列（idle），并把 `m_flTimeWeaponIdle` 设回 `GetGameTime()`。

### 2.5 奔跑速度

- 用 `m_flLaggedMovementValue`（`Prop_Data`）写移速倍率：`Run_ApplySpeed`（Run.inc:59）写 `runspeed`，`Run_RestoreSpeed`（Run.inc:72）恢复 1.0。
- 注意：**机瞄也共用 `m_flLaggedMovementValue`**（缩放 FOV 速度），两者互斥不会同时生效，无冲突。

### 2.6 屏幕抖动（重点坑）

参数：幅度 = 配置 `runshake`，频率固定 `0.4`，时长固定 `2.0`，每 `RUN_SHAKE_TIME` 秒触发一次。

**重要坑（经过多轮实测确认的引擎行为）**：

1. **CS:S 的 Shake 消息幅度是"累加"机制**：每次收到 `SHAKE_START`（command=0）都在现有幅度上**累加**，不是覆盖；`SHAKE_STOP`（command=1）只清时长、不清幅度。幅度**跨死亡重生保留，只有换图才清零** → 反复触发会越晃越大。
2. **负值会被引擎 clamp 到 0**：发负幅度不会产生"反向摆动"，而是被归零。
3. **最终方案**（`Run_OnPostThinkPost` 抖动段，Run.inc:404）：每次触发**先发一条 `-9999`（`RUN_SHAKE_RESET_AMP`）**把累计幅度 clamp 归零，**紧接着发 `+runshake`**。这样每次幅度都精确等于配置值、零累加、连续抖动。`Run_CreateShake`（Run.inc:421）带 duration 参数；`Run_End` 时 `Run_StopShake`（Run.inc:440）发 command=1 立即消除抖动尾巴。

### 2.7 奔跑与其他系统的互斥（闭环）

- 奔跑侧门槛（Run_OnPlayerRunCmd / Run_TryStart / Run_OnPostThinkPost）：`g_bZooming || g_iZoomInSwitch || g_bInspecting` 时不可启动/强制退出。
- 机瞄侧（Zoom.inc:98）与检视侧（Inspect.inc:68 / :113 / :291）均加了 `g_bRunning` 门槛：奔跑中不可开镜/检视。
- 换弹/换枪（`bReloading`、`m_flNextAttack`）期间不可奔跑；奔跑中禁止换弹（`buttons &= ~IN_RELOAD`）。

### 2.8 API（v6.2 补完 + 新增）

v6.1 遗留的检视 API 已补完，v6.2 新增奔跑 API，全部注册在 `RegisterWeaponAPI`（API.inc:5）：

- native `Han_IsClientRunning(int client)`（API.inc:95，声明 include/HanWeaponSystem.inc:83）
- forward `Han_OnClientRun(int client, bool running)`（API.inc:18，声明 :91），开始/结束奔跑时 `Run_FireForward`（Run.inc:459）触发
- 补完：`Han_IsClientInspecting` / `Han_IsWeaponInspectable` / `Han_OnClientInspect`（见 v6.1 文档）

### 2.9 检视音效（v6.1 待办，已补完）

- 配置键 `inspectsound`：检视音效路径，可缺省不填=不播放；多个用英文逗号隔开随机播一个；只自己听得见（`EmitSoundToClient`）。
- 固定通道 `SNDCHAN_INSPECT = SNDCHAN_STATIC`（Inspect.inc:21），检视开始播放（Inspect.inc:190-211），检视结束/停止用 `Inspect_StopSound`（Inspect.inc:142）`StopSound`。
- 音效需预缓存（主 .sp OnPluginStart 已有 `PrecacheInspectSounds` 逻辑，随 Sounds 模块一起）。

---

## 3. 调用链（奔跑）

```
SDKHook_OnPlayerRunCmd (Combat.inc:125)
  └─ Run_OnPlayerRunCmd(client, buttons, bIsReloading)
       ├─ 奔跑中: 剔除IN_SPEED / 禁止换弹(Plugin_Changed透传) + 停止条件检测
       └─ 未奔跑: IN_SPEED 边沿检测 → 点按判定 → Run_TryStart → Run_Start

SDKHook_PostThinkPost (ViewModel.inc:287)
  └─ Run_OnPostThinkPost(client)
       ├─ 结束条件(死亡/换枪/机瞄/检视/换弹)
       ├─ 状态推进 + VM[1] 动画强制(runin/runseq交替/runout)
       ├─ 压 idle (m_flTimeWeaponIdle)
       └─ 抖动触发(-9999归零 + runshake)

事件: Event_PlayerDeath (ViewModel.inc:304)
  └─ Run_End(client)（恢复速度/动画/停抖动）

主 .sp: OnPluginStart (RunConfig_OnPluginStart + Run_OnPluginStart)
主 .sp: OnMapStart (RunConfig_OnMapStart → 配置初始化/同步/读取)
主 .sp: OnClientDisconnect (Run_Reset, :111)
```

---

## 4. 新增/修改文件清单（v6.1 → v6.2）

### 新增

- `HanWeaponSystem/HanWeaponSystemRunConfig.inc`：奔跑配置初始化/同步/读取（含双语 credit）
- `HanWeaponSystem/HanWeaponSystemRun.inc`：奔跑核心逻辑（含双语 credit）

### 修改

- `[H-AN_CSS]HanWeaponSystem v.6.1.sp` → `[H-AN_CSS]HanWeaponSystem v.6.2.sp`：include 顺序插入 RunConfig/Run；`OnPluginStart`/`OnMapStart` 调 `RunConfig_OnPluginStart`/`Run_OnPluginStart`/`RunConfig_OnMapStart`；`OnClientDisconnect` 调 `Run_Reset`
- `HanWeaponSystemGlobals.inc`：`VERSION "6.2"`；新增奔跑全局（见 2.3）
- `HanWeaponSystemCombat.inc`：`OnPlayerRunCmd` 末尾调 `Run_OnPlayerRunCmd`，`Plugin_Changed` 透传
- `HanWeaponSystemViewModel.inc`：末尾调 `Run_OnPostThinkPost`；`Event_PlayerDeath` 调 `Run_End`
- `HanWeaponSystemZoom.inc` / `HanWeaponSystemInspect.inc`：加 `g_bRunning` 互斥门槛
- `HanWeaponSystemAPI.inc` + `include/HanWeaponSystem.inc`：奔跑 API + 补完检视 API
- `HanWeaponSystemInspect.inc` / `InspectConfig.inc`：补完检视音效（v6.1 待办）

---

## 5. 已知坑与决策记录（务必记住）

1. **Shake 幅度累加 + clamp 归零**：CS:S 的 Shake 是累加制（`SHAKE_START` 累加、`SHAKE_STOP` 不清幅、跨重生保留、换图才清零），负值被 clamp 到 0。**每次触发前先发 `-9999` 强制归零再发正值**，才能保证幅度恒定不越跑越大。
2. **点按触发必须用边沿检测**：`buttons` 的 `IN_SPEED` 上升/下降沿 + `g_fSpeedPressTime` 按捺时长（≤0.2s 判点按），长按保持静步。与 F 键 `impulse` 边沿检测同理。
3. **起源同序列不重播**：奔跑循环用 `runseq` / `runseq+1` 两序列交替（`g_bRunFrame`），配置只填动作1。
4. **`runtime` 单位是帧（30fps）**，不是秒，内部 `/30.0` 换算。
5. **`m_flLaggedMovementValue` 与机瞄共用**：奔跑速度倍率走它，机瞄也走它，互斥无冲突；写前检查 `g_bRunSpeedApplied` 防重复写。
6. **奔跑中剔除 `IN_SPEED`**：否则按住 Shift 会被引擎静步减速，`Plugin_Changed` 透传按键修改。
7. **触发门槛顺序**：点按判定在未奔跑分支才做；奔跑中只同步 `g_bSpeedDown` 状态不做触发。
8. **互斥闭环**：奔跑 ↔ 机瞄 ↔ 检视 ↔ 换弹，三方各自加对另外两方的门槛。
9. **奔跑结束清尾巴**：`Run_End` 必须发 `Shake` 停止消息（command=1）+ 恢复 VM[1] 原生序列 + 恢复速度 + `m_flTimeWeaponIdle`。

---

## 6. 遗留事项 / 待办（下一个窗口）

- [ ] **git 仓库同步 v6.2**：按 00 规则，开发环境确认 OK 后全量复制到 `H-AN-CSS-HanWeaponSystem` 并 commit。
- [ ] 空仓换弹（见 v6.0 文档第 5 节指南）
- [ ] 奔跑可选**手柄 Rumble 反馈**（可选加项，非必要）：Rumble 消息有 STOP/LOOP/UPDATE_SCALE flag 可正确管理手柄马达震动，与画面 Shake 无关；如需奔跑时手柄跟着震可加。

## 7. 快速索引（关键文件:行号）

| 功能 | 位置 |
|---|---|
| 每 tick 心脏 | `HanWeaponSystemViewModel.inc:68` |
| 奔跑触发检测（点按边沿） | `HanWeaponSystemRun.inc:87` |
| 奔跑触发尝试（门槛） | `HanWeaponSystemRun.inc:173` |
| 奔跑开始 | `HanWeaponSystemRun.inc:205` |
| 奔跑停止（runout） | `HanWeaponSystemRun.inc:242` |
| 奔跑结束（恢复） | `HanWeaponSystemRun.inc:276` |
| 奔跑每 tick 后处理 | `HanWeaponSystemRun.inc:313` |
| 屏幕抖动（-9999归零方案） | `HanWeaponSystemRun.inc:404 / :421` |
| 停止抖动 | `HanWeaponSystemRun.inc:440` |
| 奔跑事件转发 | `HanWeaponSystemRun.inc:459` |
| 奔跑重置 | `HanWeaponSystemRun.inc:473` |
| 奔跑配置同步 | `HanWeaponSystemRunConfig.inc:82` |
| 奔跑配置读取 | `HanWeaponSystemRunConfig.inc:249` |
| 奔跑 API 注册 | `HanWeaponSystemAPI.inc:5 / :14 / :18 / :95` |
| 奔跑 API 声明 | `include/HanWeaponSystem.inc:83 / :91` |
| Combat 调用链 | `HanWeaponSystemCombat.inc:125` |
| 死亡清理 | `HanWeaponSystemViewModel.inc:304` |
| 互斥门槛（Zoom侧） | `HanWeaponSystemZoom.inc:98` |
| 互斥门槛（Inspect侧） | `HanWeaponSystemInspect.inc:68 / :113 / :291` |
| 检视音效 | `HanWeaponSystemInspect.inc:142 / :190` |