/* ************************************************************************
*		File: screen.h                                      Part of CircleMUD *
*	 Usage: header file with ANSI color codes for online color              *
*																																					*
*	 All rights reserved.  See license.doc for complete information.        *
*																																					*
*	 Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*	 CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/* $Id: screen.h,v 1.3 2002/04/07 17:59:35 arcanere Exp $ */

#define	KNRM  "\x1B[0m"
#define	KRED  "\x1B[0;31m"
#define	KGRN  "\x1B[0;32m"
#define	KYEL  "\x1B[0;33m"
#define	KBLU  "\x1B[0;34m"
#define	KMAG  "\x1B[0;35m"
#define	KCYN  "\x1B[0;36m"
#define	KWHT  "\x1B[0;37m"

#define	KHNRM  "\x1B[0m"
#define	KHRED  "\x1B[1;31m"
#define	KHGRN  "\x1B[1;32m"
#define	KHYEL  "\x1B[1;33m"
#define	KHBLU  "\x1B[1;34m"
#define	KHMAG  "\x1B[1;35m"
#define	KHCYN  "\x1B[1;36m"
#define	KHWHT  "\x1B[1;37m"

#define	KNUL  ""

/* conditional color.  pass it a pointer to a char_data and a color level. */
#define	C_OFF	0
#define	C_SPR	1
#define	C_NRM	2
#define	C_CMP	3
#define	_clrlevel(ch) (!IS_NPC(ch) ? (PRF_FLAGGED((ch), PRF_COLOR_1) ? 1 : 0) + \
					 (PRF_FLAGGED((ch), PRF_COLOR_2) ? 2 : 0) : 0)
#define	clr(ch,lvl) (_clrlevel(ch) >= (lvl))
#define	CCNRM(ch,lvl)  (clr((ch),(lvl))?KNRM:KNUL)
#define	CCRED(ch,lvl)  (clr((ch),(lvl))?KRED:KNUL)
#define	CCGRN(ch,lvl)  (clr((ch),(lvl))?KGRN:KNUL)
#define	CCYEL(ch,lvl)  (clr((ch),(lvl))?KYEL:KNUL)
#define	CCBLU(ch,lvl)  (clr((ch),(lvl))?KBLU:KNUL)
#define	CCMAG(ch,lvl)  (clr((ch),(lvl))?KMAG:KNUL)
#define	CCCYN(ch,lvl)  (clr((ch),(lvl))?KCYN:KNUL)
#define	CCWHT(ch,lvl)  (clr((ch),(lvl))?KWHT:KNUL)

#define	CCHNRM(ch,lvl)  (clr((ch),(lvl))?KHNRM:KNUL)
#define	CCHRED(ch,lvl)  (clr((ch),(lvl))?KHRED:KNUL)
#define	CCHGRN(ch,lvl)  (clr((ch),(lvl))?KHGRN:KNUL)
#define	CCHYEL(ch,lvl)  (clr((ch),(lvl))?KHYEL:KNUL)
#define	CCHBLU(ch,lvl)  (clr((ch),(lvl))?KHBLU:KNUL)
#define	CCHMAG(ch,lvl)  (clr((ch),(lvl))?KHMAG:KNUL)
#define	CCHCYN(ch,lvl)  (clr((ch),(lvl))?KHCYN:KNUL)
#define	CCHWHT(ch,lvl)  (clr((ch),(lvl))?KHWHT:KNUL)

#define	COLOR_LEV(ch) (_clrlevel(ch))

#define	QNRM CCNRM(ch,C_SPR)
#define	QRED CCRED(ch,C_SPR)
#define	QGRN CCGRN(ch,C_SPR)
#define	QYEL CCYEL(ch,C_SPR)
#define	QBLU CCBLU(ch,C_SPR)
#define	QMAG CCMAG(ch,C_SPR)
#define	QCYN CCCYN(ch,C_SPR)
#define	QWHT CCWHT(ch,C_SPR)

#define	QHRED CCHRED(ch,C_SPR)
#define	QHGRN CCHGRN(ch,C_SPR)
#define	QHYEL CCHYEL(ch,C_SPR)
#define	QHBLU CCHBLU(ch,C_SPR)
#define	QHMAG CCHMAG(ch,C_SPR)
#define	QHCYN CCHCYN(ch,C_SPR)
#define	QHWHT CCHWHT(ch,C_SPR)
