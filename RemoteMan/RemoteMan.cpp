
// RemoteMan.cpp : 定义应用程序的类行为。
//

#include "stdafx.h"
#include "RemoteMan.h"
#include "RemoteManDlg.h"
#include "InputPasswordDlg.h"
#include "Aes.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CRemoteManApp

BEGIN_MESSAGE_MAP(CRemoteManApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CRemoteManApp 构造

CRemoteManApp::CRemoteManApp()
{
	// 支持重新启动管理器
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的一个 CRemoteManApp 对象
//控制模式名称，更改这个后要同步更改图标顺序
char const CTRL_MODE[4][7]={CTRL_MODE_RDP_NAME,CTRL_MODE_RADMIN_NAME,CTRL_MODE_SSH_NAME,CTRL_MODE_VNC_NAME};
CRemoteManApp theApp;


// CRemoteManApp 初始化

BOOL CRemoteManApp::InitInstance()
{
	// 如果一个运行在 Windows XP 上的应用程序清单指定要
	// 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
	//则需要 InitCommonControlsEx()。否则，将无法创建窗口。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 将它设置为包括所有要在应用程序中使用的
	// 公共控件类。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	if (!AfxSocketInit())
	{
		AfxMessageBox("初始化套接字错误");
		return FALSE;
	}

	AfxEnableControlContainer();

	// 创建 shell 管理器，以防对话框包含
	// 任何 shell 树视图控件或 shell 列表视图控件。
	CShellManager *pShellManager = new CShellManager;

	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项
	// TODO: 应适当修改该字符串，
	// 例如修改为公司或组织名
	SetRegistryKey(_T("应用程序向导生成的本地应用程序"));

	CRemoteManDlg dlg;			//在这里打开了数据库并读取了参数
	//输入开机密码
	if (dlg.SysConfig.SysPassword[0]!=0)
	{
		char str[66];
		CInputPasswordDlg pswdlg;
		if (pswdlg.DoModal()!=IDOK) return FALSE;
		if (pswdlg.m_Password.GetLength()>PASSWORD_MAXLEN ||
			strcmp(dlg.SysConfig.SysPassword, AesEnCodeToStr((char const*)pswdlg.m_Password,pswdlg.m_Password.GetLength(),str,AES_KEY))!=0)
		{
			AfxMessageBox("密码错误");
			return FALSE;
		}
	}

	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: 在此放置处理何时用
		//  “确定”来关闭对话框的代码
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: 在此放置处理何时用
		//  “取消”来关闭对话框的代码
	}

	// 删除上面创建的 shell 管理器。
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	// 由于对话框已关闭，所以将返回 FALSE 以便退出应用程序，
	//  而不是启动应用程序的消息泵。
	return FALSE;
}

