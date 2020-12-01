#pragma once


static  const   int     DIM_TEXT_LEN =  128;                // Dim Text Buffer Length
// CDimEdit

class CDimEdit : public CEdit
{
	DECLARE_DYNAMIC(CDimEdit)

public:
	CDimEdit();
	virtual ~CDimEdit();

	void    SetShowDimControl( bool bShow );                // Show Or Hide The Dim Control
	void    SetDimText( LPCTSTR cpText );                   // Set The Dim Text
	void    SetDimColor( COLORREF crDColor );               // Set The Dim Color
	void    SetDimOffset( char cRedOS, char cGreenOS,
		char cBlueOS );                           // Set The Dim Color Offset

	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT &rect, CWnd *pParentWnd, UINT nID, CCreateContext *pContext = NULL);
protected:
	virtual void PreSubclassWindow();
	afx_msg void OnChange();
	afx_msg void OnSetfocus();
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnKillfocus();
	void        DrawDimText( void );                        // Draw The Dim Text

	COLORREF    m_crDimTextColor;                           // "Hard" Dim Text Color
	TCHAR       m_caDimText[ DIM_TEXT_LEN + 1 ];            // Dim Text Buffer
	bool        m_bShowDimText;                             // Are We Showing The Dim Text?
	bool        m_bUseDimOffset;                            // Are We Using The Offset Colors (Not Hard Color)?
	char        m_cRedOS;                                   // Red Color Dim Offset
	char        m_cGreenOS;                                 // Green Color Dim Offset
	char        m_cBlueOS;                                  // Blue Color Dim Offset
	int         m_iDimTextLen;                              // Length Of Dim Text
protected:
	DECLARE_MESSAGE_MAP()
};


