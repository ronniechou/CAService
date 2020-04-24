#ifndef AUTHORITYPARAMETER_H
#define AUTHORITYPARAMETER_H

#include "UFC.h"
#define CLUSTER                                     // add white list to listen request from other IP (fail over)
#define DOWNLOAD_REMOTE_DATA                        // use curl-scp to download remote CADATA
#define COVER_IP_LIMIT 20

#define MSG_GET_B2B_UK_FAIL         "FAIL TO GET B2B UK"
#define MSG_CULR_INIT_FAIL          "FAIL TO INIT CURL"
#define MSG_RECORD_CADATA_FAIL      "FAIL TO RECORD CADATA"
#define MSG_RECORD_CADATA_SUCCESS   "SUCCESSFUL TO RECORD CADATA"
#define MSG_CA_VERIFY_FAIL          "CA VERIFY FAIL"
#define MSG_CA_VERIFY_OK            "CA VERIFY OK"

#define LOGONTIMEOUT                            3
#define CLIENT_MBUS_KEY                         "ClientMbusKey"
#define CLIENT_CA_SIGNATURE                     "CASIGNATURE"

#define CLIENT_ORDER_DATE                       "DATE"
#define CLIENT_ORDER_MARKET                     "MARKET"
#define CLIENT_ORDER_NID                        "NID"
#define CLIENT_BROKERID                         "BROKERID"
#define CLIENT_VERIFIED_NO                      "VERIFIEDNO"

#define CLIENT_ID                               "ID"
#define CLIENT_ACCOUNT                          "ACCOUNT"
#define CLIENT_IP                               "IP"
#define CLIENT_CA_SUBJECT                       "CASUBJECT"
#define CLIENT_CA_SERIAL_NUMBER                 "CASERIALNUMBER"
#define CLIENT_CA_NOTAFTER                      "CANOTAFTER"
#define CLIENT_CA_RESULT_ACCEPT                 "ACCEPT"
#define CLIENT_CA_RESULT_MSG                    "MSG"
#define CLIENT_ORDER                            "ORDER"
#define REPLY_SUBJECT                           "SPEEDY.CA.REPLY"
#define LISTEN_SUBJECT                          "SPEEDY.CA"
#define REQUEST_FROM                            "REQUEST_FROM"
#define REQUEST_FROM_CARECORD                   "CARecord"



extern UFC::AnsiString          g_asClientLogonSubject;             // MBus Subject
extern UFC::AnsiString          g_asClientLogonKey;                 // MBus Key
extern UFC::AnsiString          g_asCAServerIP;                     // IP of Authority Server
extern UFCType::Int32           g_iCAServerPort;                    // Port no. of Authority Server 
extern UFC::AnsiString          g_asConfigName;                     // Config file name
extern UFC::AnsiString          g_asLogPath;  
extern UFC::Int32               g_iDEBUG_LEVEL;            
extern BOOL                     g_bRunning;
extern UFC::AnsiString          g_asLocalIP;
extern BOOL                     g_bCAByHttp;
extern UFC::AnsiString          g_asCAServerURL;
extern UFC::AnsiString          g_asRecordCALogURL;
extern UFC::AnsiString          g_asB2B_UK;
extern UFC::AnsiString          g_asLogDate;
extern UFC::AnsiString          g_asAppName;
extern UFC::AnsiString          g_asMBusServerIP;

extern UFC::AnsiString          g_asLogPrefixName;
extern UFC::AnsiString          g_asSpeedyCADataPath;
extern UFC::AnsiString          g_asSpeedyCAData;
extern UFC::AnsiString          g_asSpeedyStandardCAData;
extern UFC::AnsiString          g_asCAServiceCAData;
extern UFC::AnsiString          g_asCAServiceFailLog;
extern UFC::AnsiString          g_asCAServiceRecoverLog;
extern UFC::AnsiString          g_asCAServiceStandardRecoverLog;
extern UFC::PHashMap<UFC::AnsiString,UFC::AnsiString*>   g_CAOkMap;
extern BOOL                     g_bFirstRun;

extern UFC::List<UFC::Int32>    g_iListWhiteList;
extern UFC::AnsiString          g_asArrayCoverIPList[COVER_IP_LIMIT][5];
extern UFC::Int32               g_iArrayCoverIPCounts;

extern BOOL                     g_bIsRemoteHost;
extern UFC::AnsiString          g_asRemoteHostID;
extern UFC::AnsiString          g_asRemoteHostPW;

#endif /* AUTHORITYPARAMETER_H */

