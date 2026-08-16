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