/************************************************************
  Copyright (C), BroadXT Inc  2019
  FileName   :pf_map_block.h
  Author     :zhangyan     
  Version    :v0.1         
  Date       :2021-03-15
  Description:
  History:
  <author>       <time>       <version >   <desc>
  zhangyan       2021-03-15   v0.1         create 
***********************************************************/
#ifndef PF_MAP_BLOCK_H

#define PF_MAP_BLOCK_H

#include "pl.h"
#include <math.h>


#undef GLOBAL
#ifdef PF_MAP_BLOCK_CPP
#define GLOBAL    
#else
#define GLOBAL   extern 
#endif


//#define pf_map_log(ucLogLevel, format, ...)                     pf_log(ucLogLevel, THIS_LOG_MODULE, format, ## __VA_ARGS__)
#define pf_map_log( format, ...)                                printf(  format, ## __VA_ARGS__)




#define MAP_INFO            0
#define MAP_ROAD            1 
#define MAP_CONNECTION      2  
#define MAP_LINK            3
#define MAP_SECTION         4
#define MAP_LANE            5
#define MAP_WAITING_LANE    6
#define MAP_TRAFFICLIGHT    7


#define MAP_EXTRAPOINT      8
#define MAP_LANEMARKER      9
#define MAP_PAVEMENT        10

#define MAP_NO_RUN_ZONE     11
#define MAP_PARKING_ZONE    12
#define MAP_CENTER_LINE     13
#define MAP_ROADEDGE        14
#define MAP_MILEAGE         15

#define MAP_LANE_LINK       16

#define MAP_ELEMENT_MAX     17


#define MAP_CONNECTION_VIRTUAL      23  



typedef enum
{
    ROAD_CITY = 0,
    ROAD_HIGHWAY = 1,
    ROAD_TYPE_MAX,
}PF_MAP_ROAD_TYPE_E;


typedef enum
{
    LINK_TURN_LEFT = 0,
    LINK_STRAIGHT = 1,
    LINK_TURN_RIGHT = 2,
    LINK_TURN_AROUND = 3,
    LINK_TYPE_MAX,
}PF_MAP_LINK_TYPE_E;


typedef enum
{
    CONNECTION_VIRTUAL = 0,
    CONNECTION_REAL = 1,
    CONNECTION_TYPE_MAX,
}PF_MAP_CONNECTION_TYPE_E;


typedef enum
{
    LANE_VEHICLE = 0,
    LANE_BICYCLE = 1,
    LANE_EMERGENCY = 2,
    LANE_RAMP = 3,
    LANE_BUS = 4,
    LANE_TYPE_MAX,
}PF_MAP_LANE_TYPE_E;


typedef enum
{
    LANE_NONE = 0,
    LANE_STRAIGHT = 1,
    LANE_TURN_LEFT = 2,
    LANE_STRAIGHT_LEFT = 3,
    LANE_TURN_RIGHT = 4,
    LANE_STRAIGHT_RIGHT = 5,
    LANE_LEFT_RIGHT = 6,
    LANE_LEFT_STRAIGHT_RIGHT = 7,
    LANE_TURN_AROUND = 8,
    LANE_STRAIGHT_AROUND = 9,
    LANE_LEFT_AROUND = 10,
    LANE_STRAIGHT_LEFT_AROUND = 11,
    LANE_RIGHT_AROUND = 12,
    LANE_STRAIGHT_RIGHT_AROUND = 13,
    LANE_LEFT_RIGHT_AROUND = 14,
    LANE_LEFT_STRAIGHT_RIGHT_AROUND = 15,
    LANE_TURN_TYPE_MAX,
    
}PF_MAP_LANE_TURN_TYPE_E;




/*
交通信号灯类型,参考国标“GA-T-1049.2-2013交通信号控制系统.pdf”中的B.9信号灯组类型 
10:机动车主灯(大灯，非箭头灯，一般的红黄绿)
11:机动车直行箭头灯
12:机动车左转箭头灯
13:机动车右转箭头灯
14:机动车掉头箭头灯
21:非机动车灯
22:直行非机动车灯
23:左转非机动车灯
31:行人灯
99:其他 
*/

typedef enum
{
    LIGHT_VEHICLE_MAIN = 10,
    LIGHT_VEHICLE_STRAIGHT = 11,
    LIGHT_VEHICLE_LEFT = 12,
    LIGHT_VEHICLE_RIGHT = 13,
    LIGHT_VEHICLE_AROUND = 14,
    LIGHT_BICYCLE = 21,
    LIGHT_BICYCLE_STRAIGHT = 22,
    LIGHT_BICYCLE_LEFT = 23,
    LIGHT_WALK = 31,
    LIGHT_TYPE_OTHER = 99,
    LIGHT_TYPE_MAX,
}PF_MAP_TRAFFIC_LIGHT_TYPE_E;



/*
车道流向,参考国标“GA-T-1049.2-2013交通信号控制系统.pdf”中的B.13 车道流向
11:直行
12:左转
13:右转
21:直左混行
22:直右混行
23:左右混行
24:直左右混行
31:掉头
99:其他

*/



typedef enum
{
    LIGHT_STRAIGHT = 11,
    LIGHT_TURN_LEFT = 12,
    LIGHT_TURN_RIGHT = 13,
    LIGHT_STRAIGHT_LEFT = 21,
    LIGHT_STRAIGHT_RIGHT = 22,
    LIGHT_LEFT_RIGHT = 23,
    LIGHT_LEFT_STRAIGHT_RIGHT = 24,
    LIGHT_TURN_AROUND = 31,
    LIGHT_TURN_TYPE_OTHER = 99,
    LIGHT_TURN_TYPE_MAX,
}PF_MAP_TRAFFIC_LIGHT_TURN_TYPE_E;


typedef enum
{
    LIGHT_COMBINATION_HORIZONTAL = 0,
    LIGHT_COMBINATION_VERTICAL = 1,
    LIGHT_COMBINATION_MAX,
}PF_MAP_TRAFFIC_LIGHT_COMBINATION_E;



