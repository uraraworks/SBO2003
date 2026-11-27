/* ピンボールの盤の寸法を JSON で書き出す（make_art_template.py が使う）。
   common/Pinball.h の定数と PinballTable をそのまま出すので、台の形を変えても絵のテンプレートが追従する。
   g++ -std=c++11 -I<リポジトリルート> pinball_dump.cpp -o pinball_dump && ./pinball_dump */
#include <stdio.h>
#include "common/Pinball.h"

static double Dot(int nFp) { return (double)nFp / PB_FP; }

int main()
{
	const PinballTable &T = PinballGetTable ();
	int i, nLx;

	printf ("{\n");
	printf ("\"W\": %d, \"H\": %d, \"VIEW_H\": %d, \"YOFS\": %d,\n", PB_W, PB_H, PB_VIEW_H, PB_YOFS);
	printf ("\"BALL_R\": %g, \"BUMPER_R\": %g, \"TARGET_R\": %g, \"FLIP_LEN\": %d, \"FLIP_R\": %g,\n",
		Dot (PB_BALL_R), Dot (PB_BUMPER_R), Dot (PB_TARGET_R), PB_FLIP_LEN, Dot (PB_FLIP_R));
	printf ("\"FLIP_REST_DEG\": %d, \"FLIP_UP_DEG\": %d,\n", PbFlipDeg (0), PbFlipDeg (PB_FLIP_STEPS));
	printf ("\"BUMPERS\": %d, \"TARGETS\": %d,\n", PB_BUMPERS, PB_TARGETS);
	printf ("\"arch\": {\"cx\": %g, \"cy\": %g, \"r\": %g},\n", Dot (T.nArchCx), Dot (T.nArchCy), Dot (T.nArchR));
	printf ("\"pivot\": {\"lx\": %g, \"rx\": %g, \"y\": %g},\n", Dot (T.nPivotLx), Dot (T.nPivotRx), Dot (T.nPivotY));
	printf ("\"lane_left\": %g, \"start\": {\"x\": %g, \"y\": %g}, \"drain_y\": %g,\n", Dot (T.nLaneL), Dot (T.nStartX), Dot (T.nStartY), Dot (T.nDrainY));
	printf ("\"walls\": [");
	for (i = 0, nLx = 0; i < T.nSeg; i ++) {
		if (T.aSeg[i].nTarget >= 0) continue;
		printf ("%s[%g, %g, %g, %g]", nLx ++ ? ", " : "", Dot (T.aSeg[i].x1), Dot (T.aSeg[i].y1), Dot (T.aSeg[i].x2), Dot (T.aSeg[i].y2));
	}
	printf ("],\n\"targets\": [");
	for (i = 0, nLx = 0; i < T.nSeg; i ++) {
		if (T.aSeg[i].nTarget < 0) continue;
		printf ("%s[%g, %g, %g, %g, %d]", nLx ++ ? ", " : "", Dot (T.aSeg[i].x1), Dot (T.aSeg[i].y1), Dot (T.aSeg[i].x2), Dot (T.aSeg[i].y2), T.aSeg[i].nTarget);
	}
	printf ("],\n\"bumpers\": [");
	for (i = 0; i < PB_BUMPERS; i ++) {
		printf ("%s[%g, %g]", i ? ", " : "", Dot (T.aBumper[i].x), Dot (T.aBumper[i].y));
	}
	printf ("]\n}\n");
	return 0;
}
