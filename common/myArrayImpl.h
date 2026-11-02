/* Copyright(C)URARA-works 2005 */
/* ========================================================================= */
/* ファイル名：	MyArrayImpl.h												 */
/* 内容：		MFCのCArrayみたいな配列クラス 実装ファイル					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2005/01/18													 */
/* ========================================================================= */

#ifndef MYARRAYIMPLE_H
#define MYARRAYIMPLE_H


/* ========================================================================= */
/* 関数名：	CmyArray<TYPE, ARG_TYPE>::CmyArray								 */
/* 内容：	コンストラクタ													 */
/* 日付：	2005/01/18														 */
/* ========================================================================= */

template<class TYPE, class ARG_TYPE>
CmyArray<TYPE, ARG_TYPE>::CmyArray()
{
	m_pData = NULL;
	m_nCount = 0;
}


/* ========================================================================= */
/* 関数名：	CmyArray<TYPE, ARG_TYPE>::~CmyArray								 */
/* 内容：	デストラクタ													 */
/* 日付：	2005/01/18														 */
/* ========================================================================= */

template<class TYPE, class ARG_TYPE>
CmyArray<TYPE, ARG_TYPE>::~CmyArray()
{
	RemoveAll ();
}


/* ========================================================================= */
/* 関数名：	CmyArray<TYPE, ARG_TYPE>::GetSize								 */
/* 内容：	要素数を取得													 */
/* 戻り値：	要素数															 */
/* 日付：	2005/01/18														 */
/* ========================================================================= */

template<class TYPE, class ARG_TYPE>
int CmyArray<TYPE, ARG_TYPE>::GetSize(void)
{
	return m_nCount;
}


/* ========================================================================= */
/* 関数名：	CmyArray<TYPE, ARG_TYPE>::RemoveAll								 */
/* 内容：	全要素を削除													 */
/* 日付：	2005/01/18														 */
/* ========================================================================= */

template<class TYPE, class ARG_TYPE>
void CmyArray<TYPE, ARG_TYPE>::RemoveAll(void)
{
	if (m_pData) {
		delete [] m_pData;
		m_pData = NULL;
	}
	m_nCount = 0;
}


/* ========================================================================= */
/* 関数名：	CmyArray<TYPE, ARG_TYPE>::GetAt									 */
/* 内容：	指定要素を取得													 */
/* 戻り値：	要素のデータ													 */
/* 日付：	2005/01/18														 */
/* ========================================================================= */

template<class TYPE, class ARG_TYPE>
TYPE CmyArray<TYPE, ARG_TYPE>::GetAt(
	int nIndex)		/* [in] 要素ID */
{
	TYPE Ret;

	ZeroMemory (&Ret, sizeof (TYPE));

	if (nIndex >= m_nCount) {
		goto Exit;
	}

	Ret = m_pData[nIndex];

Exit:
	return Ret;
}


/* ========================================================================= */
/* 関数名：	CmyArray<TYPE, ARG_TYPE>::SetAt									 */
/* 内容：	指定要素を更新													 */
/* 日付：	2005/01/18														 */
/* ========================================================================= */

template<class TYPE, class ARG_TYPE>
void CmyArray<TYPE, ARG_TYPE>::SetAt(
	int nIndex,					/* [in] 要素ID */
	ARG_TYPE newElement)		/* [in] 新しいデータ */
{
	if (nIndex >= m_nCount) {
		goto Exit;
	}

	m_pData[nIndex] = newElement;

Exit:
	return;
}


/* ========================================================================= */
/* 関数名：	CmyArray<TYPE, ARG_TYPE>::Add									 */
/* 内容：	要素を追加														 */
/* 戻り値：	追加された要素ID												 */
/* 日付：	2005/01/18														 */
/* ========================================================================= */

