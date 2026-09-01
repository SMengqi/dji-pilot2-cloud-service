#ifndef _INIHANMINGDISTANCEMAP_H_
#define _INIHANMINGDISTANCEMAP_H_
/*
 * @Author: jiyoufeng    (jiyoufeng@broadxt.com)
 * @Date: 2022-08-18 10:50:34
 * @LastEditors: error: git config user.name && git config user.email & please set dead value or install git
 * @LastEditTime: 2022-11-03 10:43:51
 * @Description:初始化汉明距离近似值map
 * Copyright (c) 2022 by BroadXT Inc, All Rights Reserved.
 */
#include <utility>
#include <cmath>
#include <unistd.h>
#include "drc_event.pb.h"
#include "pl_type.h"
#include <regex>

typedef std::map<std::pair<std::string, std::string>, F32> hanmingValueMap;

class InitHanmingDistanceValueMap
{
public:
    //从配置文件中导入数据至map
    static bool iniHanmingDistanceMap(drc_event::HanmingLetterList mapData);
    
    //计算汉明距离
    static F32 hanmingDistanceForStr(const std::string &leftStr, const std::string &rightStr,U32 flag = 0);  //flag传1，当传参的某一个为空，返回另一个车牌长度

    //计算所调用的近似值map
    static hanmingValueMap hanmingPlateApproximateCharacterMap;

    static BOOL FuzzyMatchingSwitch;
};


#endif