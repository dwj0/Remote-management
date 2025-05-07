
// RemoteManDlg.cpp : ʵ���ļ�
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
VNCType boolean,\r\n\
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
insert into %sConfigTab values(0, 2, 0, true, '', '', '', '', '','', 0,500, false, true,'',false,0,0,true, true, true, 0, true, 1);\r\n\
COMMIT;\r\n\
";


//����66�ֽ�˵����������󳤶�Ϊ32�ֽ�(16-31�ֽڶ���Ҫ���䵽32�ֽڵ�)��ʹ��ASCII�洢=64�ֽ�
// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

//Ĭ���б����
static int const ListDefColumnWidth[]={80,160,140,55,100,37};

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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
	// mstsc.exe��ʹ�õ���unicode,���Ա��������ַ�ת��
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

// CRemoteManDlg �Ի���
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
	strcpy_s(Host.Name,sizeof(Host.Name),column_value[1]);				//��������
	Host.CtrlMode=atoi(column_value[3]);								//��������
	strcpy_s(Host.HostAddress,sizeof(Host.HostAddress),column_value[4]);//������ַ
	Host.HostPort=atoi(column_value[5]);								//�˿�
	strcpy_s(Host.Account,sizeof(Host.Account),column_value[6]);		//�ʺ�
	if (column_value[7]!=NULL)											//����
		strcpy_s(Host.Password,sizeof(Host.Password),column_value[7]);
	if (column_value[8]!=NULL)											//˵��
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
	//��������
	strcpy_s(Host.Name,sizeof(Host.Name),CodeConverter::Utf8ToAscii(column_value[1]).c_str());	
	//��ID
	Host.ParentId=atoi(column_value[2]);	
	//��������
	Host.CtrlMode=atoi(column_value[3]);	
	//������ַ
	strcpy_s(Host.HostAddress,sizeof(Host.HostAddress),CodeConverter::Utf8ToAscii(column_value[4]).c_str());	
	//�˿�
	Host.HostPort=atoi(column_value[5]);		
	//�ʺ�
	strcpy_s(Host.Account,sizeof(Host.Account),CodeConverter::Utf8ToAscii(column_value[6]).c_str());
	//����
	if (column_value[7]!=NULL)																		
		strcpy_s(Host.Password,sizeof(Host.Password),column_value[7]);
	//˵��
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

	for (int i = 0; i < n_column; i++)
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
		else if (strcmp(column_name[i], "VNCType") == 0)
			pConfig->VNCType = atoi(column_value[i]);
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
	//���DatabaseVer!=-1ʱ���������µ����ݿ�汾��Ҫ�����ݽ���UTF8-ASCIIת��
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
	//������ñ��Ƿ����
	char const *sqlstr = "select count(type) from sqlite_master where tbl_name='ConfigTab';";
	TRACE("%s\r\n",sqlstr);
	rc = sqlite3_exec(m_pDB, sqlstr, ReadIntCallback, &TabCnt, NULL);
	//������ʱ�������
	if (TabCnt==0)
	{
		char sqlstr[1536];
		rc=sprintf_s(sqlstr,sizeof(sqlstr),InitTabSqlStr,"","","","");		//����4�������ݿ������
		TRACE("%s\r\n",sqlstr);
		rc=sqlite3_exec(m_pDB,sqlstr,NULL,NULL,NULL);
	}
	return rc==0;
}

void CRemoteManDlg::DataBaseConversion(int Ver)
{
	int rc;
	//��DatabaseVer=-1ʱ����������ֶβ����ڣ�Ҫ��ӣ���ͬʱ���VNCPath�к�CheckOnlineTimeOut�С� ���⣬֮ǰ���ݿⱣ�����GB2312���룬Ҫת����UTF8��ʽ
	if (SysConfig.DatabaseVer==-1)
	{
		SysConfig.DatabaseVer=0;
		if (AfxMessageBox("�������ݿ����Ascii��UTF8�ı���ת��\r\nת�������ݿⲻ�����ھɰ����",MB_OKCANCEL)!=IDOK)
			exit(0);
		//�����
		char sqlstr[1024]="alter table ConfigTab add column DatabaseVer int;"
			"alter table ConfigTab add column CheckOnlineTimeOut int;"
			"alter table ConfigTab add column VNCPath char(256);";
		TRACE("%s\r\n",sqlstr);
		rc=sqlite3_exec(m_pDB,sqlstr,NULL,NULL,NULL);
		//ת��ConfigTab�ı���
		rc=sprintf_s(sqlstr,sizeof(sqlstr),"update ConfigTab set RadminPath='%s',SSHPath='%s',VNCPath='%s' where id=0;",
			SysConfig.RadminPath, SysConfig.SSHPath, SysConfig.VNCPath);
		TRACE("%s\r\n",sqlstr);
		rc=sqlite3_exec(m_pDB,CodeConverter::AsciiToUtf8(sqlstr).c_str(),NULL,NULL,NULL);
		//ת��GroupTab�ı��� 
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
		//ת��HostTab�ı��� 
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
	//��DatabaseVer=0ʱ��ȱ��SSHParamFormat��
	if (SysConfig.DatabaseVer==0)
	{
		SysConfig.DatabaseVer=1;
		//�����
		char sqlstr[1024]="alter table ConfigTab add column SSHParamFormat char(64);"
			"update ConfigTab set DatabaseVer=1 where id=0;";
		TRACE("%s\r\n",sqlstr);
		rc=sqlite3_exec(m_pDB,sqlstr,NULL,NULL,NULL);
	}
	//��DatabaseVer=1ʱ��ȱ��WinScpPath��
	if (SysConfig.DatabaseVer==1)
	{
		SysConfig.DatabaseVer=2;
		//�����
		char sqlstr[1024]="alter table ConfigTab add column WinScpPath char(256);"
			"update ConfigTab set DatabaseVer=2 where id=0;";
		TRACE("%s\r\n",sqlstr);
		rc=sqlite3_exec(m_pDB,sqlstr,NULL,NULL,NULL);
	}
	//��DatabaseVer=2ʱ��ȱ��VNCType��
	if (SysConfig.DatabaseVer == 2)
	{
		SysConfig.DatabaseVer = 3;
		//�����
		char sqlstr[1024] = "alter table ConfigTab add column VNCType boolean;"
			"update ConfigTab set DatabaseVer=3,VNCType=1 where id=0;";
		TRACE("%s\r\n", sqlstr);
		rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
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
	//�ȱ���һ�����ݿ�,��һ�ź��ҩ
	char BakPath[MAX_PATH];
	sprintf_s(BakPath,MAX_PATH,"%s.bak",Path);
	CFileStatus fstatus;
	if (CFile::GetStatus(Path,fstatus))
		CopyFile(Path,BakPath,FALSE);

	if (!OpenUserDb(CodeConverter::AsciiToUtf8(Path).c_str()))
	{
		char str[200];
		sprintf_s(str,sizeof(str),"�����ݿ�ʧ�ܣ�%s",sqlite3_errmsg(m_pDB));
		AfxMessageBox(str);
		exit(0);
	}
	//��ȡ����
	memset(&SysConfig,0,sizeof(CONFIG_STRUCT));
	char const *sqlstr="select * from ConfigTab where id=0;";
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, ReadConfigCallback, &SysConfig, NULL);
	//���ݿ�汾ת��
	DataBaseConversion(SysConfig.DatabaseVer);
	if (SysConfig.CheckOnlineTimeOut<=0) SysConfig.CheckOnlineTimeOut=500;

	m_nListDragIndex=-1; 
	m_pDragImage=NULL;
}

CRemoteManDlg::~CRemoteManDlg()
{
	//�������򿪵ķ���
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

//TVN_ENDLABELEDIT ɾ�����л᲻�����öϵ㣬����������


// CRemoteManDlg ��Ϣ�������
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
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(SysConfig.VNCType == VNC_TYPE_TIGHTVNC ? IDI_ICON_VNC : IDI_REALVNC));
	m_ToolbarImageList.Add(AfxGetApp()->LoadIcon(IDI_SET));

	UINT array[14]={ID_MENU_ADDGROUP,ID_MENU_DELGROUP,ID_SEPARATOR,
					ID_MENU_ADDHOST,ID_MENU_EDITHOST,ID_MENU_DELHOST,ID_SEPARATOR,
					ID_MENU_CONNENT,IDC_TOOLER_OPENMSTSC,IDC_TOOLER_OPENRADMIN,IDC_TOOLER_OPENSSH,IDC_TOOLER_OPENVNC,ID_SEPARATOR,
					IDC_TOOLER_SET};
	m_ToolBar.Create(this);
	m_ToolBar.SetButtons(array,14);
	m_ToolBar.SetButtonText(0,"��ӷ���");
	m_ToolBar.SetButtonText(1,"ɾ������");
	m_ToolBar.SetButtonText(3,"�������");
	m_ToolBar.SetButtonText(4,"�༭����");
	m_ToolBar.SetButtonText(5,"ɾ������");
	m_ToolBar.SetButtonText(7,"��������");
	m_ToolBar.SetButtonText(8,"Զ������");
	m_ToolBar.SetButtonText(9,"Radmin");
	m_ToolBar.SetButtonText(10,"SSH���");
	m_ToolBar.SetButtonText(11,"VNC");
	m_ToolBar.SetButtonText(13,"����");

	m_ToolBar.GetToolBarCtrl().SetImageList(&m_ToolbarImageList);
	m_ToolBar.SetSizes(CSize(70,56),CSize(32,32));

	m_ToolBar.MoveWindow(CRect(0,-1,820,62));	//�ƶ��������ڸ����ڵ�λ��
	m_ToolBar.ShowWindow(SW_SHOW);				//��ʾ������

	//ʹ��������ͷ
	//DWORD dwExStyle = TBSTYLE_EX_DRAWDDARROWS;
	//m_ToolBar.GetToolBarCtrl().SendMessage(TB_SETEXTENDEDSTYLE, 0, (LPARAM)dwExStyle);
	//DWORD dwStyle = m_ToolBar.GetButtonStyle(m_ToolBar.CommandToIndex(IDC_TOOLER_OPENSSH));
	//dwStyle |= TBSTYLE_DROPDOWN;
	//m_ToolBar.SetButtonStyle(m_ToolBar.CommandToIndex(IDC_TOOLER_OPENSSH), dwStyle);
	//dwStyle = m_ToolBar.GetButtonStyle(m_ToolBar.CommandToIndex(IDC_TOOLER_OPENVNC));
	//dwStyle |= TBSTYLE_DROPDOWN;
	//m_ToolBar.SetButtonStyle(m_ToolBar.CommandToIndex(IDC_TOOLER_OPENVNC), dwStyle);
//	RepositionBars(AFX_IDW_CONTROLBAR_FIRST,AFX_IDW_CONTROLBAR_LAST,0);
}

