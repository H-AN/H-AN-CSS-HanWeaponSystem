// ============================================================================
// >> 资源下载统一管理(HanWeaponSystemDownload) - v6.5
//    所有客户端资源下载统一由 configs/HanWeaponSystem/Download.cfg 驱动。
//    其余模块只负责预缓存, 不承担下载逻辑。
//    本地测试/无下载站: 配置留空即可, 插件不要求任何下载。
//    容错: 每一条路径独立处理, 找不到/空目录/扫描失败仅跳过并提示, 绝不卡插件。
// ============================================================================

#define DOWNLOAD_CFG_PATH "configs/HanWeaponSystem/Download.cfg"

// 允许加入下载表的文件后缀(不区分大小写)
static const char g_sDownloadExts[][] = {
    "vtf", "vmt",                   // 纹理 / 材质
    "vtx", "mdl", "vvd", "phy",     // 模型
    "wav", "mp3"                    // 音效
};

void Download_OnPluginStart()
{
    CreateDownloadConfigIfNeeded();
}

void Download_OnMapStart()
{
    ProcessDownloadConfig();
}

// ============================================================================
// >> 无配置则创建默认(带用法说明 + 默认必需资源两行)
// ============================================================================
void CreateDownloadConfigIfNeeded()
{
    char path[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, path, sizeof(path), DOWNLOAD_CFG_PATH);

    if (FileExists(path))
        return;

    char dir[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, dir, sizeof(dir), "configs/HanWeaponSystem");
    if (!DirExists(dir))
        CreateDirectory(dir, 511);

    Handle file = OpenFile(path, "w");
    if (file == INVALID_HANDLE)
        return;

    WriteFileLine(file, "// HanWeaponSystem 资源下载配置文件");
    WriteFileLine(file, "// 用法: 每行一个路径, // 开头为注释, 反斜杠 \\ 会自动转换为 /");
    WriteFileLine(file, "// 1) 填目录(无后缀): 递归扫描该目录, 自动下载以下后缀的文件: vtf vmt vtx mdl vvd phy wav mp3");
    WriteFileLine(file, "//    例: sound/weapons/axe");
    WriteFileLine(file, "// 2) 填单个文件(带后缀): 精确下载该文件");
    WriteFileLine(file, "//    例: sound/weapons/axe/axe_break.wav");
    WriteFileLine(file, "// 本地测试或没有下载站时: 全部留空, 插件不做任何下载");
    WriteFileLine(file, "//");
    WriteFileLine(file, "// 默认必需资源(插件机瞄材质, 仓库已附带, 建议保留)");
    WriteFileLine(file, "materials/zoom/zoomin.vmt");
    WriteFileLine(file, "materials/zoom/zoomin.vtf");

    CloseHandle(file);
}

// ============================================================================
// >> 处理下载配置(每张图开始执行; 任何异常仅跳过该条, 不影响插件运行)
// ============================================================================
void ProcessDownloadConfig()
{
    char path[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, path, sizeof(path), DOWNLOAD_CFG_PATH);

    if (!FileExists(path))
        return;

    Handle file = OpenFile(path, "r");
    if (file == INVALID_HANDLE)
        return;

    char line[PLATFORM_MAX_PATH];
    while (!IsEndOfFile(file) && ReadFileLine(file, line, sizeof(line)))
    {
        TrimString(line);

        if (line[0] == '\0' || (line[0] == '/' && line[1] == '/'))
            continue;

        NormalizePath(line);

        // 去掉末尾斜杠, 避免拼接路径出现双斜杠
        int l = strlen(line);
        while (l > 0 && line[l - 1] == '/')
            line[--l] = '\0';

        if (IsDownloadFileExt(line))
            AddDownloadFile(line);
        else
            ScanDownloadDir(line);
    }

    CloseHandle(file);
}

// ============================================================================
// >> 判断路径是否带允许下载的后缀(带后缀=单文件, 无后缀=目录)
// ============================================================================
bool IsDownloadFileExt(const char[] path)
{
    int len = strlen(path);
    if (len == 0)
        return false;

    int lastDot = -1;
    for (int i = len - 1; i >= 0; i--)
    {
        if (path[i] == '.')
        {
            lastDot = i;
            break;
        }
        if (path[i] == '/' || path[i] == '\\')
            break;
    }

    if (lastDot < 0 || lastDot == len - 1)
        return false;

    char ext[8];
    strcopy(ext, sizeof(ext), path[lastDot + 1]);

    for (int i = 0; i < sizeof(ext) && ext[i] != '\0'; i++)
    {
        if (ext[i] >= 'A' && ext[i] <= 'Z')
            ext[i] += 'a' - 'A';
    }

    for (int i = 0; i < sizeof(g_sDownloadExts); i++)
    {
        if (StrEqual(ext, g_sDownloadExts[i]))
            return true;
    }

    return false;
}

// ============================================================================
// >> 精确单文件: 存在才加入下载表(相对 mod 根目录, 无需 BuildPath)
// ============================================================================
void AddDownloadFile(const char[] path)
{
    if (!FileExists(path))
    {
        PrintToServer("[HanWeaponSystem] 下载: 文件不存在, 跳过: %s", path);
        return;
    }

    AddFileToDownloadsTable(path);
}

// ============================================================================
// >> 目录: 递归扫描加入下载表(不存在/为空只提示不中断)
// ============================================================================
void ScanDownloadDir(const char[] dirPath)
{
    if (!DirExists(dirPath))
    {
        PrintToServer("[HanWeaponSystem] 下载: 目录不存在, 跳过: %s", dirPath);
        return;
    }

    int found = ScanDownloadDirRecursive(dirPath);

    if (found == 0)
        PrintToServer("[HanWeaponSystem] 下载: 目录为空, 无可用文件: %s", dirPath);
}

int ScanDownloadDirRecursive(const char[] relPath)
{
    int found = 0;

    Handle dir = OpenDirectory(relPath);
    if (dir == INVALID_HANDLE)
        return 0;

    char fileName[PLATFORM_MAX_PATH];
    FileType type;
    while (ReadDirEntry(dir, fileName, sizeof(fileName), type))
    {
        if (StrEqual(fileName, ".") || StrEqual(fileName, ".."))
            continue;

        if (type == FileType_Directory)
        {
            char relChild[PLATFORM_MAX_PATH];
            Format(relChild, sizeof(relChild), "%s/%s", relPath, fileName);
            found += ScanDownloadDirRecursive(relChild);
        }
        else if (type == FileType_File && IsDownloadFileExt(fileName))
        {
            char relFile[PLATFORM_MAX_PATH];
            Format(relFile, sizeof(relFile), "%s/%s", relPath, fileName);
            AddFileToDownloadsTable(relFile);
            found++;
        }
    }

    CloseHandle(dir);
    return found;
}
