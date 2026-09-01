#define THIS_MODULE MODULE_LCF

/* Includes ------------------------------------------------------------------*/
#include "lcfMain.h"

#include "jsonUtil.h"
#include "bxt_mqtt_config.pb.h"
#include "bxt_cloud_common.pb.h"
#include "pl_utils.h"
#include "pf_map_block_3d.h"

#include <unistd.h>
#include <string>

/* Private constants ---------------------------------------------------------*/
 
/* Exported values -----------------------------------------------------------*/

bxt_mqtt::mqtt_config_data s_pbMqttCfg;
bxt_cloud_common::common_message s_pbCommonCfg;

/* Private functions declaration ---------------------------------------------*/
LcfMain::LcfMain() {}

void LcfMain::replacePlaceholder(std::string& topic, const std::string& placeholder, 
                                 const std::string& replacement)
{
    size_t pos = topic.find(placeholder);
    if (pos != std::string::npos) {
        topic.replace(pos, placeholder.length(), replacement);
    }
}

void LcfMain::appendDeviceSn(std::string& topic, const std::string& deviceSn)
{
    topic += deviceSn;
}

bool LcfMain::loadMqttConfig() 
{
    std::string strMqttConfig = read_json_Config_file(m_mqttConfigPath);
    if (strMqttConfig.empty()) {
        pl_log(ERR, "读取MQTT配置文件失败 | 路径: %s", m_mqttConfigPath.c_str());
        return false;
    }

    if (!json_to_proto(strMqttConfig, s_pbMqttCfg)) {
        pl_log(ERR, "MQTT配置JSON解析失败 | 路径: %s", m_mqttConfigPath.c_str());
        return false;
    }
    
    bxt_mqtt::mqtt_publish* pbPublish = s_pbMqttCfg.mutable_publish();
    if (!pbPublish) {
        pl_log(ERR, "MQTT配置中缺少publish字段");
        return false;
    }

    const std::string& aircraftSn = s_pbCommonCfg.aircraft_sn();
    const std::string& dockSn = s_pbCommonCfg.dock_sn();
    const std::string placeholder = "+";

    // 处理需要追加设备序列号的topic
    std::string topic = pbPublish->bxt_osd();
    appendDeviceSn(topic, dockSn); // aircraftSn
    pbPublish->set_bxt_osd(topic);

    // 处理需要替换占位符的topic（使用dock_sn）
    topic = pbPublish->dji_services();
    replacePlaceholder(topic, placeholder, dockSn);
    pbPublish->set_dji_services(topic);

    topic = pbPublish->kmz_request();
    replacePlaceholder(topic, placeholder, dockSn);
    pbPublish->set_kmz_request(topic);

    topic = pbPublish->dji_requests_reply();
    replacePlaceholder(topic, placeholder, dockSn);
    pbPublish->set_dji_requests_reply(topic);

    topic = pbPublish->dji_drc_down();
    replacePlaceholder(topic, placeholder, dockSn);
    pbPublish->set_dji_drc_down(topic);

    // 处理需要追加dock序列号的topic
    topic = pbPublish->bxt_uav_status();
    appendDeviceSn(topic, dockSn);
    pbPublish->set_bxt_uav_status(topic);

    topic = pbPublish->bxt_uav_result();
    appendDeviceSn(topic, dockSn);
    pbPublish->set_bxt_uav_result(topic);

    topic = pbPublish->bxt_payload_param();
    appendDeviceSn(topic, dockSn);
    pbPublish->set_bxt_payload_param(topic);

    pl_log(INF, "MQTT配置加载成功");
    pl_log(TRC, "MQTT配置详情:\n%s", s_pbMqttCfg.DebugString().c_str());

    return true;
}

bool LcfMain::loadDeviceConfig()
{
    // 地图加载失败不应阻断设备配置/MQTT配置加载，两者是独立的关注点；
    // 但仍需尝试加载，flytoController::check_block_around_line_lonlat 等障碍物检测依赖这份数据。
    std::string mapFile = "." + std::string(CONFIG_BOOTUP_PATH) + m_xmlConfig;
    S32 mapRet = get_map_info((char*)mapFile.c_str());
    if (mapRet != RET_OK) {
        pl_log(ERR, "加载3D地图文件: %s 失败 | ret: %d，障碍物检测将拿不到真实地图数据", mapFile.c_str(), mapRet);
    } else {
        pl_log(INF, "加载3D地图文件: %s 成功", mapFile.c_str());
    }

    std::string strDeviceConfig = read_json_Config_file(m_deviceConfigPath);
    if (strDeviceConfig.empty()) {
        pl_log(ERR, "读取设备配置文件失败 | 路径: %s", m_deviceConfigPath.c_str());
        return false;
    }

    if (!json_to_proto(strDeviceConfig, s_pbCommonCfg)) {
        pl_log(ERR, "设备配置JSON解析失败 | 路径: %s", m_deviceConfigPath.c_str());
        return false;
    }

    pl_log(INF, "设备配置加载成功");
    pl_log(TRC, "设备配置详情:\n%s", s_pbCommonCfg.DebugString().c_str());

    return true;
}
/**********************************************************************************************
* @function   : lcf_init
*
* @discription: lcf_init
* @input      :
* @output     :
* @date       : 2025/10/22
**********************************************************************************************/
S32 lcf_init(U32 ulModuleId)
{
    pl_log(INF, "lcf init module ID is %d", ulModuleId);
    
    LcfMain& instance = LcfMain::getInstance();
    
    // 先加载设备配置，因为MQTT配置需要用到设备配置中的序列号
    if (!instance.loadDeviceConfig()) {
        pl_log(ERR, "设备配置加载失败，初始化终止");
        return PF_RET_FAILURE;
    }

    if (!instance.loadMqttConfig()) {
        pl_log(ERR, "MQTT配置加载失败，初始化终止");
        return PF_RET_FAILURE;
    }
    
    return PF_RET_SUCCESS;
}

/**********************************************************************************************
* @function   : lcf_entry
*
* @discription: lcf_entry
* @input      :
* @output     :
* @date       : 2025/10/22
**********************************************************************************************/
void lcf_entry(U32 ulSrcModuleId, U32 ulMsgId, U32 ulDstModuleId, 
                               void* pcvMsg, U32 ulLength)
{
    (void)ulSrcModuleId;
    (void)ulDstModuleId;
    (void)pcvMsg;
    (void)ulLength;
    
    switch(ulMsgId)
    {
        case COMMON_REG_IND: { 
            pl_log(INF, "收到注册消息，LCF模块已就绪");
        } break;
        
        default: { 
            pl_log(ERR, "未预期的消息ID | msgId: %d(0x%x)", ulMsgId, ulMsgId); 
        } break;
    }
}



