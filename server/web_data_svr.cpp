/* ========================================================================= */
/* ファイル名：	web_data_svr.cpp											 */
/* 概要：		AI 支援用のデータ API（ゲームの設定データを JSON で読み書き）	 */
/* ========================================================================= */
/*
 * 管理者ウィンドウで編集している「設定データ」を、HTTP の API（と MCP）から
 * 読み書きできるようにする。キー（AI 用キー）の確認・下見・変更履歴・元に戻すは
 * Node.js 側（server/node/sbosvr_data.js）が持ち、ここはデータそのものを扱う。
 *
 *   JS → C : SboWebDataSchema  扱えるデータの種類と項目の一覧（JSON）
 *            SboWebDataCount   件数
 *            SboWebDataGet     1 件を JSON で返す（無ければ "null"）
 *            SboWebDataApply   変更（JSON のオブジェクト）を確かめ、bApply なら反映する
 *                              id が件数と同じなら新しく追加する
 *
 * 反映した時は、管理者ウィンドウから変えた時と同じようにログイン中の全員へ送り、
 * そのデータのファイル（SBODATA/SboData*.dat）をすぐ保存する。
 * 文字列はゲームの中でも UTF-8 で持っている（JS とも UTF-8 でやり取りする）。
 */
#include "SboSvr/src/include.h"
#include "Glbldata.h"
#include "Info/InfoJob.h"
#include "SboSvrRecvCmd.h"
#include "SboSvrSendCmd.h"
#include "SboSvrAdminUndo.h"
#include "SboSvrNpcRoute.h"
#include "SboSvrRecycle.h"
#include "SboSvrMarket.h"
#include "SboSvrMapSwapEdit.h"
#include "SboSvrDungeonDaily.h"
#include "SboSvrSubGameProc.h"
#include "CharProc/IncludeCharProc.h"

#include <emscripten.h>
#include <string>
#include <vector>
#include <algorithm>
#include <map>

extern BOOL SaveItemInfo(void);
extern BOOL SaveStTmpInfo(void);
extern BOOL SaveJobInfo(void);
extern BOOL SaveMagicInfo(void);
extern BOOL SaveMagicUserInfo(void);
extern BOOL SaveEquUserInfo(void);
extern BOOL SaveMakerInfo(void);
extern BOOL SaveAffixInfo(void);
extern BOOL SaveRecipeInfo(void);
extern BOOL SaveQuestInfo(void);
extern BOOL SaveFlagNameInfo(void);
extern BOOL SaveTalkInfo(void);
extern BOOL SaveStoryInfo(void);
extern BOOL SaveFlagGroupInfo(void);
extern int GetMapNo(DWORD);
extern BOOL SaveCharInfo(void);
extern BOOL RecvCmdApplyAdminPacket(const BYTE *pData, DWORD dwSize, LPCSTR pszSource, BYTE byScreen);	/* SboSvrRecvCmd.cpp */
extern void ApplyStTmpToChars(DWORD dwID);		/* SboSvrRecvCmd.cpp */

/* ------------------------------------------------------------------------- */
/* 小さな JSON 読み取り（JS から渡される変更内容だけ読めればよい）				 */
/* ------------------------------------------------------------------------- */
struct JVal {
	enum { NUL, BOOL_, NUM, STR, ARR, OBJ } t;
	double n;
	bool b;
	std::string s;
	std::vector<JVal> a;
	std::vector<std::pair<std::string, JVal> > o;

	JVal() : t(NUL), n(0), b(false) {}
	const JVal *Get(const char *pszKey) const {
		for (size_t i = 0; i < o.size(); i ++) {
			if (o[i].first == pszKey) {
				return &o[i].second;
			}
		}
		return NULL;
	}
};

class CJsonReader
{
public:
	CJsonReader(const char *psz) : m_p(psz), m_nDepth(0) {}

	bool Parse(JVal &v) {
		if (!Value(v)) {
			return false;
		}
		Ws();
		return *m_p == 0;
	}

private:
	const char *m_p;
	int m_nDepth;

	void Ws() {
		while (*m_p == ' ' || *m_p == '\t' || *m_p == '\r' || *m_p == '\n') {
			m_p ++;
		}
	}
	static void PutUtf8(std::string &s, unsigned int c) {
		if (c < 0x80) {
			s += (char)c;
		} else if (c < 0x800) {
			s += (char)(0xC0 | (c >> 6));
			s += (char)(0x80 | (c & 0x3F));
		} else if (c < 0x10000) {
			s += (char)(0xE0 | (c >> 12));
			s += (char)(0x80 | ((c >> 6) & 0x3F));
			s += (char)(0x80 | (c & 0x3F));
		} else {
			s += (char)(0xF0 | (c >> 18));
			s += (char)(0x80 | ((c >> 12) & 0x3F));
			s += (char)(0x80 | ((c >> 6) & 0x3F));
			s += (char)(0x80 | (c & 0x3F));
		}
	}
	bool Hex4(unsigned int &c) {
		c = 0;
		for (int i = 0; i < 4; i ++) {
			char ch = *m_p++;
			c <<= 4;
			if (ch >= '0' && ch <= '9') c |= ch - '0';
			else if (ch >= 'a' && ch <= 'f') c |= ch - 'a' + 10;
			else if (ch >= 'A' && ch <= 'F') c |= ch - 'A' + 10;
			else return false;
		}
		return true;
	}
	bool Str(std::string &s) {
		if (*m_p != '"') {
			return false;
		}
		m_p ++;
		while (*m_p && *m_p != '"') {
			if (*m_p == '\\') {
				m_p ++;
				char ch = *m_p++;
				switch (ch) {
				case '"': s += '"'; break;
				case '\\': s += '\\'; break;
				case '/': s += '/'; break;
				case 'b': s += '\b'; break;
				case 'f': s += '\f'; break;
				case 'n': s += '\n'; break;
				case 'r': s += '\r'; break;
				case 't': s += '\t'; break;
				case 'u': {
						unsigned int c, c2;
						if (!Hex4(c)) {
							return false;
						}
						if (c >= 0xD800 && c <= 0xDBFF) {
							if (m_p[0] != '\\' || m_p[1] != 'u') {
								return false;
							}
							m_p += 2;
							if (!Hex4(c2) || c2 < 0xDC00 || c2 > 0xDFFF) {
								return false;
							}
							c = 0x10000 + ((c - 0xD800) << 10) + (c2 - 0xDC00);
						}
						PutUtf8(s, c);
					}
					break;
				default:
					return false;
				}
			} else {
				s += *m_p++;
			}
		}
		if (*m_p != '"') {
			return false;
		}
		m_p ++;
		return true;
	}
	bool Value(JVal &v) {
		Ws();
		if (++m_nDepth > 16) {
			return false;
		}
		bool bRet = false;
		char ch = *m_p;
		if (ch == '{') {
			v.t = JVal::OBJ;
			m_p ++;
			Ws();
			if (*m_p == '}') {
				m_p ++;
				bRet = true;
			} else {
				for (;;) {
					std::pair<std::string, JVal> kv;
					Ws();
					if (!Str(kv.first)) break;
					Ws();
					if (*m_p++ != ':') break;
					if (!Value(kv.second)) break;
					v.o.push_back(kv);
					Ws();
					if (*m_p == ',') { m_p ++; continue; }
					if (*m_p == '}') { m_p ++; bRet = true; }
					break;
				}
			}
		} else if (ch == '[') {
			v.t = JVal::ARR;
			m_p ++;
			Ws();
			if (*m_p == ']') {
				m_p ++;
				bRet = true;
			} else {
				for (;;) {
					JVal e;
					if (!Value(e)) break;
					v.a.push_back(e);
					Ws();
					if (*m_p == ',') { m_p ++; continue; }
					if (*m_p == ']') { m_p ++; bRet = true; }
					break;
				}
			}
		} else if (ch == '"') {
			v.t = JVal::STR;
			bRet = Str(v.s);
		} else if (!strncmp(m_p, "true", 4)) {
			v.t = JVal::BOOL_; v.b = true; m_p += 4; bRet = true;
		} else if (!strncmp(m_p, "false", 5)) {
			v.t = JVal::BOOL_; v.b = false; m_p += 5; bRet = true;
		} else if (!strncmp(m_p, "null", 4)) {
			v.t = JVal::NUL; m_p += 4; bRet = true;
		} else if (ch == '-' || (ch >= '0' && ch <= '9')) {
			char *pEnd = NULL;
			v.t = JVal::NUM;
			v.n = strtod(m_p, &pEnd);
			bRet = (pEnd != m_p);
			m_p = pEnd;
		}
		m_nDepth --;
		return bRet;
	}
};

/* ------------------------------------------------------------------------- */
/* JSON を書く																 */
/* ------------------------------------------------------------------------- */
static std::string JsonStr(const char *psz)
{
	std::string out = "\"";
	for (const unsigned char *p = (const unsigned char *)(psz ? psz : ""); *p; p ++) {
		unsigned char c = *p;
		if (c == '"' || c == '\\') {
			out += '\\';
			out += (char)c;
		} else if (c == '\n') {
			out += "\\n";
		} else if (c == '\r') {
			out += "\\r";
		} else if (c == '\t') {
			out += "\\t";
		} else if (c < 0x20) {
			char sz[8];
			snprintf(sz, sizeof(sz), "\\u%04x", c);
			out += sz;
		} else {
			out += (char)c;
		}
	}
	out += "\"";
	return out;
}

static std::string JsonNum(long long n)
{
	char sz[32];
	snprintf(sz, sizeof(sz), "%lld", n);
	return sz;
}

/* ------------------------------------------------------------------------- */
/* 項目の表																	 */
/* ------------------------------------------------------------------------- */
enum {
	FT_U8 = 0,		/* BYTE */
	FT_S8,			/* char（-128〜127） */
	FT_U16,			/* WORD */
	FT_U32,			/* DWORD */
	FT_STR,			/* 決まった長さの char 配列（UTF-8。nSize は終端込みのバイト数） */
	FT_U8ARR,		/* BYTE の配列（nSize は要素数） */
	FT_U16ARR,		/* WORD の配列（nSize は要素数） */
};

#define FF_READONLY		(0x01)		/* 読むだけ */
#define FF_MULTILINE	(0x02)		/* 改行を入れてよい */
#define FF_PADMAX		(0x04)		/* 配列の「無し」が最大値（短い配列の残りを 0 ではなく 255／65535 で埋める。フラグの番号の配列用） */

typedef struct _DATAFIELD {
	const char		*pszName;		/* 項目名（JSON のキー。ソースの変数名と同じ） */
	const char		*pszLabel;		/* 日本語の名前 */
	int				nType;
	size_t			nOffset;
	size_t			nSize;
	int				nFlags;
	const char		*pszNote;		/* AI 向けの説明（値の意味など。NULL 可） */
} DATAFIELD, *PDATAFIELD;

#define FLD(st, name, label, type, flags, note)	{ #name, label, type, offsetof(st, name), 0, flags, note }
#define FLDS(st, name, label, type, flags, note)	{ #name, label, type, offsetof(st, name), sizeof(((st *)0)->name), flags, note }
#define FLDA(st, name, label, type, elem, flags, note)	{ #name, label, type, offsetof(st, name), sizeof(((st *)0)->name) / sizeof(elem), flags, note }

/* JSON のキーをメンバー名と別にしたい時（メンバー名を変えても、保存済みの JSON や既存のページのキーを変えないため） */
#define FLDK(st, name, key, label, type, flags, note)	{ key, label, type, offsetof(st, name), 0, flags, note }

static const char *s_pszNoteMagicType =
	"0=効果無し 1=HP減少 2=HP増加 3=明かり 4=魔法発生 5=帰還 6=HP減少(攻撃力) 7=魔法発生(自分) 8=蘇生 9=HP減少(HP消費) 10=HP減少(率) 11=加速(データ1=速くなる率%・2=秒) 12=毒(データ1=2秒ごとのダメージ・2=秒・3=成功率%) 13=眠り(データ1=秒・2=成功率%) 14=攻撃力アップ(データ1=上がる率%・2=秒・3=最低でも上がる量、プレイヤーだけ) 15=守備力アップ(同じ) 16=隠れる(データ2=秒、自分にかける。敵が見つけられる範囲が半分になり、攻撃すると解ける)";
static const char *s_pszNoteAttr = "0=無し 1=火 2=水 3=風 4=土 5=光 6=闇";

/* 特技（魔法）情報 */
static const DATAFIELD s_aFieldMagic[] = {
	FLDS(MAGICINFO, szName,				"名前",					FT_STR, 0, "範囲は名前の括弧に書く慣習（例: 炎(前方)）"),
	FLD (MAGICINFO, byType,				"効果種別",				FT_U8, 0, s_pszNoteMagicType),
	FLD (MAGICINFO, byTypeInfoID,		"種別情報ID",			FT_U8, 0, "効果種別ごとの付加情報（状態変化の種類など）"),
	FLD (MAGICINFO, byUserInfo,			"使える職業",			FT_U8, 0, "magicUser（特技を使える職業）の番号"),
	FLD (MAGICINFO, byTarget,			"使用対象",				FT_U8, 0, "0=PC 1=NPC 2=両方"),
	FLD (MAGICINFO, byTargetType,		"発動種別",				FT_U8, 0, "0=自分 1=前方 2=周囲 3=画面内 4=前方横一列 5=選択 6=選択(自分以外)"),
	FLD (MAGICINFO, byTargetArea,		"発動範囲",				FT_U8, 0, NULL),
	FLD (MAGICINFO, byTargetCount,		"発動人数",				FT_U8, 0, "0=一人 1=全員"),
	FLD (MAGICINFO, byTargetEfectID,	"エフェクト番号(相手)",	FT_U8, 0, NULL),
	FLD (MAGICINFO, byUserEfectID,		"エフェクト番号(利用者)",	FT_U8, 0, NULL),
	FLD (MAGICINFO, byTime,				"詠唱時間(秒)",			FT_U8, 0, NULL),
	FLD (MAGICINFO, byAttr,				"属性",					FT_U8, 0, s_pszNoteAttr),
	FLD (MAGICINFO, wData1,				"付加データ1",			FT_U16, 0, "意味は効果種別による。HP減少/増加は効果量の最小、率系は%、蘇生は復活時HP%、魔法発生は発生させる特技の番号"),
	FLD (MAGICINFO, wData2,				"付加データ2",			FT_U16, 0, "意味は効果種別による。HP減少/増加は効果量の最大、魔法発生は残る秒数"),
	FLD (MAGICINFO, wData3,				"付加データ3",			FT_U16, 0, "意味は効果種別による"),
	FLD (MAGICINFO, wData4,				"付加データ4",			FT_U16, 0, "意味は効果種別による。魔法発生は 1 で前へ飛ぶ"),
	FLD (MAGICINFO, wMana,				"消費Mana",				FT_U16, 0, NULL),
	FLDS(MAGICINFO, szExplain,			"説明文",				FT_STR, FF_MULTILINE, "店で特技の書を見た時に出る説明。空なら数値から自動で作る"),
};

/* 特技を使える職業・装備できる職業（名前だけ。職業の一覧は別に扱う） */
static const DATAFIELD s_aFieldMagicUser[] = {
	FLDS(MAGICUSERINFO, szName,			"名前",					FT_STR, 0, NULL),
};
static const DATAFIELD s_aFieldEquUser[] = {
	FLDS(EQUUSERINFO, szName,			"名前",					FT_STR, 0, NULL),
};

/* 職業情報（名前とレベルアップ情報は別に扱う） */
static const DATAFIELD s_aFieldJob[] = {
	FLD (JOBINFO, dwJobID,				"職業ID",				FT_U32, FF_READONLY, "職業情報の内部ID。キャラの職業や jobIds で使うのはこれではなく id（番号）の方"),
	FLD (JOBINFO, bySkillType,			"特技種別",				FT_U8, 0, NULL),
	FLD (JOBINFO, byMagicType,			"魔法種別",				FT_U8, 0, NULL),
	FLD (JOBINFO, byEquType,			"装備種別",				FT_U8, 0, NULL),
	FLD (JOBINFO, dwFlg,				"フラグ",				FT_U32, 0, "1=アイテム情報が見れる"),
};

/* レベルアップ情報（1 件目が Lv1 の初期値、n 件目がレベル n になるのに必要な経験値と各能力の上限） */
static const DATAFIELD s_aFieldLvUp[] = {
	FLD (LVUPINFO, byPW,				"力の上限",				FT_U8, 0, NULL),
	FLD (LVUPINFO, byDEX,				"素早さの上限",			FT_U8, 0, NULL),
	FLD (LVUPINFO, byST,				"体力の上限",			FT_U8, 0, NULL),
	FLD (LVUPINFO, byIT,				"賢さの上限",			FT_U8, 0, NULL),
	FLD (LVUPINFO, byLK,				"運のよさの上限",		FT_U8, 0, NULL),
	FLD (LVUPINFO, wMAXHP,				"最大HPの上限",			FT_U16, 0, NULL),
	FLD (LVUPINFO, wMAXMANA,			"最大Manaの上限",		FT_U16, 0, NULL),
	FLD (LVUPINFO, dwExp,				"必要経験値",			FT_U32, 0, NULL),
};

/* ステータステンプレート（NPC・敵の能力のひな形） */
static const DATAFIELD s_aFieldStTmp[] = {
	FLDS(STATUSTEMPLATEINFO, szName,		"名前",				FT_STR, 0, NULL),
	FLD (STATUSTEMPLATEINFO, byLevel,		"レベル",			FT_U8, 0, NULL),
	FLD (STATUSTEMPLATEINFO, byPW,			"力",				FT_U8, 0, NULL),
	FLD (STATUSTEMPLATEINFO, byDEX,			"素早さ",			FT_U8, 0, NULL),
	FLD (STATUSTEMPLATEINFO, byST,			"体力",				FT_U8, 0, NULL),
	FLD (STATUSTEMPLATEINFO, byIT,			"賢さ",				FT_U8, 0, NULL),
	FLD (STATUSTEMPLATEINFO, byLK,			"運のよさ",			FT_U8, 0, NULL),
	FLD (STATUSTEMPLATEINFO, byNpcType,		"NPC種別",			FT_U8, 0, "0=未設定 1=移動しない 2=ランダム移動 11=戦闘1 12=検索して追尾 13=戦闘2 24=宝箱の敵（ふだんは宝箱のふり。調べると正体を現して戦闘2になる） など"),
	FLD (STATUSTEMPLATEINFO, byDropProbability,	"ドロップ率",	FT_U8, 0, NULL),
	FLD (STATUSTEMPLATEINFO, byMoveProbabilityX,	"移動確率X",	FT_U8, 0, NULL),
	FLD (STATUSTEMPLATEINFO, byMoveProbabilityY,	"移動確率Y",	FT_U8, 0, NULL),
	FLD (STATUSTEMPLATEINFO, bySearchCx,	"検索範囲X",		FT_U8, 0, NULL),
	FLD (STATUSTEMPLATEINFO, bySearchCy,	"検索範囲Y",		FT_U8, 0, NULL),
	FLDA(STATUSTEMPLATEINFO, abyMagic,		"使う特技",			FT_U8ARR, BYTE, 0, "magic の番号の並び（0 は無し）"),
	FLD (STATUSTEMPLATEINFO, chAttrFire,	"属性(火)",			FT_S8, 0, NULL),
	FLD (STATUSTEMPLATEINFO, chAttrWater,	"属性(水)",			FT_S8, 0, NULL),
	FLD (STATUSTEMPLATEINFO, chAttrWind,	"属性(風)",			FT_S8, 0, NULL),
	FLD (STATUSTEMPLATEINFO, chAttrEarth,	"属性(土)",			FT_S8, 0, NULL),
	FLD (STATUSTEMPLATEINFO, chAttrLight,	"属性(光)",			FT_S8, 0, NULL),
	FLD (STATUSTEMPLATEINFO, chAttrDark,	"属性(闇)",			FT_S8, 0, NULL),
	FLD (STATUSTEMPLATEINFO, wMaxHP,		"最大HP",			FT_U16, 0, NULL),
	FLD (STATUSTEMPLATEINFO, wMaxMana,		"最大Mana",			FT_U16, 0, NULL),
	FLD (STATUSTEMPLATEINFO, wDFMagic,		"魔法防御",			FT_U16, 0, NULL),
	FLDA(STATUSTEMPLATEINFO, waItem,		"所持アイテム",		FT_U16ARR, WORD, 0, "item の番号の並び（0 は無し）"),
	FLD (STATUSTEMPLATEINFO, dwGold,		"所持金",			FT_U32, 0, NULL),
	FLD (STATUSTEMPLATEINFO, dwExp,			"経験値",			FT_U32, 0, NULL),
	FLD (STATUSTEMPLATEINFO, byStatusResist,	"毒・眠りの効きにくさ",	FT_U8, 0, "%。特技の成功率をこの分下げる。100 で効かない（ボス向け）"),
	FLD (STATUSTEMPLATEINFO, byGhost,		"半透明（霊）",		FT_U8, 0, "1 にすると、この元から出る敵（NPC発生・依頼の敵）が半透明（霊）で描かれる。見た目だけで、敵に見つかりにくくなるなどはしない。0=ふつう"),
	FLD (STATUSTEMPLATEINFO, byAffixRate,	"付与が付く確率(%)",	FT_U8, 0, "この敵が装備（武器〜指輪）を落とした時に、付与（接頭辞・接尾辞）が付く確率。付いた物は未鑑定で落ちる。0〜100。古いデータは 10。付与表（affix）が 0 件なら付かない"),
	FLD (STATUSTEMPLATEINFO, byAffixRank,	"付与の格の最大",	FT_U8, 0, "付く付与の格（affix の byRank）の上限。1〜3。古いデータは 1"),
	FLD (STATUSTEMPLATEINFO, byNoFloorDamage,	"ダメージ床が効かない",	FT_U8, 0, "1 にすると毒沼・溶岩などのダメージ床でHPが減らず毒にもならず、よけずに素通りして追いかける（ボス向け）。0=ふつう（ダメージ床をよける）"),
};

/* アイテム情報（共通の項目。使った時のメッセージ・種別ごとの詳細は別に扱う） */
static const char *s_pszNoteItemType =
	"0=効果無し 1=特定位置へ移動 2=HP回復 3=武器 4=鎧 5=盾 6=兜 7=種 8=サブゲーム参加 9=酒 10=メッセージ表示 "
	"11=キャラ画像変更 12=明かり 13=HP減少 14=魔法習得(特技の書) 15=Mana回復 16=HP・Mana回復 17=小手 18=靴 "
	"19=首飾り 20=指輪1 21=指輪2 22=蘇生 23=特技発動(使うと特技が出る道具)。変えると detail は 0 から作り直す";
static const DATAFIELD s_aFieldItem[] = {
	FLDS(ITEMINFO, szName,				"名前",					FT_STR, 0, NULL),
	FLD (ITEMINFO, byType,				"種別",					FT_U8, 0, s_pszNoteItemType),
	FLD (ITEMINFO, byData,				"付加データ",			FT_U8, 0, "武器は攻撃力、防具は守備力（首飾り・指輪は detail.byEquType の効果量）"),
	FLD (ITEMINFO, byFlg,				"フラグ",				FT_U8, 0, "ビット: 1=捨てられない 2=使うとなくなる 4=取り引き不可 8=大事な物（手持ちではなく大事な物の欄に入る。依頼の品も同じ欄）"),
	FLD (ITEMINFO, bySound,				"使った時の音",			FT_U8, 0, NULL),
	FLD (ITEMINFO, bySexDistinction,	"性別",					FT_U8, 0, "0=全員 1=男性のみ 2=女性のみ"),
	FLD (ITEMINFO, byEraseProbability,	"無くなる確率",			FT_U8, 0, NULL),
	FLD (ITEMINFO, wGrpID,				"画像ID",				FT_U16, 0, NULL),
	FLD (ITEMINFO, dwGold,				"値段",					FT_U32, 0, NULL),
};

/* 作り手情報（町の作り手NPC（料理人など）。NPC の台詞 "@cNN" の NN がこの番号） */
static const char *s_pszNoteRecipeState = "0=削除済み（空き枠。新しく足す時はここを使い回す） 1=有効 2=停止中（一覧に出ない）";
static const DATAFIELD s_aFieldMaker[] = {
	FLD (MAKERINFO, byState,			"状態",					FT_U8, 0, s_pszNoteRecipeState),
	FLD (MAKERINFO, byKind,				"種類",					FT_U8, 0, "0=レシピで作る（今までの作り手） 1=鑑定屋（手持ちの未鑑定の装備を、max(10, 元の値段/10) ゴールドで鑑定する。台詞は szMsgTalk=話しかけた時・szMsgDone=鑑定できた時・szMsgLack=鑑定する物が無い時。レシピは使わない）"),
	FLDS(MAKERINFO, szName,				"名前",					FT_STR, 0, "例: 料理人"),
	FLDS(MAKERINFO, szMsgTalk,			"話しかけた時の台詞",	FT_STR, FF_MULTILINE, "NPC の台詞と同じ制御コード（@0 入力待ち・@1 改行・@2 点滅無しで入力待ち）が使える"),
	FLDS(MAKERINFO, szMsgDone,			"作った時の台詞",		FT_STR, FF_MULTILINE, NULL),
	FLDS(MAKERINFO, szMsgLack,			"材料が足りない時の台詞",	FT_STR, FF_MULTILINE, NULL),
};

/* 付与情報（装備に付く接頭辞・接尾辞。docs/item-affix.md） */
static const DATAFIELD s_aFieldAffix[] = {
	FLD (ITEMAFFIXINFO, wID,			"付与番号",				FT_U16, FF_READONLY, "足す時に自動で振られる（1〜。派生アイテムの wPre・wSuf がこの番号で指すので変わらない・使い回さない）。id（配列の番号）とは別。消せない（出さないなら wWeight=0）"),
	FLD (ITEMAFFIXINFO, byPos,			"位置",					FT_U8, 0, "0=名前の前に付く（鋭い） 1=名前の後ろに付く（の加護）"),
	FLDS(ITEMAFFIXINFO, szName,			"名前",					FT_STR, 0, "例: 鋭い・腐った・＋1・の加護"),
	FLD (ITEMAFFIXINFO, wEquMask,		"付く装備種別",			FT_U16, 0, "ビット: 1=武器 2=鎧 4=盾 8=兜 16=小手 32=靴 64=首飾り 128=指輪"),
	FLD (ITEMAFFIXINFO, chRate,			"主の値への%",			FT_S8, 0, "主の値（武器は攻撃力、防具・首飾り・指輪は守備力か魔防＝item の byData）に対する%。負も可。前と後ろは合算"),
	FLD (ITEMAFFIXINFO, chAdd,			"主の値への固定加算",	FT_S8, 0, "主の値への固定加算。負も可。最終値 = byData + byData×合計Rate/100 + 合計Add を 0〜255 に収める"),
	FLD (ITEMAFFIXINFO, chDFMagic,		"魔防加算",				FT_S8, 0, "プレイヤーの魔法防御への加算（負も可）"),
	FLD (ITEMAFFIXINFO, byRank,			"格",					FT_U8, 0, "1〜3。敵の byAffixRank 以下の格だけ出る"),
	FLD (ITEMAFFIXINFO, wWeight,		"出やすさ",				FT_U16, 0, "くじの重み。0 なら出ない"),
	FLD (ITEMAFFIXINFO, wGoldRate,		"売値の倍率(%)",		FT_U16, 0, "100=そのまま。鑑定済みの売値 = 元の値段 × 前後の倍率の積"),
	FLD (ITEMAFFIXINFO, byBad,			"ハズレ印",				FT_U8, 0, "1 なら表示でハズレと分かる印を付ける（表示用）"),
	FLD (ITEMAFFIXINFO, wNext,			"次の段",				FT_U16, 0, "鍛えた時に置き換わる次の段の付与（affix の wID。0=これ以上は鍛えられない）。＋1→＋2→＋3 のように、位置が同じ（後ろ）の付与を指す"),
};

/* 派生アイテム情報（読むだけ。アイテム番号 16384 以降 = 行番号 + 16384） */
static const DATAFIELD s_aFieldItemDerive[] = {
	FLD (ITEMDERIVEINFO, wBase,			"元のアイテム",			FT_U16, FF_READONLY, "item の番号"),
	FLD (ITEMDERIVEINFO, wPre,			"前の付与",				FT_U16, FF_READONLY, "affix の wID（0=なし）"),
	FLD (ITEMDERIVEINFO, wSuf,			"後ろの付与",			FT_U16, FF_READONLY, "affix の wID（0=なし）"),
	FLD (ITEMDERIVEINFO, byUnknown,		"未鑑定",				FT_U8, FF_READONLY, "1=未鑑定（付与の効果は出ない）"),
};

/* レシピ情報（作り手に頼んで、材料からアイテムを作ってもらう） */
static const DATAFIELD s_aFieldRecipe[] = {
	FLD (RECIPEINFO, byState,			"状態",					FT_U8, 0, s_pszNoteRecipeState),
	FLD (RECIPEINFO, byMaker,			"作り手",				FT_U8, 0, "maker の番号"),
	FLD (RECIPEINFO, wSort,				"並び順",				FT_U16, 0, "作り手の画面の一覧で小さい順に並ぶ（同じなら番号順）"),
	FLD (RECIPEINFO, wItem,				"できるアイテム",		FT_U16, 0, "item の番号"),
	FLD (RECIPEINFO, byCount,			"できる個数",			FT_U8, 0, "1 以上。持ち物に入りきらない分は預かり所へ送る"),
	FLDA(RECIPEINFO, awMatItem,			"材料のアイテム",		FT_U16ARR, WORD, 0, "item の番号を最大 4 種（0 は使わない欄）。1 種以上必要。持ち物（装備中・捨てられない物を除く）を先に、足りない分を預かり所から使う"),
	FLDA(RECIPEINFO, abyMatCount,		"材料の個数",			FT_U8ARR, BYTE, 0, "awMatItem と同じ並びの個数（アイテムが 0 の欄は 0 にする）"),
	FLD (RECIPEINFO, dwGold,			"手数料",				FT_U32, 0, "作ってもらう時に払うゴールド"),
	FLD (RECIPEINFO, byKind,			"種類",					FT_U8, 0, "0=普通（材料から wItem を byCount 個作る） 1=鍛える（手持ちの装備 1 つの後ろの付与を、あれば付与の wNext の段へ進める。無ければ wAffix を付ける。wAffix は位置が後ろ（byPos=1）の付与。wNext が 0 なら「これ以上は鍛えられない」） 2=付け替える（対象の wAffix の位置（前/後ろ）の付与を wAffix に置き換える。無ければ付ける。同じなら断る）。1・2 は wItem・byCount を使わない（できる個数は 1 固定）。対象は鑑定済みの派生装備（未鑑定は断る）。材料と手数料は普通と同じ。詳しくは docs/recipe.md"),
	FLD (RECIPEINFO, wAffix,			"付ける付与",			FT_U16, 0, "種類が 1・2 の時に付ける付与（affix の wID）。種類 0 では 0"),
	FLD (RECIPEINFO, byAllowPlain,		"普通の装備も対象",		FT_U8, 0, "1=付与の無い普通の装備も対象にしてよい（例: 普通の剣→剣＋1）。0=鑑定済みの派生装備だけ"),
	FLD (RECIPEINFO, wEquMask,			"対象の装備種別",		FT_U16, 0, "対象にする装備種別のビット（1=武器 2=鎧 4=盾 8=兜 16=小手 32=靴 64=首飾り 128=指輪）。0=付与側（affix の wEquMask）に従う。付与の wEquMask に無い種別は常に対象外"),
};

/* フラグの名前表（印・数値の番号に付ける名前。管理用でプレイヤーには出ない） */
static const DATAFIELD s_aFieldFlagName[] = {
	FLD (FLAGNAMEINFO, byState,			"状態",					FT_U8, 0, "0=削除済み（枠だけ残る。同じ種類・番号をもう一度使う時は、足さずにこの枠の byState を 1 に戻す） 1=有効"),
	FLD (FLAGNAMEINFO, byKind,			"種類",					FT_U8, 0, "0=印（ON/OFF） 1=数値（0〜255）。同じ種類・番号の組は 1 件まで"),
	FLD (FLAGNAMEINFO, wNo,				"番号",					FT_U16, 0, "印は 0〜1023、数値は 0〜127。割り振りの目安: 印は 1〜199 本筋・200〜599 地域・600〜899 お使い・900〜1023 予備。数値は 0〜127"),
	FLDS(FLAGNAMEINFO, szName,			"名前",					FT_STR, 0, "全角 20 字ほど（UTF-8 で最大 95 バイト）。例: 「港町の事件を解決した」"),
};

/* フラグのグループ（印・数値の番号の帯に「メイン名」を付ける。番号 n が帯 wFrom〜wTo に入るなら、メイン=グループ・サブ=n-wFrom+1 で「ルシー-3」と呼べる。
   見せ方・入力・絞り込みだけの層で、会話データ・依頼・NPC の見える条件・思い出の本などが覚えている番号は変わらない） */
static const DATAFIELD s_aFieldFlagGroup[] = {
	FLD (FLAGGROUPINFO, wID,			"グループ番号",			FT_U16, FF_READONLY, "足す時に自動で振られる（1〜255。変わらない）。管理者ウィンドウの「グループ番号-サブ」の入力（例 3-2）はこの番号で指す。id（配列の番号）とは別"),
	FLD (FLAGGROUPINFO, byState,		"状態",					FT_U8, 0, "0=削除済み（枠だけ残る。帯はほかのグループが使える） 1=有効。足す時に省くと 1"),
	FLD (FLAGGROUPINFO, byKind,			"種類",					FT_U8, 0, "0=印（ON/OFF） 1=数値（0〜255）。同じ種類で有効なグループの帯が重なるのは不可（足す時に省くと 0）"),
	FLDS(FLAGGROUPINFO, szName,			"メイン名",				FT_STR, 0, "全角 20 字ほど（UTF-8 で最大 63 バイト）。例: 「ルシー」。有効なグループでは空にできない"),
	FLD (FLAGGROUPINFO, wSort,			"並び順",				FT_U16, 0, "一覧で小さい順に並ぶ（同じなら番号順）"),
};

/* 会話セット（lines は項目表の外で扱う） */
static const DATAFIELD s_aFieldTalk[] = {
	FLD (TALKINFO, byState,				"状態",					FT_U8, 0, "0=削除済み（枠だけ残る。話しかけても「……」と言うだけ。同じ番号をもう一度使う時は、足さずにこの枠の byState を 1 に戻す） 1=有効。足す時に省くと 1"),
	FLD (TALKINFO, wID,					"会話セット番号",		FT_U16, 0, "1〜999。NPC の台詞を \"@tNNN\"（NNN はこの番号。例 @t012）にすると、その NPC に話しかけた時にこのセットを使う。足す時に必ず指定（ほかのセットと重ならない）。あとから変えられない。id（配列の番号）とは別"),
	FLDS(TALKINFO, szName,				"名前",					FT_STR, 0, "管理用のメモ（プレイヤーには出ない）。最大 63 バイト"),
};

/* 会話セットの 1 行 */
static const char *s_pszNoteTalkNoBit = "キャラのフラグの印の番号（0〜1023）。65535=無し（印 0 を使えるよう 0 ではなく 65535 が無し。短い配列は残りが無しになる）";
static const char *s_pszNoteTalkNoNum = "キャラのフラグの数値の番号（0〜127）。255=無し（短い配列は残りが無しになる）";
static const DATAFIELD s_aFieldTalkLine[] = {
	FLD (TALKLINEINFO, byRank,			"順位",					FT_U8, 0, "小さいほど先に見る。条件に合う行のうち順位が一番小さい行が選ばれ、同じ順位が複数あればくじで 1 行"),
	FLD (TALKLINEINFO, byLvMin,			"レベル下限",			FT_U8, 0, "0=気にしない"),
	FLD (TALKLINEINFO, byLvMax,			"レベル上限",			FT_U8, 0, "0=気にしない"),
	FLDA(TALKLINEINFO, awNeed,			"立っている必要のある印",	FT_U16ARR, WORD, FF_PADMAX, s_pszNoteTalkNoBit),
	FLDA(TALKLINEINFO, awNot,			"立っていてはいけない印",	FT_U16ARR, WORD, FF_PADMAX, s_pszNoteTalkNoBit),
	FLDA(TALKLINEINFO, abyCondNo,		"数値の条件の数値番号",	FT_U8ARR, BYTE, FF_PADMAX, s_pszNoteTalkNoNum),
	FLDA(TALKLINEINFO, abyCondCmp,		"数値の条件の比べ方",	FT_U8ARR, BYTE, 0, "abyCondNo と同じ並び。0=以上 1=ちょうど 2=以下"),
	FLDA(TALKLINEINFO, abyCondVal,		"数値の条件の値",		FT_U8ARR, BYTE, 0, "abyCondNo と同じ並び。0〜255"),
	FLDS(TALKLINEINFO, szSpeech,		"台詞",					FT_STR, FF_MULTILINE, "NPC の台詞と同じ書き方（@0 入力待ち・@1 改行・@2 点滅無しで入力待ち など。ページ送りあり）。話しかけた本人にだけ出る。最大 1023 バイト。空だと「……」"),
	FLDA(TALKLINEINFO, awSetOn,			"話した後に立てる印",	FT_U16ARR, WORD, FF_PADMAX, s_pszNoteTalkNoBit),
	FLDA(TALKLINEINFO, awSetOff,		"話した後に消す印",		FT_U16ARR, WORD, FF_PADMAX, s_pszNoteTalkNoBit),
	FLDA(TALKLINEINFO, abyActNo,		"話した後に動かす数値の番号",	FT_U8ARR, BYTE, FF_PADMAX, s_pszNoteTalkNoNum),
	FLDA(TALKLINEINFO, abyActOp,		"話した後の数値の動かし方",	FT_U8ARR, BYTE, 0, "abyActNo と同じ並び。0=○にする 1=○足す（255 で止まる）"),
	FLDA(TALKLINEINFO, abyActVal,		"話した後の数値の値",	FT_U8ARR, BYTE, 0, "abyActNo と同じ並び。0〜255"),
	FLD (TALKLINEINFO, wGiveItem,		"話した後に渡すアイテム",	FT_U16, 0, "item の番号（0=渡さない）。この行で話すたびに 1 つ渡す（回数は数えない）。一度きりにしたい時は、同じ行の awSetOn で印を立て、awNot に同じ印を入れる。持ち物がいっぱいなら預かり所へ、大事な物の印のある品は大事な物の欄へ。台詞のあとに「○○を手に入れました。」と知らせる"),
	FLD (TALKLINEINFO, wQuestID,		"受けている依頼の条件",	FT_U16, 0, "quest の依頼番号（wID）。0=条件なし。0 以外なら、話した人がいまその依頼を受けていて、段が byQuestStepMin〜byQuestStepMax の時だけこの行に合う"),
	FLD (TALKLINEINFO, byQuestStepMin,	"依頼の段の下限",		FT_U8, 0, "wQuestID が 0 以外の時。段は 0 から数える（quest の見えるNPC の byStepMin と同じ）"),
	FLD (TALKLINEINFO, byQuestStepMax,	"依頼の段の上限",		FT_U8, 0, "wQuestID が 0 以外の時。255=どの段でも（quest の見えるNPC の byStepMax と同じ）。省くと 255"),
	FLD (TALKLINEINFO, wNeedItem,		"持っている物の条件",	FT_U16, 0, "item の番号（0=条件なし）。0 以外なら、話した人がそのアイテムを手持ちか大事な物の欄に 1 つ以上持っている時だけ合う（預かり所は見ない）"),
	FLD (TALKLINEINFO, wTakeItem,		"話した後に取り上げるアイテム",	FT_U16, 0, "item の番号（0=取り上げない）。この行で話した後に、手持ち→大事な物の欄の順にこの品を 1 つ取り上げる（装備中の品は取らない。預かり所は見ない）。引換券→景品のような引き換え用で、wGiveItem と組む。取り上げる品を持っていない時、または渡す品を受け取れない時は、何も取らず印・数値・渡すも動かさない（台詞は言う）"),
	FLD (TALKLINEINFO, byAsk,			"はい・いいえで聞く",	FT_U8, 0, "0=今まで通り 1=台詞のあとに「はい・いいえ」を選ばせる。1 の行は、台詞を言った時には印・数値・渡す物・取り上げる物をまだ動かさず、「はい」の答えが来た時（話しかけた NPC の近く 2 マス以内・同じマップ・60 秒以内・その行がまだ条件に合う時だけ）に、今までと同じ反映をする。「いいえ」は何も動かさず szNoMsg を言う。答えが無効（遠い・時間切れ・ほかの NPC に話した・マップ移動・切断）なら何も起きない"),
	FLDS(TALKLINEINFO, szNoMsg,			"いいえの返事",			FT_STR, FF_MULTILINE, "byAsk=1 の行で「いいえ」を選んだ時の返事（台詞と同じ書き方）。最大 127 バイト。空だと「そうですか。また声をかけてくださいね。」"),
};

/* 依頼情報（ギルドの仕事依頼。npcs・goals は項目表の外で扱う） */
#define MAX_QUESTLEVELMIN	99			/* 依頼の最低レベルの上限（プレイヤーのレベルの上限に合わせる。0=制限なし） */
static const DATAFIELD s_aFieldQuest[] = {
	FLD (QUESTINFO, wID,				"依頼番号",				FT_U16, FF_READONLY, "足す時に自動で振られる（1〜511。ほかの依頼と重ならない）。プレイヤーの受注・クリア済みの印はこの番号で覚えているので変わらない。id（配列の番号）とは別"),
	FLD (QUESTINFO, byState,			"状態",					FT_U8, 0, "0=止める（一覧に出ない。受けている人は続けられる） 1=出す。足す時に省くと 1"),
	FLD (QUESTINFO, byGuild,			"ギルド番号",			FT_U8, 0, "NPC の台詞を \"@gNN\"（NN はこの番号）にしたギルドの人に話すと、この番号の依頼が一覧に出る"),
	FLD (QUESTINFO, wSort,				"並び順",				FT_U16, 0, "一覧で小さい順に並ぶ（同じなら依頼番号順）"),
	FLD (QUESTINFO, byOnce,				"一度きり",				FT_U8, 0, "1=クリアしたら二度と受けられない（クリア済みの印が残り、一覧からも消える） 0=何度でも"),
	FLDS(QUESTINFO, szName,				"名前",					FT_STR, 0, "一覧に出る依頼の名前"),
	FLDS(QUESTINFO, szExplain,			"説明",					FT_STR, FF_MULTILINE, "一覧で選んだ時に出る説明（報酬は自動で出る）"),
	FLDS(QUESTINFO, szMsgAccept,		"受けた時の台詞",		FT_STR, FF_MULTILINE, "省くと「依頼を受けました。」"),
	FLDS(QUESTINFO, szMsgDone,			"達成した時の台詞",		FT_STR, FF_MULTILINE, "省くと「ありがとう！ 依頼は完了です。」"),
	FLDS(QUESTINFO, szIntro,			"導入のお話",			FT_STR, FF_MULTILINE, "受ける前に読ませる長い文（最大 1023 バイト）。NPC の台詞と同じ制御コード（@0=入力待ち＝次のページ @1=改行 @2=終わり）。空なら読ませずにすぐ「受けますか？」。受けていない依頼で決定を押すと、まずこの文を 1 ページずつ読み、読み終わったら受けるか選ぶ。途中でキャンセルすると何も起きず一覧に戻る"),
	FLD (QUESTINFO, dwGold,				"報酬のお金",			FT_U32, 0, "ゴールド（持てる上限 999999 まで）"),
	FLD (QUESTINFO, dwExp,				"報酬の経験値",			FT_U32, 0, NULL),
	FLD (QUESTINFO, wRewardItem,		"報酬のアイテム",		FT_U16, 0, "item の番号（0=なし）。持ち物がいっぱいなら預かり所へ。どちらもいっぱいだと渡せずやり直し"),
	FLD (QUESTINFO, wQuestItem,			"依頼の品",				FT_U16, 0, "item の番号（0=回収しない）。見えるNPC（宝箱）で渡し、ギルドで回収する。取り消した時は持ち物と預かり所から全部消す"),
	FLD (QUESTINFO, byStepCount,		"段の数",				FT_U8, 0, "1〜4。受けた時は 0 段目。最後の段（段の数-1）に進み、依頼の品を持っていればギルドで渡せる。足す時に省くと 1"),
	FLD (QUESTINFO, byChallenge,		"討伐チャレンジ",		FT_U8, 0, "0=ふつうの依頼 1=異空間の討伐チャレンジ 2=闘技場の討伐チャレンジ（闘技場は戦う様子をほかの人が観戦できる。観客は見えるだけで手は出せず、同じ依頼は一度に 1 組。異空間は挑戦中の組にだけ見え、何組でも同時に。入口の見えるNPCに乗ると、近くで同じ依頼を受けた仲間と一緒に行き先へ移り、全員が貸し出しセットの状態になる。出ている敵を全部倒すと元に戻って最後の段へ。倒れる・パーティを抜ける・切断・行き先のマップから出ると、その人だけ中断して元に戻る。報酬は初めてクリアした時だけで、2 回目からは記録だけ）"),
	FLD (QUESTINFO, wLoanStTmp,			"貸し出しセット",		FT_U16, 0, "討伐チャレンジの時。stTmp（ステータステンプレート）の id。レベル・能力・最大HP/Mana・特技・持ち物・お金・属性をこれにする。持ち物の武器・鎧などは種類ごとに最初の 1 つを自動で装備する。職業と見た目はそのまま"),
	FLD (QUESTINFO, byMemberMin,		"必要な人数",			FT_U8, 0, "討伐チャレンジの時。0・1=一人でもよい。2 以上はパーティを組んでいないと受けられず、受けると近く（同じマップの 8 マス以内）の仲間全員が一緒に受ける。後から入った仲間は参加できない"),
	FLDA(QUESTINFO, awDoneSetFlag,		"完了した時に立てる印",	FT_U16ARR, WORD, FF_PADMAX, "キャラのフラグの印の番号（0〜1023）を最大 2 つ。65535=無し（足す時に省くと 2 つとも無し。短い配列は残りが無しになる）。ギルドで依頼を完了して報酬を受け取った時に立てる（討伐チャレンジの 2 回目からは動かさない）。名前は flag で付ける"),
	FLD (QUESTINFO, byDoneNumNo,		"完了した時に動かす数値の番号",	FT_U8, 0, "キャラのフラグの数値の番号（0〜127）。255=無し（足す時に省くと無し）"),
	FLD (QUESTINFO, byDoneNumOp,		"完了した時の数値の動かし方",	FT_U8, 0, "0=○にする 1=○足す（255 で止まる）。番号が無しなら使わない"),
	FLD (QUESTINFO, byDoneNumVal,		"完了した時の数値の値",	FT_U8, 0, "0〜255。番号が無しなら使わない"),
	FLD (QUESTINFO, wNeedFlag,			"受ける条件の印",		FT_U16, 0, "キャラのフラグの印の番号（0〜1023）。この印が立っている人だけ、ギルドの一覧に出て受けられる。65535=条件なし（足す時に省くと条件なし）"),
	FLD (QUESTINFO, wNotFlag,			"受けられなくなる印",	FT_U16, 0, "キャラのフラグの印の番号（0〜1023）。この印が立っている人は、ギルドの一覧に出ず受けられない。65535=条件なし（足す時に省くと条件なし）。すでに受けている人は続けられる"),
	FLD (QUESTINFO, byLevelMin,			"最低レベル",			FT_U8, 0, "キャラのレベルがこれ未満の人にはギルドの一覧に出さず、受けられない。0=制限なし（足す時に省くと 0）。管理者は常に見える。既に受けている人には影響しない"),
	FLD (QUESTINFO, wCancelStory,		"取り消しで戻すお話",	FT_U16, 0, "依頼を取り消した時に、最初に戻す思い出の本のお話の番号（0=何もしない）。そのお話とつながる後のお話の印・数値・持ち物・宝箱が、やり直しと同じように戻る。管理者ウィンドウ・ほかの取り消しでは戻さない"),
};

/* 思い出の本のシナリオ（クリア済みのミニシナリオをキャラごとにやり直す。NPC の台詞を "@r" にした語り部に話すと、そのキャラがクリアしたシナリオの一覧が開く）。
   印・数値の「無し」は 0 ではなく 65535・255（0 のままだと印 0・数値 0 になる） */
static const char *s_pszNoteStoryNoBit = "キャラのフラグの印の番号（0〜1023）。65535=無し";
static const DATAFIELD s_aFieldStory[] = {
	FLD (STORYINFO, wID,				"シナリオ番号",			FT_U16, FF_READONLY, "足す時に自動で振られる（1〜255。ほかのシナリオと重ならない）。後ろのお話の wPrevStory がこの番号で指すので変わらない。id（配列の番号）とは別"),
	FLD (STORYINFO, byState,			"状態",					FT_U8, 0, "0=止める（本の一覧に出ない。前のお話をやり直す時の「つながる後のお話」としては、止めていても巻き込まれる） 1=出す。足す時に省くと 1"),
	FLD (STORYINFO, wSort,				"並び順",				FT_U16, 0, "本の一覧で小さい順に並ぶ（同じなら番号順）"),
	FLDS(STORYINFO, szName,				"名前",					FT_STR, 0, "本の一覧に出るお話の名前（最大 63 バイト）"),
	FLDS(STORYINFO, szExplain,			"説明",					FT_STR, 0, "本の一覧で選んだ時に出る一行説明（最大 255 バイト）"),
	FLD (STORYINFO, wClearFlag,			"クリアした印",			FT_U16, 0, "キャラのフラグの印の番号（0〜1023。必ず指定）。この印が立っている人の本に出る。やり直しで消えるよう、通常は awFlagFrom・awFlagTo の範囲に含める"),
	FLD (STORYINFO, wPrevStory,			"前のお話",				FT_U16, 0, "前のお話の wID（0=無し）。このお話をやり直すと、wPrevStory をたどってこのお話につながる後のお話（子・孫…）もいっしょに最初に戻る。自分や後のお話は指せない（ループ不可）"),
	FLDA(STORYINFO, awFlagFrom,			"消す印の範囲の先頭",	FT_U16ARR, WORD, FF_PADMAX, "最大 2 つ。awFlagTo と同じ並びで、先頭〜最後（その印も含む）の印をやり直す時に消す。65535=無し（短い配列は残りが無しになる）。先頭と最後は両方入れるか両方無しにし、先頭 <= 最後。もらった印などやり直しても残したい印は範囲の外に置く"),
	FLDA(STORYINFO, awFlagTo,			"消す印の範囲の最後",	FT_U16ARR, WORD, FF_PADMAX, s_pszNoteStoryNoBit),
	FLDA(STORYINFO, abyNum,				"0 に戻す数値",			FT_U8ARR, BYTE, FF_PADMAX, "最大 4 つ。キャラのフラグの数値の番号（0〜127）。やり直す時に 0 にする。255=無し（短い配列は残りが無しになる）"),
	FLDA(STORYINFO, awQuest,			"受け直せるようにする依頼", FT_U16ARR, WORD, 0, "最大 4 つ。quest の依頼番号（wID）。やり直す時にクリア済みの印を消し、一度きりの依頼でもまた受けられる。0=無し。報酬は初回だけ（もらい済みの印は残る）。やり直す人がこの依頼を受けている間はやり直せない"),
	FLD (STORYINFO, byCountNum,			"やり直した回数の数値",	FT_U8, 0, "キャラのフラグの数値の番号（0〜127）。やり直すたびにこの数値が +1 される（選んだお話だけ。255 で止まる。0 に戻す数値と重なっても最後に +1）。255=数えない（足す時に省くと数えない）。話の中で「2 周目」の台詞を出し分けるのに使える"),
	FLDA(STORYINFO, awTakeItem,			"取り上げる持ち物",		FT_U16ARR, WORD, 0, "やり直す時に取り上げる持ち物（item 番号、最大 8。大事な物欄と手持ちから消す。装備中・預かり所は触らない）。0=無し（省くと無し）。持っている分は全部消える。やり直す人が持っていなければ何も起きない"),
	FLDA(STORYINFO, awTakeChest,		"戻す宝箱",				FT_U16ARR, WORD, FF_PADMAX, "やり直す時にもう一度開けられるように戻す宝箱の番号（最大 8。0〜2047、65535=無し。宝箱 0 も指せる。短い配列は残りが無しになる。足す時に省くと全部無し）。指定した宝箱の「開けた記録」だけが消え、本人の画面の宝箱は閉じた絵に戻る。指定しない宝箱は戻らない"),
};

/* 思い出の本のシナリオの「いまの目的」の 1 行（条件は会話データの行と同じ形・同じ判定）。
   （クリアの印 wClearFlag は見ない）上から見て最初に全部合う行の szGoal をコマンド窓の「目的」に出す */
static const DATAFIELD s_aFieldStoryGoal[] = {
	FLDA(STORYGOAL, awNeed,				"立っている必要のある印",	FT_U16ARR, WORD, FF_PADMAX, s_pszNoteTalkNoBit),
	FLDA(STORYGOAL, awNot,				"立っていてはいけない印",	FT_U16ARR, WORD, FF_PADMAX, s_pszNoteTalkNoBit),
	FLDA(STORYGOAL, abyCondNo,			"数値の条件の数値番号",	FT_U8ARR, BYTE, FF_PADMAX, s_pszNoteTalkNoNum),
	FLDA(STORYGOAL, abyCondCmp,			"数値の条件の比べ方",	FT_U8ARR, BYTE, 0, "abyCondNo と同じ並び。0=以上 1=ちょうど 2=以下"),
	FLDA(STORYGOAL, abyCondVal,			"数値の条件の値",		FT_U8ARR, BYTE, 0, "abyCondNo と同じ並び。0〜255"),
	FLDS(STORYGOAL, szGoal,				"いまの目的",			FT_STR, 0, "コマンド窓の「目的」に「シナリオ名」と並べて出す文（最大 127 バイト）。空の行は飛ばす"),
};

/* 依頼の見えるNPC（受けた人とそのパーティにだけ見える。話すと台詞が出る。台詞の中身はサーバーが送り、見えるNPC自体の台詞は "@q" 固定） */
static const DATAFIELD s_aFieldQuestNpc[] = {
	FLD (QUESTNPCINFO, dwMapID,			"出るマップID",			FT_U32, 0, "マップ情報の dwID（配列の番号ではなくマップID）"),
	FLD (QUESTNPCINFO, byX,				"X座標",				FT_U8, 0, NULL),
	FLD (QUESTNPCINFO, byY,				"Y座標",				FT_U8, 0, NULL),
	FLD (QUESTNPCINFO, byDir,			"向き",					FT_U8, 0, "0=上 1=下 2=左 3=右"),
	FLD (QUESTNPCINFO, byKind,			"種類",					FT_U8, 0, "1=ワープポータル（話すと行き先へ移る） 2=宝箱（話すと wItem をもらう） 3=人（台詞だけ） 4=討伐チャレンジの入口（乗ると挑戦を始めて行き先へ移る。行き先は dwDestMapID・byDestX・byDestY） 5=討伐チャレンジの敵（挑戦が始まった時に出る。強さは wStTmp）。0 は使わない欄（足す時は 1〜5 を指定）"),
	FLD (QUESTNPCINFO, wGrpID,			"見た目の画像ID",		FT_U16, 0, "キャラ画像ID（宝箱・ポータルも画像の番号で指定）"),
	FLDS(QUESTNPCINFO, szName,			"名前",					FT_STR, 0, NULL),
	FLDS(QUESTNPCINFO, szSpeech,		"話した時の台詞",		FT_STR, FF_MULTILINE, "話した人にだけ出る"),
	FLD (QUESTNPCINFO, byStepMin,		"この段から出る",		FT_U8, 0, "0〜3"),
	FLD (QUESTNPCINFO, byStepMax,		"この段まで出る",		FT_U8, 0, "0〜3。255=どの段でも"),
	FLD (QUESTNPCINFO, byFlgNeed,		"出る条件の旗",			FT_U8, 0, "0=条件なし 1〜16=この旗が立っていたら出る"),
	FLD (QUESTNPCINFO, byFlgHide,		"消える条件の旗",		FT_U8, 0, "0=条件なし 1〜16=この旗が立っていたら消える"),
	FLD (QUESTNPCINFO, dwDestMapID,		"ポータルの行き先マップID",	FT_U32, 0, "ポータルの時。マップ情報の dwID。同じマップならそのマップ内の移動"),
	FLD (QUESTNPCINFO, byDestX,			"ポータルの行き先X",	FT_U8, 0, NULL),
	FLD (QUESTNPCINFO, byDestY,			"ポータルの行き先Y",	FT_U8, 0, NULL),
	FLD (QUESTNPCINFO, wItem,			"宝箱で渡すアイテム",	FT_U16, 0, "item の番号（0=なし）。持ち物がいっぱいなら預かり所へ"),
	FLD (QUESTNPCINFO, bySetFlg,		"話したら立てる旗",		FT_U8, 0, "0=なし 1〜16。宝箱は、この旗が立っていると 2 度目はもらえない"),
	FLD (QUESTNPCINFO, byNextStep,		"話したら進む段",		FT_U8, 0, "0〜3。255=変えない"),
	FLD (QUESTNPCINFO, wStTmp,			"敵の強さ",				FT_U16, 0, "種類が 5（敵）の時。stTmp（ステータステンプレート）の id。レベル・能力・HP・特技・動き方（byNpcType）をこれにする。経験値・お金・落とす物は無し"),
};

/* アイテムの種別ごとの詳細 */
static const char *s_pszNoteEqu = "equUser（装備できる職業）の番号";
static const DATAFIELD s_aFieldItemRura[] = {
	FLD (ITEMINFO_RURA, dwEnableMapID,	"使えるマップID",		FT_U32, 0, "先頭ビットはフラグ"),
	FLD (ITEMINFO_RURA, dwMapID,		"移動先マップID",		FT_U32, 0, NULL),
	FLD (ITEMINFO_RURA, dwX,			"移動先X",				FT_U32, 0, NULL),
	FLD (ITEMINFO_RURA, dwY,			"移動先Y",				FT_U32, 0, NULL),
};
static const DATAFIELD s_aFieldItemHpUp[] = {
	FLD (ITEMINFO_HPUP, byRate,			"率で回復",				FT_U8, 0, "1 なら wMin/wMax を%として扱う"),
	FLD (ITEMINFO_HPUP, wMin,			"最低回復値",			FT_U16, 0, NULL),
	FLD (ITEMINFO_HPUP, wMax,			"最大回復値",			FT_U16, 0, NULL),
};
static const DATAFIELD s_aFieldItemWeapon[] = {
	FLD (ITEMINFO_WEAPON, byEfcID,		"攻撃時のエフェクト",	FT_U8, 0, NULL),
	FLD (ITEMINFO_WEAPON, byMin,		"攻撃力減少幅",			FT_U8, 0, NULL),
	FLD (ITEMINFO_WEAPON, byAtackType,	"攻撃種別",				FT_U8, 0, "0=直接攻撃 1=飛び道具 2=なぎ払い(自分の左右にも) 3=貫通(1マス先にも)"),
	FLD (ITEMINFO_WEAPON, byData1,		"付加データ1",			FT_U8, 0, NULL),
	FLD (ITEMINFO_WEAPON, byData2,		"付加データ2",			FT_U8, 0, NULL),
	FLD (ITEMINFO_WEAPON, byData3,		"付加データ3",			FT_U8, 0, NULL),
	FLD (ITEMINFO_WEAPON, byData4,		"付加データ4",			FT_U8, 0, NULL),
	FLD (ITEMINFO_WEAPON, byData5,		"付加データ5",			FT_U8, 0, NULL),
	FLD (ITEMINFO_WEAPON, wEqu,			"装備できる職業",		FT_U16, 0, s_pszNoteEqu),
};
static const DATAFIELD s_aFieldItemEqu[] = {	/* 鎧・盾・兜・小手・靴（wEqu だけ） */
	FLD (ITEMINFO_ARMOR, wEqu,			"装備できる職業",		FT_U16, 0, s_pszNoteEqu),
};
static const DATAFIELD s_aFieldItemAcc[] = {	/* 首飾り・指輪 */
	FLD (ITEMINFO_EQU_ACC1, byEquType,	"装備効果",				FT_U8, 0, "0=効果無し 1=守備力 2=魔法防御"),
	FLD (ITEMINFO_EQU_ACC1, wEqu,		"装備できる職業",		FT_U16, 0, s_pszNoteEqu),
};
static const DATAFIELD s_aFieldItemSeed[] = {
	FLD (ITEMINFO_SEED, byType,			"種類",					FT_U8, 0, NULL),
	FLD (ITEMINFO_SEED, wMax,			"上限値",				FT_U16, 0, NULL),
};
static const DATAFIELD s_aFieldItemSubGame[] = {
	FLD (ITEMINFO_SUBGAME, byType,		"種類",					FT_U8, 0, NULL),
};
static const DATAFIELD s_aFieldItemSake[] = {
	FLD (ITEMINFO_SAKE, chPoint,		"度数",					FT_S8, 0, NULL),
};
static const DATAFIELD s_aFieldItemMsg[] = {	/* pMsgTo・pMsgMe は別に扱う */
	FLD (ITEMINFO_MSG, byType,			"表示種別",				FT_U8, 0, NULL),
	FLD (ITEMINFO_MSG, byEfectID,		"エフェクト番号",		FT_U8, 0, NULL),
	FLDS(ITEMINFO_MSG, szCharEfc,		"文字エフェクト",		FT_STR, 0, NULL),
};
static const DATAFIELD s_aFieldItemGrpChg[] = {
	FLD (ITEMINFO_GRPCHG, wGrpID,		"変更する画像ID",		FT_U16, 0, "0 は記録した画像"),
};
static const DATAFIELD s_aFieldItemLight[] = {
	FLD (ITEMINFO_LIGHT, byLevel,		"明るさ",				FT_U8, 0, NULL),
	FLD (ITEMINFO_LIGHT, wTime,			"維持時間",				FT_U16, 0, NULL),
};
static const DATAFIELD s_aFieldItemHpDown[] = {
	FLD (ITEMINFO_HPDOWN, byEfectID,	"エフェクト番号",		FT_U8, 0, NULL),
	FLD (ITEMINFO_HPDOWN, byRate,		"率で減少",				FT_U8, 0, NULL),
	FLD (ITEMINFO_HPDOWN, wMin,			"最低減少値",			FT_U16, 0, NULL),
	FLD (ITEMINFO_HPDOWN, wMax,			"最大減少値",			FT_U16, 0, NULL),
};
static const DATAFIELD s_aFieldItemMagic[] = {
	FLD (ITEMINFO_MAGIC, byID,			"覚える特技",			FT_U8, 0, "magic の番号"),
};
static const DATAFIELD s_aFieldItemUseMagic[] = {
	FLD (ITEMINFO_MAGIC, byID,			"発動する特技",			FT_U8, 0, "magic の番号（覚えていなくても使え、Mana は減らない）"),
};
static const DATAFIELD s_aFieldItemManaUp[] = {
	FLD (ITEMINFO_MANAUP, byRate,		"率で回復",				FT_U8, 0, NULL),
	FLD (ITEMINFO_MANAUP, wMin,			"最低回復値",			FT_U16, 0, NULL),
	FLD (ITEMINFO_MANAUP, wMax,			"最大回復値",			FT_U16, 0, NULL),
};
static const DATAFIELD s_aFieldItemHpManaUp[] = {
	FLD (ITEMINFO_HPMANAUP, byRateHP,	"率で回復(HP)",			FT_U8, 0, NULL),
	FLD (ITEMINFO_HPMANAUP, byRateMana,	"率で回復(Mana)",		FT_U8, 0, NULL),
	FLD (ITEMINFO_HPMANAUP, wMinHP,		"最低回復値(HP)",		FT_U16, 0, NULL),
	FLD (ITEMINFO_HPMANAUP, wMaxHP,		"最大回復値(HP)",		FT_U16, 0, NULL),
	FLD (ITEMINFO_HPMANAUP, wMinMana,	"最低回復値(Mana)",		FT_U16, 0, NULL),
	FLD (ITEMINFO_HPMANAUP, wMaxMana,	"最大回復値(Mana)",		FT_U16, 0, NULL),
};
static const DATAFIELD s_aFieldItemResurrection[] = {
	FLD (ITEMINFO_RESURRECTION, byRate,	"回復率",				FT_U8, 0, NULL),
};

#define COUNTOF(a)	((int)(sizeof(a) / sizeof((a)[0])))

/* アイテムの種別 → 詳細の項目表 */
static const DATAFIELD *ItemDetailFields(BYTE byType, int &nCount)
{
	switch (byType) {
	case ITEMTYPE_RURA:			nCount = COUNTOF(s_aFieldItemRura);			return s_aFieldItemRura;
	case ITEMTYPE_HPUP:			nCount = COUNTOF(s_aFieldItemHpUp);			return s_aFieldItemHpUp;
	case ITEMTYPE_WEAPON:		nCount = COUNTOF(s_aFieldItemWeapon);		return s_aFieldItemWeapon;
	case ITEMTYPE_ARMOR:
	case ITEMTYPE_SHILD:
	case ITEMTYPE_HELMET:
	case ITEMTYPE_EQU_HAND:
	case ITEMTYPE_EQU_LEG:		nCount = COUNTOF(s_aFieldItemEqu);			return s_aFieldItemEqu;
	case ITEMTYPE_EQU_ACC1:
	case ITEMTYPE_EQU_ACC2:
	case ITEMTYPE_EQU_ACC3:		nCount = COUNTOF(s_aFieldItemAcc);			return s_aFieldItemAcc;
	case ITEMTYPE_SEED:			nCount = COUNTOF(s_aFieldItemSeed);			return s_aFieldItemSeed;
	case ITEMTYPE_SUBGAME:		nCount = COUNTOF(s_aFieldItemSubGame);		return s_aFieldItemSubGame;
	case ITEMTYPE_SAKE:			nCount = COUNTOF(s_aFieldItemSake);			return s_aFieldItemSake;
	case ITEMTYPE_MSG:			nCount = COUNTOF(s_aFieldItemMsg);			return s_aFieldItemMsg;
	case ITEMTYPE_GRPCHG:		nCount = COUNTOF(s_aFieldItemGrpChg);		return s_aFieldItemGrpChg;
	case ITEMTYPE_LIGHT:		nCount = COUNTOF(s_aFieldItemLight);		return s_aFieldItemLight;
	case ITEMTYPE_HPDOWN:		nCount = COUNTOF(s_aFieldItemHpDown);		return s_aFieldItemHpDown;
	case ITEMTYPE_MAGIC:		nCount = COUNTOF(s_aFieldItemMagic);		return s_aFieldItemMagic;
	case ITEMTYPE_MANAUP:		nCount = COUNTOF(s_aFieldItemManaUp);		return s_aFieldItemManaUp;
	case ITEMTYPE_HPMANAUP:		nCount = COUNTOF(s_aFieldItemHpManaUp);		return s_aFieldItemHpManaUp;
	case ITEMTYPE_RESURRECTION:	nCount = COUNTOF(s_aFieldItemResurrection);	return s_aFieldItemResurrection;
	case ITEMTYPE_USEMAGIC:		nCount = COUNTOF(s_aFieldItemUseMagic);		return s_aFieldItemUseMagic;
	}
	nCount = 0;
	return NULL;
}

/* ------------------------------------------------------------------------- */
/* 項目の読み書き（共通）														 */
/* ------------------------------------------------------------------------- */
static const char *TypeName(int nType)
{
	switch (nType) {
	case FT_U8:		return "u8";
	case FT_S8:		return "s8";
	case FT_U16:	return "u16";
	case FT_U32:	return "u32";
	case FT_STR:	return "string";
	case FT_U8ARR:	return "u8[]";
	case FT_U16ARR:	return "u16[]";
	}
	return "?";
}

static void TypeRange(int nType, long long &nMin, long long &nMax)
{
	switch (nType) {
	case FT_S8:		nMin = -128;	nMax = 127;			break;
	case FT_U16:
	case FT_U16ARR:	nMin = 0;		nMax = 65535;		break;
	case FT_U32:	nMin = 0;		nMax = 4294967295LL;	break;
	default:		nMin = 0;		nMax = 255;			break;
	}
}

static std::string FieldValueJson(const BYTE *pBase, const DATAFIELD &f)
{
	const BYTE *p = pBase + f.nOffset;
	std::string out;
	size_t i;

	switch (f.nType) {
	case FT_U8:		return JsonNum(*p);
	case FT_S8:		return JsonNum(*(const signed char *)p);
	case FT_U16:	return JsonNum(*(const WORD *)p);
	case FT_U32:	return JsonNum(*(const DWORD *)p);
	case FT_STR:
		{
			std::string s((const char *)p, strnlen((const char *)p, f.nSize));
			return JsonStr(s.c_str());
		}
	case FT_U8ARR:
	case FT_U16ARR:
		out = "[";
		for (i = 0; i < f.nSize; i ++) {
			if (i) {
				out += ",";
			}
			out += JsonNum((f.nType == FT_U8ARR) ? p[i] : ((const WORD *)p)[i]);
		}
		out += "]";
		return out;
	}
	return "null";
}

static std::string FieldsJson(const BYTE *pBase, const DATAFIELD *paField, int nCount)
{
	std::string out;
	for (int i = 0; i < nCount; i ++) {
		if (i) {
			out += ",";
		}
		out += JsonStr(paField[i].pszName) + ":" + FieldValueJson(pBase, paField[i]);
	}
	return out;
}

/* 整数か確かめて範囲に収まるか */
static bool GetInt(const JVal &v, long long nMin, long long nMax, long long &n, std::string &strErr, const std::string &strName)
{
	if (v.t != JVal::NUM || v.n != (double)(long long)v.n) {
		strErr = strName + " は整数で指定してください";
		return false;
	}
	n = (long long)v.n;
	if (n < nMin || n > nMax) {
		strErr = strName + " は " + JsonNum(nMin) + "〜" + JsonNum(nMax) + " の範囲で指定してください";
		return false;
	}
	return true;
}

/* 文字列として使えるか（長さ・制御文字） */
static bool CheckText(const std::string &s, size_t nBufSize, bool bMultiLine, std::string &strErr, const std::string &strName)
{
	if (s.size() + 1 > nBufSize) {
		strErr = strName + " が長すぎます（UTF-8 で " + JsonNum((long long)nBufSize - 1) + " バイトまで。今は " + JsonNum((long long)s.size()) + " バイト）";
		return false;
	}
	for (size_t i = 0; i < s.size(); i ++) {
		unsigned char c = (unsigned char)s[i];
		if (c < 0x20 && !(bMultiLine && (c == '\n' || c == '\r'))) {
			strErr = strName + " に使えない文字（改行・制御文字）が入っています";
			return false;
		}
		if (c == 0x7F) {
			strErr = strName + " に使えない文字（制御文字）が入っています";
			return false;
		}
	}
	return true;
}

static bool SetField(BYTE *pBase, const DATAFIELD &f, const JVal &v, std::string &strErr, const std::string &strName)
{
	BYTE *p = pBase + f.nOffset;
	long long n, nMin, nMax;

	if (f.nFlags & FF_READONLY) {
		strErr = strName + " は変えられません";
		return false;
	}
	TypeRange(f.nType, nMin, nMax);
	switch (f.nType) {
	case FT_U8:
		if (!GetInt(v, nMin, nMax, n, strErr, strName)) return false;
		*p = (BYTE)n;
		return true;
	case FT_S8:
		if (!GetInt(v, nMin, nMax, n, strErr, strName)) return false;
		*(signed char *)p = (signed char)n;
		return true;
	case FT_U16:
		if (!GetInt(v, nMin, nMax, n, strErr, strName)) return false;
		*(WORD *)p = (WORD)n;
		return true;
	case FT_U32:
		if (!GetInt(v, nMin, nMax, n, strErr, strName)) return false;
		*(DWORD *)p = (DWORD)n;
		return true;
	case FT_STR:
		if (v.t != JVal::STR) {
			strErr = strName + " は文字列で指定してください";
			return false;
		}
		if (!CheckText(v.s, f.nSize, (f.nFlags & FF_MULTILINE) != 0, strErr, strName)) return false;
		memset(p, 0, f.nSize);
		memcpy(p, v.s.data(), v.s.size());
		return true;
	case FT_U8ARR:
	case FT_U16ARR:
		if (v.t != JVal::ARR) {
			strErr = strName + " は数の配列で指定してください";
			return false;
		}
		if (v.a.size() > f.nSize) {
			strErr = strName + " は " + JsonNum((long long)f.nSize) + " 個までです";
			return false;
		}
		for (size_t i = 0; i < f.nSize; i ++) {
			n = (f.nFlags & FF_PADMAX) ? nMax : 0;
			if (i < v.a.size() && !GetInt(v.a[i], nMin, nMax, n, strErr, strName + "[" + JsonNum((long long)i) + "]")) {
				return false;
			}
			if (f.nType == FT_U8ARR) {
				p[i] = (BYTE)n;
			} else {
				((WORD *)p)[i] = (WORD)n;
			}
		}
		return true;
	}
	strErr = strName + " は変えられません";
	return false;
}

/* 項目表から名前で探す */
static const DATAFIELD *FindField(const DATAFIELD *paField, int nCount, const std::string &strName)
{
	for (int i = 0; i < nCount; i ++) {
		if (strName == paField[i].pszName) {
			return &paField[i];
		}
	}
	return NULL;
}

/* 項目表の説明（スキーマ）を JSON にする */
static std::string FieldSchemaJson(const DATAFIELD *paField, int nCount)
{
	std::string out = "[";
	for (int i = 0; i < nCount; i ++) {
		const DATAFIELD &f = paField[i];
		long long nMin, nMax;
		TypeRange(f.nType, nMin, nMax);
		if (i) {
			out += ",";
		}
		out += "{\"name\":" + JsonStr(f.pszName) + ",\"label\":" + JsonStr(f.pszLabel) +
				",\"type\":" + JsonStr(TypeName(f.nType));
		if (f.nType == FT_STR) {
			out += ",\"maxBytes\":" + JsonNum((long long)f.nSize - 1);
		} else {
			out += ",\"min\":" + JsonNum(nMin) + ",\"max\":" + JsonNum(nMax);
		}
		if (f.nType == FT_U8ARR || f.nType == FT_U16ARR) {
			out += ",\"count\":" + JsonNum((long long)f.nSize);
		}
		if (f.nFlags & FF_READONLY) {
			out += ",\"readonly\":true";
		}
		if (f.nFlags & FF_MULTILINE) {
			out += ",\"multiline\":true";
		}
		if (f.nFlags & FF_PADMAX) {
			out += ",\"padMax\":true";
		}
		if (f.pszNote) {
			out += ",\"note\":" + JsonStr(f.pszNote);
		}
		out += "}";
	}
	out += "]";
	return out;
}

/* 変更の中に、知らない項目が無いか（打ち間違いを黙って無視しない） */
static bool CheckKnownKeys(const JVal &chg, const DATAFIELD *paField, int nCount, const char **apszExtra, std::string &strErr)
{
	for (size_t i = 0; i < chg.o.size(); i ++) {
		const std::string &k = chg.o[i].first;
		if (FindField(paField, nCount, k)) {
			continue;
		}
		bool bFound = false;
		for (int j = 0; apszExtra && apszExtra[j]; j ++) {
			if (k == apszExtra[j]) {
				bFound = true;
				break;
			}
		}
		if (!bFound) {
			strErr = "知らない項目です: " + k;
			return false;
		}
	}
	return true;
}

/* 項目表にある項目だけを反映する */
static bool ApplyFields(BYTE *pBase, const DATAFIELD *paField, int nCount, const JVal &chg, std::string &strErr, const std::string &strPrefix)
{
	for (size_t i = 0; i < chg.o.size(); i ++) {
		const DATAFIELD *pf = FindField(paField, nCount, chg.o[i].first);
		if (pf && !SetField(pBase, *pf, chg.o[i].second, strErr, strPrefix + chg.o[i].first)) {
			return false;
		}
	}
	return true;
}

/* 職業 ID の並び（jobIds） */
static bool GetJobIds(const JVal &v, std::vector<BYTE> &aOut, std::string &strErr)
{
	if (v.t != JVal::ARR) {
		strErr = "jobIds は数の配列で指定してください";
		return false;
	}
	if (v.a.size() > 255) {
		strErr = "jobIds は 255 個までです";
		return false;
	}
	aOut.clear();
	for (size_t i = 0; i < v.a.size(); i ++) {
		long long n;
		if (!GetInt(v.a[i], 0, 255, n, strErr, "jobIds[" + JsonNum((long long)i) + "]")) {
			return false;
		}
		aOut.push_back((BYTE)n);
	}
	return true;
}

static std::string JobIdsJson(BYTE byCount, const BYTE *paTbl)
{
	std::string out = "[";
	for (int i = 0; i < byCount && paTbl; i ++) {
		if (i) {
			out += ",";
		}
		out += JsonNum(paTbl[i]);
	}
	return out + "]";
}

/* ------------------------------------------------------------------------- */
/* 種類ごとの処理																 */
/* ------------------------------------------------------------------------- */
enum {
	KIND_MAGIC = 0,
	KIND_MAGICUSER,
	KIND_EQUUSER,
	KIND_JOB,
	KIND_STTMP,
	KIND_ITEM,
	KIND_MAKER,
	KIND_RECIPE,
	KIND_QUEST,
	KIND_FLAG,
	KIND_TALK,
	KIND_STORY,
	KIND_FLAGGROUP,
	KIND_AFFIX,
	/* ここから下は読むだけ */
	KIND_CHAR,
	KIND_MAP,
	KIND_MAPPARTS,
	KIND_EFC,
	KIND_COMEFC,
	KIND_LEAVE,
	KIND_SUBGAME,
	KIND_ITEMDERIVE,
	KIND_MAX
};
#define KIND_FIRST_READONLY	KIND_CHAR

/* 書き換えられる種類か（預かり所はプレイヤーのデータなので読むだけ） */
static bool KindWritable(int nKind)
{
	/* 預かり所（プレイヤーの持ち物）だけは書き込めない */
	return (nKind < KIND_MAX) && (nKind != KIND_LEAVE) && (nKind != KIND_ITEMDERIVE);
}

static const char *s_apszKindName[] = { "magic", "magicUser", "equUser", "job", "stTmp", "item", "maker", "recipe", "quest", "flag", "talk", "story", "flagGroup", "affix", "char", "map", "mapParts", "efc", "comEfc", "leave", "subGame", "itemDerive" };
static const char *s_apszKindLabel[] = {
	"特技（魔法）情報", "特技を使える職業", "装備できる職業", "職業情報とレベルアップ情報", "ステータステンプレート（NPC・敵のひな形）", "アイテム情報", "作り手（料理人など）", "レシピ（作り手に頼んで作ってもらう物）", "ギルドの仕事依頼", "フラグの名前表（印・数値に付ける名前）", "会話データ（フラグで台詞を出し分ける）", "思い出の本のシナリオ（クリア済みのミニシナリオをやり直す）", "フラグのグループ（印・数値の番号の帯に「メイン-サブ」の名前を付ける）", "付与（装備に付く接頭辞・接尾辞）",
	"キャラ情報（プレイヤー・NPC）", "マップ情報", "マップパーツ", "エフェクト情報", "吹き出し情報", "預かり所", "サブゲーム情報", "派生アイテム（付与付き・未鑑定の番号）"
};
static const char *s_apszKindNote[] = {
	"番号は 0 から。item の detail.byID（特技の書）や stTmp の abyMagic から参照される",
	"jobIds に職業の番号（job の id、0 から）を並べる。magic の byUserInfo から参照される",
	"jobIds に職業の番号（job の id、0 から）を並べる。装備アイテムの detail.wEqu から参照される",
	"szName は職業名。levelUp の 1 件目が Lv1 の初期値、n 件目はレベル n になるのに必要な経験値（dwExp）と各能力の上限",
	"変えると、このテンプレートを使っている NPC（発生元を除く）の能力もすぐ置き換わる",
	"番号 0 は「なし」として使われる。detail は byType ごとに項目が違う（スキーマの detailByType を参照）。szMsg は使った時のメッセージ",
	"町の作り手NPC（料理人など）。NPC の台詞を \"@cNN\"（NN は作り手の番号 2 桁、例 @c00）にすると、その NPC に話しかけた時に作り手の画面が開く。削除は byState=0（空き枠になり、次に足す時に使い回す）。ファイルが無い時は 0 件",
	"作り手に頼んで材料からアイテムを作ってもらうレシピ。byMaker は maker の番号、wItem・awMatItem は item の番号。材料は持ち物と預かり所の両方から使い、できた物は持ち物へ（いっぱいなら預かり所へ）。削除は byState=0",
	"ギルドの仕事依頼（読み書き。削除なし。止めるのは byState=0）。NPC の台詞を \"@gNN\"（NN=byGuild）にするとギルドの人になり、話すと一覧が開く。受けた人とそのパーティにだけ「見えるNPC」（npcs。ポータル・宝箱・人）が出て、段（0 から）と旗（1〜16）で出し分ける。goals は段ごとの「いまの目的」（文字列の配列）、npcs は配列で丸ごと置き換え。出口は既存のマップのイベントで作る（データだけ）。足す時の依頼番号（wID）は自動。完了した時にキャラのフラグを動かせる（awDoneSetFlag・byDoneNumNo・byDoneNumOp・byDoneNumVal。古い依頼は「無し」）。受ける条件の印は wNeedFlag・wNotFlag（65535=条件なし。合わない人には一覧に出ず受けられない）",
	"キャラごとにずっと残る「フラグ」（印 1024 個=ON/OFF の 0〜1023、数値 128 個=0〜255 の 0〜127）に付ける名前の表（読み書き）。名前は管理用でプレイヤーには出ない。byKind が 0=印・1=数値、wNo が番号で、同じ組は 1 件まで。削除は byState=0（枠だけ残り、足し直す時はその枠の byState を 1 に戻す）。ファイルが無い時は 0 件。キャラのフラグの中身は char の読み出しの flags で読める（読むだけ）。番号の割り振りの目安: 印は 1〜199 本筋・200〜599 地域・600〜899 お使い・900〜1023 予備",
	"会話データ（読み書き）。NPC の台詞を \"@tNNN\"（NNN=wID。例 @t012）にすると、話しかけた人のフラグ（印・数値）とレベルで lines（最大 16 行）から 1 行を選び、その人にだけ台詞を出す（台詞はクライアントへ送らない）。選び方: 条件（立っている必要のある印 awNeed 4・立っていてはいけない印 awNot 4・数値の条件 2・レベル下限上限）に全部合う行のうち byRank が一番小さい行、同じ順位が複数ならくじ。合う行が無ければ「……」。話した後に印を立てる・消す・数値を動かす。lines は配列で丸ごと置き換え。削除は byState=0（枠だけ残り、足し直す時はその枠の byState を 1 に戻す）。ファイルが無い時は 0 件",
	"思い出の本（読み書き。削除なし。止めるのは byState=0）。goals（いまの目的。条件に合う最初の行の文をコマンド窓の「目的」に出す）も持つ。NPC の台詞を \"@r\" にすると語り部になり、話すと、そのキャラがクリアしたシナリオ（byState=1 で、クリアした印 wClearFlag が立っている）の一覧が wSort 順に開く。選んで確認すると、そのシナリオと、wPrevStory をたどってそれにつながる後のシナリオ全部について、印の範囲（awFlagFrom〜awFlagTo）を消し、数値（abyNum）を 0 に戻し、依頼（awQuest）のクリア済みの印を消して、最初からやり直せる（byCountNum の数値は選んだシナリオだけ +1）。awTakeItem の持ち物（大事な物欄と手持ちの装備していない分。預かり所は触らない）も全部取り上げ、awTakeChest の宝箱（0〜2047。65535=無し）は開けた記録を消して、もう一度開けられるように戻す（宝箱の見た目も閉じた絵に戻る）。それ以外の宝箱は戻さない。依頼の報酬は初回だけ（もらい済みの印は消えない）。そのキャラが戻す依頼のどれかを受けている間と、語り部から離れている時は断る。足す時のシナリオ番号（wID 1〜255）は自動。印・数値の「無し」は 65535・255。ファイルが無い時は 0 件",
	"フラグのグループ（読み書き。削除は byState=0）。キャラのフラグの印（0〜1023）・数値（0〜127）の番号の帯（bands。1 グループ 16 本まで。互換で帯 1 は wFrom・wTo でも読み書きできる）に「メイン名」szName を付けて、「メイン-サブ」で管理するための層（例: ルシー＝印 100〜139。後から番号が飛んだ場所に続きの帯 300〜319 を足せる）。番号 n が有効なグループのどれかの帯に入る時、メイン=そのグループ・サブ=帯の順に続けて数えた番号（1 始まり。ルシー=100〜139＋300〜319 なら 100〜139 がサブ 1〜40・300〜319 がサブ 41〜60）で、「ルシー-3（103）」のように呼べる（flag の一覧にも計算した group・sub が出る。読むだけ）。見せ方・入力・絞り込みだけの層で、会話データ・依頼・NPC の見える条件・思い出の本などが覚えている番号は一切変わらない。同じ種類で有効なグループのすべての帯（同じグループの帯同士も）が重なるのは不可。足す時のグループ番号（wID 1〜255）は自動。ファイルが無い時は 0 件",
	"装備に付く「鋭い」「の加護」などの付与表（読み書き。足す・直すだけで消せない）。位置 byPos（0=前・1=後ろ）・名前 szName・付く装備種別 wEquMask（ビット）・主の値（item の byData）への% chRate と固定加算 chAdd・魔防加算 chDFMagic・格 byRank（1〜3）・出やすさ wWeight（0 なら出ない）・売値の倍率 wGoldRate（100=そのまま）・ハズレ印 byBad。足す時の付与番号（wID。1〜）は自動で、派生アイテムがこの番号で指すので変わらない・使い回さない。敵（stTmp）の byAffixRate・byAffixRank で、ドロップした装備に付く。ファイルが無い時は 0 件（0 件なら付与は付かない）。詳しくは docs/item-affix.md",
	"プレイヤーと NPC（敵・町の人・発生元など）の全キャラ。番号（id）はキャラのテーブル番号で再起動で変わることがある。NPC は npcId（変わらない番号）で指す。isPlayer でプレイヤーか分かる。書き換えられるのは保存される NPC だけ（プレイヤー・発生した敵は不可）。足す時は dwMapID・dwX・dwY が要る。消す時は変更に {\"deleted\": true}。アカウント名・パスワード・IP は出さない",
	"マップの情報・イベント・店・メッセージ・アイテム配置。番号は配列の番号（dwID はマップID で、キャラの dwMapID はこちら）。events・shops・messages は配列で丸ごと置き換え。マス目のパーツは sbo_map_tiles で読み、sbo_map_tiles_set で書く。大きさの変更・マップの追加はできない",
	"マップパーツ（マス目の parts に入る番号）の表示・当たり判定。byHitType 0 なら通れる。anime は配列で丸ごと置き換え（コマ数 byAnimeCount も変わる）。追加できる（消せない）",
	"エフェクト情報（特技の byTargetEfectID などから参照される）。anime は配列で丸ごと置き換え。追加できる（消せない・255 番まで）",
	"吹き出しの種類（番号 0 から、ゲーム内の吹き出し番号は 1 から。99 個で固定）。anime はコマごとの待ち時間（×10ミリ秒）と画像ID で、配列で丸ごと置き換え",
	"読むだけ。プレイヤーごとの預かり所（キャラの dwLeaveID がこの番号）。items は item の番号と個数",
	"爆弾ゲーム・迷路・ダンジョン・戦場ゲーム・すごろくの設定。detail は byType ごとに項目が違う（省いた項目は前の値のまま）。種別（byType）の変更・追加・すごろくのマス（events・parts）は管理者ウィンドウで",
	"読むだけ。付与が付いた装備のアイテム番号の表（番号 = 16384 + id）。{元のアイテム wBase・前の付与 wPre・後ろの付与 wSuf（affix の wID。0=なし）・未鑑定 byUnknown}。同じ組み合わせは同じ番号で、ドロップや鑑定で初めて出た組み合わせが自動で足される（SBODATA/SboDataItemDerive.dat）。プレイヤーの持ち物・預かり所・取引所などはこの番号で持つ"
};
static const int s_anKindMax[] = { 256, 256, 256, 256, 65535, 65535, MAX_MAKER, MAX_RECIPE, MAX_QUEST - 1, MAX_FLAGNAMEINFO, MAX_TALK - 1, MAX_STORY - 1, MAX_FLAGGROUP - 1, MAX_AFFIX, 0, 0, 65535, 255, MAX_COMEFC, 0, 0, MAX_ITEMDERIVE };

static int KindFromName(const char *pszKind)
{
	for (int i = 0; i < KIND_MAX; i ++) {
		if (pszKind && !strcmp(pszKind, s_apszKindName[i])) {
			return i;
		}
	}
	return -1;
}

static int KindCount(int nKind)
{
	switch (nKind) {
	case KIND_MAGIC:		return g_aMagicInfo.GetSize();
	case KIND_MAGICUSER:	return g_aMagicUserInfo.GetSize();
	case KIND_EQUUSER:		return g_aEquUserInfo.GetSize();
	case KIND_JOB:			return g_JobInfo.GetCount();
	case KIND_STTMP:		return g_aStTmpInfo.GetSize();
	case KIND_ITEM:			return g_aItemInfo.GetSize();
	case KIND_MAKER:		return g_aMakerInfo.GetSize();
	case KIND_RECIPE:		return g_aRecipeInfo.GetSize();
	case KIND_QUEST:		return g_aQuestInfo.GetSize();
	case KIND_FLAG:			return g_aFlagNameInfo.GetSize();
	case KIND_TALK:			return g_aTalkInfo.GetSize();
	case KIND_STORY:		return g_aStoryInfo.GetSize();
	case KIND_FLAGGROUP:	return g_aFlagGroupInfo.GetSize();
	case KIND_AFFIX:		return g_aAffixInfo.GetSize();
	case KIND_ITEMDERIVE:	return g_aItemDerive.GetSize();
	case KIND_CHAR:			return g_aCharInfo.GetSize();
	case KIND_MAP:			return g_aMapInfo.GetSize();
	case KIND_MAPPARTS:		return g_wCountMapParts;
	case KIND_EFC:			return g_EfcInfo.GetCount();
	case KIND_COMEFC:		return MAX_COMEFC;
	case KIND_LEAVE:		return g_aLeaveInfo.GetSize();
	case KIND_SUBGAME:		return g_aSubaGameInfo.GetSize();
	}
	return 0;
}

/* --- フラグのグループ（メイン-サブ。計算して出すだけで、番号は変えない） ---------- */
/* 種類 byKind の番号 wNo が帯に入る有効なグループ（無ければ NULL。重なりは作れないので 1 つだけ） */
static const FLAGGROUPINFO *FlagGroupOf(BYTE byKind, WORD wNo)
{
	for (int i = 0; i < g_aFlagGroupInfo.GetSize(); i ++) {
		if (FlagGroupHas(g_aFlagGroupInfo[i], byKind, wNo)) {
			return g_aFlagGroupInfo[i];
		}
	}
	return NULL;
}

/* kind "flagGroup" の記録（項目表の分 + bands と、互換の wFrom・wTo（帯 1）） */
static std::string FlagGroupJson(const FLAGGROUPINFO *p)
{
	std::string out = FieldsJson((const BYTE *)p, s_aFieldFlagGroup, COUNTOF(s_aFieldFlagGroup)) + ",\"bands\":[";
	int nBand = 0;
	WORD wFirstFrom = 0, wFirstTo = 0;

	for (int i = 0; i < MAX_FLAGGROUPBAND; i ++) {
		if (!FlagGroupBandUsed(p, i)) {
			continue;
		}
		if (nBand == 0) {
			wFirstFrom = p->awFrom[i];
			wFirstTo = p->awTo[i];
		}
		out += std::string(nBand ? "," : "") + "{\"wFrom\":" + JsonNum(p->awFrom[i]) + ",\"wTo\":" + JsonNum(p->awTo[i]) + "}";
		nBand ++;
	}
	return out + "],\"wFrom\":" + JsonNum(wFirstFrom) + ",\"wTo\":" + JsonNum(wFirstTo);
}

/* kind "flag" の記録に付ける、計算した group（メイン名）・sub（サブ。1 始まり）。どのグループにも入らなければ "" と 0 */
static std::string FlagGroupExtraJson(BYTE byKind, WORD wNo)
{
	const FLAGGROUPINFO *pGrp = FlagGroupOf(byKind, wNo);

	return std::string(",\"group\":") + JsonStr(pGrp ? pGrp->szName : "") + ",\"sub\":" + JsonNum(pGrp ? FlagGroupSub(pGrp, wNo) : 0);
}

/* --- アイテムの作業用コピー ------------------------------------------------ */
typedef struct _ITEMWORK {
	ITEMINFO			Info;			/* pData・pMsg は使わない */
	std::vector<BYTE>	aDetail;		/* 詳細（GetItemDataSize の大きさ） */
	std::string			strMsg;			/* 使った時のメッセージ */
	std::string			strMsgTo;		/* メッセージ表示: 他のキャラに使った時 */
	std::string			strMsgMe;		/* メッセージ表示: 他のキャラに使われた時 */
} ITEMWORK;

static void ItemToWork(PITEMINFO pItem, ITEMWORK &w)
{
	w.Info = *pItem;
	w.Info.pData = NULL;
	w.Info.pMsg = NULL;
	int nSize = GetItemDataSize(pItem->byType);
	w.aDetail.assign(nSize > 0 ? nSize : 0, 0);
	if (pItem->pData && nSize > 0) {
		memcpy(&w.aDetail[0], pItem->pData, nSize);
	}
	w.strMsg = pItem->pMsg ? pItem->pMsg : "";
	w.strMsgTo.clear();
	w.strMsgMe.clear();
	if (pItem->byType == ITEMTYPE_MSG && pItem->pData) {
		PITEMINFO_MSG pMsg = (PITEMINFO_MSG)pItem->pData;
		w.strMsgTo = pMsg->pMsgTo ? pMsg->pMsgTo : "";
		w.strMsgMe = pMsg->pMsgMe ? pMsg->pMsgMe : "";
		/* 作業用のコピーにはポインタを残さない */
		PITEMINFO_MSG pW = (PITEMINFO_MSG)&w.aDetail[0];
		pW->pMsgTo = NULL;
		pW->pMsgMe = NULL;
	}
}

static LPSTR NewStr(const std::string &s)
{
	if (s.empty()) {
		return NULL;
	}
	LPSTR p = new char[s.size() + 1];
	memcpy(p, s.c_str(), s.size() + 1);
	return p;
}

static void FreeItemData(PITEMINFO pItem)
{
	if (pItem->byType == ITEMTYPE_MSG && pItem->pData) {
		PITEMINFO_MSG pInfo = (PITEMINFO_MSG)pItem->pData;
		SAFE_DELETE_ARRAY(pInfo->pMsgMe);
		SAFE_DELETE_ARRAY(pInfo->pMsgTo);
	}
	SAFE_DELETE_ARRAY(pItem->pMsg);
	if (pItem->pData) {
		delete [] (PBYTE)pItem->pData;
		pItem->pData = NULL;
	}
}

static void WorkToItem(const ITEMWORK &w, PITEMINFO pItem)
{
	FreeItemData(pItem);
	*pItem = w.Info;
	pItem->pData = NULL;
	pItem->pMsg = NewStr(w.strMsg);
	if (!w.aDetail.empty()) {
		PBYTE pData = new BYTE[w.aDetail.size()];
		memcpy(pData, &w.aDetail[0], w.aDetail.size());
		pItem->pData = pData;
		if (pItem->byType == ITEMTYPE_MSG) {
			PITEMINFO_MSG pInfo = (PITEMINFO_MSG)pData;
			pInfo->pMsgTo = NewStr(w.strMsgTo);
			pInfo->pMsgMe = NewStr(w.strMsgMe);
		}
	}
}

static std::string ItemWorkJson(const ITEMWORK &w)
{
	std::string out = FieldsJson((const BYTE *)&w.Info, s_aFieldItem, COUNTOF(s_aFieldItem));
	out += ",\"szMsg\":" + JsonStr(w.strMsg.c_str());
	int nCount = 0;
	const DATAFIELD *paField = ItemDetailFields(w.Info.byType, nCount);
	out += ",\"detail\":{";
	if (paField && !w.aDetail.empty()) {
		out += FieldsJson(&w.aDetail[0], paField, nCount);
		if (w.Info.byType == ITEMTYPE_MSG) {
			out += ",\"msgTo\":" + JsonStr(w.strMsgTo.c_str());
			out += ",\"msgMe\":" + JsonStr(w.strMsgMe.c_str());
		}
	}
	out += "}";
	return out;
}

#define MAX_ITEMMSG		4096		/* 使った時のメッセージの長さ（バイト。読み物を数ページ書けるよう 4096。保存は文字列そのままなので形式の変換は要らない） */

static bool ItemApply(ITEMWORK &w, const JVal &chg, std::string &strErr)
{
	static const char *apszExtra[] = { "szMsg", "detail", NULL };
	if (!CheckKnownKeys(chg, s_aFieldItem, COUNTOF(s_aFieldItem), apszExtra, strErr)) {
		return false;
	}
	BYTE byTypeOld = w.Info.byType;
	if (!ApplyFields((BYTE *)&w.Info, s_aFieldItem, COUNTOF(s_aFieldItem), chg, strErr, "")) {
		return false;
	}
	if (w.Info.byType >= ITEMTYPE_MAX) {
		strErr = "byType は 0〜" + JsonNum(ITEMTYPE_MAX - 1) + " で指定してください";
		return false;
	}
	/* 種別が変わったら詳細は作り直す */
	if (w.Info.byType != byTypeOld) {
		int nSize = GetItemDataSize(w.Info.byType);
		w.aDetail.assign(nSize > 0 ? nSize : 0, 0);
		w.strMsgTo.clear();
		w.strMsgMe.clear();
	}
	const JVal *pMsg = chg.Get("szMsg");
	if (pMsg) {
		if (pMsg->t != JVal::STR) {
			strErr = "szMsg は文字列で指定してください";
			return false;
		}
		if (!CheckText(pMsg->s, MAX_ITEMMSG, true, strErr, "szMsg")) {
			return false;
		}
		w.strMsg = pMsg->s;
	}
	const JVal *pDetail = chg.Get("detail");
	if (pDetail) {
		if (pDetail->t != JVal::OBJ) {
			strErr = "detail はオブジェクトで指定してください";
			return false;
		}
		int nCount = 0;
		const DATAFIELD *paField = ItemDetailFields(w.Info.byType, nCount);
		static const char *apszMsgExtra[] = { "msgTo", "msgMe", NULL };
		if (!CheckKnownKeys(*pDetail, paField, nCount, (w.Info.byType == ITEMTYPE_MSG) ? apszMsgExtra : NULL, strErr)) {
			strErr = "detail: " + strErr + "（この種別の detail の項目はスキーマの detailByType を見てください）";
			return false;
		}
		if (paField && !w.aDetail.empty() &&
				!ApplyFields(&w.aDetail[0], paField, nCount, *pDetail, strErr, "detail.")) {
			return false;
		}
		if (w.Info.byType == ITEMTYPE_MSG) {
			const char *apsz[] = { "msgTo", "msgMe" };
			std::string *apStr[] = { &w.strMsgTo, &w.strMsgMe };
			for (int i = 0; i < 2; i ++) {
				const JVal *p = pDetail->Get(apsz[i]);
				if (!p) {
					continue;
				}
				if (p->t != JVal::STR) {
					strErr = std::string("detail.") + apsz[i] + " は文字列で指定してください";
					return false;
				}
				if (!CheckText(p->s, MAX_ITEMMSG, true, strErr, std::string("detail.") + apsz[i])) {
					return false;
				}
				*apStr[i] = p->s;
			}
		}
	}
	return true;
}

/* --- 職業の作業用コピー ---------------------------------------------------- */
#define MAX_JOBNAME		64			/* 職業名の長さ（バイト、終端含む） */
#define MAX_LVUPCOUNT	255			/* レベルアップ情報の件数 */

typedef struct _JOBWORK {
	JOBINFO					Info;		/* 数の項目だけ使う（pszName・aLevelUpInfo は使わない） */
	std::string				strName;
	std::vector<LVUPINFO>	aLvUp;
} JOBWORK;

static void JobToWork(PJOBINFO pJob, JOBWORK &w)
{
	w.Info.bySkillType	= pJob->bySkillType;
	w.Info.byMagicType	= pJob->byMagicType;
	w.Info.byEquType	= pJob->byEquType;
	w.Info.dwJobID		= pJob->dwJobID;
	w.Info.dwFlg		= pJob->dwFlg;
	w.strName = pJob->pszName ? pJob->pszName : "";
	w.aLvUp.clear();
	for (int i = 0; i < pJob->aLevelUpInfo.GetSize(); i ++) {
		w.aLvUp.push_back(*pJob->aLevelUpInfo[i]);
	}
}

static void WorkToJob(const JOBWORK &w, PJOBINFO pJob)
{
	pJob->bySkillType	= w.Info.bySkillType;
	pJob->byMagicType	= w.Info.byMagicType;
	pJob->byEquType		= w.Info.byEquType;
	pJob->dwFlg			= w.Info.dwFlg;
	g_JobInfo.SetJobName(pJob, w.strName.c_str());
	g_JobInfo.DeleteAllLvUpInfo(pJob);
	for (size_t i = 0; i < w.aLvUp.size(); i ++) {
		PLVUPINFO pLv = g_JobInfo.GetNewLvUpInfo();
		*pLv = w.aLvUp[i];
		pJob->aLevelUpInfo.Add(pLv);
	}
}

static std::string JobWorkJson(const JOBWORK &w)
{
	std::string out = FieldsJson((const BYTE *)&w.Info, s_aFieldJob, COUNTOF(s_aFieldJob));
	out += ",\"szName\":" + JsonStr(w.strName.c_str());
	out += ",\"levelUp\":[";
	for (size_t i = 0; i < w.aLvUp.size(); i ++) {
		if (i) {
			out += ",";
		}
		out += "{" + FieldsJson((const BYTE *)&w.aLvUp[i], s_aFieldLvUp, COUNTOF(s_aFieldLvUp)) + "}";
	}
	out += "]";
	return out;
}

static bool LvUpApply(LVUPINFO &lv, const JVal &e, const std::string &strName, std::string &strErr)
{
	if (e.t != JVal::OBJ) {
		strErr = strName + " はオブジェクトで指定してください";
		return false;
	}
	return CheckKnownKeys(e, s_aFieldLvUp, COUNTOF(s_aFieldLvUp), NULL, strErr) &&
			ApplyFields((BYTE *)&lv, s_aFieldLvUp, COUNTOF(s_aFieldLvUp), e, strErr, strName + ".");
}

static bool JobApply(JOBWORK &w, const JVal &chg, std::string &strErr)
{
	static const char *apszExtra[] = { "szName", "levelUp", NULL };
	if (!CheckKnownKeys(chg, s_aFieldJob, COUNTOF(s_aFieldJob), apszExtra, strErr)) {
		return false;
	}
	if (!ApplyFields((BYTE *)&w.Info, s_aFieldJob, COUNTOF(s_aFieldJob), chg, strErr, "")) {
		return false;
	}
	const JVal *pName = chg.Get("szName");
	if (pName) {
		if (pName->t != JVal::STR) {
			strErr = "szName は文字列で指定してください";
			return false;
		}
		if (!CheckText(pName->s, MAX_JOBNAME, false, strErr, "szName")) {
			return false;
		}
		w.strName = pName->s;
	}
	const JVal *pLv = chg.Get("levelUp");
	if (pLv) {
		/*
		 * 配列なら丸ごと置き換える（件数も変わる。各要素で省いた項目は同じ位置の前の値、新しい行は 0）。
		 * オブジェクトなら {"番号": {変える項目}} で、その行だけ変える（番号が今の件数と同じなら 1 行足す）。
		 */
		std::vector<LVUPINFO> aNew;
		if (pLv->t == JVal::ARR) {
			if (pLv->a.size() > MAX_LVUPCOUNT) {
				strErr = "levelUp は " + JsonNum(MAX_LVUPCOUNT) + " 件までです";
				return false;
			}
			for (size_t i = 0; i < pLv->a.size(); i ++) {
				LVUPINFO lv;
				memset(&lv, 0, sizeof(lv));
				if (i < w.aLvUp.size()) {
					lv = w.aLvUp[i];
				}
				if (!LvUpApply(lv, pLv->a[i], "levelUp[" + JsonNum((long long)i) + "]", strErr)) {
					return false;
				}
				aNew.push_back(lv);
			}
		} else if (pLv->t == JVal::OBJ) {
			aNew = w.aLvUp;
			for (size_t i = 0; i < pLv->o.size(); i ++) {
				const std::string &k = pLv->o[i].first;
				char *pEnd = NULL;
				long n = strtol(k.c_str(), &pEnd, 10);
				if (k.empty() || *pEnd || n < 0 || n > (long)aNew.size() || n >= MAX_LVUPCOUNT) {
					strErr = "levelUp の番号 " + k + " は 0〜" + JsonNum((long long)aNew.size()) + " で指定してください（今の件数と同じ番号は追加）";
					return false;
				}
				if (n == (long)aNew.size()) {
					LVUPINFO lv;
					memset(&lv, 0, sizeof(lv));
					aNew.push_back(lv);
				}
				if (!LvUpApply(aNew[n], pLv->o[i].second, "levelUp[" + k + "]", strErr)) {
					return false;
				}
			}
		} else {
			strErr = "levelUp は配列（丸ごと置き換え）か、{\"番号\": {...}} のオブジェクト（その行だけ変える）で指定してください";
			return false;
		}
		w.aLvUp = aNew;
	}
	return true;
}

/* --- 特技を使える職業・装備できる職業の作業用コピー -------------------------- */
typedef struct _USERWORK {
	char				szName[MAX_EQUNAME];
	std::vector<BYTE>	aJobIds;
} USERWORK;

static bool UserApply(USERWORK &w, const DATAFIELD *paField, const JVal &chg, std::string &strErr)
{
	static const char *apszExtra[] = { "jobIds", NULL };
	if (!CheckKnownKeys(chg, paField, 1, apszExtra, strErr)) {
		return false;
	}
	/* szName は MAGICUSERINFO・EQUUSERINFO とも先頭にあり、同じ大きさ */
	if (!ApplyFields((BYTE *)w.szName, paField, 1, chg, strErr, "")) {
		return false;
	}
	const JVal *p = chg.Get("jobIds");
	if (p && !GetJobIds(*p, w.aJobIds, strErr)) {
		return false;
	}
	return true;
}

template <class T>
static void UserToWork(T *pInfo, USERWORK &w)
{
	memcpy(w.szName, pInfo->szName, sizeof(w.szName));
	w.aJobIds.assign(pInfo->paJobIDTbl, pInfo->paJobIDTbl ? pInfo->paJobIDTbl + pInfo->byCount : pInfo->paJobIDTbl);
}

template <class T>
static void WorkToUser(const USERWORK &w, T *pInfo)
{
	memcpy(pInfo->szName, w.szName, sizeof(pInfo->szName));
	SAFE_DELETE_ARRAY(pInfo->paJobIDTbl);
	pInfo->byCount = (BYTE)w.aJobIds.size();
	if (!w.aJobIds.empty()) {
		pInfo->paJobIDTbl = new BYTE[w.aJobIds.size()];
		memcpy(pInfo->paJobIDTbl, &w.aJobIds[0], w.aJobIds.size());
	}
}

static std::string UserWorkJson(const USERWORK &w, const DATAFIELD *paField)
{
	return FieldsJson((const BYTE *)w.szName, paField, 1) + ",\"jobIds\":" +
			JobIdsJson((BYTE)w.aJobIds.size(), w.aJobIds.empty() ? NULL : &w.aJobIds[0]);
}


/* ------------------------------------------------------------------------- */
/* 読むだけのデータの項目表（シミュレーション・調べもの用）						 */
/* ------------------------------------------------------------------------- */
#define RO	FF_READONLY

static const DATAFIELD s_aFieldChar[] = {
	FLDS(CHARINFO, szName,				"名前",					FT_STR, 0, NULL),
	FLD (CHARINFO, byNpcType,			"NPC種別",				FT_U8, 0, "0=プレイヤー（未設定） 1=移動しない 2=ランダム移動 5=NPC発生（発生元） 11=戦闘1 12=検索して追尾 13=戦闘2 16=重なったキャラに魔法 17=重なったキャラに攻撃 24=宝箱の敵（調べると正体を現して戦闘2になる） など"),
	FLD (CHARINFO, byAdminLV,			"管理権限",				FT_U8, RO, NULL),
	FLD (CHARINFO, dwMapID,				"マップID",				FT_U32, 0, "map の dwID"),
	FLD (CHARINFO, dwX,					"X",					FT_U32, 0, NULL),
	FLD (CHARINFO, dwY,					"Y",					FT_U32, 0, NULL),
	FLD (CHARINFO, byDirection,			"向き",					FT_U8, 0, NULL),
	FLD (CHARINFO, dwStTmplateID,		"ステータステンプレート",	FT_U32, 0, "stTmp の番号。4294967295 は無し"),
	FLD (CHARINFO, dwParentTblID,		"発生元のキャラ",		FT_U32, RO, "NPC発生で出てきたキャラの発生元（char の番号）"),
	FLD (CHARINFO, byPutCount,			"最大発生数",			FT_U8, 0, "NPC発生（発生元）の時"),
	FLD (CHARINFO, byPutOnce,			"補充しない",			FT_U8, 0, "NPC発生（発生元）の時。1 にすると、最大発生数まで出したらそれ以上は出さない（倒されても出し直さない。数えるのはダンジョンの空間ごと。ダンジョンを作り直すと数が戻る）。0=今までどおり倒されたら出し直す"),
	FLD (CHARINFO, byPutWave,			"防衛のウェーブ番号",			FT_U8, 0, "NPC発生（発生元）の時。拠点防衛の組の空間（サブゲーム byClearMode=2）で、そのウェーブの時だけ出す（0=ふつう。防衛の空間では 0 の発生元は出さない）。最大発生数の数を、発生周期（秒。0 なら 1 秒）おきに 1 体ずつ出し切る"),
	FLD (CHARINFO, wTimePutInterval,	"発生周期",				FT_U16, 0, NULL),
	FLD (CHARINFO, byAreaX,				"移動範囲(横)",			FT_U8, 0, NULL),
	FLD (CHARINFO, byAreaY,				"移動範囲(縦)",			FT_U8, 0, NULL),
	FLD (CHARINFO, byMoveProbabilityX,	"移動確率X",			FT_U8, 0, NULL),
	FLD (CHARINFO, byMoveProbabilityY,	"移動確率Y",			FT_U8, 0, NULL),
	FLD (CHARINFO, bySearchCx,			"検索範囲X",			FT_U8, 0, NULL),
	FLD (CHARINFO, bySearchCy,			"検索範囲Y",			FT_U8, 0, NULL),
	FLD (CHARINFO, byDropProbability,	"ドロップ率",			FT_U8, 0, NULL),
	FLD (CHARINFO, byGhost,			"半透明（霊）",			FT_U8, 0, "1 にすると半透明（霊）で描かれる（見た目だけ。ふつうの NPC・敵どちらにも付けられる）。2 にすると体を描かず名前だけ見える（目印用）。0=ふつう。NPC発生で出る敵は、発生元の NPC かステータステンプレート（stTmp の byGhost）の設定になる"),
	FLD (CHARINFO, wShowFlag,			"見える条件の印",		FT_U16, 0, "キャラのフラグの印の番号（0〜1023）。この印が立っている人にだけ見える（立っていない人には居ないのと同じ）。65535=条件なし（足す時に省くと条件なし）。町の人（byNpcType=1・2）向け"),
	FLD (CHARINFO, wHideFlag,			"見えない条件の印",		FT_U16, 0, "キャラのフラグの印の番号（0〜1023）。この印が立っている人には見えない。65535=条件なし。wShowFlag と両方ある時は、両方の条件を満たす人にだけ見える"),
	FLD (CHARINFO, byAtackRate,			"攻撃力の割合",			FT_U8, 0, NULL),
	FLD (CHARINFO, byJOB,				"職業",					FT_U8, RO, "job の番号"),
	FLD (CHARINFO, byLevel,				"レベル",				FT_U8, RO, NULL),
	FLD (CHARINFO, byPW,				"力",					FT_U8, RO, NULL),
	FLD (CHARINFO, byDEX,				"素早さ",				FT_U8, RO, NULL),
	FLD (CHARINFO, byST,				"体力",					FT_U8, RO, NULL),
	FLD (CHARINFO, byIT,				"賢さ",					FT_U8, RO, NULL),
	FLD (CHARINFO, byLK,				"運のよさ",				FT_U8, RO, NULL),
	FLD (CHARINFO, wAT,					"攻撃力",				FT_U16, RO, "装備込みの値"),
	FLD (CHARINFO, wDF,					"守備力",				FT_U16, RO, "装備込みの値"),
	FLD (CHARINFO, wDFMagic,			"魔法防御",				FT_U16, RO, NULL),
	FLD (CHARINFO, wMaxHP,				"最大HP",				FT_U16, RO, NULL),
	FLD (CHARINFO, wHP,					"HP",					FT_U16, RO, NULL),
	FLD (CHARINFO, wMaxMana,			"最大Mana",				FT_U16, RO, NULL),
	FLD (CHARINFO, wMana,				"Mana",					FT_U16, RO, NULL),
	FLD (CHARINFO, chAttrFire,			"属性(火)",				FT_S8, RO, NULL),
	FLD (CHARINFO, chAttrWater,			"属性(水)",				FT_S8, RO, NULL),
	FLD (CHARINFO, chAttrWind,			"属性(風)",				FT_S8, RO, NULL),
	FLD (CHARINFO, chAttrEarth,			"属性(土)",				FT_S8, RO, NULL),
	FLD (CHARINFO, chAttrLight,			"属性(光)",				FT_S8, RO, NULL),
	FLD (CHARINFO, chAttrDark,			"属性(闇)",				FT_S8, RO, NULL),
	FLDA(CHARINFO, abyMagic,			"覚えている特技",		FT_U8ARR, BYTE, RO, "magic の番号"),
	FLDA(CHARINFO, abyMagicKey,			"特技ショートカット",	FT_U8ARR, BYTE, RO, "1〜5 キーの枠に登録した magic の番号（0 は空）"),
	FLDA(CHARINFO, awKeyItem,			"ショートカットの装備品・道具",	FT_U16ARR, WORD, RO, "abyMagicKey と同じ 1〜5 キーの枠に登録した item の番号（0 は特技の枠か空き。装備品は持ち替え、道具は使う）"),
	FLD (CHARINFO, byOptMagicBar,		"特技ショートカットの帯",	FT_U8, RO, "0=表示 1=非表示（アカウントごとの設定）"),
	FLDA(CHARINFO, waItem,				"持ち物",				FT_U16ARR, WORD, RO, "item の番号"),
	FLDA(CHARINFO, waImportant,			"大事な物",				FT_U16ARR, WORD, RO, "item の番号（0 は空。鍵や依頼の品など。持ち物とは別の欄）"),
	FLDA(CHARINFO, waEqu,				"装備",					FT_U16ARR, WORD, RO, "持ち物（waItem）の何番目か。65535 は無し。順に 武器・鎧・盾・兜・小手・靴・首飾り・指輪1・指輪2"),
	FLD (CHARINFO, dwGold,				"所持金",				FT_U32, RO, NULL),
	FLD (CHARINFO, dwExp,				"経験値",				FT_U32, RO, NULL),
	FLD (CHARINFO, wGrpID,				"画像ID",				FT_U16, 0, NULL),
	FLD (CHARINFO, wMakeYear,			"作成年",				FT_U16, RO, NULL),
	FLD (CHARINFO, wMakeMonth,			"作成月",				FT_U16, RO, NULL),
	FLD (CHARINFO, wMakeDay,			"作成日",				FT_U16, RO, NULL),
	FLD (CHARINFO, dwPlayTime,			"総プレイ時間(分)",		FT_U32, RO, NULL),
	FLD (CHARINFO, dwLoginCount,		"ログイン回数",			FT_U32, RO, NULL),
	FLD (CHARINFO, byCharState,			"キャラ状態",			FT_U8, RO, NULL),
	FLD (CHARINFO, byAccState,			"アカウント状態",		FT_U8, RO, NULL),
	FLD (CHARINFO, byRoofLV,			"屋根レベル",			FT_U8, RO, NULL),
	FLD (CHARINFO, bySaveFlg,			"保存用フラグ",			FT_U8, RO, NULL),
	FLD (CHARINFO, wGrpMain,			"画像の種別(メイン)",	FT_U16, 0, NULL),
	FLD (CHARINFO, wGrpSub,				"画像の種別(サブ)",		FT_U16, 0, NULL),
	FLD (CHARINFO, wGrpIDBack,			"画像ID(記録用)",		FT_U16, RO, NULL),
	FLD (CHARINFO, dwStartMapID,		"開始マップID",			FT_U32, RO, "map の dwID"),
	FLD (CHARINFO, dwStartX,			"開始X",				FT_U32, 0, NULL),
	FLD (CHARINFO, dwStartY,			"開始Y",				FT_U32, 0, NULL),
	FLD (CHARINFO, dwLeaveID,			"預かり所の番号",		FT_U32, RO, "leave の番号"),
	FLD (CHARINFO, clName,				"名前の色",				FT_U32, 0, "COLORREF（0x00BBGGRR）"),
	FLD (CHARINFO, clSpeech,			"発言の色",				FT_U32, 0, "COLORREF（0x00BBGGRR）"),
};

static const DATAFIELD s_aFieldMap[] = {
	FLD (MAPINFO, dwID,					"マップID",				FT_U32, RO, "キャラの dwMapID・イベントの移動先で使う番号"),
	FLD (MAPINFO, nCX,					"横のマス数",			FT_U32, RO, NULL),
	FLD (MAPINFO, nCY,					"縦のマス数",			FT_U32, RO, NULL),
	FLD (MAPINFO, nStartX,				"初期位置X",			FT_U32, 0, NULL),
	FLD (MAPINFO, nStartY,				"初期位置Y",			FT_U32, 0, NULL),
	FLD (MAPINFO, nEntranceX,			"入り口X",				FT_U32, 0, NULL),
	FLD (MAPINFO, nEntranceY,			"入り口Y",				FT_U32, 0, NULL),
	FLDK(MAPINFO, wBGM, "byBGM",		"BGM",					FT_U16, 0, "曲の番号。0=無し、1〜は MIDI の曲（管理者ウィンドウの BGM 一覧の順ではなく SOUND_ の番号）、1000〜は音声ファイル(.m4a)の曲（1000=始まりの町（新） 1001=爆弾ゲーム場（新））。キーは byBGM のまま（メンバー名は wBGM）"),
	FLD (MAPINFO, byFlg,				"フラグ",				FT_U8, 0, "ビット: 1=エフェクト可 2=戦闘可 4=サブゲーム 8=暗闇 16=マップ内放送 32=移動系効果禁止 64=魔法禁止 128=キャラの灯り無し(暗いマップでマップの光源だけで見える)"),
	FLD (MAPINFO, byLightLevel,			"明るさ",				FT_U8, 0, NULL),
	FLD (MAPINFO, byWeather,			"天候",					FT_U8, 0, NULL),
	FLD (MAPINFO, wViewHideParts,		"見えない所のパーツ",	FT_U16, 0, "見える範囲（byViewRange）の外に描く mapParts の番号。0 なら黒。森なら草原・木のパーツにすると、暗い洞窟ではなく見通しの悪い森のようになる（一度見た所は本当の地形を少し暗く描く）"),
	FLD (MAPINFO, byViewRange,			"見える範囲",			FT_U8, 0, "0 なら制限なし。1〜10 にすると、暗くはならないが自分から見える範囲（マス）の外と壁の向こうが見えなくなる（一度見た所は薄暗く残る）。暗くしない範囲（darkAreas）では効かない"),
};

static const DATAFIELD s_aFieldMapParts[] = {
	FLD (MAPPARTSINFO, byViewType,		"表示種別",				FT_U8, 0, NULL),
	FLD (MAPPARTSINFO, byHitType,		"当たり判定種別",		FT_U8, 0, "0 なら通れる。19=橋(縦に渡る。上下に進む人は橋の上、左右に進む人は橋の下) 20=橋(横に渡る)。橋の上では横を向けない"),
	FLD (MAPPARTSINFO, byAnimeType,		"アニメ種別",			FT_U8, 0, NULL),
	FLD (MAPPARTSINFO, byAnimeCount,	"アニメのコマ数",		FT_U8, RO, "anime の数（anime を変えると変わる）"),
	FLD (MAPPARTSINFO, byLevel,			"透明度",				FT_U8, 0, NULL),
	FLD (MAPPARTSINFO, byLight,			"灯りの範囲",			FT_U8, 0, "暗いマップでこのパーツが周りを照らす範囲（マス）。0 なら光らない。1〜7"),
	FLD (MAPPARTSINFO, byDamage,		"ダメージ床",			FT_U8, 0, "このパーツのマスに1歩入るたびに減るHP。0 ならダメージ床ではない。HPは1で止まり倒れない（毒沼・溶岩など）"),
	FLD (MAPPARTSINFO, byPoison,		"毒になる秒数",			FT_U8, 0, "ダメージ床を踏むと毒になる秒数。0 なら毒にならない。毒は2秒ごとに byDamage ずつ減る（毒・眠りの効きにくさで防げる）"),
	FLD (MAPPARTSINFO, byLow,			"低い",					FT_U8, 0, "1 なら低い物（水面・柵など）。ぶつかって通れないが、灯り・矢・飛ぶ特技・見通しは越える（カウンターと同じ）。0 なら普通"),
	FLD (MAPPARTSINFO, wGrpID,			"画像ID",				FT_U16, 0, NULL),
	FLD (MAPPARTSINFO, wBackID,			"重ねる時の背景ID",		FT_U16, 0, NULL),
};

static const DATAFIELD s_aFieldEfc[] = {
	FLD (EFCINFO, byType,				"画像の種別",			FT_U8, 0, NULL),
	FLD (EFCINFO, bySound,				"効果音ID",				FT_U8, 0, NULL),
};

/* マップのイベントの種別ごとの詳細 */
static const DATAFIELD s_aFieldEvMove[] = {
	FLD (EVENTINFO_MOVE, dwX,			"移動先X",				FT_U32, 0, NULL),
	FLD (EVENTINFO_MOVE, dwY,			"移動先Y",				FT_U32, 0, NULL),
	FLD (EVENTINFO_MOVE, byEfect,		"エフェクト",			FT_U8, 0, NULL),
};
static const DATAFIELD s_aFieldEvMapChg[] = {
	FLD (EVENTINFO_MAPCHG, dwMapID,		"移動先マップID",		FT_U32, 0, NULL),
	FLD (EVENTINFO_MAPCHG, dwX,			"移動先X",				FT_U32, 0, NULL),
	FLD (EVENTINFO_MAPCHG, dwY,			"移動先Y",				FT_U32, 0, NULL),
	FLD (EVENTINFO_MAPCHG, byEfect,		"エフェクト",			FT_U8, 0, NULL),
	FLD (EVENTINFO_MAPCHG, byRandomRange,	"ばらける範囲",			FT_U8, 0, "0=移動先ちょうど。1以上=移動先から何マス以内にある、移動先と同じパーツのマスから空き優先でくじ（船のベッドなど）"),
};
static const DATAFIELD s_aFieldEvMsg[] = {
	FLD (EVENTINFO_MSG, dwNo,			"メッセージ番号",		FT_U32, 0, "このマップの messages の番号"),
	FLD (EVENTINFO_MSG, byType,			"種別",					FT_U8, 0, "0=自動 1=調べる 2=話すと同じ"),
	FLD (EVENTINFO_MSG, wGrpID,			"書き換える画像ID",		FT_U16, 0, NULL),
	FLD (EVENTINFO_MSG, dwX,			"X",					FT_U32, 0, NULL),
	FLD (EVENTINFO_MSG, dwY,			"Y",					FT_U32, 0, NULL),
};
static const DATAFIELD s_aFieldEvDoor[] = {
	FLD (EVENTINFO_DOOR, wGrpID,		"扉の画像ID",			FT_U16, 0, NULL),
	FLD (EVENTINFO_DOOR, wBackGrpID,	"背景の画像ID",			FT_U16, 0, NULL),
	FLD (EVENTINFO_DOOR, byTimeClose,	"閉まるまで(100ms)",	FT_U8, 0, NULL),
	FLD (EVENTINFO_DOOR, byType,		"種別",					FT_U8, 0, "0=カギ不要 1=敵を全部倒す（範囲の敵がいない間は開きっぱなし、敵が出ると閉まる。扉コマンドでは開かない） 2=鍵（wKeyItem の品を大事な物の欄か持ち物に持っていれば開く。鍵は減らない。傭兵は開けられない）"),
	FLD (EVENTINFO_DOOR, wKeyItem,		"鍵の品",				FT_U16, 0, "種別2の時に必要な item の番号"),
	FLD (EVENTINFO_DOOR, wAreaX1,		"敵を数える範囲 左上X",	FT_U16, 0, "種別1の時に数える範囲。4つとも0ならマップ全体"),
	FLD (EVENTINFO_DOOR, wAreaY1,		"敵を数える範囲 左上Y",	FT_U16, 0, NULL),
	FLD (EVENTINFO_DOOR, wAreaX2,		"敵を数える範囲 右下X",	FT_U16, 0, NULL),
	FLD (EVENTINFO_DOOR, wAreaY2,		"敵を数える範囲 右下Y",	FT_U16, 0, NULL),
	FLD (EVENTINFO_DOOR, wEnemyStTmp,	"数える敵の種類",		FT_U16, 0, "種別1の時に数える敵の stTmp（NPC のステータステンプレート番号）。その stTmp の敵だけ数える。65535=指定なしで範囲の敵を全部数える（既定）"),
	FLDS(EVENTINFO_DOOR, szLockMsg,		"鍵がない時の文",		FT_STR, 0, "種別2（鍵）の時だけ使える。鍵を持っていない人が開けようとした時に、その人にだけ出る文（最大 95 バイト。末尾に @2 が無ければ自動で付く）。空なら既定の「鍵がかかっている。」。種別が2以外の扉に空でない文は指定できない"),
};
static const DATAFIELD s_aFieldEvHpMana[] = {
	FLD (EVENTINFO_HPMANA, byType,		"種別",					FT_U8, 0, NULL),
};
static const DATAFIELD s_aFieldEvOnCharMap[] = {
	FLD (EVENTINFO_ONCHARMAP, wGrpIDOn,	"乗っている時の画像ID",	FT_U16, 0, NULL),
	FLD (EVENTINFO_ONCHARMAP, wGrpIDOff,	"乗っていない時の画像ID",	FT_U16, 0, NULL),
	FLD (EVENTINFO_ONCHARMAP, dwX,		"X",					FT_U32, 0, NULL),
	FLD (EVENTINFO_ONCHARMAP, dwY,		"Y",					FT_U32, 0, NULL),
};

static const DATAFIELD *EventFields(BYTE byType, int &nCount)
{
	switch (byType) {
	case EVENTTYPE_MOVE:		nCount = COUNTOF(s_aFieldEvMove);		return s_aFieldEvMove;
	case EVENTTYPE_MAPCHG:		nCount = COUNTOF(s_aFieldEvMapChg);		return s_aFieldEvMapChg;
	case EVENTTYPE_MSG:			nCount = COUNTOF(s_aFieldEvMsg);		return s_aFieldEvMsg;
	case EVENTTYPE_DOOR:		nCount = COUNTOF(s_aFieldEvDoor);		return s_aFieldEvDoor;
	case EVENTTYPE_HPMANA:		nCount = COUNTOF(s_aFieldEvHpMana);		return s_aFieldEvHpMana;
	case EVENTTYPE_ONCHARMAP:	nCount = COUNTOF(s_aFieldEvOnCharMap);	return s_aFieldEvOnCharMap;
	}
	nCount = 0;
	return NULL;
}

static std::string TextJson(LPCSTR psz)
{
	return JsonStr(psz ? psz : "");
}

/* キャラ 1 件 */
static std::string CharJson(PCHARINFO pChar)
{
	/* 見える・見えない条件の印は、中では「番号+1（0=無し）」。外には「番号（65535=無し）」で出す */
	CHARINFO Tmp;
	CopyMemory(&Tmp, pChar, sizeof(Tmp));
	Tmp.wShowFlag = CharFlagFromWire(pChar->wShowFlag);
	Tmp.wHideFlag = CharFlagFromWire(pChar->wHideFlag);
	std::string out = FieldsJson((const BYTE *)&Tmp, s_aFieldChar, COUNTOF(s_aFieldChar));
	bool bPlayer = (pChar->byNpcType == NPCTYPE_INIT) && pChar->szAcc[0];
	out += ",\"npcId\":" + JsonNum(pChar->dwNpcID);
	out += std::string(",\"isPlayer\":") + (bPlayer ? "true" : "false");
	out += std::string(",\"online\":") + ((bPlayer && pChar->pID) ? "true" : "false");
	out += ",\"szNpcSpeech\":" + TextJson(pChar->pszNpcSpeech);
	/* 転職前の職業で覚えていた特技（職業の番号: [magic の番号...]、空の職業は省く） */
	out += ",\"jobMagic\":{";
	bool bFirst = true;
	for (int j = 0; j < MAX_JOBMAGIC; j ++) {
		std::string strRow;
		for (int k = 0; k < MAX_USEMAGIC; k ++) {
			if (pChar->abyJobMagic[j][k] == 0) {
				continue;
			}
			if (!strRow.empty()) strRow += ",";
			strRow += JsonNum(pChar->abyJobMagic[j][k]);
		}
		if (strRow.empty()) {
			continue;
		}
		if (!bFirst) out += ",";
		bFirst = false;
		out += "\"" + JsonNum(j) + "\":[" + strRow + "]";
	}
	out += "}";
	/* 巡回ルート（動かない・ランダム移動の村人に道しるべが2つ以上あると巡回する） */
	out += ",\"route\":{\"mode\":" + JsonNum(pChar->byRouteMode) + ",\"wait\":" + JsonNum(pChar->byRouteWait) + ",\"points\":[";
	for (int i = 0; i < pChar->byRouteCount && i < MAX_NPCROUTE; i ++) {
		if (i) out += ",";
		out += "[" + JsonNum(pChar->abyRouteX[i]) + "," + JsonNum(pChar->abyRouteY[i]) + "]";
	}
	out += "]}";
	/* 町転送の行き先帳（新しく記録した順。[マップID, 横, 縦]） */
	out += ",\"townWarp\":[";
	for (int i = 0; i < pChar->byTownWarpCount && i < MAX_TOWNWARP; i ++) {
		if (i) out += ",";
		out += "[" + JsonNum(pChar->adwTownWarpMapID[i]) + "," + JsonNum(pChar->abyTownWarpX[i]) + "," + JsonNum(pChar->abyTownWarpY[i]) + "]";
	}
	out += "]";
	/* ギルドの依頼（読むだけ。受けている依頼の番号・段・旗・数） */
	out += ",\"quest\":{\"questId\":" + JsonNum(pChar->wQuestID) + ",\"step\":" + JsonNum(pChar->byQuestStep) +
			",\"flags\":" + JsonNum(pChar->wQuestFlg) + ",\"counts\":[";
	for (int i = 0; i < MAX_QUESTCOUNT; i ++) {
		if (i) out += ",";
		out += JsonNum(pChar->awQuestCount[i]);
	}
	out += "]";
	/* クリア済みの依頼の番号（abyQuestDone）と、報酬をもうもらっている依頼の番号（abyQuestRewarded。思い出の本でやり直しても消えない） */
	for (int nKind = 0; nKind < 2; nKind ++) {
		const BYTE *pbyBits = nKind ? pChar->abyQuestRewarded : pChar->abyQuestDone;
		bool bFirstQ = true;

		out += nKind ? ",\"questRewarded\":[" : ",\"questDone\":[";
		for (int i = 0; i < MAX_QUEST; i ++) {
			if (pbyBits[i / 8] & (1 << (i % 8))) {
				if (!bFirstQ) out += ",";
				bFirstQ = false;
				out += JsonNum(i);
			}
		}
		out += "]";
	}
	out += "}";
	/* キャラのフラグ（読むだけ。立っている印の番号と、0 でない数値。プレイヤーだけ） */
	if (bPlayer) {
		out += ",\"flags\":{\"bits\":[";
		bool bFirst = true;
		for (int i = 0; i < MAX_FLAGBIT; i ++) {
			if (pChar->abyFlag[i / 8] & (1 << (i % 8))) {
				if (!bFirst) out += ",";
				bFirst = false;
				out += JsonNum(i);
			}
		}
		out += "],\"nums\":{";
		bFirst = true;
		for (int i = 0; i < MAX_FLAGNUM; i ++) {
			if (pChar->abyFlagNum[i]) {
				if (!bFirst) out += ",";
				bFirst = false;
				out += "\"" + JsonNum(i) + "\":" + JsonNum(pChar->abyFlagNum[i]);
			}
		}
		out += "}";
		/* 開けた宝箱の番号（読むだけ。印とは別の記録） */
		out += ",\"chests\":[";
		bFirst = true;
		for (int i = 0; i < MAX_CHESTNO; i ++) {
			if (pChar->abyChestOpened[i / 8] & (1 << (i % 8))) {
				if (!bFirst) out += ",";
				bFirst = false;
				out += JsonNum(i);
			}
		}
		out += "]}";
	}
	return out;
}

/* 依頼 1 件（goals は段の数ぶん、npcs は使っている欄だけ。bID=false なら wID（項目表の先頭）を入れない） */
static std::string QuestJson(const QUESTINFO *pQuest, bool bID = true)
{
	std::string out = bID ? FieldsJson((const BYTE *)pQuest, s_aFieldQuest, COUNTOF(s_aFieldQuest)) :
			FieldsJson((const BYTE *)pQuest, s_aFieldQuest + 1, COUNTOF(s_aFieldQuest) - 1);
	bool bFirst = true;

	out += ",\"goals\":[";
	for (int i = 0; i < MAX_QUESTSTEP; i ++) {
		if (i >= max((int)pQuest->byStepCount, 1) && pQuest->aszGoal[i][0] == 0) {
			break;
		}
		if (i) out += ",";
		out += JsonStr(pQuest->aszGoal[i]);
	}
	out += "],\"npcs\":[";
	for (int i = 0; i < MAX_QUESTNPC; i ++) {
		if (pQuest->aNpc[i].byKind == QUESTNPCKIND_NONE) {
			continue;
		}
		if (!bFirst) out += ",";
		bFirst = false;
		out += "{" + FieldsJson((const BYTE *)&pQuest->aNpc[i], s_aFieldQuestNpc, COUNTOF(s_aFieldQuestNpc)) + "}";
	}
	out += "]";
	return out;
}

/* 会話セットの 1 行を「無し」で埋めた姿にする（0 で埋めると印 0・数値 0 を指す条件になるので） */
static void TalkLineInit(TALKLINEINFO *pLine)
{
	memset(pLine, 0, sizeof(*pLine));
	for (int i = 0; i < MAX_TALKNEED; i ++) {
		pLine->awNeed[i] = FLAG_NOBIT;
		pLine->awNot[i] = FLAG_NOBIT;
	}
	for (int i = 0; i < MAX_TALKCOND; i ++) {
		pLine->abyCondNo[i] = FLAG_NONUM;
	}
	for (int i = 0; i < MAX_TALKSET; i ++) {
		pLine->awSetOn[i] = FLAG_NOBIT;
		pLine->awSetOff[i] = FLAG_NOBIT;
	}
	for (int i = 0; i < MAX_TALKACT; i ++) {
		pLine->abyActNo[i] = FLAG_NONUM;
	}
	pLine->byQuestStepMax = QUESTNPC_NOSTEP;		/* 依頼の段の上限は「どの段でも」から */
}

/* 会話セット 1 件（lines は使っている行だけ） */
static std::string TalkJson(const TALKINFO *pTalk)
{
	std::string out = FieldsJson((const BYTE *)pTalk, s_aFieldTalk, COUNTOF(s_aFieldTalk));

	out += ",\"lines\":[";
	for (int i = 0; i < MAX_TALKLINE && i < (int)pTalk->byLineCount; i ++) {
		if (i) out += ",";
		out += "{" + FieldsJson((const BYTE *)&pTalk->aLine[i], s_aFieldTalkLine, COUNTOF(s_aFieldTalkLine)) + "}";
	}
	out += "]";
	return out;
}

/* 思い出の本の「いまの目的」の 1 行を「無し」で埋めた姿にする（0 で埋めると印 0・数値 0 を指す条件になるので） */
static void StoryGoalInit(STORYGOAL *pGoal)
{
	memset(pGoal, 0, sizeof(*pGoal));
	for (int i = 0; i < MAX_TALKNEED; i ++) {
		pGoal->awNeed[i] = FLAG_NOBIT;
		pGoal->awNot[i] = FLAG_NOBIT;
	}
	for (int i = 0; i < MAX_TALKCOND; i ++) {
		pGoal->abyCondNo[i] = FLAG_NONUM;
	}
}

/* 「いまの目的」の行が使われているか（文があるか、「無し」でない条件がある） */
static bool StoryGoalUsed(const STORYGOAL *pGoal)
{
	if (pGoal->szGoal[0]) {
		return true;
	}
	for (int i = 0; i < MAX_TALKNEED; i ++) {
		if (pGoal->awNeed[i] != FLAG_NOBIT || pGoal->awNot[i] != FLAG_NOBIT) {
			return true;
		}
	}
	for (int i = 0; i < MAX_TALKCOND; i ++) {
		if (pGoal->abyCondNo[i] != FLAG_NONUM) {
			return true;
		}
	}
	return false;
}

/* シナリオ 1 件（goals は使っている最後の行まで） */
static std::string StoryJson(const STORYINFO *pStory)
{
	std::string out = FieldsJson((const BYTE *)pStory, s_aFieldStory, COUNTOF(s_aFieldStory));
	int nLast = 0;

	for (int i = 0; i < MAX_STORYGOAL; i ++) {
		if (StoryGoalUsed(&pStory->aGoal[i])) {
			nLast = i + 1;
		}
	}
	out += ",\"goals\":[";
	for (int i = 0; i < nLast; i ++) {
		if (i) out += ",";
		out += "{" + FieldsJson((const BYTE *)&pStory->aGoal[i], s_aFieldStoryGoal, COUNTOF(s_aFieldStoryGoal)) + "}";
	}
	out += "]";
	return out;
}

/* マップ 1 件（マス目は入れない。SboWebDataMapTiles で読む） */
static std::string MapJson(PMAPINFO pMap)
{
	std::string out = FieldsJson((const BYTE *)pMap, s_aFieldMap, COUNTOF(s_aFieldMap));
	int i, j;
	out += ",\"szName\":" + TextJson(pMap->pszName);

	/* 暗くする範囲（dark 1=暗くする 0=暗くしない 2=明るいが壁の向こうは見えない。無ければマップ全体が明るさの設定どおり） */
	out += ",\"darkAreas\":[";
	for (i = 0; i < pMap->byDarkAreaCount && i < MAX_MAPDARKAREA; i ++) {
		const MAPDARKAREA *pArea = &pMap->aDarkArea[i];
		if (i) out += ",";
		out += "{\"x1\":" + JsonNum(pArea->wX1) + ",\"y1\":" + JsonNum(pArea->wY1) +
				",\"x2\":" + JsonNum(pArea->wX2) + ",\"y2\":" + JsonNum(pArea->wY2) +
				",\"dark\":" + JsonNum(pArea->byDark) + "}";
	}
	out += "]";

	/* 光源（暗いマップで、その位置から range マスまで照らす。パーツの灯りと同じ） */
	out += ",\"lights\":[";
	for (i = 0; i < pMap->byLightCount && i < MAX_MAPLIGHT; i ++) {
		const MAPLIGHT *pLight = &pMap->aLight[i];
		if (i) out += ",";
		out += "{\"x\":" + JsonNum(pLight->wX) + ",\"y\":" + JsonNum(pLight->wY) +
				",\"range\":" + JsonNum(pLight->byRange) +
				",\"power\":" + JsonNum(pLight->byPower ? pLight->byPower : 100) +
				",\"flicker\":" + JsonNum((pLight->byFlg & MAPLIGHTFLG_NOFLICKER) ? 0 : 1) +
				",\"warm\":" + JsonNum((pLight->byFlg & MAPLIGHTFLG_WARM) ? 1 : 0) + "}";
	}
	out += "]";

	/* パーツの灯りの上書き（全マップ共通のパーツの byLight を、このマップだけ変える。0=光らない） */
	out += ",\"partsLights\":[";
	for (i = 0; i < pMap->byPartsLightCount && i < MAX_MAPPARTSLIGHT; i ++) {
		const MAPPARTSLIGHT *pPL = &pMap->aPartsLight[i];
		if (i) out += ",";
		out += "{\"wParts\":" + JsonNum(pPL->wParts) + ",\"byLight\":" + JsonNum(pPL->byLight) + "}";
	}
	out += "]";

	/* 印で変わるマス（kind 0=印で変わる・1=宝箱・2=リサイクル箱。見た目と通れるかを、その人の印・開けた宝箱の記録・開いているリサイクル箱で差し替える） */
	out += ",\"swaps\":[";
	for (i = 0; i < pMap->bySwapCount && i < MAX_MAPSWAP; i ++) {
		const MAPSWAP *pSw = &pMap->aSwap[i];
		if (i) out += ",";
		out += "{\"kind\":" + JsonNum(pSw->byKind) + ",\"x\":" + JsonNum(pSw->wX) + ",\"y\":" + JsonNum(pSw->wY) +
				",\"wParts\":" + JsonNum(pSw->wParts);
		if (pSw->byKind == MAPSWAPKIND_CHEST) {
			out += ",\"wChestNo\":" + JsonNum(pSw->wChestNo) + ",\"wGiveItem\":" + JsonNum(pSw->wGiveItem) +
					",\"szMsg\":" + JsonStr(pSw->szMsg) + ",\"szMemo\":" + JsonStr(pSw->szMemo) +
					",\"wNeedFlag\":" + JsonNum(pSw->wNeedFlag) + ",\"szLockMsg\":" + JsonStr(pSw->szLockMsg);
		} else if (pSw->byKind == MAPSWAPKIND_RECYCLE) {
			out += ",\"wBoxNo\":" + JsonNum(pSw->wBoxNo) + ",\"szMemo\":" + JsonStr(pSw->szMemo);
		} else {
			out += ",\"wFlag\":" + JsonNum(pSw->wFlag) + ",\"byCmp\":" + JsonNum(pSw->byCmp);
		}
		out += "}";
	}
	out += "]";

	/* イベント */
	out += ",\"events\":[";
	for (i = 0; i < pMap->nEventCount && pMap->pEvent; i ++) {
		PEVENTINFO pEv = &pMap->pEvent[i];
		int nCount = 0;
		const DATAFIELD *paField = EventFields(pEv->byType, nCount);
		if (i) out += ",";
		out += "{\"no\":" + JsonNum(i) + ",\"x\":" + JsonNum(pEv->dwX) + ",\"y\":" + JsonNum(pEv->dwY) +
				",\"type\":" + JsonNum(pEv->byType) + ",\"detail\":{";
		if (paField && pEv->pData) {
			out += FieldsJson((const BYTE *)pEv->pData, paField, nCount);
			/* 扉の追加マス（0 件の時は出さない） */
			if (pEv->byType == EVENTTYPE_DOOR) {
				PEVENTINFO_DOOR pDoor = (PEVENTINFO_DOOR)pEv->pData;
				if (pDoor->byExtraCount) {
					out += ",\"extraCells\":[";
					for (j = 0; j < pDoor->byExtraCount && j < MAX_DOOREXTRA; j ++) {
						if (j) out += ",";
						out += "{\"dx\":" + JsonNum(pDoor->aExtra[j].sbyOffX) + ",\"dy\":" + JsonNum(pDoor->aExtra[j].sbyOffY) +
								",\"closed\":" + JsonNum(pDoor->aExtra[j].wClosedParts) + ",\"open\":" + JsonNum(pDoor->aExtra[j].wOpenParts) + "}";
					}
					out += "]";
				}
			}
		}
		out += "}}";
	}
	out += "]";

	/* 店（NPC の台詞 "@5NN" の NN が店の番号） */
	out += ",\"shops\":[";
	for (i = 0; i < pMap->byCountShop && pMap->pShopInfo; i ++) {
		PSHOPINFO pShop = &pMap->pShopInfo[i];
		if (i) out += ",";
		out += "{\"no\":" + JsonNum(i) + ",\"items\":[";
		for (j = 0; j < pShop->byCountItem && j < MAX_SHOPITEM; j ++) {
			if (j) out += ",";
			out += JsonNum(pShop->awItem[j]);
		}
		out += "],\"messages\":[";
		for (j = 0; j < MAX_SHOPMSG; j ++) {
			if (j) out += ",";
			out += TextJson(pShop->apMsg[j]);
		}
		out += "]}";
	}
	out += "]";

	/* メッセージ（イベントのメッセージ番号から参照される） */
	out += ",\"messages\":[";
	for (i = 0; i < pMap->nMsgCount && pMap->pMsg; i ++) {
		if (i) out += ",";
		out += TextJson(pMap->pMsg[i]);
	}
	out += "]";

	/* 置いてあるアイテム */
	out += ",\"items\":[";
	for (i = 0; i < pMap->nItemCount && pMap->pItem; i ++) {
		PITEMPOSINFO pItem = &pMap->pItem[i];
		if (i) out += ",";
		out += "{\"item\":" + JsonNum(pItem->dwID) + ",\"x\":" + JsonNum(pItem->dwX) + ",\"y\":" + JsonNum(pItem->dwY) + "}";
	}
	out += "]";
	return out;
}

/* マップパーツ 1 件（アニメするパーツはコマごとの画像も。画面に出るのは 1 コマ目から） */
static std::string MapPartsJson(PMAPPARTSINFO pParts)
{
	std::string out = FieldsJson((const BYTE *)pParts, s_aFieldMapParts, COUNTOF(s_aFieldMapParts));
	out += ",\"anime\":[";
	for (int i = 0; pParts->pAnimeInfo && i < pParts->byAnimeCount; i ++) {
		PANIMEINFO pAnime = &pParts->pAnimeInfo[i];
		if (i) out += ",";
		out += "{\"byWait\":" + JsonNum(pAnime->byWait) + ",\"byLevel\":" + JsonNum(pAnime->byLevel) +
				",\"wGrpID\":" + JsonNum(pAnime->wGrpID) + ",\"wBackID\":" + JsonNum(pAnime->wBackID) + "}";
	}
	return out + "]";
}

static const DATAFIELD s_aFieldComEfc[] = {
	FLD (COMEFCINFO, bySound,			"鳴らす音",				FT_U8, 0, NULL),
};

static std::string ComEfcJson(PCOMEFCINFO pInfo)
{
	std::string out = FieldsJson((const BYTE *)pInfo, s_aFieldComEfc, COUNTOF(s_aFieldComEfc));
	out += ",\"anime\":[";
	for (int i = 0; pInfo->pAnime && i < pInfo->byAnimeCount; i ++) {
		if (i) out += ",";
		out += "{\"byWait\":" + JsonNum(pInfo->pAnime[i].byWait) + ",\"byGrpID\":" + JsonNum(pInfo->pAnime[i].byGrpID) + "}";
	}
	return out + "]";
}

static std::string LeaveJson(PLEAVEINFO pInfo)
{
	std::string out = "\"byUse\":" + JsonNum(pInfo->byUse) + ",\"dwGold\":" + JsonNum(pInfo->dwGold) + ",\"items\":[";
	int nCount = (pInfo->byCount < MAX_LEAVEITEM) ? pInfo->byCount : MAX_LEAVEITEM;
	for (int i = 0; i < nCount; i ++) {
		if (i) out += ",";
		out += "{\"item\":" + JsonNum(pInfo->waItem[i]) + ",\"count\":" + JsonNum(pInfo->byItemCount[i]) + "}";
	}
	return out + "]";
}

/* サブゲームの種別ごとの詳細（保存される項目だけ） */
static const DATAFIELD s_aFieldSgBomb[] = {
	FLD (SUBGAME_BOMBINFO, dwMapID,		"マップID",				FT_U32, 0, "map の dwID"),
	FLD (SUBGAME_BOMBINFO, dwX,			"参加受付X",			FT_U32, 0, NULL),
	FLD (SUBGAME_BOMBINFO, dwY,			"参加受付Y",			FT_U32, 0, NULL),
	FLD (SUBGAME_BOMBINFO, dwStartX,	"参加開始X",			FT_U32, 0, NULL),
	FLD (SUBGAME_BOMBINFO, dwStartY,	"参加開始Y",			FT_U32, 0, NULL),
	FLD (SUBGAME_BOMBINFO, byX1,		"エリア開始X",			FT_U8, 0, NULL),
	FLD (SUBGAME_BOMBINFO, byY1,		"エリア開始Y",			FT_U8, 0, NULL),
	FLD (SUBGAME_BOMBINFO, byX2,		"エリア終了X",			FT_U8, 0, NULL),
	FLD (SUBGAME_BOMBINFO, byY2,		"エリア終了Y",			FT_U8, 0, NULL),
	FLD (SUBGAME_BOMBINFO, byGameTime,	"制限時間",				FT_U8, 0, NULL),
	FLD (SUBGAME_BOMBINFO, byMaxBomb,	"最大設置数",			FT_U8, 0, NULL),
	FLD (SUBGAME_BOMBINFO, byMaxFire,	"最大火力",				FT_U8, 0, NULL),
};
static const DATAFIELD s_aFieldSgMaze[] = {
	FLD (SUBGAME_MAZEINFO, dwMapID,		"マップID",				FT_U32, 0, "map の dwID"),
	FLD (SUBGAME_MAZEINFO, byX1,		"エリア開始X",			FT_U8, 0, NULL),
	FLD (SUBGAME_MAZEINFO, byY1,		"エリア開始Y",			FT_U8, 0, NULL),
	FLD (SUBGAME_MAZEINFO, byX2,		"エリア終了X",			FT_U8, 0, NULL),
	FLD (SUBGAME_MAZEINFO, byY2,		"エリア終了Y",			FT_U8, 0, NULL),
	FLD (SUBGAME_MAZEINFO, byCharX,		"管理人X",				FT_U8, 0, NULL),
	FLD (SUBGAME_MAZEINFO, byCharY,		"管理人Y",				FT_U8, 0, NULL),
	FLD (SUBGAME_MAZEINFO, byStartX,	"スタートX",			FT_U8, 0, NULL),
	FLD (SUBGAME_MAZEINFO, byStartY,	"スタートY",			FT_U8, 0, NULL),
	FLD (SUBGAME_MAZEINFO, byGoalFromX,	"ゴール(移動元)X",		FT_U8, FF_READONLY, "迷路を作るたびに変わるゴールの位置（読むだけ）"),
	FLD (SUBGAME_MAZEINFO, byGoalFromY,	"ゴール(移動元)Y",		FT_U8, FF_READONLY, "迷路を作るたびに変わるゴールの位置（読むだけ）"),
	FLD (SUBGAME_MAZEINFO, byGoalToX,	"ゴール(移動先)X",		FT_U8, 0, NULL),
	FLD (SUBGAME_MAZEINFO, byGoalToY,	"ゴール(移動先)Y",		FT_U8, 0, NULL),
	FLD (SUBGAME_MAZEINFO, wChipIDStart,"開始地点へのパーツ",	FT_U16, 0, "mapParts の番号"),
	FLD (SUBGAME_MAZEINFO, wChipIDGoal,	"ゴールのパーツ",		FT_U16, 0, "mapParts の番号"),
	FLD (SUBGAME_MAZEINFO, wChipIDRoad,	"道のパーツ",			FT_U16, 0, "mapParts の番号"),
	FLD (SUBGAME_MAZEINFO, wChipIDBlock,"壁のパーツ",			FT_U16, 0, "mapParts の番号"),
	FLD (SUBGAME_MAZEINFO, wChipIDRoad2,"道に混ぜるパーツ",		FT_U16, 0, "mapParts の番号。0=混ぜない。道のマスを割合（byRoad2Rate）でこのパーツにする（落ち葉・ひび等）"),
	FLD (SUBGAME_MAZEINFO, byRoad2Rate,	"道に混ぜる割合(%)",	FT_U8, 0, "0〜100"),
	FLD (SUBGAME_MAZEINFO, wChipIDBlock2,"壁に混ぜるパーツ",	FT_U16, 0, "mapParts の番号。0=混ぜない。壁のマスを割合（byBlock2Rate）でこのパーツにする（コケ・ひび等）"),
	FLD (SUBGAME_MAZEINFO, byBlock2Rate,"壁に混ぜる割合(%)",	FT_U8, 0, "0〜100"),
	FLD (SUBGAME_MAZEINFO, wChipIDBlockFace,"壁の手前の面のパーツ",FT_U16, 0, "mapParts の番号。0=使わない。すぐ下が道の壁の下の段をこのパーツにして立体に見せる"),
	FLD (SUBGAME_MAZEINFO, byPathWidth,	"通路の太さ",			FT_U8, 0, "1〜3マス。0=2マス（昔から）。迷路の1区画を何マス四方にするか。作り直した時から効く"),
	FLD (SUBGAME_MAZEINFO, wChipIDBridgeV,"立体交差の橋(縦)",	FT_U16, 0, "mapParts の番号。0=使わない。当たり判定が 19（橋・縦に渡る）のパーツ。上下に進む人は橋の上、左右に進む人は橋の下を通る"),
	FLD (SUBGAME_MAZEINFO, wChipIDBridgeH,"立体交差の橋(横)",	FT_U16, 0, "mapParts の番号。0=使わない。当たり判定が 20（橋・横に渡る）のパーツ"),
	FLD (SUBGAME_MAZEINFO, byCrossRate,	"立体交差の多さ",		FT_U8, 0, "0〜100（%）。0=作らない。交差できる場所のうち、橋にする割合。橋のパーツ（縦・横のどちらか）が要る。作り直した時から効く"),
};
static const DATAFIELD s_aFieldSgDungeon[] = {
	FLD (SUBGAME_DUNGEONINFO, dwMapID,	"マップID",				FT_U32, 0, "map の dwID"),
	FLD (SUBGAME_DUNGEONINFO, byX1,		"エリア開始X",			FT_U8, 0, NULL),
	FLD (SUBGAME_DUNGEONINFO, byY1,		"エリア開始Y",			FT_U8, 0, NULL),
	FLD (SUBGAME_DUNGEONINFO, byX2,		"エリア終了X",			FT_U8, 0, NULL),
	FLD (SUBGAME_DUNGEONINFO, byY2,		"エリア終了Y",			FT_U8, 0, NULL),
	FLD (SUBGAME_DUNGEONINFO, byCharX,	"管理人X",				FT_U8, 0, NULL),
	FLD (SUBGAME_DUNGEONINFO, byCharY,	"管理人Y",				FT_U8, 0, NULL),
	FLD (SUBGAME_DUNGEONINFO, byStartX,	"スタートX",			FT_U8, 0, NULL),
	FLD (SUBGAME_DUNGEONINFO, byStartY,	"スタートY",			FT_U8, 0, NULL),
	FLD (SUBGAME_DUNGEONINFO, byGoalX,	"ゴール(移動元)X",		FT_U8, 0, NULL),
	FLD (SUBGAME_DUNGEONINFO, byGoalY,	"ゴール(移動元)Y",		FT_U8, 0, NULL),
	FLD (SUBGAME_DUNGEONINFO, wChipIDStart,"開始地点へのパーツ",FT_U16, 0, "mapParts の番号"),
	FLD (SUBGAME_DUNGEONINFO, wChipIDGoal,"ゴールのパーツ",		FT_U16, 0, "mapParts の番号"),
	FLD (SUBGAME_DUNGEONINFO, wChipIDRoad,"道のパーツ",			FT_U16, 0, "mapParts の番号"),
	FLD (SUBGAME_DUNGEONINFO, wChipIDBlock,"壁のパーツ",		FT_U16, 0, "mapParts の番号"),
	FLD (SUBGAME_DUNGEONINFO, dwThemeID,"テーマ番号",			FT_U32, 0, "ダンジョン生成のテーマ（SboWebDungeonThemes.json の id）。0=今までの作り方。0 以外でテーマが見つかれば、作り直しのたびにエリアの大きさで生成してマス目・扉・入口を置く（見つからなければ今までの作り方）"),
	FLD (SUBGAME_DUNGEONINFO, dwBuildNo,"作り直し番号",			FT_U32, FF_READONLY, "テーマで作り直すたびに増える番号（実行中だけ。保存しない・読むだけ）"),
	FLD (SUBGAME_DUNGEONINFO, byTmplCount,"番外編のテンプレート数",	FT_U8, 0, "0〜8。0=番外編にしない（今までどおり、入口に乗れば誰でも自分の姿で入れる）。1 以上だと番外編のミニゲームになる: 入口にパーティのリーダー（ソロならその人）が乗ると、組の全員に wTmplID・szTmplName1〜8 の先頭 byTmplCount 件を一覧に出し、全員が選び終えたら（30秒で締め切り）一緒に新しい空間へ入る。中ではステータステンプレートの姿になり、経験値・お金は入らない。拾った物・ドロップは中で使えるが、外へは持ち帰れない（どの出方でも元の持ち物に戻る）。テーマ番号（dwThemeID）が 0 でない時だけ使える"),
	FLDA(SUBGAME_DUNGEONINFO, wTmplID,	"番外編のテンプレート番号",	FT_U16ARR, WORD, 0, "stTmp の番号（8 個。先頭 byTmplCount 件を使う）。存在しない番号は一覧から外す"),
	{ "szTmplName1", "番外編の表示名1", FT_STR, offsetof(SUBGAME_DUNGEONINFO, szTmplName[0]), MAX_DGTMPLNAME, 0, "wTmplID の先頭から数えて同じ番号のテンプレートの、選ぶ窓に出す名前（空ならテンプレートの名前）" },
	{ "szTmplName2", "番外編の表示名2", FT_STR, offsetof(SUBGAME_DUNGEONINFO, szTmplName[1]), MAX_DGTMPLNAME, 0, NULL },
	{ "szTmplName3", "番外編の表示名3", FT_STR, offsetof(SUBGAME_DUNGEONINFO, szTmplName[2]), MAX_DGTMPLNAME, 0, NULL },
	{ "szTmplName4", "番外編の表示名4", FT_STR, offsetof(SUBGAME_DUNGEONINFO, szTmplName[3]), MAX_DGTMPLNAME, 0, NULL },
	{ "szTmplName5", "番外編の表示名5", FT_STR, offsetof(SUBGAME_DUNGEONINFO, szTmplName[4]), MAX_DGTMPLNAME, 0, NULL },
	{ "szTmplName6", "番外編の表示名6", FT_STR, offsetof(SUBGAME_DUNGEONINFO, szTmplName[5]), MAX_DGTMPLNAME, 0, NULL },
	{ "szTmplName7", "番外編の表示名7", FT_STR, offsetof(SUBGAME_DUNGEONINFO, szTmplName[6]), MAX_DGTMPLNAME, 0, NULL },
	{ "szTmplName8", "番外編の表示名8", FT_STR, offsetof(SUBGAME_DUNGEONINFO, szTmplName[7]), MAX_DGTMPLNAME, 0, NULL },
	FLD (SUBGAME_DUNGEONINFO, byRebuildMode,"作り直しの方式",	FT_U8, 0, "2=作り直さない（範囲のマス目は手で作ったものをそのまま使う。生成・作り直し・探索マップ用の情報は作らない。入り口のマスにワープのイベントが無ければ自動で置く。番外編＝byTmplCount 1 以上で使い、テーマ番号は不要）。0=今まで通り（誰もいなくなって 5 秒たつと、そのつど新しいシードで作り直す＝気まぐれ）。1=日替わり（その日のシードは日付（日本時間）とダンジョン番号＝このサブゲームの番号から決まり、同じ日なら何度作っても・サーバーを再起動しても同じ形。日付が変わったら、範囲に誰もいない時に作り直す。誰かいる間は作り直さず、新しく入る組は前の日の形に入る。無人 5 秒の作り直しはしない。テーマ番号（dwThemeID）が 0 でない時だけ有効）。日替わりだと、番外編（byTmplCount 1 以上）の敵の出る位置が版のシードで決まりどの組も同じになり、最短記録がつく（読むのは /api/data/dungeon/records、今すぐ作り直すのは /api/data/dungeon/rebuild）"),
	FLD (SUBGAME_DUNGEONINFO, byClearMode,"クリア条件",		FT_U8, 0, "2=拠点防衛（byRebuildMode=2 と組み合わせる。ゴール地点に拠点が立ち、ウェーブごとに敵が来る。拠点が倒れると負け、全部のウェーブをしのぐと勝ち。結果は組の全員に知らせて 10 秒後に全員外へ戻る。経験値・お金なし）。0=ゴールのワープに着く（今まで通り）。1=敵を全部倒す（その組の空間の、NPC発生の「補充しない」印の敵を全部倒した時。そういう発生元が範囲に無ければ 0 と同じ）。クリアの瞬間にクリア時間が決まり、組の全員に知らせて 10 秒後に全員（倒れている人も）外へ戻る。記録は 0 と同じ扱い。番外編（byTmplCount 1 以上）で使う"),
	FLD (SUBGAME_DUNGEONINFO, wDefBaseHP,	"拠点の最大HP",			FT_U16, 0, "拠点防衛（byClearMode=2）の時。0=1000。拠点の位置はゴール地点（byGoalX・byGoalY）、プレイヤーの開始位置は入り口（byStartX・byStartY）"),
	FLD (SUBGAME_DUNGEONINFO, wDefBaseStTmp,"拠点の見た目のテンプレート",	FT_U16, 0, "拠点防衛の時。拠点キャラの見た目に使うステータステンプレートの番号（stTmp）。0=既定"),
	FLD (SUBGAME_DUNGEONINFO, byDefWaveCount,"ウェーブの数",			FT_U8, 0, "拠点防衛の時。1〜。NPC発生の「防衛のウェーブ番号」（byPutWave）が 1〜この数の発生元がそのウェーブに出る。最後のウェーブを全部倒すと勝ち"),
	FLD (SUBGAME_DUNGEONINFO, byDefPrepSec,	"準備時間(秒)",		FT_U8, 0, "拠点防衛の時。開始前とウェーブとウェーブの間の準備時間。0=20 秒。準備に入る時に、倒れている人だけ復活する（HP・Manaの全回復はしない。回復は泉で）"),
	FLD (SUBGAME_DUNGEONINFO, wDefWaveMaxSec,"1つのウェーブの最長時間(秒)",	FT_U16, 0, "拠点防衛の時。過ぎたら敵が残っていても次のウェーブへ（最後のウェーブは敵を倒し切るまで続く）。0=180"),
	FLD (SUBGAME_DUNGEONINFO, byDefSpringRange,"泉の範囲(使わない)",	FT_U8, 0, "今は使わない（2026-11-21 から泉は「泉の位置」の 1 マスだけ。泉の位置が 0,0 の時は拠点のまわり 1 マス）。泉にいる間、HP と Mana が 1 秒ごとにそれぞれ最大値の「泉の回復(%/秒)」の分（既定10%。最低 1）戻る（防衛の空間では Mana の自然回復は止まる）"),
	FLD (SUBGAME_DUNGEONINFO, byDefSpringX,	"泉の位置X",		FT_U8, 0, "拠点防衛の時。拠点から離れた場所に置くと、拠点のまわりで戦いながら Mana を回復し続けることができなくなる。0,0=拠点の位置"),
	FLD (SUBGAME_DUNGEONINFO, byDefSpringY,	"泉の位置Y",		FT_U8, 0, "拠点防衛の時。拠点から離れた場所に置くと、拠点のまわりで戦いながら Mana を回復し続けることができなくなる。0,0=拠点の位置"),
	FLD (SUBGAME_DUNGEONINFO, byDefSpringPct,"泉の回復(%/秒)",	FT_U8, 0, "拠点防衛の時。泉にいる間、約1秒ごとに HP と Mana がそれぞれ最大値のこの%（最低1）戻る。0=10"),
	FLD (SUBGAME_DUNGEONINFO, byDefArena,	"観戦あり",			FT_U8, 0, "拠点防衛の時だけ。1=観戦あり。組は闘技場の空間で遊び（ふつうの世界の人から中が見えて手は出せない）、一度に1組だけ。別の組が遊んでいる間の受付は断る。0=今まで通り"),
	FLD (SUBGAME_DUNGEONINFO, byViewX1,	"観客席X1",			FT_U8, 0, "観戦ありの時。ふつうの世界の同じマップで、この四角（X1,Y1〜X2,Y2）の中にいる人が観客。上の帯と、ウェーブ・残り敵などの組全体へのお知らせが届く。全部 0=観客席なし"),
	FLD (SUBGAME_DUNGEONINFO, byViewY1,	"観客席Y1",			FT_U8, 0, "観客席の範囲の開始Y（byViewX1 の説明を参照）"),
	FLD (SUBGAME_DUNGEONINFO, byViewX2,	"観客席X2",			FT_U8, 0, "観客席の範囲の終了X（byViewX1 の説明を参照）"),
	FLD (SUBGAME_DUNGEONINFO, byViewY2,	"観客席Y2",			FT_U8, 0, "観客席の範囲の終了Y（byViewX1 の説明を参照）"),
	FLD (SUBGAME_DUNGEONINFO, byFixStartX,	"中の開始位置X",	FT_U8, 0, "固定の形（byRebuildMode=2）の時。外の入口（byStartX・byStartY）に乗ると組の空間のこの位置へ移る。0,0=入口と同じマス（今まで通り）"),
	FLD (SUBGAME_DUNGEONINFO, byFixStartY,	"中の開始位置Y",	FT_U8, 0, "固定の形（byRebuildMode=2）の時。外の入口（byStartX・byStartY）に乗ると組の空間のこの位置へ移る。0,0=入口と同じマス（今まで通り）"),
	FLDS(SUBGAME_DUNGEONINFO, szMessages, "番外編の文言一覧", FT_STR, FF_MULTILINE, "1行に key=文。空なら既定文。差し込みは {name}・{time}・{rank} 等。キー一覧は docs/dungeon-gen.md"),
	FLD (SUBGAME_DUNGEONINFO, dwDailyVer,	"日替わりの版番号",		FT_U32, FF_READONLY, "今の形の版の通し番号（SboDungeonDaily.json の no。0=まだ作っていない。実行中だけ・読むだけ）"),
};
static const DATAFIELD s_aFieldSgWar[] = {
	FLD (SUBGAME_WARINFO, dwMapID,		"マップID",				FT_U32, 0, "map の dwID"),
	FLD (SUBGAME_WARINFO, byMember,		"1チームの人数",		FT_U8, 0, NULL),
	FLD (SUBGAME_WARINFO, byX,			"参加受付X",			FT_U8, 0, NULL),
	FLD (SUBGAME_WARINFO, byY,			"参加受付Y",			FT_U8, 0, NULL),
	FLD (SUBGAME_WARINFO, byX1,			"エリア開始X",			FT_U8, 0, NULL),
	FLD (SUBGAME_WARINFO, byY1,			"エリア開始Y",			FT_U8, 0, NULL),
	FLD (SUBGAME_WARINFO, byX2,			"エリア終了X",			FT_U8, 0, NULL),
	FLD (SUBGAME_WARINFO, byY2,			"エリア終了Y",			FT_U8, 0, NULL),
	FLD (SUBGAME_WARINFO, byHP1X,		"チーム1のHP X",		FT_U8, 0, NULL),
	FLD (SUBGAME_WARINFO, byHP2X,		"チーム2のHP X",		FT_U8, 0, NULL),
	FLD (SUBGAME_WARINFO, byHPY,		"チームのHP Y",			FT_U8, 0, NULL),
	FLD (SUBGAME_WARINFO, byJudgeX,		"審判X",				FT_U8, 0, NULL),
	FLD (SUBGAME_WARINFO, byJudgeY,		"審判Y",				FT_U8, 0, NULL),
	FLD (SUBGAME_WARINFO, byRes1X,		"チーム1の蘇生カウンタX",FT_U8, 0, NULL),
	FLD (SUBGAME_WARINFO, byRes2X,		"チーム2の蘇生カウンタX",FT_U8, 0, NULL),
	FLD (SUBGAME_WARINFO, byResY,		"蘇生カウンタY",		FT_U8, 0, NULL),
	FLD (SUBGAME_WARINFO, byStart1X,	"チーム1の開始X",		FT_U8, 0, NULL),
	FLD (SUBGAME_WARINFO, byStart2X,	"チーム2の開始X",		FT_U8, 0, NULL),
	FLD (SUBGAME_WARINFO, byStartY,		"開始Y",				FT_U8, 0, NULL),
	FLD (SUBGAME_WARINFO, byCastle1X,	"チーム1の城X",			FT_U8, 0, NULL),
	FLD (SUBGAME_WARINFO, byCastle2X,	"チーム2の城X",			FT_U8, 0, NULL),
	FLD (SUBGAME_WARINFO, byCastleY,	"城Y",					FT_U8, 0, NULL),
};
static const DATAFIELD s_aFieldSgSugoroku[] = {
	FLD (SUBGAME_SUGOROKUINFO, dwMapID,	"マップID",				FT_U32, 0, "map の dwID"),
	FLD (SUBGAME_SUGOROKUINFO, byX,		"参加受付X",			FT_U8, 0, NULL),
	FLD (SUBGAME_SUGOROKUINFO, byY,		"参加受付Y",			FT_U8, 0, NULL),
	FLD (SUBGAME_SUGOROKUINFO, byRule,	"ルール",				FT_U8, 0, NULL),
	FLD (SUBGAME_SUGOROKUINFO, byMaxCount,"最大参加人数",		FT_U8, 0, NULL),
};

static const char *s_apszSubGameName[] = { "なし", "爆弾ゲーム", "迷路", "ダンジョン", "戦場ゲーム", "すごろく" };

static const DATAFIELD *SubGameFields(int nType, int *pnCount)
{
	switch (nType) {
	case SUBGAMEID_BOMB:		*pnCount = COUNTOF(s_aFieldSgBomb);		return s_aFieldSgBomb;
	case SUBGAMEID_MAZE:		*pnCount = COUNTOF(s_aFieldSgMaze);		return s_aFieldSgMaze;
	case SUBGAMEID_DUNGEON:		*pnCount = COUNTOF(s_aFieldSgDungeon);	return s_aFieldSgDungeon;
	case SUBGAMEID_WAR:			*pnCount = COUNTOF(s_aFieldSgWar);		return s_aFieldSgWar;
	case SUBGAMEID_SUGOROKU:	*pnCount = COUNTOF(s_aFieldSgSugoroku);	return s_aFieldSgSugoroku;
	}
	*pnCount = 0;
	return NULL;
}

static std::string SubGameJson(PSUBGAMEINFO pInfo)
{
	std::string out = "\"byType\":" + JsonNum(pInfo->byType);
	out += std::string(",\"typeName\":\"") + ((pInfo->byType < SUBGAMEID_MAX) ? s_apszSubGameName[pInfo->byType] : "?") + "\"";
	int nCount = 0;
	const DATAFIELD *paField = SubGameFields(pInfo->byType, &nCount);
	out += ",\"detail\":{";
	if (paField && pInfo->pInfo) {
		out += FieldsJson((const BYTE *)pInfo->pInfo, paField, nCount);
		if (pInfo->byType == SUBGAMEID_SUGOROKU) {
			PSUBGAME_SUGOROKUINFO pSg = (PSUBGAME_SUGOROKUINFO)pInfo->pInfo;
			out += ",\"events\":[";
			for (int i = 0; i < pSg->aEventInfo.GetSize(); i ++) {
				SUGOROKUEVENTINFO &Ev = pSg->aEventInfo[i];
				if (i) out += ",";
				out += "{\"x\":" + JsonNum(Ev.byX) + ",\"y\":" + JsonNum(Ev.byY) + ",\"byEventType\":" + JsonNum(Ev.byEventType) + ",\"byPara1\":" + JsonNum(Ev.byPara1) + "}";
			}
			out += "],\"parts\":[";
			for (int i = 0; i < pSg->aPartsInfo.GetSize(); i ++) {
				SUGOROKUPARTSINFO &Pt = pSg->aPartsInfo[i];
				if (i) out += ",";
				out += "{\"byType\":" + JsonNum(Pt.byType) + ",\"wPartsID\":" + JsonNum(Pt.wPartsID) + "}";
			}
			out += "]";
		}
	}
	return out + "}";
}

static std::string EfcJson(PEFCINFO pEfc)
{
	std::string out = FieldsJson((const BYTE *)pEfc, s_aFieldEfc, COUNTOF(s_aFieldEfc)) +
			",\"animeCount\":" + JsonNum(pEfc->aAnime.GetCount()) + ",\"anime\":[";
	for (int i = 0; i < pEfc->aAnime.GetCount(); i ++) {
		PEFCANIMEINFO pAnime = pEfc->aAnime.GetPtr(i);
		if (i) out += ",";
		out += "{\"byWait\":" + JsonNum(pAnime->byWait) + ",\"byLevel\":" + JsonNum(pAnime->byLevel) + ",\"wGrpID\":" + JsonNum(pAnime->wGrpID) + "}";
	}
	return out + "]";
}

/* ------------------------------------------------------------------------- */
/* JS（HTTP の受付）から呼ばれる												 */
/* ------------------------------------------------------------------------- */
static std::string s_strResult;

/* 扱えるデータの種類と項目の一覧 */
extern "C" EMSCRIPTEN_KEEPALIVE const char *SboWebDataSchema(void)
{
	std::string out = "{\"kinds\":[";
	for (int i = 0; i < KIND_MAX; i ++) {
		if (i) {
			out += ",";
		}
		out += "{\"kind\":" + JsonStr(s_apszKindName[i]) + ",\"label\":" + JsonStr(s_apszKindLabel[i]) +
				",\"note\":" + JsonStr(s_apszKindNote[i]) + ",\"count\":" + JsonNum(KindCount(i)) +
				",\"maxCount\":" + JsonNum(s_anKindMax[i]) + ",\"fields\":";
		switch (i) {
		case KIND_MAGIC:
			out += FieldSchemaJson(s_aFieldMagic, COUNTOF(s_aFieldMagic));
			break;
		case KIND_MAGICUSER:
		case KIND_EQUUSER:
			out += FieldSchemaJson((i == KIND_MAGICUSER) ? s_aFieldMagicUser : s_aFieldEquUser, 1);
			out.insert(out.size() - 1, ",{\"name\":\"jobIds\",\"label\":\"職業IDの一覧\",\"type\":\"u8[]\",\"min\":0,\"max\":255,\"count\":255,\"note\":\"job の id（番号）を並べる（丸ごと置き換え）\"}");
			break;
		case KIND_JOB:
			out += FieldSchemaJson(s_aFieldJob, COUNTOF(s_aFieldJob));
			out.insert(out.size() - 1, ",{\"name\":\"szName\",\"label\":\"職業名\",\"type\":\"string\",\"maxBytes\":" + JsonNum(MAX_JOBNAME - 1) + "}"
					",{\"name\":\"levelUp\",\"label\":\"レベルアップ情報\",\"type\":\"object[]\",\"count\":" + JsonNum(MAX_LVUPCOUNT) +
					",\"note\":\"配列なら丸ごと置き換え（件数も変わる）。{\\\"4\\\": {\\\"dwExp\\\": 100}} のようなオブジェクトならその行だけ変える（今の件数と同じ番号は 1 行追加）。省いた項目は前の値のまま\",\"fields\":" +
					FieldSchemaJson(s_aFieldLvUp, COUNTOF(s_aFieldLvUp)) + "}");
			break;
		case KIND_STTMP:
			out += FieldSchemaJson(s_aFieldStTmp, COUNTOF(s_aFieldStTmp));
			break;
		case KIND_ITEM:
			out += FieldSchemaJson(s_aFieldItem, COUNTOF(s_aFieldItem));
			out.insert(out.size() - 1, ",{\"name\":\"szMsg\",\"label\":\"使った時のメッセージ\",\"type\":\"string\",\"maxBytes\":" + JsonNum(MAX_ITEMMSG - 1) + ",\"multiline\":true}"
					",{\"name\":\"detail\",\"label\":\"種別ごとの詳細\",\"type\":\"object\",\"note\":\"byType ごとの項目は detailByType を参照。省いた項目は前の値のまま\"}");
			out += ",\"detailByType\":{";
			for (int t = 0, nOut = 0; t < ITEMTYPE_MAX; t ++) {
				int nCount = 0;
				const DATAFIELD *paField = ItemDetailFields((BYTE)t, nCount);
				if (!paField) {
					continue;
				}
				if (nOut ++) {
					out += ",";
				}
				std::string strFields = FieldSchemaJson(paField, nCount);
				if (t == ITEMTYPE_MSG) {
					strFields.insert(strFields.size() - 1,
							",{\"name\":\"msgTo\",\"label\":\"他のキャラに使った時のメッセージ\",\"type\":\"string\",\"maxBytes\":" + JsonNum(MAX_ITEMMSG - 1) + ",\"multiline\":true}"
							",{\"name\":\"msgMe\",\"label\":\"他のキャラに使われた時のメッセージ\",\"type\":\"string\",\"maxBytes\":" + JsonNum(MAX_ITEMMSG - 1) + ",\"multiline\":true}");
				}
				out += "\"" + JsonNum(t) + "\":" + strFields;
			}
			out += "}";
			break;
		case KIND_MAKER:
			out += FieldSchemaJson(s_aFieldMaker, COUNTOF(s_aFieldMaker));
			break;
		case KIND_RECIPE:
			out += FieldSchemaJson(s_aFieldRecipe, COUNTOF(s_aFieldRecipe));
			break;
		case KIND_QUEST:
			out += FieldSchemaJson(s_aFieldQuest, COUNTOF(s_aFieldQuest));
			out.insert(out.size() - 1, ",{\"name\":\"goals\",\"type\":\"string[]\",\"label\":\"段ごとのいまの目的\",\"maxCount\":" + JsonNum(MAX_QUESTSTEP) + ",\"maxBytes\":" + JsonNum(MAX_QUESTGOAL - 1) + ",\"note\":\"0 段目から順に。最後の段の目的は「ギルドへ持ち帰る」などにする。配列で丸ごと置き換え\"}"
					",{\"name\":\"npcs\",\"type\":\"object[]\",\"label\":\"見えるNPC\",\"maxCount\":" + JsonNum(MAX_QUESTNPC) + ",\"fields\":" + FieldSchemaJson(s_aFieldQuestNpc, COUNTOF(s_aFieldQuestNpc)) + ",\"note\":\"配列で丸ごと置き換え。省いた項目は 0（byStepMax は省くと 255=どの段でも、byNextStep は省くと 255=変えない）\"}");
			break;
		case KIND_FLAG:
			out += FieldSchemaJson(s_aFieldFlagName, COUNTOF(s_aFieldFlagName));
			out.insert(out.size() - 1, ",{\"name\":\"group\",\"type\":\"string\",\"label\":\"グループ（メイン名）\",\"readonly\":true,\"note\":\"この番号が入る有効なグループ（flagGroup）のメイン名。どのグループにも入らなければ空。計算して出すだけで、書き込みはできない\"}"
					",{\"name\":\"sub\",\"type\":\"u16\",\"label\":\"グループの中の番号（サブ）\",\"readonly\":true,\"note\":\"帯の順に続けて数えた番号（1 始まり。ルシー=100〜139＋300〜319 の 300 はサブ 41）。「ルシー-3」の 3。グループに入らなければ 0。読むだけ\"}");
			break;
		case KIND_FLAGGROUP:
			out += FieldSchemaJson(s_aFieldFlagGroup, COUNTOF(s_aFieldFlagGroup));
			out.insert(out.size() - 1, ",{\"name\":\"bands\",\"type\":\"object[]\",\"label\":\"番号の帯\",\"minCount\":1,\"maxCount\":" + JsonNum(MAX_FLAGGROUPBAND) + ",\"fields\":"
					"[{\"name\":\"wFrom\",\"type\":\"u16\",\"label\":\"帯の先頭\",\"note\":\"その番号も含む。印は 0〜1023、数値は 0〜127\"},"
					"{\"name\":\"wTo\",\"type\":\"u16\",\"label\":\"帯の最後\",\"note\":\"その番号も含む。wFrom 以上\"}]"
					",\"note\":\"配列で丸ごと置き換え。1〜16 本で、順番がサブの数え順（帯 1 の続きが帯 2）。例: ルシー=[{wFrom:100,wTo:139},{wFrom:300,wTo:319}] なら 100〜139 がサブ 1〜40・300〜319 がサブ 41〜60。足す時は bands か wFrom・wTo のどちらかが必須。同じ種類のすべての有効なグループのすべての帯（同じグループの帯同士も）と重ねられない\"}"
					",{\"name\":\"wFrom\",\"type\":\"u16\",\"label\":\"帯 1 の先頭（互換）\",\"note\":\"bands[0].wFrom と同じ。bands を使わずこれと wTo だけを渡すと帯 1 本のグループになる（直す時は帯 1 だけが変わり、帯 2 以降は残る）\"}"
					",{\"name\":\"wTo\",\"type\":\"u16\",\"label\":\"帯 1 の最後（互換）\",\"note\":\"bands[0].wTo と同じ\"}");
			break;
		case KIND_AFFIX:
			out += FieldSchemaJson(s_aFieldAffix, COUNTOF(s_aFieldAffix));
			break;
		case KIND_ITEMDERIVE:
			out += FieldSchemaJson(s_aFieldItemDerive, COUNTOF(s_aFieldItemDerive));
			out.insert(out.size() - 1, ",{\"name\":\"itemId\",\"type\":\"u16\",\"label\":\"アイテム番号\",\"readonly\":true,\"note\":\"この派生アイテムの番号（16384 + id）。持ち物・預かり所などはこの番号で持つ\"}");
			break;
		case KIND_TALK:
			out += FieldSchemaJson(s_aFieldTalk, COUNTOF(s_aFieldTalk));
			out.insert(out.size() - 1, ",{\"name\":\"lines\",\"type\":\"object[]\",\"label\":\"行\",\"maxCount\":" + JsonNum(MAX_TALKLINE) + ",\"fields\":" + FieldSchemaJson(s_aFieldTalkLine, COUNTOF(s_aFieldTalkLine)) + ",\"note\":\"配列で丸ごと置き換え。省いた項目は、印・数値の番号が無し（65535・255）・それ以外は 0。台詞が空の行は「……」と言う。1 行ごとに順位・条件・台詞・話した後の動きを持つ\"}");
			break;
		case KIND_STORY:
			out += FieldSchemaJson(s_aFieldStory, COUNTOF(s_aFieldStory));
			out.insert(out.size() - 1, ",{\"name\":\"goals\",\"type\":\"object[]\",\"label\":\"いまの目的\",\"maxCount\":" + JsonNum(MAX_STORYGOAL) + ",\"fields\":" + FieldSchemaJson(s_aFieldStoryGoal, COUNTOF(s_aFieldStoryGoal)) + ",\"note\":\"配列で丸ごと置き換え（省くと今のまま。足す時に省くと無し）。クリアの印（wClearFlag）は見ず（クリアで消したい時は行の awNot にクリアの印を入れる）、上から見て条件（印・数値。会話データの行と同じ判定）に全部合う最初の行の szGoal を、コマンド窓の「目的」に「シナリオ名」と並べて出す。szGoal が空の行は飛ばし、合う行が無ければそのシナリオは出さない。省いた条件は無し（印 65535・数値 255）、短い配列は残りの行が無し\"}");
			break;
		case KIND_CHAR:
			out += FieldSchemaJson(s_aFieldChar, COUNTOF(s_aFieldChar));
			out.insert(out.size() - 1, ",{\"name\":\"npcId\",\"type\":\"u32\",\"label\":\"NPC番号\",\"readonly\":true,\"note\":\"保存される NPC ごとに決まった番号（再起動しても変わらない）。id（テーブル番号）は再起動で変わることがあるので、NPC を指す時はこちら。プレイヤー・発生した敵は 0\"}"
					",{\"name\":\"isPlayer\",\"type\":\"bool\",\"label\":\"プレイヤーか\",\"readonly\":true},{\"name\":\"online\",\"type\":\"bool\",\"label\":\"ログイン中か\",\"readonly\":true}"
					",{\"name\":\"szNpcSpeech\",\"type\":\"string\",\"label\":\"NPC の台詞\",\"note\":\"@5NN は店（NN はそのマップの店の番号）・@cNN は作り手（NN は maker の番号）\"}"
					",{\"name\":\"jobMagic\",\"type\":\"object\",\"label\":\"転職前の職業で覚えていた特技\",\"readonly\":true}"
					",{\"name\":\"townWarp\",\"type\":\"object\",\"label\":\"町転送の行き先帳\",\"readonly\":true,\"note\":\"教会で復活位置を記録した町（新しい順）。[マップID, 横, 縦]\"}"
					",{\"name\":\"flags\",\"type\":\"object\",\"label\":\"キャラのフラグ\",\"readonly\":true,\"note\":\"{bits: 立っている印の番号の並び, nums: {数値の番号: 値}（0 でないものだけ）}。プレイヤーだけ。書き込みはできない（管理者ウィンドウで変える）\"}"
					",{\"name\":\"quest\",\"type\":\"object\",\"label\":\"受けているギルドの依頼\",\"readonly\":true,\"note\":\"{questId: 依頼番号（0=受けていない）, step: 段, flags: 旗（ビット 0=旗1 …）, counts: 数 4 つ, questDone: クリア済みの依頼番号の並び, questRewarded: 報酬をもうもらっている依頼番号の並び（思い出の本でやり直しても消えない）}。プレイヤーだけ\"}"
					",{\"name\":\"route\",\"type\":\"object\",\"label\":\"巡回ルート\",\"note\":\"{mode: 0=折り返す 1=最初に戻る, wait: 止まる秒数 0〜60, points: [[x,y],...] 2〜16 個（巡回をやめる時は []）}。動かない・ランダム移動の NPC だけ\"}"
					",{\"name\":\"deleted\",\"type\":\"bool\",\"label\":\"消す\",\"note\":\"変更に {\\\"deleted\\\": true} を入れるとその NPC を消す（記録は残り、元に戻すと足し直す）\"}");
			break;
		case KIND_MAP:
			out += FieldSchemaJson(s_aFieldMap, COUNTOF(s_aFieldMap));
			out.insert(out.size() - 1, ",{\"name\":\"szName\",\"type\":\"string\",\"label\":\"マップ名\"}"
					",{\"name\":\"events\",\"type\":\"object[]\",\"label\":\"イベント\",\"note\":\"配列で丸ごと置き換え（{x, y, type, detail}。no は読む時だけ）。type: 0=座標変更 1=マップ間移動 2=メッセージ 3=扉 4=HP/Mana回復 5=ステータス初期化 6=乗っている間マップ変更。detail は eventDetailByType\"}"
					",{\"name\":\"shops\",\"type\":\"object[]\",\"label\":\"店\",\"note\":\"配列で丸ごと置き換え。{items: item の番号（10 個まで）, messages: 店の台詞 12 個（いらっしゃい・ありがとう・他に御用は・どれを買う・お金不足・持てない・買う確認(%s=品名 %d=値段)・買った・どれを売る・売る確認・売れない・売った）}。NPC の台詞 @5NN（NN=店の番号 2 桁）で開く\"}"
					",{\"name\":\"messages\",\"type\":\"string[]\",\"label\":\"メッセージ\",\"note\":\"配列で丸ごと置き換え。イベント（type 2）の detail.dwNo がこの番号\"}"
					",{\"name\":\"items\",\"type\":\"object[]\",\"label\":\"置いてあるアイテム\",\"readonly\":true,\"note\":\"落ちている物（遊んでいる間に変わる）\"}");
			out += ",\"eventDetailByType\":{";
			for (int t = 0, nOut = 0; t < EVENTTYPE_MAX; t ++) {
				int nCount = 0;
				const DATAFIELD *paField = EventFields((BYTE)t, nCount);
				if (!paField) {
					continue;
				}
				if (nOut ++) {
					out += ",";
				}
				std::string strSchema = FieldSchemaJson(paField, nCount);
				if (t == EVENTTYPE_DOOR) {
					strSchema.insert(strSchema.size() - 1, std::string(",{\"name\":\"extraCells\",\"type\":\"object[]\",\"label\":\"追加マス\",\"count\":") + JsonNum(MAX_DOOREXTRA) +
							",\"note\":\"扉の位置からずらしたマスのパーツも、扉の開閉に合わせて切り替える（見た目と通れる判定の両方）。配列で丸ごと置き換え（省くと今のまま、[] で全部消す）。"
							"{dx, dy: 扉からのずらし幅 -8〜8（両方 0 は不可）, closed: 閉まっている時の mapParts の番号, open: 開いている時の mapParts の番号}。"
							"最大 " + JsonNum(MAX_DOOREXTRA) + " 件。マップの外のマスは不可。0 件の時は get の detail にこの項目は出ない。マップ本来のそのマスの絵も closed と同じにしておくと、入りたての人の画面と合う\"}");
				}
				out += "\"" + JsonNum(t) + "\":" + strSchema;
			}
			out += "}";
			break;
		case KIND_MAPPARTS:
			out += FieldSchemaJson(s_aFieldMapParts, COUNTOF(s_aFieldMapParts));
			out.insert(out.size() - 1, ",{\"name\":\"anime\",\"type\":\"object[]\",\"label\":\"アニメのコマ（byWait=待ち時間×10ミリ秒・byLevel=透明度・wGrpID=画像ID・wBackID=背景ID）\"}");
			break;
		case KIND_EFC:
			out += FieldSchemaJson(s_aFieldEfc, COUNTOF(s_aFieldEfc));
			out.insert(out.size() - 1, ",{\"name\":\"animeCount\",\"type\":\"u32\",\"label\":\"アニメのコマ数\",\"readonly\":true}"
					",{\"name\":\"anime\",\"type\":\"object[]\",\"label\":\"アニメのコマ（byWait=待ち時間×10ミリ秒・byLevel=透明度・wGrpID=画像ID）\",\"note\":\"配列で丸ごと置き換え\"}");
			break;
		case KIND_COMEFC:
			out += FieldSchemaJson(s_aFieldComEfc, COUNTOF(s_aFieldComEfc));
			out.insert(out.size() - 1, ",{\"name\":\"anime\",\"type\":\"object[]\",\"label\":\"コマ（byWait=待ち時間×10ミリ秒・byGrpID=画像ID）\"}");
			break;
		case KIND_LEAVE:
			out += "[{\"name\":\"byUse\",\"type\":\"u8\",\"label\":\"使用中なら1\"},{\"name\":\"dwGold\",\"type\":\"u32\",\"label\":\"預けたお金\"},{\"name\":\"items\",\"type\":\"object[]\",\"label\":\"預けたアイテム（item=item の番号・count=個数）\"}]";
			break;
		case KIND_SUBGAME:
			out += "[{\"name\":\"byType\",\"type\":\"u8\",\"label\":\"種別\",\"note\":\"0=無し 1=爆弾ゲーム 2=迷路 3=ダンジョン 4=戦場ゲーム 5=すごろく。足せるのは迷路(2)だけ（byType と detail を指定）。変えられるのは 迷路⇔無し(0) だけで、0 にすると迷路が止まる\"},{\"name\":\"typeName\",\"type\":\"string\",\"label\":\"種別の名前\",\"readonly\":true},{\"name\":\"detail\",\"type\":\"object\",\"label\":\"種別ごとの設定（detailByType）。すごろくは events・parts も\"}]";
			out += ",\"detailByType\":{";
			for (int t = SUBGAMEID_BOMB; t < SUBGAMEID_MAX; t ++) {
				int nCount = 0;
				const DATAFIELD *paField = SubGameFields(t, &nCount);
				if (t > SUBGAMEID_BOMB) out += ",";
				out += "\"" + JsonNum(t) + "\":" + FieldSchemaJson(paField, nCount);
			}
			out += "}";
			break;
		}
		if (!KindWritable(i)) {
			out += ",\"readonly\":true";
		}
		out += "}";
	}
	out += "]}";
	s_strResult = out;
	return s_strResult.c_str();
}

/* 件数（知らない種類は -1） */
extern "C" EMSCRIPTEN_KEEPALIVE int SboWebDataCount(const char *pszKind)
{
	int nKind = KindFromName(pszKind);
	return (nKind < 0) ? -1 : KindCount(nKind);
}

/* 1 件を JSON で返す（{"id":番号, ...}。無ければ "null"） */
static bool CharIsEmpty(PCHARINFO p);
static std::string RecordJson(int nKind, int nID)
{
	if (nID < 0 || nID >= KindCount(nKind)) {
		return "null";
	}
	std::string out = "{\"id\":" + JsonNum(nID) + ",";
	switch (nKind) {
	case KIND_MAGIC:
		out += FieldsJson((const BYTE *)g_aMagicInfo[nID], s_aFieldMagic, COUNTOF(s_aFieldMagic));
		break;
	case KIND_MAGICUSER:
		{
			USERWORK w;
			UserToWork(g_aMagicUserInfo[nID], w);
			out += UserWorkJson(w, s_aFieldMagicUser);
		}
		break;
	case KIND_EQUUSER:
		{
			USERWORK w;
			UserToWork(g_aEquUserInfo[nID], w);
			out += UserWorkJson(w, s_aFieldEquUser);
		}
		break;
	case KIND_JOB:
		{
			JOBWORK w;
			JobToWork(g_JobInfo.GetPtr(nID), w);
			out += JobWorkJson(w);
		}
		break;
	case KIND_STTMP:
		out += FieldsJson((const BYTE *)g_aStTmpInfo[nID], s_aFieldStTmp, COUNTOF(s_aFieldStTmp));
		break;
	case KIND_ITEM:
		{
			ITEMWORK w;
			ItemToWork(g_aItemInfo[nID], w);
			out += ItemWorkJson(w);
		}
		break;
	case KIND_MAKER:
		out += FieldsJson((const BYTE *)g_aMakerInfo[nID], s_aFieldMaker, COUNTOF(s_aFieldMaker));
		break;
	case KIND_RECIPE:
		out += FieldsJson((const BYTE *)g_aRecipeInfo[nID], s_aFieldRecipe, COUNTOF(s_aFieldRecipe));
		break;
	case KIND_QUEST:
		out += QuestJson(g_aQuestInfo[nID]);
		break;
	case KIND_FLAG:
		out += FieldsJson((const BYTE *)g_aFlagNameInfo[nID], s_aFieldFlagName, COUNTOF(s_aFieldFlagName)) +
				FlagGroupExtraJson(g_aFlagNameInfo[nID]->byKind, g_aFlagNameInfo[nID]->wNo);
		break;
	case KIND_FLAGGROUP:
		out += FlagGroupJson(g_aFlagGroupInfo[nID]);
		break;
	case KIND_AFFIX:
		out += FieldsJson((const BYTE *)g_aAffixInfo[nID], s_aFieldAffix, COUNTOF(s_aFieldAffix));
		break;
	case KIND_ITEMDERIVE:
		out += FieldsJson((const BYTE *)g_aItemDerive[nID], s_aFieldItemDerive, COUNTOF(s_aFieldItemDerive)) +
				",\"itemId\":" + JsonNum(ITEMID_DERIVE_BASE + nID);
		break;
	case KIND_TALK:
		out += TalkJson(g_aTalkInfo[nID]);
		break;
	case KIND_STORY:
		out += StoryJson(g_aStoryInfo[nID]);
		break;
	case KIND_CHAR:
		if (CharIsEmpty(g_aCharInfo[nID])) {
			return "null";
		}
		out += CharJson(g_aCharInfo[nID]);
		break;
	case KIND_MAP:
		out += MapJson(g_aMapInfo[nID]);
		break;
	case KIND_MAPPARTS:
		out += MapPartsJson(&g_aMapPartsInfo[nID]);
		break;
	case KIND_EFC:
		{
			PEFCINFO pEfc = g_EfcInfo.GetPtr(nID);
			if (pEfc == NULL) {
				return "null";
			}
			out += EfcJson(pEfc);
		}
		break;
	case KIND_COMEFC:
		out += ComEfcJson(&g_pComEfcInfo[nID]);
		break;
	case KIND_LEAVE:
		if (g_aLeaveInfo[nID] == NULL) {
			return "null";
		}
		out += LeaveJson(g_aLeaveInfo[nID]);
		break;
	case KIND_SUBGAME:
		if (g_aSubaGameInfo[nID] == NULL) {
			return "null";
		}
		out += SubGameJson(g_aSubaGameInfo[nID]);
		break;
	}
	return out + "}";
}

/*
 * 宝箱（印で変わるマスの kind 1）の一覧。全マップ通し・読むだけ。
 *   chests … 宝箱番号の順。mapNo はマップの番号（map の id）、mapName はマップ名、item はアイテム名（無ければ ""）
 *   retired … 番号は使ったが今はどのマップにも無い（消した）宝箱の番号。使い回さない
 *   nextNo … 次に自動で振る番号（-1=使い切り）
 */
extern "C" EMSCRIPTEN_KEEPALIVE const char *SboWebDataChests()
{
	std::vector<std::pair<int, std::pair<int, int> > > aRow;		/* (宝箱番号, (マップ番号, 行の番号)) */
	std::vector<char> abyUsed(MAX_CHESTNO, 0);
	for (int i = 0; i < g_aMapInfo.GetSize(); i ++) {
		PMAPINFO pMap = g_aMapInfo[i];
		for (int j = 0; j < pMap->bySwapCount && j < MAX_MAPSWAP; j ++) {
			if (pMap->aSwap[j].byKind == MAPSWAPKIND_CHEST && pMap->aSwap[j].wChestNo < MAX_CHESTNO) {
				aRow.push_back(std::make_pair((int)pMap->aSwap[j].wChestNo, std::make_pair(i, j)));
				abyUsed[pMap->aSwap[j].wChestNo] = 1;
			}
		}
	}
	std::sort(aRow.begin(), aRow.end());
	std::string out = "{\"chests\":[";
	for (size_t k = 0; k < aRow.size(); k ++) {
		PMAPINFO pMap = g_aMapInfo[aRow[k].second.first];
		const MAPSWAP *pSw = &pMap->aSwap[aRow[k].second.second];
		if (k) out += ",";
		out += "{\"chestNo\":" + JsonNum(pSw->wChestNo) + ",\"mapNo\":" + JsonNum(aRow[k].second.first) +
				",\"mapName\":" + TextJson(pMap->pszName) + ",\"x\":" + JsonNum(pSw->wX) + ",\"y\":" + JsonNum(pSw->wY) +
				",\"wParts\":" + JsonNum(pSw->wParts) + ",\"wGiveItem\":" + JsonNum(pSw->wGiveItem) +
				",\"item\":" + JsonStr((pSw->wGiveItem != 0 && pSw->wGiveItem < g_aItemInfo.GetSize()) ? g_aItemInfo[pSw->wGiveItem]->szName : "") +
				",\"szMsg\":" + JsonStr(pSw->szMsg) + ",\"szMemo\":" + JsonStr(pSw->szMemo) + "}";
	}
	out += "],\"retired\":[";
	int nHigh = ChestHighAll();
	bool bFirst = true;
	for (int n = 0; n < nHigh && n < MAX_CHESTNO; n ++) {
		if (!abyUsed[n]) {
			if (!bFirst) out += ",";
			bFirst = false;
			out += JsonNum(n);
		}
	}
	int nNext = nHigh;
	out += "],\"nextNo\":" + JsonNum(nNext < MAX_CHESTNO ? nNext : -1) + ",\"max\":" + JsonNum(MAX_CHESTNO) + "}";
	s_strResult = out;
	return s_strResult.c_str();
}

/*
 * リサイクルボックス（マップに置く箱。印で変わるマスの kind 2。docs/recycle.md）の中身と記録。読むだけ。
 *   boxes … 箱番号（boxNo。全マップ通し。宝箱の番号とは別）の順。mapNo はマップの番号（map の id）、mapName はマップ名、
 *           x・y は箱のマス、wParts は開いた絵のパーツ、memo は管理用のメモ、kinds は入っている種類の数、
 *           total は個数の合計、stock は {itemId, item, count}（入れた順）。missing が true なのは、
 *           中身が残っているのにその番号の箱がどのマップにも無いもの（箱を消した）
 *   retired … 番号は使ったが今はどのマップにも無い（消した）箱の番号。使い回さない
 *   nextNo … 次に自動で振る番号（-1=使い切り）
 *   log … 記録（新しい順。最大 RECYCLE_LOGMAX 件）。time は UNIX 秒、date は日本時間の「年-月-日 時:分」、
 *         action は "put"（入れた）か "take"（もらった）、acc はアカウント、name はキャラ名、boxNo は箱番号
 *   today … 今日（日本時間）に 1 個以上もらったアカウントと、その個数・あと何個もらえるか（daily）
 *   limits … 決まり（種類・個数・1 日の数・記録の件数・箱番号の数）
 */
static std::string RecycleDateText(DWORD dwSec)
{
	time_t t = (time_t)dwSec + 9 * 60 * 60;		/* 日本時間 */
	struct tm tmJst;
	char sz[32];
	gmtime_r(&t, &tmJst);
	snprintf(sz, sizeof(sz), "%04d-%02d-%02d %02d:%02d", tmJst.tm_year + 1900, tmJst.tm_mon + 1, tmJst.tm_mday, tmJst.tm_hour, tmJst.tm_min);
	return sz;
}

static std::string RecycleItemName(WORD wItemID)
{
	return JsonStr((wItemID != 0 && wItemID < g_aItemInfo.GetSize()) ? g_aItemInfo[wItemID]->szName : "");
}

static std::string RecycleStockJson(DWORD dwBoxNo, int &nKinds, int &nTotal)
{
	std::string strStock;
	nKinds = 0;
	nTotal = 0;
	for (size_t k = 0; k < g_aRecycleStock.size(); k ++) {
		const RECYCLESTOCK &S = g_aRecycleStock[k];
		if (S.dwBoxNo != dwBoxNo) {
			continue;
		}
		if (nKinds) strStock += ",";
		strStock += "{\"itemId\":" + JsonNum(S.wItemID) + ",\"item\":" + RecycleItemName(S.wItemID) + ",\"count\":" + JsonNum(S.byCount) + "}";
		nKinds ++;
		nTotal += S.byCount;
	}
	return strStock;
}

extern "C" EMSCRIPTEN_KEEPALIVE const char *SboWebDataRecycle()
{
	std::string out = "{\"boxes\":[";
	std::vector<std::pair<int, std::pair<int, int> > > aRow;		/* (箱番号, (マップ番号, 行の番号)) */
	std::vector<char> abyUsed(MAX_RECYCLEBOX + 1, 0);
	for (int i = 0; i < g_aMapInfo.GetSize(); i ++) {
		PMAPINFO pMap = g_aMapInfo[i];
		for (int j = 0; j < pMap->bySwapCount && j < MAX_MAPSWAP; j ++) {
			if (pMap->aSwap[j].byKind == MAPSWAPKIND_RECYCLE && pMap->aSwap[j].wBoxNo >= 1 && pMap->aSwap[j].wBoxNo <= MAX_RECYCLEBOX) {
				aRow.push_back(std::make_pair((int)pMap->aSwap[j].wBoxNo, std::make_pair(i, j)));
				abyUsed[pMap->aSwap[j].wBoxNo] = 1;
			}
		}
	}
	std::sort(aRow.begin(), aRow.end());
	bool bFirst = true;
	for (size_t r = 0; r < aRow.size(); r ++) {
		PMAPINFO pMap = g_aMapInfo[aRow[r].second.first];
		const MAPSWAP *pSw = &pMap->aSwap[aRow[r].second.second];
		int nKinds = 0, nTotal = 0;
		std::string strStock = RecycleStockJson(pSw->wBoxNo, nKinds, nTotal);
		if (!bFirst) out += ",";
		bFirst = false;
		out += "{\"boxNo\":" + JsonNum(pSw->wBoxNo) + ",\"mapNo\":" + JsonNum(aRow[r].second.first) + ",\"mapName\":" + TextJson(pMap->pszName) +
				",\"x\":" + JsonNum(pSw->wX) + ",\"y\":" + JsonNum(pSw->wY) + ",\"wParts\":" + JsonNum(pSw->wParts) +
				",\"memo\":" + JsonStr(pSw->szMemo) +
				",\"kinds\":" + JsonNum(nKinds) + ",\"total\":" + JsonNum(nTotal) + ",\"missing\":false,\"stock\":[" + strStock + "]}";
	}
	/* 中身が残っているのに箱がどのマップにも無い番号 */
	std::vector<DWORD> aMissing;
	for (size_t k = 0; k < g_aRecycleStock.size(); k ++) {
		DWORD dwBoxNo = g_aRecycleStock[k].dwBoxNo;
		bool bHave = dwBoxNo <= (DWORD)MAX_RECYCLEBOX && abyUsed[dwBoxNo];
		if (!bHave && std::find(aMissing.begin(), aMissing.end(), dwBoxNo) == aMissing.end()) {
			aMissing.push_back(dwBoxNo);
		}
	}
	for (size_t m = 0; m < aMissing.size(); m ++) {
		int nKinds = 0, nTotal = 0;
		std::string strStock = RecycleStockJson(aMissing[m], nKinds, nTotal);
		if (!bFirst) out += ",";
		bFirst = false;
		out += "{\"boxNo\":" + JsonNum(aMissing[m]) + ",\"mapNo\":-1,\"mapName\":\"\",\"x\":0,\"y\":0,\"wParts\":0,\"memo\":\"\",\"kinds\":" +
				JsonNum(nKinds) + ",\"total\":" + JsonNum(nTotal) + ",\"missing\":true,\"stock\":[" + strStock + "]}";
	}
	out += "],\"retired\":[";
	int nHigh = RecycleBoxHighAll();
	bool bFirstRet = true;
	for (int n = 1; n <= nHigh && n <= MAX_RECYCLEBOX; n ++) {
		if (!abyUsed[n]) {
			if (!bFirstRet) out += ",";
			bFirstRet = false;
			out += JsonNum(n);
		}
	}
	out += "],\"nextNo\":" + JsonNum(nHigh < MAX_RECYCLEBOX ? nHigh + 1 : -1);
	out += ",\"max\":" + JsonNum(MAX_RECYCLEBOX);
	out += ",\"log\":[";
	for (size_t k = g_aRecycleLog.size(); k > 0; k --) {
		const RECYCLELOG &L = g_aRecycleLog[k - 1];
		if (k != g_aRecycleLog.size()) out += ",";
		out += "{\"time\":" + JsonNum(L.dwTime) + ",\"date\":" + JsonStr(RecycleDateText(L.dwTime).c_str()) +
				",\"acc\":" + JsonStr(L.szAcc) + ",\"name\":" + JsonStr(L.szName) + ",\"boxNo\":" + JsonNum(L.dwBoxNo) +
				",\"action\":" + JsonStr(L.byKind == RECYCLELOG_TAKE ? "take" : "put") +
				",\"itemId\":" + JsonNum(L.wItemID) + ",\"item\":" + RecycleItemName(L.wItemID) + ",\"count\":" + JsonNum(L.byCount) + "}";
	}
	out += "],\"daily\":[";
	DWORD dwToday = RecycleDay(RecycleNowSec());
	bool bFirstDaily = true;
	for (size_t k = 0; k < g_aRecycleDaily.size(); k ++) {
		const RECYCLEDAILY &D = g_aRecycleDaily[k];
		if (D.dwDay != dwToday || D.byCount == 0) {
			continue;
		}
		if (!bFirstDaily) out += ",";
		bFirstDaily = false;
		out += "{\"acc\":" + JsonStr(D.szAcc) + ",\"count\":" + JsonNum(D.byCount) + ",\"left\":" + JsonNum(RecycleLeft(D.szAcc)) + "}";
	}
	out += "],\"limits\":{\"maxKind\":" + JsonNum(RECYCLE_MAXKIND) + ",\"maxCount\":" + JsonNum(RECYCLE_MAXCOUNT) +
			",\"dayLimit\":" + JsonNum(RECYCLE_DAYLIMIT) + ",\"logMax\":" + JsonNum(RECYCLE_LOGMAX) + ",\"range\":" + JsonNum(RECYCLE_RANGE) + ",\"maxBox\":" + JsonNum(MAX_RECYCLEBOX) + "}}";
	s_strResult = out;
	return s_strResult.c_str();
}

/*
 * 取引所（委託販売。docs/market.md）の出品と売上の状況。読むだけ（期限切れへの切り替えもしない）。
 *   items … 出品（新しい順＝出品IDの大きい順）。id は出品ID、acc は出品者のアカウント、seller は出品したキャラ名、
 *           charId はそのキャラのテーブルID、itemId・item は品、price は値段、fee は売れた時の手数料、net は手取り、
 *           time・date は出品した時刻（UNIX 秒・日本時間の「年-月-日 時:分」）、limit・limitDate は期限、
 *           state は "on"（出品中）か "expired"（期限切れ。保存上はまだ出品中でも期限を過ぎていれば expired）、
 *           leftSec は期限までの残り秒（期限切れは 0）
 *   accounts … アカウントごとの出品数（出品中・期限切れ）と、受付に貯まっている売上 gold（出品か売上のあるアカウントだけ。アカウント名の順）
 *   total … 出品中・期限切れの件数、出品中の値段の合計、売上の合計
 *   limits … 決まり（1 アカウントの出品数・期限の日数・手数料%・値段の上限・受付からの距離・買う一覧の件数）
 */
extern "C" EMSCRIPTEN_KEEPALIVE const char *SboWebDataMarket()
{
	DWORD dwNow = (DWORD)time(NULL);
	std::vector<size_t> aIdx;
	for (size_t k = 0; k < g_aMarketItem.size(); k ++) {
		aIdx.push_back(k);
	}
	std::sort(aIdx.begin(), aIdx.end(), [](size_t a, size_t b) { return g_aMarketItem[a].dwID > g_aMarketItem[b].dwID; });

	/* アカウントごとの集計（アカウント名 → 出品中・期限切れ・売上） */
	struct ACCSUM { int nOn; int nExpired; DWORD dwGold; };
	std::map<std::string, ACCSUM> mapAcc;
	int nOn = 0, nExpired = 0;
	unsigned long long qwOnPrice = 0, qwGold = 0;

	std::string out = "{\"items\":[";
	for (size_t r = 0; r < aIdx.size(); r ++) {
		const MARKETITEM &M = g_aMarketItem[aIdx[r]];
		DWORD dwLimit = M.dwTime + MARKET_LIMITSEC;
		bool bExpired = (M.byState != MARKETSTATE_ON) || (dwNow >= M.dwTime && dwNow - M.dwTime >= MARKET_LIMITSEC);
		DWORD dwFee = (DWORD)((unsigned long long)M.dwPrice * MARKET_FEE_PERCENT / 100);
		std::string strAcc(M.szAcc, strnlen(M.szAcc, MAX_ACCNAME));
		ACCSUM &S = mapAcc[strAcc];
		if (bExpired) {
			S.nExpired ++;
			nExpired ++;
		} else {
			S.nOn ++;
			nOn ++;
			qwOnPrice += M.dwPrice;
		}
		if (r) out += ",";
		out += "{\"id\":" + JsonNum(M.dwID) + ",\"acc\":" + JsonStr(strAcc.c_str()) +
				",\"seller\":" + JsonStr(std::string(M.szSeller, strnlen(M.szSeller, MAX_CHARNAMEBUF)).c_str()) +
				",\"charId\":" + JsonNum(M.dwCharID) + ",\"itemId\":" + JsonNum(M.wItemID) + ",\"item\":" + RecycleItemName(M.wItemID) +
				",\"price\":" + JsonNum(M.dwPrice) + ",\"fee\":" + JsonNum(dwFee) + ",\"net\":" + JsonNum(M.dwPrice - dwFee) +
				",\"time\":" + JsonNum(M.dwTime) + ",\"date\":" + JsonStr(RecycleDateText(M.dwTime).c_str()) +
				",\"limit\":" + JsonNum(dwLimit) + ",\"limitDate\":" + JsonStr(RecycleDateText(dwLimit).c_str()) +
				",\"state\":" + JsonStr(bExpired ? "expired" : "on") +
				",\"leftSec\":" + JsonNum(bExpired ? 0 : (dwLimit > dwNow ? dwLimit - dwNow : 0)) + "}";
	}
	for (size_t k = 0; k < g_aMarketSale.size(); k ++) {
		const MARKETSALE &Sale = g_aMarketSale[k];
		if (Sale.szAcc[0] == 0 || Sale.dwGold == 0) {
			continue;
		}
		std::string strAcc(Sale.szAcc, strnlen(Sale.szAcc, MAX_ACCNAME));
		mapAcc[strAcc].dwGold += Sale.dwGold;
		qwGold += Sale.dwGold;
	}
	out += "],\"accounts\":[";
	bool bFirst = true;
	for (std::map<std::string, ACCSUM>::const_iterator it = mapAcc.begin(); it != mapAcc.end(); ++ it) {
		if (!bFirst) out += ",";
		bFirst = false;
		out += "{\"acc\":" + JsonStr(it->first.c_str()) + ",\"on\":" + JsonNum(it->second.nOn) + ",\"expired\":" + JsonNum(it->second.nExpired) +
				",\"gold\":" + JsonNum(it->second.dwGold) + "}";
	}
	out += "],\"total\":{\"on\":" + JsonNum(nOn) + ",\"expired\":" + JsonNum(nExpired) +
			",\"onPrice\":" + JsonNum((long long)qwOnPrice) + ",\"gold\":" + JsonNum((long long)qwGold) + "}";
	out += ",\"limits\":{\"maxPut\":" + JsonNum(MARKET_MAXPUT) + ",\"days\":" + JsonNum(MARKET_DAYS) + ",\"feePercent\":" + JsonNum(MARKET_FEE_PERCENT) +
			",\"priceMax\":" + JsonNum(MARKET_PRICEMAX) + ",\"range\":" + JsonNum(MARKET_RANGE) + ",\"listMax\":" + JsonNum(MARKET_LISTMAX) + "}}";
	s_strResult = out;
	return s_strResult.c_str();
}

extern "C" EMSCRIPTEN_KEEPALIVE const char *SboWebDataGet(const char *pszKind, int nID)
{
	int nKind = KindFromName(pszKind);
	s_strResult = (nKind < 0) ? "null" : RecordJson(nKind, nID);
	return s_strResult.c_str();
}

/* ========================================================================= */
/* NPC・マップの書き込み														 */
/* ========================================================================= */
extern BOOL SaveMapInfo(void);

/* 空き枠（消したキャラ）か */
static bool CharIsEmpty(PCHARINFO p)
{
	return (p == NULL) || ((p->dwMapID == 0) && (p->dwX == 0) && (p->dwY == 0) && (p->szAcc[0] == 0) && (p->szName[0] == 0) && (p->byNpcType == NPCTYPE_INIT));
}

/* 書き換えてよい NPC か（プレイヤー・発生元から出てきた敵などは不可） */
static bool CharCheckEditable(PCHARINFO p, std::string &strErr)
{
	if (CharIsEmpty(p)) {
		strErr = "この番号のキャラはいません（消したか、空き枠です）";
		return false;
	}
	if (p->byNpcType == NPCTYPE_INIT || p->szAcc[0]) {
		strErr = "プレイヤーのキャラは変えられません";
		return false;
	}
	if (p->dwParentTblID != (DWORD)-1) {
		strErr = "発生元から出てきたキャラ（敵など）は変えられません。発生元（byNpcType=5）の方を変えてください";
		return false;
	}
	return true;
}

/* 巡回ルート {"mode":回り方,"wait":秒,"points":[[x,y],...]} を読む */
static bool CharReadRoute(const JVal &v, int &nMode, int &nWait, std::vector<int> &aX, std::vector<int> &aY, std::string &strErr)
{
	long long n;
	if (v.t != JVal::OBJ) {
		strErr = "route は {\"mode\":0,\"wait\":0,\"points\":[[x,y],...]} の形で指定してください";
		return false;
	}
	const JVal *pMode = v.Get("mode"), *pWait = v.Get("wait"), *pPts = v.Get("points");
	nMode = 0;
	nWait = 0;
	if (pMode) { if (!GetInt(*pMode, 0, 255, n, strErr, "route.mode")) return false; nMode = (int)n; }
	if (pWait) { if (!GetInt(*pWait, 0, 255, n, strErr, "route.wait")) return false; nWait = (int)n; }
	if (pPts) {
		if (pPts->t != JVal::ARR || pPts->a.size() > MAX_NPCROUTE) {
			strErr = "route.points は [[x,y],...] の配列（" + JsonNum(MAX_NPCROUTE) + " 個まで）で指定してください";
			return false;
		}
		for (size_t i = 0; i < pPts->a.size(); i ++) {
			const JVal &pt = pPts->a[i];
			long long x, y;
			if (pt.t != JVal::ARR || pt.a.size() != 2 ||
					!GetInt(pt.a[0], 0, 255, x, strErr, "route.points[" + JsonNum((long long)i) + "][0]") ||
					!GetInt(pt.a[1], 0, 255, y, strErr, "route.points[" + JsonNum((long long)i) + "][1]")) {
				if (strErr.empty()) strErr = "route.points は [[x,y],...] の配列で指定してください";
				return false;
			}
			aX.push_back((int)x);
			aY.push_back((int)y);
		}
	}
	return true;
}

static bool CharRouteSame(PCHARINFO p, int nMode, int nWait, const std::vector<int> &aX, const std::vector<int> &aY)
{
	if (p->byRouteMode != nMode || p->byRouteWait != nWait || p->byRouteCount != aX.size()) {
		return false;
	}
	for (size_t i = 0; i < aX.size(); i ++) {
		if (p->abyRouteX[i] != aX[i] || p->abyRouteY[i] != aY[i]) {
			return false;
		}
	}
	return true;
}

/* キャラ（NPC）の変更・追加・削除。bDeleted は消した時 true（record は null） */
static bool CharApply(int nID, bool bAdd, const JVal &chg, bool bApply, std::string &strErr, std::string &strRecord, int &nIDOut, bool &bDeleted)
{
	static const char *apszExtra[] = { "szNpcSpeech", "route", "deleted", "isPlayer", "online", "jobMagic", "townWarp", "quest", "flags", "npcId", NULL };
	PCHARINFO pSrc = bAdd ? NULL : g_aCharInfo[nID];
	DWORD dwNpcIdAdd = 0;
	bDeleted = false;
	nIDOut = nID;

	/* npcId は足す時だけ指定できる（消した NPC を元に戻す時に同じ番号で足し直す） */
	const JVal *pNpcId = chg.Get("npcId");
	if (pNpcId) {
		long long n = 0;
		if (!bAdd) {
			if (!(pNpcId->t == JVal::NUM && pNpcId->n == pSrc->dwNpcID)) {
				strErr = "npcId は変えられません";
				return false;
			}
		} else {
			if (!GetInt(*pNpcId, 1, 0x7fffffff, n, strErr, "npcId")) {
				return false;
			}
			if (CharFindByNpcId((DWORD)n) >= 0) {
				strErr = "npcId " + JsonNum(n) + " はもう使われています";
				return false;
			}
			dwNpcIdAdd = (DWORD)n;
		}
	}

	if (!bAdd && !CharCheckEditable(pSrc, strErr)) {
		return false;
	}
	if (!CheckKnownKeys(chg, s_aFieldChar, COUNTOF(s_aFieldChar), apszExtra, strErr)) {
		return false;
	}
	/* 消す */
	const JVal *pDel = chg.Get("deleted");
	if (pDel) {
		if (pDel->t != JVal::BOOL_) {
			strErr = "deleted は true / false で指定してください";
			return false;
		}
		if (pDel->b) {
			if (bAdd) {
				strErr = "足す時に deleted は使えません";
				return false;
			}
			if (bApply) {
				DeleteChar((DWORD)nID);
				SaveCharInfo();
			}
			bDeleted = true;
			return true;
		}
	}

	/* 作業用のコピーで確かめる（ポインタは使わない） */
	CHARINFO w;
	if (bAdd) {
		ZeroMemory(&w, sizeof(w));
		w.byNpcType = NPCTYPE_NOMOVE;
		w.dwStTmplateID = (DWORD)-1;
		w.dwParentTblID = (DWORD)-1;
		w.byDirection = 1;
		w.wShowFlag = FLAG_NOBIT;
		w.wHideFlag = FLAG_NOBIT;
		static const char *apszNeed[] = { "dwMapID", "dwX", "dwY", NULL };
		for (int k = 0; apszNeed[k]; k ++) {
			if (!chg.Get(apszNeed[k])) {
				strErr = std::string("NPC を足す時は ") + apszNeed[k] + " が要ります（dwMapID・dwX・dwY）";
				return false;
			}
		}
	} else {
		CopyMemory(&w, pSrc, sizeof(w));
		w.wShowFlag = CharFlagFromWire(pSrc->wShowFlag);	/* 作業用は API の形（番号。65535=無し） */
		w.wHideFlag = CharFlagFromWire(pSrc->wHideFlag);
	}
	if (!ApplyFields((BYTE *)&w, s_aFieldChar, COUNTOF(s_aFieldChar), chg, strErr, "")) {
		return false;
	}
	if (w.byNpcType == NPCTYPE_INIT || w.byNpcType >= NPCTYPE_MAX) {
		strErr = "byNpcType は 1〜" + JsonNum(NPCTYPE_MAX - 1) + " で指定してください（0 はプレイヤー用）";
		return false;
	}
	int nMapNo = GetMapNo(w.dwMapID);
	if (nMapNo < 0 || w.dwMapID >= (DWORD)g_aMapInfo.GetSize()) {
		strErr = "dwMapID のマップがありません";
		return false;
	}
	if (!bAdd && w.dwMapID != pSrc->dwMapID) {
		strErr = "別のマップへは移せません（消して、移したいマップに足し直してください）";
		return false;
	}
	PMAPINFO pMap = g_aMapInfo[nMapNo];
	if (w.dwX >= (DWORD)pMap->nCX || w.dwY >= (DWORD)pMap->nCY) {
		strErr = "dwX・dwY がマップの外です（" + JsonNum(pMap->nCX) + "×" + JsonNum(pMap->nCY) + "）";
		return false;
	}
	if ((w.dwStartX || w.dwStartY) && (w.dwStartX >= (DWORD)pMap->nCX || w.dwStartY >= (DWORD)pMap->nCY)) {
		strErr = "dwStartX・dwStartY がマップの外です";
		return false;
	}
	if (w.dwStTmplateID != (DWORD)-1 && w.dwStTmplateID >= (DWORD)g_aStTmpInfo.GetSize()) {
		strErr = "dwStTmplateID は stTmp の番号（0〜" + JsonNum(g_aStTmpInfo.GetSize() - 1) + "）か 4294967295（無し）で指定してください";
		return false;
	}
	if (w.byDirection > 7) {
		strErr = "byDirection は 0〜7 で指定してください";
		return false;
	}
	if ((w.wShowFlag >= MAX_FLAGBIT && w.wShowFlag != FLAG_NOBIT) || (w.wHideFlag >= MAX_FLAGBIT && w.wHideFlag != FLAG_NOBIT)) {
		strErr = "wShowFlag・wHideFlag は印の番号（0〜" + JsonNum(MAX_FLAGBIT - 1) + "）か 65535（条件なし）で指定してください";
		return false;
	}
	/* 台詞 */
	std::string strSpeech = (pSrc && pSrc->pszNpcSpeech) ? pSrc->pszNpcSpeech : "";
	const JVal *pSpeech = chg.Get("szNpcSpeech");
	if (pSpeech) {
		if (pSpeech->t != JVal::STR) {
			strErr = "szNpcSpeech は文字列で指定してください";
			return false;
		}
		if (!CheckText(pSpeech->s, 4096, false, strErr, "szNpcSpeech")) {
			return false;
		}
		strSpeech = pSpeech->s;
	}
	/* 巡回ルート（今と同じなら何もしない） */
	bool bRoute = false;
	int nMode = 0, nWait = 0;
	std::vector<int> aX, aY;
	const JVal *pRoute = chg.Get("route");
	if (pRoute) {
		if (!CharReadRoute(*pRoute, nMode, nWait, aX, aY, strErr)) {
			return false;
		}
		bool bSame = bAdd ? (aX.empty() && nMode == 0) : CharRouteSame(pSrc, nMode, nWait, aX, aY);
		if (!bSame) {
			const int anDummy[1] = { 0 };
			LPCSTR pszErr = NpcRouteCheck(&w, nMode, nWait, (int)aX.size(), aX.empty() ? anDummy : &aX[0], aY.empty() ? anDummy : &aY[0]);
			if (pszErr) {
				strErr = std::string("route: ") + pszErr;
				return false;
			}
			bRoute = true;
		}
	}

	if (!bApply) {
		/* 変更後の姿（ポインタを差し替えた一時コピーで作る） */
		w.pszNpcSpeech = (LPSTR)strSpeech.c_str();
		if (bRoute) {
			w.byRouteMode = (BYTE)nMode;
			w.byRouteWait = (BYTE)nWait;
			w.byRouteCount = (BYTE)aX.size();
			for (size_t i = 0; i < aX.size(); i ++) {
				w.abyRouteX[i] = (BYTE)aX[i];
				w.abyRouteY[i] = (BYTE)aY[i];
			}
		}
		if (bAdd) {
			w.dwNpcID = dwNpcIdAdd;
		}
		w.wShowFlag = CharFlagToWire(w.wShowFlag);	/* CharJson は中の形（番号+1）を受け取る */
		w.wHideFlag = CharFlagToWire(w.wHideFlag);
		strRecord = CharJson(&w);
		return true;
	}

	PCHARINFO pDst;
	bool bViewChg = false;
	if (bAdd) {
		/* 管理者ウィンドウの「キャラ追加」と同じ作り方 */
		PCHARINFO pNew = new CHARINFO;
		ZeroMemory(pNew, sizeof(CHARINFO));
		pNew->byNpcType = w.byNpcType;
		SetInitCharData(pNew);
		pNew->dwMapID = w.dwMapID;
		pNew->dwX = w.dwX;
		pNew->dwY = w.dwY;
		pNew->dwXBack = w.dwX;
		pNew->dwYBack = w.dwY;
		pNew->dwStartMapID = w.dwMapID;
		pNew->dwStartX = (w.dwStartX || w.dwStartY) ? w.dwStartX : w.dwX;
		pNew->dwStartY = (w.dwStartX || w.dwStartY) ? w.dwStartY : w.dwY;
		pNew->byDirection = w.byDirection;
		pNew->byGameState = GAMESTATE_NPC;
		CopyMemory(pNew->szName, w.szName, sizeof(pNew->szName));
		pDst = AddChar(pNew);
		pDst->pID = pDst->dwTblID + 1;
		pDst->dwNpcID = dwNpcIdAdd;
		CharNpcIdEnsure(pDst);
		nIDOut = (int)pDst->dwTblID;
	} else {
		pDst = pSrc;
		pDst->dwX = w.dwX;
		pDst->dwY = w.dwY;
		pDst->dwXBack = w.dwX;
		pDst->dwYBack = w.dwY;
		pDst->dwStartX = w.dwStartX;
		pDst->dwStartY = w.dwStartY;
		pDst->byDirection = w.byDirection;
		CopyMemory(pDst->szName, w.szName, sizeof(pDst->szName));
	}
	/* 管理者ウィンドウの「キャラ情報」と同じ項目を移す */
	if (pDst->byNpcType != w.byNpcType) {
		CCharProcBase CharProcBase;
		SAFE_DELETE(pDst->pProc);
		pDst->byNpcType = w.byNpcType;
		CharProcBase.SetProc(pDst);
	}
	pDst->dwStTmplateID		= w.dwStTmplateID;
	pDst->byPutCount		= w.byPutCount;
	pDst->byPutOnce			= w.byPutOnce;
	pDst->byPutWave			= w.byPutWave;
	pDst->wTimePutInterval	= w.wTimePutInterval;
	pDst->byAreaX			= w.byAreaX;
	pDst->byAreaY			= w.byAreaY;
	pDst->byMoveProbabilityX = w.byMoveProbabilityX;
	pDst->byMoveProbabilityY = w.byMoveProbabilityY;
	pDst->bySearchCx		= w.bySearchCx;
	pDst->bySearchCy		= w.bySearchCy;
	pDst->byDropProbability	= w.byDropProbability;
	pDst->byAtackRate		= w.byAtackRate;
	pDst->byGhost			= w.byGhost;
	WORD wShowIn = CharFlagToWire(w.wShowFlag);	/* 中の形（番号+1。0=無し）にして入れる */
	WORD wHideIn = CharFlagToWire(w.wHideFlag);
	bViewChg				= (pDst->wShowFlag != wShowIn) || (pDst->wHideFlag != wHideIn);
	pDst->wShowFlag			= wShowIn;
	pDst->wHideFlag			= wHideIn;
	pDst->wGrpID			= w.wGrpID;
	pDst->wGrpMain			= w.wGrpMain;
	pDst->wGrpSub			= w.wGrpSub;
	pDst->clName			= w.clName;
	pDst->clSpeech			= w.clSpeech;
	pDst->byGameState		= GAMESTATE_NPC;
	SAFE_DELETE_ARRAY(pDst->pszNpcSpeech);
	if (!strSpeech.empty()) {
		pDst->pszNpcSpeech = new char[strSpeech.size() + 1];
		CopyMemory(pDst->pszNpcSpeech, strSpeech.c_str(), strSpeech.size() + 1);
	}
	if (pDst->dwStTmplateID != (DWORD)-1) {
		ApplyStTmpToChars(pDst->dwStTmplateID);
	}
	RenewATDF(pDst->dwTblID);
	/* NPC発生（発生元）なら、もう出ているキャラにも反映 */
	RenewPutChildChar(pDst);
	if (bRoute) {
		std::vector<BYTE> bx(aX.begin(), aX.end()), by(aY.begin(), aY.end());
		const BYTE byDummy[1] = { 0 };
		NpcRouteApply(pDst, (BYTE)nMode, (BYTE)nWait, (int)aX.size(), bx.empty() ? byDummy : &bx[0], by.empty() ? byDummy : &by[0]);
	}
	SendCmdCHARINFO3(pDst->dwTblID);
	if (!bAdd) {
		SendCmdCHARINFO(pDst->pID, -1);
		SendCmdCHARPOS(pDst->pID, pDst->dwTblID, 0, 1);
	}
	/* 見える・見えない条件を変えたら、そのマップの全員に見え直しを送る */
	if (bViewChg) {
		CharViewRefreshNpc(pDst);
	}
	SaveCharInfo();
	strRecord = CharJson(pDst);
	return true;
}

/* --- マップ --------------------------------------------------------------- */
typedef struct _MAPEVWORK {
	DWORD				dwX, dwY;
	BYTE				byType;
	std::vector<BYTE>	aData;			/* GetEventDataSize の大きさ（0 なら詳細なし） */
} MAPEVWORK;

typedef struct _MAPSHOPWORK {
	std::vector<WORD>	aItem;
	std::string			astrMsg[MAX_SHOPMSG];
} MAPSHOPWORK;

/* 今のマップの中身（イベント・店・メッセージ）を作業用にする */
static void MapToWork(PMAPINFO pMap, std::vector<MAPEVWORK> &aEv, std::vector<MAPSHOPWORK> &aShop, std::vector<std::string> &aMsg)
{
	int i, j;
	for (i = 0; i < pMap->nEventCount && pMap->pEvent; i ++) {
		PEVENTINFO pEv = &pMap->pEvent[i];
		MAPEVWORK e;
		e.dwX = pEv->dwX;
		e.dwY = pEv->dwY;
		e.byType = pEv->byType;
		int nSize = GetEventDataSize(pEv->byType);
		if (pEv->pData && nSize > 0) {
			e.aData.assign((const BYTE *)pEv->pData, (const BYTE *)pEv->pData + nSize);
		}
		aEv.push_back(e);
	}
	for (i = 0; i < pMap->byCountShop && pMap->pShopInfo; i ++) {
		PSHOPINFO pShop = &pMap->pShopInfo[i];
		MAPSHOPWORK s;
		for (j = 0; j < pShop->byCountItem && j < MAX_SHOPITEM; j ++) {
			s.aItem.push_back(pShop->awItem[j]);
		}
		for (j = 0; j < MAX_SHOPMSG; j ++) {
			s.astrMsg[j] = pShop->apMsg[j] ? pShop->apMsg[j] : "";
		}
		aShop.push_back(s);
	}
	for (i = 0; i < pMap->nMsgCount && pMap->pMsg; i ++) {
		aMsg.push_back(pMap->pMsg[i] ? pMap->pMsg[i] : "");
	}
}

/* 作業用の中身から MapJson と同じ形を作る（下見用） */
static std::string MapWorkJson(PMAPINFO pBase, const std::string &strName, const std::vector<MAPEVWORK> &aEv,
		const std::vector<MAPSHOPWORK> &aShop, const std::vector<std::string> &aMsg)
{
	MAPINFO m = *pBase;
	std::vector<EVENTINFO> aEvInfo(aEv.size());
	std::vector<SHOPINFO> aShopInfo(aShop.size());
	std::vector<LPSTR> apMsg(aMsg.size());
	for (size_t i = 0; i < aEv.size(); i ++) {
		aEvInfo[i].dwX = aEv[i].dwX;
		aEvInfo[i].dwY = aEv[i].dwY;
		aEvInfo[i].byType = aEv[i].byType;
		aEvInfo[i].pData = aEv[i].aData.empty() ? NULL : (PVOID)&aEv[i].aData[0];
	}
	for (size_t i = 0; i < aShop.size(); i ++) {
		ZeroMemory(&aShopInfo[i], sizeof(SHOPINFO));
		aShopInfo[i].byCountItem = (BYTE)aShop[i].aItem.size();
		for (size_t j = 0; j < aShop[i].aItem.size(); j ++) aShopInfo[i].awItem[j] = aShop[i].aItem[j];
		for (int j = 0; j < MAX_SHOPMSG; j ++) aShopInfo[i].apMsg[j] = (LPSTR)aShop[i].astrMsg[j].c_str();
	}
	for (size_t i = 0; i < aMsg.size(); i ++) apMsg[i] = (LPSTR)aMsg[i].c_str();
	m.pszName = (char *)strName.c_str();
	m.nEventCount = (int)aEv.size();
	m.pEvent = aEvInfo.empty() ? NULL : &aEvInfo[0];
	m.byCountShop = (BYTE)aShop.size();
	m.pShopInfo = aShopInfo.empty() ? NULL : &aShopInfo[0];
	m.nMsgCount = (int)aMsg.size();
	m.pMsg = apMsg.empty() ? NULL : &apMsg[0];
	return MapJson(&m);
}

/* マップにいるプレイヤーへの送り直し（ResendMapToPlayers）は SboSvrQuest.cpp。ダンジョンの作り直しと共用 */

/* 扉の追加マス（extraCells）を扉に読む。丸ごと置き換え。範囲・マップの外・重なりを確かめる */
static bool ParseDoorExtraCells(const JVal &v, DWORD dwX, DWORD dwY, int nCX, int nCY, PEVENTINFO_DOOR pDoor, std::string &strErr, const std::string &strName)
{
	static const char *apszKey[] = { "dx", "dy", "closed", "open", NULL };
	DOOREXTRA aTmp[MAX_DOOREXTRA];
	long long n;

	if (v.t != JVal::ARR) {
		strErr = strName + " は配列で指定してください";
		return false;
	}
	if (v.a.size() > MAX_DOOREXTRA) {
		strErr = strName + " は " + JsonNum(MAX_DOOREXTRA) + " 件までです";
		return false;
	}
	ZeroMemory(aTmp, sizeof(aTmp));
	for (size_t i = 0; i < v.a.size(); i ++) {
		const JVal &e = v.a[i];
		std::string strP = strName + "[" + JsonNum((long long)i) + "]";
		if (e.t != JVal::OBJ) {
			strErr = strP + " はオブジェクトで指定してください";
			return false;
		}
		for (size_t k = 0; k < e.o.size(); k ++) {
			bool bOk = false;
			for (int m = 0; apszKey[m]; m ++) if (e.o[k].first == apszKey[m]) bOk = true;
			if (!bOk) {
				strErr = strP + ": 知らない項目です: " + e.o[k].first;
				return false;
			}
		}
		const JVal *pdx = e.Get("dx"), *pdy = e.Get("dy"), *pc = e.Get("closed"), *po = e.Get("open");
		if (!pdx || !pdy || !pc || !po) {
			strErr = strP + " には dx・dy・closed・open が要ります";
			return false;
		}
		if (g_wCountMapParts <= 0) {
			strErr = "パーツがありません";
			return false;
		}
		if (!GetInt(*pdx, -DOOREXTRA_OFFMAX, DOOREXTRA_OFFMAX, n, strErr, strP + ".dx")) return false;
		aTmp[i].sbyOffX = (signed char)n;
		if (!GetInt(*pdy, -DOOREXTRA_OFFMAX, DOOREXTRA_OFFMAX, n, strErr, strP + ".dy")) return false;
		aTmp[i].sbyOffY = (signed char)n;
		if (!GetInt(*pc, 0, g_wCountMapParts - 1, n, strErr, strP + ".closed")) return false;
		aTmp[i].wClosedParts = (WORD)n;
		if (!GetInt(*po, 0, g_wCountMapParts - 1, n, strErr, strP + ".open")) return false;
		aTmp[i].wOpenParts = (WORD)n;
		if (aTmp[i].sbyOffX == 0 && aTmp[i].sbyOffY == 0) {
			strErr = strP + ": dx・dy が両方 0 です（扉のマス自身は追加マスにできません）";
			return false;
		}
		long long nCellX = (long long)dwX + aTmp[i].sbyOffX, nCellY = (long long)dwY + aTmp[i].sbyOffY;
		if (nCellX < 0 || nCellX >= nCX || nCellY < 0 || nCellY >= nCY) {
			strErr = strP + ": 扉の位置 (" + JsonNum((long long)dwX) + "," + JsonNum((long long)dwY) + ") からずらしたマス (" + JsonNum(nCellX) + "," + JsonNum(nCellY) + ") がマップの外です";
			return false;
		}
		for (size_t j = 0; j < i; j ++) {
			if (aTmp[j].sbyOffX == aTmp[i].sbyOffX && aTmp[j].sbyOffY == aTmp[i].sbyOffY) {
				strErr = strP + ": dx・dy が " + JsonNum((long long)j) + " 件目と同じマスです";
				return false;
			}
		}
	}
	pDoor->byExtraCount = (BYTE)v.a.size();
	CopyMemory(pDoor->aExtra, aTmp, sizeof(pDoor->aExtra));
	return true;
}

/* swaps（印で変わるマス）を w に読む。JSON を MAPSWAPEDITROW に直すだけで、範囲・種類違いの欄・重なり・番号の自動振りは
   ゲーム内の管理者ウィンドウと同じ MapSwapBuild（SboSvrMapSwapEdit.cpp）が行う */
static bool ParseMapSwaps(const JVal &sw, int nID, MAPINFO &w, std::string &strErr)
{
	static const char *apszKey[] = { "kind", "x", "y", "wParts", "wFlag", "byCmp", "wChestNo", "wBoxNo", "wGiveItem", "szMsg", "szMemo", "wNeedFlag", "szLockMsg", NULL };
	long long n;
	const long long nAny = 1LL << 40;		/* 型だけ確かめる（範囲は MapSwapBuild） */

	if (sw.t != JVal::ARR) {
		strErr = "swaps は配列で指定してください";
		return false;
	}
	if (sw.a.size() > MAX_MAPSWAP) {
		strErr = "swaps は " + JsonNum(MAX_MAPSWAP) + " 個までです";
		return false;
	}
	std::vector<MAPSWAPEDITROW> aRow(sw.a.size());
	for (size_t i = 0; i < sw.a.size(); i ++) {
		const JVal &e = sw.a[i];
		std::string strP = "swaps[" + JsonNum((long long)i) + "]";
		MAPSWAPEDITROW &r = aRow[i];
		if (e.t != JVal::OBJ) {
			strErr = strP + " はオブジェクトで指定してください";
			return false;
		}
		for (size_t k = 0; k < e.o.size(); k ++) {
			bool bOk = false;
			for (int m = 0; apszKey[m]; m ++) if (e.o[k].first == apszKey[m]) bOk = true;
			if (!bOk) {
				strErr = strP + ": 知らない項目です: " + e.o[k].first;
				return false;
			}
		}
		const JVal *pk = e.Get("kind"), *px = e.Get("x"), *py = e.Get("y"), *pp = e.Get("wParts");
		r.nKind = MAPSWAPKIND_FLAG;
		if (pk) {
			if (pk->t == JVal::STR && (pk->s == "flag" || pk->s == "chest" || pk->s == "recycle")) {
				r.nKind = (pk->s == "chest") ? MAPSWAPKIND_CHEST : (pk->s == "recycle") ? MAPSWAPKIND_RECYCLE : MAPSWAPKIND_FLAG;
			} else {
				if (!GetInt(*pk, -nAny, nAny, n, strErr, strP + ".kind")) return false;
				r.nKind = n;
			}
		}
		if (!px || !py || !pp) {
			strErr = strP + " には x・y・wParts が要ります";
			return false;
		}
		if (!GetInt(*px, -nAny, nAny, r.nX, strErr, strP + ".x")) return false;
		if (!GetInt(*py, -nAny, nAny, r.nY, strErr, strP + ".y")) return false;
		if (!GetInt(*pp, -nAny, nAny, r.nParts, strErr, strP + ".wParts")) return false;
		struct { const char *pszKey; DWORD dwBit; long long *pn; } aInt[] = {
			{ "wFlag", MAPSWAPEDIT_FLAG, &r.nFlag }, { "byCmp", MAPSWAPEDIT_CMP, &r.nCmp },
			{ "wChestNo", MAPSWAPEDIT_CHESTNO, &r.nChestNo }, { "wBoxNo", MAPSWAPEDIT_BOXNO, &r.nBoxNo },
			{ "wGiveItem", MAPSWAPEDIT_GIVEITEM, &r.nGiveItem },
			{ "wNeedFlag", MAPSWAPEDIT_NEEDFLAG, &r.nNeedFlag } };
		for (size_t m = 0; m < COUNTOF(aInt); m ++) {
			const JVal *pv = e.Get(aInt[m].pszKey);
			if (!pv) continue;
			if (!GetInt(*pv, -nAny, nAny, *aInt[m].pn, strErr, strP + "." + aInt[m].pszKey)) return false;
			r.dwHas |= aInt[m].dwBit;
		}
		struct { const char *pszKey; DWORD dwBit; std::string *ps; } aStr[] = {
			{ "szMsg", MAPSWAPEDIT_MSG, &r.strMsg }, { "szMemo", MAPSWAPEDIT_MEMO, &r.strMemo },
			{ "szLockMsg", MAPSWAPEDIT_LOCKMSG, &r.strLockMsg } };
		for (size_t m = 0; m < COUNTOF(aStr); m ++) {
			const JVal *pv = e.Get(aStr[m].pszKey);
			if (!pv) continue;
			if (pv->t != JVal::STR) {
				strErr = strP + "." + aStr[m].pszKey + " は文字列で指定してください";
				return false;
			}
			*aStr[m].ps = pv->s;
			r.dwHas |= aStr[m].dwBit;
		}
	}
	return MapSwapBuild(nID, w, aRow, "swaps", strErr) ? true : false;
}

/* マップの変更（設定・イベント・店・メッセージ。配列は丸ごと置き換え） */
static bool MapApply(int nID, const JVal &chg, bool bApply, std::string &strErr, std::string &strRecord)
{
	static const char *apszExtra[] = { "szName", "darkAreas", "lights", "partsLights", "swaps", "events", "shops", "messages", "items", NULL };
	PMAPINFO pMap = g_aMapInfo[nID];
	long long n;

	if (!CheckKnownKeys(chg, s_aFieldMap, COUNTOF(s_aFieldMap), apszExtra, strErr)) {
		return false;
	}
	MAPINFO w = *pMap;
	if (!ApplyFields((BYTE *)&w, s_aFieldMap, COUNTOF(s_aFieldMap), chg, strErr, "")) {
		return false;
	}
	if (w.nStartX < 0 || w.nStartX >= w.nCX || w.nStartY < 0 || w.nStartY >= w.nCY ||
			w.nEntranceX < 0 || w.nEntranceX >= w.nCX || w.nEntranceY < 0 || w.nEntranceY >= w.nCY) {
		strErr = "初期位置・入り口がマップの外です（" + JsonNum(w.nCX) + "×" + JsonNum(w.nCY) + "）";
		return false;
	}
	if ((int)w.wViewHideParts >= (int)g_wCountMapParts) {
		strErr = "wViewHideParts のパーツ " + JsonNum(w.wViewHideParts) + " がありません（mapParts は " + JsonNum(g_wCountMapParts) + " 個）";
		return false;
	}
	if (w.byViewRange > 10) {
		strErr = "byViewRange は 0〜10 で指定してください";
		return false;
	}
	if (!IsValidBgmNo(w.wBGM)) {
		strErr = "byBGM は 0〜" + JsonNum(BGM_MIDI_MAX - 1) + " または音声BGMの番号（" + JsonNum(BGM_AUDIO_FIRST) + "〜）で指定してください";
		return false;
	}
	std::string strName = pMap->pszName ? pMap->pszName : "";
	const JVal *pName = chg.Get("szName");
	if (pName) {
		if (pName->t != JVal::STR || !CheckText(pName->s, 256, false, strErr, "szName")) {
			if (strErr.empty()) strErr = "szName は文字列で指定してください";
			return false;
		}
		strName = pName->s;
	}

	/* 暗くする範囲（丸ごと置き換え） */
	const JVal *pAreas = chg.Get("darkAreas");
	if (pAreas) {
		static const char *apszAreaKey[] = { "x1", "y1", "x2", "y2", "dark", NULL };
		if (pAreas->t != JVal::ARR) {
			strErr = "darkAreas は配列で指定してください";
			return false;
		}
		if (pAreas->a.size() > MAX_MAPDARKAREA) {
			strErr = "darkAreas は " + JsonNum(MAX_MAPDARKAREA) + " 個までです";
			return false;
		}
		ZeroMemory(w.aDarkArea, sizeof(w.aDarkArea));
		w.byDarkAreaCount = (BYTE)pAreas->a.size();
		for (size_t i = 0; i < pAreas->a.size(); i ++) {
			const JVal &e = pAreas->a[i];
			std::string strP = "darkAreas[" + JsonNum((long long)i) + "]";
			if (e.t != JVal::OBJ) {
				strErr = strP + " はオブジェクトで指定してください";
				return false;
			}
			for (size_t k = 0; k < e.o.size(); k ++) {
				bool bOk = false;
				for (int m = 0; apszAreaKey[m]; m ++) if (e.o[k].first == apszAreaKey[m]) bOk = true;
				if (!bOk) {
					strErr = strP + ": 知らない項目です: " + e.o[k].first;
					return false;
				}
			}
			const JVal *px1 = e.Get("x1"), *py1 = e.Get("y1"), *px2 = e.Get("x2"), *py2 = e.Get("y2"), *pd = e.Get("dark");
			if (!px1 || !py1 || !px2 || !py2) {
				strErr = strP + " には x1・y1・x2・y2 が要ります";
				return false;
			}
			PMAPDARKAREA pArea = &w.aDarkArea[i];
			long long x1, y1, x2, y2;
			if (!GetInt(*px1, 0, w.nCX - 1, x1, strErr, strP + ".x1")) return false;
			if (!GetInt(*py1, 0, w.nCY - 1, y1, strErr, strP + ".y1")) return false;
			if (!GetInt(*px2, 0, w.nCX - 1, x2, strErr, strP + ".x2")) return false;
			if (!GetInt(*py2, 0, w.nCY - 1, y2, strErr, strP + ".y2")) return false;
			pArea->wX1 = (WORD)min(x1, x2);
			pArea->wY1 = (WORD)min(y1, y2);
			pArea->wX2 = (WORD)max(x1, x2);
			pArea->wY2 = (WORD)max(y1, y2);
			pArea->byDark = 1;
			if (pd) {
				if (!GetInt(*pd, 0, 2, n, strErr, strP + ".dark")) return false;
				pArea->byDark = (BYTE)n;
			}
		}
	}

	/* 光源（丸ごと置き換え） */
	const JVal *pLights = chg.Get("lights");
	if (pLights) {
		static const char *apszLightKey[] = { "x", "y", "range", "power", "flicker", "warm", NULL };
		if (pLights->t != JVal::ARR) {
			strErr = "lights は配列で指定してください";
			return false;
		}
		if (pLights->a.size() > MAX_MAPLIGHT) {
			strErr = "lights は " + JsonNum(MAX_MAPLIGHT) + " 個までです";
			return false;
		}
		ZeroMemory(w.aLight, sizeof(w.aLight));
		w.byLightCount = (BYTE)pLights->a.size();
		for (size_t i = 0; i < pLights->a.size(); i ++) {
			const JVal &e = pLights->a[i];
			std::string strP = "lights[" + JsonNum((long long)i) + "]";
			if (e.t != JVal::OBJ) {
				strErr = strP + " はオブジェクトで指定してください";
				return false;
			}
			for (size_t k = 0; k < e.o.size(); k ++) {
				bool bOk = false;
				for (int m = 0; apszLightKey[m]; m ++) if (e.o[k].first == apszLightKey[m]) bOk = true;
				if (!bOk) {
					strErr = strP + ": 知らない項目です: " + e.o[k].first;
					return false;
				}
			}
			const JVal *px = e.Get("x"), *py = e.Get("y"), *pr = e.Get("range");
			if (!px || !py || !pr) {
				strErr = strP + " には x・y・range が要ります";
				return false;
			}
			PMAPLIGHT pLight = &w.aLight[i];
			if (!GetInt(*px, 0, w.nCX - 1, n, strErr, strP + ".x")) return false;
			pLight->wX = (WORD)n;
			if (!GetInt(*py, 0, w.nCY - 1, n, strErr, strP + ".y")) return false;
			pLight->wY = (WORD)n;
			if (!GetInt(*pr, 1, 7, n, strErr, strP + ".range")) return false;
			pLight->byRange = (BYTE)n;
			const JVal *pp = e.Get("power"), *pf = e.Get("flicker"), *pw = e.Get("warm");
			pLight->byPower = 100;
			if (pp) {
				if (!GetInt(*pp, 1, 100, n, strErr, strP + ".power")) return false;
				pLight->byPower = (BYTE)n;
			}
			if (pf) {
				if (!GetInt(*pf, 0, 1, n, strErr, strP + ".flicker")) return false;
				if (n == 0) pLight->byFlg |= MAPLIGHTFLG_NOFLICKER;
			}
			if (pw) {
				if (!GetInt(*pw, 0, 1, n, strErr, strP + ".warm")) return false;
				if (n) pLight->byFlg |= MAPLIGHTFLG_WARM;
			}
		}
	}

	/* パーツの灯りの上書き（丸ごと置き換え） */
	const JVal *pPartsLights = chg.Get("partsLights");
	if (pPartsLights) {
		static const char *apszPLKey[] = { "wParts", "byLight", NULL };
		if (pPartsLights->t != JVal::ARR) {
			strErr = "partsLights は配列で指定してください";
			return false;
		}
		if (pPartsLights->a.size() > MAX_MAPPARTSLIGHT) {
			strErr = "partsLights は " + JsonNum(MAX_MAPPARTSLIGHT) + " 件までです";
			return false;
		}
		ZeroMemory(w.aPartsLight, sizeof(w.aPartsLight));
		w.byPartsLightCount = (BYTE)pPartsLights->a.size();
		for (size_t i = 0; i < pPartsLights->a.size(); i ++) {
			const JVal &e = pPartsLights->a[i];
			std::string strP = "partsLights[" + JsonNum((long long)i) + "]";
			if (e.t != JVal::OBJ) {
				strErr = strP + " はオブジェクトで指定してください";
				return false;
			}
			for (size_t k = 0; k < e.o.size(); k ++) {
				bool bOk = false;
				for (int m = 0; apszPLKey[m]; m ++) if (e.o[k].first == apszPLKey[m]) bOk = true;
				if (!bOk) {
					strErr = strP + ": 知らない項目です: " + e.o[k].first;
					return false;
				}
			}
			const JVal *pwp = e.Get("wParts"), *pbl = e.Get("byLight");
			if (!pwp || !pbl) {
				strErr = strP + " には wParts・byLight が要ります";
				return false;
			}
			if (g_wCountMapParts <= 0) {
				strErr = "パーツがありません";
				return false;
			}
			if (!GetInt(*pwp, 0, g_wCountMapParts - 1, n, strErr, strP + ".wParts")) return false;
			w.aPartsLight[i].wParts = (WORD)n;
			if (!GetInt(*pbl, 0, 7, n, strErr, strP + ".byLight")) return false;
			w.aPartsLight[i].byLight = (BYTE)n;
			for (size_t j = 0; j < i; j ++) {
				if (w.aPartsLight[j].wParts == w.aPartsLight[i].wParts) {
					strErr = strP + ".wParts: パーツ " + JsonNum(w.aPartsLight[i].wParts) + " が重複しています";
					return false;
				}
			}
		}
	}

	/* 印で変わるマス（丸ごと置き換え） */
	const JVal *pSwaps = chg.Get("swaps");
	if (pSwaps && !ParseMapSwaps(*pSwaps, nID, w, strErr)) {
		return false;
	}

	std::vector<MAPEVWORK> aEvOld, aEv;
	std::vector<MAPSHOPWORK> aShopOld, aShop;
	std::vector<std::string> aMsgOld, aMsg;
	MapToWork(pMap, aEvOld, aShopOld, aMsgOld);
	aEv = aEvOld;
	aShop = aShopOld;
	aMsg = aMsgOld;

	/* イベント */
	const JVal *pEvs = chg.Get("events");
	if (pEvs) {
		static const char *apszEvKey[] = { "no", "x", "y", "type", "detail", NULL };
		if (pEvs->t != JVal::ARR) {
			strErr = "events は配列で指定してください";
			return false;
		}
		aEv.clear();
		for (size_t i = 0; i < pEvs->a.size(); i ++) {
			const JVal &e = pEvs->a[i];
			std::string strP = "events[" + JsonNum((long long)i) + "]";
			if (e.t != JVal::OBJ) {
				strErr = strP + " はオブジェクトで指定してください";
				return false;
			}
			for (size_t k = 0; k < e.o.size(); k ++) {
				bool bOk = false;
				for (int m = 0; apszEvKey[m]; m ++) if (e.o[k].first == apszEvKey[m]) bOk = true;
				if (!bOk) {
					strErr = strP + ": 知らない項目です: " + e.o[k].first;
					return false;
				}
			}
			const JVal *px = e.Get("x"), *py = e.Get("y"), *pt = e.Get("type");
			if (!px || !py || !pt) {
				strErr = strP + " には x・y・type が要ります";
				return false;
			}
			MAPEVWORK ev;
			if (!GetInt(*px, 0, w.nCX - 1, n, strErr, strP + ".x")) return false;
			ev.dwX = (DWORD)n;
			if (!GetInt(*py, 0, w.nCY - 1, n, strErr, strP + ".y")) return false;
			ev.dwY = (DWORD)n;
			if (!GetInt(*pt, 0, EVENTTYPE_MAX - 1, n, strErr, strP + ".type")) return false;
			ev.byType = (BYTE)n;
			int nSize = GetEventDataSize(ev.byType);
			if (nSize > 0) {
				/* 同じ番号・同じ種別の前の詳細を土台にする（項目表に無い中身を消さない） */
				if (i < aEvOld.size() && aEvOld[i].byType == ev.byType && (int)aEvOld[i].aData.size() == nSize) {
					ev.aData = aEvOld[i].aData;
				} else {
					ev.aData.assign(nSize, 0);
					if (ev.byType == EVENTTYPE_DOOR) {
						((PEVENTINFO_DOOR)&ev.aData[0])->wEnemyStTmp = 65535;	/* 敵の種類は指定なしが既定 */
					}
				}
				int nCount = 0;
				const DATAFIELD *paField = EventFields(ev.byType, nCount);
				const JVal *pd = e.Get("detail");
				if (pd) {
					if (pd->t != JVal::OBJ) {
						strErr = strP + ".detail はオブジェクトで指定してください";
						return false;
					}
					static const char *apszDoorExtra[] = { "extraCells", NULL };
					if (!CheckKnownKeys(*pd, paField, nCount, (ev.byType == EVENTTYPE_DOOR) ? apszDoorExtra : NULL, strErr) ||
							!ApplyFields(&ev.aData[0], paField, nCount, *pd, strErr, strP + ".detail.")) {
						return false;
					}
					if ((ev.byType == EVENTTYPE_DOOR) && pd->Get("extraCells") &&
							!ParseDoorExtraCells(*pd->Get("extraCells"), ev.dwX, ev.dwY, w.nCX, w.nCY, (PEVENTINFO_DOOR)&ev.aData[0], strErr, strP + ".detail.extraCells")) {
						return false;
					}
					if (ev.byType == EVENTTYPE_DOOR) {
						WORD wSt = ((PEVENTINFO_DOOR)&ev.aData[0])->wEnemyStTmp;
						if ((wSt != 65535) && ((int)wSt >= g_aStTmpInfo.GetSize())) {
							strErr = strP + ".detail.wEnemyStTmp は stTmp の番号（0〜" + JsonNum(g_aStTmpInfo.GetSize() - 1) + "）か 65535（指定なし）で指定してください";
							return false;
						}
						/* 鍵なしの文は、種別が「鍵」の扉だけ（それ以外は空にしておく） */
						PEVENTINFO_DOOR pDoorChk = (PEVENTINFO_DOOR)&ev.aData[0];
						if (pDoorChk->szLockMsg[0] && pDoorChk->byType != EVENTDOOTTYPE_KEY) {
							if (pd->Get("szLockMsg")) {
								strErr = strP + ".detail.szLockMsg は種別が 2（鍵）の扉だけで指定できます";
								return false;
							}
							ZeroMemory(pDoorChk->szLockMsg, sizeof(pDoorChk->szLockMsg));	/* 鍵でなくなる扉は、前の文を持ち越さない */
						}
					}
				}
			} else if (ev.dwX == 0 && ev.dwY == 0) {
				strErr = strP + ": 詳細の無いイベントは (0,0) に置けません";
				return false;
			}
			aEv.push_back(ev);
		}
	}

	/* 店 */
	const JVal *pShops = chg.Get("shops");
	if (pShops) {
		if (pShops->t != JVal::ARR || pShops->a.size() > 100) {
			strErr = "shops は配列（100 軒まで）で指定してください";
			return false;
		}
		aShop.clear();
		for (size_t i = 0; i < pShops->a.size(); i ++) {
			const JVal &s = pShops->a[i];
			std::string strP = "shops[" + JsonNum((long long)i) + "]";
			if (s.t != JVal::OBJ) {
				strErr = strP + " はオブジェクトで指定してください";
				return false;
			}
			for (size_t k = 0; k < s.o.size(); k ++) {
				const std::string &key = s.o[k].first;
				if (key != "no" && key != "items" && key != "messages") {
					strErr = strP + ": 知らない項目です: " + key;
					return false;
				}
			}
			MAPSHOPWORK sw;
			if (i < aShopOld.size()) {
				sw = aShopOld[i];
			}
			const JVal *pi = s.Get("items");
			if (pi) {
				if (pi->t != JVal::ARR || pi->a.size() > MAX_SHOPITEM) {
					strErr = strP + ".items は item の番号の配列（" + JsonNum(MAX_SHOPITEM) + " 個まで）で指定してください";
					return false;
				}
				sw.aItem.clear();
				for (size_t j = 0; j < pi->a.size(); j ++) {
					if (!GetInt(pi->a[j], 1, g_aItemInfo.GetSize() - 1, n, strErr, strP + ".items[" + JsonNum((long long)j) + "]")) return false;
					sw.aItem.push_back((WORD)n);
				}
			}
			const JVal *pm = s.Get("messages");
			if (pm) {
				if (pm->t != JVal::ARR || pm->a.size() > MAX_SHOPMSG) {
					strErr = strP + ".messages は文字列の配列（" + JsonNum(MAX_SHOPMSG) + " 個まで）で指定してください";
					return false;
				}
				for (size_t j = 0; j < pm->a.size(); j ++) {
					std::string strN = strP + ".messages[" + JsonNum((long long)j) + "]";
					if (pm->a[j].t != JVal::STR) {
						strErr = strN + " は文字列で指定してください";
						return false;
					}
					if (!CheckText(pm->a[j].s, 1024, false, strErr, strN)) return false;
					sw.astrMsg[j] = pm->a[j].s;
				}
			} else if (i >= aShopOld.size()) {
				strErr = strP + ": 新しい店には messages（" + JsonNum(MAX_SHOPMSG) + " 個）が要ります";
				return false;
			}
			aShop.push_back(sw);
		}
	}

	/* メッセージ */
	const JVal *pMsgs = chg.Get("messages");
	if (pMsgs) {
		if (pMsgs->t != JVal::ARR) {
			strErr = "messages は文字列の配列で指定してください";
			return false;
		}
		aMsg.clear();
		for (size_t i = 0; i < pMsgs->a.size(); i ++) {
			std::string strN = "messages[" + JsonNum((long long)i) + "]";
			if (pMsgs->a[i].t != JVal::STR) {
				strErr = strN + " は文字列で指定してください";
				return false;
			}
			if (!CheckText(pMsgs->a[i].s, 4096, false, strErr, strN)) return false;
			aMsg.push_back(pMsgs->a[i].s);
		}
	}

	if (!bApply) {
		strRecord = MapWorkJson(&w, strName, aEv, aShop, aMsg);
		return true;
	}

	/* 反映 */
	WORD wBGMOld = pMap->wBGM;
	std::vector<int> aOldExtraXY;		/* 変える前の扉の追加マスの座標（x,y の並び） */
	/* マップ全体の情報（マップ情報の送り直しが要るもの）が変わるか。
	 * 店・イベント・メッセージだけなら、マップに入り直させずに差分だけ送る */
	bool bMapWide =
		pMap->nStartX != w.nStartX || pMap->nStartY != w.nStartY ||
		pMap->nEntranceX != w.nEntranceX || pMap->nEntranceY != w.nEntranceY ||
		pMap->wBGM != w.wBGM || pMap->byFlg != w.byFlg ||
		pMap->byLightLevel != w.byLightLevel || pMap->byWeather != w.byWeather ||
		pMap->byViewRange != w.byViewRange || pMap->wViewHideParts != w.wViewHideParts ||
		pMap->byDarkAreaCount != w.byDarkAreaCount ||
		memcmp(pMap->aDarkArea, w.aDarkArea, sizeof(pMap->aDarkArea)) != 0 ||
		pMap->byLightCount != w.byLightCount ||
		memcmp(pMap->aLight, w.aLight, sizeof(pMap->aLight)) != 0 ||
		pMap->byPartsLightCount != w.byPartsLightCount ||
		memcmp(pMap->aPartsLight, w.aPartsLight, sizeof(pMap->aPartsLight)) != 0 ||
		strName != std::string(pMap->pszName ? (const char *)pMap->pszName : "");
	/* 店が減る時はクライアントの店の数を減らす手段が無いので、マップ情報ごと送り直す */
	if (pShops && aShop.size() < (size_t)pMap->byCountShop) {
		bMapWide = true;
	}
	pMap->nStartX		= w.nStartX;
	pMap->nStartY		= w.nStartY;
	pMap->nEntranceX	= w.nEntranceX;
	pMap->nEntranceY	= w.nEntranceY;
	pMap->wBGM			= w.wBGM;
	pMap->byFlg			= w.byFlg;
	pMap->byLightLevel	= w.byLightLevel;
	pMap->byWeather		= w.byWeather;
	pMap->byViewRange	= w.byViewRange;
	pMap->wViewHideParts	= w.wViewHideParts;
	pMap->byDarkAreaCount	= w.byDarkAreaCount;
	CopyMemory(pMap->aDarkArea, w.aDarkArea, sizeof(pMap->aDarkArea));
	pMap->byLightCount		= w.byLightCount;
	CopyMemory(pMap->aLight, w.aLight, sizeof(pMap->aLight));
	pMap->byPartsLightCount	= w.byPartsLightCount;
	CopyMemory(pMap->aPartsLight, w.aPartsLight, sizeof(pMap->aPartsLight));
	/* 印で変わるマスが変わったら、マップ全体の送り直しは要らず、いるプレイヤーに差し替え一覧だけ送る */
	bool bSwapChg = MapSwapChanged(*pMap, w) ? true : false;
	MapSwapSet(pMap, w);
	SAFE_DELETE_ARRAY(pMap->pszName);
	if (!strName.empty()) {
		pMap->pszName = new char[strName.size() + 1];
		CopyMemory(pMap->pszName, strName.c_str(), strName.size() + 1);
	}
	if (pEvs) {
		/* 変える前の扉の追加マスの座標（変えた後、本人の画面を戻すため） */
		for (int i = 0; i < pMap->nEventCount && pMap->pEvent; i ++) {
			PEVENTINFO pOld = &pMap->pEvent[i];
			if (pOld->byType == EVENTTYPE_DOOR && pOld->pData) {
				PEVENTINFO_DOOR pOldDoor = (PEVENTINFO_DOOR)pOld->pData;
				for (int j = 0; j < pOldDoor->byExtraCount && j < MAX_DOOREXTRA; j ++) {
					aOldExtraXY.push_back((int)pOld->dwX + pOldDoor->aExtra[j].sbyOffX);
					aOldExtraXY.push_back((int)pOld->dwY + pOldDoor->aExtra[j].sbyOffY);
				}
			}
		}
		for (int i = 0; i < pMap->nEventCount && pMap->pEvent; i ++) {
			if (pMap->pEvent[i].pData) {
				delete [] (PBYTE)pMap->pEvent[i].pData;
			}
		}
		SAFE_DELETE_ARRAY(pMap->pEvent);
		pMap->nEventCount = (int)aEv.size();
		if (!aEv.empty()) {
			pMap->pEvent = new EVENTINFO[aEv.size()];
			for (size_t i = 0; i < aEv.size(); i ++) {
				PEVENTINFO pEv = &pMap->pEvent[i];
				pEv->dwX = aEv[i].dwX;
				pEv->dwY = aEv[i].dwY;
				pEv->byType = aEv[i].byType;
				pEv->pData = NULL;
				if (!aEv[i].aData.empty()) {
					PBYTE p = new BYTE[aEv[i].aData.size()];
					CopyMemory(p, &aEv[i].aData[0], aEv[i].aData.size());
					pEv->pData = p;
				}
			}
		}
		pMap->nProcEvent = 0;
		g_nProcMap = 0;
	}
	if (pShops) {
		for (int i = 0; i < pMap->byCountShop && pMap->pShopInfo; i ++) {
			for (int j = 0; j < MAX_SHOPMSG; j ++) {
				SAFE_DELETE_ARRAY(pMap->pShopInfo[i].apMsg[j]);
			}
		}
		SAFE_DELETE_ARRAY(pMap->pShopInfo);
		pMap->byCountShop = (BYTE)aShop.size();
		if (!aShop.empty()) {
			pMap->pShopInfo = new SHOPINFO[aShop.size()];
			ZeroMemory(pMap->pShopInfo, sizeof(SHOPINFO) * aShop.size());
			for (size_t i = 0; i < aShop.size(); i ++) {
				PSHOPINFO pShop = &pMap->pShopInfo[i];
				pShop->byCountItem = (BYTE)aShop[i].aItem.size();
				for (size_t j = 0; j < aShop[i].aItem.size(); j ++) {
					pShop->awItem[j] = aShop[i].aItem[j];
				}
				for (int j = 0; j < MAX_SHOPMSG; j ++) {
					const std::string &s = aShop[i].astrMsg[j];
					pShop->apMsg[j] = new char[s.size() + 1];
					CopyMemory(pShop->apMsg[j], s.c_str(), s.size() + 1);
				}
			}
		}
	}
	if (pMsgs) {
		for (int i = 0; i < pMap->nMsgCount && pMap->pMsg; i ++) {
			SAFE_DELETE_ARRAY(pMap->pMsg[i]);
		}
		SAFE_DELETE_ARRAY(pMap->pMsg);
		pMap->nMsgCount = (int)aMsg.size();
		if (!aMsg.empty()) {
			pMap->pMsg = new LPSTR[aMsg.size()];
			for (size_t i = 0; i < aMsg.size(); i ++) {
				pMap->pMsg[i] = new char[aMsg[i].size() + 1];
				CopyMemory(pMap->pMsg[i], aMsg[i].c_str(), aMsg[i].size() + 1);
			}
		}
	}
	/* そのマップにいるプレイヤーへ、マップに入った時と同じ情報を送り直す */
	if (wBGMOld != pMap->wBGM) {
		SendCmdCHGBGM(URARASOCK_SENDALL, pMap->dwID, pMap->wBGM);
	}
	if (bMapWide) {
		/* クライアントはマップ情報を受け取るとキャラを全部消すので、
		 * マップに入った時と同じくキャラも送り直す（送らないと自分のキャラが消えて操作できなくなる） */
		ResendMapToPlayers(pMap, nID);
	} else {
		if (bSwapChg) {
			MapSwapSendMap(pMap);
		}
		if (pShops) {
			for (int i = 0; i < pMap->byCountShop; i ++) {
				SendCmdSHOPMap((DWORD)nID, (BYTE)i);
			}
		}
		if (pEvs) {
			for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
				PCHARINFO pChar = g_aCharInfo[i];
				if (pChar && pChar->pID && pChar->szAcc[0] && pChar->dwMapID == pMap->dwID && pChar->byNpcType == NPCTYPE_INIT) {
					SendCmdEVENTCOUNT(pChar->pID, (DWORD)nID);
					for (int j = 0; j < pMap->nEventCount; j ++) {
						SendCmdEVENTINFO(pChar->pID, nID, j);
					}
					SendCmdEVENTPOS(pChar->pID, (DWORD)nID);
				}
			}
		}
	}
	/* 扉の追加マスが変わった（前か後に追加マスがある）なら、いる人の画面の追加マスを今の状態に直す */
	if (pEvs) {
		bool bNewExtra = false;
		for (size_t i = 0; i < aEv.size(); i ++) {
			if (aEv[i].byType == EVENTTYPE_DOOR && !aEv[i].aData.empty() && ((PEVENTINFO_DOOR)&aEv[i].aData[0])->byExtraCount) {
				bNewExtra = true;
			}
		}
		if (bNewExtra || !aOldExtraXY.empty()) {
			SpaceDoorExtraRefresh(pMap, aOldExtraXY.empty() ? NULL : &aOldExtraXY[0], (int)aOldExtraXY.size());
		}
	}
	SaveMapInfo();
	strRecord = MapJson(pMap);
	return true;
}

/*
 * マップのマス目を書き換える（パーツ番号。1 回 128×128 マスまで）
 *   pszParts は w×h 個の数をカンマ区切り（左上から行ごと）。
 *   結果: {"ok":true,"before":[[...]],"after":[[...]]} / {"ok":false,"error":"理由"}
 */
extern "C" EMSCRIPTEN_KEEPALIVE const char *SboWebDataTilesSet(int nID, int x, int y, int w, int h, const char *pszParts, int bApply)
{
	std::string strErr;
	std::vector<WORD> aNew;
	if (nID < 0 || nID >= g_aMapInfo.GetSize()) {
		strErr = "マップの番号が範囲外です";
	} else if (w <= 0 || h <= 0 || w > 128 || h > 128) {
		strErr = "w・h は 1〜128 で指定してください";
	} else {
		PMAPINFO pMap = g_aMapInfo[nID];
		if (pMap->pwMap == NULL || x < 0 || y < 0 || x + w > pMap->nCX || y + h > pMap->nCY) {
			strErr = "範囲がマップの外です（" + JsonNum(pMap->nCX) + "×" + JsonNum(pMap->nCY) + "）";
		} else {
			const char *p = pszParts ? pszParts : "";
			while (*p && strErr.empty()) {
				char *pEnd;
				long v = strtol(p, &pEnd, 10);
				if (pEnd == p) {
					strErr = "parts の書き方が不正です";
					break;
				}
				if (v < 0 || v >= g_wCountMapParts) {
					strErr = "パーツ番号 " + JsonNum(v) + " はありません（0〜" + JsonNum(g_wCountMapParts - 1) + "）";
					break;
				}
				aNew.push_back((WORD)v);
				p = pEnd;
				if (*p == ',') p ++;
			}
			if (strErr.empty() && aNew.size() != (size_t)(w * h)) {
				strErr = "parts の数が w×h（" + JsonNum(w * h) + "）と合いません（" + JsonNum((long long)aNew.size()) + "）";
			}
		}
	}
	if (!strErr.empty()) {
		s_strResult = "{\"ok\":false,\"error\":" + JsonStr(strErr.c_str()) + "}";
		return s_strResult.c_str();
	}
	PMAPINFO pMap = g_aMapInfo[nID];
	std::string strBefore = "[", strAfter = "[";
	int nChanged = 0;
	for (int yy = 0; yy < h; yy ++) {
		if (yy) { strBefore += ","; strAfter += ","; }
		strBefore += "[";
		strAfter += "[";
		for (int xx = 0; xx < w; xx ++) {
			WORD &wPart = pMap->pwMap[(x + xx) + (y + yy) * pMap->nCX];
			WORD wNew = aNew[xx + yy * w];
			if (xx) { strBefore += ","; strAfter += ","; }
			strBefore += JsonNum(wPart);
			strAfter += JsonNum(wNew);
			if (wPart != wNew) {
				nChanged ++;
			}
		}
		strBefore += "]";
		strAfter += "]";
	}
	strBefore += "]";
	strAfter += "]";
	if (bApply && nChanged) {
		for (int yy = 0; yy < h; yy ++) {
			for (int xx = 0; xx < w; xx ++) {
				WORD &wPart = pMap->pwMap[(x + xx) + (y + yy) * pMap->nCX];
				WORD wNew = aNew[xx + yy * w];
				if (wPart != wNew) {
					wPart = wNew;
					if (nChanged <= 400) {
						SendCmdCHGMAP(pMap->dwID, wNew, x + xx, y + yy);
					}
				}
			}
		}
		if (nChanged > 400) {
			ResendMapToPlayers(pMap, nID);
		}
		SaveMapInfo();
	}
	s_strResult = "{\"ok\":true,\"mapId\":" + JsonNum(pMap->dwID) + ",\"changed\":" + JsonNum(nChanged) +
			",\"before\":" + strBefore + ",\"after\":" + strAfter + "}";
	return s_strResult.c_str();
}

/* ------------------------------------------------------------------------- */
/* マップパーツ・エフェクト・吹き出し・サブゲームの書き込み					 */
/* ------------------------------------------------------------------------- */
extern BOOL SaveMapPartsInfo(void);
extern BOOL SaveEfcInfo(void);
extern BOOL SaveComEfcInfo(void);
extern BOOL SaveSubGameInfo(void);

/* anime の配列を読む（各要素の項目は 0 から上限まで。省いた項目は 0） */
struct ANIMEFLD {
	const char *pszName;
	long long nMax;
};
static bool ReadAnime(const JVal &v, const ANIMEFLD *paFld, int nFld, int nMaxCount,
		std::vector<std::vector<long long> > &aOut, std::string &strErr)
{
	if (v.t != JVal::ARR) {
		strErr = "anime は配列で指定してください";
		return false;
	}
	if ((int)v.a.size() > nMaxCount) {
		strErr = "anime は " + JsonNum(nMaxCount) + " コマまでです";
		return false;
	}
	aOut.clear();
	for (size_t i = 0; i < v.a.size(); i ++) {
		const JVal &e = v.a[i];
		std::string strName = "anime[" + JsonNum((long long)i) + "]";
		if (e.t != JVal::OBJ) {
			strErr = strName + " はオブジェクトで指定してください";
			return false;
		}
		std::vector<long long> aVal(nFld, 0);
		for (size_t k = 0; k < e.o.size(); k ++) {
			int nIdx = -1;
			for (int f = 0; f < nFld; f ++) {
				if (e.o[k].first == paFld[f].pszName) {
					nIdx = f;
				}
			}
			if (nIdx < 0) {
				strErr = strName + " の知らない項目です: " + e.o[k].first;
				return false;
			}
			if (!GetInt(e.o[k].second, 0, paFld[nIdx].nMax, aVal[nIdx], strErr, strName + "." + e.o[k].first)) {
				return false;
			}
		}
		aOut.push_back(aVal);
	}
	return true;
}

static const ANIMEFLD s_aAnimeParts[] = { { "byWait", 255 }, { "byLevel", 255 }, { "wGrpID", 65535 }, { "wBackID", 65535 } };
static const ANIMEFLD s_aAnimeEfc[] = { { "byWait", 255 }, { "byLevel", 255 }, { "wGrpID", 65535 } };
static const ANIMEFLD s_aAnimeComEfc[] = { { "byWait", 255 }, { "byGrpID", 255 } };

static bool MapPartsApply(int nID, bool bAdd, const JVal &chg, bool bApply, std::string &strErr, std::string &strRecord)
{
	static const char *apszExtra[] = { "anime", NULL };
	if (bAdd && g_wCountMapParts >= 65535) {
		strErr = "これ以上は追加できません";
		return false;
	}
	if (!CheckKnownKeys(chg, s_aFieldMapParts, COUNTOF(s_aFieldMapParts), apszExtra, strErr)) {
		return false;
	}
	MAPPARTSINFO w;
	ZeroMemory(&w, sizeof(w));
	if (!bAdd) {
		w = g_aMapPartsInfo[nID];
	}
	if (!ApplyFields((BYTE *)&w, s_aFieldMapParts, COUNTOF(s_aFieldMapParts), chg, strErr, "")) {
		return false;
	}
	/* anime は作業用の配列で持つ（ポインタは反映する時に作り直す） */
	std::vector<ANIMEINFO> aAnime;
	const JVal *pAnime = chg.Get("anime");
	if (pAnime) {
		std::vector<std::vector<long long> > a;
		if (!ReadAnime(*pAnime, s_aAnimeParts, COUNTOF(s_aAnimeParts), 255, a, strErr)) {
			return false;
		}
		for (size_t i = 0; i < a.size(); i ++) {
			ANIMEINFO ai;
			ai.byWait = (BYTE)a[i][0];
			ai.byLevel = (BYTE)a[i][1];
			ai.wGrpID = (WORD)a[i][2];
			ai.wBackID = (WORD)a[i][3];
			aAnime.push_back(ai);
		}
	} else if (!bAdd) {
		for (int i = 0; w.pAnimeInfo && i < w.byAnimeCount; i ++) {
			aAnime.push_back(w.pAnimeInfo[i]);
		}
	}
	w.byAnimeCount = (BYTE)aAnime.size();
	w.byAnimeNo = 0;
	w.pAnimeInfo = aAnime.empty() ? NULL : &aAnime[0];
	strRecord = MapPartsJson(&w);
	if (!bApply) {
		return true;
	}
	if (bAdd) {
		/* RecvDataProcMAPPARTSADD と同じく 1 つ大きい配列へ移す */
		PMAPPARTSINFO pNew = new MAPPARTSINFO[g_wCountMapParts + 1];
		ZeroMemory(pNew, sizeof(MAPPARTSINFO) * (g_wCountMapParts + 1));
		for (int i = 0; i < g_wCountMapParts; i ++) {
			pNew[i] = g_aMapPartsInfo[i];
		}
		SAFE_DELETE_ARRAY(g_aMapPartsInfo);
		g_aMapPartsInfo = pNew;
		nID = g_wCountMapParts;
		g_wCountMapParts ++;
	}
	PMAPPARTSINFO pDst = &g_aMapPartsInfo[nID];
	SAFE_DELETE_ARRAY(pDst->pAnimeInfo);
	ANIMEINFO *pNewAnime = NULL;
	if (!aAnime.empty()) {
		pNewAnime = new ANIMEINFO[aAnime.size()];
		CopyMemory(pNewAnime, &aAnime[0], aAnime.size() * sizeof(ANIMEINFO));
	}
	DWORD dwLast = pDst->dwLastAnime;
	*pDst = w;
	pDst->pAnimeInfo = pNewAnime;
	pDst->dwLastAnime = dwLast;
	SendCmdMAPPARTS(URARASOCK_SENDALL, nID);
	SaveMapPartsInfo();
	return true;
}

static bool EfcApply(int nID, bool bAdd, const JVal &chg, bool bApply, std::string &strErr, std::string &strRecord)
{
	static const char *apszExtra[] = { "anime", "animeCount", NULL };
	if (bAdd && g_EfcInfo.GetCount() >= 255) {
		strErr = "エフェクトは 255 個までです";
		return false;
	}
	if (!CheckKnownKeys(chg, s_aFieldEfc, COUNTOF(s_aFieldEfc), apszExtra, strErr)) {
		return false;
	}
	PEFCINFO pSrc = bAdd ? NULL : g_EfcInfo.GetPtr(nID);
	if (!bAdd && pSrc == NULL) {
		strErr = "エフェクトが見つかりません";
		return false;
	}
	/* aAnime はクラスなので、項目だけを作業用に写す */
	EFCINFO tmp;
	tmp.byType = pSrc ? pSrc->byType : 0;
	tmp.bySound = pSrc ? pSrc->bySound : 0;
	if (!ApplyFields((BYTE *)&tmp, s_aFieldEfc, COUNTOF(s_aFieldEfc), chg, strErr, "")) {
		return false;
	}
	std::vector<EFCANIMEINFO> aAnime;
	const JVal *pAnime = chg.Get("anime");
	if (pAnime) {
		std::vector<std::vector<long long> > a;
		if (!ReadAnime(*pAnime, s_aAnimeEfc, COUNTOF(s_aAnimeEfc), 255, a, strErr)) {
			return false;
		}
		for (size_t i = 0; i < a.size(); i ++) {
			EFCANIMEINFO ai;
			ai.byWait = (BYTE)a[i][0];
			ai.byLevel = (BYTE)a[i][1];
			ai.wGrpID = (WORD)a[i][2];
			aAnime.push_back(ai);
		}
	} else if (pSrc) {
		for (int i = 0; i < pSrc->aAnime.GetCount(); i ++) {
			aAnime.push_back(*pSrc->aAnime.GetPtr(i));
		}
	}
	/* animeCount は読むだけ（書き出した JSON をそのまま渡された時のため、anime の数と同じなら黙って受ける） */
	const JVal *pAnimeCount = chg.Get("animeCount");
	if (pAnimeCount && !(pAnimeCount->t == JVal::NUM && pAnimeCount->n == (double)aAnime.size())) {
		strErr = "animeCount は変えられません（anime を変えてください）";
		return false;
	}
	strRecord = FieldsJson((const BYTE *)&tmp, s_aFieldEfc, COUNTOF(s_aFieldEfc)) + ",\"animeCount\":" + JsonNum((long long)aAnime.size()) + ",\"anime\":[";
	for (size_t i = 0; i < aAnime.size(); i ++) {
		if (i) strRecord += ",";
		strRecord += "{\"byWait\":" + JsonNum(aAnime[i].byWait) + ",\"byLevel\":" + JsonNum(aAnime[i].byLevel) + ",\"wGrpID\":" + JsonNum(aAnime[i].wGrpID) + "}";
	}
	strRecord += "]";
	if (!bApply) {
		return true;
	}
	if (bAdd) {
		g_EfcInfo.AddNew();
		nID = g_EfcInfo.GetCount() - 1;
	}
	PEFCINFO pDst = g_EfcInfo.GetPtr(nID);
	pDst->byType = tmp.byType;
	pDst->bySound = tmp.bySound;
	pDst->aAnime.DeleteAll();
	for (size_t i = 0; i < aAnime.size(); i ++) {
		PEFCANIMEINFO p = pDst->aAnime.GetNew();
		*p = aAnime[i];
		pDst->aAnime.Add(p);
	}
	SendCmdEFC((BYTE)nID);
	SaveEfcInfo();
	return true;
}

static bool ComEfcApply(int nID, bool bAdd, const JVal &chg, bool bApply, std::string &strErr, std::string &strRecord)
{
	static const char *apszExtra[] = { "anime", NULL };
	if (bAdd) {
		strErr = "吹き出しは " + JsonNum(MAX_COMEFC) + " 個で固定なので足せません（空いている番号を書き換えてください）";
		return false;
	}
	if (!CheckKnownKeys(chg, s_aFieldComEfc, COUNTOF(s_aFieldComEfc), apszExtra, strErr)) {
		return false;
	}
	PCOMEFCINFO pSrc = &g_pComEfcInfo[nID];
	COMEFCINFO w = *pSrc;
	if (!ApplyFields((BYTE *)&w, s_aFieldComEfc, COUNTOF(s_aFieldComEfc), chg, strErr, "")) {
		return false;
	}
	std::vector<COMEFCANIMEINFO> aAnime;
	const JVal *pAnime = chg.Get("anime");
	if (pAnime) {
		std::vector<std::vector<long long> > a;
		if (!ReadAnime(*pAnime, s_aAnimeComEfc, COUNTOF(s_aAnimeComEfc), 255, a, strErr)) {
			return false;
		}
		for (size_t i = 0; i < a.size(); i ++) {
			COMEFCANIMEINFO ai;
			ai.byWait = (BYTE)a[i][0];
			ai.byGrpID = (BYTE)a[i][1];
			aAnime.push_back(ai);
		}
	} else {
		for (int i = 0; pSrc->pAnime && i < pSrc->byAnimeCount; i ++) {
			aAnime.push_back(pSrc->pAnime[i]);
		}
	}
	w.byAnimeCount = (BYTE)aAnime.size();
	w.pAnime = aAnime.empty() ? NULL : &aAnime[0];
	strRecord = ComEfcJson(&w);
	if (!bApply) {
		return true;
	}
	/* RecvDataProcCHGCEF と同じ消し方（1 コマの時は new 1 個で作られている） */
	if (pSrc->pAnime) {
		if (pSrc->byAnimeCount == 1) {
			SAFE_DELETE(pSrc->pAnime);
		} else {
			SAFE_DELETE_ARRAY(pSrc->pAnime);
		}
	}
	pSrc->bySound = w.bySound;
	pSrc->byAnimeCount = w.byAnimeCount;
	pSrc->pAnime = NULL;
	if (!aAnime.empty()) {
		pSrc->pAnime = new COMEFCANIMEINFO[aAnime.size()];
		CopyMemory(pSrc->pAnime, &aAnime[0], aAnime.size() * sizeof(COMEFCANIMEINFO));
	}
	SendCmdCHGCEF(URARASOCK_SENDALL, (WORD)nID);
	SaveComEfcInfo();
	return true;
}

extern void MakeMazeProc(BYTE);			/* SboSvrSubGameProc.cpp */
extern void StopMaze(BYTE);				/* SboSvrSubGameProc.cpp */

/*
 * 迷路の設定を変えたら、中に誰もいなければすぐ作り直す（新しい見た目・太さで）
 *   遊んでいる人がいる時は作り直さない（いつも通り、みんな出て 5 秒たったら作り直す）
 */
static void SubGameRenewIfEmpty(int nID)
{
	PSUBGAMEINFO pInfo = g_aSubaGameInfo[nID];
	if (pInfo == NULL || pInfo->pInfo == NULL || nID > 255 || pInfo->byType != SUBGAMEID_MAZE) {
		return;
	}
	PSUBGAME_MAZEINFO p = (PSUBGAME_MAZEINFO)pInfo->pInfo;
	if (p->pMapTmp || p->dwMapID >= (DWORD)g_aMapInfo.GetSize() || g_aMapInfo[p->dwMapID] == NULL) {
		return;
	}
	int nCount = g_aCharInfo.GetSize();
	for (int i = 0; i < nCount; i ++) {
		PCHARINFO pChar = g_aCharInfo[i];
		if (pChar == NULL || pChar->byGameState == GAMESTATE_NPC || pChar->byGameState < GAMESTATE_MOVE) {
			continue;
		}
		if ((pChar->dwMapID == p->dwMapID) &&
			(pChar->dwX >= (DWORD)p->byX1) && (pChar->dwX <= (DWORD)p->byX2) &&
			(pChar->dwY >= (DWORD)p->byY1) && (pChar->dwY <= (DWORD)p->byY2)) {
			return;
		}
	}
	MakeMazeProc((BYTE)nID);
}

/* 番外編ダンジョンの設定の確かめ（テンプレートの数は 0〜MAX_DGTMPL） */
static bool CheckDungeonDetail(const SUBGAME_DUNGEONINFO *p, std::string &strErr)
{
	if (p->byTmplCount > MAX_DGTMPL) {
		strErr = "detail.byTmplCount は 0〜" + JsonNum(MAX_DGTMPL) + " にしてください";
		return false;
	}
	if (p->byClearMode > DGCLEAR_DEFENSE) {
		strErr = "detail.byClearMode は 0（ゴール）・1（敵を全部倒す）・2（拠点防衛）のどれかにしてください";
		return false;
	}
	if (p->byRebuildMode > DGREBUILD_FIXED) {
		strErr = "detail.byRebuildMode は 0（今まで通り）・1（日替わり）・2（作り直さない）のどれかにしてください";
		return false;
	}
	return true;
}

/* 迷路の設定がマップに収まっているか確かめる */
static bool CheckMazeDetail(const SUBGAME_MAZEINFO *p, std::string &strErr)
{
	if (p->dwMapID >= (DWORD)g_aMapInfo.GetSize() || g_aMapInfo[p->dwMapID] == NULL) {
		strErr = "detail.dwMapID のマップがありません";
		return false;
	}
	PMAPINFO pMap = g_aMapInfo[p->dwMapID];
	if (p->byX1 >= p->byX2 || p->byY1 >= p->byY2) {
		strErr = "迷路の範囲は byX1 < byX2・byY1 < byY2 にしてください";
		return false;
	}
	if ((int)p->byX2 >= pMap->nCX || (int)p->byY2 >= pMap->nCY) {
		strErr = "迷路の範囲（byX2・byY2）がマップの外に出ています（マップは " + JsonNum(pMap->nCX) + "×" + JsonNum(pMap->nCY) + "）";
		return false;
	}
	int nWidth = (p->byPathWidth >= 1 && p->byPathWidth <= 3) ? p->byPathWidth : 2;
	if ((p->byX2 - p->byX1) / nWidth + 1 < 3 || (p->byY2 - p->byY1) / nWidth + 1 < 3) {
		strErr = "迷路の範囲が狭すぎます（通路の太さの 3 倍以上にしてください）";
		return false;
	}
	if (p->byPathWidth > 3) {
		strErr = "通路の太さ（byPathWidth）は 0〜3 にしてください（0 は 2 マス）";
		return false;
	}
	if (p->byRoad2Rate > 100 || p->byBlock2Rate > 100 || p->byCrossRate > 100) {
		strErr = "混ぜる割合・立体交差の多さは 0〜100 にしてください";
		return false;
	}
	int nParts = (int)g_wCountMapParts;
	const WORD awParts[] = { p->wChipIDStart, p->wChipIDGoal, p->wChipIDRoad, p->wChipIDBlock, p->wChipIDRoad2, p->wChipIDBlock2, p->wChipIDBlockFace,
							 p->wChipIDBridgeV, p->wChipIDBridgeH };
	for (size_t i = 0; i < COUNTOF(awParts); i ++) {
		if ((int)awParts[i] >= nParts) {
			strErr = "パーツ番号 " + JsonNum(awParts[i]) + " がありません（mapParts は " + JsonNum(nParts) + " 個）";
			return false;
		}
	}
	/* 立体交差の橋は、当たり判定が橋のパーツでないと上と下に分かれない */
	if (p->wChipIDBridgeV && g_aMapPartsInfo[p->wChipIDBridgeV].byHitType != PARTSHIT_BRIDGEV) {
		strErr = "wChipIDBridgeV のパーツ " + JsonNum(p->wChipIDBridgeV) + " は当たり判定（byHitType）が 19（橋・縦に渡る）ではありません";
		return false;
	}
	if (p->wChipIDBridgeH && g_aMapPartsInfo[p->wChipIDBridgeH].byHitType != PARTSHIT_BRIDGEH) {
		strErr = "wChipIDBridgeH のパーツ " + JsonNum(p->wChipIDBridgeH) + " は当たり判定（byHitType）が 20（橋・横に渡る）ではありません";
		return false;
	}
	if (p->byCrossRate && !p->wChipIDBridgeV && !p->wChipIDBridgeH) {
		strErr = "立体交差（byCrossRate）を使う時は、橋のパーツ（wChipIDBridgeV か wChipIDBridgeH）も指定してください";
		return false;
	}
	if ((int)p->byStartX >= pMap->nCX || (int)p->byStartY >= pMap->nCY ||
		(int)p->byGoalToX >= pMap->nCX || (int)p->byGoalToY >= pMap->nCY) {
		strErr = "入口（byStartX/Y）かゴールの出口（byGoalToX/Y）がマップの外です";
		return false;
	}
	return true;
}

/*
 * サブゲームを変える・足す
 *   足せるのは迷路（byType 2）だけ。迷路を byType 0 にすると止まる（元に戻す時の「消す」）。
 *   種類が 0 の枠は迷路にして使い回せる。
 */
static bool SubGameApply(int nID, bool bAdd, const JVal &chg, bool bApply, std::string &strErr, std::string &strRecord)
{
	static const char *apszKnown[] = { "byType", "typeName", "detail", NULL };
	if (!CheckKnownKeys(chg, NULL, 0, apszKnown, strErr)) {
		return false;
	}
	PSUBGAMEINFO pInfo = bAdd ? NULL : g_aSubaGameInfo[nID];
	if (!bAdd && pInfo == NULL) {
		strErr = "サブゲームが見つかりません";
		return false;
	}
	int nTypeNow = bAdd ? SUBGAMEID_NONE : pInfo->byType;
	int nTypeNew = nTypeNow;
	const JVal *pType = chg.Get("byType");
	if (pType) {
		if (pType->t != JVal::NUM) {
			strErr = "byType は数で指定してください";
			return false;
		}
		nTypeNew = (int)pType->n;
	}
	if (nTypeNew != nTypeNow) {
		bool bOk = (nTypeNow == SUBGAMEID_NONE && nTypeNew == SUBGAMEID_MAZE) ||
				   (nTypeNow == SUBGAMEID_MAZE && nTypeNew == SUBGAMEID_NONE);
		if (!bOk) {
			strErr = bAdd ? "足せるのは迷路（byType 2）だけです" :
					"種類（byType）は 迷路(2)⇔無し(0) だけ変えられます（ほかは管理者ウィンドウで）";
			return false;
		}
	}
	if (bAdd && nTypeNew != SUBGAMEID_MAZE) {
		strErr = "足せるのは迷路（byType 2）だけです。byType: 2 と detail を指定してください";
		return false;
	}
	const JVal *pName = chg.Get("typeName");
	if (pName && !(pName->t == JVal::STR && pName->s == ((nTypeNew < SUBGAMEID_MAX) ? s_apszSubGameName[nTypeNew] : "?"))) {
		strErr = "typeName は変えられません";
		return false;
	}
	const JVal *pDetail = chg.Get("detail");

	/* 迷路を止める（種類を 0 に） */
	if (nTypeNew == SUBGAMEID_NONE && nTypeNow == SUBGAMEID_MAZE) {
		SUBGAMEINFO Tmp;
		ZeroMemory(&Tmp, sizeof(Tmp));
		strRecord = SubGameJson(&Tmp);
		if (!bApply) {
			return true;
		}
		StopMaze((BYTE)nID);
		PSUBGAME_MAZEINFO pOld = (PSUBGAME_MAZEINFO)pInfo->pInfo;
		pInfo->byType = SUBGAMEID_NONE;
		pInfo->pInfo = NULL;
		if (pOld) {
			SAFE_DELETE_ARRAY(pOld->pMapTmp);
			delete [] (BYTE *)pOld;
		}
		SaveSubGameInfo();
		return true;
	}

	int nCount = 0;
	const DATAFIELD *paField = SubGameFields(nTypeNew, &nCount);
	int nSize = GetSubGameDataSize(nTypeNew);
	bool bNew = (nTypeNew != nTypeNow);		/* 足す・空き枠を迷路にする */
	if (pDetail) {
		if (paField == NULL || nSize <= 0 || (!bNew && pInfo->pInfo == NULL)) {
			strErr = "このサブゲームには設定がありません";
			return false;
		}
		if (pDetail->t != JVal::OBJ) {
			strErr = "detail はオブジェクトで指定してください";
			return false;
		}
		static const char *apszSgExtra[] = { "events", "parts", NULL };
		if (!CheckKnownKeys(*pDetail, paField, nCount, (nTypeNew == SUBGAMEID_SUGOROKU) ? apszSgExtra : NULL, strErr)) {
			return false;
		}
		if (pDetail->Get("events") || pDetail->Get("parts")) {
			strErr = "すごろくのマス（events・parts）は管理者ウィンドウで変えてください";
			return false;
		}
	} else if (bNew) {
		strErr = "迷路の設定（detail）を指定してください";
		return false;
	}

	/* 作業用に丸ごと写して当てる（ポインタは作業用では使わない） */
	std::vector<BYTE> aWork(nSize > 0 ? nSize : 0, 0);
	if (!bNew && pInfo->pInfo && nSize > 0) {
		CopyMemory(&aWork[0], pInfo->pInfo, nSize);
	}
	if (pDetail && !ApplyFields(&aWork[0], paField, nCount, *pDetail, strErr, "detail.")) {
		return false;
	}
	if (nTypeNew == SUBGAMEID_MAZE && pDetail && !CheckMazeDetail((const SUBGAME_MAZEINFO *)&aWork[0], strErr)) {
		return false;
	}
	if (nTypeNew == SUBGAMEID_DUNGEON && pDetail && !CheckDungeonDetail((const SUBGAME_DUNGEONINFO *)&aWork[0], strErr)) {
		return false;
	}

	/* 結果の JSON（作業用の値で作る） */
	{
		SUBGAMEINFO Tmp;
		ZeroMemory(&Tmp, sizeof(Tmp));
		Tmp.byType = (BYTE)nTypeNew;
		Tmp.pInfo = aWork.empty() ? NULL : &aWork[0];
		if (nTypeNew == SUBGAMEID_SUGOROKU && pInfo) {
			Tmp.pInfo = pInfo->pInfo;		/* すごろくは events などがあるのでそのまま（detail は上で変えられない項目だけ） */
			if (pDetail) {
				std::vector<BYTE> aBack((BYTE *)pInfo->pInfo, (BYTE *)pInfo->pInfo + nSize);
				CopyMemory(pInfo->pInfo, &aWork[0], nSize);
				strRecord = SubGameJson(&Tmp);
				CopyMemory(pInfo->pInfo, &aBack[0], nSize);
			} else {
				strRecord = SubGameJson(&Tmp);
			}
		} else {
			strRecord = SubGameJson(&Tmp);
		}
	}
	if (!bApply) {
		return true;
	}

	if (bNew) {
		/* 新しい迷路の枠を作る */
		BYTE *pNew = new BYTE[nSize];
		CopyMemory(pNew, &aWork[0], nSize);
		PSUBGAME_MAZEINFO pMaze = (PSUBGAME_MAZEINFO)pNew;
		pMaze->pMapTmp = NULL;
		pMaze->dwTblID = (DWORD)-1;
		pMaze->dwCountTmp = 0;
		pMaze->dwTimeCheck = 0;
		pMaze->dwTblIDCheck = 0;
		pMaze->byTmpX = pMaze->byTmpY = 0;
		if (bAdd) {
			pInfo = new SUBGAMEINFO;
			ZeroMemory(pInfo, sizeof(SUBGAMEINFO));
			pInfo->byType = (BYTE)nTypeNew;
			pInfo->pInfo = pNew;
			g_aSubaGameInfo.Add(pInfo);
			nID = g_aSubaGameInfo.GetSize() - 1;
		} else {
			pInfo->byType = (BYTE)nTypeNew;
			pInfo->pInfo = pNew;
		}
	} else if (pDetail && nSize > 0) {
		/* 元のマップ番号で通知してから設定を書き戻す（下見では捨てない） */
		if (nTypeNew == SUBGAMEID_DUNGEON) {
			InvalidateDungeonAutoMap((PSUBGAME_DUNGEONINFO)pInfo->pInfo);
		}
		/* 項目表にある所だけを書き戻す（作成中の設計図などのポインタは触らない） */
		BYTE *pDst = (BYTE *)pInfo->pInfo;
		for (int i = 0; i < nCount; i ++) {
			/* 文字・配列は項目表の大きさの分だけ写す（以前は先頭 1 バイトしか書き戻していなかった） */
			size_t n;
			switch (paField[i].nType) {
			case FT_U32:	n = 4;								break;
			case FT_U16:	n = 2;								break;
			case FT_STR:	n = paField[i].nSize;				break;
			case FT_U8ARR:	n = paField[i].nSize;				break;
			case FT_U16ARR:	n = paField[i].nSize * sizeof(WORD);	break;
			default:		n = 1;								break;
			}
			CopyMemory(pDst + paField[i].nOffset, &aWork[paField[i].nOffset], n);
		}
	}
	SaveSubGameInfo();
	if (pDetail || bNew) {
		SubGameRenewIfEmpty(nID);
	}
	return true;
}

/*
 * 変更を確かめ、bApply なら反映する
 *   pszChanges は JSON のオブジェクト（変える項目だけ）。id が件数と同じなら追加する。
 *   結果: {"ok":true,"id":番号,"added":bool,"record":{変更後}} / {"ok":false,"error":"理由"}
 *   bApply が 0 の時は何も変えず、変更後の姿（record）だけ返す。
 */
extern "C" EMSCRIPTEN_KEEPALIVE const char *SboWebDataApply(const char *pszKind, int nID, const char *pszChanges, int bApply, const char *pszSource)
{
	static const BYTE abyUndoCmd[] = { CMD_MAGICINFO, CMD_MAGICUSERINFO, CMD_EQUUSERINFO, CMD_JOBINFO, CMD_STTMPLATE, CMD_CHGITEM };
	BOOL bUndo = FALSE;
	std::string strErr;
	int nKind = KindFromName(pszKind);
	int nCount = (nKind < 0) ? 0 : KindCount(nKind);
	bool bAdd = false;
	JVal chg;
	std::string strRecord;

	if (nKind < 0) {
		strErr = "知らない種類です";
		goto Exit;
	}
	if (nID < 0 || nID > nCount) {
		strErr = "番号が範囲外です（0〜" + JsonNum(nCount - 1) + "、追加は " + JsonNum(nCount) + "）";
		goto Exit;
	}
	if (!KindWritable(nKind)) {
		strErr = "読むだけのデータです";
		goto Exit;
	}
	bAdd = (nID == nCount);
	if (nKind >= KIND_FIRST_READONLY) {
		/* NPC・マップなど（項目表に無い作りなので別の関数で扱う） */
		CJsonReader reader(pszChanges ? pszChanges : "");
		bool bDeleted = false;
		int nIDOut = nID;
		if (!reader.Parse(chg) || chg.t != JVal::OBJ) {
			strErr = "変更内容は JSON のオブジェクトで指定してください";
		} else if (chg.Get("id")) {
			strErr = "id は変えられません";
		} else if (nKind == KIND_CHAR) {
			CharApply(nID, bAdd, chg, bApply != 0, strErr, strRecord, nIDOut, bDeleted);
		} else if (nKind == KIND_MAP) {
			if (bAdd) {
				strErr = "マップは足せません（管理者ウィンドウで足してください）";
			} else {
				MapApply(nID, chg, bApply != 0, strErr, strRecord);
			}
		} else if (nKind == KIND_MAPPARTS) {
			MapPartsApply(nID, bAdd, chg, bApply != 0, strErr, strRecord);
		} else if (nKind == KIND_EFC) {
			EfcApply(nID, bAdd, chg, bApply != 0, strErr, strRecord);
		} else if (nKind == KIND_COMEFC) {
			ComEfcApply(nID, bAdd, chg, bApply != 0, strErr, strRecord);
		} else if (nKind == KIND_SUBGAME) {
			SubGameApply(nID, bAdd, chg, bApply != 0, strErr, strRecord);
		}
		if (!strErr.empty()) {
			s_strResult = "{\"ok\":false,\"error\":" + JsonStr(strErr.c_str()) + "}";
		} else if (bDeleted) {
			s_strResult = "{\"ok\":true,\"id\":" + JsonNum(nID) + ",\"added\":false,\"deleted\":true,\"record\":null}";
		} else {
			s_strResult = "{\"ok\":true,\"id\":" + JsonNum(nIDOut) + ",\"added\":" + (bAdd ? "true" : "false") +
					",\"record\":{\"id\":" + JsonNum(nIDOut) + "," + strRecord + "}}";
		}
		return s_strResult.c_str();
	}
	if (bAdd && nCount >= s_anKindMax[nKind]) {
		strErr = "これ以上は追加できません";
		goto Exit;
	}
	{
		CJsonReader reader(pszChanges ? pszChanges : "");
		if (!reader.Parse(chg) || chg.t != JVal::OBJ) {
			strErr = "変更内容は JSON のオブジェクトで指定してください";
			goto Exit;
		}
	}
	for (size_t i = 0; i < chg.o.size(); i ++) {
		if (chg.o[i].first == "id") {
			strErr = "id は変えられません";
			goto Exit;
		}
	}

	/*
	 * 管理者ウィンドウの「元に戻す」（SboSvrAdminUndo.cpp）にも記録する（既存のデータを変える時だけ）。
	 * 確かめの途中で失敗した時は AdminUndoAfter を呼ばないので記録されない（次の AdminUndoBefore* で捨てられる）。
	 */
	/* 作り手・レシピは管理者ウィンドウの画面が無いので、AI 用の変更履歴（SboWebDataHistory.json）で元に戻す */
	if (bApply && !bAdd && nKind <= KIND_ITEM) {
		bUndo = AdminUndoBeforeTarget(abyUndoCmd[nKind], (DWORD)nID, pszSource ? pszSource : "API", 0);
	}

	switch (nKind) {
	case KIND_MAGIC:
		{
			MAGICINFO Info;
			memset(&Info, 0, sizeof(Info));
			if (!bAdd) {
				Info = *g_aMagicInfo[nID];
			}
			if (!CheckKnownKeys(chg, s_aFieldMagic, COUNTOF(s_aFieldMagic), NULL, strErr) ||
					!ApplyFields((BYTE *)&Info, s_aFieldMagic, COUNTOF(s_aFieldMagic), chg, strErr, "")) {
				goto Exit;
			}
			strRecord = FieldsJson((const BYTE *)&Info, s_aFieldMagic, COUNTOF(s_aFieldMagic));
			if (bApply) {
				if (bAdd) {
					PMAGICINFO pNew = new MAGICINFO;
					*pNew = Info;
					g_aMagicInfo.Add(pNew);
				} else {
					*g_aMagicInfo[nID] = Info;
				}
				SendCmdMAGICINFO(URARASOCK_SENDALL, (BYTE)nID);
				SaveMagicInfo();
			}
		}
		break;

	case KIND_MAGICUSER:
	case KIND_EQUUSER:
		{
			const DATAFIELD *paField = (nKind == KIND_MAGICUSER) ? s_aFieldMagicUser : s_aFieldEquUser;
			USERWORK w;
			memset(w.szName, 0, sizeof(w.szName));
			if (!bAdd) {
				if (nKind == KIND_MAGICUSER) {
					UserToWork(g_aMagicUserInfo[nID], w);
				} else {
					UserToWork(g_aEquUserInfo[nID], w);
				}
			}
			if (!UserApply(w, paField, chg, strErr)) {
				goto Exit;
			}
			strRecord = UserWorkJson(w, paField);
			if (bApply) {
				if (nKind == KIND_MAGICUSER) {
					if (bAdd) {
						PMAGICUSERINFO pNew = new MAGICUSERINFO;
						memset(pNew, 0, sizeof(*pNew));
						g_aMagicUserInfo.Add(pNew);
					}
					WorkToUser(w, g_aMagicUserInfo[nID]);
					SendCmdMAGICUSERINFO(URARASOCK_SENDALL, (BYTE)nID);
					SaveMagicUserInfo();
				} else {
					if (bAdd) {
						PEQUUSERINFO pNew = new EQUUSERINFO;
						memset(pNew, 0, sizeof(*pNew));
						g_aEquUserInfo.Add(pNew);
					}
					WorkToUser(w, g_aEquUserInfo[nID]);
					SendCmdEQUUSERINFO(URARASOCK_SENDALL, (BYTE)nID);
					SaveEquUserInfo();
				}
			}
		}
		break;

	case KIND_JOB:
		{
			JOBWORK w;
			memset(&w.Info, 0, sizeof(w.Info));
			if (!bAdd) {
				JobToWork(g_JobInfo.GetPtr(nID), w);
			}
			if (!JobApply(w, chg, strErr)) {
				goto Exit;
			}
			strRecord = JobWorkJson(w);
			if (bApply) {
				PJOBINFO pJob;
				if (bAdd) {
					pJob = g_JobInfo.GetNew();
					g_JobInfo.Add(pJob);		/* 職業 ID はここで決まる */
				} else {
					pJob = g_JobInfo.GetPtr(nID);
				}
				WorkToJob(w, pJob);
				SendCmdJOBINFO(URARASOCK_SENDALL, (BYTE)nID);
				SaveJobInfo();
			}
		}
		break;

	case KIND_STTMP:
		{
			STATUSTEMPLATEINFO Info;
			memset(&Info, 0, sizeof(Info));
			if (!bAdd) {
				Info = *g_aStTmpInfo[nID];
			} else {
				Info.byAffixRate = 10;		/* 足す時に省いたら古いデータと同じ（10%・格 1） */
				Info.byAffixRank = 1;
			}
			if (!CheckKnownKeys(chg, s_aFieldStTmp, COUNTOF(s_aFieldStTmp), NULL, strErr) ||
					!ApplyFields((BYTE *)&Info, s_aFieldStTmp, COUNTOF(s_aFieldStTmp), chg, strErr, "")) {
				goto Exit;
			}
			if (Info.byAffixRate > 100) {
				strErr = "byAffixRate は 0〜100 で指定してください";
				goto Exit;
			}
			if (Info.byAffixRank > 3) {
				strErr = "byAffixRank は 0〜3 で指定してください";
				goto Exit;
			}
			strRecord = FieldsJson((const BYTE *)&Info, s_aFieldStTmp, COUNTOF(s_aFieldStTmp));
			if (bApply) {
				if (bAdd) {
					PSTATUSTEMPLATEINFO pNew = new STATUSTEMPLATEINFO;
					*pNew = Info;
					g_aStTmpInfo.Add(pNew);
					SendCmdSTTMPLATE(nID, CMDSTTMPLATETYPE_ADD);
				} else {
					*g_aStTmpInfo[nID] = Info;
					SendCmdSTTMPLATE(nID, CMDSTTMPLATETYPE_CHG);
					ApplyStTmpToChars(nID);
				}
				SaveStTmpInfo();
			}
		}
		break;

	case KIND_ITEM:
		{
			ITEMWORK w;
			memset(&w.Info, 0, sizeof(w.Info));
			if (!bAdd) {
				ItemToWork(g_aItemInfo[nID], w);
			}
			if (!ItemApply(w, chg, strErr)) {
				goto Exit;
			}
			strRecord = ItemWorkJson(w);
			if (bApply) {
				if (bAdd) {
					PITEMINFO pNew = new ITEMINFO;
					memset(pNew, 0, sizeof(*pNew));
					g_aItemInfo.Add(pNew);
				}
				WorkToItem(w, g_aItemInfo[nID]);
				/* 管理者ウィンドウから変えた時と同じく、全員へ送って攻撃力・守備力を更新する */
				SendCmdCHGITEM(URARASOCK_SENDALL, nID);
				for (int i = 0; i < g_aCharInfo.GetSize(); i ++) {
					PCHARINFO pChar = g_aCharInfo[i];
					RenewATDF(i);
					SendCmdCHARSTATUS(pChar->pID, pChar->dwTblID);
				}
				SaveItemInfo();
			}
		}
		break;

	case KIND_MAKER:
		{
			MAKERINFO Info;
			memset(&Info, 0, sizeof(Info));
			if (!bAdd) {
				Info = *g_aMakerInfo[nID];
			} else {
				Info.byState = RECIPESTATE_ON;		/* 足す時に状態を省いたら有効にする */
			}
			if (!CheckKnownKeys(chg, s_aFieldMaker, COUNTOF(s_aFieldMaker), NULL, strErr) ||
					!ApplyFields((BYTE *)&Info, s_aFieldMaker, COUNTOF(s_aFieldMaker), chg, strErr, "")) {
				goto Exit;
			}
			if (Info.byState > RECIPESTATE_OFF) {
				strErr = "byState は 0（削除済み）・1（有効）・2（停止中）のどれかで指定してください";
				goto Exit;
			}
			if (Info.byKind > MAKERKIND_APPRAISE) {
				strErr = "byKind は 0（レシピで作る）か 1（鑑定屋）で指定してください";
				goto Exit;
			}
			if (Info.byState != RECIPESTATE_NONE && Info.szName[0] == 0) {
				strErr = "szName（名前）は空にできません";
				goto Exit;
			}
			strRecord = FieldsJson((const BYTE *)&Info, s_aFieldMaker, COUNTOF(s_aFieldMaker));
			if (bApply) {
				if (bAdd) {
					PMAKERINFO pNew = new MAKERINFO;
					*pNew = Info;
					g_aMakerInfo.Add(pNew);
				} else {
					*g_aMakerInfo[nID] = Info;
				}
				SendCmdMAKERINFO(URARASOCK_SENDALL, (WORD)nID);
				SaveMakerInfo();
			}
		}
		break;

	case KIND_RECIPE:
		{
			RECIPEINFO Info;
			memset(&Info, 0, sizeof(Info));
			if (!bAdd) {
				Info = *g_aRecipeInfo[nID];
			} else {
				Info.byState = RECIPESTATE_ON;
				Info.byCount = 1;
			}
			if (!CheckKnownKeys(chg, s_aFieldRecipe, COUNTOF(s_aFieldRecipe), NULL, strErr) ||
					!ApplyFields((BYTE *)&Info, s_aFieldRecipe, COUNTOF(s_aFieldRecipe), chg, strErr, "")) {
				goto Exit;
			}
			if (Info.byState > RECIPESTATE_OFF) {
				strErr = "byState は 0（削除済み）・1（有効）・2（停止中）のどれかで指定してください";
				goto Exit;
			}
			if (Info.byState != RECIPESTATE_NONE) {
				int nMat = 0;
				if (Info.byMaker >= g_aMakerInfo.GetSize()) {
					strErr = "byMaker は maker の番号（0〜" + JsonNum(g_aMakerInfo.GetSize() - 1) + "）で指定してください。先に maker を足してください";
					goto Exit;
				}
				if (Info.byKind == RECIPEKIND_NORMAL && (Info.wItem == 0 || Info.wItem >= g_aItemInfo.GetSize())) {
					strErr = "wItem は item の番号（1〜" + JsonNum(g_aItemInfo.GetSize() - 1) + "）で指定してください";
					goto Exit;
				}
				if (Info.byKind > RECIPEKIND_SWAP) {
					strErr = "byKind は 0（普通）・1（鍛える）・2（付け替える）のどれかで指定してください";
					goto Exit;
				}
				if (Info.byKind != RECIPEKIND_NORMAL) {
					/* 鍛える・付け替える：できる物は無く（個数は 1 固定）、付与と対象の装備を決める */
					PITEMAFFIXINFO pAffix = NULL;
					for (int k = 0; k < g_aAffixInfo.GetSize(); k ++) {
						if (Info.wAffix != 0 && g_aAffixInfo[k]->wID == Info.wAffix) {
							pAffix = g_aAffixInfo[k];
						}
					}
					if (pAffix == NULL) {
						strErr = "wAffix は affix の wID（付与番号）で指定してください。先に affix を足してください";
						goto Exit;
					}
					if (Info.byKind == RECIPEKIND_FORGE && pAffix->byPos != AFFIXPOS_BACK) {
						strErr = "鍛えるレシピの wAffix は位置が後ろ（byPos=1）の付与にしてください";
						goto Exit;
					}
					if (Info.wEquMask & ~(WORD)AFFIXEQU_ALL) {
						strErr = "wEquMask は 0〜255（0=付与側に従う。武器=1・鎧=2・盾=4・兜=8・小手=16・靴=32・首飾り=64・指輪=128 のビットの和）で指定してください";
						goto Exit;
					}
					Info.wItem = 0;
					Info.byCount = 1;
					Info.byAllowPlain = Info.byAllowPlain ? 1 : 0;
				} else {
					Info.wAffix = 0;
					Info.wEquMask = 0;
					Info.byAllowPlain = 0;
				}
				if (Info.byKind == RECIPEKIND_NORMAL && Info.byCount < 1) {
					strErr = "byCount（できる個数）は 1 以上にしてください";
					goto Exit;
				}
				for (int k = 0; k < MAX_RECIPEMAT; k ++) {
					if (Info.awMatItem[k] == 0) {
						Info.abyMatCount[k] = 0;
						continue;
					}
					if (Info.awMatItem[k] >= g_aItemInfo.GetSize()) {
						strErr = "awMatItem[" + JsonNum(k) + "] は item の番号で指定してください";
						goto Exit;
					}
					if (Info.abyMatCount[k] < 1) {
						strErr = "abyMatCount[" + JsonNum(k) + "]（材料の個数）は 1 以上にしてください";
						goto Exit;
					}
					nMat ++;
				}
				if (nMat == 0) {
					strErr = "材料を 1 種以上指定してください（awMatItem・abyMatCount）";
					goto Exit;
				}
			}
			strRecord = FieldsJson((const BYTE *)&Info, s_aFieldRecipe, COUNTOF(s_aFieldRecipe));
			if (bApply) {
				if (bAdd) {
					PRECIPEINFO pNew = new RECIPEINFO;
					*pNew = Info;
					g_aRecipeInfo.Add(pNew);
				} else {
					*g_aRecipeInfo[nID] = Info;
				}
				SendCmdRECIPEINFO(URARASOCK_SENDALL, (WORD)nID);
				SaveRecipeInfo();
			}
		}
		break;

	case KIND_AFFIX:
		{
			ITEMAFFIXINFO Info;
			memset(&Info, 0, sizeof(Info));
			if (!bAdd) {
				Info = *g_aAffixInfo[nID];
			} else {
				Info.byRank = 1;
				Info.wEquMask = AFFIXEQU_ALL;
				Info.wWeight = 100;
				Info.wGoldRate = 100;
			}
			if (!CheckKnownKeys(chg, s_aFieldAffix, COUNTOF(s_aFieldAffix), NULL, strErr) ||
					!ApplyFields((BYTE *)&Info, s_aFieldAffix, COUNTOF(s_aFieldAffix), chg, strErr, "")) {
				goto Exit;
			}
			if (Info.byPos > AFFIXPOS_BACK) {
				strErr = "byPos は 0（前に付く）か 1（後ろに付く）で指定してください";
				goto Exit;
			}
			if (Info.szName[0] == 0) {
				strErr = "szName（名前）は空にできません";
				goto Exit;
			}
			if (Info.wEquMask == 0 || (Info.wEquMask & ~(WORD)AFFIXEQU_ALL)) {
				strErr = "wEquMask は 1〜255（武器=1・鎧=2・盾=4・兜=8・小手=16・靴=32・首飾り=64・指輪=128 のビットの和）で指定してください";
				goto Exit;
			}
			if (Info.byRank < 1 || Info.byRank > 3) {
				strErr = "byRank（格）は 1〜3 で指定してください";
				goto Exit;
			}
			if (Info.wNext != 0) {
				/* 次の段は、表にある別の付与で、位置が同じもの（＋1→＋2→＋3 の鎖が自分に戻らないように、自分は指せない） */
				PITEMAFFIXINFO pNext = NULL;
				for (int k = 0; k < g_aAffixInfo.GetSize(); k ++) {
					if (g_aAffixInfo[k]->wID == Info.wNext) {
						pNext = g_aAffixInfo[k];
					}
				}
				if (pNext == NULL || (!bAdd && Info.wNext == Info.wID)) {
					strErr = "wNext は表にある別の付与の wID で指定してください（0 ならこれ以上は鍛えられない）";
					goto Exit;
				}
				if (pNext->byPos != Info.byPos) {
					strErr = "wNext は位置（byPos）が同じ付与にしてください";
					goto Exit;
				}
			}
			/* 付与番号を振る（今ある一番大きい番号 + 1。使い回さない） */
			if (bAdd) {
				DWORD dwNew = 1;
				for (int k = 0; k < g_aAffixInfo.GetSize(); k ++) {
					if ((DWORD)g_aAffixInfo[k]->wID >= dwNew) {
						dwNew = (DWORD)g_aAffixInfo[k]->wID + 1;
					}
				}
				if (dwNew > 65535) {
					strErr = "付与がいっぱいで足せません";
					goto Exit;
				}
				Info.wID = (WORD)dwNew;
			}
			strRecord = FieldsJson((const BYTE *)&Info, s_aFieldAffix, COUNTOF(s_aFieldAffix));
			if (bApply) {
				if (bAdd) {
					PITEMAFFIXINFO pNew = new ITEMAFFIXINFO;
					*pNew = Info;
					g_aAffixInfo.Add(pNew);
				} else {
					*g_aAffixInfo[nID] = Info;
				}
				SendCmdAFFIXINFO(URARASOCK_SENDALL, (WORD)nID);
				SaveAffixInfo();
			}
		}
		break;

	case KIND_QUEST:
		{
			static const char *apszQuestExtra[] = { "goals", "npcs", NULL };
			QUESTINFO Info;
			memset(&Info, 0, sizeof(Info));
			if (!bAdd) {
				Info = *g_aQuestInfo[nID];
			} else {
				Info.byState = QUESTSTATE_ON;
				Info.byStepCount = 1;
				for (int k = 0; k < MAX_QUESTNPC; k ++) {
					Info.aNpc[k].byStepMax = QUESTNPC_NOSTEP;
					Info.aNpc[k].byNextStep = QUESTNPC_NOSTEP;
				}
				/* 完了した時に立てる印・動かす数値は「無し」から（0 のままだと印 0・数値 0 になる） */
				for (int k = 0; k < MAX_QUESTDONESET; k ++) {
					Info.awDoneSetFlag[k] = FLAG_NOBIT;
				}
				Info.byDoneNumNo = FLAG_NONUM;
				Info.wNeedFlag = FLAG_NOBIT;
				Info.wNotFlag = FLAG_NOBIT;
			}
			if (!CheckKnownKeys(chg, s_aFieldQuest, COUNTOF(s_aFieldQuest), apszQuestExtra, strErr)) {
				goto Exit;
			}
			/* wID は読むだけ（自動で振る）。同じ値を書き戻すのは許す（取得した記録をそのまま渡せるように） */
			{
				JVal chg2 = chg;
				for (size_t k = 0; k < chg2.o.size(); k ++) {
					if (chg2.o[k].first == "wID") {
						if (!(chg2.o[k].second.t == JVal::NUM && !bAdd && chg2.o[k].second.n == Info.wID)) {
							strErr = "wID は変えられません（足す時に自動で振られます）";
							goto Exit;
						}
						chg2.o.erase(chg2.o.begin() + k);
						break;
					}
				}
				if (!ApplyFields((BYTE *)&Info, s_aFieldQuest, COUNTOF(s_aFieldQuest), chg2, strErr, "")) {
					goto Exit;
				}
			}
			/* 段ごとの目的 */
			if (const JVal *pGoals = chg.Get("goals")) {
				if (pGoals->t != JVal::ARR || pGoals->a.size() > (size_t)MAX_QUESTSTEP) {
					strErr = "goals は文字列の配列で、" + JsonNum(MAX_QUESTSTEP) + " 個までです";
					goto Exit;
				}
				memset(Info.aszGoal, 0, sizeof(Info.aszGoal));
				for (size_t k = 0; k < pGoals->a.size(); k ++) {
					const JVal &g = pGoals->a[k];
					if (g.t != JVal::STR || !CheckText(g.s, MAX_QUESTGOAL, false, strErr, "goals[" + JsonNum((long long)k) + "]")) {
						if (strErr.empty()) strErr = "goals[" + JsonNum((long long)k) + "] は文字列で指定してください";
						goto Exit;
					}
					memcpy(Info.aszGoal[k], g.s.data(), g.s.size());
				}
			}
			/* 見えるNPC（丸ごと置き換え） */
			if (const JVal *pNpcs = chg.Get("npcs")) {
				if (pNpcs->t != JVal::ARR || pNpcs->a.size() > (size_t)MAX_QUESTNPC) {
					strErr = "npcs はオブジェクトの配列で、" + JsonNum(MAX_QUESTNPC) + " 個までです";
					goto Exit;
				}
				for (int k = 0; k < MAX_QUESTNPC; k ++) {
					memset(&Info.aNpc[k], 0, sizeof(Info.aNpc[k]));
					Info.aNpc[k].byStepMax = QUESTNPC_NOSTEP;
					Info.aNpc[k].byNextStep = QUESTNPC_NOSTEP;
				}
				for (size_t k = 0; k < pNpcs->a.size(); k ++) {
					const JVal &n = pNpcs->a[k];
					std::string strName = "npcs[" + JsonNum((long long)k) + "].";
					if (n.t != JVal::OBJ) {
						strErr = "npcs の要素はオブジェクトで指定してください";
						goto Exit;
					}
					if (!CheckKnownKeys(n, s_aFieldQuestNpc, COUNTOF(s_aFieldQuestNpc), NULL, strErr) ||
							!ApplyFields((BYTE *)&Info.aNpc[k], s_aFieldQuestNpc, COUNTOF(s_aFieldQuestNpc), n, strErr, strName)) {
						goto Exit;
					}
				}
			}
			/* 内容の確認 */
			if (Info.byState > QUESTSTATE_ON) {
				strErr = "byState は 0（止める）か 1（出す）で指定してください";
				goto Exit;
			}
			for (int k = 0; k < MAX_QUESTDONESET; k ++) {
				if (Info.awDoneSetFlag[k] >= MAX_FLAGBIT && Info.awDoneSetFlag[k] != FLAG_NOBIT) {
					strErr = "awDoneSetFlag[" + JsonNum(k) + "] は印の番号（0〜" + JsonNum(MAX_FLAGBIT - 1) + "）か 65535（無し）で指定してください";
					goto Exit;
				}
			}
			if ((Info.wNeedFlag >= MAX_FLAGBIT && Info.wNeedFlag != FLAG_NOBIT) || (Info.wNotFlag >= MAX_FLAGBIT && Info.wNotFlag != FLAG_NOBIT)) {
				strErr = "wNeedFlag・wNotFlag は印の番号（0〜" + JsonNum(MAX_FLAGBIT - 1) + "）か 65535（条件なし）で指定してください";
				goto Exit;
			}
			if (Info.byLevelMin > MAX_QUESTLEVELMIN) {
				strErr = "byLevelMin（最低レベル）は 0〜" + JsonNum(MAX_QUESTLEVELMIN) + " で指定してください（0=制限なし）";
				goto Exit;
			}
			if (Info.wCancelStory != 0) {
				bool bFound = false;
				for (int k = 0; k < g_aStoryInfo.GetSize(); k ++) {
					if (g_aStoryInfo[k]->wID == Info.wCancelStory) {
						bFound = true;
						break;
					}
				}
				if (Info.wCancelStory >= MAX_STORY || !bFound) {
					strErr = "wCancelStory（取り消しで戻すお話）は 0（何もしない）か、思い出の本のお話の wID（1〜" + JsonNum(MAX_STORY - 1) + "）で指定してください";
					goto Exit;
				}
			}
			if (Info.byDoneNumNo >= MAX_FLAGNUM && Info.byDoneNumNo != FLAG_NONUM) {
				strErr = "byDoneNumNo は数値の番号（0〜" + JsonNum(MAX_FLAGNUM - 1) + "）か 255（無し）で指定してください";
				goto Exit;
			}
			if (Info.byDoneNumOp >= TALKNUMOP_MAX) {
				strErr = "byDoneNumOp は 0（○にする）か 1（○足す）で指定してください";
				goto Exit;
			}
			if (Info.szName[0] == 0) {
				strErr = "szName（名前）は空にできません";
				goto Exit;
			}
			if (Info.byStepCount < 1 || Info.byStepCount > MAX_QUESTSTEP) {
				strErr = "byStepCount（段の数）は 1〜" + JsonNum(MAX_QUESTSTEP) + " で指定してください";
				goto Exit;
			}
			if ((Info.wRewardItem != 0 && Info.wRewardItem >= g_aItemInfo.GetSize()) ||
					(Info.wQuestItem != 0 && Info.wQuestItem >= g_aItemInfo.GetSize())) {
				strErr = "wRewardItem・wQuestItem は item の番号（0 はなし）で指定してください";
				goto Exit;
			}
			for (int k = 0; k < MAX_QUESTNPC; k ++) {
				PQUESTNPCINFO pN = &Info.aNpc[k];
				std::string strName = "npcs[" + JsonNum(k) + "].";
				if (pN->byKind == QUESTNPCKIND_NONE) {
					continue;
				}
				if (pN->byKind >= QUESTNPCKIND_MAX) {
					strErr = strName + "byKind は 1（ポータル）・2（宝箱）・3（人）・4（討伐チャレンジの入口）・5（討伐チャレンジの敵）で指定してください";
					goto Exit;
				}
				int nMapNo = GetMapNo(pN->dwMapID);
				if (nMapNo < 0) {
					strErr = strName + "dwMapID はマップ情報の dwID（マップID）で指定してください";
					goto Exit;
				}
				if ((int)pN->byX >= g_aMapInfo[nMapNo]->nCX || (int)pN->byY >= g_aMapInfo[nMapNo]->nCY) {
					strErr = strName + "byX・byY がマップの外です";
					goto Exit;
				}
				if (pN->byDir > 3) {
					strErr = strName + "byDir は 0〜3（上・下・左・右）で指定してください";
					goto Exit;
				}
				if (pN->byStepMin >= MAX_QUESTSTEP || (pN->byStepMax != QUESTNPC_NOSTEP && pN->byStepMax >= MAX_QUESTSTEP) ||
						(pN->byNextStep != QUESTNPC_NOSTEP && pN->byNextStep >= MAX_QUESTSTEP)) {
					strErr = strName + "byStepMin・byStepMax・byNextStep の段は 0〜" + JsonNum(MAX_QUESTSTEP - 1) + "（byStepMax・byNextStep は 255 も可）で指定してください";
					goto Exit;
				}
				if (pN->byFlgNeed > 16 || pN->byFlgHide > 16 || pN->bySetFlg > 16) {
					strErr = strName + "byFlgNeed・byFlgHide・bySetFlg は 0〜16 で指定してください";
					goto Exit;
				}
				if ((pN->byKind == QUESTNPCKIND_PORTAL) || (pN->byKind == QUESTNPCKIND_GATE)) {
					int nDest = GetMapNo(pN->dwDestMapID);
					if (nDest < 0 || (int)pN->byDestX >= g_aMapInfo[nDest]->nCX || (int)pN->byDestY >= g_aMapInfo[nDest]->nCY) {
						strErr = strName + "ポータルの dwDestMapID（マップID）・byDestX・byDestY が正しくありません";
						goto Exit;
					}
				}
				if (pN->wItem != 0 && pN->wItem >= g_aItemInfo.GetSize()) {
					strErr = strName + "wItem は item の番号（0 はなし）で指定してください";
					goto Exit;
				}
				if ((pN->byKind == QUESTNPCKIND_ENEMY) && (pN->wStTmp >= g_aStTmpInfo.GetSize())) {
					strErr = strName + "wStTmp（敵の強さ）は stTmp の id で指定してください";
					goto Exit;
				}
				if (((pN->byKind == QUESTNPCKIND_GATE) || (pN->byKind == QUESTNPCKIND_ENEMY)) && (Info.byChallenge == QUESTCHALLENGE_NONE)) {
					strErr = strName + "入口（4）・敵（5）は討伐チャレンジ（byChallenge 1・2）の依頼にだけ置けます";
					goto Exit;
				}
			}
			if (Info.byChallenge >= QUESTCHALLENGE_MAX) {
				strErr = "byChallenge は 0（ふつう）・1（異空間の討伐チャレンジ）・2（闘技場の討伐チャレンジ）で指定してください";
				goto Exit;
			}
			if (Info.byChallenge != QUESTCHALLENGE_NONE) {
				if (Info.wLoanStTmp >= g_aStTmpInfo.GetSize()) {
					strErr = "wLoanStTmp（貸し出しセット）は stTmp の id で指定してください";
					goto Exit;
				}
				if (Info.byMemberMin > MAX_PARTYMEMBER) {
					strErr = "byMemberMin（必要な人数）は 0〜" + JsonNum(MAX_PARTYMEMBER) + " で指定してください";
					goto Exit;
				}
			}
			/* 依頼番号を振る（1〜MAX_QUEST-1 で使われていない一番小さい番号） */
			if (bAdd) {
				WORD wNew = 0;
				for (WORD w = 1; w < MAX_QUEST && wNew == 0; w ++) {
					bool bUsed = false;
					for (int k = 0; k < g_aQuestInfo.GetSize(); k ++) {
						if (g_aQuestInfo[k]->wID == w) {
							bUsed = true;
							break;
						}
					}
					if (!bUsed) {
						wNew = w;
					}
				}
				if (wNew == 0) {
					strErr = "依頼がいっぱいで足せません";
					goto Exit;
				}
				Info.wID = wNew;
			}
			strRecord = QuestJson(&Info);
			if (bApply) {
				if (bAdd) {
					PQUESTINFO pNew = new QUESTINFO;
					*pNew = Info;
					g_aQuestInfo.Add(pNew);
				} else {
					*g_aQuestInfo[nID] = Info;
				}
				SaveQuestInfo();
			}
		}
		break;

	case KIND_FLAG:
		{
			FLAGNAMEINFO Info;
			memset(&Info, 0, sizeof(Info));
			if (!bAdd) {
				Info = *g_aFlagNameInfo[nID];
			} else {
				Info.byState = FLAGNAMESTATE_ON;
			}
			/* group・sub は計算して出す読むだけの項目。今の値と同じものを書き戻すのは許す（取得した記録をそのまま渡せるように） */
			{
				static const char *apszFlagExtra[] = { "group", "sub", NULL };
				JVal chg2 = chg;
				if (!CheckKnownKeys(chg, s_aFieldFlagName, COUNTOF(s_aFieldFlagName), apszFlagExtra, strErr)) {
					goto Exit;
				}
				for (size_t k = 0; k < chg2.o.size(); ) {
					const std::string &strKey = chg2.o[k].first;
					if (strKey == "group" || strKey == "sub") {
						const FLAGGROUPINFO *pGrp = bAdd ? NULL : FlagGroupOf(Info.byKind, Info.wNo);
						const JVal &v = chg2.o[k].second;
						bool bSame = !bAdd && ((strKey == "group") ?
								(v.t == JVal::STR && v.s == (pGrp ? pGrp->szName : "")) :
								(v.t == JVal::NUM && v.n == (pGrp ? FlagGroupSub(pGrp, Info.wNo) : 0)));
						if (!bSame) {
							strErr = strKey + " は計算して出す読むだけの項目です（変えられません。番号の帯はグループ flagGroup で決めます）";
							goto Exit;
						}
						chg2.o.erase(chg2.o.begin() + k);
					} else {
						k ++;
					}
				}
				if (!ApplyFields((BYTE *)&Info, s_aFieldFlagName, COUNTOF(s_aFieldFlagName), chg2, strErr, "")) {
					goto Exit;
				}
			}
			if (Info.byState > FLAGNAMESTATE_ON) {
				strErr = "byState は 0（削除済み）か 1（有効）で指定してください";
				goto Exit;
			}
			if (Info.byKind >= FLAGKIND_MAX) {
				strErr = "byKind は 0（印）か 1（数値）で指定してください";
				goto Exit;
			}
			if (Info.wNo >= ((Info.byKind == FLAGKIND_BIT) ? MAX_FLAGBIT : MAX_FLAGNUM)) {
				strErr = "wNo は、印なら 0〜" + JsonNum(MAX_FLAGBIT - 1) + "、数値なら 0〜" + JsonNum(MAX_FLAGNUM - 1) + " で指定してください";
				goto Exit;
			}
			if (Info.byState == FLAGNAMESTATE_ON && Info.szName[0] == 0) {
				strErr = "szName（名前）は空にできません（消す時は byState を 0 にします）";
				goto Exit;
			}
			/* 同じ種類・番号の組は 1 件まで（削除済みの枠も数える。足し直す時はその枠を直す） */
			for (int k = 0; k < g_aFlagNameInfo.GetSize(); k ++) {
				if ((bAdd || k != nID) && g_aFlagNameInfo[k]->byKind == Info.byKind && g_aFlagNameInfo[k]->wNo == Info.wNo) {
					strErr = std::string("同じ種類・番号の名前がもうあります（id ") + JsonNum(k) + "。" +
							(g_aFlagNameInfo[k]->byState == FLAGNAMESTATE_NONE ? "削除済みの枠なので、足さずにその id の byState を 1 に戻して名前を付けてください）" : "足さずにその id を更新してください）");
					goto Exit;
				}
			}
			strRecord = FieldsJson((const BYTE *)&Info, s_aFieldFlagName, COUNTOF(s_aFieldFlagName)) + FlagGroupExtraJson(Info.byKind, Info.wNo);
			if (bApply) {
				if (bAdd) {
					PFLAGNAMEINFO pNew = new FLAGNAMEINFO;
					*pNew = Info;
					g_aFlagNameInfo.Add(pNew);
				} else {
					*g_aFlagNameInfo[nID] = Info;
				}
				SaveFlagNameInfo();
			}
		}
		break;

	case KIND_TALK:
		{
			static const char *apszTalkExtra[] = { "lines", NULL };
			TALKINFO Info;
			WORD wIDOld;
			memset(&Info, 0, sizeof(Info));
			if (!bAdd) {
				Info = *g_aTalkInfo[nID];
			} else {
				Info.byState = TALKSTATE_ON;
				for (int k = 0; k < MAX_TALKLINE; k ++) {
					TalkLineInit(&Info.aLine[k]);
				}
			}
			wIDOld = Info.wID;
			if (!CheckKnownKeys(chg, s_aFieldTalk, COUNTOF(s_aFieldTalk), apszTalkExtra, strErr) ||
					!ApplyFields((BYTE *)&Info, s_aFieldTalk, COUNTOF(s_aFieldTalk), chg, strErr, "")) {
				goto Exit;
			}
			/* 会話セット番号は足す時に決め、あとから変えられない（同じ値を書き戻すのは許す） */
			if (!bAdd && Info.wID != wIDOld) {
				strErr = "wID は変えられません（足す時に決めます）";
				goto Exit;
			}
			if (bAdd) {
				if (chg.Get("wID") == NULL) {
					strErr = "足す時は wID（会話セット番号 1〜" + JsonNum(MAX_TALK - 1) + "）を指定してください";
					goto Exit;
				}
				if (Info.wID < 1 || Info.wID >= MAX_TALK) {
					strErr = "wID（会話セット番号）は 1〜" + JsonNum(MAX_TALK - 1) + " で指定してください";
					goto Exit;
				}
				for (int k = 0; k < g_aTalkInfo.GetSize(); k ++) {
					if (g_aTalkInfo[k]->wID == Info.wID) {
						strErr = "wID " + JsonNum(Info.wID) + " の会話セットがもうあります（id " + JsonNum(k) + "。" +
								(g_aTalkInfo[k]->byState == TALKSTATE_NONE ? "削除済みの枠なので、足さずにその id の byState を 1 に戻してください）" : "足さずにその id を更新してください）");
						goto Exit;
					}
				}
			}
			/* 行（丸ごと置き換え） */
			if (const JVal *pLines = chg.Get("lines")) {
				if (pLines->t != JVal::ARR || pLines->a.size() > (size_t)MAX_TALKLINE) {
					strErr = "lines はオブジェクトの配列で、" + JsonNum(MAX_TALKLINE) + " 行までです";
					goto Exit;
				}
				for (int k = 0; k < MAX_TALKLINE; k ++) {
					TalkLineInit(&Info.aLine[k]);
				}
				for (size_t k = 0; k < pLines->a.size(); k ++) {
					const JVal &l = pLines->a[k];
					std::string strName = "lines[" + JsonNum((long long)k) + "].";
					if (l.t != JVal::OBJ) {
						strErr = "lines の要素はオブジェクトで指定してください";
						goto Exit;
					}
					if (!CheckKnownKeys(l, s_aFieldTalkLine, COUNTOF(s_aFieldTalkLine), NULL, strErr) ||
							!ApplyFields((BYTE *)&Info.aLine[k], s_aFieldTalkLine, COUNTOF(s_aFieldTalkLine), l, strErr, strName)) {
						goto Exit;
					}
				}
				Info.byLineCount = (BYTE)pLines->a.size();
			}
			/* 内容の確認 */
			if (Info.byState > TALKSTATE_ON) {
				strErr = "byState は 0（削除済み）か 1（有効）で指定してください";
				goto Exit;
			}
			for (int k = 0; k < (int)Info.byLineCount && k < MAX_TALKLINE; k ++) {
				const TALKLINEINFO &L = Info.aLine[k];
				std::string strName = "lines[" + JsonNum(k) + "].";
				if (L.byLvMin != 0 && L.byLvMax != 0 && L.byLvMin > L.byLvMax) {
					strErr = strName + "byLvMin（レベル下限）が byLvMax（レベル上限）より大きいです";
					goto Exit;
				}
				if (L.wGiveItem != 0 && L.wGiveItem >= g_aItemInfo.GetSize()) {
					strErr = strName + "wGiveItem は item の番号（0 は渡さない）で指定してください";
					goto Exit;
				}
				if (L.wNeedItem != 0 && L.wNeedItem >= g_aItemInfo.GetSize()) {
					strErr = strName + "wNeedItem は item の番号（0 は条件なし）で指定してください";
					goto Exit;
				}
				if (L.wTakeItem != 0 && L.wTakeItem >= g_aItemInfo.GetSize()) {
					strErr = strName + "wTakeItem は item の番号（0 は取り上げない）で指定してください";
					goto Exit;
				}
				if (L.byAsk > 1) {
					strErr = strName + "byAsk は 0（今まで通り）か 1（はい・いいえで聞く）で指定してください";
					goto Exit;
				}
				if (L.wQuestID != 0 && L.byQuestStepMax != QUESTNPC_NOSTEP && L.byQuestStepMin > L.byQuestStepMax) {
					strErr = strName + "byQuestStepMin（依頼の段の下限）が byQuestStepMax（上限）より大きいです";
					goto Exit;
				}
				for (int j = 0; j < MAX_TALKNEED; j ++) {
					if ((L.awNeed[j] >= MAX_FLAGBIT && L.awNeed[j] != FLAG_NOBIT) || (L.awNot[j] >= MAX_FLAGBIT && L.awNot[j] != FLAG_NOBIT)) {
						strErr = strName + "awNeed・awNot は印の番号（0〜" + JsonNum(MAX_FLAGBIT - 1) + "）か 65535（無し）で指定してください";
						goto Exit;
					}
				}
				for (int j = 0; j < MAX_TALKSET; j ++) {
					if ((L.awSetOn[j] >= MAX_FLAGBIT && L.awSetOn[j] != FLAG_NOBIT) || (L.awSetOff[j] >= MAX_FLAGBIT && L.awSetOff[j] != FLAG_NOBIT)) {
						strErr = strName + "awSetOn・awSetOff は印の番号（0〜" + JsonNum(MAX_FLAGBIT - 1) + "）か 65535（無し）で指定してください";
						goto Exit;
					}
				}
				for (int j = 0; j < MAX_TALKCOND; j ++) {
					if (L.abyCondNo[j] >= MAX_FLAGNUM && L.abyCondNo[j] != FLAG_NONUM) {
						strErr = strName + "abyCondNo は数値の番号（0〜" + JsonNum(MAX_FLAGNUM - 1) + "）か 255（無し）で指定してください";
						goto Exit;
					}
					if (L.abyCondCmp[j] >= TALKCMP_MAX) {
						strErr = strName + "abyCondCmp は 0（以上）・1（ちょうど）・2（以下）のどれかで指定してください";
						goto Exit;
					}
				}
				for (int j = 0; j < MAX_TALKACT; j ++) {
					if (L.abyActNo[j] >= MAX_FLAGNUM && L.abyActNo[j] != FLAG_NONUM) {
						strErr = strName + "abyActNo は数値の番号（0〜" + JsonNum(MAX_FLAGNUM - 1) + "）か 255（無し）で指定してください";
						goto Exit;
					}
					if (L.abyActOp[j] >= TALKNUMOP_MAX) {
						strErr = strName + "abyActOp は 0（○にする）か 1（○足す）で指定してください";
						goto Exit;
					}
				}
			}
			strRecord = TalkJson(&Info);
			if (bApply) {
				if (bAdd) {
					PTALKINFO pNew = new TALKINFO;
					*pNew = Info;
					g_aTalkInfo.Add(pNew);
				} else {
					*g_aTalkInfo[nID] = Info;
				}
				SaveTalkInfo();
			}
		}
		break;

	case KIND_STORY:
		{
			static const char *apszStoryExtra[] = { "goals", NULL };
			STORYINFO Info;
			memset(&Info, 0, sizeof(Info));
			for (int k = 0; k < MAX_STORYGOAL; k ++) {
				StoryGoalInit(&Info.aGoal[k]);
			}
			if (!bAdd) {
				Info = *g_aStoryInfo[nID];
			} else {
				Info.byState = STORYSTATE_ON;
				/* 印・数値は「無し」から（0 のままだと印 0・数値 0 になる） */
				Info.wClearFlag = FLAG_NOBIT;
				for (int k = 0; k < MAX_STORYFLAGRANGE; k ++) {
					Info.awFlagFrom[k] = FLAG_NOBIT;
					Info.awFlagTo[k] = FLAG_NOBIT;
				}
				for (int k = 0; k < MAX_STORYNUM; k ++) {
					Info.abyNum[k] = FLAG_NONUM;
				}
				Info.byCountNum = FLAG_NONUM;
				/* 宝箱の番号は 0 も使えるので、「無し」から（0 のままだと宝箱 0 を戻してしまう） */
				for (int k = 0; k < MAX_STORYCHEST; k ++) {
					Info.awTakeChest[k] = FLAG_NOBIT;
				}
			}
			if (!CheckKnownKeys(chg, s_aFieldStory, COUNTOF(s_aFieldStory), apszStoryExtra, strErr)) {
				goto Exit;
			}
			/* wID は読むだけ（自動で振る）。同じ値を書き戻すのは許す（取得した記録をそのまま渡せるように） */
			{
				JVal chg2 = chg;
				for (size_t k = 0; k < chg2.o.size(); k ++) {
					if (chg2.o[k].first == "wID") {
						if (!(chg2.o[k].second.t == JVal::NUM && !bAdd && chg2.o[k].second.n == Info.wID)) {
							strErr = "wID は変えられません（足す時に自動で振られます）";
							goto Exit;
						}
						chg2.o.erase(chg2.o.begin() + k);
						break;
					}
				}
				if (!ApplyFields((BYTE *)&Info, s_aFieldStory, COUNTOF(s_aFieldStory), chg2, strErr, "")) {
					goto Exit;
				}
			}
			/* いまの目的の行（丸ごと置き換え。省くと今のまま） */
			if (const JVal *pGoals = chg.Get("goals")) {
				if (pGoals->t != JVal::ARR || pGoals->a.size() > (size_t)MAX_STORYGOAL) {
					strErr = "goals はオブジェクトの配列で、" + JsonNum(MAX_STORYGOAL) + " 行までです";
					goto Exit;
				}
				for (int k = 0; k < MAX_STORYGOAL; k ++) {
					StoryGoalInit(&Info.aGoal[k]);
				}
				for (size_t k = 0; k < pGoals->a.size(); k ++) {
					const JVal &l = pGoals->a[k];
					std::string strName = "goals[" + JsonNum((long long)k) + "].";
					if (l.t != JVal::OBJ) {
						strErr = "goals の要素はオブジェクトで指定してください";
						goto Exit;
					}
					if (!CheckKnownKeys(l, s_aFieldStoryGoal, COUNTOF(s_aFieldStoryGoal), NULL, strErr) ||
							!ApplyFields((BYTE *)&Info.aGoal[k], s_aFieldStoryGoal, COUNTOF(s_aFieldStoryGoal), l, strErr, strName)) {
						goto Exit;
					}
				}
			}
			/* 内容の確認 */
			if (Info.byState > STORYSTATE_ON) {
				strErr = "byState は 0（止める）か 1（出す）で指定してください";
				goto Exit;
			}
			if (Info.szName[0] == 0) {
				strErr = "szName（名前）は空にできません";
				goto Exit;
			}
			if (Info.wClearFlag >= MAX_FLAGBIT) {
				strErr = "wClearFlag（クリアした印）は印の番号（0〜" + JsonNum(MAX_FLAGBIT - 1) + "）で指定してください";
				goto Exit;
			}
			for (int k = 0; k < MAX_STORYFLAGRANGE; k ++) {
				bool bFrom = (Info.awFlagFrom[k] != FLAG_NOBIT), bTo = (Info.awFlagTo[k] != FLAG_NOBIT);
				if ((bFrom && Info.awFlagFrom[k] >= MAX_FLAGBIT) || (bTo && Info.awFlagTo[k] >= MAX_FLAGBIT)) {
					strErr = "awFlagFrom・awFlagTo は印の番号（0〜" + JsonNum(MAX_FLAGBIT - 1) + "）か 65535（無し）で指定してください";
					goto Exit;
				}
				if (bFrom != bTo) {
					strErr = "awFlagFrom[" + JsonNum(k) + "] と awFlagTo[" + JsonNum(k) + "] は、両方入れるか両方 65535（無し）にしてください";
					goto Exit;
				}
				if (bFrom && Info.awFlagFrom[k] > Info.awFlagTo[k]) {
					strErr = "awFlagFrom[" + JsonNum(k) + "]（範囲の先頭）が awFlagTo[" + JsonNum(k) + "]（範囲の最後）より大きいです";
					goto Exit;
				}
			}
			for (int k = 0; k < MAX_STORYNUM; k ++) {
				if (Info.abyNum[k] >= MAX_FLAGNUM && Info.abyNum[k] != FLAG_NONUM) {
					strErr = "abyNum は数値の番号（0〜" + JsonNum(MAX_FLAGNUM - 1) + "）か 255（無し）で指定してください";
					goto Exit;
				}
			}
			if (Info.byCountNum >= MAX_FLAGNUM && Info.byCountNum != FLAG_NONUM) {
				strErr = "byCountNum は数値の番号（0〜" + JsonNum(MAX_FLAGNUM - 1) + "）か 255（数えない）で指定してください";
				goto Exit;
			}
			for (int k = 0; k < MAX_STORYGOAL; k ++) {
				const STORYGOAL &G = Info.aGoal[k];
				std::string strName = "goals[" + JsonNum(k) + "].";
				for (int j = 0; j < MAX_TALKNEED; j ++) {
					if ((G.awNeed[j] >= MAX_FLAGBIT && G.awNeed[j] != FLAG_NOBIT) || (G.awNot[j] >= MAX_FLAGBIT && G.awNot[j] != FLAG_NOBIT)) {
						strErr = strName + "awNeed・awNot は印の番号（0〜" + JsonNum(MAX_FLAGBIT - 1) + "）か 65535（無し）で指定してください";
						goto Exit;
					}
				}
				for (int j = 0; j < MAX_TALKCOND; j ++) {
					if (G.abyCondNo[j] >= MAX_FLAGNUM && G.abyCondNo[j] != FLAG_NONUM) {
						strErr = strName + "abyCondNo は数値の番号（0〜" + JsonNum(MAX_FLAGNUM - 1) + "）か 255（無し）で指定してください";
						goto Exit;
					}
					if (G.abyCondCmp[j] >= TALKCMP_MAX) {
						strErr = strName + "abyCondCmp は 0（以上）・1（ちょうど）・2（以下）のどれかで指定してください";
						goto Exit;
					}
				}
			}
			for (int k = 0; k < MAX_STORYQUEST; k ++) {
				if (Info.awQuest[k] == 0) {
					continue;
				}
				bool bFound = false;
				for (int m = 0; m < g_aQuestInfo.GetSize(); m ++) {
					if (g_aQuestInfo[m]->wID == Info.awQuest[k]) {
						bFound = true;
						break;
					}
				}
				if (!bFound) {
					strErr = "awQuest[" + JsonNum(k) + "] の依頼番号 " + JsonNum(Info.awQuest[k]) + " の依頼（quest の wID）がありません（0 は無し）";
					goto Exit;
				}
			}
			for (int k = 0; k < MAX_STORYTAKEITEM; k ++) {
				if (Info.awTakeItem[k] != 0 && Info.awTakeItem[k] >= g_aItemInfo.GetSize()) {
					strErr = "awTakeItem[" + JsonNum(k) + "] は item の番号（1〜" + JsonNum(g_aItemInfo.GetSize() - 1) + "）で指定してください（0 は無し）";
					goto Exit;
				}
			}
			for (int k = 0; k < MAX_STORYCHEST; k ++) {
				if (Info.awTakeChest[k] != FLAG_NOBIT && Info.awTakeChest[k] >= MAX_CHESTNO) {
					strErr = "awTakeChest[" + JsonNum(k) + "] は宝箱の番号（0〜" + JsonNum(MAX_CHESTNO - 1) + "）か 65535（無し）で指定してください";
					goto Exit;
				}
			}
			/* 前のお話は、あるシナリオで、自分や自分につながる後のお話ではないこと（ループしない） */
			if (Info.wPrevStory != 0) {
				WORD w = Info.wPrevStory;
				for (int n = 0; n <= MAX_STORY; n ++) {
					PSTORYINFO pPrev = NULL;
					for (int m = 0; m < g_aStoryInfo.GetSize(); m ++) {
						if (g_aStoryInfo[m]->wID == w) {
							pPrev = g_aStoryInfo[m];
							break;
						}
					}
					if (pPrev == NULL) {
						strErr = "wPrevStory の " + JsonNum(w) + " のシナリオ（wID）がありません（0 は無し）";
						goto Exit;
					}
					if (!bAdd && w == Info.wID) {
						strErr = "wPrevStory が自分自身か、自分につながる後のお話を指しています（つながりがループします）";
						goto Exit;
					}
					w = pPrev->wPrevStory;
					if (w == 0) {
						break;
					}
				}
			}
			/* シナリオ番号を振る（1〜MAX_STORY-1 で使われていない一番小さい番号） */
			if (bAdd) {
				WORD wNew = 0;
				for (WORD w = 1; w < MAX_STORY && wNew == 0; w ++) {
					bool bUsed = false;
					for (int k = 0; k < g_aStoryInfo.GetSize(); k ++) {
						if (g_aStoryInfo[k]->wID == w) {
							bUsed = true;
							break;
						}
					}
					if (!bUsed) {
						wNew = w;
					}
				}
				if (wNew == 0) {
					strErr = "シナリオがいっぱいで足せません";
					goto Exit;
				}
				Info.wID = wNew;
			}
			strRecord = StoryJson(&Info);
			if (bApply) {
				if (bAdd) {
					PSTORYINFO pNew = new STORYINFO;
					*pNew = Info;
					g_aStoryInfo.Add(pNew);
				} else {
					*g_aStoryInfo[nID] = Info;
				}
				SaveStoryInfo();
			}
		}
		break;

	case KIND_FLAGGROUP:
		{
			static const char *apszGroupExtra[] = { "bands", "wFrom", "wTo", NULL };
			FLAGGROUPINFO Info;
			memset(&Info, 0, sizeof(Info));
			FlagGroupClearBands(&Info);
			if (!bAdd) {
				Info = *g_aFlagGroupInfo[nID];
			} else {
				Info.byState = FLAGGROUPSTATE_ON;
			}
			if (!CheckKnownKeys(chg, s_aFieldFlagGroup, COUNTOF(s_aFieldFlagGroup), apszGroupExtra, strErr)) {
				goto Exit;
			}
			/* wID は読むだけ（自動で振る）。同じ値を書き戻すのは許す（取得した記録をそのまま渡せるように） */
			{
				JVal chg2 = chg;
				const JVal *pBands = chg.Get("bands");
				const JVal *pFrom = chg.Get("wFrom");
				const JVal *pTo = chg.Get("wTo");

				for (size_t k = 0; k < chg2.o.size(); k ++) {
					if (chg2.o[k].first == "wID") {
						if (!(chg2.o[k].second.t == JVal::NUM && !bAdd && chg2.o[k].second.n == Info.wID)) {
							strErr = "wID は変えられません（足す時に自動で振られます）";
							goto Exit;
						}
						chg2.o.erase(chg2.o.begin() + k);
						break;
					}
				}
				if (!ApplyFields((BYTE *)&Info, s_aFieldFlagGroup, COUNTOF(s_aFieldFlagGroup), chg2, strErr, "")) {
					goto Exit;
				}
				/* 番号の帯（bands は丸ごと置き換え。wFrom・wTo だけなら帯 1 の互換の書き方） */
				if (bAdd && pBands == NULL && (pFrom == NULL || pTo == NULL)) {
					strErr = "足す時は bands（番号の帯の配列）か、wFrom・wTo（帯 1 本の先頭と最後）を指定してください";
					goto Exit;
				}
				if (pFrom != NULL && (pFrom->t != JVal::NUM || pFrom->n < 0 || pFrom->n > 65534 || pFrom->n != (double)(long long)pFrom->n)) {
					strErr = "wFrom は 0〜65534 の整数で指定してください";
					goto Exit;
				}
				if (pTo != NULL && (pTo->t != JVal::NUM || pTo->n < 0 || pTo->n > 65535 || pTo->n != (double)(long long)pTo->n)) {
					strErr = "wTo は 0〜65535 の整数で指定してください";
					goto Exit;
				}
				if (pBands != NULL) {
					if (pBands->t != JVal::ARR || pBands->a.size() < 1 || pBands->a.size() > (size_t)MAX_FLAGGROUPBAND) {
						strErr = "bands は {wFrom, wTo} の配列で、1〜" + JsonNum(MAX_FLAGGROUPBAND) + " 本で指定してください";
						goto Exit;
					}
					FlagGroupClearBands(&Info);
					for (size_t k = 0; k < pBands->a.size(); k ++) {
						const JVal &bd = pBands->a[k];
						std::string strName = "bands[" + JsonNum((long long)k) + "]";
						const JVal *pBF = (bd.t == JVal::OBJ) ? bd.Get("wFrom") : NULL;
						const JVal *pBT = (bd.t == JVal::OBJ) ? bd.Get("wTo") : NULL;
						if (bd.t != JVal::OBJ || pBF == NULL || pBT == NULL || bd.o.size() != 2 ||
								pBF->t != JVal::NUM || pBT->t != JVal::NUM ||
								pBF->n < 0 || pBF->n > 65534 || pBT->n < 0 || pBT->n > 65535 ||
								pBF->n != (double)(long long)pBF->n || pBT->n != (double)(long long)pBT->n) {
							strErr = strName + " は {\"wFrom\":先頭, \"wTo\":最後}（0〜65534 の整数）で指定してください";
							goto Exit;
						}
						Info.awFrom[k] = (WORD)pBF->n;
						Info.awTo[k] = (WORD)pBT->n;
					}
					if ((pFrom != NULL && (WORD)pFrom->n != Info.awFrom[0]) || (pTo != NULL && (WORD)pTo->n != Info.awTo[0])) {
						strErr = "bands と wFrom・wTo を一緒に渡す時は、wFrom・wTo を bands[0] と同じ値にしてください（どちらか一方だけで指定できます）";
						goto Exit;
					}
				} else {
					/* 互換: wFrom・wTo は帯 1 だけを変える（帯 2 以降は今のまま） */
					if (pFrom != NULL) {
						Info.awFrom[0] = (WORD)pFrom->n;
					}
					if (pTo != NULL) {
						Info.awTo[0] = (WORD)pTo->n;
					}
				}
			}
			/* 内容の確認 */
			if (Info.byState > FLAGGROUPSTATE_ON) {
				strErr = "byState は 0（削除済み）か 1（有効）で指定してください";
				goto Exit;
			}
			if (Info.byKind >= FLAGKIND_MAX) {
				strErr = "byKind は 0（印）か 1（数値）で指定してください";
				goto Exit;
			}
			if (Info.byState == FLAGGROUPSTATE_ON && Info.szName[0] == 0) {
				strErr = "szName（メイン名）は空にできません（消す時は byState を 0 にします）";
				goto Exit;
			}
			{
				WORD wMax = (Info.byKind == FLAGKIND_BIT) ? MAX_FLAGBIT : MAX_FLAGNUM;
				/* 帯は詰めて使う（途中が空なら前へ詰める。順番がサブの数え順なので順序は変えない） */
				{
					int nTo = 0;
					for (int k = 0; k < MAX_FLAGGROUPBAND; k ++) {
						if (FlagGroupBandUsed(&Info, k)) {
							Info.awFrom[nTo] = Info.awFrom[k];
							Info.awTo[nTo] = Info.awTo[k];
							nTo ++;
						}
					}
					for (int k = nTo; k < MAX_FLAGGROUPBAND; k ++) {
						Info.awFrom[k] = FLAGGROUPBAND_NONE;
						Info.awTo[k] = 0;
					}
					if (nTo == 0) {
						strErr = "番号の帯は 1 本以上必要です";
						goto Exit;
					}
				}
				for (int k = 0; k < MAX_FLAGGROUPBAND; k ++) {
					if (!FlagGroupBandUsed(&Info, k)) {
						continue;
					}
					if (Info.awFrom[k] >= wMax || Info.awTo[k] >= wMax || Info.awFrom[k] > Info.awTo[k]) {
						strErr = std::string(FlagGroupBandCount(&Info) > 1 ? "帯 " + JsonNum(k + 1) + " の " : "") + "wFrom〜wTo は、印なら 0〜" + JsonNum(MAX_FLAGBIT - 1) + "、数値なら 0〜" + JsonNum(MAX_FLAGNUM - 1) + " の範囲で、wFrom <= wTo にしてください";
						goto Exit;
					}
				}
			}
			/* 帯同士が重ならないこと（同じグループの中も）。同じ種類で、有効なグループのすべての帯とも重ならないこと（削除済みの枠の帯は数えない） */
			for (int k = 0; k < MAX_FLAGGROUPBAND; k ++) {
				for (int m = k + 1; m < MAX_FLAGGROUPBAND; m ++) {
					if (FlagGroupBandUsed(&Info, k) && FlagGroupBandUsed(&Info, m) &&
							Info.awFrom[k] <= Info.awTo[m] && Info.awFrom[m] <= Info.awTo[k]) {
						strErr = "帯 " + JsonNum(k + 1) + "（" + JsonNum(Info.awFrom[k]) + "〜" + JsonNum(Info.awTo[k]) + "）と帯 " + JsonNum(m + 1) + "（" +
								JsonNum(Info.awFrom[m]) + "〜" + JsonNum(Info.awTo[m]) + "）が重なっています（帯は重ねられません）";
						goto Exit;
					}
				}
			}
			if (Info.byState == FLAGGROUPSTATE_ON) {
				for (int k = 0; k < g_aFlagGroupInfo.GetSize(); k ++) {
					const FLAGGROUPINFO *pO = g_aFlagGroupInfo[k];
					if (!((bAdd || k != nID) && pO->byState == FLAGGROUPSTATE_ON && pO->byKind == Info.byKind)) {
						continue;
					}
					for (int b = 0; b < MAX_FLAGGROUPBAND; b ++) {
						for (int c = 0; c < MAX_FLAGGROUPBAND; c ++) {
							if (FlagGroupBandUsed(&Info, b) && FlagGroupBandUsed(pO, c) &&
									Info.awFrom[b] <= pO->awTo[c] && pO->awFrom[c] <= Info.awTo[b]) {
								strErr = "番号の帯 " + JsonNum(Info.awFrom[b]) + "〜" + JsonNum(Info.awTo[b]) + (FlagGroupBandCount(&Info) > 1 ? "（帯 " + JsonNum(b + 1) + "）" : "") + " が、グループ「" + pO->szName + "」（id " + JsonNum(k) +
										"・" + (FlagGroupBandCount(pO) > 1 ? "帯 " + JsonNum(c + 1) + " の " : "") + JsonNum(pO->awFrom[c]) + "〜" + JsonNum(pO->awTo[c]) + "）と重なっています（同じ種類で帯は重ねられません）";
								goto Exit;
							}
						}
					}
				}
			}
			/* グループ番号を振る（1〜MAX_FLAGGROUP-1 で使われていない一番小さい番号） */
			if (bAdd) {
				WORD wNew = 0;
				for (WORD w = 1; w < MAX_FLAGGROUP && wNew == 0; w ++) {
					bool bUsed = false;
					for (int k = 0; k < g_aFlagGroupInfo.GetSize(); k ++) {
						if (g_aFlagGroupInfo[k]->wID == w) {
							bUsed = true;
							break;
						}
					}
					if (!bUsed) {
						wNew = w;
					}
				}
				if (wNew == 0) {
					strErr = "グループがいっぱいで足せません";
					goto Exit;
				}
				Info.wID = wNew;
			}
			strRecord = FlagGroupJson(&Info);
			if (bApply) {
				if (bAdd) {
					PFLAGGROUPINFO pNew = new FLAGGROUPINFO;
					*pNew = Info;
					g_aFlagGroupInfo.Add(pNew);
				} else {
					*g_aFlagGroupInfo[nID] = Info;
				}
				SaveFlagGroupInfo();
			}
		}
		break;

	}

Exit:
	if (bUndo && strErr.empty()) {
		AdminUndoAfter();
	}
	if (!strErr.empty()) {
		s_strResult = "{\"ok\":false,\"error\":" + JsonStr(strErr.c_str()) + "}";
	} else {
		s_strResult = "{\"ok\":true,\"id\":" + JsonNum(nID) + ",\"added\":" + (bAdd ? "true" : "false") +
				",\"record\":{\"id\":" + JsonNum(nID) + "," + strRecord + "}}";
	}
	return s_strResult.c_str();
}

/*
 * 管理者ウィンドウ（ゲーム画面の旧管理者画面）から依頼を書き込む。
 *   nNo: 依頼の配列の番号（-1 なら足す）。届いた QUESTINFO を JSON にして
 *   SboWebDataApply を通すので、確認・番号の振り方・保存はデータAPIと同じ。
 *   結果: strErr が空ならうまくいった（nNoOut に書き込んだ番号）
 */
void SboDataQuestApplyInfo(int nNo, const QUESTINFO *pInfo, int &nNoOut, std::string &strErr)
{
	QUESTINFO Info;
	bool bAdd = (nNo < 0);

	nNoOut = -1;
	strErr.clear();
	if (!bAdd && nNo >= g_aQuestInfo.GetSize()) {
		strErr = "依頼の番号が範囲外です（ほかの人が変えたかもしれません。取得し直してください）";
		return;
	}
	Info = *pInfo;
	/* 文字の欄は必ず終端させる */
	Info.szName[sizeof(Info.szName) - 1] = 0;
	Info.szExplain[sizeof(Info.szExplain) - 1] = 0;
	Info.szMsgAccept[sizeof(Info.szMsgAccept) - 1] = 0;
	Info.szMsgDone[sizeof(Info.szMsgDone) - 1] = 0;
	Info.szIntro[sizeof(Info.szIntro) - 1] = 0;
	for (int i = 0; i < MAX_QUESTSTEP; i ++) {
		Info.aszGoal[i][MAX_QUESTGOAL - 1] = 0;
	}
	for (int i = 0; i < MAX_QUESTNPC; i ++) {
		Info.aNpc[i].szName[sizeof(Info.aNpc[i].szName) - 1] = 0;
		Info.aNpc[i].szSpeech[sizeof(Info.aNpc[i].szSpeech) - 1] = 0;
	}
	if (!bAdd) {
		/* 番号は変えられない（データAPIと同じく、今の番号を書き戻す形にする） */
		Info.wID = g_aQuestInfo[nNo]->wID;
	}
	std::string strJson = "{" + QuestJson(&Info, !bAdd) + "}";
	const char *pszRes = SboWebDataApply("quest", bAdd ? g_aQuestInfo.GetSize() : nNo, strJson.c_str(), 1, "管理者ウィンドウ");

	CJsonReader reader(pszRes ? pszRes : "");
	JVal res;
	if (!reader.Parse(res) || res.t != JVal::OBJ) {
		strErr = "書き込めませんでした";
		return;
	}
	const JVal *pOk = res.Get("ok");
	if (pOk == NULL || pOk->t != JVal::BOOL_ || !pOk->b) {
		const JVal *pErr = res.Get("error");
		strErr = (pErr && pErr->t == JVal::STR) ? pErr->s : std::string("書き込めませんでした");
		/* 「npcs[0].」は画面の言い方（見えるNPC 1 の）に直す */
		for (int i = 0; i < MAX_QUESTNPC; i ++) {
			std::string strFrom = "npcs[" + JsonNum(i) + "].";
			size_t nPos = strErr.find(strFrom);
			if (nPos != std::string::npos) {
				strErr.replace(nPos, strFrom.size(), "見えるNPC " + JsonNum(i + 1) + " の ");
			}
		}
		return;
	}
	const JVal *pID = res.Get("id");
	nNoOut = (pID && pID->t == JVal::NUM) ? (int)pID->n : (bAdd ? g_aQuestInfo.GetSize() - 1 : nNo);
}

/*
 * SboWebDataApply の結果（JSON）を、管理者ウィンドウ用の「理由の文」と書き込んだ番号にする
 *   うまくいった時は strErr が空になり、nNoOut に書き込んだ番号が入る
 */
static void ApplyResultToErr(const char *pszRes, int nNoDefault, int &nNoOut, std::string &strErr)
{
	CJsonReader reader(pszRes ? pszRes : "");
	JVal res;
	if (!reader.Parse(res) || res.t != JVal::OBJ) {
		strErr = "書き込めませんでした";
		return;
	}
	const JVal *pOk = res.Get("ok");
	if (pOk == NULL || pOk->t != JVal::BOOL_ || !pOk->b) {
		const JVal *pErr = res.Get("error");
		strErr = (pErr && pErr->t == JVal::STR) ? pErr->s : std::string("書き込めませんでした");
		return;
	}
	const JVal *pID = res.Get("id");
	nNoOut = (pID && pID->t == JVal::NUM) ? (int)pID->n : nNoDefault;
}

/*
 * 管理者ウィンドウからフラグの名前表を書き込む（依頼と同じ作り）。
 *   nNo: 名前表の配列の番号（-1 なら足す）。届いた FLAGNAMEINFO を JSON にして
 *   SboWebDataApply を通すので、確認（同じ種類・番号の重なり・範囲）・保存はデータAPIと同じ。
 *   結果: strErr が空ならうまくいった（nNoOut に書き込んだ番号）
 */
void SboDataFlagNameApplyInfo(int nNo, const FLAGNAMEINFO *pInfo, int &nNoOut, std::string &strErr)
{
	FLAGNAMEINFO Info;
	bool bAdd = (nNo < 0);

	nNoOut = -1;
	strErr.clear();
	if (!bAdd && nNo >= g_aFlagNameInfo.GetSize()) {
		strErr = "名前表の番号が範囲外です（ほかの人が変えたかもしれません。取得し直してください）";
		return;
	}
	Info = *pInfo;
	Info.szName[sizeof(Info.szName) - 1] = 0;
	std::string strJson = "{" + FieldsJson((const BYTE *)&Info, s_aFieldFlagName, COUNTOF(s_aFieldFlagName)) + "}";
	const char *pszRes = SboWebDataApply("flag", bAdd ? g_aFlagNameInfo.GetSize() : nNo, strJson.c_str(), 1, "管理者ウィンドウ");
	ApplyResultToErr(pszRes, bAdd ? g_aFlagNameInfo.GetSize() - 1 : nNo, nNoOut, strErr);
}

/*
 * 管理者ウィンドウから思い出の本のシナリオを書き込む（依頼と同じ作り）。
 *   nNo: シナリオの配列の番号（-1 なら足す）。届いた STORYINFO を JSON にして
 *   SboWebDataApply を通すので、確認（印の範囲・依頼の番号・前のお話のループ）・番号の振り方・保存はデータAPIと同じ。
 *   シナリオ番号（wID）は足す時に自動で振り、直す時は今の番号を書き戻す（変えられない）。
 *   結果: strErr が空ならうまくいった（nNoOut に書き込んだ番号）
 */
void SboDataStoryApplyInfo(int nNo, const STORYINFO *pInfo, int &nNoOut, std::string &strErr)
{
	STORYINFO Info;
	bool bAdd = (nNo < 0);

	nNoOut = -1;
	strErr.clear();
	if (!bAdd && nNo >= g_aStoryInfo.GetSize()) {
		strErr = "シナリオの番号が範囲外です（ほかの人が変えたかもしれません。取得し直してください）";
		return;
	}
	Info = *pInfo;
	/* 文字の欄は必ず終端させる */
	Info.szName[sizeof(Info.szName) - 1] = 0;
	Info.szExplain[sizeof(Info.szExplain) - 1] = 0;
	if (!bAdd) {
		Info.wID = g_aStoryInfo[nNo]->wID;
	}
	/* 「いまの目的」（goals）は管理者ウィンドウでは編集しないので JSON に含めない（含めないと今の値が残る。足す時は無し）。
	   届いた STORYINFO に目的が入っていても、それは使わない */
	std::string strJson = "{" + (bAdd ? FieldsJson((const BYTE *)&Info, s_aFieldStory + 1, COUNTOF(s_aFieldStory) - 1) :
			FieldsJson((const BYTE *)&Info, s_aFieldStory, COUNTOF(s_aFieldStory))) + "}";
	const char *pszRes = SboWebDataApply("story", bAdd ? g_aStoryInfo.GetSize() : nNo, strJson.c_str(), 1, "管理者ウィンドウ");
	ApplyResultToErr(pszRes, bAdd ? g_aStoryInfo.GetSize() - 1 : nNo, nNoOut, strErr);
}

/*
 * 管理者ウィンドウからフラグのグループを書き込む（思い出の本と同じ作り）。
 *   nNo: グループの配列の番号（-1 なら足す）。届いた FLAGGROUPINFO を JSON にして
 *   SboWebDataApply を通すので、確認（帯の範囲・同じ種類での重なり）・番号の振り方・保存はデータAPIと同じ。
 *   グループ番号（wID）は足す時に自動で振り、直す時は今の番号を書き戻す（変えられない）。
 *   結果: strErr が空ならうまくいった（nNoOut に書き込んだ番号）
 */
void SboDataFlagGroupApplyInfo(int nNo, const FLAGGROUPINFO *pInfo, int &nNoOut, std::string &strErr)
{
	FLAGGROUPINFO Info;
	bool bAdd = (nNo < 0);

	nNoOut = -1;
	strErr.clear();
	if (!bAdd && nNo >= g_aFlagGroupInfo.GetSize()) {
		strErr = "グループの番号が範囲外です（ほかの人が変えたかもしれません。取得し直してください）";
		return;
	}
	Info = *pInfo;
	/* 文字の欄は必ず終端させる */
	Info.szName[sizeof(Info.szName) - 1] = 0;
	if (!bAdd) {
		Info.wID = g_aFlagGroupInfo[nNo]->wID;
	}
	/* 帯は bands で渡す（wFrom・wTo は帯 1 の互換なので付けない） */
	std::string strJson = "{" + (bAdd ? FieldsJson((const BYTE *)&Info, s_aFieldFlagGroup + 1, COUNTOF(s_aFieldFlagGroup) - 1) :
			FieldsJson((const BYTE *)&Info, s_aFieldFlagGroup, COUNTOF(s_aFieldFlagGroup))) + ",\"bands\":[";
	{
		int nBand = 0;
		for (int i = 0; i < MAX_FLAGGROUPBAND; i ++) {
			if (FlagGroupBandUsed(&Info, i)) {
				strJson += std::string(nBand ? "," : "") + "{\"wFrom\":" + JsonNum(Info.awFrom[i]) + ",\"wTo\":" + JsonNum(Info.awTo[i]) + "}";
				nBand ++;
			}
		}
	}
	strJson += "]}";
	const char *pszRes = SboWebDataApply("flagGroup", bAdd ? g_aFlagGroupInfo.GetSize() : nNo, strJson.c_str(), 1, "管理者ウィンドウ");
	ApplyResultToErr(pszRes, bAdd ? g_aFlagGroupInfo.GetSize() - 1 : nNo, nNoOut, strErr);
}

/*
 * 管理者ウィンドウから会話データを書き込む（依頼と同じ作り）。
 *   nNo: 会話セットの配列の番号（-1 なら足す）。届いた TALKINFO を JSON にして
 *   SboWebDataApply を通すので、確認・保存はデータAPIと同じ。
 *   会話セット番号（wID）は足す時に決め、直す時は今の番号を書き戻す（変えられない）。
 *   結果: strErr が空ならうまくいった（nNoOut に書き込んだ番号）
 */
void SboDataTalkApplyInfo(int nNo, const TALKINFO *pInfo, int &nNoOut, std::string &strErr)
{
	TALKINFO Info;
	bool bAdd = (nNo < 0);

	nNoOut = -1;
	strErr.clear();
	if (!bAdd && nNo >= g_aTalkInfo.GetSize()) {
		strErr = "会話セットの番号が範囲外です（ほかの人が変えたかもしれません。取得し直してください）";
		return;
	}
	Info = *pInfo;
	/* 文字の欄は必ず終端させる。行の数は範囲に収める */
	Info.szName[sizeof(Info.szName) - 1] = 0;
	if (Info.byLineCount > MAX_TALKLINE) {
		Info.byLineCount = MAX_TALKLINE;
	}
	for (int i = 0; i < MAX_TALKLINE; i ++) {
		Info.aLine[i].szSpeech[sizeof(Info.aLine[i].szSpeech) - 1] = 0;
	}
	if (!bAdd) {
		Info.wID = g_aTalkInfo[nNo]->wID;
	}
	std::string strJson = "{" + TalkJson(&Info) + "}";
	const char *pszRes = SboWebDataApply("talk", bAdd ? g_aTalkInfo.GetSize() : nNo, strJson.c_str(), 1, "管理者ウィンドウ");
	ApplyResultToErr(pszRes, bAdd ? g_aTalkInfo.GetSize() - 1 : nNo, nNoOut, strErr);
	/* 「lines[0].」は画面の言い方（行 1 の）に直す */
	for (int i = 0; i < MAX_TALKLINE && !strErr.empty(); i ++) {
		std::string strFrom = "lines[" + JsonNum(i) + "].";
		size_t nPos = strErr.find(strFrom);
		if (nPos != std::string::npos) {
			strErr.replace(nPos, strFrom.size(), "行 " + JsonNum(i + 1) + " の ");
		}
	}
}

/*
 * マップのマス目（範囲を指定。1 回 128×128 マスまで）
 *   {"id":番号,"mapId":マップID,"cx":横,"cy":縦,"x":..,"y":..,"w":..,"h":..,
 *    "parts":[[パーツ番号,...],...],"flags":[[...]],"roof":[[...]]}（行ごと）
 */
extern "C" EMSCRIPTEN_KEEPALIVE const char *SboWebDataMapTiles(int nID, int x, int y, int w, int h)
{
	if (nID < 0 || nID >= g_aMapInfo.GetSize()) {
		s_strResult = "{\"error\":\"マップの番号が範囲外です\"}";
		return s_strResult.c_str();
	}
	PMAPINFO pMap = g_aMapInfo[nID];
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (w <= 0 || x + w > pMap->nCX) w = pMap->nCX - x;
	if (h <= 0 || y + h > pMap->nCY) h = pMap->nCY - y;
	if (w > 128) w = 128;
	if (h > 128) h = 128;
	if (w < 0) w = 0;
	if (h < 0) h = 0;

	std::string out = "{\"id\":" + JsonNum(nID) + ",\"mapId\":" + JsonNum(pMap->dwID) +
			",\"cx\":" + JsonNum(pMap->nCX) + ",\"cy\":" + JsonNum(pMap->nCY) +
			",\"x\":" + JsonNum(x) + ",\"y\":" + JsonNum(y) + ",\"w\":" + JsonNum(w) + ",\"h\":" + JsonNum(h);
	const char *apszName[] = { "parts", "flags", "roof" };
	for (int k = 0; k < 3; k ++) {
		out += std::string(",\"") + apszName[k] + "\":[";
		for (int yy = y; yy < y + h; yy ++) {
			if (yy > y) out += ",";
			out += "[";
			for (int xx = x; xx < x + w; xx ++) {
				int n = xx + yy * pMap->nCX;
				long long v = 0;
				if (k == 0 && pMap->pwMap)			v = pMap->pwMap[n];
				else if (k == 1 && pMap->pbyFlg)	v = pMap->pbyFlg[n];
				else if (k == 2 && pMap->pbyRoofLV)	v = pMap->pbyRoofLV[n];
				if (xx > x) out += ",";
				out += JsonNum(v);
			}
			out += "]";
		}
		out += "]";
	}
	out += "}";
	s_strResult = out;
	return s_strResult.c_str();
}

/* ========================================================================= */
/* NPCの巡回ルートを設定する（管理ページ用）								 */
/*   nTblID   : char の番号													 */
/*   nMode    : 0=折り返す 1=最初に戻る										 */
/*   nWait    : 各道しるべで止まる秒数(0〜60)								 */
/*   pszPoints: "x,y;x,y;..."（空なら巡回をやめる。1点だけは不可）			 */
/* 戻り値は {"ok":true,"char":{...}} か {"error":"..."}						 */
/* ========================================================================= */
static const char *RouteError(const char *pszMsg)
{
	s_strResult = std::string("{\"error\":") + JsonStr(pszMsg) + "}";
	return s_strResult.c_str();
}

extern "C" EMSCRIPTEN_KEEPALIVE const char *SboWebNpcRouteSet(int nTblID, int nMode, int nWait, const char *pszPoints)
{
	if (nTblID < 0 || nTblID >= g_aCharInfo.GetSize()) {
		return RouteError("キャラの番号が範囲外です");
	}
	PCHARINFO pChar = g_aCharInfo[nTblID];
	/* まず NPC の種類・回り方・秒数・マップを調べる（道しるべの検査より前のエラーを先に返す） */
	const char *pszError = NpcRouteCheck(pChar, nMode, nWait, 0, NULL, NULL);
	if (pszError) {
		return RouteError(pszError);
	}
	PMAPINFO pMap = g_aMapInfo[pChar->dwMapID];

	/* 道しるべを読む */
	std::vector<int> aX, aY;
	const char *p = pszPoints ? pszPoints : "";
	while (*p) {
		char *pEnd;
		long x = strtol(p, &pEnd, 10);
		if (pEnd == p || *pEnd != ',') {
			return RouteError("道しるべの書き方が不正です");
		}
		p = pEnd + 1;
		long y = strtol(p, &pEnd, 10);
		if (pEnd == p) {
			return RouteError("道しるべの書き方が不正です");
		}
		p = pEnd;
		if (x < 0 || x >= pMap->nCX || y < 0 || y >= pMap->nCY || x > 255 || y > 255) {
			return RouteError("道しるべがマップの外です");
		}
		aX.push_back((int)x);
		aY.push_back((int)y);
		if (*p == ';') {
			p ++;
		} else if (*p) {
			return RouteError("道しるべの書き方が不正です");
		}
	}

	/* 検査（管理者ウィンドウからの保存と同じ決まり） */
	const int anDummy[1] = { 0 };
	pszError = NpcRouteCheck(pChar, nMode, nWait, (int)aX.size(),
			aX.empty() ? anDummy : &aX[0], aY.empty() ? anDummy : &aY[0]);
	if (pszError) {
		return RouteError(pszError);
	}

	/* 保存のコマンドにして管理者の操作と同じ流れで処理する（「元に戻す」にも記録される） */
	CMDPARA_NPCROUTE Para;
	NpcRouteMakePara(Para, NPCROUTE_SET, pChar);
	Para.dwTblID		= (DWORD)nTblID;
	Para.byRouteCount	= (BYTE)aX.size();
	Para.byRouteMode	= (BYTE)nMode;
	Para.byRouteWait	= (BYTE)nWait;
	ZeroMemory(Para.abyRouteX, sizeof(Para.abyRouteX));
	ZeroMemory(Para.abyRouteY, sizeof(Para.abyRouteY));
	for (size_t i = 0; i < aX.size(); i ++) {
		Para.abyRouteX[i] = (BYTE)aX[i];
		Para.abyRouteY[i] = (BYTE)aY[i];
	}
	if (!RecvCmdApplyAdminPacket((const BYTE *)&Para, sizeof(Para), "管理ページ", ADMINSCR_NPCROUTE)) {
		return RouteError("巡回ルートを反映できませんでした");
	}

	s_strResult = "{\"ok\":true,\"char\":{" + CharJson(pChar) + "}}";
	return s_strResult.c_str();
}

/* NPC番号（npcId）から今のキャラの番号（テーブル番号）を探す。無ければ -1 */
extern "C" EMSCRIPTEN_KEEPALIVE int SboWebCharFindNpc(int nNpcID)
{
	return (nNpcID <= 0) ? -1 : CharFindByNpcId((DWORD)nNpcID);
}

/* ------------------------------------------------------------------------- */
/* マップの追加・大きさの拡張（管理ページの「マップ確認」）					 */
/* ------------------------------------------------------------------------- */

/* 古い w×h の配列を、新しい nw×nh の (dx, dy) の位置へ写す（空いた所は fill） */
template <class T> static T *ResizeGrid(const T *pOld, int w, int h, int nw, int nh, int dx, int dy, T fill)
{
	T *pNew = new T[nw * nh];
	for (int i = 0; i < nw * nh; i ++) {
		pNew[i] = fill;
	}
	if (pOld) {
		for (int y = 0; y < h; y ++) {
			for (int x = 0; x < w; x ++) {
				pNew[(x + dx) + (y + dy) * nw] = pOld[x + y * w];
			}
		}
	}
	return pNew;
}

/*
 * マップの左上に dx 列・dy 行足した時に、そのマップの座標を持っているものを全部ずらす。
 * （昔の RenewDataPos は暗い範囲・光源・巡回ルート・町転送・依頼のNPC・すごろくに対応していなかったので、別に作った）
 *   abyChgMap … イベント（行き先）が変わったマップの番号に 1 を入れる
 */
static void ShiftMapPos(PMAPINFO pMap, int dx, int dy, std::vector<char> &abyChgMap)
{
	DWORD dwID = pMap->dwID;
	int i, j;

	/* このマップの設定 */
	pMap->nStartX += dx;
	pMap->nStartY += dy;
	pMap->nEntranceX += dx;
	pMap->nEntranceY += dy;
	for (i = 0; i < pMap->byDarkAreaCount && i < MAX_MAPDARKAREA; i ++) {
		pMap->aDarkArea[i].wX1 += dx;
		pMap->aDarkArea[i].wY1 += dy;
		pMap->aDarkArea[i].wX2 += dx;
		pMap->aDarkArea[i].wY2 += dy;
	}
	for (i = 0; i < pMap->byLightCount && i < MAX_MAPLIGHT; i ++) {
		pMap->aLight[i].wX += dx;
		pMap->aLight[i].wY += dy;
	}
	for (i = 0; i < pMap->bySwapCount && i < MAX_MAPSWAP; i ++) {
		pMap->aSwap[i].wX += dx;
		pMap->aSwap[i].wY += dy;
	}
	for (i = 0; i < pMap->nItemCount && pMap->pItem; i ++) {
		pMap->pItem[i].dwX += dx;
		pMap->pItem[i].dwY += dy;
	}

	/* イベント（このマップのイベントの場所と行き先・ほかのマップからこのマップへの行き先） */
	for (j = 0; j < g_aMapInfo.GetSize(); j ++) {
		PMAPINFO pMapTmp = g_aMapInfo[j];
		for (i = 0; i < pMapTmp->nEventCount && pMapTmp->pEvent; i ++) {
			PEVENTINFO pEv = &pMapTmp->pEvent[i];
			if (pMapTmp == pMap) {
				pEv->dwX += dx;
				pEv->dwY += dy;
				abyChgMap[j] = 1;
				if (pEv->pData && pEv->byType == EVENTTYPE_MOVE) {
					((PEVENTINFO_MOVE)pEv->pData)->dwX += dx;
					((PEVENTINFO_MOVE)pEv->pData)->dwY += dy;
				}
				if (pEv->pData && pEv->byType == EVENTTYPE_ONCHARMAP) {
					((PEVENTINFO_ONCHARMAP)pEv->pData)->dwX += dx;
					((PEVENTINFO_ONCHARMAP)pEv->pData)->dwY += dy;
				}
			}
			if (pEv->pData && pEv->byType == EVENTTYPE_MAPCHG) {
				PEVENTINFO_MAPCHG pChg = (PEVENTINFO_MAPCHG)pEv->pData;
				if (pChg->dwMapID == dwID) {
					pChg->dwX += dx;
					pChg->dwY += dy;
					abyChgMap[j] = 1;
				}
			}
		}
	}

	/* キャラ（今いる場所・復活地点・巡回ルート・町転送の行き先帳・橋の上下を決めた場所） */
	for (i = 0; i < g_aCharInfo.GetSize(); i ++) {
		PCHARINFO pChar = g_aCharInfo[i];
		if (pChar == NULL) {
			continue;
		}
		if (pChar->dwMapID == dwID) {
			pChar->dwX += dx;
			pChar->dwY += dy;
			pChar->dwXBack += dx;
			pChar->dwYBack += dy;
			for (j = 0; j < pChar->byRouteCount && j < MAX_NPCROUTE; j ++) {
				pChar->abyRouteX[j] += dx;
				pChar->abyRouteY[j] += dy;
			}
		}
		if (pChar->dwStartMapID == dwID) {
			pChar->dwStartX += dx;
			pChar->dwStartY += dy;
		}
		if (pChar->dwBridgeMapID == dwID) {
			pChar->dwBridgeX += dx;
			pChar->dwBridgeY += dy;
		}
		for (j = 0; j < pChar->byTownWarpCount && j < MAX_TOWNWARP; j ++) {
			if (pChar->adwTownWarpMapID[j] == dwID) {
				pChar->abyTownWarpX[j] += dx;
				pChar->abyTownWarpY[j] += dy;
			}
		}
	}

	/* 移動アイテム（キラメルの翼など）の行き先 */
	for (i = 0; i < g_aItemInfo.GetSize(); i ++) {
		PITEMINFO pItem = g_aItemInfo[i];
		if (pItem && pItem->byType == ITEMTYPE_RURA && pItem->pData) {
			PITEMINFO_RURA pRura = (PITEMINFO_RURA)pItem->pData;
			if (pRura->dwMapID == dwID) {
				pRura->dwX += dx;
				pRura->dwY += dy;
				SendCmdCHGITEM(URARASOCK_SENDALL, i);
			}
		}
	}

	/* 依頼の見えるNPC（出る場所・ポータルの行き先） */
	for (i = 0; i < g_aQuestInfo.GetSize(); i ++) {
		PQUESTINFO pQuest = g_aQuestInfo[i];
		if (pQuest == NULL) {
			continue;
		}
		for (j = 0; j < MAX_QUESTNPC; j ++) {
			PQUESTNPCINFO pNpc = &pQuest->aNpc[j];
			if (pNpc->byKind && pNpc->dwMapID == dwID) {
				pNpc->byX += dx;
				pNpc->byY += dy;
			}
			if (pNpc->byKind && pNpc->dwDestMapID == dwID) {
				pNpc->byDestX += dx;
				pNpc->byDestY += dy;
			}
		}
	}

	/* サブゲーム */
	for (i = 0; i < g_aSubaGameInfo.GetSize(); i ++) {
		PSUBGAMEINFO pInfo = g_aSubaGameInfo[i];
		if (pInfo == NULL || pInfo->pInfo == NULL) {
			continue;
		}
		switch (pInfo->byType) {
		case SUBGAMEID_BOMB:
			{
				PSUBGAME_BOMBINFO p = (PSUBGAME_BOMBINFO)pInfo->pInfo;
				if (p->dwMapID == dwID) {
					p->byX1 += dx; p->byY1 += dy; p->byX2 += dx; p->byY2 += dy;
					p->dwStartX += dx; p->dwStartY += dy; p->dwX += dx; p->dwY += dy;
				}
			}
			break;
		case SUBGAMEID_MAZE:
			{
				PSUBGAME_MAZEINFO p = (PSUBGAME_MAZEINFO)pInfo->pInfo;
				if (p->dwMapID == dwID) {
					p->byX1 += dx; p->byY1 += dy; p->byX2 += dx; p->byY2 += dy;
				}
			}
			break;
		case SUBGAMEID_DUNGEON:
			{
				PSUBGAME_DUNGEONINFO p = (PSUBGAME_DUNGEONINFO)pInfo->pInfo;
				if (p->dwMapID == dwID) {
					p->byX1 += dx; p->byY1 += dy; p->byX2 += dx; p->byY2 += dy;
				}
			}
			break;
		case SUBGAMEID_WAR:
			{
				PSUBGAME_WARINFO p = (PSUBGAME_WARINFO)pInfo->pInfo;
				if (p->dwMapID == dwID) {
					p->byX += dx; p->byY += dy;
					p->byX1 += dx; p->byY1 += dy; p->byX2 += dx; p->byY2 += dy;
					p->byHP1X += dx; p->byHP2X += dx; p->byHPY += dy;
					p->byJudgeX += dx; p->byJudgeY += dy;
					p->byRes1X += dx; p->byRes2X += dx; p->byResY += dy;
					p->byStart1X += dx; p->byStart2X += dx; p->byStartY += dy;
					p->byCastle1X += dx; p->byCastle2X += dx; p->byCastleY += dy;
				}
			}
			break;
		case SUBGAMEID_SUGOROKU:
			{
				PSUBGAME_SUGOROKUINFO p = (PSUBGAME_SUGOROKUINFO)pInfo->pInfo;
				if (p->dwMapID == dwID) {
					p->byX += dx; p->byY += dy;
					p->dwStartX += dx; p->dwStartY += dy;
					p->dwGoalX += dx; p->dwGoalY += dy;
					for (j = 0; j < p->aEventInfo.GetSize(); j ++) {
						p->aEventInfo[j].byX += dx;
						p->aEventInfo[j].byY += dy;
					}
					for (j = 0; j < p->aPlayerInfo.GetSize(); j ++) {
						p->aPlayerInfo[j].dwX += dx;
						p->aPlayerInfo[j].dwY += dy;
					}
				}
			}
			break;
		}
	}
}

/*
 * マップの大きさを広げる（上・下・左・右に足す行・列の数。縮めることはできない）。
 *   足したマスは wFill のパーツで埋める。上・左に足した時は、マップの座標を持っているものを全部ずらす。
 *   結果: {"ok":true,"cx":..,"cy":..,"shiftX":..,"shiftY":..} / {"ok":false,"error":"理由"}
 */
extern "C" EMSCRIPTEN_KEEPALIVE const char *SboWebDataMapResize(int nID, int nTop, int nBottom, int nLeft, int nRight, int nFill, int bApply)
{
	std::string strErr;
	if (nID < 0 || nID >= g_aMapInfo.GetSize()) {
		strErr = "マップの番号が範囲外です";
	} else if (nTop < 0 || nBottom < 0 || nLeft < 0 || nRight < 0) {
		strErr = "足す数は 0 以上で指定してください（縮めることはできません）";
	} else if (nTop + nBottom + nLeft + nRight == 0) {
		strErr = "足す行・列がありません";
	} else if (nFill < 0 || nFill >= g_wCountMapParts) {
		strErr = "埋めるパーツ番号 " + JsonNum(nFill) + " はありません（0〜" + JsonNum(g_wCountMapParts - 1) + "）";
	} else {
		PMAPINFO pMap = g_aMapInfo[nID];
		if (pMap->nCX + nLeft + nRight > 255 || pMap->nCY + nTop + nBottom > 255) {
			strErr = "広げた後の大きさが 255 マスを超えます（今は " + JsonNum(pMap->nCX) + "×" + JsonNum(pMap->nCY) + "）";
		}
	}
	if (!strErr.empty()) {
		s_strResult = "{\"ok\":false,\"error\":" + JsonStr(strErr.c_str()) + "}";
		return s_strResult.c_str();
	}
	PMAPINFO pMap = g_aMapInfo[nID];
	int nCX = pMap->nCX, nCY = pMap->nCY;
	int nNewCX = nCX + nLeft + nRight, nNewCY = nCY + nTop + nBottom;
	if (bApply) {
		WORD *pwNew = ResizeGrid<WORD>(pMap->pwMap, nCX, nCY, nNewCX, nNewCY, nLeft, nTop, (WORD)nFill);
		DWORD *pdwNew = ResizeGrid<DWORD>(pMap->pdwMapChar, nCX, nCY, nNewCX, nNewCY, nLeft, nTop, 0);
		SAFE_DELETE_ARRAY(pMap->pwMap);
		SAFE_DELETE_ARRAY(pMap->pdwMapChar);
		pMap->pwMap = pwNew;
		pMap->pdwMapChar = pdwNew;
		if (pMap->pbyRoofLV) {
			PBYTE p = ResizeGrid<BYTE>(pMap->pbyRoofLV, nCX, nCY, nNewCX, nNewCY, nLeft, nTop, 0);
			SAFE_DELETE_ARRAY(pMap->pbyRoofLV);
			pMap->pbyRoofLV = p;
		}
		if (pMap->pbyFlg) {
			PBYTE p = ResizeGrid<BYTE>(pMap->pbyFlg, nCX, nCY, nNewCX, nNewCY, nLeft, nTop, 0);
			SAFE_DELETE_ARRAY(pMap->pbyFlg);
			pMap->pbyFlg = p;
		}
		pMap->nCX = nNewCX;
		pMap->nCY = nNewCY;
		std::vector<char> abyChgMap(g_aMapInfo.GetSize(), 0);
		if (nLeft || nTop) {
			ShiftMapPos(pMap, nLeft, nTop, abyChgMap);
		}
		/* 保存して、ログイン中の人へ知らせる */
		SaveMapInfo();
		SaveCharInfo();
		SaveItemInfo();
		SaveSubGameInfo();
		SaveQuestInfo();
		ResendMapToPlayers(pMap, nID);
		for (int j = 0; j < (int)abyChgMap.size(); j ++) {
			if (abyChgMap[j] && j != nID) {
				SendCmdEVENTPOS(URARASOCK_SENDALL, j);
			}
		}
	}
	s_strResult = "{\"ok\":true,\"mapId\":" + JsonNum(pMap->dwID) + ",\"cx\":" + JsonNum(nNewCX) + ",\"cy\":" + JsonNum(nNewCY) +
			",\"oldCx\":" + JsonNum(nCX) + ",\"oldCy\":" + JsonNum(nCY) +
			",\"shiftX\":" + JsonNum(nLeft) + ",\"shiftY\":" + JsonNum(nTop) + "}";
	return s_strResult.c_str();
}

extern BOOL DungeonDailyRebuildNow(int nID, std::string &strErr);		/* SboSvrSubGameProc.cpp */

/*
 * 日替わりダンジョンの版と最短記録（読むだけ。docs/dungeon-gen.md の「番外編 段階2」）。
 *   nID … サブゲームの番号（-1 ならすべて）。nVerNo … 版の通し番号（0 ならすべての版）。bTheme … 版にテーマの写しも入れる
 *   結果: {"ok":true,"genVersion":生成器の版,"today":"日付","dungeons":[{"id","byRebuildMode","dwThemeID","dwBuildNo","nowVersion",
 *         "versions":[{"no","date","rebuild","seed","genVer","themeId","w","h","themeRef","created","records":[{"names","forms","count","ms","at"}]}]}]}
 *   版は古い順。records は時間の短い順（上位 20 件）。ダンジョンは「日替わりになっている」か「保存した版がある」ものだけ
 */
extern "C" EMSCRIPTEN_KEEPALIVE const char *SboWebDataDungeonRecords(int nID, int nVerNo, int bTheme)
{
	std::string out = "{\"ok\":true,\"genVersion\":" + JsonNum(DungeonDailyGenVersion()) + ",\"today\":" + JsonStr(DungeonDailyDate(DungeonDailyNow()).c_str()) + ",\"dungeons\":[";
	bool bFirst = true;
	for (int i = 0; i < g_aSubaGameInfo.GetSize(); i ++) {
		PSUBGAMEINFO pInfo = g_aSubaGameInfo[i];
		if (nID >= 0 && i != nID) {
			continue;
		}
		if (pInfo == NULL || pInfo->byType != SUBGAMEID_DUNGEON || pInfo->pInfo == NULL) {
			continue;
		}
		PSUBGAME_DUNGEONINFO p = (PSUBGAME_DUNGEONINFO)pInfo->pInfo;
		if (p->byRebuildMode != DGREBUILD_DAILY && DungeonDailyVersionCount(i) == 0) {
			continue;
		}
		if (!bFirst) out += ",";
		bFirst = false;
		out += "{\"id\":" + JsonNum(i) + ",\"byRebuildMode\":" + JsonNum(p->byRebuildMode) + ",\"dwThemeID\":" + JsonNum(p->dwThemeID) +
				",\"dwBuildNo\":" + JsonNum(p->dwBuildNo) + ",\"nowVersion\":" + JsonNum(p->dwDailyVer) +
				",\"versions\":" + DungeonDailyVersionsJson(i, (DWORD)max(nVerNo, 0), bTheme != 0) + "}";
	}
	out += "]}";
	if (bFirst && nID >= 0) {
		out = "{\"ok\":false,\"error\":\"そのサブゲームは日替わりのダンジョンではありません（byRebuildMode が 1 のダンジョンか、保存した版があるものだけ）\"}";
	}
	s_strResult = out;
	return s_strResult.c_str();
}

/*
 * 日替わりダンジョンを今すぐ作り直す（管理者の「今すぐ作り直す」と同じ）。中の組には知らせてから外へ戻し、新しいランダムなシードで作る。
 *   結果: {"ok":true,"id":..,"version":{新しい版（テーマの写しなし）}} / {"ok":false,"error":"理由"}
 */
extern "C" EMSCRIPTEN_KEEPALIVE const char *SboWebDataDungeonRebuild(int nID)
{
	std::string strErr;
	if (!DungeonDailyRebuildNow(nID, strErr)) {
		s_strResult = "{\"ok\":false,\"error\":" + JsonStr(strErr.c_str()) + "}";
		return s_strResult.c_str();
	}
	PSUBGAME_DUNGEONINFO p = (PSUBGAME_DUNGEONINFO)g_aSubaGameInfo[nID]->pInfo;
	std::string strVer = DungeonDailyVersionsJson(nID, p->dwDailyVer, false);
	s_strResult = "{\"ok\":true,\"id\":" + JsonNum(nID) + ",\"dwBuildNo\":" + JsonNum(p->dwBuildNo) + ",\"version\":" +
			((strVer.size() > 2) ? strVer.substr(1, strVer.size() - 2) : std::string("null")) + "}";
	return s_strResult.c_str();
}

/*
 * マップを足す（大きさ cx×cy・wFill のパーツで埋める・名前）。番号はいちばん後ろ。
 *   結果: {"ok":true,"id":..,"mapId":..} / {"ok":false,"error":"理由"}
 */
extern "C" EMSCRIPTEN_KEEPALIVE const char *SboWebDataMapAdd(int nCX, int nCY, int nFill, const char *pszName)
{
	std::string strErr;
	std::string strName = pszName ? pszName : "";
	if (nCX < 1 || nCY < 1 || nCX > 255 || nCY > 255) {
		strErr = "大きさは 1〜255 マスで指定してください";
	} else if (nFill < 0 || nFill >= g_wCountMapParts) {
		strErr = "埋めるパーツ番号 " + JsonNum(nFill) + " はありません（0〜" + JsonNum(g_wCountMapParts - 1) + "）";
	} else {
		CheckText(strName, 256, false, strErr, "szName");
	}
	if (!strErr.empty()) {
		s_strResult = "{\"ok\":false,\"error\":" + JsonStr(strErr.c_str()) + "}";
		return s_strResult.c_str();
	}
	PMAPINFO pMap = new MAPINFO;
	ZeroMemory(pMap, sizeof(MAPINFO));
	/* マップID は番号と同じにそろえる（キャラの dwMapID・GetMapNo はこれで探す） */
	pMap->dwID = (DWORD)g_aMapInfo.GetSize();
	if (g_dwNewMapID <= pMap->dwID) {
		g_dwNewMapID = pMap->dwID + 1;
	}
	pMap->nCX = nCX;
	pMap->nCY = nCY;
	pMap->pwMap = ResizeGrid<WORD>(NULL, 0, 0, nCX, nCY, 0, 0, (WORD)nFill);
	pMap->pdwMapChar = ResizeGrid<DWORD>(NULL, 0, 0, nCX, nCY, 0, 0, 0);
	if (!strName.empty()) {
		pMap->pszName = new char[strName.size() + 1];
		CopyMemory(pMap->pszName, strName.c_str(), strName.size() + 1);
	}
	int nID = g_aMapInfo.Add(pMap);
	SaveMapInfo();
	s_strResult = "{\"ok\":true,\"id\":" + JsonNum(nID) + ",\"mapId\":" + JsonNum(pMap->dwID) + "}";
	return s_strResult.c_str();
}
