#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //for sleep(()
#include "tinyxml2.h"
#include "FileStream.h"
#include "CARecord.h"
#include <curl/curl.h>

//------------------------------------------------------------------------------
CARecord::CARecord()
: PThread( NULL )
, FIsStop(TRUE)
, FCAHandel(NULL)
{
    
}
//------------------------------------------------------------------------------
CARecord::CARecord(MBusAuthorityListener* pCAHandel)
: PThread( NULL )
, FIsStop(TRUE)
, FCAHandel(pCAHandel)
{
}
//------------------------------------------------------------------------------
CARecord::~CARecord()
{   
    UFC::BufferedLog::Printf(" [%s][%s]  terminate thread...", __FILE__,__FUNCTION__);
    Terminate();    
    UFC::BufferedLog::Printf(" [%s][%s]  terminate thread...done", __FILE__,__FUNCTION__);
}
//------------------------------------------------------------------------------
void CARecord::Run( void )
{    
    ///> FirstRun mode , rename CARecover.log
    if( g_bFirstRun )
    {
        UFC::AnsiString asNewName;
        //asNewName.Printf("%sCARecover.%s.%d.log",g_asLogPath.c_str(),g_asLogDate.c_str(),UFC::GetSecondsToday() );        
        asNewName.Printf("%s%sRecover.%s.%d.log",g_asLogPath.c_str(),g_asLogPrefixName.c_str(),g_asLogDate.c_str(),UFC::GetSecondsToday() );
        rename(g_asCAServiceRecoverLog.c_str(),asNewName.c_str());
    }
    else
    {
        ///> read remote host RecoverFile
#ifdef DOWNLOAD_REMOTE_DATA        
        if(g_bIsRemoteHost == TRUE)
            DownloadRemoteRecoverFile();
#endif 
        ///> read local host RecoverFile
        ReadRecoverFile( g_asCAServiceRecoverLog );
    }
    
    ///> thread start
    Start();
    FIsStop = FALSE;
}
//------------------------------------------------------------------------------
void CARecord::ReadRecoverFile(UFC::AnsiString asFileName)
{
    UFC::PStringList StrList;
    UFC::AnsiString asBuffer,asKey,asRecordResult,asPostData,asVerifiedNo;
    fstream file;
    char buffer[1024];
    int iCount=0;
    
    ///>get CARecover.log into FCAOkList & g_CAOkMap
    // 0000131186,00020180716085811761878,Y,<root><IDNO>%s</IDNO><ISCHKCA>N</ISCHKCA><ISCHKRA>N</ISCHKRA><ISCALOG>Y</ISCALOG><CONTENT>%s</CONTENT><SIGN>%s</SIGN><CERTSERIAL></CERTSERIAL><COMTYPE>%s</COMTYPE><USERID>%s</USERID><COMPANY>%s</COMPANY></root>
    // NID,VerifiedNo,RecordResult,PostData
    file.open(asFileName.c_str(), ios::in);

    if(file)
    {
        while( !file.eof() )
        {  
            memset(buffer,'\0',sizeof(buffer));
            file.getline(buffer,sizeof(buffer));
            if(strlen(buffer) ==0 )
                continue;            

            StrList.SetStrings(buffer,",");
            asKey = StrList[0];
            asVerifiedNo = StrList[1];
            asRecordResult = StrList[2];

            if(StrList.ItemCount() > 3)
                asPostData = StrList[3];
            asBuffer.Printf("%s,%s",asVerifiedNo.c_str(),asPostData.c_str() );

            if( !g_CAOkMap.IsExists(asKey) )
                g_CAOkMap.Add( asKey, new UFC::AnsiString(asBuffer.c_str()) );

            if( asRecordResult.UpperCase().AnsiCompare("Y") == 0 && FRecordOkList.IndexOf(asKey) <0 )
                FRecordOkList.Add(asKey);
            iCount++;
        }
        file.close();
        UFC::BufferedLog::Printf(" [%s][%s] Recover %d datas from File (%s)",__FILE__,__FUNCTION__,iCount, asFileName.c_str()  );
    }else{
        UFC::BufferedLog::Printf(" [%s][%s] RecoverFile (%s) not exist",__FILE__,__FUNCTION__, asFileName.c_str()  );
    }
}
//------------------------------------------------------------------------------
size_t RecordCALogCallback(void * ptr, size_t size, size_t nmemb, void *data)
{
    UFC::AnsiString* pMsg = (UFC::AnsiString*)data;
    size_t totalSize = size*nmemb;
    pMsg->AppendPrintf("%s",(char*)ptr);
    return totalSize; 
}
//------------------------------------------------------------------------------
void CARecord::CheckRecordResult()
{    
    for(int i=0;i<g_CAOkMap.ItemCount();i++)
    {
        UFC::AnsiString *pasValue,asKey,asErrMsg,asBuffer,asPostData,asVerifiedNo;
        UFC::PStringList StrList;
        
        g_CAOkMap.GetItem(i,asKey,pasValue);
        StrList.SetStrings(pasValue->c_str(),",");
        
        
        //UFC::BufferedLog::Printf(" [%s][%s] StrList.ItemCount=%d,pasValue=%s",__FILE__,__FUNCTION__,  StrList.ItemCount(),pasValue->c_str()  );
        //UFC::BufferedLog::Printf(" [%s][%s] key=%s,asVerifiedNo=%s,postData=%s",__FILE__,__FUNCTION__,  asKey.c_str(),StrList[0].c_str(),StrList[1].c_str()  );
        
        if(FRecordOkList.IndexOf(asKey) <0 )
        {
            if( StrList.ItemCount() < 2)
            {
                UFC::BufferedLog::Printf(" [%s][%s] %s,loss post data,key=%s,Value=%s",__FILE__,__FUNCTION__, MSG_RECORD_CADATA_FAIL,asKey.c_str(),pasValue->c_str()  );
                continue;
            } 
            asVerifiedNo = StrList[0];
            asPostData = StrList[1];
            
            UFC::BufferedLog::Printf(" [%s][%s] Key=%s,VerifiedNo=%s,go to record CA data",__FILE__,__FUNCTION__,asKey.c_str(),asVerifiedNo.c_str() );
            BOOL bResult = RecordCAData(asPostData,&asErrMsg);
            
            ///> save result to CARecover.log
            //asBuffer.Printf("%sCARecover.%s.log",g_asLogPath.c_str(),g_asLogDate.c_str() );
            //UFC::FileStreamEx RecoverFile( asBuffer.c_str(), "a"); 
            UFC::FileStreamEx RecoverFile( g_asCAServiceRecoverLog.c_str(), "a");
            
            if(bResult)
            {
                FRecordOkList.Add(asKey);
                asBuffer.Printf("%s,%s,%s\n",asKey.c_str(),asVerifiedNo.c_str(),"Y" );
                UFC::BufferedLog::Printf(" [%s][%s] %s,Key=%s,msg=%s",__FILE__,__FUNCTION__,MSG_RECORD_CADATA_SUCCESS, asKey.c_str(), asErrMsg.c_str() );
            }
            else
            {
                asBuffer.Printf("%s,%s,%s,%s\n",asKey.c_str(),asVerifiedNo.c_str(),"N",asPostData.c_str() );
                UFC::BufferedLog::Printf(" [%s][%s] %s,Key=%s,msg=%s",__FILE__,__FUNCTION__, MSG_RECORD_CADATA_FAIL,asKey.c_str(), asErrMsg.c_str() );
            }  
            RecoverFile.Write( asBuffer.c_str(),asBuffer.Length() );
            RecoverFile.Flush();     
        }        
    }
}
//------------------------------------------------------------------------------
BOOL CARecord::RecordCAData(UFC::AnsiString asPostData,UFC::AnsiString *pasErrMsg)
{    
    UFC::AnsiString asURL,asBuffer;
    asURL.Printf("%s?UK=%s&app=SpeedyGW&vip=127.0.0.1",g_asRecordCALogURL.c_str(),g_asB2B_UK.c_str() );
    
    //UFC::BufferedLog::Printf(" [%s][%s] PostData=%s",__FILE__,__FUNCTION__, asPostData.c_str() );
    
    CURL *curl = NULL;
    CURLcode res;
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
        curl_easy_setopt(curl,CURLOPT_URL,asURL.c_str() );    
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);      ///> trust server ca
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, RecordCALogCallback); ///> setup callback
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
                
        ///> parse xml 
        tinyxml2::XMLDocument doc;
        int error = doc.Parse(pasErrMsg->c_str() );   
        if(error == 0)
        {    
            tinyxml2::XMLHandle docHandle( &doc );
            tinyxml2::XMLElement* subElement = docHandle.FirstChildElement( "root" ).FirstChildElement( "CALOGRTN" ).ToElement();

            if(subElement)
            {     
                if( strcmp(subElement->GetText(),"1") == 0 )
                {
                    subElement = docHandle.FirstChildElement( "root" ).FirstChildElement( "CALOGRTNDESC" ).ToElement();
                    if(subElement)
                        pasErrMsg->Printf("%s",subElement->GetText() );                                       
                }else{
                    subElement = docHandle.FirstChildElement( "root" ).FirstChildElement( "CALOGRTNDESC" ).ToElement();
                    if(subElement)
                        pasErrMsg->Printf("%s",subElement->GetText() );   
                    return FALSE;
                }
            }else
                return FALSE;
        }else{
            UFC::BufferedLog::Printf( " [%s][%s]doc.Parse fail , msg=%s ",  __FILE__,__FUNCTION__,doc.GetErrorStr1());   
            return FALSE;
        } 
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
BOOL CARecord::RecordCAData(UFC::PStringList StrList,UFC::AnsiString *pasErrMsg,UFC::AnsiString *pasPostData)
{    
    // column : char(4) Length of Record (include 0x0A)
    // column 0: YYYY/MM/DD HH:MM:SS.uuuuuu 
    // column 1: Market 
    // column 2: NID (Speedy NID)
    // column 3: ID  (IDNO/Account)
    // column 4: BrokerID
    // column 5: Account
    // column 6: PeerIP    
    // column 7: VerifiedNo
    // column 8: Subject of User CA
    // column 9: Plain Text (Order)
    // column10: Signatured Text by CA
    // column11: 0x0A
    
    UFC::AnsiString asPostData;
    //asPostData.Printf("<root><IDNO>%s</IDNO><ISCHKCA>N</ISCHKCA><ISCHKRA>N</ISCHKRA><ISCALOG>Y</ISCALOG><CONTENT>%s</CONTENT><SIGN>%s</SIGN><CERTSERIAL></CERTSERIAL><COMTYPE>%s</COMTYPE><USERID></USERID><COMPANY>%s</COMPANY></root>",StrList[3].c_str(),StrList[9].c_str(),StrList[7].c_str(),StrList[1].c_str(),StrList[4].c_str() );
    pasPostData->Printf("<root><IDNO>%s</IDNO><ISCHKCA>N</ISCHKCA><ISCHKRA>N</ISCHKRA><ISCALOG>Y</ISCALOG><CONTENT>%s</CONTENT><SIGN>%s</SIGN><CERTSERIAL></CERTSERIAL><COMTYPE>%s</COMTYPE><USERID>%s</USERID><COMPANY>%s</COMPANY></root>"
        ,StrList[3].c_str(),StrList[9].c_str(),StrList[7].c_str(),StrList[1].c_str(),StrList[5].c_str(),StrList[4].c_str() );
    
    return RecordCAData(*pasPostData,pasErrMsg);
}
//------------------------------------------------------------------------------
void CARecord::ProcessCAData(UFC::AnsiString asCALogFile,int *iIndex)
{
    fstream File;
    char buffer[10240];
    int len = 0;
    UFC::AnsiString asLine,asBuffer,asKey,asVerifiedNo,asPostData,asValue;
    
    File.open(asCALogFile.c_str(), ios::in);
    if(!File)
    {
        UFC::BufferedLog::Printf(" [%s][%s] %s NOT exist.",__FILE__,__FUNCTION__,  asCALogFile.c_str() );
        return;        
    }  
    File.seekg(*iIndex,ios::beg);
    while(true)
    {
        memset(buffer,'\0',sizeof(buffer));
        File.read(buffer,4); 
    
        if(File.eof() )
            break;

        len = atoi(buffer) ;
        if(len <= sizeof(buffer))
            File.read(buffer,len);   
        else
            File.read(buffer,sizeof(buffer));  
        
        asLine = buffer;
        UFC::PStringList StrList;
        StrList.SetStrings(buffer,"|"); 
        
        //UFC::BufferedLog::Printf(" [%s][%s](%s) len=%d,buffer=%s",__FILE__,__FUNCTION__,asCALogFile.c_str(),len ,buffer);
        if(StrList.ItemCount() < 11 )
        {
            UFC::BufferedLog::Printf(" [%s][%s](%s) wrong format,StrList.ItemCount=%d,buffer=%s",__FILE__,__FUNCTION__,asCALogFile.c_str(),StrList.ItemCount() ,buffer);
            continue;        
        }
        asKey = StrList[2];
        asVerifiedNo = StrList[7];

        ///> check CA VerifiedNo already in CARecover.log 
        if( g_CAOkMap.IsExists(asKey) )
            continue;
        
        if(asVerifiedNo.LowerCase().AnsiPos("null") < 0 && asVerifiedNo.Length() == 23)
        {  
            UFC::BufferedLog::Printf(" [%s][%s](%s) Key=%s,VerifiedNo=%s,go to record CA data",__FILE__,__FUNCTION__,asCALogFile.c_str(),  asKey.c_str(),asVerifiedNo.c_str() );
            UFC::AnsiString asErrMsg;
            BOOL bResult = RecordCAData(StrList,&asErrMsg,&asPostData);
            
            asValue.Printf("%s,%s",asVerifiedNo.c_str(),asPostData.c_str());            
            g_CAOkMap.Add(asKey, new UFC::AnsiString( asValue.c_str() ) );
            
            ///> save result to CARecover.log
            //asBuffer.Printf("%sCARecover.%s.log",g_asLogPath.c_str(),g_asLogDate.c_str() );
            //UFC::FileStreamEx RecoverFile( asBuffer.c_str(), "a"); 
            UFC::FileStreamEx RecoverFile( g_asCAServiceRecoverLog.c_str(), "a");
            
            if(bResult)
            {
                FRecordOkList.Add(asKey);
                asBuffer.Printf("%s,%s,%s\n",asKey.c_str(),asVerifiedNo.c_str(),"Y" );
                UFC::BufferedLog::Printf(" [%s][%s] %s,Key=%s,msg=%s",__FILE__,__FUNCTION__,MSG_RECORD_CADATA_SUCCESS, asKey.c_str(), asErrMsg.c_str() );
            }
            else
            {
                asBuffer.Printf("%s,%s,%s,%s\n",asKey.c_str(),asVerifiedNo.c_str(),"N",asPostData.c_str() );
                UFC::BufferedLog::Printf(" [%s][%s] %s,Key=%s,msg=%s",__FILE__,__FUNCTION__, MSG_RECORD_CADATA_FAIL,asKey.c_str(), asErrMsg.c_str() );
            }  
            RecoverFile.Write( asBuffer.c_str(),asBuffer.Length() );
            RecoverFile.Flush();           
        }
        else
        {              
            UFC::BufferedLog::Printf(" [%s][%s](%s) Key=%s,VerifiedNo=%s,not yet check CA",__FILE__,__FUNCTION__,asCALogFile.c_str(), asKey.c_str() ,  asVerifiedNo.c_str());
                
            if(FCAHandel != NULL)
            {
                MTree mtData;  
                mtData.append(CLIENT_ORDER_DATE, StrList[0].c_str() );      //date
                mtData.append(CLIENT_ORDER_MARKET, StrList[1].c_str() );    //market
                mtData.append(CLIENT_ORDER_NID, StrList[2].c_str() );       //NID
                mtData.append(CLIENT_ID, StrList[3].c_str() );              //ClientID
                mtData.append(CLIENT_BROKERID, StrList[4].c_str() );              //BrokerID
                mtData.append(CLIENT_ACCOUNT, StrList[5].c_str() );       //Client帳號 
                mtData.append(CLIENT_IP, StrList[6].c_str() );              //ClientIP
                mtData.append(CLIENT_VERIFIED_NO, StrList[7].c_str() );              //VERIFIED_NO
                mtData.append(CLIENT_CA_SUBJECT, StrList[8].c_str() );    //CA Subject
                mtData.append(CLIENT_ORDER, StrList[9].c_str() );           //明文
                mtData.append(CLIENT_CA_SIGNATURE,StrList[10].c_str() );     //簽文 

                //mtData.append(REQUEST_FROM, REQUEST_FROM_CARECORD );     
                FCAHandel->InQueue(&mtData,UFC::Hostname);                
            }
        } 
    }
    File.clear();
    *iIndex = File.tellg();
    File.close();
}
//------------------------------------------------------------------------------
static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
    struct FtpFile *out = (struct FtpFile *)stream;
    if(out && !out->stream) 
    {
        // open file for writing 
        out->stream = fopen(out->filename, "wb");
        if(!out->stream)
            return -1; // failure, can't open file to write  
    }
    return fwrite(buffer, size, nmemb, out->stream);
}
//------------------------------------------------------------------------------
BOOL CARecord::DownloadRemoteRecoverFile()
{
    UFC::AnsiString asFileName;
    BOOL bResult;
    asFileName.Printf("%sCAServiceRecover.Remote.%s.%s.log",g_asLogPath.c_str(),g_asMBusServerIP.c_str(),g_asLogDate.c_str() );
    bResult = DownloadRemoteDATA(g_asCAServiceStandardRecoverLog,asFileName);
    if(bResult)
        ReadRecoverFile(asFileName);
}
//------------------------------------------------------------------------------
BOOL CARecord::DownloadRemoteCADATA()
{
    DownloadRemoteDATA(g_asSpeedyStandardCAData,g_asSpeedyCAData);
}
//------------------------------------------------------------------------------
BOOL CARecord::DownloadRemoteDATA(UFC::AnsiString asFromFileName,UFC::AnsiString asToFileName)
{
    BOOL bResult = FALSE;
    CURL *curl;
    CURLcode res;
    struct FtpFile ftpFile;
    UFC::AnsiString asSCP_URL;
       
    ftpFile.filename = asToFileName.c_str();
    ftpFile.stream = NULL;
    
    asSCP_URL.Printf("scp://%s:%s@%s%s",g_asRemoteHostID.c_str(),g_asRemoteHostPW.c_str(),g_asMBusServerIP.c_str(),asFromFileName.c_str() );
    //asSCP_URL = "scp://oms:oms5432@10.229.3.102/oms/Speedy/bin/SpeedyCA.20180709.DATA";
    //UFC::BufferedLog::Printf( " [%s][%s] SCP URL=%s\n", __FILE__,__func__,asSCP_URL.c_str() );
    
    try
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();
        if(curl) 
        {
            curl_easy_setopt(curl, CURLOPT_URL,asSCP_URL.c_str() );
            //curl_easy_setopt(curl, CURLOPT_URL,"scp://oms:oms5432@10.214.220.94/oms/Speedy/log/history_log/ronnie.log");
            //curl_easy_setopt(curl, CURLOPT_USERPWD, "oms5432");
            curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_SCP);
            //curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, TRUE); 
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpFile);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            if(CURLE_OK == res)
                bResult = TRUE;
            else
                UFC::BufferedLog::Printf( " [%s][%s] scp download file failed(code=%d,msg=%s,URL=%s)", __FILE__,__func__,res, curl_easy_strerror(res) ,asSCP_URL.c_str() );            

            if(ftpFile.stream)
                fclose(ftpFile.stream);
            curl_global_cleanup();
        }
    }
    catch( UFC::Exception &e )
    {
        UFC::BufferedLog::Printf( " [%s][%s] scp download file failed(msg=%s,URL=%s)", __FILE__,__func__, e.what() ,asSCP_URL.c_str() ); 
        bResult = FALSE;
    }
    catch(...)
    {
        UFC::BufferedLog::Printf( " [%s][%s] scp download file failed(msg=Unknown exception occurred,URL=%s)", __FILE__,__func__, asSCP_URL.c_str() ); 
        bResult = FALSE;
    }
    return bResult;
}
//------------------------------------------------------------------------------
void CARecord::Execute( void )
{ 
    //UFC::AnsiString asCADataName ="";
    int iIndexSpeedy = 0,iIndexCAService = 0;
    
    while ( !IsTerminated()  )
    {
        ///> process SpeedyCA.DATA
#ifdef DOWNLOAD_REMOTE_DATA        
        if(g_bIsRemoteHost == TRUE)
            DownloadRemoteCADATA();
#endif        
        ProcessCAData(g_asSpeedyCAData,&iIndexSpeedy);
        
        ///> process CAServiceCA.DATA
        ProcessCAData(g_asCAServiceCAData,&iIndexCAService);
        
        ///> check whether record log success or not, if not then re-record
        CheckRecordResult();
        sleep(30);   //use UFC::SleepMS(1000) will core dump when release.
        //UFC::SleepMS(30*1000);
    }
    FIsStop = TRUE;
    //UFC::BufferedLog::Printf( " [%s][%s] xxxxxxxxxxxxxxxxxxxxxxxxxxx ,IsTerminated=%d",  __FILE__,__FUNCTION__,IsTerminated()); 
}
//------------------------------------------------------------------------------
void CARecord::StopService()
{
    //UFC::BufferedLog::Printf( " [%s][%s].... ",  __FILE__,__FUNCTION__);   
    Terminate();
    //UFC::BufferedLog::Printf( " [%s][%s]....ok",  __FILE__,__FUNCTION__);   
}
//------------------------------------------------------------------------------
BOOL CARecord::IsStop()
{
    return FIsStop;
}
//------------------------------------------------------------------------------