/*
（0北/1东北/2东/3东南/4南/5西南/6西/7西北）
*/
typedef enum
{
    DIRECTION_NORTH = 0,
    DIRECTION_NORTH_EAST = 1,
    DIRECTION_EAST = 2,
    DIRECTION_SOUTH_EAST = 3,
    DIRECTION_SOUTH = 4,
    DIRECTION_SOUTH_WEST = 5,
    DIRECTION_WEST = 6,
    DIRECTION_NORTH_WEST = 7,
    DIRECTION_MAX,
}PF_MAP_TRAFFIC_DIRECTION_E;





typedef enum
{
    LINE_WHITE_DOTTED = 0,
    LINE_WHITE_SOLID = 1,
    LINE_YELLOW_SOLID = 2,
    LINE_YELLOW_DOTTED = 3,
    LINE_DOUBLE_WHITE_SOLID = 4,
    LINE_DOUBLE_WHITE_DOTTED = 5,
    LINE_DOUBLE_YELLOW_SOLID = 6,
    LINE_WHITE_SOLID_DOTTED = 7,
    LINE_WHITE_DOTTED_SOLID = 8,
    LINE_YELLOW_SOLID_DOTTED = 9,
    LINE_YELLOW_DOTTED_SOLID = 10,
    LINE_VIRTUAL = 11,
    LINE_TYPE_MAX,
}PF_MAP_LINE_TYPE_E;









typedef struct
{
    S32      mapId;               // 地图ID   
    S32      type;                // 地图类型
    double   utm_x_min;           // 地图范围 UTM坐标 x 方向最小值 
    double   utm_y_min;           // 地图范围 UTM坐标 y 方向最小值 
    double   utm_x_max;           // 地图范围 UTM坐标 x 方向最大值 
    double   utm_y_max;           // 地图范围 UTM坐标 y 方向最大值     
    char     mapname[128];        // 地图名称
    char     createTime[32];      // 创建时间
    char     version[32];         // 地图版本
    char     producer[40];        // 地图文件出品商
    char     sourceFile[40];      // 地图源文件
    char     utm_zone[16];        // UTM坐标区号
}PF_MAP_INFO_ST;



typedef struct
{
    S32      ID;                  // 道路ID   
    S32      length;              // 道路长度，单位（米）    
    S16      laneWidth;           // 车道平均宽度，单位（厘米）  
    S8       type;                // 道路类型        /0城市道路/1高速公路
    S8       dividerLine_i;       // 道路中心线 index;  -1:无中心线
    char     roadName[116];       // 道路名称
}PF_MAP_ROAD_ST;




typedef struct
{
    S32      ID;                  // 路口ID   
    S16      s16angle;            // 路口行驶方向角   INVALID_ANGLE：无效角度
    S16      extrapoint_i;        // 外边界点集  index
    S16      link_i;              // 路段连接关系 index ; 
    S16      pavement_i;          // 斑马线 index;      -1:无斑马线
    S16      laneLink_i;          // 车道连接关系 index ; 
    S8       linkCount;           // 路段连接关系数量 ; 
    S8       pavementCount;       // 斑马线数量; 
    S8       laneLinkCount;       // 车道连接关系数量 ; 
    S8       type;                // 路口类型           /0虚拟路口/1真实路口
    S8       road_count;          // 路口所属道路数量   
    S8       road_i[5];           // 路口所属道路 index;  -1:无效
}PF_MAP_CONNECTION_ST;



typedef struct
{
    S32      ID;                  // 路段ID   
    S16      fromConnection_i;    // 来向路口 index;  -1:无来向路口
    S16      toConnection_i;      // 去向路口 index;  -1:无去向路口
    S16      tracficLight_i;      // 交通信号灯 index; -1:无交通信号灯  
    S16      lane_i;              // 车道 index;  
    S8       tracficLightCount;   // 交通信号灯数量
    S8       laneCount;           // 车道数量
    S8       road_i;              // 所属道路 index 
    S8       type;                // 道路类型        /0城市道路/1高速公路
}PF_MAP_SECTION_ST;




typedef struct
{
    S32      ID;                  // 车道ID   
    S8       type;                // 车道类型        /0机动/1非机动/2应急/3匝道/4公交
    S8       turnType;            // 车道转向类型    /0无方向/1直行/2左转/4右转/8掉头
    U8       limitSpeedMax;       // 车道限速最大值;   km/h    
    U8       limitSpeedMin;       // 车道限速最小值;   km/h
    S16      leftLanemarker_i;    // 左边车道线 index 
    S16      centreLanemarker_i;  // 车道中心线 index  
    S16      rightLanemarker_i;   // 右边车道线 index 
    S16      section_i;           // 所属路段 index
    S32      s32angle;
    S16      waitingLane_i;       // 等待区车道 index      -1:无等待区车道
    S8       laneNumber;          // 车道在道路中的编号      
    S8       roadSide;
}PF_MAP_LANE_ST;




typedef struct
{
    S16      lane_i;              // 所属车道 index   
    S16      leftLanemarker_i;    // 左边车道线 index 
    S16      centreLanemarker_i;  // 车道中心线 index  
    S16      rightLanemarker_i;   // 右边车道线 index 
}PF_MAP_WAITING_LANE_ST;





typedef struct
{
    DOUBLE   utm_x;                  // 灯组位置 UTM坐标 x 方向 
    DOUBLE   utm_y;                  // 灯组位置 UTM坐标 y 方向 
    DOUBLE   utm_z;                  // 灯组位置 UTM坐标 z 方向 
    S8       index;                  // 灯组在信号灯中的序号   
    S8       entrance;               // 灯组控制入口路段方向            PF_MAP_TRAFFIC_DIRECTION_E
    S8       light_type;             // 灯组信号灯类型                  PF_MAP_TRAFFIC_LIGHT_TYPE_E
    S8       number;                 // 灯组中灯的数量
    S8       turn_type;              // 灯组信号灯控制车流向类型        PF_MAP_TRAFFIC_LIGHT_TURN_TYPE_E
    S8       combination_type;       // 灯组信号灯排列模式              PF_MAP_TRAFFIC_LIGHT_COMBINATION_E
    S8       countdown_number_width; // 倒计时数字位数;         0:无
    S8       independent_pole;       // 是否有独立灯杆          0:无        1:独立灯杆
    S8       is_waiting_area_light;  // 是否为待转区控制灯 
    S8       bind_lane_count;        // 灯组绑定车道的数量
    S8       bind_lane_index[6];     // 灯组绑定车道顺序号列表 laneNumber
}PF_MAP_LAMP_GROUP_ST;




