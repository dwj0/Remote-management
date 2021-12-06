
// RemoteManDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "RemoteMan.h"
#include "RemoteManDlg.h"
#include "afxdialogex.h"
#include "Aes.h"
#include "CodeConverter.h"
#include "FileDialogEx.h"
#include <Wincrypt.h>

#pragma comment(lib, "Crypt32.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

char const InitTabSqlStr[] = "\
BEGIN TRANSACTION;\r\n\
CREATE TABLE %sConfigTab(\r\n\
id INTEGER primary key not null,\r\n\
DatabaseVer int,\r\n\
GroupLastSelId int,\r\n\
ParentShowHost boolean,\r\n\
Password char(66),\r\n\
RadminPath char(256),\r\n\
SSHPath char(256),\r\n\
WinScpPath char(256),\r\n\
VNCPath char(256),\r\n\
SSHParamFormat char(64),\r\n\
CheckOnlineTimeOut int,\r\n\
MstscConsole boolean,\r\n\
MstscUseDrive boolean,\r\n\
MstscLocalDrive char(24),\r\n\
MstscRemoteAudio boolean,\r\n\
MstscColor int,\r\n\
MstscWinpos int,\r\n\
MstscDeskImg boolean,\r\n\
MstscFontSmooth boolean,\r\n\
MstscThemes boolean,\r\n\
RadminCtrlMode int,\r\n\
RadminFullScreen boolean,\r\n\
RadminColor int\r\n\
);\r\n\
\r\n\
CREATE TABLE %sGroupTab(\r\n\
id INTEGER primary key AUTOINCREMENT,\r\n\
Name char(64) not null,\r\n\
ParentId int not null\r\n\
);\r\n\
\r\n\
CREATE TABLE %sHostTab(\r\n\
id INTEGER primary key AUTOINCREMENT,\r\n\
Name char(64) not null,\r\n\
ParentId int  not null,\r\n\
CtrlMode int  not null,\r\n\
HostAddress char(64) not null,\r\n\
HostPort int not null,\r\n\
Account char(32) not null,\r\n\
Password char(66) not null,\r\n\
HostReadme char(256)\r\n\
);\r\n\
insert into %sConfigTab values(0, 2, 0, true, '', '', '', '', '','', 500, false, true,'',false,0,0,true, true, true, 0, true, 1);\r\n\
COMMIT;\r\n\
";


//密码66字节说明：密码最大长度为32字节(16-31字节都是要扩充到32字节的)，使用ASCII存储=64字节
// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

//默认列表框宽度
static int const ListDefColumnWidth[]={80,160,140,55,100,37};

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

char *CryptRDPPassword(char const *Password, char *OutPassword, int MaxLen)
{
	DATA_BLOB DataIn ;
	DATA_BLOB DataOut ;
	// mstsc.exe中使用的是unicode,所以必须做宽字符转换
	DWORD n=0;
	BYTE wPassword[64];
	while (*Password)
	{
		wPassword[n++]=*Password++;
		wPassword[n++]=0;
	}
	wPassword[n]=0;

	DataIn . pbData = wPassword ;
	DataIn . cbData = n ;
	if ( CryptProtectData ( &DataIn , L"psw" , // A description string
		//to be included with the
		// encrypted data.
		NULL , // Optional entropy not used.
		NULL , // Reserved.
		NULL , // Pass NULL for the
		// prompt structure.
		CRYPTPROTECT_UI_FORBIDDEN,
		&DataOut ) )
	{
		int len=0;
		for (n=0; n<DataOut.cbData; n++)
		{
			len+=sprintf_s(OutPassword+len,MaxLen-len,"%02X",DataOut.pbData[n]);
		}
		return OutPassword;
	}
	return "";
}

// CRemoteManDlg 对话框
static int ReadGroupCallBack_Ascii(void* para, int n_column, char** column_value, char** column_name)
{
	if (column_value[1]!=NULL)
	{
		CArray<GROUP_STRUCT ,GROUP_STRUCT&> *pGroupArray=(CArray<GROUP_STRUCT ,GROUP_STRUCT&>*)para;
		GROUP_STRUCT g;
		g.Id=atoi(column_value[0]);
		strcpy_s(g.Name,sizeof(g.Name),column_value[1]);
		pGroupArray->Add(g);
	}
	return 0;
}

static int ReadGroupCallBack(void* para, int n_column, char** column_value, char** column_name)
{
	if (column_value[1]!=NULL)
	{
		CArray<GROUP_STRUCT ,GROUP_STRUCT&> *pGroupArray=(CArray<GROUP_STRUCT ,GROUP_STRUCT&>*)para;
		GROUP_STRUCT g;
		g.Id=atoi(column_value[0]);
		strcpy_s(g.Name,sizeof(g.Name),CodeConverter::Utf8ToAscii(column_value[1]).c_str());
		pGroupArray->Add(g);
	}
	return 0;
}

static int ReadHostCallBack_Ascii(void* para, int n_column, char** column_value, char** column_name)
{
	CArray<HOST_STRUCT,HOST_STRUCT&>*pHostArray=(CArray<HOST_STRUCT,HOST_STRUCT&>*)para;

	HOST_STRUCT Host;
	memset(&Host,0,sizeof(Host));
	Host.Id=atoi(column_value[0]);										//Id
	strcpy_s(Host.Name,sizeof(Host.Name),column_value[1]);				//主机名称
	Host.CtrlMode=atoi(column_value[3]);								//控制类型
	strcpy_s(Host.HostAddress,sizeof(Host.HostAddress),column_value[4]);//主机地址
	Host.HostPort=atoi(column_value[5]);								//端口
	strcpy_s(Host.Account,sizeof(Host.Account),column_value[6]);		//帐号
	if (column_value[7]!=NULL)											//密码
		strcpy_s(Host.Password,sizeof(Host.Password),column_value[7]);
	if (column_value[8]!=NULL)											//说明
		strcpy_s(Host.ReadMe,sizeof(Host.ReadMe),column_value[8]);
	pHostArray->Add(Host);
	return 0;
}

static int ReadHostCallBack(void* para, int n_column, char** column_value, char** column_name)
{
	CArray<HOST_STRUCT,HOST_STRUCT&>*pHostArray=(CArray<HOST_STRUCT,HOST_STRUCT&>*)para;

	HOST_STRUCT Host;
	memset(&Host,0,sizeof(Host));
	//Id
	Host.Id=atoi(column_value[0]);																	
	//主机名称
	strcpy_s(Host.Name,sizeof(Host.Name),CodeConverter::Utf8ToAscii(column_value[1]).c_str());	
	//父ID
	Host.ParentId=atoi(column_value[2]);	
	//控制类型
	Host.CtrlMode=atoi(column_value[3]);	
	//主机地址
	strcpy_s(Host.HostAddress,sizeof(Host.HostAddress),CodeConverter::Utf8ToAscii(column_value[4]).c_str());	
	//端口
	Host.HostPort=atoi(column_value[5]);		
	//帐号
	strcpy_s(Host.Account,sizeof(Host.Account),CodeConverter::Utf8ToAscii(column_value[6]).c_str());
	//密码
	if (column_value[7]!=NULL)																		
		strcpy_s(Host.Password,sizeof(Host.Password),column_value[7]);
	//说明
	if (column_value[8]!=NULL)																		
		strcpy_s(Host.ReadMe,sizeof(Host.ReadMe),CodeConverter::Utf8ToAscii(column_value[8]).c_str());
	pHostArray->Add(Host);
	return 0;
}

void CRemoteManDlg::EnumTreeData(HTREEITEM hItem, int ParentNode)
{
	char sqlstr[64];
	CArray<GROUP_STRUCT ,GROUP_STRUCT&>GroupArray;
	GroupArray.SetSize(0,20);
	sprintf_s(sqlstr,sizeof(sqlstr),"select * from GroupTab where ParentId=%d;",ParentNode);
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, ReadGroupCallBack, &GroupArray, NULL);
	for (int i=0; i<GroupArray.GetSize();i++)
	{
		GROUP_STRUCT g=GroupArray[i];
		HTREEITEM hNewItem=m_Tree.InsertItem(g.Name,0,1,hItem);
		m_Tree.SetItemData(hNewItem,g.Id);
		EnumTreeData(hNewItem,g.Id);
		if (g.Id==SysConfig.GroupLastSelId) m_Tree.SelectItem(hNewItem);
	}
}

static int ReadConfigCallback(void* para, int n_column, char** column_value, char** column_name)
{
	CONFIG_STRUCT *pConfig = (CONFIG_STRUCT*)para;
	pConfig->DatabaseVer=-1;

	for (int i=0; i<n_column; i++)
	{
		if (column_value[i]==NULL) column_value[i]="";
		if (strcmp(column_name[i],"DatabaseVer")==0)
			pConfig->DatabaseVer =atoi(column_value[i]);
		else if (strcmp(column_name[i],"GroupLastSelId")==0)
			pConfig->GroupLastSelId =atoi(column_value[i]);
		else if (strcmp(column_name[i],"Password")==0)
			strcpy_s(pConfig->SysPassword,sizeof(pConfig->SysPassword), column_value[i]);
		else if (strcmp(column_name[i],"ParentShowHost")==0)
			pConfig->ParentShowHost=column_value[i][0]!='0' && column_value[i][0]!=0;
		else if (strcmp(column_name[i],"RadminPath")==0)
			strcpy_s(pConfig->RadminPath,sizeof(pConfig->RadminPath),column_value[i]);
		else if (strcmp(column_name[i],"SSHPath")==0)
			strcpy_s(pConfig->SSHPath,sizeof(pConfig->SSHPath),column_value[i]);
		else if (strcmp(column_name[i],"WinScpPath")==0)
			strcpy_s(pConfig->WinScpPath,sizeof(pConfig->WinScpPath),column_value[i]);
		else if (strcmp(column_name[i],"VNCPath")==0)
			strcpy_s(pConfig->VNCPath,sizeof(pConfig->VNCPath),column_value[i]);
		else if (strcmp(column_name[i],"SSHParamFormat")==0)
			strcpy_s(pConfig->SSHParamFormat,sizeof(pConfig->SSHParamFormat),column_value[i]);
		else if (strcmp(column_name[i],"CheckOnlineTimeOut")==0)
			pConfig->CheckOnlineTimeOut =atoi(column_value[i]);
		else if (strcmp(column_name[i],"MstscConsole")==0)
			pConfig->MstscConsole=column_value[i][0]!='0' && column_value[i][0]!=0;
		else if (strcmp(column_name[i],"MstscUseDrive")==0)
			pConfig->MstscUseDrive=column_value[i][0]!='0' && column_value[i][0]!=0;
		else if (strcmp(column_name[i],"MstscLocalDrive")==0)
			strcpy_s(pConfig->MstscLocalDrive,sizeof(pConfig->MstscLocalDrive),column_value[i]);
		else if (strcmp(column_name[i],"MstscRemoteAudio")==0)
			pConfig->MstscRemoteAudio =column_value[i][0]!='0' && column_value[i][0]!=0;
		else if (strcmp(column_name[i],"MstscColor")==0)
			pConfig->MstscColor =atoi(column_value[i]);
		else if (strcmp(column_name[i],"MstscWinpos")==0)
			pConfig->MstscWinpos =atoi(column_value[i]);
		else if (strcmp(column_name[i],"MstscDeskImg")==0)
			pConfig->MstscDeskImg =column_value[i][0]!='0' && column_value[i][0]!=0;
		else if (strcmp(column_name[i],"MstscFontSmooth")==0)
			pConfig->MstscFontSmooth =column_value[i][0]!='0' && column_value[i][0]!=0;
		else if (strcmp(column_name[i],"MstscThemes")==0)
			pConfig->MstscThemes =column_value[i][0]!='0' && column_value[i][0]!=0;
		else if (strcmp(column_name[i],"RadminCtrlMode")==0)
			pConfig->RadminCtrlMode =atoi(column_value[i]);
		else if (strcmp(column_name[i],"RadminFullScreen")==0)
			pConfig->RadminFullScreen =column_value[i][0]!='0' && column_value[i][0]!=0;
		else if (strcmp(column_name[i],"RadminColor")==0)
			pConfig->RadminColor =atoi(column_value[i]);
	}
	//如果DatabaseVer!=-1时，表明是新的数据库版本，要对数据进行UTF8-ASCII转换
	if (pConfig->DatabaseVer!=-1)
	{
		strcpy_s(pConfig->RadminPath,sizeof(pConfig->RadminPath),CodeConverter::Utf8ToAscii(pConfig->RadminPath).c_str());
		strcpy_s(pConfig->SSHPath,sizeof(pConfig->SSHPath),CodeConverter::Utf8ToAscii(pConfig->SSHPath).c_str());
		strcpy_s(pConfig->VNCPath,sizeof(pConfig->VNCPath),CodeConverter::Utf8ToAscii(pConfig->VNCPath).c_str());
	}

	return 0;
}

