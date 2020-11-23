#pragma once


// CInputPasswordDlg 对话框

class CInputPasswordDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CInputPasswordDlg)

public:
	CInputPasswordDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CInputPasswordDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG4 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	CString m_Password;
};
