/* Copyright(C)URARA-works 2003-2005 */
/* ========================================================================= */
/* 登録クラス名：	SBOSVR													 */
/* 概要：			共通のinclude											 */
/* 作成：			年がら年中春うらら(URARA-works)							 */
/* 作成開始日：		2003/05/02												 */
/* ========================================================================= */

#pragma once

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windowsx.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <shlwapi.h>

#include "../../common/myString.h"
#include "../../common/SboText.h"
#include "../../common/CUraraSockTCP.h"
#include "../../common/myArray.h"
#include "../../common/TextOutput.h"
#include "../../common/TypedefCommon.h"
#include "../../common/SboPacket.h"
#include "../../common/Info/InfoEfc.h"
#include "SboSvrSendCmd.h"
#include "Glbldata.h"
#include "TypedefSvr.h"

extern long genrand();				/* 乱数取得 */

/* Copyright(C)URARA-works 2003-2005 */
