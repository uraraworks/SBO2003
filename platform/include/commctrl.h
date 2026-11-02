/* Web版: commctrl.h の代わり。使われているコモンコントロールだけ用意する */
#pragma once
#include "win32compat.h"

#define WC_LISTVIEW			"SysListView32"

void InitCommonControls(void);

/* ------------------------------------------------------------------------- */
/* リストビュー																 */
/* ------------------------------------------------------------------------- */
typedef struct tagLVITEM {
	UINT	mask;
	int		iItem;
	int		iSubItem;
	UINT	state;
	UINT	stateMask;
	LPSTR	pszText;
	int		cchTextMax;
	int		iImage;
	LPARAM	lParam;
	int		iIndent;
} LVITEM, LV_ITEM, *LPLVITEM;

typedef struct tagLVCOLUMN {
	UINT	mask;
	int		fmt;
	int		cx;
	LPSTR	pszText;
	int		cchTextMax;
	int		iSubItem;
	int		iImage;
	int		iOrder;
} LVCOLUMN, LV_COLUMN, *LPLVCOLUMN;

typedef struct tagNMLISTVIEW {
	NMHDR	hdr;
	int		iItem;
	int		iSubItem;
	UINT	uNewState;
	UINT	uOldState;
	UINT	uChanged;
	POINT	ptAction;
	LPARAM	lParam;
} NMLISTVIEW, NM_LISTVIEW, *LPNMLISTVIEW;

typedef struct tagNMITEMACTIVATE {
	NMHDR	hdr;
	int		iItem;
	int		iSubItem;
	UINT	uNewState;
	UINT	uOldState;
	UINT	uChanged;
	POINT	ptAction;
	LPARAM	lParam;
	UINT	uKeyFlags;
} NMITEMACTIVATE, *LPNMITEMACTIVATE;

/* スタイル */
#define LVS_ICON				0x0000
#define LVS_REPORT				0x0001
#define LVS_SMALLICON			0x0002
#define LVS_LIST				0x0003
#define LVS_TYPEMASK			0x0003
#define LVS_SINGLESEL			0x0004
#define LVS_SHOWSELALWAYS		0x0008
#define LVS_SORTASCENDING		0x0010
#define LVS_SORTDESCENDING		0x0020
#define LVS_SHAREIMAGELISTS		0x0040
#define LVS_NOLABELWRAP			0x0080
#define LVS_AUTOARRANGE			0x0100
#define LVS_EDITLABELS			0x0200
#define LVS_OWNERDATA			0x1000
#define LVS_NOSCROLL			0x2000
#define LVS_ALIGNTOP			0x0000
#define LVS_ALIGNLEFT			0x0800
#define LVS_NOCOLUMNHEADER		0x4000
#define LVS_NOSORTHEADER		0x8000

#define LVS_EX_GRIDLINES		0x00000001
#define LVS_EX_SUBITEMIMAGES	0x00000002
#define LVS_EX_CHECKBOXES		0x00000004
#define LVS_EX_TRACKSELECT		0x00000008
#define LVS_EX_HEADERDRAGDROP	0x00000010
#define LVS_EX_FULLROWSELECT	0x00000020
#define LVS_EX_ONECLICKACTIVATE	0x00000040

/* 項目 */
#define LVIF_TEXT				0x0001
#define LVIF_IMAGE				0x0002
#define LVIF_PARAM				0x0004
#define LVIF_STATE				0x0008
#define LVIF_INDENT				0x0010

#define LVIS_FOCUSED			0x0001
#define LVIS_SELECTED			0x0002
#define LVIS_CUT				0x0004
#define LVIS_DROPHILITED		0x0008
#define LVIS_OVERLAYMASK		0x0F00
#define LVIS_STATEIMAGEMASK		0xF000
#define INDEXTOSTATEIMAGEMASK(i)	((i) << 12)

#define LVNI_ALL				0x0000
#define LVNI_FOCUSED			0x0001
#define LVNI_SELECTED			0x0002

