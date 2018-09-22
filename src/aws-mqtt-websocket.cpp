//#define USE_PUBSUB 1 // use PubSubClient library https://github.com/knolleary/pubsubclient
#define USE_PAHO 1 // use Eclipse Paho library https://projects.eclipse.org/projects/technology.paho

#include <Arduino.h>
#include <Stream.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h> // https://github.com/esp8266/Arduino
#else
#include <WiFi.h>
#endif


#include <ArduinoLog.h>
#include <ArduinoJson.h>

#define MAX_JSON_SIZE 512

//#define DEBUG_ECHO 1

//WEBSockets
#include <WebSocketsClient.h>
#define AWS_WEBSOCKET_BUFFER_SIZE 1024

//MQTT config
#define MAX_MQTT_PAYLOAD_SIZE 1000
#define MAX_MQTT_PACKET_BUF_SIZE  1024
#define MAX_MQTT_MESSAGE_HANDLERS 2

#if defined(USE_PUBSUB)
//MQTT PUBSUBCLIENT LIB
#include <PubSubClient.h>
#elif defined(USE_PAHO)
//MQTT PAHO
#include <SPI.h>
#include <IPStack.h>
#include <Countdown.h>
#include <MQTTClient.h>
#endif

//AWS MQTT Websocket
#include "Client.h"
#include "AWSWebSocketClient.h"
#include "CircularByteBuffer.h"

extern int publishJson(JsonObject &jsonObject);
extern char* getDeviceName();
extern char* getTimestamp();
extern char* getMacAddress();

//AWS IOT configuration, CHANGE the following
//#include "myAWSus-east2.h"
#include "myAWSus-west.h"
char aws_endpoint[] = MY_AWS_ENDPOINT;
char aws_key[] = MY_AWS_IAM_KEY;
char aws_secret[] = MY_AWS_IAM_SECRET_KEY;
char aws_region[] = MY_AWS_REGION;
const char *aws_topic_prefix = "iot";
const char *aws_update = "update";
const char *aws_control = "control";
const char *aws_observe = "update";
int port = 443;

// topic
char publishTopic[128] = "noTopic";
char subscribeTopic[128] = "noTopic";

// Websocket
AWSWebSocketClient awsWSclient(AWS_WEBSOCKET_BUFFER_SIZE);

#if defined(USE_PUBSUB)
//MQTT PUBSUBCLIENT LIB
PubSubClient mqttClient(awsWSclient);
#elif defined(USE_PAHO)
IPStack ipstack(awsWSclient);
MQTT::Client<IPStack, Countdown, MAX_MQTT_PACKET_BUF_SIZE, MAX_MQTT_MESSAGE_HANDLERS> mqttClient(ipstack);
#endif

// stop WiFi
void stopWiFi()
{
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
#ifdef ESP8266
  WiFi.forceSleepBegin();
#endif // ESP8266
}

//# of connections
long connection = 0;

//generate random mqtt clientID
byte *generateClientID()
{
  byte *cID = new byte[23]();
  for (int i = 0; i < 22; i += 1)
    cID[i] = (byte)random(1, 256);
  return cID;
}

// callback function
void (*eventHandler)(JsonObject &object) = 0;

void setEventHandler(void (*handler)(JsonObject &objec))
{
  eventHandler = handler;
}

//count messages arrived
int arrivedcount = 0;

