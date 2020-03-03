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
* Filename : ftp-c.c
* Version  : V2.01.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#define    FTPc_MODULE

#include  "ftp-c.h"
#include  <KAL/kal.h>
#include  <FS/net_fs.h>

#include  <Source/net_err.h>
#include  <Source/net_tcp.h>
#include  <Source/net_tmr.h>
#include  <Source/net_conn.h>
#include  <Source/net_app.h>
#include  <Source/net_sock.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                    DEFAULT CONFIGURATION DEFINES
*********************************************************************************************************
*/

#ifndef FTPc_CFG_DFLT_CTRL_MAX_CONN_TIMEOUT_MS
    #define  FTPc_CFG_DFLT_CTRL_MAX_CONN_TIMEOUT_MS    5000u    /* Maximum inactivity time (ms) on CONNECT.             */
#endif

#ifndef FTPc_CFG_DFLT_CTRL_MAX_RX_TIMEOUT_MS
    #define  FTPc_CFG_DFLT_CTRL_MAX_RX_TIMEOUT_MS      5000u    /* Maximum inactivity time (ms) on RX.                  */
#endif

#ifndef FTPc_CFG_DFLT_CTRL_MAX_TX_TIMEOUT_MS
    #define  FTPc_CFG_DFLT_CTRL_MAX_TX_TIMEOUT_MS      5000u    /* Maximum inactivity time (ms) on TX.                  */
#endif

#ifndef FTPc_CFG_DFLT_CTRL_MAX_RX_DLY_MS
    #define  FTPc_CFG_DFLT_CTRL_MAX_RX_DLY_MS           100u    /* Delay between each retries on RX.                    */
#endif

#ifndef FTPc_CFG_DFLT_CTRL_MAX_RX_REPLY_LEN
    #define  FTPc_CFG_DFLT_CTRL_MAX_RX_REPLY_LEN       9196u    /* Max server reply size if lengthy welcome msg is rx'd.*/
#endif

#ifndef FTPc_CFG_DFLT_CTRL_MAX_TX_RETRY
    #define  FTPc_CFG_DFLT_CTRL_MAX_TX_RETRY              3u    /* Maximum number of retries on TX.                     */
#endif

#ifndef FTPc_CFG_DFLT_CTRL_MAX_TX_DLY_MS
    #define  FTPc_CFG_DFLT_CTRL_MAX_TX_DLY_MS           100u    /* Delay between each retries on TX.                    */
#endif

#ifndef FTPc_CFG_DFLT_DTP_MAX_CONN_TIMEOUT_MS
    #define  FTPc_CFG_DFLT_DTP_MAX_CONN_TIMEOUT_MS     5000u    /* Maximum inactivity time (ms) on CONNECT.             */
#endif

#ifndef FTPc_CFG_DFLT_DTP_MAX_RX_TIMEOUT_MS
    #define  FTPc_CFG_DFLT_DTP_MAX_RX_TIMEOUT_MS       5000u    /* Maximum inactivity time (ms) on RX.                  */
#endif

#ifndef FTPc_CFG_DFLT_DTP_MAX_TX_TIMEOUT_MS
    #define  FTPc_CFG_DFLT_DTP_MAX_TX_TIMEOUT_MS       5000u    /* Maximum inactivity time (ms) on TX.                  */
#endif

#ifndef FTPc_CFG_DFLT_DTP_MAX_TX_RETRY
    #define  FTPc_CFG_DFLT_DTP_MAX_TX_RETRY               3u    /* Maximum number of retries on TX.                     */
#endif

#ifndef FTPc_CFG_DFLT_DTP_MAX_TX_DLY_MS
    #define  FTPc_CFG_DFLT_DTP_MAX_TX_DLY_MS            100u    /* Delay between each retries on TX.                    */
#endif


/*
*********************************************************************************************************
*                                            FTP COMMANDS
*********************************************************************************************************
*/

#define  FTP_CMD_NOOP                                      0
#define  FTP_CMD_QUIT                                      1
#define  FTP_CMD_REIN                                      2
#define  FTP_CMD_SYST                                      3
#define  FTP_CMD_FEAT                                      4
#define  FTP_CMD_HELP                                      5
#define  FTP_CMD_USER                                      6
#define  FTP_CMD_PASS                                      7
#define  FTP_CMD_MODE                                      8
#define  FTP_CMD_TYPE                                      9
#define  FTP_CMD_STRU                                     10
#define  FTP_CMD_PASV                                     11
#define  FTP_CMD_PORT                                     12
#define  FTP_CMD_PWD                                      13
#define  FTP_CMD_CWD                                      14
#define  FTP_CMD_CDUP                                     15
#define  FTP_CMD_MKD                                      16
#define  FTP_CMD_RMD                                      17
#define  FTP_CMD_NLST                                     18
#define  FTP_CMD_LIST                                     19
#define  FTP_CMD_RETR                                     20
#define  FTP_CMD_STOR                                     21
#define  FTP_CMD_APPE                                     22
#define  FTP_CMD_REST                                     23
#define  FTP_CMD_DELE                                     24
#define  FTP_CMD_RNFR                                     25
#define  FTP_CMD_RNTO                                     26
#define  FTP_CMD_SIZE                                     27
#define  FTP_CMD_MDTM                                     28
#define  FTP_CMD_PBSZ                                     29
#define  FTP_CMD_PROT                                     30
#define  FTP_CMD_EPSV                                     31
#define  FTP_CMD_EPRT                                     32
#define  FTP_CMD_MAX                                      33    /* This line MUST be the LAST!                          */


/*
*********************************************************************************************************
*                                         FTP REPLY MESSAGES
*********************************************************************************************************
*/

#define  FTP_REPLY_CODE_ALREADYOPEN                      125
#define  FTP_REPLY_CODE_OKAYOPENING                      150
#define  FTP_REPLY_CODE_OKAY                             200
#define  FTP_REPLY_CODE_SYSTEMSTATUS                     211
#define  FTP_REPLY_CODE_FILESTATUS                       213
#define  FTP_REPLY_CODE_HELPMESSAGE                      214
#define  FTP_REPLY_CODE_SYSTEMTYPE                       215
#define  FTP_REPLY_CODE_SERVERREADY                      220
#define  FTP_REPLY_CODE_SERVERCLOSING                    221
#define  FTP_REPLY_CODE_CLOSINGSUCCESS                   226
#define  FTP_REPLY_CODE_ENTERPASVMODE                    227
#define  FTP_REPLY_CODE_ENTEREXTPASVMODE                 229
#define  FTP_REPLY_CODE_LOGGEDIN                         230
#define  FTP_REPLY_CODE_ACTIONCOMPLETE                   250
#define  FTP_REPLY_CODE_PATHNAME                         257
#define  FTP_REPLY_CODE_NEEDPASSWORD                     331
#define  FTP_REPLY_CODE_NEEDMOREINFO                     350
#define  FTP_REPLY_CODE_NOSERVICE                        421
#define  FTP_REPLY_CODE_CANTOPENDATA                     425
#define  FTP_REPLY_CODE_CLOSEDCONNABORT                  426
#define  FTP_REPLY_CODE_PARMSYNTAXERR                    501
#define  FTP_REPLY_CODE_CMDNOSUPPORT                     502
#define  FTP_REPLY_CODE_CMDBADSEQUENCE                   503
#define  FTP_REPLY_CODE_PARMNOSUPPORT                    504
#define  FTP_REPLY_CODE_NOTLOGGEDIN                      530
#define  FTP_REPLY_CODE_NOTFOUND                         550
#define  FTP_REPLY_CODE_ACTIONABORTED                    551
#define  FTP_REPLY_CODE_NOSPACE                          552
#define  FTP_REPLY_CODE_NAMEERR                          553
#define  FTP_REPLY_CODE_PBSZ                             554
#define  FTP_REPLY_CODE_PROT                             555

#define  FTP_REPLY_CODE_LEN                                3
#define  FTP_REPLY_CODE_MULTI_LINE_INDICATOR             '-'


/*
*********************************************************************************************************
*                                          FTP USASCII CODES
*********************************************************************************************************
*/

#define  FTP_ASCII_LF                                    '\n'   /* Line feed.                                           */
#define  FTP_ASCII_CR                                    '\r'   /* Carriage return.                                     */
#define  FTP_ASCII_SPACE                                 ' '    /* Space.                                               */

#define  FTP_ASCII_EPSV_PREFIX                           "|||"
#define  FTP_MULTI_REPLY_DELIM                           "/"    /* Indicates multiple FTP server replies in resp. buf.  */

#define  FTP_EOL_DELIMITER_LEN                            2     /* EOL delimiter is CRLF.                               */


/*
*********************************************************************************************************
*                                            FTP DATA TYPE
*********************************************************************************************************
*/

                                                                /* Data type "IMAGE" supported only.                    */
#define  FTP_TYPE_ASCII                                  'A'
#define  FTP_TYPE_EBCDIC                                 'E'
#define  FTP_TYPE_IMAGE                                  'I'
#define  FTP_TYPE_LOCAL                                  'L'


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/

                                                                /* This structure is used to build a table of command   */
                                                                /* codes and their corresponding string.                */
typedef struct  FTPc_CmdStruct {
           CPU_INT08U   CmdCode;
    const  CPU_CHAR    *CmdStr;
}  FTPc_CMD_STRUCT;


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      FTP CLIENT CONFIGURATION
*********************************************************************************************************
*/

static  const  FTPc_CFG  *FTPc_CfgPtr;

static  const  FTPc_CFG   FTPc_DfltCfg = {

    FTPc_CFG_DFLT_CTRL_MAX_CONN_TIMEOUT_MS,
    FTPc_CFG_DFLT_CTRL_MAX_RX_TIMEOUT_MS,
    FTPc_CFG_DFLT_CTRL_MAX_TX_TIMEOUT_MS,

    FTPc_CFG_DFLT_CTRL_MAX_RX_DLY_MS,
    FTPc_CFG_DFLT_CTRL_MAX_RX_REPLY_LEN,

    FTPc_CFG_DFLT_CTRL_MAX_TX_RETRY,
    FTPc_CFG_DFLT_CTRL_MAX_TX_DLY_MS,

    FTPc_CFG_DFLT_DTP_MAX_CONN_TIMEOUT_MS,
    FTPc_CFG_DFLT_DTP_MAX_RX_TIMEOUT_MS,
    FTPc_CFG_DFLT_DTP_MAX_TX_TIMEOUT_MS,

    FTPc_CFG_DFLT_DTP_MAX_TX_RETRY,
    FTPc_CFG_DFLT_DTP_MAX_TX_DLY_MS
};


/*
*********************************************************************************************************
*                                          INITIALIZED DATA
*********************************************************************************************************
*/

                                                                /* This structure is used to build a table of command   */
                                                                /* codes and their corresponding string.  The context   */
                                                                /* is the state(s) in which the command is allowed.     */