/* 列 */
#define LVCF_FMT				0x0001
#define LVCF_WIDTH				0x0002
#define LVCF_TEXT				0x0004
#define LVCF_SUBITEM			0x0008
#define LVCFMT_LEFT				0x0000
#define LVCFMT_RIGHT			0x0001
#define LVCFMT_CENTER			0x0002

#define LVSCW_AUTOSIZE			-1
#define LVSCW_AUTOSIZE_USEHEADER	-2

/* メッセージ */
#define LVM_FIRST				0x1000
#define LVM_GETITEMCOUNT		(LVM_FIRST + 4)
#define LVM_GETITEM				(LVM_FIRST + 5)
#define LVM_SETITEM				(LVM_FIRST + 6)
#define LVM_INSERTITEM			(LVM_FIRST + 7)
#define LVM_DELETEITEM			(LVM_FIRST + 8)
#define LVM_DELETEALLITEMS		(LVM_FIRST + 9)
#define LVM_GETNEXTITEM			(LVM_FIRST + 12)
#define LVM_ENSUREVISIBLE		(LVM_FIRST + 19)
#define LVM_GETCOLUMN			(LVM_FIRST + 25)
#define LVM_SETCOLUMN			(LVM_FIRST + 26)
#define LVM_INSERTCOLUMN		(LVM_FIRST + 27)
#define LVM_DELETECOLUMN		(LVM_FIRST + 28)
#define LVM_GETCOLUMNWIDTH		(LVM_FIRST + 29)
#define LVM_SETCOLUMNWIDTH		(LVM_FIRST + 30)
#define LVM_SETITEMSTATE		(LVM_FIRST + 43)
#define LVM_GETITEMSTATE		(LVM_FIRST + 44)
#define LVM_GETITEMTEXT			(LVM_FIRST + 45)
#define LVM_SETITEMTEXT			(LVM_FIRST + 46)
#define LVM_SETITEMCOUNT		(LVM_FIRST + 47)
#define LVM_GETSELECTEDCOUNT	(LVM_FIRST + 50)
#define LVM_SETEXTENDEDLISTVIEWSTYLE	(LVM_FIRST + 54)
#define LVM_GETEXTENDEDLISTVIEWSTYLE	(LVM_FIRST + 55)
#define LVM_SETSELECTIONMARK	(LVM_FIRST + 67)
#define LVM_GETSELECTIONMARK	(LVM_FIRST + 66)

/* 通知 */
#define NM_FIRST				0U
#define NM_CLICK				(NM_FIRST - 2)
#define NM_DBLCLK				(NM_FIRST - 3)
#define NM_RETURN				(NM_FIRST - 4)
#define NM_RCLICK				(NM_FIRST - 5)
#define NM_RDBLCLK				(NM_FIRST - 6)
#define LVN_FIRST				(0U - 100U)
#define LVN_ITEMCHANGING		(LVN_FIRST - 0)
#define LVN_ITEMCHANGED			(LVN_FIRST - 1)
#define LVN_INSERTITEM			(LVN_FIRST - 2)
#define LVN_DELETEITEM			(LVN_FIRST - 3)
#define LVN_DELETEALLITEMS		(LVN_FIRST - 4)
#define LVN_COLUMNCLICK			(LVN_FIRST - 8)
#define LVN_KEYDOWN				(LVN_FIRST - 55)

