// AddHostDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "RemoteMan.h"
#include "AddHostDlg.h"
#include "afxdialogex.h"


// CAddHostDlg 对话框

IMPLEMENT_DYNAMIC(CAddHostDlg, CDialogEx)

CAddHostDlg::CAddHostDlg(HOST_STRUCT const *pHost,  HTREEITEM hItem, CWnd* pParent /*=NULL*/)
	: CDialogEx(CAddHostDlg::IDD, pParent)
	, m_hParentItem(hItem)
	, IsPasswordChange(false)
	, m_Password(_T(""))
{
	if (pHost)
	{
		bAddHost=false;
		m_Password=pHost->Password;
		memcpy(&m_Host, pHost, sizeof(m_Host));
	}
	else
	{
		bAddHost=true;
		memset(&m_Host, 0, sizeof(m_Host));
	}
}

CAddHostDlg::~CAddHostDlg()
{
}

void CAddHostDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_PASSWORD, m_Password);
}


BEGIN_MESSAGE_MAP(CAddHostDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CAddHostDlg::OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_COMBO_CTRLMODE, &CAddHostDlg::OnCbnSelchangeComboCtrlmode)
	ON_EN_CHANGE(IDC_EDIT_PASSWORD, &CAddHostDlg::OnEnChangeEditPassword)
END_MESSAGE_MAP()


// CAddHostDlg 消息处理程序


BOOL CAddHostDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	CComboBox *pBox = (CComboBox*)GetDlgItem(IDC_COMBO_CTRLMODE);
	for(int i=0; i<sizeof(CTRL_MODE)/sizeof(CTRL_MODE[0]);i++)
	{
		pBox->AddString(CTRL_MODE[i]);
	}
	if (bAddHost)
	{
		pBox->SetCurSel(0);
		SetCtrlModeDefPort(0);
		((CComboBox*)GetDlgItem(IDC_COMBO_USER))->SetCurSel(2);
	}
	else
	{
		pBox->SetCurSel(m_Host.CtrlMode);
		SetDlgItemInt(IDC_EDIT_HOSTPORT,m_Host.HostPort);
		SetDlgItemText(IDC_EDIT_HOSTADDR,m_Host.HostAddress);
		SetDlgItemText(IDC_EDIT_HOSTNAME,m_Host.Name);
		SetDlgItemText(IDC_COMBO_USER,m_Host.Account);
		SetDlgItemText(IDC_EDIT_README,m_Host.ReadMe);
		SetWindowText("编辑主机");
		SetDlgItemText(IDOK,"确定");
		SetDlgItemText(IDCANCEL,"取消");
	}

#ifdef SHOW_HOST_PASSWORD
	CEdit *pEdit = (CEdit*)GetDlgItem(IDC_EDIT_PASSWORD);
	pEdit->SetPasswordChar(0);
#endif

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CAddHostDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CString str;
	//服务器名称
	GetDlgItemText(IDC_EDIT_HOSTNAME,str);
	str.Trim();
	if (str.GetLength()>=sizeof(m_Host.Name))
	{
		str.Format("主机名称设置错误,超过%d字节。",sizeof(m_Host.Name)-1);
		MessageBox(str,"错误",MB_ICONERROR);
		return;
	}
	strcpy_s(m_Host.Name,sizeof(m_Host.Name), str);
	//服务器地址
	GetDlgItemText(IDC_EDIT_HOSTADDR,str);
	str.Trim();
	if (str.GetLength()>=sizeof(m_Host.HostAddress) || str.GetLength()<2)
	{
		str.Format("主机地址设置错误,超过%d字节。",sizeof(m_Host.HostAddress)-1);
		MessageBox(str,"错误",MB_ICONERROR);
		return;
	}
	strcpy_s(m_Host.HostAddress, sizeof(m_Host.HostAddress), str);
	if (m_Host.Name[0]==0) strcpy_s(m_Host.Name,sizeof(m_Host.Name),str);
	//端口
	m_Host.HostPort=GetDlgItemInt(IDC_EDIT_HOSTPORT);
	if (m_Host.HostPort==0 || m_Host.HostPort>=0x10000)
	{
		MessageBox("主机端口设置错误","错误",MB_ICONERROR);
		return;
	}
	//控制模式
	m_Host.CtrlMode=((CComboBox*)GetDlgItem(IDC_COMBO_CTRLMODE))->GetCurSel();
	//用户名
	GetDlgItemText(IDC_COMBO_USER,str);
	str.Trim();
	if (str.GetLength()>=sizeof(m_Host.Account))
	{
		str.Format("用户名设置错误,超过%d字节。",sizeof(m_Host.Account)-1);
		MessageBox(str,"错误",MB_ICONERROR);
		return;
	}
	strcpy_s(m_Host.Account,sizeof(m_Host.Account),str);
	//密码
	GetDlgItemText(IDC_EDIT_PASSWORD,str);
	if (str.GetLength()>PASSWORD_MAXLEN)
	{
		str.Format("用户密码错误,超过%d字节。",PASSWORD_MAXLEN);
		MessageBox(str,"错误",MB_ICONERROR);
		return;
	}
	strcpy_s(m_Host.Password,sizeof(m_Host.Password),str);
	//说明
	GetDlgItemText(IDC_EDIT_README,str);
	if (str.GetLength()>=sizeof(m_Host.ReadMe))
	{
		str.Format("主机说明设置错误,超过%d字节。",sizeof(m_Host.ReadMe)-1);
		MessageBox(str,"错误",MB_ICONERROR);
		return;
	}
	strcpy_s(m_Host.ReadMe,sizeof(m_Host.ReadMe),str);

	//发送配置
	if (bAddHost)
		GetParent()->SendMessage(WM_ADDHOST_MESSAGE, WPARAM(&m_Host), (LPARAM)m_hParentItem);
	else
		CDialogEx::OnOK();
}


void CAddHostDlg::OnCbnSelchangeComboCtrlmode()
{
	// TODO: 在此添加控件通知处理程序代码
	SetCtrlModeDefPort(((CComboBox*)GetDlgItem(IDC_COMBO_CTRLMODE))->GetCurSel());
}


void CAddHostDlg::SetCtrlModeDefPort(int CtrlMode)
{
	if (strcmp(CTRL_MODE[CtrlMode],CTRL_MODE_RDP_NAME)==0)
		SetDlgItemInt(IDC_EDIT_HOSTPORT,3389);
	else if (strcmp(CTRL_MODE[CtrlMode],CTRL_MODE_RADMIN_NAME)==0)
		SetDlgItemInt(IDC_EDIT_HOSTPORT,4899);
	else if (strcmp(CTRL_MODE[CtrlMode],CTRL_MODE_SSH_NAME)==0)
		SetDlgItemInt(IDC_EDIT_HOSTPORT,22);
	else if (strcmp(CTRL_MODE[CtrlMode],CTRL_MODE_VNC_NAME)==0)
		SetDlgItemInt(IDC_EDIT_HOSTPORT,5900);
}


void CAddHostDlg::OnEnChangeEditPassword()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	IsPasswordChange=true;
}