BOOL CRemoteManDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	HICON hico=(HICON)LoadImage(AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDI_MSTSC),IMAGE_ICON,64,64,LR_DEFAULTCOLOR);
	((CStatic*)GetDlgItem(IDC_STATIC_PIC))->SetIcon(hico);

	//���ó����־�����ڵ�ʵ������ʱ��ԭ����
	::SetProp(m_hWnd,AfxGetApp()->m_pszExeName,(HANDLE)1);

	InitToolBar();

	m_ImageList.Create(24,24,ILC_COLOR24|ILC_MASK,1,1);
	m_ImageList.SetBkColor(RGB(255,255,255));
	m_ImageList.Add(AfxGetApp()->LoadIcon(IDI_NODE_CLOSE));
	m_ImageList.Add(AfxGetApp()->LoadIcon(IDI_NODE_OPEN));
	m_ImageList.Add(AfxGetApp()->LoadIcon(IDR_MAINFRAME));
	m_ImageList.Add(AfxGetApp()->LoadIcon(IDI_RADMIN));
	m_ImageList.Add(AfxGetApp()->LoadIcon(IDI_SSH));
	m_ImageList.Add(AfxGetApp()->LoadIcon(SysConfig.VNCType == VNC_TYPE_TIGHTVNC ? IDI_ICON_VNC : IDI_REALVNC));
	m_Tree.SetImageList(&m_ImageList,LVSIL_NORMAL);

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
	m_List.SetImageList(&m_ImageList,LVSIL_SMALL);
	m_List.InsertColumn(0,"����",LVCFMT_LEFT,ListDefColumnWidth[0]);
	m_List.InsertColumn(1,"����������",LVCFMT_LEFT,ListDefColumnWidth[1]);
	m_List.InsertColumn(2,"����",LVCFMT_LEFT,ListDefColumnWidth[2]);
	m_List.InsertColumn(3,"�˿�",LVCFMT_LEFT,ListDefColumnWidth[3]);
	m_List.InsertColumn(4,"�˻�",LVCFMT_LEFT,ListDefColumnWidth[4]);
	m_List.InsertColumn(5,"״̬",LVCFMT_LEFT,ListDefColumnWidth[5]);

	((CButton*)GetDlgItem(IDC_CHECK_MST_SHOW_WALLPAPER))->SetCheck(SysConfig.MstscDeskImg);
	((CButton*)GetDlgItem(IDC_CHECK_MST_DRIVE))->SetCheck(SysConfig.MstscUseDrive);
	((CButton*)GetDlgItem(IDC_CHECK_MST_AUDIO))->SetCheck(SysConfig.MstscRemoteAudio);
	((CComboBox*)GetDlgItem(IDC_COMBO_MST_WINPOS))->SetCurSel(SysConfig.MstscWinpos);
	((CButton*)GetDlgItem(IDC_CHECK_RADMIN_FULLSCREEN))->SetCheck(SysConfig.RadminFullScreen);
	((CComboBox*)GetDlgItem(IDC_COMBO_RADMIN_CTRLMODE))->SetCurSel(SysConfig.RadminCtrlMode);

	//��ȡ����
	EnumTreeData(TVI_ROOT,0);
	if (m_Tree.GetCount()==0)
		SetDlgItemText(IDC_EDIT_README,"������ӷ���.");

	m_SearchEdit.SetDimText("������������������������");
	m_SearchEdit.SetDimColor(RGB(150,150,250));

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CRemoteManDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CRemoteManDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CRemoteManDlg::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
//	CDialogEx::OnOK();
} 



