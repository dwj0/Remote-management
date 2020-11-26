// SysSetDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "RemoteMan.h"
#include "SysSetDlg.h"
#include "afxdialogex.h"

#define IDC_CHECKBOX   (0x2000) // 控件的ID号

// CSysSetDlg 对话框

IMPLEMENT_DYNAMIC(CSysSetDlg, CDialogEx)

CSysSetDlg::CSysSetDlg(bool ParentShowHost,char const *MstDriveStr,int MstColor,BOOL MstShowDeskImg,BOOL MstFontSmooth,	BOOL MstThemes,
	int RadminColor,char const *RadminPath,char const *SshPath, char const *VNCPath, char const *Format, int TimeOut, CWnd* pParent/*=NULL*/)
	: CDialogEx(CSysSetDlg::IDD, pParent)
	, m_ParentShowHost(ParentShowHost)
	, m_MstDriveStr(MstDriveStr)
	, m_MstColor(MstColor)
	, m_MstShowDeskImg(MstShowDeskImg)
	, m_MstFontSmooth(MstFontSmooth)
	, m_MstThemes(MstThemes)
	, m_RadminColor(RadminColor)
	, m_RadminPath(RadminPath)
	, m_SshPath(SshPath)
	, m_VNCPath(VNCPath)
	, m_SSHFormat(Format)
	, m_TimeOut(TimeOut)
	, m_SrcPassword("******")
{

}

CSysSetDlg::~CSysSetDlg()
{
}

void CSysSetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_PARENT_SHOW_HOST, m_ParentShowHost);
	DDX_CBIndex(pDX, IDC_COMBO_MST_COLOR, m_MstColor);
	DDX_Check(pDX, IDC_CHECK_MST_SHOW_DISKIMG, m_MstShowDeskImg);
	DDX_Check(pDX, IDC_CHECK_MST_FONTSMOOTH, m_MstFontSmooth);
	DDX_Check(pDX, IDC_CHECK_MST_THEMES, m_MstThemes);
	DDX_CBIndex(pDX, IDC_COMBO_RADMIN_COLOR, m_RadminColor);
	DDX_Text(pDX, IDC_EDIT_RADMIN_PATH, m_RadminPath);
	DDX_Text(pDX, IDC_EDIT_SSH_PATH, m_SshPath);
	DDX_Text(pDX, IDC_EDIT_SRCPSAAWORD, m_SrcPassword);
	DDX_Text(pDX, IDC_EDIT_TIMEOUT, m_TimeOut);
	DDX_Text(pDX, IDC_EDIT_VNC_PATH, m_VNCPath);
	DDX_Text(pDX, IDC_EDIT_SSH_FROMAT, m_SSHFormat);
}


BEGIN_MESSAGE_MAP(CSysSetDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_CHANGE_PASSWORD, &CSysSetDlg::OnBnClickedBtnChangePassword)
	ON_BN_CLICKED(IDOK, &CSysSetDlg::OnBnClickedOk)
	ON_COMMAND_RANGE(IDC_BTN_SSH_PATH,IDC_BTN_VNC_PATH,&CSysSetDlg::OnBnClickedSelPath)
	ON_BN_CLICKED(IDC_BTN_SSH_FORMAT_HELP, &CSysSetDlg::OnBnClickedBtnSshFormatHelp)
END_MESSAGE_MAP()


BOOL CSysSetDlg::OnInitDialog()
{
	int n=0;
	char Buf[128];
	RECT Rect;
	CONST UINT MAX_CHECKBOX_WIDTH = 30;  // 定义CheckBox的宽度

	CDialogEx::OnInitDialog();
	// TODO:  在此添加额外的初始化
	GetDlgItem(IDC_COMBO_MST_COLOR)->GetWindowRect(&Rect);
	ScreenToClient(&Rect);
	Rect.top+=28;
	Rect.bottom+=28;

	//形式是这样: "C:\<NULL>D:\<NULL>E:\<NULL><NULL>" 
	GetLogicalDriveStrings(sizeof(Buf),Buf);
	for (char *s=Buf; *s!=0 && n<sizeof(DriveCheck)/sizeof(DriveCheck[0]); s+=4)
	{
		s[2]=0;
		Rect.right=Rect.left+MAX_CHECKBOX_WIDTH;
		DriveCheck[n].Create(s, WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, Rect, this, IDC_CHECKBOX + n);
		if (m_MstDriveStr.Find(s[0])>=0)
			DriveCheck[n].SetCheck(TRUE);
		n++;
		Rect.left+=MAX_CHECKBOX_WIDTH+10;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

// CSysSetDlg 消息处理程序


void CSysSetDlg::OnBnClickedBtnChangePassword()
{
	// TODO: 在此添加控件通知处理程序代码
	char Src[20], New[20], Ack[20];
	int n0=GetDlgItemText(IDC_EDIT_SRCPSAAWORD,Src,sizeof(Src)-1);
	int n1=GetDlgItemText(IDC_EDIT_NEWPASSWORD,New,sizeof(New)-1);
	GetDlgItemText(IDC_EDIT_ACKPASSWORD,Ack,sizeof(Ack)-1);
	if (n1>PASSWORD_MAXLEN || strcmp(Ack,New)!=0)
	{
		MessageBox("新密码信息不正确或密码超过16字节!");
		return;
	}
	char const *Res = (char *)GetParent()->SendMessage(WM_MODIFY_PASSWORD_MESSAGE, WPARAM(Src),LPARAM(New));
	MessageBox(Res,"密码修改");
}


void CSysSetDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	char str[2];
	m_MstDriveStr="";
	for (int i=0; i<sizeof(DriveCheck)/sizeof(DriveCheck[0]); i++)
	{
		if (DriveCheck[i].m_hWnd==0) break;
		if (!DriveCheck[i].GetCheck()) continue;
		DriveCheck[i].GetWindowText(str,2);
		str[1]=0;
		m_MstDriveStr+=str;
	}

	CDialogEx::OnOK();
}

void CSysSetDlg::OnBnClickedSelPath(UINT Id)
{
	CFileDialog fdlg(TRUE,NULL,NULL,6UL,"*.exe|*.exe||");
	if (fdlg.DoModal()==IDOK)
	{
		SetDlgItemText(IDC_EDIT_SSH_PATH+Id-IDC_BTN_SSH_PATH, fdlg.GetPathName());
	}
}

void CSysSetDlg::OnBnClickedBtnSshFormatHelp()
{
	// TODO: 在此添加控件通知处理程序代码
	char const *str = "命令行中，%1:代表主机地址，%2:代表端口，%3:代表帐户名，%4:代表密码\r\n\r\n"
		"SecureCRT的命令行格式为： /ssh2 %3@%1 /P %2 /PASSWORD %4\r\n"
		"Putty的命令行格式为： -ssh -l %3 -pw %4 -P %2 %1\r\n"
		"WinSCP的命令行格式为： %3:%4@%1:%2\r\n\r\n"
		"其它软件可参考以上自己填写，如果为空，则根据以上3个的文件名自动匹配.";
	MessageBox(str,"SSH 命令行参数帮助");
}
