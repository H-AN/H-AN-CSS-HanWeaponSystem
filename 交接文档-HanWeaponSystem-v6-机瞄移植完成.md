# 交接文档：HanWeaponSystem v6.0（机瞄移植完成）

> 作者/交接人：华仔 H-AN
> 日期：2026-08-17
> 目的：记录 v5.0 → v6.0 重构 + IronSight 机瞄功能移植的完整现状、架构、规范，为后续「持枪奔跑 / 检视 / 空仓换弹」等动画序列类功能整合提供上下文。

---

## 1. 插件总览

| 项 | 值 |
|---|---|
| 插件名 | `[华仔]武器系统` |
| 主文件 | `addons/sourcemod/scripting/[H-AN_CSS]HanWeaponSystem v.6.0.sp` |
| 版本宏 | `VERSION "6.0"`（`HanWeaponSystemGlobals.inc` 顶部） |
| 作者 | 华仔 H-AN，QQ 407866133，github https://github.com/H-AN |
| 产物 | `scripting/compiled/[H-AN_CSS]HanWeaponSystem v.6.0.smx` |
| 编译 | `scripting/compile.exe "[H-AN_CSS]HanWeaponSystem v.6.0.sp"`（当前零警告零报错） |
| 运行框架 | SourceMod 1.12 / Counter-Strike: Source（经典句柄式 SDKHooks + EntityOutput 全可用） |

### 1.1 文件结构

主 .sp 只做组装，逻辑全部拆到 `HanWeaponSystem/` 目录下 10 个 inc，按依赖顺序 include：

```
[H-AN_CSS]HanWeaponSystem v.6.0.sp
├── HanWeaponSystemGlobals.inc      全局变量/宏/VERSION
├── HanWeaponSystemConfig.inc       主武器配置 HanWeaponData.cfg + 指令 + 击杀事件
├── HanWeaponSystemSounds.inc       武器音效 + 霰弹枪火光 TE
├── HanWeaponSystemCamera.inc       机瞄镜头 point_camera/info_camera_link
├── HanWeaponSystemZoomConfig.inc   机瞄独立配置 HanWeaponZoomData.cfg（主配置自动同步）
├── HanWeaponSystemZoom.inc         机瞄核心（触发/过渡/模型/特效）
├── HanWeaponSystemWeapon.inc       武器给予 + 假世界模型 + 传输拦截
├── HanWeaponSystemCombat.inc       后坐力/攻速/换弹速度 + OnPlayerRunCmd 总入口
├── HanWeaponSystemViewModel.inc    双 VM 模型驱动（每 tick 心脏）
└── HanWeaponSystemAPI.inc          对外 API（native + GlobalForward + 插件库）
```

依赖顺序：`Globals → Config → Sounds → Camera → ZoomConfig → Zoom → Weapon → Combat → ViewModel → API`。

### 1.2 配置文件（运行时自动生成）

- `addons/sourcemod/configs/HanWeaponSystem/HanWeaponData.cfg`：武器主配置（vmodel、音效、指令、假模型、后坐力等）
- `addons/sourcemod/configs/HanWeaponSystem/HanWeaponZoomData.cfg`：机瞄配置。**与主配置自动同步**——主配置新增武器时自动追加默认机瞄键组（`ZoomConfig_OnMapStart` → `SyncZoomConfig`）。

---

## 2. 核心架构：双 VM 模型驱动

整个动画/机瞄/外观体系都建立在 **双 ViewModel** 之上，这是理解一切后续功能（奔跑/检视/空仓换弹）的关键。

### 2.1 双 VM 模型（VM[0] / VM[1]）

- `ClientVM[MAXPLAYERS+1][2]`：`[0]` = 原始 VM（引擎预测、播放原始动作序列），`[1]` = 复制 VM（引擎默认空 VM，代码手动喂模型/动画）。
- 机制：原始武器模型不改 VM[0] 的序列，而是**在 VM[1] 上叠加自选模型**，同时把 VM[0] 设为 `EF_NODRAW`、VM[1] 清除 `EF_NODRAW`。
- 实现位置：`HanWeaponSystemViewModel.inc` 的 `OnPostThinkPost`（**每玩家每 tick 心脏**）。

