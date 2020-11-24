#ifndef MY_BROKER_H
#define MY_BROKER_H

#include <uMQTTBroker.h>
#include "..\..\logger.h"

class myMQTTBroker: public uMQTTBroker
{
public:
    typedef Logger<myMQTTBroker> Log;

    myMQTTBroker(uint16_t portno=1883, uint16_t max_subscriptions=10000, uint16_t max_retained_topics=30)
        : uMQTTBroker(portno,max_subscriptions,max_retained_topics), callback(nullptr)
    { /* ... */ }

    virtual bool onConnect(IPAddress addr, uint16_t client_count)
    {
      Log::info(addr.toString()+" connected");
      return true;
    }
    
    virtual bool onAuth(String username, String password)
    {
      Log::info("Username/Password: "+username+"/"+password);
      return true;
    }
    
    virtual void onData(String topic, const char *data, uint32_t length)
    {
      char data_str[length+1];
      os_memcpy(data_str, data, length);
      data_str[length] = '\0';
      
      if(callback)
        callback(topic.c_str(),data,length);
    }

    void set_callback(void (*foo)(const char*,const char*,unsigned int))
    {
        callback = foo;
    }

private:
    void (*callback)(const char*,const char*,uint32_t);
};

#endif