HBRUSH CRemoteManDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  �ڴ˸��� DC ���κ����� 
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
		SysConfig.VNCType = Dlg.m_VNCType;
		
		char sqlstr[1024];
		int n=sprintf_s(sqlstr,sizeof(sqlstr),"update ConfigTab set ParentShowHost=%s,RadminPath='%s',SSHPath='%s',WinScpPath='%s',VNCPath='%s',"
											  "SSHParamFormat='%s',VNCType=%d,CheckOnlineTimeOut=%d,MstscLocalDrive='%s',MstscColor=%d,"
											  "MstscConsole=%s,MstscFontSmooth=%s,MstscThemes=%s,RadminColor=%d where id=0;",
			SysConfig.ParentShowHost ? "true":"false",
			SysConfig.RadminPath,
			SysConfig.SSHPath,
			SysConfig.WinScpPath,
			SysConfig.VNCPath,
			SysConfig.SSHParamFormat,
			SysConfig.VNCType,
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
		//���ж�������������Ƿ���������Ƶķ������
		int GroupCnt=0;
		char sqlstr[128];
		sprintf_s(sqlstr,sizeof(sqlstr),"select count() from GroupTab where ParentId=%d and Name='%s';",ParentId,dlg.m_GroupName);
		TRACE("%s\r\n",sqlstr);		
		int rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), ReadIntCallback, &GroupCnt, NULL);
		if (GroupCnt>0)
		{
			MessageBox("�÷������Ѿ�����","����",MB_ICONERROR);
			return;
		}
		//��ӵ����ݿ�
		sprintf_s(sqlstr,sizeof(sqlstr),"insert into GroupTab values(NULL,'%s',%d);",dlg.m_GroupName,ParentId);
		TRACE("%s\r\n",sqlstr);
		rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), NULL, NULL, NULL);
		//��ȡ�������ݵ�ID
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
	//�����ӷ��鼰�ֵܷ���
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
	if (MessageBox("ɾ�����齫ͬʱɾ���ӷ��鼰��Ӧ������ȷ��ɾ����","ע��",MB_OKCANCEL|MB_ICONWARNING)!=IDOK) return;
	CArray<int ,int>GroupArray;
	GroupArray.SetSize(0,20);
	//ö����ID
	EnumChildGroupId(hItem,GroupArray);
	//ɾ����
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
	//ɾ������
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
	//ɾ�����ؼ��ڵ���б��
	SetDlgItemText(IDC_EDIT_README,"");
	m_List.DeleteAllItems();
	m_Tree.DeleteItem(hItem);
}

afx_msg LRESULT CRemoteManDlg::OnAddHostMessage(WPARAM wParam, LPARAM lParam)
{
	HOST_STRUCT *pHost = (HOST_STRUCT*)wParam;
	int ParentId=m_Tree.GetItemData(HTREEITEM(lParam));

	char sqlstr[512],str[68];
	//�鿴���������Ƿ����
	int HostCnt=0;
	sprintf_s(sqlstr,sizeof(sqlstr),"select count() from HostTab where ParentId=%d and Name='%s' and CtrlMode=%d;",
		ParentId, pHost->Name, pHost->CtrlMode);
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), ReadIntCallback, &HostCnt, NULL);
	if (HostCnt>0)
	{
		MessageBox("���������Ѵ���.","����",MB_ICONERROR);
		return 1;
	}
	//��ӵ����ݿ�
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
	//��ȡID
	int Id=0;
	sprintf_s(sqlstr,sizeof(sqlstr),"select id from HostTab where ParentId=%d and Name='%s';",ParentId,pHost->Name);
	TRACE("%s\r\n",sqlstr);
	rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), ReadIntCallback, &Id, NULL);
	//��ӵ��б�
	ListAddHost(pHost,Id);

	return 0;
}

void CRemoteManDlg::OnMenuClickedAddHost(void)
{
	HTREEITEM hItem=m_Tree.GetSelectedItem();
	if (hItem==NULL || m_Tree.GetCount()==0)
	{
		MessageBox("������ӷ����ѡ�����.","����",MB_ICONERROR);
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
	//�������Ƿ�Ҫ��ʾ����
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
	//�ٶ������˷��������ƣ��ȼ��������Ƿ����
	int HostCnt=0;
	sprintf_s(sqlstr,sizeof(sqlstr),"select count() from HostTab where ParentId=%d and Name='%s' and CtrlMode=%d and Id!=%d;",
		Host.ParentId, Dlg.m_Host.Name, Dlg.m_Host.CtrlMode,Host.Id);
	TRACE("%s\r\n",sqlstr);
	 rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), ReadIntCallback, &HostCnt, NULL);
	if (HostCnt>0)
	{
		MessageBox("���������Ѵ���.","����",MB_ICONERROR);
		return;
	}
	//�������ݿ�
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
	//�����б��
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
	//�г�ѡ�����,ɾ�����ݿ�
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
	//�Ӻ�ʼɾ���б�
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

#define TIGHTVNC_DEF_PATH	"TightVnc\\tvnviewer.exe"
#define REALVNC_DEF_PATH	"RealVNC\\vncviewer.exe"
#define RADMIN_DEF_PATH		"radmin.exe"
#define SSH_DEF_PATH		NULL
#define WINSCP_DEF_PATH		NULL
char const *GetExePath(char const *ConfigPath, char const *DefPath)
{
	char const *Path=ConfigPath[0]==0 ? DefPath:ConfigPath;			//��·��Ϊ��ʱʹ��ͬĿ¼�µ�tvnviewer.exe
	//�鿴�ļ��Ƿ����
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
		AfxMessageBox("Radmin·�����ô���");
		return;
	}
	WinExec(Path,SW_SHOW);
}

void CRemoteManDlg::OnToolbarClickedOpenSSH(void)
{
	CMenu Menu;
	Menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu *pSubMenu=Menu.GetSubMenu(7);
	CPoint point;
	GetCursorPos(&point);
	int id = pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL | TPM_RETURNCMD, point.x, point.y, this);

	char const *path = NULL;
	if (id == ID_MENU_OPENSSH)
	{
		path = GetExePath(SysConfig.SSHPath, SSH_DEF_PATH);
		if (path == NULL)
		{
			MessageBox("SSH·�����ô���");
			return;
		}
	}
	else if (id == ID_MENU_OPEN_WINSCP)
	{
		path = GetExePath(SysConfig.WinScpPath, SSH_DEF_PATH);
		if (path == NULL)
		{
			MessageBox("WinScp·�����ô���");
			return;
		}
	}
	if (path != NULL) WinExec(path, SW_SHOW);
}

void CRemoteManDlg::OnToolbarClickedOpenVNC(void)
{
	char const *VNCPath = GetExePath(SysConfig.VNCPath, SysConfig.VNCType == VNC_TYPE_REALVNC ? REALVNC_DEF_PATH : TIGHTVNC_DEF_PATH);
	if (VNCPath==NULL)
	{
		AfxMessageBox("VNC·�����ô���");
		return;
	}

	CMenu Menu;
	Menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu *pSubMenu = Menu.GetSubMenu(8);
	CPoint point;
	GetCursorPos(&point);
	int id = pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL | TPM_RETURNCMD, point.x, point.y, this);

	if (id == ID_MENU_OPEN_VNCVIEW)
	{
		WinExec(VNCPath, SW_SHOW);
	}
	else if (id == ID_MENU_OPEN_VNCLISTEN)
	{
		ShellExecute(NULL, "open", VNCPath, "-listen", NULL, SW_SHOWNORMAL);
	}

}

