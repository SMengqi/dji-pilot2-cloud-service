#define THIS_MODULE MODULE_DEVICE

#include "pl.h"
#include "rcProperties.h"
#include "json.h"

std::string RcProperties::getDrcStateDesc()
{
    switch (m_drcState) {
        case 0: return "未连接";
        case 1: return "连接中";
        case 2: return "已连接";
        default: return "未知状态";
    }
}

uint16_t RcProperties::getDrcState()
{
    return m_drcState;
}

void RcProperties::parseRcOsdData(const std::string& msg)
{
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    std::string errs;

    bool parsingSuccessful = reader->parse(
        msg.c_str(), msg.c_str() + msg.size(),
        &root, &errs
    );

    if (!parsingSuccessful || !errs.empty()) {
        pl_log(ERR, "解析遥控器OSD数据失败: %s", errs.c_str());
        return;
    }

    const Json::Value& data = root["data"];
    if (!data.isObject()) {
        pl_log(ERR, "遥控器OSD数据缺少data字段: %s", msg.c_str());
        return;
    }

    if (data.isMember("drc_state")) {
        m_drcState = static_cast<uint16_t>(data["drc_state"].asUInt());
    }

    pl_log(INF, "DRC链路状态: %s", getDrcStateDesc().c_str());
}
