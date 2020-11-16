
// RemoteManDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "RemoteMan.h"
#include "RemoteManDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteManDlg 对话框




CRemoteManDlg::CRemoteManDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRemoteManDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteManDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_PIC, m_Ico);
	DDX_Control(pDX, IDC_LIST1, m_List);
}

BEGIN_MESSAGE_MAP(CRemoteManDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CRemoteManDlg::OnBnClickedOk)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// CRemoteManDlg 消息处理程序
void CRemoteManDlg::InitToolBar(void)
{
	m_ToolbarImageList.Create(32,32,ILC_COLOR24|ILC_MASK,1,1);
	m_ToolbarImageList.SetBkColor(RGB(255,255,255));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_ADDNODE));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_ADDNODE));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_DELNODE));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_PC));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_EDIT));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_DEL));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_OPEN));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDR_MAINFRAME));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_RADMIN));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_SSH));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_SET));

	UINT array[14]={IDC_TOOLER_ADDROOTGROUP,IDC_TOOLER_ADDGROUP,IDC_TOOLER_DELGROUP,ID_SEPARATOR,
					IDC_TOOLER_NEW,IDC_TOOLER_EDIT,IDC_TOOLER_DEL,ID_SEPARATOR,
					IDC_TOOLER_OPEN,IDC_TOOLER_MSTSC,IDC_TOOLER_RADMIN,IDC_TOOLER_SSH,ID_SEPARATOR,
					IDC_TOOLER_SET};
	m_ToolBar.Create(this);
	m_ToolBar.SetButtons(array,14);
	m_ToolBar.SetButtonText(0,"添加根分组");
	m_ToolBar.SetButtonText(1,"添加分组");
	m_ToolBar.SetButtonText(2,"删除分组");

	m_ToolBar.SetButtonText(4,"添加服务器");
	m_ToolBar.SetButtonText(5,"编辑服务器");
	m_ToolBar.SetButtonText(6,"删除服务器");

	m_ToolBar.SetButtonText(8,"连接");
	m_ToolBar.SetButtonText(9,"远程桌面");
	m_ToolBar.SetButtonText(10,"Radmin");
	m_ToolBar.SetButtonText(11,"SSH");

	m_ToolBar.SetButtonText(13,"设置");

	m_ToolBar.GetToolBarCtrl().SetImageList(&m_ToolbarImageList);
	m_ToolBar.SetSizes(CSize(72,56),CSize(20,34));
	m_ToolBar.EnableToolTips(TRUE);

	RepositionBars(AFX_IDW_CONTROLBAR_FIRST,AFX_IDW_CONTROLBAR_LAST,0);
}

BOOL CRemoteManDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	HICON hico=(HICON)LoadImage(AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDI_MSTSC),IMAGE_ICON,64,64,LR_DEFAULTCOLOR);
	m_Ico.SetIcon(hico);

	InitToolBar();
	SetDlgItemText(IDC_EDIT1,"请添加服务器");

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
	m_List.InsertColumn(0,"类型",LVCFMT_LEFT,72);
	m_List.InsertColumn(1,"服务器名称",LVCFMT_LEFT,160);
	m_List.InsertColumn(2,"域名",LVCFMT_LEFT,150);
	m_List.InsertColumn(3,"端口",LVCFMT_LEFT,64);
	m_List.InsertColumn(4,"账户",LVCFMT_LEFT,120);



	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteManDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteManDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteManDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CRemoteManDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
//	CDialogEx::OnOK();
} 



HBRUSH CRemoteManDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  在此更改 DC 的任何特性 
	return hbr;
}
