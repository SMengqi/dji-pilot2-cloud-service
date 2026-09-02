#ifndef RCPROPERTIES_H_
#define RCPROPERTIES_H_

#include <string>
#include <cstdint>

/**
 * 遥控器(网关)属性解析器，对应设计文档3.1节。
 * 输入 thing/product/{rc_sn}/osd 消息，走纯JSON解析（Pilot2无proto定义）。
 * 目前只取 drc_state 字段，其余字段(capacity_percent/wireless_link等)用不到，不解析。
 */
class RcProperties {
public:
    RcProperties() = default;
    ~RcProperties() = default;

    void parseRcOsdData(const std::string& msg);

    uint16_t getDrcState();

private:
    std::string getDrcStateDesc();

private:
    uint16_t m_drcState = 0;
};

#endif /*RCPROPERTIES_H_*/
