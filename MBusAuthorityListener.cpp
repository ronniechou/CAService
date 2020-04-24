/* 
 * File:   MBusAuthorityListener.cpp
 * Author: Yuanta
 *
 * Created on Feb 24, 2017
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <libgen.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "AuthorityParameter.h"
#include "MBusAuthorityListener.h"
#include "ecgw.h"
#include "common.h"
#include <curl/curl.h>

#include <iostream>
using namespace std;

//------------------------------------------------------------------------------
void MBusAuthorityListener::OnConnect( UFC::PClientSocket *Socket)
{    
    UFC::BufferedLog::Printf( " [%s][%s] connect to %s:%d succeed.", __FILE__,__func__,FSocketObject->GetPeerIPAddress().c_str(),FSocketObject->GetPort());     
    return;
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::OnDisconnect( UFC::PClientSocket *Socket)
{
    UFC::BufferedLog::Printf( " [%s][%s] OnDisconnect!", __FILE__,__func__);
    return;
}
//------------------------------------------------------------------------------
BOOL MBusAuthorityListener::OnDataArrived( UFC::PClientSocket *Socket)
{
    //UFC::BufferedLog::Printf( " [%s][%s] OnDataArrived!", __FILE__,__func__);
    UFC::UInt8      rcvb[4096];
    int iRecvSize   =0;
    
    ///> receive data from EC server
    memset(rcvb,0,sizeof(rcvb));
    iRecvSize = Socket->RecvBuffer( rcvb,sizeof(rcvb) );    
    UFC::BufferedLog::Printf( " [%s][%s] receive buffer=%s", __FILE__,__func__,rcvb);
    
    ///> check result
    ProcessCAResponse(rcvb);  
    
    ///> triggle event to release next process for ProcessCARequest()   
    FLogonEvent.SetEvent();  
    return true;
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::OnIdle( UFC::PClientSocket *Socket)
{
    UFC::BufferedLog::Printf( " [%s][%s] OnIdle!", __FILE__,__func__);
    return;
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::OnMigoMessage( const UFC::AnsiString& Subject, const UFC::AnsiString& Key, MTree* mtData )
{
    UFC::BufferedLog::Printf(" ");
    UFC::BufferedLog::Printf( " [%s][%s] Subject=%s,Key=%s",  __FILE__,__FUNCTION__,Subject.c_str(),Key.c_str());    
    
#ifdef CLUSTER  
        ///> if request is from MBus (not from file) => make sure ip in white list. 
        UFC::Int32         iRemoteIP;
        mtData->get( "_IP", iRemoteIP );
        if( g_iListWhiteList.IndexOf(iRemoteIP) < 0 )
        {
            UFC::BufferedLog::Printf(" [%s][%s] _IP=%d(%s) not in white list.", __FILE__,__func__, iRemoteIP,ConvertIntIPtoString(iRemoteIP) ); 
            return;
        }
#endif
    
    if (Subject != g_asClientLogonSubject)
    {
        //UFC::BufferedLog::Printf( " [%s][%s] Subject & Key not Match!",  __FILE__,__FUNCTION__);
        return;
    }
    mtData->append(CLIENT_MBUS_KEY,Key);
    FAuthorityQueue.Inqueue( new MTree(*mtData) );
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::InitParameter()
{
    FCA_Date ="";   
    FCA_Market ="";   
    FCA_NID ="";   
    FCA_ID ="";   
    FCA_BrokerID ="";   
    FCA_Account ="";   
    FCA_IP ="";  
    FCA_Verified_NO ="";
    FCA_Subject ="";
    FCA_Order ="";
    FCA_Signature ="";
    FClientMbusKey ="";
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::InQueue(MTree* mtData,UFC::AnsiString asMBusKey)
{    
    mtData->append(CLIENT_MBUS_KEY,asMBusKey.c_str() );
    FAuthorityQueue.Inqueue( new MTree(*mtData) );
}
//------------------------------------------------------------------------------
MBusAuthorityListener::MBusAuthorityListener(CheckSystemListener* pCheckSystemListener)
: PThread( NULL )
, FThreadName( "FMBusAuthorityThread" )
, FAuthorityQueue()
, FAPPListener(pCheckSystemListener)
{   
    if(g_bCAByHttp)
        FSocketObject = NULL;
    else
    {
        FSocketObject = new UFC::PClientSocket(g_asCAServerIP,g_iCAServerPort);
    
        ///setup Listener for PClientSocket    
        FSocketObject->SetListener(this);
        FSocketObject->Start();        
    }
}
//------------------------------------------------------------------------------
MBusAuthorityListener::~MBusAuthorityListener()
{
    Terminate();    
    UFC::SleepMS(1000); // to avoid core dump.
    if(FSocketObject != NULL)
    {
        UFC::BufferedLog::Printf(" [%s][%s]  delete FSocketObject...", __FILE__,__FUNCTION__);
        delete FSocketObject;
        FSocketObject = NULL;
        UFC::BufferedLog::Printf(" [%s][%s]  delete FSocketObject...done", __FILE__,__FUNCTION__);
    }
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::Run( void )
{
    Start();
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::Execute( void )
{
    UFC::BufferedLog::Printf( " [%s][%s] ",  __FILE__,__FUNCTION__);    
    while ( ! IsTerminated()  )
    {   
        InitParameter();
        MTree* GetMTree = FAuthorityQueue.Dequeue( 1 );
        
        //if(GetMTree != NULL)
        //{
        if(GetMTree == NULL)
            continue;
            
            if( !GetMTree->get(CLIENT_CA_SIGNATURE, FCA_Signature ) )
            {
                UFC::BufferedLog::Printf( " [%s][%s] loss info for CASIGNATURE ",  __FILE__,__FUNCTION__);
                continue;
            }
            //UFC::BufferedLog::Printf( " [%s][%s] CADATA=%s",  __FILE__,__FUNCTION__,asCADATA.c_str());
            
            GetMTree->get(CLIENT_ORDER_DATE, FCA_Date);
            GetMTree->get(CLIENT_ORDER_MARKET, FCA_Market);   
            if( !GetMTree->get(CLIENT_ORDER_NID, FCA_NID ) )           
                FCA_NID = "null";           
            GetMTree->get(CLIENT_ID, FCA_ID );              //ClientID
            GetMTree->get(CLIENT_BROKERID, FCA_BrokerID );              //BrokerID
            GetMTree->get(CLIENT_ACCOUNT, FCA_Account );       //Client帳號 
            GetMTree->get(CLIENT_IP, FCA_IP );              //ClientIP
            GetMTree->get(CLIENT_VERIFIED_NO, FCA_Verified_NO );              //VERIFIED_NO
            GetMTree->get(CLIENT_CA_SUBJECT, FCA_Subject);    //CA Subject
            GetMTree->get(CLIENT_ORDER, FCA_Order );           //明文
            GetMTree->get(CLIENT_MBUS_KEY, FClientMbusKey);
            
            /*
            UFC::BufferedLog::Printf( " [%s][%s] FCA_Date=%s", __FILE__,__func__,FCA_Date.c_str() );
            UFC::BufferedLog::Printf( " [%s][%s] FCA_Market=%s", __FILE__,__func__,FCA_Market.c_str() );
            UFC::BufferedLog::Printf( " [%s][%s] FCA_NID=%s", __FILE__,__func__,FCA_NID.c_str() );
            UFC::BufferedLog::Printf( " [%s][%s] FCA_ID=%s", __FILE__,__func__,FCA_ID.c_str() );
            UFC::BufferedLog::Printf( " [%s][%s] FCA_BrokerID=%s", __FILE__,__func__,FCA_BrokerID.c_str() );
            UFC::BufferedLog::Printf( " [%s][%s] FCA_Account=%s", __FILE__,__func__,FCA_Account.c_str() );
            UFC::BufferedLog::Printf( " [%s][%s] FCA_IP=%s", __FILE__,__func__,FCA_IP.c_str() );
            UFC::BufferedLog::Printf( " [%s][%s] FCA_Verified_NO=%s", __FILE__,__func__,FCA_Verified_NO.c_str() );
            UFC::BufferedLog::Printf( " [%s][%s] FCA_Subject=%s", __FILE__,__func__,FCA_Subject.c_str() );
            UFC::BufferedLog::Printf( " [%s][%s] FCA_Order=%s", __FILE__,__func__,FCA_Order.c_str() );
            UFC::BufferedLog::Printf( " [%s][%s] FCA_Signature=%s", __FILE__,__func__,FCA_Signature.c_str() );    
             * */        
            
            ///> send logon request to Server
            UFC::AnsiString asErrMsg;
            //if( ProcessCARequest(asCAData,asOrder,asPeerIP,asID,asCASubject,&asErrMsg) == FALSE)
            if( ProcessCARequest(&asErrMsg) == FALSE)
            {
               ReturnResponse(FALSE,asErrMsg);
               SaveCAResultToFile(FALSE,asErrMsg);
               UFC::BufferedLog::Printf( " [%s][%s] %s,NID=%s,msg=%s", __FILE__,__func__,MSG_CA_VERIFY_FAIL,FCA_NID.c_str(),asErrMsg.c_str() );
            }            
            delete GetMTree;
            GetMTree = NULL;
        //} 
    }
}
//------------------------------------------------------------------------------
//BOOL MBusAuthorityListener::ProcessCARequest( UFC::AnsiString asCAData,UFC::AnsiString asOrder,UFC::AnsiString asPeerIP,UFC::AnsiString asID,UFC::AnsiString asCASubject,UFC::AnsiString *pasErrMsg )
BOOL MBusAuthorityListener::ProcessCARequest(UFC::AnsiString *pasErrMsg )
{
    if(g_bCAByHttp)        
        return ProcessCARequestbyHttp(pasErrMsg);
    else
        //return ProcessCARequestbySocket(asCAData,asOrder,asPeerIP,asID,asCASubject,pasErrMsg);
        return ProcessCARequestbySocket(pasErrMsg);
}
//------------------------------------------------------------------------------
size_t CURLCallback(void * ptr, size_t size, size_t nmemb, void *data)
{
    UFC::AnsiString* pMsg = (UFC::AnsiString*)data;
    size_t totalSize = size*nmemb;
    pMsg->AppendPrintf("%s",(char*)ptr);
    return totalSize; 
}
//------------------------------------------------------------------------------
BOOL MBusAuthorityListener::ProcessCARequestbyHttp(UFC::AnsiString *pasErrMsg)
{
    CURL *curl = NULL;
    CURLcode res;
    UFC::AnsiString asPostData;    
    asPostData.Printf("signature=%s&account=%s&data=%s",FCA_Signature.c_str(),FCA_Account.c_str(),FCA_Order.c_str() );
    try
    {
        if(!curl )
            curl_easy_cleanup(curl);
        curl = curl_easy_init();
        if(!curl)
        {
            pasErrMsg->Printf("%s",MSG_CULR_INIT_FAIL); 
            return FALSE;
        } 
        curl_easy_setopt(curl,CURLOPT_URL,g_asCAServerURL.c_str() );    
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);      ///> trust server ca
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CURLCallback); ///> setup callback
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void * )pasErrMsg);   ///> setup input parameter for callback
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, LOGONTIMEOUT);           ///> set timeout
        curl_easy_setopt(curl, CURLOPT_POST, 1);            ///> use POST
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, asPostData.c_str());   ///> assign post data

        res = curl_easy_perform(curl);    
        if (res != CURLE_OK)
        {
            pasErrMsg->Printf("%s",curl_easy_strerror(res));
            return FALSE;
        }
        curl_easy_cleanup(curl);
    }
    catch( UFC::Exception &e )
    {
        pasErrMsg->Printf("UFC exception occurred:%s",e.what());        
        return FALSE;
    }
    catch(...)
    {
        pasErrMsg->Printf("Unknown exception occurred");        
        return FALSE;
    }    
    
    ///> check result 
    ProcessCAResponse(pasErrMsg);
    return TRUE;
}
//------------------------------------------------------------------------------
//BOOL MBusAuthorityListener::ProcessCARequestbySocket( UFC::AnsiString asCAData,UFC::AnsiString asOrder,UFC::AnsiString asPeerIP,UFC::AnsiString asID,UFC::AnsiString asCASubject,UFC::AnsiString *pasErrMsg )
BOOL MBusAuthorityListener::ProcessCARequestbySocket(UFC::AnsiString *pasErrMsg)
{
    if( FSocketObject == NULL)
    {
        pasErrMsg->Printf("FSocketObject is NULL!");              
        return FALSE;
    }     
    try
    { 
        char sndb[10240];
        int iSndl = 10240;
        UFC::AnsiString asSendData,asEncodeOrder;
        //asEncodeOrder = base64_encode(reinterpret_cast<const unsigned char*>(asOrder.c_str()), asOrder.Length() );        
        //asSendData.Printf("%s;%s;%s;%s;%s",asID.c_str(),asEncodeOrder.c_str(),asCAData.c_str(),asCASubject.c_str(),asPeerIP.c_str());        
        //asSendData.Printf("Y120549473;MjAxNzEwMDYgNTE2IDUxNjQ1MzggtMGzZiBGWEZKNyBCIDEgra27+TExMzEuNiAgICAgUk9EIKbbsMog;MIIGsQYJKoZIhvcNAQcCoIIGojCCBp4CAQExCzAJBgUrDgMCGgUAMEsGCSqGSIb3DQEHAaA+BDwyMDE3MTAwNiA1MTYgNTE2NDUzOCC0wbNmIEZYRko3IEIgMSCtrbv5MTEzMS42ICAgICBST0QgptuwyiCgggSdMI;aa;22");
        //UFC::BufferedLog::Printf( " [%s][%s] send buffer=%s", __FILE__,__func__,asSendData.c_str()  );
        
        asEncodeOrder = base64_encode(reinterpret_cast<const unsigned char*>(FCA_Order.c_str()), FCA_Order.Length() ); 
        asSendData.Printf("%s;%s;%s;%s;%s",FCA_ID.c_str(),asEncodeOrder.c_str(),FCA_Signature.c_str(),FCA_Subject.c_str(),FCA_IP.c_str());
        UFC::BufferedLog::Printf( " [%s][%s] send buffer=%s", __FILE__,__func__,asSendData.c_str()  );
        
        memset(sndb,'\x02',sizeof(sndb) ); 
        
        if( asSendData.Length() > sizeof(sndb) )
            memcpy(sndb,asSendData.c_str() ,sizeof(sndb));
        else        
            memcpy(sndb,asSendData.c_str() ,asSendData.Length() );
        
        //for(int i = asSendData.Length();i< sizeof(sndb) ;i++)
        //    sndb[i] = '\x02';
        
        ///> connect server
        FSocketObject->Connect( LOGONTIMEOUT );
        if(!FSocketObject->IsConnect())
        {
            pasErrMsg->Printf("Connect to server  %s:%d  fail",FSocketObject->GetPeerIPAddress().c_str(),FSocketObject->GetPort() );                      
            return FALSE;
        }
        ///> send packet to server        
        //UFC::BufferedLog::Printf( " [%s][%s] iSndl=%d", __FILE__,__func__,iSndl);
        //UFC::BufferedLog::Printf( " [%s][%s] sndb=%s", __FILE__,__func__,sndb);
   
        FLogonEvent.ResetEvent();
        FSocketObject->BlockSend(sndb,iSndl);  
        
        if( FLogonEvent.WaitFor( LOGONTIMEOUT ) == FALSE )        
        {  
            pasErrMsg->Printf("Timeout,no response from server  %s:%d",FSocketObject->GetPeerIPAddress().c_str(),FSocketObject->GetPort());                 
            return FALSE;
        }
        else        
            FLogonEvent.ResetEvent();    
        
        FSocketObject->Disconnect();
    }
    catch( UFC::Exception &e )
    {
        pasErrMsg->Printf("UFC exception occurred:%s",e.what());        
        return FALSE;
    }
    catch(...)
    {
        pasErrMsg->Printf("Unknown exception occurred");        
        return FALSE;
    }    
    return TRUE;
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::ProcessCAResponse(UFC::AnsiString *pasMsg)
{
    int ret = pasMsg->Length(); 
    //struct mod stMod;    
    BOOL bAccept;
    
    if (ret == 23)
    {
        bAccept = TRUE;
        //memset(stMod.retc, '0', sizeof(stMod.retc));
        //memcpy(stMod.emsg, pasMsg, ret);
        //stMod.emsg[ret] = 0x0;
        UFC::BufferedLog::Printf( " [%s][%s] %s,NID=%s,emsg=%s", __FILE__,__func__,MSG_CA_VERIFY_OK,FCA_NID.c_str(),pasMsg->c_str());
        
        FCA_Verified_NO = *pasMsg;
        if(FCA_NID.LowerCase().AnsiPos("null") >=0 )
            FCA_NID = FCA_Verified_NO;
    }
    else
    {
        bAccept = FALSE;
        //memset(stMod.retc, '8', sizeof(stMod.retc));
        //sprintf(stMod.emsg, pasMsg);  
        UFC::BufferedLog::Printf( " [%s][%s] %s,NID=%s,msg=%s", __FILE__,__func__,MSG_CA_VERIFY_FAIL,FCA_NID.c_str(),pasMsg->c_str());
    }    
    
    ///> 傳送結果至MBus 
    ReturnResponse(bAccept,*pasMsg); 
    SaveCAResultToFile(bAccept,*pasMsg);
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::ProcessCAResponse(UFC::UInt8 *rcvb)
{
    UFC::AnsiString asMsg = (char*) rcvb;
    ProcessCAResponse(&asMsg);
/*
    int ret = strlen( (char*) rcvb);    
    struct mod stMod;    
    BOOL bAccept;
    
    if (ret == 23)
    {
        bAccept = TRUE;
        memset(stMod.retc, '0', sizeof(stMod.retc));
        memcpy(stMod.emsg, rcvb, ret);
        stMod.emsg[ret] = 0x0;
        UFC::BufferedLog::Printf( " [%s][%s] %s,NID=%s,emsg=%s", __FILE__,__func__,MSG_CA_VERIFY_OK,FCA_NID.c_str(),stMod.emsg);
        
        FCA_Verified_NO = (char*)rcvb;
        if(FCA_NID.LowerCase().AnsiPos("null") >=0 )
            FCA_NID = FCA_Verified_NO;
    }
    else
    {
        bAccept = FALSE;
        memset(stMod.retc, '8', sizeof(stMod.retc));
        sprintf(stMod.emsg, (char*) rcvb);  
        UFC::BufferedLog::Printf( " [%s][%s] %s,NID=%s,msg=%s", __FILE__,__func__,MSG_CA_VERIFY_FAIL,FCA_NID.c_str(),stMod.emsg);
    }    
    
    ///> 傳送結果至MBus 
    ReturnResponse(bAccept,stMod.emsg); 
    SaveCAResultToFile(bAccept,stMod.emsg);
*/
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::ReturnResponse(BOOL Accept,UFC::AnsiString Msg)
{
    MTree mtData;
    if(Accept)
        mtData.append(CLIENT_CA_RESULT_ACCEPT,"1");  
    else
        mtData.append(CLIENT_CA_RESULT_ACCEPT,"0");  
    if( !Msg.IsEmpty() )
        mtData.append(CLIENT_CA_RESULT_MSG,Msg);
    
    ///> To avoid MBus carsh, make sure MBusKey is not null 
    if( FClientMbusKey.IsEmpty())
    {
        if( strlen(UFC::Hostname) != 0)
            FClientMbusKey = UFC::Hostname;
        else
            FClientMbusKey = " ";
    }
    
    FAPPListener->Send(REPLY_SUBJECT,FClientMbusKey,mtData); 
    
    ///> show MTree
    UFC::AnsiString asTemp;
    for(int i=0;i<mtData.getNodeCount();i++)
    {
        MNode *pNode = mtData.get(i); 
        if(pNode->getName() == "_IP")
        {
            int *pIP = (int*)pNode->getData();            
            asTemp.AppendPrintf("%s=%s,",pNode->getName().c_str(), ConvertIntIPtoString(*pIP) );   
        }
        else
            asTemp.AppendPrintf("%s=%s,",pNode->getName().c_str(),pNode->getData());        
    }    
    UFC::BufferedLog::Printf( " [%s][%s] subject=%s,key=%s,%s", __FILE__,__func__,REPLY_SUBJECT,FClientMbusKey.c_str(),asTemp.c_str() );
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::SaveCAResultToFile(BOOL bAccept,UFC::AnsiString Msg)
{
    int size = 0;
    UFC::AnsiString asFileName,asBuffer,asData;
    
    if(bAccept)
    {
        //asFileName.Printf("%sCAServiceCA.%s.DATA",g_asLogPath.c_str(),g_asLogDate.c_str() );
        //UFC::FileStreamEx CADataFile( asFileName.c_str(), "a");      
        UFC::FileStreamEx CADataFile( g_asCAServiceCAData.c_str(), "a");  
        asBuffer.Printf("%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s\x0A",FCA_Date.c_str(),FCA_Market.c_str(),FCA_NID.c_str(),FCA_ID.c_str(),FCA_BrokerID.c_str(),FCA_Account.c_str()
                        ,FCA_IP.c_str(),FCA_Verified_NO.c_str(),FCA_Subject.c_str(),FCA_Order.c_str(),FCA_Signature.c_str() );
        size = asBuffer.Length() +1;
        asData.Printf("%4d|%s",size,asBuffer.c_str() );

        CADataFile.Write( asData.c_str(),asData.Length() );
        CADataFile.Flush();        
    }else{
        //asFileName.Printf("%sCAFail.%s.log",g_asLogPath.c_str(),g_asLogDate.c_str() );
        //UFC::FileStreamEx File( asFileName.c_str(), "a");      
        UFC::FileStreamEx File( g_asCAServiceFailLog.c_str(), "a"); 
        asData.Printf("%s|%s|%s|%s|%s|%s|%s|%s|%s|%s\x0A",FCA_Date.c_str(),FCA_Market.c_str(),FCA_NID.c_str(),FCA_ID.c_str(),FCA_BrokerID.c_str(),FCA_Account.c_str()
                        ,FCA_IP.c_str(),FCA_Order.c_str(),FCA_Verified_NO.c_str(),Msg.c_str() );       
        File.Write( asData.c_str(),asData.Length() );
        File.Flush();        
    }    
}
//------------------------------------------------------------------------------