static  const  FTPc_CMD_STRUCT  FTPc_Cmd[] = {
    { FTP_CMD_NOOP,  (const  CPU_CHAR *)"NOOP" },
    { FTP_CMD_QUIT,  (const  CPU_CHAR *)"QUIT" },
    { FTP_CMD_REIN,  (const  CPU_CHAR *)"REIN" },
    { FTP_CMD_SYST,  (const  CPU_CHAR *)"SYST" },
    { FTP_CMD_FEAT,  (const  CPU_CHAR *)"FEAT" },
    { FTP_CMD_HELP,  (const  CPU_CHAR *)"HELP" },
    { FTP_CMD_USER,  (const  CPU_CHAR *)"USER" },
    { FTP_CMD_PASS,  (const  CPU_CHAR *)"PASS" },
    { FTP_CMD_MODE,  (const  CPU_CHAR *)"MODE" },
    { FTP_CMD_TYPE,  (const  CPU_CHAR *)"TYPE" },
    { FTP_CMD_STRU,  (const  CPU_CHAR *)"STRU" },
    { FTP_CMD_PASV,  (const  CPU_CHAR *)"PASV" },
    { FTP_CMD_PORT,  (const  CPU_CHAR *)"PORT" },
    { FTP_CMD_PWD,   (const  CPU_CHAR *)"PWD"  },
    { FTP_CMD_CWD,   (const  CPU_CHAR *)"CWD"  },
    { FTP_CMD_CDUP,  (const  CPU_CHAR *)"CDUP" },
    { FTP_CMD_MKD,   (const  CPU_CHAR *)"MKD"  },
    { FTP_CMD_RMD,   (const  CPU_CHAR *)"RMD"  },
    { FTP_CMD_NLST,  (const  CPU_CHAR *)"NLST" },
    { FTP_CMD_LIST,  (const  CPU_CHAR *)"LIST" },
    { FTP_CMD_RETR,  (const  CPU_CHAR *)"RETR" },
    { FTP_CMD_STOR,  (const  CPU_CHAR *)"STOR" },
    { FTP_CMD_APPE,  (const  CPU_CHAR *)"APPE" },
    { FTP_CMD_REST,  (const  CPU_CHAR *)"REST" },
    { FTP_CMD_DELE,  (const  CPU_CHAR *)"DELE" },
    { FTP_CMD_RNFR,  (const  CPU_CHAR *)"RNFR" },
    { FTP_CMD_RNTO,  (const  CPU_CHAR *)"RNTO" },
    { FTP_CMD_SIZE,  (const  CPU_CHAR *)"SIZE" },
    { FTP_CMD_MDTM,  (const  CPU_CHAR *)"MDTM" },
    { FTP_CMD_PBSZ,  (const  CPU_CHAR *)"PBSZ" },
    { FTP_CMD_PROT,  (const  CPU_CHAR *)"PROT" },
    { FTP_CMD_PASV,  (const  CPU_CHAR *)"EPSV" },
    { FTP_CMD_EPRT,  (const  CPU_CHAR *)"EPRT" },
    { FTP_CMD_MAX,   (const  CPU_CHAR *)"MAX"  }                /* This line MUST be the LAST!                          */
};


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  CPU_INT32U  FTPc_WaitForStatus(FTPc_CONN   *p_conn,
                                       CPU_CHAR    *p_ctrl_data,
                                       CPU_INT16U   ctrl_data_len,
                                       NET_ERR     *p_err);

static  CPU_INT32S  FTPc_RxReply      (CPU_INT32S   sock_id,
                                       CPU_CHAR    *p_data,
                                       CPU_INT16U   data_len,
                                       NET_ERR     *p_err);

static  CPU_BOOLEAN  FTPc_Tx          (CPU_INT32S   sock_id,
                                       CPU_CHAR    *p_data,
                                       CPU_INT16U   data_len,
                                       CPU_INT32U   timeout_ms,
                                       CPU_INT16U   retry_max,
                                       CPU_INT32U   time_dly_ms,
                                       NET_ERR     *p_err);

static  NET_SOCK_ID  FTPc_Conn        (FTPc_CONN   *p_conn,
                                       CPU_CHAR    *p_ctrl_buf,
                                       CPU_INT16U   ctrl_buf_size,
                                       FTPc_ERR    *p_err);


/*
*********************************************************************************************************
*                                              FTPc_Open()
*
* Description : Open connection to an FTP server.
*
* Argument(s) : p_conn          Pointer to FTP Client Connection object.
*
*               p_cfg           Pointer to FTPc Configuration object.
*                               DEF_NULL to use internal default configuration.
*
*               p_secure_cfg    Pointer to a secure configuration structure if secured connection is     required or
*                               DEF_NULL                                    if secured connection is not required.
*
*               p_host_server   Pointer to hostname/IP address string of the server.
*
*               port_nbr        IP port of the server.
*
*               p_user          Pointer to account username on the server.
*
*               p_pass          Pointer to account password on the server.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   TFTPc_ERR_NONE              Open Connection to server succeeded.
*                                   FTPc_ERR_CONN_FAIL          Open Connection failed.
*                                   FTPc_ERR_TX_CMD             Sending Command failed.
*                                   FTPc_ERR_RX_CMD_RESP_FAIL   Receiving Command response failed.
*                                   FTPc_ERR_LOGGEDIN           Log in failed.
*
* Return(s)   : DEF_FAIL        connection failed.
*               DEF_OK          connection successful.
*
* Caller(s)   : Application.
*
* Note(s)     :  (1) Network security module MUST be available & enabled to open a secure FTP
*                    connection.
*
*                (2) If the secure mode is enabled, the client MUST send a PBSZ & PROT commands.
*********************************************************************************************************
*/

CPU_BOOLEAN  FTPc_Open (      FTPc_CONN        *p_conn,
                        const FTPc_CFG         *p_cfg,
                        const FTPc_SECURE_CFG  *p_secure_cfg,
                              CPU_CHAR         *p_host_server,
                              NET_PORT_NBR      port_nbr,
                              CPU_CHAR         *p_user,
                              CPU_CHAR         *p_pass,
                              FTPc_ERR         *p_err)
{
    CPU_CHAR     ctrl_buf[FTPc_CTRL_NET_BUF_SIZE];
    CPU_INT32U   ctrl_buf_size;
    CPU_INT32U   buf_size;
    CPU_INT32U   reply_code;
    CPU_BOOLEAN  rtn_code;
    NET_ERR      err;


    if (p_cfg == DEF_NULL) {
        p_cfg       = &FTPc_DfltCfg;
        FTPc_CfgPtr =  p_cfg;
    } else {
        FTPc_CfgPtr =  p_cfg;
    }

#ifdef  NET_SECURE_MODULE_EN
    p_conn->SecureCfgPtr = p_secure_cfg;
#endif

    ctrl_buf_size = sizeof(ctrl_buf);

    p_conn->SockAddrFamily = NetApp_ClientStreamOpenByHostname(&p_conn->SockID,
                                                                p_host_server,
                                                                port_nbr,
                                                               &p_conn->SockAddr,
                                     (NET_APP_SOCK_SECURE_CFG *)p_secure_cfg,
                                                                p_cfg->CtrlConnMaxTimout_ms,
                                                               &err);
    switch (err) {
        case NET_APP_ERR_NONE:
             break;

        default:
            *p_err = FTPc_ERR_CONN_FAIL;
             FTPc_TRACE_DBG(("FTPc NetSock_Open() failed: error #%u, line #%u.\n", (unsigned int)*p_err, (unsigned int)__LINE__));
             rtn_code = DEF_FAIL;
             goto exit;
    }

                                                                /* ---------------- CFG SOCK BLOCK OPT ---------------- */
    (void)NetSock_CfgBlock(p_conn->SockID, NET_SOCK_BLOCK_SEL_BLOCK, &err);
    if (err != NET_SOCK_ERR_NONE) {
       *p_err = FTPc_ERR_CONN_FAIL;
        goto exit_close_sock;
    }

                                                                /* Receive status lines until "server ready status".    */
    while (DEF_OK) {
        reply_code = FTPc_WaitForStatus(p_conn, 0, 0, &err);
        if ((reply_code != FTP_REPLY_CODE_SERVERREADY) &&
            (reply_code != 0u)) {
            continue;
        }
        break;
    }
    
    if (reply_code == 0u) {                                     /* Server reply is either malformed OR too long.        */
        goto exit_close_sock;
    }


                                                                /* ******** USERNAME ************************************/
                                                                /* Send USER command.                                   */
    buf_size = Str_FmtPrint((char *)ctrl_buf, ctrl_buf_size, "%s %s\r\n", FTPc_Cmd[FTP_CMD_USER].CmdStr, p_user);
    rtn_code = FTPc_Tx(p_conn->SockID,
                       ctrl_buf,
                       buf_size,
                       p_cfg->CtrlTxMaxTimout_ms,
                       p_cfg->CtrlTxMaxRetry,
                       p_cfg->CtrlTxMaxDly_ms,
                      &err);
    FTPc_TRACE_INFO(("FTPc TX: %s", ctrl_buf));
    if (rtn_code == DEF_FAIL) {
       *p_err    = FTPc_ERR_TX_CMD;
        goto exit_close_sock;
    }

                                                                /* Receive status line.                                 */
    reply_code = FTPc_WaitForStatus(p_conn, 0, 0, &err);
    if (reply_code != FTP_REPLY_CODE_NEEDPASSWORD) {
       *p_err = FTPc_ERR_RX_CMD_RESP_FAIL;
        goto exit_close_sock;
    }

                                                                /* ******** PASSWORD ************************************/
                                                                /* Send PASS command.                                   */
    buf_size = Str_FmtPrint((char *)ctrl_buf, ctrl_buf_size, "%s %s\r\n", FTPc_Cmd[FTP_CMD_PASS].CmdStr, p_pass);
    rtn_code = FTPc_Tx(p_conn->SockID,
                       ctrl_buf,
                       buf_size,
                       p_cfg->CtrlTxMaxTimout_ms,
                       p_cfg->CtrlTxMaxRetry,
                       p_cfg->CtrlTxMaxDly_ms,
                      &err);

    FTPc_TRACE_INFO(("FTPc TX: PASS ******"));
    if (rtn_code == DEF_FAIL) {
        *p_err = FTPc_ERR_TX_CMD;
         goto exit_close_sock;
    }

                                                                /* Receive status line.                                 */
    reply_code = FTPc_WaitForStatus(p_conn, 0, 0, &err);
    if (reply_code != FTP_REPLY_CODE_LOGGEDIN) {
        *p_err = FTPc_ERR_LOGGEDIN;
         goto exit_close_sock;
    }

#ifdef  NET_SECURE_MODULE_EN
    if (p_conn->SecureCfgPtr != DEF_NULL) {                    /* See Note #2.                                          */
                                                               /* Send PBSZ command.                                    */
        buf_size = Str_FmtPrint((char *)ctrl_buf, ctrl_buf_size, "%s %s\r\n", FTPc_Cmd[FTP_CMD_PBSZ].CmdStr, (CPU_CHAR *)"0");
        rtn_code = FTPc_Tx(p_conn->SockID,
                           ctrl_buf,
                           buf_size,
                           p_cfg->CtrlTxMaxTimout_ms,
                           p_cfg->CtrlTxMaxRetry,
                           p_cfg->CtrlTxMaxDly_ms,
                           &err);
        FTPc_TRACE_INFO(("FTPc TX: %s", ctrl_buf));
        if (rtn_code == DEF_FAIL) {
            *p_err = FTPc_ERR_TX_CMD;
             goto exit_close_sock;
        }

                                                               /* Receive status line.                                  */
        reply_code = FTPc_WaitForStatus(p_conn, 0, 0, &err);
        if (reply_code != FTP_REPLY_CODE_OKAY) {
            *p_err = FTPc_ERR_RX_CMD_RESP_FAIL;
             goto exit_close_sock;
        }
                                                                /* Send PROT command.                                   */
        buf_size = Str_FmtPrint((char *)ctrl_buf, ctrl_buf_size, "%s %s\r\n", FTPc_Cmd[FTP_CMD_PROT].CmdStr, "P");
        rtn_code = FTPc_Tx(p_conn->SockID,
                           ctrl_buf,
                           buf_size,
                           p_cfg->CtrlTxMaxTimout_ms,
                           p_cfg->CtrlTxMaxRetry,
                           p_cfg->CtrlTxMaxDly_ms,
                          &err);
        FTPc_TRACE_INFO(("FTPc TX: %s", ctrl_buf));
        if (rtn_code == DEF_FAIL) {
            *p_err = FTPc_ERR_TX_CMD;
             goto exit_close_sock;
        }

                                                                /* Receive status line.                                 */
        reply_code = FTPc_WaitForStatus(p_conn, 0, 0, &err);
        if (reply_code != FTP_REPLY_CODE_OKAY) {
            *p_err = FTPc_ERR_RX_CMD_RESP_FAIL;
             goto exit_close_sock;
        }
    }
#endif

   *p_err = FTPc_ERR_NONE;

    goto exit;

exit_close_sock:
    NetSock_Close(p_conn->SockID, &err);
    p_conn->SockID = NET_SOCK_ID_NONE;
    rtn_code = DEF_FAIL;

exit:
    return (rtn_code);
}