static int ReadIntCallback(void* para, int n_column, char** column_value, char** column_name)
{
	*(int*)para = atoi(column_value[0]);
	return 0;
}

bool CRemoteManDlg::OpenUserDb(char const *DbPath)
{
	int TabCnt=0;
	int rc = sqlite3_open(DbPath,&m_pDB);
	if (rc) return false;
	//检查配置表是否存在
	char const *sqlstr = "select count(type) from sqlite_master where tbl_name='ConfigTab';";
	TRACE("%s\r\n",sqlstr);
	rc = sqlite3_exec(m_pDB, sqlstr, ReadIntCallback, &TabCnt, NULL);
	//表不存在时创建表格
	if (TabCnt==0)
	{
		char sqlstr[1536];
		rc=sprintf_s(sqlstr,sizeof(sqlstr),InitTabSqlStr,"","","","");		//插入4个主数据库的名称
		TRACE("%s\r\n",sqlstr);
		rc=sqlite3_exec(m_pDB,sqlstr,NULL,NULL,NULL);
	}
	return rc==0;
}

void CRemoteManDlg::DataBaseConversion(int Ver)
{
	int rc;
	//当DatabaseVer=-1时，表明这个字段不存在，要添加，并同时添加VNCPath列和CheckOnlineTimeOut列。 另外，之前数据库保存的是GB2312编码，要转换成UTF8格式
	if (SysConfig.DatabaseVer==-1)
	{
		SysConfig.DatabaseVer=0;
		if (AfxMessageBox("将对数据库进行Ascii到UTF8的编码转换\r\n转换后数据库不能用于旧版软件",MB_OKCANCEL)!=IDOK)
			exit(0);
		//添加列
		char sqlstr[1024]="alter table ConfigTab add column DatabaseVer int;"
			"alter table ConfigTab add column CheckOnlineTimeOut int;"
			"alter table ConfigTab add column VNCPath char(256);";
		TRACE("%s\r\n",sqlstr);
		rc=sqlite3_exec(m_pDB,sqlstr,NULL,NULL,NULL);
		//转换ConfigTab的编码
		rc=sprintf_s(sqlstr,sizeof(sqlstr),"update ConfigTab set RadminPath='%s',SSHPath='%s',VNCPath='%s' where id=0;",
			SysConfig.RadminPath, SysConfig.SSHPath, SysConfig.VNCPath);
		TRACE("%s\r\n",sqlstr);
		rc=sqlite3_exec(m_pDB,CodeConverter::AsciiToUtf8(sqlstr).c_str(),NULL,NULL,NULL);
		//转换GroupTab的编码 
		CArray<GROUP_STRUCT,GROUP_STRUCT&>GroupArray;
		GroupArray.SetSize(0,20);
		strcpy_s(sqlstr,sizeof(sqlstr),"select * from GroupTab;");
		TRACE("%s\r\n",sqlstr);
		rc=sqlite3_exec(m_pDB,sqlstr,ReadGroupCallBack_Ascii,&GroupArray,NULL);
		for (int i=0; i<GroupArray.GetSize(); i++)
		{
			GROUP_STRUCT g=GroupArray[i];
			rc=sprintf_s(sqlstr,sizeof(sqlstr),"update GroupTab set Name='%s' where id=%d;",g.Name,g.Id);
			TRACE("%s\r\n",sqlstr);
			rc=sqlite3_exec(m_pDB,CodeConverter::AsciiToUtf8(sqlstr).c_str(),NULL,NULL,NULL);
		}
		//转换HostTab的编码 
		CArray<HOST_STRUCT,HOST_STRUCT&>HostArray;
		HostArray.SetSize(0,20);
		strcpy_s(sqlstr,sizeof(sqlstr),"select * from HostTab;");
		TRACE("%s\r\n",sqlstr);
		rc=sqlite3_exec(m_pDB,sqlstr,ReadHostCallBack_Ascii,&HostArray,NULL);
		for (int i=0; i<HostArray.GetSize(); i++)
		{
			HOST_STRUCT h=HostArray[i];
			rc=sprintf_s(sqlstr,sizeof(sqlstr),"update HostTab set Name='%s',Account='%s',HostReadme='%s' where id=%d;",
				h.Name, h.Account, h.ReadMe,h.Id);
			TRACE("%s\r\n",sqlstr);
			rc=sqlite3_exec(m_pDB,CodeConverter::AsciiToUtf8(sqlstr).c_str(),NULL,NULL,NULL);
		}
	}
	//当DatabaseVer=0时，缺少SSHParamFormat列
	if (SysConfig.DatabaseVer==0)
	{
		SysConfig.DatabaseVer=1;
		//添加列
		char sqlstr[1024]="alter table ConfigTab add column SSHParamFormat char(64);"
			"update ConfigTab set DatabaseVer=1 where id=0;";
		TRACE("%s\r\n",sqlstr);
		rc=sqlite3_exec(m_pDB,sqlstr,NULL,NULL,NULL);
	}
	//当DatabaseVer=1时，缺少WinScpPath列
	if (SysConfig.DatabaseVer==1)
	{
		SysConfig.DatabaseVer=2;
		//添加列
		char sqlstr[1024]="alter table ConfigTab add column WinScpPath char(256);"
			"update ConfigTab set DatabaseVer=2 where id=0;";
		TRACE("%s\r\n",sqlstr);
		rc=sqlite3_exec(m_pDB,sqlstr,NULL,NULL,NULL);
	}
}

CRemoteManDlg::CRemoteManDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRemoteManDlg::IDD, pParent)
	, bScanExit(false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	char Path[MAX_PATH];
	GetModuleFileName(NULL,Path,MAX_PATH);
	char *p=strrchr(Path,'\\');
	if (p)
	{
		p[1]=0;
		strcat_s(Path,sizeof(Path),"User.db");
	}
	else
		strcpy_s(Path,sizeof(Path),"User.db");
	//先备份一下数据库,做一颗后悔药
	char BakPath[MAX_PATH];
	sprintf_s(BakPath,MAX_PATH,"%s.bak",Path);
	CFileStatus fstatus;
	if (CFile::GetStatus(Path,fstatus))
		CopyFile(Path,BakPath,FALSE);

	if (!OpenUserDb(CodeConverter::AsciiToUtf8(Path).c_str()))
	{
		char str[200];
		sprintf_s(str,sizeof(str),"打开数据库失败：%s",sqlite3_errmsg(m_pDB));
		AfxMessageBox(str);
		exit(0);
	}
	//读取参数
	memset(&SysConfig,0,sizeof(CONFIG_STRUCT));
	char const *sqlstr="select * from ConfigTab where id=0;";
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, ReadConfigCallback, &SysConfig, NULL);
	//数据库版本转换
	DataBaseConversion(SysConfig.DatabaseVer);
	if (SysConfig.CheckOnlineTimeOut<=0) SysConfig.CheckOnlineTimeOut=500;

	m_nListDragIndex=-1; 
	m_pDragImage=NULL;
}

CRemoteManDlg::~CRemoteManDlg()
{
	//更新最后打开的分组
	char sqlstr[128];
	int n=sprintf_s(sqlstr,sizeof(sqlstr),"update ConfigTab set GroupLastSelId=%d where id=0;",SysConfig.GroupLastSelId);
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);

	sqlite3_close(m_pDB); 
}

void CRemoteManDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_List);
	DDX_Control(pDX, IDC_TREE1, m_Tree);
	DDX_Control(pDX, IDC_EDIT_SEARCH, m_SearchEdit);
}

BEGIN_MESSAGE_MAP(CRemoteManDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDOK, &CRemoteManDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_TOOLER_SET, &CRemoteManDlg::OnToolbarClickedSysSet)
	ON_BN_CLICKED(ID_MENU_ADDGROUP, &CRemoteManDlg::OnMenuClickedAddGroup)
	ON_BN_CLICKED(ID_MENU_DELGROUP, &CRemoteManDlg::OnMenuClickedDelGroup)
	ON_BN_CLICKED(ID_MENU_ADDHOST, &CRemoteManDlg::OnMenuClickedAddHost)
	ON_BN_CLICKED(ID_MENU_EDITHOST, &CRemoteManDlg::OnMenuClickedEditHost)
	ON_BN_CLICKED(ID_MENU_DELHOST, &CRemoteManDlg::OnMenuClickedDelHost)
	ON_BN_CLICKED(ID_MENU_CONNENT, &CRemoteManDlg::OnMenuClickedConnentHost)
	ON_BN_CLICKED(ID_MENU_WINSCP_CONNENT, &CRemoteManDlg::OnMenuClickedWinScpConnent)
	ON_BN_CLICKED(ID_MENU_RENAMEGROUP, &CRemoteManDlg::OnMenuClickedRenameGroup)
	ON_BN_CLICKED(ID_MENU_EXPORTGROUP,&CRemoteManDlg::OnMenuClickedExportGroup)
	ON_BN_CLICKED(ID_MENU_IMPORTGROUP,&CRemoteManDlg::OnMenuClickedImportGroup)
	ON_BN_CLICKED(ID_MENU_PING,&CRemoteManDlg::OnMenuClickedPing)
	ON_COMMAND_RANGE(ID_MENU_OPENSSH,ID_MENU_OPEN_WINSCP,&CRemoteManDlg::OnMenuClickedOpenSSH)
	ON_BN_CLICKED(IDC_TOOLER_OPENRADMIN, &CRemoteManDlg::OnToolbarClickedOpenRadmin)
	ON_BN_CLICKED(IDC_TOOLER_OPENMSTSC, &CRemoteManDlg::OnToolbarClickedOpenMstsc)
	ON_BN_CLICKED(IDC_TOOLER_OPENSSH, &CRemoteManDlg::OnToolbarClickedOpenSSH)
	ON_BN_CLICKED(IDC_TOOLER_OPENVNC, &CRemoteManDlg::OnToolbarClickedOpenVNC)
	ON_BN_CLICKED(IDC_CHECK_MST_SHOW_WALLPAPER, &CRemoteManDlg::OnBnClickedCheckMstShowWallpaper)
	ON_BN_CLICKED(IDC_CHECK_MST_DRIVE, &CRemoteManDlg::OnBnClickedCheckMstDrive)
	ON_BN_CLICKED(IDC_CHECK_MST_AUDIO, &CRemoteManDlg::OnBnClickedCheckMstAudio)
	ON_BN_CLICKED(IDC_CHECK_RADMIN_FULLSCREEN, &CRemoteManDlg::OnBnClickedCheckRadminFullscreen)
	ON_CBN_SELCHANGE(IDC_COMBO_MST_WINPOS, &CRemoteManDlg::OnCbnSelchangeComboMstWinpos)
	ON_CBN_SELCHANGE(IDC_COMBO_RADMIN_CTRLMODE, &CRemoteManDlg::OnCbnSelchangeComboRadminCtrlmode)
	ON_MESSAGE(WM_ADDHOST_MESSAGE, &CRemoteManDlg::OnAddHostMessage)
	ON_MESSAGE(WM_MODIFY_PASSWORD_MESSAGE, &CRemoteManDlg::OnModifyPasswordMessage)
	ON_NOTIFY(NM_RCLICK, IDC_TREE1, &CRemoteManDlg::OnNMRClickTree1)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &CRemoteManDlg::OnNMRClickList1)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, &CRemoteManDlg::OnNMDblclkList1)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE1, &CRemoteManDlg::OnTvnSelchangedTree1)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, &CRemoteManDlg::OnLvnItemchangedList1)
	ON_NOTIFY(LVN_BEGINDRAG, IDC_LIST1, &CRemoteManDlg::OnLvnBegindragList1)
	ON_NOTIFY(TVN_ENDLABELEDIT, IDC_TREE1, &CRemoteManDlg::OnTvnEndlabeleditTree1)
	ON_COMMAND_RANGE(ID_MENU_FULLCTRL,ID_MENU_CLOSEHOST,&CRemoteManDlg::OnMenuClickedRadminCtrl)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_BN_CLICKED(IDC_BTN_CHECK_ONLINE, &CRemoteManDlg::OnBnClickedBtnCheckOnline)
	ON_BN_CLICKED(IDC_BTN_SEARCH, &CRemoteManDlg::OnBnClickedBtnSearch)
	ON_WM_SIZE()
	ON_WM_SETTINGCHANGE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

