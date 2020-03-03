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
*                                    FTP CLIENT CONFIGURATION FILE
*
*                                              TEMPLATE
*
* Filename : ftp-c_cfg.h
* Version  : V2.01.00
*********************************************************************************************************
*/

#ifndef  FTPc_CFG_MODULE_PRESENT
#define  FTPc_CFG_MODULE_PRESENT

#include  <Source/ftp-c_type.h>


/*
*********************************************************************************************************
*                                     FTPc CONFIGURATION DEFINES
*********************************************************************************************************
*/
                                                                /* ENABLED/DISABLE File System usage.                   */
#define  FTPc_CFG_USE_FS                                 DEF_ENABLED
                                                                /* DEF_DISABLED  Functions using FS DISABLED            */
                                                                /* DEF_ENABLED   Functions using FS ENABLED             */


/*
*********************************************************************************************************
*                                               TRACING
*********************************************************************************************************
*/

#ifndef  TRACE_LEVEL_OFF
#define  TRACE_LEVEL_OFF                                   0
#endif

#ifndef  TRACE_LEVEL_INFO
#define  TRACE_LEVEL_INFO                                  1
#endif

#ifndef  TRACE_LEVEL_DBG
#define  TRACE_LEVEL_DBG                                   2
#endif

#define  FTPc_TRACE_LEVEL                       TRACE_LEVEL_INFO
#define  FTPc_TRACE                             printf

#endif
