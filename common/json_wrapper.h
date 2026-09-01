/*json_wrapper.h*/
#ifndef JSON_WRAPPER_H
#define JSON_WRAPPER_H

#include "json.h"
#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept> // 为了使用 std::runtime_error

class JsonTools {
public:
    // 将 JSON 字符串解析为 Json::Value 对象
    static Json::Value parseJson(const std::string& jsonStr);

    // 将 Json::Value 对象转换为 JSON 字符串
    static std::string serializeJson(const Json::Value& root);

    // 格式化输出 json对象
    static std::string DebugString(const Json::Value& root);

    // 从 Json::Value 对象中获取字符串值
    static std::string getString(const Json::Value& root, const std::string& key);

    // 从 Json::Value 对象中获取整数值
    static int getInt(const Json::Value& root, const std::string& key);

    static long long getInt64(const Json::Value& root, const std::string& key);

    // 从 Json::Value 对象中获取布尔值
    static bool getBool(const Json::Value& root, const std::string& key);

    // 从 Json::Value 对象中获取双精度浮点数
    static double getDouble(const Json::Value& root, const std::string& key);
    
    // 从 Json::Value 对象中获取单精度浮点数
    static float getFloat(const Json::Value& root, const std::string& key);

    // 从 Json::Value 对象中获取嵌套的 Json::Value
    // static Json::Value getNestedValue(const Json::Value& root, const std::string& key);
    static Json::Value getNestedValue(const Json::Value& root, const std::string& key);

    // 从文件中 获取一行json数据
    static std::string getStringFromFile(const std::string& fileName);
};

#endif // JSON_WRAPPER_H