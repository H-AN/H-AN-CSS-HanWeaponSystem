// ============================================================================
// >> Camera 系统(内联版，仅供本插件使用，不影响外部 camera 插件)
// ============================================================================
// >> 原作者声明 / Original Author Credit
// 本模块的相机逻辑移植自开源插件 "Camera"（屏幕材质挂接实体）。
// This module's camera logic is ported from the open-source plugin "Camera"
// (screen materials with hook entity).
//
// 原作者 / Original Author: 花花花。(Flower)
// 版本 / Version: Flower
// 主页 / Homepage: mufiu.com
//
// 感谢原作者的无私分享！本实现已在此基础上内联重构。
// Special thanks to the original author for sharing! This implementation has
// been refactored and inlined on top of the original work.
// ============================================================================

int gI_Camera[MAXPLAYERS + 1];
int gI_CameraLink[MAXPLAYERS + 1][2048];

void Camera_OnMapStart()
{
    for (int i = 1; i <= MaxClients; i++)
    {
        gI_Camera[i] = 0;
        for (int j = MaxClients + 1; j < GetMaxEntities(); j++)
        {
            gI_CameraLink[i][j] = 0;
        }
    }
}

void SpawnCameraFrame(any data)
{
    DataPack dp = view_as<DataPack>(data);
    dp.Reset();
    int entity = dp.ReadCell();
    int target = dp.ReadCell();
    int fov = dp.ReadCell();
    delete dp;

    SpawnCamera(target, entity, fov);
}

void SpawnCamera(int client, int entity, int fov)
{
    char sCameraName[MAX_NAME_LENGTH], sTargetName[MAX_NAME_LENGTH];
    FormatEx(sCameraName, sizeof(sCameraName), "sm_camera_%d", client);
    GetEntPropString(entity, Prop_Data, "m_iName", sTargetName, sizeof(sTargetName));

    if (!gI_Camera[client] || !(IsValidEdict(gI_Camera[client]) && IsValidEntity(gI_Camera[client])))
    {
        int camera = CreateEntityByName("point_camera");

        if (fov < 0 || fov > 90)
            fov = 90;
        char FovStr[4];
        IntToString(fov, FovStr, sizeof(FovStr));
        TrimString(FovStr);

        DispatchKeyValue(camera, "FOV", FovStr);
        DispatchKeyValue(camera, "targetname", sCameraName);
        DispatchKeyValue(camera, "fogMaxDensity", "1");
        DispatchKeyValue(camera, "fogStart", "2048");
        DispatchKeyValue(camera, "fogEnd", "4096");
        DispatchKeyValue(camera, "targetname", sCameraName);
        DispatchKeyValue(camera, "spawnflags", "0");
        DispatchKeyValue(camera, "rendersize", "2048");

        DispatchSpawn(camera);
        ActivateEntity(camera);

        gI_Camera[client] = camera;
    }

    if (!gI_CameraLink[client][entity] || !(IsValidEdict(gI_CameraLink[client][entity]) && IsValidEntity(gI_CameraLink[client][entity])))
    {
        int camera_link = CreateEntityByName("info_camera_link");
        char sCameraLinkName[MAX_NAME_LENGTH];
        FormatEx(sCameraLinkName, sizeof(sCameraLinkName), "sm_link_%d%d", gI_Camera[client], entity);

        DispatchKeyValue(camera_link, "targetname", sCameraLinkName);
        DispatchKeyValue(camera_link, "PointCamera", sCameraName);
        DispatchKeyValue(camera_link, "target", sTargetName);

        DispatchSpawn(camera_link);
        ActivateEntity(camera_link);

        gI_CameraLink[client][entity] = camera_link;
    }

    SetCameraParent(client);
}

void SetCameraParent(int client)
{
    if (gI_Camera[client] && IsValidEdict(gI_Camera[client]) && IsValidEntity(gI_Camera[client]))
    {
        AcceptEntityInput(gI_Camera[client], "ClearParent");

        float fOrigin[3], fAngles[3];
        int viewmodel = GetEntPropEnt(client, Prop_Send, "m_hViewModel");
        GetClientAbsOrigin(client, fOrigin);
        GetClientAbsAngles(client, fAngles);
        TeleportEntity(gI_Camera[client], fOrigin, fAngles, NULL_VECTOR);

        SetVariantString("!activator");
        AcceptEntityInput(gI_Camera[client], "SetParent", viewmodel);
    }
}

void Camera_ResetCamera(int target)
{
    SetCameraParent(target);
}

bool Camera_HasLink(int entity, int target)
{
    return gI_CameraLink[target][entity] && IsValidEdict(gI_CameraLink[target][entity]) && IsValidEntity(gI_CameraLink[target][entity]);
}

void Camera_RemoveLink(int entity, int target, bool clean)
{
    if (clean && gI_Camera[target] && IsValidEdict(gI_Camera[target]) && IsValidEntity(gI_Camera[target]))
    {
        RemoveEdict(gI_Camera[target]);
        AcceptEntityInput(gI_Camera[target], "Kill");
    }
    if (gI_CameraLink[target][entity] && IsValidEdict(gI_CameraLink[target][entity]) && IsValidEntity(gI_CameraLink[target][entity]))
    {
        RemoveEdict(gI_CameraLink[target][entity]);
        AcceptEntityInput(gI_CameraLink[target][entity], "Kill");
    }
}

void Camera_CreateLink(int entity, int target, int fov, bool nextframe)
{
    char sTargetName[MAX_NAME_LENGTH];
    GetEntPropString(entity, Prop_Data, "m_iName", sTargetName, sizeof(sTargetName));
    if (StrEqual(sTargetName, ""))
    {
        FormatEx(sTargetName, sizeof(sTargetName), "sm_entity_%d", entity);
        SetEntPropString(entity, Prop_Data, "m_iName", sTargetName);
    }

    if (nextframe)
    {
        DataPack dp = new DataPack();
        dp.WriteCell(entity);
        dp.WriteCell(target);
        dp.WriteCell(fov);
        RequestFrame(SpawnCameraFrame, dp);

        return;
    }

    SpawnCamera(target, entity, fov);
}

int Camera_GetCamera(int client)
{
    return gI_Camera[client];
}