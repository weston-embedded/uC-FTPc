/*
*********************************************************************************************************
*                                               uC/FTPc
*                                       The Embedded FTP Client
*
*                    Copyright 2004-2020 Silicon Laboratories Inc. www.silabs.com
*
*                                 SPDX-License-Identifier: APACHE-2.0
*
*               This software is subject to an open source license and is distributed by
*                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
*                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                             FTP CLIENT
*
* Filename : ftp-c_type.h
* Version  : V2.01.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               MODULE
*
* Note(s) : (1) This header file is protected from multiple pre-processor inclusion through use of the
*               FTPc present pre-processor macro definition.
*********************************************************************************************************
*/

#ifndef  FTPc_TYPE_MODULE_PRESENT                               /* See Note #1.                                         */
#define  FTPc_TYPE_MODULE_PRESENT


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <cpu.h>


/*
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                    FTPc CONFIGURATION DATA TYPE
*********************************************************************************************************
*/

typedef  struct  ftpc_cfg {
    CPU_INT32U  CtrlConnMaxTimout_ms;
    CPU_INT32U  CtrlRxMaxTimout_ms;
    CPU_INT32U  CtrlTxMaxTimout_ms;

    CPU_INT32U  CtrlRxMaxDly_ms;
    CPU_INT32U  CtrlRxMaxReplyLength;

    CPU_INT32U  CtrlTxMaxRetry;
    CPU_INT32U  CtrlTxMaxDly_ms;

    CPU_INT32U  DTP_ConnMaxTimout_ms;
    CPU_INT32U  DTP_RxMaxTimout_ms;
    CPU_INT32U  DTP_TxMaxTimout_ms;

    CPU_INT32U  DTP_TxMaxRetry;
    CPU_INT32U  DTP_TxMaxDly_ms;
} FTPc_CFG;


/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

#endif      /* FTPc_TYPE_MODULE_PRESENT  */
