#ifndef DRC_JSON_UTILS_H
#define DRC_JSON_UTILS_H


#include <google/protobuf/util/json_util.h>
#include <iostream>
using google::protobuf::util::JsonStringToMessage;
bool proto_to_json(const google::protobuf::Message &message, std::string &json, bool add_white_space = false);

bool json_to_proto(const std::string &json, google::protobuf::Message &message);

// 与json_to_proto相同，但忽略JSON中proto未声明的未知字段，避免因个别新增字段导致整体解析失败
bool json_to_proto2(const std::string &json, google::protobuf::Message &message);

std::string read_json_Config_file(std::string &path);


#endif // DRC_JSON_UTILS_H
