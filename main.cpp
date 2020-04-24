/*
 * main.cpp
 *
 *  Created on: 2017.02.24
 *      Author: YuantaFuture
 */
#include <signal.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include "CheckSystemListener.h"
#include "MBusAuthorityListener.h"
#include "NameValueMessage.h"
#include "AuthorityParameter.h"
#include "common.h"
#include "CARecord.h"
#include "iniFile.h"
#include "tinyxml2.h"
#include <curl/curl.h>
#include <ifaddrs.h>

//*************************** setup compile for which host *********************************************
#define CompileFor 1                                      //0 :local test,1:speedy
//******************************************************************************************************

using namespace std;
//-----------------------------------------------------------------------------------
// global Objects
//-----------------------------------------------------------------------------------
#if CompileFor == 0    
    UFC::AnsiString             g_asSpeedyCADataPath        = "./";               // CA Data path
#elif CompileFor == 1     
    UFC::AnsiString             g_asSpeedyCADataPath        = "/oms/Speedy/bin/";               // CA Data path
#endif

UFC::AnsiString             g_asConfigPath          = ""; 
UFC::AnsiString             g_asLogPath             = "";            // Log path      
CheckSystemListener*        FAPPListener            = NULL;         // MBus event listener
MBusAuthorityListener*      FMBusAuthorityThread    = NULL;         // MBus Authority event listener thread
CARecord*                   FCARecordThread         = NULL;         // for record cA log
UFC::AnsiString             g_asAppName             = "CAService";   // Application name
UFC::AnsiString             g_asMBusServerIP        = "127.0.0.1";
UFC::Int32                  FMBusServerPort         = 12345 ;        //< default : 12345
UFC::AnsiString             g_asClientLogonSubject  = LISTEN_SUBJECT;       // MBus Subject
UFC::AnsiString             g_asClientLogonKey      = "all";                // MBus Key
UFC::AnsiString             g_asLogPrefixName       = g_asAppName;
UFC::AnsiString             g_asSpeedyCAPrefixName  = "SpeedyCA";

///> CA server 
UFC::AnsiString             g_asCAServerIP          = "10.216.34.6";        // CA Server for UseSocket to check CA
UFCType::Int32              g_iCAServerPort         = 7777;                 // Port no. of Authority Server 
UFC::Int32                  g_iDEBUG_LEVEL          = 0;            
BOOL                        g_bRunning              = FALSE;
UFC::AnsiString             g_asLocalIP             = "127.0.0.1";
BOOL                        g_bCAByHttp             = FALSE;
UFC::AnsiString             g_asCAServerURL         = "http://10.216.34.6/taica/ss/verify_cgi.php";        // web service  URL for CAByHttp to check CA
UFC::AnsiString             g_asRecordCALogURL      = "http://10.214.19.1/IBQry/chkCA.aspx";   // web service  URL for result saving

///> B2B URL
UFC::AnsiString             g_asB2B_URL             = "http://10.214.19.1/FutureGLWeb/login/atmOnlineLogin.asp?UserID=b2ygw&userpsw=123456";
UFC::AnsiString             g_asB2B_UK              = "";
UFC::AnsiString             g_asLogDate             ="";
UFC::PHashMap<UFC::AnsiString,UFC::AnsiString*>   g_CAOkMap;
BOOL                        g_bFirstRun             = FALSE;

///> default failover info
UFC::Int32                  g_iMBusServerIP             = 0;
UFC::List<UFC::Int32>       g_iListWhiteList;
UFC::List<UFC::Int32>       g_iListLocalIPList;
UFC::AnsiString             g_asArrayCoverIPList[COVER_IP_LIMIT][5];    //IP,Host,int IP,Active(0/1),Appname
UFC::Int32                  g_iArrayCoverIPCounts       = 0;