void CRemoteManDlg::MstscConnent(HOST_STRUCT const *pHost, CONFIG_STRUCT const *pConfig)
{
	char RdpStr[2048],str[768];
	//����ģʽ
	int len=sprintf_s(RdpStr,sizeof(RdpStr),"screen mode id:i:%d\r\n",pConfig->MstscWinpos==0?2:1);
	//���
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
	//��ɫλ��
	len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"session bpp:i:%d\r\n",pConfig->MstscColor==0 ? 16:pConfig->MstscColor==1 ? 24:32);
	//��ʾλ�ü����ڴ�С
	int ScreenWidth= GetSystemMetrics(SM_CXFULLSCREEN);
	int ScreenHeight= GetSystemMetrics(SM_CYFULLSCREEN);
	int xPos = (ScreenWidth-Width-40)/2, yPos=(ScreenHeight-Height-40)/2;
	if (xPos<0) xPos=0;
	if (yPos<0) yPos=0;
	len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"winposstr:s:0,1,%d,%d,%d,%d\r\n",xPos,yPos,xPos+Width+40,yPos+Height+60);
	//Զ�̷�������ַ
	if (pHost->HostPort==3389)
		len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"full address:s:%s\r\n",pHost->HostAddress);
	else
		len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"full address:s:%s:%d\r\n",pHost->HostAddress,pHost->HostPort);
	//�����ݴ��䵽�ͻ��˼����ʱ�Ƿ�����ݽ���ѹ��
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"compression:i:1\r\n");
	len+=sizeof("compression:i:1\r\n")-1;
	//ȷ����ʱ�� Windows �����Ӧ�õ��������ӵ�Զ�̻Ự
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"keyboardhook:i:2\r\n");
	len+=sizeof("keyboardhook:i:2\r\n")-1;
	//��������
	len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"audiomode:i:%d\r\n",pConfig->MstscRemoteAudio ? 0:2);
	//������ 
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"redirectclipboard:i:1\r\n");
	len+=sizeof("redirectclipboard:i:1\r\n")-1;
	//PnP���弴���豸
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"devicestoredirect:s:\r\n");
	len+=sizeof("devicestoredirect:s:\r\n")-1;
	//����������
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
	//�Ƿ��Զ����Ӵ�ӡ��
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"redirectprinters:i:0\r\n");
	len+=sizeof("redirectprinters:i:0\r\n")-1;
	//�Ƿ��Զ�����COM���п�
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"redirectcomports:i:0\r\n");
	len+=sizeof("redirectcomports:i:0\r\n")-1;
	//�Ƿ��Զ��������ܿ�
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"redirectsmartcards:i:0\r\n");
	len+=sizeof("redirectsmartcards:i:0\r\n")-1;
	//ȫ��ģʽʱ�Ƿ���ʾ������
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"displayconnectionbar:i:1\r\n");
	len+=sizeof("displayconnectionbar:i:1\r\n")-1;
	//�ڶϿ����Ӻ��Ƿ��Զ�������������
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"autoreconnection enabled:i:1\r\n");
	len+=sizeof("autoreconnection enabled:i:1\r\n")-1;
	//�û�������
	len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"username:s:%s\r\n",pHost->Account);
	//ָ��Ҫ��Զ�̻Ự����Ϊ shell����������Դ���������Զ������ĳ���
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"alternate shell:s:\r\n");
	len+=sizeof("alternate shell:s:\r\n")-1;
	////RDP��������ʱ�Զ�������Ӧ�ó������ڵ��ļ���λ�ó�
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"shell working directory:s:\r\n");
	len+=sizeof("shell working directory:s:\r\n")-1;
	//RDP�����������
	if (pHost->Password[0]!=0)
		len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"password 51:b:%s\r\n",CryptRDPPassword(pHost->Password,str,sizeof(str)));
	//��ֹ���������Զ����
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"networkautodetect:i:0\r\n");
	len+=sizeof("networkautodetect:i:0\r\n")-1;
	//ָ����������Ϊ������10M�����
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"connection type:i:6\r\n");
	len+=sizeof("connection type:i:6\r\n")-1;
	//�Ƿ��ֹ��ʾ���汳��
	len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"disable wallpaper:i:%d\r\n",pConfig->MstscDeskImg?0:1);
	//�Ƿ��ֹ����
	len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"disable themes:i:%d\r\n",pConfig->MstscThemes?0:1);
	//�Ƿ���������ƽ��
	len+=sprintf_s(RdpStr+len,sizeof(RdpStr)-len,"allow font smoothing:i:%d\r\n",pConfig->MstscFontSmooth?1:0);
	//���ļ����ϵ���λ��ʱ�Ƿ���ʾ�ļ�������
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"disable full window drag:i:1\r\n");
	len+=sizeof("disable full window drag:i:1\r\n")-1;
	//���ò˵�����
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"disable menu anims:i:1\r\n");
	len+=sizeof("disable menu anims:i:1\r\n")-1;
	//���ù������
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"disable cursor setting:i:0\r\n");
	len+=sizeof("disable cursor setting:i:0\r\n")-1;
	//�Ƿ�λͼ�����ڱ��ؼ������
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"bitmapcachepersistenable:i:1\r\n");
	len+=sizeof("bitmapcachepersistenable:i:1\r\n")-1;
	//��������������֤��������
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"authentication level:i:0\r\n");
	len+=sizeof("authentication level:i:0\r\n")-1;
	//?
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"prompt for credentials:i:0\r\n");
	len+=sizeof("prompt for credentials:i:0\r\n")-1;
	//ȷ���Ƿ񱣴��û���ƾ�ݲ��������� RD ���غ�Զ�̼����
	strcpy_s(RdpStr+len,sizeof(RdpStr)-len,"promptcredentialonce:i:1\r\n");
	len+=sizeof("promptcredentialonce:i:1\r\n")-1;

	//�洢���ļ�,����str����
	GetTempPath(sizeof(str),str);
	strcat_s(str,sizeof(str),pHost->Name);
//	sprintf_s(str,sizeof(str),"c:\\%s_RDP.rdp",pHost->HostAddress);
	CFile file;
	if (!file.Open(str,CFile::modeCreate|CFile::modeWrite|CFile::typeBinary)) return;
	file.Write(RdpStr,len);
	file.Close();

	//����
	char szBuffer[MAX_PATH];
	SHGetSpecialFolderPath(NULL, szBuffer, CSIDL_SYSTEM, FALSE);
	sprintf_s(RdpStr,sizeof(RdpStr),"%s\\Mstsc.exe /%s %s", szBuffer,pConfig->MstscConsole?"console":"admin",str);	//������
	WinExec(RdpStr, SW_SHOW);
	TRACE("Rdp�ļ�����=%d Rdp������:%s\r\n",len,RdpStr);
	Sleep(1000);
	DeleteFile(str);
}

void RadminConnent(HOST_STRUCT const *pHost, CONFIG_STRUCT const *pConfig, int CtrlMode)
{
	static const char MODE[][10]={"","/noinput","/file","/shutdown"};
	static const char COLOUR[][8]={"/8bpp","/16bpp","/24bpp"};
	//�鿴�ļ��Ƿ����
	char const *RadminPath=GetExePath(pConfig->RadminPath,RADMIN_DEF_PATH);
	if (RadminPath==NULL)
	{
		AfxMessageBox("Radmin·�����ô���");
		return;
	}
	char str1[100],str2[64];
	//����Radmin���ӷ�����
	if (pHost->HostPort==4899)
		sprintf_s(str1,sizeof(str1),"/connect:%s %s %s",pHost->HostAddress,MODE[CtrlMode],COLOUR[pConfig->RadminColor]);
	else
		sprintf_s(str1,sizeof(str1),"/connect:%s:%d %s %s",pHost->HostAddress,pHost->HostPort,MODE[CtrlMode],COLOUR[pConfig->RadminColor]);
	if (pConfig->RadminFullScreen)
		strcat_s(str1,sizeof(str1)," /fullscreen");
	TRACE("%s\r\n",str1);
	ShellExecute(NULL,"open",RadminPath,str1,NULL,SW_SHOW);
	//��������
	sprintf_s(str1,sizeof(str1),"Radmin ��ȫ�ԣ�%s",pHost->HostAddress);	//Radmin2.2�Ĵ�������
	sprintf_s(str2,sizeof(str2),"Radmin ��ȫ��: %s",pHost->HostAddress);	//Radmin3.4�Ĵ�������
	//����Radmin��������
	HWND hWnd;
	clock_t t2, t1 = clock();
	for (t2 = t1; t2 - t1 < 8000; t2 = clock())
	{
		Sleep(1);
		hWnd = FindWindow(NULL, str1);
		if (hWnd != NULL)
			break;
		hWnd = FindWindow(NULL, str2);
		if (hWnd != NULL)
			break;
	}
	if (hWnd == NULL)	return;
	//��д��Ϣ������
	HWND UserWnd=::GetDlgItem(hWnd,0x7ff);
	HWND PasswordWnd=::GetDlgItem(hWnd,0x800);
	if (UserWnd!=NULL)
		::SendMessage(UserWnd,WM_SETTEXT,0,(LPARAM)pHost->Account);
	if (PasswordWnd!=NULL)
		::SendMessage(PasswordWnd,WM_SETTEXT,0,(LPARAM)pHost->Password);
	::PostMessage(hWnd,WM_COMMAND,0x78,0);
}

