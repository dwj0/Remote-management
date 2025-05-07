
// RemoteManDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "../sqlite3/sqlite3.h"
#include "SysSetDlg.h"
#include "AddGroupDlg.h"
#include "AddHostDlg.h"
#include "dimedit.h"

struct GROUP_STRUCT 
{
	int		Id;
	char	Name[64];
	int		Parent;
};

enum { VNC_TYPE_REALVNC = 0, VNC_TYPE_TIGHTVNC };

// CRemoteManDlg 对话框
class CRemoteManDlg : public CDialogEx
{
//工具栏ID号
	enum { IDC_TOOLER_OPENRADMIN = 10001, IDC_TOOLER_OPENMSTSC, IDC_TOOLER_OPENSSH, IDC_TOOLER_OPENVNC, IDC_TOOLER_SET };
// 构造
public:
	CRemoteManDlg(CWnd* pParent = NULL);	// 标准构造函数
	virtual ~CRemoteManDlg();

// 对话框数据
	enum { IDD = IDD_REMOTEMAN_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:    
	int m_nListDragIndex; 
	CImageList *m_pDragImage;

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
	CONFIG_STRUCT SysConfig;
	sqlite3	*m_pDB;
	afx_msg void OnBnClickedOk();
	void InitToolBar(void);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	CListCtrl m_List;
	void OnToolbarClickedSysSet(void);
	bool OpenUserDb(char const *DbPath);
	CTreeCtrl m_Tree;
	void EnumTreeData(HTREEITEM hItem, int ParentNode);
	afx_msg void OnTvnSelchangedTree1(NMHDR *pNMHDR, LRESULT *pResult);
	void LoadHostList(HTREEITEM hItem);
	afx_msg void OnLvnItemchangedList1(NMHDR *pNMHDR, LRESULT *pResult);
protected:
	afx_msg LRESULT OnModifyPasswordMessage(WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnBnClickedCheckMstShowWallpaper();
	afx_msg void OnBnClickedCheckMstDrive();
	afx_msg void OnBnClickedCheckMstAudio();
	afx_msg void OnCbnSelchangeComboMstWinpos();
	afx_msg void OnBnClickedCheckRadminFullscreen();
	afx_msg void OnCbnSelchangeComboRadminCtrlmode();
	void OnMenuClickedAddGroup(void);
	void OnMenuClickedDelGroup(void);
	void OnMenuClickedAddHost(void);
	void OnMenuClickedEditHost(void);
	void OnMenuClickedDelHost(void);
	void OnToolbarClickedOpenMstsc(void);
	void OnToolbarClickedOpenRadmin(void);
	void OnToolbarClickedOpenSSH(void);
	void OnMenuClickedConnentHost(void);
	void OnMenuClickedWinScpConnent(void);
	void EnumChildGroupId(HTREEITEM hItem,CArray<int ,int>&GroupArray);
protected:
	afx_msg LRESULT OnAddHostMessage(WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnNMRClickTree1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult);

	void MstscConnent(HOST_STRUCT const *pHost, CONFIG_STRUCT const *pConfig);
	afx_msg void OnNMDblclkList1(NMHDR *pNMHDR, LRESULT *pResult);
	void OnMenuClickedRadminCtrl(UINT Id);
	void ConnentHost(int RadminCtrlMode);
	afx_msg void OnLvnBegindragList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	void OnMenuClickedRenameGroup(void);
	afx_msg void OnTvnEndlabeleditTree1(NMHDR *pNMHDR, LRESULT *pResult);
	void ListAddHost(HOST_STRUCT const * pHost, int Id, int nItem=-1);
	void OnMenuClickedExportGroup(void);
	void OnMenuClickedImportGroup(void);
	void ImportGroup(HTREEITEM hItem, int ExportId);
	afx_msg void OnBnClickedBtnCheckOnline();
	afx_msg void OnBnClickedBtnSearch();
	void DataBaseConversion(int Ver);
	CDimEdit m_SearchEdit;
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	bool GetSelectHost(HOST_STRUCT *pHost);
	bool bScanExit;
	void OnToolbarClickedOpenVNC(void);
	afx_msg void OnDestroy();
	void OnMenuClickedPing(void);
};