//TVN_ENDLABELEDIT 删除这行会不能设置断点，不信你试试


// CRemoteManDlg 消息处理程序
void CRemoteManDlg::InitToolBar(void)
{
	m_ToolbarImageList.Create(32,32,ILC_COLOR24|ILC_MASK,1,1);
	m_ToolbarImageList.SetBkColor(RGB(255,255,255));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_ADDNODE));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_DELNODE));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_PC));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_EDIT));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_DEL));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_OPEN));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDR_MAINFRAME));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_RADMIN));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_SSH));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_ICON_VNC));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_SET));

	UINT array[14]={ID_MENU_ADDGROUP,ID_MENU_DELGROUP,ID_SEPARATOR,
					ID_MENU_ADDHOST,ID_MENU_EDITHOST,ID_MENU_DELHOST,ID_SEPARATOR,
					ID_MENU_CONNENT,IDC_TOOLER_OPENMSTSC,IDC_TOOLER_OPENRADMIN,IDC_TOOLER_OPENSSH,IDC_TOOLER_OPENVNC,ID_SEPARATOR,
					IDC_TOOLER_SET};
	m_ToolBar.Create(this);
	m_ToolBar.SetButtons(array,14);
	m_ToolBar.SetButtonText(0,"添加分组");
	m_ToolBar.SetButtonText(1,"删除分组");
	m_ToolBar.SetButtonText(3,"添加主机");
	m_ToolBar.SetButtonText(4,"编辑主机");
	m_ToolBar.SetButtonText(5,"删除主机");
	m_ToolBar.SetButtonText(7,"连接主机");
	m_ToolBar.SetButtonText(8,"远程桌面");
	m_ToolBar.SetButtonText(9,"Radmin");
	m_ToolBar.SetButtonText(10,"SSH软件");
	m_ToolBar.SetButtonText(11,"VNC");
	m_ToolBar.SetButtonText(13,"设置");

	m_ToolBar.GetToolBarCtrl().SetImageList(&m_ToolbarImageList);
	m_ToolBar.SetSizes(CSize(70,56),CSize(32,32));

	m_ToolBar.MoveWindow(CRect(0,-1,820,62));	//移动工具栏在父窗口的位置
	m_ToolBar.ShowWindow(SW_SHOW);				//显示工具栏
//	RepositionBars(AFX_IDW_CONTROLBAR_FIRST,AFX_IDW_CONTROLBAR_LAST,0);
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
	((CStatic*)GetDlgItem(IDC_STATIC_PIC))->SetIcon(hico);

	//设置程序标志，用于单实例运行时还原窗口
	::SetProp(m_hWnd,AfxGetApp()->m_pszExeName,(HANDLE)1);

	InitToolBar();

	m_ImageList.Create(24,24,ILC_COLOR24|ILC_MASK,1,1);
	m_ImageList.SetBkColor(RGB(255,255,255));
	m_ImageList.Add(AfxGetApp()->LoadIcon(IDI_NODE_CLOSE));
	m_ImageList.Add(AfxGetApp()->LoadIcon(IDI_NODE_OPEN));
	m_ImageList.Add(AfxGetApp()->LoadIcon(IDR_MAINFRAME));
	m_ImageList.Add(AfxGetApp()->LoadIcon(IDI_RADMIN));
	m_ImageList.Add(AfxGetApp()->LoadIcon(IDI_SSH));
	m_ImageList.Add(AfxGetApp()->LoadIcon(IDI_ICON_VNC));
	m_Tree.SetImageList(&m_ImageList,LVSIL_NORMAL);

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
	m_List.SetImageList(&m_ImageList,LVSIL_SMALL);
	m_List.InsertColumn(0,"类型",LVCFMT_LEFT,ListDefColumnWidth[0]);
	m_List.InsertColumn(1,"服务器名称",LVCFMT_LEFT,ListDefColumnWidth[1]);
	m_List.InsertColumn(2,"主机",LVCFMT_LEFT,ListDefColumnWidth[2]);
	m_List.InsertColumn(3,"端口",LVCFMT_LEFT,ListDefColumnWidth[3]);
	m_List.InsertColumn(4,"账户",LVCFMT_LEFT,ListDefColumnWidth[4]);
	m_List.InsertColumn(5,"状态",LVCFMT_LEFT,ListDefColumnWidth[5]);

	((CButton*)GetDlgItem(IDC_CHECK_MST_SHOW_WALLPAPER))->SetCheck(SysConfig.MstscDeskImg);
	((CButton*)GetDlgItem(IDC_CHECK_MST_DRIVE))->SetCheck(SysConfig.MstscUseDrive);
	((CButton*)GetDlgItem(IDC_CHECK_MST_AUDIO))->SetCheck(SysConfig.MstscRemoteAudio);
	((CComboBox*)GetDlgItem(IDC_COMBO_MST_WINPOS))->SetCurSel(SysConfig.MstscWinpos);
	((CButton*)GetDlgItem(IDC_CHECK_RADMIN_FULLSCREEN))->SetCheck(SysConfig.RadminFullScreen);
	((CComboBox*)GetDlgItem(IDC_COMBO_RADMIN_CTRLMODE))->SetCurSel(SysConfig.RadminCtrlMode);

	//读取分组
	EnumTreeData(TVI_ROOT,0);
	if (m_Tree.GetCount()==0)
		SetDlgItemText(IDC_EDIT_README,"请先添加分组.");

	m_SearchEdit.SetDimText("输入主机名或域名进行搜索");
	m_SearchEdit.SetDimColor(RGB(150,150,250));

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


void CRemoteManDlg::OnToolbarClickedSysSet(void)
{
	CSysSetDlg Dlg(&SysConfig);
	if (Dlg.DoModal()==IDOK)
	{
		SysConfig.ParentShowHost=Dlg.m_ParentShowHost!=0;
		strcpy_s(SysConfig.RadminPath,sizeof(SysConfig.RadminPath),Dlg.m_RadminPath);
		strcpy_s(SysConfig.SSHPath,sizeof(SysConfig.SSHPath),Dlg.m_SshPath);
		strcpy_s(SysConfig.WinScpPath,sizeof(SysConfig.WinScpPath),Dlg.m_WinScpPath);
		strcpy_s(SysConfig.VNCPath,sizeof(SysConfig.VNCPath),Dlg.m_VNCPath);
		strcpy_s(SysConfig.MstscLocalDrive,sizeof(SysConfig.MstscLocalDrive),Dlg.m_MstDriveStr);
		strcpy_s(SysConfig.SSHParamFormat,sizeof(SysConfig.SSHParamFormat),Dlg.m_SSHFormat);
		SysConfig.MstscColor=Dlg.m_MstColor;
		SysConfig.MstscConsole=Dlg.m_MstConsole!=0;
		SysConfig.MstscFontSmooth=Dlg.m_MstFontSmooth!=0;
		SysConfig.MstscThemes=Dlg.m_MstThemes!=0;
		SysConfig.RadminColor=Dlg.m_RadminColor;
		SysConfig.CheckOnlineTimeOut=Dlg.m_TimeOut;
		
		char sqlstr[1024];
		int n=sprintf_s(sqlstr,sizeof(sqlstr),"update ConfigTab set ParentShowHost=%s,RadminPath='%s',SSHPath='%s',WinScpPath='%s',VNCPath='%s',"
											  "SSHParamFormat='%s',CheckOnlineTimeOut=%d,MstscLocalDrive='%s',MstscColor=%d,"
											  "MstscConsole=%s,MstscFontSmooth=%s,MstscThemes=%s,RadminColor=%d where id=0;",
			SysConfig.ParentShowHost ? "true":"false",
			SysConfig.RadminPath,
			SysConfig.SSHPath,
			SysConfig.WinScpPath,
			SysConfig.VNCPath,
			SysConfig.SSHParamFormat,
			SysConfig.CheckOnlineTimeOut,
			SysConfig.MstscLocalDrive,
			SysConfig.MstscColor,
			SysConfig.MstscConsole ? "true":"false",
			SysConfig.MstscFontSmooth ? "true":"false",
			SysConfig.MstscThemes ? "true":"false",
			SysConfig.RadminColor
			);
		TRACE("%s\r\n",sqlstr);
		int rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), NULL, NULL, NULL);
	}
}

void CRemoteManDlg::OnMenuClickedAddGroup(void)
{
	CString GroupName;
	HTREEITEM hItem = m_Tree.GetSelectedItem();
	if (hItem!=NULL)
		GroupName=m_Tree.GetItemText(hItem);

	CAddGroupDlg dlg(GroupName);
	if (dlg.DoModal()==IDOK)
	{
		int ParentId=0;
		if (dlg.m_AddRoot)
			hItem=TVI_ROOT;
		else
			ParentId=m_Tree.GetItemData(hItem);
		//先判断这个父分组下是否有这个名称的分组存在
		int GroupCnt=0;
		char sqlstr[128];
		sprintf_s(sqlstr,sizeof(sqlstr),"select count() from GroupTab where ParentId=%d and Name='%s';",ParentId,dlg.m_GroupName);
		TRACE("%s\r\n",sqlstr);		
		int rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), ReadIntCallback, &GroupCnt, NULL);
		if (GroupCnt>0)
		{
			MessageBox("该分组名已经存在","错误",MB_ICONERROR);
			return;
		}
		//添加到数据库
		sprintf_s(sqlstr,sizeof(sqlstr),"insert into GroupTab values(NULL,'%s',%d);",dlg.m_GroupName,ParentId);
		TRACE("%s\r\n",sqlstr);
		rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), NULL, NULL, NULL);
		//读取这条数据的ID
		int Id=0;
		sprintf_s(sqlstr,sizeof(sqlstr),"select id from GroupTab where ParentId=%d and Name='%s';",ParentId,dlg.m_GroupName);
		TRACE("%s\r\n",sqlstr);
		rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), ReadIntCallback, &Id, NULL);
		hItem=m_Tree.InsertItem(dlg.m_GroupName,0,1,hItem);
		m_Tree.SetItemData(hItem,Id);
		m_Tree.SelectItem(hItem);
	}
}

void CRemoteManDlg::EnumChildGroupId(HTREEITEM hItem,CArray<int ,int>&GroupArray)
{
	HTREEITEM hChildItem;
	GroupArray.Add(m_Tree.GetItemData(hItem));
	//查找子分组及兄弟分组
	hChildItem=m_Tree.GetChildItem(hItem);
	while (hChildItem!=0)
	{
		if (hChildItem==0) return;
		EnumChildGroupId(hChildItem,GroupArray);
		hChildItem=m_Tree.GetNextSiblingItem(hChildItem);
	}
}