### 2.2 每 tick 驱动心脏：`ViewModel_OnPostThinkPost`

`OnPostThinkPost(int client)`（ViewModel.inc:66）职责（按优先级）：

1. 观战者分支：向观战者显示被观战目标当前武器的 vmodel。
2. 死亡分支：清空。
3. **换武器检测**（`static int OldWeapon[MAXPLAYERS+1]`，`WeaponIndex != OldWeapon`）：
   - 有 vmodel → 复制 VM[0] 数据到 VM[1]、设 VM[1] 模型、`SDKHook(ClientVM[0], SetTransmit, OnTransmit)` 屏蔽 0 号 VM。
   - 无 vmodel → VM[1] `EF_NODRAW`、`SDKUnhook` 传输拦截、`IsCustom=false`。
4. 未换武器但 `IsCustom` → 持续把 VM[0] 的 `m_nSequence / m_flCycle / m_flPlaybackRate` 复制到 VM[1]（动画跟随）。
5. 末尾调用 `Zoom_OnPostThinkPost(client)`（机瞄屏震）。

### 2.3 SDKHook 挂点清单

| 钩子 | 实体 | 作用 |
|---|---|---|
| `OnPostThinkPost`（ViewModel.inc:66） | 玩家 | 每 tick 驱动双 VM + 动画复制 |
| `WeaponSwitch` / `WeaponEquip` → `WeaponHook` | 玩家 | 武器给予/换弹钩子 |
| `WeaponSwitchPost` → `WeaponSwitchPost` | 玩家 | 切换后假世界模型刷新 |
| `WeaponDrop` → `OnWeaponDrop` | 玩家 | 丢武器清假模型 |
| `OnTakeDamage` → `OnTakeDamage` / `BlockKnowBack` | 玩家 | 击杀伤害 + 防击退 |
| `PostThink` / `PostThinkPost` → `Hook_OnPostThinkPostSpeed`（Combat） | 玩家 | 攻速/换弹速度 |
| `OnPlayerRunCmd` → `Combat_OnPlayerRunCmd`（Combat.inc:76） | 玩家 | 后坐力 + 换弹检测 + **机瞄触发入口** |
| `Weapon` 的 `SetTransmit` → `SetTransmit_CallBack`（Weapon.inc:111） | 假世界模型 | 只让持有者看到 |
| VM[0] 的 `SetTransmit` → `OnTransmit`（ViewModel.inc:330） | 0 号 VM | **屏蔽 0 号 VM 传输（消灭残留枪口火光）** |

> 注意：`OnTransmit` 只拦截 `viewer > 0`（真实玩家）。本地监听服务器 viewer==0 时无效属正常，真实服务器不受影响（已确认）。

### 2.4 调用链（机瞄）

```
SDKHook_OnPlayerRunCmd (Combat.inc:76)
  ├─ 后坐力 recoil
  ├─ g_bPressingAttack2 记录
  ├─ 换弹检测 bIsReloading → IncreaseReloadSpeed
  └─ Zoom_OnPlayerRunCmd(client, buttons, bIsReloading)   ← 机瞄触发

SDKHook_PostThinkPost (ViewModel.inc:66)
  ├─ 双 VM 驱动/动画复制
  └─ Zoom_OnPostThinkPost(client)                          ← 机瞄屏震
```

---

## 3. 机瞄功能完整实现记录（本次移植成功）

来源：旧插件 `IronSight Plugin`（作者 boss, cjsrk，v0.8）。Camera 部分来源：花花花。(Flower) / mufiu.com / "Camera"。已按作者要求在相关 inc 顶部加双语 credit 注释。

### 3.1 配置字段（HanWeaponZoomData.cfg 每个武器键组）