/*
*********************************************************************************************************
*                                             FTPc_Close()
*
* Description : Close FTP connection.
*
* Argument(s) : p_conn      Pointer to FTP Client Connection object.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FTPc_ERR_NONE                   Connection closing was successful.
*                               FTPc_ERR_TX_CMD                 Sending Command failed.
*                               FTPc_ERR_RX_CMD_RESP_FAIL       Receiving Command response failed.
*
* Return(s)   : DEF_FAIL        FTP connection close failed.
*               DEF_OK          FTP connection close successful.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  FTPc_Close (FTPc_CONN  *p_conn,
                         FTPc_ERR   *p_err)
{
    const  FTPc_CFG     *p_cfg;
           CPU_CHAR      ctrl_buf[FTPc_CTRL_NET_BUF_SIZE];
           CPU_INT32U    ctrl_buf_size;
           CPU_INT32U    buf_size;
           CPU_INT32U    reply_code;
           CPU_BOOLEAN   rtn_code;
           NET_ERR       err;


    p_cfg = FTPc_CfgPtr;

    ctrl_buf_size = sizeof(ctrl_buf);
                                                                /* Send QUIT command.                                   */
    buf_size = Str_FmtPrint((char *)ctrl_buf, ctrl_buf_size, "%s\r\n", FTPc_Cmd[FTP_CMD_QUIT].CmdStr);
    rtn_code = FTPc_Tx(p_conn->SockID,
                       ctrl_buf,
                       buf_size,
                       p_cfg->CtrlTxMaxTimout_ms,
                       p_cfg->CtrlTxMaxRetry,
                       p_cfg->CtrlTxMaxDly_ms,
                      &err);
    FTPc_TRACE_INFO(("FTPc TX: %s", ctrl_buf));
    if (rtn_code == DEF_FAIL) {
        FTPc_TRACE_INFO(("FTPc CLOSE CTRL socket.\n"));
       *p_err    = FTPc_ERR_TX_CMD;
        rtn_code = DEF_FAIL;
        goto exit;
    }

                                                                /* Receive status line.                                 */
    reply_code = FTPc_WaitForStatus(p_conn, 0, 0, &err);
    if (reply_code != FTP_REPLY_CODE_SERVERCLOSING) {
        NetSock_Close(p_conn->SockID, &err);
        FTPc_TRACE_INFO(("FTPc CLOSE CTRL socket.\n"));
       *p_err    = FTPc_ERR_RX_CMD_RESP_FAIL;
        rtn_code = DEF_FAIL;
        goto exit;
    }

   *p_err = FTPc_ERR_NONE;


exit:                                                           /* Close socket.                                        */
    NetSock_Close(p_conn->SockID, &err);
    FTPc_TRACE_INFO(("FTPc CLOSE CTRL socket.\n"));

   (void)err;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                            FTPc_RecvBuf()
*
* Description : Receive a file from an FTP server into a memory buffer.
*
* Argument(s) : p_conn              Pointer to FTPc Connection object.
*
*               remote_file_name    Pointer to name of the file in FTP server.
*
*               p_buf               Pointer to memory buffer to hold received file.
*
*               buf_len             Size of the memory buffer.
*
*               p_file_size         Variable that will received the size of the file received.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FTPc_ERR_NONE                   Received file successfully.
*                               FTPc_ERR_TX_CMD                 Sending Command failed.
*                               FTPc_ERR_RX_CMD_RESP_FAIL       Receiving Command response failed.
*                               FTPc_ERR_FILE_BUF_LEN           Invalid buffer length.
*
* Return(s)   : DEF_FAIL        reception failed.
*               DEF_OK          reception successful.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  FTPc_RecvBuf (FTPc_CONN   *p_conn,
                           CPU_CHAR    *p_remote_file_name,
                           CPU_INT08U  *p_buf,
                           CPU_INT32U   buf_len,
                           CPU_INT32U  *p_file_size,
                           FTPc_ERR    *p_err)
{
    const  FTPc_CFG         *p_cfg;
           NET_SOCK_ID       sock_dtp_id;
           CPU_CHAR          ctrl_buf[FTPc_CTRL_NET_BUF_SIZE];
           CPU_INT32U        ctrl_buf_size;
           CPU_INT32U        buf_size;
           CPU_INT32U        reply_code;
           CPU_BOOLEAN       rtn_code;
           CPU_INT32U        rx_pkt_cnt;
           CPU_CHAR         *tmp_buf;
           CPU_INT32S        tmp_val;
           CPU_INT32U        rx_len;
           CPU_INT32U        bytes_recv;
           CPU_INT32U        got_file_size;
           NET_ERR           err;


    p_cfg = FTPc_CfgPtr;

    ctrl_buf_size = sizeof(ctrl_buf);
                                                                /* Send TYPE command.                                   */
    buf_size = Str_FmtPrint((char *)ctrl_buf, ctrl_buf_size, "%s %c\r\n", FTPc_Cmd[FTP_CMD_TYPE].CmdStr, FTP_TYPE_IMAGE);
    rtn_code = FTPc_Tx(p_conn->SockID,
                       ctrl_buf,
                       buf_size,
                       p_cfg->CtrlTxMaxTimout_ms,
                       p_cfg->CtrlTxMaxRetry,
                       p_cfg->CtrlTxMaxDly_ms,
                      &err);
    FTPc_TRACE_INFO(("FTPc TX: %s", ctrl_buf));
    if (rtn_code == DEF_FAIL) {
       *p_err = FTPc_ERR_TX_CMD;
        rtn_code = DEF_FAIL;
        goto exit;
    }

                                                                /* Receive status line.                                 */
    reply_code = FTPc_WaitForStatus(p_conn, 0, 0, &err);
    if (reply_code != FTP_REPLY_CODE_OKAY) {
       *p_err = FTPc_ERR_RX_CMD_RESP_FAIL;
        rtn_code = DEF_FAIL;
        goto exit;
    }
                                                                /* Send SIZE command.                                   */
    buf_size = Str_FmtPrint((char *)ctrl_buf, ctrl_buf_size, "%s %s\r\n", FTPc_Cmd[FTP_CMD_SIZE].CmdStr, p_remote_file_name);
    rtn_code = FTPc_Tx(p_conn->SockID,
                       ctrl_buf,
                       buf_size,
                       p_cfg->CtrlTxMaxTimout_ms,
                       p_cfg->CtrlTxMaxRetry,
                       p_cfg->CtrlTxMaxDly_ms,
                      &err);
    FTPc_TRACE_INFO(("FTPc TX: %s", ctrl_buf));
    if (rtn_code == DEF_FAIL) {
       *p_err    = FTPc_ERR_TX_CMD;
        rtn_code = DEF_FAIL;
        goto exit;
    }
                                                                /* Receive status line.                                 */
    reply_code = FTPc_WaitForStatus(p_conn, ctrl_buf, ctrl_buf_size, &err);
    if (reply_code != FTP_REPLY_CODE_FILESTATUS) {
       *p_err    = FTPc_ERR_RX_CMD_RESP_FAIL;
        rtn_code = DEF_FAIL;
        goto exit;
    }

    tmp_buf       = ctrl_buf;
    tmp_val       = Str_ParseNbr_Int32S(tmp_buf, &tmp_buf, 10);/* Skip result code.                                    */
    tmp_buf++;

    tmp_val       = Str_ParseNbr_Int32S(tmp_buf, &tmp_buf, 10);/* Get file size.                                       */
    tmp_buf++;
    got_file_size = tmp_val;
    if (got_file_size > buf_len) {
       *p_err    = FTPc_ERR_FILE_BUF_LEN;
        rtn_code = DEF_FAIL;
        goto exit;
    }


    sock_dtp_id = FTPc_Conn(p_conn, ctrl_buf, ctrl_buf_size, p_err);
    if (sock_dtp_id == NET_SOCK_ID_NONE) {
        rtn_code = DEF_FAIL;
        goto exit;
    }

                                                                /* Send RETR command.                                   */
    buf_size = Str_FmtPrint((char *)ctrl_buf, ctrl_buf_size, "%s %s\r\n", FTPc_Cmd[FTP_CMD_RETR].CmdStr, p_remote_file_name);
    rtn_code = FTPc_Tx(p_conn->SockID,
                       ctrl_buf,
                       buf_size,
                       p_cfg->CtrlTxMaxTimout_ms,
                       p_cfg->CtrlTxMaxRetry,
                       p_cfg->CtrlTxMaxDly_ms,
                       &err);
    FTPc_TRACE_INFO(("FTPc TX: %s", ctrl_buf));
    if (rtn_code == DEF_FAIL) {
       *p_err = FTPc_ERR_TX_CMD;
        rtn_code = DEF_FAIL;
        goto exit_close_dtp_sock;
    }

                                                                /* Receive status line.                                 */
    reply_code = FTPc_WaitForStatus(p_conn, 0, 0, &err);
    if ((reply_code != FTP_REPLY_CODE_ALREADYOPEN) &&
        (reply_code != FTP_REPLY_CODE_OKAYOPENING)) {
       *p_err    = FTPc_ERR_RX_CMD_RESP_FAIL;
        rtn_code = DEF_FAIL;
        goto exit_close_dtp_sock;
    }

    NetSock_CfgTimeoutRxQ_Set(sock_dtp_id, p_cfg->DTP_RxMaxTimout_ms, &err);

    bytes_recv = 0;
    rx_pkt_cnt = 0;
    while (bytes_recv < got_file_size) {

        tmp_buf = ((CPU_CHAR *)p_buf) + bytes_recv;
        rx_len  = got_file_size - bytes_recv;
        if (rx_len > DEF_INT_16S_MAX_VAL) {
            rx_len = DEF_INT_16S_MAX_VAL;
        }

        tmp_val = NetSock_RxData(sock_dtp_id, tmp_buf, rx_len, NET_SOCK_FLAG_NONE, &err);
        if (tmp_val > 0) {
            bytes_recv += tmp_val;
        }
        if ((err != NET_SOCK_ERR_NONE) &&
            (err != NET_SOCK_ERR_RX_Q_EMPTY) &&
            (err != NET_SOCK_ERR_RX_Q_CLOSED)) {
             FTPc_TRACE_DBG(("FTPc NetSock_RxData() failed: error #%u, line #%u.\n", (unsigned int)err, (unsigned int)__LINE__));
             break;
        }

        FTPc_TRACE_DBG(("FTPc RX DATA #%03u.\n", rx_pkt_cnt));

                                                                /* In this case, a timeout represents an end-of-file    */
                                                                /* condition.                                           */
        if ((err == NET_SOCK_ERR_RX_Q_EMPTY) ||
            (err == NET_SOCK_ERR_RX_Q_CLOSED)) {
             break;
        }
        rx_pkt_cnt++;
    }


   *p_file_size = got_file_size;

exit_close_dtp_sock:
                                                                /* Close socket.                                        */
    NetSock_Close(sock_dtp_id, &err);
    FTPc_TRACE_INFO(("FTPc CLOSE DTP socket.\n"));


    if (*p_err == FTPc_ERR_NONE) {
                                                                /* Receive status line.                                 */
        reply_code = FTPc_WaitForStatus(p_conn, 0, 0, &err);
        if (reply_code != FTP_REPLY_CODE_CLOSINGSUCCESS) {
           *p_err = FTPc_ERR_RX_CMD_RESP_FAIL;
            rtn_code = DEF_FAIL;
            goto exit;
        }

        rtn_code = DEF_OK;

    } else {
        goto exit;
    }


   *p_err = FTPc_ERR_NONE;

exit:
    return (rtn_code);
}