void CRemoteManDlg::OnMenuClickedDelGroup(void)
{
	HTREEITEM hItem=m_Tree.GetSelectedItem();
	if (hItem==0) return;
	if (MessageBox("删除分组将同时删除子分组及对应主机，确认删除？","注意",MB_OKCANCEL|MB_ICONWARNING)!=IDOK) return;
	CArray<int ,int>GroupArray;
	GroupArray.SetSize(0,20);
	//枚举组ID
	EnumChildGroupId(hItem,GroupArray);
	//删除组
	int sqlstrlen=25+GroupArray.GetSize()*23;
	char *sqlstr=new char[sqlstrlen];
	int len=sprintf_s(sqlstr,sqlstrlen,"delete from GroupTab where id=%d",GroupArray[0]);
	for (int i=1; i<GroupArray.GetSize(); i++)
	{
		len+=sprintf_s(sqlstr+len,sqlstrlen-len," or id=%d",GroupArray[i]);
	}
	sqlstr[len]=';';
	sqlstr[len+1]=0;
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
	//删除主机
	len=sprintf_s(sqlstr,sqlstrlen,"delete from HostTab where ParentId=%d",GroupArray[0]);
	for (int i=1; i<GroupArray.GetSize(); i++)
	{
		len+=sprintf_s(sqlstr+len,sqlstrlen-len," or ParentId=%d",GroupArray[i]);
	}
	sqlstr[len]=';';
	sqlstr[len+1]=0;
	TRACE("%s\r\n",sqlstr);
	rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
	delete sqlstr;
	//删除树控件节点和列表框
	SetDlgItemText(IDC_EDIT_README,"");
	m_List.DeleteAllItems();
	m_Tree.DeleteItem(hItem);
}

afx_msg LRESULT CRemoteManDlg::OnAddHostMessage(WPARAM wParam, LPARAM lParam)
{
	HOST_STRUCT *pHost = (HOST_STRUCT*)wParam;
	int ParentId=m_Tree.GetItemData(HTREEITEM(lParam));

	char sqlstr[512],str[68];
	//查看该主机名是否存在
	int HostCnt=0;
	sprintf_s(sqlstr,sizeof(sqlstr),"select count() from HostTab where ParentId=%d and Name='%s' and CtrlMode=%d;",
		ParentId, pHost->Name, pHost->CtrlMode);
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), ReadIntCallback, &HostCnt, NULL);
	if (HostCnt>0)
	{
		MessageBox("该主机名已存在.","错误",MB_ICONERROR);
		return 1;
	}
	//添加到数据库
	sprintf_s(sqlstr,sizeof(sqlstr),"insert into HostTab values(NULL,'%s',%d,%d,'%s',%d,'%s','%s','%s');", 	
		pHost->Name, 
		ParentId, 
		pHost->CtrlMode, 
		pHost->HostAddress, 
		pHost->HostPort, 
		pHost->Account, 
		AesEnCodeToStr(pHost->Password,strlen(pHost->Password),str,AES_KEY), 
		pHost->ReadMe);
	TRACE("%s\r\n",sqlstr);
	rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), NULL, NULL, NULL);
	//读取ID
	int Id=0;
	sprintf_s(sqlstr,sizeof(sqlstr),"select id from HostTab where ParentId=%d and Name='%s';",ParentId,pHost->Name);
	TRACE("%s\r\n",sqlstr);
	rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), ReadIntCallback, &Id, NULL);
	//添加到列表
	ListAddHost(pHost,Id);

	return 0;
}

void CRemoteManDlg::OnMenuClickedAddHost(void)
{
	HTREEITEM hItem=m_Tree.GetSelectedItem();
	if (hItem==NULL || m_Tree.GetCount()==0)
	{
		MessageBox("请先添加分组或选择分组.","错误",MB_ICONERROR);
		return;
	}
	CAddHostDlg Dlg(NULL, hItem);
	Dlg.DoModal();
}

void CRemoteManDlg::ListAddHost(HOST_STRUCT const * pHost, int Id, int nItem)
{
	char str[12];
	if (nItem==-1)
		nItem=m_List.GetItemCount();
	m_List.InsertItem(nItem, CTRL_MODE[pHost->CtrlMode],2+pHost->CtrlMode);
	m_List.SetItemText(nItem, 1, pHost->Name);
	m_List.SetItemText(nItem, 2, pHost->HostAddress);
	sprintf_s(str,sizeof(str),"%d",pHost->HostPort);
	m_List.SetItemText(nItem, 3, str);
	m_List.SetItemText(nItem, 4, pHost->Account);
	m_List.SetItemData(nItem,Id);
}

void CRemoteManDlg::OnMenuClickedEditHost(void)
{
	if (m_List.GetSelectedCount()!=1) return;
	int n=m_List.GetSelectionMark();
	int Id=m_List.GetItemData(n);

	char sqlstr[512],str[68];
	CArray<HOST_STRUCT,HOST_STRUCT&>HostArray;
	sprintf_s(sqlstr,sizeof(sqlstr),"select * from HostTab where Id=%d;",Id);
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, ReadHostCallBack, &HostArray, NULL);
	if (HostArray.GetSize()!=1) return;
	HOST_STRUCT Host=HostArray[0];
	//设置中是否要显示密码
#ifdef SHOW_HOST_PASSWORD
	if (Host.Password[0]!=0)
	{
		byte data[36];
		int len=StringToBytes(Host.Password,data);
		if (len>0)
		{
			len=AesDeCode(data,len,AES_KEY);
			if (len>0) strcpy_s(Host.Password,sizeof(Host.Password),(char*)data);
		}
	}
#else
	strcpy_s(Host.Password,sizeof(Host.Password),"********");
#endif

	CAddHostDlg Dlg(&Host, 0);
	if (Dlg.DoModal()!=IDOK) return;
	//假定更新了服务器名称，先检查该名称是否存在
	int HostCnt=0;
	sprintf_s(sqlstr,sizeof(sqlstr),"select count() from HostTab where ParentId=%d and Name='%s' and CtrlMode=%d and Id!=%d;",
		Host.ParentId, Dlg.m_Host.Name, Dlg.m_Host.CtrlMode,Host.Id);
	TRACE("%s\r\n",sqlstr);
	 rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), ReadIntCallback, &HostCnt, NULL);
	if (HostCnt>0)
	{
		MessageBox("该主机名已存在.","错误",MB_ICONERROR);
		return;
	}
	//更新数据库
	if (Dlg.IsPasswordChange)
	{
		sprintf_s(sqlstr,sizeof(sqlstr),"update HostTab set Name='%s',CtrlMode=%d,HostAddress='%s',HostPort=%d,Account='%s',"
			"Password='%s',HostReadme='%s' where id=%d;",
			Dlg.m_Host.Name, 
			Dlg.m_Host.CtrlMode, 
			Dlg.m_Host.HostAddress, 
			Dlg.m_Host.HostPort, 
			Dlg.m_Host.Account, 
			AesEnCodeToStr(Dlg.m_Host.Password,strlen(Dlg.m_Host.Password),str,AES_KEY), 
			Dlg.m_Host.ReadMe,
			Id);
	}
	else
	{
		sprintf_s(sqlstr,sizeof(sqlstr),"update HostTab set Name='%s',CtrlMode=%d,HostAddress='%s',HostPort=%d,Account='%s',"
			"HostReadme='%s' where id=%d;",
			Dlg.m_Host.Name, 
			Dlg.m_Host.CtrlMode, 
			Dlg.m_Host.HostAddress, 
			Dlg.m_Host.HostPort, 
			Dlg.m_Host.Account, 
			Dlg.m_Host.ReadMe,
			Id);
	}
	TRACE("%s\r\n",sqlstr);
	rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), NULL, NULL, NULL);
	//更新列表框
	m_List.SetItem(n,0,LVIF_IMAGE,NULL,2+Dlg.m_Host.CtrlMode,0,0,0);
	m_List.SetItemText(n,0,CTRL_MODE[Dlg.m_Host.CtrlMode]);
	m_List.SetItemText(n,1,Dlg.m_Host.Name);
	m_List.SetItemText(n,2,Dlg.m_Host.HostAddress);
	sprintf_s(sqlstr,sizeof(sqlstr),"%d",Dlg.m_Host.HostPort);
	m_List.SetItemText(n,3,sqlstr);
	m_List.SetItemText(n,4,Dlg.m_Host.Account);
	SetDlgItemText(IDC_EDIT_README,Dlg.m_Host.ReadMe);
	
	m_List.SetFocus();
}


void CRemoteManDlg::OnMenuClickedDelHost(void)
{
	int Cnt=m_List.GetSelectedCount();
	if (Cnt==0) return;
	int *Sels=new int[Cnt];
	//列出选择的项,删除数据库
	int sqlstrlen=30+Cnt*17;
	char *sqlstr=new char[sqlstrlen];
	int len=sprintf_s(sqlstr,sqlstrlen,"delete from HostTab where ");
	POSITION  pos=m_List.GetFirstSelectedItemPosition();
	for (int i=0; pos!=NULL && i<Cnt; i++)
	{
		Sels[i]=m_List.GetNextSelectedItem(pos);
		len+=sprintf_s(sqlstr+len,sqlstrlen-len, i==0 ? "id=%d":" or id=%d",m_List.GetItemData(Sels[i]));
	}
	sqlstr[len]=';';
	sqlstr[len+1]=0;
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
	//从后开始删除列表
	for (int i=Cnt-1; i>=0; i--)
		m_List.DeleteItem(Sels[i]);
	//
	delete Sels;
	delete sqlstr;
}


void CRemoteManDlg::OnToolbarClickedOpenMstsc(void)
{
	char szBuffer[MAX_PATH];
	SHGetSpecialFolderPath(NULL, szBuffer, CSIDL_SYSTEM, FALSE);
	strcat_s(szBuffer,MAX_PATH,"\\Mstsc.exe");
	WinExec(szBuffer,SW_SHOW);
}

#define VNC_DEF_PATH	"TightVnc\\tvnviewer.exe"
#define RADMIN_DEF_PATH	"radmin.exe"
#define SSH_DEF_PATH	NULL
#define WINSCP_DEF_PATH	NULL
char const *GetExePath(char const *ConfigPath, char const *DefPath)
{
	char const *Path=ConfigPath[0]==0 ? DefPath:ConfigPath;			//当路径为空时使用同目录下的tvnviewer.exe
	//查看文件是否存在
	CFileStatus fstatus;
	if (Path==NULL || strstr(Path,".exe")==NULL || !CFile::GetStatus(Path,fstatus))
		return NULL;
	return Path;
}


void CRemoteManDlg::OnToolbarClickedOpenRadmin(void)
{
	char const *Path=GetExePath(SysConfig.RadminPath,RADMIN_DEF_PATH);
	if (Path==NULL)
	{
		AfxMessageBox("Radmin路径设置错误");
		return;
	}
	WinExec(Path,SW_SHOW);
}

void CRemoteManDlg::OnMenuClickedOpenSSH(UINT id)
{
	char const *path;
	if (id==ID_MENU_OPENSSH)
	{
		path = GetExePath(SysConfig.SSHPath,SSH_DEF_PATH);
		if (path==NULL)
		{
			MessageBox("SSH路径设置错误");
			return;
		}
	}
	else
	{
		path = GetExePath(SysConfig.WinScpPath,SSH_DEF_PATH);
		if (path==NULL)
		{
			MessageBox("WinScp路径设置错误");
			return;
		}
	}
	WinExec(path,SW_SHOW);
}

void CRemoteManDlg::OnToolbarClickedOpenSSH(void)
{
	CMenu Menu;
	Menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu *pSubMenu=Menu.GetSubMenu(7);
	CPoint point;
	GetCursorPos(&point);
	pSubMenu->TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_VERTICAL,point.x,point.y,this);
}

void CRemoteManDlg::OnToolbarClickedOpenVNC(void)
{
	char const *VNCPath=GetExePath(SysConfig.VNCPath,VNC_DEF_PATH);
	if (VNCPath==NULL)
	{
		AfxMessageBox("VNC路径设置错误");
		return;
	}
	WinExec(VNCPath,SW_SHOW);
}