///> CAService ini & data & log
UFC::AnsiString             g_asConfigName                  = "CAService.ini";     // Config file name
UFC::AnsiString             g_asCAServiceCAData             = "";                
UFC::AnsiString             g_asSpeedyCAData                = "";   
UFC::AnsiString             g_asCAServiceFailLog            = "";  
UFC::AnsiString             g_asCAServiceRecoverLog         = "";
UFC::AnsiString             g_asCAServiceStandardRecoverLog = "";
UFC::AnsiString             g_asSpeedyStandardCAData        = "";

///> CAService for remote host
BOOL                        g_bIsRemoteHost                 = FALSE;
UFC::AnsiString             g_asRemoteHostID                = "";
UFC::AnsiString             g_asRemoteHostPW                = "";

//-----------------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------------
void AtStart( void );
void AtSignal( int signum );
void AtExit( void );
void StopObjects( void );
void PrintStartUp( void );
void ParseConfig( void );
void Initialize( void );
void CreateLogObject(void);
void AddListenerFromConfig();

//------------------------------------------------------------------------------
///> signal handler
void AtSignal( int signum )
{
    UFC::BufferedLog::Printf(" [%s][%s] Got signal=%d", __FILE__,__FUNCTION__,signum);    
    
    if(signum == 2) //crtl+c
        exit(0);
    
    //if( FAPPListener->IsConnected() == FALSE)
    //    exit(0);
    g_bRunning = FALSE;
}
//------------------------------------------------------------------------------
void AtExit( void )
{
    g_bRunning = FALSE;
    UFC::BufferedLog::Printf(" [%s][%s]", __FILE__,__FUNCTION__);
    UFC::BufferedLog::FlushToFile();    
    
    //UFC::BufferedLog::Printf(" [%s][%s] AtExit1111,g_bRunning=%d", __FILE__,__FUNCTION__,g_bRunning);
    StopObjects(); 
    //UFC::BufferedLog::Printf(" [%s][%s] AtExit222,g_bRunning=%d", __FILE__,__FUNCTION__,g_bRunning);
}
//------------------------------------------------------------------------------
void StopObjects( void )
{
       
        UFC::BufferedLog::Printf(" [%s][%s] STOP FCARecordThread...    FCARecordThread->IsStop()=%d", __FILE__,__FUNCTION__,FCARecordThread->IsStop());
        if ( FCARecordThread )
        {            
            FCARecordThread->StopService();            
            //delete FCARecordThread;
            //FCARecordThread = NULL;
        }        
        while(! FCARecordThread->IsStop() )
        {
            UFC::BufferedLog::Printf(" [%s][%s]  FCARecordThread->IsStop()=%d, wait for STOP!", __FILE__,__FUNCTION__,FCARecordThread->IsStop());
            UFC::SleepMS(1000);
        } 
        UFC::BufferedLog::Printf(" [%s][%s] STOP FCARecordThread...done   FCARecordThread->IsStop()=%d", __FILE__,__FUNCTION__,FCARecordThread->IsStop());
        
        UFC::BufferedLog::Printf(" [%s][%s] STOP FAPPListener...", __FILE__,__FUNCTION__);
        if ( FAPPListener )
        {
            delete FAPPListener; 
            FAPPListener = NULL;
        }
        UFC::BufferedLog::Printf(" [%s][%s] STOP FAPPListener...done", __FILE__,__FUNCTION__);
        
        UFC::BufferedLog::Printf(" [%s][%s] STOP FMBusAuthorityThread...", __FILE__,__FUNCTION__);
        if ( FMBusAuthorityThread )
        {
            delete FMBusAuthorityThread; 
            FMBusAuthorityThread = NULL;
        }    
        UFC::BufferedLog::Printf(" [%s][%s] STOP FMBusAuthorityThread...done", __FILE__,__FUNCTION__);
}
//------------------------------------------------------------------------------
size_t CURLCallbackFun(void * ptr, size_t size, size_t nmemb, void *data)
{
    UFC::AnsiString* pMsg = (UFC::AnsiString*)data;
    size_t totalSize = size*nmemb;
    pMsg->AppendPrintf("%s",(char*)ptr);
    return totalSize; 
}
//------------------------------------------------------------------------------
BOOL GetB2BUK(UFC::AnsiString *pasErrMsg)
{
    CURL *curl = NULL;
    CURLcode res;
    
    //UFC::BufferedLog::Printf( " [%s][%s] URL=%s ",  __FILE__,__FUNCTION__,g_asB2B_URL.c_str() ); 
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
        curl_easy_setopt(curl,CURLOPT_URL,g_asB2B_URL.c_str() );    
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);      ///> trust server ca
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CURLCallbackFun); ///> setup callback
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)pasErrMsg);   ///> setup input parameter for callback
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, LOGONTIMEOUT);           ///> set timeout        

        res = curl_easy_perform(curl);    
        if (res != CURLE_OK)
        {
            pasErrMsg->Printf("%s",curl_easy_strerror(res));
            return FALSE;
        }
        curl_easy_cleanup(curl);
        
        // parser xml
        tinyxml2::XMLDocument doc;
        int error = doc.Parse(pasErrMsg->c_str() );   
        if(error == 0)
        {    
            tinyxml2::XMLHandle docHandle( &doc );
            tinyxml2::XMLElement* subElement = docHandle.FirstChildElement( "root" ).FirstChildElement( "result" ).ToElement();

            if(subElement)
            {     
                if( strcmp(subElement->GetText(),"00") == 0 )
                {
                    subElement = docHandle.FirstChildElement( "root" ).FirstChildElement( "UK" ).ToElement();  
                    if(subElement)
                    {                
                        pasErrMsg->Printf("%s",subElement->GetText());
                        return TRUE;
                    }else
                        return FALSE;
                }else{
                    subElement = docHandle.FirstChildElement( "root" ).FirstChildElement( "message" ).ToElement();
                    if(subElement)
                        UFC::BufferedLog::Printf( " [%s][%s] Fail to get UK,msg=%s ",  __FILE__,__FUNCTION__,subElement->GetText() );   
                    return FALSE;
                }
            }
        }else
            UFC::BufferedLog::Printf( " [%s][%s]doc.Parse fail , msg=%s ",  __FILE__,__FUNCTION__,doc.GetErrorStr1());   
            return FALSE;        
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
void ParseArg(int argc, char** argv)
{
    g_asConfigPath.Printf("%s/../cfg/",UFC::GetCurrentDir().c_str());
    g_asConfigName.Printf("%s%s",g_asConfigPath.c_str() ,"CAService.ini");
    
    for(int i=1;i<argc;i++)
    {
        UFC::AnsiString asArgv = argv[i]; 
        if( asArgv.AnsiCompare("-F")  == 0 )
            g_bFirstRun = TRUE; 
        else if( asArgv.AnsiPos("-host=") >= 0 )
        {
            g_bIsRemoteHost = TRUE;
            UFC::AnsiString asHost = asArgv.SubString( strlen("-host=") , -1 );
            asHost.TrimRight();
            if(asHost.Length() == 0)
                asHost = "127.0.0.1";
            
            g_asLogPrefixName.Printf("%s%s",g_asAppName.c_str(),asHost.c_str() );
            g_asConfigName.Printf("%s%s.ini",g_asConfigPath.c_str(),g_asLogPrefixName.c_str() );
            g_asSpeedyCAPrefixName.Printf("SpeedyCA%s",asHost.c_str() );
        }
    }
}
//------------------------------------------------------------------------------
void GetLogDate()
{
    UFC::GetYYYYMMDD(g_asLogDate,FALSE); 
    g_asLogPath.Printf("%s/../log/",UFC::GetCurrentDir().c_str());
    g_asCAServiceCAData.Printf("%s%sCA.%s.DATA",g_asLogPath.c_str(),g_asLogPrefixName.c_str(),g_asLogDate.c_str() );    
    g_asCAServiceFailLog.Printf("%s%sFail.%s.log",g_asLogPath.c_str(),g_asLogPrefixName.c_str(),g_asLogDate.c_str() );
    g_asCAServiceRecoverLog.Printf("%s%sRecover.%s.log",g_asLogPath.c_str(),g_asLogPrefixName.c_str(),g_asLogDate.c_str() );
    g_asCAServiceStandardRecoverLog.Printf("%sCAServiceRecover.%s.log",g_asLogPath.c_str(),g_asLogDate.c_str() );
    
    g_asSpeedyCAData.Printf("%s%s.%s.DATA",g_asSpeedyCADataPath.c_str(),g_asSpeedyCAPrefixName.c_str(),g_asLogDate.c_str() );
    g_asSpeedyStandardCAData.Printf("%sSpeedyCA.%s.DATA",g_asSpeedyCADataPath.c_str(),g_asLogDate.c_str() );
}
//------------------------------------------------------------------------------
void GetLocalIP()
{    
    struct sockaddr_in SocketAddress;
    UFC::Int32  intLocalIPAddress = UFC::PSocket::GetLocalIPAddress();
    //UFC::BufferedLog::Printf(" [%s][%s] intLocalIPAddress=%d,%08X", __FILE__,__func__,intLocalIPAddress,intLocalIPAddress );
    
    ///> If NOT BigEndian => reverse order of byte
    if( !IsBigEndian() )
        EndianSwap( (unsigned int&) intLocalIPAddress);  
    memcpy(  &SocketAddress.sin_addr ,   &intLocalIPAddress,   sizeof(struct sockaddr) );    
    g_asLocalIP = inet_ntoa( SocketAddress.sin_addr );
    UFC::BufferedLog::Printf(" [%s][%s] g_asLocalIP=%s", __FILE__,__func__,g_asLocalIP.c_str() );
}
//------------------------------------------------------------------------------
///> Load config for  FAppName/FMBusServerIP/FMBusServerPort
void ParseConfig()
{
    UFC::UiniFile Config(g_asConfigName);    
    UFC::AnsiString Value;
    
    if( Config.GetValue( "Setting", "AppName",Value ) == TRUE )    
        g_asAppName = Value;
    if( Config.GetValue( "Setting", "MBusServerIP",Value ) == TRUE )    
        g_asMBusServerIP = Value;
    if( Config.GetValue( "Setting", "MBusServerPort",Value ) == TRUE )    
        FMBusServerPort = Value.ToInt();
    if( Config.GetValue( "Setting", "CAServerIP",Value ) == TRUE )    
        g_asCAServerIP = Value;
    if( Config.GetValue( "Setting", "CAServerPort",Value ) == TRUE )    
        g_iCAServerPort = Value.ToInt();
    if( Config.GetValue( "Setting", "CAByHttp",Value ) == TRUE ) 
    {
        if(Value.Length() >0 && Value.UpperCase().AnsiCompare("Y") ==0 )
            g_bCAByHttp = TRUE;
        else
            g_bCAByHttp = FALSE;        
    }
    if( Config.GetValue( "Setting", "CAServerURL",Value ) == TRUE )    
        g_asCAServerURL = Value;
    if( Config.GetValue( "Setting", "RecordCALogURL",Value ) == TRUE )    
        g_asRecordCALogURL = Value;
    if( Config.GetValue( "Setting", "B2B_URL",Value ) == TRUE )    
        g_asB2B_URL = Value;
    if( Config.GetValue( "Setting", "RemoteHostID",Value ) == TRUE )    
        g_asRemoteHostID = Value;
    if( Config.GetValue( "Setting", "RemoteHostPW",Value ) == TRUE )    
        g_asRemoteHostPW = Value;
    
    /*
    fstream fin;
    fin.open(g_asConfigName.c_str(),ios::in);
    
    if(!fin)
    {
        UFC::BufferedLog::Printf(" [%s][%s] %s NOT exist.",__FILE__,__FUNCTION__, g_asConfigName.c_str());
        return;        
    }    
    char caTempLine[128];
    UFC::AnsiString asLine;
    
    while( fin.getline(caTempLine,sizeof(caTempLine)) )
    {        
        if(strlen(caTempLine) == 0)
            continue;        
        asLine = caTempLine;
        asLine.TrimLeft();        
        if(asLine.FirstChar() == '#' || asLine.Length() == 0)
            continue;         
        UFC::AnsiString asMBusServerPort,asCAServerPort,asTemp; 
        UFC::NameValueMessage NV_ConfigLine( "|" );
        NV_ConfigLine.FromString( asLine );    
        NV_ConfigLine.Get("AppName", FAppName);
        NV_ConfigLine.Get("MBusServerIP", g_asMBusServerIP);
        NV_ConfigLine.Get("MBusServerPort", asMBusServerPort);   
        NV_ConfigLine.Get("CAServerIP", g_asCAServerIP); 
        NV_ConfigLine.Get("CAServerPort", asCAServerPort); 
        
        //ronnie
        if( NV_ConfigLine.IsExists("CAByHttp") )  
        {
            NV_ConfigLine.Get("CAByHttp", asTemp);
            if(asTemp.Length() >0 && asTemp.UpperCase().AnsiCompare("Y") ==0 )
                g_bCAByHttp = TRUE;
            else
                g_bCAByHttp = FALSE;
        }            
        if( NV_ConfigLine.IsExists("CAServerURL") )        
            NV_ConfigLine.Get("CAServerURL", g_asCAServerURL); 
        if( NV_ConfigLine.IsExists("RecordCALogURL") )        
            NV_ConfigLine.Get("RecordCALogURL", g_asRecordCALogURL);         

        if(asMBusServerPort.c_str() != NULL)
            FMBusServerPort = asMBusServerPort.ToInt();
        if(asCAServerPort.c_str() != NULL)
            g_iCAServerPort = asCAServerPort.ToInt();
    }   
    fin.close();
     * */
}
//------------------------------------------------------------------------------
///> Create Log Object
void CreateLogObject()
{ 
    if( UFC::FileExists(g_asLogPath) == false)           
        mkdir(g_asLogPath,S_IRWXU);       
    UFC::AnsiString TradeDate;
    UFC::GetTradeYYYYMMDD( TradeDate );
    UFC::BufferedLog::SetLogObject( new UFC::BufferedLog( g_asLogPath + g_asLogPrefixName + "." + TradeDate + ".log" ,10240,true,true) );  
    UFC::BufferedLog::SetDebugMode( g_iDEBUG_LEVEL );
    UFC::BufferedLog::SetPrintToStdout( TRUE ); 
    
    ///> create empty CAData of CAService
    UFC::FileStream64 CAFile(g_asCAServiceCAData.c_str(),"a");        
}
//------------------------------------------------------------------------------
void GetB2BUK_UntilSuccess(UFC::AnsiString *pasErrMsg)
{
    while(TRUE)
    {
        *pasErrMsg = "";
        if ( GetB2BUK(pasErrMsg) == TRUE )
            break;
        else
            UFC::BufferedLog::Printf( " [%s][%s] %s,msg=%s ",  __FILE__,__FUNCTION__,MSG_GET_B2B_UK_FAIL,pasErrMsg->c_str() ); 
        UFC::SleepMS(30*1000);
    }
}
//------------------------------------------------------------------------------
///> set signal handlers
void AtStart( void )
{
    atexit( AtExit );
    signal( SIGINT,  AtSignal );
    signal( SIGQUIT, AtSignal );
    signal( SIGTERM, AtSignal );
    signal( SIGHUP,  AtSignal );
    signal( SIGKILL, AtSignal );
    signal( SIGQUIT, AtSignal );
    signal( SIGABRT, AtSignal );
    signal( SIGFPE, AtSignal );
    signal( SIGILL, AtSignal );
//    signal( SIGSEGV, AtSignal );  // capture core dump event
}
//------------------------------------------------------------------------------
///> Print startup screen.
void PrintStartUp( void )
{
    UFC::AnsiString asFlag;
#ifdef CLUSTER  
    asFlag.AppendPrintf("%s, ","CLUSTER");
#endif 
#ifdef DOWNLOAD_REMOTE_DATA  
    asFlag.AppendPrintf("%s, ","DOWNLOAD_REMOTE_DATA");
#endif 
    
    // Print startup screen
    UFC::BufferedLog::Printf( " ____________________________________________" );
    UFC::BufferedLog::Printf( " " );
    UFC::BufferedLog::Printf( "    Yuanta %s", g_asAppName.c_str());
    UFC::BufferedLog::Printf( "    Startup on %s at %s ", UFC::Hostname, UFC::GetDateString().c_str() );
    UFC::BufferedLog::Printf( " ");
    UFC::BufferedLog::Printf( "    Ver : 1.0.1 Build Date:%s ",  __DATE__ );
    UFC::BufferedLog::Printf( "    [%d bits version]          ",  sizeof(void*)*8 );
    UFC::BufferedLog::Printf( "    FirstRun           : %d         ", g_bFirstRun  );    
    UFC::BufferedLog::Printf( "    cfg                : %s         ",  g_asConfigName.c_str() );
    UFC::BufferedLog::Printf( "    LogDate            : %s         ",  g_asLogDate.c_str() ); 
    UFC::BufferedLog::Printf( "    CAServiceCAData    : %s         ",  g_asCAServiceCAData.c_str() );   
    UFC::BufferedLog::Printf( "    SpeedyCAData       : %s         ",  g_asSpeedyCAData.c_str() );
    UFC::BufferedLog::Printf( "    CARecover log      : %s         ",  g_asCAServiceRecoverLog.c_str() );
    UFC::BufferedLog::Printf( "    CAFail log         : %s         ",  g_asCAServiceFailLog.c_str() );
    UFC::BufferedLog::Printf( "    MBusServer         : %s:%d      ",g_asMBusServerIP.c_str(),FMBusServerPort   );
    UFC::BufferedLog::Printf( "    B2B Login URL      : %s         ",g_asB2B_URL.c_str()   );    
    UFC::BufferedLog::Printf( "    B2B UK             : %s         ",g_asB2B_UK.c_str()   );
    
    if(g_bCAByHttp)
    {
        asFlag.AppendPrintf("%s, ","CAByHttp");
        UFC::BufferedLog::Printf( "    CAServerURL        : %s            ",g_asCAServerURL.c_str()   );
    }
    else
        UFC::BufferedLog::Printf( "    CAServer           : %s:%d         ",g_asCAServerIP.c_str(),g_iCAServerPort   );
    if(g_bIsRemoteHost)
        UFC::BufferedLog::Printf( "    RemoteHostID/PW    : %s/%s         ",g_asRemoteHostID.c_str(),g_asRemoteHostPW.c_str()   );
    
    UFC::BufferedLog::Printf( "    Record CA URL      : %s         ",g_asRecordCALogURL.c_str()   );
    UFC::BufferedLog::Printf( "    flag               : %s         ",asFlag.c_str()   );  
    
    UFC::BufferedLog::Printf( " ");
    UFC::BufferedLog::Printf( " ____________________________________________" );
}
//------------------------------------------------------------------------------
void Initialize( void )
{     
    if (  FAPPListener == NULL)
    {
        FAPPListener = new CheckSystemListener( g_asAppName, g_asMBusServerIP, FMBusServerPort );
        UFC::BufferedLog::DebugPrintf( UFC::dlInformation, " [%s] Register MBus <appname:%s>", __func__ , g_asAppName.c_str() );
        FAPPListener->StartService();  
    }
    if ( FMBusAuthorityThread == NULL)
        FMBusAuthorityThread = new MBusAuthorityListener(FAPPListener);
    
    if( FCARecordThread == NULL)
    {
        FCARecordThread = new CARecord(FMBusAuthorityThread); 
        //FCARecordThread->Run(); 
    }
    
    ///> get localhost ip
    struct ifaddrs * ifAddrStruct=NULL;
    void * tmpAddrPtr=NULL;
    getifaddrs(&ifAddrStruct);    
    while (ifAddrStruct!=NULL) 
    {
        if (ifAddrStruct->ifa_addr->sa_family==AF_INET) 
        {
            tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);                
            g_iListLocalIPList.Add( inet_addr(addressBuffer) );
        }
        ifAddrStruct=ifAddrStruct->ifa_next;
    }      
    ///> If target MBusServerIP is not LocalIP, create another MBus connection for LocalIP to monitor in SpeedyCenter           
    if(g_asMBusServerIP != "127.0.0.1" && g_iListLocalIPList.IndexOf( inet_addr( g_asMBusServerIP.c_str() ) ) < 0 )
    {  
        UFC::BufferedLog::Printf(" [%s][%s] connect local MBus  127.0.0.1", __FILE__,__func__ );
        CheckSystemListener *FCenterListener = new CheckSystemListener( g_asAppName, "127.0.0.1", 12345 );
        FCenterListener->StartService();
    }
    
