#include "json2pb.h"
#include "Geofencing.h"

Geofencing *Geofencing::m_instance = nullptr; //类的静态成员初始化
Geofencing *Geofencing::m_instance_filter = nullptr;


Geofencing::Geofencing()
{
    ;
}

Geofencing::~Geofencing()
{
    ;
}

Geofencing *Geofencing::GetInstance()
{
    if (m_instance == nullptr)
    {
        m_instance = new Geofencing();
    }
    return m_instance;
}

Geofencing *Geofencing::GetInstanceFilter()
{
    if (m_instance_filter == nullptr)
    {
        m_instance_filter = new Geofencing();
    }
    return m_instance_filter;
}

/**
 * @brief 通过json初始化
 * @param strCfg 
 */
void Geofencing::InitConfig(string &strCfg)
{
    json2pb(this->stDrsuRangeConfig, (const char *)strCfg.c_str(), strCfg.length());

    InitPolygonList();
}

/**
 * @brief 通过pb初始化
 * @param pbConfig 
 */
void Geofencing::InitConfig(const if_drsu_geofencing::drsu_cfg_file& pbConfig)
{
    this->stDrsuRangeConfig = pbConfig;

    InitPolygonList();
}

/**
 * @brief 通过json初始化IPRange
 * @param strCfg 
 */
void Geofencing::InitIPRangeConfig(string &strCfg)
{
    json2pb(this->stIPRangeConfig, (const char *)strCfg.c_str(), strCfg.length());

    if_drsu_geofencing::ip_range_config_data *pIPRangeCfgData = NULL;
    int list_size = this->stIPRangeConfig.ip_cfg_list_size();
    for (int index = 0; index < list_size; index++)
    {
        pIPRangeCfgData = this->stIPRangeConfig.mutable_ip_cfg_list(index);
        if (NULL == pIPRangeCfgData)
        {
            continue;
        }

        /*Init IP Grid */
        analysisIPRangeCfgData(pIPRangeCfgData);
    }
}

void Geofencing::AddDrsuPos(U32 ulDrsuId, drc_vector3d pos)
{
    this->stDrsuPosMap[ulDrsuId] = pos;
}


void Geofencing::AddPolygon(U32 ulDrsuId, int PointNums, std::vector<std::pair<double, double>> &polygon)
{
    double pgon[POINT_MAX_NUMBERS][2];

    memset(pgon, 0, sizeof(pgon));

    for (int i = 0; i < PointNums; i++)
    {
        pgon[i][0] = polygon[i].first;
        pgon[i][1] = polygon[i].second;
    }

    /* Create Grid */
    GridSetup(pgon, PointNums, GRID_RESOLUTION, &PolyGrid[PolyCnt]);

    // DrsuPolygonIndex[ulDrsuId].push_back(PolyCnt);

    auto iter = DrsuPolygonIndex.find(ulDrsuId);

    if (iter == DrsuPolygonIndex.end())
    {
        vector<int> vp = {PolyCnt};
        DrsuPolygonIndex.insert({ulDrsuId, vp});
    }
    else
    {
        iter->second.push_back(PolyCnt);
    }

    PolyCnt++;
}

U32 Geofencing::QueryPoint(double x, double y, U32 ulDrsuID)
{
    int ret = OUT_OF_RANGE;
    double point[2];

    point[0] = x, point[1] = y;

    auto iter = DrsuPolygonIndex.find(ulDrsuID);

    if (iter != DrsuPolygonIndex.end())
    {
        for (auto i : iter->second)
        {
            if (GridTest(&PolyGrid[i], point))
            {
                ret = ALLOCATION_SUCCESS;
                break;
            }
        }
    }
    else
    {

        ret = OUT_OF_RANGE;
    }

    return ret;
}

void Geofencing::analysisDrsuRangeCfgData(if_drsu_geofencing::drsu_range_config_data *pDrsuRangeCfgData)
{
    U32 drsuId;
    int point_counts;
    double longitude_x, latitude_y;
    std::vector<std::pair<double, double>> polygon;
    if_drsu_geofencing::Longitude_And_Latitude *pCoordinate = NULL;
    if_drsu_geofencing::Polygon_GeoFencing *pPolygon = NULL;

    if (NULL == pDrsuRangeCfgData)
    {
        return;
    }

    int geofencing_size = pDrsuRangeCfgData->polygon_geofencing_list_size();
    if (geofencing_size <= 0)
    {
        return;
    }

    drsuId = pDrsuRangeCfgData->uldrsu_id();

    for (int polygon_index = 0; polygon_index < geofencing_size; polygon_index++)
    {
        pPolygon = pDrsuRangeCfgData->mutable_polygon_geofencing_list(polygon_index);

        /* Get all points */
        int coordinate_size = pPolygon->geofencing_coordinate_list_size();
        for (int coordinate_index = 0; coordinate_index < coordinate_size; coordinate_index++)
        {
            pCoordinate = pPolygon->mutable_geofencing_coordinate_list(coordinate_index);

            longitude_x = pCoordinate->longitude();
            latitude_y = pCoordinate->latitude();

            polygon.emplace_back(std::make_pair(longitude_x, latitude_y));
        }

        point_counts = coordinate_size;

        /* Init Polygon */
        AddPolygon(drsuId, point_counts, polygon);

        polygon.clear();
    }
}