/*
*********************************************************************************************************
*                                            FTPc_SendBuf()
*
* Description : Send a memory buffer to an FTP server.
*
* Argument(s) : p_conn              Pointer to FTPc Connection object.
*
*               p_remote_file_name  Pointer to name of the file in FTP server.
*
*               p_buf               Pointer to memory buffer to send.
*
*               buf_len             Size of the memory buffer.
*
*               append              if DEF_YES, existing file on FTP server will be appended with
*                                       memory buffer. If file doesn't exist on FTP server, it will be
*                                       created.
*                                   if DEF_NO, existing file on FTP server will be overwritten.
*                                       If file doesn't exist on FTP server, it will be created.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FTPc_ERR_NONE                   Transmission was successful.
*                               FTPc_ERR_TX_CMD                 Sending Command failed.
*                               FTPc_ERR_RX_CMD_RESP_FAIL       Receiving Command response failed.
*
* Return(s)   : DEF_FAIL        transmission failed.
*               DEF_OK          transmission successful.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  FTPc_SendBuf (FTPc_CONN    *p_conn,
                           CPU_CHAR     *p_remote_file_name,
                           CPU_INT08U   *p_buf,
                           CPU_INT32U    buf_len,
                           CPU_BOOLEAN   append,
                           FTPc_ERR     *p_err)
{
    const  FTPc_CFG     *p_cfg;
           NET_SOCK_ID   sock_dtp_id;
           CPU_CHAR      ctrl_buf[FTPc_CTRL_NET_BUF_SIZE];
           CPU_INT32U    ctrl_buf_size;
           CPU_INT32U    buf_size;
           CPU_INT32U    reply_code;
           CPU_BOOLEAN   rtn_code;
           CPU_INT32U    bytes_sent;
           CPU_INT32U    tx_pkt_cnt;
           NET_ERR       err;


    p_cfg = FTPc_CfgPtr;

    ctrl_buf_size = sizeof(ctrl_buf);
                                                                /* Send TYPE command.                                   */
    buf_size = Str_FmtPrint((char *)ctrl_buf, ctrl_buf_size, "%s %c\r\n", FTPc_Cmd[FTP_CMD_TYPE].CmdStr, FTP_TYPE_IMAGE);
    rtn_code = FTPc_Tx(p_conn->SockID,
                       ctrl_buf,
                       buf_size,
                       p_cfg->CtrlTxMaxTimout_ms,
                       p_cfg->CtrlTxMaxRetry,
                       p_cfg->CtrlTxMaxDly_ms,
                      &err);
    FTPc_TRACE_INFO(("FTPc TX: %s", ctrl_buf));
    if (rtn_code == DEF_FAIL) {
       *p_err    = FTPc_ERR_TX_CMD;
        rtn_code = DEF_FAIL;
        goto exit;
    }

                                                                /* Receive status line.                                 */
    reply_code = FTPc_WaitForStatus(p_conn, 0, 0, &err);
    if (reply_code != FTP_REPLY_CODE_OKAY) {
       *p_err    = FTPc_ERR_RX_CMD_RESP_FAIL;
        rtn_code = DEF_FAIL;
        goto exit;
    }

    sock_dtp_id = FTPc_Conn(p_conn, ctrl_buf, ctrl_buf_size, p_err);
    if (sock_dtp_id == NET_SOCK_ID_NONE) {
         rtn_code = DEF_FAIL;
         goto exit;
    }


    if (append == DEF_YES) {
                                                                /* Send APPE command.                                   */
        buf_size = Str_FmtPrint((char *)ctrl_buf, ctrl_buf_size, "%s %s\r\n", FTPc_Cmd[FTP_CMD_APPE].CmdStr, p_remote_file_name);
        rtn_code = FTPc_Tx(p_conn->SockID,
                           ctrl_buf,
                           buf_size,
                           p_cfg->CtrlTxMaxTimout_ms,
                           p_cfg->CtrlTxMaxRetry,
                           p_cfg->CtrlTxMaxDly_ms,
                           &err);
        FTPc_TRACE_INFO(("FTPc TX: %s", ctrl_buf));
        if (rtn_code == DEF_FAIL) {
            *p_err    = FTPc_ERR_TX_CMD;
             rtn_code = DEF_FAIL;
             goto exit_close_dtp_sock;
        }
    } else {
                                                                /* Send STOR command.                                   */
        buf_size = Str_FmtPrint((char *)ctrl_buf, ctrl_buf_size, "%s %s\r\n", FTPc_Cmd[FTP_CMD_STOR].CmdStr, p_remote_file_name);
        rtn_code = FTPc_Tx(p_conn->SockID,
                           ctrl_buf,
                           buf_size,
                           p_cfg->CtrlTxMaxTimout_ms,
                           p_cfg->CtrlTxMaxRetry,
                           p_cfg->CtrlTxMaxDly_ms,
                           &err);
        FTPc_TRACE_INFO(("FTPc TX: %s", ctrl_buf));
        if (rtn_code == DEF_FAIL) {
            *p_err    = FTPc_ERR_TX_CMD;
             rtn_code = DEF_FAIL;
             goto exit_close_dtp_sock;
        }
    }
                                                                /* Receive status line.                                 */
    reply_code = FTPc_WaitForStatus(p_conn, 0, 0, &err);
    if (reply_code != FTP_REPLY_CODE_OKAYOPENING) {
       *p_err    = FTPc_ERR_RX_CMD_RESP_FAIL;
        rtn_code = DEF_FAIL;
        goto exit_close_dtp_sock;
    }

    NetSock_CfgTimeoutRxQ_Set(sock_dtp_id, p_cfg->DTP_TxMaxTimout_ms, &err);
    bytes_sent = 0;
    tx_pkt_cnt = 0;
    while (bytes_sent < buf_len) {
        buf_size = buf_len - bytes_sent;
        if (buf_size > FTPc_DTP_NET_BUF_SIZE) {
            buf_size = FTPc_DTP_NET_BUF_SIZE;
        }

        FTPc_TRACE_DBG(("FTPc TX DATA #%03u... ", tx_pkt_cnt));
        rtn_code = FTPc_Tx(            sock_dtp_id,
                           (CPU_CHAR *)p_buf,
                                       buf_size,
                                       0,
                                       p_cfg->DTP_TxMaxRetry,
                                       p_cfg->DTP_TxMaxDly_ms,
                                      &err);
        if (rtn_code == DEF_FAIL) {
            FTPc_TRACE_DBG(("FTPc FTPc_Tx() failed: error #%u, line #%u.\n", (unsigned int)err, (unsigned int)__LINE__));
            break;
        }

        FTPc_TRACE_DBG(("\n"));
        bytes_sent += buf_size;
        p_buf       += buf_size;
        tx_pkt_cnt++;
    }

exit_close_dtp_sock:
                                                                /* Close socket.                                        */
    NetSock_Close(sock_dtp_id, &err);
    FTPc_TRACE_INFO(("FTPc CLOSE DTP socket.\n"));

    if (*p_err == FTPc_ERR_NONE) {
                                                                /* Receive status line.                                 */
        reply_code = FTPc_WaitForStatus(p_conn, 0, 0, &err);
        if (reply_code != FTP_REPLY_CODE_CLOSINGSUCCESS) {
            *p_err = FTPc_ERR_RX_CMD_RESP_FAIL;
             rtn_code = DEF_FAIL;
             goto exit;
         }

         rtn_code = DEF_OK;

     } else {
         goto exit;
     }

   *p_err = FTPc_ERR_NONE;

exit:
    return (rtn_code);
}