void CRemoteManDlg::MstscConnent(HOST_STRUCT const *pHost, CONFIG_STRUCT const *pConfig)
{
	char RdpStr[2048],str[768];
	//连接模式
	int len=sprintf_s(RdpStr,sizeof(RdpStr),"screen mode id:i:%d\r\n",pConfig->MstscWinpos==0?2:1);
	//宽高
	int Width=1024,Height=768;
	if (pConfig->MstscWinpos!=0)
	{
		((CComboBox*)GetDlgItem(IDC_COMBO_MST_WINPOS))->GetLBText(pConfig->MstscWinpos,str);
		char *p=strchr(str,' ');
		*p=0;
		p+=4;
		Width=atoi(str);
		Height=atoi(p);
	}
	len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"desktopwidth:i:%d\r\ndesktopheight:i:%d\r\n",Width,Height);
	//颜色位数
	len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"session bpp:i:%d\r\n",pConfig->MstscColor==0 ? 16:pConfig->MstscColor==1 ? 24:32);
	//显示位置及窗口大小
	int ScreenWidth= GetSystemMetrics(SM_CXFULLSCREEN);
	int ScreenHeight= GetSystemMetrics(SM_CYFULLSCREEN);
	int xPos = (ScreenWidth-Width-40)/2, yPos=(ScreenHeight-Height-40)/2;
	if (xPos<0) xPos=0;
	if (yPos<0) yPos=0;
	len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"winposstr:s:0,1,%d,%d,%d,%d\r\n",xPos,yPos,xPos+Width+40,yPos+Height+60);
	//远程服务器地址
	if (pHost->HostPort==3389)
		len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"full address:s:%s\r\n",pHost->HostAddress);
	else
		len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"full address:s:%s:%d\r\n",pHost->HostAddress,pHost->HostPort);
	//将数据传输到客户端计算机时是否对数据进行压缩
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"compression:i:1\r\n");
	len+=sizeof("compression:i:1\r\n")-1;
	//确定何时将 Windows 键组合应用到桌面连接的远程会话
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"keyboardhook:i:2\r\n");
	len+=sizeof("keyboardhook:i:2\r\n")-1;
	//声音设置
	len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"audiomode:i:%d\r\n",pConfig->MstscRemoteAudio ? 0:2);
	//剪贴板 
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"redirectclipboard:i:1\r\n");
	len+=sizeof("redirectclipboard:i:1\r\n")-1;
	//PnP即插即用设备
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"devicestoredirect:s:\r\n");
	len+=sizeof("devicestoredirect:s:\r\n")-1;
	//磁盘驱动器
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"drivestoredirect:s:");
	len+=sizeof("drivestoredirect:s:")-1;
	if (pConfig->MstscUseDrive)
	{
		char volume[]="C:\\";
		for (int i=0; pConfig->MstscLocalDrive[i]>='A' && pConfig->MstscLocalDrive[i]<='Z'; i++)
		{
			volume[0]=pConfig->MstscLocalDrive[i];
			GetVolumeInformation(volume,str,16,NULL,NULL,NULL,NULL,0);
			len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"%s (%c:);",str,pConfig->MstscLocalDrive[i]);
		}
	}
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"\r\n");
	len+=2;
	//是否自动连接打印机
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"redirectprinters:i:0\r\n");
	len+=sizeof("redirectprinters:i:0\r\n")-1;
	//是否自动连接COM串行口
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"redirectcomports:i:0\r\n");
	len+=sizeof("redirectcomports:i:0\r\n")-1;
	//是否自动连接智能卡
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"redirectsmartcards:i:0\r\n");
	len+=sizeof("redirectsmartcards:i:0\r\n")-1;
	//全屏模式时是否显示连接栏
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"displayconnectionbar:i:1\r\n");
	len+=sizeof("displayconnectionbar:i:1\r\n")-1;
	//在断开连接后是否自动尝试重新连接
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"autoreconnection enabled:i:1\r\n");
	len+=sizeof("autoreconnection enabled:i:1\r\n")-1;
	//用户名和域
	len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"username:s:%s\r\n",pHost->Account);
	//指定要在远程会话中作为 shell（而不是资源管理器）自动启动的程序
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"alternate shell:s:\r\n");
	len+=sizeof("alternate shell:s:\r\n")-1;
	////RDP进行连接时自动启动的应用程序所在的文件夹位置程
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"shell working directory:s:\r\n");
	len+=sizeof("shell working directory:s:\r\n")-1;
	//RDP密码加密数据
	if (pHost->Password[0]!=0)
		len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"password 51:b:%s\r\n",CryptRDPPassword(pHost->Password,str,sizeof(str)));
	//禁止网络质量自动检测
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"networkautodetect:i:0\r\n");
	len+=sizeof("networkautodetect:i:0\r\n")-1;
	//指定连接类型为局域网10M或更高
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"connection type:i:6\r\n");
	len+=sizeof("connection type:i:6\r\n")-1;
	//是否禁止显示桌面背景
	len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"disable wallpaper:i:%d\r\n",pConfig->MstscDeskImg?0:1);
	//是否禁止主题
	len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"disable themes:i:%d\r\n",pConfig->MstscThemes?0:1);
	//是否允许字体平滑
	len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"allow font smoothing:i:%d\r\n",pConfig->MstscFontSmooth?1:0);
	//将文件夹拖到新位置时是否显示文件夹内容
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"disable full window drag:i:1\r\n");
	len+=sizeof("disable full window drag:i:1\r\n")-1;
	//禁用菜单动画
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"disable menu anims:i:1\r\n");
	len+=sizeof("disable menu anims:i:1\r\n")-1;
	//禁用光标设置
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"disable cursor setting:i:0\r\n");
	len+=sizeof("disable cursor setting:i:0\r\n")-1;
	//是否将位图缓存在本地计算机上
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"bitmapcachepersistenable:i:1\r\n");
	len+=sizeof("bitmapcachepersistenable:i:1\r\n")-1;
	//定义服务器身份验证级别设置
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"authentication level:i:0\r\n");
	len+=sizeof("authentication level:i:0\r\n")-1;
	//?
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"prompt for credentials:i:0\r\n");
	len+=sizeof("prompt for credentials:i:0\r\n")-1;
	//确定是否保存用户的凭据并将其用于 RD 网关和远程计算机
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"promptcredentialonce:i:1\r\n");
	len+=sizeof("promptcredentialonce:i:1\r\n")-1;

	//存储到文件,借用str变量
	GetTempPath(sizeof(str),str);
	strcat_s(str,sizeof(str),pHost->Name);
//	sprintf_s(str,sizeof(str),"c:\\%s_RDP.rdp",pHost->HostAddress);
	CFile file;
	if (!file.Open(str,CFile::modeCreate|CFile::modeWrite|CFile::typeBinary)) return;
	file.Write(RdpStr,len);
	file.Close();

	//启动
	char szBuffer[MAX_PATH];
	SHGetSpecialFolderPath(NULL, szBuffer, CSIDL_SYSTEM, FALSE);
	sprintf_s(RdpStr,sizeof(RdpStr),"%s\\Mstsc.exe /%s %s", szBuffer,pConfig->MstscConsole?"console":"admin",str);	//命令行
	WinExec(RdpStr, SW_SHOW);
	TRACE("Rdp文件长度=%d Rdp命令行:%s\r\n",len,RdpStr);
	Sleep(1000);
	DeleteFile(str);
}

void RadminConnent(HOST_STRUCT const *pHost, CONFIG_STRUCT const *pConfig, int CtrlMode)
{
	static const char MODE[][10]={"","/noinput","/file","/shutdown"};
	static const char COLOUR[][8]={"/8bpp","/16bpp","/24bpp"};
	//查看文件是否存在
	char const *RadminPath=GetExePath(pConfig->RadminPath,RADMIN_DEF_PATH);
	if (RadminPath==NULL)
	{
		AfxMessageBox("Radmin路径设置错误");
		return;
	}
	char str1[100],str2[64];
	//启动Radmin连接服务器
	if (pHost->HostPort==4899)
		sprintf_s(str1,sizeof(str1),"/connect:%s %s %s",pHost->HostAddress,MODE[CtrlMode],COLOUR[pConfig->RadminColor]);
	else
		sprintf_s(str1,sizeof(str1),"/connect:%s:%d %s %s",pHost->HostAddress,pHost->HostPort,MODE[CtrlMode],COLOUR[pConfig->RadminColor]);
	if (pConfig->RadminFullScreen)
		strcat_s(str1,sizeof(str1)," /fullscreen");
	TRACE("%s\r\n",str1);
	ShellExecute(NULL,"open",RadminPath,str1,NULL,SW_SHOW);
	//窗口名称
	sprintf_s(str1,sizeof(str1),"Radmin 安全性：%s",pHost->HostAddress);	//Radmin2.2的窗口名称
	sprintf_s(str2,sizeof(str2),"Radmin 安全性: %s",pHost->HostAddress);	//Radmin3.4的窗口名称
	//查找Radmin启动窗口
	HWND hWnd;
	clock_t t2,t1=clock();
	for (t2=t1;t2-t1<8000;t2=clock())
	{
		Sleep(1);
		hWnd=FindWindow(NULL,str1);
		if (hWnd!=NULL)
			break;
		hWnd=FindWindow(NULL,str2);
		if (hWnd!=NULL)
			break;
	}
	if (hWnd==NULL)	return;
	//填写信息并发送
	HWND UserWnd=::GetDlgItem(hWnd,0x7ff);
	HWND PasswordWnd=::GetDlgItem(hWnd,0x800);
	if (UserWnd!=NULL)
		::SendMessage(UserWnd,WM_SETTEXT,0,(LPARAM)pHost->Account);
	if (PasswordWnd!=NULL)
		::SendMessage(PasswordWnd,WM_SETTEXT,0,(LPARAM)pHost->Password);
	::PostMessage(hWnd,WM_COMMAND,0x78,0);
}

void VNCConnent(HOST_STRUCT const *pHost, CONFIG_STRUCT const *pConfig)
{
	char const *VNCPath=GetExePath(pConfig->VNCPath,VNC_DEF_PATH);
	if (VNCPath==NULL)
	{
		AfxMessageBox("VNC路径设置错误");
		return;
	}
	char str[512];
	//启动VNC连接服务器
	sprintf_s(str,sizeof(str),"%s %s:%d -password=%s",VNCPath,pHost->HostAddress,pHost->HostPort,pHost->Password);
	TRACE("%s\r\n",str);
//	AfxMessageBox(str);
	WinExec(str,SW_SHOW);
}

void SSHConnent(HOST_STRUCT const *pHost, CONFIG_STRUCT const *pConfig)
{
	char str[MAX_PATH*2];
	//查看文件是否存在
	char const *SSHPath = GetExePath(pConfig->SSHPath,SSH_DEF_PATH);
	if (SSHPath==NULL)
	{
		AfxMessageBox("SSH路径设置错误");
		return;
	}
	//参数格式
	char const *Format=pConfig->SSHParamFormat;
	if (Format[0]==0)
	{
		CString Path=SSHPath;
		Path.MakeLower();
		if (Path.Find("securecrt.exe")!=-1)
			Format="/ssh2 %3@%1 /P %2 /PASSWORD %4";
		else if (Path.Find("putty.exe")!=-1)
			Format="-ssh -l %3 -pw %4 -P %2 %1";
/*		else if (Path.Find("winscp.exe")!=-1)
			Format="%3:%4@%1:%2";*/
		else
		{
			AfxMessageBox("SSH命令行参数格式设置错误.");
			return;
		}
	}

	//连接
	int len=sprintf_s(str,sizeof(str),"%s ",SSHPath);
	while (*Format)
	{
		if (*Format!='%')
		{
			str[len++]=*Format++;
			continue;
		}
		Format++;
		switch (*Format++)
		{
		case '1':
			len+=sprintf_s(str+len,sizeof(str)-len,"%s",pHost->HostAddress);
			break;
		case '2':
			len+=sprintf_s(str+len,sizeof(str)-len,"%d",pHost->HostPort);
			break;
		case '3':
			len+=sprintf_s(str+len,sizeof(str)-len,"%s",pHost->Account);
			break;
		case '4':
			len+=sprintf_s(str+len,sizeof(str)-len,"%s",pHost->Password);
			break;
		default:
			str[len++]='%';
			Format--;
		}
	}
	TRACE("%s\r\n",str);
	WinExec(str,SW_SHOW);
}

