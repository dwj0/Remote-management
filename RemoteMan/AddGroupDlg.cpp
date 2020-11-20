// AddGroupDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "RemoteMan.h"
#include "AddGroupDlg.h"
#include "afxdialogex.h"


// CAddGroupDlg 对话框

IMPLEMENT_DYNAMIC(CAddGroupDlg, CDialogEx)

CAddGroupDlg::CAddGroupDlg(char const *GroupName, CWnd* pParent /*=NULL*/)
	: CDialogEx(CAddGroupDlg::IDD, pParent)
{
	strcpy_s(m_GroupName,sizeof(m_GroupName),GroupName);
}

CAddGroupDlg::~CAddGroupDlg()
{
}

void CAddGroupDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CAddGroupDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CAddGroupDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CAddGroupDlg 消息处理程序


BOOL CAddGroupDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	int sel=0;
	CComboBox *pBox=(CComboBox*)GetDlgItem(IDC_COMBO1);
	pBox->AddString("根目录");
	if (m_GroupName[0]!=0)
	{
		pBox->AddString(m_GroupName);
		sel=1;
	}
	pBox->SetCurSel(sel);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CAddGroupDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CString str;
	GetDlgItemText(IDC_EDIT1,str);
	str.Trim();
	if (str.GetLength()==0)
	{
		MessageBox("请输入分组名称","错误");
		return;
	}
	strcpy_s(m_GroupName,sizeof(m_GroupName),str);

	m_AddRoot = ((CComboBox*)GetDlgItem(IDC_COMBO1))->GetCurSel()==0;
	CDialogEx::OnOK();
}
