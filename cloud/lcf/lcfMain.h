#ifndef LCFMAIN_H_
#define LCFMAIN_H_

#include "event.h"
#include "pl.h"
#include "pf_thread_mon.h"

#include <string>

class LcfMain {
public:
    static LcfMain& getInstance() {
        static LcfMain instance;
        return instance;
    }

    // 禁止拷贝构造和赋值（单例模式配套）
    LcfMain(const LcfMain&) = delete;
    LcfMain& operator=(const LcfMain&) = delete;

    // 加载mqtt配置文件
    bool loadMqttConfig();
    bool loadDeviceConfig();

private:
    LcfMain();
    ~LcfMain() = default;

    // 替换topic中的占位符
    void replacePlaceholder(std::string& topic, const std::string& placeholder, 
                           const std::string& replacement);
    // 追加设备序列号到topic
    void appendDeviceSn(std::string& topic, const std::string& deviceSn);

private:
    std::string m_mqttConfigPath   { "mqtt_config.json" };
    std::string m_deviceConfigPath { "device_config.json" };
    std::string m_xmlConfig { "高精地图.xml" };
};

#endif /*LCFMAIN_H_*/