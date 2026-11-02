/* Copyright(C)URARA-works 2005 */
/* ========================================================================= */
/* ファイル名：	myArray.h													 */
/* 内容：		MFCのCArrayみたいな配列クラス 定義ファイル					 */
/* 作成：		年がら年中春うらら(URARA-works)								 */
/* 作成開始日：	2005/01/18													 */
/* ========================================================================= */

#ifndef MYARRAY_H
#define MYARRAY_H

/* ========================================================================= */
/* クラス宣言																 */
/* ========================================================================= */
template<class TYPE, class ARG_TYPE> class CmyArray
{
public:
			CmyArray();									/* コンストラクタ */
	virtual ~CmyArray();								/* デストラクタ */

	int		GetSize		(void);										/* 要素数を取得 */
	void	RemoveAll	(void);										/* 全要素を削除 */
	TYPE	GetAt		(int nIndex);								/* 指定要素を取得 */
	void	SetAt		(int nIndex, ARG_TYPE newElement);			/* 指定要素を更新 */
	int		Add			(ARG_TYPE newElement);						/* 要素を追加 */
	void	InsertAt	(int nIndex, ARG_TYPE newElement);			/* 指定位置に追加 */
	void	RemoveAt	(int nIndex, int nCount = 1);				/* 指定要素を削除 */
	void	Copy		(CmyArray *pSrc);							/* コピー */

	TYPE& operator [](int nIndex);								/* 指定要素へ直接アクセス */


private:
	TYPE		*m_pData;					/* データ */
	int			m_nCount;					/* 要素数 */
};

typedef CmyArray<DWORD, DWORD>		  ARRAYDWORD;
typedef CmyArray<DWORD, DWORD>		*PARRAYDWORD;

/* 実装部分をインクルード */
#include "myArrayImpl.h"

#endif
/* Copyright(C)URARA-works 2005 */