bool CRemoteManDlg::GetSelectHost(HOST_STRUCT *pHost)
{
	if (m_List.GetSelectedCount()!=1) return false;
	int n=m_List.GetSelectionMark();
	//获取主机信息
	char sqlstr[128];
	CArray<HOST_STRUCT,HOST_STRUCT&>HostArray;
	sprintf_s(sqlstr,sizeof(sqlstr),"select * from HostTab where Id=%d;",m_List.GetItemData(n));
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, ReadHostCallBack, &HostArray, NULL);
	if (HostArray.GetSize()!=1) return false;
	*pHost=HostArray[0];
	if (pHost->Password[0]!=0)
	{
		byte data[36];
		int len=StringToBytes(pHost->Password,data);
		if (len>0)
		{
			len=AesDeCode(data,len,AES_KEY);
			if (len>0) strcpy_s(pHost->Password,sizeof(pHost->Password),(char*)data);
		}
	}
	return true;
}

void CRemoteManDlg::ConnentHost(int CtrlMode)
{
	HOST_STRUCT Host;
	if (!GetSelectHost(&Host)) return;
	//
	if (strcmp(CTRL_MODE[Host.CtrlMode],CTRL_MODE_RDP_NAME)==0)
		MstscConnent(&Host,&SysConfig);
	else if (strcmp(CTRL_MODE[Host.CtrlMode],CTRL_MODE_RADMIN_NAME)==0)
		RadminConnent(&Host,&SysConfig,CtrlMode);
	else if (strcmp(CTRL_MODE[Host.CtrlMode],CTRL_MODE_SSH_NAME)==0)
		SSHConnent(&Host,&SysConfig);
	else if (strcmp(CTRL_MODE[Host.CtrlMode],CTRL_MODE_VNC_NAME)==0)
		VNCConnent(&Host,&SysConfig);
}

void CRemoteManDlg::OnMenuClickedConnentHost(void)
{
	ConnentHost(SysConfig.RadminCtrlMode);
}

void CRemoteManDlg::OnMenuClickedRadminCtrl(UINT Id)
{
	ConnentHost(Id-ID_MENU_FULLCTRL);
}

void CRemoteManDlg::OnMenuClickedWinScpConnent(void)
{
	//查看文件是否存在
	char const *WinScpPath = GetExePath(SysConfig.WinScpPath,WINSCP_DEF_PATH);
	if (WinScpPath==NULL)
	{
		MessageBox("WinScp路径设置错误");
		return;
	}
	char str[MAX_PATH*2];
	HOST_STRUCT Host;
	if (!GetSelectHost(&Host)) return;
	
	//命令行
	sprintf_s(str,sizeof(str),"%s scp://%s:%s@%s:%d",WinScpPath, Host.Account, Host.Password, Host.HostAddress, Host.HostPort);
	TRACE("%s\r\n",str);
	WinExec(str,SW_SHOW);
}

void CRemoteManDlg::OnMenuClickedRenameGroup(void)
{
	HTREEITEM nItem=m_Tree.GetSelectedItem();
	if (nItem==NULL)
		return;
	m_Tree.EditLabel(nItem);
}

void CRemoteManDlg::OnTvnEndlabeleditTree1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CString Name=pTVDispInfo->item.pszText;
	Name.Trim();
	if (Name.GetLength()==0 || Name.GetLength()>=64 || pTVDispInfo->item.mask==0)
		return;
	
	char sqlstr[128];
	int Cnt=0;
	int Id=m_Tree.GetItemData(pTVDispInfo->item.hItem);
	HTREEITEM hParentItem=m_Tree.GetParentItem(pTVDispInfo->item.hItem);
	int ParentId=hParentItem==0 ? 0:m_Tree.GetItemData(hParentItem);
	//先检查此名称是否存在
	int rc=sprintf_s(sqlstr,sizeof(sqlstr),"select count() from GroupTab where ParentId=%d and Name='%s' and Id!=%d;",ParentId,Name,Id);
	TRACE("%s\r\n",sqlstr);
	rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), ReadIntCallback, &Cnt, NULL);
	if (Cnt!=0) 
	{
		MessageBox("该分组名称已经存在","错误",MB_ICONERROR);
		return;
	}
	//更新数据库
	sprintf_s(sqlstr,sizeof(sqlstr),"update GroupTab set Name='%s' where Id=%d", Name, Id);
	TRACE("%s\r\n",sqlstr);
	rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), NULL, NULL, NULL);

	*pResult = 1;
}

void CRemoteManDlg::LoadHostList(HTREEITEM hItem)
{
	int rc;
	CArray<int ,int>GroupArray;
	GroupArray.SetSize(0,20);
	//枚举组ID
	if (SysConfig.ParentShowHost)
		EnumChildGroupId(hItem,GroupArray);
	else
		GroupArray.Add(m_Tree.GetItemData(hItem));
	//载入主机表
	int sqlstrlen=28+GroupArray.GetSize()*23,len;
	char *sqlstr=new char[sqlstrlen];
	len=sprintf_s(sqlstr,sqlstrlen,"select * from HostTab where ");
	for (int i=0; i<GroupArray.GetSize();i++)
		len+=sprintf_s(sqlstr+len,sqlstrlen-len,i==0 ? "ParentId=%d":" or ParentId=%d",GroupArray[i]);
	sqlstr[len++]=';';
	sqlstr[len]=0;
	CArray<HOST_STRUCT,HOST_STRUCT&>HostArray;
	HostArray.SetSize(0,20);
	TRACE("%s\r\n",sqlstr);
	rc = sqlite3_exec(m_pDB, sqlstr, ReadHostCallBack, &HostArray, NULL);
	for (int i=0; i<HostArray.GetSize(); i++)
	{
		HOST_STRUCT Host=HostArray[i];
		ListAddHost(&Host,Host.Id);
	}
	delete sqlstr;
}

void CRemoteManDlg::OnTvnSelchangedTree1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	m_List.DeleteAllItems();
	SetDlgItemText(IDC_EDIT_README,"");
	SysConfig.GroupLastSelId=m_Tree.GetItemData(pNMTreeView->itemNew.hItem);
	LoadHostList(pNMTreeView->itemNew.hItem);
	*pResult = 0;
}

static int ReadStrCallback(void* para, int n_column, char** column_value, char** column_name)
{
	if (column_value[0]!=NULL)
		strcpy_s((char*)para,256,CodeConverter::Utf8ToAscii(column_value[0]).c_str());
	return 0;
}

void CRemoteManDlg::OnLvnItemchangedList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	if (pNMLV->uNewState==0) return;
	int n=m_List.GetSelectedCount();
	if (n!=1) 
	{
		SetDlgItemText(IDC_EDIT_README,"");
		return;
	}
	int Id=m_List.GetItemData(pNMLV->iItem);

	char sqlstr[64], Readme[256]={0};
	sprintf_s(sqlstr,sizeof(sqlstr),"select HostReadme from HostTab where id=%d;",Id);
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, ReadStrCallback, Readme, NULL);
	SetDlgItemText(IDC_EDIT_README,Readme);
	
	*pResult = 0;
}

afx_msg LRESULT CRemoteManDlg::OnModifyPasswordMessage(WPARAM wParam, LPARAM lParam)
{
	char const *Src=(char*)wParam;
	char const *New=(char*)lParam;
	char sqlstr[128];

	if (Src[0]!=0 || SysConfig.SysPassword[0]!=0)
	{
		if (strcmp(SysConfig.SysPassword,  AesEnCodeToStr(Src,strlen(Src),sqlstr,AES_KEY))!=0)
			return LRESULT("原密码错误.");
	}

	strcpy_s(SysConfig.SysPassword,sizeof(SysConfig.SysPassword),New[0]==0 ? "":AesEnCodeToStr(New,strlen(New),sqlstr,AES_KEY));
	int n=sprintf_s(sqlstr,sizeof(sqlstr),"update ConfigTab set Password='%s' where id=0;",SysConfig.SysPassword);
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);

	return LRESULT("密码修改成功.");
}

void CRemoteManDlg::OnBnClickedCheckMstShowWallpaper()
{
	// TODO: 在此添加控件通知处理程序代码
	SysConfig.MstscDeskImg=((CButton*)GetDlgItem(IDC_CHECK_MST_SHOW_WALLPAPER))->GetCheck()!=0;

	char sqlstr[128];
	int n=sprintf_s(sqlstr,sizeof(sqlstr),"update ConfigTab set MstscDeskImg=%s where id=0;",SysConfig.MstscDeskImg?"true":"false");
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
}

void CRemoteManDlg::OnBnClickedCheckMstDrive()
{
	// TODO: 在此添加控件通知处理程序代码
	SysConfig.MstscUseDrive=((CButton*)GetDlgItem(IDC_CHECK_MST_DRIVE))->GetCheck()!=0;

	char sqlstr[128];
	int n=sprintf_s(sqlstr,sizeof(sqlstr),"update ConfigTab set MstscUseDrive=%s where id=0;",SysConfig.MstscUseDrive?"true":"false");
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
}

void CRemoteManDlg::OnBnClickedCheckMstAudio()
{
	// TODO: 在此添加控件通知处理程序代码
	SysConfig.MstscRemoteAudio=((CButton*)GetDlgItem(IDC_CHECK_MST_AUDIO))->GetCheck()!=0;

	char sqlstr[128];
	int n=sprintf_s(sqlstr,sizeof(sqlstr),"update ConfigTab set MstscRemoteAudio=%s where id=0;",SysConfig.MstscRemoteAudio?"true":"false");
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
}

void CRemoteManDlg::OnCbnSelchangeComboMstWinpos()
{
	// TODO: 在此添加控件通知处理程序代码
	SysConfig.MstscWinpos=((CComboBox*)GetDlgItem(IDC_COMBO_MST_WINPOS))->GetCurSel();

	char sqlstr[128];
	int n=sprintf_s(sqlstr,sizeof(sqlstr),"update ConfigTab set MstscWinpos=%d where id=0;",SysConfig.MstscWinpos);
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
}

void CRemoteManDlg::OnBnClickedCheckRadminFullscreen()
{
	// TODO: 在此添加控件通知处理程序代码
	SysConfig.RadminFullScreen=((CButton*)GetDlgItem(IDC_CHECK_RADMIN_FULLSCREEN))->GetCheck()!=0;

	char sqlstr[128];
	int n=sprintf_s(sqlstr,sizeof(sqlstr),"update ConfigTab set RadminFullScreen=%s where id=0;",SysConfig.RadminFullScreen?"true":"false");
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
}


void CRemoteManDlg::OnCbnSelchangeComboRadminCtrlmode()
{
	// TODO: 在此添加控件通知处理程序代码
	SysConfig.RadminCtrlMode=((CComboBox*)GetDlgItem(IDC_COMBO_RADMIN_CTRLMODE))->GetCurSel();

	char sqlstr[128];
	int n=sprintf_s(sqlstr,sizeof(sqlstr),"update ConfigTab set RadminCtrlMode=%d where id=0;",SysConfig.RadminCtrlMode);
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
}


void CRemoteManDlg::OnNMRClickTree1(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	CMenu Menu;
	Menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu *pSubMenu=Menu.GetSubMenu(m_Tree.GetSelectedItem()==NULL ? 0:1);
	CPoint point;
	GetCursorPos(&point);
	pSubMenu->TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_VERTICAL,point.x,point.y,this);

	*pResult = 0;
}


