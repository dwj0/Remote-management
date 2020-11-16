
// RemoteManDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"

//工具栏ID号
enum {IDC_TOOLER_NEW=10001,IDC_TOOLER_EDIT,IDC_TOOLER_DEL,IDC_TOOLER_ADDGROUP,IDC_TOOLER_ADDROOTGROUP,IDC_TOOLER_DELGROUP,
	IDC_TOOLER_OPEN,IDC_TOOLER_RADMIN,IDC_TOOLER_MSTSC, IDC_TOOLER_SSH, IDC_TOOLER_SET};

// CRemoteManDlg 对话框
class CRemoteManDlg : public CDialogEx
{
// 构造
public:
	CRemoteManDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_REMOTEMAN_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON			m_hIcon;
	CToolBar		m_ToolBar;
	CImageList		m_ImageList,m_ToolbarImageList;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	void InitToolBar(void);
	CStatic m_Ico;
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	CListCtrl m_List;
};