/*
*********************************************************************************************************
*                                            FTPc_RecvFile()
*
* Description : Receive a file from an FTP server to the file system.
*
* Argument(s) : p_conn              Pointer to FTPc Connection object.
*
*               p_remote_file_name  Pointer to name of the file in FTP server.
*
*               p_local_file_name   Pointer to  name of the file in file system.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FTPc_ERR_NONE                   Reception of file was successful.
*                               FTPc_ERR_TX_CMD                 Sending Command failed.
*                               FTPc_ERR_RX_CMD_RESP_FAIL       Receiving Command response failed.
*                               FTPc_ERR_FILE_NOT_FOUND         File of server not found.
*                               FTPc_ERR_FAULT                  Reception faulted.
*                               FTPc_ERR_FILE_OPEN_FAIL         File opening faulted on FS.
*
* Return(s)   : DEF_FAIL        reception failed.
*               DEF_OK          reception successful.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  FTPc_RecvFile (FTPc_CONN  *p_conn,
                            CPU_CHAR   *p_remote_file_name,
                            CPU_CHAR   *p_local_file_name,
                            FTPc_ERR   *p_err)
{
#if (FTPc_CFG_USE_FS > 0)
    const  FTPc_CFG     *p_cfg;
           NET_SOCK_ID   sock_dtp_id;
           CPU_CHAR      data_buf[FTPc_DTP_NET_BUF_SIZE];
           CPU_CHAR      ctrl_buf[FTPc_CTRL_NET_BUF_SIZE];
           CPU_INT32U    ctrl_buf_size;
           CPU_INT32U    buf_size;
           CPU_INT32U    reply_code;
           CPU_BOOLEAN   rtn_code;
           CPU_INT32U    rx_pkt_cnt;
           void         *p_file;
           CPU_SIZE_T    fs_len;
           NET_ERR       err;


    p_cfg = FTPc_CfgPtr;

   *p_err = FTPc_ERR_NONE;

    ctrl_buf_size = sizeof(ctrl_buf);
                                                                /* Send TYPE command.                                   */
    buf_size = Str_FmtPrint((char *)ctrl_buf, ctrl_buf_size, "%s %c\r\n", FTPc_Cmd[FTP_CMD_TYPE].CmdStr, FTP_TYPE_IMAGE);
    rtn_code = FTPc_Tx(p_conn->SockID,
                       ctrl_buf,
                       buf_size,
                       p_cfg->CtrlTxMaxTimout_ms,
                       p_cfg->CtrlTxMaxRetry,
                       p_cfg->CtrlTxMaxDly_ms,
                      &err);
    FTPc_TRACE_INFO(("FTPc TX: %s", ctrl_buf));
    if (rtn_code == DEF_FAIL) {
       *p_err    = FTPc_ERR_TX_CMD;
        rtn_code = DEF_FAIL;
        goto exit;
    }

                                                                /* Receive status line.                                 */
    reply_code = FTPc_WaitForStatus(p_conn, 0, 0, &err);
    if (reply_code != FTP_REPLY_CODE_OKAY) {
        *p_err    = FTPc_ERR_RX_CMD_RESP_FAIL;
         rtn_code = DEF_FAIL;
         goto exit;
    }

    sock_dtp_id = FTPc_Conn(p_conn, ctrl_buf, ctrl_buf_size, p_err);
    if (sock_dtp_id == NET_SOCK_ID_NONE) {
        rtn_code = DEF_FAIL;
        goto exit;
    }


                                                                /* Send RETR command.                                   */
    buf_size = Str_FmtPrint((char *)ctrl_buf, ctrl_buf_size, "%s %s\r\n", FTPc_Cmd[FTP_CMD_RETR].CmdStr, p_remote_file_name);
    rtn_code = FTPc_Tx(p_conn->SockID,
                       ctrl_buf,
                       buf_size,
                       p_cfg->CtrlTxMaxTimout_ms,
                       p_cfg->CtrlTxMaxRetry,
                       p_cfg->CtrlTxMaxDly_ms,
                       &err);
    FTPc_TRACE_INFO(("FTPc TX: %s", ctrl_buf));
    if (rtn_code == DEF_FAIL) {
        *p_err    = FTPc_ERR_TX_CMD;
         rtn_code = DEF_FAIL;
         goto exit_close_dtp_sock;
    }
                                                                /* Receive status line.                                 */
    reply_code = FTPc_WaitForStatus(p_conn, 0, 0, &err);
    switch (reply_code) {
        case FTP_REPLY_CODE_ALREADYOPEN:
        case FTP_REPLY_CODE_OKAYOPENING:
             break;

        case FTP_REPLY_CODE_NOTFOUND:
            *p_err    = FTPc_ERR_FILE_NOT_FOUND;
             rtn_code = DEF_FAIL;
             goto exit_close_dtp_sock;

        default:
            *p_err    = FTPc_ERR_FAULT;
             rtn_code = DEF_FAIL;
             goto exit_close_dtp_sock;
    }


    p_file = NetFS_FileOpen(p_local_file_name,
                            NET_FS_FILE_MODE_CREATE,
                            NET_FS_FILE_ACCESS_RD_WR);
    if (p_file == (void *)0) {
        FTPc_TRACE_DBG(("FTPc NetFS_FileOpen failed, line #%u.\n", (unsigned int)__LINE__));
       *p_err    = FTPc_ERR_FILE_OPEN_FAIL;
        rtn_code = DEF_FAIL;
        goto exit_close_dtp_sock;
    }

    NetSock_CfgTimeoutRxQ_Set(sock_dtp_id,
                              p_cfg->DTP_RxMaxTimout_ms,
                              &err);

    rx_pkt_cnt = 0;
    while (DEF_TRUE) {
        buf_size = NetSock_RxData(sock_dtp_id, data_buf, sizeof(data_buf), NET_SOCK_FLAG_NONE, &err);
        if ((err != NET_SOCK_ERR_NONE) &&
            (err != NET_SOCK_ERR_RX_Q_EMPTY) &&
            (err != NET_SOCK_ERR_RX_Q_CLOSED)) {
              FTPc_TRACE_DBG(("FTPc NetSock_RxData() failed: error #%u, line #%u.\n", (unsigned int)err, (unsigned int)__LINE__));
              break;
        }

        FTPc_TRACE_DBG(("FTPc RX DATA #%03u.\n", rx_pkt_cnt));

                                                                /* In this case, a timeout represents an end-of-file    */
                                                                /* condition.                                           */
        if ((err == NET_SOCK_ERR_RX_Q_EMPTY) ||
            (err == NET_SOCK_ERR_RX_Q_CLOSED)) {
             break;
        }

       (void)NetFS_FileWr( p_file,
                           data_buf,
                           buf_size,
                          &fs_len);
        if (fs_len != buf_size) {
            FTPc_TRACE_DBG(("FTPc NetFS_FileWr() failed, line #%u.\n", (unsigned int)__LINE__));
            break;
        }
        rx_pkt_cnt++;
    }

    NetFS_FileClose(p_file);

exit_close_dtp_sock:
                                                                /* Close socket.                                        */
    NetSock_Close(sock_dtp_id, &err);
    FTPc_TRACE_INFO(("FTPc CLOSE DTP socket.\n"));


    if (*p_err == FTPc_ERR_NONE) {
                                                                /* Receive status line.                                 */
        reply_code = FTPc_WaitForStatus(p_conn, 0, 0, &err);
        if (reply_code != FTP_REPLY_CODE_CLOSINGSUCCESS) {
           *p_err = FTPc_ERR_RX_CMD_RESP_FAIL;
            rtn_code = DEF_FAIL;
            goto exit;
        }

        rtn_code = DEF_OK;

    } else {
        goto exit;
    }


   *p_err = FTPc_ERR_NONE;

exit:
    return (rtn_code);
#else
    return (DEF_FAIL);
#endif
}


/*
*********************************************************************************************************
*                                            FTPc_SendFile()
*
* Description : Send a file located in the file system to an FTP server.
*
* Argument(s) : p_conn              Pointer to FTPc Connection object.
*
*               p_remote_file_name  Pointer to name of the file in FTP server.
*
*               p_local_file_name   Pointer to name of the file in file system.
*
*               append              if DEF_YES, existing file on FTP server will be appended with
*                                       local file.  If file doesn't exist on FTP server, it will be
*                                       created.
*                                   if DEF_NO, existing file on FTP server will be overwritten.
*                                       If file doesn't exist on FTP server, it will be created.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FTPc_ERR_NONE                   Transmission of file was successful.
*                               FTPc_ERR_TX_CMD                 Sending Command failed.
*                               FTPc_ERR_RX_CMD_RESP_FAIL       Receiving Command response failed.
*                               FTPc_ERR_FILE_OPEN_FAIL         File opening faulted on FS.
*
* Return(s)   : DEF_FAIL        transmission failed.
*               DEF_OK          transmission successful.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  FTPc_SendFile (FTPc_CONN    *p_conn,
                            CPU_CHAR     *p_remote_file_name,
                            CPU_CHAR     *p_local_file_name,
                            CPU_BOOLEAN   append,
                            FTPc_ERR     *p_err)
{
#if (FTPc_CFG_USE_FS == DEF_ENABLED)
    const  FTPc_CFG     *p_cfg;
           NET_SOCK_ID   sock_dtp_id;
           CPU_CHAR      data_buf[FTPc_DTP_NET_BUF_SIZE];
           CPU_CHAR      ctrl_buf[FTPc_CTRL_NET_BUF_SIZE];
           CPU_INT32U    ctrl_buf_size;
           CPU_INT32U    buf_size;
           CPU_INT32U    reply_code;
           CPU_BOOLEAN   rtn_code;
           CPU_INT32U    tx_pkt_cnt;
           void         *p_file;
           CPU_SIZE_T    fs_len;
           CPU_BOOLEAN   fs_err;
           NET_ERR       err;


    p_cfg = FTPc_CfgPtr;

    ctrl_buf_size = sizeof(ctrl_buf);
                                                                /* Send TYPE command.                                   */
    buf_size = Str_FmtPrint((char *)ctrl_buf, ctrl_buf_size, "%s %c\r\n", FTPc_Cmd[FTP_CMD_TYPE].CmdStr, FTP_TYPE_IMAGE);
    rtn_code = FTPc_Tx(p_conn->SockID,
                       ctrl_buf,
                       buf_size,
                       p_cfg->CtrlTxMaxTimout_ms,
                       p_cfg->CtrlTxMaxRetry,
                       p_cfg->CtrlTxMaxDly_ms,
                      &err);
    FTPc_TRACE_INFO(("FTPc TX: %s", ctrl_buf));
    if (rtn_code == DEF_FAIL) {
        *p_err    = FTPc_ERR_TX_CMD;
         rtn_code = DEF_FAIL;
         goto exit;
    }

                                                                /* Receive status line.                                 */
    reply_code = FTPc_WaitForStatus(p_conn, 0, 0, &err);
    if (reply_code != FTP_REPLY_CODE_OKAY) {
        *p_err    = FTPc_ERR_RX_CMD_RESP_FAIL;
         rtn_code = DEF_FAIL;
         goto exit;
    }

    sock_dtp_id = FTPc_Conn(p_conn, ctrl_buf, ctrl_buf_size, p_err);
    if (sock_dtp_id == NET_SOCK_ID_NONE) {
        rtn_code = DEF_FAIL;
        goto exit;
    }


    if (append == DEF_YES) {
                                                                /* Send APPE command.                                   */
        buf_size = Str_FmtPrint((char *)ctrl_buf, ctrl_buf_size, "%s %s\r\n", FTPc_Cmd[FTP_CMD_APPE].CmdStr, p_remote_file_name);
        rtn_code = FTPc_Tx(p_conn->SockID,
                           ctrl_buf,
                           buf_size,
                           p_cfg->CtrlTxMaxTimout_ms,
                           p_cfg->CtrlTxMaxRetry,
                           p_cfg->CtrlTxMaxDly_ms,
                          &err);
        FTPc_TRACE_INFO(("FTPc TX: %s", ctrl_buf));
        if (rtn_code == DEF_FAIL) {
           *p_err = FTPc_ERR_TX_CMD;
            rtn_code = DEF_FAIL;
            goto exit_close_dtp_sock;
        }
    } else {
                                                                /* Send STOR command.                                   */
        buf_size = Str_FmtPrint((char *)ctrl_buf, ctrl_buf_size, "%s %s\r\n", FTPc_Cmd[FTP_CMD_STOR].CmdStr, p_remote_file_name);
        rtn_code = FTPc_Tx(p_conn->SockID,
                           ctrl_buf,
                           buf_size,
                           p_cfg->CtrlTxMaxTimout_ms,
                           p_cfg->CtrlTxMaxRetry,
                           p_cfg->CtrlTxMaxDly_ms,
                           &err);
        FTPc_TRACE_INFO(("FTPc TX: %s", ctrl_buf));
        if (rtn_code == DEF_FAIL) {
           *p_err = FTPc_ERR_TX_CMD;
            rtn_code = DEF_FAIL;
            goto exit_close_dtp_sock;
        }
    }
                                                                /* Receive status line.                                 */
    reply_code = FTPc_WaitForStatus(p_conn, 0, 0, &err);
    if (reply_code != FTP_REPLY_CODE_OKAYOPENING) {
       *p_err = FTPc_ERR_RX_CMD_RESP_FAIL;
        rtn_code = DEF_FAIL;
        goto exit_close_dtp_sock;
    }

    p_file = NetFS_FileOpen(p_local_file_name,
                            NET_FS_FILE_MODE_OPEN,
                            NET_FS_FILE_ACCESS_RD);
    if (p_file == (void *)0) {
       *p_err = FTPc_ERR_FILE_OPEN_FAIL;
        rtn_code = DEF_FAIL;
        goto exit_close_dtp_sock;
    }

    tx_pkt_cnt = 0;

    NetSock_CfgTimeoutRxQ_Set(sock_dtp_id, p_cfg->DTP_TxMaxTimout_ms, &err);

    while (DEF_TRUE) {
        fs_err = NetFS_FileRd(p_file,
                              data_buf,
                              sizeof(data_buf),
                             &fs_len);
        if (fs_len == 0) {
            if (fs_err != DEF_OK) {
                FTPc_TRACE_DBG(("FTPc NetFS_FileRd() failed, line #%u.\n", (unsigned int)__LINE__));
            }
            break;
        }

        FTPc_TRACE_DBG(("FTPc TX DATA #%03u... ", tx_pkt_cnt));
        rtn_code = FTPc_Tx(sock_dtp_id,
                           data_buf,
                           fs_len,
                           0,
                           p_cfg->DTP_TxMaxRetry,
                           p_cfg->DTP_TxMaxDly_ms,
                           &err);
        if (rtn_code == DEF_FAIL) {
            FTPc_TRACE_DBG(("FTPc FTPc_Tx() failed: error #%u, line #%u.\n", (unsigned int)err, (unsigned int)__LINE__));
            break;
        }

        FTPc_TRACE_DBG(("\n"));
        if (fs_len != sizeof(data_buf)) {
            break;
        }
        tx_pkt_cnt++;
    }

    NetFS_FileClose(p_file);


