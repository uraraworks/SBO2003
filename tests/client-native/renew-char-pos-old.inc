void RenewCharPosInfoOld(void)
{
	int i, nNo, nCount;

	/* まずは消去 */
	for (i = 0; i < DRAW_PARTS_Y * 2 + 3; i ++) {
		g_aCharPos[i].RemoveAll ();
	}

	if (g_nMyCharID < 0) {
		return;
	}

	PCHARINFO pMyChar = g_aCharInfo[g_nMyCharID];

	nCount = g_aCharInfo.GetSize ();

	ZeroMemory (g_pMapDarkTmp, SCRMARTSSIZE);
	s_nLightMaskCount = 0;

	for (i = 0; i < nCount; i ++) {
		PCHARINFO pChar;

		pChar = g_aCharInfo[i];
		nNo = DRAW_PARTS_Y + 1;
		nNo += (pChar->dwY - pMyChar->dwY);

		if (nNo < 0 || nNo >= DRAW_PARTS_Y * 2 + 3) {
			continue;
		}

		/* 依頼の見える物（宝箱・光）は同じ行の中で先に描く（上に乗った人が宝箱に隠れていた） */
		if (pChar->byNpcType == NPCTYPE_QUESTNPC) {
			g_aCharPos[nNo].InsertAt (0, i);
		} else {
			g_aCharPos[nNo].Add (i);
		}
		if (pMyChar != pChar) {
			RenewMapDark (pChar);
		}
	}
	RenewMapDark (NULL);
	RenewMapDarkParts ();
	RenewMapDarkHideArea ();
}
