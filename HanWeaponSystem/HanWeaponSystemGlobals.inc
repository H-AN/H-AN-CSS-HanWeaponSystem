#define VERSION "6.8"
#define MAX_WEAPON 256
#define EF_NODRAW 32

bool g_bWeaponConfigReady = false;

Handle g_hWeaponConfig = INVALID_HANDLE;
Handle g_hWeaponTrie = INVALID_HANDLE;

char g_WeaponCommand[MAX_WEAPON][64];
char g_WeaponClassname[MAX_WEAPON][64];
char g_WeaponUseClassname[MAX_WEAPON][64];
char g_WeaponTeam[MAX_WEAPON][64];
char g_WeaponAmmo[MAX_WEAPON][16];
int g_WeaponMaxAmmo[MAX_WEAPON];   // 弹夹容量(从 scripts/<useclassname>.txt 根节点 clip_size 预缓存, 0=无子弹/脚本缺失, 换弹音效不服务)
Handle g_hWeaponScript[MAX_WEAPON]; // 解析后的武器脚本KV句柄(scripts/<useclassname>.txt 根节点, 插件加载时只解析一次; INVALID_HANDLE=解析失败)
int g_WeaponCount = 0;

bool SpawnCheck[MAXPLAYERS + 1];
int ClientVM[MAXPLAYERS + 1][2];
bool IsCustom[MAXPLAYERS + 1];
bool IsCustom2[MAXPLAYERS + 1];
bool g_bVM0BlockHooked[MAXPLAYERS + 1];     // 0号VM 传输屏蔽钩子是否已挂(新武器保险用)

float g_fModifyNextAttack[MAXPLAYERS + 1];
bool g_bPressingAttack2[MAXPLAYERS + 1];
bool g_bWeaponAutoFire[MAXPLAYERS + 1];
bool g_bGrenadePullPrev[MAXPLAYERS + 1];   // 手雷拉保险音上升沿记录(持自定义雷+IN_ATTACK按下)

int offsCollision;
int g_FakeModels[2048];
char g_GrenadePrefix[2048][64]; // 手雷投射物 → 自定义音效文件夹前缀(SpawnPost记录, 投射物音效同名文件替换用; 空=非自定义雷)

// ============================================================================
// >> 机瞄全局
// ============================================================================
Handle g_hZoomConfig = INVALID_HANDLE;

bool g_bZooming[MAXPLAYERS + 1];
int g_iZoomInSwitch[MAXPLAYERS + 1];        // 0=无过渡 1=开镜过渡中 2=关镜过渡中
float g_fZoomBlock[MAXPLAYERS + 1];         // 机瞄切换防抖时间

bool g_bSideAiming[MAXPLAYERS + 1];
int g_iSideAimInSwitch[MAXPLAYERS + 1];     // 0=无过渡 1=进入侧瞄过渡中 2=退出侧瞄过渡中
float g_fSideAimBlock[MAXPLAYERS + 1];      // 侧瞄切换防抖时间

Handle g_ForwardClientZoom = INVALID_HANDLE;
Handle g_ForwardClientSideAim = INVALID_HANDLE;

// ============================================================================
// >> 检视全局
// ============================================================================
Handle g_hInspectConfig = INVALID_HANDLE;

bool g_bInspecting[MAXPLAYERS + 1];
int g_iInspectWeapon[MAXPLAYERS + 1];       // 检视开始时武器实体
float g_fInspectEnd[MAXPLAYERS + 1];        // 检视结束时间
float g_fInspectBlock[MAXPLAYERS + 1];      // 检视触发防抖时间
int g_iInspectRestart[MAXPLAYERS + 1];      // 重复检视重启标记(先切其他序列再切回强制重播)
char g_sInspectSound[MAXPLAYERS + 1][PLATFORM_MAX_PATH]; // 当前检视音效路径(用于停止/重播)
int g_iInspectSoundGen[MAXPLAYERS + 1];  // 检视音效调度代次(每次检视递增, 防旧定时器串播)

Handle g_ForwardClientInspect = INVALID_HANDLE;

// ============================================================================
// >> 奔跑全局
// ============================================================================
Handle g_hRunConfig = INVALID_HANDLE;

bool g_bRunning[MAXPLAYERS + 1];         // 奔跑流程中(stage != 0), 供互斥/API使用
int g_iRunStage[MAXPLAYERS + 1];         // 0=无 1=runin过渡 2=奔跑循环 3=runout过渡
int g_iRunWeapon[MAXPLAYERS + 1];        // 奔跑开始时武器实体
float g_fRunSwitch[MAXPLAYERS + 1];      // 循环切换/过渡结束时间戳
bool g_bRunFrame[MAXPLAYERS + 1];        // 循环动画两帧切换标记
float g_fSpeedPressTime[MAXPLAYERS + 1]; // IN_SPEED 按下时刻(点按判定用)
bool g_bSpeedDown[MAXPLAYERS + 1];       // IN_SPEED 当前是否按下(边沿检测用)
float g_fRunShakeNext[MAXPLAYERS + 1];   // 下次抖动触发时间(每 RUN_SHAKE_INTERVAL 秒一次)
float g_fRunShakeYaw[MAXPLAYERS + 1];    // 抖动弹簧: 当前偏航角(度, 左右摆动, 写入 m_vecPunchAngle.y)
float g_fRunShakeYawVel[MAXPLAYERS + 1]; // 抖动弹簧: 偏航角速度(度/秒)
float g_fRunShakePitch[MAXPLAYERS + 1];  // 抖动弹簧: 当前俯仰角(度, 点头, 写入 m_vecPunchAngle.x)
float g_fRunShakePitchVel[MAXPLAYERS + 1]; // 抖动弹簧: 俯仰角速度(度/秒)
bool g_bRunShakeSign[MAXPLAYERS + 1];    // 抖动脉冲交替符号(模拟左右脚交替摆动)

Handle g_ForwardClientRun = INVALID_HANDLE;

// ============================================================================
// >> 空仓/战术换弹全局
// ============================================================================
Handle g_hEmptyReloadConfig = INVALID_HANDLE;

bool g_bEmptyReloading[MAXPLAYERS + 1];     // 空仓/战术换弹动画接管中(由换弹钩子置位)
int g_iEmptyReloadWeapon[MAXPLAYERS + 1];   // 换弹接管开始时武器实体
int g_iEmptyReloadSeq[MAXPLAYERS + 1];      // 正在强制的换弹动画序列号

bool g_bEmptyReloadTiming[MAXPLAYERS + 1];     // 换弹时长同步接管中(跳过全局reloadspeed)
int g_iEmptyReloadTimeFrames[MAXPLAYERS + 1];  // 配置帧数
int g_iEmptyReloadFps[MAXPLAYERS + 1];         // 配置fps(默认30, 按需改24)
float g_fEmptyReloadStart[MAXPLAYERS + 1];     // 接管起始时间(用于计算结束时刻)

Handle g_ForwardClientEmptyReload = INVALID_HANDLE;
Handle g_ForwardClientTacticalReload = INVALID_HANDLE;