template<class TYPE, class ARG_TYPE>
int CmyArray<TYPE, ARG_TYPE>::Add(
	ARG_TYPE newElement)		/* [in] 追加するデータ */
{
	TYPE *pData;

	/* 追加分の要素を確保 */
	pData = new TYPE[m_nCount + 1];

	/* 追加前のデータをコピー */
	CopyMemory (pData, m_pData, sizeof (TYPE) * m_nCount);
	pData[m_nCount] = newElement;

	if (m_pData) {
		/* 今までのデータを破棄 */
		delete [] m_pData;
	}

	m_pData = pData;
	m_nCount ++;

	/* 新しいIDを返す */
	return (m_nCount - 1);
}


/* ========================================================================= */
/* 関数名：	CmyArray<TYPE, ARG_TYPE>::InsertAt								 */
/* 内容：	指定位置に追加													 */
/* 日付：	2005/08/19														 */
/* ========================================================================= */

template<class TYPE, class ARG_TYPE>
void CmyArray<TYPE, ARG_TYPE>::InsertAt(
	int nIndex,					/* [in] 要素ID */
	ARG_TYPE newElement)		/* [in] 追加するデータ */
{
	int i, nTmp;
	TYPE *pData;

	/* 追加分の要素を確保 */
	pData = new TYPE[m_nCount + 1];

	nTmp = 0;
	/* 追加前のデータをコピー */
	for (i = 0; i < m_nCount + 1; i ++) {
		if (i == nIndex) {
			continue;
		}
		CopyMemory (&pData[i], &m_pData[nTmp], sizeof (TYPE));
		nTmp ++;
	}
	pData[nIndex] = newElement;

	if (m_pData) {
		/* 今までのデータを破棄 */
		delete [] m_pData;
	}

	m_pData = pData;
	m_nCount ++;
}


/* ========================================================================= */
/* 関数名：	CmyArray<TYPE, ARG_TYPE>::RemoveAt								 */
/* 内容：	指定要素を削除													 */
/* 日付：	2005/01/18														 */
/* ========================================================================= */

template<class TYPE, class ARG_TYPE>
void CmyArray<TYPE, ARG_TYPE>::RemoveAt(
	int nIndex,		/* [in] 要素ID */
	int nCount)		/* [in] 削除数 */
{
	int i, j, nID;
	TYPE *pData;

	/* 指定分だけ要素を削除するループ */
	for (j = 0; j < nCount; j ++) {
		/* 配列からはみ出す？ */
		if (nIndex >= m_nCount) {
			break;
		}

		/* 削除した分のデータを確保 */
		pData = new TYPE[m_nCount - 1];
		/* 指定要素以外をコピーするループ */
		for (i = 0, nID = 0; i < m_nCount; i ++) {
			if (nIndex == i) {
				continue;
			}
			pData[nID] = m_pData[i];
			nID ++;
		}
		if (m_pData) {
			delete [] m_pData;
		}
		m_pData = pData;
		m_nCount --;
	}
}


/* ========================================================================= */
/* 関数名：	CmyArray<TYPE, ARG_TYPE>::Copy									 */
/* 内容：	コピー															 */
/* 日付：	2005/06/03														 */
/* ========================================================================= */

template<class TYPE, class ARG_TYPE>
void CmyArray<TYPE, ARG_TYPE>::Copy(
	CmyArray *pSrc)		/* [in] コピー元 */
{
	int i, nCount;

	RemoveAll ();
	nCount = pSrc->GetSize ();
	for (i = 0; i < nCount; i ++) {
		Add (pSrc->GetAt (i));
	}
}


/* ========================================================================= */
/* 関数名：	CmyArray<TYPE, ARG_TYPE>::operator []							 */
/* 内容：	指定要素へ直接アクセス											 */
/* 戻り値：	要素															 */
/* 日付：	2005/01/18														 */
/* ========================================================================= */

template<class TYPE, class ARG_TYPE>
TYPE& CmyArray<TYPE, ARG_TYPE>::operator [](
	int nIndex)		/* [in] 要素ID */
{
	return m_pData[nIndex];
}

#endif
/* Copyright(C)URARA-works 2005 */
