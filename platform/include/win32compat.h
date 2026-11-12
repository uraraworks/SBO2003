/* ========================================================================= */
/* ファイル名：	win32compat.h												 */
/* 概要：		Web(Emscripten)版・Mac/Linux版のための Win32 API 互換レイヤ	 */
/*				windows.h などの代わりに読み込まれ、ゲーム本体のコードを		 */
/*				ほぼそのままコンパイルできるようにする						 */
/* ========================================================================= */
/*
 * 方針
 *  - 型・定数・構造体は Win32 と同じ名前・同じ意味で定義する
 *  - 関数の実体は platform/*.cpp（ブラウザ専用の HTML 部品などは web/*.cpp） にある
 *    - ウィンドウ・メッセージ・タイマー → w32_core.cpp
 *    - GDI(DC・ビットマップ・フォント・文字描画) → w32_gdi.cpp
 *    - ファイル・INI・リソース → w32_file.cpp
 *  - wasm32 は long もポインタも 32bit なので、Win32 と同じキャストがそのまま通り、
 *    保存データや通信の構造体の大きさも Windows 版と揃う
 *  - 実行時の文字コードは Shift-JIS（ゲームデータ・通信に合わせる）
 */
#pragma once

#ifdef _WIN32
#error "win32compat.h は Emscripten（ブラウザ・Node.js）向けです"
#endif

/*
 * 動かし方の区別
 *   W32_BROWSER  … Emscripten でブラウザ向け（画面・入力・IndexedDB を使う。クライアント）
 *   W32_HEADLESS … 画面なし（サーバー。Node.js 向け）
 */
#ifndef __EMSCRIPTEN__
#error "win32compat.h は Emscripten でビルドしてください"
#endif
#ifndef W32_HEADLESS
#define W32_BROWSER
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <wchar.h>

/* ------------------------------------------------------------------------- */
/* 呼び出し規約などのキーワード（意味を持たないので空にする）				 */
/* ------------------------------------------------------------------------- */
#define WINAPI
#define WINAPIV
#define APIENTRY
#define CALLBACK
#define PASCAL
#define FAR
#define NEAR
#define far
#define near
#define CONST const
#define IN
#define OUT
#define OPTIONAL
#define __cdecl
#define __stdcall
#define _cdecl
#define __declspec(x)
#define __forceinline inline
#define _inline inline

/* ------------------------------------------------------------------------- */
/* 基本型																	 */
/* ------------------------------------------------------------------------- */
typedef int					BOOL;
typedef unsigned char		BYTE;
typedef unsigned short		WORD;
typedef unsigned long		DWORD;
typedef long				LONG;
typedef unsigned long		ULONG;
typedef int					INT;
typedef unsigned int		UINT;
typedef short				SHORT;
typedef unsigned short		USHORT;
typedef char				CHAR;
typedef unsigned char		UCHAR;
typedef wchar_t				WCHAR;
typedef float				FLOAT;
typedef double				DOUBLE;
typedef long long			LONGLONG;
typedef unsigned long long	ULONGLONG;
typedef unsigned long long	DWORDLONG;
typedef long long			__int64;
typedef void				VOID;
typedef LONG				HRESULT;
typedef BYTE				BOOLEAN;
typedef char				TCHAR;
typedef unsigned char		TBYTE;

typedef int					INT_PTR;
typedef unsigned int		UINT_PTR;
typedef long				LONG_PTR;
typedef unsigned long		ULONG_PTR;
typedef ULONG_PTR			DWORD_PTR;
typedef ULONG_PTR			SIZE_T;

typedef BOOL				*PBOOL, *LPBOOL;
typedef BYTE				*PBYTE, *LPBYTE;
typedef WORD				*PWORD, *LPWORD;
typedef DWORD				*PDWORD, *LPDWORD;
typedef LONG				*PLONG, *LPLONG;
typedef INT					*PINT, *LPINT;
typedef UINT				*PUINT;
typedef void				*PVOID, *LPVOID;
typedef const void			*LPCVOID;
typedef CHAR				*PCHAR, *LPSTR, *PSTR;
typedef const CHAR			*LPCSTR, *PCSTR;
typedef WCHAR				*LPWSTR;
typedef const WCHAR			*LPCWSTR;
typedef TCHAR				*LPTSTR, *PTSTR;
typedef const TCHAR			*LPCTSTR;
typedef SHORT				*PSHORT;

typedef UINT_PTR			WPARAM;
typedef LONG_PTR			LPARAM;
typedef LONG_PTR			LRESULT;
typedef DWORD				COLORREF, *LPCOLORREF;
typedef WORD				ATOM;
typedef WORD				LANGID;
typedef DWORD				LCID;

/* ハンドル類（中身は互換レイヤ内部の構造体） */
typedef void				*HANDLE;
typedef HANDLE				*PHANDLE, *LPHANDLE;
typedef struct W32Wnd		*HWND;
typedef struct W32Dc		*HDC;
typedef struct W32GdiObj	*HGDIOBJ;
typedef HGDIOBJ				HBITMAP, HFONT, HBRUSH, HPEN, HRGN, HPALETTE;
typedef struct W32Module	*HMODULE;
typedef HMODULE				HINSTANCE;
typedef struct W32Res		*HRSRC;
typedef void				*HGLOBAL, *HLOCAL;
typedef void				*HICON, *HCURSOR, *HMENU, *HACCEL, *HIMC, *HKEY, *HDWP;
typedef HANDLE				HDROP;
typedef int					(*FARPROC)(void);

