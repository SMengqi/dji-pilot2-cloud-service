
#include <iostream>
#include <unistd.h>
#include "MqttClient.h"

using namespace std;

#define PUBLISH_TIMEOUT_MS      10000L
// paho的connectTimeout单位是秒, token::wait_for单位是毫秒, 等待时间必须大于连接超时
#define CONNECT_TIMEOUT_SEC     5
#define CONNECT_WAIT_MS         ((CONNECT_TIMEOUT_SEC + 1) * 1000L)
#define PUB_RECONNECT_INTERVAL  std::chrono::seconds(5)

MqttClient::MqttClient(string strServer, string clientId)
	: m_strServer(strServer)
	, m_clientId(clientId)
	, m_client(m_strServer, m_clientId)
	, m_callback(m_client, m_connOpts)
{
	m_connOpts.set_connect_timeout(CONNECT_TIMEOUT_SEC);
	m_client.set_callback(m_callback);
}

MqttClient::~MqttClient()
{
	this->disconnect();
}

void MqttClient::setMqttOptions()
{
	m_connOpts.set_keep_alive_interval(20);
	m_connOpts.set_clean_session(true);
	mqtt::message willmsg("will/topic", "Client disconnected", 1, true);
	mqtt::will_options will(willmsg);
	m_connOpts.set_will(will);
}

void MqttClient::setMqttUserAndPasswd(string user, string passwd)
{
	m_connOpts.set_user_name(user);
	m_connOpts.set_password(passwd);
}

bool MqttClient::connect()
{
	if (m_client.is_connected())
	{
		return true;
	}

	try
	{
		mqtt::token_ptr conntok = m_client.connect(m_connOpts);
		if (!conntok->wait_for(CONNECT_WAIT_MS))
		{
			cout << "connect timeout: " << m_strServer << endl;
			return false;
		}
	}
	catch (const mqtt::exception& e)
	{
		cout << "connect exception: " << e.what() << endl;
		return false;
	}

	return m_client.is_connected();
}

bool MqttClient::isConnected(void)
{
	return m_client.is_connected();
}

void MqttClient::subscribe(const string& strTopic, const int& ulQos)
{
	if (!m_client.is_connected())
	{
		cout << "MQTT subscribe topic " << strTopic << " err, client not connected" << endl;
		return;
	}

	try
	{
		m_client.subscribe(strTopic, ulQos, nullptr, m_callback);
		cout << "MQTT subscribe topic " << strTopic << " success" << endl;
	}
	catch (const mqtt::exception& e)
	{
		cout << "subscribe exception: " << e.what() << endl;
	}
}

void MqttClient::publish(const string& strTopic, const string& strPayload, const int& ulQos)
{
	if (!m_client.is_connected())
	{
		auto now = std::chrono::steady_clock::now();
		if (now - m_lastConnectTry < PUB_RECONNECT_INTERVAL)
		{
			return;
		}
		m_lastConnectTry = now;

		cout << "MQTT publish client not connected, reconnecting..." << endl;
		if (!this->connect())
		{
			cout << "MQTT reconnect failed, drop message on topic " << strTopic << endl;
			return;
		}
		cout << "MQTT reconnect success" << endl;
	}

	try
	{
		m_pubMsg = mqtt::make_message(strTopic, strPayload);
		m_pubMsg->set_qos(ulQos);
		m_client.publish(m_pubMsg)->wait_for(PUBLISH_TIMEOUT_MS);
	}
	catch (const mqtt::exception& e)
	{
		cout << "publish exception: " << e.what() << endl;
	}
}

void MqttClient::disconnect()
{
	try
	{
		m_client.disconnect()->wait();
	}
	catch (const mqtt::exception& e)
	{
		cout << "disconnect exception: " << e.what() << endl;
	}
}

void MqttClient::setOnReadCallback(f_onRead_CB onRead_CB)
{
	m_callback.setOnReadCallback(onRead_CB);
}

Callback::Callback(mqtt::async_client& client, mqtt::connect_options& options)
	: m_client(client)
	, m_options(options)
	, m_onRead_CB(nullptr)
{

}

// 断线后不在回调里重连: sub由mqttsub_entry的循环重连, pub由publish()按需重连
void Callback::connection_lost(const string& cause)
{
	cout << "Callback::connection_lost Connection lost" << endl;
	if (!cause.empty())
	{
		cout << "\tcause: " << cause << endl;
	}
}

void Callback::delivery_complete(mqtt::delivery_token_ptr tok)
{
	// cout << "\tDelivery complete for token: " << (tok ? tok->get_message_id() : -1) << endl;
}

void Callback::on_success(const mqtt::token& tok)
{

}

void Callback::on_failure(const mqtt::token& tok)
{
	cout << "Callback::on_failure, token type: " << static_cast<int>(tok.get_type())
		 << ", rc: " << tok.get_return_code() << endl;
}

void Callback::connected(const string &cause)
{
	cout << "connect success" << endl;
}

void Callback::message_arrived(mqtt::const_message_ptr msg)
{

	const string strTopic = msg->get_topic();
	const string strPayload = msg->to_string();

	if (!m_onRead_CB)
	{
		cout << "Callback::message_arrived recv callback pointer is null" << endl;
		return;
	}

	m_onRead_CB(strTopic, strPayload);
}

void Callback::setOnReadCallback(f_onRead_CB onRead_CB)
{
	m_onRead_CB = onRead_CB;
}
