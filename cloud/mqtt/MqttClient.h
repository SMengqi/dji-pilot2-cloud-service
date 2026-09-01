
#ifndef INCLUDE_MQTTCLIENT_H_
#define INCLUDE_MQTTCLIENT_H_

#include <atomic>
#include <string>
#include <functional>
#include "mqtt/async_client.h"

using f_onRead_CB = std::function<void(std::string, std::string)>;

class Callback: public mqtt::callback,
				public mqtt::iaction_listener
{
public:
	Callback(mqtt::async_client& client, mqtt::connect_options& options);
public:	// override function
	void connection_lost(const std::string& cause) override;
	void delivery_complete(mqtt::delivery_token_ptr tok) override;
	void on_success(const mqtt::token& tok) override;
	void on_failure(const mqtt::token& tok) override;
	void connected(const std::string& cause) override;
	void message_arrived(mqtt::const_message_ptr msg) override;

public:
	void setOnReadCallback(f_onRead_CB onRead_CB);

private:	// normal function
	void reconnect();

private:
	mqtt::async_client& m_client;
	mqtt::connect_options& m_options;
	int m_nRetry;

	f_onRead_CB m_onRead_CB;
};

//class DeliveryActionListener: public ActionListener
//{
//public:
//	DeliveryActionListener();
//	bool is_done() const;
//private:
//	std::atomic<bool> m_done;
//	void on_failure(const mqtt::token& tok) override;
//	void on_success(const mqtt::token& tok) override;
//};

// subscriber

class MqttClient
{
public:
	MqttClient(std::string strServer, std::string clientId);
	~MqttClient();

public:
	void setMqttOptions();
	void setMqttUserAndPasswd(std::string user, std::string passwd);
	bool connect();

	void subscribe(const std::string& strTopic, const int& ulQos = 0);
	void publish(const std::string& strTopic, const std::string& strPayload, const int& ulQos = 0);

	bool isConnected();
	void disconnect();

public:
	void setOnReadCallback(f_onRead_CB onRead_CB);

private:
	std::string m_strServer;
	std::string m_clientId;
//	bool m_subFlag;	// false: publish; true: subscribe

private:
	//声明一个MQTTClient
	mqtt::async_client m_client;
	Callback m_callback;
	//初始化MQTT Client选项
	mqtt::connect_options m_connOpts;
	//#define MQTTClient_message_initializer { {'M', 'Q', 'T', 'M'}, 0, 0, NULL, 0, 0, 0, 0 }
	mqtt::message_ptr m_pubMsg;
};
#endif /* INCLUDE_MQTTCLIENT_H_ */