struct EnumFindStr
{
	TCHAR const *str;
	UINT id;
	BOOL useNext;
	HWND hwnd;
};

BOOL CALLBACK EnumWindowFunc(HWND hwnd, LPARAM lvoid)
{
	EnumFindStr *pfs = (EnumFindStr*)lvoid;
	TCHAR windowtext[256];
	::GetWindowText(hwnd, windowtext, 256);
	TRACE(_T("hwnd=%08x\r\n"), hwnd);
	if (pfs->useNext == 2)
	{
		pfs->hwnd = hwnd;
		return 0;
	}
	else if (_tcscmp(windowtext, pfs->str) == 0)
	{
		if (pfs->useNext == 0)
		{
			pfs->hwnd = hwnd;
			return 0;
		}
		else
			pfs->useNext = 2;
	}
	return 1;
}

void VNCConnent(HOST_STRUCT const *pHost, CONFIG_STRUCT const *pConfig)
{
	char const *VNCPath = GetExePath(pConfig->VNCPath, pConfig->VNCType == VNC_TYPE_REALVNC ? REALVNC_DEF_PATH : TIGHTVNC_DEF_PATH);
	if (VNCPath==NULL)
	{
		AfxMessageBox("VNC·�����ô���");
		return;
	}
	char str[512], ConnectWndNameStr[256], hoststr[128];
	sprintf_s(hoststr, 128, "%s:%d", pHost->HostAddress, pHost->HostPort);
	//����VNC���ӷ�����
	if (pConfig->VNCType == VNC_TYPE_TIGHTVNC)
	{
		sprintf_s(str, sizeof(str), "%s %s -password=%s", VNCPath, hoststr, pHost->Password);
		TRACE("%s\r\n",str);
	//	AfxMessageBox(str);
		WinExec(str,SW_SHOW);
	}
	else
	{
		sprintf_s(str, sizeof(str), "%s %s", VNCPath, hoststr);
		TRACE("%s\r\n", str);
		ShellExecute(NULL, _T("open"), VNCPath, hoststr, NULL, SW_SHOWNORMAL);
		//�������Ӵ���
		sprintf_s(ConnectWndNameStr, sizeof(ConnectWndNameStr), "%s - RealVNC Viewer", hoststr);	//Radmin2.2�Ĵ�������
		HWND hMainWnd;
		clock_t t2, t1 = clock();
		for (t2 = t1; t2 - t1 < 2000; t2 = clock())
		{
			Sleep(1);
			hMainWnd = ::FindWindow(NULL, ConnectWndNameStr);
			if (hMainWnd != NULL)
			{
				TRACE(_T("�����Ӵ��ھ����%08X\r\n"), hMainWnd);
				break;
			}
		}
		//�����Ӵ��ڳ��ֺ󣬲��������֤���ڣ�ֱ�������Ӵ��ڱ��ر�
		while (hMainWnd != NULL)
		{
			//��ݳ�ͻ����
			HWND hWnd = ::FindWindow(NULL, _T("Identity Check"));
			if (hWnd != NULL)
			{
				TRACE(_T("��ݳ�ͻ���ھ����%08X\r\n"), hWnd);
				Sleep(50);
				//���Continue
				EnumFindStr fs = { _T("Continue"), 0x119, 0, NULL };
				::EnumChildWindows(hWnd, EnumWindowFunc, (LPARAM)&fs);
				if (fs.hwnd != NULL)
				{
					TRACE(_T("Continue��ť�����%08X\r\n"), fs.hwnd);
					::SendMessage(fs.hwnd, WM_LBUTTONDOWN, 0, MAKELONG(10, 10));
					::SendMessage(fs.hwnd, WM_LBUTTONUP, 0, MAKELONG(10, 10));
					Sleep(100);
				}
			}
			//�����֤����
			hWnd = ::FindWindow(NULL, _T("�����֤"));		//��ݳ�ͻ����
			if (hWnd != NULL)
			{
				TRACE(_T("�����֤���ھ����%08X\r\n"), hWnd);
				Sleep(50);
				//�����û��������룬���OK
				EnumFindStr fsuser = { _T("�û���:"), 0, 1, NULL };
				::EnumChildWindows(hWnd, EnumWindowFunc, (LPARAM)&fsuser);
				EnumFindStr fspass = { _T("����:"), 0, 1, NULL };
				::EnumChildWindows(hWnd, EnumWindowFunc, (LPARAM)&fspass);
				EnumFindStr fsok = { _T("OK"), 0, 0, NULL };
				::EnumChildWindows(hWnd, EnumWindowFunc, (LPARAM)&fsok);
				TRACE(_T("�û����ؼ������%08X������ؼ������%08X��OK��ť�����%08X\r\n"), fsuser.hwnd, fspass.hwnd, fsok.hwnd);
				if (fsuser.hwnd != NULL && fspass.hwnd != NULL && fsok.hwnd != NULL)
				{
					LONG style = GetWindowLong(fsuser.hwnd, GWL_STYLE);
					if (!(style & WS_DISABLED) && pHost->Account[0] != 0)
						::SendMessage(fsuser.hwnd, WM_SETTEXT, 0, (LPARAM)pHost->Account);
					::SendMessage(fspass.hwnd, WM_SETTEXT, 0, (LPARAM)pHost->Password);
					::SendMessage(fsok.hwnd, WM_LBUTTONDOWN, 0, MAKELONG(10, 10));
					::SendMessage(fsok.hwnd, WM_LBUTTONUP, 0, MAKELONG(10, 10));
					break;
				}
			}
			//��������Ӵ�����û���˳�
			Sleep(10);
			if (!::IsWindow(hMainWnd))	hMainWnd = NULL;
		}
		TRACE(_T("����\r\n"));
	}
}