void CRemoteManDlg::OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	int Index=0;
	int SelCnt=m_List.GetSelectedCount();

	if (SelCnt==0)
		Index=2;
	else if (SelCnt==1)
	{
		char str[12];
		m_List.GetItemText(pNMItemActivate->iItem,0,str,10);
		if (strcmp(str,CTRL_MODE_RADMIN_NAME)==0)
			Index=5;
		else if (strcmp(str,CTRL_MODE_RDP_NAME)==0 || strcmp(str,CTRL_MODE_VNC_NAME)==0)
			Index=4;
		else if (strcmp(str,CTRL_MODE_SSH_NAME)==0)
			Index=6;
//		else if (strcmp(str,CTRL_MODE_VNC_NAME)==0)
//			Index=7;
		else
			return;
	}
	else
		Index=3;

	CMenu Menu;
	Menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu *pSubMenu=Menu.GetSubMenu(Index);
	CPoint point;
	GetCursorPos(&point);
	pSubMenu->TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_VERTICAL,point.x,point.y,this);

	*pResult = 0;
}

void CRemoteManDlg::OnNMDblclkList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	OnMenuClickedConnentHost();
	*pResult = 0;
}


void CRemoteManDlg::OnLvnBegindragList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	m_nListDragIndex = pNMLV->iItem;
	POINT pt={8,8};
	m_pDragImage=m_List.CreateDragImage(m_nListDragIndex,&pt);
	if (m_pDragImage==NULL)
	{
		m_nListDragIndex = -1; 
		return ; 
	}
	m_pDragImage->BeginDrag(0,CPoint(8,8));
	m_pDragImage->DragEnter(GetDesktopWindow(), pNMLV->ptAction);
	SetCapture();
	*pResult = 0;
}


void CRemoteManDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_nListDragIndex!=-1)
	{     
		CPoint pt(point); 
		ClientToScreen(&pt); 
		m_pDragImage->DragMove(pt); 
		m_pDragImage->DragShowNolock(false); 
		m_pDragImage->DragShowNolock(true);
	} 
	CDialogEx::OnMouseMove(nFlags, point);
}


void CRemoteManDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_nListDragIndex==-1) 
	{
		CDialogEx::OnLButtonUp(nFlags, point);
		return;
	}
	//释放资源
	ReleaseCapture(); 
	m_pDragImage->DragLeave(GetDesktopWindow()); 
	m_pDragImage->EndDrag(); 
	delete m_pDragImage; 
	m_nListDragIndex=-1;
	//判断拖入处是否是树控件的标签处
	CPoint pt(point); 
	ClientToScreen(&pt); 
	CWnd *pWnd = WindowFromPoint(pt); 
	if (pWnd==&m_List)
	{
		//拖动到的位置m, 拖动项n
		m_List.ScreenToClient(&pt);
		int m=m_List.HitTest(pt,&nFlags);
		if (m==-1) m=m_List.GetItemCount()-1;
		if (m_List.GetSelectedCount()!=1)
		{
			MessageBox("当前只支持单项拖动排序。","错误",MB_ICONERROR);
			return;
		}
		int n=m_List.GetSelectionMark();
		if (n==m) return;
		TRACE("从%d拖动到%d\r\n",n,m);
		//先读取出选中的主机
		int nId=m_List.GetItemData(n);
		CArray<HOST_STRUCT,HOST_STRUCT&>HostArray;
		char sqlstr[128];
		int rc=sprintf_s(sqlstr,sizeof(sqlstr),"select * from HostTab where id=%d;",nId);
		TRACE("%s\r\n",sqlstr);
		rc = sqlite3_exec(m_pDB, sqlstr, ReadHostCallBack, &HostArray, NULL);
		if (HostArray.GetSize()!=1) return;
		//读取最大的ID，临时ID使用MaxID+1
		int MaxId=0;
		strcpy_s(sqlstr,sizeof(sqlstr),"select max(id) from HostTab;");
		TRACE("%s\r\n",sqlstr);
		rc = sqlite3_exec(m_pDB, sqlstr, ReadIntCallback, &MaxId, NULL);
		//更新数据库
		//先将拖动到的项设置为一个临时ID
		rc = sprintf_s(sqlstr,sizeof(sqlstr),"update HostTab set id=%d where id=%d",MaxId+1,nId);
		TRACE("%s\r\n",sqlstr);
		rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
		//循环更改
		for (int k=n; k!=m;)
		{
			k+=n>=m ? -1:1;		//下一个的方向
			int tmpId=m_List.GetItemData(k);
			m_List.SetItemData(k,nId);
			rc = sprintf_s(sqlstr,sizeof(sqlstr),"update HostTab set id=%d where id=%d",nId,tmpId);
			TRACE("%s\r\n",sqlstr);
			rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
			nId=tmpId;
		}
		//还原拖动项的ID
		rc = sprintf_s(sqlstr,sizeof(sqlstr),"update HostTab set id=%d where id=%d",nId,MaxId+1);
		TRACE("%s\r\n",sqlstr);
		rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
		//列表框删除后重新添加
		m_List.DeleteItem(n);
		ListAddHost(&HostArray[0],nId,m);
	}
	else if (pWnd==&m_Tree) 
	{
		m_Tree.ScreenToClient(&pt);
		HTREEITEM hItem=m_Tree.HitTest(pt, &nFlags); //用于获取拖入处的Item
		if (hItem == NULL)  return;

		int Cnt=m_List.GetSelectedCount();
		if (Cnt==0) return;
		//更新数据库
		int ParentId = m_Tree.GetItemData(hItem);
		int sqlstrlen=44+17*Cnt;
		char *sqlstr=new char[sqlstrlen];
		int len=sprintf_s(sqlstr,sqlstrlen,"update HostTab set ParentId=%d where ",ParentId);
		POSITION pos=m_List.GetFirstSelectedItemPosition();
		for (int i=0; pos!=NULL && i<Cnt; i++)
		{
			int n=m_List.GetNextSelectedItem(pos);
			len+=sprintf_s(sqlstr+len,sqlstrlen-len, i==0 ? "id=%d" : " or id=%d",m_List.GetItemData(n));
		}
		sqlstr[len++]=';';
		sqlstr[len]=0;
		TRACE("%s\r\n",sqlstr);
		int rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
		//
		delete sqlstr;
		//重新选择树hItem
		m_Tree.SelectItem(hItem);
	}
}

void CRemoteManDlg::OnMenuClickedExportGroup(void)
{
	HTREEITEM hItem = m_Tree.GetSelectedItem();
	if (hItem==NULL) return;
	CFileDialog fdlg(FALSE,".db","ExportGroup",6,"*.db|*.db||");
	if (fdlg.DoModal()!=IDOK) return;
	//附加数据库
	char sqlstr[1536];
	sprintf_s(sqlstr,sizeof(sqlstr),"attach database '%s' as 'export';",fdlg.GetPathName());
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), NULL, NULL, NULL);
	if (rc!=0)
	{
		MessageBox("导出分组出错.","错误",MB_ICONERROR);
		return;
	}
	//添加表格
	rc=sprintf_s(sqlstr,sizeof(sqlstr),InitTabSqlStr,"export.","export.","export.","export.");	//要播放4个数据库名称
	TRACE("%s\r\n",sqlstr);
	rc=sqlite3_exec(m_pDB,sqlstr,NULL,NULL,NULL);
	if (rc!=0)
	{
		MessageBox("导出分组出错.","错误",MB_ICONERROR);
		return;
	}
	//枚举组ID	
	CArray<int ,int>GroupArray;
	GroupArray.SetSize(0,20);
	//枚举组ID
	EnumChildGroupId(hItem,GroupArray);
	//导出分组
	int sqlstrlen=58+23*GroupArray.GetSize(),len;
	char *sqlstr1=new char[sqlstrlen];
	len=sprintf_s(sqlstr1,sqlstrlen,"insert into export.GroupTab select * from GroupTab where ");
	for (int i=0; i<GroupArray.GetSize(); i++)
		len+=sprintf_s(sqlstr1+len,sqlstrlen-len,i==0?"id=%d":" or id=%d",GroupArray[i]);
	sqlstr1[len++]=';';
	sqlstr1[len]=0;
	TRACE("%s\r\n",sqlstr1);
	rc = sqlite3_exec(m_pDB, sqlstr1, NULL, NULL, NULL);
	//更改首分组父ID为0
	sprintf_s(sqlstr,sizeof(sqlstr),"update export.GroupTab set ParentId=0 where Id=%d;",GroupArray[0]);
	TRACE("%s\r\n",sqlstr);
	rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
	//导出主机
	len=sprintf_s(sqlstr1,sqlstrlen,"insert into export.HostTab select * from HostTab where ");
	for (int i=0; i<GroupArray.GetSize(); i++)
		len+=sprintf_s(sqlstr1+len,sqlstrlen-len,i==0?"ParentId=%d":" or ParentId=%d",GroupArray[i]);
	sqlstr1[len++]=';';
	sqlstr1[len]=0;
	TRACE("%s\r\n",sqlstr1);
	rc = sqlite3_exec(m_pDB, sqlstr1, NULL, NULL, NULL);
	//分离数据库
	strcpy_s(sqlstr,sizeof(sqlstr),"detach database 'export';");
	TRACE("%s\r\n",sqlstr);
	rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
	//
	delete sqlstr1;
}

/*
ImportParentId: 导入到主数据库的父分组ID
ExportParentId：导出的附加数据库的分组ID
*/
void CRemoteManDlg::ImportGroup(HTREEITEM hItem, int ExportParentId)
{
	int rc;
	char sqlstr[512];
	CArray<GROUP_STRUCT,GROUP_STRUCT&>GroupArray;
	GroupArray.SetSize(0,20);
	rc=sprintf_s(sqlstr,sizeof(sqlstr),"select * from export.GroupTab where ParentId=%d;",ExportParentId);
	TRACE("%s\r\n",sqlstr);
	rc = sqlite3_exec(m_pDB, sqlstr, ReadGroupCallBack, &GroupArray, NULL);
	//将该分组下的子分组添加到数据库和树控件，并扫描该子分组
	for (int i=0; i<GroupArray.GetSize(); i++)
	{
		GROUP_STRUCT g=GroupArray[i];
		//添加到数据库,先要检测此分组名是否存在
		int Id=0;
		HTREEITEM hChildItem=m_Tree.GetChildItem(hItem);
		while (hChildItem)
		{
			if (strcmp(m_Tree.GetItemText(hChildItem),g.Name)==0)
				break;
			hChildItem=m_Tree.GetNextSiblingItem(hChildItem);
		}
		//如果存在
		if (hChildItem!=NULL)
			Id=m_Tree.GetItemData(hChildItem);
		//没存在时，添加到数据库，并回读ID，添加到树控件
		else
		{
			Id=hItem==NULL || hItem==TVI_ROOT ? 0: m_Tree.GetItemData(hItem);		//这是要导入的父ID
			rc=sprintf_s(sqlstr,sizeof(sqlstr),"insert into GroupTab values(NULL,'%s',%d);",g.Name,Id);
			TRACE("%s\r\n",sqlstr);
			rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), NULL, NULL, NULL);
			rc=sprintf_s(sqlstr,sizeof(sqlstr),"select Id from GroupTab where ParentId=%d and Name='%s';",Id,g.Name);
			TRACE("%s\r\n",sqlstr);
			rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), ReadIntCallback, &Id, NULL);	//这是导入的子ID
			hChildItem=m_Tree.InsertItem(g.Name,0,1,hItem);
			m_Tree.SetItemData(hChildItem, Id);
		}
		//将该分组下的主机添加到数据库
		CArray<HOST_STRUCT,HOST_STRUCT&>HostArray;
		rc=sprintf_s(sqlstr,sizeof(sqlstr),"select * from export.HostTab where ParentId=%d;",g.Id);
		TRACE("%s\r\n",sqlstr);
		rc = sqlite3_exec(m_pDB, sqlstr, ReadHostCallBack, &HostArray, NULL);
		for (int i=0; i<HostArray.GetSize(); i++)
		{
			HOST_STRUCT Host=HostArray[i];
			//查看该主机名是否存在
			int Cnt;
			rc=sprintf_s(sqlstr,sizeof(sqlstr),"select count() from HostTab where ParentId=%d and Name='%s' and CtrlMode=%d;",
				Id, Host.Name, Host.CtrlMode);
			TRACE("%s\r\n",sqlstr);
			rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), ReadIntCallback, &Cnt, NULL);
			//添加到数据库
			if (Cnt==0)
			{
				rc=sprintf_s(sqlstr,sizeof(sqlstr),"insert into HostTab values(NULL,'%s',%d,%d,'%s',%d,'%s','%s','%s');",
					Host.Name, Id, Host.CtrlMode, Host.HostAddress, Host.HostPort, Host.Account, Host.Password, Host.ReadMe);
				TRACE("%s\r\n",sqlstr);
				rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), NULL, NULL, NULL);
			}
		}
		//枚举子分组
		ImportGroup(hChildItem,g.Id);
	}
}

