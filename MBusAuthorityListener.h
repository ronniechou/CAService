/* 
 * File:   CheckSystemListener.cpp
 * Author: pmf
 * 
 * Created on Feb 24, 2017
 */

#ifndef MBUSAUTHORITYLISTENER_H_
#define MBUSAUTHORITYLISTENER_H_

#include "MSubscriber.h"
#include "List.h"
#include "CheckSystemListener.h"

//------------------------------------------------------------------------------
typedef UFC::PtrQueue<MTree>  TMemoryQueue;

class MBusAuthorityListener : public UFC::PThread, public MessageListener ,public UFC::SocketClientListener
{
protected:
    CheckSystemListener*    FAPPListener;
    UFC::AnsiString         FThreadName;   
    UFC::PClientSocket      *FSocketObject;    
    TMemoryQueue            FAuthorityQueue;    
    UFC::PEvent             FLogonEvent;
    
    UFC::AnsiString         FCA_Date;   
    UFC::AnsiString         FCA_Market;   
    UFC::AnsiString         FCA_NID;   
    UFC::AnsiString         FCA_ID;   
    UFC::AnsiString         FCA_BrokerID;   
    UFC::AnsiString         FCA_Account;   
    UFC::AnsiString         FCA_IP;  
    UFC::AnsiString         FCA_Verified_NO;
    UFC::AnsiString         FCA_Subject;
    UFC::AnsiString         FCA_Order;
    UFC::AnsiString         FCA_Signature;
    UFC::AnsiString         FClientMbusKey;

protected: /// implement MessageListener interface
    virtual void OnMigoMessage( const UFC::AnsiString& Subject, const UFC::AnsiString& Key, MTree* Data );

protected: /// Implement interface PThread
    void Execute( void );    

protected:  ///Implement interface SocketClientListener
    void OnConnect( UFC::PClientSocket *Socket);
    void OnDisconnect( UFC::PClientSocket *Socket);
    BOOL OnDataArrived( UFC::PClientSocket *Socket);
    void OnIdle( UFC::PClientSocket *Socket);    

public:
    void Run( void );    
    //BOOL ProcessCARequest( UFC::AnsiString asCAData,UFC::AnsiString asOrder,UFC::AnsiString asPeerIP,UFC::AnsiString asID,UFC::AnsiString asCASubject,UFC::AnsiString *pasErrMsg );  
    //BOOL ProcessCARequestbyCURL( UFC::AnsiString asCAData,UFC::AnsiString asOrder,UFC::AnsiString asPeerIP,UFC::AnsiString asID,UFC::AnsiString asCASubject,UFC::AnsiString *pasErrMsg );
    //BOOL ProcessCARequestbySocket( UFC::AnsiString asCAData,UFC::AnsiString asOrder,UFC::AnsiString asPeerIP,UFC::AnsiString asID,UFC::AnsiString asCASubject,UFC::AnsiString *pasErrMsg );
    BOOL ProcessCARequest(UFC::AnsiString *pasErrMsg ); 
    BOOL ProcessCARequestbyHttp(UFC::AnsiString *pasErrMsg ); 
    BOOL ProcessCARequestbySocket(UFC::AnsiString *pasErrMsg ); 
    
    void ProcessCAResponse(UFC::UInt8 *rcvb);   
    void ProcessCAResponse(UFC::AnsiString *pasMsg);
    void ReturnResponse(BOOL bAccept,UFC::AnsiString Msg);
    
    void InQueue(MTree* mtData,UFC::AnsiString asMBusKey);
    void InitParameter();
    void SaveCAResultToFile(BOOL bAccept,UFC::AnsiString Msg);
public:    
    MBusAuthorityListener(CheckSystemListener* pCheckSystemListener);
    virtual ~MBusAuthorityListener();
};

#endif /* MBUSAUTHORITYLISTENER_H_ */
