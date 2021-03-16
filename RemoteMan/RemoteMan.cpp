
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

static HANDLE hSem;

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

	//只运行一个实例
	hSem = CreateSemaphore(NULL, 1, 1, m_pszExeName);
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		// 关闭信号量句柄
//		CloseHandle(hSem);
		// 寻找先前实例的主窗口
		HWND hWndPrevious = ::GetWindow(::GetDesktopWindow(),GW_CHILD);
		while (::IsWindow(hWndPrevious))
		{
			// 检查窗口是否有预设的标记?
			// 有，则是我们寻找的主窗
			if (::GetProp(hWndPrevious, m_pszExeName))
			{
				// 如果窗口已经缩小在任务栏中，那么首先先打开
				if(!::IsWindowVisible(hWndPrevious))
					::PostMessage(hWndPrevious, WM_SYSCOMMAND, SC_MINIMIZE, 0);
				// 主窗口已最小化，则恢复其大小
				if (::IsIconic(hWndPrevious))
					::ShowWindow(hWndPrevious,SW_RESTORE);
				// 将主窗激活
				::SetForegroundWindow(hWndPrevious);
				// 将主窗的对话框激活
				::SetForegroundWindow(
					::GetLastActivePopup(hWndPrevious));
				// 退出本实例
				return FALSE;
			}
			// 继续寻找下一个窗口
			hWndPrevious = ::GetWindow(hWndPrevious,GW_HWNDNEXT);
		}
		// 前一实例已存在，但找不到其主窗
		AfxMessageBox("已有一个实例在运行，但找不到它的主窗口！");
		// 可能出错了
		// 退出本实例
		return FALSE;
	}

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



int CRemoteManApp::ExitInstance()
{
	// TODO: 在此添加专用代码和/或调用基类
	CloseHandle(hSem);
	return CWinApp::ExitInstance();
}
