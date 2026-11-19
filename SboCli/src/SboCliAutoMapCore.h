/* Copyright(C)URARA-works 2003-2026 */
/* ========================================================================= */
/* ファイル名：	SboCliAutoMapCore.h											 */
/* 内容：		探索マップ（オートマップ）の記録と線の作り方				 */
/*				テーマで作った気まぐれダンジョンの範囲だけで使う			 */
/* 備考：		ゲーム本体の型（Windows の型・グローバル）に依存しない		 */
/*				標準の C++ だけで書いている。tests/client-native/automap.cpp	 */
/*				が、そのまま取り込んで単体で確かめる						 */
/* ========================================================================= */

#pragma once

#include <cstddef>
#include <vector>

namespace AutoMapCore
{

using std::size_t;

/* 部屋（壁を含む外枠。マップ座標）。group が同じ部屋は、いっしょに開ける */
struct Room {
	int x, y, w, h, group;
};

/* 部屋どうしのつながり（出入口の穴の範囲。マップ座標）。type 1=アーチ 2=扉 3=壁ごと抜いた */
struct Link {
	int a, b, x, y, w, h, type;
};

/* 線（マス目の角の座標。範囲の左上が 0,0） */
struct Seg {
	int x1, y1, x2, y2;
};

enum {
	CELL_REC	= 1,		/* 記録済み（地図に出す） */
	CELL_OPEN	= 2,		/* 出入口として出したマス（歩けるものとして線を引く） */
	CELL_LINK	= 4			/* 出入口の穴のマス（部屋を開ける判定には使わない） */
};

class Map
{
public:
	Map() { Clear (); }

	/* 全部消す（探索マップを使わない状態） */
	void Clear (void)
	{
		m_bValid = false;
		m_dwMapID = 0;
		m_dwBuildNo = 0;
		m_x1 = m_y1 = m_x2 = m_y2 = 0;
		m_nW = m_nH = 0;
		m_aRoom.clear ();
		m_aLink.clear ();
		m_abyCell.clear ();
		m_abyRoomOpen.clear ();
		m_abyLinkShown.clear ();
		m_bDirty = true;
	}

	/* 記録だけ消す（ダンジョンの範囲から出た時） */
	void ClearRecord (void)
	{
		size_t i;
		for (i = 0; i < m_abyCell.size (); i ++) {
			m_abyCell[i] &= CELL_LINK;
		}
		for (i = 0; i < m_abyRoomOpen.size (); i ++) {
			m_abyRoomOpen[i] = 0;
		}
		for (i = 0; i < m_abyLinkShown.size (); i ++) {
			m_abyLinkShown[i] = 0;
		}
		m_bDirty = true;
	}

	/* サーバーからの情報を受け取る。マップ・作り直し番号・範囲が覚えているものと違えば記録を全部消す */
	void Set (unsigned long dwMapID, unsigned long dwBuildNo, int x1, int y1, int x2, int y2,
			  const std::vector<Room> &aRoom, const std::vector<Link> &aLink)
	{
		bool bSame = m_bValid && (m_dwMapID == dwMapID) && (m_dwBuildNo == dwBuildNo) &&
					 (m_x1 == x1) && (m_y1 == y1) && (m_x2 == x2) && (m_y2 == y2) &&
					 (m_aRoom.size () == aRoom.size ()) && (m_aLink.size () == aLink.size ());
		std::vector<unsigned char> abyKeepCell, abyKeepRoom, abyKeepLink;

		if (bSame) {
			abyKeepCell = m_abyCell;
			abyKeepRoom = m_abyRoomOpen;
			abyKeepLink = m_abyLinkShown;
		}
		Clear ();
		if ((x2 < x1) || (y2 < y1)) {
			return;
		}
		m_bValid = true;
		m_dwMapID = dwMapID;
		m_dwBuildNo = dwBuildNo;
		m_x1 = x1;
		m_y1 = y1;
		m_x2 = x2;
		m_y2 = y2;
		m_nW = x2 - x1 + 1;
		m_nH = y2 - y1 + 1;
		m_aRoom = aRoom;
		m_aLink = aLink;
		/* 範囲からはみ出す四角は範囲に収める（壊れた情報で長い繰り返しにならないように） */
		for (size_t i = 0; i < m_aRoom.size (); i ++) {
			ClipRect (m_aRoom[i].x, m_aRoom[i].y, m_aRoom[i].w, m_aRoom[i].h);
		}
		for (size_t i = 0; i < m_aLink.size (); i ++) {
			ClipRect (m_aLink[i].x, m_aLink[i].y, m_aLink[i].w, m_aLink[i].h);
		}
		m_abyCell.assign ((size_t)m_nW * m_nH, 0);
		m_abyRoomOpen.assign (m_aRoom.size (), 0);
		m_abyLinkShown.assign (m_aLink.size (), 0);
		for (size_t i = 0; i < m_aLink.size (); i ++) {
			const Link &lk = m_aLink[i];
			for (int y = lk.y; y < lk.y + lk.h; y ++) {
				for (int x = lk.x; x < lk.x + lk.w; x ++) {
					if (InRange (x, y)) {
						m_abyCell[Index (x, y)] |= CELL_LINK;
					}
				}
			}
		}
		if (bSame) {
			/* 同じ作り直し番号の送り直し（マップの書き込み・入り直し）は、記録を残す。
			   CELL_LINK は作り直した値、記録のビットだけ戻す */
			for (size_t i = 0; i < m_abyCell.size (); i ++) {
				m_abyCell[i] |= (abyKeepCell[i] & (CELL_REC | CELL_OPEN));
			}
			m_abyRoomOpen = abyKeepRoom;
			m_abyLinkShown = abyKeepLink;
		}
		m_bDirty = true;
	}