exit_close_dtp_sock:
                                                                /* Close socket.                                        */
    NetSock_Close(sock_dtp_id, &err);
    FTPc_TRACE_INFO(("FTPc CLOSE DTP socket.\n"));

    if (*p_err == FTPc_ERR_NONE) {
                                                                /* Receive status line.                                 */
        reply_code = FTPc_WaitForStatus(p_conn, 0, 0, &err);
        if (reply_code != FTP_REPLY_CODE_CLOSINGSUCCESS) {
            *p_err = FTPc_ERR_RX_CMD_RESP_FAIL;
             rtn_code = DEF_FAIL;
             goto exit;
         }

         rtn_code = DEF_OK;

     } else {
         goto exit;
     }


   *p_err = FTPc_ERR_NONE;

exit:
    return (rtn_code);
#else
    return (DEF_FAIL);
#endif
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                         FTPc_WaitForStatus()
*
* Description : Receive FTP server reply after a request and analyze it to find the server status reply.
*
* Argument(s) : p_conn          Pointer to FTPc Connection object.
*
*               p_ctrl_data     Pointer to  buffer  that will receive control reply data.
*
*               ctrl_data_len   Size of the buffer  that will receive control reply data.
*
*               p_err           Pointer to variable that will receive the return error code from NetSock_RxData().
*
* Return(s)   : FTP server status code   if no error;
*               0                        otherwise.
*
* Caller(s)   : FTPc_Open(),
*               FTPc_Close(),
*               FTPc_RecvBuf(),
*               FTPc_SendBuf(),
*               FTPc_RecvFile(),
*               FTPc_SendFile().
*
* Note(s)     : (1) Even though the server reply code can be retrieved from the first 3 octets in the
*                   received message, it is mandatory that the whole response be taken from the socket
*                   receive queue in order to make sure no "left over" messages will be present at the
*                   socket at a later call to this function.
*                   
*                   (a) However, section 5.4 of RFC 959 titled "SEQUENCING OF COMMANDS AND REPLIES" 
*                       states the following in its second paragraph:
*
*                      "Certain commands require a second reply for which the user should also wait.
*                       These replies may, for example, report on the progress or completion of file
*                       transfer or the closing of the data connection. They are secondary replies to
*                       file transfer commands."
*
*                       To this end, we prepend the "/" character to p_conn->Buf to let this function know in 
*                       a future call that the non-zero data in p_conn->Buf is part of a multi-response reply
*                       whose response code is yet to be decoded.
*
*               (2) As data is consumed from the socket receive queue, in a situation where the server's 
*                   welcome message extends beyond the size of the buffer once FTPc_RxReply() is called 
*                   to retrieve more data from the socket receive queue, the reply code is no longer  
*                   guaranteed to be the first three octets in the receive message. Thus the search string
*                   must be advanced until the next instance of the reply code. This applies only if the
*                   reply message is already known to be multiline.
*********************************************************************************************************
*/

