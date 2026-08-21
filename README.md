# [华仔]武器系统 HanWeaponSystem

![语言](https://img.shields.io/badge/语言-中文-red) ![English](https://img.shields.io/badge/Language-English-blue) ![Game](https://img.shields.io/badge/Game-CS%3A%20Source-yellow) ![Platform](https://img.shields.io/badge/SourceMod-1.12-orange)

**[ 简体中文 ]** | [ English ](readme.en.md)

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

| 操作 | 按键/命令 |
|---|---|
| 购买武器 | `sm_buy`（可在 BuyData.cfg 修改） |
| 出售武器 | `sm_sell`（可在 BuyData.cfg 修改） |
| 机瞄 | 鼠标右键 |
| 侧瞄 | 鼠标中键 |
| 检视 | F 键 |
| 奔跑 | Shift |

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

## 常见问题

**Q：添加一把新武器最少要配什么？**
A：主配置里 `classname` + `useclassname` + `vmodel` + `wmodel` 四项即可，其余字段全部可选。

**Q：为什么枪声有两个声音重叠？**
A：请确认填写了 `firesound`。已配置射击音效的武器会自动屏蔽原版枪声，只播自定义音效。

**Q：修改配置后不生效？**
A：需要 `sm plugins reload` 重载插件，并且玩家重新获取武器（换武器时会重新读取缓存）。

---

## 支持作者

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/Z8Z31PY52N)