void SSHConnent(HOST_STRUCT const *pHost, CONFIG_STRUCT const *pConfig)
{
	char str[MAX_PATH*2];
	//�鿴�ļ��Ƿ����
	char const *SSHPath = GetExePath(pConfig->SSHPath,SSH_DEF_PATH);
	if (SSHPath==NULL)
	{
		AfxMessageBox("SSH·�����ô���");
		return;
	}
	//������ʽ
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
			AfxMessageBox("SSH�����в�����ʽ���ô���.");
			return;
		}
	}

	//����
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
	//��ȡ������Ϣ
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
	//�鿴�ļ��Ƿ����
	char const *WinScpPath = GetExePath(SysConfig.WinScpPath,WINSCP_DEF_PATH);
	if (WinScpPath==NULL)
	{
		MessageBox("WinScp·�����ô���");
		return;
	}
	char str[MAX_PATH*2];
	HOST_STRUCT Host;
	if (!GetSelectHost(&Host)) return;
	
	//������
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
	//�ȼ��������Ƿ����
	int rc = sprintf_s(sqlstr, sizeof(sqlstr), "select count() from GroupTab where ParentId=%d and Name='%s' and Id!=%d;", ParentId, (char const*)Name, Id);
	TRACE("%s\r\n",sqlstr);
	rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), ReadIntCallback, &Cnt, NULL);
	if (Cnt!=0) 
	{
		MessageBox("�÷��������Ѿ�����","����",MB_ICONERROR);
		return;
	}
	//�������ݿ�
	sprintf_s(sqlstr,sizeof(sqlstr),"update GroupTab set Name='%s' where Id=%d", (char const*)Name, Id);
	TRACE("%s\r\n",sqlstr);
	rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), NULL, NULL, NULL);

	*pResult = 1;
}

void CRemoteManDlg::LoadHostList(HTREEITEM hItem)
{
	int rc;
	CArray<int ,int>GroupArray;
	GroupArray.SetSize(0,20);
	//ö����ID
	if (SysConfig.ParentShowHost)
		EnumChildGroupId(hItem,GroupArray);
	else
		GroupArray.Add(m_Tree.GetItemData(hItem));
	//����������
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
			return LRESULT("ԭ�������.");
	}

	strcpy_s(SysConfig.SysPassword,sizeof(SysConfig.SysPassword),New[0]==0 ? "":AesEnCodeToStr(New,strlen(New),sqlstr,AES_KEY));
	int n=sprintf_s(sqlstr,sizeof(sqlstr),"update ConfigTab set Password='%s' where id=0;",SysConfig.SysPassword);
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);

	return LRESULT("�����޸ĳɹ�.");
}

void CRemoteManDlg::OnBnClickedCheckMstShowWallpaper()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	SysConfig.MstscDeskImg=((CButton*)GetDlgItem(IDC_CHECK_MST_SHOW_WALLPAPER))->GetCheck()!=0;

	char sqlstr[128];
	int n=sprintf_s(sqlstr,sizeof(sqlstr),"update ConfigTab set MstscDeskImg=%s where id=0;",SysConfig.MstscDeskImg?"true":"false");
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
}

void CRemoteManDlg::OnBnClickedCheckMstDrive()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	SysConfig.MstscUseDrive=((CButton*)GetDlgItem(IDC_CHECK_MST_DRIVE))->GetCheck()!=0;

	char sqlstr[128];
	int n=sprintf_s(sqlstr,sizeof(sqlstr),"update ConfigTab set MstscUseDrive=%s where id=0;",SysConfig.MstscUseDrive?"true":"false");
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
}

void CRemoteManDlg::OnBnClickedCheckMstAudio()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	SysConfig.MstscRemoteAudio=((CButton*)GetDlgItem(IDC_CHECK_MST_AUDIO))->GetCheck()!=0;

	char sqlstr[128];
	int n=sprintf_s(sqlstr,sizeof(sqlstr),"update ConfigTab set MstscRemoteAudio=%s where id=0;",SysConfig.MstscRemoteAudio?"true":"false");
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
}

void CRemoteManDlg::OnCbnSelchangeComboMstWinpos()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	SysConfig.MstscWinpos=((CComboBox*)GetDlgItem(IDC_COMBO_MST_WINPOS))->GetCurSel();

	char sqlstr[128];
	int n=sprintf_s(sqlstr,sizeof(sqlstr),"update ConfigTab set MstscWinpos=%d where id=0;",SysConfig.MstscWinpos);
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
}

void CRemoteManDlg::OnBnClickedCheckRadminFullscreen()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	SysConfig.RadminFullScreen=((CButton*)GetDlgItem(IDC_CHECK_RADMIN_FULLSCREEN))->GetCheck()!=0;

	char sqlstr[128];
	int n=sprintf_s(sqlstr,sizeof(sqlstr),"update ConfigTab set RadminFullScreen=%s where id=0;",SysConfig.RadminFullScreen?"true":"false");
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
}


void CRemoteManDlg::OnCbnSelchangeComboRadminCtrlmode()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	SysConfig.RadminCtrlMode=((CComboBox*)GetDlgItem(IDC_COMBO_RADMIN_CTRLMODE))->GetCurSel();

	char sqlstr[128];
	int n=sprintf_s(sqlstr,sizeof(sqlstr),"update ConfigTab set RadminCtrlMode=%d where id=0;",SysConfig.RadminCtrlMode);
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
}


void CRemoteManDlg::OnNMRClickTree1(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	OnMenuClickedConnentHost();
	*pResult = 0;
}


void CRemoteManDlg::OnLvnBegindragList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
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
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	if (m_nListDragIndex==-1) 
	{
		CDialogEx::OnLButtonUp(nFlags, point);
		return;
	}
	//�ͷ���Դ
	ReleaseCapture(); 
	m_pDragImage->DragLeave(GetDesktopWindow()); 
	m_pDragImage->EndDrag(); 
	delete m_pDragImage; 
	m_nListDragIndex=-1;
	//�ж����봦�Ƿ������ؼ��ı�ǩ��
	CPoint pt(point); 
	ClientToScreen(&pt); 
	CWnd *pWnd = WindowFromPoint(pt); 
	if (pWnd==&m_List)
	{
		//�϶�����λ��m, �϶���n
		m_List.ScreenToClient(&pt);
		int m=m_List.HitTest(pt,&nFlags);
		if (m==-1) m=m_List.GetItemCount()-1;
		if (m_List.GetSelectedCount()!=1)
		{
			MessageBox("��ǰֻ֧�ֵ����϶�����","����",MB_ICONERROR);
			return;
		}
		int n=m_List.GetSelectionMark();
		if (n==m) return;
		TRACE("��%d�϶���%d\r\n",n,m);
		//�ȶ�ȡ��ѡ�е�����
		int nId=m_List.GetItemData(n);
		CArray<HOST_STRUCT,HOST_STRUCT&>HostArray;
		char sqlstr[128];
		int rc=sprintf_s(sqlstr,sizeof(sqlstr),"select * from HostTab where id=%d;",nId);
		TRACE("%s\r\n",sqlstr);
		rc = sqlite3_exec(m_pDB, sqlstr, ReadHostCallBack, &HostArray, NULL);
		if (HostArray.GetSize()!=1) return;
		//��ȡ����ID����ʱIDʹ��MaxID+1
		int MaxId=0;
		strcpy_s(sqlstr,sizeof(sqlstr),"select max(id) from HostTab;");
		TRACE("%s\r\n",sqlstr);
		rc = sqlite3_exec(m_pDB, sqlstr, ReadIntCallback, &MaxId, NULL);
		//�������ݿ�
		//�Ƚ��϶�����������Ϊһ����ʱID
		rc = sprintf_s(sqlstr,sizeof(sqlstr),"update HostTab set id=%d where id=%d",MaxId+1,nId);
		TRACE("%s\r\n",sqlstr);
		rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
		//ѭ������
		for (int k=n; k!=m;)
		{
			k+=n>=m ? -1:1;		//��һ���ķ���
			int tmpId=m_List.GetItemData(k);
			m_List.SetItemData(k,nId);
			rc = sprintf_s(sqlstr,sizeof(sqlstr),"update HostTab set id=%d where id=%d",nId,tmpId);
			TRACE("%s\r\n",sqlstr);
			rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
			nId=tmpId;
		}
		//��ԭ�϶����ID
		rc = sprintf_s(sqlstr,sizeof(sqlstr),"update HostTab set id=%d where id=%d",nId,MaxId+1);
		TRACE("%s\r\n",sqlstr);
		rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
		//�б��ɾ�����������
		m_List.DeleteItem(n);
		ListAddHost(&HostArray[0],nId,m);
	}
	else if (pWnd==&m_Tree) 
	{
		m_Tree.ScreenToClient(&pt);
		HTREEITEM hItem=m_Tree.HitTest(pt, &nFlags); //���ڻ�ȡ���봦��Item
		if (hItem == NULL)  return;

		int Cnt=m_List.GetSelectedCount();
		if (Cnt==0) return;
		//�������ݿ�
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
		//����ѡ����hItem
		m_Tree.SelectItem(hItem);
	}
}