| 键 | 含义 |
|---|---|
| `zoom` | 0=不支持 1=支持机瞄 |
| `vmodel` | 机瞄时 1 号 VM 显示的基础模型路径 |
| `zoommodel` | 机瞄时显示的模型（完整路径，通常带镜片） |
| `zoomanimmove` | 开镜/关镜过渡动画序列号 |
| `zoomanimtime` | 过渡时长（秒） |
| `zoomanimfire` | 开火时机瞄动画序列号 |
| `zoomfov` | 开镜 FOV（**必须 0<fov<90 才会创建镜头**，否则只有模型无镜片画面） |
| `zoomaccuracy` | 开镜精准度倍率 |
| `zoomspeed` | 开镜移速倍率（走路速度乘法） |
| `zoomcrosshair` | 开镜准星样式（0/1/2） |
| `zoompunch` | 每帧屏震乘数（**默认 0**，>0 会逐帧放大视角抖动，易转圈） |

旧 IronSight 位置式行（11 字段）→ 新键值对照：
`<weapon><vmodel><zoommodel><zoomanimmove><zoomanimtime><zoomanimfire><zoomfov><zoomaccuracy><zoomspeed><zoomcrosshair><zoompunch>`

### 3.2 触发逻辑（现在只保留「点按」）

位置：`Zoom_OnPlayerRunCmd`（Zoom.inc:70）

- 检测 `IN_ATTACK2` 上升沿（`static s_bPrevAttack2[MAXPLAYERS+1]`）。
- **点按开镜，再按解除**；切换有 0.5s 冷却 `g_fZoomBlock`（防按键抖动重复触发）。
- 已删除内容（v6.0 中途决策）：`!zoom` 菜单、Cookie、按住/长按模式、`g_iZoomModeOverride`、API `Han_SetClientZoomMode/Han_GetClientZoomMode`。`zoommode` 配置字段不再读取，模板中已移除。
- 门槛（换弹/换枪 `m_flNextAttack` 期间禁止机瞄，正在机瞄则强制退出）——但**无弹药特殊武器豁免**（见 3.5）。

### 3.3 过渡与模型切换

- `StartZoom` → 记录 `g_iZoomInSwitch=1`，读 `zoomanimmove` 序列号，`ApplyZoomModel` 切换 VM[1] 模型为 `zoommodel`。
- `Timer_ZoomTransition` 计时过渡（`zoomanimtime` 秒）→ `FinishZoomTransition`（`g_iZoomInSwitch=0`）。
- `Zoom_OnPostThinkPost`：若过渡中序列号已离开衔接号则提前结束过渡（兼容模型自身动画）。
- 开火时 `zoomanimfire` 序列号通过 `Zoom_OnPostThinkPost` 联动。

### 3.4 机瞄特效（`ApplyZoomEffects`）

- FOV 设置（`m_iFOV` / `m_iDefaultFOV`）。
- `zoomfov>0 && zoomfov<90` 时创建 `point_camera` + `info_camera_link`（Camera.inc 的 `SpawnCamera` / `Camera_CreateLink`）。
  - **重要坑**：机瞄镜片材质（`screen.vmt`）用 `_rt_Camera` 渲染，**必须有 point_camera 才有画面**。若配置 `zoomfov=90` 则不建相机 → 开镜黑屏。需保证配置 `zoomfov<90`（用户已在服务器侧改配置解决）。
- 准星（`zoomcrosshair`）、移动速度（`zoomspeed`）、精准度（`zoomaccuracy`）按配置写入。
- 屏震：`zoompunch` 乘数每帧乘到 `m_vecPunchAngle`。

### 3.5 无弹药特殊武器（关键新增）

`Zoom_IsNoAmmoWeapon(weapon)`：`GetEntProp(weapon, Prop_Data, "m_iPrimaryAmmoType") == -1`（即脚本 `primary_ammo "None"`）。

- 此类武器（如自建 `weapon_xunfeidan`，脚本 `clip_size -1` / `primary_ammo None`）**豁免换弹门槛**：换弹/nextattack 期间不强制退镜，ReloadEffect 消息不触发退镜（`Zoom_OnReloadEffect` 跳过）。
- 判定不解析脚本文件，直接读引擎权威属性。

