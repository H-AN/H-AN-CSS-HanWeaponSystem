#if defined _HanWeaponSystem_included_
#endinput
#endif
#define _HanWeaponSystem_included_

/**
 * 获取玩家的 ViewModel 实体。
 *
 * @param client    玩家索引（1 ~ MaxClients）
 * @param index     模型类型，0 = ViewModel, 1 = Predicted ViewModel
 * @return          实体索引，失败返回 -1
 */
native int GetClientViewModel(int client, int index);

/**
 * 获取玩家是否在按住右键（IN_ATTACK2），
 * 即使原始按钮被修改，也能正确判断。
 *
 * @param client 玩家索引
 * @return true = 按下右键，false = 没有按
 */
native bool IsClientPressingAttack2(int client);

/**
 * 当前武器是否支持自动连发（由主插件配置判断）
 */
native bool IsWeaponAutoFire(int client);

/**
 * 玩家当前是否处于机瞄状态。
 *
 * @param client 玩家索引
 * @return true = 机瞄中
 */
native bool Han_IsClientZooming(int client);

/**
 * 当前武器是否支持机瞄（zoom 配置为 1）。
 *
 * @param client 玩家索引
 * @return true = 可机瞄
 */
native bool Han_IsWeaponZoomable(int client);

/**
 * 玩家机瞄状态变化时触发。
 *
 * @param client  玩家索引
 * @param zooming true = 进入机瞄, false = 退出机瞄
 */
forward void Han_OnClientZoom(int client, bool zooming);

/**
 * 玩家当前是否处于侧瞄状态。
 *
 * @param client 玩家索引
 * @return true = 侧瞄中
 */
native bool Han_IsClientSideAiming(int client);

/**
 * 当前武器是否支持侧瞄（sideaim 配置为 1 且填写了侧瞄模型）。
 *
 * @param client 玩家索引
 * @return true = 可侧瞄
 */
native bool Han_IsWeaponSideAimable(int client);

/**
 * 玩家侧瞄状态变化时触发。
 *
 * @param client 玩家索引
 * @param aiming true = 进入侧瞄, false = 退出侧瞄
 */
forward void Han_OnClientSideAim(int client, bool aiming);

/**
 * 玩家当前是否处于检视中。
 *
 * @param client 玩家索引
 * @return true = 检视中
 */
native bool Han_IsClientInspecting(int client);

/**
 * 当前武器是否支持检视（inspect 配置为 1）。
 *
 * @param client 玩家索引
 * @return true = 可检视
 */
native bool Han_IsWeaponInspectable(int client);

/**
 * 玩家检视状态变化时触发。
 *
 * @param client     玩家索引
 * @param inspecting true = 开始检视, false = 结束检视
 */
forward void Han_OnClientInspect(int client, bool inspecting);

/**
 * 玩家当前是否处于持枪奔跑中。
 *
 * @param client 玩家索引
 * @return true = 奔跑中
 */
native bool Han_IsClientRunning(int client);

/**
 * 玩家持枪奔跑状态变化时触发。
 *
 * @param client  玩家索引
 * @param running true = 开始奔跑, false = 结束奔跑(含过渡播完)
 */
forward void Han_OnClientRun(int client, bool running);

/**
 * 玩家开始空仓换弹时触发（弹匣打空后换弹，一次性广播，无结束事件）。
 *
 * @param client 玩家索引
 */
forward void Han_OnClientEmptyReload(int client);

/**
 * 玩家开始战术换弹时触发（弹匣还有子弹时换弹，一次性广播，无结束事件）。
 *
 * @param client 玩家索引
 */
forward void Han_OnClientTacticalReload(int client);

/**
 * 播放玩家的自定义动画（最高优先级）。
 *
 * 触发时强制退出机瞄/侧瞄/奔跑/检视/换弹接管；播放期间 机瞄/侧瞄/奔跑 可打断
 * （interruptable=true 时）；检视不会打断；结束只回 idle，不恢复被中断状态。
 * 适用自定义模型武器（双VM驱动，1号VM 播放动画序列）。
 *
 * @param client        玩家索引
 * @param seq           动画序列号
 * @param frames        动画时长（帧，30fps 基准）
 * @param repeat        true=播放中允许重播打断自己, false=播放中再次调用返回 false
 * @param interruptable true=可被机瞄/侧瞄/奔跑打断(默认), false=绝对霸体播完
 * @return true=已开始播放, false=参数非法/不在游戏/播放中且不允许重播
 */
native bool Han_SetClientCustomAnim(int client, int seq, int frames, bool repeat = false, bool interruptable = true);

/**
 * 玩家当前是否在播放自定义动画。
 *
 * @param client 玩家索引
 * @return true = 播放中
 */
