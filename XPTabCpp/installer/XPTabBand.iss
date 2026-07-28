; ============================================================
; XPTabBand Inno Setup 安装脚本
; ============================================================
; 编译方法：
;   1. 安装 Inno Setup：https://jrsoftware.org/isdl.php
;   2. 双击此 .iss 文件，打开 Inno Setup Compiler
;   3. 按 Ctrl+F9 或菜单 Build → Compile
;   4. 生成的 setup.exe 在 .iss 同目录的 Output\ 子目录
; ============================================================

#define MyAppName          "XPTabBand"
#define MyAppVersion       "1.2.0"
#define MyAppPublisher     "XPTabCpp Project"
#define MyAppURL          "https://github.com/yourname/XPTabBand"
#define MyAppExeName       "XPTabBand"

[Setup]
; 应用基本信息
AppId={{8F2E4A1B-3C5D-4E6F-9A8B-7C6D5E4F3A2B}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
; 不要在开始菜单创建组（用户不需要快捷方式）
OutputDir=Output
OutputBaseFilename=XPTabBand-{#MyAppVersion}-setup
Compression=lzma2/ultra64
SolidCompression=yes
; 要求管理员权限
PrivilegesRequired=admin
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
; 安装包图标（与 DLL 一致）
SetupIconFile=XPTabBand.ico
; 卸载时显示自定义图标
UninstallDisplayIcon={app}\XPTabBand.ico
UninstallDisplayName={#MyAppName} {#MyAppVersion}
; 向导风格
WizardStyle=modern
ShowLanguageDialog=yes

[Languages]
Name: "chinesesimp"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; 主 DLL
Source: "..\deploy\XPTabBand.dll"; DestDir: "{app}"; Flags: ignoreversion
; 应用图标（用于卸载列表显示）
Source: "XPTabBand.ico"; DestDir: "{app}"; Flags: ignoreversion
; README 文档
Source: "..\deploy\README.md"; DestDir: "{app}"; Flags: ignoreversion

[Run]
; 注册 COM 组件（regsvr32 会调用 DLL 的 DllRegisterServer）
Filename: "{sys}\regsvr32.exe"; Parameters: "/s ""{app}\XPTabBand.dll"""; \
    Flags: runhidden; StatusMsg: "正在注册 COM 组件..."

[UninstallRun]
; 注销 COM 组件
Filename: "{sys}\regsvr32.exe"; Parameters: "/u /s ""{app}\XPTabBand.dll"""; \
    Flags: runhidden; RunOnceId: "UnregCOM"

[Code]
// ============================================================
// 自定义函数：关闭资源管理器
// ============================================================
procedure CloseExplorer;
var
  ResultCode: Integer;
begin
  // taskkill 会关闭所有 explorer.exe，包括任务栏和文件夹窗口
  Exec(ExpandConstant('{cmd}'), '/c taskkill /f /im explorer.exe', '', SW_HIDE,
       ewWaitUntilTerminated, ResultCode);
  Sleep(2000);
end;

// ============================================================
// 自定义函数：启动资源管理器
// ============================================================
procedure StartExplorer;
var
  ResultCode: Integer;
begin
  Exec(ExpandConstant('{cmd}'), '/c start explorer.exe', '', SW_HIDE,
       ewWaitUntilTerminated, ResultCode);
  Sleep(2000);
end;

// ============================================================
// 安装前：关闭 Explorer，避免 DLL 被占用
// ============================================================
function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  WizardForm.StatusLabel.Caption := '正在关闭资源管理器...';
  CloseExplorer;
end;

// ============================================================
// 安装后：启动 Explorer
// ============================================================
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    WizardForm.StatusLabel.Caption := '正在启动资源管理器...';
    StartExplorer;
  end;
end;

// ============================================================
// 卸载前：询问是否保留收藏夹数据
// ============================================================
var
  KeepFavorites: Boolean;

function InitializeUninstall(): Boolean;
begin
  KeepFavorites := (MsgBox('是否保留收藏夹数据？' #13#10 '选择"是"保留，选择"否"删除。',
                          mbConfirmation, MB_YESNO or MB_DEFBUTTON1) = IDYES);
  Result := True;
end;

// ============================================================
// 卸载前：关闭 Explorer
// ============================================================
function InitializeUninstallProgress(): Boolean;
begin
  // 显示提示
  MsgBox('即将关闭资源管理器以完成卸载。' #13#10 '请保存所有正在编辑的文件。',
         mbInformation, MB_OK);

  // 关闭 explorer
  CloseExplorer;

  Result := True;
end;

// ============================================================
// 卸载后：删除收藏夹数据 + 启动 Explorer
// ============================================================
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  FavDir: String;
begin
  if CurUninstallStep = usPostUninstall then
  begin
    // 删除收藏夹数据
    if not KeepFavorites then
    begin
      FavDir := ExpandConstant('{userappdata}\XPTabCpp');
      if DirExists(FavDir) then
      begin
        DelTree(FavDir, True, True, True);
      end;
    end;

    // 启动 explorer
    StartExplorer;
  end;
end;
