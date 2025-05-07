#pragma once


// CSysSetDlg �Ի���
#define WM_MODIFY_PASSWORD_MESSAGE		(WM_USER+1)

struct CONFIG_STRUCT{
	int  DatabaseVer;
	int  GroupLastSelId;				//�ϴιػ�ʱ�򿪵���ID
	char SysPassword[66];				//ϵͳ���룬ʹ��AES�������31�ֽ�����
	bool ParentShowHost;				//�������Ƿ���ʾ�ӷ��������
	char RadminPath[256];				//RADMIN·�������Ϊ�գ���ΪͬĿ¼�µ�radmin.exe
	char SSHPath[256];					//SSH·��
	char WinScpPath[256];				//WinScp��·��
	char VNCPath[256];					//VNC·�������Ϊ�գ���ΪͬĿ¼�µ�VNC.exe
	char SSHParamFormat[64];			//SSH�����еĲ�����ʽ��%1:��ַ %2:�˿� %3:�ʻ� %4:����,���Ϊ�գ������ļ����Զ�ѡ��
	int  VNCType;						//VNC���ͣ�0��RealVNC, 1��TightVNC
	int  CheckOnlineTimeOut;			//���߼�ⳬʱʱ��ms
	bool MstscConsole;					//Զ������ʹ��Console����
	bool MstscUseDrive;					//�Ƿ����ӱ��ط���
	char MstscLocalDrive[24];			//Զ������ӳ�䱾�ط�������ʽ:CDEF
	bool MstscRemoteAudio;				//Զ������ʹ��Զ����Ƶ
	int  MstscColor;					//Զ��������ɫ
	int  MstscWinpos;					//Զ������ֱ���
	bool MstscDeskImg;					//Զ������ʹ�����汳��
	bool MstscFontSmooth;				//Զ������ʹ������ƽ��
	bool MstscThemes;					//Զ�������Ӿ���ʽ
	int  RadminCtrlMode;				//RADMIN����ģʽ
	bool RadminFullScreen;				//RADMINʹ��ȫ������
	int  RadminColor;					//RADMIN��ɫ
};


class CSysSetDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSysSetDlg)

public:
	CSysSetDlg(CONFIG_STRUCT const *pConfig, CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CSysSetDlg();

// �Ի�������
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

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