void CRemoteManDlg::OnMenuClickedExportGroup(void)
{
	HTREEITEM hItem = m_Tree.GetSelectedItem();
	if (hItem==NULL) return;
	CFileDialog fdlg(FALSE,".db","ExportGroup",6,"*.db|*.db||");
	if (fdlg.DoModal()!=IDOK) return;
	//�������ݿ�
	char sqlstr[1536];
	sprintf_s(sqlstr, sizeof(sqlstr), "attach database '%s' as 'export';", (char const*)fdlg.GetPathName());
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), NULL, NULL, NULL);
	if (rc!=0)
	{
		MessageBox("�����������.","����",MB_ICONERROR);
		return;
	}
	//��ӱ��
	rc=sprintf_s(sqlstr,sizeof(sqlstr),InitTabSqlStr,"export.","export.","export.","export.");	//Ҫ����4�����ݿ�����
	TRACE("%s\r\n",sqlstr);
	rc=sqlite3_exec(m_pDB,sqlstr,NULL,NULL,NULL);
	if (rc!=0)
	{
		MessageBox("�����������.","����",MB_ICONERROR);
		return;
	}
	//ö����ID	
	CArray<int ,int>GroupArray;
	GroupArray.SetSize(0,20);
	//ö����ID
	EnumChildGroupId(hItem,GroupArray);
	//��������
	int sqlstrlen=58+23*GroupArray.GetSize(),len;
	char *sqlstr1=new char[sqlstrlen];
	len=sprintf_s(sqlstr1,sqlstrlen,"insert into export.GroupTab select * from GroupTab where ");
	for (int i=0; i<GroupArray.GetSize(); i++)
		len+=sprintf_s(sqlstr1+len,sqlstrlen-len,i==0?"id=%d":" or id=%d",GroupArray[i]);
	sqlstr1[len++]=';';
	sqlstr1[len]=0;
	TRACE("%s\r\n",sqlstr1);
	rc = sqlite3_exec(m_pDB, sqlstr1, NULL, NULL, NULL);
	//�����׷��鸸IDΪ0
	sprintf_s(sqlstr,sizeof(sqlstr),"update export.GroupTab set ParentId=0 where Id=%d;",GroupArray[0]);
	TRACE("%s\r\n",sqlstr);
	rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
	//��������
	len=sprintf_s(sqlstr1,sqlstrlen,"insert into export.HostTab select * from HostTab where ");
	for (int i=0; i<GroupArray.GetSize(); i++)
		len+=sprintf_s(sqlstr1+len,sqlstrlen-len,i==0?"ParentId=%d":" or ParentId=%d",GroupArray[i]);
	sqlstr1[len++]=';';
	sqlstr1[len]=0;
	TRACE("%s\r\n",sqlstr1);
	rc = sqlite3_exec(m_pDB, sqlstr1, NULL, NULL, NULL);
	//�������ݿ�
	strcpy_s(sqlstr,sizeof(sqlstr),"detach database 'export';");
	TRACE("%s\r\n",sqlstr);
	rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
	//
	delete sqlstr1;
}