	bool IsValid (void) const { return m_bValid; }
	unsigned long GetMapID (void) const { return m_dwMapID; }
	unsigned long GetBuildNo (void) const { return m_dwBuildNo; }
	int GetW (void) const { return m_nW; }
	int GetH (void) const { return m_nH; }
	int GetX1 (void) const { return m_x1; }
	int GetY1 (void) const { return m_y1; }

	bool InRange (int x, int y) const
	{
		return m_bValid && (x >= m_x1) && (x <= m_x2) && (y >= m_y1) && (y <= m_y2);
	}

	/* 自分の位置。部屋の中に立っていれば、その部屋（つながった広間ごと）の壁と出入口を記録する */
	void Visit (int x, int y)
	{
		if (!InRange (x, y)) {
			return;
		}
		/* 出入口の穴の上は、どの部屋の中でもない（一歩進んで中に入ったら開ける） */
		if (m_abyCell[Index (x, y)] & CELL_LINK) {
			return;
		}
		for (size_t i = 0; i < m_aRoom.size (); i ++) {
			const Room &r = m_aRoom[i];
			if ((x >= r.x) && (x < r.x + r.w) && (y >= r.y) && (y < r.y + r.h)) {
				OpenGroup (r.group);
			}
		}
	}

	/* 見えたマス */
	void Seen (int x, int y)
	{
		if (!InRange (x, y)) {
			return;
		}
		unsigned char &by = m_abyCell[Index (x, y)];
		if (!(by & CELL_REC)) {
			by |= CELL_REC;
			m_bDirty = true;
		}
	}

	bool IsRecorded (int x, int y) const
	{
		return InRange (x, y) && (m_abyCell[Index (x, y)] & CELL_REC);
	}

	bool IsRoomOpen (size_t nRoom) const
	{
		return (nRoom < m_abyRoomOpen.size ()) && m_abyRoomOpen[nRoom];
	}

	size_t GetLinkCount (void) const { return m_aLink.size (); }
	const Link &GetLink (size_t i) const { return m_aLink[i]; }
	bool IsLinkShown (size_t i) const { return (i < m_abyLinkShown.size ()) && m_abyLinkShown[i]; }

	/* 記録が増えた（線を作り直す必要がある） */
	bool IsDirty (void) const { return m_bDirty; }
	void SetDirty (void) { m_bDirty = true; }