/* マクロ（中身は SendMessage） */
#define ListView_GetItemCount(h)		(int)::SendMessage((h), LVM_GETITEMCOUNT, 0, 0L)
#define ListView_GetItem(h, pi)			(BOOL)::SendMessage((h), LVM_GETITEM, 0, (LPARAM)(LV_ITEM *)(pi))
#define ListView_SetItem(h, pi)			(BOOL)::SendMessage((h), LVM_SETITEM, 0, (LPARAM)(const LV_ITEM *)(pi))
#define ListView_InsertItem(h, pi)		(int)::SendMessage((h), LVM_INSERTITEM, 0, (LPARAM)(const LV_ITEM *)(pi))
#define ListView_DeleteItem(h, i)		(BOOL)::SendMessage((h), LVM_DELETEITEM, (WPARAM)(int)(i), 0L)
#define ListView_DeleteAllItems(h)		(BOOL)::SendMessage((h), LVM_DELETEALLITEMS, 0, 0L)
#define ListView_GetNextItem(h, i, f)	(int)::SendMessage((h), LVM_GETNEXTITEM, (WPARAM)(int)(i), MAKELPARAM((f), 0))
#define ListView_EnsureVisible(h, i, f)	(BOOL)::SendMessage((h), LVM_ENSUREVISIBLE, (WPARAM)(int)(i), MAKELPARAM((f), 0))
#define ListView_GetColumn(h, i, p)		(BOOL)::SendMessage((h), LVM_GETCOLUMN, (WPARAM)(int)(i), (LPARAM)(LV_COLUMN *)(p))
#define ListView_SetColumn(h, i, p)		(BOOL)::SendMessage((h), LVM_SETCOLUMN, (WPARAM)(int)(i), (LPARAM)(const LV_COLUMN *)(p))
#define ListView_InsertColumn(h, i, p)	(int)::SendMessage((h), LVM_INSERTCOLUMN, (WPARAM)(int)(i), (LPARAM)(const LV_COLUMN *)(p))
#define ListView_DeleteColumn(h, i)		(BOOL)::SendMessage((h), LVM_DELETECOLUMN, (WPARAM)(int)(i), 0)
#define ListView_GetColumnWidth(h, i)	(int)::SendMessage((h), LVM_GETCOLUMNWIDTH, (WPARAM)(int)(i), 0)
#define ListView_SetColumnWidth(h, i, cx)	(BOOL)::SendMessage((h), LVM_SETCOLUMNWIDTH, (WPARAM)(int)(i), MAKELPARAM((cx), 0))
#define ListView_GetSelectedCount(h)	(UINT)::SendMessage((h), LVM_GETSELECTEDCOUNT, 0, 0L)
#define ListView_GetSelectionMark(h)	(int)::SendMessage((h), LVM_GETSELECTIONMARK, 0, 0)
#define ListView_SetSelectionMark(h, i)	(int)::SendMessage((h), LVM_SETSELECTIONMARK, 0, (LPARAM)(i))
#define ListView_SetExtendedListViewStyle(h, s)	(DWORD)::SendMessage((h), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, (LPARAM)(s))
#define ListView_GetExtendedListViewStyle(h)	(DWORD)::SendMessage((h), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0)
#define ListView_GetItemState(h, i, m)	(UINT)::SendMessage((h), LVM_GETITEMSTATE, (WPARAM)(i), (LPARAM)(m))

#define ListView_SetItemState(h, i, data, m) \
	{ LV_ITEM _lvi; _lvi.stateMask = (m); _lvi.state = (data); \
	  ::SendMessage((h), LVM_SETITEMSTATE, (WPARAM)(i), (LPARAM)(LV_ITEM *)&_lvi); }
#define ListView_GetItemText(h, i, sub, text, max) \
	{ LV_ITEM _lvi; _lvi.iSubItem = (sub); _lvi.cchTextMax = (max); _lvi.pszText = (text); \
	  ::SendMessage((h), LVM_GETITEMTEXT, (WPARAM)(i), (LPARAM)(LV_ITEM *)&_lvi); }
#define ListView_SetItemText(h, i, sub, text) \
	{ LV_ITEM _lvi; _lvi.iSubItem = (sub); _lvi.pszText = (text); \
	  ::SendMessage((h), LVM_SETITEMTEXT, (WPARAM)(i), (LPARAM)(LV_ITEM *)&_lvi); }
#define ListView_SetCheckState(h, i, f) \
	ListView_SetItemState((h), (i), INDEXTOSTATEIMAGEMASK((f) ? 2 : 1), LVIS_STATEIMAGEMASK)
#define ListView_GetCheckState(h, i) \
	((((UINT)(::SendMessage((h), LVM_GETITEMSTATE, (WPARAM)(i), LVIS_STATEIMAGEMASK))) >> 12) - 1)
