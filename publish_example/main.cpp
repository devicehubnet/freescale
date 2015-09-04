#include "MQTTClient.h"
#include "MQTTEthernet.h"
#include "rtos.h"
#include <stdlib.h>

#define __APP_SW_REVISION__ "1"

#define MQTT_PORT 1883

#define MQTT_MAX_PACKET_SIZE 250

#include "K64F.h"

bool quickstartMode = true;

bool connected = false;
bool mqttConnecting = false;
bool netConnected = false;
bool netConnecting = false;
bool ethernetInitialising = true;
int connack_rc = 0; // MQTT connack return code
int retryAttempt = 0;

int blink_interval = 0;

char* ip_addr = "";
char* gateway_addr = "";
char* host_addr = "";
int connectTimeout = 1000;

#define API_KEY         "COPY YOUR APY KEY HERE"
#define PROJECT_ID      "COPY YOUR PROJECT ID HERE"
#define AN_SENSOR_NAME  "COPY THE NAME OF A SENSOR FROM YOUR PROJECT"
#define AN_ACTUATOR_NAME "COPY THE NAME OF A ACTUATOR FROM YOUR PROJECT"
#define DEVICE_UUID     "COPY YOUR DEVICE ID HERE"
#define ID              "aab"                         

char* pubTopic = "/a/"API_KEY"/p/"PROJECT_ID"/d/"DEVICE_UUID"/sensor/"AN_SENSOR_NAME"/data";
char id[30] = ID;                 

int connect(MQTT::Client<MQTTEthernet, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTEthernet* ipstack)
{   
    const char* host = "mqtt.devicehub.net";
    
    char hostname[strlen(host) + 1];
    sprintf(hostname, "%s", host);
    EthernetInterface& eth = ipstack->getEth();
    ip_addr = eth.getIPAddress();
    gateway_addr = eth.getGateway();
    
    char clientId[strlen(id)];
    sprintf(clientId, "%s",id);
    
    // Network debug statements 
    LOG("=====================================\n");
    LOG("Connecting Ethernet.\n");
    LOG("IP ADDRESS: %s\n", eth.getIPAddress());
    LOG("MAC ADDRESS: %s\n", eth.getMACAddress());
    LOG("Gateway: %s\n", eth.getGateway());
    LOG("Network Mask: %s\n", eth.getNetworkMask());
    LOG("Server Hostname: %s\n", hostname);
    LOG("Client ID: %s\n", clientId);
    LOG("=====================================\n");
    
    netConnecting = true;
    int rc = ipstack->connect(hostname, MQTT_PORT, connectTimeout);
    if (rc != 0)
    {
        WARN("IP Stack connect returned: %d\n", rc);    
        return rc;
    }
    netConnected = true;
    netConnecting = false;

    // MQTT Connect
    mqttConnecting = true;
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = clientId;

    
    if ((rc = client->connect(data)) == 0) 
    {       
        connected = true;
    }
    else
        WARN("MQTT connect returned %d\n", rc);
    if (rc >= 0)
        connack_rc = rc;
    mqttConnecting = false;
    return rc;
}


int getConnTimeout(int attemptNumber)
{  // First 10 attempts try within 3 seconds, next 10 attempts retry after every 1 minute
   // after 20 attempts, retry every 10 minutes
    return (attemptNumber < 10) ? 3 : (attemptNumber < 20) ? 60 : 600;
}


void attemptConnect(MQTT::Client<MQTTEthernet, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTEthernet* ipstack)
{
    connected = false;
   
    while (!linkStatus()) 
    {
        wait(1.0f);
        WARN("Ethernet link not present. Check cable connection\n");
    }
        
    while (connect(client, ipstack) != MQTT_CONNECTION_ACCEPTED) 
    {    
          
        int timeout = getConnTimeout(++retryAttempt);
        WARN("Retry attempt number %d waiting %d\n", retryAttempt, timeout);
        
        // this works - reset the system when the retry count gets to a threshold
        if (retryAttempt == 5)
            NVIC_SystemReset();
        else
            wait(timeout);
    }
}


int publish(MQTT::Client<MQTTEthernet, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTEthernet* ipstack)
{
    MQTT::Message message;
    
    int analog_sensor = rand() % 100;        
    char buf[250];
    sprintf(buf,"{\"value\": %d }", analog_sensor);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf);
    
    LOG("Publishing %s\n", buf);
    return client->publish(pubTopic, message);
}


char* getMac(EthernetInterface& eth, char* buf, int buflen)    // Obtain MAC address
{   
    strncpy(buf, eth.getMACAddress(), buflen);

    char* pos;                                                 // Remove colons from mac address
    while ((pos = strchr(buf, ':')) != NULL)
        memmove(pos, pos + 1, strlen(pos) + 1);
    return buf;
}

int main()
{    
    
    led2 = LED2_OFF; // K64F: turn off the main board LED 
        
    LOG("***** DeviceHub.net example *****\n");
    MQTTEthernet ipstack;
    ethernetInitialising = false;
    MQTT::Client<MQTTEthernet, Countdown, MQTT_MAX_PACKET_SIZE> client(ipstack);
    LOG("Ethernet Initialized\n"); 
    
    if (quickstartMode)
        getMac(ipstack.getEth(), id, sizeof(id));
        
    attemptConnect(&client, &ipstack);

    
    blink_interval = 0;
    int count = 0;
    while (true)
    {
        if (++count == 100)
        {               // Publish a message every second
            if (publish(&client, &ipstack) != 0) 
                attemptConnect(&client, &ipstack);   // if we have lost the connection
            count = 0;
        }
        
        if (blink_interval == 0)
            led2 = LED2_OFF;
        else if (count % blink_interval == 0)
            led2 = !led2;
        client.yield(10);  // allow the MQTT client to receive messages
    }
}