### 3.6 已知坑与决策记录（务必记住）

1. **残留枪口火光**：靠 `SDKHook(ClientVM[0], SetTransmit, OnTransmit)` 屏蔽 0 号 VM 传输解决。本地监听服务器 viewer==0 无效，真实服务器 OK。
2. **开镜黑屏**：`screen.vmt` 用 `_rt_Camera`，必须 `zoomfov<90` 建相机。**不要动代码，改配置**。
3. **开镜转圈/Bad SetLocalAngles**：`zoompunch=10` 是每帧乘数会指数爆炸。**配置必须 0**。
4. **点按动画重播/跳动**：0.2s 冷却太短，已改 0.5s。
5. **长按模式作废**：CS:S 引擎按住 IN_ATTACK2 时无法同时开火，体验差 → 已彻底删除长按逻辑。
6. **旧插件 Event_WeaponFire 的 `GetWeaponClip==0 return` 逻辑未带入** v6.0（无 weapon_fire 钩子、无 GetWeaponClip 使用；weapon_fire 仅被无关的 `Run_with_weapon.sp` / `Run Lookat01.sp` 钩）。
7. **newdecls 数组语法坑**：TE/UserMsg 回调的数组参数必须写 `const int[]` / `const char[]`，不能写旧式 `Players[]` 之类（已修复 `WeaponFireBullets`、`Zoom_OnReloadEffect`）。

---

## 4. 代码规范与风格（新功能必须遵守）

- **语言**：注释中文为主 + 关键行保留英文短注（沿用旧插件习惯）。
- **命名**：函数 `PascalCase` 带模块前缀（`Zoom_`/`Camera_`/`ViewModel_`/`Combat_`/`Config_`/`Weapon_`/`Sound`）；全局变量 `g_` + 类型前缀（`g_bZooming` / `g_iZoomInSwitch` / `g_fZoomBlock`）；静态局部 `static 前缀 s_`；模块常量 `g_` 大写（`g_FakeModels`）；宏全大写（`EF_NODRAW`）。
- **类型前缀**：`bool=b`、`int=i`、`float=f`、`Handle=h`、`char 数组` 用 `s_`（如 `sWeapon`）。
- **每文件结构**：`// ====` 分隔注释块标记章节；`// >> 标题` 作小标题。
- **newdecls required**：数组参数一律 `const int[]`/`const char[]`；不写旧式 `public Action:`。
- **Handle 管理**：所有 `new Menu/KeyValues/Handle` 用完 `delete`；原生句柄全局用 `INVALID_HANDLE` 初始化；客户端相关数组 `MAXPLAYERS+1`。
- **SDKHooks**：实体钩子注意换实体后要重新挂；钩子重复挂安全（SDKHooks 自动去重）。
- **不用 emoji**；不写无意义注释；改动后必须 `compile.exe` 编译零警告零报错才算完成。
- **配置规范**：数值类键存字符串用 KV 读取；新增键要同步写进 `WriteDefaultZoomConfig` 模板 + `SyncZoomConfig` 追加默认值逻辑。

---

## 5. 未来功能整合指南（持枪奔跑 / 检视 / 空仓换弹）

这三类功能与机瞄一样，本质是**操作动画序列 + VM 模型/动画状态**，可直接复用现有骨架：

### 5.1 复用现有驱动点

| 需求 | 挂点 | 说明 |
|---|---|---|
| 事件型（切到检视/空仓） | `Combat_OnPlayerRunCmd` 或 `WeaponHook`（WeaponSwitch 后） | 在按钮检测后触发一次 |
| 每 tick 动画保持 | `ViewModel_OnPostThinkPost`（双 VM 分支） | 在复制动画处按状态改写序列 |
| 按钮上升沿 | 参照 `s_bPrevAttack2` 模式（`IN_RELOAD`/`IN_USE`） | 各写各的 static 前缀数组 |
| 状态机 | 参照 `g_iZoomInSwitch`（0/1/2 三态） | 加新全局状态数组，OnClientDisconnect 里清 |
| 播放期间锁输入 | 参照机瞄换弹门槛 | 可复用 `m_flNextAttack` 或加 own 冷却 |

