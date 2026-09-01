
#include <iostream>
#include <unistd.h>
#include "MqttClient.h"

using namespace std;

#define TIMEOUT     10000L

bool s_reconnectFlag = false;

MqttClient::MqttClient(string strServer, string clientId)
	: m_strServer(strServer)
	, m_clientId(clientId)
	, m_client(m_strServer, m_clientId)
	, m_callback(m_client, m_connOpts)
{
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

// bool MqttClient::connect()
// {
// 	try
// 	{
// 		mqtt::token_ptr conntok = m_client.connect(m_connOpts);
// 		cout << "Waiting for the connection..." << endl;
// 		conntok->wait_for(1000);
// 	}
// 	catch (const mqtt::exception& e)
// 	{
// 		cout << "connect exception: " << e.what() << endl;
// 		return false;
// 	}

// 	return true;
// }
bool MqttClient::connect()
{
    try
    {
        m_connOpts.set_connect_timeout(2000); // 设置连接超时时间为5秒
        mqtt::token_ptr conntok = m_client.connect(m_connOpts);
        cout << "Waiting for the connection..." << endl;
        conntok->wait_for(1000);
    }
    catch (const mqtt::exception& e)
    {
        cout << "connect exception: " << e.what() << endl;
        return false;
    }

    return true;
}

bool MqttClient::isConnected(void)
{
    if (m_client.is_connected())
	{
		return true;
	}
	return false;
}

void MqttClient::subscribe(const string& strTopic, const int& ulQos)
{
	if (m_client.is_connected())
	{
		m_client.subscribe(strTopic, ulQos, nullptr, m_callback);
		cout << "MQTT subscribe topic " << strTopic << " success" << endl;
	}else
	{
		cout << "MQTT subscribe topic " << strTopic << " err, client connect fail" << endl;
		try {
			std::cout << "尝试重新连接..." << std::endl;
			mqtt::token_ptr conntok = m_client.connect(m_connOpts);
			conntok->wait_for(1000);
			m_client.subscribe(strTopic, ulQos, nullptr, m_callback);
			
		} catch (const mqtt::exception& exc) {
			std::cerr << "重新连接失败: " << exc.what() << std::endl;
			// std::this_thread::sleep_for(std::chrono::seconds(5));
		}
		// std::cout << "重新连接成功" << std::endl;
	}
	
}

void MqttClient::publish(const string& strTopic, const string& strPayload, const int& ulQos)
{
	try
	{
		if(m_client.is_connected())
		{
			m_pubMsg = mqtt::make_message(strTopic, strPayload);
		    m_pubMsg->set_qos(ulQos);
		    m_client.publish(m_pubMsg)->wait_for(TIMEOUT);
		    // cout << "Waiting for up to " << (int)(TIMEOUT/1000) << " seconds for publication of " << strPayload
		    // 		<< " on topic " << strTopic << " for client with ClientID: " << m_clientId << endl;
		}
		else
		{
            try {
                std::cout << "尝试重新连接..." << std::endl;
                mqtt::token_ptr conntok = m_client.connect(m_connOpts);
                conntok->wait_for(1000);
                std::cout << "重新连接成功" << std::endl;
				m_pubMsg = mqtt::make_message(strTopic, strPayload);
		        m_pubMsg->set_qos(ulQos);
		        m_client.publish(m_pubMsg)->wait_for(TIMEOUT);
            } catch (const mqtt::exception& exc) {
                std::cerr << "重新连接失败: " << exc.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
		}
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
	, m_nRetry(0)
	, m_onRead_CB(nullptr)
{

}

void Callback::connection_lost(const string& cause)
{
	cout << "PublishCallback::connection_lost Connection lost" << endl;
	if (!cause.empty())
	{
		cout << "\tcause: " << cause << endl;
	}

	cout << "reconnect......" << endl;
	m_nRetry = 0;
	s_reconnectFlag = true;
	this->reconnect();
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
	cout << "Connection attempt failed" << endl;
	if (++m_nRetry > 3)
	{
		exit(1);
	}
	reconnect();
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

	// cout << "Callback::message_arrived...Trigger a message" << endl;
	m_onRead_CB(strTopic, strPayload);
}

void Callback::setOnReadCallback(f_onRead_CB onRead_CB)
{
	m_onRead_CB = onRead_CB;
}

void Callback::reconnect()
{
	usleep(2500);
	try {
		m_client.connect(m_options, nullptr, *this);
	} catch (const mqtt::exception& e) {
		cout << "PublishCallback::reconnect error: " << e.what() << endl;
		exit(1);
	}
}

