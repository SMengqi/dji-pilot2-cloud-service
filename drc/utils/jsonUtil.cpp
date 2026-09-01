#define THIS_MODULE MODULE_DRC_COMMON

#include "jsonUtil.h"
#include "pl.h"



std::string read_json_Config_file(std::string &path)
{
    U32 ulFileLen = 0;

    //构建文件读取路径
    std::string filePath = std::string(CONFIG_BOOTUP_PATH) + path;

    if (PF_RET_SUCCESS != pf_get_root_path_file_length((const S8 *)filePath.c_str(), &ulFileLen))
    {
        pl_log(ERR, "pf read config len fail filename %s ", path.c_str());
        return "";
    }

    if (0 == ulFileLen)
    {
        pl_log(ERR, "pf read config len is 0  filename %s ", path.c_str());
        return "";
    }

    S8 *pTmpMemory = (S8 *)pf_malloc(ulFileLen);
    if (PF_RET_SUCCESS != pf_read_root_path_file((const S8 *)filePath.c_str(), pTmpMemory, ulFileLen))
    {
        pl_log(ERR, "pf read config content  fail filename %s ", path.c_str());
        return "";
    }
    std::string str((char *)pTmpMemory);
    pf_free(pTmpMemory);
    return str;
}


bool proto_to_json(const google::protobuf::Message &message, std::string &json, bool add_white_space)
{
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = add_white_space;
    options.always_print_primitive_fields = false; // true // 输出空字段
    options.preserve_proto_field_names = true; // true // 保留字段名而非驼峰式


    return MessageToJsonString(message, &json, options).ok();
}

bool json_to_proto(const std::string &json, google::protobuf::Message &message)
{
    return JsonStringToMessage(json, &message).ok();
}

bool json_to_proto2(const std::string& json, google::protobuf::Message& message) 
{
    google::protobuf::util::JsonParseOptions options;
    options.ignore_unknown_fields = true;  // 忽略proto未声明的字段，避免其它厂商/固件新增字段导致整体解析失败

    auto status = google::protobuf::util::JsonStringToMessage(json, &message, options);
    
    if (!status.ok()) {
        // pl_log(ERR, "JSON Parse Error: %s", status.message());
        std::cout << "JSON Parse Error: " << status.message() << std::endl;
        return false;
    }
    return true;
}
