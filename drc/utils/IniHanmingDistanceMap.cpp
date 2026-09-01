/*
 * @Author: jiyoufeng    (jiyoufeng@broadxt.com)
 * @Date: 2022-08-18 10:52:40
 * @LastEditors: error: git config user.name && git config user.email & please set dead value or install git
 * @LastEditTime: 2022-11-16 12:19:10
 * @Description:初始化汉明距离近似值map
 * Copyright (c) 2022 by BroadXT Inc, All Rights Reserved.
 */
#include "IniHanmingDistanceMap.h"

typedef std::pair<std::pair<std::string, std::string>, F32> set_pair;


//构造函数空map
hanmingValueMap InitHanmingDistanceValueMap::hanmingPlateApproximateCharacterMap;
BOOL InitHanmingDistanceValueMap::FuzzyMatchingSwitch {false};
bool InitHanmingDistanceValueMap::iniHanmingDistanceMap(drc_event::HanmingLetterList mapData)
{
    InitHanmingDistanceValueMap::hanmingPlateApproximateCharacterMap.clear();

    int size = mapData.hanmingletter_size();

    for (int i = 0; i < size; i++)
    {
        drc_event::HanmingLetterInfo L = mapData.hanmingletter(i);
        std::string original_str = L.sletter();
        int appCharacter_size = L.appcharacter_size();

        for (int j = 0; i < appCharacter_size; i++)
        {
            drc_event::PairValue p = L.appcharacter(i);
            std::string approximate_str = p.sletter();
            std::pair<std::string, std::string> to_insert_01(original_str, approximate_str);
            std::pair<std::string, std::string> to_insert_10(approximate_str, original_str);
            InitHanmingDistanceValueMap::hanmingPlateApproximateCharacterMap.insert(set_pair(to_insert_01, p.value()));
            InitHanmingDistanceValueMap::hanmingPlateApproximateCharacterMap.insert(set_pair(to_insert_10, p.value()));
        }
    }
    if(InitHanmingDistanceValueMap::hanmingPlateApproximateCharacterMap.size() > 0)
    {
        InitHanmingDistanceValueMap::FuzzyMatchingSwitch = true;
    }

    return 1;
}


static  std::regex base_regFilterChinese("[\u4e00-\u9fa5]+");
static  std::regex base_regOther("[A-Z0-9]+");
F32 InitHanmingDistanceValueMap::hanmingDistanceForStr(const std::string &leftStr, const std::string &rightStr,U32 flag)
{
    F32 ans = 0;
    if(flag == 0)
    {
        //存在车牌信息缺失，则认为车牌一致
        if (leftStr.empty() || rightStr.empty())
        {
            return ans;
        }
    }
    if(flag == 1)
    {
        if (leftStr.empty() || rightStr.empty())
        {
            ans = std::max(leftStr.size(),rightStr.size());
            return ans;
        }
    }
    //获取汉字
    std::string tempLeftStrChinese(std::regex_replace(leftStr, base_regOther, ""));
    std::string tempRightStrChinese(std::regex_replace(rightStr, base_regOther, ""));
    S32 leftChineseLength = tempLeftStrChinese.length();
    S32 rightChineseLength = tempRightStrChinese.length();
    ans += abs(leftChineseLength - rightChineseLength) / 3;
    for (U32 i = 0; i < leftChineseLength; i += 3)
    {

        if (i >= rightChineseLength)
        {
            break;
        }
        if ((tempLeftStrChinese[i] ^ tempRightStrChinese[i]) ||
            (tempLeftStrChinese[i + 1] ^ tempRightStrChinese[i + 1]) ||
            (tempLeftStrChinese[i + 2] ^ tempRightStrChinese[i + 2]))
        {
            if(InitHanmingDistanceValueMap::FuzzyMatchingSwitch)
            {
                std::string left_c(tempLeftStrChinese.begin() + i, tempLeftStrChinese.begin() + i + 3);
                std::string right_c(tempRightStrChinese.begin() + i, tempRightStrChinese.begin() + i + 3);
                hanmingValueMap::iterator iter;
                std::pair<std::string, std::string> value{left_c, right_c};
                iter = InitHanmingDistanceValueMap::hanmingPlateApproximateCharacterMap.find(value);
                //<"浙","鄂">有对应值
                if (iter != InitHanmingDistanceValueMap::hanmingPlateApproximateCharacterMap.end())
                {
                    ans += iter->second;
                    continue;
                }
                else //没有找到两车牌对应的值
                {
                    ans += 1;
                }
            }
            else
            {
                ans += 1;
            }
        }
    }

    std::string tempLeftStr(std::regex_replace(leftStr, base_regFilterChinese, ""));
    std::string tempRightStr(std::regex_replace(rightStr, base_regFilterChinese, ""));
    if (tempLeftStr.empty() || tempRightStr.empty())
    {
        return ans;
    }

    S32 leftLength = tempLeftStr.length();
    S32 rightLength = tempRightStr.length();

    //获取车牌位数差值
    ans += abs(leftLength - rightLength);
    for (U32 i = 0; i < leftLength; ++i)
    {
        if (i >= rightLength)
        {
            break;
        }
        if (tempLeftStr[i] ^ tempRightStr[i])
        {
            if(InitHanmingDistanceValueMap::FuzzyMatchingSwitch)
            {
                std::string left_s{tempLeftStr[i]};
                std::string right_s{tempRightStr[i]};
                hanmingValueMap::iterator iter;
                std::pair<std::string, std::string> num_value(left_s, right_s);
                iter = InitHanmingDistanceValueMap::hanmingPlateApproximateCharacterMap.find(num_value);
                if (iter != InitHanmingDistanceValueMap::hanmingPlateApproximateCharacterMap.end())
                {
                    ans += iter->second;
                    continue;
                }
                else
                {
                    ans += 1;
                }
            }
            else
            {
                ans +=1;
            }

        }
    }
    return ans;
}