	/* 壁の線を作る：歩ける記録済みマスと、歩けないマス（範囲の外を含む）の境目。
	   fnWalk(x, y) はマップ座標のマスが歩けるか。出入口として出したマスは常に歩ける扱い */
	template <class F>
	void BuildSegments (F fnWalk, std::vector<Seg> &aSeg)
	{
		static const int anDX[4] = { 0, 0, -1, 1 }, anDY[4] = { -1, 1, 0, 0 };
		int x, y, i;

		aSeg.clear ();
		if (!m_bValid) {
			return;
		}
		for (y = m_y1; y <= m_y2; y ++) {
			for (x = m_x1; x <= m_x2; x ++) {
				if (!(m_abyCell[Index (x, y)] & CELL_REC) || !Walk (fnWalk, x, y)) {
					continue;
				}
				for (i = 0; i < 4; i ++) {
					if (Walk (fnWalk, x + anDX[i], y + anDY[i])) {
						continue;
					}
					Seg s;
					int cx = x - m_x1, cy = y - m_y1;
					switch (i) {
					case 0:	s.x1 = cx;		s.y1 = cy;		s.x2 = cx + 1;	s.y2 = cy;		break;
					case 1:	s.x1 = cx;		s.y1 = cy + 1;	s.x2 = cx + 1;	s.y2 = cy + 1;	break;
					case 2:	s.x1 = cx;		s.y1 = cy;		s.x2 = cx;		s.y2 = cy + 1;	break;
					default:s.x1 = cx + 1;	s.y1 = cy;		s.x2 = cx + 1;	s.y2 = cy + 1;	break;
					}
					aSeg.push_back (s);
				}
			}
		}
		m_bDirty = false;
	}

private:
	/* 四角を範囲の中に収める（全部外なら幅・高さ 0） */
	void ClipRect (int &x, int &y, int &w, int &h) const
	{
		int x2 = x + w, y2 = y + h;

		if (x < m_x1) { x = m_x1; }
		if (y < m_y1) { y = m_y1; }
		if (x2 > m_x2 + 1) { x2 = m_x2 + 1; }
		if (y2 > m_y2 + 1) { y2 = m_y2 + 1; }
		w = (x2 > x) ? x2 - x : 0;
		h = (y2 > y) ? y2 - y : 0;
	}

	int Index (int x, int y) const { return (y - m_y1) * m_nW + (x - m_x1); }

	template <class F>
	bool Walk (F fnWalk, int x, int y) const
	{
		if (!InRange (x, y)) {
			return false;
		}
		if (m_abyCell[Index (x, y)] & CELL_OPEN) {
			return true;
		}
		return fnWalk (x, y) ? true : false;
	}

	/* グループの部屋を全部開ける。部屋の外枠を記録し、その部屋につながる出入口を出す */
	void OpenGroup (int nGroup)
	{
		size_t i;
		bool bNew = false;

		for (i = 0; i < m_aRoom.size (); i ++) {
			const Room &r = m_aRoom[i];
			if ((r.group != nGroup) || m_abyRoomOpen[i]) {
				continue;
			}
			m_abyRoomOpen[i] = 1;
			bNew = true;
			for (int y = r.y; y < r.y + r.h; y ++) {
				for (int x = r.x; x < r.x + r.w; x ++) {
					if (InRange (x, y)) {
						m_abyCell[Index (x, y)] |= CELL_REC;
					}
				}
			}
		}
		if (!bNew) {
			return;
		}
		for (i = 0; i < m_aLink.size (); i ++) {
			const Link &lk = m_aLink[i];
			if (m_abyLinkShown[i]) {
				continue;
			}
			bool bA = (lk.a >= 0) && ((size_t)lk.a < m_aRoom.size ()) && m_abyRoomOpen[lk.a];
			bool bB = (lk.b >= 0) && ((size_t)lk.b < m_aRoom.size ()) && m_abyRoomOpen[lk.b];
			if (!bA && !bB) {
				continue;
			}
			m_abyLinkShown[i] = 1;
			for (int y = lk.y; y < lk.y + lk.h; y ++) {
				for (int x = lk.x; x < lk.x + lk.w; x ++) {
					if (InRange (x, y)) {
						m_abyCell[Index (x, y)] |= (CELL_REC | CELL_OPEN);
					}
				}
			}
		}
		m_bDirty = true;
	}

private:
	bool m_bValid;
	unsigned long m_dwMapID, m_dwBuildNo;
	int m_x1, m_y1, m_x2, m_y2, m_nW, m_nH;
	std::vector<Room> m_aRoom;
	std::vector<Link> m_aLink;
	std::vector<unsigned char> m_abyCell;
	std::vector<unsigned char> m_abyRoomOpen;
	std::vector<unsigned char> m_abyLinkShown;
	bool m_bDirty;
};

}	/* namespace AutoMapCore */