static  CPU_INT32U  FTPc_WaitForStatus (FTPc_CONN   *p_conn,
                                        CPU_CHAR    *p_ctrl_data,
                                        CPU_INT16U   ctrl_data_len,
                                        NET_ERR     *p_err)
{
    const  FTPc_CFG     *p_cfg;
           CPU_CHAR     *p_buf;
           CPU_CHAR     *p_str_multiline;
           CPU_CHAR     *p_search_str;
           CPU_CHAR     *trunc_str;
           CPU_CHAR      end_of_cmd[FTP_EOL_DELIMITER_LEN + FTP_REPLY_CODE_LEN + 2];
           CPU_CHAR      multiline_token[FTP_EOL_DELIMITER_LEN + FTP_REPLY_CODE_LEN + 2];
           CPU_INT16U    reply_code;
           CPU_INT16U    rx_reply_ctr;
           CPU_INT32U    len;
           CPU_INT32U    buf_len;
           CPU_INT32S    rx_reply_tot_len;
           CPU_INT32S    rx_reply_pkt_len;
           CPU_BOOLEAN   find_eof;
           CPU_BOOLEAN   parse_done;
           CPU_BOOLEAN   is_msg_truncated;
           CPU_BOOLEAN   truncated_parsed;
           CPU_BOOLEAN   token_found;
           CPU_BOOLEAN   rx_reply;
           CPU_INT16S    cmp;


    Mem_Clr(end_of_cmd, sizeof(end_of_cmd));

    p_cfg                                                       =  FTPc_CfgPtr;
    end_of_cmd[0u]                                              =  FTP_ASCII_CR;
    end_of_cmd[1u]                                              =  FTP_ASCII_LF;
    end_of_cmd[FTP_EOL_DELIMITER_LEN + FTP_REPLY_CODE_LEN]      =  FTP_ASCII_SPACE;
    end_of_cmd[FTP_EOL_DELIMITER_LEN + FTP_REPLY_CODE_LEN + 1u] = (CPU_CHAR)'\0';
    reply_code                                                  =  0u;
    rx_reply_tot_len                                            =  0;
    rx_reply_pkt_len                                            =  0;
    find_eof                                                    =  DEF_NO;
    parse_done                                                  =  DEF_NO;
    rx_reply                                                    =  DEF_NO;
    p_buf                                                       = (CPU_CHAR *)&p_conn->Buf[0u];
    buf_len                                                     =  sizeof(p_conn->Buf);
    is_msg_truncated                                            =  DEF_FALSE;
    truncated_parsed                                            =  DEF_FALSE;
    token_found                                                 =  DEF_FALSE;
    rx_reply_ctr                                                =  0u;

    
    if (*p_buf == (CPU_CHAR)'\0') {
        rx_reply = DEF_YES;
    }

    NetSock_CfgTimeoutRxQ_Set(p_conn->SockID, p_cfg->CtrlRxMaxTimout_ms, p_err);
    while (parse_done == DEF_NO) {
        if (rx_reply == DEF_YES) {
            if (rx_reply_tot_len < p_cfg->CtrlRxMaxReplyLength) {
                KAL_Dly(5u);                                    /* Allow Rx Task to place new data onto Large Rx buffer.*/
                p_buf            = (CPU_CHAR *)&p_conn->Buf[0u];
                rx_reply_pkt_len = FTPc_RxReply(p_conn->SockID,
                                                p_buf,
                                                buf_len - 1u,
                                                p_err);
            } else {
                NetSock_CfgRxQ_Size(p_conn->SockID, 0u, p_err);
               *p_err = NET_SOCK_ERR_RX_Q_CLOSED;
            }
            switch (*p_err) {
                case NET_SOCK_ERR_NONE:
                case NET_ERR_RX:
                case NET_ERR_INIT_INCOMPLETE:
                case NET_SOCK_ERR_RX_Q_EMPTY:
                case NET_ERR_FAULT_LOCK_ACQUIRE:
                     rx_reply_ctr++;
                     break;


                case NET_ERR_FAULT_NULL_PTR:
                case NET_SOCK_ERR_INVALID_DATA_SIZE:
                case NET_SOCK_ERR_CLOSED:
                case NET_SOCK_ERR_RX_Q_CLOSED:
                case NET_SOCK_ERR_NOT_USED:
                case NET_SOCK_ERR_INVALID_TYPE:
                case NET_SOCK_ERR_INVALID_SOCK:
                case NET_SOCK_ERR_INVALID_FLAG:
                case NET_SOCK_ERR_INVALID_ADDR_LEN:
                case NET_SOCK_ERR_INVALID_OP:
                case NET_SOCK_ERR_INVALID_FAMILY:
                case NET_SOCK_ERR_INVALID_PROTOCOL:
                case NET_SOCK_ERR_INVALID_STATE:
                case NET_SOCK_ERR_CONN_FAIL:
                case NET_SOCK_ERR_FAULT:
                case NET_CONN_ERR_NOT_USED:
                case NET_CONN_ERR_INVALID_CONN:
                case NET_CONN_ERR_INVALID_ADDR_LEN:
                case NET_CONN_ERR_ADDR_NOT_USED:
                default:
                     return (0u);
            }
        }

        if ((rx_reply         == DEF_NO) ||
            (rx_reply_pkt_len  > 0     )) {

            if (rx_reply == DEF_YES) {
                rx_reply_tot_len        +=  rx_reply_pkt_len;
                p_buf[rx_reply_pkt_len]  = (CPU_CHAR)'\0';      /* Append termination char.                             */
                FTPc_TRACE_INFO(("%s", p_buf + rx_reply_pkt_len));
            }
                                                                /* Flag msg as truncated if it's taken more than one... */
                                                                /* ...attempt to parse the entire reply.                */
            is_msg_truncated = (*(p_buf + rx_reply_pkt_len - 1u) != FTP_ASCII_LF && !token_found);
            p_search_str     = p_buf;

            while ((rx_reply_ctr  > 0u) && 
                   (reply_code   == 0u)) {
                p_str_multiline = Str_Char_N(p_buf,             /* Check if it's a multiline reply.                     */
                                             rx_reply_pkt_len,
                                             FTP_REPLY_CODE_MULTI_LINE_INDICATOR);

                if ((p_str_multiline           != (CPU_CHAR *)DEF_NULL) && 
                    ((p_str_multiline - p_buf) != FTP_REPLY_CODE_LEN  )) {
                    reply_code = (CPU_INT16U)Str_ParseNbr_Int32U(p_str_multiline - FTP_REPLY_CODE_LEN, 0, 10);
                    if (reply_code == 0u) {
                        p_buf           = p_str_multiline + 1u; /* Not multiline if '-' is found & is not preceded...   */
                        p_str_multiline = (CPU_CHAR *)DEF_NULL; /*  ...by the reply code.                               */
                    }
                } else {
                    find_eof = DEF_TRUE;
                    break;
                }
            }

            if (p_str_multiline == DEF_NULL) {
                p_buf = p_search_str;
            }
                                                                /* Build end-of-command token to search.                */
            if (end_of_cmd[FTP_EOL_DELIMITER_LEN] == (CPU_CHAR)'\0') {
                if (rx_reply == DEF_TRUE) {
                    Str_Copy_N((CPU_CHAR *)end_of_cmd + FTP_EOL_DELIMITER_LEN,
                                           p_buf,
                               (CPU_SIZE_T)FTP_REPLY_CODE_LEN);
                }
            }

            if (rx_reply_ctr == 0u) {                           /* -  CASE I: NOTHING HAS BEEN CONSUMED FROM RX Q YET - */
                cmp = Str_Cmp_N(p_buf,                          /* First check if buf contains data from a prev. reply. */
                                FTP_MULTI_REPLY_DELIM, 
                                1u);

                rx_reply = (cmp != 0);                          /* If it doesn't, consume data & place it in rx buffer. */
                if (rx_reply) {
                    continue;
                }

                p_buf++;                                        /* If it does, advance pointer & find "\r\n" EOF token. */
                find_eof = DEF_YES;
            } else if (rx_reply_ctr == 1u) {                    /* - CASE II: RESPONSE LENGTH SPANS ONE BUFFER SO FAR - */
                if (*p_str_multiline == FTP_REPLY_CODE_MULTI_LINE_INDICATOR) {
                                                                /* Check if end-of-command token came in the same pkt.  */
                    p_search_str = Str_Str_N(p_buf, end_of_cmd, buf_len);
                    token_found  = (p_search_str != (void *)0);

                    if ((token_found != DEF_TRUE) || (token_found && is_msg_truncated)) {
                        len = sizeof(multiline_token);
                        Str_Copy_N( multiline_token,            /* Save last seven octets of rx buffer in case the ...  */
                                   &p_buf[FTPc_CTRL_NET_BUF_SIZE - len],
                                    len);                       /* ... end-of-command token itself was truncated.       */

                        Mem_Clr(&p_conn->Buf[0u], buf_len);     /* Clear entire buffer; end-of-cmd token was not found. */
                        is_msg_truncated = DEF_TRUE;        
                    } else {
                        find_eof      = DEF_YES;
                        p_search_str += 2u;                     /* See Note #2.                                         */
                    }
                } else {
                    len      = rx_reply_pkt_len;
                    find_eof = DEF_YES;
                }
            } else {                                            /* - CASE III: RESPONSE LENGTH SPANS MULTIPLE BUFFERS - */
                if (token_found && !find_eof) {
                    rx_reply = DEF_NO;
                    find_eof = DEF_YES;
                    continue;
                }
                                                                /* Handle case where end-of-cmd token might be truncated*/
                if ((*p_str_multiline != FTP_REPLY_CODE_MULTI_LINE_INDICATOR) && 
                    ( rx_reply_pkt_len > 1u)) {
                    len       = sizeof(multiline_token);        /* No multiline char found, check if end of cmd token...*/
                                                                /*...was truncated in between buffer reads.             */
                    trunc_str = Str_Char_N(multiline_token,     /* Find '\r' in the last seven octets of old buffer ... */
                                           len,                 /* ...and if found, examine how truncated the token was.*/
                                           FTP_ASCII_CR);
                    len       = sizeof(multiline_token) - Str_Len(trunc_str);
                                                                /* Move partial token to the beginning of the array ... */
                    Str_Copy(multiline_token, trunc_str);       /* ... and clear the rest of the array.                 */
                    Mem_Clr((multiline_token + sizeof(multiline_token) - len), len);
                    len = Str_Len(multiline_token);

                    if (len != 0u) {                            /* Was token partially contained in prev. buffer read?  */
                        Str_Copy_N(multiline_token + len,       /* Reconstruct token for comparison.                    */
                                   p_buf,
                                   (sizeof(multiline_token) - len - 1u));

                        len = Str_Len(end_of_cmd);
                        cmp = Str_Cmp_N(multiline_token,        /* Does reconstructed token match end-of-cmd token?     */
                                        end_of_cmd, 
                                        len); 
                        if (cmp == 0) { 
                            truncated_parsed = DEF_TRUE;        /* Indicates truncated token was successfully parsed.   */
                            rx_reply         = DEF_NO;
                            is_msg_truncated = DEF_NO;
                            token_found      = DEF_YES;
                        }
                    }
                }
            }
                                                                /* -     SEARCH FOR TERMINATING "\r\n" CHARACTERS     - */
                                                                /* Rx pkt as long as end of file is not rx'd.           */
            if (find_eof == DEF_YES) {
                if (rx_reply_ctr <= 1u) {
                    p_search_str  = Str_Char_N(p_search_str, len, FTP_ASCII_CR);
                    if ( (p_search_str       != (void *)0)     &&
                       (*(p_search_str + 1u) ==  FTP_ASCII_LF)) {
                        parse_done    = DEF_YES;
                                                                /* Chk reply code.                                      */
                        reply_code    = Str_ParseNbr_Int32U(p_buf, 0, 10);  
                        p_search_str += 2u;

                        if (*p_search_str != (CPU_CHAR)'\0') {  /* Check if rest of buf has reply data (See Note #1a).  */
                            len = Str_Len(p_search_str) + 1u;
                                                                /* Prepend "/" delimiter at the beginning of buffer.    */
                            Str_Copy_N(p_buf, FTP_MULTI_REPLY_DELIM, 1u);
                            Str_Copy_N(p_buf + 1u, p_search_str, len);

                            if (p_ctrl_data != (CPU_CHAR *)0) {
                                if (len > ctrl_data_len) {
                                    len = ctrl_data_len;
                                }
                                Str_Copy_N(p_ctrl_data, p_buf, len);
                            }
                        } else {
                            if (p_ctrl_data != (CPU_CHAR *)0) {
                                Str_Copy_N(p_ctrl_data, p_buf, ctrl_data_len);
                            }
                            (rx_reply_tot_len > 0)                         ?
                            (Mem_Clr(&p_conn->Buf[0u], rx_reply_pkt_len))  :
                            (Mem_Clr(&p_conn->Buf[0u], buf_len));  
                        }
                    } else if (rx_reply == DEF_NO) {
                        rx_reply         = DEF_YES;
                    }
                } else {
                    if ((token_found      == DEF_TRUE) || 
                        (truncated_parsed == DEF_TRUE)) {
                        len = Str_Len(multiline_token);
                                                                /* Check whether terminating "\r\n" itself is split ... */
                                                                /* ... between buffer reads if new buffer contains  ... */
                                                                /* ... only one byte.                                   */
                        if (( rx_reply_pkt_len         == 1u)            &&
                            (*p_buf                    == FTP_ASCII_LF)  &&
                            ( multiline_token[len - 1] == FTP_ASCII_CR))  {
                            p_search_str     = end_of_cmd;      /* Token was found, just point to end-of-command string.*/
                            p_buf            = p_search_str;
                            truncated_parsed = DEF_TRUE;
                        } else { 
                            p_search_str = p_buf;
                            len          = token_found ? Str_Len(p_search_str) : rx_reply_pkt_len;
                        }
                    } else {
                        len = buf_len - (p_search_str - p_buf);
                    }
                                                                /* Search for last instance of '\r' in last rx'd packet */
                    while (DEF_ON) {
                        trunc_str     = p_search_str;
                        p_search_str  = Str_Char_N(p_search_str, len, FTP_ASCII_CR);

                        if (truncated_parsed == DEF_TRUE || p_search_str == (void *)0) {
                            break;
                        } else if (p_search_str < (CPU_CHAR *)(p_conn->Buf + rx_reply_pkt_len - FTP_EOL_DELIMITER_LEN)) {
                            trunc_str    = p_search_str;
                            trunc_str   += FTP_EOL_DELIMITER_LEN;
                            p_search_str = trunc_str;
                        } else {
                            p_buf = trunc_str;
                            break;
                        }
                    }
                    if ( (p_search_str       != (void *)0)     &&
                       (*(p_search_str + 1u) ==  FTP_ASCII_LF)) {
                        parse_done    = DEF_YES;
                                                                /* Chk reply code.                                      */
                        reply_code    = (token_found || truncated_parsed)        ?
                                        (Str_ParseNbr_Int32U(end_of_cmd, 0, 10)) :
                                        (Str_ParseNbr_Int32U(p_buf,      0, 10));
                        p_search_str += 2u;

                        if (*p_search_str != (CPU_CHAR)'\0') {
                            len = buf_len - (p_search_str - p_buf);
                            Str_Copy_N(p_buf, p_search_str, len);
                   
                            if (p_ctrl_data != (CPU_CHAR *)0) {
                                if (len > ctrl_data_len) {
                                    len = ctrl_data_len;
                                }
                                Str_Copy_N(p_ctrl_data, p_buf, len);
                            }
                        } else {
                            if (p_ctrl_data != (CPU_CHAR *)0) {
                                Str_Copy_N(p_ctrl_data, p_buf, ctrl_data_len);
                            } 
                        }
                        Mem_Clr(&p_conn->Buf[0u], (rx_reply_pkt_len));
                    } else if (rx_reply == DEF_NO) {
                        rx_reply = DEF_YES;
                    }
                }
            }
        } else if (rx_reply_pkt_len < 0) {
            return (0u);
        }
    }

    NetSock_CfgTimeoutRxQ_Set(p_conn->SockID, NET_TMR_TIME_INFINITE, p_err);

    return (reply_code);
}


/*
*********************************************************************************************************
*                                             FTPc_RxReply()
*
* Description : Receive reply data.
*
* Argument(s) : sock_id     TCP socket ID.
*
*               p_data      Pointer to  buffer  that will receive data.
*
*               data_len    Size of the buffer  that will receive data.
*
*               p_err       Pointer to variable that will receive the return error code from NetSock_RxData().
*
* Return(s)   : Number of positive data octets received, if NO errors;
*               0                                        otherwise.
*
* Caller(s)   : FTPc_WaitForStatus().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT32S  FTPc_RxReply (CPU_INT32S   sock_id,
                                  CPU_CHAR    *p_data,
                                  CPU_INT16U   data_len,
                                  NET_ERR     *p_err)
{
    const  FTPc_CFG     *p_cfg;
           CPU_INT32S    pkt_size;
           CPU_BOOLEAN   rx_done;


    p_cfg = FTPc_CfgPtr;

    pkt_size = 0;
    rx_done  = DEF_NO;

    while (rx_done != DEF_YES) {

        pkt_size = NetSock_RxData(sock_id,
                                  p_data,
                                  data_len,
                                  NET_SOCK_FLAG_NONE,
                                  p_err);
        switch (*p_err) {
            case NET_SOCK_ERR_NONE:
                 rx_done = DEF_YES;
                 break;


            case NET_ERR_RX:                                    /* If transitory rx err(s), ...                         */
            case NET_ERR_INIT_INCOMPLETE:
            case NET_SOCK_ERR_RX_Q_EMPTY:
            case NET_ERR_FAULT_LOCK_ACQUIRE:
                 KAL_Dly(p_cfg->CtrlRxMaxDly_ms);
                 FTPc_TRACE_DBG(("FTPc NetSock_RxData(): error #%u, line #%u.\n", (unsigned int)*p_err, (unsigned int)__LINE__));
                 break;


            default:
                 pkt_size = 0;
                 rx_done  = DEF_YES;
                 FTPc_TRACE_DBG(("FTPc NetSock_RxData() failed: error #%u, line #%u.\n", (unsigned int)*p_err, (unsigned int)__LINE__));
                 break;
        }
    }

    return (pkt_size);
}