typedef struct
{
    S32      ID;                     // 交通信号灯ID   
    S8       lampGroupCount;         // 灯组数量
    S8       sectionCount;           // 关联路段数量
    S16      sectionIndexList[5];    // 所属路段编号列表 
    DOUBLE   start_utm_x;            // 交通信号灯横杆起点位置 UTM坐标 x 方向 
    DOUBLE   start_utm_y;            // 交通信号灯横杆起点位置 UTM坐标 y 方向 
    DOUBLE   start_utm_z;            // 交通信号灯横杆起点位置 UTM坐标 z 方向 
    DOUBLE   end_utm_x;              // 交通信号灯横杆终点位置 UTM坐标 x 方向 
    DOUBLE   end_utm_y;              // 交通信号灯横杆终点位置 UTM坐标 y 方向 
    DOUBLE   end_utm_z;              // 交通信号灯横杆终点位置 UTM坐标 z 方向 
    PF_MAP_LAMP_GROUP_ST lamp_group[8];
}PF_MAP_TRAFFICLIGHT_ST;





typedef struct
{
    S16      type;                // 连接类型;         -1:无效连接;   0:左转;   1:直行;   2:右转;   3:掉头
    S16      fromSection_i;       // 来向路段 index;   -1:无来向路段; 
    S16      toSection_i;         // 去向路段 index;   -1:无去向路段; 
    S16      s16rsv;            
}PF_MAP_LINK_ST;



typedef struct
{
    S16      type;                // 连接类型;         -1:无效连接;   0:左转;   1:直行;   2:右转;   3:掉头
    S16      fromLane_i;          // 来向车道 index;   -1:无来向路段; 
    S16      toLane_i;            // 去向车道 index;   -1:无去向路段; 
    S16      s16rsv;            
}PF_MAP_LANE_LINK_ST;





typedef struct
{
    DOUBLE   node_x;    // 起点 UTM 坐标
    DOUBLE   node_y;    // 起点 UTM 坐标
    DOUBLE   k;         // 斜率， 注释：如果 (k==0&&b==0) 则连线角度为 90 度，公式表示为 x = node_x；
    DOUBLE   b;         // 截距
}PF_MAP_NODE_LINE_ST;



typedef struct
{
    S32      ID; 
    S16      nodeCount;
    S8       type;          // 线类型；  0白虚线/1白实线/2黄实线/3黄虚线/4双白实线/5双白虚线/6双黄实线/7白色实虚线/8白色虚实线/9黄色实虚线/10黄色虚实线/11虚拟车道线
    S8       insideMode;    // 多边形内侧判定      0:非封闭多边形/1:左边为内侧/2:右边为内侧  

//    PF_MAP_NODE_LINE_ST   nodeline[];         
}PF_MAP_LINE_INFO_ST;



typedef struct
{
    S32      ID; 
    S16      section_i;          // 斑马线对应交通信号灯所属路段 index
    S8       nodeCount;
    S8       insideMode;         // 多边形内侧判定      0:非封闭多边形/1:左边为内侧/2:右边为内侧  
//    PF_MAP_NODE_LINE_ST   nodeline[];         
}PF_MAP_PAVEMENT_INFO_ST;



typedef struct
{
    S32      ID; 
    S16      nodeCount;
    S8       type;        
    S8       insideMode;    // 多边形内侧判定      0:非封闭多边形/1:左边为内侧/2:右边为内侧  
    S32      road_i;        // 所属道路 index
    S16      angle;         // 方向角度
    S16      roadSide;      // 上行/下行
//    PF_MAP_NODE_LINE_ST   nodeline[];         
}PF_MAP_NORUNZONE_INFO_ST;






typedef struct
{
    S32      ID; 
    S16      nodeCount;
    S8       acrossType;    // 道路边沿的跨越性：( 0:不可跨越路沿 1:可跨越路沿 2:机动车不可跨越行人可跨越路沿 )
    S8       type;          // 道路边沿类型 ( 0:绿化带  1:正常安全岛  2:栏杆处安全岛  3:栏杆  4:人行道  5:车道路沿过渡带 )

//    PF_MAP_NODE_LINE_ST   nodeline[];         
}PF_MAP_ROADEDGE_INFO_ST;





typedef struct
{
    S32      ID;            // 高速公路里程桩ID   
    S32      number;        // 里程桩上的实际数值，单位：米
    double   distance;      // 里程桩距离高精地图起点的里程值（单位：米) 
    double   utm_x;         // 里程桩 UTM 坐标 x 值
    double   utm_y;         // 里程桩 UTM 坐标 y 值
    S8       road_i;        // 里程桩所属道路 index
    S8       side;          // 里程桩路侧信息  1：里程桩位于高速公路上行（起点至终点）侧，2：里程桩位于高速公路下行（终点至起点）侧
    CHAR     content[30];   // 里程桩上的内容，参考为“G25_D04_K20_M100”，表示的位置为G25高速04路段20公里100米
}PF_MAP_MILEAGE_ST;



typedef struct
{
    S32      x;    
    S32      y;    
    DOUBLE   z;    
}PF_MAP_UTM_XYZ_ST;





typedef struct
{
    DOUBLE      x;                 
    DOUBLE      y;                 
}PF_MAP_NODE_STRUCT;



GLOBAL PF_MAP_INFO_ST*               pstMap;
GLOBAL PF_MAP_ROAD_ST*               pstRoad;
GLOBAL PF_MAP_CONNECTION_ST*         pstConnection;
GLOBAL PF_MAP_LINK_ST*               pstLink;
GLOBAL PF_MAP_LANE_LINK_ST*          pstLaneLink;

GLOBAL PF_MAP_SECTION_ST*            pstSection;
GLOBAL PF_MAP_LANE_ST*               pstLane;
GLOBAL PF_MAP_WAITING_LANE_ST*       pstWaitingLane;
GLOBAL PF_MAP_TRAFFICLIGHT_ST*       pstTrafficLight;