#ifdef CLUSTER         
        ///> Add g_asMBusServerIP to WhiteList  
        if(g_asMBusServerIP == "127.0.0.1")
        { 
            for(int i=0;i<g_iListLocalIPList.ItemCount();i++)
                g_iListWhiteList.Add( g_iListLocalIPList[i] );
        }else{
            g_iMBusServerIP = inet_addr(g_asMBusServerIP.c_str());
            g_iListWhiteList.Add(g_iMBusServerIP);
        } 
        PrintWhiteList(); 
#endif 
}
//------------------------------------------------------------------------------
void AddListenerFromConfig()
{
    if(FAPPListener == NULL || FMBusAuthorityThread == NULL)
        return; 
    BOOL bHaveAddListener = FALSE;    
    fstream fin;    
    try
    {
        fin.open(g_asConfigName.c_str(),ios::in);    
        if(!fin)        
            throw bHaveAddListener;
            
        char caTempLine[128];
        UFC::AnsiString asLine;    

        while( fin.getline(caTempLine,sizeof(caTempLine)) )
        {
            if(strlen(caTempLine) == 0)
                continue;
            UFC::AnsiString asSubject,asKey,asMonitoringProcess,asCoverIP,asHost,asCoverAppName;;      
            asLine = caTempLine;
            if(asLine.FirstChar() == '#' || asLine.Length() == 0)
                continue;
            UFC::NameValueMessage NV_ConfigLine( "|" );
            NV_ConfigLine.FromString( asLine );    
            NV_ConfigLine.Get("MonitoringProcess", asMonitoringProcess);
            NV_ConfigLine.Get("Subject", asSubject);
            NV_ConfigLine.Get("Key", asKey);         
            
#ifdef CLUSTER        
            ///> add CoverIP to ArrayCoverIP
            if( NV_ConfigLine.IsExists("CoverIP") && NV_ConfigLine.IsExists("Host") )
            {
                if(g_iArrayCoverIPCounts < COVER_IP_LIMIT)
                { 
                    NV_ConfigLine.Get("CoverIP", asCoverIP);
                    NV_ConfigLine.Get("Host", asHost);
                    if( NV_ConfigLine.IsExists("CoverAppName") )
                        NV_ConfigLine.Get("CoverAppName", asCoverAppName); 
                    else
                        asCoverAppName = g_asAppName;
                    
                    UFC::Int32 iIP = inet_addr(asCoverIP.c_str());
                    g_asArrayCoverIPList[g_iArrayCoverIPCounts][0] = asCoverIP;
                    g_asArrayCoverIPList[g_iArrayCoverIPCounts][1] = asHost;                 
                    g_asArrayCoverIPList[g_iArrayCoverIPCounts][2].Printf("%d",iIP ); 
                    g_asArrayCoverIPList[g_iArrayCoverIPCounts][3] = "0"; 
                    g_asArrayCoverIPList[g_iArrayCoverIPCounts][4] = asCoverAppName;

                    ///> add monitor process
                    FAPPListener->AddMonitoringProcess( asHost, asCoverAppName); 
                    UFC::BufferedLog::Printf(" [%s][%s] Add MonitoringProcess Host=%s,AppName=%s", __FILE__,__FUNCTION__,asHost.c_str() , asCoverAppName.c_str() );

                    ///> add CoverIP to Whitelist
                    g_iListWhiteList.Add( iIP );
                    UFC::BufferedLog::Printf(" [%s][%s] Add IP=%d,%s into whitelist", __FILE__,__func__,iIP,ConvertIntIPtoString(iIP) );                 
                    g_iArrayCoverIPCounts++;

                }else{
                    UFC::BufferedLog::Printf(" [%s][%s] counts of coverIPList over limit", __FILE__,__FUNCTION__);
                }            
            }
#endif 
            ///> 設定從MBUS監控的Hostname 
            if(asMonitoringProcess.c_str() != NULL)
            {
                if(NV_ConfigLine.IsExists("MonitoringHost"))
                    NV_ConfigLine.Get("MonitoringHost", asHost);
                else
                    asHost = UFC::Hostname;
                
                FAPPListener->AddMonitoringProcess( asHost, asMonitoringProcess);  
                UFC::BufferedLog::Printf(" [%s][%s] Add MonitoringProcess Host=%s,AppName=%s", __FILE__,__FUNCTION__,asHost.c_str() , asMonitoringProcess.c_str()); 
            }
            ///> 設定從MBUS接收的Subject及Key，
            if(asSubject.c_str() != NULL && asKey.c_str() != NULL)
            {       
                bHaveAddListener = TRUE;
                FAPPListener->AddListener(asSubject, asKey, FMBusAuthorityThread);
                UFC::BufferedLog::Printf(" [%s][%s] Add subject(%s) & key(%s) ", __FILE__,__FUNCTION__,asSubject.c_str() , asKey.c_str());
                g_asClientLogonSubject = asSubject;
                g_asClientLogonKey = asKey;
            }        
        }    
        fin.close();
        PrintWhiteList();
        if( bHaveAddListener == FALSE )
            throw bHaveAddListener;
    }
    catch(BOOL err)
    {
        UFC::BufferedLog::Printf(" [%s][%s] Use default Subject/Key into Listener.(%s/%s)", __FILE__,__FUNCTION__,g_asClientLogonSubject.c_str(), g_asClientLogonKey.c_str());
        FAPPListener->AddListener(g_asClientLogonSubject, g_asClientLogonKey, FMBusAuthorityThread);
        return; 
    }
}
//------------------------------------------------------------------------------
/// main program
int main(int argc, char** argv)
{
    ParseArg(argc,argv);
    GetLogDate();
    GetLocalIP();     
    ParseConfig();      ///> get AppName/MBusServerIP/Port from config. 
    
    CreateLogObject(); 
    GetB2BUK_UntilSuccess(&g_asB2B_UK);    
    AtStart();          ///> set signal handlers. 
    PrintStartUp();
    Initialize();       ///> new listeners.
    
    try
    {        
        g_bRunning = TRUE; 
        UFC::BufferedLog::Printf(" [%s][%s] Start MBus Authority listener", __FILE__,__func__ );
        
        FMBusAuthorityThread->Run(); 
        AddListenerFromConfig(); 
        FCARecordThread->Run(); 
        
        while ( g_bRunning )
        {  
            UFC::SleepMS( 5*1000 );
        }
        UFC::BufferedLog::Printf(" [%s][%s] Daemon job interrupted!", __FILE__, __func__ );
    }
    catch( UFC::Exception &e )
    {
        UFC::BufferedLog::Printf(" *ERR* [%s] UFC exception occurred <Reason:%s>\n", __func__, e.what() );
    }
    catch(...)
    {
        UFC::BufferedLog::Printf(" *ERR* [%s] Unknown exception occurred\n", __func__ );
    }
    return 0;
}



