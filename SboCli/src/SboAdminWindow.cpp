/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOCLI													 */
/* 概要：			管理者ウィンドウ										 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/04/29												 */
/* ========================================================================= */
#include "stdafx.h"
#include <commctrl.h>
#include "Glbldata.h"
#include "SboCliSendCmd.h"
#include "SboCliRecvCmd.h"

#include "AdminWindowOld/AdminDlgJobInfo.h"
#include "AdminWindowOld/AdminDlgMagicInfo.h"
#include "AdminWindowOld/AdminDlgMagicUserInfo.h"
#include "AdminWindowOld/AdminDlgCharMagicInfo.h"
#include "AdminWindowOld/AdminDlgEquUserInfo.h"
#include "AdminWindowOld/AdminDlgAccSearch.h"
#include "AdminWindowOld/AdminDlgEtcSubGame.h"
#include "AdminWindowOld/SelectCharGrpWindow.h"
#include "AdminWindowOld/AdminAutoLayout.h"
#include "AdminWindowOld/AdminDlgMapLight.h"
#include "AdminWindowOld/AdminDlgStTmpInfo.h"
#include "AdminWindowOld/AdminDlgNpcRoute.h"

/* ========================================================================= */
/* 定数定義																	 */
/* ========================================================================= */
#define MAPINFO_LIGHTBTN_Y		190									/* マップ情報:「明るさ・灯り・見え方の設定」ボタンの位置 */
#define MAPINFO_SUMMARY_Y		216									/* マップ情報: 明るさ・灯り・見え方の一覧の位置 */
#define MAPINFO_SUMMARY_GAP		16									/* マップ情報: 一覧の段の間 */
#define CLNAME "SboAdminWindow"									/* 登録クラス名 */

/* ボタンID */
enum {
	IDC_ADDPARTS = 1,											/* パーツ追加 */
	IDC_SENDPARTS,												/* サーバーに送信 */
	IDC_RELOADPARTS,											/* 更新 */
	IDC_ADDMAPUP,												/* マップ上に追加 */
	IDC_ADDMAPDOWN,												/* マップ下に追加 */
	IDC_ADDMAPLEFT,												/* マップ左に追加 */
	IDC_ADDMAPRIGHT,											/* マップ右に追加 */
	IDC_SAVEMAP,												/* マップ保存 */
	IDC_ADDEVENT,												/* イベント追加 */
	IDC_SENDEVENT,												/* サーバーに送信 */
	IDC_GETEVENT,												/* イベント取得 */
	IDC_TMPCOMBO1,												/* コンボボックス１ */
	IDC_TMPCOMBO2,												/* コンボボックス２ */
	IDC_TMPCOMBO3,												/* コンボボックス３ */
	IDC_TMPCOMBO4,												/* コンボボックス４ */
	IDC_TMPCOMBO5,												/* コンボボックス５ */
	IDC_TMPEDIT1,												/* エディット１ */
	IDC_TMPEDIT2,												/* エディット２ */
	IDC_TMPEDIT3,												/* エディット３ */
	IDC_TMPEDIT4,												/* エディット４ */
	IDC_TMPEDIT5,												/* エディット５ */
	IDC_TMPEDIT6,												/* エディット６ */
	IDC_TMPEDIT7,												/* エディット７ */
	IDC_TMPEDIT8,												/* エディット８ */
	IDC_TMPEDIT9,												/* エディット９ */
	IDC_TMPEDIT10,												/* エディット１０ */
	IDC_TMPEDIT11,												/* エディット１１ */
	IDC_TMPCHECK1,												/* チェック１ */
	IDC_TMPCHECK2,												/* チェック２ */
	IDC_TMPCHECK3,												/* チェック３ */
	IDC_TMPCHECK4,												/* チェック４ */
	IDC_TMPCHECK5,												/* チェック５ */
	IDC_TMPCHECK6,												/* チェック６ */
	IDC_TMPCHECK7,												/* チェック７ */
	IDC_BUTTON11,												/* ボタン１１ */
	IDC_BUTTON12,												/* ボタン１２ */
	IDC_BUTTON13,												/* ボタン１３ */
	IDC_BUTTON14,												/* ボタン１４ */
	IDC_BUTTON15,												/* ボタン１５ */
	IDC_BUTTON16,												/* ボタン１６ */
	IDC_SCRBAR1,												/* スクロールバー１ */
	IDC_LIST2,													/* リストコントロール２ */
	IDC_RENEW,
	IDC_TMPCHECK8,												/* チェック８ */
};

/* ========================================================================= */
/* 構造体定義																 */
/* ========================================================================= */

/* ========================================================================= */
/* グローバル変数定義														 */
/* ========================================================================= */
static HWND
	m_hWndDialog,												/* ダイアログ */
	m_hWndMapPartsAddParts,										/* パーツ追加 */
	m_hWndMapPartsSendParts,									/* サーバーに送信 */
	m_hWndMapPartsReloadParts,									/* 更新 */
	m_hWndMapPartsScrGrp,										/* スクロール：グラフィック */
	m_hWndMapPartsScrParts,										/* スクロール：パーツ */
	m_hWndMapAddUp,												/* マップ上に追加 */
	m_hWndMapAddDown,											/* マップ下に追加 */
	m_hWndMapAddLeft,											/* マップ左に追加 */
	m_hWndMapAddRight,											/* マップ右に追加 */
	m_hWndMapAddDelCount,										/* マップ増減幅 */
	m_hWndMapSave,												/* マップ保存 */
	m_hWndEventAdd,												/* イベント追加 */
	m_hWndEventSend,											/* サーバーに送信 */
	m_hWndEventList,											/* 種別リスト */
	m_hWndEventGet,												/* イベント取得 */
	m_hWndEdit1,												/* エディット１ */
	m_hWndEdit2,												/* エディット２ */
	m_hWndEdit3,												/* エディット３ */
	m_hWndEdit4,												/* エディット４ */
	m_hWndEdit5,												/* エディット５ */
	m_hWndEdit6,												/* エディット６ */
	m_hWndEdit7,												/* エディット７ */
	m_hWndEdit8,												/* エディット８ */
	m_hWndEdit9,												/* エディット９ */
	m_hWndEdit10,												/* エディット１０ */
	m_hWndEdit11,												/* エディット１１ */
	m_hWndEdit12,												/* エディット１２ */
	m_hWndEdit13,												/* エディット１３ */
	m_hWndEdit14,												/* エディット１４ */
	m_hWndEdit15,												/* エディット１５ */
	m_hWndEdit16,												/* エディット１６ */
	m_hWndEdit17,												/* エディット１７ */
	m_hWndEdit18,												/* エディット１８ */
	m_hWndEdit19,												/* エディット１９ */
	m_hWndEdit20,												/* エディット２０ */
	m_hWndEdit21,												/* エディット２１ */
	m_hWndEdit22,												/* エディット２２ */
	m_hWndEdit23,												/* エディット２３ */
	m_hWndEdit24,												/* エディット２４ */
	m_hWndEdit25,												/* エディット２５ */
	m_hWndCheck1,												/* チェック１ */
	m_hWndCheck2,												/* チェック２ */
	m_hWndCheck3,												/* チェック３ */
	m_hWndCheck4,												/* チェック４ */
	m_hWndCheck5,												/* チェック５ */
	m_hWndCheck6,												/* チェック６ */
	m_hWndCheck7,												/* チェック７ */
	m_hWndCheck8,												/* チェック８ */
	m_hWndButton1,												/* ボタン１ */
	m_hWndButton2,												/* ボタン２ */
	m_hWndButton3,												/* ボタン３ */
	m_hWndButton4,												/* ボタン４ */
	m_hWndButton5,												/* ボタン５ */
	m_hWndButton6,												/* ボタン６ */
	m_hWndButton7,												/* ボタン７ */
	m_hWndButton8,												/* ボタン８ */
	m_hWndButton9,												/* ボタン９ */
	m_hWndButton10,												/* ボタン１０ */
	m_hWndButton11,												/* ボタン１１ */
	m_hWndButton12,												/* ボタン１２ */
	m_hWndButton13,												/* ボタン１３ */
	m_hWndButton14,												/* ボタン１４ */
	m_hWndCombo1,												/* コンボボックス１ */
	m_hWndCombo2,												/* コンボボックス２ */
	m_hWndCombo3,												/* コンボボックス３ */
	m_hWndCombo4,												/* コンボボックス４ */
	m_hWndCombo5,												/* コンボボックス５ */
	m_hWndScrBar1,												/* スクロールバー１ */
	m_hWndList1,												/* リストコントロール１ */
	m_hWndList2;												/* リストコントロール２ */

PCMDPARA_ADDCHARINFO m_pAddCharInfo;							/* キャラ追加用テンポラリ */
PCMDPARA_CHARINFO m_pParaCharInfo;								/* キャラ情報用テンポラリ */
PCHARINFO m_pCharInfo;											/* キャラ情報テンポラリ */
DWORD m_dwSelectID,												/* 選択IDテンポラリ */
	  m_dwSelectID2,											/* 選択IDテンポラリ２ */
	  m_dwSelectpID;											/* 選択IDのID */
PCMDPARA_SVRINFO m_pSvrInfo;									/* サーバー情報 */

/* ========================================================================= */
/* 関数プロトタイプ宣言														 */
/* ========================================================================= */
void MyTextOut(HDC, int, int, char *, COLORREF Color = RGB (255, 255, 255));	/* 文字描画 */
void MakeAdminWindow				(void);								/* 管理者ウィンドウ作成 */
LRESULT WINAPI WndProcAdminWindow	(HWND, UINT, WPARAM, LPARAM);		/* メッセージの処理 */
static LRESULT WndProcAdminWindowSub	(HWND, UINT, WPARAM, LPARAM);	/* メッセージの処理（本体） */
static int   GetAdminBgmCount		(void);								/* BGM一覧の数（MIDIの曲＋音声ファイルの曲） */
static DWORD GetAdminBgmNo			(int);								/* BGM一覧の何番目かからBGM番号を求める */
LRESULT CALLBACK WndProcDialog		(HWND, UINT, WPARAM, LPARAM);		/* ダイアログのメッセージ処理 */
void AdminOnCommand					(HWND, int, HWND, UINT);			/* メッセージハンドラ(WM_COMMAND) */
void AdminOnCommandMapParts			(HWND, int, HWND, UINT);			/* メッセージハンドラ(WM_COMMAND)パーツ情報) */
void AdminOnCommandMapInfo			(HWND, int, HWND, UINT);			/* メッセージハンドラ(WM_COMMAND)マップ情報 */
void AdminOnCommandMapEvent			(HWND, int, HWND, UINT);			/* メッセージハンドラ(WM_COMMAND)イベント情報 */
void AdminOnCommandAddChar			(HWND, int, HWND, UINT);			/* メッセージハンドラ(WM_COMMAND)キャラ追加 */
void AdminOnCommandCharInfo			(HWND, int, HWND, UINT);			/* メッセージハンドラ(WM_COMMAND)キャラ情報 */
static void RecordAdminRuraEdit		(HWND);								/* 管理人ルーラの入力欄を記録して保存 */
BOOL SaveAdminRura					(void);								/* 管理人ルーラ情報保存(SboCliMain.cpp) */
void AdminOnCommandCharStatus		(HWND, int, HWND, UINT);			/* メッセージハンドラ(WM_COMMAND)キャラステータス情報 */
void AdminOnCommandCharLeave		(HWND, int, HWND, UINT);			/* メッセージハンドラ(WM_COMMAND)預かり所情報 */
void AdminOnCommandAddMap			(HWND, int, HWND, UINT);			/* メッセージハンドラ(WM_COMMAND)マップ追加 */
void AdminOnCommandMsg				(HWND, int, HWND, UINT);			/* メッセージハンドラ(WM_COMMAND)メッセージ */
void AdminOnCommandAccAdd			(HWND, int, HWND, UINT);			/* メッセージハンドラ(WM_COMMAND)アカウント追加 */
void AdminOnCommandAccInfo			(HWND, int, HWND, UINT);			/* メッセージハンドラ(WM_COMMAND)アカウント編集 */
void AdminOnCommandEtcVersion		(HWND, int, HWND, UINT);			/* メッセージハンドラ(WM_COMMAND)バージョン通知 */
void AdminOnCommandCefEdit			(HWND, int, HWND, UINT);			/* メッセージハンドラ(WM_COMMAND)吹き出し設定 */
void AdminOnCommandItemList			(HWND, int, HWND, UINT);			/* メッセージハンドラ(WM_COMMAND)アイテム一覧 */
void AdminOnCommandCharItem			(HWND, int, HWND, UINT);			/* メッセージハンドラ(WM_COMMAND)キャラアイテム編集 */
void AdminOnCommandSvrInfo			(HWND, int, HWND, UINT);			/* メッセージハンドラ(WM_COMMAND)サーバー情報 */
void AdminOnCommandShopInfo			(HWND, int, HWND, UINT);			/* メッセージハンドラ(WM_COMMAND)店情報 */
void AdminOnCommandEfcEdit			(HWND, int, HWND, UINT);			/* メッセージハンドラ(WM_COMMAND)エフェクト設定 */
void AdminOnCommandSubGameInfo		(HWND, int, HWND, UINT);			/* メッセージハンドラ(WM_COMMAND)サブゲーム情報 */
void AdminOnPaint					(HWND);								/* メッセージハンドラ(WM_PAINT) */
void AdminOnHScroll					(HWND, HWND, UINT, int);			/* メッセージハンドラ(WM_HSCROLL) */
void AdminOnVScroll					(HWND, HWND, UINT, int);			/* メッセージハンドラ(WM_VSCROLL) */
void AdminOnLButtonDown				(HWND, BOOL, int, int, UINT);		/* メッセージハンドラ(WM_LBUTTONDOWN) */
void AdminOnRButtonDown				(HWND, BOOL, int, int, UINT);		/* メッセージハンドラ(WM_RBUTTONDOWN) */
void AdminOnPaintMapParts			(HDC);								/* メッセージハンドラ(WM_PAINT)マップパーツ編集ウィンドウ */
void AdminOnPaintMapInfo			(HDC);								/* メッセージハンドラ(WM_PAINT)マップ情報ウィンドウ */
void AdminOnPaintMapEvent			(HDC);								/* メッセージハンドラ(WM_PAINT)イベント情報ウィンドウ */
void AdminOnPaintAddChar			(HDC);								/* メッセージハンドラ(WM_PAINT)キャラ追加ウィンドウ */
void AdminOnPaintCharInfo			(HDC);								/* メッセージハンドラ(WM_PAINT)キャラ情報ウィンドウ */
void AdminOnPaintCharStatus			(HDC);								/* メッセージハンドラ(WM_PAINT)キャラステータス情報ウィンドウ */
void AdminOnPaintMsg				(HDC);								/* メッセージハンドラ(WM_PAINT)メッセージウィンドウ */
void AdminOnPaintAccAdd				(HDC);								/* メッセージハンドラ(WM_PAINT)アカウント追加ウィンドウ */
void AdminOnPaintAccInfo			(HDC);								/* メッセージハンドラ(WM_PAINT)アカウント編集ウィンドウ */
void AdminOnPaintCefEdit			(HDC);								/* メッセージハンドラ(WM_PAINT)吹き出し設定ウィンドウ */
void AdminOnPaintItemList			(HDC);								/* メッセージハンドラ(WM_PAINT)アイテム一覧ウィンドウ */
void AdminOnPaintCharItem			(HDC);								/* メッセージハンドラ(WM_PAINT)所持アイテム情報ウィンドウ */
void AdminOnPaintSvrInfo			(HDC);								/* メッセージハンドラ(WM_PAINT)サーバー情報ウィンドウ */
void AdminOnPaintShopInfo			(HDC);								/* メッセージハンドラ(WM_PAINT)店情報ウィンドウ */
void AdminOnPaintEfcEdit			(HDC);								/* メッセージハンドラ(WM_PAINT)エフェクト設定ウィンドウ */
void AdminOnPaintSubGame			(HDC);								/* メッセージハンドラ(WM_PAINT)サブゲーム情報ウィンドウ */
void AdminOnPaintEtcVersion			(HDC);								/* メッセージハンドラ(WM_PAINT)バージョン通知ウィンドウ */
void AdminOnHScrollMapParts			(HWND, UINT, int);					/* メッセージハンドラ(WM_HSCROLL)マップパーツ編集ウィンドウ */
void AdminOnVScrollMapParts			(HWND, UINT, int);					/* メッセージハンドラ(WM_VSCROLL)マップパーツ編集ウィンドウ */
void AdminOnHScrollAddChar			(HWND, UINT, int);					/* メッセージハンドラ(WM_HSCROLL)キャラ追加ウィンドウ */
void AdminOnHScrollCefEdit			(HWND, UINT, int);					/* メッセージハンドラ(WM_HSCROLL)吹き出し設定ウィンドウ */
void AdminOnHScrollEfcEdit			(HWND, UINT, int);					/* メッセージハンドラ(WM_HSCROLL)エフェクト設定ウィンドウ */
void AdminOnLButtonDownMapParts		(BOOL, int, int, UINT);				/* メッセージハンドラ(WM_LBUTTONDOWN)マップパーツ編集ウィンドウ */
void AdminOnRButtonDownMapParts		(BOOL, int, int, UINT);				/* メッセージハンドラ(WM_RBUTTONDOWN)マップパーツ編集ウィンドウ */
void AdminOnLButtonDownAddChar		(BOOL, int, int, UINT);				/* メッセージハンドラ(WM_LBUTTONDOWN)キャラ追加ウィンドウ */
void AdminOnLButtonDownCharInfo		(BOOL, int, int, UINT);				/* メッセージハンドラ(WM_LBUTTONDOWN)キャラ情報ウィンドウ */
void AdminOnLButtonDownCefEdit		(BOOL, int, int, UINT);				/* メッセージハンドラ(WM_LBUTTONDOWN)吹き出し設定ウィンドウ */
void AdminOnLButtonDownEfcEdit		(BOOL, int, int, UINT);				/* メッセージハンドラ(WM_LBUTTONDOWN)エフェクト設定ウィンドウ */
void AdminOnSelectCharGrpCharInfo	(int nCode, int nMain, int nID);	/* メッセージハンドラ(WM_ADMINSELECTCHARGRP)キャラ情報ウィンドウ */
HWND CreateWindowEdit				(int, int, int, int, DWORD);		/* エディットボックスウィンドウ作成 */
void MakeAdminWnd					(int);								/* 各ウィンドウ作成 */
void DeleteAdminWnd					(void);								/* 各ウィンドウ消去 */
void MakeAdminWndMapParts			(void);								/* マップパーツ編集ウィンドウ作成 */
void DeleteAdminWndMapParts			(void);								/* マップパーツ編集ウィンドウ消去 */
void MakeAdminWndMapInfo			(void);								/* マップ情報ウィンドウ作成 */
void DeleteAdminWndMapInfo			(void);								/* マップ情報ウィンドウ消去 */
void AdminMapInfoRefresh			(void);								/* マップ情報画面の表示を直す */
static int  AdminMapInfoContentHeight	(void);							/* マップ情報画面の中身の高さ */
void MakeAdminWndEventInfo			(void);								/* イベント情報ウィンドウ作成 */
void DeleteAdminWndEventInfo		(void);								/* イベント情報ウィンドウ消去 */
void MakeAdminWndEventInfoType		(void);								/* 種類別イベント情報ウィンドウ作成 */
void DeleteAdminWndEventInfoType	(void);								/* 種類別イベント情報ウィンドウ消去 */
void MakeAdminWndAddChar			(void);								/* キャラ追加ウィンドウ作成 */
void DeleteAdminWndAddChar			(void);								/* キャラ追加ウィンドウ消去 */
void MakeAdminWndCharInfo			(void);								/* キャラ情報ウィンドウ作成 */
void DeleteAdminWndCharInfo			(void);								/* キャラ情報ウィンドウ消去 */
void MakeAdminWndCharStatus			(void);								/* キャラステータス情報ウィンドウ作成 */
void DeleteAdminWndCharStatus		(void);								/* キャラステータス情報ウィンドウ消去 */
void MakeAdminWndCharLeave			(void);								/* 預かり所情報ウィンドウ作成 */
void DeleteAdminWndCharLeave		(void);								/* 預かり所情報ウィンドウ消去 */
void MakeAdminWndAddMap				(void);								/* マップ追加ウィンドウ作成 */
void DeleteAdminWndAddMap			(void);								/* マップ追加ウィンドウ消去 */
void MakeAdminWndMsg				(void);								/* メッセージ情報ウィンドウ作成 */
void DeleteAdminWndMsg				(void);								/* メッセージ情報ウィンドウ消去 */
void MakeAdminWndAccAdd				(void);								/* アカウント追加ウィンドウ作成 */
void DeleteAdminWndAccAdd			(void);								/* アカウント追加ウィンドウ消去 */
void MakeAdminWndAccInfo			(void);								/* アカウント編集ウィンドウ作成 */
void DeleteAdminWndAccInfo			(void);								/* アカウント編集ウィンドウ消去 */
void MakeAdminWndCefEdit			(void);								/* 吹き出し設定ウィンドウ作成 */
void DeleteAdminWndCefEdit			(void);								/* 吹き出し設定ウィンドウ消去 */
void MakeAdminWndEtcVersion			(void);								/* バージョン通知ウィンドウ作成 */
void DeleteAdminWndEtcVersion		(void);								/* バージョン通知ウィンドウ消去 */
void MakeAdminWndItemList			(void);								/* アイテム一覧ウィンドウ作成 */
void DeleteAdminWndItemList			(void);								/* アイテム一覧ウィンドウ消去 */
void MakeItemEditWindow				(BYTE);								/* 種類別アイテム編集ウィンドウ作成 */
void DeleteItemEditWindow			(void);								/* 種類別アイテム編集ウィンドウ消去 */
void MakeAdminWndCharItem			(void);								/* キャラアイテム編集ウィンドウ作成 */
void DeleteAdminWndCharItem			(void);								/* キャラアイテム編集ウィンドウ消去 */
void MakeAdminWndSvrInfo			(void);								/* サーバー情報ウィンドウ作成 */
void DeleteAdminWndSvrInfo			(void);								/* サーバー情報ウィンドウ消去 */
void MakeAdminWndShopInfo			(void);								/* 店情報ウィンドウ作成 */
void DeleteAdminWndShopInfo			(void);								/* 店情報ウィンドウ消去 */
void MakeAdminWndEfcEdit			(void);								/* エフェクト設定ウィンドウ作成 */
void DeleteAdminWndEfcEdit			(void);								/* エフェクト設定ウィンドウ消去 */
void MakeAdminWndSubGameInfo		(void);								/* サブゲーム情報ウィンドウ作成 */
void DeleteAdminWndSubGameInfo		(void);								/* サブゲーム情報ウィンドウ消去 */
void MakeAdminWndSubGameInfoType	(void);								/* 種類別サブゲーム情報ウィンドウ作成 */
void DeleteAdminWndSubGameInfoType	(void);								/* 種類別サブゲーム情報ウィンドウ消去 */
static BYTE AdminUndoScreenFromType	(int);								/* 「元に戻す」の画面番号(ADMINSCR_*)を取得 */
static void AdminUndoNotifyScreen	(void);								/* 開いている画面をサーバーに知らせる */
static void AdminUndoRequest		(void);								/* 「この画面の変更を元に戻す」 */
static void AdminUndoRefresh		(BYTE);								/* 元に戻した後に画面を作り直す */

/* サーバーに知らせた画面（0xFF ならまだ知らせていない） */
static BYTE s_byAdminUndoScreen = 0xFF;

/* ========================================================================= */
/* 関数名：GetAdminBgmCount													 */
/* 内容：BGM一覧の数（MIDIの曲＋音声ファイルの曲）							 */
/* ========================================================================= */
static int GetAdminBgmCount(void)
{
	int nCount = 0;

	for (int i = 0; adwBgmNoTbl[i] != 0xFFFFFFFF; i ++) {
		nCount ++;
	}
	return nCount + BGM_AUDIO_COUNT;
}

/* ========================================================================= */
/* 関数名：GetAdminBgmNo													 */
/* 内容：BGM一覧の何番目かからBGM番号を求める（末尾は音声ファイルの曲）		 */
/* ========================================================================= */
static DWORD GetAdminBgmNo(int nIndex)
{
	int nMidi = GetAdminBgmCount () - BGM_AUDIO_COUNT;

	if (nIndex < 0) {
		return 0;
	}
	if (nIndex >= nMidi) {
		nIndex -= nMidi;
		return (nIndex < BGM_AUDIO_COUNT) ? g_aBgmAudio[nIndex].wNo : 0;
	}
	return adwBgmNoTbl[nIndex];
}

/* ========================================================================= */
/* 関数名：MyTextOut														 */
/* 内容：文字描画															 */
/* 日付：2002/07/15															 */
/* ========================================================================= */
void MyTextOut(HDC hdc, int iX, int iY, char *pStr, COLORREF Color)
{
	SetTextColor (hdc, Color);
	TextOut (hdc, iX, iY, pStr, strlen (pStr));
}

/* ========================================================================= */
/* 関数名：MakeAdminWindow													 */
/* 内容：管理者ウィンドウ作成												 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void MakeAdminWindow(void)
{
	WNDCLASS wc;
	RECT rect;

	if (g_hWndAdmin) {
		return;
	}

	wc.hInstance = g_hInstance;
	wc.lpszClassName = CLNAME;
	wc.lpfnWndProc = (WNDPROC)WndProcAdminWindow;
	wc.style = 0;
	wc.hIcon = LoadIcon ((HINSTANCE)NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor ((HINSTANCE)NULL, IDC_ARROW);
	wc.lpszMenuName = "IDR_MENU1";
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)GetStockObject (BLACK_BRUSH);
	/* ウィンドウのクラスを登録 */
	RegisterClass (&wc);

	if (rcAdmin.right && rcAdmin.bottom) {
		SetRect (&rect, rcAdmin.left, rcAdmin.top, 400, 350);

	} else {
		RECT rcTmp;
		GetWindowRect (g_hWndMain, &rcTmp);
		SetRect (&rect, rcTmp.right, rcTmp.top, 400, 350);
	}

	/* ウィンドウ作成 */
	g_hWndAdmin = CreateWindow (CLNAME, "管理者用ウィンドウ",
				WS_OVERLAPPED | WS_THICKFRAME | WS_CLIPCHILDREN,
				rect.left, rect.top, rect.right, rect.bottom,
				g_hWndMain, NULL, g_hInstance, NULL);
	/* 400x350 を元の大きさとして覚え、前回広げていたらその大きさに戻す */
	AdminAutoLayoutInit (g_hWndAdmin);
	if (rcAdmin.right && rcAdmin.bottom) {
		int cx = rcAdmin.right - rcAdmin.left;
		int cy = rcAdmin.bottom - rcAdmin.top;
		if (cx > rect.right || cy > rect.bottom) {
			SetWindowPos (g_hWndAdmin, NULL, 0, 0, (cx > rect.right) ? cx : rect.right, (cy > rect.bottom) ? cy : rect.bottom,
					SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		}
	}
	ShowWindow (g_hWndAdmin, SW_SHOWNOACTIVATE);

	s_byAdminUndoScreen = 0xFF;
	AdminUndoNotifyScreen ();
}

/* ========================================================================= */
/* 関数名：WndProcAdminWindow												 */
/* 内容：メッセージの処理													 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
LRESULT WINAPI WndProcAdminWindow(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	/* 画面が高くて入りきらない時の縦スクロール（ホイール・右端のスクロールバー） */
	if ((msg == WM_MOUSEWHEEL || msg == WM_VSCROLL) && AdminAutoLayoutOnScrollMsg (hWnd, msg, wParam, lParam)) {
		return 0;
	}
	/* スクロールしている時は、クリック位置を描いた中身の座標に直す */
	if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN) {
		int nScroll = AdminAutoLayoutGetScrollY (hWnd);
		if (nScroll) {
			lParam = MAKELPARAM (GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam) + nScroll);
		}
	}

	LRESULT lResult = WndProcAdminWindowSub (hWnd, msg, wParam, lParam);

	/* 大きさが変わった時と、画面の部品が作り直されたかもしれない時に並べ直す */
	switch (msg) {
	case WM_SIZE:
	case WM_COMMAND:
	case WM_NOTIFY:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		AdminAutoLayout (hWnd, TRUE);
		break;
	default:
		if (msg >= WM_USER) {
			AdminAutoLayout (hWnd, TRUE);
		}
		break;
	}
	return lResult;
}

/* ========================================================================= */
/* 関数名：WndProcAdminWindowSub											 */
/* 内容：メッセージの処理（本体）											 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
static LRESULT WndProcAdminWindowSub(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	HANDLE_MSG (hWnd, WM_COMMAND, AdminOnCommand);
	HANDLE_MSG (hWnd, WM_PAINT, AdminOnPaint);
	HANDLE_MSG (hWnd, WM_HSCROLL, AdminOnHScroll);
	HANDLE_MSG (hWnd, WM_VSCROLL, AdminOnVScroll);
	HANDLE_MSG (hWnd, WM_LBUTTONDOWN, AdminOnLButtonDown);
	HANDLE_MSG (hWnd, WM_RBUTTONDOWN, AdminOnRButtonDown);

	case WM_CREATE:
		g_nAdminWindowType = 0;
		g_nSelectPartsID = 0;
		/* Alt + F4を無効化 */
		SetClassLong (hWnd, GCL_STYLE, GetClassLong (hWnd, GCL_STYLE) | (CS_NOCLOSE));
		break;

	case WM_ADMINPOSLEFT:
		switch (g_nAdminWindowType) {
		case IDM_CHAR_NPCROUTE:
			NpcRouteOnPosLeft ((int)wParam, (int)lParam);
			break;

		case IDM_MAP_EVENT:
			if ((SendMessage (m_hWndCheck1, BM_GETCHECK, 0, 0) & BST_CHECKED) == 0) {
				if (g_pMapInfo->pbyFlg[g_pEventInfo->dwX + g_pMapInfo->nCX *
							g_pEventInfo->dwY] & MAPFLG_EVENT) {
					/* クリックされた座標のイベントを取得 */
					SendCmdEVENTGET (-1, (DWORD)wParam, (DWORD)lParam);
				}
				break;
			}
			g_pEventInfo->dwX = (DWORD)wParam;
			g_pEventInfo->dwY = (DWORD)lParam;
			break;

		case IDM_MAP_INFO:
			/* 明るさ・灯り・見え方の設定を開いている時は、そちらで座標を使う */
			if (MapLightDlgOnPos ((int)wParam, (int)lParam)) {
				break;
			}
			g_pMapInfo->nStartX = (DWORD)wParam;
			g_pMapInfo->nStartY = (DWORD)lParam;
			break;

		case IDM_CHAR_ADD:
			m_pAddCharInfo->dwX = (DWORD)wParam;
			m_pAddCharInfo->dwY = (DWORD)lParam;
			break;

		case IDM_ETC_SUBGAME:
			OnMapLbuttonDownDlgEtcSubGame ((int)wParam, (int)lParam);
			break;

		case IDM_CHAR_ITEM:
			{
				DWORD dwX = (DWORD)wParam, dwY = (DWORD)lParam;
				int i, nCount = g_aCharInfo.GetSize ();

				for (i = 0; i < nCount; i ++) {
					/* クリック地点でキャラ発見？ */
					if ((g_aCharInfo[i]->dwX == dwX) && (g_aCharInfo[i]->dwY == dwY)) {
						PCHARINFO pChar = g_aCharInfo[i];
						SendCmdREQCHARITEM (i);
						m_dwSelectID = i;
						m_dwSelectpID = pChar->pID;
						break;
					}
				}
				ListView_DeleteAllItems (m_hWndList1);
				ZeroMemory (m_pCharInfo, sizeof (CHARINFO));
				if (i == nCount) {
					m_dwSelectID = -1;
					m_dwSelectpID = -1;
				}
			}
			break;

		case IDM_CHAR_STATUS:
			{
				DWORD dwX = (DWORD)wParam, dwY = (DWORD)lParam;
				int i, nCount = g_aCharInfo.GetSize ();

				for (i = 0; i < nCount; i ++) {
					/* クリック地点でキャラ発見？ */
					if ((g_aCharInfo[i]->dwX == dwX) && (g_aCharInfo[i]->dwY == dwY)) {
						PCHARINFO pChar = g_aCharInfo[i];
						SendCmdREQCHARSTATUS (i);
						m_dwSelectID = i;
						m_dwSelectpID = pChar->pID;
						break;
					}
				}
				HWND *ahWndTbl[23] = {
					&m_hWndEdit1,
					&m_hWndEdit2,
					&m_hWndEdit3,
					&m_hWndEdit4,
					&m_hWndEdit5,
					&m_hWndEdit6,
					&m_hWndEdit7,
					&m_hWndEdit8,
					&m_hWndEdit9,
					&m_hWndEdit10,
					&m_hWndEdit12,
					&m_hWndEdit13,
					&m_hWndEdit14,
					&m_hWndEdit15,
					&m_hWndEdit17,
					&m_hWndEdit18,
					&m_hWndEdit19,
					&m_hWndEdit20,
					&m_hWndEdit21,
					&m_hWndEdit22,
					&m_hWndEdit23,
					&m_hWndEdit24,
					&m_hWndEdit25
				};
				if (i == nCount) {
					m_dwSelectID = -1;
					m_dwSelectpID = -1;
				}
				for (i = 0; i < 23; i ++) {
					SetWindowText (*ahWndTbl[i], "");
				}
				SetWindowText (m_hWndEdit11, "");
				SendMessage (m_hWndCombo1, CB_SETCURSEL, 0, 0);
				SendMessage (m_hWndCombo2, CB_SETCURSEL, 0, 0);
				SendMessage (m_hWndCombo3, CB_SETCURSEL, 0, 0);
			}
			break;

		case IDM_CHAR_INFO:
			{
				DWORD dwX = (DWORD)wParam, dwY = (DWORD)lParam;
				int i, j, nCount = g_aCharInfo.GetSize ();
				char szTmp[20];

				SetWindowText (m_hWndEdit1, "");
				SetWindowText (m_hWndEdit5, "");
				SetWindowText (m_hWndEdit10, "");
				SetWindowText (m_hWndEdit11, "");
				SetWindowText (m_hWndEdit12, "");
				SetWindowText (m_hWndEdit13, "");
				SetWindowText (m_hWndEdit14, "");
				SetWindowText (m_hWndEdit15, "");
				SetWindowText (m_hWndEdit16, "");
				SetWindowText (m_hWndEdit17, "");
				SetWindowText (m_hWndEdit18, "");
				SetWindowText (m_hWndEdit19, "");
				SetWindowText (m_hWndEdit20, "");
				SetWindowText (m_hWndEdit21, "");
				SetWindowText (m_hWndEdit22, "");
				SendMessage (m_hWndCombo1, CB_SETCURSEL, 0, 0);
				SendMessage (m_hWndCombo2, CB_SETCURSEL, 0, 0);
				SendMessage (m_hWndCombo3, CB_SETCURSEL, 0, 0);
				SendMessage (m_hWndCombo4, CB_SETCURSEL, 0, 0);
				SendMessage (m_hWndCombo5, CB_SETCURSEL, 0, 0);
				for (i = 0; i < nCount; i ++) {
					/* クリック地点でキャラ発見？ */
					if ((g_aCharInfo[i]->dwX == dwX) && (g_aCharInfo[i]->dwY == dwY)) {
						PCHARINFO pChar = g_aCharInfo[i];
						m_pParaCharInfo->pID = pChar->pID;
						m_pParaCharInfo->dwTblID = pChar->dwTblID;
						m_pParaCharInfo->byAdminLV = pChar->byAdminLV;
						m_pParaCharInfo->byCharState = pChar->byCharState;
						m_pParaCharInfo->byDirection = pChar->byDirection;
						m_pParaCharInfo->byRoofLV = pChar->byRoofLV;
						m_pParaCharInfo->byEfect = pChar->byEfect;
						m_pParaCharInfo->byNpcType = pChar->byNpcType;
						m_pParaCharInfo->wGrpID = pChar->wGrpID;
						m_pParaCharInfo->wGrpMain = pChar->wGrpMain;
						m_pParaCharInfo->wGrpSub = pChar->wGrpSub;
						m_pParaCharInfo->dwMapID = pChar->dwMapID;
						m_pParaCharInfo->dwStartMapID = pChar->dwStartMapID;
						m_pParaCharInfo->dwStartX = pChar->dwStartX;
						m_pParaCharInfo->dwStartY = pChar->dwStartY;
						m_pParaCharInfo->dwX = pChar->dwX;
						m_pParaCharInfo->dwY = pChar->dwY;
						m_pParaCharInfo->clName = pChar->clName;
						m_pParaCharInfo->byAreaY = pChar->byAreaY;
						m_pParaCharInfo->byPutCount = pChar->byPutCount;
						m_pParaCharInfo->byDropProbability = pChar->byDropProbability;
						m_pParaCharInfo->byMoveProbabilityX = pChar->byMoveProbabilityX;
						m_pParaCharInfo->byMoveProbabilityY = pChar->byMoveProbabilityY;
						m_pParaCharInfo->bySearchCx = pChar->bySearchCx;
						m_pParaCharInfo->bySearchCy = pChar->bySearchCy;
						m_pParaCharInfo->wTimePutInterval = pChar->wTimePutInterval;
						m_pParaCharInfo->dwStTmplateID = pChar->dwStTmplateID;
						m_pParaCharInfo->dwNpcID = pChar->dwNpcID;
						m_pParaCharInfo->clName = pChar->clName;
						m_pParaCharInfo->clSpeech = pChar->clSpeech;
						wsprintf (m_pParaCharInfo->szName, "%s", pChar->szName);
						wsprintf (m_pParaCharInfo->szAcc, "%s", pChar->szAcc);
						wsprintf (m_pParaCharInfo->szPas, "%s", pChar->szPas);
						if (pChar->pszNpcSpeech) {
							int nSize = strlen (pChar->pszNpcSpeech) + 1;
							SAFE_DELETE_ARRAY (m_pParaCharInfo->pszNpcSpeech);
							m_pParaCharInfo->pszNpcSpeech = new char[nSize];
							CopyMemory (m_pParaCharInfo->pszNpcSpeech, pChar->pszNpcSpeech, nSize);
							SetWindowText (m_hWndEdit1, m_pParaCharInfo->pszNpcSpeech);

						} else {
							SetWindowText (m_hWndEdit1, "");
						}
						SetWindowText (m_hWndEdit5, m_pParaCharInfo->szName);
						SetWindowText (m_hWndEdit10, "");
						SetWindowText (m_hWndEdit11, "");
						wsprintf (szTmp, "%d", m_pParaCharInfo->byAreaX);
						SetWindowText (m_hWndEdit12, szTmp);
						wsprintf (szTmp, "%d", m_pParaCharInfo->byAreaY);
						SetWindowText (m_hWndEdit13, szTmp);
						wsprintf (szTmp, "%d", m_pParaCharInfo->byPutCount);
						SetWindowText (m_hWndEdit14, szTmp);
						wsprintf (szTmp, "%d", m_pParaCharInfo->wTimePutInterval);
						SetWindowText (m_hWndEdit15, szTmp);
						wsprintf (szTmp, "%d", m_pParaCharInfo->dwStartMapID);
						SetWindowText (m_hWndEdit16, szTmp);
						wsprintf (szTmp, "%d", m_pParaCharInfo->dwStartX);
						SetWindowText (m_hWndEdit17, szTmp);
						wsprintf (szTmp, "%d", m_pParaCharInfo->dwStartY);
						SetWindowText (m_hWndEdit18, szTmp);
						wsprintf (szTmp, "%d", m_pParaCharInfo->byMoveProbabilityX);
						SetWindowText (m_hWndEdit19, szTmp);
						wsprintf (szTmp, "%d", m_pParaCharInfo->byMoveProbabilityY);
						SetWindowText (m_hWndEdit20, szTmp);
						wsprintf (szTmp, "%d", m_pParaCharInfo->bySearchCx);
						SetWindowText (m_hWndEdit21, szTmp);
						wsprintf (szTmp, "%d", m_pParaCharInfo->bySearchCy);
						SetWindowText (m_hWndEdit22, szTmp);
						SendMessage (m_hWndCombo1, CB_SETCURSEL, m_pParaCharInfo->byDirection, 0);
						SendMessage (m_hWndCombo2, CB_SETCURSEL, m_pParaCharInfo->byNpcType, 0);
						SendMessage (m_hWndCombo3, CB_SETCURSEL, m_pParaCharInfo->byAdminLV, 0);

						if (m_pParaCharInfo->dwStTmplateID != -1) {
							SendMessage (m_hWndCombo5, CB_SETCURSEL, m_pParaCharInfo->dwStTmplateID + 1, 0);
						} else {
							SendMessage (m_hWndCombo5, CB_SETCURSEL, 0, 0);
						}
						for (j = 0; ; j ++) {
							if (aColorTbl[j] == 0) {
								break;
							}
							if (aColorTbl[j] == m_pParaCharInfo->clName) {
								SendMessage (m_hWndCombo4, CB_SETCURSEL, j, 0);
								break;
							}
						}
						SendCmdGETACCINFO (m_pParaCharInfo->dwTblID);
						break;
					}
				}
				if (i == nCount) {
					ZeroMemory (m_pParaCharInfo, sizeof (CMDPARA_CHARINFO));
				}
			}
			break;

		case IDM_CHAR_MAGIC:
			{
				DWORD dwX = (DWORD)wParam, dwY = (DWORD)lParam;
				int i, nCount = g_aCharInfo.GetSize ();

				for (i = 0; i < nCount; i ++) {
					/* クリック地点でキャラ発見？ */
					if ((g_aCharInfo[i]->dwX == dwX) && (g_aCharInfo[i]->dwY == dwY)) {
						PCHARINFO pChar = g_aCharInfo[i];
						SendCmdREQCHARSTATUS ((DWORD)i);
						SetCharMagicInfo (NULL);
						break;
					}
				}
				if (i >= nCount) {
					SetCharMagicInfo (NULL);
				}
			}
			break;
		}
		InvalidateRect (hWnd, NULL, TRUE);
		break;

	case WM_ADMINPOSRIGHT:
		switch (g_nAdminWindowType) {
		case IDM_CHAR_NPCROUTE:
			NpcRouteOnPosRight ((int)wParam, (int)lParam);
			break;

		case IDM_MAP_EVENT:
			if ((SendMessage (m_hWndCheck1, BM_GETCHECK, 0, 0) & BST_CHECKED) == 0) {
				break;
			}
			if (g_pEventInfo->pData) {
				switch (g_pEventInfo->byType) {
				case EVENTTYPE_MOVE:		/* 座標変更 */
					{
						PEVENTINFO_MOVE pMove = (PEVENTINFO_MOVE)g_pEventInfo->pData;
						pMove->dwX = (DWORD)wParam;
						pMove->dwY = (DWORD)lParam;
						MakeAdminWndEventInfoType ();
					}
					break;

				case EVENTTYPE_ONCHARMAP:	/* キャラが乗っている間マップ変更 */
					{
						PEVENTINFO_ONCHARMAP pInfo = (PEVENTINFO_ONCHARMAP)g_pEventInfo->pData;
						pInfo->dwX = (DWORD)wParam;
						pInfo->dwY = (DWORD)lParam;
						MakeAdminWndEventInfoType ();
					}
					break;
				}
			}
			break;

		case IDM_CHAR_INFO:
			m_pParaCharInfo->dwX = (DWORD)wParam;
			m_pParaCharInfo->dwY = (DWORD)lParam;
			break;

		case IDM_ETC_SUBGAME:
			if (g_pSubGameInfo == NULL) {
				break;
			}
			if (g_pSubGameInfo->pInfo == NULL) {
				break;
			}
			switch (g_pSubGameInfo->byType) {
			case SUBGAMEID_BOMB:		/* 爆弾ゲーム */
				{
					PSUBGAME_BOMBINFOSAVE pBomb = (PSUBGAME_BOMBINFOSAVE)g_pSubGameInfo->pInfo;
					pBomb->dwStartX = (DWORD)wParam;
					pBomb->dwStartY = (DWORD)lParam;
					MakeAdminWndSubGameInfoType ();
				}
				break;

			case SUBGAMEID_MAZE:		/* 迷路 */
				{
					PSUBGAME_MAZEINFOSAVE pMaze = (PSUBGAME_MAZEINFOSAVE)g_pSubGameInfo->pInfo;
					pMaze->byX2 = (BYTE)wParam;
					pMaze->byY2 = (BYTE)lParam;
					MakeAdminWndSubGameInfoType ();
				}
				break;

			case SUBGAMEID_DUNGEON:		/* ダンジョン */
				{
					PSUBGAME_DUNGEONINFOSAVE pDungeon = (PSUBGAME_DUNGEONINFOSAVE)g_pSubGameInfo->pInfo;
					pDungeon->byX2 = (BYTE)wParam;
					pDungeon->byY2 = (BYTE)lParam;
					MakeAdminWndSubGameInfoType ();
				}
				break;
			}
			break;
		}
		InvalidateRect (hWnd, NULL, TRUE);
		break;

	/* 右ダブルクリック */
	case WM_ADMINRBUTTONDBLCLK:
		{
			if (g_nAdminWindowType != IDM_CHAR_INFO) {
				break;
			}
			PostMessage (hWnd, WM_COMMAND, IDC_BUTTON2, 0);
		}
		break;

	case WM_ADMINCHGEVENT:
		g_nSelectEventID = (int)wParam;
		SendMessage (m_hWndEventList, CB_SETCURSEL, g_pEventInfo->byType, 0);
		AdminOnCommandMapEvent (NULL, IDC_RENEW, NULL, 0);
		InvalidateRect (hWnd, NULL, TRUE);
		break;

	case WM_MAPPARTSCOUNT:
		SetScrollRange (m_hWndMapPartsScrParts, SB_CTL, 0, max ((g_wCountMapParts / 10) - 3, 0), FALSE);
		InvalidateRect (hWnd, NULL, TRUE);
		break;

	/* マップメッセージ受信 */
	case WM_ADMINMSGREQ:
		{
			if (g_nAdminWindowType != IDM_MAP_MSG) {
				break;
			}
			LPSTR pMsg = (LPSTR)wParam;
			SetWindowText (m_hWndEdit1, pMsg);
			delete [] pMsg;
		}
		break;

	/* アカウント・パスワード取得 */
	case WM_ADMINGETACC:
		{
			LPSTR pszAcc, pszPas;
			pszAcc = (LPSTR)wParam;
			pszPas = (LPSTR)lParam;

			if (g_nAdminWindowType == IDM_CHAR_INFO) {
				SetWindowText (m_hWndEdit10, pszAcc);
				SetWindowText (m_hWndEdit11, pszPas);
			}

			delete [] pszAcc;
			delete [] pszPas;
		}
		break;

	/* アカウント情報要求 */
	case WM_ADMINGETACCINFO:
		{
			PCMDPARA_CHARINFO pPara = (PCMDPARA_CHARINFO)wParam;
			if (!m_pParaCharInfo) {
				delete pPara;
				break;
			}

			ZeroMemory (m_pParaCharInfo, sizeof (m_pParaCharInfo));
			m_pParaCharInfo->pID = pPara->pID;
			m_pParaCharInfo->dwTblID = pPara->dwTblID;
			m_pParaCharInfo->byAdminLV = pPara->byAdminLV;
			m_pParaCharInfo->byCharState = pPara->byCharState;
			m_pParaCharInfo->byAccState = pPara->byAccState;
			m_pParaCharInfo->byDirection = pPara->byDirection;
			m_pParaCharInfo->byRoofLV = pPara->byRoofLV;
			m_pParaCharInfo->byEfect = pPara->byEfect;
			m_pParaCharInfo->byNpcType = pPara->byNpcType;
			m_pParaCharInfo->byAreaX = pPara->byAreaX;
			m_pParaCharInfo->byAreaY = pPara->byAreaY;
			m_pParaCharInfo->byPutCount = pPara->byPutCount;
			m_pParaCharInfo->byDropProbability = pPara->byDropProbability;
			m_pParaCharInfo->byMoveProbabilityX = pPara->byMoveProbabilityX;
			m_pParaCharInfo->byMoveProbabilityY = pPara->byMoveProbabilityY;
			m_pParaCharInfo->bySearchCx = pPara->bySearchCx;
			m_pParaCharInfo->bySearchCy = pPara->bySearchCy;
			m_pParaCharInfo->wGrpID = pPara->wGrpID;
			m_pParaCharInfo->wGrpMain = pPara->wGrpMain;
			m_pParaCharInfo->wGrpSub = pPara->wGrpSub;
			m_pParaCharInfo->wTimePutInterval = pPara->wTimePutInterval;
			m_pParaCharInfo->dwMapID = pPara->dwMapID;
			m_pParaCharInfo->dwStartMapID = pPara->dwStartMapID;
			m_pParaCharInfo->dwStartX = pPara->dwStartX;
			m_pParaCharInfo->dwStartY = pPara->dwStartY;
			m_pParaCharInfo->dwX = pPara->dwX;
			m_pParaCharInfo->dwY = pPara->dwY;
			m_pParaCharInfo->dwLoginCount = pPara->dwLoginCount;
			m_pParaCharInfo->dwPlayTime = pPara->dwPlayTime;
			m_pParaCharInfo->dwStTmplateID = pPara->dwStTmplateID;
			m_pParaCharInfo->dwNpcID = pPara->dwNpcID;
			m_pParaCharInfo->dwLastIPAddress = pPara->dwLastIPAddress;
			m_pParaCharInfo->clName = pPara->clName;
			m_pParaCharInfo->clSpeech = pPara->clSpeech;
			wsprintf (m_pParaCharInfo->szName, "%s", pPara->szName);
			wsprintf (m_pParaCharInfo->szAcc, "%s", pPara->szAcc);
			wsprintf (m_pParaCharInfo->szPas, "%s", pPara->szPas);
			CopyMemory (m_pParaCharInfo->szMacAdr, pPara->szMacAdr, 6);

			if (g_nAdminWindowType != IDM_ACC_INFO) {
				if (m_pParaCharInfo->pszNpcSpeech) {
					SetWindowText (m_hWndEdit1, m_pParaCharInfo->pszNpcSpeech);

				} else {
					SetWindowText (m_hWndEdit1, "");
				}
				SetWindowText (m_hWndEdit5, m_pParaCharInfo->szName);
				SetWindowText (m_hWndEdit10, "");
				SetWindowText (m_hWndEdit11, "");
				char szTmp[32];
				wsprintf (szTmp, "%d", m_pParaCharInfo->byAreaX);
				SetWindowText (m_hWndEdit12, szTmp);
				wsprintf (szTmp, "%d", m_pParaCharInfo->byAreaY);
				SetWindowText (m_hWndEdit13, szTmp);
				wsprintf (szTmp, "%d", m_pParaCharInfo->byPutCount);
				SetWindowText (m_hWndEdit14, szTmp);
				wsprintf (szTmp, "%d", m_pParaCharInfo->wTimePutInterval);
				SetWindowText (m_hWndEdit15, szTmp);
				wsprintf (szTmp, "%d", m_pParaCharInfo->dwStartMapID);
				SetWindowText (m_hWndEdit16, szTmp);
				wsprintf (szTmp, "%d", m_pParaCharInfo->dwStartX);
				SetWindowText (m_hWndEdit17, szTmp);
				wsprintf (szTmp, "%d", m_pParaCharInfo->dwStartY);
				SetWindowText (m_hWndEdit18, szTmp);
				wsprintf (szTmp, "%d", m_pParaCharInfo->byMoveProbabilityX);
				SetWindowText (m_hWndEdit19, szTmp);
				wsprintf (szTmp, "%d", m_pParaCharInfo->byMoveProbabilityY);
				SetWindowText (m_hWndEdit20, szTmp);
				wsprintf (szTmp, "%d", m_pParaCharInfo->bySearchCx);
				SetWindowText (m_hWndEdit21, szTmp);
				wsprintf (szTmp, "%d", m_pParaCharInfo->bySearchCy);
				SetWindowText (m_hWndEdit22, szTmp);
				SendMessage (m_hWndCombo1, CB_SETCURSEL, m_pParaCharInfo->byDirection, 0);
				SendMessage (m_hWndCombo2, CB_SETCURSEL, m_pParaCharInfo->byNpcType, 0);
				SendMessage (m_hWndCombo3, CB_SETCURSEL, m_pParaCharInfo->byAdminLV, 0);
				for (int j = 0; ; j ++) {
					if (aColorTbl[j] == 0) {
						break;
					}
					if (aColorTbl[j] == m_pParaCharInfo->clName) {
						SendMessage (m_hWndCombo4, CB_SETCURSEL, j, 0);
						break;
					}
				}
				if (m_pParaCharInfo->dwStTmplateID != -1) {
					SendMessage (m_hWndCombo5, CB_SETCURSEL, m_pParaCharInfo->dwStTmplateID + 1, 0);
				} else {
					SendMessage (m_hWndCombo5, CB_SETCURSEL, 0, 0);
				}
			}
			InvalidateRect (hWnd, NULL, TRUE);
			delete pPara;
		}
		break;

	/* アイテム情報更新 */
	case WM_ADMINITEMCHG:
		{
			if (g_nAdminWindowType != IDM_ITEM_LIST) {
				break;
			}

			int nID = (int)wParam;
			LV_ITEM item;
			PITEMINFO pItem;
			char szTmp[128];
			int nCount = g_aItemInfo.GetSize ();
			ZeroMemory (&item, sizeof (item));
			ZeroMemory (szTmp, sizeof (szTmp));
			item.mask = LVIF_TEXT;

			/* 増えた？ */
			if (nID >= ListView_GetItemCount (m_hWndList1)) {
				pItem = g_aItemInfo[nCount - 1];
				wsprintf (szTmp, "%d", nCount - 1);
				item.iItem = nCount - 1;
				/* ID */
				item.iSubItem = 0;
				item.pszText = szTmp;
				ListView_InsertItem (m_hWndList1, &item);
				ListView_EnsureVisible (m_hWndList1, nCount - 1, FALSE);

			/* 更新 */
			} else {
				pItem = g_aItemInfo[nID];
				item.iItem = nID;
			}
			/* 名前 */
			item.iSubItem = 1;
			item.pszText = pItem->szName;
			ListView_SetItem (m_hWndList1, &item);

			/* 種別 */
			item.iSubItem = 2;
			item.pszText = (LPSTR)aItemeTypeNameTbl[pItem->byType];
			ListView_SetItem (m_hWndList1, &item);

			/* 効果 */
			item.iSubItem = 3;
			wsprintf (szTmp, "%d", pItem->byData);
			item.pszText = szTmp;
			ListView_SetItem (m_hWndList1, &item);

			/* 金額 */
			item.iSubItem = 4;
			wsprintf (szTmp, "%d", pItem->dwGold);
			item.pszText = szTmp;
			ListView_SetItem (m_hWndList1, &item);
		}
		break;

	/* キャラアイテム情報 */
	case WM_ADMINCHARITEM:
		{
			if (g_nAdminWindowType != IDM_CHAR_ITEM) {
				break;
			}
			if (m_dwSelectID == -1) {
				break;
			}

			PCHARINFO pChar = g_aCharInfo[wParam];
			CopyMemory (m_pCharInfo, pChar, sizeof (CHARINFO));
			ListView_DeleteAllItems (m_hWndList1);

			int i;
			LV_ITEM item;
			for (i = 0; i < MAX_MYITEM; i ++) {
				ZeroMemory (&item, sizeof (item));
				item.mask = LVIF_TEXT;
				item.iItem = i;
				item.iSubItem = 0;
				ListView_InsertItem (m_hWndList1, &item);
				item.iSubItem = 1;
				if (m_pCharInfo->waItem[i]) {
					item.pszText = g_aItemInfo[m_pCharInfo->waItem[i]]->szName;
				}
				ListView_SetItem (m_hWndList1, &item);
			}
			for (i = 0; i < MAX_EQUTYPE; i ++) {
				if (m_pCharInfo->waEqu[i] == 0xFFFF) {
					continue;
				}
				ZeroMemory (&item, sizeof (item));
				item.mask = LVIF_TEXT;
				item.iItem = m_pCharInfo->waEqu[i];
				item.iSubItem = 0;
				item.pszText = (LPSTR)aEquNameTbl[i];
				ListView_SetItem (m_hWndList1, &item);
			}
		}
		break;

	/* ステータス情報更新 */
	case WM_ADMINCHARSTATUS:
		{
			if (g_nAdminWindowType == IDM_CHAR_MAGIC) {
				PCHARINFO pChar = g_aCharInfo[wParam];
				SetCharMagicInfo (pChar);
				break;
			}
			if (g_nAdminWindowType != IDM_CHAR_STATUS) {
				break;
			}
			if (m_dwSelectID == -1) {
				break;
			}
			if ((DWORD)wParam >= (DWORD)g_aCharInfo.GetSize ()) {
				break;
			}
			if (m_dwSelectID >= (DWORD)g_aCharInfo.GetSize ()) {
				break;
			}

			PCHARINFO pCharTmp = g_aCharInfo[m_dwSelectID];
			PCHARINFO pChar = g_aCharInfo[wParam];
			/* 選択してないキャラのステータス？ */
			if (pCharTmp != pChar) {
				break;
			}

			HWND *ahWndTbl[15] = {
				&m_hWndEdit1,
				&m_hWndEdit2,
				&m_hWndEdit3,
				&m_hWndEdit4,
				&m_hWndEdit5,
				&m_hWndEdit6,
				&m_hWndEdit7,
				&m_hWndEdit8,
				&m_hWndEdit9,
				&m_hWndEdit10,
				&m_hWndEdit12,
				&m_hWndEdit13,
				&m_hWndEdit14,
				&m_hWndEdit15,
				&m_hWndEdit16
			};

			char szTmp[100];
			ZeroMemory (szTmp, sizeof (szTmp));

			/* 名前 */
			wsprintf (szTmp, "%s", pChar->szName);
			SetWindowText (m_hWndEdit11, szTmp);
			/* 力 */
			wsprintf (szTmp, "%d", pChar->byPW);
			SetWindowText (*ahWndTbl[0], szTmp);
			/* 素早さ */
			wsprintf (szTmp, "%d", pChar->byDEX);
			SetWindowText (*ahWndTbl[1], szTmp);
			/* 体力 */
			wsprintf (szTmp, "%d", pChar->byST);
			SetWindowText (*ahWndTbl[2], szTmp);
			/* 賢さ */
			wsprintf (szTmp, "%d", pChar->byIT);
			SetWindowText (*ahWndTbl[3], szTmp);
			/* 運のよさ */
			wsprintf (szTmp, "%d", pChar->byLK);
			SetWindowText (*ahWndTbl[4], szTmp);
			/* 最大HP */
			wsprintf (szTmp, "%d", pChar->wMaxHP);
			SetWindowText (*ahWndTbl[5], szTmp);
			/* HP */
			wsprintf (szTmp, "%d", pChar->wHP);
			SetWindowText (*ahWndTbl[6], szTmp);
			/* 最大Mana */
			wsprintf (szTmp, "%d", pChar->wMaxMana);
			SetWindowText (*ahWndTbl[7], szTmp);
			/* Mana */
			wsprintf (szTmp, "%d", pChar->wMana);
			SetWindowText (*ahWndTbl[8], szTmp);
			/* 所持金 */
			wsprintf (szTmp, "%ld", pChar->dwGold);
			SetWindowText (*ahWndTbl[9], szTmp);
			/* ドロップ率 */
			wsprintf (szTmp, "%d", pChar->byDropProbability);
			SetWindowText (*ahWndTbl[10], szTmp);
			/* 移動確率X */
			wsprintf (szTmp, "%d", pChar->byMoveProbabilityX);
			SetWindowText (*ahWndTbl[11], szTmp);
			/* 移動確率Y */
			wsprintf (szTmp, "%d", pChar->byMoveProbabilityY);
			SetWindowText (*ahWndTbl[12], szTmp);
			/* 検索範囲X */
			wsprintf (szTmp, "%d", pChar->bySearchCx);
			SetWindowText (*ahWndTbl[13], szTmp);
			/* 検索範囲Y */
			wsprintf (szTmp, "%d", pChar->bySearchCy);
			SetWindowText (*ahWndTbl[14], szTmp);
			/* 火 */
			wsprintf (szTmp, "%d", pChar->chAttrFire);
			SetWindowText (m_hWndEdit17, szTmp);
			/* 水 */
			wsprintf (szTmp, "%d", pChar->chAttrWater);
			SetWindowText (m_hWndEdit18, szTmp);
			/* 風 */
			wsprintf (szTmp, "%d", pChar->chAttrWind);
			SetWindowText (m_hWndEdit19, szTmp);
			/* 土 */
			wsprintf (szTmp, "%d", pChar->chAttrEarth);
			SetWindowText (m_hWndEdit20, szTmp);
			/* 光 */
			wsprintf (szTmp, "%d", pChar->chAttrLight);
			SetWindowText (m_hWndEdit21, szTmp);
			/* 闇 */
			wsprintf (szTmp, "%d", pChar->chAttrDark);
			SetWindowText (m_hWndEdit22, szTmp);
			/* 魔法防御 */
			wsprintf (szTmp, "%d", pChar->wDFMagic);
			SetWindowText (m_hWndEdit23, szTmp);
			/* レベル */
			wsprintf (szTmp, "%d", pChar->byLevel);
			SetWindowText (m_hWndEdit24, szTmp);
			/* 経験値 */
			wsprintf (szTmp, "%u", pChar->dwExp);
			SetWindowText (m_hWndEdit25, szTmp);

			SendMessage (m_hWndCombo1, CB_SETCURSEL, pChar->bySaveFlg & CHARINFOSAVEFLG_FEMALE, 0);
			SendMessage (m_hWndCombo2, CB_SETCURSEL, pChar->byNpcType, 0);
			SendMessage (m_hWndCombo3, CB_SETCURSEL, pChar->byDrunkLV, 0);
		}
		break;

	/* エフェクト情報更新 */
	case WM_ADMINEFCINFO:
		if (g_nAdminWindowType != IDM_EFC_EDIT) {
			break;
		}
		PostMessage (hWnd, WM_COMMAND, (WPARAM)IDC_BUTTON3, 0);
		InvalidateRect (hWnd, NULL, TRUE);
		break;

	/* サーバー情報取得 */
	case WM_ADMINSVRINFO:
		SAFE_DELETE (m_pSvrInfo);
		m_pSvrInfo = (PCMDPARA_SVRINFO)wParam;
		InvalidateRect (hWnd, NULL, TRUE);
		break;

	/* 店情報更新 */
	case WM_ADMINSHOPINFO:
		InvalidateRect (hWnd, NULL, TRUE);
		if (g_nAdminWindowType != IDM_MAP_SHOP) {
			break;
		}
		if ((m_dwSelectID == -1) || (m_dwSelectID != (DWORD)wParam)) {
			break;
		}
		PostMessage (hWnd, WM_COMMAND, (WPARAM)IDC_BUTTON6, 0);
		break;

	/* ステータステンプレート情報更新 */
	case WM_ADMINSTTMPLATE:
		{
			if (g_nAdminWindowType == IDM_CHAR_STTMP) {
				RenewStTmpInfo ();
				break;
			}
			if (g_nAdminWindowType != IDM_CHAR_STATUS) {
				break;
			}
			int nNo = ListView_GetNextItem (m_hWndList1, -1, LVNI_ALL | LVNI_SELECTED);
			ListView_DeleteAllItems (m_hWndList1);
			for (int i = 0; i < g_aStTmpInfo.GetSize (); i ++) {
				PSTATUSTEMPLATEINFO pInfo = g_aStTmpInfo[i];
				LV_ITEM item;
				ZeroMemory (&item, sizeof (item));
				item.mask = LVIF_TEXT;
				item.iItem = i;
				char szTmp[MAX_CHARNAMEBUF];

				/* 名前 */
				ZeroMemory (szTmp, sizeof (szTmp));
				item.iSubItem = 0;
				item.pszText = pInfo->szName;
				ListView_InsertItem (m_hWndList1, &item);
				/* 移動種別 */
				ZeroMemory (szTmp, sizeof (szTmp));
				item.iSubItem = 1;
				item.pszText = (LPSTR)aNpcMoveTypeTbl[pInfo->byNpcType];
				ListView_SetItem (m_hWndList1, &item);
				/* 力 */
				wsprintf (szTmp, "%d", pInfo->byPW);
				item.iSubItem = 2;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* 素早さ */
				wsprintf (szTmp, "%d", pInfo->byDEX);
				item.iSubItem = 3;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* 体力 */
				wsprintf (szTmp, "%d", pInfo->byST);
				item.iSubItem = 4;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* 賢さ */
				wsprintf (szTmp, "%d", pInfo->byIT);
				item.iSubItem = 5;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* 運のよさ */
				wsprintf (szTmp, "%d", pInfo->byLK);
				item.iSubItem = 6;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* 最大HP */
				wsprintf (szTmp, "%d", pInfo->wMaxHP);
				item.iSubItem = 7;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* 最大Mana */
				wsprintf (szTmp, "%d", pInfo->wMaxMana);
				item.iSubItem = 8;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* 所持金 */
				wsprintf (szTmp, "%d", pInfo->dwGold);
				item.iSubItem = 9;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* ドロップ率 */
				wsprintf (szTmp, "%d", pInfo->byDropProbability);
				item.iSubItem = 10;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* 移動確率X */
				wsprintf (szTmp, "%d", pInfo->byMoveProbabilityX);
				item.iSubItem = 11;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* 移動確率Y */
				wsprintf (szTmp, "%d", pInfo->byMoveProbabilityY);
				item.iSubItem = 12;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* 検索範囲X */
				wsprintf (szTmp, "%d", pInfo->bySearchCx);
				item.iSubItem = 13;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* 検索範囲Y */
				wsprintf (szTmp, "%d", pInfo->bySearchCy);
				item.iSubItem = 14;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* 火 */
				wsprintf (szTmp, "%d", pInfo->chAttrFire);
				item.iSubItem = 15;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* 水 */
				wsprintf (szTmp, "%d", pInfo->chAttrWater);
				item.iSubItem = 16;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* 風 */
				wsprintf (szTmp, "%d", pInfo->chAttrWind);
				item.iSubItem = 17;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* 土 */
				wsprintf (szTmp, "%d", pInfo->chAttrEarth);
				item.iSubItem = 18;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* 光 */
				wsprintf (szTmp, "%d", pInfo->chAttrLight);
				item.iSubItem = 19;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* 闇 */
				wsprintf (szTmp, "%d", pInfo->chAttrDark);
				item.iSubItem = 20;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* 魔法防御 */
				wsprintf (szTmp, "%d", pInfo->wDFMagic);
				item.iSubItem = 21;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* レベル */
				wsprintf (szTmp, "%d", pInfo->byLevel);
				item.iSubItem = 22;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
				/* 経験値 */
				wsprintf (szTmp, "%u", pInfo->dwExp);
				item.iSubItem = 23;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
			}
			ListView_SetItemState (m_hWndList1, nNo, LVNI_SELECTED, LVNI_SELECTED);
			ListView_EnsureVisible (m_hWndList1, nNo, TRUE);
		}
		break;

	/* サブゲーム情報更新 */
	case WM_ADMINSUBGAMEINFO:
		if (g_nAdminWindowType != IDM_ETC_SUBGAME) {
			break;
		}
		EtcSubGameRenewInfo ();
		break;

	/* 預かり所情報更新 */
	case WM_ADMINLEAVEINFO:
		{
			LV_ITEM item;
			ZeroMemory (&item, sizeof (item));
			item.mask = LVIF_TEXT;
			item.iItem = g_aStTmpInfo.GetSize ();

			char szTmp[10];
			ListView_DeleteAllItems (m_hWndList1);
			for (int i = 0; i < g_pLeaveInfo->byCount; i ++) {
				item.iItem = i;
				item.iSubItem = 0;
				item.pszText = g_aItemInfo[g_pLeaveInfo->waItem[i]]->szName;
				ListView_InsertItem (m_hWndList1, &item);

				wsprintf (szTmp, "%d", g_pLeaveInfo->byItemCount[i]);
				item.iSubItem = 1;
				item.pszText = szTmp;
				ListView_SetItem (m_hWndList1, &item);
			}
			wsprintf (szTmp, "%ld", g_pLeaveInfo->dwGold);
			SetWindowText (m_hWndEdit3, szTmp);
		}
		break;

	/* アカウント検索 */
	case WM_ADMINACCSEARCH:
		if (g_nAdminWindowType != IDM_ACC_SEARCH) {
			break;
		}
		AccSearchSetTblID ((DWORD)wParam);
		break;

	/* 職業情報更新 */
	case WM_ADMINJOBINFO:
		if (g_nAdminWindowType != IDM_ETC_JOBINFO) {
			break;
		}
		RenewJobInfo ();
		break;

	/* 魔法情報更新 */
	case WM_ADMINMAGICINFO:
		if (g_nAdminWindowType != IDM_ETC_MAGICINFO) {
			break;
		}
		RenewMagicInfo ();
		break;

	/* 魔法利用者情報更新 */
	case WM_ADMINMAGICUSERINFO:
		if (g_nAdminWindowType != IDM_ETC_MAGICUSERINFO) {
			break;
		}
		RenewMagicUserInfo ();
		break;

	/* 装備職業情報更新 */
	case WM_ADMINEQUUSERINFO:
		if (g_nAdminWindowType != IDM_ETC_EQUUSERINFO) {
			break;
		}
		RenewEquUserInfo ();
		break;

	/* キャラ画像が選択された */
	case WM_ADMINSELECTCHARGRP:
		if (g_nAdminWindowType == IDM_CHAR_INFO) {
			AdminOnSelectCharGrpCharInfo (wParam, HIWORD(lParam), LOWORD(lParam));
			break;
		}
		break;

	case WM_DESTROY:
		DeleteAdminWnd ();
		PostQuitMessage (0);
		break;

	default:
		/* 修理しないメッセージはOSに返す */
		return DefWindowProc (hWnd, msg, wParam, lParam);
	}
	return FALSE;
}

/* ========================================================================= */
/* 関数名：AdminOnCommand													 */
/* 内容：WM_COMMAND処理														 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void AdminOnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	/* 編集→この画面の変更を元に戻す */
	if (id == IDM_ADMIN_UNDO) {
		AdminUndoRequest ();
		return;
	}

	switch (id) {
	case IDM_MAP_PARTS:
	case IDM_MAP_INFO:
	case IDM_MAP_EVENT:
	case IDM_CHAR_ADD:
	case IDM_CHAR_INFO:
	case IDM_CHAR_STATUS:
	case IDM_CHAR_LEAVE:
	case IDM_MAP_ADD:
	case IDM_MAP_MSG:
	case IDM_ACC_ADD:
	case IDM_ACC_INFO:
	case IDM_CEF_EDIT:
	case IDM_ETC_VERSION:
	case IDM_ITEM_LIST:
	case IDM_CHAR_ITEM:
	case IDM_ETC_SVRINFO:
	case IDM_MAP_SHOP:
	case IDM_EFC_EDIT:
		MakeAdminWnd (id);
		break;
	case IDM_ETC_JOBINFO:
		DeleteAdminWnd ();
		g_nAdminWindowType = IDM_ETC_JOBINFO;
		ShowDlgJobInfo (hWnd);
		break;
	case IDM_ETC_MAGICINFO:
		DeleteAdminWnd ();
		g_nAdminWindowType = IDM_ETC_MAGICINFO;
		ShowDlgMagicInfo (hWnd);
		break;
	case IDM_ETC_MAGICUSERINFO:
		DeleteAdminWnd ();
		g_nAdminWindowType = IDM_ETC_MAGICUSERINFO;
		ShowDlgMagicUserInfo (hWnd);
		break;
	case IDM_CHAR_MAGIC:
		DeleteAdminWnd ();
		g_nAdminWindowType = IDM_CHAR_MAGIC;
		g_bAdminPos = TRUE;
		ShowDlgCharMagicInfo (hWnd);
		break;
	case IDM_ETC_EQUUSERINFO:
		DeleteAdminWnd ();
		g_nAdminWindowType = IDM_ETC_EQUUSERINFO;
		ShowDlgEquUserInfo (hWnd);
		break;
	case IDM_CHAR_STTMP:
		DeleteAdminWnd ();
		g_nAdminWindowType = IDM_CHAR_STTMP;
		ShowDlgStTmpInfo (hWnd);
		break;
	case IDM_CHAR_NPCROUTE:
		DeleteAdminWnd ();
		g_nAdminWindowType = IDM_CHAR_NPCROUTE;
		g_bAdminPos = TRUE;
		ShowDlgNpcRoute (hWnd);
		break;
	case IDM_ACC_SEARCH:
		DeleteAdminWnd ();
		g_nAdminWindowType = IDM_ACC_SEARCH;
		ShowDlgAccSearch (hWnd);
		break;
	case IDM_ETC_SUBGAME:
		DeleteAdminWnd ();
		g_nAdminWindowType = IDM_ETC_SUBGAME;
		g_bAdminPos = TRUE;
		ShowDlgEtcSubGame (hWnd);
		break;
	}

	switch (g_nAdminWindowType) {
	case IDM_MAP_PARTS:			AdminOnCommandMapParts (hWnd, id, hWndCtl, codeNotify);		break;
	case IDM_MAP_INFO:			AdminOnCommandMapInfo (hWnd, id, hWndCtl, codeNotify);		break;
	case IDM_MAP_EVENT:			AdminOnCommandMapEvent (hWnd, id, hWndCtl, codeNotify);		break;
	case IDM_CHAR_ADD:			AdminOnCommandAddChar (hWnd, id, hWndCtl, codeNotify);		break;
	case IDM_CHAR_INFO:			AdminOnCommandCharInfo (hWnd, id, hWndCtl, codeNotify);		break;
	case IDM_CHAR_STATUS:		AdminOnCommandCharStatus (hWnd, id, hWndCtl, codeNotify);	break;
	case IDM_CHAR_LEAVE:		AdminOnCommandCharLeave (hWnd, id, hWndCtl, codeNotify);	break;
	case IDM_MAP_ADD:			AdminOnCommandAddMap (hWnd, id, hWndCtl, codeNotify);		break;
	case IDM_MAP_MSG:			AdminOnCommandMsg (hWnd, id, hWndCtl, codeNotify);			break;
	case IDM_ACC_ADD:			AdminOnCommandAccAdd (hWnd, id, hWndCtl, codeNotify);		break;
	case IDM_ACC_INFO:			AdminOnCommandAccInfo (hWnd, id, hWndCtl, codeNotify);		break;
	case IDM_CEF_EDIT:			AdminOnCommandCefEdit (hWnd, id, hWndCtl, codeNotify);		break;
	case IDM_ETC_VERSION:		AdminOnCommandEtcVersion (hWnd, id, hWndCtl, codeNotify);	break;
	case IDM_ITEM_LIST:			AdminOnCommandItemList (hWnd, id, hWndCtl, codeNotify);		break;
	case IDM_CHAR_ITEM:			AdminOnCommandCharItem (hWnd, id, hWndCtl, codeNotify);		break;
	case IDM_ETC_SVRINFO:		AdminOnCommandSvrInfo (hWnd, id, hWndCtl, codeNotify);		break;
	case IDM_MAP_SHOP:			AdminOnCommandShopInfo (hWnd, id, hWndCtl, codeNotify);		break;
	case IDM_EFC_EDIT:			AdminOnCommandEfcEdit (hWnd, id, hWndCtl, codeNotify);		break;
	case IDM_ETC_SUBGAME:		AdminOnCommandSubGameInfo (hWnd, id, hWndCtl, codeNotify);	break;
	}

	/* 画面が変わったらサーバーに知らせる（「元に戻す」は画面ごとに記録するので） */
	AdminUndoNotifyScreen ();
}

/* ========================================================================= */
/* 関数名：AdminUndoScreenFromType											 */
/* 内容：管理者ウィンドウの画面(IDM_*)から「元に戻す」の画面番号を取得		 */
/* ========================================================================= */
static BYTE AdminUndoScreenFromType(int nType)
{
	switch (nType) {
	case IDM_MAP_PARTS:			return ADMINSCR_MAPPARTS;
	case IDM_MAP_INFO:			return ADMINSCR_MAPINFO;
	case IDM_MAP_EVENT:			return ADMINSCR_EVENT;
	case IDM_MAP_MSG:			return ADMINSCR_MSG;
	case IDM_MAP_SHOP:			return ADMINSCR_SHOP;
	case IDM_CHAR_STATUS:		return ADMINSCR_CHARSTATUS;
	case IDM_CHAR_ITEM:			return ADMINSCR_CHARITEM;
	case IDM_CHAR_MAGIC:		return ADMINSCR_CHARMAGIC;
	case IDM_CHAR_STTMP:		return ADMINSCR_STTMP;
	case IDM_ITEM_LIST:			return ADMINSCR_ITEM;
	case IDM_CEF_EDIT:			return ADMINSCR_CEF;
	case IDM_EFC_EDIT:			return ADMINSCR_EFC;
	case IDM_ETC_JOBINFO:		return ADMINSCR_JOBINFO;
	case IDM_ETC_MAGICINFO:		return ADMINSCR_MAGICINFO;
	case IDM_ETC_MAGICUSERINFO:	return ADMINSCR_MAGICUSERINFO;
	case IDM_ETC_EQUUSERINFO:	return ADMINSCR_EQUUSERINFO;
	case IDM_CHAR_NPCROUTE:		return ADMINSCR_NPCROUTE;
	}
	return ADMINSCR_NONE;
}

/* ========================================================================= */
/* 関数名：AdminUndoNotifyScreen											 */
/* 内容：開いている画面をサーバーに知らせる（変わった時だけ）				 */
/* ========================================================================= */
static void AdminUndoNotifyScreen(void)
{
	BYTE byScreen = AdminUndoScreenFromType (g_nAdminWindowType);

	if (g_hWndAdmin == NULL) {
		return;
	}
	if (byScreen == s_byAdminUndoScreen) {
		return;
	}
	s_byAdminUndoScreen = byScreen;
	SendCmdADMINUNDO (ADMINUNDO_SCREEN, byScreen, 0);
}

/* ========================================================================= */
/* 関数名：AdminUndoRequest													 */
/* 内容：「この画面の変更を元に戻す」（まずサーバーに戻せる変更を聞く）		 */
/* ========================================================================= */
static void AdminUndoRequest(void)
{
	BYTE byScreen = AdminUndoScreenFromType (g_nAdminWindowType);

	if (byScreen == ADMINSCR_NONE) {
		MessageBox (g_hWndAdmin,
				"この画面は「元に戻す」に対応していません。\n\n"
				"対応している画面：パーツ（マップの塗り替え）・マップ情報・イベント情報・メッセージ・店情報・"
				"ステータス情報・所持アイテム情報・所持魔法情報・ステータステンプレート情報・アイテム一覧・ふきだし・効果いろいろ・"
				"職業情報（レベルアップ情報も）・魔法情報・魔法利用者情報・装備職業情報・NPCの巡回",
				"元に戻す", MB_OK);
		return;
	}
	AdminUndoNotifyScreen ();
	SendCmdADMINUNDO (ADMINUNDO_INFOREQ, byScreen, 0);
}

/* ========================================================================= */
/* 関数名：AdminUndoOnRecv													 */
/* 内容：「元に戻す」の応答（戻せる変更の案内・戻した結果）					 */
/* ========================================================================= */
void AdminUndoOnRecv(const void *pParaIn)
{
	const CMDPARA_ADMINUNDO *pPara = (const CMDPARA_ADMINUNDO *)pParaIn;
	char szTmp[MAX_ADMINUNDOMSG + 256];

	switch (pPara->byCmdSub) {
	case ADMINUNDO_INFO:		/* 戻せる変更の案内 → 確かめてから戻す */
		if ((pPara->wCount == 0) || (pPara->dwStepID == 0)) {
			MessageBox (g_hWndAdmin, pPara->szMsg, "元に戻す", MB_OK);
			break;
		}
		snprintf (szTmp, sizeof (szTmp),
				"%s\nを元に戻しますか？\n\n（この画面では %d 回分戻せます。戻した後でやり直すことはできません）",
				pPara->szMsg, (int)pPara->wCount);
		if (MessageBox (g_hWndAdmin, szTmp, "元に戻す", MB_YESNO | MB_ICONQUESTION) == IDYES) {
			SendCmdADMINUNDO (ADMINUNDO_EXEC, pPara->byScreen, pPara->dwStepID);
		}
		break;

	case ADMINUNDO_RESULT:		/* 戻した結果 */
		MessageBox (g_hWndAdmin, pPara->szMsg, "元に戻す", MB_OK);
		if (pPara->byResult) {
			AdminUndoRefresh (pPara->byScreen);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminUndoRefresh													 */
/* 内容：元に戻した後、その画面を開いたままなら作り直して新しい内容を出す	 */
/* ========================================================================= */
static void AdminUndoRefresh(BYTE byScreen)
{
	int nType = g_nAdminWindowType;

	if (g_hWndAdmin == NULL) {
		return;
	}
	if (AdminUndoScreenFromType (nType) != byScreen) {
		return;
	}

	switch (nType) {
	case IDM_MAP_PARTS:
		/* マップはそのまま書き換わるので、選んでいるパーツを残すため作り直さない */
		break;

	/* ダイアログで出す画面はメニューと同じ処理で出し直す */
	case IDM_ETC_JOBINFO:
	case IDM_ETC_MAGICINFO:
	case IDM_ETC_MAGICUSERINFO:
	case IDM_CHAR_MAGIC:
	case IDM_ETC_EQUUSERINFO:
	case IDM_CHAR_STTMP:
		AdminOnCommand (g_hWndAdmin, nType, NULL, 0);
		break;

	/* 選んでいるNPCや編集中の道しるべを残したいので、巡回ルートだけ取り直す */
	case IDM_CHAR_NPCROUTE:
		NpcRouteReload ();
		break;

	default:
		/* 同じ画面だと作り直さないので、いったん画面なしにしてから作る */
		g_nAdminWindowType = 0;
		MakeAdminWnd (nType);
		break;
	}
	InvalidateRect (g_hWndAdmin, NULL, TRUE);
}

/* ========================================================================= */
/* 関数名：WndProcDialog													 */
/* 内容：ダイアログのメッセージ処理											 */
/* 日付：2004/03/02															 */
/* ========================================================================= */
LRESULT CALLBACK WndProcDialog(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
	HANDLE_MSG (hWnd, WM_COMMAND, AdminOnCommand);
	HANDLE_MSG (hWnd, WM_HSCROLL, AdminOnHScroll);
	HANDLE_MSG (hWnd, WM_VSCROLL, AdminOnVScroll);
	HANDLE_MSG (hWnd, WM_LBUTTONDOWN, AdminOnLButtonDown);
	HANDLE_MSG (hWnd, WM_RBUTTONDOWN, AdminOnRButtonDown);
	default:	return 0;
	}
	return 1;
}


/* ========================================================================= */
/* 関数名：AdminOnCommandMapParts											 */
/* 内容：WM_COMMAND処理(パーツ情報)											 */
/* 日付：2003/06/04															 */
/* ========================================================================= */
void AdminOnCommandMapParts(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	PMAPPARTSINFO pParts;
	PANIMEINFO pAnime;
	char szTmp[10];

	switch (id) {
	/* パーツ追加 */
	case IDC_BUTTON1:
		SendCmdMAPPARTSADD ();
		break;

	/* アニメ追加 */
	case IDC_BUTTON5:
		SendCmdMAPPARTSADDANIME (g_nSelectPartsID);
		break;

	/* サーバーに送信 */
	case IDC_BUTTON2:
		{
			pParts = (PMAPPARTSINFO)&g_pMapPartsInfo[g_nSelectPartsID];
			if (pParts->pAnimeInfo == NULL) {
				ZeroMemory (szTmp, sizeof (szTmp));
				GetWindowText (m_hWndEdit2, szTmp, sizeof (szTmp));
				pParts->byLevel = (BYTE)atoi (szTmp);
			}
			/* 灯りの範囲(0〜7マス) */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit3, szTmp, sizeof (szTmp));
			pParts->byLight = (BYTE)max (0, min (atoi (szTmp), 7));

			SendCmdMAPPARTS (g_nSelectPartsID);
		}
		break;

	/* 更新 */
	case IDC_BUTTON3:
		SendCmdRELOADPARTS (g_nSelectPartsID);
		break;

	/* ｸﾘｱ */
	case IDC_BUTTON4:
		{
			pParts = (PMAPPARTSINFO)&g_pMapPartsInfo[g_nSelectPartsID];
			if (pParts->pAnimeInfo) {
				SAFE_DELETE_ARRAY (pParts->pAnimeInfo);
			}
			ZeroMemory (pParts, sizeof (MAPPARTSINFO));

			char szTmp[16] = {0};
			g_byAdminAnimeNo = 0;
			SendMessage (m_hWndCombo1, CB_SETCURSEL, pParts->byHitType, 0);
			SetWindowText (m_hWndEdit1, szTmp);
			SetWindowText (m_hWndEdit2, "");
			SetWindowText (m_hWndEdit3, "");
			InvalidateRect (g_hWndAdmin, NULL, TRUE);
		}
		break;

	/* ← */
	case IDC_BUTTON6:
		pParts = (PMAPPARTSINFO)&g_pMapPartsInfo[g_nSelectPartsID];
		if (!pParts->byAnimeCount) {
			break;
		}
		ZeroMemory (szTmp, sizeof (szTmp));
		GetWindowText (m_hWndEdit1, szTmp, 10);
		pAnime = &pParts->pAnimeInfo[g_byAdminAnimeNo];
		pAnime->byWait = atoi (szTmp);

		GetWindowText (m_hWndEdit2, szTmp, sizeof (szTmp));
		pAnime->byLevel = (BYTE)atoi (szTmp);

		if (g_byAdminAnimeNo > 0) {
			g_byAdminAnimeNo --;
			pAnime = &pParts->pAnimeInfo[g_byAdminAnimeNo];
			wsprintf (szTmp, "%d", pAnime->byWait);
			SetWindowText (m_hWndEdit1, szTmp);
			wsprintf (szTmp, "%d", pAnime->byLevel);
			SetWindowText (m_hWndEdit2, szTmp);
			InvalidateRect (g_hWndAdmin, NULL, TRUE);
		}
		break;

	/* → */
	case IDC_BUTTON7:
		pParts = (PMAPPARTSINFO)&g_pMapPartsInfo[g_nSelectPartsID];
		if (!pParts->byAnimeCount) {
			break;
		}
		ZeroMemory (szTmp, sizeof (szTmp));
		GetWindowText (m_hWndEdit1, szTmp, 10);
		pAnime = &pParts->pAnimeInfo[g_byAdminAnimeNo];
		pAnime->byWait = atoi (szTmp);

		GetWindowText (m_hWndEdit2, szTmp, sizeof (szTmp));
		pAnime->byLevel = (BYTE)atoi (szTmp);

		if (g_byAdminAnimeNo < pParts->byAnimeCount - 1) {
			g_byAdminAnimeNo ++;
			pAnime = &pParts->pAnimeInfo[g_byAdminAnimeNo];
			wsprintf (szTmp, "%d", pAnime->byWait);
			SetWindowText (m_hWndEdit1, szTmp);
			wsprintf (szTmp, "%d", pAnime->byLevel);
			SetWindowText (m_hWndEdit2, szTmp);
			InvalidateRect (g_hWndAdmin, NULL, TRUE);
		}
		break;

	/* 種別 */
	case IDC_COMBO1:
		{
			(&g_pMapPartsInfo[g_nSelectPartsID])->byHitType =
					(BYTE)SendMessage (m_hWndCombo1, CB_GETCURSEL, 0, 0);
			InvalidateRect (g_hWndAdmin, NULL, TRUE);
		}
		break;

	/* 当たり判定マーク */
	case IDC_MARK:
		if (SendMessage (GetDlgItem (m_hWndDialog, IDC_MARK), BM_GETCHECK, 0, 0) & BST_CHECKED) {
			g_bMapMarkView = TRUE;
		} else {
			g_bMapMarkView = FALSE;
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnCommandMapInfo											 */
/* 内容：WM_COMMAND処理(マップ情報)											 */
/* 日付：2003/06/04															 */
/* ========================================================================= */
void AdminOnCommandMapInfo(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id) {
	/* 座標 */
	case IDC_TMPCHECK1:
		if (SendMessage (m_hWndCheck1, BM_GETCHECK, 0, 0) & BST_CHECKED) {
			g_bAdminPos = TRUE;

		} else {
			g_bAdminPos = FALSE;
		}
		/* 明るさ・灯り・見え方の設定を開いている間は座標を拾い続ける */
		if (IsDlgMapLightOpen ()) {
			g_bAdminPos = TRUE;
		}
		break;

	/* 明るさ・灯り・見え方の設定 */
	case IDC_BUTTON2:
		ShowDlgMapLight (g_hWndAdmin);
		break;

	/* 明るさ・灯り・見え方の設定を閉じた */
	case IDOK:
	case IDCANCEL:
		AdminMapInfoRefresh ();
		break;

	/* ｴﾌｪｸﾄ許可 */
	case IDC_TMPCHECK2:
		if (SendMessage (m_hWndCheck2, BM_GETCHECK, 0, 0) & BST_CHECKED) {
			g_pMapInfo->byFlg |= MAPFLG_EFECT;

		} else {
			g_pMapInfo->byFlg &= ~MAPFLG_EFECT;
		}
		break;

	/* 戦闘許可 */
	case IDC_TMPCHECK3:
		if (SendMessage (m_hWndCheck3, BM_GETCHECK, 0, 0) & BST_CHECKED) {
			g_pMapInfo->byFlg |= MAPFLG_BATTLE;

		} else {
			g_pMapInfo->byFlg &= ~MAPFLG_BATTLE;
		}
		break;

	/* 暗闇 */
	case IDC_TMPCHECK4:
		if (SendMessage (m_hWndCheck4, BM_GETCHECK, 0, 0) & BST_CHECKED) {
			g_pMapInfo->byFlg |= MAPFLG_DARK;

		} else {
			g_pMapInfo->byFlg &= ~MAPFLG_DARK;
		}
		break;

	/* マップ内放送 */
	case IDC_TMPCHECK5:
		if (SendMessage (m_hWndCheck5, BM_GETCHECK, 0, 0) & BST_CHECKED) {
			g_pMapInfo->byFlg |= MAPFLG_SPEECH;

		} else {
			g_pMapInfo->byFlg &= ~MAPFLG_SPEECH;
		}
		break;

	/* 移動効果禁止 */
	case IDC_TMPCHECK6:
		if (SendMessage (m_hWndCheck6, BM_GETCHECK, 0, 0) & BST_CHECKED) {
			g_pMapInfo->byFlg |= MAPFLG_MOVE;

		} else {
			g_pMapInfo->byFlg &= ~MAPFLG_MOVE;
		}
		break;

	/* 魔法禁止 */
	/* キャラの灯り無し */
	case IDC_TMPCHECK8:
		if (SendMessage (m_hWndCheck8, BM_GETCHECK, 0, 0) & BST_CHECKED) {
			g_pMapInfo->byFlg |= MAPFLG_NOCHARLIGHT;

		} else {
			g_pMapInfo->byFlg &= ~MAPFLG_NOCHARLIGHT;
		}
		break;

	case IDC_TMPCHECK7:
		if (SendMessage (m_hWndCheck7, BM_GETCHECK, 0, 0) & BST_CHECKED) {
			g_pMapInfo->byFlg |= MAPFLG_MAGIC;

		} else {
			g_pMapInfo->byFlg &= ~MAPFLG_MAGIC;
		}
		break;

	/* 送信 */
	case IDC_BUTTON1:
		{
			char szTmp[130];
			BYTE r, g, b;
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit1, szTmp, 10);
			szTmp[9] = 0;
			r = (BYTE)atoi (szTmp);
			GetWindowText (m_hWndEdit2, szTmp, 10);
			szTmp[9] = 0;
			g = (BYTE)atoi (szTmp);
			GetWindowText (m_hWndEdit3, szTmp, 10);
			szTmp[9] = 0;
			b = (BYTE)atoi (szTmp);
			g_pMapInfo->clChg = RGB (r, g, b);

			GetWindowText (m_hWndEdit5, szTmp, 4);
			szTmp[3] = 0;
			BYTE byTmp = (BYTE)atoi (szTmp);
			byTmp = min (byTmp, 100);
			g_pMapInfo->byLightLevel = byTmp;

			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit6, szTmp, 4);
			g_pMapInfo->nEntranceX = atoi(szTmp);

			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit7, szTmp, 4);
			g_pMapInfo->nEntranceY = atoi(szTmp);

			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit4, szTmp, 128);
			SAFE_DELETE_ARRAY (g_pMapInfo->pszName);
			g_pMapInfo->pszName = new char[strlen (szTmp) + 1];
			ZeroMemory (g_pMapInfo->pszName, strlen (szTmp) + 1);
			strcat ((LPSTR)g_pMapInfo->pszName, szTmp);

			/* 明るさの範囲・光源・見え方は「明るさ・灯り・見え方の設定」で決めたものをそのまま送る */
			SendCmdCHGMAPINFO ();
			MapLightDlgOnSent ();
		}
		break;

	/* BGM選択 */
	case IDC_TMPCOMBO1:
		if (g_pMapInfo->wBGM < SOUND_MAX && aSoundTbl[g_pMapInfo->wBGM]) {
			DXAudio.SetLoopPoints (&aSoundTbl[g_pMapInfo->wBGM], 0);
		}
		g_pMapInfo->wBGM = (WORD)GetAdminBgmNo ((int)SendMessage (m_hWndCombo1, CB_GETCURSEL, 0, 0));
		break;

	/* 天候 */
	case IDC_TMPCOMBO2:
		g_pMapInfo->byWeather = (BYTE)SendMessage (m_hWndCombo2, CB_GETCURSEL, 0, 0);
		break;

	/* 上に追加 */
	case IDC_ADDMAPUP:
		{
			char szBuf[10];
			ZeroMemory (szBuf, sizeof (szBuf));
			GetWindowText (m_hWndMapAddDelCount, szBuf, 10);
			SendCmdADDDELMAP (0, (BYTE)atoi (szBuf));
		}
		break;

	/* 下に追加 */
	case IDC_ADDMAPDOWN:
		{
			char szBuf[10];
			ZeroMemory (szBuf, sizeof (szBuf));
			GetWindowText (m_hWndMapAddDelCount, szBuf, 10);
			SendCmdADDDELMAP (1, (BYTE)atoi (szBuf));
		}
		break;

	/* 左に追加 */
	case IDC_ADDMAPLEFT:
		{
			char szBuf[10];
			ZeroMemory (szBuf, sizeof (szBuf));
			GetWindowText (m_hWndMapAddDelCount, szBuf, 10);
			SendCmdADDDELMAP (2, (BYTE)atoi (szBuf));
		}
		break;

	/* 右に追加 */
	case IDC_ADDMAPRIGHT:
		{
			char szBuf[10];
			ZeroMemory (szBuf, sizeof (szBuf));
			GetWindowText (m_hWndMapAddDelCount, szBuf, 10);
			SendCmdADDDELMAP (3, (BYTE)atoi (szBuf));
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnCommandMapEvent											 */
/* 内容：WM_COMMAND処理(イベント情報)										 */
/* 日付：2003/06/04															 */
/* ========================================================================= */
void AdminOnCommandMapEvent(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id) {
	/* 取得 */
	case IDC_GETEVENT:
		if (g_nSelectEventID >= g_nEventCount) {
			g_nSelectEventID = g_nEventCount - 1;
		}
		if (g_nSelectEventID < 0) {
			break;
		}
		SendCmdEVENTGET (g_nSelectEventID);
		break;

	/* 追加 */
	case IDC_ADDEVENT:
		SendCmdEVENTADD ();
		break;

	/* 削除 */
	case IDC_BUTTON3:
		if (!g_pEventInfo) {
			break;
		}
		SAFE_DELETE_ARRAY (g_pEventInfo->pData);
		ZeroMemory (g_pEventInfo, sizeof (EVENTINFO));
		MakeAdminWndEventInfoType ();
		InvalidateRect (g_hWndAdmin, NULL, TRUE);
		SendMessage (m_hWndEventList, CB_SETCURSEL, 0, 0);
		PostMessage (hWnd, WM_COMMAND, (WPARAM)IDC_SENDEVENT, 0);
		PostMessage (hWnd, WM_COMMAND, (WPARAM)IDC_GETEVENT, 0);
		break;

	/* 種別 */
	case IDC_TMPCOMBO1:
		{
			if (g_nEventCount == 0) {
				break;
			}

			if (!g_pEventInfo) {
				break;
			}

			g_pEventInfo->byType = (BYTE)SendMessage (m_hWndEventList, CB_GETCURSEL, 0, 0);

			int nSize = GetEventDataSize (g_pEventInfo->byType);
			SAFE_DELETE_ARRAY (g_pEventInfo->pData);
			if (nSize) {
				g_pEventInfo->pData = new BYTE[nSize];
				ZeroMemory (g_pEventInfo->pData, nSize);
			}

			MakeAdminWndEventInfoType ();
			InvalidateRect (g_hWndAdmin, NULL, TRUE);
		}
		break;

	/* 移動エフェクト種別 */
	case IDC_TMPCOMBO2:
		{
			if (g_nEventCount == 0) {
				break;
			}

			if (!g_pEventInfo) {
				break;
			}

			if (g_pEventInfo->byType == EVENTTYPE_MOVE) {
				PEVENTINFO_MOVE pMove = (PEVENTINFO_MOVE)g_pEventInfo->pData;
				pMove->byEfect = (BYTE)SendMessage (m_hWndCombo2, CB_GETCURSEL, 0, 0);
				break;
			}
			if (g_pEventInfo->byType == EVENTTYPE_MAPCHG) {
				PEVENTINFO_MAPCHG pMap = (PEVENTINFO_MAPCHG)g_pEventInfo->pData;
				pMap->byEfect = (BYTE)SendMessage (m_hWndCombo2, CB_GETCURSEL, 0, 0);
				break;
			}
			if (g_pEventInfo->byType == EVENTTYPE_MSG) {
				PEVENTINFO_MSG pMsg = (PEVENTINFO_MSG)g_pEventInfo->pData;
				pMsg->byType = (BYTE)SendMessage (m_hWndCombo2, CB_GETCURSEL, 0, 0);
				break;
			}
			if (g_pEventInfo->byType == EVENTTYPE_DOOR) {
				PEVENTINFO_DOOR pDoor = (PEVENTINFO_DOOR)g_pEventInfo->pData;
				pDoor->byType = (BYTE)SendMessage (m_hWndCombo2, CB_GETCURSEL, 0, 0);
				break;
			}
			if (g_pEventInfo->byType == EVENTTYPE_HPMANA) {
				PEVENTINFO_HPMANA pHpMp = (PEVENTINFO_HPMANA)g_pEventInfo->pData;
				pHpMp->byType = (BYTE)SendMessage (m_hWndCombo2, CB_GETCURSEL, 0, 0);
				break;
			}
		}
		break;

	/* 送信 */
	case IDC_SENDEVENT:
		if (g_nEventCount == 0) {
			break;
		}
		if (g_pEventInfo->pData) {
			switch (g_pEventInfo->byType) {
			case EVENTTYPE_MOVE:
				{
					PEVENTINFO_MOVE pMove = (PEVENTINFO_MOVE)g_pEventInfo->pData;
					char szTmp[10];
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit1, szTmp, 5);
					pMove->dwX = atoi (szTmp);
					GetWindowText (m_hWndEdit2, szTmp, 5);
					pMove->dwY = atoi (szTmp);
					pMove->byEfect = (BYTE)SendMessage (m_hWndCombo2, CB_GETCURSEL, 0, 0);
				}
				break;

			case EVENTTYPE_MAPCHG:
				{
					PEVENTINFO_MAPCHG pMap = (PEVENTINFO_MAPCHG)g_pEventInfo->pData;
					char szTmp[10];
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit1, szTmp, 5);
					pMap->dwMapID = atoi (szTmp);

					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit2, szTmp, 5);
					pMap->dwX = atoi (szTmp);

					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit3, szTmp, 5);
					pMap->dwY = atoi (szTmp);
					pMap->byEfect = (BYTE)SendMessage (m_hWndCombo2, CB_GETCURSEL, 0, 0);

					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit4, szTmp, 5);
					int nRange = atoi (szTmp);
					pMap->byRandomRange = (BYTE)((nRange < 0) ? 0 : (nRange > 255) ? 255 : nRange);
				}
				break;

			case EVENTTYPE_MSG:			/* メッセージ */
				{
					PEVENTINFO_MSG pMsg = (PEVENTINFO_MSG)g_pEventInfo->pData;
					char szTmp[10];
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit1, szTmp, 5);
					pMsg->dwNo = atoi (szTmp);
					pMsg->byType = (BYTE)SendMessage (m_hWndCombo2, CB_GETCURSEL, 0, 0);

					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit2, szTmp, 5);
					pMsg->wGrpID = atoi (szTmp);
					
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit3, szTmp, 5);
					pMsg->dwX = atoi (szTmp);

					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit4, szTmp, 5);
					pMsg->dwY = atoi (szTmp);
				}
				break;

			case EVENTTYPE_DOOR:
				{
					PEVENTINFO_DOOR pDoor = (PEVENTINFO_DOOR)g_pEventInfo->pData;
					char szTmp[10];
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit1, szTmp, 5);
					pDoor->wGrpID = atoi (szTmp);

					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit2, szTmp, 5);
					pDoor->wBackGrpID = atoi (szTmp);

					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit3, szTmp, 5);
					pDoor->byTimeClose = (BYTE)atoi (szTmp);
					pDoor->byType = (BYTE)SendMessage (m_hWndCombo2, CB_GETCURSEL, 0, 0);
				}
				break;

			case EVENTTYPE_HPMANA:
				{
					PEVENTINFO_HPMANA pHpMp = (PEVENTINFO_HPMANA)g_pEventInfo->pData;
					pHpMp->byType = (BYTE)SendMessage (m_hWndCombo2, CB_GETCURSEL, 0, 0);
				}
				break;

			case EVENTTYPE_ONCHARMAP:	/* キャラが乗っている間マップ変更 */
				{
					PEVENTINFO_ONCHARMAP pInfo = (PEVENTINFO_ONCHARMAP)g_pEventInfo->pData;
					char szTmp[10];
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit1, szTmp, 5);
					pInfo->wGrpIDOn = atoi (szTmp);

					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit2, szTmp, 5);
					pInfo->wGrpIDOff = atoi (szTmp);
					
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit3, szTmp, 5);
					pInfo->dwX = atoi (szTmp);

					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit4, szTmp, 5);
					pInfo->dwY = atoi (szTmp);
				}
				break;
			}
		}
		SendCmdEVENTINFO ();
		break;

	/* ← */
	case IDC_BUTTON1:
	/* → */
	case IDC_BUTTON2:
		if (g_nEventCount == 0) {
			break;
		}
	case IDC_RENEW:
		{
			if (g_nEventCount == 0) {
				break;
			}

			if (!g_pEventInfo) {
				break;
			}
			if (id == IDC_BUTTON1) {
				if (g_nSelectEventID > 0) {
					if (GetKeyState (VK_SHIFT) & 0x8000) {
						if (g_nSelectEventID >= 5) {
							g_nSelectEventID -= 5;
						}
					} else {
						g_nSelectEventID --;
					}
					SendCmdEVENTGET (g_nSelectEventID);
				}
				break;
			} else if (id == IDC_BUTTON2) {
				if (g_nSelectEventID < g_nEventCount - 1) {
					if (GetKeyState (VK_SHIFT) & 0x8000) {
						if (g_nSelectEventID < g_nEventCount - 5) {
							g_nSelectEventID += 5;
						}
					} else {
						g_nSelectEventID ++;
					}
					SendCmdEVENTGET (g_nSelectEventID);
				}
				break;
			}
			MakeAdminWndEventInfoType ();
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnCommandAddChar											 */
/* 内容：WM_COMMAND処理(キャラ追加)											 */
/* 日付：2003/06/08															 */
/* ========================================================================= */
void AdminOnCommandAddChar(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id) {
	/* 送信 */
	case IDC_BUTTON1:
		/* キャラ名を取得 */
		GetWindowText (m_hWndEdit1, m_pAddCharInfo->szName, sizeof (m_pAddCharInfo->szName));
		m_pAddCharInfo->byDirection = (BYTE)SendMessage (m_hWndCombo1, CB_GETCURSEL, 0, 0);
		m_pAddCharInfo->byNpcType = (BYTE)SendMessage (m_hWndCombo2, CB_GETCURSEL, 0, 0);
		SendCmdADDCHAR (m_pAddCharInfo);
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnCommandCharInfo											 */
/* 内容：WM_COMMAND処理(キャラ情報)											 */
/* 日付：2003/06/09															 */
/* ========================================================================= */
void AdminOnCommandCharInfo(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	/* ルーラの入力欄が書き換えられたら、その場で記録して保存する
	   （ブラウザ版はウィンドウを閉じる処理が来ないので、終了時の保存だけでは残らない） */
	if (codeNotify == EN_CHANGE && hWndCtl &&
		(hWndCtl == m_hWndEdit6 || hWndCtl == m_hWndEdit7 || hWndCtl == m_hWndEdit8 || hWndCtl == m_hWndEdit9)) {
		RecordAdminRuraEdit (hWndCtl);
		return;
	}

	switch (id) {
	/* BAN */
	case IDC_BUTTON1:
		if (!m_pParaCharInfo) {
			break;
		}
		SendCmdBANCHAR (m_pParaCharInfo->dwTblID);
		break;

	/* 取得 */
	case IDC_BUTTON13:
		SendCmdGETACCINFO (m_pParaCharInfo->dwTblID);
		break;

	/* 画像 */
	case IDC_BUTTON16:
		/* 画像選択ウィンドウ表示 */
		ShowSelectCharGrpWindow (hWnd, 0);
		break;

	/* 削除 */
	case IDC_BUTTON9:
		if (!m_pParaCharInfo) {
			break;
		}
		if (MessageBox (hWnd, "選択されたキャラ情報を削除します。\nよろしいですか？",
				"削除確認", MB_YESNO | MB_ICONQUESTION) != IDYES) {
			break;
		}
		SendCmdDELCHAR (m_pParaCharInfo->dwTblID);
		break;

	/* アカウント・パスワード取得 */
	case IDC_BUTTON10:
		if (!m_pParaCharInfo) {
			break;
		}
		SendCmdGETACC (m_pParaCharInfo->dwTblID);
		break;

	/* アカウント・パスワード更新 */
	case IDC_BUTTON11:
		{
			if (!m_pParaCharInfo) {
				break;
			}

			LPSTR pszTmp = new char[MAX_ACCNAME],
				  pszTmp2 = new char[MAX_ACCNAME];
			ZeroMemory (pszTmp, MAX_ACCNAME);
			ZeroMemory (pszTmp2, MAX_ACCNAME);
			GetWindowText (m_hWndEdit10, pszTmp, MAX_ACCNAME);
			GetWindowText (m_hWndEdit11, pszTmp2, MAX_ACCNAME);
			if (strlen (pszTmp)) {
				SendCmdCHGACC (m_pParaCharInfo->dwTblID, pszTmp, pszTmp2);
				SetWindowText (m_hWndEdit10, "");
				SetWindowText (m_hWndEdit11, "");
			}
			SAFE_DELETE_ARRAY (pszTmp);
			SAFE_DELETE_ARRAY (pszTmp2);
		}
		break;

	/* 向き */
	case IDC_TMPCOMBO1:
		m_pParaCharInfo->byDirection = (BYTE)SendMessage (m_hWndCombo1, CB_GETCURSEL, 0, 0);
		InvalidateRect (g_hWndAdmin, NULL, TRUE);
		break;

	/* NPC移動種別 */
	case IDC_TMPCOMBO2:
		m_pParaCharInfo->byNpcType = (BYTE)SendMessage (m_hWndCombo2, CB_GETCURSEL, 0, 0);
		break;

	/* 管理者権限 */
	case IDC_TMPCOMBO3:
		m_pParaCharInfo->byAdminLV = (BYTE)SendMessage (m_hWndCombo3, CB_GETCURSEL, 0, 0);
		break;
	/* 名前の色 */

	case IDC_TMPCOMBO4:
		m_pParaCharInfo->clName = aColorTbl[(BYTE)SendMessage (m_hWndCombo4, CB_GETCURSEL, 0, 0)];
		m_pParaCharInfo->clSpeech = aColorTbl[(BYTE)SendMessage (m_hWndCombo4, CB_GETCURSEL, 0, 0)];
		break;

	/* ステータステンプレート情報 */
	case IDC_TMPCOMBO5:
		m_pParaCharInfo->dwStTmplateID = (DWORD)SendMessage (m_hWndCombo5, CB_GETCURSEL, 0, 0);
		if (m_pParaCharInfo->dwStTmplateID == 0) {
			m_pParaCharInfo->dwStTmplateID = -1;
		} else {
			m_pParaCharInfo->dwStTmplateID --;
		}
		break;

	/* 送信 */
	case IDC_BUTTON2:
		{
			if (!m_pParaCharInfo) {
				break;
			}

			int nSize = 601;
			LPSTR pszTmp = new char[nSize];
			ZeroMemory (pszTmp, nSize);
			GetWindowText (m_hWndEdit1, pszTmp, nSize - 1);
			SAFE_DELETE_ARRAY (m_pParaCharInfo->pszNpcSpeech);
			m_pParaCharInfo->pszNpcSpeech = new char[strlen (pszTmp) + 1];
			ZeroMemory (m_pParaCharInfo->pszNpcSpeech, strlen (pszTmp) + 1);
			wsprintf (m_pParaCharInfo->pszNpcSpeech, "%s", pszTmp);

			ZeroMemory (pszTmp, nSize);
			ZeroMemory (m_pParaCharInfo->szName, sizeof (m_pParaCharInfo->szName));
			GetWindowText (m_hWndEdit5, pszTmp, sizeof (m_pParaCharInfo->szName));
			wsprintf (m_pParaCharInfo->szName, "%s", pszTmp);
			m_pParaCharInfo->szName[sizeof (m_pParaCharInfo->szName) - 1] = 0;

			/* NPC発生関連 */
			ZeroMemory (pszTmp, nSize);
			GetWindowText (m_hWndEdit12, pszTmp, 4);
			m_pParaCharInfo->byAreaX = (BYTE)atoi (pszTmp);

			ZeroMemory (pszTmp, nSize);
			GetWindowText (m_hWndEdit13, pszTmp, 4);
			m_pParaCharInfo->byAreaY = (BYTE)atoi (pszTmp);

			ZeroMemory (pszTmp, nSize);
			GetWindowText (m_hWndEdit14, pszTmp, 4);
			m_pParaCharInfo->byPutCount = (BYTE)atoi (pszTmp);

			ZeroMemory (pszTmp, nSize);
			GetWindowText (m_hWndEdit15, pszTmp, 6);
			m_pParaCharInfo->wTimePutInterval = (WORD)atoi (pszTmp);

			ZeroMemory (pszTmp, nSize);
			GetWindowText (m_hWndEdit16, pszTmp, 6);
			m_pParaCharInfo->dwStartMapID = (WORD)atoi (pszTmp);

			ZeroMemory (pszTmp, nSize);
			GetWindowText (m_hWndEdit17, pszTmp, 6);
			m_pParaCharInfo->dwStartX = (WORD)atoi (pszTmp);
			
			ZeroMemory (pszTmp, nSize);
			GetWindowText (m_hWndEdit18, pszTmp, 6);
			m_pParaCharInfo->dwStartY = (WORD)atoi (pszTmp);
			
			ZeroMemory (pszTmp, nSize);
			GetWindowText (m_hWndEdit19, pszTmp, 6);
			m_pParaCharInfo->byMoveProbabilityX = (BYTE)atoi (pszTmp);
			ZeroMemory (pszTmp, nSize);
			GetWindowText (m_hWndEdit20, pszTmp, 6);
			m_pParaCharInfo->byMoveProbabilityY = (BYTE)atoi (pszTmp);
			ZeroMemory (pszTmp, nSize);
			GetWindowText (m_hWndEdit21, pszTmp, 6);
			m_pParaCharInfo->bySearchCx = (BYTE)atoi (pszTmp);
			ZeroMemory (pszTmp, nSize);
			GetWindowText (m_hWndEdit22, pszTmp, 6);
			m_pParaCharInfo->bySearchCy = (BYTE)atoi (pszTmp);
			
			SendCmdCHARINFO (m_pParaCharInfo);
			SAFE_DELETE_ARRAY (pszTmp);
		}
		break;

	/* マップ移動 */
	case IDC_BUTTON3:
		{
			if (!m_pParaCharInfo) {
				break;
			}
			if ((m_pParaCharInfo->szAcc[0] == 0) &&
				(m_pParaCharInfo->byNpcType == 0) &&
				(m_pParaCharInfo->dwMapID == 0) &&
				(m_pParaCharInfo->dwX == 0) &&
				(m_pParaCharInfo->dwY == 0)) {
				break;
			}

			char szTmp[10];
			DWORD dwX, dwY, dwMapID;
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit2, szTmp, 9);
			if (strlen (szTmp) == 0) {
				break;
			}
			dwMapID = (DWORD)atoi (szTmp);
			GetWindowText (m_hWndEdit3, szTmp, 9);
			if (strlen (szTmp) == 0) {
				break;
			}
			dwX = (DWORD)atoi (szTmp);
			GetWindowText (m_hWndEdit4, szTmp, 9);
			if (strlen (szTmp) == 0) {
				break;
			}
			dwY = (DWORD)atoi (szTmp);
			SendCmdMAPMOVE (m_pParaCharInfo->dwTblID, dwX, dwY, dwMapID, 0);
		}
		break;

	/* ルーラ← */
	case IDC_BUTTON14:
	/* ルーラ→ */
	case IDC_BUTTON15:
		{
			char szTmp[20];
			PADMINRURAINFO pInfo = NULL;
			/* まず、現在表示されている内容を記録 */
			if (g_AdminRuraInfo.GetSize ()) {
				 pInfo = g_AdminRuraInfo[g_dwSelctRura];
			}
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
			if (pInfo) {
				pInfo->dwMapID = atoi (szTmp);
			}

			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit7, szTmp, sizeof (szTmp));
			if (pInfo) {
				pInfo->dwX = atoi (szTmp);
			}

			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit8, szTmp, sizeof (szTmp));
			if (pInfo) {
				pInfo->dwY = atoi (szTmp);
			}

			/* コメントは全角を含むので、広い欄で受けてから文字の途中で切らずに記録する */
			{
				char szComment[256];
				ZeroMemory (szComment, sizeof (szComment));
				GetWindowText (m_hWndEdit9, szComment, sizeof (szComment));
				if (pInfo) {
					SboTextCopy (pInfo->szComment, sizeof (pInfo->szComment), szComment);
				}
			}

			SaveAdminRura ();

			if (id == IDC_BUTTON14) {
				if (g_dwSelctRura < 1) {
					break;
				} else {
					g_dwSelctRura --;
				}
			}

			if (id == IDC_BUTTON15) {
				if (g_dwSelctRura + 1 >= (DWORD)g_AdminRuraInfo.GetSize ()) {
					break;
				} else {
					g_dwSelctRura ++;
				}
			}
			pInfo = g_AdminRuraInfo[g_dwSelctRura];
			wsprintf (szTmp, "%ld", pInfo->dwMapID);
			SetWindowText (m_hWndEdit6, szTmp);

			wsprintf (szTmp, "%ld", pInfo->dwX);
			SetWindowText (m_hWndEdit7, szTmp);

			wsprintf (szTmp, "%ld", pInfo->dwY);
			SetWindowText (m_hWndEdit8, szTmp);

			SetWindowText (m_hWndEdit9, pInfo->szComment);
			InvalidateRect (hWnd, NULL, TRUE);
		}
		break;

	/* GET */
	case IDC_BUTTON6:
		{
			char szTmp[10];
			PADMINRURAINFO pInfo = NULL;
			if (g_AdminRuraInfo.GetSize ()) {
				 pInfo = g_AdminRuraInfo[g_dwSelctRura];
			}
			wsprintf (szTmp, "%ld", m_pParaCharInfo->dwMapID);
			SetWindowText (m_hWndEdit6, szTmp);
			if (pInfo) {
				pInfo->dwMapID = atoi (szTmp);
			}

			wsprintf (szTmp, "%ld", m_pParaCharInfo->dwX);
			SetWindowText (m_hWndEdit7, szTmp);
			if (pInfo) {
				pInfo->dwX = atoi (szTmp);
			}

			wsprintf (szTmp, "%ld", m_pParaCharInfo->dwY);
			SetWindowText (m_hWndEdit8, szTmp);
			if (pInfo) {
				pInfo->dwY = atoi (szTmp);
			}

			SetWindowText (m_hWndEdit9, "");
			if (pInfo) {
				ZeroMemory (pInfo->szComment, sizeof (pInfo->szComment));
			}
			SaveAdminRura ();
		}
		break;

	/* PUT */
	case IDC_BUTTON7:
		{
			char szTmp[10];
			GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
			SetWindowText (m_hWndEdit2, szTmp);

			GetWindowText (m_hWndEdit7, szTmp, sizeof (szTmp));
			SetWindowText (m_hWndEdit3, szTmp);

			GetWindowText (m_hWndEdit8, szTmp, sizeof (szTmp));
			SetWindowText (m_hWndEdit4, szTmp);
		}
		break;

	/* ADD */
	case IDC_BUTTON8:
		{
			PADMINRURAINFO pInfoNew = new ADMINRURAINFO;
			ZeroMemory (pInfoNew, sizeof (ADMINRURAINFO));
			g_AdminRuraInfo.Add (pInfoNew);
			SaveAdminRura ();
			InvalidateRect (hWnd, NULL, TRUE);
		}
		break;
	}
}


/* ========================================================================= */
/* 関数名：RecordAdminRuraEdit												 */
/* 内容：管理人ルーラの入力欄を、選択中のルーラに記録して保存する			 */
/* ========================================================================= */
static void RecordAdminRuraEdit(HWND hWndCtl)
{
	char szTmp[256];
	PADMINRURAINFO pInfo;

	if (g_dwSelctRura >= (DWORD)g_AdminRuraInfo.GetSize ()) {
		return;
	}
	pInfo = g_AdminRuraInfo[g_dwSelctRura];

	/* 書き換えられた欄だけを記録する（Windows 版は SetWindowText でも通知が来るので、
	   ←→で表示を切り替えている途中に他の欄の古い値を書き込まないように） */
	ZeroMemory (szTmp, sizeof (szTmp));
	GetWindowText (hWndCtl, szTmp, sizeof (szTmp));
	if (hWndCtl == m_hWndEdit6) {
		pInfo->dwMapID = atoi (szTmp);
	} else if (hWndCtl == m_hWndEdit7) {
		pInfo->dwX = atoi (szTmp);
	} else if (hWndCtl == m_hWndEdit8) {
		pInfo->dwY = atoi (szTmp);
	} else {
		/* 欄より長い時も、文字の途中で切って「?」にならないようにする */
		SboTextCopy (pInfo->szComment, sizeof (pInfo->szComment), szTmp);
	}
	SaveAdminRura ();
}

/* ========================================================================= */
/* 関数名：AdminOnCommandCharStatus											 */
/* 内容：WM_COMMAND処理(キャラステータス情報)								 */
/* 日付：2003/10/01															 */
/* ========================================================================= */
void AdminOnCommandCharStatus(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id) {
	/* 送信 */
	case IDC_BUTTON1:
		if (m_dwSelectID == -1) {
			break;
		}
		{
			HWND *ahWndTbl[15] = {
				&m_hWndEdit1,
				&m_hWndEdit2,
				&m_hWndEdit3,
				&m_hWndEdit4,
				&m_hWndEdit5,
				&m_hWndEdit6,
				&m_hWndEdit7,
				&m_hWndEdit8,
				&m_hWndEdit9,
				&m_hWndEdit10,
				&m_hWndEdit12,
				&m_hWndEdit13,
				&m_hWndEdit14,
				&m_hWndEdit15,
				&m_hWndEdit16
			};
			PCHARINFO pChar = g_aCharInfo[m_dwSelectID];
			if (pChar->pID != m_dwSelectpID) {
				int nTmp = GetCharNo (m_dwSelectpID);
				if (nTmp < 0) {
					break;
				}
				m_dwSelectID = (DWORD)nTmp;
				pChar = g_aCharInfo[m_dwSelectID];
			}
			char szTmp[10];

			/* 力 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[0], szTmp, sizeof (szTmp));
			pChar->byPW = (BYTE)atoi (szTmp);
			SetWindowText (*ahWndTbl[0], "");
			/* 素早さ */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[1], szTmp, sizeof (szTmp));
			pChar->byDEX = (BYTE)atoi (szTmp);
			SetWindowText (*ahWndTbl[1], "");
			/* 体力 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[2], szTmp, sizeof (szTmp));
			pChar->byST = (BYTE)atoi (szTmp);
			SetWindowText (*ahWndTbl[2], "");
			/* 賢さ */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[3], szTmp, sizeof (szTmp));
			pChar->byIT = (BYTE)atoi (szTmp);
			SetWindowText (*ahWndTbl[3], "");
			/* 運のよさ */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[4], szTmp, sizeof (szTmp));
			pChar->byLK = (BYTE)atoi (szTmp);
			SetWindowText (*ahWndTbl[4], "");
			/* 最大HP */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[5], szTmp, sizeof (szTmp));
			pChar->wMaxHP = (WORD)atoi (szTmp);
			SetWindowText (*ahWndTbl[5], "");
			/* HP */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[6], szTmp, sizeof (szTmp));
			pChar->wHP = (WORD)atoi (szTmp);
			SetWindowText (*ahWndTbl[6], "");
			/* 最大Mana */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[7], szTmp, sizeof (szTmp));
			pChar->wMaxMana = (WORD)atoi (szTmp);
			SetWindowText (*ahWndTbl[7], "");
			/* Mana */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[8], szTmp, sizeof (szTmp));
			pChar->wMana = (WORD)atoi (szTmp);
			SetWindowText (*ahWndTbl[8], "");
			/* 所持金 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[9], szTmp, sizeof (szTmp));
			pChar->dwGold = (DWORD)atoi (szTmp);
			SetWindowText (*ahWndTbl[9], "");
			/* ドロップ率 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[10], szTmp, sizeof (szTmp));
			pChar->byDropProbability = (BYTE)atoi (szTmp);
			SetWindowText (*ahWndTbl[10], "");
			/* 移動確率X */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[11], szTmp, sizeof (szTmp));
			pChar->byMoveProbabilityX = (BYTE)atoi (szTmp);
			SetWindowText (*ahWndTbl[11], "");
			/* 移動確率Y */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[12], szTmp, sizeof (szTmp));
			pChar->byMoveProbabilityY = (BYTE)atoi (szTmp);
			SetWindowText (*ahWndTbl[12], "");
			/* 検索範囲X */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[13], szTmp, sizeof (szTmp));
			pChar->bySearchCx = (BYTE)atoi (szTmp);
			SetWindowText (*ahWndTbl[13], "");
			/* 検索範囲Y */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[14], szTmp, sizeof (szTmp));
			pChar->bySearchCy = (BYTE)atoi (szTmp);
			SetWindowText (*ahWndTbl[14], "");
			/* 火 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit17, szTmp, sizeof (szTmp));
			pChar->chAttrFire = (char)atoi (szTmp);
			SetWindowText (m_hWndEdit17, "");
			/* 水 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit18, szTmp, sizeof (szTmp));
			pChar->chAttrWater = (char)atoi (szTmp);
			SetWindowText (m_hWndEdit18, "");
			/* 風 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit19, szTmp, sizeof (szTmp));
			pChar->chAttrWind = (char)atoi (szTmp);
			SetWindowText (m_hWndEdit19, "");
			/* 土 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit20, szTmp, sizeof (szTmp));
			pChar->chAttrEarth = (char)atoi (szTmp);
			SetWindowText (m_hWndEdit20, "");
			/* 光 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit21, szTmp, sizeof (szTmp));
			pChar->chAttrLight = (char)atoi (szTmp);
			SetWindowText (m_hWndEdit21, "");
			/* 闇 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit22, szTmp, sizeof (szTmp));
			pChar->chAttrDark = (char)atoi (szTmp);
			SetWindowText (m_hWndEdit22, "");
			/* 魔法防御 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit23, szTmp, sizeof (szTmp));
			pChar->wDFMagic = (WORD)atoi (szTmp);
			SetWindowText (m_hWndEdit23, "");
			/* レベル */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit24, szTmp, sizeof (szTmp));
			pChar->byLevel = (BYTE)atoi (szTmp);
			SetWindowText (m_hWndEdit24, "");
			/* 経験値 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit25, szTmp, sizeof (szTmp));
			pChar->dwExp = (DWORD)atoi (szTmp);
			SetWindowText (m_hWndEdit25, "");
			/* 性別 */
			if (SendMessage (m_hWndCombo1, CB_GETCURSEL, 0, 0)) {
				pChar->bySaveFlg |= CHARINFOSAVEFLG_FEMALE;
			} else {
				pChar->bySaveFlg &= ~CHARINFOSAVEFLG_FEMALE;
			}
			SendMessage (m_hWndCombo1, CB_SETCURSEL, 0, 0);
			/* 移動種別 */
			pChar->byNpcType = (BYTE)SendMessage (m_hWndCombo2, CB_GETCURSEL, 0, 0);
			SendMessage (m_hWndCombo2, CB_SETCURSEL, 0, 0);
			/* 酔っ払いレベル */
			pChar->byDrunkLV = (BYTE)SendMessage (m_hWndCombo3, CB_GETCURSEL, 0, 0);
			pChar->byDrunkPoint = pChar->byDrunkLV * 10;
			SendMessage (m_hWndCombo3, CB_SETCURSEL, 0, 0);
		}
		SendCmdCHARSTATUS (m_dwSelectID);
		m_dwSelectID = -1;
		m_dwSelectpID = -1;
		break;

	/* 更新 */
	case IDC_BUTTON2:
	/* 追加 */
	case IDC_BUTTON3:
		{
			int nNo = ListView_GetNextItem (m_hWndList1, -1, LVNI_ALL | LVNI_SELECTED);
			LV_ITEM item;
			ZeroMemory (&item, sizeof (item));
			item.mask = LVIF_TEXT;
			item.iItem = g_aStTmpInfo.GetSize ();
			if (id == IDC_BUTTON2) {
				if (nNo < 0) {
					break;
				}
				item.iItem = nNo;
			}

			PSTATUSTEMPLATEINFO pInfo = NULL;
			if (id == IDC_BUTTON2) {
				pInfo = g_aStTmpInfo[nNo];
			} else {
				pInfo = new STATUSTEMPLATEINFO;
				ZeroMemory (pInfo, sizeof (STATUSTEMPLATEINFO));
			}

			HWND *ahWndTbl[15] = {
				&m_hWndEdit1,
				&m_hWndEdit2,
				&m_hWndEdit3,
				&m_hWndEdit4,
				&m_hWndEdit5,
				&m_hWndEdit6,
				&m_hWndEdit7,
				&m_hWndEdit8,
				&m_hWndEdit9,
				&m_hWndEdit10,
				&m_hWndEdit12,
				&m_hWndEdit13,
				&m_hWndEdit14,
				&m_hWndEdit15,
				&m_hWndEdit16
			};
			char szTmp[MAX_CHARNAMEBUF];

			/* 名前 */
			ZeroMemory (pInfo->szName, sizeof (pInfo->szName));
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit11, szTmp, sizeof (szTmp));
			strcat (pInfo->szName, szTmp);
			/* 力 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[0], szTmp, sizeof (szTmp));
			pInfo->byPW = (BYTE)atoi (szTmp);
			/* 素早さ */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[1], szTmp, sizeof (szTmp));
			pInfo->byDEX = (BYTE)atoi (szTmp);
			/* 体力 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[2], szTmp, sizeof (szTmp));
			pInfo->byST = (BYTE)atoi (szTmp);
			/* 賢さ */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[3], szTmp, sizeof (szTmp));
			pInfo->byIT = (BYTE)atoi (szTmp);
			/* 運のよさ */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[4], szTmp, sizeof (szTmp));
			pInfo->byLK = (BYTE)atoi (szTmp);
			/* 最大HP */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[5], szTmp, sizeof (szTmp));
			pInfo->wMaxHP = (WORD)atoi (szTmp);
			/* 最大Mana */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[7], szTmp, sizeof (szTmp));
			pInfo->wMaxMana = (WORD)atoi (szTmp);
			/* 所持金 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[9], szTmp, sizeof (szTmp));
			pInfo->dwGold = (DWORD)atoi (szTmp);
			/* ドロップ率 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[10], szTmp, sizeof (szTmp));
			pInfo->byDropProbability = (BYTE)atoi (szTmp);
			/* 移動確率X */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[11], szTmp, sizeof (szTmp));
			pInfo->byMoveProbabilityX = (BYTE)atoi (szTmp);
			/* 移動確率Y */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[12], szTmp, sizeof (szTmp));
			pInfo->byMoveProbabilityY = (BYTE)atoi (szTmp);
			/* 検索範囲X */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[13], szTmp, sizeof (szTmp));
			pInfo->bySearchCx = (BYTE)atoi (szTmp);
			/* 検索範囲Y */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (*ahWndTbl[14], szTmp, sizeof (szTmp));
			pInfo->bySearchCy = (BYTE)atoi (szTmp);

			/* 火 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit17, szTmp, sizeof (szTmp));
			pInfo->chAttrFire = (char)atoi (szTmp);
			/* 水 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit18, szTmp, sizeof (szTmp));
			pInfo->chAttrWater = (char)atoi (szTmp);
			/* 風 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit19, szTmp, sizeof (szTmp));
			pInfo->chAttrWind = (char)atoi (szTmp);
			/* 土 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit20, szTmp, sizeof (szTmp));
			pInfo->chAttrEarth = (char)atoi (szTmp);
			/* 光 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit21, szTmp, sizeof (szTmp));
			pInfo->chAttrLight = (char)atoi (szTmp);
			/* 闇 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit22, szTmp, sizeof (szTmp));
			pInfo->chAttrDark = (char)atoi (szTmp);
			/* 魔法防御 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit23, szTmp, sizeof (szTmp));
			pInfo->wDFMagic = (WORD)atoi (szTmp);
			/* レベル */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit24, szTmp, sizeof (szTmp));
			pInfo->byLevel = (BYTE)atoi (szTmp);
			/* 経験値 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit25, szTmp, sizeof (szTmp));
			pInfo->dwExp = (DWORD)atoi (szTmp);

			/* 移動種別 */
			pInfo->byNpcType = (BYTE)SendMessage (m_hWndCombo2, CB_GETCURSEL, 0, 0);
			if (id == IDC_BUTTON3) {
				g_aStTmpInfo.Add (pInfo);
				SendCmdSTTMPLATE (g_aStTmpInfo.GetSize () - 1, CMDSTTMPLATETYPE_ADD);
				g_aStTmpInfo.RemoveAt (g_aStTmpInfo.GetSize () - 1);
				SAFE_DELETE (pInfo);
			} else {
				SendCmdSTTMPLATE (nNo, CMDSTTMPLATETYPE_CHG);
			}
		}
		break;

	/* 反映 */
	case IDC_BUTTON4:
		{
//			if (m_dwSelectID == -1) {
//				break;
//			}
			int nNo = ListView_GetNextItem (m_hWndList1, -1, LVNI_ALL | LVNI_SELECTED);
			if (nNo < 0) {
				break;
			}
			LV_ITEM item;
			ZeroMemory (&item, sizeof (item));
			item.mask = LVIF_TEXT;
			item.iItem = g_aStTmpInfo.GetSize ();
			item.iItem = nNo;

			PSTATUSTEMPLATEINFO pInfo = NULL;
			HWND *ahWndTbl[15] = {
				&m_hWndEdit1,
				&m_hWndEdit2,
				&m_hWndEdit3,
				&m_hWndEdit4,
				&m_hWndEdit5,
				&m_hWndEdit6,
				&m_hWndEdit7,
				&m_hWndEdit8,
				&m_hWndEdit9,
				&m_hWndEdit10,
				&m_hWndEdit12,
				&m_hWndEdit13,
				&m_hWndEdit14,
				&m_hWndEdit15,
				&m_hWndEdit16
			};

			char szTmp[100];
			ZeroMemory (szTmp, sizeof (szTmp));

			/* 名前 */
			ListView_GetItemText (m_hWndList1, nNo, 0, szTmp, sizeof (szTmp) - 1);
			SetWindowText (m_hWndEdit11, szTmp);
			/* 力 */
			ListView_GetItemText (m_hWndList1, nNo, 2, szTmp, sizeof (szTmp) - 1);
			SetWindowText (*ahWndTbl[0], szTmp);
			/* 素早さ */
			ListView_GetItemText (m_hWndList1, nNo, 3, szTmp, sizeof (szTmp) - 1);
			SetWindowText (*ahWndTbl[1], szTmp);
			/* 体力 */
			ListView_GetItemText (m_hWndList1, nNo, 4, szTmp, sizeof (szTmp) - 1);
			SetWindowText (*ahWndTbl[2], szTmp);
			/* 賢さ */
			ListView_GetItemText (m_hWndList1, nNo, 5, szTmp, sizeof (szTmp) - 1);
			SetWindowText (*ahWndTbl[3], szTmp);
			/* 運のよさ */
			ListView_GetItemText (m_hWndList1, nNo, 6, szTmp, sizeof (szTmp) - 1);
			SetWindowText (*ahWndTbl[4], szTmp);
			/* 最大HP */
			ListView_GetItemText (m_hWndList1, nNo, 7, szTmp, sizeof (szTmp) - 1);
			SetWindowText (*ahWndTbl[5], szTmp);
			SetWindowText (*ahWndTbl[6], szTmp);
			/* 最大Mana */
			ListView_GetItemText (m_hWndList1, nNo, 8, szTmp, sizeof (szTmp) - 1);
			SetWindowText (*ahWndTbl[7], szTmp);
			SetWindowText (*ahWndTbl[8], szTmp);
			/* 所持金 */
			ListView_GetItemText (m_hWndList1, nNo, 9, szTmp, sizeof (szTmp) - 1);
			SetWindowText (*ahWndTbl[9], szTmp);
			/* ドロップ率 */
			ListView_GetItemText (m_hWndList1, nNo, 10, szTmp, sizeof (szTmp) - 1);
			SetWindowText (*ahWndTbl[10], szTmp);
			/* 移動確率X */
			ListView_GetItemText (m_hWndList1, nNo, 11, szTmp, sizeof (szTmp) - 1);
			SetWindowText (*ahWndTbl[11], szTmp);
			/* 移動確率Y */
			ListView_GetItemText (m_hWndList1, nNo, 12, szTmp, sizeof (szTmp) - 1);
			SetWindowText (*ahWndTbl[12], szTmp);
			/* 検索範囲X */
			ListView_GetItemText (m_hWndList1, nNo, 13, szTmp, sizeof (szTmp) - 1);
			SetWindowText (*ahWndTbl[13], szTmp);
			/* 検索範囲Y */
			ListView_GetItemText (m_hWndList1, nNo, 14, szTmp, sizeof (szTmp) - 1);
			SetWindowText (*ahWndTbl[14], szTmp);
			/* 火 */
			ListView_GetItemText (m_hWndList1, nNo, 15, szTmp, sizeof (szTmp) - 1);
			SetWindowText (m_hWndEdit17, szTmp);
			/* 水 */
			ListView_GetItemText (m_hWndList1, nNo, 16, szTmp, sizeof (szTmp) - 1);
			SetWindowText (m_hWndEdit18, szTmp);
			/* 風 */
			ListView_GetItemText (m_hWndList1, nNo, 17, szTmp, sizeof (szTmp) - 1);
			SetWindowText (m_hWndEdit19, szTmp);
			/* 土 */
			ListView_GetItemText (m_hWndList1, nNo, 18, szTmp, sizeof (szTmp) - 1);
			SetWindowText (m_hWndEdit20, szTmp);
			/* 光 */
			ListView_GetItemText (m_hWndList1, nNo, 19, szTmp, sizeof (szTmp) - 1);
			SetWindowText (m_hWndEdit21, szTmp);
			/* 闇 */
			ListView_GetItemText (m_hWndList1, nNo, 20, szTmp, sizeof (szTmp) - 1);
			SetWindowText (m_hWndEdit22, szTmp);
			/* 魔法防御 */
			ListView_GetItemText (m_hWndList1, nNo, 21, szTmp, sizeof (szTmp) - 1);
			SetWindowText (m_hWndEdit23, szTmp);
			/* レベル */
			ListView_GetItemText (m_hWndList1, nNo, 22, szTmp, sizeof (szTmp) - 1);
			SetWindowText (m_hWndEdit24, szTmp);
			/* 経験値 */
			ListView_GetItemText (m_hWndList1, nNo, 23, szTmp, sizeof (szTmp) - 1);
			SetWindowText (m_hWndEdit25, szTmp);
			/* 移動種別 */
			SendMessage (m_hWndCombo2, CB_SETCURSEL, g_aStTmpInfo[nNo]->byNpcType, 0);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnCommandCharLeave											 */
/* 内容：WM_COMMAND処理(預かり所情報)										 */
/* 日付：2004/02/07															 */
/* ========================================================================= */
void AdminOnCommandCharLeave(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id) {
	/* 取得 */
	case IDC_BUTTON1:
		{
			char szTmp[128];
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit1, szTmp, sizeof (szTmp));
			if (strlen (szTmp) == 0) {
				break;
			}
			SendCmdLEAVEINFOGET (atoi (szTmp));
		}
		break;

	/* 送信 */
	case IDC_BUTTON2:
		{
			char szTmp[10];
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit3, szTmp, sizeof (szTmp));
			g_pLeaveInfo->dwGold = (DWORD)atoi (szTmp);

			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit1, szTmp, sizeof (szTmp));
			SendCmdLEAVEINFO ((DWORD)atoi (szTmp));
		}
		break;

	/* 更新 */
	case IDC_BUTTON3:
		{
			int nNo = ListView_GetNextItem (m_hWndList1, -1, LVNI_ALL | LVNI_SELECTED);
			if (nNo < 0) {
				break;
			}
			int nItemID = SendMessage (m_hWndCombo1, CB_GETCURSEL, 0, 0);
			if (nItemID <= 0) {
				break;
			}

			char szTmp[10];
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit2, szTmp, sizeof (szTmp));
			if (strlen (szTmp) == 0) {
				break;
			}
			int nItemCount = (int)atoi (szTmp);
			if ((nItemCount == 0) || (nItemCount >= 256)) {
				break;
			}

			LV_ITEM item;
			ZeroMemory (&item, sizeof (item));
			item.mask = LVIF_TEXT;
			item.iItem = nNo;
			item.iSubItem = 0;
			item.pszText = g_aItemInfo[nItemID]->szName;
			ListView_SetItem (m_hWndList1, &item);

			item.iSubItem = 1;
			item.pszText = szTmp;
			ListView_SetItem (m_hWndList1, &item);

			g_pLeaveInfo->waItem[nNo] = nItemID;
			g_pLeaveInfo->byItemCount[nNo] = nItemCount;
		}
		break;

	/* 削除 */
	case IDC_BUTTON4:
		{
			int nNo = ListView_GetNextItem (m_hWndList1, -1, LVNI_ALL | LVNI_SELECTED);
			if (nNo < 0) {
				break;
			}
			
			int i;
			for (i = nNo; i < MAX_LEAVEITEM - 1; i ++) {
				g_pLeaveInfo->waItem[i] = g_pLeaveInfo->waItem[i + 1];
				g_pLeaveInfo->byItemCount[i] = g_pLeaveInfo->byItemCount[i + 1];
			}
			g_pLeaveInfo->waItem[i] = 0;
			g_pLeaveInfo->byItemCount[i] = 0;
			g_pLeaveInfo->byCount --;

			ListView_DeleteItem (m_hWndList1, nNo);
		}
		break;

	/* 追加 */
	case IDC_BUTTON5:
		{
			int nCount = ListView_GetItemCount (m_hWndList1);
			if (nCount >= MAX_LEAVEITEM) {
				break;
			}

			int nItemID = SendMessage (m_hWndCombo1, CB_GETCURSEL, 0, 0);
			if (nItemID <= 0) {
				break;
			}

			char szTmp[10];
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit2, szTmp, sizeof (szTmp));
			if (strlen (szTmp) == 0) {
				break;
			}
			int nItemCount = (int)atoi (szTmp);
			if ((nItemCount == 0) || (nItemCount >= 256)) {
				break;
			}

			LV_ITEM item;
			ZeroMemory (&item, sizeof (item));
			item.mask = LVIF_TEXT;
			item.iItem = nCount;
			item.iSubItem = 0;
			item.pszText = g_aItemInfo[nItemID]->szName;
			ListView_InsertItem (m_hWndList1, &item);

			item.iSubItem = 1;
			item.pszText = szTmp;
			ListView_SetItem (m_hWndList1, &item);

			g_pLeaveInfo->waItem[nCount] = nItemID;
			g_pLeaveInfo->byItemCount[nCount] = nItemCount;
			g_pLeaveInfo->byCount ++;
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnCommandAddMap												 */
/* 内容：WM_COMMAND処理(マップ追加)											 */
/* 日付：2003/07/06															 */
/* ========================================================================= */
void AdminOnCommandAddMap(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id) {
	/* 追加 */
	case IDC_BUTTON1:
		SendCmdADDMAP ();
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnCommandMsg												 */
/* 内容：WM_COMMAND処理(メッセージ)											 */
/* 日付：2003/07/15															 */
/* ========================================================================= */
void AdminOnCommandMsg(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id) {
	/* ← */
	case IDC_BUTTON1:
		{
			if (g_nSelectPartsID > 0) {
				g_nSelectPartsID --;
				SendCmdMAPMSGREQ (g_nSelectPartsID);
			}
			InvalidateRect (g_hWndAdmin, NULL, TRUE);
		}
		break;

	/* → */
	case IDC_BUTTON2:
		{
			if (g_nSelectPartsID < g_pMapInfo->nMsgCount - 1) {
				g_nSelectPartsID ++;
				SendCmdMAPMSGREQ (g_nSelectPartsID);
			}
			InvalidateRect (g_hWndAdmin, NULL, TRUE);
		}
		break;

	/* 追加 */
	case IDC_BUTTON3:
		{
			LPSTR pszTmp = new char[300];
			ZeroMemory (pszTmp, 300);
			GetWindowText (m_hWndEdit1, pszTmp, 300);
			SendCmdADDMSG (pszTmp);
			SAFE_DELETE_ARRAY (pszTmp);
			g_pMapInfo->nMsgCount ++;
		}
		break;

	/* 送信 */
	case IDC_BUTTON4:
		{
			LPSTR pszTmp = new char[2048];
			ZeroMemory (pszTmp, 2048);
			GetWindowText (m_hWndEdit1, pszTmp, 2048);
			SendCmdCHGMSG (pszTmp, g_nSelectPartsID);
			SAFE_DELETE_ARRAY (pszTmp);
		}
		break;

	/* 取得 */
	case IDC_BUTTON5:
		SendCmdMAPMSGREQ (g_nSelectPartsID);
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnCommandAccAdd												 */
/* 内容：WM_COMMAND処理(アカウント追加)										 */
/* 日付：2003/08/23															 */
/* ========================================================================= */
void AdminOnCommandAccAdd(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id) {
	/* 送信 */
	case IDC_BUTTON1:
		{
			LPSTR pszTmp = new char[MAX_ACCNAME],
				  pszTmp2 = new char[MAX_ACCNAME];
			ZeroMemory (pszTmp, MAX_ACCNAME);
			ZeroMemory (pszTmp2, MAX_ACCNAME);
			GetWindowText (m_hWndEdit1, pszTmp, MAX_ACCNAME);
			GetWindowText (m_hWndEdit2, pszTmp2, MAX_ACCNAME);
			SendCmdACCADD (pszTmp, pszTmp2);
			SetWindowText (m_hWndEdit1, "");
			SetWindowText (m_hWndEdit2, "");
			SAFE_DELETE_ARRAY (pszTmp);
			SAFE_DELETE_ARRAY (pszTmp2);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnCommandAccInfo											 */
/* 内容：WM_COMMAND処理(アカウント編集)										 */
/* 日付：2003/08/30															 */
/* ========================================================================= */
void AdminOnCommandAccInfo(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id) {
	/* ← */
	case IDC_BUTTON1:
		{
			char szTmp[10];
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit1, szTmp, sizeof (szTmp));
			m_dwSelectID = (DWORD)atoi (szTmp);

			if (m_dwSelectID < 1) {
				break;
			}
			m_dwSelectID --;

			wsprintf (szTmp, "%d", m_dwSelectID);
			SetWindowText (m_hWndEdit1, szTmp);
			SendCmdGETACCINFO (m_dwSelectID);
		}
		break;

	/* → */
	case IDC_BUTTON2:
		{
			char szTmp[10];
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit1, szTmp, sizeof (szTmp));
			m_dwSelectID = (DWORD)atoi (szTmp);
			m_dwSelectID ++;

			wsprintf (szTmp, "%d", m_dwSelectID);
			SetWindowText (m_hWndEdit1, szTmp);
			SendCmdGETACCINFO (m_dwSelectID);
		}
		break;

	/* 削除 */
	case IDC_BUTTON3:
		if (m_pParaCharInfo->dwTblID) {
			SendCmdDELCHAR (m_pParaCharInfo->dwTblID);
			SendCmdGETACCINFO (m_pParaCharInfo->dwTblID);
		}
		break;

	/* 許可 */
	case IDC_BUTTON4:
		if (m_pParaCharInfo->dwTblID) {
			BYTE byTmp = m_pParaCharInfo->byAccState;
			byTmp &= ~ACCSTATE_DISABLE;
			SendCmdCHGACCSTATE (m_pParaCharInfo->dwTblID, byTmp);
		}
		break;

	/* 拒否 */
	case IDC_BUTTON5:
		if (m_pParaCharInfo->dwTblID) {
			BYTE byTmp = m_pParaCharInfo->byAccState;
			byTmp |= ACCSTATE_DISABLE;
			SendCmdCHGACCSTATE (m_pParaCharInfo->dwTblID, byTmp);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnCommandEtcVersion											 */
/* 内容：WM_COMMAND処理(バージョン通知)										 */
/* 日付：2003/09/05															 */
/* ========================================================================= */
void AdminOnCommandEtcVersion(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id) {
	/* 通知 */
	case IDC_BUTTON1:
		SendCmdCLIVERSION ();
		break;

	/* 管理人ON/OFF通知 */
	case IDC_BUTTON2:
		SendCmdCHGADMINON ();
		break;

	/* 放送前に鳴らす */
	case IDC_BUTTON3:
		SendCmdPLAYSOUNDALL (SOUND_PINPON);
		break;

	/* 当たり判定ON */
	case IDC_BUTTON4:
		g_bBlock = TRUE;
		break;

	/* 当たり判定OFF */
	case IDC_BUTTON5:
		g_bBlock = FALSE;
		break;

	/* 鐘 */
	case IDC_BUTTON6:
		SendCmdPLAYSOUNDALL (SOUND_KANE);
		g_bySoundCount ++;
		InvalidateRect (hWnd, NULL, TRUE);
		break;

	/* ｶｳﾝﾄﾘｾｯﾄ */
	case IDC_BUTTON7:
		g_bySoundCount = 0;
		InvalidateRect (hWnd, NULL, TRUE);
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnCommandItemList											 */
/* 内容：WM_COMMAND処理(アイテム一覧)										 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void AdminOnCommandItemList(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id) {
	/* 追加 */
	case IDC_BUTTON1:
		SendCmdADDITEM ();
		DeleteItemEditWindow ();
		break;

	/* 詳細 */
	case IDC_BUTTON2:
		{
			int nNo = ListView_GetNextItem (m_hWndList1, -1, LVNI_ALL | LVNI_SELECTED);
			if (nNo < 0) {
				m_dwSelectID = -1;
				break;
			}
			PITEMINFO pItem = g_aItemInfo[nNo];
			/* ウィンドウ作成 */
			MakeItemEditWindow (pItem->byType);
			m_dwSelectID = nNo;
			char szTmp[1024];

			/* 名前 */
			SetWindowText (m_hWndEdit1, pItem->szName);
			/* 値段 */
			ZeroMemory (szTmp, sizeof (szTmp));
			wsprintf (szTmp, "%ld", pItem->dwGold);
			SetWindowText (m_hWndEdit2, szTmp);
			/* 無くなる確率 */
			ZeroMemory (szTmp, sizeof (szTmp));
			wsprintf (szTmp, "%d", pItem->byEraseProbability);
			SetWindowText (m_hWndEdit4, szTmp);
			/* 使った時のメッセージ */
			if (pItem->pMsg) {
				SetWindowText (m_hWndEdit3, (LPSTR)pItem->pMsg);
			}
			/* 捨てれないフラグ */
			SendMessage (m_hWndCheck1, BM_SETCHECK, pItem->byFlg & ITEMFLG_DROP, 0);
			/* なくなるフラグ */
			SendMessage (m_hWndCheck2, BM_SETCHECK, pItem->byFlg & ITEMFLG_LOST, 0);
			/* 取引不可フラグ */
			SendMessage (m_hWndCheck5, BM_SETCHECK, pItem->byFlg & ITEMFLG_TRADE, 0);
			BYTE byNo = 0;
			int i;
			for (i = 0; ; i ++) {
				if (adwSoundNoTbl[i] == -1) {
					break;
				}
				if (pItem->bySound == adwSoundNoTbl[i]) {
					byNo = i;
					break;
				}
			}
			SendMessage (m_hWndCombo2, CB_SETCURSEL, i, 0);
			SendMessage (m_hWndCombo4, CB_SETCURSEL, pItem->bySexDistinction, 0);

			/* 種類別にデータ取得 */
			switch (pItem->byType) {
			case ITEMTYPE_RURA:		/* 特定位置へ移動 */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_RURA pInfo = (PITEMINFO_RURA)pItem->pData;
					/* マップID */
					wsprintf (szTmp, "%ld", pInfo->dwMapID);
					SetWindowText (m_hWndEdit5, szTmp);
					/* X座標 */
					wsprintf (szTmp, "%ld", pInfo->dwX);
					SetWindowText (m_hWndEdit6, szTmp);
					/* Y座標 */
					wsprintf (szTmp, "%ld", pInfo->dwY);
					SetWindowText (m_hWndEdit7, szTmp);
					/* 限定するマップID */
					DWORD dwTmp = pInfo->dwEnableMapID;
					if (dwTmp & 0x80000000) {
						SendMessage (m_hWndCheck3, BM_SETCHECK, TRUE, 0);
						dwTmp &= ~0x80000000;
					}
					wsprintf (szTmp, "%ld", dwTmp);
					SetWindowText (m_hWndEdit8, szTmp);
				}
				break;

			case ITEMTYPE_HPUP:	/* HP回復 */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_HPUP pInfo = (PITEMINFO_HPUP)pItem->pData;
					/* 最大回復値 */
					wsprintf (szTmp, "%ld", pInfo->wMin);
					SetWindowText (m_hWndEdit6, szTmp);
					/* 最小回復値 */
					wsprintf (szTmp, "%ld", pInfo->wMax);
					SetWindowText (m_hWndEdit7, szTmp);
					/* 回復率 */
					SendMessage (m_hWndCheck3, BM_SETCHECK, pInfo->byRate, 0);
				}
				break;

			case ITEMTYPE_WEAPON:	/* 武器 */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_WEAPON pInfo = (PITEMINFO_WEAPON)pItem->pData;
					/* 効果 */
					wsprintf (szTmp, "%d", pItem->byData);
					SetWindowText (m_hWndEdit6, szTmp);
					/* 攻撃時に発動するエフェクト番号 */
					wsprintf (szTmp, "%d", pInfo->byEfcID);
					SetWindowText (m_hWndEdit7, szTmp);
					/* 攻撃力減少幅 */
					wsprintf (szTmp, "%d", pInfo->byMin);
					SetWindowText (m_hWndEdit8, szTmp);
					/* 付加データ1 */
					wsprintf (szTmp, "%d", pInfo->byData1);
					SetWindowText (m_hWndEdit9, szTmp);
					/* 付加データ2 */
					wsprintf (szTmp, "%d", pInfo->byData2);
					SetWindowText (m_hWndEdit10, szTmp);
					/* 付加データ3 */
					wsprintf (szTmp, "%d", pInfo->byData3);
					SetWindowText (m_hWndEdit11, szTmp);
					/* 付加データ4 */
					wsprintf (szTmp, "%d", pInfo->byData4);
					SetWindowText (m_hWndEdit12, szTmp);
					/* 付加データ5 */
					wsprintf (szTmp, "%d", pInfo->byData5);
					SetWindowText (m_hWndEdit13, szTmp);
					/* 装備できるキャラ情報 */
					SendMessage (m_hWndCombo3, CB_SETCURSEL, pInfo->wEqu, 0);
					/* 攻撃種別 */
					SendMessage (m_hWndCombo5, CB_SETCURSEL, pInfo->byAtackType, 0);
				}
				break;

			case ITEMTYPE_ARMOR:	/* 鎧 */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_ARMOR pInfo = (PITEMINFO_ARMOR)pItem->pData;
					/* 効果 */
					wsprintf (szTmp, "%ld", pItem->byData);
					SetWindowText (m_hWndEdit6, szTmp);
					/* 装備できるキャラ情報 */
					SendMessage (m_hWndCombo3, CB_SETCURSEL, pInfo->wEqu, 0);
				}
				break;

			case ITEMTYPE_SHILD:	/* 盾 */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_SHILD pInfo = (PITEMINFO_SHILD)pItem->pData;
					/* 効果 */
					wsprintf (szTmp, "%ld", pItem->byData);
					SetWindowText (m_hWndEdit6, szTmp);
					/* 装備できるキャラ情報 */
					SendMessage (m_hWndCombo3, CB_SETCURSEL, pInfo->wEqu, 0);
				}
				break;

			case ITEMTYPE_HELMET:	/* 兜 */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_HELMET pInfo = (PITEMINFO_HELMET)pItem->pData;
					/* 効果 */
					wsprintf (szTmp, "%ld", pItem->byData);
					SetWindowText (m_hWndEdit6, szTmp);
					/* 装備できるキャラ情報 */
					SendMessage (m_hWndCombo3, CB_SETCURSEL, pInfo->wEqu, 0);
				}
				break;

			case ITEMTYPE_SEED:		/* 種 */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_SEED pInfo = (PITEMINFO_SEED)pItem->pData;
					/* 上限値 */
					wsprintf (szTmp, "%ld", pInfo->wMax);
					SetWindowText (m_hWndEdit6, szTmp);
					SendMessage (m_hWndCombo3, CB_SETCURSEL, pInfo->byType, 0);
				}
				break;

			case ITEMTYPE_SUBGAME:	/* サブゲーム参加 */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_SUBGAME pInfo = (PITEMINFO_SUBGAME)pItem->pData;
					SendMessage (m_hWndCombo3, CB_SETCURSEL, pInfo->byType, 0);
				}
				break;

			case ITEMTYPE_SAKE:		/* 酒 */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_SAKE pInfo = (PITEMINFO_SAKE)pItem->pData;
					/* 度数 */
					wsprintf (szTmp, "%ld", pInfo->chPoint);
					SetWindowText (m_hWndEdit6, szTmp);
				}
				break;

			case ITEMTYPE_MSG:		/* メッセージ表示 */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_MSG pInfo = (PITEMINFO_MSG)pItem->pData;
					/* 他のキャラに使った時のメッセージ */
					SetWindowText (m_hWndEdit6, (LPSTR)pInfo->pMsgTo);
					/* 他のキャラに使われた時のメッセージ */
					SetWindowText (m_hWndEdit7, (LPSTR)pInfo->pMsgMe);
					/* 文字エフェクト */
					SetWindowText (m_hWndEdit8, (LPSTR)pInfo->szCharEfc);
					/* エフェクト番号 */
					wsprintf (szTmp, "%d", pInfo->byEfectID);
					SetWindowText (m_hWndEdit9, szTmp);
					/* 表示種別 */
					SendMessage (m_hWndCombo3, CB_SETCURSEL, pInfo->byType, 0);
				}
				break;

			case ITEMTYPE_GRPCHG:		/* キャラ画像変更 */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_GRPCHG pInfo = (PITEMINFO_GRPCHG)pItem->pData;
					/* 変更するグラフィックID */
					wsprintf (szTmp, "%d", pInfo->wGrpID);
					SetWindowText (m_hWndEdit6, szTmp);
				}
				break;

			case ITEMTYPE_LIGHT:		/* 明かり */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_LIGHT pInfo = (PITEMINFO_LIGHT)pItem->pData;
					/* 明るさレベル */
					wsprintf (szTmp, "%d", pInfo->byLevel);
					SetWindowText (m_hWndEdit6, szTmp);
					/* 維持時間 */
					wsprintf (szTmp, "%d", pInfo->wTime);
					SetWindowText (m_hWndEdit7, szTmp);
				}
				break;

			case ITEMTYPE_HPDOWN:	/* HP減少 */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_HPDOWN pInfo = (PITEMINFO_HPDOWN)pItem->pData;
					/* 最低減少値 */
					wsprintf (szTmp, "%d", pInfo->wMin);
					SetWindowText (m_hWndEdit6, szTmp);
					/* 最大減少値 */
					wsprintf (szTmp, "%d", pInfo->wMax);
					SetWindowText (m_hWndEdit7, szTmp);
					/* エフェクト番号 */
					wsprintf (szTmp, "%d", pInfo->byEfectID);
					SetWindowText (m_hWndEdit8, szTmp);
					/* ﾀﾞﾒｰｼﾞ率 */
					SendMessage (m_hWndCheck3, BM_SETCHECK, pInfo->byRate, 0);
				}
				break;

			case ITEMTYPE_MAGIC:	/* 魔法習得 */
			case ITEMTYPE_USEMAGIC:	/* 特技発動 */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_MAGIC pInfo = (PITEMINFO_MAGIC)pItem->pData;
					/* 習得する魔法 */
					SendMessage (m_hWndCombo3, CB_SETCURSEL, pInfo->byID, 0);
				}
				break;

			case ITEMTYPE_MANAUP:	/* Mana回復 */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_MANAUP pInfo = (PITEMINFO_MANAUP)pItem->pData;
					/* 最大回復値 */
					wsprintf (szTmp, "%ld", pInfo->wMin);
					SetWindowText (m_hWndEdit6, szTmp);
					/* 最小回復値 */
					wsprintf (szTmp, "%ld", pInfo->wMax);
					SetWindowText (m_hWndEdit7, szTmp);
					/* 回復率 */
					SendMessage (m_hWndCheck3, BM_SETCHECK, pInfo->byRate, 0);
				}
				break;

			case ITEMTYPE_HPMANAUP:	/* HP・Mana回復 */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_HPMANAUP pInfo = (PITEMINFO_HPMANAUP)pItem->pData;
					/* 最大回復値(HP) */
					wsprintf (szTmp, "%ld", pInfo->wMinHP);
					SetWindowText (m_hWndEdit6, szTmp);
					/* 最小回復値(HP) */
					wsprintf (szTmp, "%ld", pInfo->wMaxHP);
					SetWindowText (m_hWndEdit7, szTmp);
					/* 最大回復値(Mana) */
					wsprintf (szTmp, "%ld", pInfo->wMinMana);
					SetWindowText (m_hWndEdit8, szTmp);
					/* 最小回復値(Mana) */
					wsprintf (szTmp, "%ld", pInfo->wMaxMana);
					SetWindowText (m_hWndEdit9, szTmp);
					/* 回復率(HP) */
					SendMessage (m_hWndCheck3, BM_SETCHECK, pInfo->byRateHP, 0);
					/* 回復率(Mana) */
					SendMessage (m_hWndCheck4, BM_SETCHECK, pInfo->byRateMana, 0);
				}
				break;

			case ITEMTYPE_EQU_HAND:	/* 装備(小手) */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_EQU_HAND pInfo = (PITEMINFO_EQU_HAND)pItem->pData;
					/* 効果 */
					wsprintf (szTmp, "%ld", pItem->byData);
					SetWindowText (m_hWndEdit6, szTmp);
					/* 装備できるキャラ情報 */
					SendMessage (m_hWndCombo3, CB_SETCURSEL, pInfo->wEqu, 0);
				}
				break;

			case ITEMTYPE_EQU_LEG:	/* 装備(靴) */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_EQU_LEG pInfo = (PITEMINFO_EQU_LEG)pItem->pData;
					/* 効果 */
					wsprintf (szTmp, "%ld", pItem->byData);
					SetWindowText (m_hWndEdit6, szTmp);
					/* 装備できるキャラ情報 */
					SendMessage (m_hWndCombo3, CB_SETCURSEL, pInfo->wEqu, 0);
				}
				break;

			case ITEMTYPE_EQU_ACC1:	/* 装備(首飾り) */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_EQU_ACC1 pInfo = (PITEMINFO_EQU_ACC1)pItem->pData;
					/* 効果 */
					wsprintf (szTmp, "%ld", pItem->byData);
					SetWindowText (m_hWndEdit6, szTmp);
					/* 装備できるキャラ情報 */
					SendMessage (m_hWndCombo3, CB_SETCURSEL, pInfo->wEqu, 0);
					/* 装備効果 */
					SendMessage (m_hWndCombo5, CB_SETCURSEL, pInfo->byEquType, 0);
				}
				break;

			case ITEMTYPE_EQU_ACC2:	/* 装備(指輪1) */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_EQU_ACC2 pInfo = (PITEMINFO_EQU_ACC2)pItem->pData;
					/* 効果 */
					wsprintf (szTmp, "%ld", pItem->byData);
					SetWindowText (m_hWndEdit6, szTmp);
					/* 装備できるキャラ情報 */
					SendMessage (m_hWndCombo3, CB_SETCURSEL, pInfo->wEqu, 0);
					/* 装備効果 */
					SendMessage (m_hWndCombo5, CB_SETCURSEL, pInfo->byEquType, 0);
				}
				break;

			case ITEMTYPE_EQU_ACC3:	/* 装備(指輪2) */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_EQU_ACC3 pInfo = (PITEMINFO_EQU_ACC3)pItem->pData;
					/* 効果 */
					wsprintf (szTmp, "%ld", pItem->byData);
					SetWindowText (m_hWndEdit6, szTmp);
					/* 装備できるキャラ情報 */
					SendMessage (m_hWndCombo3, CB_SETCURSEL, pInfo->wEqu, 0);
					/* 装備効果 */
					SendMessage (m_hWndCombo5, CB_SETCURSEL, pInfo->byEquType, 0);
				}
				break;

			case ITEMTYPE_RESURRECTION:	/* 蘇生 */
				{
					if (pItem->pData == NULL) {
						break;
					}
					PITEMINFO_RESURRECTION pInfo = (PITEMINFO_RESURRECTION)pItem->pData;
					/* 回復率 */
					wsprintf (szTmp, "%d", pInfo->byRate);
					SetWindowText (m_hWndEdit6, szTmp);
				}
				break;
			}
		}
		break;

	/* ｸﾘｱ */
	case IDC_BUTTON3:
		{
			int nNo = ListView_GetNextItem (m_hWndList1, -1, LVNI_ALL | LVNI_SELECTED);
			if (nNo < 0) {
				break;
			}
			PITEMINFO pItem = g_aItemInfo[nNo];
			switch (pItem->byType) {
			case ITEMTYPE_MSG:		/* メッセージ表示 */
				{
					PITEMINFO_MSG pInfo = (PITEMINFO_MSG)pItem->pData;
					if (pInfo) {
						SAFE_DELETE_ARRAY (pInfo->pMsgMe);
						SAFE_DELETE_ARRAY (pInfo->pMsgTo);
					}
				}
				break;
			}
			SAFE_DELETE_ARRAY (pItem->pData);
			ZeroMemory (pItem, sizeof (ITEMINFO));
			PostMessage (hWnd, WM_ADMINITEMCHG, nNo, 0);
			DeleteItemEditWindow ();
		}
		break;

	/* 送信 */
	case IDC_BUTTON4:
		{
			if (m_dwSelectID == -1) {
				break;
			}

			int nNo = ListView_GetNextItem (m_hWndList1, -1, LVNI_ALL | LVNI_SELECTED);
			if (nNo < 0) {
				break;
			}
			PITEMINFO pItem = g_aItemInfo[nNo];
			char szTmp[1024];

			/* 名前を取得 */
			ZeroMemory (pItem->szName, sizeof (pItem->szName));
			GetWindowText (m_hWndEdit1, pItem->szName, sizeof (pItem->szName));
			pItem->szName[SboTextFitWidth (pItem->szName, MAX_ITEMNAME_WIDTH)] = 0;		/* 画面に収まる長さまで */
			/* 種別を取得 */
			pItem->byType = (BYTE)SendMessage (m_hWndCombo1, CB_GETCURSEL, 0, 0);
			/* 効果音を取得 */
			pItem->bySound = (BYTE)adwSoundNoTbl[SendMessage (m_hWndCombo2, CB_GETCURSEL, 0, 0)];
			/* 性別による種別を取得 */
			pItem->bySexDistinction = (BYTE)adwSoundNoTbl[SendMessage (m_hWndCombo4, CB_GETCURSEL, 0, 0)];
			/* 値段を取得 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit2, szTmp, sizeof (szTmp));
			pItem->dwGold = (DWORD)atoi (szTmp);
			/* 無くなる確率を取得 */
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit4, szTmp, sizeof (szTmp));
			pItem->byEraseProbability = (BYTE)atoi (szTmp);
			/* 捨てれないフラグ */
			pItem->byFlg &= ~ITEMFLG_DROP;
			if (SendMessage (m_hWndCheck1, BM_GETCHECK, 0, 0) & BST_CHECKED) {
				pItem->byFlg |= ITEMFLG_DROP;
			}
			/* なくなるフラグ */
			pItem->byFlg &= ~ITEMFLG_LOST;
			if (SendMessage (m_hWndCheck2, BM_GETCHECK, 0, 0) & BST_CHECKED) {
				pItem->byFlg |= ITEMFLG_LOST;
			}
			/* 取引不可フラグ */
			pItem->byFlg &= ~ITEMFLG_TRADE;
			if (SendMessage (m_hWndCheck5, BM_GETCHECK, 0, 0) & BST_CHECKED) {
				pItem->byFlg |= ITEMFLG_TRADE;
			}
			/* 使った時のメッセージ */
			switch (pItem->byType) {
			case ITEMTYPE_MSG:		/* メッセージ表示 */
				{
					PITEMINFO_MSG pInfo = (PITEMINFO_MSG)pItem->pData;
					if (pInfo) {
						SAFE_DELETE_ARRAY (pInfo->pMsgMe);
						SAFE_DELETE_ARRAY (pInfo->pMsgTo);
					}
				}
				break;
			}
			SAFE_DELETE_ARRAY (pItem->pMsg);
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit3, szTmp, sizeof (szTmp));
			pItem->pMsg = new char[strlen (szTmp) + 1];
			ZeroMemory (pItem->pMsg, sizeof (strlen (szTmp) + 1));
			strcpy ((LPSTR)pItem->pMsg, szTmp);

			SAFE_DELETE_ARRAY (pItem->pData);
			/* 種類別にデータ取得 */
			switch (pItem->byType) {
			case ITEMTYPE_RURA:		/* 特定位置へ移動 */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_RURA)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_RURA));
					PITEMINFO_RURA pInfo = (PITEMINFO_RURA)pItem->pData;

					/* マップID */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit5, szTmp, sizeof (szTmp));
					pInfo->dwMapID = atoi (szTmp);
					/* X座標 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
					pInfo->dwX = atoi (szTmp);
					/* Y座標 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit7, szTmp, sizeof (szTmp));
					pInfo->dwY = atoi (szTmp);
					/* 限定するマップID */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit8, szTmp, sizeof (szTmp));
					pInfo->dwEnableMapID = atoi (szTmp);
					if (SendMessage (m_hWndCheck3, BM_GETCHECK, 0, 0) & BST_CHECKED) {
						pInfo->dwEnableMapID |= 0x80000000;
					}
				}
				break;

			case ITEMTYPE_HPUP:	/* HP回復 */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_HPUP)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_HPUP));
					PITEMINFO_HPUP pInfo = (PITEMINFO_HPUP)pItem->pData;

					/* 最低回復値 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
					pInfo->wMin = atoi (szTmp);
					/* 最大回復値 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit7, szTmp, sizeof (szTmp));
					pInfo->wMax = atoi (szTmp);
					/* 回復率 */
					pInfo->byRate = 0;
					if (SendMessage (m_hWndCheck3, BM_GETCHECK, 0, 0) & BST_CHECKED) {
						pInfo->byRate = 1;
					}
				}
				break;

			case ITEMTYPE_WEAPON:	/* 武器 */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_WEAPON)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_WEAPON));
					PITEMINFO_WEAPON pInfo = (PITEMINFO_WEAPON)pItem->pData;

					/* 効果 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
					pItem->byData = (BYTE)atoi (szTmp);

					/* 攻撃時に発動するエフェクト番号 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit7, szTmp, sizeof (szTmp));
					pInfo->byEfcID = (BYTE)atoi (szTmp);

					/* 攻撃力減少幅 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit8, szTmp, sizeof (szTmp));
					pInfo->byMin = (BYTE)atoi (szTmp);

					/* 付加データ1 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit9, szTmp, sizeof (szTmp));
					pInfo->byData1 = (BYTE)atoi (szTmp);

					/* 付加データ2 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit10, szTmp, sizeof (szTmp));
					pInfo->byData2 = (BYTE)atoi (szTmp);

					/* 付加データ3 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit11, szTmp, sizeof (szTmp));
					pInfo->byData3 = (BYTE)atoi (szTmp);

					/* 付加データ4 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit12, szTmp, sizeof (szTmp));
					pInfo->byData4 = (BYTE)atoi (szTmp);

					/* 付加データ5 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit13, szTmp, sizeof (szTmp));
					pInfo->byData5 = (BYTE)atoi (szTmp);

					/* 装備できるキャラ情報 */
					pInfo->wEqu = (WORD)SendMessage (m_hWndCombo3, CB_GETCURSEL, 0, 0);
					/* 攻撃種別 */
					pInfo->byAtackType = (BYTE)SendMessage (m_hWndCombo5, CB_GETCURSEL, 0, 0);
				}
				break;

			case ITEMTYPE_ARMOR:	/* 鎧 */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_ARMOR)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_ARMOR));
					PITEMINFO_ARMOR pInfo = (PITEMINFO_ARMOR)pItem->pData;

					/* 効果 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
					pItem->byData = (BYTE)atoi (szTmp);

					/* 装備できるキャラ情報 */
					pInfo->wEqu = (WORD)SendMessage (m_hWndCombo3, CB_GETCURSEL, 0, 0);
				}
				break;

			case ITEMTYPE_SHILD:	/* 盾 */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_SHILD)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_SHILD));
					PITEMINFO_SHILD pInfo = (PITEMINFO_SHILD)pItem->pData;

					/* 効果 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
					pItem->byData = (BYTE)atoi (szTmp);

					/* 装備できるキャラ情報 */
					pInfo->wEqu = (WORD)SendMessage (m_hWndCombo3, CB_GETCURSEL, 0, 0);
				}
				break;

			case ITEMTYPE_HELMET:	/* 兜 */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_HELMET)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_HELMET));
					PITEMINFO_HELMET pInfo = (PITEMINFO_HELMET)pItem->pData;

					/* 効果 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
					pItem->byData = (BYTE)atoi (szTmp);

					/* 装備できるキャラ情報 */
					pInfo->wEqu = (WORD)SendMessage (m_hWndCombo3, CB_GETCURSEL, 0, 0);
				}
				break;

			case ITEMTYPE_SEED:		/* 種 */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_SEED)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_SEED));
					PITEMINFO_SEED pInfo = (PITEMINFO_SEED)pItem->pData;

					/* 種類 */
					pInfo->byType = (BYTE)SendMessage (m_hWndCombo3, CB_GETCURSEL, 0, 0);

					/* 上限値 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
					pInfo->wMax = (WORD)atoi (szTmp);
				}
				break;

			case ITEMTYPE_SUBGAME:	/* サブゲーム参加 */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_SUBGAME)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_SUBGAME));
					PITEMINFO_SUBGAME pInfo = (PITEMINFO_SUBGAME)pItem->pData;

					/* 種類 */
					pInfo->byType = (BYTE)SendMessage (m_hWndCombo3, CB_GETCURSEL, 0, 0);
				}
				break;

			case ITEMTYPE_SAKE:		/* 酒 */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_SAKE)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_SAKE));
					PITEMINFO_SAKE pInfo = (PITEMINFO_SAKE)pItem->pData;

					/* 度数 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
					pInfo->chPoint = (char)atoi (szTmp);
					pInfo->chPoint = min (pInfo->chPoint, (CHARSTATEDRUNK_MAX - 1) * 10);
					pInfo->chPoint = max (pInfo->chPoint, (CHARSTATEDRUNK_MAX - 1) * -10);
				}
				break;

			case ITEMTYPE_MSG:	/* メッセージ表示 */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_MSG)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_MSG));
					PITEMINFO_MSG pInfo = (PITEMINFO_MSG)pItem->pData;

					/* 他のキャラに使った時のメッセージ */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
					pInfo->pMsgTo = new char[strlen (szTmp) + 1];
					strcpy ((LPSTR)pInfo->pMsgTo, szTmp);

					/* 他のキャラに使われた時のメッセージ */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit7, szTmp, sizeof (szTmp));
					pInfo->pMsgMe = new char[strlen (szTmp) + 1];
					strcpy ((LPSTR)pInfo->pMsgMe, szTmp);

					/* 文字エフェクト */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit8, szTmp, sizeof (szTmp) - 1);
					szTmp[SboTextFitWidth (szTmp, MAX_LENCHAREFC_WIDTH)] = 0;		/* 画面に収まる長さまで */
					strcpy ((LPSTR)pInfo->szCharEfc, szTmp);
					/* 表示種別 */
					pInfo->byType = (BYTE)SendMessage (m_hWndCombo3, CB_GETCURSEL, 0, 0);

					/* エフェクト番号 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit9, szTmp, 4);
					pInfo->byEfectID = (BYTE)atoi (szTmp);
					pInfo->byEfectID = min (g_EfcInfo.GetCount (), pInfo->byEfectID);
					if (g_EfcInfo.GetCount () == 0) {
						pInfo->byEfectID = 0;
					}
				}
				break;

			case ITEMTYPE_GRPCHG:		/* キャラ画像変更 */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_GRPCHG)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_GRPCHG));
					PITEMINFO_GRPCHG pInfo = (PITEMINFO_GRPCHG)pItem->pData;

					/* 変更するグラフィックID */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
					int nTmp = atoi (szTmp);
					nTmp = max (nTmp, 0);
					nTmp = min (nTmp, g_wCountCharGrp * 16);
					pInfo->wGrpID = (WORD)nTmp;
				}
				break;

			case ITEMTYPE_LIGHT:		/* 明かり */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_LIGHT)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_LIGHT));
					PITEMINFO_LIGHT pInfo = (PITEMINFO_LIGHT)pItem->pData;

					/* 明るさレベル */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
					int nTmp = atoi (szTmp);
					nTmp = max (nTmp, 0);
					nTmp = min (nTmp, 20);
					pInfo->byLevel = (BYTE)nTmp;
					/* 維持時間 */
					GetWindowText (m_hWndEdit7, szTmp, sizeof (szTmp));
					nTmp = atoi (szTmp);
					nTmp = max (nTmp, 0);
					nTmp = min (nTmp, 65535);
					pInfo->wTime = (WORD)nTmp;
				}
				break;

			case ITEMTYPE_HPDOWN:	/* HP減少 */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_HPDOWN)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_HPDOWN));
					PITEMINFO_HPDOWN pInfo = (PITEMINFO_HPDOWN)pItem->pData;

					/* 最低減少値 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
					pInfo->wMin = (WORD)atoi (szTmp);
					/* 最大減少値 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit7, szTmp, sizeof (szTmp));
					pInfo->wMax = (WORD)atoi (szTmp);
					/* 最大減少値 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit8, szTmp, sizeof (szTmp));
					pInfo->byEfectID = (BYTE)atoi (szTmp);
					/* ダメージ率 */
					pInfo->byRate = 0;
					if (SendMessage (m_hWndCheck3, BM_GETCHECK, 0, 0) & BST_CHECKED) {
						pInfo->byRate = 1;
					}
				}
				break;

			case ITEMTYPE_MAGIC:	/* 魔法習得 */
			case ITEMTYPE_USEMAGIC:	/* 特技発動 */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_MAGIC)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_MAGIC));
					PITEMINFO_MAGIC pInfo = (PITEMINFO_MAGIC)pItem->pData;

					/* 習得する魔法 */
					pInfo->byID = (BYTE)SendMessage (m_hWndCombo3, CB_GETCURSEL, 0, 0);
				}
				break;

			case ITEMTYPE_MANAUP:	/* Mana回復 */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_MANAUP)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_MANAUP));
					PITEMINFO_MANAUP pInfo = (PITEMINFO_MANAUP)pItem->pData;

					/* 最低回復値 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
					pInfo->wMin = atoi (szTmp);
					/* 最大回復値 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit7, szTmp, sizeof (szTmp));
					pInfo->wMax = atoi (szTmp);
					/* 回復率 */
					pInfo->byRate = 0;
					if (SendMessage (m_hWndCheck3, BM_GETCHECK, 0, 0) & BST_CHECKED) {
						pInfo->byRate = 1;
					}
				}
				break;

			case ITEMTYPE_HPMANAUP:	/* HP・Mana回復 */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_HPMANAUP)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_HPMANAUP));
					PITEMINFO_HPMANAUP pInfo = (PITEMINFO_HPMANAUP)pItem->pData;

					/* 最低回復値(HP) */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
					pInfo->wMinHP = atoi (szTmp);
					/* 最大回復値(HP) */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit7, szTmp, sizeof (szTmp));
					pInfo->wMaxHP = atoi (szTmp);
					/* 最低回復値(Mana) */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit8, szTmp, sizeof (szTmp));
					pInfo->wMinMana = atoi (szTmp);
					/* 最大回復値(Mana) */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit9, szTmp, sizeof (szTmp));
					pInfo->wMaxMana = atoi (szTmp);
					/* 回復率(HP) */
					pInfo->byRateHP = 0;
					if (SendMessage (m_hWndCheck3, BM_GETCHECK, 0, 0) & BST_CHECKED) {
						pInfo->byRateHP = 1;
					}
					/* 回復率(Mana) */
					pInfo->byRateMana = 0;
					if (SendMessage (m_hWndCheck4, BM_GETCHECK, 0, 0) & BST_CHECKED) {
						pInfo->byRateMana = 1;
					}
				}
				break;

			case ITEMTYPE_EQU_HAND:	/* 装備(小手) */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_EQU_HAND)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_EQU_HAND));
					PITEMINFO_EQU_HAND pInfo = (PITEMINFO_EQU_HAND)pItem->pData;

					/* 効果 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
					pItem->byData = (BYTE)atoi (szTmp);

					/* 装備できるキャラ情報 */
					pInfo->wEqu = (WORD)SendMessage (m_hWndCombo3, CB_GETCURSEL, 0, 0);
				}
				break;

			case ITEMTYPE_EQU_LEG:	/* 装備(靴) */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_EQU_LEG)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_EQU_LEG));
					PITEMINFO_EQU_LEG pInfo = (PITEMINFO_EQU_LEG)pItem->pData;

					/* 効果 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
					pItem->byData = (BYTE)atoi (szTmp);

					/* 装備できるキャラ情報 */
					pInfo->wEqu = (WORD)SendMessage (m_hWndCombo3, CB_GETCURSEL, 0, 0);
				}
				break;

			case ITEMTYPE_EQU_ACC1:	/* 装備(首飾り) */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_EQU_ACC1)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_EQU_ACC1));
					PITEMINFO_EQU_ACC1 pInfo = (PITEMINFO_EQU_ACC1)pItem->pData;

					/* 効果 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
					pItem->byData = (BYTE)atoi (szTmp);

					/* 装備できるキャラ情報 */
					pInfo->wEqu = (WORD)SendMessage (m_hWndCombo3, CB_GETCURSEL, 0, 0);
					/* 装備効果 */
					pInfo->byEquType = (BYTE)SendMessage (m_hWndCombo5, CB_GETCURSEL, 0, 0);
				}
				break;

			case ITEMTYPE_EQU_ACC2:	/* 装備(指輪1) */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_EQU_ACC2)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_EQU_ACC2));
					PITEMINFO_EQU_ACC2 pInfo = (PITEMINFO_EQU_ACC2)pItem->pData;

					/* 効果 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
					pItem->byData = (BYTE)atoi (szTmp);

					/* 装備できるキャラ情報 */
					pInfo->wEqu = (WORD)SendMessage (m_hWndCombo3, CB_GETCURSEL, 0, 0);
					/* 装備効果 */
					pInfo->byEquType = (BYTE)SendMessage (m_hWndCombo5, CB_GETCURSEL, 0, 0);
				}
				break;

			case ITEMTYPE_EQU_ACC3:	/* 装備(指輪2) */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_EQU_ACC3)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_EQU_ACC3));
					PITEMINFO_EQU_ACC3 pInfo = (PITEMINFO_EQU_ACC3)pItem->pData;

					/* 効果 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
					pItem->byData = (BYTE)atoi (szTmp);

					/* 装備できるキャラ情報 */
					pInfo->wEqu = (WORD)SendMessage (m_hWndCombo3, CB_GETCURSEL, 0, 0);
					/* 装備効果 */
					pInfo->byEquType = (BYTE)SendMessage (m_hWndCombo5, CB_GETCURSEL, 0, 0);
				}
				break;

			case ITEMTYPE_RESURRECTION:	/* 回復率 */
				{
					pItem->pData = new BYTE[sizeof (ITEMINFO_RESURRECTION)];
					ZeroMemory (pItem->pData, sizeof (ITEMINFO_RESURRECTION));
					PITEMINFO_RESURRECTION pInfo = (PITEMINFO_RESURRECTION)pItem->pData;

					/* 効果 */
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit6, szTmp, sizeof (szTmp));
					pInfo->byRate = (BYTE)atoi (szTmp);
				}
				break;
			}

			SendCmdCHGITEM (nNo);
			DeleteItemEditWindow ();
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnCommandCharItem											 */
/* 内容：WM_COMMAND処理(キャラアイテム編集)									 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void AdminOnCommandCharItem(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id) {
	/* 送信 */
	case IDC_BUTTON1:
		if (m_dwSelectID == -1) {
			break;
		}
		{
			PCHARINFO pChar = g_aCharInfo[m_dwSelectID];
			if (pChar->pID != m_dwSelectpID) {
				int nTmp = GetCharNo (m_dwSelectpID);
				if (nTmp < 0) {
					break;
				}
				m_dwSelectID = (DWORD)nTmp;
				pChar = g_aCharInfo[m_dwSelectID];
			}
			CopyMemory (pChar->waItem, m_pCharInfo->waItem, sizeof (pChar->waItem));
		}
		SendCmdCHARITEM (m_dwSelectID);
		m_dwSelectID = -1;
		m_dwSelectpID = -1;
		ListView_DeleteAllItems (m_hWndList1);
		InvalidateRect (hWnd, NULL, TRUE);
		break;

	/* 追加 */
	case IDC_BUTTON2:
		{
			int nItemID = SendMessage (m_hWndCombo1, CB_GETCURSEL, 0, 0);
			if (nItemID <= 0) {
				break;
			}
			int i, nCount = -1;
			for (i = 0; i < MAX_MYITEM; i ++) {
				if (m_pCharInfo->waItem[i] == 0) {
					nCount = i;
					break;
				}
			}
			if (nCount < 0) {
				break;
			}

			LV_ITEM item;
			ZeroMemory (&item, sizeof (item));
			item.mask = LVIF_TEXT;
			item.iItem = nCount;
			item.iSubItem = 1;
			item.pszText = g_aItemInfo[nItemID]->szName;
			ListView_SetItem (m_hWndList1, &item);
			m_pCharInfo->waItem[nCount] = nItemID;
		}
		break;

	/* 更新 */
	case IDC_BUTTON3:
		{
			int nItemID = SendMessage (m_hWndCombo1, CB_GETCURSEL, 0, 0);
			if (nItemID <= 0) {
				break;
			}
			int nSelect = ListView_GetNextItem (m_hWndList1, -1, LVNI_ALL | LVNI_SELECTED);
			if (nSelect < 0) {
				break;
			}
			LV_ITEM item;
			ZeroMemory (&item, sizeof (item));
			item.mask = LVIF_TEXT;
			item.iItem = nSelect;
			item.iSubItem = 1;
			item.pszText = g_aItemInfo[nItemID]->szName;
			ListView_SetItem (m_hWndList1, &item);
			m_pCharInfo->waItem[nSelect] = nItemID;
		}
		break;

	/* 消去 */
	case IDC_BUTTON4:
		{
			int nSelect = ListView_GetNextItem (m_hWndList1, -1, LVNI_ALL | LVNI_SELECTED);
			if (nSelect < 0) {
				break;
			}
			LV_ITEM item;
			ZeroMemory (&item, sizeof (item));
			item.mask = LVIF_TEXT;
			item.iItem = nSelect;
			item.iSubItem = 1;
			ListView_SetItem (m_hWndList1, &item);
			m_pCharInfo->waItem[nSelect] = 0;
		}
		break;

	/* ←取得 */
	case IDC_BUTTON5:
		{
			int nID = SendMessage (m_hWndCombo2, CB_GETCURSEL, 0, 0);
			if (nID < 0) {
				break;
			}
			PSTATUSTEMPLATEINFO pInfo = g_aStTmpInfo[nID];
			ListView_DeleteAllItems (m_hWndList1);
			ZeroMemory (m_pCharInfo, sizeof (CHARINFO));

			int i;
			LV_ITEM item;
			for (i = 0; i < MAX_MYITEM; i ++) {
				ZeroMemory (&item, sizeof (item));
				item.mask = LVIF_TEXT;
				item.iItem = i;
				item.iSubItem = 0;
				ListView_InsertItem (m_hWndList1, &item);
				item.iSubItem = 1;
				if (pInfo->waItem[i]) {
					item.pszText = g_aItemInfo[pInfo->waItem[i]]->szName;
					m_pCharInfo->waItem[i] = pInfo->waItem[i];
				}
				ListView_SetItem (m_hWndList1, &item);
			}
		}
		break;

	/* →反映 */
	case IDC_BUTTON6:
		{
			int nID = SendMessage (m_hWndCombo2, CB_GETCURSEL, 0, 0);
			if (nID < 0) {
				break;
			}
			PSTATUSTEMPLATEINFO pInfo = g_aStTmpInfo[nID];
			CopyMemory (pInfo->waItem, m_pCharInfo->waItem, sizeof (pInfo->waItem));
			ListView_DeleteAllItems (m_hWndList1);
			ZeroMemory (m_pCharInfo, sizeof (CHARINFO));
			SendCmdSTTMPLATE ((DWORD)nID, CMDSTTMPLATETYPE_CHG);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnCommandSvrInfo											 */
/* 内容：WM_COMMAND処理(サーバー情報)										 */
/* 日付：2003/09/18															 */
/* ========================================================================= */
void AdminOnCommandSvrInfo(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id) {
	/* 送信 */
	case IDC_BUTTON1:
		SendCmdSVRINFOREQ ();
		break;

	/* 保存 */
	case IDC_SAVEMAP:
		SendCmdSAVEMAP ();
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnCommandShopInfo											 */
/* 内容：WM_COMMAND処理(店情報)												 */
/* 日付：2003/09/20															 */
/* ========================================================================= */
void AdminOnCommandShopInfo(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id) {
	/* ← */
	case IDC_BUTTON1:
		if (g_pMapInfo->byCountShop == 0) {
			break;
		}
		if (m_dwSelectID == 0) {
			break;
		}
		m_dwSelectID --;
		PostMessage (hWnd, WM_COMMAND, (WPARAM)IDC_BUTTON6, 0);
		break;

	/* → */
	case IDC_BUTTON2:
		if (g_pMapInfo->byCountShop == 0) {
			break;
		}
		if (m_dwSelectID >= (DWORD)g_pMapInfo->byCountShop - 1) {
			break;
		}
		m_dwSelectID ++;
		PostMessage (hWnd, WM_COMMAND, (WPARAM)IDC_BUTTON6, 0);
		break;

	/* 送信 */
	case IDC_BUTTON7:
		if (g_pMapInfo->byCountShop == 0) {
			break;
		}
		{
			PSHOPINFO pShop = &g_pMapInfo->pShopInfo[m_dwSelectID];
			int i;
			char szTmp[2048];

			/* メッセージを取得 */
			for (i = 0; i < MAX_SHOPMSG; i ++) {
				ZeroMemory (szTmp, sizeof (szTmp));
				ListView_GetItemText (m_hWndList1, i, 1, szTmp, sizeof (szTmp) - 1);
				SAFE_DELETE_ARRAY (pShop->apMsg[i]);
				int nLen = strlen (szTmp);
				pShop->apMsg[i] = new char[nLen + 1];
				ZeroMemory (pShop->apMsg[i], nLen + 1);
				strcat (pShop->apMsg[i], szTmp);
			}

			ZeroMemory (pShop->awItem, sizeof (pShop->awItem));
			pShop->byCountItem = 0;
			/* 売り物を取得 */
			for (i = 0; i < ListView_GetItemCount (m_hWndList2); i ++) {
				if (i >= MAX_SHOPITEM) {
					break;
				}
				ZeroMemory (szTmp, sizeof (szTmp));
				ListView_GetItemText (m_hWndList2, i, 0, szTmp, sizeof (szTmp) - 1);
				pShop->awItem[i] = (WORD)atoi (szTmp);
				pShop->byCountItem ++;
			}
			SendCmdCHGSHOP ((BYTE)m_dwSelectID);
		}
		break;

	/* 追加 */
	case IDC_BUTTON8:
		{
			SendCmdADDSHOP ();
		}
		break;

	/* 取得 */
	case IDC_BUTTON6:
		if (g_pMapInfo->byCountShop == 0) {
			break;
		}
		{
			PSHOPINFO pShop = &g_pMapInfo->pShopInfo[m_dwSelectID];
			LV_ITEM item;
			int i;
			ZeroMemory (&item, sizeof (item));
			item.mask = LVIF_TEXT;
			item.iSubItem = 1;

			/* メッセージを登録 */
			for (i = 0; i < MAX_SHOPMSG; i ++) {
				item.iItem = i;
				item.pszText = pShop->apMsg[i];
				ListView_SetItem (m_hWndList1, &item);
			}

			ZeroMemory (&item, sizeof (item));
			item.mask = LVIF_TEXT;

			/* 売り物一覧 */
			ListView_DeleteAllItems (m_hWndList2);
			for (i = 0; i < pShop->byCountItem; i ++) {
				char szTmp[64];
				wsprintf (szTmp, "%d", pShop->awItem[i]);
				item.iItem = i;
				item.iSubItem = 0;
				item.pszText = szTmp;
				ListView_InsertItem (m_hWndList2, &item);

				item.iSubItem = 1;
				item.pszText = g_aItemInfo[pShop->awItem[i]]->szName;
				ListView_SetItem (m_hWndList2, &item);
			}

			ShowWindow (m_hWndEdit1, SW_HIDE);
			ShowWindow (m_hWndButton9, SW_HIDE);

			InvalidateRect (hWnd, NULL, TRUE);
		}
		break;

	/* 編集 */
	case IDC_BUTTON3:
		if (g_pMapInfo->byCountShop == 0) {
			break;
		}
		{
			int nNo = ListView_GetNextItem (m_hWndList1, -1, LVNI_ALL | LVNI_SELECTED);
			if (nNo < 0) {
				break;
			}

			ShowWindow (m_hWndEdit1, SW_SHOW);
			ShowWindow (m_hWndButton9, SW_SHOW);

			char szTmp[1024];
			ZeroMemory (szTmp, sizeof (szTmp));
			ListView_GetItemText (m_hWndList1, nNo, 1, szTmp, sizeof (szTmp) - 1);
			SetWindowText (m_hWndEdit1, szTmp);
		}
		break;

	/* 更新 */
	case IDC_BUTTON9:
		if (g_pMapInfo->byCountShop == 0) {
			break;
		}
		{
			int nNo = ListView_GetNextItem (m_hWndList1, -1, LVNI_ALL | LVNI_SELECTED);
			if (nNo < 0) {
				break;
			}

			char szTmp[1024];
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit1, szTmp, sizeof (szTmp) - 1);

			LV_ITEM item;
			ZeroMemory (&item, sizeof (item));
			item.mask = LVIF_TEXT;
			item.iItem = nNo;
			item.iSubItem = 1;
			item.pszText = szTmp;
			ListView_SetItem (m_hWndList1, &item);
			SetWindowText (m_hWndEdit1, "");

			ShowWindow (m_hWndEdit1, SW_HIDE);
			ShowWindow (m_hWndButton9, SW_HIDE);
		}
		break;

	/* 売り物追加 */
	case IDC_BUTTON4:
		if (g_pMapInfo->byCountShop == 0) {
			break;
		}
		{
			int nNo = ListView_GetNextItem (m_hWndList2, -1, LVNI_ALL | LVNI_SELECTED);
			if (nNo < 0) {
				nNo = ListView_GetItemCount (m_hWndList2);
			}

			if (ListView_GetItemCount (m_hWndList2) >= MAX_SHOPITEM) {
				break;
			}

			int nItemID = SendMessage (m_hWndCombo1, CB_GETCURSEL, 0, 0);
			if (nItemID <= 0) {
				break;
			}

			char szTmp[16];
			LV_ITEM item;
			ZeroMemory (&item, sizeof (item));

			item.mask = LVIF_TEXT;
			item.iItem = nNo;
			wsprintf (szTmp, "%d", nItemID);
			item.pszText = szTmp;
			ListView_InsertItem (m_hWndList2, &item);

			item.iSubItem = 1;
			item.pszText = g_aItemInfo[nItemID]->szName;
			ListView_SetItem (m_hWndList2, &item);
		}
		break;

	/* 売り物削除 */
	case IDC_BUTTON5:
		if (g_pMapInfo->byCountShop == 0) {
			break;
		}
		{
			int nNo = ListView_GetNextItem (m_hWndList2, -1, LVNI_ALL | LVNI_SELECTED);
			if (nNo < 0) {
				break;
			}
			ListView_DeleteItem (m_hWndList2, nNo);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnCommandCefEdit											 */
/* 内容：WM_COMMAND処理(吹き出し設定)										 */
/* 日付：2003/09/06															 */
/* ========================================================================= */
void AdminOnCommandCefEdit(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id) {
	/* ← */
	case IDC_BUTTON1:
	/* → */
	case IDC_BUTTON2:
		{
			char szTmp[128];
			PCOMEFCINFO pComEfc = &g_pComEfcInfo[m_dwSelectID];
			PCOMEFCANIMEINFO pAnime = &pComEfc->pAnime[m_dwSelectID2];

			if (pAnime) {
				GetWindowText (m_hWndEdit3, szTmp, sizeof (szTmp));
				pAnime->byWait = (BYTE)atoi (szTmp);
			}

			if (id == IDC_BUTTON1) {
				if (m_dwSelectID2 < 1) {
					break;
				}
				m_dwSelectID2 --;

			} else {
				if ((pComEfc->byAnimeCount == 0) || (m_dwSelectID2 >= (DWORD)pComEfc->byAnimeCount - 1)) {
					break;
				}
				m_dwSelectID2 ++;
			}

			pAnime = &pComEfc->pAnime[m_dwSelectID2];
			if (pAnime) {
				wsprintf (szTmp, "%d", pAnime->byGrpID);
				SetWindowText (m_hWndEdit2, szTmp);
				wsprintf (szTmp, "%d", pAnime->byWait);
				SetWindowText (m_hWndEdit3, szTmp);
			}
			InvalidateRect (hWnd, NULL, TRUE);
		}
		break;

	/* 取得 */
	case IDC_BUTTON3:
		{
			char szTmp[32];
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit1, szTmp, sizeof (szTmp));
			m_dwSelectID = atoi (szTmp);
			if (m_dwSelectID > 0) {
				m_dwSelectID --;
			}
			m_dwSelectID = min (m_dwSelectID, 99);

			SetWindowText (m_hWndEdit2, "");
			SetWindowText (m_hWndEdit3, "");

			PCOMEFCINFO pComEfc = &g_pComEfcInfo[m_dwSelectID];
			PCOMEFCANIMEINFO pAnime = NULL;
			pAnime = &pComEfc->pAnime[0];
			if (pAnime) {
				wsprintf (szTmp, "%d", pAnime->byGrpID);
				SetWindowText (m_hWndEdit2, szTmp);
				wsprintf (szTmp, "%d", pAnime->byWait);
				SetWindowText (m_hWndEdit3, szTmp);
			}
			m_dwSelectID2 = 0;
			BYTE byNo = 0;
			for (int i = 0; ; i ++) {
				if (adwSoundNoTbl[i] == -1) {
					break;
				}
				if (pComEfc->bySound == adwSoundNoTbl[i]) {
					byNo = i;
					break;
				}
			}
			SendMessage (m_hWndCombo1, CB_SETCURSEL, byNo, 0);
			InvalidateRect (hWnd, NULL, TRUE);
		}
		break;

	/* 追加 */
	case IDC_BUTTON4:
		{
			PCOMEFCINFO pComEfc = &g_pComEfcInfo[m_dwSelectID];
			pComEfc->byAnimeCount ++;
			PCOMEFCANIMEINFO pAnimeNew = new COMEFCANIMEINFO[pComEfc->byAnimeCount];
			ZeroMemory (pAnimeNew, sizeof (COMEFCANIMEINFO) * pComEfc->byAnimeCount);
			if (pComEfc->pAnime) {
				CopyMemory (pAnimeNew, pComEfc->pAnime, sizeof (COMEFCANIMEINFO) * (pComEfc->byAnimeCount - 1));
				SAFE_DELETE_ARRAY (pComEfc->pAnime);
			}
			pComEfc->pAnime = pAnimeNew;
			InvalidateRect (hWnd, NULL, TRUE);
		}
		break;

	/* クリア */
	case IDC_BUTTON5:
		{
			PCOMEFCINFO pComEfc = &g_pComEfcInfo[m_dwSelectID];
			SAFE_DELETE_ARRAY (pComEfc->pAnime);
			ZeroMemory (pComEfc, sizeof (COMEFCINFO));
			m_dwSelectID2 = 0;
			SetWindowText (m_hWndEdit2, "");
			SetWindowText (m_hWndEdit3, "");
			SendMessage (m_hWndCombo1, CB_SETCURSEL, 0, 0);
			InvalidateRect (hWnd, NULL, TRUE);
		}
		break;

	/* 送信 */
	case IDC_BUTTON6:
		{
			PCOMEFCINFO pComEfc = &g_pComEfcInfo[m_dwSelectID];
			pComEfc->bySound = (BYTE)adwSoundNoTbl[SendMessage (m_hWndCombo1, CB_GETCURSEL, 0, 0)];
			SendCmdCHGCEF (m_dwSelectID);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnCommandEfcEdit											 */
/* 内容：WM_COMMAND処理(エフェクト設定)										 */
/* 日付：2003/09/23															 */
/* ========================================================================= */
void AdminOnCommandEfcEdit(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id) {
	/* 追加 */
	case IDC_BUTTON7:
		SendCmdADDEFC ();
		break;

	/* ← */
	case IDC_BUTTON1:
	/* → */
	case IDC_BUTTON2:
		{
			int nCount;
			char szTmp[128];
			ZeroMemory (szTmp, sizeof (szTmp));
			PEFCINFO pEfc = g_EfcInfo.GetPtr (m_dwSelectID);
			PEFCANIMEINFO pAnime = pEfc->aAnime.GetPtr (m_dwSelectID2);

			if (pAnime) {
				GetWindowText (m_hWndEdit2, szTmp, sizeof (szTmp));
				pAnime->byWait = (BYTE)atoi (szTmp);
				GetWindowText (m_hWndEdit3, szTmp, sizeof (szTmp));
				pAnime->byLevel = (BYTE)atoi (szTmp);
			}

			if (id == IDC_BUTTON1) {
				if (m_dwSelectID2 < 1) {
					break;
				}
				m_dwSelectID2 --;

			} else {
				nCount = pEfc->aAnime.GetCount ();
				if ((nCount == 0) || (m_dwSelectID2 >= (DWORD)nCount - 1)) {
					break;
				}
				m_dwSelectID2 ++;
			}

			pAnime = pEfc->aAnime.GetPtr (m_dwSelectID2);
			if (pAnime) {
				wsprintf (szTmp, "%d", pAnime->wGrpID);
				SetWindowText (m_hWndEdit1, szTmp);
				wsprintf (szTmp, "%d", pAnime->byWait);
				SetWindowText (m_hWndEdit2, szTmp);
				wsprintf (szTmp, "%d", pAnime->byLevel);
				SetWindowText (m_hWndEdit3, szTmp);
			}
			InvalidateRect (hWnd, NULL, TRUE);
		}
		break;

	/* 取得 */
	case IDC_BUTTON3:
		{
			if (g_EfcInfo.GetCount () == 0) {
				break;
			}
			char szTmp[32];
			ZeroMemory (szTmp, sizeof (szTmp));
			GetWindowText (m_hWndEdit4, szTmp, sizeof (szTmp));
			m_dwSelectID = (DWORD)atoi (szTmp);
			if (m_dwSelectID > 0) {
				m_dwSelectID --;
			}
			m_dwSelectID = min (m_dwSelectID, (DWORD)g_EfcInfo.GetCount () - 1);

			SetWindowText (m_hWndEdit1, "");
			SetWindowText (m_hWndEdit2, "");
			SetWindowText (m_hWndEdit3, "");

			PEFCINFO pEfc = g_EfcInfo.GetPtr (m_dwSelectID);
			PEFCANIMEINFO pAnime = NULL;
			pAnime = pEfc->aAnime.GetPtr (0);
			if (pAnime) {
				wsprintf (szTmp, "%d", pAnime->wGrpID);
				SetWindowText (m_hWndEdit1, szTmp);
				wsprintf (szTmp, "%d", pAnime->byWait);
				SetWindowText (m_hWndEdit2, szTmp);
				wsprintf (szTmp, "%d", pAnime->byLevel);
				SetWindowText (m_hWndEdit3, szTmp);
			}
			m_dwSelectID2 = 0;

			BYTE byNo = 0;
			for (int i = 0; ; i ++) {
				if (adwSoundNoTbl[i] == -1) {
					break;
				}
				if (pEfc->bySound == adwSoundNoTbl[i]) {
					byNo = i;
					break;
				}
			}
			SendMessage (m_hWndCombo1, CB_SETCURSEL, byNo, 0);
			SendMessage (m_hWndCombo2, CB_SETCURSEL, pEfc->byType, 0);
			InvalidateRect (hWnd, NULL, TRUE);
		}
		break;

	/* 追加 */
	case IDC_BUTTON4:
		g_EfcInfo.AddNewAnime (m_dwSelectID);
		InvalidateRect (hWnd, NULL, TRUE);
		break;

	/* クリア */
	case IDC_BUTTON5:
		g_EfcInfo.DeleteAllAnime (m_dwSelectID);
		m_dwSelectID2 = 0;
		SetWindowText (m_hWndEdit1, "");
		SetWindowText (m_hWndEdit2, "");
		SetWindowText (m_hWndEdit3, "");
		SendMessage (m_hWndCombo1, CB_SETCURSEL, 0, 0);
		SendMessage (m_hWndCombo2, CB_SETCURSEL, 0, 0);
		InvalidateRect (hWnd, NULL, TRUE);
		break;

	/* 送信 */
	case IDC_BUTTON6:
		{
			PEFCINFO pEfc = g_EfcInfo.GetPtr (m_dwSelectID);
			char szTmp[32];
			ZeroMemory (szTmp, sizeof (szTmp));

			/* 効果音を取得 */
			pEfc->bySound = (BYTE)adwSoundNoTbl[SendMessage (m_hWndCombo1, CB_GETCURSEL, 0, 0)];

			SendCmdCHGEFC ((BYTE)m_dwSelectID);
		}
		break;

	/* 画像種別 */
	case IDC_TMPCOMBO2:
		{
			PEFCINFO pEfc = g_EfcInfo.GetPtr (m_dwSelectID);
			pEfc->byType = (BYTE)SendMessage (m_hWndCombo2, CB_GETCURSEL, 0, 0);

			if (pEfc->byType == 1) {
				SetScrollRange (m_hWndScrBar1, SB_CTL, 0, (g_wCountEfcGrp * 16 * 8) - 5, FALSE);
				SetScrollPos (m_hWndScrBar1, SB_CTL, 0, TRUE);
			} else {
				SetScrollRange (m_hWndScrBar1, SB_CTL, 0, (g_wCountEfcGrp1632 * 16 * 8) - 5, FALSE);
				SetScrollPos (m_hWndScrBar1, SB_CTL, 0, TRUE);
			}
			InvalidateRect (hWnd, NULL, TRUE);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnCommandSubGameInfo										 */
/* 内容：WM_COMMAND処理(サブゲーム情報)										 */
/* 日付：2003/11/08															 */
/* ========================================================================= */
void AdminOnCommandSubGameInfo(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id) {
	/* 取得 */
	case IDC_BUTTON1:
		if (g_pSubGameInfo) {
			SAFE_DELETE_ARRAY (g_pSubGameInfo->pInfo);
		}
		MakeAdminWndSubGameInfoType ();
		SendCmdSUBGAMEINFOGET (g_nSelectSubGameID);
		break;

	/* 追加 */
	case IDC_BUTTON2:
		SendCmdSUBGAMEINFOADD ();
		break;

	/* ← */
	case IDC_BUTTON5:
		if (g_nSelectSubGameID <= 0) {
			break;
		}
		g_nSelectSubGameID --;
		PostMessage (hWnd, WM_COMMAND, IDC_BUTTON1, 0);
		break;

	/* → */
	case IDC_BUTTON6:
		if (g_nSubGameCount <= 0) {
			break;
		}
		if (g_nSelectSubGameID >= g_nSubGameCount - 1) {
			break;
		}
		g_nSelectSubGameID ++;
		PostMessage (hWnd, WM_COMMAND, IDC_BUTTON1, 0);
		break;

	/* クリア */
	case IDC_BUTTON4:
		if (g_pSubGameInfo) {
			SAFE_DELETE_ARRAY (g_pSubGameInfo->pInfo);
			ZeroMemory (g_pSubGameInfo, sizeof (SUBGAMEINFO));
			MakeAdminWndSubGameInfoType ();
			InvalidateRect (hWnd, NULL, TRUE);
		}
		break;

	/* 送信 */
	case IDC_BUTTON3:
		{
			if (g_nSubGameCount == 0) {
				break;
			}
			if (g_pSubGameInfo == NULL) {
				break;
			}
			char szTmp[10];
			switch (g_pSubGameInfo->byType) {
			case SUBGAMEID_BOMB:		/* 爆弾ゲーム */
				{
					PSUBGAME_BOMBINFOSAVE pBomb = (PSUBGAME_BOMBINFOSAVE)g_pSubGameInfo->pInfo;
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit1, szTmp, 5);
					pBomb->dwMapID = atoi (szTmp);
					GetWindowText (m_hWndEdit2, szTmp, 5);
					pBomb->dwX = atoi (szTmp);
					GetWindowText (m_hWndEdit3, szTmp, 5);
					pBomb->dwY = atoi (szTmp);
					GetWindowText (m_hWndEdit4, szTmp, 5);
					pBomb->dwStartX = atoi (szTmp);
					GetWindowText (m_hWndEdit5, szTmp, 5);
					pBomb->dwStartY = atoi (szTmp);

					GetWindowText (m_hWndEdit6, szTmp, 5);
					pBomb->byX1 = (BYTE)atoi (szTmp);
					GetWindowText (m_hWndEdit7, szTmp, 5);
					pBomb->byY1 = (BYTE)atoi (szTmp);
					GetWindowText (m_hWndEdit8, szTmp, 5);
					pBomb->byX2 = (BYTE)atoi (szTmp);
					GetWindowText (m_hWndEdit9, szTmp, 5);
					pBomb->byY2 = (BYTE)atoi (szTmp);
				}
				break;

			case SUBGAMEID_MAZE:		/* 迷路 */
				{
					PSUBGAME_MAZEINFOSAVE pMaze = (PSUBGAME_MAZEINFOSAVE)g_pSubGameInfo->pInfo;
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit1, szTmp, 5);
					pMaze->dwMapID = atoi (szTmp);

					GetWindowText (m_hWndEdit2, szTmp, 5);
					pMaze->byX1 = (BYTE)atoi (szTmp);
					GetWindowText (m_hWndEdit3, szTmp, 5);
					pMaze->byY1 = (BYTE)atoi (szTmp);
					GetWindowText (m_hWndEdit4, szTmp, 5);
					pMaze->byX2 = (BYTE)atoi (szTmp);
					GetWindowText (m_hWndEdit5, szTmp, 5);
					pMaze->byY2 = (BYTE)atoi (szTmp);

					GetWindowText (m_hWndEdit6, szTmp, 5);
					pMaze->byStartX = (BYTE)atoi (szTmp);
					GetWindowText (m_hWndEdit7, szTmp, 5);
					pMaze->byStartY = (BYTE)atoi (szTmp);

					GetWindowText (m_hWndEdit8, szTmp, 5);
					pMaze->byGoalToX = (BYTE)atoi (szTmp);
					GetWindowText (m_hWndEdit9, szTmp, 5);
					pMaze->byGoalToY = (BYTE)atoi (szTmp);

					GetWindowText (m_hWndEdit10, szTmp, 5);
					pMaze->wChipIDStart = (WORD)atoi (szTmp);
					GetWindowText (m_hWndEdit11, szTmp, 5);
					pMaze->wChipIDGoal = (WORD)atoi (szTmp);
					GetWindowText (m_hWndEdit12, szTmp, 5);
					pMaze->wChipIDRoad = (WORD)atoi (szTmp);
					GetWindowText (m_hWndEdit13, szTmp, 5);
					pMaze->wChipIDBlock = (WORD)atoi (szTmp);
				}
				break;

			case SUBGAMEID_DUNGEON:		/* ダンジョン */
				{
					PSUBGAME_DUNGEONINFOSAVE pDungeon = (PSUBGAME_DUNGEONINFOSAVE)g_pSubGameInfo->pInfo;
					ZeroMemory (szTmp, sizeof (szTmp));
					GetWindowText (m_hWndEdit1, szTmp, 5);
					pDungeon->dwMapID = atoi (szTmp);

					GetWindowText (m_hWndEdit2, szTmp, 5);
					pDungeon->byX1 = (BYTE)atoi (szTmp);
					GetWindowText (m_hWndEdit3, szTmp, 5);
					pDungeon->byY1 = (BYTE)atoi (szTmp);
					GetWindowText (m_hWndEdit4, szTmp, 5);
					pDungeon->byX2 = (BYTE)atoi (szTmp);
					GetWindowText (m_hWndEdit5, szTmp, 5);
					pDungeon->byY2 = (BYTE)atoi (szTmp);

					GetWindowText (m_hWndEdit6, szTmp, 5);
					pDungeon->byStartX = (BYTE)atoi (szTmp);
					GetWindowText (m_hWndEdit7, szTmp, 5);
					pDungeon->byStartY = (BYTE)atoi (szTmp);

					GetWindowText (m_hWndEdit8, szTmp, 5);
					pDungeon->byGoalX = (BYTE)atoi (szTmp);
					GetWindowText (m_hWndEdit9, szTmp, 5);
					pDungeon->byGoalY = (BYTE)atoi (szTmp);

					GetWindowText (m_hWndEdit10, szTmp, 5);
					pDungeon->wChipIDStart = (WORD)atoi (szTmp);
					GetWindowText (m_hWndEdit11, szTmp, 5);
					pDungeon->wChipIDGoal = (WORD)atoi (szTmp);
					GetWindowText (m_hWndEdit12, szTmp, 5);
					pDungeon->wChipIDRoad = (WORD)atoi (szTmp);
					GetWindowText (m_hWndEdit13, szTmp, 5);
					pDungeon->wChipIDBlock = (WORD)atoi (szTmp);
				}
				break;
			}
			SendCmdSUBGAMEINFO (g_nSelectSubGameID);
		}
		break;

	/* 座標 */
	case IDC_TMPCHECK1:
		if (SendMessage (m_hWndCheck1, BM_GETCHECK, 0, 0) & BST_CHECKED) {
			g_bAdminPos = TRUE;

		} else {
			g_bAdminPos = FALSE;
		}
		break;

	/* 種別 */
	case IDC_TMPCOMBO1:
		{
			if (g_nSubGameCount == 0) {
				break;
			}
			if (g_pSubGameInfo == NULL) {
				break;
			}

			SAFE_DELETE_ARRAY (g_pSubGameInfo->pInfo);
			g_pSubGameInfo->byType = (BYTE)SendMessage (m_hWndCombo1, CB_GETCURSEL, 0, 0);

			int nSize = GetSubGameDataSize (g_pSubGameInfo->byType);
			if (nSize) {
				g_pSubGameInfo->pInfo = new BYTE[nSize];
				ZeroMemory (g_pSubGameInfo->pInfo, nSize);
				switch (g_pSubGameInfo->byType) {
				case SUBGAMEID_BOMB:		/* 爆弾ゲーム */
					{
						PSUBGAME_BOMBINFOSAVE pBomb = (PSUBGAME_BOMBINFOSAVE)g_pSubGameInfo->pInfo;
						pBomb->dwMapID = g_pMapInfo->dwID;
					}
					break;

				case SUBGAMEID_MAZE:		/* 迷路 */
					{
						PSUBGAME_MAZEINFOSAVE pMaze = (PSUBGAME_MAZEINFOSAVE)g_pSubGameInfo->pInfo;
						pMaze->dwMapID = g_pMapInfo->dwID;
					}
					break;

				case SUBGAMEID_DUNGEON:		/* ダンジョン */
					{
						PSUBGAME_DUNGEONINFOSAVE pDungeon = (PSUBGAME_DUNGEONINFOSAVE)g_pSubGameInfo->pInfo;
						pDungeon->dwMapID = g_pMapInfo->dwID;
					}
					break;
				}
			}
			MakeAdminWndSubGameInfoType ();
			InvalidateRect (g_hWndAdmin, NULL, TRUE);
		}
		break;

	/* 作成 */
	case IDC_BUTTON7:
		{
			if (g_nSubGameCount == 0) {
				break;
			}
			if (g_pSubGameInfo == NULL) {
				break;
			}
			switch (g_pSubGameInfo->byType) {
			case SUBGAMEID_MAZE:		/* 迷路 */
				SendCmdSubMAZERENEW ((BYTE)g_nSelectSubGameID);
				break;

			case SUBGAMEID_DUNGEON:		/* 迷路 */
				SendCmdSubDUNGEONRENEW ((BYTE)g_nSelectSubGameID);
				break;
			}
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnPaint														 */
/* 内容：WM_PAINT処理														 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void AdminOnPaint(HWND hWnd)
{
	HDC hdc;
	PAINTSTRUCT ps;
	void (*pfnPaint)(HDC) = NULL;

	hdc = BeginPaint(hWnd, &ps);

	/* 選択状態別に描画 */
	switch (g_nAdminWindowType) {
	case IDM_MAP_PARTS:	pfnPaint = AdminOnPaintMapParts;	break;
	case IDM_MAP_INFO:	pfnPaint = AdminOnPaintMapInfo;	break;
	case IDM_MAP_EVENT:	pfnPaint = AdminOnPaintMapEvent;	break;
	case IDM_MAP_MSG:	pfnPaint = AdminOnPaintMsg;	break;
	case IDM_CHAR_ADD:	pfnPaint = AdminOnPaintAddChar;	break;
	case IDM_CHAR_INFO:	pfnPaint = AdminOnPaintCharInfo;	break;
	case IDM_CHAR_STATUS:	pfnPaint = AdminOnPaintCharStatus;	break;
	case IDM_ACC_ADD:	pfnPaint = AdminOnPaintAccAdd;	break;
	case IDM_ACC_INFO:	pfnPaint = AdminOnPaintAccInfo;	break;
	case IDM_CEF_EDIT:	pfnPaint = AdminOnPaintCefEdit;	break;
	case IDM_ITEM_LIST:	pfnPaint = AdminOnPaintItemList;	break;
	case IDM_CHAR_ITEM:	pfnPaint = AdminOnPaintCharItem;	break;
	case IDM_ETC_SVRINFO:	pfnPaint = AdminOnPaintSvrInfo;	break;
	case IDM_MAP_SHOP:	pfnPaint = AdminOnPaintShopInfo;	break;
	case IDM_EFC_EDIT:	pfnPaint = AdminOnPaintEfcEdit;	break;
	case IDM_ETC_SUBGAME:	pfnPaint = AdminOnPaintSubGame;	break;
	case IDM_ETC_VERSION:	pfnPaint = AdminOnPaintEtcVersion;	break;
	}
	/* ウィンドウを広げた時は、一覧の右横・下に描いた文字も部品と一緒にずらす */
	if (pfnPaint) {
		AdminAutoLayoutPaint (hWnd, hdc, pfnPaint);
	}

	EndPaint(hWnd, &ps);
}

/* ========================================================================= */
/* 関数名：AdminOnLButtonDown												 */
/* 内容：WM_LBUTTONDOWN処理													 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void AdminOnLButtonDown(HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	/* 選択状態別に描画 */
	switch (g_nAdminWindowType) {
	/* パーツ */
	case IDM_MAP_PARTS:
		AdminOnLButtonDownMapParts (fDoubleClick, x, y, keyFlags);
		break;

	/* キャラ追加 */
	case IDM_CHAR_ADD:
		AdminOnLButtonDownAddChar (fDoubleClick, x, y, keyFlags);
		break;

	/* キャラ情報 */
	case IDM_CHAR_INFO:
		AdminOnLButtonDownCharInfo (fDoubleClick, x, y, keyFlags);
		break;

	/* 吹き出し設定 */
	case IDM_CEF_EDIT:
		AdminOnLButtonDownCefEdit (fDoubleClick, x, y, keyFlags);
		break;

	/* エフェクト設定 */
	case IDM_EFC_EDIT:
		AdminOnLButtonDownEfcEdit (fDoubleClick, x, y, keyFlags);
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnRButtonDown												 */
/* 内容：WM_RBUTTONDOWN処理													 */
/* 日付：2003/06/04															 */
/* ========================================================================= */
void AdminOnRButtonDown(HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	/* 選択状態別に描画 */
	switch (g_nAdminWindowType) {
	/* パーツ */
	case IDM_MAP_PARTS:
		AdminOnRButtonDownMapParts (fDoubleClick, x, y, keyFlags);
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnHScroll													 */
/* 内容：WM_HSCROLL処理(マップパーツ編集ウィンドウ)							 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void AdminOnHScroll(HWND hWnd, HWND hWndCtl, UINT code, int pos)
{
	/* 選択状態別に描画 */
	switch (g_nAdminWindowType) {
	/* パーツ */
	case IDM_MAP_PARTS:
		AdminOnHScrollMapParts (hWndCtl, code, pos);
		break;

	/* キャラ追加 */
	case IDM_CHAR_ADD:
	/* キャラ情報 */
	case IDM_CHAR_INFO:
		AdminOnHScrollAddChar (hWndCtl, code, pos);
		break;

	/* 吹き出し設定 */
	case IDM_CEF_EDIT:
		AdminOnHScrollCefEdit (hWndCtl, code, pos);
		break;

	/* エフェクト設定 */
	case IDM_EFC_EDIT:
		AdminOnHScrollEfcEdit (hWndCtl, code, pos);
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnVScroll													 */
/* 内容：WM_VSCROLL処理(マップパーツ編集ウィンドウ)							 */
/* 日付：2003/06/04															 */
/* ========================================================================= */
void AdminOnVScroll(HWND hWnd, HWND hWndCtl, UINT code, int pos)
{
	/* 選択状態別に描画 */
	switch (g_nAdminWindowType) {
	/* パーツ */
	case IDM_MAP_PARTS:
		AdminOnVScrollMapParts (hWndCtl, code, pos);
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnPaintMapParts												 */
/* 内容：WM_PAINT処理(マップパーツ編集ウィンドウ)							 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void AdminOnPaintMapParts(HDC hDC)
{
	HFONT hFontOld;
	char szBuf[64];
	PMAPPARTSINFO pParts;

	if ((g_nSelectPartsID < 0) ||
		(g_nSelectPartsID >= g_wCountMapParts) ||
		(!g_pMapPartsInfo)) {
		return;
	}
	pParts = (PMAPPARTSINFO)&g_pMapPartsInfo[g_nSelectPartsID];

	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);

	wsprintf (szBuf, "パーツ数:%ld", g_wCountMapParts);
	SetWindowText (GetDlgItem (m_hWndDialog, IDC_PARTSCOUNT), szBuf);
	wsprintf (szBuf, "パーツID:%ld", g_nSelectPartsID);
	SetWindowText (GetDlgItem (m_hWndDialog, IDC_PARTSID), szBuf);

	if (pParts->byAnimeCount) {
		PANIMEINFO pAnime = &pParts->pAnimeInfo[g_byAdminAnimeNo];
		if (pAnime) {
			wsprintf (szBuf, "ｱﾆﾒ:%d/%d", g_byAdminAnimeNo + 1, pParts->byAnimeCount);
			SetWindowText (GetDlgItem (m_hWndDialog, IDC_ANIME), szBuf);
			wsprintf (szBuf, "メインGrpID:%d", pAnime->wGrpID);
			SetWindowText (GetDlgItem (m_hWndDialog, IDC_MAINGRP), szBuf);
			wsprintf (szBuf, "背景GrpID:%d", pAnime->wBackID);
			SetWindowText (GetDlgItem (m_hWndDialog, IDC_BACKGRP), szBuf);
		}

	} else {
		SetWindowText (GetDlgItem (m_hWndDialog, IDC_ANIME), "ｱﾆﾒ:");
		wsprintf (szBuf, "メインGrpID:%d", pParts->wGrpID);
		SetWindowText (GetDlgItem (m_hWndDialog, IDC_MAINGRP), szBuf);
		wsprintf (szBuf, "背景GrpID:%d", pParts->wBackID);
		SetWindowText (GetDlgItem (m_hWndDialog, IDC_BACKGRP), szBuf);
	}

	MyTextOut (hDC, 12 * 10 - 16, 12 * 4 + 1, "透明度:");

	SelectObject (hDC, hFontOld);

	/* パーツグラフィックの表示 */
	int i, xx, yy, no, pos = GetScrollPos (m_hWndMapPartsScrGrp, SB_CTL);
	HDC hDCTmp;
	CImg32 *pDibTmp = new CImg32;
	pDibTmp->Create (32, 32);
	DWORD dwID, dwNo;

	HDC hDCTmp2 = GetWindowDC (GetDlgItem (m_hWndDialog, IDC_PARTSGRP));
	BitBlt (hDCTmp2, 0, 0, 32 * 5, 32, NULL, 0, 0, BLACKNESS);

	for (i = 0; i < 5; i ++) {
		dwNo = (pos + i) / (g_byMapGrpCountX * g_byMapGrpCountY);
		dwID = (pos + i) % (g_byMapGrpCountX * g_byMapGrpCountY);
		xx = dwID % g_byMapGrpCountX * 32;
		yy = dwID / g_byMapGrpCountX * 32;

		pDibTmp->Blt (0, 0, 32, 32, g_apDibMap[dwNo], xx, yy);
		hDCTmp = pDibTmp->Lock ();
		BitBlt (hDCTmp2, i * 32, 0, 32, 32, hDCTmp, 0, 0, SRCCOPY);
		pDibTmp->Unlock ();
	}
	delete pDibTmp;
	ReleaseDC (GetDlgItem (m_hWndDialog, IDC_PARTSGRP), hDCTmp2);

	pDibTmp = new CImg32;
	pDibTmp->Create (32, 32);
	pos = GetScrollPos (m_hWndMapPartsScrParts, SB_CTL) * 10;
	hDCTmp2 = GetWindowDC (GetDlgItem (m_hWndDialog, IDC_PARTS));
	BitBlt (hDCTmp2, 0, 0, 32 * 10, 32 * 4, NULL, 0, 0, BLACKNESS);
	/* パーツリストの表示 */
	for (i = 0; i < 40; i ++) {
		if (i + pos >= g_wCountMapParts) {
			break;
		}
		pParts = (PMAPPARTSINFO)&g_pMapPartsInfo[i + pos];
		no = pParts->wBackID;
		if (pParts->byAnimeCount) {
			PANIMEINFO pAnime = &pParts->pAnimeInfo[0];
			if (pAnime == NULL) {
				continue;
			}
			no = pAnime->wBackID;
		}

		dwNo = no / (g_byMapGrpCountX * g_byMapGrpCountY);
		dwID = no % (g_byMapGrpCountX * g_byMapGrpCountY);
		xx = dwID % g_byMapGrpCountX * 32;
		yy = dwID / g_byMapGrpCountX * 32;

		pDibTmp->Blt (0, 0, 32, 32, g_apDibMap[dwNo], xx, yy);
		hDCTmp = pDibTmp->Lock ();
		BitBlt (hDCTmp2, (i % 10) * 32, (i / 10) * 32, 32, 32, hDCTmp, 0, 0, SRCCOPY);
		pDibTmp->Unlock ();

		pParts = (PMAPPARTSINFO)&g_pMapPartsInfo[i + pos];
		no = pParts->wGrpID;
		if (pParts->byAnimeCount) {
			PANIMEINFO pAnime = &pParts->pAnimeInfo[0];
			no = pAnime->wGrpID;
		}

		dwNo = no / (g_byMapGrpCountX * g_byMapGrpCountY);
		dwID = no % (g_byMapGrpCountX * g_byMapGrpCountY);
		xx = dwID % g_byMapGrpCountX * 32;
		yy = dwID / g_byMapGrpCountX * 32;

		pDibTmp->Blt (0, 0, 32, 32, g_apDibMap[dwNo], xx, yy, TRUE);
		hDCTmp = pDibTmp->Lock ();
		BitBlt (hDCTmp2, (i % 10) * 32, (i / 10) * 32, 32, 32, hDCTmp, 0, 0, SRCCOPY);
		pDibTmp->Unlock ();
	}
	delete pDibTmp;
}

/* ========================================================================= */
/* 関数名：AdminOnPaintMapInfo												 */
/* 内容：WM_PAINT処理(マップ情報ウィンドウ)									 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void AdminOnPaintMapInfo(HDC hDC)
{
	HFONT hFontOld;
	char szBuf[64];

	if (!g_pMapInfo) {
		return;
	}

	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);

	wsprintf (szBuf, "マップID:%ld", g_pMapInfo->dwID);
	MyTextOut (hDC, 0, 12 * 0, szBuf);
	wsprintf (szBuf, "横サイズ:%ld", g_pMapInfo->nCX);
	MyTextOut (hDC, 0, 12 * 1, szBuf);
	wsprintf (szBuf, "縦サイズ:%ld", g_pMapInfo->nCY);
	MyTextOut (hDC, 0, 12 * 2, szBuf);
	wsprintf (szBuf, "スタート位置(X):%ld", g_pMapInfo->nStartX);
	MyTextOut (hDC, 0, 12 * 3, szBuf);
	wsprintf (szBuf, "スタート位置(Y):%ld", g_pMapInfo->nStartY);
	MyTextOut (hDC, 0, 12 * 4, szBuf);
	wsprintf (szBuf, "BGM:");
	MyTextOut (hDC, 0, 12 * 5, szBuf);
	wsprintf (szBuf, "マップ名(半角128文字以内):");
	MyTextOut (hDC, 0, 12 * 7, szBuf);
	wsprintf (szBuf, "入り口(X):");
	MyTextOut (hDC, 0, 12 * 10, szBuf);
	wsprintf (szBuf, "入り口(Y):");
	MyTextOut (hDC, 0 * 5, 12 * 11 + 6, szBuf);
	wsprintf (szBuf, "天候:");
	MyTextOut (hDC, 0, 12 * 13 + 3, szBuf);

	/* 明るさ・灯り・見え方（変えるのは上のボタンから開く小窓で。ここは今の内容の一覧） */
	{
		int i, y = MAPINFO_SUMMARY_Y;
		char szTmp[128];
		COLORREF clHead = RGB (255, 220, 120), clNone = RGB (160, 160, 160);

		if (g_pMapInfo->byViewRange) {
			wsprintf (szTmp, "見える範囲: %dマス", g_pMapInfo->byViewRange);
		} else {
			wsprintf (szTmp, "見える範囲: 制限なし");
		}
		MyTextOut (hDC, 0, y, szTmp, clHead);
		if (g_pMapInfo->wViewHideParts) {
			wsprintf (szTmp, "見えない所: パーツ%d", g_pMapInfo->wViewHideParts);
		} else {
			wsprintf (szTmp, "見えない所: 黒");
		}
		MyTextOut (hDC, 12 * 12, y, szTmp, clHead);
		y += MAPINFO_SUMMARY_GAP;

		wsprintf (szTmp, "明るさの範囲 %d/%d（下ほど優先）", g_pMapInfo->byDarkAreaCount, MAX_MAPDARKAREA);
		MyTextOut (hDC, 0, y, szTmp, clHead);
		y += 12;
		for (i = 0; i < g_pMapInfo->byDarkAreaCount && i < MAX_MAPDARKAREA; i ++) {
			MapDarkAreaLabel (i, szTmp, sizeof (szTmp));
			MyTextOut (hDC, 12, y, szTmp);
			y += 12;
		}
		if (g_pMapInfo->byDarkAreaCount == 0) {
			MyTextOut (hDC, 12, y, "なし（暗闇・明るさの設定どおりマップ全体）", clNone);
			y += 12;
		}
		y += MAPINFO_SUMMARY_GAP - 12;

		wsprintf (szTmp, "光源 %d/%d", g_pMapInfo->byLightCount, MAX_MAPLIGHT);
		MyTextOut (hDC, 0, y, szTmp, clHead);
		y += 12;
		for (i = 0; i < g_pMapInfo->byLightCount && i < MAX_MAPLIGHT; i ++) {
			MapLightLabel (i, szTmp, sizeof (szTmp));
			MyTextOut (hDC, 12, y, szTmp);
			y += 12;
		}
		if (g_pMapInfo->byLightCount == 0) {
			MyTextOut (hDC, 12, y, "なし", clNone);
		}
	}

	MyTextOut (hDC, 12 * 23, 0, "赤:");
	MyTextOut (hDC, 12 * 23, 16, "緑:");
	MyTextOut (hDC, 12 * 23, 32, "青:");

	SelectObject (hDC, hFontOld);
}

/* ========================================================================= */
/* 関数名：AdminOnPaintMapEvent												 */
/* 内容：WM_PAINT処理(イベント情報ウィンドウ)								 */
/* 日付：2003/06/02															 */
/* ========================================================================= */
void AdminOnPaintMapEvent(HDC hDC)
{
	HFONT hFontOld;
	char szBuf[64];

	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);

	wsprintf (szBuf, "イベント登録数:%ld", g_nEventCount);
	MyTextOut (hDC, 0, 12 * 0, szBuf);
	wsprintf (szBuf, "%ld", g_nSelectEventID);
	MyTextOut (hDC, 0, 12 * 1, szBuf);

	if (g_pEventInfo) {
		wsprintf (szBuf, "X座標:%ld", g_pEventInfo->dwX);
		MyTextOut (hDC, 0, 12 * 2, szBuf);
		wsprintf (szBuf, "Y座標:%ld", g_pEventInfo->dwY);
		MyTextOut (hDC, 0, 12 * 3, szBuf);
		wsprintf (szBuf, "種別:%d", g_pEventInfo->byType);
		MyTextOut (hDC, 0, 12 * 4, szBuf);

		if (g_pEventInfo->pData) {
			switch (g_pEventInfo->byType) {
			case EVENTTYPE_MOVE:
				{
					PEVENTINFO_MOVE pMove = (PEVENTINFO_MOVE)g_pEventInfo->pData;
					MyTextOut (hDC, 0, 64 + 16 * 0, "移動先X座標:");
					MyTextOut (hDC, 0, 64 + 16 * 1, "移動先Y座標:");
					MyTextOut (hDC, 0, 64 + 16 * 2 + 2, "エフェクト:");
				}
				break;

			case EVENTTYPE_MAPCHG:
				{
					PEVENTINFO_MAPCHG pMap = (PEVENTINFO_MAPCHG)g_pEventInfo->pData;
					MyTextOut (hDC, 0, 64 + 16 * 0, "マップID:");
					MyTextOut (hDC, 0, 64 + 16 * 1, "移動先X座標:");
					MyTextOut (hDC, 0, 64 + 16 * 2, "移動先Y座標:");
					MyTextOut (hDC, 0, 64 + 16 * 3 + 2, "エフェクト:");
					MyTextOut (hDC, 0, 64 + 16 * 5, "ばらける範囲:");
					MyTextOut (hDC, 12 * 7 + 36, 64 + 16 * 5, "マス(0=しない)");
				}
				break;

			case EVENTTYPE_MSG:
				{
					PEVENTINFO_MSG pMsg = (PEVENTINFO_MSG)g_pEventInfo->pData;
					MyTextOut (hDC, 0, 64 + 16 * 0, "メッセージID:");
					MyTextOut (hDC, 0, 64 + 16 * 1 + 2, "種別:");
					MyTextOut (hDC, 0, 64 + 16 * 3, "書き換えｸﾞﾗﾌｨｯｸID:");
					MyTextOut (hDC, 0, 64 + 16 * 4, "X:");
					MyTextOut (hDC, 0, 64 + 16 * 5, "Y:");
				}
				break;

			case EVENTTYPE_DOOR:
				{
					PEVENTINFO_DOOR pDoor = (PEVENTINFO_DOOR)g_pEventInfo->pData;
					MyTextOut (hDC, 0, 64 + 16 * 0, "グラフィックID:");
					MyTextOut (hDC, 0, 64 + 16 * 1, "背景グラフィックID:");
					MyTextOut (hDC, 0, 64 + 16 * 2, "閉まるまでの時間(100ms):");
					MyTextOut (hDC, 0, 64 + 16 * 3 + 2, "種別:");
				}
				break;

			case EVENTTYPE_HPMANA:
				{
					PEVENTINFO_DOOR pDoor = (PEVENTINFO_DOOR)g_pEventInfo->pData;
					MyTextOut (hDC, 0, 64 + 16 * 0 + 2, "種別:");
				}
				break;

			case EVENTTYPE_ONCHARMAP:	/* キャラが乗っている間マップ変更 */
				{
					PEVENTINFO_ONCHARMAP pInfo = (PEVENTINFO_ONCHARMAP)g_pEventInfo->pData;
					MyTextOut (hDC, 0, 64 + 16 * 0, "乗っている:");
					MyTextOut (hDC, 0, 64 + 16 * 1, "乗っていない:");
					MyTextOut (hDC, 0, 64 + 16 * 2, "X:");
					MyTextOut (hDC, 0, 64 + 16 * 3, "Y:");
				}
				break;
			}
		}
	}

	SelectObject (hDC, hFontOld);
}

/* ========================================================================= */
/* 関数名：AdminOnPaintAddChar												 */
/* 内容：WM_PAINT処理(キャラ追加ウィンドウ)									 */
/* 日付：2003/06/08															 */
/* ========================================================================= */
void AdminOnPaintAddChar(HDC hDC)
{
	HFONT hFontOld;
	char szBuf[64];

	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);

	wsprintf (szBuf, "グラフィックID:%d", m_pAddCharInfo->wGrpID);
	MyTextOut (hDC, 0, 12 * 3, szBuf);
	wsprintf (szBuf, "X座標:%ld", m_pAddCharInfo->dwX);
	MyTextOut (hDC, 0, 12 * 4, szBuf);
	wsprintf (szBuf, "Y座標:%ld", m_pAddCharInfo->dwY);
	MyTextOut (hDC, 0, 12 * 5, szBuf);

	SelectObject (hDC, hFontOld);

	int i, xx, yy, pos = GetScrollPos (m_hWndScrBar1, SB_CTL);
	HDC hDCTmp;
	CImg32 *pDibTmp = new CImg32;
	pDibTmp->Create (32, 32);
	for (i = 0; i < 5; i ++) {

		xx = 2 * 32;
		yy = ((pos + i) % 16) * 32;

		pDibTmp->Blt (0, 0, 32, 32, g_apDibChar[(pos + i) / 16], xx, yy);
		hDCTmp = pDibTmp->Lock ();
		BitBlt (hDC, 12 * 17 + i * 32, 12 * 5 - 32, 32, 32, hDCTmp, 0, 0, SRCCOPY);
		pDibTmp->Unlock ();
	}
	delete pDibTmp;
}

/* ========================================================================= */
/* 関数名：AdminOnPaintCharInfo												 */
/* 内容：WM_PAINT処理(キャラ情報ウィンドウ)									 */
/* 日付：2003/06/09															 */
/* ========================================================================= */
void AdminOnPaintCharInfo(HDC hDC)
{
	HFONT hFontOld;
	char szBuf[64];

	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);

	wsprintf (szBuf, "キャラ名:");
	MyTextOut (hDC, 0, 12 * 2, szBuf);
	wsprintf (szBuf, "テーブルID:%ld", m_pParaCharInfo->dwTblID);
	MyTextOut (hDC, 0, 12 * 3, szBuf);
	wsprintf (szBuf, "グラフィックID:%d(%d:%d)", m_pParaCharInfo->wGrpID, m_pParaCharInfo->wGrpMain, m_pParaCharInfo->wGrpSub);
	MyTextOut (hDC, 0, 12 * 4, szBuf);
	wsprintf (szBuf, "エフェクト:%d", m_pParaCharInfo->byEfect);
	MyTextOut (hDC, 0, 12 * 5, szBuf);
	/* NPC番号（保存する NPC ごとの消えない番号。プレイヤー・発生した敵は無し） */
	if (m_pParaCharInfo->dwNpcID) {
		wsprintf (szBuf, "NPC番号:%lu", m_pParaCharInfo->dwNpcID);
	} else {
		wsprintf (szBuf, "NPC番号:なし");
	}
	MyTextOut (hDC, 12 * 8, 12 * 5, szBuf);
	wsprintf (szBuf, "座標:(%ld,%ld,%ld)", m_pParaCharInfo->dwMapID, m_pParaCharInfo->dwX, m_pParaCharInfo->dwY);
	MyTextOut (hDC, 0, 12 * 6, szBuf);
	MyTextOut (hDC, 0, 12 * 7, "開始マップID:");
	MyTextOut (hDC, 0, 12 * 8, "開始X座標:");
	MyTextOut (hDC, 0, 12 * 9, "開始Y座標:");

	MyTextOut (hDC, 0, 12 * 10, "移動確率X:");
	MyTextOut (hDC, 0, 12 * 11, "移動確率Y:");
	MyTextOut (hDC, 0, 12 * 12, "検索範囲X:");
	MyTextOut (hDC, 0, 12 * 13, "検索範囲Y:");

	MyTextOut (hDC, 0, 12 * 14, "会話内容:");

	MyTextOut (hDC, 12 * 10, 12 * 7 + 4, "向き:");
	MyTextOut (hDC, 12 * 9, 12 * 9 + 4, "NPC移動:");

	MyTextOut (hDC, 12 * 10, 12 * 11, "MAPID:");
	MyTextOut (hDC, 12 * 10, 12 * 11 + 16, "X:");
	MyTextOut (hDC, 12 * 10, 12 * 11 + 32, "Y:");

	wsprintf (szBuf, "ルーラID:%d/%d", g_dwSelctRura + 1, g_AdminRuraInfo.GetSize ());
	MyTextOut (hDC, 12 * 22, 12 * 7, szBuf);

	MyTextOut (hDC, 12 * 22, 12 * 16, "アカウント名:");
	MyTextOut (hDC, 12 * 22, 12 * 18, "パスワード:");

	MyTextOut (hDC, 0, 12 * 17, "移動範囲X:");
	MyTextOut (hDC, 0, 12 * 18, "移動範囲Y:");
	MyTextOut (hDC, 0, 12 * 19, "発生数:");
	MyTextOut (hDC, 0, 12 * 20, "発生周期(秒):");

	MyTextOut (hDC, 12 * 10, 12 * 16 + 8, "権限:");
	MyTextOut (hDC, 12 * 10, 12 * 18 + 8, "色:");

	SelectObject (hDC, hFontOld);

	int i, xx, yy, pos = GetScrollPos (m_hWndScrBar1, SB_CTL);
	HDC hDCTmp;
	CImg32 *pDibTmp = new CImg32;
	pDibTmp->Create (32, 32);
	for (i = 0; i < 5; i ++) {

		xx = 2 * 32;
		yy = ((pos + i) % 16) * 32;

		pDibTmp->Blt (0, 0, 32, 32, g_apDibChar[(pos + i) / 16], xx, yy);
		hDCTmp = pDibTmp->Lock ();
		BitBlt (hDC, 12 * 17 + i * 32, 12 * 5 - 32, 32, 32, hDCTmp, 0, 0, SRCCOPY);
		pDibTmp->Unlock ();
	}
	delete pDibTmp;
}

/* ========================================================================= */
/* 関数名：AdminOnPaintCharStatus											 */
/* 内容：WM_PAINT処理(キャラステータス情報ウィンドウ)						 */
/* 日付：2003/10/01															 */
/* ========================================================================= */
void AdminOnPaintCharStatus(HDC hDC)
{
	HFONT hFontOld;

	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);

	MyTextOut (hDC, 12 * 12,		34,					"のステータス:");
	MyTextOut (hDC, 12 * 1,			52 + 15 * 0,		"力:");
	MyTextOut (hDC, 12 * 1 + 100,	52 + 15 * 0,		"素早さ:");
	MyTextOut (hDC, 12 * 1 + 200,	52 + 15 * 0,		"体力:");
	MyTextOut (hDC, 12 * 1,			52 + 15 * 1,		"賢さ:");
	MyTextOut (hDC, 12 * 1 + 100,	52 + 15 * 1,		"運のよさ:");
	MyTextOut (hDC, 12 * 1 + 200,	52 + 15 * 1,		"最大HP:");
	MyTextOut (hDC, 12 * 1,			52 + 15 * 2,		"HP:");
	MyTextOut (hDC, 12 * 1 + 100,	52 + 15 * 2,		"最大Mana:");
	MyTextOut (hDC, 12 * 1 + 200,	52 + 15 * 2,		"Mana:");
	MyTextOut (hDC, 12 * 1,			52 + 15 * 3,		"所持金:");
	MyTextOut (hDC, 12 * 1 + 100,	52 + 15 * 3,		"ﾄﾞﾛｯﾌﾟ率:");
	MyTextOut (hDC, 12 * 1 + 200,	52 + 15 * 3,		"レベル:");
	MyTextOut (hDC, 12 * 1,			52 + 15 * 4,		"火:");
	MyTextOut (hDC, 12 * 1 + 50,	52 + 15 * 4,		"水:");
	MyTextOut (hDC, 12 * 1 + 100,	52 + 15 * 4,		"風:");
	MyTextOut (hDC, 12 * 1 + 150,	52 + 15 * 4,		"土:");
	MyTextOut (hDC, 12 * 1 + 200,	52 + 15 * 4,		"光:");
	MyTextOut (hDC, 12 * 1 + 250,	52 + 15 * 4,		"闇:");
	MyTextOut (hDC, 12 * 1,			52 + 15 * 5,		"魔法防御:");
	MyTextOut (hDC, 12 * 1 + 100,	52 + 15 * 5,		"経験値:");

	MyTextOut (hDC, 12 * 1,			52 + 15 * 6 + 4,	"性別:");
	MyTextOut (hDC, 12 * 1 + 76,	52 + 15 * 6 + 4,	"移動種別:");
	MyTextOut (hDC, 12 * 18 + 60,	52 + 15 * 6 + 4,	"酔い:");
	MyTextOut (hDC, 12,				15 * 9 + 40,		"ステータステンプレート情報:");

	MyTextOut (hDC, 12 * 1 + 300 - 10,	34 + 12 * 0,	"移動確率X:");
	MyTextOut (hDC, 12 * 1 + 300 - 10,	34 + 12 * 1,	"移動確率Y:");
	MyTextOut (hDC, 12 * 1 + 300 - 10,	34 + 12 * 2,	"検索範囲X:");
	MyTextOut (hDC, 12 * 1 + 300 - 10,	34 + 12 * 3,	"検索範囲Y:");

	SelectObject (hDC, hFontOld);
}

/* ========================================================================= */
/* 関数名：AdminOnPaintMsg													 */
/* 内容：WM_PAINT処理(メッセージウィンドウ)									 */
/* 日付：2003/07/14															 */
/* ========================================================================= */
void AdminOnPaintMsg(HDC hDC)
{
	HFONT hFontOld;
	char szBuf[64];

	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);

	if (g_pMapInfo->nMsgCount) {
		wsprintf (szBuf, "%d/%d", g_nSelectPartsID + 1, g_pMapInfo->nMsgCount);
		MyTextOut (hDC, 0, 0, szBuf);
	}

	SelectObject (hDC, hFontOld);
}

/* ========================================================================= */
/* 関数名：AdminOnPaintAccAdd												 */
/* 内容：WM_PAINT処理(アカウント追加ウィンドウ)								 */
/* 日付：2003/08/23															 */
/* ========================================================================= */
void AdminOnPaintAccAdd(HDC hDC)
{
	HFONT hFontOld;

	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);

	MyTextOut (hDC, 0, 12 * 2, "アカウント名:");
	MyTextOut (hDC, 0, 12 * 4, "パスワード:");

	SelectObject (hDC, hFontOld);
}

/* ========================================================================= */
/* 関数名：AdminOnPaintAccAddInfo											 */
/* 内容：WM_PAINT処理(アカウント編集ウィンドウ)								 */
/* 日付：2003/08/30															 */
/* ========================================================================= */
void AdminOnPaintAccInfo(HDC hDC)
{
	HFONT hFontOld;
	IN_ADDR AddrTmp;

	if (!m_pParaCharInfo) {
		return;
	}

	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);

	char szTmp[128];

	if (m_pParaCharInfo->byNpcType) {
		MyTextOut (hDC, 0, 12 * 2, "このアカウントはNPCです。");
	} else if (strlen (m_pParaCharInfo->szAcc) == 0) {
		MyTextOut (hDC, 0, 12 * 2, "削除対象？");
	}

	wsprintf (szTmp, "テーブルID:%ld", m_pParaCharInfo->dwTblID);
	MyTextOut (hDC, 0, 12 * 4, szTmp);
	wsprintf (szTmp, "アカウント状態:%d", m_pParaCharInfo->byAccState);
	MyTextOut (hDC, 0, 12 * 5, szTmp);
	wsprintf (szTmp, "キャラ名:%s", m_pParaCharInfo->szName);
	MyTextOut (hDC, 0, 12 * 6, szTmp);
	wsprintf (szTmp, "アカウント名:%s", m_pParaCharInfo->szAcc);
	MyTextOut (hDC, 0, 12 * 7, szTmp);
	wsprintf (szTmp, "パスワード:%s", m_pParaCharInfo->szPas);
	MyTextOut (hDC, 0, 12 * 8, szTmp);
	wsprintf (szTmp, "管理者権限:%d", m_pParaCharInfo->byAdminLV);
	MyTextOut (hDC, 0, 12 * 9, szTmp);
	wsprintf (szTmp, "NPC移動種別:%d", m_pParaCharInfo->byNpcType);
	MyTextOut (hDC, 0, 12 * 10, szTmp);
	wsprintf (szTmp, "グラフィックID:%d(%d:%d)", m_pParaCharInfo->wGrpID, m_pParaCharInfo->wGrpMain, m_pParaCharInfo->wGrpSub);
	MyTextOut (hDC, 0, 12 * 11, szTmp);
	wsprintf (szTmp, "マップID:%d", m_pParaCharInfo->dwMapID);
	MyTextOut (hDC, 0, 12 * 12, szTmp);
	wsprintf (szTmp, "X座標:%d", m_pParaCharInfo->dwX);
	MyTextOut (hDC, 0, 12 * 13, szTmp);
	wsprintf (szTmp, "Y座標:%d", m_pParaCharInfo->dwY);
	MyTextOut (hDC, 0, 12 * 14, szTmp);
	wsprintf (szTmp, "ログイン回数:%ld", m_pParaCharInfo->dwLoginCount);
	MyTextOut (hDC, 0, 12 * 15, szTmp);
	wsprintf (szTmp, "プレイ時間:%03d:%02d",
				m_pParaCharInfo->dwPlayTime / 60, m_pParaCharInfo->dwPlayTime % 60);
	MyTextOut (hDC, 0, 12 * 16, szTmp);
	AddrTmp.S_un.S_addr = m_pParaCharInfo->dwLastIPAddress;
	wsprintf (szTmp, "最終IPアドレス:%d.%d.%d.%d",
			AddrTmp.S_un.S_un_b.s_b1,
			AddrTmp.S_un.S_un_b.s_b2,
			AddrTmp.S_un.S_un_b.s_b3,
			AddrTmp.S_un.S_un_b.s_b4);
	MyTextOut (hDC, 0, 12 * 17, szTmp);
	wsprintf (szTmp, "MACアドレス:%02X-%02X-%02X-%02X-%02X-%02X",
				(BYTE)m_pParaCharInfo->szMacAdr[0], (BYTE)m_pParaCharInfo->szMacAdr[1], (BYTE)m_pParaCharInfo->szMacAdr[2],
				(BYTE)m_pParaCharInfo->szMacAdr[3], (BYTE)m_pParaCharInfo->szMacAdr[4], (BYTE)m_pParaCharInfo->szMacAdr[5]);
	MyTextOut (hDC, 0, 12 * 18, szTmp);


	SelectObject (hDC, hFontOld);
}

/* ========================================================================= */
/* 関数名：AdminOnPaintCefEdit												 */
/* 内容：WM_PAINT処理(吹き出し設定ウィンドウ)								 */
/* 日付：2003/08/31															 */
/* ========================================================================= */
void AdminOnPaintCefEdit(HDC hDC)
{
	HFONT hFontOld;

	PCOMEFCINFO pComEfc = &g_pComEfcInfo[m_dwSelectID];
	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);

	char szTmp[128];

	MyTextOut (hDC, 0, 12 * 6, "吹き出し画像ID:");
	MyTextOut (hDC, 0, 12 * 7 + 4, "待ち時間(100ms):");

	if (pComEfc->byAnimeCount) {
		wsprintf (szTmp, "%d/%d", m_dwSelectID2 + 1, pComEfc->byAnimeCount);

	} else {
		wsprintf (szTmp, "0/0");
	}
	MyTextOut (hDC, 12 * 1, 12 * 4, szTmp);

	SelectObject (hDC, hFontOld);

	int i, xx, yy, pos = GetScrollPos (m_hWndScrBar1, SB_CTL);
	HDC hDCTmp;
	CImg32 *pDibTmp = new CImg32;
	pDibTmp->Create (32, 32);
	for (i = 0; i < 5; i ++) {
		xx = ((pos + i) % 8) * 32;
		yy = ((pos + i) / 8) * 32;

		pDibTmp->Blt (0, 0, 32, 32, g_pDibComEfc, xx, yy);
		hDCTmp = pDibTmp->Lock ();
		BitBlt (hDC, 12 * 17 + i * 32, 12 * 5 - 32, 32, 32, hDCTmp, 0, 0, SRCCOPY);
		pDibTmp->Unlock ();
	}
	delete pDibTmp;
}

/* ========================================================================= */
/* 関数名：AdminOnPaintItemList												 */
/* 内容：WM_PAINT処理(アイテム一覧ウィンドウ)								 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void AdminOnPaintItemList(HDC hDC)
{
	HFONT hFontOld;

	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);

	if (m_dwSelectID == -1) {
		SelectObject (hDC, hFontOld);
		return;
	}

	PITEMINFO pItem = g_aItemInfo[m_dwSelectID];

	MyTextOut (hDC, 12 * 1, 132 - 12, "アイテム名(半角30文字以内):");
	MyTextOut (hDC, 12 * 1, 135 + 14 * 1, "値段:");
	MyTextOut (hDC, 12 * 9, 135 + 14 * 1, "消滅％:");
	MyTextOut (hDC, 12 * 1, 135 + 14 * 2, "使用時のメッセージ:");
	MyTextOut (hDC, 12 * 15, 120 - 2, "種別:");
	MyTextOut (hDC, 12 * 15, 120 - 2 + 22, "効果音:");
	MyTextOut (hDC, 12 * 16, 137 + 22 + 16, "性別:");


	switch (pItem->byType) {
	case ITEMTYPE_NONE:		/* 効果無し */
		break;

	case ITEMTYPE_RURA:		/* 特定位置へ移動 */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 4, "移動先:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 5, "マップID:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 6, "X座標:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 7, "Y座標:");
		break;

	case ITEMTYPE_HPUP:	/* HP回復 */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 5, "最低回復値:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 6, "最大回復値:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 7, "回復率:");
		break;

	case ITEMTYPE_WEAPON:	/* 武器 */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 5, "効果:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 6, "エフェクトID:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 7, "攻撃力減少幅:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 8, "付加データ1:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 9, "付加データ2:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 10, "付加データ3:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 11, "付加データ4:");
		MyTextOut (hDC, 12 * 12, 132 + 14 * 11, "付加データ5:");
		MyTextOut (hDC, 12 * 14, 132 + 14 * 5 + 4, "装備職業:");
		MyTextOut (hDC, 12 * 14, 132 + 14 * 6 + 10, "攻撃種別:");
		break;

	case ITEMTYPE_ARMOR:	/* 鎧 */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 5, "効果:");
		MyTextOut (hDC, 12 * 14, 132 + 14 * 5 + 4, "装備職業:");
		break;

	case ITEMTYPE_SHILD:	/* 盾 */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 5, "効果:");
		MyTextOut (hDC, 12 * 14, 132 + 14 * 5 + 4, "装備職業:");
		break;

	case ITEMTYPE_HELMET:	/* 兜 */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 5, "効果:");
		MyTextOut (hDC, 12 * 14, 132 + 14 * 5 + 4, "装備職業:");
		break;

	case ITEMTYPE_SEED:		/* 種 */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 4 + 2, "種類:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 6, "上限値:");
		break;

	case ITEMTYPE_SUBGAME:	/* サブゲーム参加 */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 4 + 2, "種類:");
		break;

	case ITEMTYPE_SAKE:		/* 酒 */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 4 + 2, "度数:");
		break;

	case ITEMTYPE_MSG:	/* メッセージ表示 */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 4 + 2, "他のキャラに使った時のメッセージ:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 6 + 2, "他のキャラに使われた時のメッセージ:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 8 + 2, "文字エフェクト:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 10 + 2, "他のキャラに使われた時のエフェクト番号:");
		MyTextOut (hDC, 12 * 15, 132 + 14 * 8 + 2 - 12, "表示種別:");
		break;

	case ITEMTYPE_GRPCHG:	/* キャラ画像変更 */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 4 + 2, "画像ID:");
		break;

	case ITEMTYPE_LIGHT:	/* 明かり */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 4 + 2, "明るさレベル:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 5 + 2, "維持時間:");
		break;

	case ITEMTYPE_HPDOWN:	/* HP減少 */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 5, "最低減少値:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 6, "最大減少値:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 7, "ｴﾌｪｸﾄ番号:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 8, "ダメージ率:");
		break;

	case ITEMTYPE_MAGIC:	/* 魔法習得 */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 5 + 2, "習得する魔法:");
		break;

	case ITEMTYPE_USEMAGIC:	/* 特技発動 */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 5 + 2, "発動する特技:");
		break;

	case ITEMTYPE_MANAUP:	/* Mana回復 */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 5, "最低回復値:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 6, "最大回復値:");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 7, "回復率:");
		break;

	case ITEMTYPE_HPMANAUP:	/* HP・Mana回復 */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 5, "最低回復値(HP):");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 6, "最大回復値(HP):");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 7, "最低回復値(Mana):");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 8, "最大回復値(Mana):");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 9, "回復率(HP):");
		MyTextOut (hDC, 12 * 1, 132 + 14 * 10, "回復率(Mana):");
		break;

	case ITEMTYPE_EQU_HAND:	/* 装備(小手) */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 5, "効果:");
		MyTextOut (hDC, 12 * 14, 132 + 14 * 5 + 4, "装備職業:");
		break;

	case ITEMTYPE_EQU_LEG:	/* 装備(靴) */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 5, "効果:");
		MyTextOut (hDC, 12 * 14, 132 + 14 * 5 + 4, "装備職業:");
		break;

	case ITEMTYPE_EQU_ACC1:	/* 装備(首飾り) */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 5, "効果:");
		MyTextOut (hDC, 12 * 14, 132 + 14 * 5 + 4, "装備職業:");
		MyTextOut (hDC, 12 * 14, 132 + 14 * 5 + 22 + 4, "装備効果:");
		break;

	case ITEMTYPE_EQU_ACC2:	/* 装備(指輪1) */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 5, "効果:");
		MyTextOut (hDC, 12 * 14, 132 + 14 * 5 + 4, "装備職業:");
		MyTextOut (hDC, 12 * 14, 132 + 14 * 5 + 22 + 4, "装備効果:");
		break;

	case ITEMTYPE_EQU_ACC3:	/* 装備(指輪2) */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 5, "効果:");
		MyTextOut (hDC, 12 * 14, 132 + 14 * 5 + 4, "装備職業:");
		MyTextOut (hDC, 12 * 14, 132 + 14 * 5 + 22 + 4, "装備効果:");
		break;

	case ITEMTYPE_RESURRECTION:	/* 蘇生 */
		MyTextOut (hDC, 12 * 1, 132 + 14 * 5, "回復率:");
		break;

	default:
		break;
	}

	SelectObject (hDC, hFontOld);
}

/* ========================================================================= */
/* 関数名：AdminOnPaintCharItem												 */
/* 内容：WM_PAINT処理(所持アイテム情報ウィンドウ)							 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void AdminOnPaintCharItem(HDC hDC)
{
	HFONT hFontOld;
	PCHARINFO pChar = NULL;

	if (m_dwSelectID != -1) {
		pChar = g_aCharInfo[m_dwSelectID];
	}

	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);

	char szTmp[128];
	if (pChar) {
		wsprintf (szTmp, "[%s]の所持アイテム:", pChar->szName);
		MyTextOut (hDC, 12 * 1, 40, szTmp);
	}

	MyTextOut (hDC, 230, 48, "アイテム一覧:");
	MyTextOut (hDC, 230, 112, "ｽﾃｰﾀｽﾃﾝﾌﾟﾚｰﾄ一覧:");

	SelectObject (hDC, hFontOld);
}

/* ========================================================================= */
/* 関数名：AdminOnPaintSvrInfo												 */
/* 内容：WM_PAINT処理(サーバー情報ウィンドウ)								 */
/* 日付：2003/09/18															 */
/* ========================================================================= */
void AdminOnPaintSvrInfo(HDC hDC)
{
	HFONT hFontOld;

	if (m_pSvrInfo == NULL) {
		return;
	}

	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);

	char szTmp[128];
	wsprintf (szTmp, "サーバー稼動時間: %04d:%02d:%02d",
			 m_pSvrInfo->dwTime / 3600000,
			 (m_pSvrInfo->dwTime % 3600000 - ((m_pSvrInfo->dwTime % 60000) / 1000)) / 60000,
			 (m_pSvrInfo->dwTime % 60000) / 1000);
	MyTextOut (hDC, 12 * 1, 52 + 12 * 0, szTmp);

	wsprintf (szTmp, "処理キャラ数: %ld", m_pSvrInfo->dwCharCount);
	MyTextOut (hDC, 12 * 1, 52 + 12 * 1, szTmp);

	wsprintf (szTmp, "処理マップ数: %ld", m_pSvrInfo->dwMapCount);
	MyTextOut (hDC, 12 * 1, 52 + 12 * 2, szTmp);

	SelectObject (hDC, hFontOld);
}

/* ========================================================================= */
/* 関数名：AdminOnPaintShopInfo												 */
/* 内容：WM_PAINT処理(店情報ウィンドウ)										 */
/* 日付：2003/09/20															 */
/* ========================================================================= */
void AdminOnPaintShopInfo(HDC hDC)
{
	HFONT hFontOld;

	/* 店が無い？ */
	if (g_pMapInfo->byCountShop == 0) {
		return;
	}
	if (m_dwSelectID >= g_pMapInfo->byCountShop) {
		return;
	}

	PSHOPINFO pShop = &g_pMapInfo->pShopInfo[m_dwSelectID];

	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);

	char szTmp[128];
	wsprintf (szTmp, "%d/%d", m_dwSelectID + 1, g_pMapInfo->byCountShop);
	MyTextOut (hDC, 0, 0, szTmp);

	MyTextOut (hDC, 12 * 1, 32 + 14 * 0, "各状況で使用するメッセージ内容:");
	MyTextOut (hDC, 12 * 1, 32 + 14 * 9, "売り物一覧:");

	SelectObject (hDC, hFontOld);
}

/* ========================================================================= */
/* 関数名：AdminOnPaintEfcEdit												 */
/* 内容：WM_PAINT処理(エフェクト設定ウィンドウ)								 */
/* 日付：2003/09/23															 */
/* ========================================================================= */
void AdminOnPaintEfcEdit(HDC hDC)
{
	HFONT hFontOld;

	PEFCINFO pEfc = g_EfcInfo.GetPtr (m_dwSelectID);
	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);

	char szTmp[128];

	if (g_EfcInfo.GetCount () > 0) {
		wsprintf (szTmp, "%d/%d", m_dwSelectID + 1, g_EfcInfo.GetCount ());
	} else {
		wsprintf (szTmp, "0/0");
	}
	MyTextOut (hDC, 12 * 0, 12 * 0, szTmp);

	MyTextOut (hDC, 0, 12 * 3 + 4, "画像種別:");
	MyTextOut (hDC, 0, 12 * 5, "効果音:");
	MyTextOut (hDC, 0, 12 * 9, "画像ID:");
	MyTextOut (hDC, 0, 12 * 10 + 4, "待ち時間(10ms):");
	MyTextOut (hDC, 0, 12 * 11 + 8, "透明度:");

	if (pEfc && pEfc->aAnime.GetCount ()) {
		wsprintf (szTmp, "%d/%d", m_dwSelectID2 + 1, pEfc->aAnime.GetCount ());

	} else {
		wsprintf (szTmp, "0/0");
	}
	MyTextOut (hDC, 12 * 1, 12 * 7, szTmp);

	SelectObject (hDC, hFontOld);

	int i, xx, yy, pos = GetScrollPos (m_hWndScrBar1, SB_CTL);
	HDC hDCTmp;
	CImg32 *pDibTmp = new CImg32;
	pDibTmp->Create (32, 32);
	for (i = 0; i < 5; i ++) {

		xx = ((pos + i) % 8) * 32;

		if (pEfc->byType == 1) {
			yy = ((pos + i) / 8) % 8 * 64;
			if ((pos + i) / 64 > 0) {
				continue;
			}
			pDibTmp->Blt (0, 0, 32, 32, g_apDibEfect1632[(pos + i) / 64], xx, yy + 32);
		} else {
			yy = ((pos + i) / 8) % 16 * 32;
			pDibTmp->Blt (0, 0, 32, 32, g_apDibEfect[(pos + i) / 128], xx, yy);
		}
		hDCTmp = pDibTmp->Lock ();
		BitBlt (hDC, 12 * 17 + i * 32, 12 * 5 - 32, 32, 32, hDCTmp, 0, 0, SRCCOPY);
		pDibTmp->Unlock ();
	}
	delete pDibTmp;
}

/* ========================================================================= */
/* 関数名：AdminOnPaintSubGame												 */
/* 内容：WM_PAINT処理(サブゲーム情報ウィンドウ)								 */
/* 日付：2003/11/08															 */
/* ========================================================================= */
void AdminOnPaintSubGame(HDC hDC)
{
	HFONT hFontOld;
	char szBuf[64];

	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);

	wsprintf (szBuf, "サブゲーム登録数:%ld", g_nSubGameCount);
	MyTextOut (hDC, 0, 12 * 0, szBuf);
	wsprintf (szBuf, "%ld", g_nSelectSubGameID);
	MyTextOut (hDC, 0, 12 * 1, szBuf);

	if (g_pSubGameInfo == NULL) {
		return;
	}
	if (g_pSubGameInfo->pInfo == NULL) {
		return;
	}

	switch (g_pSubGameInfo->byType) {
	case SUBGAMEID_BOMB:
		MyTextOut (hDC, 0, 64 + 16 * 0, "マップID:");
		MyTextOut (hDC, 0, 64 + 16 * 1, "参加受付X座標:");
		MyTextOut (hDC, 0, 64 + 16 * 2, "参加受付Y座標:");
		MyTextOut (hDC, 0, 64 + 16 * 3, "参加開始X座標:");
		MyTextOut (hDC, 0, 64 + 16 * 4, "参加開始Y座標:");

		MyTextOut (hDC, 0, 64 + 16 * 5, "ゲームエリア:");
		MyTextOut (hDC, 12 * 10 - 8, 64 + 16 * 5, ",");
		MyTextOut (hDC, 12 * 13 - 8, 64 + 16 * 5, "-");
		MyTextOut (hDC, 12 * 16 - 8, 64 + 16 * 5, ",");

		MyTextOut (hDC, 12 * 12, 64 + 16 * 0, "座標チェックがONの場合、");
		MyTextOut (hDC, 12 * 12, 64 + 16 * 1, "左クリックで参加受付座標");
		MyTextOut (hDC, 12 * 12, 64 + 16 * 2, "右クリックで参加開始座標を取得。");
		break;

	case SUBGAMEID_MAZE:
		MyTextOut (hDC, 0, 64 + 16 * 0, "マップID:");
		MyTextOut (hDC, 0, 64 + 16 * 1, "ゲームエリア:");
		MyTextOut (hDC, 12 * 10 - 8, 64 + 16 * 1, ",");
		MyTextOut (hDC, 12 * 13 - 8, 64 + 16 * 1, "-");
		MyTextOut (hDC, 12 * 16 - 8, 64 + 16 * 1, ",");

		MyTextOut (hDC, 0, 64 + 16 * 2, "スタート地点:");
		MyTextOut (hDC, 12 * 10 - 8, 64 + 16 * 2, ",");

		MyTextOut (hDC, 0, 64 + 16 * 3, "ゴール地点:");
		MyTextOut (hDC, 12 * 10 - 8, 64 + 16 * 3, ",");

		MyTextOut (hDC, 0, 64 + 16 * 4, "開始地点ﾁｯﾌﾟ:");
		MyTextOut (hDC, 0, 64 + 16 * 5, "ｺﾞｰﾙ地点ﾁｯﾌﾟ:");
		MyTextOut (hDC, 0, 64 + 16 * 6, "道ﾁｯﾌﾟ:");
		MyTextOut (hDC, 0, 64 + 16 * 7, "壁ﾁｯﾌﾟ:");

		MyTextOut (hDC, 12 * 13, 64 + 16 * 3, "座標チェックがONの場合、");
		MyTextOut (hDC, 12 * 13, 64 + 16 * 4, "左クリックで開始座標");
		MyTextOut (hDC, 12 * 13, 64 + 16 * 5, "右クリックで終了座標を取得。");
		break;

	case SUBGAMEID_DUNGEON:
		MyTextOut (hDC, 0, 64 + 16 * 0, "マップID:");
		MyTextOut (hDC, 0, 64 + 16 * 1, "ゲームエリア:");
		MyTextOut (hDC, 12 * 10 - 8, 64 + 16 * 1, ",");
		MyTextOut (hDC, 12 * 13 - 8, 64 + 16 * 1, "-");
		MyTextOut (hDC, 12 * 16 - 8, 64 + 16 * 1, ",");

		MyTextOut (hDC, 0, 64 + 16 * 2, "スタート地点:");
		MyTextOut (hDC, 12 * 10 - 8, 64 + 16 * 2, ",");

		MyTextOut (hDC, 0, 64 + 16 * 3, "ゴール地点:");
		MyTextOut (hDC, 12 * 10 - 8, 64 + 16 * 3, ",");

		MyTextOut (hDC, 0, 64 + 16 * 4, "開始地点ﾁｯﾌﾟ:");
		MyTextOut (hDC, 0, 64 + 16 * 5, "ｺﾞｰﾙ地点ﾁｯﾌﾟ:");
		MyTextOut (hDC, 0, 64 + 16 * 6, "道ﾁｯﾌﾟ:");
		MyTextOut (hDC, 0, 64 + 16 * 7, "壁ﾁｯﾌﾟ:");

		MyTextOut (hDC, 12 * 13, 64 + 16 * 3, "座標チェックがONの場合、");
		MyTextOut (hDC, 12 * 13, 64 + 16 * 4, "左クリックで開始座標");
		MyTextOut (hDC, 12 * 13, 64 + 16 * 5, "右クリックで終了座標を取得。");
		break;
	}
}

/* ========================================================================= */
/* 関数名：AdminOnPaintEtcVersion											 */
/* 内容：WM_PAINT処理(バージョン通知)										 */
/* 日付：2004/12/24															 */
/* ========================================================================= */
void AdminOnPaintEtcVersion(HDC hDC)
{
	HFONT hFontOld;
	char szBuf[64];

	hFontOld = (HFONT)SelectObject (hDC, g_hFont12);
	SetBkMode (hDC, TRANSPARENT);

	wsprintf (szBuf, "鐘:%d", g_bySoundCount);
	MyTextOut (hDC, 12 * 13, 12 * 4, szBuf);
}

/* ========================================================================= */
/* 関数名：AdminOnHScrollMapParts											 */
/* 内容：WM_HSCROLL処理(マップパーツ編集ウィンドウ)							 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void AdminOnHScrollMapParts(HWND hWndCtl, UINT code, int pos)
{
	switch (code) {
	case SB_LINEUP:
		pos = GetScrollPos (hWndCtl, SB_CTL);
		pos --;
		break;

	case SB_LINEDOWN:
		pos = GetScrollPos (hWndCtl, SB_CTL);
		pos ++;
		break;

	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		break;

	default:
		return;
	}
	pos = min ((g_byMapGrpCountX * g_byMapGrpCountY * g_wCountMapPartsGrp) - 5, pos);
	pos = max (0, pos);

	if (hWndCtl == m_hWndMapPartsScrGrp) {
		SetScrollPos (hWndCtl, SB_CTL, pos, TRUE);
		InvalidateRect (g_hWndAdmin, NULL, TRUE);
	}
}

/* ========================================================================= */
/* 関数名：AdminOnVScrollMapParts											 */
/* 内容：WM_VSCROLL処理(マップパーツ編集ウィンドウ)							 */
/* 日付：2003/06/04															 */
/* ========================================================================= */
void AdminOnVScrollMapParts(HWND hWndCtl, UINT code, int pos)
{
	switch (code) {
	case SB_LINEUP:
		pos = GetScrollPos (hWndCtl, SB_CTL);
		pos --;
		break;

	case SB_LINEDOWN:
		pos = GetScrollPos (hWndCtl, SB_CTL);
		pos ++;
		break;

	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		break;

	default:
		return;
	}
	pos = min ((g_wCountMapParts / 10) - 3, pos);
	pos = max (0, pos);

	if (hWndCtl == m_hWndMapPartsScrParts) {
		SetScrollPos (hWndCtl, SB_CTL, pos, TRUE);
		InvalidateRect (g_hWndAdmin, NULL, TRUE);
	}
}

/* ========================================================================= */
/* 関数名：AdminOnHScrollAddChar											 */
/* 内容：WM_HSCROLL処理(キャラ追加ウィンドウ)								 */
/* 日付：2003/06/08															 */
/* ========================================================================= */
void AdminOnHScrollAddChar(HWND hWndCtl, UINT code, int pos)
{
	switch (code) {
	case SB_LINEUP:
		pos = GetScrollPos (hWndCtl, SB_CTL);
		pos --;
		break;

	case SB_LINEDOWN:
		pos = GetScrollPos (hWndCtl, SB_CTL);
		pos ++;
		break;

	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		break;

	default:
		return;
	}
	pos = min (g_wCountCharGrp * 16 - 5, pos);
	pos = max (0, pos);

	if (hWndCtl == m_hWndScrBar1) {
		SetScrollPos (hWndCtl, SB_CTL, pos, TRUE);
		InvalidateRect (g_hWndAdmin, NULL, TRUE);
	}
}

/* ========================================================================= */
/* 関数名：AdminOnHScrollCefEdit											 */
/* 内容：WM_HSCROLL処理(吹き出し設定ウィンドウ)								 */
/* 日付：2003/09/06															 */
/* ========================================================================= */
void AdminOnHScrollCefEdit(HWND hWndCtl, UINT code, int pos)
{
	switch (code) {
	case SB_LINEUP:
		pos = GetScrollPos (hWndCtl, SB_CTL);
		pos --;
		break;

	case SB_LINEDOWN:
		pos = GetScrollPos (hWndCtl, SB_CTL);
		pos ++;
		break;

	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		break;

	default:
		return;
	}
	pos = min (128 - 5, pos);
	pos = max (0, pos);

	if (hWndCtl == m_hWndScrBar1) {
		SetScrollPos (hWndCtl, SB_CTL, pos, TRUE);
		InvalidateRect (g_hWndAdmin, NULL, TRUE);
	}
}

/* ========================================================================= */
/* 関数名：AdminOnHScrollEfcEdit											 */
/* 内容：WM_HSCROLL処理(エフェクト設定ウィンドウ)							 */
/* 日付：2003/09/23															 */
/* ========================================================================= */
void AdminOnHScrollEfcEdit(HWND hWndCtl, UINT code, int pos)
{
	PEFCINFO pEfc = g_EfcInfo.GetPtr (m_dwSelectID);

	switch (code) {
	case SB_LINEUP:
		pos = GetScrollPos (hWndCtl, SB_CTL);
		pos --;
		break;

	case SB_LINEDOWN:
		pos = GetScrollPos (hWndCtl, SB_CTL);
		pos ++;
		break;

	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		break;

	default:
		return;
	}
	if (pEfc->byType == 1) {
		pos = min ((g_wCountEfcGrp1632 * 16 * 8) - 5, pos);
	} else {
		pos = min ((g_wCountEfcGrp * 16 * 8) - 5, pos);
	}
	pos = max (0, pos);

	if (hWndCtl == m_hWndScrBar1) {
		SetScrollPos (hWndCtl, SB_CTL, pos, TRUE);
		InvalidateRect (g_hWndAdmin, NULL, TRUE);
	}
}

/* ========================================================================= */
/* 関数名：AdminOnLButtonDownMapParts										 */
/* 内容：WM_LBUTTONDOWN処理(マップパーツ編集ウィンドウ)						 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void AdminOnLButtonDownMapParts(BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	RECT rcGrp;
	POINT ptPos;
	int no;

	ptPos.x = x;
	ptPos.y = y;
	ClientToScreen (m_hWndDialog, &ptPos);

	GetWindowRect (GetDlgItem (m_hWndDialog, IDC_PARTSGRP), &rcGrp);
	/* マップグラフィックが押された？ */
	if (PtInRect (&rcGrp, ptPos)) {
		PMAPPARTSINFO pParts = (PMAPPARTSINFO)&g_pMapPartsInfo[g_nSelectPartsID];

		if (pParts->byAnimeCount) {
			PANIMEINFO pAnime = &pParts->pAnimeInfo[g_byAdminAnimeNo];
			pAnime->wGrpID = (WORD)(GetScrollPos (m_hWndMapPartsScrGrp, SB_CTL) + (ptPos.x - rcGrp.left) / 32);
		} else {
			pParts->wGrpID = (WORD)(GetScrollPos (m_hWndMapPartsScrGrp, SB_CTL) + (ptPos.x - rcGrp.left) / 32);
		}
		InvalidateRect (g_hWndAdmin, NULL, TRUE);
	}

	GetWindowRect (GetDlgItem (m_hWndDialog, IDC_PARTS), &rcGrp);
	/* パーツが押された？ */
	if (PtInRect (&rcGrp, ptPos)) {
		no = (GetScrollPos (m_hWndMapPartsScrParts, SB_CTL) * 10) +
				((ptPos.x - rcGrp.left) / 32) + ((ptPos.y - rcGrp.top) / 32 * 10);
		if (no < g_wCountMapParts) {
			char szTmp[16] = {0};
			PMAPPARTSINFO pParts = (PMAPPARTSINFO)&g_pMapPartsInfo[no];

			g_nSelectPartsID = no;
			g_byAdminAnimeNo = 0;
			SendMessage (m_hWndCombo1, CB_SETCURSEL, pParts->byHitType, 0);
			if (pParts->byAnimeCount) {
				PANIMEINFO pAnime = &pParts->pAnimeInfo[g_byAdminAnimeNo];
				if (pAnime) {
					wsprintf (szTmp, "%d", pAnime->byWait);
					SetWindowText (m_hWndEdit1, szTmp);

					ZeroMemory (szTmp, sizeof (szTmp));
					wsprintf (szTmp, "%d", pAnime->byLevel);
					SetWindowText (m_hWndEdit2, szTmp);
				}
			} else {
				SetWindowText (m_hWndEdit1, szTmp);
				ZeroMemory (szTmp, sizeof (szTmp));
				wsprintf (szTmp, "%d", pParts->byLevel);
				SetWindowText (m_hWndEdit2, szTmp);
			}
			/* 灯りの範囲 */
			wsprintf (szTmp, "%d", pParts->byLight);
			SetWindowText (m_hWndEdit3, szTmp);

			InvalidateRect (g_hWndAdmin, NULL, TRUE);
		}
	}
}

/* ========================================================================= */
/* 関数名：AdminOnRButtonDownMapParts										 */
/* 内容：WM_RBUTTONDOWN処理(マップパーツ編集ウィンドウ)						 */
/* 日付：2003/06/04															 */
/* ========================================================================= */
void AdminOnRButtonDownMapParts(BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	RECT rcGrp;
	POINT ptPos;
	int nTmpX = 12 * 17, nTmpY = 12 * 5 - 32;

	ptPos.x = x;
	ptPos.y = y;
	ClientToScreen (m_hWndDialog, &ptPos);

	GetWindowRect (GetDlgItem (m_hWndDialog, IDC_PARTSGRP), &rcGrp);
	/* マップグラフィックが押された？ */
	if (PtInRect (&rcGrp, ptPos)) {
		PMAPPARTSINFO pParts = (PMAPPARTSINFO)&g_pMapPartsInfo[g_nSelectPartsID];

		if (pParts->byAnimeCount) {
			PANIMEINFO pAnime = &pParts->pAnimeInfo[g_byAdminAnimeNo];
			pAnime->wBackID = GetScrollPos (m_hWndMapPartsScrGrp, SB_CTL) + (x - nTmpX) / 32;
		} else {
			pParts->wBackID = GetScrollPos (m_hWndMapPartsScrGrp, SB_CTL) + (x - nTmpX) / 32;
		}
		InvalidateRect (g_hWndAdmin, NULL, TRUE);
	}
}

/* ========================================================================= */
/* 関数名：AdminOnLButtonDownAddChar										 */
/* 内容：WM_LBUTTONDOWN処理(キャラ追加ウィンドウ)							 */
/* 日付：2003/06/08															 */
/* ========================================================================= */
void AdminOnLButtonDownAddChar(BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	RECT rcGrp;
	POINT ptPos;
	int nTmpX = 12 * 17, nTmpY = 12 * 5 - 32;

	ptPos.x = x;
	ptPos.y = y;

	SetRect (&rcGrp, nTmpX, nTmpY, nTmpX + 32 * 5, nTmpY + 31);
	if (PtInRect (&rcGrp, ptPos)) {
		m_pAddCharInfo->wGrpID = GetScrollPos (m_hWndScrBar1, SB_CTL) + (x - nTmpX) / 32;
		InvalidateRect (g_hWndAdmin, NULL, TRUE);
	}
}

/* ========================================================================= */
/* 関数名：AdminOnLButtonDownCharInfo										 */
/* 内容：WM_LBUTTONDOWN処理(キャラ情報ウィンドウ)							 */
/* 日付：2003/06/10															 */
/* ========================================================================= */
void AdminOnLButtonDownCharInfo(BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	RECT rcGrp;
	POINT ptPos;
	int nTmpX = 12 * 17, nTmpY = 12 * 5 - 32;

	ptPos.x = x;
	ptPos.y = y;

	SetRect (&rcGrp, nTmpX, nTmpY, nTmpX + 32 * 5, nTmpY + 31);
	if (PtInRect (&rcGrp, ptPos)) {
		m_pParaCharInfo->wGrpID = GetScrollPos (m_hWndScrBar1, SB_CTL) + (x - nTmpX) / 32;
		InvalidateRect (g_hWndAdmin, NULL, TRUE);
	}
}

/* ========================================================================= */
/* 関数名：AdminOnLButtonDownCefEdit										 */
/* 内容：WM_LBUTTONDOWN処理(吹き出し設定ウィンドウ)							 */
/* 日付：2003/09/06															 */
/* ========================================================================= */
void AdminOnLButtonDownCefEdit(BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	RECT rcGrp;
	POINT ptPos;
	int nTmpX = 12 * 17, nTmpY = 12 * 5 - 32;

	ptPos.x = x;
	ptPos.y = y;

	SetRect (&rcGrp, nTmpX, nTmpY, nTmpX + 32 * 5, nTmpY + 31);
	if (PtInRect (&rcGrp, ptPos)) {
		PCOMEFCINFO pComEfc = &g_pComEfcInfo[m_dwSelectID];
		if (pComEfc->pAnime) {
			PCOMEFCANIMEINFO pAnime = &pComEfc->pAnime[m_dwSelectID2];
			pAnime->byGrpID = GetScrollPos (m_hWndScrBar1, SB_CTL) + (x - nTmpX) / 32;

			char szTmp[128];
			wsprintf (szTmp, "%d", pAnime->byGrpID);
			SetWindowText (m_hWndEdit2, szTmp);
		}
		InvalidateRect (g_hWndAdmin, NULL, TRUE);
	}
}

/* ========================================================================= */
/* 関数名：AdminOnLButtonDownEfcEdit										 */
/* 内容：WM_LBUTTONDOWN処理(エフェクト設定ウィンドウ)						 */
/* 日付：2003/09/23															 */
/* ========================================================================= */
void AdminOnLButtonDownEfcEdit(BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	RECT rcGrp;
	POINT ptPos;
	int nTmpX = 12 * 17, nTmpY = 12 * 5 - 32;

	ptPos.x = x;
	ptPos.y = y;

	SetRect (&rcGrp, nTmpX, nTmpY, nTmpX + 32 * 5, nTmpY + 31);
	if (PtInRect (&rcGrp, ptPos)) {
		PEFCANIMEINFO pAnime = g_EfcInfo.GetPtrAnime (m_dwSelectID, m_dwSelectID2);
		if (pAnime) {
			pAnime->wGrpID = GetScrollPos (m_hWndScrBar1, SB_CTL) + (x - nTmpX) / 32;

			char szTmp[128];
			wsprintf (szTmp, "%d", pAnime->wGrpID);
			SetWindowText (m_hWndEdit1, szTmp);
		}
		InvalidateRect (g_hWndAdmin, NULL, TRUE);
	}
}

/* ========================================================================= */
/* 関数名：AdminOnSelectCharGrpCharInfo										 */
/* 内容：WM_ADMINSELECTCHARGRP(キャラ情報ウィンドウ)						 */
/* 日付：2004/09/26															 */
/* ========================================================================= */
void AdminOnSelectCharGrpCharInfo(int nCode, int nMain, int nID)
{
	m_pParaCharInfo->wGrpMain = (WORD)nMain;
	m_pParaCharInfo->wGrpID = (WORD)nID;
	m_pParaCharInfo->wGrpSub = 0;
}

/* ========================================================================= */
/* 関数名：CreateWindowEdit													 */
/* 内容：エディットボックスウィンドウ作成									 */
/* 日付：2004/01/31															 */
/* ========================================================================= */
HWND CreateWindowEdit(int x, int y, int cx, int cy, DWORD dwStyle)
{
	HWND hWnd;

	hWnd = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | dwStyle,
			x, y, cx, cy, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (hWnd, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	return hWnd;
}

/* ========================================================================= */
/* 関数名：MakeAdminWnd														 */
/* 内容：各ウィンドウ作成													 */
/* 日付：2003/06/01															 */
/* ========================================================================= */
void MakeAdminWnd(int nType)
{
	if (g_nAdminWindowType == nType) {
		return;
	}

	g_bAdminPos = FALSE;
	DeleteAdminWnd ();
	AdminAutoLayoutResetScroll (g_hWndAdmin);

	switch (nType) {
	/* パーツ */
	case IDM_MAP_PARTS:
		if (!g_pMapPartsInfo) {
			break;
		}
		g_nAdminWindowType = IDM_MAP_PARTS;
		MakeAdminWndMapParts ();
		break;

	/* マップ情報 */
	case IDM_MAP_INFO:
		g_nAdminWindowType = IDM_MAP_INFO;
		MakeAdminWndMapInfo ();
		break;

	/* イベント情報 */
	case IDM_MAP_EVENT:
		g_nAdminWindowType = IDM_MAP_EVENT;
		MakeAdminWndEventInfo ();
		break;

	/* キャラ追加 */
	case IDM_CHAR_ADD:
		g_nAdminWindowType = IDM_CHAR_ADD;
		MakeAdminWndAddChar ();
		break;

	/* キャラ情報 */
	case IDM_CHAR_INFO:
		g_nAdminWindowType = IDM_CHAR_INFO;
		MakeAdminWndCharInfo ();
		break;

	/* キャラステータス情報 */
	case IDM_CHAR_STATUS:
		g_nAdminWindowType = IDM_CHAR_STATUS;
		MakeAdminWndCharStatus ();
		break;

	/* 預かり所情報 */
	case IDM_CHAR_LEAVE:
		g_nAdminWindowType = IDM_CHAR_LEAVE;
		MakeAdminWndCharLeave ();
		break;

	/* マップ追加 */
	case IDM_MAP_ADD:
		g_nAdminWindowType = IDM_MAP_ADD;
		m_hWndDialog = CreateDialog (g_hInstance, (LPCSTR)IDD_MAP_ADD, g_hWndAdmin, (DLGPROC)WndProcDialog);
		MakeAdminWndAddMap ();
		break;

	/* メッセージ情報 */
	case IDM_MAP_MSG:
		g_nAdminWindowType = IDM_MAP_MSG;
		MakeAdminWndMsg ();
		break;

	/* アカウント追加 */
	case IDM_ACC_ADD:
		g_nAdminWindowType = IDM_ACC_ADD;
		MakeAdminWndAccAdd ();
		break;

	/* アカウント編集 */
	case IDM_ACC_INFO:
		g_nAdminWindowType = IDM_ACC_INFO;
		MakeAdminWndAccInfo ();
		break;

	/* 吹き出し設定 */
	case IDM_CEF_EDIT:
		g_nAdminWindowType = IDM_CEF_EDIT;
		MakeAdminWndCefEdit ();
		break;

	/* バージョン通知 */
	case IDM_ETC_VERSION:
		g_nAdminWindowType = IDM_ETC_VERSION;
		MakeAdminWndEtcVersion ();
		break;

	/* アイテム一覧 */
	case IDM_ITEM_LIST:
		g_nAdminWindowType = IDM_ITEM_LIST;
		MakeAdminWndItemList ();
		break;

	/* 所持アイテム情報 */
	case IDM_CHAR_ITEM:
		g_nAdminWindowType = IDM_CHAR_ITEM;
		MakeAdminWndCharItem ();
		break;

	/* サーバー情報 */
	case IDM_ETC_SVRINFO:
		g_nAdminWindowType = IDM_ETC_SVRINFO;
		MakeAdminWndSvrInfo ();
		break;

	/* 店 */
	case IDM_MAP_SHOP:
		g_nAdminWindowType = IDM_MAP_SHOP;
		MakeAdminWndShopInfo ();
		break;

	/* エフェクト設定 */
	case IDM_EFC_EDIT:
		g_nAdminWindowType = IDM_EFC_EDIT;
		MakeAdminWndEfcEdit ();
		break;

	/* サブゲーム情報 */
	case IDM_ETC_SUBGAME:
		g_nAdminWindowType = IDM_ETC_SUBGAME;
		MakeAdminWndSubGameInfo ();
		break;
	}

	if (m_hWndDialog) {
		RECT rc;
		GetClientRect (g_hWndAdmin, &rc);
		SetWindowPos (m_hWndDialog, NULL, 0, 0, rc.right, rc.bottom, SWP_NOMOVE | SWP_NOZORDER);
		ShowWindow (m_hWndDialog, SW_SHOW);
	}
}

/* ========================================================================= */
/* 関数名：DeleteAdminWnd													 */
/* 内容：各ウィンドウ消去													 */
/* 日付：2003/06/01															 */
/* ========================================================================= */
void DeleteAdminWnd(void)
{
	SAFE_DESTROYWND (m_hWndDialog);
	DeleteAdminWndMapParts ();
	DeleteAdminWndMapInfo ();
	DeleteAdminWndEventInfo ();
	DeleteAdminWndAddChar ();
	DeleteAdminWndCharInfo ();
	DeleteAdminWndCharStatus ();
	DeleteAdminWndCharLeave ();
	DeleteAdminWndAddMap ();
	DeleteAdminWndMsg ();
	DeleteAdminWndAccAdd ();
	DeleteAdminWndAccInfo ();
	DeleteAdminWndCefEdit ();
	DeleteAdminWndEtcVersion ();
	DeleteAdminWndItemList ();
	DeleteAdminWndCharItem ();
	DeleteAdminWndSvrInfo ();
	DeleteAdminWndShopInfo ();
	DeleteAdminWndEfcEdit ();
	DeleteAdminWndSubGameInfo ();

	CloseDlgJobInfo ();
	CloseDlgMagicInfo ();
	CloseDlgMagicUserInfo ();
	CloseDlgCharMagicInfo ();
	CloseDlgEquUserInfo ();
	CloseDlgStTmpInfo ();
	CloseDlgAccSearch ();
	CloseDlgNpcRoute ();
	CloseDlgEtcSubGame ();
	g_bAdminPos = FALSE;
}

/* ========================================================================= */
/* 関数名：MakeAdminWndMapParts												 */
/* 内容：マップパーツ編集ウィンドウ作成										 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void MakeAdminWndMapParts(void)
{
	int i;
	LPCTSTR liststr[] = {"無し", "ぶつかる(音付き)", "ぶつかる(音無し)", "隠れる1",
			"隠れる2(ぶつかる)", "カウンター", "足跡(雪)", "足跡(砂)", "隠れる3", "隠れる4(ぶつかる)",
			"隠れる5(背景ごと隠れる)", "隠れる6(ぶつかる/音無し)", "一歩前へ進む", "一歩上へ進む",
			"一歩下へ進む", "一歩左へ進む", "一歩右へ進む", "水の波紋", "隠れる7(隠れる3で背景ごと隠れる)",
			"橋(縦に渡る。横切る人は下)", "橋(横に渡る。縦に通る人は下)"};

	m_hWndDialog = CreateDialog (g_hInstance, (LPCSTR)IDD_MAP_PARTS, g_hWndAdmin, (DLGPROC)WndProcDialog);
	m_hWndMapPartsAddParts = GetDlgItem (m_hWndDialog, IDC_BUTTON1);
	m_hWndMapPartsSendParts = GetDlgItem (m_hWndDialog, IDC_BUTTON2);
	m_hWndMapPartsReloadParts = GetDlgItem (m_hWndDialog, IDC_BUTTON4);
	m_hWndButton4 = GetDlgItem (m_hWndDialog, IDC_BUTTON3);
	m_hWndCombo1 = GetDlgItem (m_hWndDialog, IDC_COMBO1);
	m_hWndEdit1 = GetDlgItem (m_hWndDialog, IDC_EDIT1);
	m_hWndEdit2 = GetDlgItem (m_hWndDialog, IDC_EDIT2);
	m_hWndEdit3 = GetDlgItem (m_hWndDialog, IDC_EDIT3);		/* 灯りの範囲 */
	m_hWndButton1 = GetDlgItem (m_hWndDialog, IDC_BUTTON6);
	m_hWndButton2 = GetDlgItem (m_hWndDialog, IDC_BUTTON7);
	m_hWndButton3 = GetDlgItem (m_hWndDialog, IDC_BUTTON5);
	m_hWndMapPartsScrGrp = GetDlgItem (m_hWndDialog, IDC_SCRGRP);
	SetScrollRange (m_hWndMapPartsScrGrp, SB_CTL, 0, (g_byMapGrpCountX * g_byMapGrpCountY * g_wCountMapPartsGrp) - 5, FALSE);
	
	m_hWndMapPartsScrParts = GetDlgItem (m_hWndDialog, IDC_SCRPARTS);
	SetScrollRange (m_hWndMapPartsScrParts, SB_CTL, 0, max ((g_wCountMapParts / 10) - 3, 0), FALSE);

	for (i = 0; i < PARTSHIT_MAX; i ++) {
		SendMessage (m_hWndCombo1, CB_ADDSTRING, (WPARAM)0, (LPARAM)liststr[i]);
	}
	SendMessage (m_hWndCombo1, CB_SETCURSEL, 0, 0);
	SendMessage (GetDlgItem (m_hWndDialog, IDC_MARK), BM_SETCHECK, g_bMapMarkView, 0);
	g_byAdminAnimeNo = 0;

	InvalidateRect (g_hWndAdmin, NULL, TRUE);
	UpdateWindow (g_hWndAdmin);
}

/* ========================================================================= */
/* 関数名：DeleteAdminWndMapParts											 */
/* 内容：マップパーツ編集ウィンドウ消去										 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void DeleteAdminWndMapParts(void)
{
	InvalidateRect (g_hWndAdmin, NULL, TRUE);
}

/* ========================================================================= */
/* 関数名：MakeAdminWndMapInfo												 */
/* 内容：マップ情報ウィンドウ作成											 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void MakeAdminWndMapInfo(void)
{
	m_hWndMapAddUp = CreateWindow ("BUTTON", "追加", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15, 0, 32, 16, g_hWndAdmin, (HMENU)IDC_ADDMAPUP, g_hInstance, NULL);
	SendMessage (m_hWndMapAddUp, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndMapAddDown = CreateWindow ("BUTTON", "追加", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15, 32, 32, 16, g_hWndAdmin, (HMENU)IDC_ADDMAPDOWN, g_hInstance, NULL);
	SendMessage (m_hWndMapAddDown, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndMapAddLeft = CreateWindow ("BUTTON", "追加", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15 - 32, 16, 32, 16, g_hWndAdmin, (HMENU)IDC_ADDMAPLEFT, g_hInstance, NULL);
	SendMessage (m_hWndMapAddLeft, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndMapAddRight = CreateWindow ("BUTTON", "追加", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15 + 32, 16, 32, 16, g_hWndAdmin, (HMENU)IDC_ADDMAPRIGHT, g_hInstance, NULL);
	SendMessage (m_hWndMapAddRight, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndMapAddDelCount = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
			12 * 15, 16, 32, 16, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndMapAddDelCount, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCheck1 = CreateWindow ("BUTTON", "座標", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
			12 * 7, 0, 52, 16, g_hWndAdmin, (HMENU)IDC_TMPCHECK1, g_hInstance, NULL);
	SendMessage (m_hWndCheck1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton1 = CreateWindow ("BUTTON", "送信", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 7, 16, 52, 16, g_hWndAdmin, (HMENU)IDC_BUTTON1, g_hInstance, NULL);
	SendMessage (m_hWndButton1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCombo1 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			12 * 4, 12 * 5, 12 * 24, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
	SendMessage (m_hWndCombo1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCheck2 = CreateWindow ("BUTTON", "ｴﾌｪｸﾄ許可", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
			12 * 20, 16 * 6, 72, 16, g_hWndAdmin, (HMENU)IDC_TMPCHECK2, g_hInstance, NULL);
	SendMessage (m_hWndCheck2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCheck3 = CreateWindow ("BUTTON", "戦闘許可", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
			12 * 20, 16 * 7, 72, 16, g_hWndAdmin, (HMENU)IDC_TMPCHECK3, g_hInstance, NULL);
	SendMessage (m_hWndCheck3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCheck4 = CreateWindow ("BUTTON", "暗闇", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
			12 * 20, 16 * 8, 72, 16, g_hWndAdmin, (HMENU)IDC_TMPCHECK4, g_hInstance, NULL);
	SendMessage (m_hWndCheck4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCheck5 = CreateWindow ("BUTTON", "ﾏｯﾌﾟ内放送", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
			12 * 20, 16 * 9, 72, 16, g_hWndAdmin, (HMENU)IDC_TMPCHECK5, g_hInstance, NULL);
	SendMessage (m_hWndCheck5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCheck6 = CreateWindow ("BUTTON", "移動効果禁止", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
			12 * 20, 16 * 10, 72, 16, g_hWndAdmin, (HMENU)IDC_TMPCHECK6, g_hInstance, NULL);
	SendMessage (m_hWndCheck6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCheck7 = CreateWindow ("BUTTON", "魔法禁止", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
			12 * 20, 16 * 11, 72, 16, g_hWndAdmin, (HMENU)IDC_TMPCHECK7, g_hInstance, NULL);
	SendMessage (m_hWndCheck7, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCheck8 = CreateWindow ("BUTTON", "ｷｬﾗ灯り無し", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
			12 * 20, 16 * 12, 72, 16, g_hWndAdmin, (HMENU)IDC_TMPCHECK8, g_hInstance, NULL);
	SendMessage (m_hWndCheck8, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 色変換指定 */
	m_hWndEdit1 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
			12 * 25, 0, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEdit2 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
			12 * 25, 16, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEdit3 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
			12 * 25, 32, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 明るさレベル */
	m_hWndEdit5 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
			12 * 28, 16 * 8, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* マップ名入力欄 */
	m_hWndEdit4 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
			12, 12 * 8, 12 * 15, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 入り口座標入力欄 */
	m_hWndEdit6 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
			12 * 6, 12 * 10, 12 * 3, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEdit7 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
			12 * 6, 12 * 11 + 4, 12 * 3, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit7, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCombo2 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			12 * 3, 12 * 13, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO2, g_hInstance, NULL);
	SendMessage (m_hWndCombo2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 明るさの範囲・光源・見え方は小窓で設定する（項目が多いので） */
	m_hWndButton2 = CreateWindow ("BUTTON", "明るさ・灯り・見え方の設定...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			0, MAPINFO_LIGHTBTN_Y, 12 * 16, 20, g_hWndAdmin, (HMENU)IDC_BUTTON2, g_hInstance, NULL);
	SendMessage (m_hWndButton2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	char szTmp[10];
	wsprintf (szTmp, "%ld", g_pMapInfo->wBGM);
	SetWindowText (m_hWndEdit1, szTmp);

	LPCTSTR liststr1[] = {"無し", "戦闘１", "ボス１", "ボス２", "城１", "城２", "城３",
		"教会１", "教会２", "クライマックス", "フィールド１", "フィールド２",
		"フィールド３", "フィールド４", "フィールド５", "フィールド６",	"ラスボス",
		"追憶", "メニュー", "密談，潜入", "山", "ピンチ１", "ピンチ２",
		"神殿", "古代神殿", "塔１", "塔２", "塔３", "町１", "町１（夜）", "町２", "村",
		"Clock Down", "Half Hearted", "オルゴール７", "猫の日向ぼっこ", "Opening",
		"Organ Exacise", "散る時", "戸惑いの時", "オルゴール1", "Cream Bossa", "夏の夜祭",
		"迷いの森", "望郷", "飛空艇１", "メイン２", "雪の里","Bacation on the Beach",
		"Battle", "Dance to the Moon", "田園", "Destration of Death Across the Heaven",
		"Dreaming Stars", "火山洞窟", "不気味な工場地帯", "南の国", "おやすみ系03",
		"旅愁", "深々", "雪", "黄色い太陽", "NO BACK ANSWER", "農村", 
		"闘竜士　-竜騎士の余興-", "故郷を守る者達の戦い", "右手に敵、左手に援軍、前方には義勇軍",
		"阻止すべき悪の陰謀だとか", "真夏の熱さとか澄み渡る空の青さだとかあと期間限定で売店を出してるあのおじさんとかいろいろ",
		"変なお店の名物おじさん", "酒場", "深い森の村に住む小人の踊り", "とことん田舎町",
		"氷の世界に訪れた夜の空を迎える遺跡", "異教徒達の集会", "教会の見える丘",
		"素敵な和服", "暗闇、何かの気配、その恐怖", "Ｃ級ホラー", "さぁ行こう、あの地平線の彼方へ",
		"bunch of the armed enemy", "どうも災難続きで笑えてくる一日の始まり",
		"追跡、奴はまだこの町にいる", "業を背負う者達", "遊戯的戦闘風楽曲",
		"風吹き荒れる草原", "ＦＭ音源風", "戦場を駆ける銃士",
		"民族の誇りを", "倒壊までの残り僅かな時間", "いつもより素敵に見える景色",
		"小さな戦いの勝利者達", "ドラムロール（ティンパニ）", "緊迫した雰囲気",
		"のんびりと散歩をしながら", "きらきら星", "きよしこの夜", "散る時PartⅡ",
		"ハッピーバースデイ・トゥ・ユウ", "Jingle Bells", "正月風BGM", "（聖夜）清しこの夜",
		"ジャングル", "OMOIDE", "戦闘", "恐怖、昼時のバーゲンセール", "緑溢れる広場", "暖かな北の大地での暮らし",
		"気付かないうちに過ぎ行くもの", "協会の見える丘", "旅路に思う、置き去りにした日常と、これからのこと",
		"激しいオープニング？"
		/* 音声ファイル(.m4a)の曲は、common/BgmAudio.h の表の順に末尾へ並べる（番号は GetAdminBgmNo が表から引く） */
	}, liststr2[] = {"無し", "雪", "雨", "昼夜", "空き1", "z"};
	int i, nCount, nTmp;

	/* テーブル数を計算（MIDIの曲＋末尾に音声ファイルの曲） */
	nCount = GetAdminBgmCount ();

	/* 文字列の登録 */
	/* 末尾の BGM_AUDIO_COUNT 個は音声ファイルの曲（名前は common/BgmAudio.h の表から） */
	for (i = 0; i < nCount; i ++) {
		if (i < nCount - BGM_AUDIO_COUNT) {
			SendMessage (m_hWndCombo1, CB_ADDSTRING, (WPARAM)0, (LPARAM)liststr1[i]);
		} else {
			SendMessage (m_hWndCombo1, CB_ADDSTRING, (WPARAM)0, (LPARAM)g_aBgmAudio[i - (nCount - BGM_AUDIO_COUNT)].pszName);
		}
	}
	for (i = 0; ; i ++) {
		if (liststr2[i][0] == 'z') {
			break;
		}
		SendMessage (m_hWndCombo2, CB_ADDSTRING, (WPARAM)0, (LPARAM)liststr2[i]);
	}

	/* BGM番号をテーブル番号に変換 */
	for (i = 0; i < nCount; i ++) {
		nTmp = (int)GetAdminBgmNo (i);
		if (nTmp == g_pMapInfo->wBGM) {
			nTmp = i;
			break;
		}
	}
	/* 見つからなかった？ */
	if (i == nCount) {
		nTmp = 0;
	}
	SendMessage (m_hWndCombo1, CB_SETCURSEL, nTmp, 0);
	SendMessage (m_hWndCombo2, CB_SETCURSEL, g_pMapInfo->byWeather, 0);

	SendMessage (m_hWndCheck2, BM_SETCHECK, g_pMapInfo->byFlg & MAPFLG_EFECT, 0);
	SendMessage (m_hWndCheck3, BM_SETCHECK, g_pMapInfo->byFlg & MAPFLG_BATTLE, 0);
	SendMessage (m_hWndCheck4, BM_SETCHECK, g_pMapInfo->byFlg & MAPFLG_DARK, 0);
	SendMessage (m_hWndCheck5, BM_SETCHECK, g_pMapInfo->byFlg & MAPFLG_SPEECH, 0);
	SendMessage (m_hWndCheck6, BM_SETCHECK, g_pMapInfo->byFlg & MAPFLG_MOVE, 0);
	SendMessage (m_hWndCheck7, BM_SETCHECK, g_pMapInfo->byFlg & MAPFLG_MAGIC, 0);
	SendMessage (m_hWndCheck8, BM_SETCHECK, (g_pMapInfo->byFlg & MAPFLG_NOCHARLIGHT) ? BST_CHECKED : 0, 0);

	wsprintf (szTmp, "%d", GetRValue (g_pMapInfo->clChg));
	SetWindowText (m_hWndEdit1, szTmp);
	wsprintf (szTmp, "%d", GetGValue (g_pMapInfo->clChg));
	SetWindowText (m_hWndEdit2, szTmp);
	wsprintf (szTmp, "%d", GetBValue (g_pMapInfo->clChg));
	SetWindowText (m_hWndEdit3, szTmp);
	wsprintf (szTmp, "%d", g_pMapInfo->byLightLevel);
	SetWindowText (m_hWndEdit5, szTmp);
	wsprintf (szTmp, "%d", GetRValue (g_pMapInfo->nEntranceX));
	SetWindowText (m_hWndEdit6, szTmp);
	wsprintf (szTmp, "%d", GetRValue (g_pMapInfo->nEntranceY));
	SetWindowText (m_hWndEdit7, szTmp);

	if (g_pMapInfo->pszName) {
		SetWindowText (m_hWndEdit4, (LPSTR)g_pMapInfo->pszName);
	}

	AdminMapInfoRefresh ();
	InvalidateRect (g_hWndAdmin, NULL, TRUE);
}

/* ========================================================================= */
/* 関数名：AdminMapInfoContentHeight										 */
/* 内容：マップ情報画面の中身の高さ（一覧が長いとスクロールできるように）	 */
/* ========================================================================= */
static int AdminMapInfoContentHeight(void)
{
	int nArea, nLight;

	if (g_pMapInfo == NULL) {
		return 0;
	}
	nArea	= max (1, min ((int)g_pMapInfo->byDarkAreaCount, MAX_MAPDARKAREA));
	nLight	= max (1, min ((int)g_pMapInfo->byLightCount, MAX_MAPLIGHT));
	return MAPINFO_SUMMARY_Y + MAPINFO_SUMMARY_GAP + 12 + nArea * 12 + MAPINFO_SUMMARY_GAP + nLight * 12 + 8;
}

/* ========================================================================= */
/* 関数名：AdminMapInfoRefresh												 */
/* 内容：マップ情報画面の表示を直す（明るさ・灯り・見え方を変えた時など）	 */
/* ========================================================================= */
void AdminMapInfoRefresh(void)
{
	if ((g_hWndAdmin == NULL) || (g_nAdminWindowType != IDM_MAP_INFO)) {
		return;
	}
	AdminAutoLayoutSetContentHeight (g_hWndAdmin, AdminMapInfoContentHeight ());
	InvalidateRect (g_hWndAdmin, NULL, TRUE);
}

/* ========================================================================= */
/* 関数名：DeleteAdminWndMapInfo											 */
/* 内容：マップ情報ウィンドウ消去											 */
/* 日付：2003/05/31															 */
/* ========================================================================= */
void DeleteAdminWndMapInfo(void)
{
	/* 明るさ・灯り・見え方の設定を開いていたら閉じる（送っていない変更は送るか聞く） */
	CloseDlgMapLight ();
	SAFE_DESTROYWND (m_hWndButton2);
	SAFE_DESTROYWND (m_hWndMapAddUp);
	SAFE_DESTROYWND (m_hWndMapAddDown);
	SAFE_DESTROYWND (m_hWndMapAddLeft);
	SAFE_DESTROYWND (m_hWndMapAddRight);
	SAFE_DESTROYWND (m_hWndMapAddDelCount);
	SAFE_DESTROYWND (m_hWndCheck1);
	SAFE_DESTROYWND (m_hWndCheck2);
	SAFE_DESTROYWND (m_hWndCheck3);
	SAFE_DESTROYWND (m_hWndCheck4);
	SAFE_DESTROYWND (m_hWndCheck5);
	SAFE_DESTROYWND (m_hWndCheck6);
	SAFE_DESTROYWND (m_hWndCheck7);
	SAFE_DESTROYWND (m_hWndCheck8);
	SAFE_DESTROYWND (m_hWndButton1);
	SAFE_DESTROYWND (m_hWndCombo1);
	SAFE_DESTROYWND (m_hWndCombo2);
	SAFE_DESTROYWND (m_hWndEdit1);
	SAFE_DESTROYWND (m_hWndEdit2);
	SAFE_DESTROYWND (m_hWndEdit3);
	SAFE_DESTROYWND (m_hWndEdit4);
	SAFE_DESTROYWND (m_hWndEdit5);
	SAFE_DESTROYWND (m_hWndEdit6);
	SAFE_DESTROYWND (m_hWndEdit7);
	SAFE_DESTROYWND (m_hWndEdit8);
	SAFE_DESTROYWND (m_hWndEdit9);
	SAFE_DESTROYWND (m_hWndEdit10);
	SAFE_DESTROYWND (m_hWndEdit11);
	SAFE_DESTROYWND (m_hWndEdit12);
	SAFE_DESTROYWND (m_hWndEdit13);

	InvalidateRect (g_hWndAdmin, NULL, TRUE);
}


/* ========================================================================= */
/* 関数名：MakeAdminWndEventInfo											 */
/* 内容：イベント情報ウィンドウ作成											 */
/* 日付：2003/06/01															 */
/* ========================================================================= */
void MakeAdminWndEventInfo(void)
{
	m_hWndEventGet = CreateWindow ("BUTTON", "取得", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15 - 32, 0, 32, 16, g_hWndAdmin, (HMENU)IDC_GETEVENT, g_hInstance, NULL);
	SendMessage (m_hWndEventGet, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEventAdd = CreateWindow ("BUTTON", "追加", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15, 0, 32, 16, g_hWndAdmin, (HMENU)IDC_ADDEVENT, g_hInstance, NULL);
	SendMessage (m_hWndEventAdd, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEventSend = CreateWindow ("BUTTON", "送信", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15 + 32, 0, 32, 16, g_hWndAdmin, (HMENU)IDC_SENDEVENT, g_hInstance, NULL);
	SendMessage (m_hWndEventSend, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton3 = CreateWindow ("BUTTON", "削除", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15 + 96, 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON3, g_hInstance, NULL);
	SendMessage (m_hWndButton3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEventList = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			12 * 15 + 32, 32, 12 * 10, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
	SendMessage (m_hWndEventList, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCheck1 = CreateWindow ("BUTTON", "座標", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
			12 * 5, 16, 64, 16, g_hWndAdmin, (HMENU)IDC_TMPCHECK1, g_hInstance, NULL);
	SendMessage (m_hWndCheck1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton1 = CreateWindow ("BUTTON", "←", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15 - 32, 20, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON1, g_hInstance, NULL);
	SendMessage (m_hWndButton1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton2 = CreateWindow ("BUTTON", "→", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15, 20, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON2, g_hInstance, NULL);
	SendMessage (m_hWndButton2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	int i;
	LPCTSTR liststr[] = {"移動", "マップ移動", "メッセージ", "扉", "HP/Mana回復", "ステータス初期化", "キャラが乗っている間マップ変更"};

	for (i = 0; i < EVENTTYPE_MAX; i ++) {
		SendMessage (m_hWndEventList, CB_ADDSTRING, (WPARAM)0, (LPARAM)liststr[i]);
	}
	SendMessage (m_hWndEventList, CB_SETCURSEL, 0, 0);

	g_bAdminPos = TRUE;
	g_nEventCount = 0;
	g_nSelectEventID = 0;
	g_pEventInfo = NULL;
	InvalidateRect (g_hWndAdmin, NULL, TRUE);
	SendCmdEVENTGET (g_nSelectEventID);
}

/* ========================================================================= */
/* 関数名：DeleteAdminWndEventInfo											 */
/* 内容：イベント情報ウィンドウ消去											 */
/* 日付：2003/06/01															 */
/* ========================================================================= */
void DeleteAdminWndEventInfo(void)
{
	DeleteAdminWndEventInfoType ();
	SAFE_DESTROYWND (m_hWndEventGet);
	SAFE_DESTROYWND (m_hWndEventAdd);
	SAFE_DESTROYWND (m_hWndEventSend);
	SAFE_DESTROYWND (m_hWndEventList);
	SAFE_DESTROYWND (m_hWndCheck1);
	SAFE_DESTROYWND (m_hWndButton1);
	SAFE_DESTROYWND (m_hWndButton2);

	if (g_pEventInfo) {
		SAFE_DELETE_ARRAY (g_pEventInfo->pData);
		SAFE_DELETE (g_pEventInfo);
	}
	InvalidateRect (g_hWndAdmin, NULL, TRUE);
}

/* ========================================================================= */
/* 関数名：MakeAdminWndEventInfoType										 */
/* 内容：種類別イベント情報ウィンドウ作成									 */
/* 日付：2003/10/22															 */
/* ========================================================================= */
void MakeAdminWndEventInfoType(void)
{
	if (g_nEventCount == 0) {
		return;
	}

	char szTmp[64];
	DeleteAdminWndEventInfoType ();

	switch (g_pEventInfo->byType) {
	case EVENTTYPE_MOVE:		/* 座標変更 */
		{
			PEVENTINFO_MOVE pMove = (PEVENTINFO_MOVE)g_pEventInfo->pData;
			if (pMove == NULL) {
				break;
			}

			m_hWndEdit1 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 7, 64 + 16 * 0, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndEdit2 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 7, 64 + 16 * 1, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndCombo2 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
					12 * 7 - 2, 64 + 16 * 2, 12 * 10, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO2, g_hInstance, NULL);
			SendMessage (m_hWndCombo2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
			LPCTSTR liststr[] = {"音無し", "階段音", "落ちる", "波の音"};

			for (int i = 0; i < EVENTMOVEEFECT_MAX; i ++) {
				SendMessage (m_hWndCombo2, CB_ADDSTRING, (WPARAM)0, (LPARAM)liststr[i]);
			}

			wsprintf (szTmp, "%d", pMove->dwX);
			SetWindowText (m_hWndEdit1, szTmp);
			wsprintf (szTmp, "%d", pMove->dwY);
			SetWindowText (m_hWndEdit2, szTmp);
			SendMessage (m_hWndCombo2, CB_SETCURSEL, pMove->byEfect, 0);
		}
		break;

	case EVENTTYPE_MAPCHG:	/* マップ間移動 */
		{
			PEVENTINFO_MAPCHG pMap = (PEVENTINFO_MAPCHG)g_pEventInfo->pData;
			if (pMap == NULL) {
				break;
			}

			m_hWndEdit1 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 7, 64 + 16 * 0, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndEdit2 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 7, 64 + 16 * 1, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndEdit3 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 7, 64 + 16 * 2, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndCombo2 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
					12 * 7 - 2, 64 + 16 * 3, 12 * 10, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO2, g_hInstance, NULL);
			SendMessage (m_hWndCombo2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
			LPCTSTR liststr[] = {"音無し", "階段音", "落ちる", "波の音"};

			for (int i = 0; i < EVENTMOVEEFECT_MAX; i ++) {
				SendMessage (m_hWndCombo2, CB_ADDSTRING, (WPARAM)0, (LPARAM)liststr[i]);
			}

			wsprintf (szTmp, "%ld", pMap->dwMapID);
			SetWindowText (m_hWndEdit1, szTmp);
			wsprintf (szTmp, "%d", pMap->dwX);
			SetWindowText (m_hWndEdit2, szTmp);
			wsprintf (szTmp, "%d", pMap->dwY);
			SetWindowText (m_hWndEdit3, szTmp);
			SendMessage (m_hWndCombo2, CB_SETCURSEL, pMap->byEfect, 0);

			/* ばらける範囲（移動先と同じパーツのマスからくじ） */
			m_hWndEdit4 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 7, 64 + 16 * 5, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
			wsprintf (szTmp, "%d", pMap->byRandomRange);
			SetWindowText (m_hWndEdit4, szTmp);
		}
		break;

	case EVENTTYPE_MSG:	/* メッセージ */
		{
			PEVENTINFO_MSG pMsg = (PEVENTINFO_MSG)g_pEventInfo->pData;
			if (pMsg == NULL) {
				break;
			}

			m_hWndEdit1 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 7, 64 + 16 * 0, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndCombo2 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
					12 * 7 - 2, 64 + 16 * 1, 12 * 10, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO2, g_hInstance, NULL);
			SendMessage (m_hWndCombo2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
			LPCTSTR liststr[] = {"自動発動", "その場", "一歩前"};

			for (int i = 0; i < EVENTMSGTYPE_MAX; i ++) {
				SendMessage (m_hWndCombo2, CB_ADDSTRING, (WPARAM)0, (LPARAM)liststr[i]);
			}

			m_hWndEdit2 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 13, 64 + 16 * 3, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndEdit3 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 13, 64 + 16 * 4, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndEdit4 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 13, 64 + 16 * 5, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);


			wsprintf (szTmp, "%ld", pMsg->dwNo);
			SetWindowText (m_hWndEdit1, szTmp);
			SendMessage (m_hWndCombo2, CB_SETCURSEL, pMsg->byType, 0);
			wsprintf (szTmp, "%d", pMsg->wGrpID);
			SetWindowText (m_hWndEdit2, szTmp);
			wsprintf (szTmp, "%d", pMsg->dwX);
			SetWindowText (m_hWndEdit3, szTmp);
			wsprintf (szTmp, "%d", pMsg->dwY);
			SetWindowText (m_hWndEdit4, szTmp);
		}
		break;

	case EVENTTYPE_DOOR:	/* 扉 */
		{
			PEVENTINFO_DOOR pDoor = (PEVENTINFO_DOOR)g_pEventInfo->pData;
			if (pDoor == NULL) {
				break;
			}

			m_hWndEdit1 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 13, 64 + 16 * 0, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndEdit2 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 13, 64 + 16 * 1, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndEdit3 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 13, 64 + 16 * 2, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndCombo2 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
					12 * 13 - 2, 64 + 16 * 3, 12 * 10, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO2, g_hInstance, NULL);
			SendMessage (m_hWndCombo2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
			LPCTSTR liststr[] = {"カギ不要"};

			for (int i = 0; i < EVENTDOOTTYPE_MAX; i ++) {
				SendMessage (m_hWndCombo2, CB_ADDSTRING, (WPARAM)0, (LPARAM)liststr[i]);
			}

			wsprintf (szTmp, "%d", pDoor->wGrpID);
			SetWindowText (m_hWndEdit1, szTmp);
			wsprintf (szTmp, "%d", pDoor->wBackGrpID);
			SetWindowText (m_hWndEdit2, szTmp);
			wsprintf (szTmp, "%d", pDoor->byTimeClose);
			SetWindowText (m_hWndEdit3, szTmp);
			SendMessage (m_hWndCombo2, CB_SETCURSEL, pDoor->byType, 0);
		}
		break;

	case EVENTTYPE_HPMANA:	/* HP/Mana回復 */
		{
			PEVENTINFO_HPMANA pHpMana = (PEVENTINFO_HPMANA)g_pEventInfo->pData;
			if (pHpMana == NULL) {
				break;
			}

			m_hWndCombo2 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
					12 * 13 - 2, 64 + 16 * 0, 12 * 10, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO2, g_hInstance, NULL);
			SendMessage (m_hWndCombo2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
			LPCTSTR liststr[] = {"HP回復", "Mana回復", "HP/Mana回復"};

			for (int i = 0; i < EVENTHPMANATYPE_MAX; i ++) {
				SendMessage (m_hWndCombo2, CB_ADDSTRING, (WPARAM)0, (LPARAM)liststr[i]);
			}

			SendMessage (m_hWndCombo2, CB_SETCURSEL, pHpMana->byType, 0);
		}
		break;

	case EVENTTYPE_ONCHARMAP:	/* キャラが乗っている間マップ変更 */
		{
			PEVENTINFO_ONCHARMAP pInfo = (PEVENTINFO_ONCHARMAP)g_pEventInfo->pData;
			if (pInfo == NULL) {
				break;
			}

			m_hWndEdit1 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 13, 64 + 16 * 0, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndEdit2 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 13, 64 + 16 * 1, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndEdit3 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 13, 64 + 16 * 2, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndEdit4 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 13, 64 + 16 * 3, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			wsprintf (szTmp, "%d", pInfo->wGrpIDOn);
			SetWindowText (m_hWndEdit1, szTmp);
			wsprintf (szTmp, "%d", pInfo->wGrpIDOff);
			SetWindowText (m_hWndEdit2, szTmp);
			wsprintf (szTmp, "%d", pInfo->dwX);
			SetWindowText (m_hWndEdit3, szTmp);
			wsprintf (szTmp, "%d", pInfo->dwY);
			SetWindowText (m_hWndEdit4, szTmp);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：DeleteAdminWndEventInfoType										 */
/* 内容：種類別イベント情報ウィンドウ消去									 */
/* 日付：2003/10/22															 */
/* ========================================================================= */
void DeleteAdminWndEventInfoType(void)
{
	SAFE_DESTROYWND (m_hWndEdit1);
	SAFE_DESTROYWND (m_hWndEdit2);
	SAFE_DESTROYWND (m_hWndEdit3);
	SAFE_DESTROYWND (m_hWndEdit4);
	SAFE_DESTROYWND (m_hWndCombo2);
}

/* ========================================================================= */
/* 関数名：MakeAdminWndAddChar												 */
/* 内容：キャラ追加ウィンドウ作成											 */
/* 日付：2003/06/08															 */
/* ========================================================================= */
void MakeAdminWndAddChar(void)
{
	m_hWndButton1 = CreateWindow ("BUTTON", "送信", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 10 + 32, 12 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON1, g_hInstance, NULL);
	SendMessage (m_hWndButton1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndScrBar1 = CreateWindow ("SCROLLBAR", "", WS_CHILD | WS_VISIBLE | SBS_HORZ,
			12 * 17, 12 * 5, 32 * 5, 12, g_hWndAdmin, (HMENU)IDC_SCRBAR1, g_hInstance, NULL);
	SetScrollRange (m_hWndScrBar1, SB_CTL, 0, g_wCountCharGrp * 16 - 5, FALSE);

	m_hWndEdit1 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
			12 * 0, 12 * 7, 128, 16, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCombo1 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			12 * 0, 12 * 9, 12 * 8, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
	SendMessage (m_hWndCombo1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCombo2 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			12 * 0, 12 * 11, 12 * 8, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO2, g_hInstance, NULL);
	SendMessage (m_hWndCombo2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	int i;
	LPCTSTR liststr1[] = {"上", "下", "左", "右"};

	for (i = 0; i < 4; i ++) {
		SendMessage (m_hWndCombo1, CB_ADDSTRING, (WPARAM)0, (LPARAM)liststr1[i]);
	}
	SendMessage (m_hWndCombo1, CB_SETCURSEL, 0, 0);

	for (i = 0; i < NPCTYPE_MAX; i ++) {
		SendMessage (m_hWndCombo2, CB_ADDSTRING, (WPARAM)0, (LPARAM)aNpcMoveTypeTbl[i]);
	}
	SendMessage (m_hWndCombo2, CB_SETCURSEL, 0, 0);

	SAFE_DELETE (m_pAddCharInfo);
	m_pAddCharInfo = new CMDPARA_ADDCHARINFO;
	ZeroMemory (m_pAddCharInfo, sizeof (CMDPARA_ADDCHARINFO));
	m_pAddCharInfo->dwMapID = g_aCharInfo[g_nMyCharID]->dwMapID;
	g_bAdminPos = TRUE;
}

/* ========================================================================= */
/* 関数名：DeleteAdminWndAddChar											 */
/* 内容：キャラ追加ウィンドウ消去											 */
/* 日付：2003/06/08															 */
/* ========================================================================= */
void DeleteAdminWndAddChar(void)
{
	g_bAdminPos = FALSE;
	SAFE_DESTROYWND (m_hWndButton1);
	SAFE_DESTROYWND (m_hWndScrBar1);
	SAFE_DESTROYWND (m_hWndEdit1);
	SAFE_DESTROYWND (m_hWndCombo1);
	SAFE_DESTROYWND (m_hWndCombo2);

	SAFE_DELETE (m_pAddCharInfo);
}

/* ========================================================================= */
/* 関数名：MakeAdminWndCharInfo												 */
/* 内容：キャラ情報ウィンドウ作成											 */
/* 日付：2003/06/09															 */
/* ========================================================================= */
void MakeAdminWndCharInfo(void)
{
	m_hWndButton2 = CreateWindow ("BUTTON", "送信", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 10, 12 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON2, g_hInstance, NULL);
	SendMessage (m_hWndButton2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton13 = CreateWindow ("BUTTON", "取得", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 13, 12 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON13, g_hInstance, NULL);
	SendMessage (m_hWndButton13, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton1 = CreateWindow ("BUTTON", "BAN", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 18 + 64, 12 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON1, g_hInstance, NULL);
	SendMessage (m_hWndButton1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton9 = CreateWindow ("BUTTON", "削除", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 22 + 96, 12 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON9, g_hInstance, NULL);
	SendMessage (m_hWndButton9, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndScrBar1 = CreateWindow ("SCROLLBAR", "", WS_CHILD | WS_VISIBLE | SBS_HORZ,
			12 * 17, 12 * 5, 32 * 5, 12, g_hWndAdmin, (HMENU)IDC_SCRBAR1, g_hInstance, NULL);
	SetScrollRange (m_hWndScrBar1, SB_CTL, 0, g_wCountCharGrp * 16 - 5, FALSE);

	m_hWndButton14 = CreateWindow ("BUTTON", "画像", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 13, 12 * 2, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON16, g_hInstance, NULL);
	SendMessage (m_hWndButton14, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 向き */
	m_hWndCombo1 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			12 * 13, 12 * 7, 12 * 4, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
	SendMessage (m_hWndCombo1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* NPC移動種別 */
	m_hWndCombo2 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			12 * 13, 12 * 9, 12 * 8, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO2, g_hInstance, NULL);
	SendMessage (m_hWndCombo2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 管理者権限 */
	m_hWndCombo3 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			12 * 13, 12 * 11 + 64, 12 * 8, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO3, g_hInstance, NULL);
	SendMessage (m_hWndCombo3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEdit1 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
			12 * 0, 12 * 15, 128, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEdit2 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP | ES_NUMBER,
			12 * 13, 12 * 11, 60, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEdit3 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP | ES_NUMBER,
			12 * 13, 12 * 11 + 16, 60, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEdit4 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP | ES_NUMBER,
			12 * 13, 12 * 11 + 32, 60, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 開始マップID */
	m_hWndEdit16 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
			12 * 7, 12 * 7, 24, 11, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit16, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	/* 開始X座標 */
	m_hWndEdit17 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
			12 * 7, 12 * 8, 24, 11, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit17, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	/* 開始Y座標 */
	m_hWndEdit18 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
			12 * 7, 12 * 9, 24, 11, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit18, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	/* 移動確率X */
	m_hWndEdit19 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
			12 * 7, 12 * 10, 24, 11, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit19, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	/* 移動確率Y */
	m_hWndEdit20 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
			12 * 7, 12 * 11, 24, 11, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit20, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	/* 検索範囲X */
	m_hWndEdit21 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
			12 * 7, 12 * 12, 24, 11, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit21, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	/* 検索範囲Y */
	m_hWndEdit22 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
			12 * 7, 12 * 13, 24, 11, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit22, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* キャラの名前 */
	m_hWndEdit5 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
			12 * 5, 12 * 2, 72, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton3 = CreateWindow ("BUTTON", "ﾏｯﾌﾟ移動", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 13, 12 * 11 + 48, 64, 16, g_hWndAdmin, (HMENU)IDC_BUTTON3, g_hInstance, NULL);
	SendMessage (m_hWndButton3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 名前の色 */
	m_hWndCombo4 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			12 * 13, 12 * 13 + 64, 12 * 6, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO4, g_hInstance, NULL);
	SendMessage (m_hWndCombo4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* ルーラ用 */
	m_hWndButton4 = CreateWindow ("BUTTON", "←", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 22, 12 * 8, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON14, g_hInstance, NULL);
	SendMessage (m_hWndButton4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton5 = CreateWindow ("BUTTON", "→", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 25, 12 * 8, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON15, g_hInstance, NULL);
	SendMessage (m_hWndButton5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton6 = CreateWindow ("BUTTON", "GET", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 22, 12 * 8 + 16, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON6, g_hInstance, NULL);
	SendMessage (m_hWndButton6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton7 = CreateWindow ("BUTTON", "PUT", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 25, 12 * 8 + 16, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON7, g_hInstance, NULL);
	SendMessage (m_hWndButton7, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton8 = CreateWindow ("BUTTON", "ADD", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 28, 12 * 8 + 16, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON8, g_hInstance, NULL);
	SendMessage (m_hWndButton8, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEdit6 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
			12 * 22, 12 * 11, 24, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEdit7 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
			12 * 24 + 2, 12 * 11, 24, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit7, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEdit8 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
			12 * 26 + 4, 12 * 11, 24, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit8, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEdit9 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
			12 * 22, 12 * 12 + 2, 96, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit9, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* アカウント情報用 */
	m_hWndButton10 = CreateWindow ("BUTTON", "取得", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 22, 12 * 15 - 4, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON10, g_hInstance, NULL);
	SendMessage (m_hWndButton10, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton11 = CreateWindow ("BUTTON", "更新", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 25, 12 * 15 - 4, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON11, g_hInstance, NULL);
	SendMessage (m_hWndButton11, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEdit10 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
			12 * 22, 12 * 17, 100, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit10, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEdit11 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
			12 * 22, 12 * 19, 100, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit11, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* NPC発生用 */
	/* 移動範囲X */
	m_hWndEdit12 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
			12 * 7, 12 * 17, 24, 11, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit12, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 移動範囲Y */
	m_hWndEdit13 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
			12 * 7, 12 * 18, 24, 11, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit13, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 発生数 */
	m_hWndEdit14 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
			12 * 7, 12 * 19, 24, 11, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit14, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 発生周期 */
	m_hWndEdit15 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
			12 * 7, 12 * 20, 24, 11, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit15, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	
	m_hWndCombo5 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			12 * 0, 12 * 20 + 18, 12 * 10, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO5, g_hInstance, NULL);
	SendMessage (m_hWndCombo5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	int i;
	LPCTSTR liststr1[] = {"上", "下", "左", "右"},
			liststr3[] = {"一般", "全て許可"},
			liststr4[] = {"赤", "黄", "緑", "青緑", "青", "紫", "白",
						  "(濃)赤", "(濃)橙", "(濃)青", "(濃)紫", "(濃)黄緑", "(濃)緑", "(濃)水",
						  "z"};

	/* 向き */
	for (i = 0; i < 4; i ++) {
		SendMessage (m_hWndCombo1, CB_ADDSTRING, (WPARAM)0, (LPARAM)liststr1[i]);
	}
	SendMessage (m_hWndCombo1, CB_SETCURSEL, 0, 0);

	/* NPC種別 */
	for (i = 0; i < NPCTYPE_MAX; i ++) {
		SendMessage (m_hWndCombo2, CB_ADDSTRING, (WPARAM)0, (LPARAM)aNpcMoveTypeTbl[i]);
	}
	SendMessage (m_hWndCombo2, CB_SETCURSEL, 0, 0);

	/* 管理権限 */
	for (i = 0; i < 2; i ++) {
		SendMessage (m_hWndCombo3, CB_ADDSTRING, (WPARAM)0, (LPARAM)liststr3[i]);
	}
	SendMessage (m_hWndCombo3, CB_SETCURSEL, 0, 0);

	/* 名前と発言色 */
	for (i = 0; ;i ++) {
		if (liststr4[i][0] == 'z') {
			break;
		}
		SendMessage (m_hWndCombo4, CB_ADDSTRING, (WPARAM)0, (LPARAM)liststr4[i]);
	}
	SendMessage (m_hWndCombo4, CB_SETCURSEL, 0, 0);

	/* ステータステンプレート情報 */
	SendMessage (m_hWndCombo5, CB_ADDSTRING, (WPARAM)0, (LPARAM)"未設定");
	for (i = 0; i < g_aStTmpInfo.GetSize (); i ++) {
		SendMessage (m_hWndCombo5, CB_ADDSTRING, (WPARAM)i + 1, (LPARAM)g_aStTmpInfo[i]->szName);
	}
	SendMessage (m_hWndCombo5, CB_SETCURSEL, 0, 0);

	SAFE_DELETE (m_pParaCharInfo);
	m_pParaCharInfo = new CMDPARA_CHARINFO;
	ZeroMemory (m_pParaCharInfo, sizeof (CMDPARA_CHARINFO));
	g_bAdminPos = TRUE;
	g_dwSelctRura = 0;

	char szTmp[20];
	PADMINRURAINFO pInfo = NULL;
	pInfo = g_AdminRuraInfo[g_dwSelctRura];
	wsprintf (szTmp, "%ld", pInfo->dwMapID);
	SetWindowText (m_hWndEdit6, szTmp);

	wsprintf (szTmp, "%ld", pInfo->dwX);
	SetWindowText (m_hWndEdit7, szTmp);

	wsprintf (szTmp, "%ld", pInfo->dwY);
	SetWindowText (m_hWndEdit8, szTmp);

	SetWindowText (m_hWndEdit9, pInfo->szComment);
}

/* ========================================================================= */
/* 関数名：DeleteAdminWndCharInfo											 */
/* 内容：キャラ情報ウィンドウ消去											 */
/* 日付：2003/06/08															 */
/* ========================================================================= */
void DeleteAdminWndCharInfo(void)
{
	CloseSelectCharGrpWindow ();

	SAFE_DESTROYWND (m_hWndButton1);
	SAFE_DESTROYWND (m_hWndButton2);
	SAFE_DESTROYWND (m_hWndButton3);
	SAFE_DESTROYWND (m_hWndButton4);
	SAFE_DESTROYWND (m_hWndButton5);
	SAFE_DESTROYWND (m_hWndButton6);
	SAFE_DESTROYWND (m_hWndButton7);
	SAFE_DESTROYWND (m_hWndButton8);
	SAFE_DESTROYWND (m_hWndButton9);
	SAFE_DESTROYWND (m_hWndButton10);
	SAFE_DESTROYWND (m_hWndButton11);
	SAFE_DESTROYWND (m_hWndButton13);
	SAFE_DESTROYWND (m_hWndButton14);
	SAFE_DESTROYWND (m_hWndScrBar1);
	SAFE_DESTROYWND (m_hWndCombo1);
	SAFE_DESTROYWND (m_hWndCombo2);
	SAFE_DESTROYWND (m_hWndCombo3);
	SAFE_DESTROYWND (m_hWndCombo4);
	SAFE_DESTROYWND (m_hWndCombo5);
	SAFE_DESTROYWND (m_hWndEdit1);
	SAFE_DESTROYWND (m_hWndEdit2);
	SAFE_DESTROYWND (m_hWndEdit3);
	SAFE_DESTROYWND (m_hWndEdit4);
	SAFE_DESTROYWND (m_hWndEdit5);
	SAFE_DESTROYWND (m_hWndEdit6);
	SAFE_DESTROYWND (m_hWndEdit7);
	SAFE_DESTROYWND (m_hWndEdit8);
	SAFE_DESTROYWND (m_hWndEdit9);
	SAFE_DESTROYWND (m_hWndEdit10);
	SAFE_DESTROYWND (m_hWndEdit11);
	SAFE_DESTROYWND (m_hWndEdit12);
	SAFE_DESTROYWND (m_hWndEdit13);
	SAFE_DESTROYWND (m_hWndEdit14);
	SAFE_DESTROYWND (m_hWndEdit15);
	SAFE_DESTROYWND (m_hWndEdit16);
	SAFE_DESTROYWND (m_hWndEdit17);
	SAFE_DESTROYWND (m_hWndEdit18);
	SAFE_DESTROYWND (m_hWndEdit19);
	SAFE_DESTROYWND (m_hWndEdit20);
	SAFE_DESTROYWND (m_hWndEdit21);
	SAFE_DESTROYWND (m_hWndEdit22);

	g_bAdminPos = FALSE;
	if (m_pParaCharInfo) {
		SAFE_DELETE_ARRAY (m_pParaCharInfo->pszNpcSpeech);
		SAFE_DELETE (m_pParaCharInfo);
	}
}

/* ========================================================================= */
/* 関数名：MakeAdminWndCharStatus											 */
/* 内容：キャラステータス情報ウィンドウ作成									 */
/* 日付：2003/09/30															 */
/* ========================================================================= */
void MakeAdminWndCharStatus(void)
{
	HWND *ahWndTbl[12] = {
		&m_hWndEdit1,
		&m_hWndEdit2,
		&m_hWndEdit3,
		&m_hWndEdit4,
		&m_hWndEdit5,
		&m_hWndEdit6,
		&m_hWndEdit7,
		&m_hWndEdit8,
		&m_hWndEdit9,
		&m_hWndEdit10,
		&m_hWndEdit12,
		&m_hWndEdit24
	},
	*ahWndTbl2[6] = {
		&m_hWndEdit17,
		&m_hWndEdit18,
		&m_hWndEdit19,
		&m_hWndEdit20,
		&m_hWndEdit21,
		&m_hWndEdit22
	};
	int i, x, cx;
	for (i = 0; i < 12; i ++) {
		x = 12 * 6 + (i % 3) * 100 - 3;
		cx = 32;
		if (ahWndTbl[i] == &m_hWndEdit10) {
			x -= 8;
			cx += 8;
		}
		*ahWndTbl[i] = CreateWindowEdit (x, 52 + (i / 3) * 15, cx, 14, 0);
	}
	for (i = 0; i < 6; i ++) {
		x = 12 * 3 + i * 50 - 5;
		cx = 28;
		*ahWndTbl2[i] = CreateWindowEdit (x, 52 + 15 * 4, cx, 14, 0);
	}
	m_hWndEdit11 = CreateWindowEdit (12 * 1, 34, 12 * 10, 14, ES_AUTOHSCROLL);
	m_hWndEdit13 = CreateWindowEdit (12 * 6 + 300 - 3 - 10, 34 + 12 * 0, 24, 11, ES_AUTOHSCROLL);
	m_hWndEdit14 = CreateWindowEdit (12 * 6 + 300 - 3 - 10, 34 + 12 * 1, 24, 11, ES_AUTOHSCROLL);
	m_hWndEdit15 = CreateWindowEdit (12 * 6 + 300 - 3 - 10, 34 + 12 * 2, 24, 11, ES_AUTOHSCROLL);
	m_hWndEdit16 = CreateWindowEdit (12 * 6 + 300 - 3 - 10, 34 + 12 * 3, 24, 11, ES_AUTOHSCROLL);
	/* 魔法防御 */
	m_hWndEdit23 = CreateWindowEdit (12 * 6 - 3, 52 + 15 * 5, 32, 14, 0);
	/* 経験値 */
	m_hWndEdit25 = CreateWindowEdit (12 * 6 + 100 - 3, 52 + 15 * 5, 32, 14, ES_AUTOHSCROLL);

	m_hWndButton1 = CreateWindow ("BUTTON", "送信", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 0, 1, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON1, g_hInstance, NULL);
	SendMessage (m_hWndButton1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 性別 */
	m_hWndCombo1 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			12 * 4 - 3, 15 * 6 + 52, 12 * 3, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
	SendMessage (m_hWndCombo1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	SendMessage (m_hWndCombo1, CB_ADDSTRING, (WPARAM)0, (LPARAM)"男");
	SendMessage (m_hWndCombo1, CB_ADDSTRING, (WPARAM)0, (LPARAM)"女");
	SendMessage (m_hWndCombo1, CB_SETCURSEL, 0, 0);

	/* 移動種別 */
	m_hWndCombo2 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			12 * 4 - 3 + 100, 15 * 6 + 52, 12 * 10, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO2, g_hInstance, NULL);
	SendMessage (m_hWndCombo2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	LPSTR liststr1[] = {"移動種別", "力", "素早さ", "体力", "賢さ", "運のよさ", "最大HP",
						"最大Mana", "所持金", "ドロップ率", "移動確率X", "移動確率Y",
						"検索範囲X", "検索範囲Y", "火", "水", "風", "土", "光", "闇", "魔法防御", "レベル", "経験値", "z"};
	for (i = 0; i < NPCTYPE_MAX; i ++) {
		SendMessage (m_hWndCombo2, CB_ADDSTRING, (WPARAM)0, (LPARAM)aNpcMoveTypeTbl[i]);
	}
	SendMessage (m_hWndCombo2, CB_SETCURSEL, 0, 0);

	/* 酔っ払いレベル */
	m_hWndCombo3 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			12 * 18 - 3 + 100, 15 * 6 + 52, 12 * 5, 12 * 7, g_hWndAdmin, (HMENU)IDC_TMPCOMBO3, g_hInstance, NULL);
	SendMessage (m_hWndCombo3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	for (i = 0; i < CHARSTATEDRUNK_MAX; i ++) {
		SendMessage (m_hWndCombo3, CB_ADDSTRING, (WPARAM)0, (LPARAM)aDrunkTypeTbl[i]);
	}
	SendMessage (m_hWndCombo3, CB_SETCURSEL, 0, 0);

	LV_COLUMN lvcol;
	m_hWndList1 = CreateWindowEx (WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
			WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS,
			12, 15 * 9 + 52, 350, 104, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndList1, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
	ListView_SetExtendedListViewStyle (m_hWndList1 , LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	m_hWndButton2 = CreateWindow ("BUTTON", "↓更新", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15 + 16, 15 * 9 + 50 - 16, 48, 18, g_hWndAdmin, (HMENU)IDC_BUTTON2, g_hInstance, NULL);
	SendMessage (m_hWndButton2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton4 = CreateWindow ("BUTTON", "↑反映", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15 + 64, 15 * 9 + 50 - 16, 48, 18, g_hWndAdmin, (HMENU)IDC_BUTTON4, g_hInstance, NULL);
	SendMessage (m_hWndButton4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton3 = CreateWindow ("BUTTON", "↓追加", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15 + 128, 15 * 9 + 50 - 16, 48, 18, g_hWndAdmin, (HMENU)IDC_BUTTON3, g_hInstance, NULL);
	SendMessage (m_hWndButton3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	ZeroMemory (&lvcol, sizeof (lvcol));
	lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol.fmt = LVCFMT_LEFT;
	lvcol.cx = 80;
	lvcol.pszText = "名前";
	ListView_InsertColumn (m_hWndList1, 0, &lvcol);

	for (i = 0; ; i ++) {
		if (liststr1[i][0] == 'z') {
			break;
		}
		lvcol.cx = 60;
		lvcol.pszText = liststr1[i];
		ListView_InsertColumn (m_hWndList1, i + 1, &lvcol);
	}

	/* ステータステンプレートの設定 */
	for (i = 0; i < g_aStTmpInfo.GetSize (); i ++) {
		PSTATUSTEMPLATEINFO pInfo = g_aStTmpInfo[i];
		LV_ITEM item;
		ZeroMemory (&item, sizeof (item));
		item.mask = LVIF_TEXT;
		item.iItem = i;
		char szTmp[MAX_CHARNAMEBUF];

		/* 名前 */
		ZeroMemory (szTmp, sizeof (szTmp));
		item.iSubItem = 0;
		item.pszText = pInfo->szName;
		ListView_InsertItem (m_hWndList1, &item);
		/* 移動種別 */
		ZeroMemory (szTmp, sizeof (szTmp));
		item.iSubItem = 1;
		item.pszText = (LPSTR)aNpcMoveTypeTbl[pInfo->byNpcType];
		ListView_SetItem (m_hWndList1, &item);
		/* 力 */
		wsprintf (szTmp, "%d", pInfo->byPW);
		item.iSubItem = 2;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* 素早さ */
		wsprintf (szTmp, "%d", pInfo->byDEX);
		item.iSubItem = 3;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* 体力 */
		wsprintf (szTmp, "%d", pInfo->byST);
		item.iSubItem = 4;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* 賢さ */
		wsprintf (szTmp, "%d", pInfo->byIT);
		item.iSubItem = 5;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* 運のよさ */
		wsprintf (szTmp, "%d", pInfo->byLK);
		item.iSubItem = 6;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* 最大HP */
		wsprintf (szTmp, "%d", pInfo->wMaxHP);
		item.iSubItem = 7;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* 最大Mana */
		wsprintf (szTmp, "%d", pInfo->wMaxMana);
		item.iSubItem = 8;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* 所持金 */
		wsprintf (szTmp, "%d", pInfo->dwGold);
		item.iSubItem = 9;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* ドロップ率 */
		wsprintf (szTmp, "%d", pInfo->byDropProbability);
		item.iSubItem = 10;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* 移動確率X */
		wsprintf (szTmp, "%d", pInfo->byMoveProbabilityX);
		item.iSubItem = 11;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* 移動確率Y */
		wsprintf (szTmp, "%d", pInfo->byMoveProbabilityY);
		item.iSubItem = 12;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* 検索範囲X */
		wsprintf (szTmp, "%d", pInfo->bySearchCx);
		item.iSubItem = 13;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* 検索範囲Y */
		wsprintf (szTmp, "%d", pInfo->bySearchCy);
		item.iSubItem = 14;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* 火 */
		wsprintf (szTmp, "%d", pInfo->chAttrFire);
		item.iSubItem = 15;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* 水 */
		wsprintf (szTmp, "%d", pInfo->chAttrWater);
		item.iSubItem = 16;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* 風 */
		wsprintf (szTmp, "%d", pInfo->chAttrWind);
		item.iSubItem = 17;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* 土 */
		wsprintf (szTmp, "%d", pInfo->chAttrEarth);
		item.iSubItem = 18;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* 光 */
		wsprintf (szTmp, "%d", pInfo->chAttrLight);
		item.iSubItem = 19;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* 闇 */
		wsprintf (szTmp, "%d", pInfo->chAttrDark);
		item.iSubItem = 20;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* 魔法防御 */
		wsprintf (szTmp, "%d", pInfo->wDFMagic);
		item.iSubItem = 21;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* レベル */
		wsprintf (szTmp, "%d", pInfo->byLevel);
		item.iSubItem = 22;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
		/* 経験値 */
		wsprintf (szTmp, "%u", pInfo->dwExp);
		item.iSubItem = 23;
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
	}

	g_bAdminPos = TRUE;
	m_dwSelectID = -1;
	m_dwSelectpID = -1;
}

/* ========================================================================= */
/* 関数名：DeleteAdminWndCharStatus											 */
/* 内容：キャラステータス情報ウィンドウ消去									 */
/* 日付：2003/09/30															 */
/* ========================================================================= */
void DeleteAdminWndCharStatus(void)
{
	HWND *ahWndTbl[11] = {
		&m_hWndEdit1,
		&m_hWndEdit2,
		&m_hWndEdit3,
		&m_hWndEdit4,
		&m_hWndEdit5,
		&m_hWndEdit6,
		&m_hWndEdit7,
		&m_hWndEdit8,
		&m_hWndEdit9,
		&m_hWndEdit10,
		&m_hWndEdit12
	};
	for (int i = 0; i < 11; i ++) {
		SAFE_DESTROYWND (*ahWndTbl[i]);
	}
	SAFE_DESTROYWND (m_hWndEdit11);
	SAFE_DESTROYWND (m_hWndEdit13);
	SAFE_DESTROYWND (m_hWndEdit14);
	SAFE_DESTROYWND (m_hWndEdit15);
	SAFE_DESTROYWND (m_hWndEdit16);
	SAFE_DESTROYWND (m_hWndEdit17);
	SAFE_DESTROYWND (m_hWndEdit18);
	SAFE_DESTROYWND (m_hWndEdit19);
	SAFE_DESTROYWND (m_hWndEdit20);
	SAFE_DESTROYWND (m_hWndEdit21);
	SAFE_DESTROYWND (m_hWndEdit22);
	SAFE_DESTROYWND (m_hWndEdit23);
	SAFE_DESTROYWND (m_hWndEdit24);
	SAFE_DESTROYWND (m_hWndEdit25);
	SAFE_DESTROYWND (m_hWndButton1);
	SAFE_DESTROYWND (m_hWndButton2);
	SAFE_DESTROYWND (m_hWndButton3);
	SAFE_DESTROYWND (m_hWndButton4);
	SAFE_DESTROYWND (m_hWndCombo1);
	SAFE_DESTROYWND (m_hWndCombo2);
	SAFE_DESTROYWND (m_hWndCombo3);
	SAFE_DESTROYWND (m_hWndList1);

	g_bAdminPos = FALSE;
	m_dwSelectID = 0;
	m_dwSelectpID = 0;
}

/* ========================================================================= */
/* 関数名：MakeAdminWndCharLeave											 */
/* 内容：預かり所情報ウィンドウ作成											 */
/* 日付：2004/02/07															 */
/* ========================================================================= */
void MakeAdminWndCharLeave(void)
{
	m_hWndButton1 = CreateWindow ("BUTTON", "取得", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			0, 1, 36, 18, g_hWndAdmin, (HMENU)IDC_BUTTON1, g_hInstance, NULL);
	SendMessage (m_hWndButton1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton2 = CreateWindow ("BUTTON", "送信", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			40, 1, 36, 18, g_hWndAdmin, (HMENU)IDC_BUTTON2, g_hInstance, NULL);
	SendMessage (m_hWndButton2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton3 = CreateWindow ("BUTTON", "更新", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			238 + 16 * 3, 88 + 18, 36, 18, g_hWndAdmin, (HMENU)IDC_BUTTON3, g_hInstance, NULL);
	SendMessage (m_hWndButton3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton4 = CreateWindow ("BUTTON", "削除", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			238 + 16 * 0, 88 + 18, 36, 18, g_hWndAdmin, (HMENU)IDC_BUTTON4, g_hInstance, NULL);
	SendMessage (m_hWndButton4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton5 = CreateWindow ("BUTTON", "追加", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			238 + 16 * 6, 88 + 18, 36, 18, g_hWndAdmin, (HMENU)IDC_BUTTON5, g_hInstance, NULL);
	SendMessage (m_hWndButton5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* キャラテーブルID */
	m_hWndEdit1 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
			12 * 1, 32, 36, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 個数 */
	m_hWndEdit2 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
			238 + 16 * 1, 88, 24, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 金額 */
	m_hWndEdit3 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
			230, 32, 50, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndList1 = CreateWindowEx (WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
			WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
			12, 60, 200, 12 * 13, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndList1, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
	ListView_SetExtendedListViewStyle (m_hWndList1 , LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	LV_COLUMN lvcol;
	ZeroMemory (&lvcol, sizeof (lvcol));
	lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol.fmt = LVCFMT_LEFT;

	lvcol.cx = 130;
	lvcol.pszText = "アイテム";
	ListView_InsertColumn (m_hWndList1, 0, &lvcol);

	lvcol.cx = 40;
	lvcol.pszText = "個数";
	ListView_InsertColumn (m_hWndList1, 1, &lvcol);

	/* アイテム名一覧 */
	m_hWndCombo1 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			230, 60, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
	SendMessage (m_hWndCombo1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	int i;
	for (i = 0; i < g_aItemInfo.GetSize (); i ++) {
		SendMessage (m_hWndCombo1, CB_ADDSTRING, (WPARAM)i, (LPARAM)g_aItemInfo[i]->szName);
	}

	g_bAdminPos = TRUE;
	m_dwSelectID = -1;
	m_pCharInfo = new CHARINFO;
	ZeroMemory (m_pCharInfo, sizeof (CHARINFO));
}

/* ========================================================================= */
/* 関数名：DeleteAdminWndCharLeave											 */
/* 内容：預かり所情報ウィンドウ消去											 */
/* 日付：2004/02/07															 */
/* ========================================================================= */
void DeleteAdminWndCharLeave(void)
{
	SAFE_DESTROYWND (m_hWndButton1);
	SAFE_DESTROYWND (m_hWndButton2);
	SAFE_DESTROYWND (m_hWndButton3);
	SAFE_DESTROYWND (m_hWndButton4);
	SAFE_DESTROYWND (m_hWndButton5);
	SAFE_DESTROYWND (m_hWndEdit1);
	SAFE_DESTROYWND (m_hWndEdit2);
	SAFE_DESTROYWND (m_hWndEdit3);

	SAFE_DESTROYWND (m_hWndList1);
	SAFE_DESTROYWND (m_hWndCombo1);

	g_bAdminPos = FALSE;
	m_dwSelectID = 0;
	SAFE_DELETE (m_pCharInfo);
}

/* ========================================================================= */
/* 関数名：MakeAdminWndAddMap												 */
/* 内容：マップ追加ウィンドウ作成											 */
/* 日付：2003/07/06															 */
/* ========================================================================= */
void MakeAdminWndAddMap(void)
{
}

/* ========================================================================= */
/* 関数名：DeleteAdminWndAddMap												 */
/* 内容：マップ追加ウィンドウ消去											 */
/* 日付：2003/07/06															 */
/* ========================================================================= */
void DeleteAdminWndAddMap(void)
{
	SAFE_DESTROYWND (m_hWndButton1);
}

/* ========================================================================= */
/* 関数名：MakeAdminWndMsg													 */
/* 内容：メッセージ情報ウィンドウ作成										 */
/* 日付：2003/07/14															 */
/* ========================================================================= */
void MakeAdminWndMsg(void)
{
	m_hWndButton1 = CreateWindow ("BUTTON", "←", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			16 * 2, 16 * 0, 16, 16, g_hWndAdmin, (HMENU)IDC_BUTTON1, g_hInstance, NULL);
	SendMessage (m_hWndButton1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton2 = CreateWindow ("BUTTON", "→", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			16 * 4, 16 * 0, 16, 16, g_hWndAdmin, (HMENU)IDC_BUTTON2, g_hInstance, NULL);
	SendMessage (m_hWndButton2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton3 = CreateWindow ("BUTTON", "追加", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			16 * 6, 16 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON3, g_hInstance, NULL);
	SendMessage (m_hWndButton3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton4 = CreateWindow ("BUTTON", "送信", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			16 * 8, 16 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON4, g_hInstance, NULL);
	SendMessage (m_hWndButton4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton5 = CreateWindow ("BUTTON", "取得", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			16 * 10, 16 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON5, g_hInstance, NULL);
	SendMessage (m_hWndButton5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEdit1 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
//			16 * 0, 16 * 2, 12 * 20, 12 * 5, g_hWndAdmin, NULL, g_hInstance, NULL);
			16 * 2, 16 * 2, 12 * 20, 12 * 1, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	g_nSelectPartsID = 0;
	SendCmdMAPMSGREQ (g_nSelectPartsID);
}

/* ========================================================================= */
/* 関数名：DeleteAdminWndMsg												 */
/* 内容：メッセージ情報ウィンドウ消去										 */
/* 日付：2003/07/14															 */
/* ========================================================================= */
void DeleteAdminWndMsg(void)
{
	SAFE_DESTROYWND (m_hWndButton1);
	SAFE_DESTROYWND (m_hWndButton2);
	SAFE_DESTROYWND (m_hWndButton3);
	SAFE_DESTROYWND (m_hWndButton4);
	SAFE_DESTROYWND (m_hWndButton5);
	SAFE_DESTROYWND (m_hWndEdit1);
}

/* ========================================================================= */
/* 関数名：MakeAdminWndAccAdd												 */
/* 内容：アカウント追加ウィンドウ作成										 */
/* 日付：2003/08/23															 */
/* ========================================================================= */
void MakeAdminWndAccAdd(void)
{
	m_hWndButton1 = CreateWindow ("BUTTON", "追加", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 10, 12 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON1, g_hInstance, NULL);
	SendMessage (m_hWndButton1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEdit1 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
			12 * 8, 12 * 2, 128, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEdit2 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
			12 * 8, 12 * 4, 128, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
}

/* ========================================================================= */
/* 関数名：MakeAdminWndAccAdd												 */
/* 内容：アカウント追加ウィンドウ消去										 */
/* 日付：2003/08/23															 */
/* ========================================================================= */
void DeleteAdminWndAccAdd(void)
{
	SAFE_DESTROYWND (m_hWndButton1);
	SAFE_DESTROYWND (m_hWndEdit1);
	SAFE_DESTROYWND (m_hWndEdit2);
}

/* ========================================================================= */
/* 関数名：MakeAdminWndAccInfo												 */
/* 内容：アカウント編集ウィンドウ作成										 */
/* 日付：2003/08/30															 */
/* ========================================================================= */
void MakeAdminWndAccInfo(void)
{
	m_hWndButton1 = CreateWindow ("BUTTON", "←", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 3, 12 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON1, g_hInstance, NULL);
	SendMessage (m_hWndButton1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton2 = CreateWindow ("BUTTON", "→", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 9, 12 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON2, g_hInstance, NULL);
	SendMessage (m_hWndButton2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton3 = CreateWindow ("BUTTON", "削除", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 20, 12 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON3, g_hInstance, NULL);
	SendMessage (m_hWndButton3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton4 = CreateWindow ("BUTTON", "許可", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 12, 12 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON4, g_hInstance, NULL);
	SendMessage (m_hWndButton4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton5 = CreateWindow ("BUTTON", "拒否", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15, 12 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON5, g_hInstance, NULL);
	SendMessage (m_hWndButton5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEdit1 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
			12 * 3 + 35, 2, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_dwSelectID = 0;
	m_pParaCharInfo = new CMDPARA_CHARINFO;
	ZeroMemory (m_pParaCharInfo, sizeof (CMDPARA_CHARINFO));
}

/* ========================================================================= */
/* 関数名：MakeAdminWndAccInfo												 */
/* 内容：アカウント編集ウィンドウ消去										 */
/* 日付：2003/08/30															 */
/* ========================================================================= */
void DeleteAdminWndAccInfo(void)
{
	SAFE_DESTROYWND (m_hWndButton1);
	SAFE_DESTROYWND (m_hWndButton2);
	SAFE_DESTROYWND (m_hWndButton3);
	SAFE_DESTROYWND (m_hWndEdit1);
	m_dwSelectID = 0;
	if (m_pParaCharInfo) {
		SAFE_DELETE_ARRAY (m_pParaCharInfo->pszNpcSpeech);
		SAFE_DELETE (m_pParaCharInfo);
	}
}

/* ========================================================================= */
/* 関数名：MakeAdminWndCefEdit												 */
/* 内容：吹き出し設定ウィンドウ作成											 */
/* 日付：2003/08/31															 */
/* ========================================================================= */
void MakeAdminWndCefEdit(void)
{
	m_hWndButton1 = CreateWindow ("BUTTON", "←", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 3, 12 * 4, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON1, g_hInstance, NULL);
	SendMessage (m_hWndButton1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton2 = CreateWindow ("BUTTON", "→", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 6, 12 * 4, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON2, g_hInstance, NULL);
	SendMessage (m_hWndButton2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton3 = CreateWindow ("BUTTON", "取得", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 6, 12 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON3, g_hInstance, NULL);
	SendMessage (m_hWndButton3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton4 = CreateWindow ("BUTTON", "追加", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 9, 12 * 4, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON4, g_hInstance, NULL);
	SendMessage (m_hWndButton4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton5 = CreateWindow ("BUTTON", "ｸﾘｱ", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 12, 12 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON5, g_hInstance, NULL);
	SendMessage (m_hWndButton5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton6 = CreateWindow ("BUTTON", "送信", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15, 12 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON6, g_hInstance, NULL);
	SendMessage (m_hWndButton6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEdit1 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
			12 * 1, 12 * 1, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEdit2 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
			12 * 9, 12 * 6, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndEdit3 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
			12 * 9, 12 * 7 + 4, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndScrBar1 = CreateWindow ("SCROLLBAR", "", WS_CHILD | WS_VISIBLE | SBS_HORZ,
			12 * 17, 12 * 5, 32 * 5, 12, g_hWndAdmin, (HMENU)IDC_SCRBAR1, g_hInstance, NULL);
	SetScrollRange (m_hWndScrBar1, SB_CTL, 0, 128 - 5, FALSE);

	m_hWndCombo1 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			12 * 2, 12 * 9, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
	SendMessage (m_hWndCombo1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	for (int i = 0; ; i ++) {
		if (aszSoundName[i][0] == 'z') {
			break;
		}
		SendMessage (m_hWndCombo1, CB_ADDSTRING, (WPARAM)0, (LPARAM)aszSoundName[i]);
	}


	m_dwSelectID = m_dwSelectID2 = 0;
	SetWindowText (m_hWndEdit1, "0");

	PCOMEFCINFO pComEfc = &g_pComEfcInfo[m_dwSelectID];
	PCOMEFCANIMEINFO pAnime = NULL;
	char szTmp[128];
	pAnime = &pComEfc->pAnime[0];
	if (pAnime) {
		wsprintf (szTmp, "%d", pAnime->byGrpID);
		SetWindowText (m_hWndEdit2, szTmp);
		wsprintf (szTmp, "%d", pAnime->byWait);
		SetWindowText (m_hWndEdit3, szTmp);
	}
}

/* ========================================================================= */
/* 関数名：DeleteAdminWndCefEdit											 */
/* 内容：吹き出し設定ウィンドウ消去											 */
/* 日付：2003/08/30															 */
/* ========================================================================= */
void DeleteAdminWndCefEdit(void)
{
	SAFE_DESTROYWND (m_hWndButton1);
	SAFE_DESTROYWND (m_hWndButton2);
	SAFE_DESTROYWND (m_hWndButton3);
	SAFE_DESTROYWND (m_hWndButton4);
	SAFE_DESTROYWND (m_hWndButton5);
	SAFE_DESTROYWND (m_hWndButton6);
	SAFE_DESTROYWND (m_hWndEdit1);
	SAFE_DESTROYWND (m_hWndEdit2);
	SAFE_DESTROYWND (m_hWndEdit3);
	SAFE_DESTROYWND (m_hWndCombo1);
	m_dwSelectID = m_dwSelectID2 = 0;
}

/* ========================================================================= */
/* 関数名：MakeAdminWndEtcVersion											 */
/* 内容：バージョン通知ウィンドウ作成										 */
/* 日付：2003/09/05															 */
/* ========================================================================= */
void MakeAdminWndEtcVersion(void)
{
	m_hWndButton1 = CreateWindow ("BUTTON", "バージョン通知", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 0, 12 * 1, 12 * 7, 16, g_hWndAdmin, (HMENU)IDC_BUTTON1, g_hInstance, NULL);
	SendMessage (m_hWndButton1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton2 = CreateWindow ("BUTTON", "管理人業務ON/OFF", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 0, 12 * 3, 12 * 10, 16, g_hWndAdmin, (HMENU)IDC_BUTTON2, g_hInstance, NULL);
	SendMessage (m_hWndButton2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton3 = CreateWindow ("BUTTON", "放送前に鳴らす", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 0, 12 * 5, 12 * 8, 16, g_hWndAdmin, (HMENU)IDC_BUTTON3, g_hInstance, NULL);
	SendMessage (m_hWndButton3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton4 = CreateWindow ("BUTTON", "当たり判定ON", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 0, 12 * 7, 12 * 8, 16, g_hWndAdmin, (HMENU)IDC_BUTTON4, g_hInstance, NULL);
	SendMessage (m_hWndButton4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton5 = CreateWindow ("BUTTON", "当たり判定OFF", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 9, 12 * 7, 12 * 8, 16, g_hWndAdmin, (HMENU)IDC_BUTTON5, g_hInstance, NULL);
	SendMessage (m_hWndButton5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton6 = CreateWindow ("BUTTON", "鐘", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 9, 12 * 5, 12 * 3, 16, g_hWndAdmin, (HMENU)IDC_BUTTON6, g_hInstance, NULL);
	SendMessage (m_hWndButton6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton7 = CreateWindow ("BUTTON", "ｶｳﾝﾀﾘｾｯﾄ", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 13, 12 * 5, 12 * 5, 16, g_hWndAdmin, (HMENU)IDC_BUTTON7, g_hInstance, NULL);
	SendMessage (m_hWndButton7, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
}

/* ========================================================================= */
/* 関数名：DeleteAdminWndEtcVersion											 */
/* 内容：バージョン通知ウィンドウ消去										 */
/* 日付：2003/09/05															 */
/* ========================================================================= */
void DeleteAdminWndEtcVersion(void)
{
	SAFE_DESTROYWND (m_hWndButton1);
	SAFE_DESTROYWND (m_hWndButton2);
	SAFE_DESTROYWND (m_hWndButton3);
	SAFE_DESTROYWND (m_hWndButton4);
	SAFE_DESTROYWND (m_hWndButton5);
	SAFE_DESTROYWND (m_hWndButton6);
	SAFE_DESTROYWND (m_hWndButton7);
}

/* ========================================================================= */
/* 関数名：MakeAdminWndItemList												 */
/* 内容：アイテム一覧ウィンドウ作成											 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void MakeAdminWndItemList(void)
{
	LV_COLUMN lvcol;
	m_hWndList1 = CreateWindowEx (WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
			WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS,
			12, 24, 350, 80, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndList1, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
	ListView_SetExtendedListViewStyle (m_hWndList1 , LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	ZeroMemory (&lvcol, sizeof (lvcol));
	lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol.fmt = LVCFMT_LEFT;

	lvcol.cx = 40;
	lvcol.pszText = "ID";
	ListView_InsertColumn (m_hWndList1, 0, &lvcol);

	lvcol.cx = 100;
	lvcol.pszText = "名前";
	ListView_InsertColumn (m_hWndList1, 1, &lvcol);

	lvcol.cx = 100;
	lvcol.pszText = "種別";
	ListView_InsertColumn (m_hWndList1, 2, &lvcol);

	lvcol.cx = 40;
	lvcol.pszText = "効果";
	ListView_InsertColumn (m_hWndList1, 3, &lvcol);

	lvcol.cx = 45;
	lvcol.pszText = "金額";
	ListView_InsertColumn (m_hWndList1, 4, &lvcol);


	m_hWndButton1 = CreateWindow ("BUTTON", "追加", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			16 * 1, 1, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON1, g_hInstance, NULL);
	SendMessage (m_hWndButton1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton2 = CreateWindow ("BUTTON", "編集", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			16 * 4, 1, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON2, g_hInstance, NULL);
	SendMessage (m_hWndButton2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton3 = CreateWindow ("BUTTON", "ｸﾘｱ", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			16 * 20, 1, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON3, g_hInstance, NULL);
	SendMessage (m_hWndButton3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton4 = CreateWindow ("BUTTON", "送信", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			16 * 7, 1, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON4, g_hInstance, NULL);
	SendMessage (m_hWndButton4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCombo1 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			12 * 19, 120 - 5, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
	SendMessage (m_hWndCombo1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCombo2 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			12 * 19, 120 - 5 + 22, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO2, g_hInstance, NULL);
	SendMessage (m_hWndCombo2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	LV_ITEM item;
	PITEMINFO pItem;
	char szTmp[128];
	int i, nCount = g_aItemInfo.GetSize ();
	ZeroMemory (&item, sizeof (item));
	item.mask = LVIF_TEXT;

	for (i = 0; i < nCount; i ++) {
		ZeroMemory (szTmp, sizeof (szTmp));

		pItem = g_aItemInfo[i];
		wsprintf (szTmp, "%d", i);
		item.iItem = i;
		/* ID */
		item.iSubItem = 0;
		item.pszText = szTmp;
		ListView_InsertItem (m_hWndList1, &item);

		/* 名前 */
		item.iSubItem = 1;
		item.pszText = pItem->szName;
		ListView_SetItem (m_hWndList1, &item);

		/* 種別 */
		item.iSubItem = 2;
		item.pszText = (LPSTR)aItemeTypeNameTbl[pItem->byType];
		ListView_SetItem (m_hWndList1, &item);

		/* 効果 */
		item.iSubItem = 3;
		wsprintf (szTmp, "%d", pItem->byData);
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);

		/* 金額 */
		item.iSubItem = 4;
		wsprintf (szTmp, "%d", pItem->dwGold);
		item.pszText = szTmp;
		ListView_SetItem (m_hWndList1, &item);
	}

	for (i = 0; ; i ++) {
		if (aItemeTypeNameTbl[i][0] == 'z') {
			break;
		}
		SendMessage (m_hWndCombo1, CB_ADDSTRING, (WPARAM)0, (LPARAM)aItemeTypeNameTbl[i]);
	}
	for (i = 0; ; i ++) {
		if (aszSoundName[i][0] == 'z') {
			break;
		}
		SendMessage (m_hWndCombo2, CB_ADDSTRING, (WPARAM)0, (LPARAM)aszSoundName[i]);
	}
	SendMessage (m_hWndCombo1, CB_SETCURSEL, 0, 0);
	SendMessage (m_hWndCombo2, CB_SETCURSEL, 0, 0);
	ShowWindow (m_hWndCombo1, SW_HIDE);
	ShowWindow (m_hWndCombo2, SW_HIDE);
	m_dwSelectID = -1;
}

/* ========================================================================= */
/* 関数名：DeleteAdminWndItemList											 */
/* 内容：アイテム一覧ウィンドウ消去											 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void DeleteAdminWndItemList(void)
{
	SAFE_DESTROYWND (m_hWndList1);
	SAFE_DESTROYWND (m_hWndButton1);
	SAFE_DESTROYWND (m_hWndButton2);
	SAFE_DESTROYWND (m_hWndButton3);
	SAFE_DESTROYWND (m_hWndButton4);
	SAFE_DESTROYWND (m_hWndEdit1);
	SAFE_DESTROYWND (m_hWndEdit2);
	SAFE_DESTROYWND (m_hWndEdit3);
	SAFE_DESTROYWND (m_hWndEdit4);
	SAFE_DESTROYWND (m_hWndEdit5);
	SAFE_DESTROYWND (m_hWndEdit6);
	SAFE_DESTROYWND (m_hWndEdit7);
	SAFE_DESTROYWND (m_hWndEdit8);
	SAFE_DESTROYWND (m_hWndCombo1);
	SAFE_DESTROYWND (m_hWndCombo2);
	SAFE_DESTROYWND (m_hWndCombo3);
	SAFE_DESTROYWND (m_hWndCombo4);
	SAFE_DESTROYWND (m_hWndCheck1);
	SAFE_DESTROYWND (m_hWndCheck2);
	SAFE_DESTROYWND (m_hWndCheck3);
	m_dwSelectID = 0;
}

/* ========================================================================= */
/* 関数名：MakeItemEditWindow												 */
/* 内容：種類別アイテム編集ウィンドウ作成									 */
/* 日付：2003/09/14															 */
/* ========================================================================= */
void MakeItemEditWindow(BYTE byType)
{
	int i, nCount;

	DeleteItemEditWindow ();

	/* 名前 */
	m_hWndEdit1 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
			12 * 1, 132, 150, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 値段 */
	m_hWndEdit2 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
			12 * 4, 135 + 14 * 1, 48, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 無くなる確率 */
	m_hWndEdit4 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
			12 * 4 + 56 + 12 * 4, 135 + 14 * 1, 20, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 使った時のメッセージ */
	m_hWndEdit3 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
			12 * 12, 135 + 14 * 2, 128, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCheck1 = CreateWindow ("BUTTON", "捨てれない", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
			12 * 26, 137 + 22, 72, 16, g_hWndAdmin, (HMENU)IDC_TMPCHECK1, g_hInstance, NULL);
	SendMessage (m_hWndCheck1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCheck2 = CreateWindow ("BUTTON", "なくなる", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
			12 * 26, 137 + 22 + 16, 72, 16, g_hWndAdmin, (HMENU)IDC_TMPCHECK2, g_hInstance, NULL);
	SendMessage (m_hWndCheck2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCheck5 = CreateWindow ("BUTTON", "取引不可", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
			12 * 26, 137 + 22 + 32, 72, 16, g_hWndAdmin, (HMENU)IDC_TMPCHECK5, g_hInstance, NULL);
	SendMessage (m_hWndCheck5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCombo4 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			12 * 19, 137 + 22 + 16, 72, 12 * 5, g_hWndAdmin, (HMENU)IDC_TMPCOMBO4, g_hInstance, NULL);
	SendMessage (m_hWndCombo4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	for (i = 0; i < SEXDISTINCTION_MAX; i ++) {
		SendMessage (m_hWndCombo4, CB_ADDSTRING, 0, (LPARAM)aSexDistinctionTbl[i]);
	}

	ShowWindow (m_hWndCombo1, SW_SHOW);
	ShowWindow (m_hWndCombo2, SW_SHOW);
	ShowWindow (m_hWndCombo4, SW_SHOW);

	switch (byType) {
	case ITEMTYPE_NONE:		/* 効果無し */
		SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_NONE, 0);
		break;

	case ITEMTYPE_RURA:		/* 特定位置へ移動 */
		SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_RURA, 0);
		/* マップID */
		m_hWndEdit5 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 6, 132 + 14 * 5, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		/* X座標 */
		m_hWndEdit6 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 6, 132 + 14 * 6, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		/* Y座標 */
		m_hWndEdit7 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 6, 132 + 14 * 7, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit7, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

		m_hWndCheck3 = CreateWindow ("BUTTON", "マップ限定", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
				12 * 12, 132 + 14 * 4, 72, 16, g_hWndAdmin, (HMENU)IDC_TMPCHECK1, g_hInstance, NULL);
		SendMessage (m_hWndCheck3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		/* 限定するマップID */
		m_hWndEdit8 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 12, 132 + 14 * 5 + 5, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit8, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		break;

	case ITEMTYPE_HPUP:	/* HP回復 */
		SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_HPUP, 0);
		/* 最低回復値 */
		m_hWndEdit6 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 8, 132 + 14 * 5, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		/* 最大回復値 */
		m_hWndEdit7 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 8, 132 + 14 * 6, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit7, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		/* 回復率 */
		m_hWndCheck3 = CreateWindow ("BUTTON", "回復率", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
				12 * 8, 132 + 14 * 7, 72, 16, g_hWndAdmin, (HMENU)IDC_TMPCHECK3, g_hInstance, NULL);
		SendMessage (m_hWndCheck3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		break;

	case ITEMTYPE_WEAPON:	/* 武器 */
		SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_WEAPON, 0);
		/* 効果 */
		m_hWndEdit6 = CreateWindowEdit (12 * 8, 132 + 14 * 5, 30, 12, 0);
		/* 攻撃時に発動するエフェクト番号 */
		m_hWndEdit7 = CreateWindowEdit (12 * 8, 132 + 14 * 6, 30, 12, 0);
		/* 攻撃力減少幅 */
		m_hWndEdit8 = CreateWindowEdit (12 * 8, 132 + 14 * 7, 30, 12, 0);
		/* 付加データ1 */
		m_hWndEdit9 = CreateWindowEdit (12 * 8, 132 + 14 * 8, 30, 12, 0);
		/* 付加データ2 */
		m_hWndEdit10 = CreateWindowEdit (12 * 8, 132 + 14 * 9, 30, 12, 0);
		/* 付加データ3 */
		m_hWndEdit11 = CreateWindowEdit (12 * 8, 132 + 14 * 10, 30, 12, 0);
		/* 付加データ4 */
		m_hWndEdit12 = CreateWindowEdit (12 * 8, 132 + 14 * 11, 30, 12, 0);
		/* 付加データ5 */
		m_hWndEdit13 = CreateWindowEdit (12 * 20, 132 + 14 * 11, 30, 12, 0);
		/* 装備できるキャラ情報 */
		m_hWndCombo3 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
				12 * 19, 132 + 14 * 5, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
		SendMessage (m_hWndCombo3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		nCount = g_aEquUserInfo.GetSize ();
		for (i = 0; i < nCount; i ++) {
			PEQUUSERINFO pInfo;

			pInfo = g_aEquUserInfo[i];
			SendMessage (m_hWndCombo3, CB_ADDSTRING, 0, (LPARAM)pInfo->szName);
		}
		/* 攻撃種別 */
		m_hWndCombo5 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
				12 * 19, 132 + 14 * 6 + 6
				, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO5, g_hInstance, NULL);
		SendMessage (m_hWndCombo5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		for (i = 0; i < ATACKTYPE_MAX; i ++) {
			SendMessage (m_hWndCombo5, CB_ADDSTRING, 0, (LPARAM)aAtackTypeTbl[i]);
		}
		break;

	case ITEMTYPE_ARMOR:	/* 鎧 */
		SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_ARMOR, 0);
		/* 効果 */
		m_hWndEdit6 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 8, 132 + 14 * 5, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		/* 装備できるキャラ情報 */
		m_hWndCombo3 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
				12 * 19, 132 + 14 * 5, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
		SendMessage (m_hWndCombo3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		nCount = g_aEquUserInfo.GetSize ();
		for (i = 0; i < nCount; i ++) {
			PEQUUSERINFO pInfo;

			pInfo = g_aEquUserInfo[i];
			SendMessage (m_hWndCombo3, CB_ADDSTRING, 0, (LPARAM)pInfo->szName);
		}
		break;

	case ITEMTYPE_SHILD:	/* 盾 */
		SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_SHILD, 0);
		/* 効果 */
		m_hWndEdit6 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 8, 132 + 14 * 5, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		/* 装備できるキャラ情報 */
		m_hWndCombo3 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
				12 * 19, 132 + 14 * 5, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
		SendMessage (m_hWndCombo3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		nCount = g_aEquUserInfo.GetSize ();
		for (i = 0; i < nCount; i ++) {
			PEQUUSERINFO pInfo;

			pInfo = g_aEquUserInfo[i];
			SendMessage (m_hWndCombo3, CB_ADDSTRING, 0, (LPARAM)pInfo->szName);
		}
		break;

	case ITEMTYPE_HELMET:	/* 兜 */
		SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_HELMET, 0);
		/* 効果 */
		m_hWndEdit6 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 8, 132 + 14 * 5, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		/* 装備できるキャラ情報 */
		m_hWndCombo3 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
				12 * 19, 132 + 14 * 5, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
		SendMessage (m_hWndCombo3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		nCount = g_aEquUserInfo.GetSize ();
		for (i = 0; i < nCount; i ++) {
			PEQUUSERINFO pInfo;

			pInfo = g_aEquUserInfo[i];
			SendMessage (m_hWndCombo3, CB_ADDSTRING, 0, (LPARAM)pInfo->szName);
		}
		break;

	case ITEMTYPE_SEED:		/* 種 */
		{
			SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_SEED, 0);
			/* 種類 */
			m_hWndCombo3 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
					12 * 6, 132 + 14 * 4, 12 * 6, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
			SendMessage (m_hWndCombo3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			int i;
			LPCTSTR liststr[] = {"力", "素早さ", "体力", "賢さ", "運のよさ", "最大HP", "最大Mana",
								 "力減少", "素早さ減少", "体力減少", "賢さ減少", "運のよさ減少", "最大HP減少", "最大Mana減少",
								 "z"};
			for (i = 0; ; i ++) {
				if (liststr[i][0] == 'z') {
					break;
				}
				SendMessage (m_hWndCombo3, CB_ADDSTRING, (WPARAM)0, (LPARAM)liststr[i]);
			}

			/* 上限値 */
			m_hWndEdit6 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
					12 * 6, 132 + 14 * 6, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		}
		break;

	case ITEMTYPE_SUBGAME:	/* サブゲーム参加 */
		{
			SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_SUBGAME, 0);
			/* 種類 */
			m_hWndCombo3 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
					12 * 6, 132 + 14 * 4, 12 * 6, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
			SendMessage (m_hWndCombo3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			int i;
			for (i = 0; ; i ++) {
				if (aSubGameNameTbl[i][0] == 'z') {
					break;
				}
				SendMessage (m_hWndCombo3, CB_ADDSTRING, (WPARAM)0, (LPARAM)aSubGameNameTbl[i]);
			}
		}
		break;

	case ITEMTYPE_SAKE:	/* 酒 */
		{
			SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_SAKE, 0);
			/* 度数 */
			m_hWndEdit6 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
					12 * 6, 132 + 14 * 4, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		}
		break;

	case ITEMTYPE_MSG:	/* メッセージ表示 */
		{
			SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_MSG, 0);
			/* 他のキャラに使った時のメッセージ */
			m_hWndEdit6 = CreateWindowEdit (12 * 1, 132 + 14 * 5, 128, 12, ES_AUTOHSCROLL);
			/* 他のキャラに使われた時のメッセージ */
			m_hWndEdit7 = CreateWindowEdit (12 * 1, 132 + 14 * 7, 128, 12, ES_AUTOHSCROLL);
			/* 文字エフェクト */
			m_hWndEdit8 = CreateWindowEdit (12 * 1, 132 + 14 * 9, 80, 12, 0);
			/* エフェクト番号 */
			m_hWndEdit9 = CreateWindowEdit (12 * 1, 132 + 14 * 11, 6 * 4, 12, 0);
			/* 表示種別 */
			m_hWndCombo3 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
					12 * 15, 132 + 14 * 9 - 12, 12 * 6, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
			SendMessage (m_hWndCombo3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			int i;
			LPCTSTR liststr[] = {"流す", "枠付き", "z"};
			for (i = 0; ; i ++) {
				if (liststr[i][0] == 'z') {
					break;
				}
				SendMessage (m_hWndCombo3, CB_ADDSTRING, (WPARAM)0, (LPARAM)liststr[i]);
			}
		}
		break;

	case ITEMTYPE_GRPCHG:	/* キャラ画像変更 */
		{
			SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_GRPCHG, 0);
			/* 変更するグラフィックID */
			m_hWndEdit6 = CreateWindowEdit (12 * 6, 132 + 14 * 4, 30, 12, 0);
		}
		break;

	case ITEMTYPE_LIGHT:	/* 明かり */
		{
			SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_LIGHT, 0);
			/* 明るさレベル */
			m_hWndEdit6 = CreateWindowEdit (12 * 10, 132 + 14 * 4, 30, 12, 0);
			/* 維持時間 */
			m_hWndEdit7 = CreateWindowEdit (12 * 10, 132 + 14 * 5, 30, 12, 0);
		}
		break;

	case ITEMTYPE_HPDOWN:	/* HP減少 */
		SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_HPDOWN, 0);
		/* 最低回復値 */
		m_hWndEdit6 = CreateWindowEdit (12 * 8, 132 + 14 * 5, 30, 12, 0);
		/* 最大回復値 */
		m_hWndEdit7 = CreateWindowEdit (12 * 8, 132 + 14 * 6, 30, 12, 0);
		/* エフェクト番号 */
		m_hWndEdit8 = CreateWindowEdit (12 * 8, 132 + 14 * 7, 30, 12, 0);
		/* ダメージ率 */
		m_hWndCheck3 = CreateWindow ("BUTTON", "ﾀﾞﾒｰｼﾞ率", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
				12 * 8, 132 + 14 * 8, 72, 16, g_hWndAdmin, (HMENU)IDC_TMPCHECK3, g_hInstance, NULL);
		SendMessage (m_hWndCheck3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		break;

	case ITEMTYPE_MAGIC:	/* 魔法習得 */
	case ITEMTYPE_USEMAGIC:	/* 特技発動(習得と同じく特技を選ぶ) */
		{
			SendMessage (m_hWndCombo1, CB_SETCURSEL, byType, 0);
			/* 習得する魔法 */
			m_hWndCombo3 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
					12 * 8, 132 + 14 * 5, 12 * 15, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
			SendMessage (m_hWndCombo3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			int i, nCount;
			nCount = g_aMagicInfo.GetSize ();
			for (i = 0; i < nCount; i ++) {
				PMAGICINFO pInfo;

				pInfo = g_aMagicInfo[i];
				SendMessage (m_hWndCombo3, CB_ADDSTRING, 0, (LPARAM)pInfo->szName);
			}
		}
		break;

	case ITEMTYPE_MANAUP:	/* MANA回復 */
		SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_MANAUP, 0);
		/* 最低回復値 */
		m_hWndEdit6 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 8, 132 + 14 * 5, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		/* 最大回復値 */
		m_hWndEdit7 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 8, 132 + 14 * 6, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit7, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		/* 回復率 */
		m_hWndCheck3 = CreateWindow ("BUTTON", "回復率", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
				12 * 8, 132 + 14 * 7, 72, 16, g_hWndAdmin, (HMENU)IDC_TMPCHECK3, g_hInstance, NULL);
		SendMessage (m_hWndCheck3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		break;

	case ITEMTYPE_HPMANAUP:	/* HP・Mana回復 */
		SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_HPMANAUP, 0);
		/* 最低回復値(HP) */
		m_hWndEdit6 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 12, 132 + 14 * 5, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		/* 最大回復値(HP) */
		m_hWndEdit7 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 12, 132 + 14 * 6, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit7, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		/* 最低回復値(Mana) */
		m_hWndEdit8 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 12, 132 + 14 * 7, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit8, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		/* 最大回復値(Mana) */
		m_hWndEdit9 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 12, 132 + 14 * 8, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit9, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		/* 回復率(HP) */
		m_hWndCheck3 = CreateWindow ("BUTTON", "回復率", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
				12 * 8, 132 + 14 * 9, 72, 14, g_hWndAdmin, (HMENU)IDC_TMPCHECK3, g_hInstance, NULL);
		SendMessage (m_hWndCheck3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		/* 回復率(Mana) */
		m_hWndCheck4 = CreateWindow ("BUTTON", "回復率", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
				12 * 8, 132 + 14 * 10, 72, 14, g_hWndAdmin, (HMENU)IDC_TMPCHECK4, g_hInstance, NULL);
		SendMessage (m_hWndCheck4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		break;

	case ITEMTYPE_EQU_HAND:	/* 装備(小手) */
		SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_EQU_HAND, 0);
		/* 効果 */
		m_hWndEdit6 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 8, 132 + 14 * 5, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		/* 装備できるキャラ情報 */
		m_hWndCombo3 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
				12 * 19, 132 + 14 * 5, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
		SendMessage (m_hWndCombo3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		nCount = g_aEquUserInfo.GetSize ();
		for (i = 0; i < nCount; i ++) {
			PEQUUSERINFO pInfo;

			pInfo = g_aEquUserInfo[i];
			SendMessage (m_hWndCombo3, CB_ADDSTRING, 0, (LPARAM)pInfo->szName);
		}
		break;

	case ITEMTYPE_EQU_LEG:	/* 装備(靴) */
		SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_EQU_LEG, 0);
		/* 効果 */
		m_hWndEdit6 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 8, 132 + 14 * 5, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		/* 装備できるキャラ情報 */
		m_hWndCombo3 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
				12 * 19, 132 + 14 * 5, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
		SendMessage (m_hWndCombo3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		nCount = g_aEquUserInfo.GetSize ();
		for (i = 0; i < nCount; i ++) {
			PEQUUSERINFO pInfo;

			pInfo = g_aEquUserInfo[i];
			SendMessage (m_hWndCombo3, CB_ADDSTRING, 0, (LPARAM)pInfo->szName);
		}
		break;

	case ITEMTYPE_EQU_ACC1:	/* 装備(首飾り) */
		SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_EQU_ACC1, 0);
		/* 効果 */
		m_hWndEdit6 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 8, 132 + 14 * 5, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		/* 装備できるキャラ情報 */
		m_hWndCombo3 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
				12 * 19, 132 + 14 * 5, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
		SendMessage (m_hWndCombo3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		nCount = g_aEquUserInfo.GetSize ();
		for (i = 0; i < nCount; i ++) {
			PEQUUSERINFO pInfo;

			pInfo = g_aEquUserInfo[i];
			SendMessage (m_hWndCombo3, CB_ADDSTRING, 0, (LPARAM)pInfo->szName);
		}
		/* 装備効果 */
		m_hWndCombo5 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
				12 * 19, 132 + 14 * 5 + 22, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
		SendMessage (m_hWndCombo5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		for (i = 0; ; i ++) {
			if (aEquTypeTbl[i][0] == 'z') {
				break;
			}

			SendMessage (m_hWndCombo5, CB_ADDSTRING, 0, (LPARAM)aEquTypeTbl[i]);
		}
		break;

	case ITEMTYPE_EQU_ACC2:	/* 装備(指輪1) */
		SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_EQU_ACC2, 0);
		/* 効果 */
		m_hWndEdit6 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 8, 132 + 14 * 5, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		/* 装備できるキャラ情報 */
		m_hWndCombo3 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
				12 * 19, 132 + 14 * 5, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
		SendMessage (m_hWndCombo3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		nCount = g_aEquUserInfo.GetSize ();
		for (i = 0; i < nCount; i ++) {
			PEQUUSERINFO pInfo;

			pInfo = g_aEquUserInfo[i];
			SendMessage (m_hWndCombo3, CB_ADDSTRING, 0, (LPARAM)pInfo->szName);
		}
		/* 装備効果 */
		m_hWndCombo5 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
				12 * 19, 132 + 14 * 5 + 22, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
		SendMessage (m_hWndCombo5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		for (i = 0; ; i ++) {
			if (aEquTypeTbl[i][0] == 'z') {
				break;
			}

			SendMessage (m_hWndCombo5, CB_ADDSTRING, 0, (LPARAM)aEquTypeTbl[i]);
		}
		break;

	case ITEMTYPE_EQU_ACC3:	/* 装備(指輪2) */
		SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_EQU_ACC3, 0);
		/* 効果 */
		m_hWndEdit6 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 8, 132 + 14 * 5, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		/* 装備できるキャラ情報 */
		m_hWndCombo3 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
				12 * 19, 132 + 14 * 5, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
		SendMessage (m_hWndCombo3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		nCount = g_aEquUserInfo.GetSize ();
		for (i = 0; i < nCount; i ++) {
			PEQUUSERINFO pInfo;

			pInfo = g_aEquUserInfo[i];
			SendMessage (m_hWndCombo3, CB_ADDSTRING, 0, (LPARAM)pInfo->szName);
		}
		/* 装備効果 */
		m_hWndCombo5 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
				12 * 19, 132 + 14 * 5 + 22, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
		SendMessage (m_hWndCombo5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		for (i = 0; ; i ++) {
			if (aEquTypeTbl[i][0] == 'z') {
				break;
			}

			SendMessage (m_hWndCombo5, CB_ADDSTRING, 0, (LPARAM)aEquTypeTbl[i]);
		}
		break;

	case ITEMTYPE_RESURRECTION:	/* 蘇生 */
		SendMessage (m_hWndCombo1, CB_SETCURSEL, ITEMTYPE_RESURRECTION, 0);
		/* 効果 */
		m_hWndEdit6 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
				12 * 8, 132 + 14 * 5, 30, 12, g_hWndAdmin, NULL, g_hInstance, NULL);
		SendMessage (m_hWndEdit6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
		break;

	default:
		break;
	}
	InvalidateRect (g_hWndAdmin, NULL, TRUE);
}

/* ========================================================================= */
/* 関数名：DeleteItemEditWindow												 */
/* 内容：種類別アイテム編集ウィンドウ消去									 */
/* 日付：2003/09/15															 */
/* ========================================================================= */
void DeleteItemEditWindow(void)
{
	SAFE_DESTROYWND (m_hWndEdit1);
	SAFE_DESTROYWND (m_hWndEdit2);
	SAFE_DESTROYWND (m_hWndEdit3);
	SAFE_DESTROYWND (m_hWndEdit4);
	SAFE_DESTROYWND (m_hWndEdit5);
	SAFE_DESTROYWND (m_hWndEdit6);
	SAFE_DESTROYWND (m_hWndEdit7);
	SAFE_DESTROYWND (m_hWndEdit8);
	SAFE_DESTROYWND (m_hWndEdit9);
	SAFE_DESTROYWND (m_hWndEdit10);
	SAFE_DESTROYWND (m_hWndEdit11);
	SAFE_DESTROYWND (m_hWndEdit12);
	SAFE_DESTROYWND (m_hWndEdit13);
	SAFE_DESTROYWND (m_hWndCheck1);
	SAFE_DESTROYWND (m_hWndCheck2);
	SAFE_DESTROYWND (m_hWndCheck3);
	SAFE_DESTROYWND (m_hWndCheck4);
	SAFE_DESTROYWND (m_hWndCheck5);
	SAFE_DESTROYWND (m_hWndCombo3);
	SAFE_DESTROYWND (m_hWndCombo4);
	SAFE_DESTROYWND (m_hWndCombo5);
	ShowWindow (m_hWndCombo1, SW_HIDE);
	ShowWindow (m_hWndCombo2, SW_HIDE);
	ShowWindow (m_hWndCombo3, SW_HIDE);
	ShowWindow (m_hWndCombo4, SW_HIDE);
	m_dwSelectID = -1;
	InvalidateRect (g_hWndAdmin, NULL, TRUE);
}

/* ========================================================================= */
/* 関数名：MakeAdminWndCharItem												 */
/* 内容：キャラアイテム編集ウィンドウ作成									 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void MakeAdminWndCharItem(void)
{
	m_hWndButton1 = CreateWindow ("BUTTON", "送信", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			0, 1, 36, 18, g_hWndAdmin, (HMENU)IDC_BUTTON1, g_hInstance, NULL);
	SendMessage (m_hWndButton1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton2 = CreateWindow ("BUTTON", "追加", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			238, 88, 36, 18, g_hWndAdmin, (HMENU)IDC_BUTTON2, g_hInstance, NULL);
	SendMessage (m_hWndButton2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton3 = CreateWindow ("BUTTON", "更新", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			238 + 16 * 3, 88, 36, 18, g_hWndAdmin, (HMENU)IDC_BUTTON3, g_hInstance, NULL);
	SendMessage (m_hWndButton3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton4 = CreateWindow ("BUTTON", "消去", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			238 + 16 * 6, 88, 36, 18, g_hWndAdmin, (HMENU)IDC_BUTTON4, g_hInstance, NULL);
	SendMessage (m_hWndButton4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 所持アイテム一覧 */
	m_hWndList1 = CreateWindowEx (WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
			WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
			12, 60, 200, 12 * 13, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndList1, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
	ListView_SetExtendedListViewStyle (m_hWndList1 , LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	LV_COLUMN lvcol;
	ZeroMemory (&lvcol, sizeof (lvcol));
	lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol.fmt = LVCFMT_LEFT;

	lvcol.cx = 60;
	lvcol.pszText = "装備";
	ListView_InsertColumn (m_hWndList1, 0, &lvcol);

	lvcol.cx = 130;
	lvcol.pszText = "アイテム名";
	ListView_InsertColumn (m_hWndList1, 1, &lvcol);

	/* アイテム名一覧 */
	m_hWndCombo1 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			230, 60, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
	SendMessage (m_hWndCombo1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	int i;
	for (i = 0; i < g_aItemInfo.GetSize (); i ++) {
		SendMessage (m_hWndCombo1, CB_ADDSTRING, (WPARAM)i, (LPARAM)g_aItemInfo[i]->szName);
	}
	
	/* ステータステンプレート一覧 */
	m_hWndCombo2 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			230, 128, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO2, g_hInstance, NULL);
	SendMessage (m_hWndCombo2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	for (i = 0; i < g_aStTmpInfo.GetSize (); i ++) {
		PSTATUSTEMPLATEINFO pInfo = g_aStTmpInfo[i];
		SendMessage (m_hWndCombo2, CB_ADDSTRING, (WPARAM)i, (LPARAM)pInfo->szName);
	}

	m_hWndButton5 = CreateWindow ("BUTTON", "←取得", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			248, 156, 48, 18, g_hWndAdmin, (HMENU)IDC_BUTTON5, g_hInstance, NULL);
	SendMessage (m_hWndButton5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton6 = CreateWindow ("BUTTON", "→反映", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			248 + 62, 156, 48, 18, g_hWndAdmin, (HMENU)IDC_BUTTON6, g_hInstance, NULL);
	SendMessage (m_hWndButton6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	g_bAdminPos = TRUE;
	m_dwSelectID = -1;
	m_pCharInfo = new CHARINFO;
	ZeroMemory (m_pCharInfo, sizeof (CHARINFO));
}

/* ========================================================================= */
/* 関数名：DeleteAdminWndCharItem											 */
/* 内容：キャラアイテム編集ウィンドウ消去									 */
/* 日付：2003/09/13															 */
/* ========================================================================= */
void DeleteAdminWndCharItem(void)
{
	HWND *ahWndTbl[10] = {
		&m_hWndEdit1,
		&m_hWndEdit2,
		&m_hWndEdit3,
		&m_hWndEdit4,
		&m_hWndEdit5,
		&m_hWndEdit6,
		&m_hWndEdit7,
		&m_hWndEdit8,
		&m_hWndEdit9,
		&m_hWndEdit10
	};
	for (int i = 0; i < 10; i ++) {
		SAFE_DESTROYWND (*ahWndTbl[i]);
	}
	SAFE_DESTROYWND (m_hWndButton1);
	SAFE_DESTROYWND (m_hWndButton2);
	SAFE_DESTROYWND (m_hWndButton3);
	SAFE_DESTROYWND (m_hWndButton4);
	SAFE_DESTROYWND (m_hWndButton5);
	SAFE_DESTROYWND (m_hWndButton6);
	SAFE_DESTROYWND (m_hWndList1);
	SAFE_DESTROYWND (m_hWndCombo1);

	g_bAdminPos = FALSE;
	m_dwSelectID = 0;
	SAFE_DELETE (m_pCharInfo);
}

/* ========================================================================= */
/* 関数名：MakeAdminWndSvrInfo												 */
/* 内容：サーバー情報ウィンドウ作成											 */
/* 日付：2003/09/18															 */
/* ========================================================================= */
void MakeAdminWndSvrInfo(void)
{
	m_hWndButton1 = CreateWindow ("BUTTON", "情報取得", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 0, 12 * 1, 12 * 5, 16, g_hWndAdmin, (HMENU)IDC_BUTTON1, g_hInstance, NULL);
	SendMessage (m_hWndButton1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndMapSave = CreateWindow ("BUTTON", "データ保存", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 0, 12 * 3, 12 * 6, 16, g_hWndAdmin, (HMENU)IDC_SAVEMAP, g_hInstance, NULL);
	SendMessage (m_hWndMapSave, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	SAFE_DELETE (m_pSvrInfo);
}

/* ========================================================================= */
/* 関数名：DeleteAdminWndSvrInfo											 */
/* 内容：サーバー情報ウィンドウ消去											 */
/* 日付：2003/09/18															 */
/* ========================================================================= */
void DeleteAdminWndSvrInfo(void)
{
	SAFE_DESTROYWND (m_hWndButton1);
	SAFE_DESTROYWND (m_hWndMapSave);
	SAFE_DELETE (m_pSvrInfo);
}

/* ========================================================================= */
/* 関数名：MakeAdminWndShopInfo												 */
/* 内容：店情報ウィンドウ作成												 */
/* 日付：2003/09/20															 */
/* ========================================================================= */
void MakeAdminWndShopInfo(void)
{
	m_hWndButton1 = CreateWindow ("BUTTON", "←", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 5, 2, 12 * 3, 16, g_hWndAdmin, (HMENU)IDC_BUTTON1, g_hInstance, NULL);
	SendMessage (m_hWndButton1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton2 = CreateWindow ("BUTTON", "→", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 8, 2, 12 * 3, 16, g_hWndAdmin, (HMENU)IDC_BUTTON2, g_hInstance, NULL);
	SendMessage (m_hWndButton2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton6 = CreateWindow ("BUTTON", "取得", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 12, 2, 12 * 3, 16, g_hWndAdmin, (HMENU)IDC_BUTTON6, g_hInstance, NULL);
	SendMessage (m_hWndButton6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton7 = CreateWindow ("BUTTON", "送信", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 16, 2, 12 * 3, 16, g_hWndAdmin, (HMENU)IDC_BUTTON7, g_hInstance, NULL);
	SendMessage (m_hWndButton7, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton8 = CreateWindow ("BUTTON", "追加", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 20, 2, 12 * 3, 16, g_hWndAdmin, (HMENU)IDC_BUTTON8, g_hInstance, NULL);
	SendMessage (m_hWndButton8, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton10 = CreateWindow ("BUTTON", "ｸﾘｱ", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 29, 2, 12 * 3, 16, g_hWndAdmin, (HMENU)IDC_BUTTON10, g_hInstance, NULL);
	SendMessage (m_hWndButton10, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton3 = CreateWindow ("BUTTON", "編集", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 20, 28, 12 * 3, 16, g_hWndAdmin, (HMENU)IDC_BUTTON3, g_hInstance, NULL);
	SendMessage (m_hWndButton3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* メッセージ一覧 */
	LV_COLUMN lvcol;
	m_hWndList1 = CreateWindowEx (WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
			WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS,
			12, 32 + 14, 350, 80, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndList1, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
	ListView_SetExtendedListViewStyle (m_hWndList1 , LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	ZeroMemory (&lvcol, sizeof (lvcol));
	lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol.fmt = LVCFMT_LEFT;

	lvcol.cx = 130;
	lvcol.pszText = "いつ使う？";
	ListView_InsertColumn (m_hWndList1, 0, &lvcol);

	lvcol.cx = 170;
	lvcol.pszText = "内容";
	ListView_InsertColumn (m_hWndList1, 1, &lvcol);

	LPSTR liststr1[] = {"取り引き開始時", "取り引き終了時", "次への繋ぎ",
				"買う時", "所持金不足", "もう持てない時", "買うか確認",
				"買い終わった時", "売る時", "買い取り確認", "買い取り不可",
				"買い取り終わった時",
				"z"};

	LV_ITEM item;
	ZeroMemory (&item, sizeof (item));
	item.mask = LVIF_TEXT;

	int i;
	for (i = 0; ; i ++) {
		if (liststr1[i][0] == 'z') {
			break;
		}
		item.iItem = i;
		item.pszText = liststr1[i];
		ListView_InsertItem (m_hWndList1, &item);
	}

	m_hWndEdit1 = CreateWindow ("EDIT", "", WS_CHILD | ES_AUTOHSCROLL,
			12 * 1, 32 + 14 * 7, 274, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton9 = CreateWindow ("BUTTON", "更新", WS_CHILD | BS_PUSHBUTTON,
			12 * 25, 32 + 14 * 7, 12 * 3, 16, g_hWndAdmin, (HMENU)IDC_BUTTON9, g_hInstance, NULL);
	SendMessage (m_hWndButton9, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	
	/* 売り物一覧 */
	m_hWndButton4 = CreateWindow ("BUTTON", "追加", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			202, 32 + 14 * 12, 12 * 3, 16, g_hWndAdmin, (HMENU)IDC_BUTTON4, g_hInstance, NULL);
	SendMessage (m_hWndButton4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton5 = CreateWindow ("BUTTON", "削除", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			202, 48 + 14 * 12, 12 * 3, 16, g_hWndAdmin, (HMENU)IDC_BUTTON5, g_hInstance, NULL);
	SendMessage (m_hWndButton5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndList2 = CreateWindowEx (WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
			WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS,
			12, 32 + 14 * 10, 182, 80, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndList2, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
	ListView_SetExtendedListViewStyle (m_hWndList2 , LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	ZeroMemory (&lvcol, sizeof (lvcol));
	lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol.fmt = LVCFMT_LEFT;

	lvcol.cx = 30;
	lvcol.pszText = "ID";
	ListView_InsertColumn (m_hWndList2, 0, &lvcol);

	lvcol.cx = 130;
	lvcol.pszText = "アイテム名";
	ListView_InsertColumn (m_hWndList2, 1, &lvcol);

	m_hWndCombo1 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			202, 32 + 14 * 10, 12 * 12, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
	SendMessage (m_hWndCombo1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	for (i = 0; i < g_aItemInfo.GetSize (); i ++) {
		SendMessage (m_hWndCombo1, CB_ADDSTRING, (WPARAM)i, (LPARAM)g_aItemInfo[i]->szName);
	}
	m_dwSelectID = 0;
	PostMessage (g_hWndAdmin, WM_COMMAND, (WPARAM)IDC_BUTTON6, 0);
}

/* ========================================================================= */
/* 関数名：DeleteAdminWndShopInfo											 */
/* 内容：店情報ウィンドウ消去												 */
/* 日付：2003/09/20															 */
/* ========================================================================= */
void DeleteAdminWndShopInfo(void)
{
	SAFE_DESTROYWND (m_hWndButton1);
	SAFE_DESTROYWND (m_hWndButton2);
	SAFE_DESTROYWND (m_hWndButton3);
	SAFE_DESTROYWND (m_hWndButton4);
	SAFE_DESTROYWND (m_hWndButton5);
	SAFE_DESTROYWND (m_hWndButton6);
	SAFE_DESTROYWND (m_hWndButton7);
	SAFE_DESTROYWND (m_hWndButton8);
	SAFE_DESTROYWND (m_hWndButton9);
	SAFE_DESTROYWND (m_hWndButton10);
	SAFE_DESTROYWND (m_hWndList1);
	SAFE_DESTROYWND (m_hWndList2);
	SAFE_DESTROYWND (m_hWndEdit1);
	SAFE_DESTROYWND (m_hWndCombo1);
	m_dwSelectID = 0;
}

/* ========================================================================= */
/* 関数名：MakeAdminWndEfcEdit												 */
/* 内容：エフェクト設定ウィンドウ作成										 */
/* 日付：2003/09/23															 */
/* ========================================================================= */
void MakeAdminWndEfcEdit(void)
{
	m_hWndButton1 = CreateWindow ("BUTTON", "←", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 4, 12 * 7, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON1, g_hInstance, NULL);
	SendMessage (m_hWndButton1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton2 = CreateWindow ("BUTTON", "→", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 7, 12 * 7, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON2, g_hInstance, NULL);
	SendMessage (m_hWndButton2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton3 = CreateWindow ("BUTTON", "取得", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 6, 12 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON3, g_hInstance, NULL);
	SendMessage (m_hWndButton3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton7 = CreateWindow ("BUTTON", "追加", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 9, 12 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON7, g_hInstance, NULL);
	SendMessage (m_hWndButton7, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton4 = CreateWindow ("BUTTON", "追加", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 10, 12 * 7, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON4, g_hInstance, NULL);
	SendMessage (m_hWndButton4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton5 = CreateWindow ("BUTTON", "ｸﾘｱ", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 12, 12 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON5, g_hInstance, NULL);
	SendMessage (m_hWndButton5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton6 = CreateWindow ("BUTTON", "送信", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15, 12 * 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON6, g_hInstance, NULL);
	SendMessage (m_hWndButton6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* ID */
	m_hWndEdit4 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
			12 * 0, 12 * 1, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 画像ID */
	m_hWndEdit1 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
			12 * 9, 12 * 9, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 待ち時間 */
	m_hWndEdit2 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
			12 * 9, 12 * 10 + 4, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	/* 透明度 */
	m_hWndEdit3 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE,
			12 * 9, 12 * 11 + 8, 32, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
	SendMessage (m_hWndEdit3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndScrBar1 = CreateWindow ("SCROLLBAR", "", WS_CHILD | WS_VISIBLE | SBS_HORZ,
			12 * 17, 12 * 5, 32 * 5, 12, g_hWndAdmin, (HMENU)IDC_SCRBAR1, g_hInstance, NULL);
	SetScrollRange (m_hWndScrBar1, SB_CTL, 0, (g_wCountEfcGrp * 16 * 8) - 5, FALSE);

	m_dwSelectID = m_dwSelectID2 = 0;
	SetWindowText (m_hWndEdit1, "0");
#if 0
	PCOMEFCINFO pComEfc = &g_pComEfcInfo[m_dwSelectID];
	PCOMEFCANIMEINFO pAnime = NULL;
	char szTmp[128];
	pAnime = &pComEfc->pAnime[0];
	if (pAnime) {
		wsprintf (szTmp, "%d", pAnime->byGrpID);
		SetWindowText (m_hWndEdit1, szTmp);
		wsprintf (szTmp, "%d", pAnime->byWait);
		SetWindowText (m_hWndEdit2, szTmp);
		wsprintf (szTmp, "%d", pAnime->byLevel);
		SetWindowText (m_hWndEdit3, szTmp);
	}
#endif
	/* 画像種別 */
	m_hWndCombo2 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			12 * 6, 12 * 3 + 2, 12 * 8, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO2, g_hInstance, NULL);
	SendMessage (m_hWndCombo2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	SendMessage (m_hWndCombo2, CB_ADDSTRING, (WPARAM)0, (LPARAM)"16ｘ16");
	SendMessage (m_hWndCombo2, CB_ADDSTRING, (WPARAM)0, (LPARAM)"16ｘ32");

	/* 効果音 */
	m_hWndCombo1 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			12 * 6, 12 * 4 + 10, 12 * 8, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
	SendMessage (m_hWndCombo1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);
	for (int i = 0; ; i ++) {
		if (aszSoundName[i][0] == 'z') {
			break;
		}
		SendMessage (m_hWndCombo1, CB_ADDSTRING, (WPARAM)0, (LPARAM)aszSoundName[i]);
	}

	PostMessage (g_hWndAdmin, WM_COMMAND, (WPARAM)IDC_BUTTON3, 0);
	InvalidateRect (g_hWndAdmin, NULL, TRUE);
}

/* ========================================================================= */
/* 関数名：DeleteAdminWndEfcEdit											 */
/* 内容：エフェクト設定ウィンドウ消去										 */
/* 日付：2003/09/23															 */
/* ========================================================================= */
void DeleteAdminWndEfcEdit(void)
{
	SAFE_DESTROYWND (m_hWndButton1);
	SAFE_DESTROYWND (m_hWndButton2);
	SAFE_DESTROYWND (m_hWndButton3);
	SAFE_DESTROYWND (m_hWndButton4);
	SAFE_DESTROYWND (m_hWndButton5);
	SAFE_DESTROYWND (m_hWndButton6);
	SAFE_DESTROYWND (m_hWndButton7);
	SAFE_DESTROYWND (m_hWndEdit1);
	SAFE_DESTROYWND (m_hWndEdit2);
	SAFE_DESTROYWND (m_hWndEdit3);
	SAFE_DESTROYWND (m_hWndEdit4);
	SAFE_DESTROYWND (m_hWndCombo1);
	SAFE_DESTROYWND (m_hWndCombo2);
	m_dwSelectID = m_dwSelectID2 = 0;
}

/* ========================================================================= */
/* 関数名：MakeAdminWndSubGameInfo											 */
/* 内容：サブゲーム情報ウィンドウ作成										 */
/* 日付：2003/11/08															 */
/* ========================================================================= */
void MakeAdminWndSubGameInfo(void)
{
	m_hWndButton1 = CreateWindow ("BUTTON", "取得", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15 - 32, 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON1, g_hInstance, NULL);
	SendMessage (m_hWndButton1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton2 = CreateWindow ("BUTTON", "追加", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15, 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON2, g_hInstance, NULL);
	SendMessage (m_hWndButton2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton3 = CreateWindow ("BUTTON", "送信", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15 + 32, 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON3, g_hInstance, NULL);
	SendMessage (m_hWndButton3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton4= CreateWindow ("BUTTON", "ｸﾘｱ", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15 + 96, 0, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON4, g_hInstance, NULL);
	SendMessage (m_hWndButton4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCombo1 = CreateWindow ("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
			12 * 15 + 32, 32, 12 * 10, 12 * 10, g_hWndAdmin, (HMENU)IDC_TMPCOMBO1, g_hInstance, NULL);
	SendMessage (m_hWndCombo1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndCheck1 = CreateWindow ("BUTTON", "座標", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
			12 * 5, 16, 64, 16, g_hWndAdmin, (HMENU)IDC_TMPCHECK1, g_hInstance, NULL);
	SendMessage (m_hWndCheck1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton5 = CreateWindow ("BUTTON", "←", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15 - 32, 20, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON5, g_hInstance, NULL);
	SendMessage (m_hWndButton5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	m_hWndButton6= CreateWindow ("BUTTON", "→", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			12 * 15, 20, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON6, g_hInstance, NULL);
	SendMessage (m_hWndButton6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

	int i;
	LPCTSTR liststr[] = {"未設定", "爆弾ゲーム", "迷路", "ダンジョン"};

	for (i = 0; i < SUBGAMEID_MAX; i ++) {
		SendMessage (m_hWndCombo1, CB_ADDSTRING, (WPARAM)0, (LPARAM)liststr[i]);
	}
	SendMessage (m_hWndCombo1, CB_SETCURSEL, 0, 0);
	g_pSubGameInfo = new SUBGAMEINFO;
	ZeroMemory (g_pSubGameInfo, sizeof (SUBGAMEINFO));
	g_nSubGameCount = 0;
	g_nSelectSubGameID = 0;
	InvalidateRect (g_hWndAdmin, NULL, TRUE);
	SendCmdSUBGAMEINFOGET (g_nSelectSubGameID);
}

/* ========================================================================= */
/* 関数名：DeleteAdminWndSubGameInfo										 */
/* 内容：サブゲーム情報ウィンドウ消去										 */
/* 日付：2003/11/08															 */
/* ========================================================================= */
void DeleteAdminWndSubGameInfo(void)
{
	SAFE_DESTROYWND (m_hWndCombo1);
	SAFE_DESTROYWND (m_hWndCheck1);
	SAFE_DESTROYWND (m_hWndButton1);
	SAFE_DESTROYWND (m_hWndButton2);
	SAFE_DESTROYWND (m_hWndButton3);
	SAFE_DESTROYWND (m_hWndButton4);
	SAFE_DESTROYWND (m_hWndButton5);
	SAFE_DESTROYWND (m_hWndButton6);
	if (g_pSubGameInfo) {
		SAFE_DELETE_ARRAY (g_pSubGameInfo->pInfo);
	}
	SAFE_DELETE (g_pSubGameInfo);
	InvalidateRect (g_hWndAdmin, NULL, TRUE);
}

/* ========================================================================= */
/* 関数名：MakeAdminWndSubGameInfoType										 */
/* 内容：種類別サブゲーム情報ウィンドウ作成									 */
/* 日付：2003/11/08															 */
/* ========================================================================= */
void MakeAdminWndSubGameInfoType(void)
{
	if (g_nSubGameCount == 0) {
		return;
	}

	char szTmp[32];
	DeleteAdminWndSubGameInfoType ();
	SendMessage (m_hWndCombo1, CB_SETCURSEL, 0, 0);
	if (g_pSubGameInfo == NULL) {
		return;
	}

	switch (g_pSubGameInfo->byType) {
	case SUBGAMEID_BOMB:		/* 爆弾ゲーム */
		{
			PSUBGAME_BOMBINFOSAVE pBomb = (PSUBGAME_BOMBINFOSAVE)g_pSubGameInfo->pInfo;
			if (pBomb == NULL) {
				break;
			}

			SendMessage (m_hWndCombo1, CB_SETCURSEL, g_pSubGameInfo->byType, 0);
			m_hWndEdit1 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 7, 64 + 16 * 0, 24, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit1, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndEdit2 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 7, 64 + 16 * 1, 24, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit2, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndEdit3 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 7, 64 + 16 * 2, 24, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit3, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndEdit4 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 7, 64 + 16 * 3, 24, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit4, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndEdit5 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 7, 64 + 16 * 4, 24, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit5, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			/* エリア情報 */
			m_hWndEdit6 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 7, 64 + 16 * 5, 24, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit6, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndEdit7 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 10, 64 + 16 * 5, 24, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit7, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndEdit8 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 13, 64 + 16 * 5, 24, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit8, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			m_hWndEdit9 = CreateWindow ("EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER,
					12 * 16, 64 + 16 * 5, 24, 14, g_hWndAdmin, NULL, g_hInstance, NULL);
			SendMessage (m_hWndEdit9, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			wsprintf (szTmp, "%ld", pBomb->dwMapID);
			SetWindowText (m_hWndEdit1, szTmp);
			wsprintf (szTmp, "%ld", pBomb->dwX);
			SetWindowText (m_hWndEdit2, szTmp);
			wsprintf (szTmp, "%ld", pBomb->dwY);
			SetWindowText (m_hWndEdit3, szTmp);
			wsprintf (szTmp, "%ld", pBomb->dwStartX);
			SetWindowText (m_hWndEdit4, szTmp);
			wsprintf (szTmp, "%ld", pBomb->dwStartY);
			SetWindowText (m_hWndEdit5, szTmp);
			wsprintf (szTmp, "%ld", pBomb->dwStartY);

			wsprintf (szTmp, "%d", pBomb->byX1);
			SetWindowText (m_hWndEdit6, szTmp);
			wsprintf (szTmp, "%d", pBomb->byY1);
			SetWindowText (m_hWndEdit7, szTmp);
			wsprintf (szTmp, "%d", pBomb->byX2);
			SetWindowText (m_hWndEdit8, szTmp);
			wsprintf (szTmp, "%d", pBomb->byY2);
			SetWindowText (m_hWndEdit9, szTmp);
		}
		break;

	case SUBGAMEID_MAZE:		/* 迷路 */
		{
			PSUBGAME_MAZEINFOSAVE pMaze = (PSUBGAME_MAZEINFOSAVE)g_pSubGameInfo->pInfo;
			if (pMaze == NULL) {
				break;
			}

			m_hWndButton7 = CreateWindow ("BUTTON", "作成", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					0, 32, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON7, g_hInstance, NULL);
			SendMessage (m_hWndButton7, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			SendMessage (m_hWndCombo1, CB_SETCURSEL, g_pSubGameInfo->byType, 0);
			m_hWndEdit1 = CreateWindowEdit (12 * 7, 64 + 16 * 0, 24, 14, ES_NUMBER);

			/* エリア情報 */
			m_hWndEdit2 = CreateWindowEdit (12 * 7, 64 + 16 * 1, 24, 14, ES_NUMBER);
			m_hWndEdit3 = CreateWindowEdit (12 * 10, 64 + 16 * 1, 24, 14, ES_NUMBER);
			m_hWndEdit4 = CreateWindowEdit (12 * 13, 64 + 16 * 1, 24, 14, ES_NUMBER);
			m_hWndEdit5 = CreateWindowEdit (12 * 16, 64 + 16 * 1, 24, 14, ES_NUMBER);

			/* スタート地点 */
			m_hWndEdit6 = CreateWindowEdit (12 * 7, 64 + 16 * 2, 24, 14, ES_NUMBER);
			m_hWndEdit7 = CreateWindowEdit (12 * 10, 64 + 16 * 2, 24, 14, ES_NUMBER);

			/* ゴール地点 */
			m_hWndEdit8 = CreateWindowEdit (12 * 7, 64 + 16 * 3, 24, 14, ES_NUMBER);
			m_hWndEdit9 = CreateWindowEdit (12 * 10, 64 + 16 * 3, 24, 14, ES_NUMBER);

			/* マップチップ */
			m_hWndEdit10 = CreateWindowEdit (12 * 7, 64 + 16 * 4, 24, 14, ES_NUMBER);
			m_hWndEdit11 = CreateWindowEdit (12 * 7, 64 + 16 * 5, 24, 14, ES_NUMBER);
			m_hWndEdit12 = CreateWindowEdit (12 * 7, 64 + 16 * 6, 24, 14, ES_NUMBER);
			m_hWndEdit13 = CreateWindowEdit (12 * 7, 64 + 16 * 7, 24, 14, ES_NUMBER);

			wsprintf (szTmp, "%ld", pMaze->dwMapID);
			SetWindowText (m_hWndEdit1, szTmp);
			wsprintf (szTmp, "%d", pMaze->byX1);
			SetWindowText (m_hWndEdit2, szTmp);
			wsprintf (szTmp, "%d", pMaze->byY1);
			SetWindowText (m_hWndEdit3, szTmp);
			wsprintf (szTmp, "%d", pMaze->byX2);
			SetWindowText (m_hWndEdit4, szTmp);
			wsprintf (szTmp, "%d", pMaze->byY2);
			SetWindowText (m_hWndEdit5, szTmp);
			wsprintf (szTmp, "%d", pMaze->byStartX);
			SetWindowText (m_hWndEdit6, szTmp);
			wsprintf (szTmp, "%d", pMaze->byStartY);
			SetWindowText (m_hWndEdit7, szTmp);
			wsprintf (szTmp, "%d", pMaze->byGoalToX);
			SetWindowText (m_hWndEdit8, szTmp);
			wsprintf (szTmp, "%d", pMaze->byGoalToY);
			SetWindowText (m_hWndEdit9, szTmp);
			wsprintf (szTmp, "%d", pMaze->wChipIDStart);
			SetWindowText (m_hWndEdit10, szTmp);
			wsprintf (szTmp, "%d", pMaze->wChipIDGoal);
			SetWindowText (m_hWndEdit11, szTmp);
			wsprintf (szTmp, "%d", pMaze->wChipIDRoad);
			SetWindowText (m_hWndEdit12, szTmp);
			wsprintf (szTmp, "%d", pMaze->wChipIDBlock);
			SetWindowText (m_hWndEdit13, szTmp);
		}
		break;

	case SUBGAMEID_DUNGEON:		/* ダンジョン */
		{
			PSUBGAME_DUNGEONINFOSAVE pDungeon = (PSUBGAME_DUNGEONINFOSAVE)g_pSubGameInfo->pInfo;
			if (pDungeon == NULL) {
				break;
			}

			m_hWndButton7 = CreateWindow ("BUTTON", "作成", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					0, 32, 32, 16, g_hWndAdmin, (HMENU)IDC_BUTTON7, g_hInstance, NULL);
			SendMessage (m_hWndButton7, WM_SETFONT, (WPARAM)GetStockObject (DEFAULT_GUI_FONT), 0);

			SendMessage (m_hWndCombo1, CB_SETCURSEL, g_pSubGameInfo->byType, 0);
			m_hWndEdit1 = CreateWindowEdit (12 * 7, 64 + 16 * 0, 24, 14, ES_NUMBER);

			/* エリア情報 */
			m_hWndEdit2 = CreateWindowEdit (12 * 7, 64 + 16 * 1, 24, 14, ES_NUMBER);
			m_hWndEdit3 = CreateWindowEdit (12 * 10, 64 + 16 * 1, 24, 14, ES_NUMBER);
			m_hWndEdit4 = CreateWindowEdit (12 * 13, 64 + 16 * 1, 24, 14, ES_NUMBER);
			m_hWndEdit5 = CreateWindowEdit (12 * 16, 64 + 16 * 1, 24, 14, ES_NUMBER);

			/* スタート地点 */
			m_hWndEdit6 = CreateWindowEdit (12 * 7, 64 + 16 * 2, 24, 14, ES_NUMBER);
			m_hWndEdit7 = CreateWindowEdit (12 * 10, 64 + 16 * 2, 24, 14, ES_NUMBER);

			/* ゴール地点 */
			m_hWndEdit8 = CreateWindowEdit (12 * 7, 64 + 16 * 3, 24, 14, ES_NUMBER);
			m_hWndEdit9 = CreateWindowEdit (12 * 10, 64 + 16 * 3, 24, 14, ES_NUMBER);

			/* マップチップ */
			m_hWndEdit10 = CreateWindowEdit (12 * 7, 64 + 16 * 4, 24, 14, ES_NUMBER);
			m_hWndEdit11 = CreateWindowEdit (12 * 7, 64 + 16 * 5, 24, 14, ES_NUMBER);
			m_hWndEdit12 = CreateWindowEdit (12 * 7, 64 + 16 * 6, 24, 14, ES_NUMBER);
			m_hWndEdit13 = CreateWindowEdit (12 * 7, 64 + 16 * 7, 24, 14, ES_NUMBER);

			wsprintf (szTmp, "%ld", pDungeon->dwMapID);
			SetWindowText (m_hWndEdit1, szTmp);
			wsprintf (szTmp, "%d", pDungeon->byX1);
			SetWindowText (m_hWndEdit2, szTmp);
			wsprintf (szTmp, "%d", pDungeon->byY1);
			SetWindowText (m_hWndEdit3, szTmp);
			wsprintf (szTmp, "%d", pDungeon->byX2);
			SetWindowText (m_hWndEdit4, szTmp);
			wsprintf (szTmp, "%d", pDungeon->byY2);
			SetWindowText (m_hWndEdit5, szTmp);
			wsprintf (szTmp, "%d", pDungeon->byStartX);
			SetWindowText (m_hWndEdit6, szTmp);
			wsprintf (szTmp, "%d", pDungeon->byStartY);
			SetWindowText (m_hWndEdit7, szTmp);
			wsprintf (szTmp, "%d", pDungeon->byGoalX);
			SetWindowText (m_hWndEdit8, szTmp);
			wsprintf (szTmp, "%d", pDungeon->byGoalY);
			SetWindowText (m_hWndEdit9, szTmp);
			wsprintf (szTmp, "%d", pDungeon->wChipIDStart);
			SetWindowText (m_hWndEdit10, szTmp);
			wsprintf (szTmp, "%d", pDungeon->wChipIDGoal);
			SetWindowText (m_hWndEdit11, szTmp);
			wsprintf (szTmp, "%d", pDungeon->wChipIDRoad);
			SetWindowText (m_hWndEdit12, szTmp);
			wsprintf (szTmp, "%d", pDungeon->wChipIDBlock);
			SetWindowText (m_hWndEdit13, szTmp);
		}
		break;
	}
}

/* ========================================================================= */
/* 関数名：DeleteAdminWndSubGameInfoType									 */
/* 内容：種類別サブゲーム情報ウィンドウ消去									 */
/* 日付：2003/11/08															 */
/* ========================================================================= */
void DeleteAdminWndSubGameInfoType(void)
{
	SAFE_DESTROYWND (m_hWndButton7);
	SAFE_DESTROYWND (m_hWndEdit1);
	SAFE_DESTROYWND (m_hWndEdit2);
	SAFE_DESTROYWND (m_hWndEdit3);
	SAFE_DESTROYWND (m_hWndEdit4);
	SAFE_DESTROYWND (m_hWndEdit5);
	SAFE_DESTROYWND (m_hWndEdit6);
	SAFE_DESTROYWND (m_hWndEdit7);
	SAFE_DESTROYWND (m_hWndEdit8);
	SAFE_DESTROYWND (m_hWndEdit9);
	SAFE_DESTROYWND (m_hWndEdit10);
	SAFE_DESTROYWND (m_hWndEdit11);
	SAFE_DESTROYWND (m_hWndEdit12);
	SAFE_DESTROYWND (m_hWndEdit13);
}

/* Copyright(C)URARA-works 2003-2005 */
