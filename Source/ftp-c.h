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
* Filename : ftp-c.h
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

#ifndef  FTPc_MODULE_PRESENT                                    /* See Note #1.                                         */
#define  FTPc_MODULE_PRESENT


/*
*********************************************************************************************************
*                                         FTPc VERSION NUMBER
*
* Note(s) : (1) (a) The FTPc module software version is denoted as follows :
*
*                       Vx.yy.zz
*
*                           where
*                                   V               denotes 'Version' label
*                                   x               denotes     major software version revision number
*                                   yy              denotes     minor software version revision number
*                                   zz              denotes sub-minor software version revision number
*
*               (b) The FTPc software version label #define is formatted as follows :
*
*                       ver = x.yyzz * 100 * 100
*
*                           where
*                                   ver             denotes software version number scaled as an integer value
*                                   x.yyzz          denotes software version number, where the unscaled integer
*                                                       portion denotes the major version number & the unscaled
*                                                       fractional portion denotes the (concatenated) minor
*                                                       version numbers
*********************************************************************************************************
*/

#define  FTPc_VERSION                                  20100u   /* See Note #1.                                         */


/*
*********************************************************************************************************
*                                               EXTERNS
*********************************************************************************************************
*/

#ifdef   FTPc_MODULE
#define  FTPc_EXT
#else
#define  FTPc_EXT  extern
#endif


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*
* Note(s) : (1) The FTPc module files are located in the following directories :
*
*               (a) \<Your Product Application>\ftp-c_cfg.h
*
*               (b) \<FTPc>\Source\ftp-c.h
*                                 \ftp-c.c
*
*           (2) CPU-configuration software files are located in the following directories :
*
*               (a) \<CPU-Compiler Directory>\cpu_*.*
*               (b) \<CPU-Compiler Directory>\<cpu>\<compiler>\cpu*.*
*
*                       where
*                               <CPU-Compiler Directory>        directory path for common CPU-compiler software
*                               <cpu>                           directory name for specific processor (CPU)
*                               <compiler>                      directory name for specific compiler
*
*           (3) NO compiler-supplied standard library functions SHOULD be used.
*
*               (a) Standard library functions are implemented in the custom library module(s) :
*
*                       \<Custom Library Directory>\lib_*.*
*
*                           where
*                                   <Custom Library Directory>      directory path for custom library software
*
*               (b) #### The reference to standard library header files SHOULD be removed once all custom
*                   library functions are implemented WITHOUT reference to ANY standard library function(s).
*
*           (4) Compiler MUST be configured to include as additional include path directories :
*
*               (a) '\<Your Product Application>\'                                          See Note #1a
*
*               (b) (1) '\<Network Protocol Suite>\'
*
*                   (2) (A) '\<Network Protocol Suite>\Secure\<Network Security Suite>\'
*
*               (c) '\<FTPc>\' directories                                                  See Note #1b
*
*               (d) (1) '\<CPU-Compiler Directory>\'                                        See Note #2a
*                   (2) '\<CPU-Compiler Directory>\<cpu>\<compiler>\'                       See Note #2b
*
*               (e) '\<Custom Library Directory>\'                                          See Note #3a
*
*********************************************************************************************************
*/

#include  <cpu.h>                                               /* CPU Configuration              (see Note #2b)        */
#include  <cpu_core.h>                                          /* CPU Core Library               (see Note #2a)        */

#include  <lib_def.h>                                           /* Standard        Defines        (see Note #3a)        */
#include  <lib_str.h>                                           /* Standard String Library        (see Note #3a)        */

#include  <ftp-c_cfg.h>                                         /* FTP Client Configuration File  (see Note #1a)        */
#if (FTPc_CFG_USE_FS > 0)
#include  <FS/net_fs.h>                                         /* File System Interface          (see Note #1b)        */
#endif

#include  <Source/net_sock.h>
#include  <Source/net_cfg_net.h>

#if 1                                                           /* See Note #3b.                                        */
#include  <stdio.h>
#endif

#include  "ftp-c_type.h"


/*
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                        FTPc SPECIFIC DEFINES
*
* Note(s) : (1) The size of FTPc_CTRL_NET_BUF_SIZE must be large enough to accommodate a complete
*               reply line. However, it is possible for the length of a server's welcome message to
*               span multiple buffer sizes.
*********************************************************************************************************
*/

#define  FTPc_CTRL_NET_BUF_SIZE                         1460    /* Ctrl buffer size.                                    */
#define  FTPc_DTP_NET_BUF_SIZE                          1460    /* Dtp buffer size.                                     */


/*
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      FTP ERROR CODES DATA TYPE
*********************************************************************************************************
*/

typedef enum ftpc_err {
    FTPc_ERR_NONE = 0,
    FTPc_ERR_FAULT,
    FTPc_ERR_FAULT_NULL_PTR,
    FTPc_ERR_SECURE_NOT_AVAIL,
    FTPc_ERR_INVALID_IP_FAMILY,

    FTPc_ERR_CONN_FAIL,
    FTPc_ERR_TX_CMD,

    FTPc_ERR_RX_CMD_RESP_FAIL,
    FTPc_ERR_RX_CMD_RESP_INVALID,

    FTPc_ERR_LOGGEDIN,

    FTPc_ERR_DTP_SOCK_OPEN,

    FTPc_ERR_FILE_NOT_FOUND,
    FTPc_ERR_FILE_OPEN_FAIL,

    FTPc_ERR_FILE_BUF_LEN
} FTPc_ERR;


