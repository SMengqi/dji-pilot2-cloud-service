#ifndef GEO_FENCING_H
#define GEO_FENCING_H

#include <string>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>

#include "pl_type.h"
#include "if_drsu_geofencing.pb.h"
#include "ptinpoly.h"
#include "utm.h"

#include "if_drc_common.h"
using std::string;
using std::vector;

#define GRID_RESOLUTION 30
#define POLYGON_MAX_NUMBERS 15010
#define POINT_MAX_NUMBERS 200

enum{
  ALLOCATION_SUCCESS = 0, //UP Allocation succeeded 
  PARAMETER_ERROR,    
  OUT_OF_RANGE,       //Device out of range
  COORDINATE_SYSTEM_ERROR,
  ERROR_OTHER,
};

typedef enum{
  UTM = 0,
  WGS84,
}Coordinate_System_E;

typedef struct 
{
    U32 ulSubNodeId;
    double dbPosx, dbPosy;
    vector<int> vPolygon;
}IP_Info;

class Geofencing
{
private:
    if_drsu_geofencing::drsu_cfg_file stDrsuRangeConfig;
    std::unordered_map<U32 ,vector<int> > DrsuPolygonIndex; //Save Drsu ID
    GridSet PolyGrid[POLYGON_MAX_NUMBERS]; //Save Grid Information
    int PolyCnt = 0; //Grid Count    
    std::map<U32,drc_vector3d> stDrsuPosMap;

    if_drsu_geofencing::ip_cfg_file stIPRangeConfig;
    std::unordered_map<std::string, IP_Info> IPPolygonMap;
    GridSet IPPolyGrid[POLYGON_MAX_NUMBERS]; //Save Grid Information
    int IPPolyCnt = 0; //Grid Count    
    std::unordered_map<U32,drc_vector3d> stIPInfo;

private:
    static Geofencing *m_instance;  
    static Geofencing *m_instance_filter;
public:
    static Geofencing *GetInstance();
    static Geofencing *GetInstanceFilter();

    void InitConfig(string & strCfg);
    void InitConfig(const if_drsu_geofencing::drsu_cfg_file& pbConfig);
    void InitIPRangeConfig(string & strCfg);
    
    const IP_Info* QueryIPInfo(std::string ip);
    /**********************************************************************************************
    * @function   : InitPolygonList
    *
    * @discription: Init GeoFencing
    * @input      : 
    * @output     : 
    * @date       : 2020/11/30
    **********************************************************************************************/
    void InitPolygonList();


    /**********************************************************************************************
    * @function   : FindPolygonID
    *
    * @discription: 判断该ID下是否有围栏
    * @input      : ID编号 
    * @output     : 查询结果true  false
    * @date       : 2021/11/29
    **********************************************************************************************/
    bool FindPolygonID(U32 ulID);


    /**********************************************************************************************
    * @function   : QueryLocationArea
    *
    * @discription: 判断点是否在区域范围内
    * @input      : UTM Coordinate System: 
    *                                      Coordinate_X -> East
    *                                      Coordinate_Y -> North
    *               WGS84 Coordinate System:
    *                                      Coordinate_X -> Longitude
    *                                      Coordinate_Y -> Latitude
    *               Zone: 时区 1-60
    * @output     : 查询结果
    * @date       : 2021/11/29
    **********************************************************************************************/
    U32 QueryLocationArea(double Coordinate_X,double Coordinate_Y,int Zone, U32 ulDrsuId);




  /**********************************************************************************************
    * @function   : QueryPointInDrsu
    *
    * @discription: 判断点在那个区域，返回drsuId
    * @input      : UTM Coordinate System: 
    *                                      Coordinate_X -> East
    *                                      Coordinate_Y -> North
    *               WGS84 Coordinate System:
    *                                      Coordinate_X -> Longitude
    *                                      Coordinate_Y -> Latitude
    *               Zone: 时区 1-60
    * @output     : 查询结果
    * @date       : 2021/11/29
    **********************************************************************************************/
    U32 QueryPointInDrsu(double Coordinate_X,double Coordinate_Y, int Zone = 51);
  
    std::vector<std::string>  QueryPointInIPRange(double Coordinate_X,double Coordinate_Y, int Zone = 51);


    /**********************************************************************************************/
    void AddDrsuPos(U32 ulDrsuId, drc_vector3d pos);
private:
    void AddPolygon(U32 ulDrsuId, int PointNums, std::vector<std::pair<double, double> > &polygon);

    U32  QueryPoint(double x, double y ,U32 ulDrsuID);

    U32  QueryIPPoint    (double x, double y ,std::string ip);


    /**********************************************************************************************
    * @function   : analysisUpRangeCfgData
    *
    * @discription: analysis if_up_geofencing::Polygon_GeoFencing
    * @input      : 
    * @output     : 
    * @date       : 2020/12/05
    **********************************************************************************************/
    void analysisDrsuRangeCfgData(if_drsu_geofencing::drsu_range_config_data* pDrsuRangeCfgData);    

    void analysisIPRangeCfgData(if_drsu_geofencing::ip_range_config_data* pIPRangeCfgData);
private:
    Geofencing();

    ~Geofencing();
};

#endif