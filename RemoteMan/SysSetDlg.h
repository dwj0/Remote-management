#pragma once


// CSysSetDlg 对话框
#define WM_MODIFY_PASSWORD_MESSAGE		(WM_USER+1)

struct CONFIG_STRUCT{
	int  DatabaseVer;
	int  GroupLastSelId;				//上次关机时打开的组ID
	char SysPassword[66];				//系统密码，使用AES保存最大31字节密码
	bool ParentShowHost;				//父分组是否显示子分组的主机
	char RadminPath[256];				//RADMIN路径，如果为空，则为同目录下的radmin.exe
	char SSHPath[256];					//SSH路径
	char WinScpPath[256];				//WinScp的路径
	char VNCPath[256];					//VNC路径，如果为空，则为同目录下的VNC.exe
	char SSHParamFormat[64];			//SSH命令行的参数格式，%1:地址 %2:端口 %3:帐户 %4:密码,如果为空，根据文件名自动选择
	int  VNCType;						//VNC类型，0：RealVNC, 1：TightVNC
	int  CheckOnlineTimeOut;			//在线检测超时时间ms
	bool MstscConsole;					//远程桌面使用Console连接
	bool MstscUseDrive;					//是否连接本地分区
	char MstscLocalDrive[24];			//远程桌面映射本地分区，格式:CDEF
	bool MstscRemoteAudio;				//远程桌面使用远程音频
	int  MstscColor;					//远程桌面颜色
	int  MstscWinpos;					//远程桌面分辨率
	bool MstscDeskImg;					//远程桌面使用桌面背景
	bool MstscFontSmooth;				//远程桌面使用字体平滑
	bool MstscThemes;					//远程桌面视觉样式
	int  RadminCtrlMode;				//RADMIN控制模式
	bool RadminFullScreen;				//RADMIN使用全屏控制
	int  RadminColor;					//RADMIN颜色
};


class CSysSetDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSysSetDlg)

public:
	CSysSetDlg(CONFIG_STRUCT const *pConfig, CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSysSetDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CButton  DriveCheck[16];
	BOOL m_ParentShowHost;
	CString m_MstDriveStr;
	int m_MstColor;
	BOOL m_MstConsole;
	BOOL m_MstFontSmooth;
	BOOL m_MstThemes;
	int m_RadminColor;
	CString m_RadminPath;
	CString m_SshPath;
	CString m_SrcPassword;
	int m_TimeOut;
	afx_msg void OnBnClickedBtnChangePassword();
	afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();
	void OnBnClickedSelPath(UINT Id);
	CString m_VNCPath;
	CString m_SSHFormat;
	afx_msg void OnBnClickedBtnSshFormatHelp();
	CString m_WinScpPath;
	int m_VNCType;
};