void Geofencing::InitPolygonList()
{

    if_drsu_geofencing::drsu_range_config_data *pDrsuRangeCfgData = NULL;

    int list_size = stDrsuRangeConfig.drsu_cfg_list_size();

    for (int index = 0; index < list_size; index++)
    {
        pDrsuRangeCfgData = stDrsuRangeConfig.mutable_drsu_cfg_list(index);
        if (NULL == pDrsuRangeCfgData)
        {
            continue;
        }

        /*Init Drsu Grid */
        analysisDrsuRangeCfgData(pDrsuRangeCfgData);
    }
}

bool Geofencing::FindPolygonID(U32 ulID)
{
    if (DrsuPolygonIndex.find(ulID) != DrsuPolygonIndex.end())
    {
        return true;
    }
    else
    {
        return false;
    }
}

U32 Geofencing::QueryLocationArea(double Coordinate_X, double Coordinate_Y, int Zone, U32 ulDrsuId)
{
    U32 result, ret;
    int res;
    double Point_Longitude, Point_Latitude;

    /* UTM -> WGS84 */
    res = UTM2WGS84(Coordinate_X, Coordinate_Y, Point_Latitude, Point_Longitude, Zone);
    switch (res)
    {
    case -1:
    {
        // pl_log(ERR, "Coordinate_X OR Coordinate_Y Error");
        return PARAMETER_ERROR;
    }
    break;
    case -2:
    {
        // pl_log(ERR, "UTM Zone Error");
        return PARAMETER_ERROR;
    }
    break;
    }

    /* Whether the query point is within the Drsu range */
    result = QueryPoint(Point_Longitude, Point_Latitude, ulDrsuId);

    switch (result)
    {
    case ALLOCATION_SUCCESS:
    {
        ret = ALLOCATION_SUCCESS;
    }
    break;
    case OUT_OF_RANGE:
    {
        ret = OUT_OF_RANGE;
    }
    break;
    default:
    {
        ret = ERROR_OTHER;
    }
    }

    return ret;
}

U32 Geofencing::QueryPointInDrsu(double Coordinate_X, double Coordinate_Y, int Zone)
{
     U32 result, ret;
    int res;
    double Point_Longitude, Point_Latitude;

    /* UTM -> WGS84 */
    res = UTM2WGS84(Coordinate_X, Coordinate_Y, Point_Latitude, Point_Longitude, Zone);
    std::vector<int> drsuIdVec;
    for(auto &data : this->DrsuPolygonIndex)
    {
        int ulDrsuId = data.first;
        result = QueryPoint(Point_Longitude, Point_Latitude, ulDrsuId);
        if(ALLOCATION_SUCCESS == result)
        {
            drsuIdVec.push_back(ulDrsuId);
        }
       
    }
    if(drsuIdVec.empty())
    {
        return -1;
    }
    if(drsuIdVec.size() == 1)
    {
        return drsuIdVec[0];
    }
    else
    {
        U32 ulBestDrsuId = -1;
        double dBestDistance = 9999;
        for(auto &data : drsuIdVec)
        {
            drc_vector3d &pos = this->stDrsuPosMap[data];
            double dis  = sqrt(pow(pos.dbx - Coordinate_X , 2) + pow( pos.dby - Coordinate_Y, 2));
            if(dis < dBestDistance)
            {
                dBestDistance = dis;
                ulBestDrsuId = data;
            }
        }
        return ulBestDrsuId;   
    }
    return -1;
}

