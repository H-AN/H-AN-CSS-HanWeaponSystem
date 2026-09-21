# [华仔]武器系统 HanWeaponSystem

![语言](https://img.shields.io/badge/语言-中文-red) ![English](https://img.shields.io/badge/Language-English-blue) ![Game](https://img.shields.io/badge/Game-CS%3A%20Source-yellow) ![Platform](https://img.shields.io/badge/SourceMod-1.12-orange)

**[ 简体中文 ]** | [ English ](readme.en.md)

**8.2：** 新增刀左右键距离配置、原版式直线/盒体检测、材质反馈与命中部位 API，以及武器身份/VM 展示状态 API。
**8.3：** 调整切枪动画刷新范围，独立处理自定义模型切入时的动画进度归零；重构开火动画修复的触发及清理逻辑，减少对拔枪、换弹和其他动画接管的误干预。
新增独立插件：[HanVMCrossFix](https://github.com/H-AN/HanVMCrossFix)修复跨v0-v1模型切换所导致的动画缺失问题，建议一起安装使用体验更好


如果你喜欢这个插件，可以用以下方式支持我，感谢！

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/Z8Z31PY52N)

---

## 插件介绍

[华仔]武器系统是一个 CS:Source（起源）大型武器系统插件，可以为服务器**添加自定义武器**：新模型、新动画、新音效，并自带机瞄、侧瞄、检视、奔跑、空仓/战术换弹、买卖菜单、Bot 支持等完整玩法。

- 运行环境：SourceMod 1.12 / Counter-Strike: Source
- 无任何第三方库依赖，编译只需本仓库文件

---

## 功能列表

| 功能 | 说明 |
|---|---|
| 🔫 自定义武器 | 新 V/W 模型、伤害/后坐力/攻速/击退可调 |
| 🔊 自定义音效 | 射击/切枪/换弹/检视音效，支持多段随机播放 |
| 🎯 机瞄 | 右键开镜，镜内 FOV 放大（point_camera 实现真镜片） |
| 🎯 侧瞄 | 中键触发，与机瞄平行的独立一套参数 |
| 🔍 检视 | F 键检视武器，音效仅本人与第一人称观战者可听 |
| 🏃 奔跑 | Shift 触发，带启动推力、速度上限与屏幕抖动 |
| 🔄 空仓/战术换弹 | 接管换弹动画与时长，按帧配置音效 |
| 💣 手雷体系 | 拉保险音、弹跳/爆炸音替换、飞行模型缩放 |
| 🛒 买卖系统 | `sm_buy` / `sm_sell` 购买与出售自定义武器 |
| 🤖 Bot 支持 | Bot 自动购买新武器，随机机瞄/奔跑/检视 |
| 🧩 对外 API | 提供 native 与 GlobalForward 供其他插件调用 |

---

## 安装

1. 将 `plugins` 内编译好的 `.smx` 放入服务器 `addons/sourcemod/plugins/`
2. 首次加载会自动在 `addons/sourcemod/configs/HanWeaponSystem/` 生成全部默认配置
3. 自定义武器的**武器脚本**放入服务器 `cstrike/scripts/` 文件夹
4. 模型/音效资源由插件统一管理下载

---

## 快速上手

1. 编辑 `configs/HanWeaponSystem/HanWeaponData.cfg`，参照文件内注释添加武器节点
2. 按需在 ZoomData / InspectData / RunData / EmptyReloadData 中开启对应功能
3. `sm plugins reload` 重载插件（配置自动同步到各子配置文件）
4. 游戏内输入购买命令获得武器

### 默认操作方式

| 操作 | 按键/命令 | 说明 |
|---|---|---|
| 购买武器 | `sm_buy`（可在 BuyData.cfg 修改） | 打开购买菜单 |
| 出售武器 | `sm_sell`（可在 BuyData.cfg 修改） | 打开出售菜单 |
| 机瞄 | 鼠标右键（IN_ATTACK2） | 需武器配置 `zoom 1`，点按开/关镜 |
| 侧瞄 | **鼠标中键（IN_ATTACK3）** | 需武器配置 `sideaim 1`；**默认未绑定，需手动绑定**（见下方绑定指南） |
| 检视 | F 键 或 `sm_inspect` | 需武器配置 `inspect 1`；F 键原为手电筒（impulse 100），已被检视接管 |
| 奔跑 | **点按 Shift** | 需武器配置 `run 1`：点按触发奔跑动画（带起手/收手动作），之后**按住前进持续奔跑**；松开前进、开火、切枪、瞄准会自动结束 |
| 静步 | **长按 Shift 不放** | 保留原版静步逻辑，与奔跑互不干扰 |

> 注意：机瞄/侧瞄/检视/奔跑均为**每把武器独立配置**，未配置对应字段的武器按键无效果。

### 按键绑定指南

侧瞄使用鼠标中键（IN_ATTACK3），原版 CS:S 未绑定该键，需要在控制台手动绑定：

```text
bind mouse3 +attack3
```

- 绑定一次永久生效（写入 config.cfg）；
- F 键默认已绑定 `impulse 100`（原版手电筒），装了本插件后自动变为检视，无需额外操作；
- 按了没反应的排查顺序：
  1. 该武器是否配置了对应功能字段（zoom / sideaim / inspect / run）；
  2. 侧瞄是否已执行 `bind mouse3 +attack3`；
  3. 是否处于换弹/切换武器的瞬间（部分动作会被临时屏蔽）。

---

## 配置文件说明

所有配置位于 `addons/sourcemod/configs/HanWeaponSystem/`，每个文件头部都有完整的字段注释。

### HanWeaponData.cfg（主配置，每把武器一个节点）

| 字段 | 说明 |
|---|---|
| `command` | 发放该武器的控制台命令 |
| `classname` | 创建实体用的原始 classname（如 weapon_ak47） |
| `useclassname` | 指向的武器脚本名（与 scripts/ 内脚本同名） |
| `damage` | 伤害调整，`+40` 加法或 `x1.5` 乘法 |
| `recoil` | 后坐力系数，1.0 正常，越高越抖，0 无后坐力 |
| `firespeed` | 攻速，格式 `左键值:右键值`（如 `+0.5:+0.6`） |
| `reloadspeed` | 换弹速度倍率，1.0 正常 |
| `knowback` | 命中击退力度 |
| `vmodel` / `wmodel` | 第一人称 / 世界模型路径 |
| `firesound` | 射击音效，多个用英文逗号隔开随机播放 |
| `switchsound` | 切枪音效，格式同上 |
| `wmodel_silencer` / `firesound_silencer` | 消音器变体（仅 m4a1/usp 模板有效），装消音时生效 |
| `customsoundct` / `customsoundt` | 刀类音效文件夹替换（按队伍） |
| `autofire` | 1=按住右键速射连发 |
| `Sqtype` | 动画序列同步方式（1=手动模式） |
| `skincode` | 模型皮肤索引 |
| `ammo` | 弹夹子弹量 |
| `killicon` | 击杀图标 |
| `team` | 可用队伍（all/ct/t） |
| `reloadsound` | 兜底换弹音效（帧号:路径，逗号分隔多段） |
| `usedroppedmodel` | 1=掉落时使用假 W 模型 |
| `armfix` | 1=修复左手模 |
| `shootqcfixes` | 1=修复开火动画丢失（引擎同序列不重播问题） |
| `grenadescale` | 手雷飞行模型缩放（仅手雷类生效） |

### HanWeaponZoomData.cfg（机瞄/侧瞄）

| 字段 | 说明 |
|---|---|
| `zoom` | 1=启用机瞄 |
| `zoommodel` | 机瞄时显示的模型 |
| `zoomanimmove` / `zoomanimtime` | 开镜衔接动画序号与时长（秒） |
| `zoomanimfire` | 1=机瞄时不强制同步开火动画（默认 0 同步） |
| `zoomfov` | 镜内放大 FOV（<90 启用，需模型带 _rt_camera 镜片） |
| `zoomaccuracy` / `zoomspeed` / `zoomcrosshair` / `zoompunch` / `zoomskinadd` | 精度/移速/准心/震屏/皮肤增量 |
| `sideaim...` | 侧瞄全套字段，与机瞄完全平行（sideaim 前缀） |
| `*_silencer` | model/animmove/animtime/animfire 支持消音器变体 |

### HanWeaponInspectData.cfg（检视）

| 字段 | 说明 |
|---|---|
| `inspect` | 1=启用检视 |
| `inspectseq` / `inspecttime` | 检视动画序号与时长（帧，30fps） |
| `inspectrepeat` | 1=允许反复按 F 从头重播 |
| `inspectflashlight` | 1=检视时屏蔽手电筒 |
| `inspectsound` | 检视音效，逗号分隔随机播放 |
| `*_silencer` | seq/time/sound 消音器变体 |

### HanWeaponRunData.cfg（奔跑）

| 字段 | 说明 |
|---|---|
| `run` | 1=启用奔跑 |
| `runspeed` | 启动推力（每帧叠加的速度增量，默认 50） |
| `runmaxspeed` | 速度上限（500≈2 倍速） |
| `runseq` / `runtime` | 奔跑动作序号与时长（动作 2 自动 = 序号+1） |
| `runin` / `runinlength` | 可选起手过渡动画 |
| `runout` / `runoutlength` | 可选收尾过渡动画 |
| `runshake` | 屏幕抖动幅度，0=不抖动 |
| 序列类字段 `_silencer` | 消音器变体 |

### HanWeaponEmptyReloadData.cfg（空仓/战术换弹）

| 字段 | 说明 |
|---|---|
| `emptyreload` | 1=启用空仓换弹（弹匣打空时） |
| `emptyreloadseq` / `emptyreloadtime` | 动画序号与时长（帧） |
| `emptyreloadsound` | 音效（帧号:路径，逗号分隔多段） |
| `tacticalreload` | 1=启用战术换弹（弹匣有子弹时） |
| `reloadseq` / `reloadtime` / `tacticalreloadsound` | 战术换弹对应字段 |
| `reloadfps` | 动画帧率（默认 30，按需改 24 等） |
| 以上全部支持 `_silencer` 变体 | 仅 m4a1/usp 模板，装消音时生效 |

#### 换弹音效配置方法（HLMV 帧数对齐，服务器必读）

自定义模型的换弹音效推荐用 **HLMV（模型查看器）逐帧对位** 的方式配置：

1. 用 HLMV 打开武器模型，播放换弹动画，记录关键动作出现的**帧号**（如：卸弹匣第 10 帧、插入新弹匣第 45 帧、拉栓上膛第 70 帧）；
2. 按 `帧号:音效路径` 格式填入配置，逗号分隔多段，插件会在换弹接管进行到对应帧时精确播放：

```text
"emptyreloadsound"   "10:weapons/mygun/magout.wav,45:weapons/mygun/magin.wav,70:weapons/mygun/bolt.wav"
```

3. `reloadfps` 需与 HLMV 中看到的动画实际帧率一致（默认按 30fps 换算），否则音效时机会偏移。

**为什么服务器必须这样配？**

- **本地/单机使用**时，客户端会自动播放模型内嵌的自定义音效事件，什么都不用填；
- **服务器使用**时，引擎不会播放模型内嵌的音效事件（客户端/动画驱动，服务端钩子无法拦截），必须走 **帧数 + 音效** 配置形式，由插件在服务端精确播放。

主配置的兜底字段 `reloadsound` 与空仓/战术换弹的 `emptyreloadsound` / `tacticalreloadsound` / 检视音效等均遵循同一思路。

### HanWeaponBuyData.cfg（买卖系统）

| 字段 | 说明 |
|---|---|
| `buysystem` | 1=启用买卖系统 |
| `buymode` | always=随时购买 / buytime=限购买时间 |
| `buycommand` / `sellcommand` | 购买/出售命令（默认 sm_buy / sm_sell） |
| `name` | 菜单显示名 |
| `buyable` / `price` | 是否可购买与价格（不填自动读武器脚本价格） |
| `sellable` / `sellratio` / `sellratio_other` | 出售开关与回收折扣 |

### HanWeaponBotData.cfg（Bot 系统）

| 字段 | 说明 |
|---|---|
| `botsystem` | 1=启用 Bot 武器系统（默认关闭） |
| `botmoney` | 1=真钱模式（扣 bot 金钱走账号选择） |
| `botaimcd` / `botruncd` / `botinspectcd` | 三类行为独立冷却（秒） |
| `botaimtime` | 开镜维持时间（秒） |
| `botchance` | Bot 购买权重 0-100 |
| `botaim` / `botrun` / `botinspect` | 行为触发概率 0-100 |

---

## 服务器命令

| 命令 | 说明 |
|---|---|
| `sm_buy` | 打开购买菜单（命令名可在 BuyData.cfg `buycommand` 修改） |
| `sm_sell` | 打开出售菜单（命令名可在 BuyData.cfg `sellcommand` 修改） |
| `sm_inspect` | 检视当前武器 |
| 每把武器的发放命令 | 主配置 `command` 字段注册的控制台命令，输入即获得该武器 |

---

## 服务器 cvar

| cvar | 默认 | 说明 |
|---|---|---|
| `han_wpsdisablebackweapon` | `0` | 背部武器模型：`0`=启用假背模逻辑（未手持的自定义武器显示在背部/腿部）；`1`=禁用，走引擎原生逻辑 |
| `han_oldweaponfix` | `0` | 非自定义武器的原生武器也走1号v模型：`0`=默认不启用；`1`=启用 (启用后1号模型切换1号模型动画永远不会再丢失,并支持快速近战等插件快切逻辑)|

可写入 `server.cfg` 持久生效，或管理员在控制台运行时切换（切换即时生效：禁用立即恢复引擎原生背模，启用自动重建）。

---

## 使用示例

### 示例一：添加一把特殊武器

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

配套步骤：把武器脚本放入服务器 `cstrike/scripts/weapon_mygun.txt`，模型音效加入下载表，玩家控制台输入 `sm_mygun` 即可获得。

### 示例二：密码命令防滥用（随机命令名 + 后台发放）

直接用 `sm_mygun` 这类好记的命令，任何玩家知道了都能白嫖。解决方案：**命令名用随机字符串，只让服务器后台调用**。

1. 到随机字符串生成网站（如 [suijimimashengcheng.bmcx.com](https://suijimimashengcheng.bmcx.com/)）生成一串随机字符，例如 `hrhipN2bNeVW0PBz`；
2. 填入主配置：

```text
"command"   "sm_hrhipN2bNeVW0PBz"
```

3. 该命令不对外公开，玩家无法猜测；正常获取走购买菜单（菜单内按配置名显示，不受影响）；
4. 其他系统（如补给箱拾取、任务奖励插件）在服务端调用即可发枪：

```sourcepawn
public void OnClientPutInServer(int client)
{
    // 示例：进入游戏 30 秒后通过补给箱逻辑发放
    // 实际使用时替换为你的触发条件
}

// 补给箱拾取等触发点内：
FakeClientCommand(client, "sm_hrhipN2bNeVW0PBz");
```

> 提示：`command` 字段注册的是真实控制台命令，务必使用足够长的随机串防止被穷举。

### 示例三：管理员手动发枪

管理员在控制台或聊天框执行武器的发放命令即可（如 `sm_mygun`），受主配置 `team` 字段的队伍限制约束。

---

## 开发者 API

其他插件可通过 `#include <HanWeaponSystem>` 调用本插件的全部功能接口。加载前建议检查依赖：

```sourcepawn
public void OnAllPluginsLoaded()
{
    if (!LibraryExists("HanWeaponSystem"))
        SetFailState("需要 [华仔]武器系统 主插件");
}
```

完整函数签名见仓库 `include/HanWeaponSystem.inc`，接口一览：

**Native（主动调用）**

| 接口 | 用途 |
|---|---|
| `GetClientViewModel(client, index)` | 获取玩家的 ViewModel 实体 |
| `IsClientPressingAttack2(client)` | 玩家是否按住右键（即使按钮被改写也能正确判断） |
| `IsWeaponAutoFire(client)` | 当前武器是否支持自动连发 |
| `Han_IsClientZooming(client)` / `Han_IsWeaponZoomable(client)` | 机瞄状态查询 / 武器机瞄能力查询 |
| `Han_IsClientSideAiming(client)` / `Han_IsWeaponSideAimable(client)` | 侧瞄状态 / 能力查询 |
| `Han_IsClientInspecting(client)` / `Han_IsWeaponInspectable(client)` | 检视状态 / 能力查询 |
| `Han_IsClientRunning(client)` | 是否处于持枪奔跑中 |
| `Han_SetClientCustomAnim(client, seq, frames, repeat, interruptable)` | 播放自定义动画（最高优先级，可打断/霸体双模式） |
| `Han_IsClientCustomAnim(client)` | 是否在播放自定义动画 |
| `Han_StopClientCustomAnim(client)` | 停止自定义动画（等同自然结束广播） |

**Forward（事件回调）**

| 接口 | 触发时机 |
|---|---|
| `Han_OnClientZoom(client, bool zooming)` | 机瞄状态变化 |
| `Han_OnClientSideAim(client, bool aiming)` | 侧瞄状态变化 |
| `Han_OnClientInspect(client, bool inspecting)` | 检视开始/结束 |
| `Han_OnClientRun(client, bool running)` | 奔跑开始/结束（含过渡） |
| `Han_OnClientEmptyReload(client)` | 空仓换弹开始（一次性） |
| `Han_OnClientTacticalReload(client)` | 战术换弹开始（一次性） |
| `Han_OnClientCustomAnimStart(client, seq, frames)` | 自定义动画开始（含重播） |
| `Han_OnClientCustomAnimEnd(client)` | 自定义动画结束（自然/被打断/切枪/死亡） |

---

## v8.2 新功能与 API 用法

### 新增功能

- **人质命中反馈补足**：原始刀与自定义刀命中人质时，左键播放肉体命中音效、右键播放刺中音效，并沿用 CT/T 自定义音效前缀。确认人质损血后，在本刀缓存接触点补一次血效；插件不再为人质补发墙面材质效果。默认启用，无需配置，不改变人质伤害、扣钱规则或玩家部位 API。
- **刀距离**：左右键分别使用 `knife_range_primary` / `knife_range_secondary`；采用眼睛起点、视线方向、先直线后盒体的检测方式。配置检测与原版伤害协调，缩短距离时拦截不符的命中，延长时补充伤害，避免重叠区重复扣血。
- **统一材质反馈**：所有真实刀模板实体（`CKnife`）默认补充原版 `KnifeSlash` 材质反馈，包括原始 `weapon_knife` 和新刀；新增范围造成玩家损血时补充血效。效果使用表面属性，不按模型名称猜材质。
- **统一命中部位**：攻击瞬间保存目标与部位，插件通过 API 复用。精确分类只针对主体检测已确定的玩家目标；未取得部位时明确返回未知。不会自动增加爆头伤害。
- **通用身份与 VM 状态**：查询武器是否登记、原始模板和已经完成接管的展示模式，外部插件无需维护原始武器名单。
- 保留现有动画、`firespeed` 和 `damage` 语义。

在已有刀条目中添加距离即可，模型与音效配置照旧：

```text
"classname"             "weapon_knife"
"damage"                ""
"firespeed"             "0"
"knife_range_primary"   "80.0"
"knife_range_secondary" "60.0"
```

距离缺省、留空或填 `0` 分别使用原版 **48 / 32**；支持小数，有效范围为 `(0, 8192]`，非法值回退并记录错误。长度是检测路径长度，不是两个玩家中心之间的距离；盒体仍有原版宽容度。未登记的原始刀也提供部位 API，并使用原版距离。


### 新增 native：查询身份、模式和本刀结果

编译时使用本仓库 `include/HanWeaponSystem.inc`，运行时需要 HanWeaponSystem 8.2。

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

上面是接口签名速查；包含头文件后不需要在自己的插件里再次声明这些 native。

| 接口 | 用途与返回规则 |
|---|---|
| `Han_IsManagedWeapon` | 实体是否匹配主配置的 `useclassname`；未登记的原始刀可以返回 false，但仍有刀事件 API |
| `Han_GetWeaponBaseClass` | 读取登记的 `classname`，例如 `weapon_knife`；未登记返回 false 并清空字符串，不猜测第三方实体模板 |
| `Han_GetClientViewModelMode` | `HanViewModel_VM0=0`、`HanViewModel_VM1=1`、`HanViewModel_NotReady=-1`；快切尚未接管、死亡或无有效模型时返回 NotReady |
| `Han_GetLatestKnifeAttackId` | 当前有效攻击编号；没有记录返回 0，可用于主动查询 |
| `Han_GetKnifeAttackResult` | 按明确编号复制缓存，不重新发射射线；失败清空输出。通常省略最后的 size 参数 |

登记身份与 VM 模式相互独立：原版武器启用 `han_oldweaponfix` 后可以使用 VM1，同时仍是未登记武器。查询没有副作用，不会改变显隐。NotReady 不能当成 VM0，也不要据此强行显示 VM1。

### 新增 forward：在正确阶段消费结果

```sourcepawn
forward void Han_OnKnifeAttack(int client, int weapon, int attackId,
    HanKnifeAttackType type);
forward void Han_OnKnifeTraceResult(int client, int weapon, int attackId);
forward Action Han_OnKnifeDamage(int client, int weapon, int attackId,
    int victim, bool supplemental, float &damage);
forward void Han_OnKnifeAttackFinished(int client, int weapon, int attackId);
```

| 通知 | 时机与用途 |
|---|---|
| `Han_OnKnifeAttack` | 检测缓存已备好，用于推进连招。`HanKnife_Primary=0` 为左键，`HanKnife_Secondary=1` 为右键；不要直接套用原始 PlayerAnimEvent 数值 |
| `Han_OnKnifeTraceResult` | Attack 后发布几何结果，包含挥空、世界、物体和玩家；命中不等于已经扣血 |
| `Han_OnKnifeDamage` | 扣血前、主配置 damage 调整前；原版命中和补伤害共用此入口，`supplemental=true` 表示补伤路径 |
| `Han_OnKnifeAttackFinished` | 同次 PostThinkPost 结算完成，用于读取最终结果，不用于修改已经发生的伤害 |

```text
检测并缓存 → Attack → TraceResult
                   → 有效伤害入口：Damage → 主配置 damage → 游戏伤害处理
                   → Finished（挥空或伤害被阻止也可能没有 Damage 通知）
```

这些是同步插件回调。`Han_OnKnifeDamage` 中调用查询 native 会立即返回本刀结果，不需要 Timer 或等待 `player_hurt`。返回 `Plugin_Continue` 保持伤害，`Plugin_Changed` 应用修改，`Plugin_Handled` / `Plugin_Stop` 阻止伤害。不要在回调里另行扣血或递归调用伤害接口。

### 结果字段与生命周期

使用 `HanKnifeResult result;` 接收，常用字段如下：

| 字段 | 含义 |
|---|---|
| `AttackId`, `WeaponRef`, `Type`, `Time`, `Range` | 本刀编号、武器引用、左右键、游戏时间及路径长度 |
| `Hit`, `EntityRef` | 几何命中及目标引用；世界为 0，挥空为 -1 |
| `Start[3]`, `Direction[3]`, `Position[3]`, `Normal[3]` | 起点、方向、接触点、表面法线；挥空时不把表面字段当作有效接触 |
| `SurfaceProps`, `SurfaceFlags`, `HitBox` | 主体检测的表面属性、标志和 hitbox 信息；静态物体可能以世界实体加 hitbox 表达 |
| `HitGroup`, `HitGroupValid` | 精确部位与有效标志；有效且部位为 1 才是明确头部，未知时为 0，不自动等于爆头 |
| `RangeChanged`, `DamageEntered`, `Supplemental` | 是否改距离、是否进入伤害入口、是否尝试补伤害；都不能单独证明已扣血 |
| `HealthDamage`, `Finished` | `player_hurt` 报告的玩家损血与是否完成结算；物体损血不由此字段报告 |

`WeaponRef` / `EntityRef` 是 EntRef，操作实体前用 `EntRefToEntIndex` 转换并验证。每次挥刀、切枪、攻击者死亡、断线或换图使旧记录失效，记录最长保留 2 秒。受害者死亡不会清除攻击者的本刀结果。缓存失效与“命中但部位未知”是不同情况；前者应停止使用旧数据。

### 完整示例：查看武器状态、读取结果并在扣血前调整

下面可作为示例。

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
    // 在此推进已配置的连招；请勿同时消耗 PlayerAnimEvent。
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
        damage *= 2.0; // 适用于扣除生命值前的原生及附加命中。
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

### 旧插件迁移与限制

- 用 `Han_OnKnifeAttack` 替换用于推进连招的 `PlayerAnimEvent`，不要同时消费两者。
- 用 `Han_OnKnifeDamage` 统一计算特殊伤害；补伤害不会触发旧 `SDKHook_TraceAttack`。完整迁移后移除旧伤害修改，避免倍率叠加两次。
- 如果旧插件有意把未知部位当作爆头，可自行使用 `!result.HitGroupValid || result.HitGroup == 1`；这属于该插件的玩法，不是主系统的定义。
- 新部位分类为有限距离、保留遮挡的检测，不保证与旧的无限目标射线逐次得到相同结果。
- `damage ""` 不增加主系统调整；填写后继续在上述伤害回调之后应用。原版与现有 `firespeed` 仍控制 CD，自定义命中结果改变时不会另写一套命中/挥空冷却。
- `han_knife_debug 1` 可记录攻击、伤害和结算时序，测试后恢复 0。材质效果允许旁观者重复；特殊碰撞、原版动画事件前的玻璃触发伤害、补伤路径的 TraceBleed 等边界见。

## 常见问题

**Q：添加一把新武器最少要配什么？**
A：主配置里 `classname` + `useclassname` + `vmodel` + `wmodel` 四项即可，其余字段全部可选。

**Q：为什么枪声有两个声音重叠？**
A：请确认填写了 `firesound`。已配置射击音效的武器会自动屏蔽原版枪声，只播自定义音效。

**Q：修改配置后不生效？**
A：需要 `sm plugins reload` 重载插件，并且玩家重新获取武器（换武器时会重新读取缓存）。

**Q：侧瞄按中键没反应？**
A：原版未绑定中键，先在控制台执行 `bind mouse3 +attack3`，并确认该武器配置了 `sideaim 1` 与侧瞄模型。

**Q：按 F 出来的是检视不是手电筒？**
A：这是正常行为，F 键（impulse 100）已被检视功能接管，检视动画与音效仅配置了 `inspect 1` 的武器生效，其他武器按 F 仍为原版手电逻辑。

**Q：背上/腿上看不到自定义武器模型？**
A：确认 cvar `han_wpsdisablebackweapon` 为 `0`（默认），且该武器的 `wmodel` 已正确配置；手持中的那把武器不会显示背负模型。

---

## 支持作者

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/Z8Z31PY52N)
