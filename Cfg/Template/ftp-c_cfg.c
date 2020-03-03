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
* Filename : ftp-c_cfg.c
* Version  : V2.01.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  "ftp-c_cfg.h"


/*
*********************************************************************************************************
*                                 FTP CLIENT CONFIGURATION STRUCTURE
*********************************************************************************************************
*/

const  FTPc_CFG  FTPc_Cfg = {
                                            /* ----------- CTRL CONNECTION CONFIGURATION ------------- */
    5000u,                                  /* Maximum inactivity time (ms) on CONNECT.                */
    5000u,                                  /* Maximum inactivity time (ms) on RX.                     */
    5000u,                                  /* Maximum inactivity time (ms) on TX.                     */
     100u,                                  /* Delay between each retries on RX.                       */
       3u,                                  /* Maximum number of retries on TX.                        */
     100u,                                  /* Delay between each retries on TX.                       */

                                            /* ------------ DTP CONNECTION CONFIGURATION ------------- */
    5000u,                                  /* Maximum inactivity time (ms) on CONNECT.                */
    5000u,                                  /* Maximum inactivity time (ms) on RX.                     */
    5000u,                                  /* Maximum inactivity time (ms) on TX.                     */
       3u,                                  /* Maximum number of retries on TX.                        */
     100u                                   /* Delay between each retries on TX.                       */
};