void CRemoteManDlg::OnMenuClickedImportGroup(void)
{
	char sqlstr[512];
	//读取要导入的分组
	HTREEITEM hItem=m_Tree.GetSelectedItem();
	//选择导入的文件
	CFileDialogEx fdlg(TRUE,".db","",6,"*.db|*.db||");
	fdlg.m_ofn.lpstrTitle="选择导入的分组和数据";
	if (hItem!=NULL)
		fdlg.SetGroupName(m_Tree.GetItemText(hItem));
	if (fdlg.DoModal()!=IDOK) return;
	if (fdlg.m_GroupSel==0) hItem=TVI_ROOT;
	//附加数据库
	sprintf_s(sqlstr,sizeof(sqlstr),"attach database '%s' as 'export';",fdlg.GetPathName());
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), NULL, NULL, NULL);
	if (rc!=0)
	{
		MessageBox("导入分组出错.","错误",MB_ICONERROR);
		return;
	}
	//导入数据
	ImportGroup(hItem,0);
	//分离数据库
	strcpy_s(sqlstr,sizeof(sqlstr),"detach database 'export';");
	TRACE("%s\r\n",sqlstr);
	rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
	//更新列表
	hItem=m_Tree.GetSelectedItem();
	if (hItem!=0)
	{
		m_List.DeleteAllItems();
		SetDlgItemText(IDC_EDIT_README,"");
		LoadHostList(hItem);
	}
}

static bool ScanFunction(char const *Address, int Port, int TimeOut)
{
	SOCKET m_socket=socket(AF_INET,SOCK_STREAM,0);
	sockaddr_in serveraddr;
	serveraddr.sin_family=AF_INET;
	serveraddr.sin_port=htons(Port);
	hostent *host=gethostbyname(Address);
	if (host==NULL) return false;
	char *ip=inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);
	serveraddr.sin_addr.S_un.S_addr=inet_addr(ip);
	if (serveraddr.sin_addr.S_un.S_addr==INADDR_NONE)
		return FALSE;

	unsigned long ul = 1;
	int ret = ioctlsocket(m_socket, FIONBIO, (unsigned long*)&ul);
	if(ret==SOCKET_ERROR) return FALSE;

	connect(m_socket,(sockaddr*)&serveraddr,sizeof(serveraddr));

	struct timeval timeout ;
	fd_set r;
	FD_ZERO(&r);
	FD_SET(m_socket, &r);
	timeout.tv_sec = 0;				//超时 秒
	timeout.tv_usec =TimeOut*1000;	//超时 微秒
	ret = select(0, 0, &r, 0, &timeout);
	::closesocket(m_socket);
	return ret>0;
}

static DWORD CALLBACK ScanOnlineThread(LPVOID lp)
{
	CRemoteManDlg *pDlg=(CRemoteManDlg*)lp;
	for (int i=0; i<pDlg->m_List.GetItemCount(); i++)
		pDlg->m_List.SetItemText(i,5,"---");

	for (int i=0; i<pDlg->m_List.GetItemCount() && !pDlg->bScanExit; i++)
	{
		char Address[64];
		int Port;
		pDlg->m_List.GetItemText(i,3,Address,sizeof(Address));
		Port=atoi(Address);
		pDlg->m_List.GetItemText(i,2,Address,sizeof(Address));
		if (ScanFunction(Address,Port,pDlg->SysConfig.CheckOnlineTimeOut))
			pDlg->m_List.SetItemText(i,5,"√");
		else
			pDlg->m_List.SetItemText(i,5,"×");
	}

	pDlg->m_Tree.EnableWindow(TRUE);
	pDlg->SetDlgItemText(IDC_BTN_CHECK_ONLINE,"在线检测");
	return 0;
}

void CRemoteManDlg::OnBnClickedBtnCheckOnline()
{
	// TODO: 在此添加控件通知处理程序代码
	char str[10];
	GetDlgItemText(IDC_BTN_CHECK_ONLINE,str,10);
	if (strcmp(str,"停止检测")==0)
	{
		bScanExit=true;
	}
	else
	{
		int Cnt=m_List.GetItemCount();
		if (Cnt==0) return;
		bScanExit=false;
		m_Tree.EnableWindow(FALSE);
		SetDlgItemText(IDC_BTN_CHECK_ONLINE,"停止检测");
		CreateThread(NULL,0,ScanOnlineThread,this,0,NULL);
	}
}


void CRemoteManDlg::OnBnClickedBtnSearch()
{
	// TODO: 在此添加控件通知处理程序代码
	char str[64],sqlstr[512];
	if (GetDlgItemText(IDC_EDIT_SEARCH, str, sizeof(str)-1)==0 || m_Tree.GetCount()==0) return;
	CArray<HOST_STRUCT,HOST_STRUCT&>HostArray;
	HostArray.SetSize(0,20);
	if (strcmp(str,"*")==0)
		strcpy_s(sqlstr,sizeof(sqlstr),"select * from HostTab;");
	else
		sprintf_s(sqlstr,sizeof(sqlstr),"select * from HostTab where HostAddress like '%%%s%%' or Name like '%%%s%%';",str,str);
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), ReadHostCallBack, &HostArray, NULL);

	m_List.DeleteAllItems();
	SetDlgItemText(IDC_EDIT_README,"");
	for (int i=0; i<HostArray.GetSize(); i++)
	{
		HOST_STRUCT Host=HostArray[i];
		ListAddHost(&Host,Host.Id);
	}
}


void CRemoteManDlg::OnSize(UINT nType, int cx, int cy)
{
	//右侧控件位置表，只要改变左右位置即可
	static int const RIGHT_CTRL_IDS[]={
		IDC_STATIC_PIC,IDC_BTN_CHECK_ONLINE,
		IDC_STATIC_GROUP1,IDC_CHECK_MST_SHOW_WALLPAPER,IDC_CHECK_MST_DRIVE,IDC_CHECK_MST_AUDIO,
		IDC_STATIC1,IDC_COMBO_MST_WINPOS,IDC_STATIC_GROUP2,IDC_CHECK_RADMIN_FULLSCREEN,
		IDC_STATIC2,IDC_COMBO_RADMIN_CTRLMODE};
	static int Mincx,Mincy;
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码
	TRACE("nType=%d,cx=%d,cy=%d\r\n",nType,cx,cy);
	if (nType==1) return;		//最小化
	//不能小于最小尺寸
	if (Mincx==0) 
	{
		Mincx=cx,Mincy=cy;
		return;				//第一次进入子控件还没创建呢
	}
	if (cx<Mincx) cx=Mincx;
	if (cy<Mincy) cy=Mincy;
	//移动右侧控件位置
	CRect rt;
	int offset=INT_MAX;
	for (int i=0; i<sizeof(RIGHT_CTRL_IDS)/sizeof(RIGHT_CTRL_IDS[0]); i++)
	{
		CWnd *p=GetDlgItem(RIGHT_CTRL_IDS[i]);
		p->GetWindowRect(rt);
		ScreenToClient(rt);
		if (offset==INT_MAX) offset=cx-96-rt.left;	//第一个控件的位置相对右边为96
	//	TRACE("Top=%d,Bottom=%d,Left=%d,Right=%d\r\n",rt.top,rt.bottom,rt.left,rt.right);
		rt.OffsetRect(offset,0);
		p->MoveWindow(rt);
	}
	//移动树控件和列表框,两个宽度的原始比例为：184/597
	//左边界开始为4，中间空5，右边界CX-139,底部空3
	GetDlgItem(IDC_TREE1)->GetWindowRect(rt);
	ScreenToClient(rt);
	int TreeWidth=(cx-148)*184/(184+597);
	rt.right=rt.left+TreeWidth;
	rt.bottom=cy-3;
	GetDlgItem(IDC_TREE1)->MoveWindow(rt);
	//列表框,底部空136
	rt.left=rt.right+5;
	rt.right=cx-139;
	rt.bottom=cy-136;
	GetDlgItem(IDC_LIST1)->MoveWindow(rt);
	int ListWidth=rt.right-rt.left-25;		//25:滚动条宽度
	//调整列宽
	int Sum=0;
	for (int i=0; i<sizeof(ListDefColumnWidth)/sizeof(ListDefColumnWidth[0]); i++)
		Sum+=ListDefColumnWidth[i];
	for (int i=0; i<sizeof(ListDefColumnWidth)/sizeof(ListDefColumnWidth[0]); i++)
		m_List.SetColumnWidth(i,ListWidth*ListDefColumnWidth[i]/Sum);

	//用于搜索的两个控件,底部偏差：104, 上部空：7, 编辑框右边空:109
	rt.right-=109;
	rt.top=rt.bottom+7;
	rt.bottom=rt.top+25;		//高度为25
	m_SearchEdit.MoveWindow(rt);
	//按钮
	rt.left=rt.right+12;
	rt.right=rt.left+88;
	GetDlgItem(IDC_BTN_SEARCH)->MoveWindow(rt);

	//主机说明的组控件,
	rt.left=4+TreeWidth+5;
	rt.right=cx-139;
	rt.top=cy-98;
	rt.bottom=cy-3;
	GetDlgItem(IDC_STATIC_GROUP3)->MoveWindow(rt);
	//主机说明的EDIT控件
	rt.left+=12;
	rt.right-=13;
	rt.top+=23;
	rt.bottom-=12;
	GetDlgItem(IDC_EDIT_README)->MoveWindow(rt);
	//
	Invalidate();		//有残影要重绘
}


void CRemoteManDlg::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
//	CDialogEx::OnSettingChange(uFlags, lpszSection); //避免系统设置改变时工具栏的大小被改变

	// TODO: 在此处添加消息处理程序代码
}


void CRemoteManDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
	::RemoveProp(m_hWnd,AfxGetApp()->m_pszExeName);
	// TODO: 在此处添加消息处理程序代码
}


void CRemoteManDlg::OnMenuClickedPing(void)
{
	HOST_STRUCT Host;
	if (!GetSelectHost(&Host)) return;

	char str[128];
	sprintf_s(str,sizeof(str),"ping %s -t",Host.HostAddress);
	WinExec(str,SW_SHOW);

/*
	HOST_STRUCT Host;
	if (!GetSelectHost(&Host)) return;
	char FilePath[MAX_PATH+128];
	int n=GetTempPath(MAX_PATH,FilePath);
	if (FilePath[n-1]=='\\') n-=1;
	sprintf_s(FilePath+n,sizeof(FilePath)-n,"\\ping_%s.bat",Host.HostAddress);

	CFile file;
	if (file.Open(FilePath,CFile::modeWrite|CFile::modeCreate))
	{
		file.Write("@echo off\r\n",sizeof("@echo off\r\n")-1);
		file.Write("cls\r\n",sizeof("cls\r\n")-1);
		char str[128];
		n=sprintf_s(str,sizeof(str),"ping %s -t\r\n",Host.HostAddress);
		file.Write(str,n);
		file.Write("pause\r\n",sizeof("pause\r\n")-1);
		file.Close();

		WinExec(FilePath,SW_SHOW);
		Sleep(1000);
		DeleteFile(FilePath);
	}*/
}
