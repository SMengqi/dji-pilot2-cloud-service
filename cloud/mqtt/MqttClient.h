
#ifndef INCLUDE_MQTTCLIENT_H_
#define INCLUDE_MQTTCLIENT_H_

#include <atomic>
#include <chrono>
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

private:
	mqtt::async_client& m_client;
	mqtt::connect_options& m_options;

	f_onRead_CB m_onRead_CB;
};

class MqttClient
{
public:
	MqttClient(std::string strServer, std::string clientId);
	~MqttClient();

public:
	void setMqttOptions();
	void setMqttUserAndPasswd(std::string user, std::string passwd);
	// 同步连接, 成功返回true; 失败返回false, 调用方负责重试
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

private:
	mqtt::async_client m_client;
	Callback m_callback;
	mqtt::connect_options m_connOpts;
	mqtt::message_ptr m_pubMsg;
	// publish时断线重连的节流时间戳
	std::chrono::steady_clock::time_point m_lastConnectTry;
};
#endif /* INCLUDE_MQTTCLIENT_H_ */