void Geofencing::analysisIPRangeCfgData(if_drsu_geofencing::ip_range_config_data* pIPRangeCfgData)
{
    std::string ip;
    U32 drsuId;
    int point_counts;
    double longitude_x, latitude_y;
    double ip_x, ip_y;
    std::vector<std::pair<double, double>> polygon;
    if_drsu_geofencing::Longitude_And_Latitude *pCoordinate = NULL;
    if_drsu_geofencing::Polygon_GeoFencing *pPolygon = NULL;

    if (NULL == pIPRangeCfgData)
    {
        return;
    }

    int geofencing_size = pIPRangeCfgData->polygon_geofencing_list_size();
    if (geofencing_size <= 0)
    {
        return;
    }

    ip = pIPRangeCfgData->ulip_id();    
    drsuId = pIPRangeCfgData->uldrsu_id();
    ip_x = pIPRangeCfgData->pos().x();
    ip_y = pIPRangeCfgData->pos().y();
    
    for (int polygon_index = 0; polygon_index < geofencing_size; polygon_index++)
    {
        pPolygon = pIPRangeCfgData->mutable_polygon_geofencing_list(polygon_index);

        /* Get all points */
        int coordinate_size = pPolygon->geofencing_coordinate_list_size();
        for (int coordinate_index = 0; coordinate_index < coordinate_size; coordinate_index++)
        {
            pCoordinate = pPolygon->mutable_geofencing_coordinate_list(coordinate_index);

            longitude_x = pCoordinate->longitude();
            latitude_y = pCoordinate->latitude();

            polygon.emplace_back(std::make_pair(longitude_x, latitude_y));
        }

        point_counts = coordinate_size;

        /* Init Polygon */
        double pgon[POINT_MAX_NUMBERS][2];
        memset(pgon, 0, sizeof(pgon));
        for (int i = 0; i < point_counts; i++)
        {
            pgon[i][0] = polygon[i].first;
            pgon[i][1] = polygon[i].second;
        }

        /* Create Grid */
        GridSetup(pgon, point_counts, GRID_RESOLUTION, &IPPolyGrid[IPPolyCnt]);
        auto iter = IPPolygonMap.find(ip);
        if (iter == IPPolygonMap.end())
        {
            vector<int> vp = {IPPolyCnt};
            IP_Info ipInfo;
            ipInfo.vPolygon = vp;
            ipInfo.ulSubNodeId = drsuId;
            ipInfo.dbPosx = ip_x;
            ipInfo.dbPosy = ip_y;
            IPPolygonMap.insert({ip, ipInfo});            
        }
        else
        {
            iter->second.vPolygon.push_back(IPPolyCnt);            
        }

        IPPolyCnt++;
        polygon.clear();        
    }
    
    //pl_log(WARN, "IPPolygonMap size is %d, IPPolyGrid size is %d", this->IPPolygonMap.size(), IPPolyCnt);
}

std::vector<std::string> Geofencing::QueryPointInIPRange(double Coordinate_X, double Coordinate_Y, int Zone)
{
    U32 result, ret;
    int res;
    double Point_Longitude, Point_Latitude;

    /* UTM -> WGS84 */
    res = UTM2WGS84(Coordinate_X, Coordinate_Y, Point_Latitude, Point_Longitude, Zone);    
    std::vector<std::string> ipVec;
    for(auto &data : this->IPPolygonMap)
    {
        std::string ip = data.first;        
        result = QueryIPPoint(Point_Longitude, Point_Latitude, ip);
        if(ALLOCATION_SUCCESS == result)
        {
            ipVec.push_back(ip);
        }
       
    }
    //pl_log(WARN, "(%f, %f) get WGS84 (%f, %f) Zone(%d), IPPolygonMap(%d), ipVec(%d)", Coordinate_X, Coordinate_Y, Point_Longitude, Point_Latitude, Zone, this->IPPolygonMap.size(), ipVec.size());

    return ipVec;
    //if(ipVec.empty())
    //{
    //    return "";
    //}
    //else
    //{
    //    return ipVec[0];
    //}
    //return "";
}

U32 Geofencing::QueryIPPoint(double x, double y, std::string ip)
{
    int ret = OUT_OF_RANGE;
    double point[2];
    point[0] = x, point[1] = y;

    auto iter = IPPolygonMap.find(ip);
    if (iter != IPPolygonMap.end())
    {    
        //pl_log(WARN, "find ip_%s for (%f, %f) in IPPolygonMaps", ip.c_str(), x, y);
        for (auto i : iter->second.vPolygon)
        {
            if (GridTest(&IPPolyGrid[i], point))
            {
                ret = ALLOCATION_SUCCESS;
                break;
            }
        }
    }
    else
    {

        ret = OUT_OF_RANGE;
    }

    return ret;
}

const IP_Info* Geofencing::QueryIPInfo(std::string ip)
{    
    auto iter = IPPolygonMap.find(ip);
    if (iter != IPPolygonMap.end())
    {
        return &(iter->second);
    }
    else
    {

        return nullptr;
    }
}


