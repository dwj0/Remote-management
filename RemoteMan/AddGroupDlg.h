#pragma once


// CAddGroupDlg 对话框

class CAddGroupDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CAddGroupDlg)

public:
	bool m_AddRoot;
	char m_GroupName[64];
	CAddGroupDlg(char const *GroupName, CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CAddGroupDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG2 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
};
