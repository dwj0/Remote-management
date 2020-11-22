// InputPasswordDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "RemoteMan.h"
#include "InputPasswordDlg.h"
#include "afxdialogex.h"


// CInputPasswordDlg 对话框

IMPLEMENT_DYNAMIC(CInputPasswordDlg, CDialogEx)

CInputPasswordDlg::CInputPasswordDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CInputPasswordDlg::IDD, pParent)
	, m_Password(_T(""))
{

}

CInputPasswordDlg::~CInputPasswordDlg()
{
}

void CInputPasswordDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_Password);
}


BEGIN_MESSAGE_MAP(CInputPasswordDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CInputPasswordDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CInputPasswordDlg 消息处理程序


void CInputPasswordDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	if (m_Password.GetLength()>0)
		CDialogEx::OnOK();
}
