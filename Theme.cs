using System.Drawing;

namespace FileExplorerPro;

/// <summary>
/// 主题颜色常量：深色主题（参考 VS Code Dark+）
/// 切换主题只需修改这里的常量，无需改其他代码
/// </summary>
internal static class Theme
{
    // ===== 背景色 =====
    public static readonly Color BgMain = Color.FromArgb(30, 30, 30);          // 主背景
    public static readonly Color BgSidebar = Color.FromArgb(37, 37, 38);       // 侧栏背景
    public static readonly Color BgTabBar = Color.FromArgb(45, 45, 48);        // 标签栏背景
    public static readonly Color BgTabInactive = Color.FromArgb(45, 45, 48);   // 未激活标签
    public static readonly Color BgTabActive = Color.FromArgb(30, 30, 30);     // 激活标签
    public static readonly Color BgTabHover = Color.FromArgb(60, 60, 65);      // 标签悬停
    public static readonly Color BgAddressBar = Color.FromArgb(45, 45, 48);    // 路径栏背景
    public static readonly Color BgList = Color.FromArgb(30, 30, 30);          // 文件列表背景
    public static readonly Color BgListItem = Color.FromArgb(30, 30, 30);      // 列表项
    public static readonly Color BgListItemAlt = Color.FromArgb(37, 37, 37);   // 列表项交替色
    public static readonly Color BgListItemHover = Color.FromArgb(50, 50, 55); // 列表项悬停
    public static readonly Color BgListItemSelected = Color.FromArgb(9, 71, 113); // 选中项
    public static readonly Color BgButton = Color.FromArgb(60, 60, 65);        // 按钮背景
    public static readonly Color BgButtonHover = Color.FromArgb(75, 75, 80);   // 按钮悬停
    public static readonly Color BgButtonPrimary = Color.FromArgb(0, 122, 204); // 主按钮
    public static readonly Color BgQuickJump = Color.FromArgb(40, 40, 44);     // 快速跳转面板

    // ===== 前景色 =====
    public static readonly Color FgMain = Color.FromArgb(220, 220, 220);       // 主文字
    public static readonly Color FgSecondary = Color.FromArgb(153, 153, 153);  // 次要文字
    public static readonly Color FgTabInactive = Color.FromArgb(180, 180, 180);
    public static readonly Color FgButton = Color.FromArgb(220, 220, 220);
    public static readonly Color FgPathCrumb = Color.FromArgb(86, 156, 214);   // 路径面包屑（链接蓝）

    // ===== 边框 =====
    public static readonly Color Border = Color.FromArgb(60, 60, 65);
    public static readonly Color BorderLight = Color.FromArgb(90, 90, 90);
    public static readonly Color BorderFocus = Color.FromArgb(0, 122, 204);

    // ===== 强调色 =====
    public static readonly Color Accent = Color.FromArgb(0, 122, 204);         // 蓝
    public static readonly Color AccentFolder = Color.FromArgb(220, 180, 60);  // 文件夹金
    public static readonly Color AccentClose = Color.FromArgb(232, 17, 35);    // 关闭红
    public static readonly Color AccentStar = Color.FromArgb(255, 204, 0);     // 收藏星

    // ===== 字体 =====
    public static Font GetFont(float size, FontStyle style = FontStyle.Regular)
        => new("Microsoft YaHei UI", size, style);
    public static Font GetUiFont(float size = 9) => GetFont(size);
    public static Font GetBoldFont(float size = 9) => GetFont(size, FontStyle.Bold);
    public static Font GetSmallFont(float size = 8) => GetFont(size);
}
