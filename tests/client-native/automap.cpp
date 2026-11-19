/* 探索マップの記録と線の作り方（SboCliAutoMapCore.h）の単体確認。ゲーム本体とは独立 */
#include "../../SboCli/src/SboCliAutoMapCore.h"
#include <cstdio>

using namespace AutoMapCore;

static int g_nFail = 0;
#define CHECK(c) do { if (!(c)) { printf ("NG line %d: %s\n", __LINE__, #c); g_nFail ++; } } while (0)

/* 範囲 (12,10)-(51,49)。部屋 A 床 x13..21,y12..17 / 部屋 B 床 x23..31 / 部屋 C 床 x41..46。壁は外枠（A・B は x=22 を共有）。
   A-B はアーチ(22,13,1,3)、B-C は壁ごと抜いた(32..39, 13..15) で、C は B と同じグループ */
static std::vector<Room> MakeRooms (void)
{
	std::vector<Room> a;
	Room r;
	r.x = 12; r.y = 10; r.w = 11; r.h = 9; r.group = 0; a.push_back (r);
	r.x = 22; r.y = 10; r.w = 11; r.h = 9; r.group = 1; a.push_back (r);
	r.x = 40; r.y = 10; r.w = 8;  r.h = 9; r.group = 1; a.push_back (r);
	return a;
}
static std::vector<Link> MakeLinks (void)
{
	std::vector<Link> a;
	Link l;
	l.a = 0; l.b = 1; l.x = 22; l.y = 13; l.w = 1; l.h = 3; l.type = 1; a.push_back (l);
	l.a = 1; l.b = 2; l.x = 32; l.y = 13; l.w = 8; l.h = 3; l.type = 3; a.push_back (l);
	return a;
}
struct Walk {
	bool operator () (int x, int y) const
	{
		if (y < 12 || y > 17) return false;
		if (x >= 13 && x <= 21) return true;
		if (x >= 23 && x <= 31) return true;
		if (x >= 41 && x <= 46) return true;
		if (x == 22 && y >= 13 && y <= 15) return true;
		if (x >= 32 && x <= 40 && y >= 13 && y <= 15) return true;
		return false;
	}
};

int main (void)
{
	Map m;
	std::vector<Seg> aSeg;

	CHECK (!m.IsValid ());
	m.Set (7, 1, 12, 10, 51, 49, MakeRooms (), MakeLinks ());
	CHECK (m.IsValid () && m.GetW () == 40 && m.GetH () == 40);
	m.BuildSegments (Walk (), aSeg);
	CHECK (aSeg.empty ());								/* まだ何も記録していない */

	/* 範囲の外・出入口の穴の上では開かない */
	m.Visit (5, 5);
	m.Visit (22, 14);
	m.BuildSegments (Walk (), aSeg);
	CHECK (aSeg.empty () && !m.IsRoomOpen (0));

	/* 部屋 A に入る → A の外枠と、A につながる出入口が出る。B は出ない */
	m.Visit (15, 14);
	CHECK (m.IsRoomOpen (0) && !m.IsRoomOpen (1) && !m.IsRoomOpen (2));
	CHECK (m.IsLinkShown (0) && !m.IsLinkShown (1));
	CHECK (m.IsRecorded (12, 10) && m.IsRecorded (22, 18) && m.IsRecorded (22, 14) && !m.IsRecorded (23, 14));
	m.BuildSegments (Walk (), aSeg);
	CHECK (aSeg.size () == 29);							/* 床 9×6 の外周 30 辺 - 穴の 3 辺 + 穴の上下 2 辺 */
	for (size_t i = 0; i < aSeg.size (); i ++) {
		CHECK (aSeg[i].x1 >= 0 && aSeg[i].x2 <= 40 && aSeg[i].y1 >= 0 && aSeg[i].y2 <= 40);
	}
	CHECK (!m.IsDirty ());

	/* 同じ部屋の中を歩いても増えない */
	m.Visit (16, 15);
	CHECK (!m.IsDirty ());

	/* 見えたマス（部屋の外）を足す → 線が増える。B の床は歩けるので、A の穴の向こうは線にならない */
	m.Seen (23, 12);
	m.Seen (23, 14);									/* 回りが全部床のマスは線が増えない */
	CHECK (m.IsDirty () && m.IsRecorded (23, 12) && m.IsRecorded (23, 14));
	m.BuildSegments (Walk (), aSeg);
	CHECK (aSeg.size () == 29 + 2);						/* (23,12) の上と左の壁 */
	m.Seen (60, 60);
	m.Seen (11, 10);
	CHECK (!m.IsRecorded (60, 60) && !m.IsRecorded (11, 10));

	/* 作り直し番号が同じ送り直しは記録を残す。違えば全部消す */
	m.Set (7, 1, 12, 10, 51, 49, MakeRooms (), MakeLinks ());
	CHECK (m.IsRoomOpen (0) && m.IsRecorded (23, 14) && m.IsLinkShown (0));
	m.Set (7, 2, 12, 10, 51, 49, MakeRooms (), MakeLinks ());
	CHECK (!m.IsRoomOpen (0) && !m.IsRecorded (23, 14) && !m.IsLinkShown (0));
	m.Set (8, 2, 12, 10, 51, 49, MakeRooms (), MakeLinks ());
	m.Visit (15, 14);
	m.Set (9, 2, 12, 10, 51, 49, MakeRooms (), MakeLinks ());
	CHECK (!m.IsRoomOpen (0));							/* マップ番号が違う */

	/* B に入る → 壁ごと抜いてつながった C もいっしょに開く */
	m.Visit (25, 14);
	CHECK (m.IsRoomOpen (1) && m.IsRoomOpen (2) && !m.IsRoomOpen (0));
	CHECK (m.IsLinkShown (0) && m.IsLinkShown (1));
	CHECK (m.IsRecorded (47, 18) && m.IsRecorded (35, 14));

	/* 記録だけ消す（範囲を出た時）。情報は残る */
	m.ClearRecord ();
	CHECK (m.IsValid () && !m.IsRoomOpen (1) && !m.IsRecorded (35, 14) && !m.IsLinkShown (0));
	m.BuildSegments (Walk (), aSeg);
	CHECK (aSeg.empty ());
	m.Visit (15, 14);
	CHECK (m.IsRoomOpen (0));

	/* 全部消す */
	m.Clear ();
	CHECK (!m.IsValid ());
	m.Visit (15, 14);
	m.Seen (15, 14);
	CHECK (!m.IsRecorded (15, 14));

	/* 部屋の一覧が空（洞窟）でも、見えたマスだけで地図ができる */
	m.Set (3, 1, 0, 0, 9, 9, std::vector<Room> (), std::vector<Link> ());
	m.Seen (4, 4);
	m.BuildSegments (Walk (), aSeg);
	CHECK (m.IsRecorded (4, 4));

	if (g_nFail == 0) {
		printf ("automap OK\n");
		return 0;
	}
	printf ("automap NG %d\n", g_nFail);
	return 1;
}