GLOBAL PF_MAP_LINE_INFO_ST**         pastExtrapoint;
GLOBAL PF_MAP_LINE_INFO_ST**         pastLanemarker;
GLOBAL PF_MAP_PAVEMENT_INFO_ST**     pastPavement;
GLOBAL PF_MAP_NORUNZONE_INFO_ST**    pastNoRunZone;
GLOBAL PF_MAP_LINE_INFO_ST**         pastParkingZone;
GLOBAL PF_MAP_LINE_INFO_ST**         pastDividerLine;
GLOBAL PF_MAP_ROADEDGE_INFO_ST**     pastRoadEdge;
GLOBAL PF_MAP_MILEAGE_ST*            pstMileage;




GLOBAL S32 gas32ElementCount[MAP_ELEMENT_MAX];

GLOBAL string gStrUtmZone;



typedef struct
{
    S16      lane_i;               // 车道 index   
    S16      leftLanemarker_i;     // 左边车道线 index; 
    S16      rightLanemarker_i;    // 右边车道线 index; 
    S16      leftNodeLineCount;    // 左边车道线段数量; 注释：0 表示左边车道线不在区块内    
    S16      rightNodeLineCount;   // 右边车道线段数量; 注释：0 表示右边车道线不在区块内
    S16      leftNodeLine_i;       // 左边车道线第一段 index 
    S16      rightNodeLine_i;      // 右边车道线第一段 index 
    S16      s16rsv;
    
}PF_BLOCK_LNAE_ST;



typedef struct
{
    S16      connection_i;    // connection index 
    S16      extrapoint_i;    // connection 边线 index
    S16      nodeLineCount;   // connection 边线段数量
    S16      nodeLine_i;      // connection 边线第一段 index
}PF_BLOCK_CONNECTION_ST;



typedef struct
{  
    S8       blockSate;          // 
    S8       connectionCount;    // connection 数量 
    S16      laneCount;          // 车道数量   
    S16      parkingZone_i[3];   // 区块内停车区 index        
    S16      noRunZone_i[3];     // 区块内禁行区 index     
    
//  PF_BLOCK_CONNECTION_ST  stconnection[]; 
//  PF_BLOCK_LNAE_ST        stlane[]; 
    
}PF_MAP_BLOCK_INFO_ST;


GLOBAL PF_MAP_BLOCK_INFO_ST**  pastMapBlock;


GLOBAL U32     gu32Block_w;
GLOBAL U32     gu32Block_h;
GLOBAL U32     gu32BlockCount_x;
GLOBAL U32     gu32BlockCount_y;
GLOBAL U32     gu32BlockValidCount;
GLOBAL U64     gu64BlockTotalSize;
GLOBAL U64     gu64TotalMemSize;


GLOBAL DOUBLE  gdbUtmOffset_x;
GLOBAL DOUBLE  gdbUtmOffset_y;


GLOBAL PF_MAP_UTM_XYZ_ST*            pstMapUtmXYZ;
GLOBAL S32     gs32MapUtmXYZCount;
GLOBAL S32     gs32MapUtmX_Y;


#define LINE_INFO_HEAD_LEN       sizeof(PF_MAP_LINE_INFO_ST)


#define BLOCK_STATE_INVALID      -1
#define BLOCK_STATE_UNKNOWN       0
#define BLOCK_STATE_NORMAL        1
#define BLOCK_STATE_ALL           2


#define PF_MAP_ERR               -2
#define PF_MAP_NO_ELEMENT        -1 
#define PF_MAP_SUCCESS            0

#define PF_MAP_PARKING_NO         0
#define PF_MAP_PARKING_YES        1

#define PF_MAP_PAVEMENT_NO        0
#define PF_MAP_PAVEMENT_YES       1


#define MAP_ROAD_UP_SIDE          1      // 1：里程桩位于高速公路上行（起点至终点）侧，道路分隔线右侧
#define MAP_ROAD_DOWN_SIDE        2      // 2：里程桩位于高速公路下行（终点至起点）侧，道路分隔线左侧



#define PF_MAP_CONNECTION         1
#define PF_MAP_LANE               2

#define ON_LINE                   0
#define LEFT_SIDE                 1
#define RIGHT_SIDE                2



#define DB_OFFSET                 0.00001
#define ONLINE_OFFSET             0.01


#define PAI                       3.1415926


#define INVALID_VALUE            -5 
#define INVALID_ANGLE             1111 



/**********************************************************************************************
 * @function      pf_map_utmZ_file_read
 * @brief         read  map height file  
 * @input         char*    map height file (include path)
 * @output        void
 * @return        S32      PF_MAP_ERR: read fail   
 *                         PF_MAP_SUCCESS: read success
 *********************************************************************************************/
GLOBAL S32 pf_map_utmZ_file_read(char* filename);



/**********************************************************************************************
 * @function      pf_map_block_file_read
 * @brief         read  .mapblock file  
 * @input         char*    .mapblock file name (include path)
 * @output        void
 * @return        S32      PF_MAP_ERR: read fail   
 *                         PF_MAP_SUCCESS: read success
 *********************************************************************************************/
GLOBAL S32 pf_map_block_file_read(char* mapfilename);



/**********************************************************************************************
 * @function      pf_map_block_get_laneID
 * @brief         get laneID or connectionID   
 * @input         DOUBLE utm_x       UTM coordinates x 
 * @input         DOUBLE utm_y       UTM coordinates y 
 * @output        S32 laneID         -1: invalid      
 * @output        S32 connectionID   -1: invalid    
 * @return        S32                PF_MAP_ERR: get laneID of connectionID fail  
 *                                   PF_MAP_CONNECTION: connectionID is valid
 *                                   PF_MAP_LANE: laneID is valid
 *                                    
 *********************************************************************************************/

GLOBAL S32 pf_map_block_get_laneID(DOUBLE utm_x, DOUBLE utm_y, S32* laneID, S32* connectionID);





