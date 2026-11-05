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

#include <emscripten.h>
#include <string>
#include <vector>

extern BOOL SaveItemInfo(void);
extern BOOL SaveStTmpInfo(void);
extern BOOL SaveJobInfo(void);
extern BOOL SaveMagicInfo(void);
extern BOOL SaveMagicUserInfo(void);
extern BOOL SaveEquUserInfo(void);
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

static const char *s_pszNoteMagicType =
	"0=効果無し 1=HP減少 2=HP増加 3=明かり 4=魔法発生 5=帰還 6=HP減少(攻撃力) 7=魔法発生(自分) 8=蘇生 9=HP減少(HP消費) 10=HP減少(率)";
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
	FLD (STATUSTEMPLATEINFO, byNpcType,		"NPC種別",			FT_U8, 0, "0=未設定 1=移動しない 2=ランダム移動 11=戦闘1 12=検索して追尾 13=戦闘2 など"),
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
};

/* アイテム情報（共通の項目。使った時のメッセージ・種別ごとの詳細は別に扱う） */
static const char *s_pszNoteItemType =
	"0=効果無し 1=特定位置へ移動 2=HP回復 3=武器 4=鎧 5=盾 6=兜 7=種 8=サブゲーム参加 9=酒 10=メッセージ表示 "
	"11=キャラ画像変更 12=明かり 13=HP減少 14=魔法習得(特技の書) 15=Mana回復 16=HP・Mana回復 17=小手 18=靴 "
	"19=首飾り 20=指輪1 21=指輪2 22=蘇生。変えると detail は 0 から作り直す";
static const DATAFIELD s_aFieldItem[] = {
	FLDS(ITEMINFO, szName,				"名前",					FT_STR, 0, NULL),
	FLD (ITEMINFO, byType,				"種別",					FT_U8, 0, s_pszNoteItemType),
	FLD (ITEMINFO, byData,				"付加データ",			FT_U8, 0, "武器は攻撃力、防具は守備力（首飾り・指輪は detail.byEquType の効果量）"),
	FLD (ITEMINFO, byFlg,				"フラグ",				FT_U8, 0, NULL),
	FLD (ITEMINFO, bySound,				"使った時の音",			FT_U8, 0, NULL),
	FLD (ITEMINFO, bySexDistinction,	"性別",					FT_U8, 0, "0=全員 1=男性のみ 2=女性のみ"),
	FLD (ITEMINFO, byEraseProbability,	"無くなる確率",			FT_U8, 0, NULL),
	FLD (ITEMINFO, wGrpID,				"画像ID",				FT_U16, 0, NULL),
	FLD (ITEMINFO, dwGold,				"値段",					FT_U32, 0, NULL),
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
	FLD (ITEMINFO_WEAPON, byAtackType,	"攻撃種別",				FT_U8, 0, "0=直接攻撃 1=飛び道具"),
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
			n = 0;
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
	KIND_MAX
};

static const char *s_apszKindName[] = { "magic", "magicUser", "equUser", "job", "stTmp", "item" };
static const char *s_apszKindLabel[] = {
	"特技（魔法）情報", "特技を使える職業", "装備できる職業", "職業情報とレベルアップ情報", "ステータステンプレート（NPC・敵のひな形）", "アイテム情報"
};
static const char *s_apszKindNote[] = {
	"番号は 0 から。item の detail.byID（特技の書）や stTmp の abyMagic から参照される",
	"jobIds に職業の番号（job の id、0 から）を並べる。magic の byUserInfo から参照される",
	"jobIds に職業の番号（job の id、0 から）を並べる。装備アイテムの detail.wEqu から参照される",
	"szName は職業名。levelUp の 1 件目が Lv1 の初期値、n 件目はレベル n になるのに必要な経験値（dwExp）と各能力の上限",
	"変えると、このテンプレートを使っている NPC（発生元を除く）の能力もすぐ置き換わる",
	"番号 0 は「なし」として使われる。detail は byType ごとに項目が違う（スキーマの detailByType を参照）。szMsg は使った時のメッセージ"
};
static const int s_anKindMax[] = { 256, 256, 256, 256, 65535, 65535 };

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
	}
	return 0;
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

#define MAX_ITEMMSG		1024		/* 使った時のメッセージの長さ（バイト） */

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
	}
	return out + "}";
}

extern "C" EMSCRIPTEN_KEEPALIVE const char *SboWebDataGet(const char *pszKind, int nID)
{
	int nKind = KindFromName(pszKind);
	s_strResult = (nKind < 0) ? "null" : RecordJson(nKind, nID);
	return s_strResult.c_str();
}

/*
 * 変更を確かめ、bApply なら反映する
 *   pszChanges は JSON のオブジェクト（変える項目だけ）。id が件数と同じなら追加する。
 *   結果: {"ok":true,"id":番号,"added":bool,"record":{変更後}} / {"ok":false,"error":"理由"}
 *   bApply が 0 の時は何も変えず、変更後の姿（record）だけ返す。
 */
extern "C" EMSCRIPTEN_KEEPALIVE const char *SboWebDataApply(const char *pszKind, int nID, const char *pszChanges, int bApply)
{
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
	bAdd = (nID == nCount);
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
			}
			if (!CheckKnownKeys(chg, s_aFieldStTmp, COUNTOF(s_aFieldStTmp), NULL, strErr) ||
					!ApplyFields((BYTE *)&Info, s_aFieldStTmp, COUNTOF(s_aFieldStTmp), chg, strErr, "")) {
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
	}

Exit:
	if (!strErr.empty()) {
		s_strResult = "{\"ok\":false,\"error\":" + JsonStr(strErr.c_str()) + "}";
	} else {
		s_strResult = "{\"ok\":true,\"id\":" + JsonNum(nID) + ",\"added\":" + (bAdd ? "true" : "false") +
				",\"record\":{\"id\":" + JsonNum(nID) + "," + strRecord + "}}";
	}
	return s_strResult.c_str();
}