typedef LRESULT (CALLBACK *WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef void (CALLBACK *TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);
typedef BOOL (CALLBACK *DLGPROC)(HWND, UINT, WPARAM, LPARAM);

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif
#ifndef NULL
#define NULL	0
#endif

#define S_OK			((HRESULT)0)
#define S_FALSE			((HRESULT)1)
#define E_FAIL			((HRESULT)0x80004005L)
#define SUCCEEDED(hr)	(((HRESULT)(hr)) >= 0)
#define FAILED(hr)		(((HRESULT)(hr)) < 0)

#define MAX_PATH		260
#define INFINITE		0xFFFFFFFF
#define INVALID_HANDLE_VALUE	((HANDLE)(LONG_PTR)-1)

#define _T(x)			x
#define TEXT(x)			x
#define _TEXT(x)		x

/* ------------------------------------------------------------------------- */
/* 構造体																	 */
/* ------------------------------------------------------------------------- */
typedef struct tagPOINT { LONG x; LONG y; } POINT, *PPOINT, *LPPOINT;
typedef struct tagSIZE { LONG cx; LONG cy; } SIZE, *PSIZE, *LPSIZE;
typedef struct tagRECT { LONG left; LONG top; LONG right; LONG bottom; } RECT, *PRECT, *LPRECT;
typedef const RECT *LPCRECT;
typedef struct tagPOINTS { SHORT x; SHORT y; } POINTS;

typedef struct tagMSG {
	HWND	hwnd;
	UINT	message;
	WPARAM	wParam;
	LPARAM	lParam;
	DWORD	time;
	POINT	pt;
} MSG, *PMSG, *LPMSG;

typedef struct tagWNDCLASS {
	UINT		style;
	WNDPROC		lpfnWndProc;
	int			cbClsExtra;
	int			cbWndExtra;
	HINSTANCE	hInstance;
	HICON		hIcon;
	HCURSOR		hCursor;
	HBRUSH		hbrBackground;
	LPCSTR		lpszMenuName;
	LPCSTR		lpszClassName;
} WNDCLASS, *PWNDCLASS, *LPWNDCLASS, WNDCLASSA;

typedef struct tagWNDCLASSEX {
	UINT		cbSize;
	UINT		style;
	WNDPROC		lpfnWndProc;
	int			cbClsExtra;
	int			cbWndExtra;
	HINSTANCE	hInstance;
	HICON		hIcon;
	HCURSOR		hCursor;
	HBRUSH		hbrBackground;
	LPCSTR		lpszMenuName;
	LPCSTR		lpszClassName;
	HICON		hIconSm;
} WNDCLASSEX, *PWNDCLASSEX, *LPWNDCLASSEX;

typedef struct tagCREATESTRUCT {
	LPVOID		lpCreateParams;
	HINSTANCE	hInstance;
	HMENU		hMenu;
	HWND		hwndParent;
	int			cy;
	int			cx;
	int			y;
	int			x;
	LONG		style;
	LPCSTR		lpszName;
	LPCSTR		lpszClass;
	DWORD		dwExStyle;
} CREATESTRUCT, *LPCREATESTRUCT;

typedef struct tagPAINTSTRUCT {
	HDC		hdc;
	BOOL	fErase;
	RECT	rcPaint;
	BOOL	fRestore;
	BOOL	fIncUpdate;
	BYTE	rgbReserved[32];
} PAINTSTRUCT, *PPAINTSTRUCT, *LPPAINTSTRUCT;

typedef struct tagMINMAXINFO {
	POINT ptReserved;
	POINT ptMaxSize;
	POINT ptMaxPosition;
	POINT ptMinTrackSize;
	POINT ptMaxTrackSize;
} MINMAXINFO, *LPMINMAXINFO;

typedef struct _SYSTEMTIME {
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

typedef struct _FILETIME { DWORD dwLowDateTime; DWORD dwHighDateTime; } FILETIME, *LPFILETIME;

typedef struct _SECURITY_ATTRIBUTES {
	DWORD	nLength;
	LPVOID	lpSecurityDescriptor;
	BOOL	bInheritHandle;
} SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

typedef struct _OVERLAPPED { DWORD Internal, InternalHigh, Offset, OffsetHigh; HANDLE hEvent; } OVERLAPPED, *LPOVERLAPPED;

typedef struct _CRITICAL_SECTION { int nDummy; } CRITICAL_SECTION, *LPCRITICAL_SECTION;

typedef union _LARGE_INTEGER {
	struct { DWORD LowPart; LONG HighPart; };
	LONGLONG QuadPart;
} LARGE_INTEGER;

typedef struct _GUID {
	DWORD	Data1;
	WORD	Data2;
	WORD	Data3;
	BYTE	Data4[8];
} GUID;

/* ビットマップ */
#pragma pack(push, 2)
typedef struct tagBITMAPFILEHEADER {
	WORD	bfType;
	DWORD	bfSize;
	WORD	bfReserved1;
	WORD	bfReserved2;
	DWORD	bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER, *LPBITMAPFILEHEADER;
#pragma pack(pop)

typedef struct tagBITMAPINFOHEADER {
	DWORD	biSize;
	LONG	biWidth;
	LONG	biHeight;
	WORD	biPlanes;
	WORD	biBitCount;
	DWORD	biCompression;
	DWORD	biSizeImage;
	LONG	biXPelsPerMeter;
	LONG	biYPelsPerMeter;
	DWORD	biClrUsed;
	DWORD	biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER, *LPBITMAPINFOHEADER;

typedef struct tagRGBQUAD {
	BYTE	rgbBlue;
	BYTE	rgbGreen;
	BYTE	rgbRed;
	BYTE	rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFO {
	BITMAPINFOHEADER	bmiHeader;
	RGBQUAD				bmiColors[1];
} BITMAPINFO, *PBITMAPINFO, *LPBITMAPINFO;

typedef struct tagBITMAP {
	LONG	bmType;
	LONG	bmWidth;
	LONG	bmHeight;
	LONG	bmWidthBytes;
	WORD	bmPlanes;
	WORD	bmBitsPixel;
	LPVOID	bmBits;
} BITMAP, *PBITMAP, *LPBITMAP;

typedef struct tagTEXTMETRIC {
	LONG tmHeight, tmAscent, tmDescent, tmInternalLeading, tmExternalLeading;
	LONG tmAveCharWidth, tmMaxCharWidth, tmWeight, tmOverhang;
	LONG tmDigitizedAspectX, tmDigitizedAspectY;
	BYTE tmFirstChar, tmLastChar, tmDefaultChar, tmBreakChar;
	BYTE tmItalic, tmUnderlined, tmStruckOut, tmPitchAndFamily, tmCharSet;
} TEXTMETRIC, *LPTEXTMETRIC;

typedef struct tagLOGFONT {
	LONG lfHeight, lfWidth, lfEscapement, lfOrientation, lfWeight;
	BYTE lfItalic, lfUnderline, lfStrikeOut, lfCharSet;
	BYTE lfOutPrecision, lfClipPrecision, lfQuality, lfPitchAndFamily;
	CHAR lfFaceName[32];
} LOGFONT, *LPLOGFONT;

/* マルチメディアタイマー */
typedef struct timecaps_tag { UINT wPeriodMin; UINT wPeriodMax; } TIMECAPS, *LPTIMECAPS;
typedef UINT MMRESULT;
#define TIMERR_NOERROR	0

/* ------------------------------------------------------------------------- */
/* マクロ																	 */
/* ------------------------------------------------------------------------- */
#define LOWORD(l)			((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l)			((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w)			((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w)			((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))
#define MAKEWORD(a, b)		((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#define MAKELONG(a, b)		((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#define MAKEWPARAM(l, h)	((WPARAM)(DWORD)MAKELONG(l, h))
#define MAKELPARAM(l, h)	((LPARAM)(DWORD)MAKELONG(l, h))
#define MAKELRESULT(l, h)	((LRESULT)(DWORD)MAKELONG(l, h))
#define GET_X_LPARAM(lp)	((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)	((int)(short)HIWORD(lp))

#define RGB(r, g, b)		((COLORREF)(((BYTE)(r) | ((WORD)((BYTE)(g)) << 8)) | (((DWORD)(BYTE)(b)) << 16)))
#define GetRValue(rgb)		(LOBYTE(rgb))
#define GetGValue(rgb)		(LOBYTE(((WORD)(rgb)) >> 8))
#define GetBValue(rgb)		(LOBYTE((rgb) >> 16))

#define MAKEINTRESOURCE(i)	((LPSTR)((ULONG_PTR)((WORD)(i))))
#define IS_INTRESOURCE(r)	((((ULONG_PTR)(r)) >> 16) == 0)

#define ZeroMemory(p, n)		memset((p), 0, (n))
#define FillMemory(p, n, v)		memset((p), (v), (n))
#define CopyMemory(d, s, n)		memcpy((d), (s), (n))
#define MoveMemory(d, s, n)		memmove((d), (s), (n))

#define UNREFERENCED_PARAMETER(P)	(void)(P)
#define ASSERT(x)
#define VERIFY(x)	(x)
#define _ASSERT(x)
#define _ASSERTE(x)
#define TRACE(...)

#ifndef __cplusplus
#define min(a, b)	(((a) < (b)) ? (a) : (b))
#define max(a, b)	(((a) > (b)) ? (a) : (b))
#else
/* 旧コードは min/max をマクロとして使うので、型の違う引数でも通るようにする */
/* 戻り値は値で返す（decltype(a < b ? a : b) は参照型になり、引数のコピーへの参照が残ってしまう） */
template<class A, class B> inline auto w32_min(A a, B b) -> decltype(true ? A() : B()) { return (a < b) ? a : b; }
template<class A, class B> inline auto w32_max(A a, B b) -> decltype(true ? A() : B()) { return (a > b) ? a : b; }
#define min(a, b)	w32_min((a), (b))
#define max(a, b)	w32_max((a), (b))
#endif

#ifdef __cplusplus
/* MSVC(VC6) は abs(DWORD) などを abs(int) として受け付けるので、同じように通す */
#include <cstdlib>
inline int	abs(unsigned int v)			{ return ::abs((int)v); }
inline long	abs(unsigned long v)		{ return ::labs((long)v); }
inline int	abs(unsigned short v)		{ return (int)v; }
inline int	abs(unsigned char v)		{ return (int)v; }
#endif

/* CRT の MS 拡張 */
#define stricmp		strcasecmp
#define _stricmp	strcasecmp
#define strcmpi		strcasecmp
#define _strcmpi	strcasecmp
#define strnicmp	strncasecmp
#define _strnicmp	strncasecmp
#define _snprintf	snprintf
#define _vsnprintf	vsnprintf
#define lstrlen		strlen
#define lstrcpy		strcpy
#define lstrcpyn(d, s, n)	(strncpy((d), (s), (n) - 1), (d)[(n) - 1] = 0, (d))
#define lstrcat		strcat
#define lstrcmp		strcmp
#define lstrcmpi	strcasecmp
#define wsprintf	sprintf
#define wvsprintf	vsprintf
#define _tcslen		strlen
#define _tcscpy		strcpy
#define _tcscmp		strcmp
#define _stprintf	sprintf
#define _ttoi		atoi

#ifdef __cplusplus
extern "C++" {
#endif
char	*_strlwr(char *s);
char	*_strupr(char *s);
char	*_itoa(int value, char *str, int radix);
char	*_ltoa(long value, char *str, int radix);
char	*_ultoa(unsigned long value, char *str, int radix);
#define strlwr	_strlwr
#define strupr	_strupr
#define itoa	_itoa
#define ltoa	_ltoa
#ifdef __cplusplus
}
#endif

/* ------------------------------------------------------------------------- */
/* 定数：ウィンドウスタイル													 */
/* ------------------------------------------------------------------------- */
#define WS_OVERLAPPED		0x00000000L
#define WS_POPUP			0x80000000L
#define WS_CHILD			0x40000000L
#define WS_MINIMIZE			0x20000000L
#define WS_VISIBLE			0x10000000L
#define WS_DISABLED			0x08000000L
#define WS_CLIPSIBLINGS		0x04000000L
#define WS_CLIPCHILDREN		0x02000000L
#define WS_MAXIMIZE			0x01000000L
#define WS_CAPTION			0x00C00000L
#define WS_BORDER			0x00800000L
#define WS_DLGFRAME			0x00400000L
#define WS_VSCROLL			0x00200000L
#define WS_HSCROLL			0x00100000L
#define WS_SYSMENU			0x00080000L
#define WS_THICKFRAME		0x00040000L
#define WS_GROUP			0x00020000L
#define WS_TABSTOP			0x00010000L
#define WS_MINIMIZEBOX		0x00020000L
#define WS_MAXIMIZEBOX		0x00010000L
#define WS_OVERLAPPEDWINDOW	(WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_POPUPWINDOW		(WS_POPUP | WS_BORDER | WS_SYSMENU)
#define WS_EX_DLGMODALFRAME	0x00000001L
#define WS_EX_NOPARENTNOTIFY	0x00000004L
#define WS_EX_ACCEPTFILES	0x00000010L
#define WS_EX_TRANSPARENT	0x00000020L
#define WS_EX_WINDOWEDGE	0x00000100L
#define WS_EX_CLIENTEDGE	0x00000200L
#define WS_EX_CONTROLPARENT	0x00010000L
#define WS_EX_STATICEDGE	0x00020000L
#define WS_EX_APPWINDOW		0x00040000L
#define WS_EX_TOPMOST		0x00000008L
#define WS_EX_TOOLWINDOW	0x00000080L

#define CS_VREDRAW			0x0001
#define CS_HREDRAW			0x0002
#define CS_DBLCLKS			0x0008
#define CS_NOCLOSE			0x0200

#define CW_USEDEFAULT		((int)0x80000000)

/* エディット・ボタン */
#define ES_LEFT				0x0000L
#define ES_CENTER			0x0001L
#define ES_RIGHT			0x0002L
#define ES_MULTILINE		0x0004L
#define ES_UPPERCASE		0x0008L
#define ES_LOWERCASE		0x0010L
#define ES_PASSWORD			0x0020L
#define ES_AUTOVSCROLL		0x0040L
#define ES_AUTOHSCROLL		0x0080L
#define ES_NOHIDESEL		0x0100L
#define ES_READONLY			0x0800L
#define ES_WANTRETURN		0x1000L
#define ES_NUMBER			0x2000L

#define BS_PUSHBUTTON		0x00000000L
#define BS_DEFPUSHBUTTON	0x00000001L
#define BS_CHECKBOX			0x00000002L
#define BS_AUTOCHECKBOX		0x00000003L
#define BS_RADIOBUTTON		0x00000004L
#define BS_3STATE			0x00000005L
#define BS_AUTO3STATE		0x00000006L
#define BS_GROUPBOX			0x00000007L
#define BS_AUTORADIOBUTTON	0x00000009L
#define BS_TYPEMASK			0x0000000FL
#define BS_LEFTTEXT			0x00000020L
#define BS_PUSHLIKE			0x00001000L

/* スタティック */
#define SS_LEFT				0x00000000L
#define SS_CENTER			0x00000001L
#define SS_RIGHT			0x00000002L
#define SS_ICON				0x00000003L
#define SS_BLACKRECT		0x00000004L
#define SS_GRAYRECT			0x00000005L
#define SS_WHITERECT		0x00000006L
#define SS_BLACKFRAME		0x00000007L
#define SS_GRAYFRAME		0x00000008L
#define SS_WHITEFRAME		0x00000009L
#define SS_SIMPLE			0x0000000BL
#define SS_LEFTNOWORDWRAP	0x0000000CL
#define SS_ETCHEDHORZ		0x00000010L
#define SS_ETCHEDVERT		0x00000011L
#define SS_ETCHEDFRAME		0x00000012L
#define SS_TYPEMASK			0x0000001FL
#define SS_NOPREFIX			0x00000080L
#define SS_NOTIFY			0x00000100L
#define SS_CENTERIMAGE		0x00000200L
#define SS_SUNKEN			0x00001000L

/* コンボボックス */
#define CBS_SIMPLE			0x0001L
#define CBS_DROPDOWN		0x0002L
#define CBS_DROPDOWNLIST	0x0003L
#define CBS_OWNERDRAWFIXED	0x0010L
#define CBS_AUTOHSCROLL		0x0040L
#define CBS_SORT			0x0100L
#define CBS_HASSTRINGS		0x0200L
#define CBS_NOINTEGRALHEIGHT	0x0400L
#define CBS_DISABLENOSCROLL	0x0800L

/* リストボックス */
#define LBS_NOTIFY			0x0001L
#define LBS_SORT			0x0002L
#define LBS_NOREDRAW		0x0004L
#define LBS_MULTIPLESEL		0x0008L
#define LBS_HASSTRINGS		0x0040L
#define LBS_USETABSTOPS		0x0080L
#define LBS_NOINTEGRALHEIGHT	0x0100L
#define LBS_WANTKEYBOARDINPUT	0x0400L
#define LBS_EXTENDEDSEL		0x0800L
#define LBS_DISABLENOSCROLL	0x1000L
#define LBS_STANDARD		(LBS_NOTIFY | LBS_SORT | WS_VSCROLL | WS_BORDER)

/* スクロールバー */
#define SBS_HORZ			0x0000L
#define SBS_VERT			0x0001L
#define SB_HORZ				0
#define SB_VERT				1
#define SB_CTL				2
#define SB_BOTH				3
#define SIF_RANGE			0x0001
#define SIF_PAGE			0x0002
#define SIF_POS				0x0004
#define SIF_DISABLENOSCROLL	0x0008
#define SIF_TRACKPOS		0x0010
#define SIF_ALL				(SIF_RANGE | SIF_PAGE | SIF_POS | SIF_TRACKPOS)

/* ダイアログ */
#define DS_ABSALIGN			0x0001L
#define DS_SYSMODAL			0x0002L
#define DS_LOCALEDIT		0x0020L
#define DS_SETFONT			0x0040L
#define DS_MODALFRAME		0x0080L
#define DS_NOIDLEMSG		0x0100L
#define DS_SETFOREGROUND	0x0200L
#define DS_3DLOOK			0x0004L
#define DS_FIXEDSYS			0x0008L
#define DS_NOFAILCREATE		0x0010L
#define DS_CONTROL			0x0400L
#define DS_CENTER			0x0800L
#define DS_CENTERMOUSE		0x1000L
#define DS_CONTEXTHELP		0x2000L
#define DS_SHELLFONT		(DS_SETFONT | DS_FIXEDSYS)
#define DWL_MSGRESULT		0
#define DWL_DLGPROC			4
#define DWL_USER			8
#define WC_DIALOG			"#32770"

#define BST_UNCHECKED		0x0000
#define BST_CHECKED			0x0001

/* ShowWindow */
#define SW_HIDE				0
#define SW_SHOWNORMAL		1
#define SW_NORMAL			1
#define SW_SHOWMINIMIZED	2
#define SW_SHOWMAXIMIZED	3
#define SW_SHOWNOACTIVATE	4
#define SW_SHOW				5
#define SW_MINIMIZE			6
#define SW_SHOWNA			8
#define SW_RESTORE			9

/* SetWindowPos */
#define SWP_NOSIZE			0x0001
#define SWP_NOMOVE			0x0002
#define SWP_NOZORDER		0x0004
#define SWP_NOOWNERZORDER	0x0200
#define SWP_NOREDRAW		0x0008
#define SWP_NOACTIVATE		0x0010
#define SWP_SHOWWINDOW		0x0040
#define SWP_HIDEWINDOW		0x0080
#define HWND_TOP			((HWND)0)
#define HWND_TOPMOST		((HWND)-1)
#define HWND_NOTOPMOST		((HWND)-2)

/* Get/SetWindowLong */
#define GWL_WNDPROC			(-4)
#define GWL_HINSTANCE		(-6)
#define GWL_ID				(-12)
#define GWL_STYLE			(-16)
#define GWL_EXSTYLE			(-20)
#define GWL_USERDATA		(-21)
#define GWLP_WNDPROC		GWL_WNDPROC
#define GWLP_USERDATA		GWL_USERDATA
#define GCL_STYLE			(-26)
#define GCL_WNDPROC			(-24)

/* GetSystemMetrics */
#define SM_CXSCREEN			0
#define SM_CYSCREEN			1
#define SM_CYCAPTION		4
#define SM_CXBORDER			5
#define SM_CYBORDER			6
#define SM_CXDLGFRAME		7
#define SM_CYDLGFRAME		8
#define SM_CXFRAME			32
#define SM_CYFRAME			33
#define SM_CXEDGE			45
#define SM_CYEDGE			46
#define SM_CXFIXEDFRAME		SM_CXDLGFRAME
#define SM_CYFIXEDFRAME		SM_CYDLGFRAME

/* PeekMessage */
#define PM_NOREMOVE			0x0000
#define PM_REMOVE			0x0001
#define QS_ALLINPUT			0x04FF
#define WAIT_OBJECT_0		0
#define WAIT_TIMEOUT		258
#define WAIT_FAILED			((DWORD)0xFFFFFFFF)

/* MessageBox */
#define MB_OK				0x00000000L
#define MB_OKCANCEL			0x00000001L
#define MB_YESNO			0x00000004L
#define MB_ICONHAND			0x00000010L
#define MB_ICONQUESTION		0x00000020L
#define MB_ICONEXCLAMATION	0x00000030L
#define MB_ICONINFORMATION	0x00000040L
#define MB_ICONERROR		MB_ICONHAND
#define MB_ICONWARNING		MB_ICONEXCLAMATION
#define IDOK				1
#define IDCANCEL			2
#define IDYES				6
#define IDNO				7

/* WM_ACTIVATE */
#define WA_INACTIVE			0
#define WA_ACTIVE			1
#define WA_CLICKACTIVE		2

/* マウスキー状態 */
#define MK_LBUTTON			0x0001
#define MK_RBUTTON			0x0002
#define MK_SHIFT			0x0004
#define MK_CONTROL			0x0008

/* カーソル・アイコン（中身は使わない） */
#define IDC_ARROW			MAKEINTRESOURCE(32512)
#define IDC_IBEAM			MAKEINTRESOURCE(32513)
#define IDC_WAIT			MAKEINTRESOURCE(32514)
#define IDI_APPLICATION		MAKEINTRESOURCE(32512)

#define COLOR_WINDOW		5
#define COLOR_BTNFACE		15

/* ------------------------------------------------------------------------- */
/* 定数：ウィンドウメッセージ												 */
/* ------------------------------------------------------------------------- */
#define WM_NULL				0x0000
#define WM_CREATE			0x0001
#define WM_DESTROY			0x0002
#define WM_MOVE				0x0003
#define WM_SIZE				0x0005
#define SIZE_RESTORED		0
#define WM_ACTIVATE			0x0006
#define WM_SETFOCUS			0x0007
#define WM_KILLFOCUS		0x0008
#define WM_ENABLE			0x000A
#define WM_SETTEXT			0x000C
#define WM_GETTEXT			0x000D
#define WM_GETTEXTLENGTH	0x000E
#define WM_PAINT			0x000F
#define WM_CLOSE			0x0010
#define WM_QUIT				0x0012
#define WM_ERASEBKGND		0x0014
#define WM_SHOWWINDOW		0x0018
#define WM_ACTIVATEAPP		0x001C
#define WM_SETCURSOR		0x0020
#define WM_MOUSEACTIVATE	0x0021
#define WM_GETMINMAXINFO	0x0024
#define WM_SETFONT			0x0030
#define WM_GETFONT			0x0031
#define WM_WINDOWPOSCHANGED	0x0047
#define WM_NOTIFY			0x004E
#define WM_NCDESTROY		0x0082
#define WM_NCHITTEST		0x0084
#define WM_KEYDOWN			0x0100
#define WM_KEYUP			0x0101
#define WM_CHAR				0x0102
#define WM_SYSKEYDOWN		0x0104
#define WM_SYSKEYUP			0x0105
#define WM_SYSCHAR			0x0106
#define WM_IME_STARTCOMPOSITION	0x010D
#define WM_IME_ENDCOMPOSITION	0x010E
#define WM_IME_COMPOSITION		0x010F
#define WM_INITDIALOG		0x0110
#define WM_COMMAND			0x0111
#define WM_SYSCOMMAND		0x0112
#define WM_TIMER			0x0113
#define WM_HSCROLL			0x0114
#define WM_VSCROLL			0x0115
#define WM_CTLCOLOREDIT		0x0133
#define WM_CTLCOLORSTATIC	0x0138
#define WM_MOUSEMOVE		0x0200
#define WM_LBUTTONDOWN		0x0201
#define WM_LBUTTONUP		0x0202
#define WM_LBUTTONDBLCLK	0x0203
#define WM_RBUTTONDOWN		0x0204
#define WM_RBUTTONUP		0x0205
#define WM_RBUTTONDBLCLK	0x0206
#define WM_MBUTTONDOWN		0x0207
#define WM_MBUTTONUP		0x0208
#define WM_MOUSEWHEEL		0x020A
#define WM_SIZING			0x0214
#define WM_CAPTURECHANGED	0x0215
#define WM_MOVING			0x0216
#define WM_IME_SETCONTEXT	0x0281
#define WM_IME_NOTIFY		0x0282
#define WM_USER				0x0400
#define WM_APP				0x8000

/* エディット・ボタン・リッチエディットのメッセージ */
#define EM_GETSEL			0x00B0
#define EM_SETSEL			0x00B1
#define EM_SCROLL			0x00B5
#define EM_LINESCROLL		0x00B6
#define EM_SCROLLCARET		0x00B7
#define EM_REPLACESEL		0x00C2
#define EM_GETLINECOUNT		0x00BA
#define EM_LINEINDEX		0x00BB
#define EM_LINELENGTH		0x00C1
#define EM_LIMITTEXT		0x00C5
#define EM_SETLIMITTEXT		EM_LIMITTEXT
#define EM_SETREADONLY		0x00CF
#define EM_GETCHARFORMAT	(WM_USER + 58)
#define EM_SETBKGNDCOLOR	(WM_USER + 67)
#define EM_SETCHARFORMAT	(WM_USER + 68)
#define EM_STREAMOUT		(WM_USER + 74)
#define EM_EXLIMITTEXT		(WM_USER + 53)
#define BM_GETCHECK			0x00F0
#define BM_SETCHECK			0x00F1
#define BM_GETSTATE			0x00F2
#define BM_SETSTATE			0x00F3
#define BM_SETSTYLE			0x00F4
#define BM_CLICK			0x00F5
#define BN_CLICKED			0
#define BN_DOUBLECLICKED	5
#define EN_SETFOCUS			0x0100
#define EN_KILLFOCUS		0x0200
#define EN_CHANGE			0x0300
#define EN_UPDATE			0x0400

/* コンボボックスのメッセージ */
#define CB_GETEDITSEL		0x0140
#define CB_LIMITTEXT		0x0141
#define CB_SETEDITSEL		0x0142
#define CB_ADDSTRING		0x0143
#define CB_DELETESTRING		0x0144
#define CB_DIR				0x0145
#define CB_GETCOUNT			0x0146
#define CB_GETCURSEL		0x0147
#define CB_GETLBTEXT		0x0148
#define CB_GETLBTEXTLEN		0x0149
#define CB_INSERTSTRING		0x014A
#define CB_RESETCONTENT		0x014B
#define CB_FINDSTRING		0x014C
#define CB_SELECTSTRING		0x014D
#define CB_SETCURSEL		0x014E
#define CB_SHOWDROPDOWN		0x014F
#define CB_GETITEMDATA		0x0150
#define CB_SETITEMDATA		0x0151
#define CB_GETDROPPEDCONTROLRECT	0x0152
#define CB_SETITEMHEIGHT	0x0153
#define CB_GETITEMHEIGHT	0x0154
#define CB_SETEXTENDEDUI	0x0155
#define CB_GETEXTENDEDUI	0x0156
#define CB_GETDROPPEDSTATE	0x0157
#define CB_FINDSTRINGEXACT	0x0158
#define CB_SETLOCALE		0x0159
#define CB_GETLOCALE		0x015A
#define CB_GETTOPINDEX		0x015B
#define CB_SETTOPINDEX		0x015C
#define CB_GETHORIZONTALEXTENT	0x015D
#define CB_SETHORIZONTALEXTENT	0x015E
#define CB_GETDROPPEDWIDTH	0x015F
#define CB_SETDROPPEDWIDTH	0x0160
#define CB_INITSTORAGE		0x0161
#define CB_ERR				(-1)
#define CB_ERRSPACE			(-2)
#define CBN_ERRSPACE		(-1)
#define CBN_SELCHANGE		1
#define CBN_DBLCLK			2
#define CBN_SETFOCUS		3
#define CBN_KILLFOCUS		4
#define CBN_EDITCHANGE		5
#define CBN_EDITUPDATE		6
#define CBN_DROPDOWN		7
#define CBN_CLOSEUP			8
#define CBN_SELENDOK		9
#define CBN_SELENDCANCEL	10

/* リストボックスのメッセージ */
#define LB_ADDSTRING		0x0180
#define LB_INSERTSTRING		0x0181
#define LB_DELETESTRING		0x0182
#define LB_SELITEMRANGEEX	0x0183
#define LB_RESETCONTENT		0x0184
#define LB_SETSEL			0x0185
#define LB_SETCURSEL		0x0186
#define LB_GETSEL			0x0187
#define LB_GETCURSEL		0x0188
#define LB_GETTEXT			0x0189
#define LB_GETTEXTLEN		0x018A
#define LB_GETCOUNT			0x018B
#define LB_SELECTSTRING		0x018C
#define LB_DIR				0x018D
#define LB_GETTOPINDEX		0x018E
#define LB_FINDSTRING		0x018F
#define LB_GETSELCOUNT		0x0190
#define LB_GETSELITEMS		0x0191
#define LB_SETTABSTOPS		0x0192
#define LB_GETHORIZONTALEXTENT	0x0193
#define LB_SETHORIZONTALEXTENT	0x0194
#define LB_SETCOLUMNWIDTH	0x0195
#define LB_ADDFILE			0x0196
#define LB_SETTOPINDEX		0x0197
#define LB_GETITEMRECT		0x0198
#define LB_GETITEMDATA		0x0199
#define LB_SETITEMDATA		0x019A
#define LB_SELITEMRANGE		0x019B
#define LB_SETANCHORINDEX	0x019C
#define LB_GETANCHORINDEX	0x019D
#define LB_SETCARETINDEX	0x019E
#define LB_GETCARETINDEX	0x019F
#define LB_SETITEMHEIGHT	0x01A0
#define LB_GETITEMHEIGHT	0x01A1
#define LB_FINDSTRINGEXACT	0x01A2
#define LB_ERR				(-1)
#define LB_ERRSPACE			(-2)
#define LBN_ERRSPACE		(-2)
#define LBN_SELCHANGE		1
#define LBN_DBLCLK			2
#define LBN_SELCANCEL		3
#define LBN_SETFOCUS		4
#define LBN_KILLFOCUS		5

/* スクロールバーのメッセージ */
#define SBM_SETPOS			0x00E0
#define SBM_GETPOS			0x00E1
#define SBM_SETRANGE		0x00E2
#define SBM_SETRANGEREDRAW	0x00E6
#define SBM_GETRANGE		0x00E3
#define SBM_ENABLE_ARROWS	0x00E4
#define SBM_SETSCROLLINFO	0x00E9
#define SBM_GETSCROLLINFO	0x00EA

/* ダイアログのメッセージ */
#define DM_GETDEFID			(WM_USER + 0)
#define DM_SETDEFID			(WM_USER + 1)
#define DM_REPOSITION		(WM_USER + 2)
#define WM_NEXTDLGCTL		0x0028
#define WM_GETDLGCODE		0x0087

#define SB_LINEUP			0
#define SB_LINELEFT			0
#define SB_LINEDOWN			1
#define SB_LINERIGHT		1
#define SB_PAGEUP			2
#define SB_PAGELEFT			2
#define SB_PAGEDOWN			3
#define SB_PAGERIGHT		3
#define SB_THUMBPOSITION	4
#define SB_THUMBTRACK		5
#define SB_TOP				6
#define SB_LEFT				6
#define SB_BOTTOM			7
#define SB_RIGHT			7
#define SB_ENDSCROLL		8

/* リッチエディット */
#define SCF_SELECTION		0x0001
#define SCF_WORD			0x0002
#define SCF_ALL				0x0004
#define CFM_BOLD			0x00000001
#define CFM_ITALIC			0x00000002
#define CFM_SIZE			0x80000000
#define CFM_COLOR			0x40000000
#define CFM_FACE			0x20000000
#define CFM_CHARSET			0x08000000
#define CFE_AUTOCOLOR		0x40000000
#define SF_TEXT				0x0001
#define LF_FACESIZE			32
typedef struct _charformat {
	UINT		cbSize;
	DWORD		dwMask;
	DWORD		dwEffects;
	LONG		yHeight;
	LONG		yOffset;
	COLORREF	crTextColor;
	BYTE		bCharSet;
	BYTE		bPitchAndFamily;
	char		szFaceName[LF_FACESIZE];
} CHARFORMAT, CHARFORMATA;
typedef DWORD (CALLBACK *EDITSTREAMCALLBACK)(DWORD_PTR, LPBYTE, LONG, LONG *);
typedef struct _editstream {
	DWORD_PTR			dwCookie;
	DWORD				dwError;
	EDITSTREAMCALLBACK	pfnCallback;
} EDITSTREAM;

/* ------------------------------------------------------------------------- */
/* 定数：仮想キー															 */
/* ------------------------------------------------------------------------- */
#define VK_LBUTTON		0x01
#define VK_RBUTTON		0x02
#define VK_CANCEL		0x03
#define VK_BACK			0x08
#define VK_TAB			0x09
#define VK_CLEAR		0x0C
#define VK_RETURN		0x0D
#define VK_SHIFT		0x10
#define VK_CONTROL		0x11
#define VK_MENU			0x12
#define VK_PAUSE		0x13
#define VK_CAPITAL		0x14
#define VK_KANA			0x15
#define VK_KANJI		0x19
#define VK_ESCAPE		0x1B
#define VK_CONVERT		0x1C
#define VK_NONCONVERT	0x1D
#define VK_SPACE		0x20
#define VK_PRIOR		0x21
#define VK_NEXT			0x22
#define VK_END			0x23
#define VK_HOME			0x24
#define VK_LEFT			0x25
#define VK_UP			0x26
#define VK_RIGHT		0x27
#define VK_DOWN			0x28
#define VK_SELECT		0x29
#define VK_SNAPSHOT		0x2C
#define VK_INSERT		0x2D
#define VK_DELETE		0x2E
#define VK_NUMPAD0		0x60
#define VK_NUMPAD1		0x61
#define VK_NUMPAD2		0x62
#define VK_NUMPAD3		0x63
#define VK_NUMPAD4		0x64
#define VK_NUMPAD5		0x65
#define VK_NUMPAD6		0x66
#define VK_NUMPAD7		0x67
#define VK_NUMPAD8		0x68
#define VK_NUMPAD9		0x69
#define VK_MULTIPLY		0x6A
#define VK_ADD			0x6B
#define VK_SEPARATOR	0x6C
#define VK_SUBTRACT		0x6D
#define VK_DECIMAL		0x6E
#define VK_DIVIDE		0x6F
#define VK_F1			0x70
#define VK_F2			0x71
#define VK_F3			0x72
#define VK_F4			0x73
#define VK_F5			0x74
#define VK_F6			0x75
#define VK_F7			0x76
#define VK_F8			0x77
#define VK_F9			0x78
#define VK_F10			0x79
#define VK_F11			0x7A
#define VK_F12			0x7B
#define VK_NUMLOCK		0x90
#define VK_SCROLL		0x91
#define VK_LSHIFT		0xA0
#define VK_RSHIFT		0xA1
#define VK_LCONTROL		0xA2
#define VK_RCONTROL		0xA3

/* ------------------------------------------------------------------------- */
/* 定数：GDI																 */
/* ------------------------------------------------------------------------- */
#define SRCCOPY			0x00CC0020
#define SRCPAINT		0x00EE0086
#define SRCAND			0x008800C6
#define SRCINVERT		0x00660046
#define BLACKNESS		0x00000042
#define WHITENESS		0x00FF0062

#define TRANSPARENT		1
#define OPAQUE			2

#define BI_RGB			0L
#define DIB_RGB_COLORS	0

#define WHITE_BRUSH		0
#define LTGRAY_BRUSH	1
#define GRAY_BRUSH		2
#define DKGRAY_BRUSH	3
#define BLACK_BRUSH		4
#define NULL_BRUSH		5
#define HOLLOW_BRUSH	NULL_BRUSH
#define WHITE_PEN		6
#define BLACK_PEN		7
#define NULL_PEN		8
#define SYSTEM_FONT		13
#define DEFAULT_GUI_FONT	17

#define FW_DONTCARE		0
#define FW_THIN			100
#define FW_LIGHT		300
#define FW_NORMAL		400
#define FW_REGULAR		400
#define FW_MEDIUM		500
#define FW_SEMIBOLD		600
#define FW_BOLD			700
#define FW_EXTRABOLD	800
#define FW_ULTRABOLD	800
#define FW_HEAVY		900

#define ANSI_CHARSET		0
#define DEFAULT_CHARSET		1
#define SHIFTJIS_CHARSET	128
#define OUT_DEFAULT_PRECIS	0
#define CLIP_DEFAULT_PRECIS	0
#define DEFAULT_QUALITY		0
#define NONANTIALIASED_QUALITY	3
#define ANTIALIASED_QUALITY		4
#define DEFAULT_PITCH		0
#define FIXED_PITCH			1
#define VARIABLE_PITCH		2
#define FF_DONTCARE			0x00
#define FF_ROMAN			0x10
#define FF_SWISS			0x20
#define FF_MODERN			0x30

#define PS_SOLID			0
#define PS_DASH				1
#define PS_DOT				2

#define DT_TOP				0x00000000
#define DT_LEFT				0x00000000
#define DT_CENTER			0x00000001
#define DT_RIGHT			0x00000002
#define DT_VCENTER			0x00000004
#define DT_BOTTOM			0x00000008
#define DT_WORDBREAK		0x00000010
#define DT_SINGLELINE		0x00000020
#define DT_NOCLIP			0x00000100
#define DT_CALCRECT			0x00000400
#define DT_NOPREFIX			0x00000800

#define IMAGE_BITMAP		0
#define LR_DEFAULTCOLOR		0x0000
#define LR_LOADFROMFILE		0x0010
#define LR_CREATEDIBSECTION	0x2000

#define OBJ_BITMAP			7

/* ------------------------------------------------------------------------- */
/* 定数：ファイル															 */
/* ------------------------------------------------------------------------- */
#define GENERIC_READ			0x80000000L
#define GENERIC_WRITE			0x40000000L
#define FILE_SHARE_READ			0x00000001
#define FILE_SHARE_WRITE		0x00000002
#define CREATE_NEW				1
#define CREATE_ALWAYS			2
#define OPEN_EXISTING			3
#define OPEN_ALWAYS				4
#define TRUNCATE_EXISTING		5
#define FILE_ATTRIBUTE_READONLY		0x00000001
#define FILE_ATTRIBUTE_DIRECTORY	0x00000010
#define FILE_ATTRIBUTE_NORMAL		0x00000080
#define INVALID_FILE_ATTRIBUTES		((DWORD)-1)
#define INVALID_FILE_SIZE			((DWORD)0xFFFFFFFF)
#define INVALID_SET_FILE_POINTER	((DWORD)-1)
#define FILE_BEGIN				0
#define FILE_CURRENT			1
#define FILE_END				2

#define ERROR_SUCCESS			0L
#define ERROR_FILE_NOT_FOUND	2L
#define ERROR_ALREADY_EXISTS	183L

/* ------------------------------------------------------------------------- */
/* 関数：ウィンドウ・メッセージ（w32_core.cpp）								 */
/* ------------------------------------------------------------------------- */
ATOM		RegisterClass(const WNDCLASS *pWc);
ATOM		RegisterClassEx(const WNDCLASSEX *pWc);
BOOL		GetClassInfo(HINSTANCE hInst, LPCSTR pszClass, LPWNDCLASS pWc);
BOOL		UnregisterClass(LPCSTR pszClass, HINSTANCE hInst);
HWND		CreateWindowEx(DWORD dwExStyle, LPCSTR pszClass, LPCSTR pszName, DWORD dwStyle,
				int x, int y, int cx, int cy, HWND hWndParent, HMENU hMenu, HINSTANCE hInst, LPVOID pParam);
#define		CreateWindow(c, n, s, x, y, w, h, p, m, i, a)	CreateWindowEx(0, c, n, s, x, y, w, h, p, m, i, a)
BOOL		DestroyWindow(HWND hWnd);
BOOL		ShowWindow(HWND hWnd, int nCmdShow);
BOOL		UpdateWindow(HWND hWnd);
BOOL		IsWindow(HWND hWnd);
BOOL		IsWindowVisible(HWND hWnd);
BOOL		IsIconic(HWND hWnd);
BOOL		IsWindowEnabled(HWND hWnd);
BOOL		EnableWindow(HWND hWnd, BOOL bEnable);
BOOL		MoveWindow(HWND hWnd, int x, int y, int cx, int cy, BOOL bRepaint);
BOOL		SetWindowPos(HWND hWnd, HWND hWndAfter, int x, int y, int cx, int cy, UINT uFlags);
/* 一括移動は、その場で 1 つずつ動かす */
HDWP		BeginDeferWindowPos(int nNumWindows);
HDWP		DeferWindowPos(HDWP hWinPosInfo, HWND hWnd, HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT uFlags);
BOOL		EndDeferWindowPos(HDWP hWinPosInfo);
BOOL		IsChild(HWND hWndParent, HWND hWnd);
BOOL		GetWindowRect(HWND hWnd, LPRECT pRect);
BOOL		GetClientRect(HWND hWnd, LPRECT pRect);
BOOL		ClientToScreen(HWND hWnd, LPPOINT pPt);
BOOL		ScreenToClient(HWND hWnd, LPPOINT pPt);
BOOL		AdjustWindowRect(LPRECT pRect, DWORD dwStyle, BOOL bMenu);
HWND		GetParent(HWND hWnd);
HWND		SetParent(HWND hWnd, HWND hWndParent);
HWND		GetDlgItem(HWND hWnd, int nID);
int			GetDlgCtrlID(HWND hWnd);
typedef BOOL (CALLBACK *WNDENUMPROC)(HWND, LPARAM);
BOOL		EnumChildWindows(HWND hWndParent, WNDENUMPROC pfn, LPARAM lParam);
int			GetClassName(HWND hWnd, LPSTR pszClass, int nMax);
LONG		GetWindowLong(HWND hWnd, int nIndex);
LONG		SetWindowLong(HWND hWnd, int nIndex, LONG lValue);
#define		GetWindowLongPtr	GetWindowLong
#define		SetWindowLongPtr	SetWindowLong
DWORD		GetClassLong(HWND hWnd, int nIndex);
DWORD		SetClassLong(HWND hWnd, int nIndex, LONG lValue);
BOOL		SetWindowText(HWND hWnd, LPCSTR pszText);
int			GetWindowText(HWND hWnd, LPSTR pszText, int nMax);
int			GetWindowTextLength(HWND hWnd);
HWND		SetFocus(HWND hWnd);
HWND		GetFocus(void);
HWND		SetActiveWindow(HWND hWnd);
HWND		GetActiveWindow(void);
BOOL		SetForegroundWindow(HWND hWnd);
HWND		GetForegroundWindow(void);
HWND		SetCapture(HWND hWnd);
BOOL		ReleaseCapture(void);
HWND		GetCapture(void);
HWND		FindWindow(LPCSTR pszClass, LPCSTR pszName);
BOOL		BringWindowToTop(HWND hWnd);
BOOL		FlashWindow(HWND hWnd, BOOL bInvert);

LRESULT		DefWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT		CallWindowProc(WNDPROC pfn, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT		SendMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL		PostMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void		PostQuitMessage(int nExitCode);
BOOL		PeekMessage(LPMSG pMsg, HWND hWnd, UINT uMin, UINT uMax, UINT uRemove);
BOOL		GetMessage(LPMSG pMsg, HWND hWnd, UINT uMin, UINT uMax);
BOOL		TranslateMessage(const MSG *pMsg);
LRESULT		DispatchMessage(const MSG *pMsg);
DWORD		MsgWaitForMultipleObjects(DWORD nCount, const HANDLE *pHandles, BOOL bWaitAll, DWORD dwMilliseconds, DWORD dwWakeMask);
BOOL		InvalidateRect(HWND hWnd, const RECT *pRect, BOOL bErase);
BOOL		ValidateRect(HWND hWnd, const RECT *pRect);
UINT_PTR	SetTimer(HWND hWnd, UINT_PTR nID, UINT uElapse, TIMERPROC pfn);
BOOL		KillTimer(HWND hWnd, UINT_PTR nID);
int			MessageBox(HWND hWnd, LPCSTR pszText, LPCSTR pszCaption, UINT uType);
BOOL		MessageBeep(UINT uType);

int			GetSystemMetrics(int nIndex);
short		GetKeyState(int nVirtKey);
short		GetAsyncKeyState(int nVirtKey);
BOOL		GetKeyboardState(PBYTE pKeyState);
BOOL		GetCursorPos(LPPOINT pPt);
HCURSOR		LoadCursor(HINSTANCE hInst, LPCSTR pszName);
HCURSOR		SetCursor(HCURSOR hCursor);
int			ShowCursor(BOOL bShow);
HICON		LoadIcon(HINSTANCE hInst, LPCSTR pszName);
HANDLE		LoadImage(HINSTANCE hInst, LPCSTR pszName, UINT uType, int cx, int cy, UINT fuLoad);
HMENU		LoadMenu(HINSTANCE hInst, LPCSTR pszName);
BOOL		SetMenu(HWND hWnd, HMENU hMenu);
HMENU		GetMenu(HWND hWnd);
BOOL		CheckMenuItem(HMENU hMenu, UINT uID, UINT uCheck);
BOOL		DrawMenuBar(HWND hWnd);
HMENU		CreateMenu(void);
HMENU		CreatePopupMenu(void);
BOOL		DestroyMenu(HMENU hMenu);
BOOL		AppendMenu(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCSTR pszNewItem);
BOOL		EnableMenuItem(HMENU hMenu, UINT uIDEnableItem, UINT uEnable);
HMENU		GetSubMenu(HMENU hMenu, int nPos);
int			GetMenuItemCount(HMENU hMenu);
UINT		GetMenuState(HMENU hMenu, UINT uID, UINT uFlags);

/* ------------------------------------------------------------------------- */
/* ダイアログ・コントロール（w32_dlg.cpp / w32_ctrl.cpp）						 */
/* ------------------------------------------------------------------------- */
#define MF_INSERT			0x00000000L
#define MF_CHANGE			0x00000080L
#define MF_APPEND			0x00000100L
#define MF_DELETE			0x00000200L
#define MF_REMOVE			0x00001000L
#define MF_BYCOMMAND		0x00000000L
#define MF_BYPOSITION		0x00000400L
#define MF_SEPARATOR		0x00000800L
#define MF_ENABLED			0x00000000L
#define MF_GRAYED			0x00000001L
#define MF_DISABLED			0x00000002L
#define MF_UNCHECKED		0x00000000L
#define MF_CHECKED			0x00000008L
#define MF_STRING			0x00000000L
#define MF_POPUP			0x00000010L
#define MF_MENUBARBREAK		0x00000020L
#define MF_MENUBREAK		0x00000040L
#define MF_HILITE			0x00000080L

typedef struct tagSCROLLINFO {
	UINT	cbSize;
	UINT	fMask;
	int		nMin;
	int		nMax;
	UINT	nPage;
	int		nPos;
	int		nTrackPos;
} SCROLLINFO, *LPSCROLLINFO;
typedef const SCROLLINFO *LPCSCROLLINFO;

typedef struct tagNMHDR {
	HWND		hwndFrom;
	UINT_PTR	idFrom;
	UINT		code;
} NMHDR, *LPNMHDR;

#define COLOR_SCROLLBAR		0
#define COLOR_BACKGROUND	1
#define COLOR_MENU			4
#define COLOR_WINDOWTEXT	8
#define COLOR_3DFACE		COLOR_BTNFACE
#define COLOR_BTNSHADOW		16
#define COLOR_GRAYTEXT		17
#define COLOR_BTNTEXT		18
#define COLOR_BTNHIGHLIGHT	20

HWND		CreateDialogParam(HINSTANCE hInst, LPCSTR pszTemplate, HWND hWndParent, DLGPROC pfnDlg, LPARAM lParam);
#define		CreateDialog(i, t, p, f)	CreateDialogParam((i), (t), (p), (f), 0)
INT_PTR		DialogBoxParam(HINSTANCE hInst, LPCSTR pszTemplate, HWND hWndParent, DLGPROC pfnDlg, LPARAM lParam);
#define		DialogBox(i, t, p, f)		DialogBoxParam((i), (t), (p), (f), 0)
BOOL		EndDialog(HWND hDlg, INT_PTR nResult);
LRESULT		DefDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL		IsDialogMessage(HWND hDlg, LPMSG pMsg);
BOOL		MapDialogRect(HWND hDlg, LPRECT pRect);
LONG		GetDialogBaseUnits(void);
UINT		GetDlgItemText(HWND hDlg, int nID, LPSTR pszText, int nMax);
BOOL		SetDlgItemText(HWND hDlg, int nID, LPCSTR pszText);
UINT		GetDlgItemInt(HWND hDlg, int nID, BOOL *pbTranslated, BOOL bSigned);
BOOL		SetDlgItemInt(HWND hDlg, int nID, UINT uValue, BOOL bSigned);
LRESULT		SendDlgItemMessage(HWND hDlg, int nID, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL		CheckDlgButton(HWND hDlg, int nID, UINT uCheck);
UINT		IsDlgButtonChecked(HWND hDlg, int nID);
BOOL		CheckRadioButton(HWND hDlg, int nFirst, int nLast, int nCheck);
HWND		GetNextDlgTabItem(HWND hDlg, HWND hCtl, BOOL bPrevious);

int			SetScrollPos(HWND hWnd, int nBar, int nPos, BOOL bRedraw);
int			GetScrollPos(HWND hWnd, int nBar);
BOOL		SetScrollRange(HWND hWnd, int nBar, int nMin, int nMax, BOOL bRedraw);
BOOL		GetScrollRange(HWND hWnd, int nBar, LPINT pnMin, LPINT pnMax);
int			SetScrollInfo(HWND hWnd, int nBar, LPCSCROLLINFO pSi, BOOL bRedraw);
BOOL		GetScrollInfo(HWND hWnd, int nBar, LPSCROLLINFO pSi);
BOOL		ShowScrollBar(HWND hWnd, int nBar, BOOL bShow);
BOOL		EnableScrollBar(HWND hWnd, UINT nBar, UINT uArrows);
DWORD		GetSysColor(int nIndex);
HBRUSH		GetSysColorBrush(int nIndex);

/* IME（ブラウザ側の入力欄に任せるので何もしない） */
HIMC		ImmGetContext(HWND hWnd);
BOOL		ImmReleaseContext(HWND hWnd, HIMC hImc);
BOOL		ImmSetOpenStatus(HIMC hImc, BOOL bOpen);
BOOL		ImmGetOpenStatus(HIMC hImc);
HIMC		ImmAssociateContext(HWND hWnd, HIMC hImc);

/* 矩形 */
BOOL		SetRect(LPRECT pRect, int left, int top, int right, int bottom);
BOOL		SetRectEmpty(LPRECT pRect);
BOOL		CopyRect(LPRECT pDst, const RECT *pSrc);
BOOL		OffsetRect(LPRECT pRect, int dx, int dy);
BOOL		InflateRect(LPRECT pRect, int dx, int dy);
BOOL		IntersectRect(LPRECT pDst, const RECT *pSrc1, const RECT *pSrc2);
BOOL		UnionRect(LPRECT pDst, const RECT *pSrc1, const RECT *pSrc2);
BOOL		PtInRect(const RECT *pRect, POINT pt);
BOOL		IsRectEmpty(const RECT *pRect);
BOOL		EqualRect(const RECT *pRect1, const RECT *pRect2);

/* 時間 */
DWORD		timeGetTime(void);
MMRESULT	timeBeginPeriod(UINT uPeriod);
MMRESULT	timeEndPeriod(UINT uPeriod);
MMRESULT	timeGetDevCaps(LPTIMECAPS pTc, UINT cbTc);
DWORD		GetTickCount(void);
void		Sleep(DWORD dwMilliseconds);
void		GetLocalTime(LPSYSTEMTIME pSt);
void		GetSystemTime(LPSYSTEMTIME pSt);
BOOL		QueryPerformanceCounter(LARGE_INTEGER *pCount);
BOOL		QueryPerformanceFrequency(LARGE_INTEGER *pFreq);

/* プロセス・同期（シングルスレッドなので形だけ） */
HANDLE		CreateMutex(LPSECURITY_ATTRIBUTES pAttr, BOOL bInitialOwner, LPCSTR pszName);
BOOL		ReleaseMutex(HANDLE hMutex);
/* スレッドは無いので作れない（0 を返す）。ブラウザ版の呼び出し側はスレッドを使わない */
uintptr_t	_beginthreadex(void *pSecurity, unsigned nStackSize, unsigned (*pfnStart)(void *), void *pArg, unsigned nInitFlag, unsigned *pThreadID);
HANDLE		CreateEvent(LPSECURITY_ATTRIBUTES pAttr, BOOL bManualReset, BOOL bInitialState, LPCSTR pszName);
BOOL		SetEvent(HANDLE hEvent);
BOOL		ResetEvent(HANDLE hEvent);
DWORD		WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
BOOL		CloseHandle(HANDLE hObject);
DWORD		GetLastError(void);
void		SetLastError(DWORD dwErr);
void		InitializeCriticalSection(LPCRITICAL_SECTION p);
void		DeleteCriticalSection(LPCRITICAL_SECTION p);
void		EnterCriticalSection(LPCRITICAL_SECTION p);
void		LeaveCriticalSection(LPCRITICAL_SECTION p);
DWORD		GetCurrentThreadId(void);
DWORD		GetCurrentProcessId(void);
void		OutputDebugString(LPCSTR pszText);
BOOL		IsDebuggerPresent(void);
HRESULT		CoInitialize(LPVOID pReserved);
HRESULT		CoInitializeEx(LPVOID pReserved, DWORD dwCoInit);
void		CoUninitialize(void);

/* 文字コード */
BOOL		IsDBCSLeadByte(BYTE byChar);
LPSTR		CharNext(LPCSTR psz);
LPSTR		CharPrev(LPCSTR pszStart, LPCSTR psz);

/* ------------------------------------------------------------------------- */
/* 関数：ファイル・INI・モジュール・リソース（w32_file.cpp）				 */
/* ------------------------------------------------------------------------- */
DWORD		GetModuleFileName(HMODULE hModule, LPSTR pszFileName, DWORD nSize);
/* パスのファイル名部分の先頭（区切りは \ と / のどちらでもよい） */
LPSTR		PathFindFileName(LPCSTR pszPath);
HMODULE		GetModuleHandle(LPCSTR pszName);
HMODULE		LoadLibrary(LPCSTR pszName);
BOOL		FreeLibrary(HMODULE hModule);
FARPROC		GetProcAddress(HMODULE hModule, LPCSTR pszName);
HRSRC		FindResource(HMODULE hModule, LPCSTR pszName, LPCSTR pszType);
HGLOBAL		LoadResource(HMODULE hModule, HRSRC hRes);
LPVOID		LockResource(HGLOBAL hResData);
DWORD		SizeofResource(HMODULE hModule, HRSRC hRes);
BOOL		FreeResource(HGLOBAL hResData);
int			LoadString(HINSTANCE hInst, UINT uID, LPSTR pszBuffer, int nMax);

UINT		GetPrivateProfileInt(LPCSTR pszApp, LPCSTR pszKey, INT nDefault, LPCSTR pszFile);
DWORD		GetPrivateProfileString(LPCSTR pszApp, LPCSTR pszKey, LPCSTR pszDefault, LPSTR pszRet, DWORD nSize, LPCSTR pszFile);
BOOL		WritePrivateProfileString(LPCSTR pszApp, LPCSTR pszKey, LPCSTR pszValue, LPCSTR pszFile);

HANDLE		CreateFile(LPCSTR pszName, DWORD dwAccess, DWORD dwShare, LPSECURITY_ATTRIBUTES pAttr,
				DWORD dwCreation, DWORD dwFlags, HANDLE hTemplate);
BOOL		ReadFile(HANDLE hFile, LPVOID pBuffer, DWORD dwSize, LPDWORD pdwRead, LPOVERLAPPED pOv);
BOOL		WriteFile(HANDLE hFile, LPCVOID pBuffer, DWORD dwSize, LPDWORD pdwWritten, LPOVERLAPPED pOv);
DWORD		SetFilePointer(HANDLE hFile, LONG lDistance, PLONG plDistanceHigh, DWORD dwMethod);
DWORD		GetFileSize(HANDLE hFile, LPDWORD pdwHigh);
BOOL		SetEndOfFile(HANDLE hFile);
BOOL		FlushFileBuffers(HANDLE hFile);
BOOL		DeleteFile(LPCSTR pszName);
BOOL		CopyFile(LPCSTR pszSrc, LPCSTR pszDst, BOOL bFailIfExists);
BOOL		MoveFile(LPCSTR pszSrc, LPCSTR pszDst);
BOOL		CreateDirectory(LPCSTR pszName, LPSECURITY_ATTRIBUTES pAttr);
DWORD		GetFileAttributes(LPCSTR pszName);
DWORD		GetCurrentDirectory(DWORD nSize, LPSTR pszBuffer);
BOOL		SetCurrentDirectory(LPCSTR pszName);
UINT		GetWindowsDirectory(LPSTR pszBuffer, UINT nSize);
DWORD		GetTempPath(DWORD nSize, LPSTR pszBuffer);

/* ------------------------------------------------------------------------- */
/* 関数：GDI（w32_gdi.cpp）													 */
/* ------------------------------------------------------------------------- */
HDC			GetDC(HWND hWnd);
HDC			GetWindowDC(HWND hWnd);
int			ReleaseDC(HWND hWnd, HDC hDC);
HDC			BeginPaint(HWND hWnd, LPPAINTSTRUCT pPs);
BOOL		EndPaint(HWND hWnd, const PAINTSTRUCT *pPs);
HDC			CreateCompatibleDC(HDC hDC);
BOOL		DeleteDC(HDC hDC);
HBITMAP		CreateCompatibleBitmap(HDC hDC, int cx, int cy);
HBITMAP		CreateDIBSection(HDC hDC, const BITMAPINFO *pBmi, UINT uUsage, void **ppBits, HANDLE hSection, DWORD dwOffset);
HBITMAP		CreateBitmap(int cx, int cy, UINT cPlanes, UINT cBitsPerPel, const void *pBits);
int			GetDIBits(HDC hDC, HBITMAP hBmp, UINT uStart, UINT cLines, LPVOID pBits, LPBITMAPINFO pBmi, UINT uUsage);
int			SetDIBitsToDevice(HDC hDC, int xDest, int yDest, DWORD w, DWORD h, int xSrc, int ySrc,
				UINT uStartScan, UINT cLines, const void *pBits, const BITMAPINFO *pBmi, UINT uColorUse);
int			StretchDIBits(HDC hDC, int xDest, int yDest, int wDest, int hDest, int xSrc, int ySrc, int wSrc, int hSrc,
				const void *pBits, const BITMAPINFO *pBmi, UINT uUsage, DWORD dwRop);
HGDIOBJ		SelectObject(HDC hDC, HGDIOBJ hObj);
BOOL		DeleteObject(HGDIOBJ hObj);
HGDIOBJ		GetStockObject(int nObject);
int			GetObject(HGDIOBJ hObj, int cbBuffer, LPVOID pv);
HBRUSH		CreateSolidBrush(COLORREF cl);
HPEN		CreatePen(int nStyle, int nWidth, COLORREF cl);
HFONT		CreateFont(int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight,
				DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet,
				DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality,
				DWORD fdwPitchAndFamily, LPCSTR pszFace);
HFONT		CreateFontIndirect(const LOGFONT *pLf);
BOOL		BitBlt(HDC hDest, int x, int y, int cx, int cy, HDC hSrc, int sx, int sy, DWORD dwRop);
BOOL		StretchBlt(HDC hDest, int x, int y, int cx, int cy, HDC hSrc, int sx, int sy, int scx, int scy, DWORD dwRop);
BOOL		PatBlt(HDC hDC, int x, int y, int cx, int cy, DWORD dwRop);
int			FillRect(HDC hDC, const RECT *pRect, HBRUSH hBrush);
int			FrameRect(HDC hDC, const RECT *pRect, HBRUSH hBrush);
BOOL		Rectangle(HDC hDC, int left, int top, int right, int bottom);
BOOL		MoveToEx(HDC hDC, int x, int y, LPPOINT pPt);
BOOL		LineTo(HDC hDC, int x, int y);
COLORREF	SetPixel(HDC hDC, int x, int y, COLORREF cl);
COLORREF	GetPixel(HDC hDC, int x, int y);
COLORREF	SetTextColor(HDC hDC, COLORREF cl);
COLORREF	GetTextColor(HDC hDC);
COLORREF	SetBkColor(HDC hDC, COLORREF cl);
COLORREF	GetBkColor(HDC hDC);
int			SetBkMode(HDC hDC, int nMode);
int			GetBkMode(HDC hDC);
BOOL		TextOut(HDC hDC, int x, int y, LPCSTR pszText, int nLen);
BOOL		W32VerifyMode(void);						/* 検査モード（テスト用。?verify=1） */
void		W32VerifySkipCandidate(void);				/* 「次に描く画面は前と同じはず」と宣言する（違えば検査の失敗） */
BOOL		TextOutOutline(HDC hDC, int x, int y, LPCSTR pszText, int nLen, COLORREF clOutline, COLORREF clText);	/* 黒縁取り文字（TextOut 5 回分と同じ画素） */
void		W32ShadeSpan(HDC hDC, int x, int y, int cx, int nFade, COLORREF cl, int nAlpha);	/* 横 1 行を半透明で塗り、右端 nFade ドットをぼかす（戦闘ログの帯） */
int			DrawText(HDC hDC, LPCSTR pszText, int nLen, LPRECT pRect, UINT uFormat);
BOOL		GetTextExtentPoint32(HDC hDC, LPCSTR pszText, int nLen, LPSIZE pSize);
#define		GetTextExtentPoint	GetTextExtentPoint32
BOOL		GetTextMetrics(HDC hDC, LPTEXTMETRIC pTm);
int			GetDeviceCaps(HDC hDC, int nIndex);
int			SetStretchBltMode(HDC hDC, int nMode);
#define		COLORONCOLOR	3
#define		HALFTONE		4
#define		BITSPIXEL		12

/* ------------------------------------------------------------------------- */
/* Web版専用の補助関数（互換レイヤの外から呼ぶもの）						 */
/* ------------------------------------------------------------------------- */
/* SJIS 文字列を UTF-8 に変換する。戻り値は書き込んだバイト数 */
int			W32SjisToUtf8(const char *pszSjis, int nLen, char *pszOut, int nOutSize);
/* UTF-8 文字列を SJIS に変換する（変換できない文字は '?'） */
int			W32Utf8ToSjis(const char *pszUtf8, char *pszOut, int nOutSize);
/* リソースの元ファイルの仮想ファイルシステム上のパス（無ければ NULL） */
const char	*W32GetResourcePath(HRSRC hRes);
/* 書き換えたファイルを IndexedDB へ書き戻す（少し待ってまとめて実行） */
void		W32SyncFiles(void);
/* トップレベルのウィンドウに WM_CLOSE を送るよう頼む（シグナルハンドラから呼んでよい） */
void		W32RequestClose(void);
/* 「ちょっと休憩」（MsgWaitForMultipleObjects など）を頼まれたミリ秒を取り出す（無ければ -1）
   Node.js 版サーバーはメインループを 1 回分ずつ回すので、頼まれたらそこで一度戻る */
int			W32TakeYieldRequest(void);
/* timeGetTime を同じ値に止める（W32BeginTimeCache で今の時刻を覚え、W32EndTimeCache まで同じ値を返す）。
   サーバーはキャラ 1 体の処理の中で何度も時刻を取るので、その間は 1 回で済ませて CPU を節約する */
void		W32BeginTimeCache(void);
void		W32EndTimeCache(void);
#ifdef W32_BROWSER
/* 処理時間の計測（web/w32_perf.cpp）。URL に ?perf=1 を付けた時だけ測って画面の左上に出す */
enum {
	W32PERF_FRAME = 0,		/* メインループ 1 回分 */
	W32PERF_LOGIC,			/* そのうち描画以外 */
	W32PERF_PAINT,			/* メインウィンドウの WM_PAINT 1 回分 */
	W32PERF_PRESENT,		/* canvas への転送 1 回分 */
	W32PERF_COUNT
};
BOOL		W32PerfEnabled(void);
double		W32PerfNow(void);					/* 無効の時は 0 */
void		W32PerfAdd(int nKind, double dMs);
void		W32PerfBeginFrame(void);
void		W32PerfEndFrame(void);				/* 1 秒ごとにまとめて表示する */
#endif