/**********************************************************************************************
 * @function      pf_map_get_ID_by_utm
 * @brief         get laneID or connectionID 
 *
 * @input         DOUBLE  utm_x      UTM coordinates x 
 * @input         DOUBLE  utm_y      UTM coordinates y 
 *
 * @output        S32     type       MAP_LANE   or  MAP_CONNECTION  or  MAP_NO_RUN_ZONE
 * @output        S32     s32ID      laneID     or  connectionID    or  noRunZoneID
 * @output        S32     index      lane index or  connection index or noRunZone index
 * @output        DOUBLE  dbtheta    Driving direction  (radian) (-3.14159 ~ 3.14159)
 * @output        S32     laneNumber lane number in the road 
 *
 * @return        S32                PF_MAP_ERR:     get laneID or connectionID fail  
 *                                   PF_MAP_SUCCESS: get laneID or connectionID success 
 *********************************************************************************************/

GLOBAL S32 pf_map_get_ID_by_utm(DOUBLE utm_x, DOUBLE utm_y, S32* type, S32* s32ID, S32* index, DOUBLE* dbtheta, S32* laneNumber);





/**********************************************************************************************
 * @function      pf_map_get_parking_state_by_utm
 * @brief         get parking state  
 *
 * @input         DOUBLE  utm_x      UTM coordinates x 
 * @input         DOUBLE  utm_y      UTM coordinates y 
 *
 * @return        S32                PF_MAP_ERR:         get parking state fail  
 *                                   PF_MAP_PARKING_NO:  NO Parking 
 *                                   PF_MAP_PARKING_YES: Parking allowed
 *********************************************************************************************/

GLOBAL S32 pf_map_get_parking_state_by_utm(DOUBLE utm_x, DOUBLE utm_y);





/**********************************************************************************************
 * @function      pf_map_get_drivingDirection_by_utm
 * @brief         get drivingDirection in connection
 *
 * @input         DOUBLE  utm_x         UTM coordinates x 
 * @input         DOUBLE  utm_y         UTM coordinates y 
 * @input         S32     connection_i  connection index  
 *
 * @output        DOUBLE  dbtheta       Driving direction  (radian) (-3.14159 ~ 3.14159)
 *
 * @return        S32                   PF_MAP_ERR:     get drivingDirection fail  
 *                                      PF_MAP_SUCCESS: get drivingDirection success 
 *********************************************************************************************/

GLOBAL S32 pf_map_get_drivingDirection_by_utm(DOUBLE utm_x, DOUBLE utm_y, S32 connection_i,DOUBLE* dbtheta);





/**********************************************************************************************
 * @function      pf_map_check_pavement_by_utm
 * @brief         check whether the UTM position is in the pavement 
 *
 * @input         DOUBLE  utm_x         UTM coordinates x 
 * @input         DOUBLE  utm_y         UTM coordinates y 
 * @input         DOUBLE  distance      pavement edge extension distance 
 *
 * @output        S32     pavement_i    pavement index     -1: the UTM pos is not in pavement
 * @output        S32     section_i     the index of the Section witch has the pavementTrafficLight   
 *                                      -1: ivnalid, the pavement has no traficlight info 
 *
 * @return        S32                   PF_MAP_ERR:          check pavement fail  
 *                                      PF_MAP_PAVEMENT_NO:  not in pavement
 *                                      PF_MAP_PAVEMENT_YES: in pavement
 *********************************************************************************************/

GLOBAL S32 pf_map_check_pavement_by_utm(DOUBLE utm_x, DOUBLE utm_y, DOUBLE distance, S32* pavement_i, S32* section_i);






/**********************************************************************************************
 * @function      pf_map_get_connectionInfo_by_index
 * @brief         get connectionInfo  
 *
 * @input         S32     connection_i     connection index  
 *
 * @output        S32     connectionType   PF_MAP_CONNECTION_TYPE_E   
 * @output        S32     link_i           the first link index in connection   
 * @output        S32     linkCount        the link count in connection   
 * @output        DOUBLE  dbtheta          Driving direction  (radian) (-3.14159 ~ 3.14159)
 *
 * @return        S32                      PF_MAP_ERR:      get connectionInfo fail  
 *                                         PF_MAP_SUCCESS:  get connectionInfo success
 *********************************************************************************************/

GLOBAL S32 pf_map_get_connectionInfo_by_index(S32 connection_i,S32* connectionType,
                                                S32* link_i,S32* linkCount, DOUBLE* dbtheta);



/**********************************************************************************************
 * @function      pf_map_get_linkInfo_by_index
 * @brief         get linkInfo  
 * @input         S32 link_i             link index  
 * @output        S32 fromSection_i      fromSection index      -1:no fromSection
 * @output        S32 toSection_i        toSection index        -1:no toSection
 * @output        S32 linkType           PF_MAP_LINK_TYPE_E   
 * @return        S32                    PF_MAP_ERR:      get linkInfo fail  
 *                                       PF_MAP_SUCCESS:  get linkInfo success
 *********************************************************************************************/

GLOBAL S32 pf_map_get_linkInfo_by_index(S32 link_i,S32* fromSection_i,S32* toSection_i,S32* linkType);






/**********************************************************************************************
 * @function      pf_map_get_sectionInfo_by_index
 * @brief         get sectionInfo  
 *
 * @input         S32     type               MAP_LANE   or MAP_SECTION 
 * @input         S32     index              lane index or section index 
 *
 * @output        S32     section_i          section index   
 * @output        S32     sectionType        PF_MAP_ROAD_TYPE_E   
 * @output        S32     fromConnection_i   fromConnection index     -1:no fromConnection
 * @output        S32     toConnection_i     toConnection index       -1:no toConnection
 * @output        S32     trafficLight_i     the first trafficLight index in section  
 * @output        S32     trafficLightCount  the trafficLight count in section  
 * @output        DOUBLE  dbtheta            Driving direction  (radian) (-3.14159 ~ 3.14159)
 *
 * @return        S32                     PF_MAP_ERR:      get sectionInfo fail  
 *                                        PF_MAP_SUCCESS:  get sectionInfo success
 *********************************************************************************************/

GLOBAL S32 pf_map_get_sectionInfo_by_index(S32 type,S32 index,S32* section_i,S32* sectionType,S32* fromConnection_i,
                                                    S32* toConnection_i,S32* trafficLight_i,S32* trafficLightCount,DOUBLE* dbtheta);