/*
*********************************************************************************************************
*                                      FTP SECURE CFG DATA TYPE
*********************************************************************************************************
*/

typedef  struct  ftpc_secure_cfg {
    CPU_CHAR                    *CommonName;
    NET_SOCK_SECURE_TRUST_FNCT   TrustCallback;
} FTPc_SECURE_CFG;


/*
*********************************************************************************************************
*                                    FTP CONNECTION CFG DATA TYPE
*********************************************************************************************************
*/

typedef  struct  ftpc_conn {
           NET_SOCK_ID         SockID;
           NET_SOCK_ADDR       SockAddr;
           NET_IP_ADDR_FAMILY  SockAddrFamily;
#ifdef  NET_SECURE_MODULE_EN
    const  FTPc_SECURE_CFG    *SecureCfgPtr;
#endif
           CPU_INT08U          Buf[FTPc_CTRL_NET_BUF_SIZE];
} FTPc_CONN;


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

CPU_BOOLEAN  FTPc_Open    (      FTPc_CONN        *p_conn,
                           const FTPc_CFG         *p_cfg,
                           const FTPc_SECURE_CFG  *p_secure_cfg,
                                 CPU_CHAR         *p_host_server,
                                 NET_PORT_NBR      port_nbr,
                                 CPU_CHAR         *p_user,
                                 CPU_CHAR         *p_pass,
                                 FTPc_ERR         *p_err);

CPU_BOOLEAN  FTPc_Close   (      FTPc_CONN        *p_conn,
                                 FTPc_ERR         *p_err);

CPU_BOOLEAN  FTPc_RecvBuf (      FTPc_CONN        *p_conn,
                                 CPU_CHAR         *p_remote_file_name,
                                 CPU_INT08U       *p_buf,
                                 CPU_INT32U        buf_len,
                                 CPU_INT32U       *p_file_size,
                                 FTPc_ERR         *p_err);

CPU_BOOLEAN  FTPc_SendBuf (      FTPc_CONN        *p_conn,
                                 CPU_CHAR         *p_remote_file_name,
                                 CPU_INT08U       *p_buf,
                                 CPU_INT32U        buf_len,
                                 CPU_BOOLEAN       append,
                                 FTPc_ERR         *p_err);

CPU_BOOLEAN  FTPc_RecvFile(      FTPc_CONN        *p_conn,
                                 CPU_CHAR         *p_remote_file_name,
                                 CPU_CHAR         *p_local_file_name,
                                 FTPc_ERR         *p_err);

CPU_BOOLEAN  FTPc_SendFile(      FTPc_CONN        *p_conn,
                                 CPU_CHAR         *p_remote_file_name,
                                 CPU_CHAR         *p_local_file_name,
                                 CPU_BOOLEAN       append,
                                 FTPc_ERR         *p_net);


/*
*********************************************************************************************************
*                                               TRACING
*********************************************************************************************************
*/

                                                                /* Trace level, default to TRACE_LEVEL_OFF              */
#ifndef  TRACE_LEVEL_OFF
#define  TRACE_LEVEL_OFF                                 0
#endif

#ifndef  TRACE_LEVEL_INFO
#define  TRACE_LEVEL_INFO                                1
#endif

#ifndef  TRACE_LEVEL_DBG
#define  TRACE_LEVEL_DBG                                 2
#endif

#ifndef  FTPc_TRACE_LEVEL
#define  FTPc_TRACE_LEVEL                       TRACE_LEVEL_DBG
#endif

#ifndef  FTPc_TRACE
#define  FTPc_TRACE                             printf
#endif

#if    ((defined(FTPc_TRACE))       && \
        (defined(FTPc_TRACE_LEVEL)) && \
        (FTPc_TRACE_LEVEL >= TRACE_LEVEL_INFO) )

    #if  (FTPc_TRACE_LEVEL >= TRACE_LEVEL_DBG)
        #define  FTPc_TRACE_DBG(msg)     FTPc_TRACE  msg
    #else
        #define  FTPc_TRACE_DBG(msg)
    #endif

    #define  FTPc_TRACE_INFO(msg)        FTPc_TRACE  msg

#else
    #define  FTPc_TRACE_DBG(msg)
    #define  FTPc_TRACE_INFO(msg)
#endif


/*
*********************************************************************************************************
*                                        CONFIGURATION ERRORS
*********************************************************************************************************
*/

                                                    /* If DEF_ENABLED, interface functions using FS are enabled.        */
                                                    /* If DEF_DISABLED, only functions using RAM buffers are enabled.   */
#ifndef  FTPc_CFG_USE_FS
#error  "FTPc_CFG_USE_FS not #define'd in 'ftp-c_cfg.h' see template file in package named 'ftp-c_cfg.h'"
#elif  ((FTPc_CFG_USE_FS != DEF_DISABLED) && \
        (FTPc_CFG_USE_FS != DEF_ENABLED ))
#error  "FTPc_CFG_USE_FS  illegally #define'd in 'net_cfg.h' [MUST be DEF_DISABLED || DEF_ENABLED ]"
#endif


/*
*********************************************************************************************************
*                                    NETWORK CONFIGURATION ERRORS
*********************************************************************************************************
*/

#ifndef NET_TCP_MODULE_EN
#error  "NET_TCP_CFG_EN illegally #define'd in 'net_cfg.h' [MUST be DEF_ENABLED]"
#endif


/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

#endif      /* FTPc_MODULE_PRESENT  */
