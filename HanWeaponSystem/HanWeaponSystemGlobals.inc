#define VERSION "6.1"
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
int g_WeaponCount = 0;

bool SpawnCheck[MAXPLAYERS + 1];
int ClientVM[MAXPLAYERS + 1][2];
bool IsCustom[MAXPLAYERS + 1];
bool IsCustom2[MAXPLAYERS + 1];
bool g_bVM0BlockHooked[MAXPLAYERS + 1];     // 0号VM 传输屏蔽钩子是否已挂(新武器保险用)

float g_fModifyNextAttack[MAXPLAYERS + 1];
bool g_bPressingAttack2[MAXPLAYERS + 1];
bool g_bWeaponAutoFire[MAXPLAYERS + 1];

int offsCollision;
int g_FakeModels[2048];

// ============================================================================
// >> 机瞄全局
// ============================================================================
Handle g_hZoomConfig = INVALID_HANDLE;

bool g_bZooming[MAXPLAYERS + 1];
int g_iZoomInSwitch[MAXPLAYERS + 1];        // 0=无过渡 1=开镜过渡中 2=关镜过渡中
float g_fZoomBlock[MAXPLAYERS + 1];         // 机瞄切换防抖时间

Handle g_ForwardClientZoom = INVALID_HANDLE;

// ============================================================================
// >> 检视全局
// ============================================================================
Handle g_hInspectConfig = INVALID_HANDLE;

bool g_bInspecting[MAXPLAYERS + 1];
int g_iInspectWeapon[MAXPLAYERS + 1];       // 检视开始时武器实体
float g_fInspectEnd[MAXPLAYERS + 1];        // 检视结束时间
float g_fInspectBlock[MAXPLAYERS + 1];      // 检视触发防抖时间
int g_iInspectRestart[MAXPLAYERS + 1];      // 重复检视重启标记(先切其他序列再切回强制重播)