native bool Han_IsClientCustomAnim(int client);

/**
 * 停止播放玩家的自定义动画（广播结束事件，效果等同自然结束）。
 *
 * @param client 玩家索引
 */
native void Han_StopClientCustomAnim(int client);

/**
 * 玩家自定义动画开始时触发（含重播打断自己）。
 *
 * @param client 玩家索引
 * @param seq    动画序列号
 * @param frames 动画时长（帧，30fps 基准）
 */
forward void Han_OnClientCustomAnimStart(int client, int seq, int frames);

/**
 * 玩家自定义动画结束时触发（自然结束 / 被机瞄侧瞄奔跑打断 / 切枪 / 死亡）。
 *
 * @param client 玩家索引
 */
forward void Han_OnClientCustomAnimEnd(int client);

// 8.2: 身份与展示状态彼此独立。查询不改变任何模型状态。
enum HanViewModelMode
{
    HanViewModel_NotReady = -1,
    HanViewModel_VM0 = 0,
    HanViewModel_VM1 = 1
};

native bool Han_IsManagedWeapon(int weapon);
/** 仅返回配置登记的 classname；未登记返回 false，并清空 buffer。 */
native bool Han_GetWeaponBaseClass(int weapon, char[] buffer, int maxlen);
/** 切枪尚未完成接管、死亡、无武器时返回 NotReady。 */
native HanViewModelMode Han_GetClientViewModelMode(int client);

enum HanKnifeAttackType
{
    HanKnife_Primary = 0,   // 左键；对应 PlayerAnimEvent 的 1
    HanKnife_Secondary = 1  // 右键；对应 PlayerAnimEvent 的 0
};

/**
 * 8.2 固定布局。使用 HanKnifeResult result 接收。
 * EntityRef/WeaponRef 是 EntRef，-1 表示无目标，世界为 0；不是裸实体索引。
 * Hit 是几何命中，不代表扣血。HitGroupValid=false 时 HitGroup=0，不等于爆头。
 * HealthDamage 来自 player_hurt（仅玩家），物体不报告实际扣血。
 * DamageEntered 表示进入伤害入口，Supplemental 表示尝试过补伤害，均不保证扣血。
 */
enum struct HanKnifeResult
{
    int AttackId;
    int WeaponRef;
    HanKnifeAttackType Type;
    float Time;
    float Range;
    bool Hit;
    int EntityRef;
    float Start[3];
    float Direction[3];
    float Position[3];
    float Normal[3];
    int SurfaceProps;
    int SurfaceFlags;
    int HitBox;
    int HitGroup;
    bool HitGroupValid;
    bool RangeChanged;
    bool DamageEntered;
    bool Supplemental;
    int HealthDamage;
    bool Finished;
}
/** 当前记录 ID；无记录返回 0。每次挥刀、换武器、死亡、断线、换图使旧记录失效。
 * 记录最长保留 2 秒，致死受害者仍可由攻击者的本次记录查询。
 */
native int Han_GetLatestKnifeAttackId(int client);
/** 必须传入明确 attackId；失败清零输出。只读取缓存，绝不重新发射射线。 */
// Use cell-array transport for SourcePawn 1.11; pass HanKnifeResult unchanged.
// The native name, cell layout and explicit size remain ABI-compatible with 1.12.
native bool Han_GetKnifeAttackResult(int client, int attackId, any[] result, int size = sizeof(HanKnifeResult));

/** 检测已准备好；先 Attack，再 TraceResult，再伤害入口，最后 Finished。
 * 原有 PlayerAnimEvent 与本 forward 二选一推进连招，勿重复消费。
 * 处理所有真实网络类为 CKnife 的武器，包括未登记的原始 weapon_knife。
 * 部位检测与材质反馈统一启用，无需 knife_hitgroups / knife_effects 配置。
 * HitGroupValid 与 HitGroup 仅描述命中部位，不自动施加爆头倍率。
 */
forward void Han_OnKnifeAttack(int client, int weapon, int attackId, HanKnifeAttackType type);
forward void Han_OnKnifeTraceResult(int client, int weapon, int attackId);
/** 原版与补伤害共用入口，在主系统 damage 配置调整之前执行。
 * Changed 接受新伤害，Handled/Stop 阻止本次伤害；不会因被阻止而再次补伤害。
 * 补伤害不触发 SDKHook_TraceAttack。不得在此递归发起另一笔刀伤害。
 */
forward Action Han_OnKnifeDamage(int client, int weapon, int attackId, int victim, bool supplemental, float &damage);
/** 同次 PostThinkPost 中结算完毕。查询结果包含玩家实际损血；不会在此重追踪位置。 */
forward void Han_OnKnifeAttackFinished(int client, int weapon, int attackId);
