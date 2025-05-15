
// HighResClockDlg.h: 头文件
//

#pragma once

#include <memory>
#include "d2dtext.h"

// CHighResClockDlg 对话框
class CHighResClockDlg : public CDialogEx
{
// 构造
public:
	CHighResClockDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_HIGHRESCLOCK_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
    afx_msg void OnClose();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	DECLARE_MESSAGE_MAP()


	std::unique_ptr<d2dtext> _render;
    bool _showMsg = false;

  public:
    afx_msg void OnLButtonDblClk(UINT, CPoint);
    afx_msg void OnSize(UINT, int, int);
};