#if defined(USE_PUBSUB)
//callback to handle mqtt messages
void callback(char *topic, byte *payload, unsigned int length)
{
  //  MQTT::Message &message = md.message;
  Log.notice("Message: %d\n", ++arrivedcount);

  char *json = new char[length + 1]();
  memcpy(json, payload, length);
#elif defined(USE_PAHO)
//callback to handle mqtt messages
void messageArrived(MQTT::MessageData &md)
{
  Log.notice("Message: %d\n", ++arrivedcount);
  MQTT::Message &message = md.message;
  char *topic = md.topicName.cstring;
  char *json = new char[message.payloadlen + 1]();
  memcpy(json, message.payload, message.payloadlen);
#endif
  Log.notice("Topic: %s, Payload: %s\n", topic, json);

  DynamicJsonBuffer receivedJsonBuffer;
  JsonObject &receivedJsonObject = receivedJsonBuffer.parseObject(json);

  if (receivedJsonObject.success())
  {
    if (eventHandler != 0)
    {
      (*eventHandler)(receivedJsonObject);
    }
#ifdef DEBUG_ECHO
    publishJson(receivedJsonObject);
    Log.notice("publishMessage %s\n", json);
#endif // DEBUG_ECHO
  }
  else
  {
    Log.warning("Malformed JSON received: %s\n", json);
  }
  delete json;
}

//subscribe to a mqtt topic
void subscribe()
{
#if defined(USE_PUBSUB)
  mqttClient.setCallback(callback);
  mqttClient.subscribe(subscribeTopic);
#elif defined(USE_PAHO)
  //subscript to a topic
  int rc = mqttClient.subscribe(subscribeTopic, MQTT::QOS0, messageArrived);
  if (rc != 0)
  {
    Log.notice("rc from MQTT subscribe is %d\n", rc);
    return;
  }
#endif
  Log.notice("mqttClient.subscribe(%s) subscribed\n", subscribeTopic);
}

//connects to websocket layer and mqtt layer
bool connect()
{
#if defined(USE_PUBSUB)
  if (mqttClient.connected())
  {
#elif defined(USE_PAHO)
  if (mqttClient.isConnected())
  {
#endif
    mqttClient.disconnect();
    Log.notice("mqttClient.disconnect()\n");
  }

  Log.notice("%d - conn: %d - (%d)\n", millis(), ++connection, ESP.getFreeHeap());

  int rc;
#if defined(USE_PUBSUB)
  mqttClient.setServer(aws_endpoint, port);
#elif defined(USE_PAHO)
  rc = ipstack.connect(aws_endpoint, port);
  if (rc != 1)
  {
    Log.notice("error connection to the websocket server");
    return false;
  }
  Log.notice("websocket layer connected");
#endif
  Log.notice("mqttClient.setServer(%s,%d)\n", aws_endpoint, port);

  //creating random client id
  byte *clientID = generateClientID();
#if defined(USE_PUBSUB)
  rc = mqttClient.connect((char *)clientID);
  delete[] clientID;
  if (rc == 0)
  {
    Log.error("mqttClient.connect() failed state=%d\n", mqttClient.state());
    return false;
  }
#elif defined(USE_PAHO)
  Log.notice("MQTT connecting");
  MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
  data.MQTTVersion = 4;
  data.clientID.cstring = (char *)clientID;
  rc = mqttClient.connect(data);
  delete[] clientID;
  if (rc != 0)
  {
    Log.notice("error connection to MQTT server: %d\n", rc);
    return false;
  }
#endif
  Log.notice("connected: mqttClient.connect()=>%d awsWSclient.connected()=%d\n", rc, awsWSclient.connected());
  return true;
}


int publishMessage(const char *buf)
{
  Log.notice("+publishMessage(%s) len(buf)=%d\n", buf, strlen(buf));
#if defined(USE_PUBSUB)
  Log.notice("MQTT_MAX_PACKET_SIZE=%d, len(topic)=%d\n", MQTT_MAX_PACKET_SIZE, strlen(publishTopic));
  Log.notice("mqttClient.connected()=>%d awsWSclient.connected()=%d\n", mqttClient.connected(), awsWSclient.connected());
#elif defined(USE_PAHO)
  Log.notice("mqttClient.connected()=>%d awsWSclient.connected()=%d\n", mqttClient.isConnected(), awsWSclient.connected());
#endif
  int rc;
#if defined(USE_PUBSUB)
  bool succeeded;
  succeeded = mqttClient.publish(publishTopic, buf);
  if (!succeeded)
  {
    Log.error("failed mqttClient.publish(%s, %s)=>%d state=%d\n", publishTopic, buf, succeeded, mqttClient.state());
    Log.notice("mqttClient.connected()=>%d awsWSclient.connected()=%d\n", mqttClient.connected(), awsWSclient.connected());
  }
  rc = succeeded ? 0 : (-1);
#elif defined(USE_PAHO)
  MQTT::Message message;
  message.qos = MQTT::QOS0;
  message.retained = false;
  message.dup = false;
  message.payload = (void *)buf;
  message.payloadlen = strlen(buf);
  rc = mqttClient.publish(publishTopic, message);
#endif
  Log.notice("-publishMessage(%s)=>%d\n", buf, rc);
  return rc;
}

int publishJson(JsonObject &jsonObject)
{
  char payloadBuf[MAX_MQTT_PAYLOAD_SIZE];
  char *timestamp = getTimestamp();

  Log.notice("+publishJson() %s\n", timestamp);
  jsonObject["Type"] = getDeviceName();
  jsonObject["Timestamp"] = timestamp;
  jsonObject["MAC"] = getMacAddress();

  jsonObject.printTo(payloadBuf);
  Log.notice("jsonObject=%s\n", payloadBuf);

  int rc = publishMessage(payloadBuf);

  Log.notice("-publishJson()=>%d payload=%s\n", rc, payloadBuf);
  return rc;
}

void setupMQTT()
{
  sprintf(publishTopic, "%s/%s/%s", aws_topic_prefix, getMacAddress(), aws_update);
  Log.notice("publishTopic: %s\n", publishTopic);

//  sprintf(subscribeTopic, "%s/%s/%s", aws_topic_prefix, getMacAddress(), aws_control);
  sprintf(subscribeTopic, "%s/#", aws_topic_prefix);
  Log.notice("subscribeTopic: %s\n", subscribeTopic);

  // fill AWS parameters
  awsWSclient.setAWSRegion(aws_region);
  awsWSclient.setAWSDomain(aws_endpoint);
  awsWSclient.setAWSKeyID(aws_key);
  awsWSclient.setAWSSecretKey(aws_secret);
  awsWSclient.setUseSSL(true);

  bool success;
  if ((success = connect()) != 0)
  {
    Log.notice("connect success=%d\n", success);
    subscribe();

/*
    // send startup message
    StaticJsonBuffer<MAX_JSON_SIZE> jsonBuffer;
    JsonObject &jsonObject = jsonBuffer.createObject();
    jsonObject["Start"] = "on";
    publishJson(jsonObject);
*/
  }
  else
  {
    Log.error("connect failed success=%d\n", success);
  }

  Log.notice("-setupMQTT done\n");
}

void loopClient()
{
  if (awsWSclient.connected())
  {
#if defined(USE_PUBSUB)
    mqttClient.loop();
#elif defined(USE_PAHO)
    mqttClient.yield(50);
#endif
  }
}

int loopMQTT()
{
  Log.notice("+loopMQTT()\n");

  int result;
  if (awsWSclient.connected())
  {
#if defined(USE_PUBSUB)
    mqttClient.loop();
#elif defined(USE_PAHO)
    mqttClient.yield(50);
#endif
    result = 0;
  }
  else
  {
#if defined(USE_PUBSUB)
    Log.warning("Try reconnection. mqttClient.connect()=>%d awsWSclient.connected()=%d\n", mqttClient.connected(), awsWSclient.connected());
#elif defined(USE_PAHO)
    Log.warning("Try reconnection. mqttClient.connect()=>%d awsWSclient.connected()=%d\n", mqttClient.isConnected(), awsWSclient.connected());
#endif
    //handle reconnection
    if (connect())
    {
#if defined(USE_PUBSUB)
      Log.warning("Reconnected. mqttClient.connect()=>%d awsWSclient.connected()=%d\n", mqttClient.connected(), awsWSclient.connected());
#elif defined(USE_PAHO)
      Log.warning("Reconnected. mqttClient.connect()=>%d awsWSclient.connected()=%d\n", mqttClient.isConnected(), awsWSclient.connected());
#endif
      subscribe();
      // send reconnect message
      StaticJsonBuffer<MAX_JSON_SIZE> jsonBuffer;
      JsonObject &jsonObject = jsonBuffer.createObject();
      jsonObject["Status"] = "reconnected";
      publishJson(jsonObject);
    }
    else
    {
      Log.warning("Reconnection failed.\n");
    }
    result = -1;
  }
  Log.notice("-loopMQTT() result=%d\n", result);
  return result;
}