/**********************************************************************************************
 * @function      pf_map_get_section_lane_lnfo_by_index
 * @brief         get section lane info  
 *
 * @input         S32     type               MAP_LANE   or MAP_SECTION 
 * @input         S32     index              lane index or section index 
 *
 * @output        S32     lane_count         lane count in the section    
 * @output        S32     first_lane_i       the first lane index in the section   
 * @output        S32     first_lane_num     the first lane number in the section   
 *
 * @return        S32                     PF_MAP_ERR:      get section lane info fail  
 *                                        PF_MAP_SUCCESS:  get section lane info success
 *********************************************************************************************/

GLOBAL S32 pf_map_get_section_lane_lnfo_by_index(S32 type,S32 index,S32* lane_count,S32* first_lane_i,S32* first_lane_num);






/**********************************************************************************************
 * @function      pf_map_get_trafficLightInfo_by_index
 * @brief         get trafficLightInfo  
 *
 * @input         S32     trafficLight_i   trafficLight index  
 *
 * @output        S32     trafficLightID   trafficLightID  
 * @output        DOUBLE  pos_x            trafficLight pos UTM_x  
 * @output        DOUBLE  pos_y            trafficLight pos UTM_y  
 *
 * @return        S32                      PF_MAP_ERR:      get trafficLightInfo fail  
 *                                         PF_MAP_SUCCESS:  get trafficLightInfo success
 *********************************************************************************************/

GLOBAL S32 pf_map_get_trafficLightInfo_by_index(S32 trafficLight_i,S32* trafficLightID,DOUBLE* pos_x,DOUBLE* pos_y);






/**********************************************************************************************
 * @function      pf_map_get_laneInfo_by_index
 * @brief         get laneInfo   
 *
 * @input         S32     lane_i     lane index 
 *
 * @output        S32     speedMax   lane speed limits max   (km/h)
 * @output        S32     speedMin   lane speed limits min   (km/h)
 * @output        S32     laneType   PF_MAP_LANE_TYPE_E   
 * @output        S32     turnType   PF_MAP_LANE_TURN_TYPE_E   
 * @output        DOUBLE  dbtheta    Driving direction  (radian) (-3.14159 ~ 3.14159)
 * @output        S32     section_i  section index   
 *
 * @return        S32                PF_MAP_ERR: get laneInfo fail  
 *                                   PF_MAP_SUCCESS:  get laneInfo success
 *********************************************************************************************/

GLOBAL S32 pf_map_get_laneInfo_by_index(S32 lane_i,S32* speedMax, S32* speedMin, 
                                                S32* laneType, S32* turnType, DOUBLE* dbtheta, S32* section_i);





/**********************************************************************************************
 * @function      pf_map_get_laneMarkerDistance_by_utm
 * @brief         get the distance form  pos(utm_x,utm_y) to leftLanemarker and rightLanemarker    
 *
 * @input         DOUBLE  utm_x      UTM coordinates x 
 * @input         DOUBLE  utm_y      UTM coordinates y 
 * @input         S32     lane_i     lane index      -1: no lane index  
 *
 * @output        DOUBLE  dis_left   the distance form (utm_x,utm_y) to leftLanemarker    note:if(dis_left<0) then dis_left is invalid
 * @output        DOUBLE  dis_right  the distance form (utm_x,utm_y) to rightLanemarker   note:if(dis_right<0) then dis_right is invalid
 *
 * @return        S32                PF_MAP_ERR: get laneMarker distance fail  
 *                                   PF_MAP_SUCCESS:  get laneMarker distance success
 *********************************************************************************************/

GLOBAL S32 pf_map_get_laneMarkerDistance_by_utm(DOUBLE utm_x, DOUBLE utm_y,S32 lane_i,DOUBLE* dis_left, DOUBLE* dis_right);





/**********************************************************************************************
 * @function      pf_map_get_laneEntrance_by_laneIndex
 * @brief         get UTM coordinates of lane entrance  
 *
 * @input         S32     lane_i      lane index    
 *
 * @output        DOUBLE  left_utmx   utm_x of the starting point of leftLanemarker
 * @output        DOUBLE  left_utmy   utm_y of the starting point of leftLanemarker
 * @output        DOUBLE  right_utmx  utm_x of the starting point of rightLanemarker
 * @output        DOUBLE  right_utmy  utm_y of the starting point of rightLanemarker
 *
 * @return        S32                 PF_MAP_ERR: get laneEntrance fail  
 *                                    PF_MAP_SUCCESS:  get laneEntrance success
 *********************************************************************************************/

GLOBAL S32 pf_map_get_laneEntrance_by_laneIndex(S32 lane_i,DOUBLE* left_utmx, DOUBLE* left_utmy,
                                                                          DOUBLE* right_utmx, DOUBLE* right_utmy);




/**********************************************************************************************
 * @function      pf_map_get_laneExport_by_laneIndex
 * @brief         get UTM coordinates of lane export  
 *
 * @input         S32     lane_i      lane index    
 *
 * @output        DOUBLE  left_utmx   utm_x of the end of leftLanemarker
 * @output        DOUBLE  left_utmy   utm_y of the end of leftLanemarker
 * @output        DOUBLE  right_utmx  utm_x of the end of rightLanemarker
 * @output        DOUBLE  right_utmy  utm_y of the end of rightLanemarker
 *
 * @return        S32                 PF_MAP_ERR: get laneEntrance fail  
 *                                    PF_MAP_SUCCESS:  get laneEntrance success
 *********************************************************************************************/

GLOBAL S32 pf_map_get_laneExport_by_laneIndex(S32 lane_i,DOUBLE* left_utmx, DOUBLE* left_utmy,
                                                                          DOUBLE* right_utmx, DOUBLE* right_utmy);




/**********************************************************************************************
 * @function      pf_map_get_laneMarkerType_by_laneIndex
 * @brief         get leftlaneMarkerType and rightlaneMarkerType by lane index    
 *
 * @input         S32     lane_i                   lane index 
 *
 * @output        S32     leftLaneMarkerType       PF_MAP_LINE_TYPE_E
 * @output        S32     rightLaneMarkerType      PF_MAP_LINE_TYPE_E
 *
 * @return        S32                              PF_MAP_ERR: get LaneMarkerType fail  
 *                                                 PF_MAP_SUCCESS:  get LaneMarkerType success
 *********************************************************************************************/