/*
*********************************************************************************************************
*                                               FTPc_Tx()
*
* Description : Transmit data to TCP socket, handling transient errors and incomplete buffer transmit.
*
* Argument(s) : sock_id         Socket descriptor/handle identifier of socket to transmit application data.
*
*               p_data          Pointer to application data to transmit.
*
*               data_len        Length  of data to transmit (in octets).
*
*               timeout_ms      Transmit timeout value per attempt/retry :
*
*                                   0,                          if current configured timeout value desired
*                                                                   [or NO timeout for datagram sockets
*                                                                   (see Note #5)].
*                                   NET_TMR_TIME_INFINITE,      if infinite (i.e. NO timeout) value desired.
*                                   In number of milliseconds,  otherwise.
*
*               retry_max       Maximum number of consecutive socket transmit retries.
*
*               time_dly_ms     Transitory transmit delay value, in milliseconds.
*
*               p_err           Pointer to variable that will receive the return error code from NetSock_TxData().
*
* Return(s)   : DEF_FAIL        transmission failed.
*               DEF_OK          transmission successful.
*
* Caller(s)   : FTPc_Open(),
*               FTPc_Close(),
*               FTPc_RecvBuf(),
*               FTPc_SendBuf(),
*               FTPc_RecvFile(),
*               FTPc_SendFile().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  FTPc_Tx (CPU_INT32S   sock_id,
                              CPU_CHAR    *p_data,
                              CPU_INT16U   data_len,
                              CPU_INT32U   timeout_ms,
                              CPU_INT16U   retry_max,
                              CPU_INT32U   time_dly_ms,
                              NET_ERR     *p_err)
{
    CPU_CHAR     *tx_buf;
    CPU_INT16S    tx_buf_len;
    CPU_INT16S    tx_len;
    CPU_INT16S    tx_len_tot;
    CPU_INT32U    timeout_ms_cfgd;
    CPU_INT32U    tx_retry_cnt;
    CPU_BOOLEAN   tx_done;
    CPU_BOOLEAN   tx_dly;


    if (timeout_ms != 0) {
        timeout_ms_cfgd = NetSock_CfgTimeoutTxQ_Get_ms(sock_id, p_err);
        NetSock_CfgTimeoutRxQ_Set(sock_id, timeout_ms, p_err);
    }

    tx_len_tot   = 0;
    tx_retry_cnt = 0;
    tx_done      = DEF_NO;
    tx_dly       = DEF_NO;
    while ((tx_len_tot   <  data_len ) &&                       /* While tx tot len < buf len ...                       */
           (tx_retry_cnt <  retry_max) &&                       /* ... & tx retry   < MAX     ...                       */
           (tx_done      == DEF_NO   )) {                       /* ... & tx NOT done;         ...                       */

        if (tx_dly == DEF_YES) {                                /* Dly tx, if req'd.                                    */
            KAL_Dly(time_dly_ms);
        }

        tx_buf     = p_data   + tx_len_tot;
        tx_buf_len = data_len - tx_len_tot;
        tx_len     = NetSock_TxData(sock_id,                    /* ... tx data.                                         */
                                    tx_buf,
                                    tx_buf_len,
                                    NET_SOCK_FLAG_NONE,
                                    p_err);
        switch (*p_err) {
            case NET_SOCK_ERR_NONE:
                 if (tx_len > 0) {                              /* If          tx len > 0, ...                          */
                     tx_len_tot += tx_len;                      /* ... inc tot tx len.                                  */
                     tx_dly      = DEF_NO;
                 } else {                                       /* Else dly next tx.                                    */
                     tx_dly      = DEF_YES;
                 }
                 tx_retry_cnt = 0;
                 break;


            case NET_ERR_TX:                                    /* If transitory tx err(s), ...                         */
            case NET_ERR_INIT_INCOMPLETE:
            case NET_SOCK_ERR_PORT_NBR_NONE_AVAIL:
            case NET_CONN_ERR_NONE_AVAIL:
#ifdef  NET_IPv4_MODULE_EN
            case NET_IPv4_ERR_ADDR_NONE_AVAIL:
            case NET_IPv4_ERR_ADDR_CFG_IN_PROGRESS:
#endif
#ifdef  NET_IPv6_MODULE_EN
            case NET_IPv6_ERR_ADDR_NONE_AVAIL:
            case NET_IPv6_ERR_ADDR_CFG_IN_PROGRESS:
#endif
            case NET_ERR_FAULT_LOCK_ACQUIRE:
                 tx_dly = DEF_YES;                              /* ... dly next tx.                                     */
                 tx_retry_cnt++;
                 break;


            default:
                 return (DEF_FAIL);
        }
    }

    if (*p_err != NET_SOCK_ERR_NONE) {
        return (DEF_FAIL);
    }

    if (timeout_ms != 0) {
        NetSock_CfgTimeoutRxQ_Set(sock_id, timeout_ms_cfgd, p_err);
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                             FTPc_Conn()
*
* Description : Connect a data socket in PASV or EPSV mode.
*
* Argument(s) : p_conn          Pointer to FTPc Connection object.
*
*               p_ctrl_buf      Pointer to buffer that contain the result of status message.
*
*               ctrl_buf_size   Size of control buffer.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FTPc_ERR_NONE                   Data Connection was successful.
*                               FTPc_ERR_FAULT                  Faulted because of invalid IP address family.
*                               FTPc_ERR_TX_CMD                 Sending Command failed.
*                               FTPc_ERR_RX_CMD_RESP_INVALID    Receiving Command response failed.
*                               FTPc_ERR_CONN_FAIL              Connection failed.
*
* Return(s)   : Data socket ID,   if no error.
*               NET_SOCK_ID_NONE, otherwise.
*
* Caller(s)   : FTPc_RecvBuf(),
*               FTPc_SendBuf(),
*               FTPc_RecvFile(),
*               FTPc_SendFile().
*
* Note(s)     : none.
*********************************************************************************************************
*/
static  NET_SOCK_ID  FTPc_Conn (FTPc_CONN   *p_conn,
                                CPU_CHAR    *p_ctrl_buf,
                                CPU_INT16U   ctrl_buf_size,
                                FTPc_ERR    *p_err)
{
#ifdef  NET_IPv4_MODULE_EN
           NET_IPv4_ADDR             server_ipv4;
           CPU_INT32S                tmp_val;
#endif
#ifdef  NET_IPv6_MODULE_EN
           NET_SOCK_ADDR_IPv6       *p_sock_addr_ipv6;
#endif
    const  FTPc_CFG                 *p_cfg;
           NET_APP_SOCK_SECURE_CFG  *p_secure = DEF_NULL;
           CPU_INT08U               *p_addr;
           NET_IP_ADDR_FAMILY        ip_addr_family;
           NET_SOCK_ID               sock_dtp_id;
           CPU_INT32U                buf_size;
           CPU_INT16U                server_port;
           CPU_CHAR                 *tmp_buf;
           CPU_INT32U                reply_code;
           CPU_BOOLEAN               rtn_code;
           NET_ERR                   err;


    p_cfg = FTPc_CfgPtr;

    switch (p_conn->SockAddrFamily) {
#ifdef  NET_IPv4_MODULE_EN
        case NET_IP_ADDR_FAMILY_IPv4:
             buf_size = Str_FmtPrint((char *)p_ctrl_buf, ctrl_buf_size, "%s\r\n", FTPc_Cmd[FTP_CMD_PASV].CmdStr);
             break;
#endif

        case NET_IP_ADDR_FAMILY_IPv6:
             buf_size = Str_FmtPrint((char *)p_ctrl_buf, ctrl_buf_size, "%s\r\n", FTPc_Cmd[FTP_CMD_EPSV].CmdStr);
             break;

        default:
            *p_err = FTPc_ERR_FAULT;
             return (NET_SOCK_ID_NONE);
    }

    rtn_code = FTPc_Tx(p_conn->SockID,
                       p_ctrl_buf,
                       buf_size,
                       p_cfg->CtrlTxMaxTimout_ms,
                       p_cfg->CtrlTxMaxRetry,
                       p_cfg->CtrlTxMaxDly_ms,
                      &err);
    FTPc_TRACE_INFO(("FTPc TX: %s", p_ctrl_buf));
    if (rtn_code == DEF_FAIL) {
       *p_err = FTPc_ERR_TX_CMD;
        return (NET_SOCK_ID_NONE);
    }

                                                                /* Receive status line.                                 */
    reply_code = FTPc_WaitForStatus(p_conn, p_ctrl_buf, ctrl_buf_size, &err);


    switch (p_conn->SockAddrFamily) {
#ifdef  NET_IPv4_MODULE_EN
        case NET_IP_ADDR_FAMILY_IPv4:
             if (reply_code != FTP_REPLY_CODE_ENTERPASVMODE) {
                *p_err = FTPc_ERR_RX_CMD_RESP_INVALID;
                 return (NET_SOCK_ID_NONE);
             }

             server_ipv4  = 0;
             server_port  = 0;
             tmp_buf      = Str_Char(p_ctrl_buf, '(');
             tmp_buf++;

             tmp_val      = Str_ParseNbr_Int32U(tmp_buf, &tmp_buf, 10);  /* Get IP Address MSB.                                  */
             tmp_buf++;
             server_ipv4 += tmp_val << 24;

             tmp_val      = Str_ParseNbr_Int32U(tmp_buf, &tmp_buf, 10);
             tmp_buf++;
             server_ipv4 += tmp_val << 16;

             tmp_val      = Str_ParseNbr_Int32U(tmp_buf, &tmp_buf, 10);
             tmp_buf++;
             server_ipv4 += tmp_val << 8;

             tmp_val      = Str_ParseNbr_Int32U(tmp_buf, &tmp_buf, 10);  /* Get IP Address LSB.                                  */
             tmp_buf++;
             server_ipv4 += tmp_val << 0;

             tmp_val      = Str_ParseNbr_Int32U(tmp_buf, &tmp_buf, 10);  /* Get IP Port MSB.                                     */
             tmp_buf++;
             server_port += tmp_val << 8;

             tmp_val      = Str_ParseNbr_Int32U(tmp_buf, &tmp_buf, 10);  /* Get IP Port LSB.                                     */
             tmp_buf++;
             server_port += tmp_val << 0;

             ip_addr_family =  NET_IP_ADDR_FAMILY_IPv4;
             p_addr         = (CPU_INT08U *)&server_ipv4;
             break;
#endif

#ifdef  NET_IPv6_MODULE_EN
        case NET_IP_ADDR_FAMILY_IPv6:
             if (reply_code != FTP_REPLY_CODE_ENTEREXTPASVMODE) {
                *p_err = FTPc_ERR_RX_CMD_RESP_INVALID;
                 return (NET_SOCK_ID_NONE);
             }

             tmp_buf          =  Str_Str(p_ctrl_buf, FTP_ASCII_EPSV_PREFIX);
             tmp_buf          =  tmp_buf + Str_Len(FTP_ASCII_EPSV_PREFIX);
             server_port      =  Str_ParseNbr_Int32U(tmp_buf, &tmp_buf, 10);
             ip_addr_family   =  NET_IP_ADDR_FAMILY_IPv6;
             p_sock_addr_ipv6 = (NET_SOCK_ADDR_IPv6 *)&p_conn->SockAddr;
             p_addr           = (CPU_INT08U         *)&p_sock_addr_ipv6->Addr;
             break;
#endif

        default:
            *p_err = FTPc_ERR_FAULT;
             return (NET_SOCK_ID_NONE);
    }

#ifdef  NET_SECURE_MODULE_EN
    p_secure    = (NET_APP_SOCK_SECURE_CFG *)p_conn->SecureCfgPtr;
#endif
    sock_dtp_id = NetApp_ClientStreamOpen( p_addr,
                                           ip_addr_family,
                                           server_port,
                                           DEF_NULL,
                                           p_secure,
                                           p_cfg->DTP_ConnMaxTimout_ms,
                                          &err);
    switch (err) {
        case NET_APP_ERR_NONE:
             break;

        case NET_APP_ERR_CONN_FAIL:
        case NET_ERR_IF_LINK_DOWN:
            *p_err = FTPc_ERR_CONN_FAIL;
             return (NET_SOCK_ID_NONE);

        default:
           *p_err = FTPc_ERR_FAULT;
            return (NET_SOCK_ID_NONE);
    }

   *p_err = FTPc_ERR_NONE;

    return (sock_dtp_id);
}