### 5.2 建议新增文件

按机瞄拆分惯例，建议新建 `HanWeaponSystemRun.inc`（奔跑）、`HanWeaponSystemInspect.inc`（检视）、`HanWeaponSystemEmptyReload.inc`（空仓换弹），每个文件：

1. `Xxx_OnPluginStart()`（注册命令/钩子）
2. `Xxx_OnPlayerRunCmd(client, buttons, ...)`（触发判断）
3. `Xxx_OnPostThinkPost(client)`（每 tick 动画强制）
4. 自己的状态全局 + `Xxx_Reset(client)`，并在主 .sp 的 `OnClientDisconnect` 统一清理
5. 在主 .sp 的 include 列表按依赖插入、在 `OnPluginStart` 调 `Xxx_OnPluginStart()`

### 5.3 与机瞄的冲突注意事项

- **动画复制顺序**：`ViewModel_OnPostThinkPost` 里"把 VM[0] 序列复制到 VM[1]"必须在**每个 tick**执行，若奔跑/检视要改 VM[1] 序列，需在复制之后再覆盖，且要考虑机瞄状态（开镜时优先机瞄动画）。
- **机瞄过渡中**（`g_iZoomInSwitch != 0`）建议禁止检视/奔跑动画覆盖，避免序列错乱。
- **换弹门槛变量**：`bIsReloading`（Combat 检测）+ `m_flNextAttack`，空仓换弹功能需要识别"空仓"状态（`m_iClip1==0` 但非换弹中）作触发条件。

---

## 6. 已完成的移植工作清单

- [x] 主 .sp 瘦身，逻辑拆分 10 个 inc，命名规范统一
- [x] 武器配置 HanWeaponData.cfg + 机瞄配置 HanWeaponZoomData.cfg 双配置自动同步
- [x] 双 VM 模型驱动（每 tick 动画复制 + 观战者显示 + 换武器检测）
- [x] 机瞄触发（点按开镜/再按解除 + 0.5s 防抖）
- [x] 机瞄过渡（zoomanimmove / zoomanimtime / zoomanimfire）+ 提前结束过渡
- [x] 机瞄特效（FOV/镜头/准星/移速/精准度/屏震）
- [x] Camera（point_camera + info_camera_link，含掉线/死亡清理）
- [x] 无弹药特殊武器豁免（m_iPrimaryAmmoType==-1）
- [x] 屏蔽 0 号 VM 传输消灭残留火光
- [x] 双语 credit 注释（Camera.inc / Zoom.inc）
- [x] 删除 !zoom 菜单与长按模式（v6.0 最终决策）
- [x] 全量编译零警告零报错；上服实测通过

## 7. 遗留事项 / 待办

- [ ] 后续集成：持枪奔跑、检视、空仓换弹（见第 5 节指南）
- [ ] `PLAN-HanWeaponSystem-v6.md` 为历史计划文档，其中机瞄模式相关条目已过时，仅作参考

## 8. 快速索引（关键文件:行号）

| 功能 | 位置 |
|---|---|
| 每 tick 心脏 | `HanWeaponSystemViewModel.inc:66` |
| 屏蔽 0 号 VM | `HanWeaponSystemViewModel.inc:175 / :330` |
| 机瞄触发入口 | `HanWeaponSystemZoom.inc:70` |
| 机瞄特效 | `HanWeaponSystemZoom.inc:244` |
| 屏震 | `HanWeaponSystemZoom.inc:340` |
| 特殊武器豁免 | `HanWeaponSystemZoom.inc:49` |
| Camera 镜头 | `HanWeaponSystemCamera.inc:45` |
| 后坐力/换弹/触发链 | `HanWeaponSystemCombat.inc:76 / :119` |
| 配置同步 | `HanWeaponSystemZoomConfig.inc:74` |
| 对外 API | `HanWeaponSystemAPI.inc:5` / `include/HanWeaponSystem.inc` |