GLOBAL S32 pf_map_get_laneMarkerType_by_laneIndex(S32 lane_i,S32* leftLaneMarkerType, S32* rightLaneMarkerType);






/**********************************************************************************************
 * @function      pf_map_get_preSection_by_index
 * @brief         get preSection   
 *
 * @input         S32 type         MAP_LANE   or  MAP_SECTION    or  MAP_CONNECTION
 * @input         S32 index        lane index or  section index  or  connection index
 * @input         S32 linkType     PF_MAP_LINK_TYPE_E
 *
 * @return        S32              PF_MAP_ERR:        get preSection index fail 
 *                                 PF_MAP_NO_ELEMENT: no preSection
 *                                 else:              preSection index   
 *********************************************************************************************/

GLOBAL S32 pf_map_get_preSection_by_index(S32 type, S32 index, S32 linkType);




/**********************************************************************************************
 * @function      pf_map_get_nextSection_by_index
 * @brief         get nextSection   
 *
 * @input         S32 type         MAP_LANE   or  MAP_SECTION    or  MAP_CONNECTION
 * @input         S32 index        lane index or  section index  or  connection index
 * @input         S32 linkType     PF_MAP_LINK_TYPE_E
 *
 * @return        S32              PF_MAP_ERR:        get nextSection index fail  
 *                                 PF_MAP_NO_ELEMENT: no nextSection
 *                                 else:              nextSection index   
 *********************************************************************************************/

GLOBAL S32 pf_map_get_nextSection_by_index(S32 type, S32 index, S32 linkType);




/**********************************************************************************************
 * @function      pf_map_get_preLane_by_index
 * @brief         get preLane   
 *
 * @input         S32 index        lane index 
 *
 * @return        S32              PF_MAP_ERR:        get preLane index fail  
 *                                 PF_MAP_NO_ELEMENT: no preLane
 *                                 else:              preLane index   
 *********************************************************************************************/

GLOBAL S32 pf_map_get_preLane_by_index(S32 index);



/**********************************************************************************************
 * @function      pf_map_get_entrance_distance_by_utm
 * @brief         get the distance form entrance  
 *
 * @input         DOUBLE  utm_x      UTM coordinates x 
 * @input         DOUBLE  utm_y      UTM coordinates y 
 * @input         S32     road_i     road index  
 *
 * @output        DOUBLE  distance   distance form entrance
 * @output        S32     roadside   MAP_ROAD_UP_SIDE or MAP_ROAD_DOWN_SIDE
 *
 * @return        S32                PF_MAP_ERR:     get the distance fail  
 *                                   PF_MAP_SUCCESS: get the distance success 
 *********************************************************************************************/

GLOBAL S32 pf_map_get_entrance_distance_by_utm(DOUBLE utm_x, DOUBLE utm_y, S32 road_i, DOUBLE* distance, S32* roadside);






/**********************************************************************************************
 * @function      pf_map_get_HighWayMileage_by_utm
 * @brief         get the mileage form highway entrance  
 *
 * @input         DOUBLE  utm_x      UTM coordinates x 
 * @input         DOUBLE  utm_y      UTM coordinates y 
 *
 * @output        S32     integer_km    
 * @output        S32     remainder_m    
 * @output        S32     roadside   MAP_ROAD_UP_SIDE or MAP_ROAD_DOWN_SIDE
 *
 * @return        S32                PF_MAP_ERR:     get the mileage fail  
 *                                   PF_MAP_SUCCESS: get the mileage success 
 *********************************************************************************************/

GLOBAL S32 pf_map_get_HighWayMileage_by_utm(DOUBLE utm_x, DOUBLE utm_y, S32* integer_km, S32* remainder_m, S32* roadside);







/**********************************************************************************************
 * @function      pf_map_get_roadName_by_utm
 * @brief         get road name 
 *
 * @input         DOUBLE  utm_x      UTM coordinates x 
 * @input         DOUBLE  utm_y      UTM coordinates y 
 *
 * @output        CHAR*   roadName   road name    
 *
 * @return        S32                PF_MAP_ERR:     get road name  fail  
 *                                   PF_MAP_SUCCESS: get road name  success 
 *********************************************************************************************/

GLOBAL S32 pf_map_get_roadName_by_utm(DOUBLE utm_x, DOUBLE utm_y, CHAR* roadName);



/**********************************************************************************************
 * @function      pf_map_get_road_index_by_utm
 * @brief         get road index 
 *
 * @input         DOUBLE  utm_x      UTM coordinates x 
 * @input         DOUBLE  utm_y      UTM coordinates y 
 *
 * @return        S32                PF_MAP_ERR:     get road index  fail  
 *                                   else: road index 
 *********************************************************************************************/

GLOBAL S32 pf_map_get_road_index_by_utm(DOUBLE utm_x, DOUBLE utm_y);



/**********************************************************************************************
 * @function      pf_map_get_roadInfo_by_index
 * @brief         get roadInfo  
 *
 * @input         S32     type            MAP_LANE   or MAP_SECTION    or MAP_ROAD   or MAP_CONNECTION   or  MAP_NO_RUN_ZONE
 * @input         S32     index           lane index or section index  or road index or connection index or  noRunZoneID
 *
 * @output        S32     road_i          road index   
 * @output        S32     roadType        PF_MAP_ROAD_TYPE_E   
 * @output        S32     roadLength      road length   Unit：m  
 * @output        CHAR*   roadName        road name
 *
 * @return        S32                     PF_MAP_ERR:      get roadInfo fail  
 *                                        PF_MAP_SUCCESS:  get roadInfo success
 *********************************************************************************************/

GLOBAL S32 pf_map_get_roadInfo_by_index(S32 type,S32 index,S32* road_i,S32* roadType,S32* roadLength,CHAR* roadName);






/**********************************************************************************************
 * @function      pf_map_get_index_by_id
 * @brief         get index   
 *
 * @input         S32 type         form  MAP_ROAD   to  MAP_ROADEDGE
 * @input         S32 id           id 
 *
 * @return        S32              PF_MAP_ERR: get index fail  
 *                                 else:       index
 *********************************************************************************************/

GLOBAL S32 pf_map_get_index_by_id(S32 type, S32 id);