/*
ImportParentId: ���뵽�����ݿ�ĸ�����ID
ExportParentId�������ĸ������ݿ�ķ���ID
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
	//���÷����µ��ӷ�����ӵ����ݿ�����ؼ�����ɨ����ӷ���
	for (int i=0; i<GroupArray.GetSize(); i++)
	{
		GROUP_STRUCT g=GroupArray[i];
		//��ӵ����ݿ�,��Ҫ���˷������Ƿ����
		int Id=0;
		HTREEITEM hChildItem=m_Tree.GetChildItem(hItem);
		while (hChildItem)
		{
			if (strcmp(m_Tree.GetItemText(hChildItem),g.Name)==0)
				break;
			hChildItem=m_Tree.GetNextSiblingItem(hChildItem);
		}
		//�������
		if (hChildItem!=NULL)
			Id=m_Tree.GetItemData(hChildItem);
		//û����ʱ����ӵ����ݿ⣬���ض�ID����ӵ����ؼ�
		else
		{
			Id=hItem==NULL || hItem==TVI_ROOT ? 0: m_Tree.GetItemData(hItem);		//����Ҫ����ĸ�ID
			rc=sprintf_s(sqlstr,sizeof(sqlstr),"insert into GroupTab values(NULL,'%s',%d);",g.Name,Id);
			TRACE("%s\r\n",sqlstr);
			rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), NULL, NULL, NULL);
			rc=sprintf_s(sqlstr,sizeof(sqlstr),"select Id from GroupTab where ParentId=%d and Name='%s';",Id,g.Name);
			TRACE("%s\r\n",sqlstr);
			rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), ReadIntCallback, &Id, NULL);	//���ǵ������ID
			hChildItem=m_Tree.InsertItem(g.Name,0,1,hItem);
			m_Tree.SetItemData(hChildItem, Id);
		}
		//���÷����µ�������ӵ����ݿ�
		CArray<HOST_STRUCT,HOST_STRUCT&>HostArray;
		rc=sprintf_s(sqlstr,sizeof(sqlstr),"select * from export.HostTab where ParentId=%d;",g.Id);
		TRACE("%s\r\n",sqlstr);
		rc = sqlite3_exec(m_pDB, sqlstr, ReadHostCallBack, &HostArray, NULL);
		for (int i=0; i<HostArray.GetSize(); i++)
		{
			HOST_STRUCT Host=HostArray[i];
			//�鿴���������Ƿ����
			int Cnt;
			rc=sprintf_s(sqlstr,sizeof(sqlstr),"select count() from HostTab where ParentId=%d and Name='%s' and CtrlMode=%d;",
				Id, Host.Name, Host.CtrlMode);
			TRACE("%s\r\n",sqlstr);
			rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), ReadIntCallback, &Cnt, NULL);
			//��ӵ����ݿ�
			if (Cnt==0)
			{
				rc=sprintf_s(sqlstr,sizeof(sqlstr),"insert into HostTab values(NULL,'%s',%d,%d,'%s',%d,'%s','%s','%s');",
					Host.Name, Id, Host.CtrlMode, Host.HostAddress, Host.HostPort, Host.Account, Host.Password, Host.ReadMe);
				TRACE("%s\r\n",sqlstr);
				rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), NULL, NULL, NULL);
			}
		}
		//ö���ӷ���
		ImportGroup(hChildItem,g.Id);
	}
}

void CRemoteManDlg::OnMenuClickedImportGroup(void)
{
	char sqlstr[512];
	//��ȡҪ����ķ���
	HTREEITEM hItem=m_Tree.GetSelectedItem();
	//ѡ������ļ�
	CFileDialogEx fdlg(TRUE,".db","",6,"*.db|*.db||");
	fdlg.m_ofn.lpstrTitle="ѡ����ķ��������";
	if (hItem!=NULL)
		fdlg.SetGroupName(m_Tree.GetItemText(hItem));
	if (fdlg.DoModal()!=IDOK) return;
	if (fdlg.m_GroupSel==0) hItem=TVI_ROOT;
	//�������ݿ�
	sprintf_s(sqlstr, sizeof(sqlstr), "attach database '%s' as 'export';", (char const*)fdlg.GetPathName());
	TRACE("%s\r\n",sqlstr);
	int rc = sqlite3_exec(m_pDB, CodeConverter::AsciiToUtf8(sqlstr).c_str(), NULL, NULL, NULL);
	if (rc!=0)
	{
		MessageBox("����������.","����",MB_ICONERROR);
		return;
	}
	//��������
	ImportGroup(hItem,0);
	//�������ݿ�
	strcpy_s(sqlstr,sizeof(sqlstr),"detach database 'export';");
	TRACE("%s\r\n",sqlstr);
	rc = sqlite3_exec(m_pDB, sqlstr, NULL, NULL, NULL);
	//�����б�
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

	struct addrinfo *presult = NULL;
	struct addrinfo *ptr = NULL;
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	int resval = getaddrinfo(Address, "", &hints, &presult);
	if (resval != 0 || presult->ai_family != AF_INET)
	{
		freeaddrinfo(presult);
		return FALSE;
	}
	serveraddr = *(struct sockaddr_in *) ptr->ai_addr;
	serveraddr.sin_port=htons(Port);
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
	timeout.tv_sec = 0;				//��ʱ ��
	timeout.tv_usec =TimeOut*1000;	//��ʱ ΢��
	ret = select(0, 0, &r, 0, &timeout);
	::closesocket(m_socket);
	return ret > 0;
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
			pDlg->m_List.SetItemText(i,5,"��");
		else
			pDlg->m_List.SetItemText(i,5,"��");
	}

	pDlg->m_Tree.EnableWindow(TRUE);
	pDlg->SetDlgItemText(IDC_BTN_CHECK_ONLINE,"���߼��");
	return 0;
}

void CRemoteManDlg::OnBnClickedBtnCheckOnline()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	char str[10];
	GetDlgItemText(IDC_BTN_CHECK_ONLINE,str,10);
	if (strcmp(str,"ֹͣ���")==0)
	{
		bScanExit=true;
	}
	else
	{
		int Cnt=m_List.GetItemCount();
		if (Cnt==0) return;
		bScanExit=false;
		m_Tree.EnableWindow(FALSE);
		SetDlgItemText(IDC_BTN_CHECK_ONLINE,"ֹͣ���");
		CreateThread(NULL,0,ScanOnlineThread,this,0,NULL);
	}
}


void CRemoteManDlg::OnBnClickedBtnSearch()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
	//�Ҳ�ؼ�λ�ñ�ֻҪ�ı�����λ�ü���
	static int const RIGHT_CTRL_IDS[]={
		IDC_STATIC_PIC,IDC_BTN_CHECK_ONLINE,
		IDC_STATIC_GROUP1,IDC_CHECK_MST_SHOW_WALLPAPER,IDC_CHECK_MST_DRIVE,IDC_CHECK_MST_AUDIO,
		IDC_STATIC1,IDC_COMBO_MST_WINPOS,IDC_STATIC_GROUP2,IDC_CHECK_RADMIN_FULLSCREEN,
		IDC_STATIC2,IDC_COMBO_RADMIN_CTRLMODE};
	static int Mincx,Mincy;
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: �ڴ˴������Ϣ����������
	TRACE("nType=%d,cx=%d,cy=%d\r\n",nType,cx,cy);
	if (nType==1) return;		//��С��
	//����С����С�ߴ�
	if (Mincx==0) 
	{
		Mincx=cx,Mincy=cy;
		return;				//��һ�ν����ӿؼ���û������
	}
	if (cx<Mincx) cx=Mincx;
	if (cy<Mincy) cy=Mincy;
	//�ƶ��Ҳ�ؼ�λ��
	CRect rt;
	int offset=INT_MAX;
	for (int i=0; i<sizeof(RIGHT_CTRL_IDS)/sizeof(RIGHT_CTRL_IDS[0]); i++)
	{
		CWnd *p=GetDlgItem(RIGHT_CTRL_IDS[i]);
		p->GetWindowRect(rt);
		ScreenToClient(rt);
		if (offset==INT_MAX) offset=cx-96-rt.left;	//��һ���ؼ���λ������ұ�Ϊ96
	//	TRACE("Top=%d,Bottom=%d,Left=%d,Right=%d\r\n",rt.top,rt.bottom,rt.left,rt.right);
		rt.OffsetRect(offset,0);
		p->MoveWindow(rt);
	}
	//�ƶ����ؼ����б��,������ȵ�ԭʼ����Ϊ��184/597
	//��߽翪ʼΪ4���м��5���ұ߽�CX-139,�ײ���3
	GetDlgItem(IDC_TREE1)->GetWindowRect(rt);
	ScreenToClient(rt);
	int TreeWidth=(cx-148)*184/(184+597);
	rt.right=rt.left+TreeWidth;
	rt.bottom=cy-3;
	GetDlgItem(IDC_TREE1)->MoveWindow(rt);
	//�б��,�ײ���136
	rt.left=rt.right+5;
	rt.right=cx-139;
	rt.bottom=cy-136;
	GetDlgItem(IDC_LIST1)->MoveWindow(rt);
	int ListWidth=rt.right-rt.left-25;		//25:���������
	//�����п�
	int Sum=0;
	for (int i=0; i<sizeof(ListDefColumnWidth)/sizeof(ListDefColumnWidth[0]); i++)
		Sum+=ListDefColumnWidth[i];
	for (int i=0; i<sizeof(ListDefColumnWidth)/sizeof(ListDefColumnWidth[0]); i++)
		m_List.SetColumnWidth(i,ListWidth*ListDefColumnWidth[i]/Sum);

	//���������������ؼ�,�ײ�ƫ�104, �ϲ��գ�7, �༭���ұ߿�:109
	rt.right-=109;
	rt.top=rt.bottom+7;
	rt.bottom=rt.top+25;		//�߶�Ϊ25
	m_SearchEdit.MoveWindow(rt);
	//��ť
	rt.left=rt.right+12;
	rt.right=rt.left+88;
	GetDlgItem(IDC_BTN_SEARCH)->MoveWindow(rt);

	//����˵������ؼ�,
	rt.left=4+TreeWidth+5;
	rt.right=cx-139;
	rt.top=cy-98;
	rt.bottom=cy-3;
	GetDlgItem(IDC_STATIC_GROUP3)->MoveWindow(rt);
	//����˵����EDIT�ؼ�
	rt.left+=12;
	rt.right-=13;
	rt.top+=23;
	rt.bottom-=12;
	GetDlgItem(IDC_EDIT_README)->MoveWindow(rt);
	//
	Invalidate();		//�в�ӰҪ�ػ�
}


void CRemoteManDlg::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
//	CDialogEx::OnSettingChange(uFlags, lpszSection); //����ϵͳ���øı�ʱ�������Ĵ�С���ı�

	// TODO: �ڴ˴������Ϣ����������
}


void CRemoteManDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
	::RemoveProp(m_hWnd,AfxGetApp()->m_pszExeName);
	// TODO: �ڴ˴������Ϣ����������
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
