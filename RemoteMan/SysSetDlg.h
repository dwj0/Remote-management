#pragma once


// CSysSetDlg 对话框
#define WM_MODIFY_PASSWORD_MESSAGE		(WM_USER+1)


class CSysSetDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSysSetDlg)

public:
	CSysSetDlg(	bool ParentShowHost,
				char const *MstDriveStr,
				int  MstColor,
				BOOL MstShowDeskImg,
				BOOL MstFontSmooth,
				BOOL MstThemes,
				int  RadminColor,
				char const *RadminPath,
				char const *SshPath,
				char const *VNCPath,
				int TimeOut,
				CWnd* pParent = NULL);   // 标准构造函数
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
	BOOL m_MstShowDeskImg;
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
};