/**********************************************************************************************
 * @function      pf_map_get_id_by_index
 * @brief         get id   
 *
 * @input         S32 type         form  MAP_ROAD   to  MAP_ROADEDGE
 * @input         S32 index        index 
 *
 * @return        S32              PF_MAP_ERR: get id fail  
 *                                 else:       id
 *********************************************************************************************/

GLOBAL S32 pf_map_get_id_by_index(S32 type, S32 index);




/**********************************************************************************************
 * @function      pf_map_get_utm_z_by_xy
 * @brief         get utm_z by utm_x and utm_y   
 *
 * @input         DOUBLE  utm_x      UTM coordinates x 
 * @input         DOUBLE  utm_y      UTM coordinates y 
 *
 * @return        DOUBLE             PF_MAP_ERR: get utm_z fail  
 *                                   else:       utm_z
 *********************************************************************************************/

GLOBAL DOUBLE pf_map_get_utm_z_by_xy(DOUBLE utm_x, DOUBLE utm_y);






/**********************************************************************************************
 * @function      pf_map_get_trafficLight_count
 * @brief         get trafficLight count
 *
 * @input         void
 *
 * @return        S32    trafficLight count  
 *                                   
 *********************************************************************************************/

GLOBAL S32 pf_map_get_trafficLight_count(void);








/**********************************************************************************************
 * @function      pf_map_get_lampGroup_count_by_index
 * @brief         get lamp group count   
 *
 * @input         S32  trafficLight_i        trafficLight index 
 *
 * @return        S32  lamp group count
 *                                                 
 *********************************************************************************************/

GLOBAL S32 pf_map_get_lampGroup_count_by_index(S32  trafficLight_i);





typedef struct
{
    S8       index;                  // 灯组在信号灯中的序号         
    S8       light_type;             // 灯组信号灯类型                  PF_MAP_TRAFFIC_LIGHT_TYPE_E
    S8       turn_type;              // 灯组信号灯控制车流向类型        PF_MAP_TRAFFIC_LIGHT_TURN_TYPE_E
    S8       bind_lane_count;        // 灯组绑定车道的数量
    S16      bind_lane_i[10];        // 灯组绑定车道编号列表
}PF_API_LAMP_GROUP_ST;

/**********************************************************************************************
 * @function      pf_map_get_lampGroup_info_by_index
 * @brief         get lamp group info   
 *
 * @input         S32  trafficLight_i      trafficLight index 
 * @input         S32  lampGroup_i         lampGroup index         注释: 从0开始 
 *
 * @output        PF_API_LAMP_GROUP_ST*    the struct of lamp group info 
 *                                                 
 * @return        S32                      PF_MAP_ERR: fail  
 *                                         else:       success
 *********************************************************************************************/

GLOBAL S32 pf_map_get_lampGroup_info_by_index(S32 trafficLight_i,S32 lampGroup_i,PF_API_LAMP_GROUP_ST* pstLampGroupInfo);




/**********************************************************************************************
 * @function      pf_map_get_lane_end_by_index
 * @brief         get the end of lane  
 *
 * @input         S32     lane_i         lane index 
 *
 * @output        DOUBLE  left_utmx      the end utmx of leftlanemarker 
 * @output        DOUBLE  left_utmy      the end utmy of leftlanemarker 
 * @output        DOUBLE  right_utmx     the end utmx of rightlanemarker 
 * @output        DOUBLE  right_utmy     the end utmy of rightlanemarker
 *
 * @return        S32                    PF_MAP_ERR:     fail  
 *                                       PF_MAP_SUCCESS: success
 *********************************************************************************************/

GLOBAL S32 pf_map_get_lane_end_by_index(S32 lane_i,DOUBLE* left_utmx, DOUBLE* left_utmy, 
                                                DOUBLE* right_utmx, DOUBLE* right_utmy);






/**********************************************************************************************
 * @function      pf_map_get_connection_count
 * @brief         get connection count
 *
 * @input         void
 *
 * @return        S32    connection count  
 *                                   
 *********************************************************************************************/

GLOBAL S32 pf_map_get_connection_count(void);







/**********************************************************************************************
 * @function      pf_map_get_connection_trafficLight_info
 * @brief         get the trafficLight info of connection
 *
 * @input         S32    connection_i          connection index 
 *
 * @output        S32*   ptrafficLightList     trafficLight index list   注释：需要S32数组指针，以便输出多个信号灯 index
 *
 * @return        S32    trafficLight count    PF_MAP_ERR:     fail  
 *                                             0: no trafficLight
 *                                             >0: trafficLight count
 *                                   
 *********************************************************************************************/

GLOBAL S32 pf_map_get_connection_trafficLight_info(S32 connection_i, S32* ptrafficLightList);









GLOBAL S32 pf_map_check_line_side(DOUBLE pos_x,DOUBLE pos_y,PF_MAP_NODE_LINE_ST* pnode0,S32 lineID,S32 nodeCount,
                                        S16 line_i,S16 linecount, DOUBLE* pdbinstance, S32 ispolygon);


GLOBAL S32 pf_map_check_utm_in_polygon(DOUBLE utm_x,DOUBLE utm_y, S32 type ,S32 index, DOUBLE* distance);



/**********************************************************************************************
 * @function      pf_map_get_centreNodeList
 * @brief         get centreNodeList of event area
 *
 * @input         DOUBLE  p0_x                     UTM coordinates x 
 * @input         DOUBLE  p0_y                     UTM coordinates y 
 * @input         DOUBLE  maxlen                   area len
 * @input         S32     minLaneNum               the minLaneNum in area 
 * @input         S32     maxLaneNum               the maxLaneNum in area 
 
 * @output        PF_MAP_NODE_STRUCT* pNodeList    centreNodeList (UTM coordinates)
 * @output        DOUBLE* width                    the distance form centreNodeList to left/right side
 *
 * @return        S32                              PF_MAP_ERR:     fail  
 *                                                 >0: centreNodeList node count
 *                                   
 *********************************************************************************************/

GLOBAL S32 pf_map_get_centreNodeList(DOUBLE p0_x,DOUBLE p0_y,DOUBLE maxlen,S32 minLaneNum, S32 maxLaneNum,PF_MAP_NODE_STRUCT* pNodeList,DOUBLE* width);

#endif

