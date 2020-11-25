
// RemoteManDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "sqlite3.h"
#include "SysSetDlg.h"
#include "AddGroupDlg.h"
#include "AddHostDlg.h"

struct CONFIG_STRUCT{
	int  DatabaseVer;
	int  GroupLastSelId;				//上次关机时打开的组ID
	char SysPassword[66];				//系统密码，使用AES保存最大31字节密码
	bool ParentShowHost;				//父分组是否显示子分组的主机
	char RadminPath[256];				//RADMIN路径，如果为空，则为同目录下的radmin.exe
	char SSHPath[256];					//SSH路径，如果为空，则为同目录下的SecureCRT.exe
	char VNCPath[256];					//VNC路径
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

struct GROUP_STRUCT 
{
	int		Id;
	char	Name[64];
	int		Parent;
};

// CRemoteManDlg 对话框
class CRemoteManDlg : public CDialogEx
{
//工具栏ID号
enum {IDC_TOOLER_OPENRADMIN=10001,IDC_TOOLER_OPENMSTSC, IDC_TOOLER_OPENSSH, IDC_TOOLER_SET};
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
	afx_msg void OnBnClickedCheckMstConsole();
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
	void ListAddHost(HOST_STRUCT const * pHost, int Id);
	void OnMenuClickedExportGroup(void);
	void OnMenuClickedImportGroup(void);
	void ImportGroup(HTREEITEM hItem, int ExportId);
	afx_msg void OnBnClickedBtnCheckOnline();
	afx_msg void OnBnClickedBtnSearch();
	void DataBaseConversion(int Ver);
};
