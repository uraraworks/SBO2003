	/* 画面に見えている範囲だけ処理する */
	nSX = 32 - anPosAddTblX[pMyChar->byDirection] * pMyChar->byDotMove;
	nSY = 32 - anPosAddTblY[pMyChar->byDirection] * pMyChar->byDotMove;
	nCX = SCRSIZEX;
	nCY = SCRSIZEY;
	if (nSX < 0) { nCX += nSX; nSX = 0; }
	if (nSY < 0) { nCY += nSY; nSY = 0; }
	nCX = min (nCX, nWidth - nSX);
	nCY = min (nCY, nHeight - nSY);

	for (y = nSY; y < nSY + nCY; y ++) {
		PDWORD pDst;
		float fTy, fVisY0, fVisY1;
		int nTy, nMY0, nMY1;

		/* 見えるマスかどうかをマスの中心どうしでなめらかにつなぐ(縦) */
		fTy = (y - 16) / 32.0f;
		nTy = (int)floorf (fTy);
		fTy -= nTy;
		nMY0 = nMapW * max (0, min (nTy, DRAW_PARTS_Y * 2 + 2));
		nMY1 = nMapW * max (0, min (nTy + 1, DRAW_PARTS_Y * 2 + 2));
		for (x = 0; x < nMapW; x ++) {
			int y0 = max (0, min (nTy, DRAW_PARTS_Y * 2 + 2));
			int y1 = max (0, min (nTy + 1, DRAW_PARTS_Y * 2 + 2));
			fVisY0 = (pMapDark[x + nMapW * y0] > 1) ? 1.0f : 0.0f;
			fVisY1 = (pMapDark[x + nMapW * y1] > 1) ? 1.0f : 0.0f;
			afVisX[x] = fVisY0 + (fVisY1 - fVisY0) * fTy;
		}

		pDst = (PDWORD)(pBits + (nWidth * 4) * (nHeight - 1 - y)) + nSX;
		for (x = nSX; x < nSX + nCX; x ++, pDst ++) {
			float fTx, fVis, fLight, fWarm;
			int nTx, x0, x1, nLevel, nWarm;

			/* 見えるマスかどうか(横) */
			fTx = (x - 16) / 32.0f;
			nTx = (int)floorf (fTx);
			fTx -= nTx;
			x0 = max (0, min (nTx, nMapW - 1));
			x1 = max (0, min (nTx + 1, nMapW - 1));
			fVis = afVisX[x0] + (afVisX[x1] - afVisX[x0]) * fTx;
			/* 見えるマスの縁だけ少しぼかす(真ん中は暗くしない) */
			fVis *= 1.6f;
			if (fVis <= 0.0f) {
				*pDst &= 0xFF000000;
				continue;
			}

			/* 灯りからの距離でなめらかに暗くする(灯りが重なると明るくなる) */
			fLight = 0.0f;
			fWarm = 0.0f;
			/* 「明るく壁で隠す」範囲の中のマスは全部明るい */
			if (bHide && abyHide[(x / 32) + nMapW * (y / 32)]) {
				fLight = min (fVis, 1.0f);
				i = nLight;
			} else {
				i = 0;
			}
			for (; i < nLight; i ++) {
				float fDX, fDY, fD, fVisLight;
				if (afR[i] <= 0.0f) {
					fLight = min (fVis, 1.0f);
					fWarm = 0.0f;
					break;
				}
				fDX = (float)(x - anX[i]);
				fDY = (float)(y - anY[i]);
				fD = fDX * fDX + fDY * fDY;
				if (fD >= afR[i] * afR[i]) {
					continue;		/* 届かない灯りは平方根を取らない */
				}
				/* この灯りが照らせたマスだけ明るくする(マスの境目はなめらかに) */
				if (apMask[i]) {
					const BYTE *pMask = apMask[i];
					float fA = pMask[x0 + nMY0] + (pMask[x1 + nMY0] - pMask[x0 + nMY0]) * fTx;
					float fB = pMask[x0 + nMY1] + (pMask[x1 + nMY1] - pMask[x0 + nMY1]) * fTx;
					fVisLight = (fA + (fB - fA) * fTy) * 1.6f;
					if (fVisLight <= 0.0f) {
						continue;
					}
					fVisLight = min (fVisLight, 1.0f);
				} else {
					fVisLight = min (fVis, 1.0f);
				}
				fD = sqrtf (fD);
				if (fD < afR[i]) {
					float fAdd = (1.0f - fD / afR[i]) * afPower[i] * fVisLight;
					fLight += fAdd;
					fWarm += fAdd * afWarm[i];
				}
			}
			if (fLight > 1.0f) {
				fLight = 1.0f;
			}

			nLevel = (int)(fLight * 256.0f);
			/* 暖色の光源の光が届いている所は橙色に寄せる */
			nWarm = (int)(min (fWarm, 1.0f) * 256.0f);
			if ((nLevel >= 256) && (nWarm <= 0)) {
				continue;
			}
			nLevel = min (nLevel, 256);
			{
				DWORD dw = *pDst;
				DWORD r = ((dw >> 16) & 0xFF) * nLevel >> 8;
				DWORD g = ((dw >> 8) & 0xFF) * nLevel >> 8;
				DWORD b = (dw & 0xFF) * nLevel >> 8;
				if (nWarm > 0) {
					r = min ((DWORD)255, r * (256 + nWarm * 12 / 100) >> 8);
					g = g * (256 - nWarm * 14 / 100) >> 8;
					b = b * (256 - nWarm * 42 / 100) >> 8;
				}
				*pDst = (dw & 0xFF000000) | (r << 16) | (g << 8) | b;
			}
		}
	}