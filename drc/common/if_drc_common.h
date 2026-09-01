/************************************************************
  Copyright (C), Innofidei Inc  2020
  FileName: if_drc_common.h
  Author: liyafei
  Version : v0.1
  Date:2020-5-26
  Description:
  History:
  <author>       <time>       <version >   <desc>
  liyafei      2020-5-26        v0.1        build
***********************************************************/
#ifndef IF_DRC_COMMON_H
#define IF_DRC_COMMON_H

#include <Eigen/Dense>
using Eigen::MatrixXd;
using Eigen::VectorXd;


#include "option.h"
#include "event.h"
#include <unordered_map>
#include <list>
#include "if_drc_drsu.pb.h"
#include "drc_event.pb.h"
#include <string>
using std::unordered_map;

typedef std::unordered_map<U32, std::list<U64>> GLOBAL_Y_MAP;
typedef std::unordered_map<U32, GLOBAL_Y_MAP> GlobalMap;


/******************** common start********************/
#define DRC_MSG_HEADER_LEN_BYTE (DRC_MSG_HEADER_LEN_U32 * 4)
#define DRC_MSG_HEADER_LEN_U32 4  //source id ,desitination id,msg length,msg type
#define DRC_MSG_HEADER_SOURCE_IND 0
#define DRC_MSG_HEADER_DESIT_IND 1
#define DRC_MSG_HEADER_MSG_LEN_IND 2
#define DRC_MSG_HEADER_MSG_TYPE_IND 3

//设备ID定义
#define RC_DEVICE_ID      0x1346600
#define PROCE_CLUSTER_ID  0x1346610
#define RH_CLUSTER_ID     0x1346620
#define EVENT_CLUSTER_ID  0x1346630
#define VT_CLUSTER_ID     0x1346640
#define ACU_CLUSTER_ID    0x1346650
#define EHMS_CLUSTER_ID   0x1346660

#define DRC_VERSION_NUMBER_LEN 64
#define DRC_ALARM_DISCRIPTION_LEN 256
#define DRC_ALARM_NUMBER_MAX 96
#define MAX_DRSU_SUB_DEV_NUM 6

#define DRC_CRM_MSG_POWER_7 10000000
#define DRC_FUSION_PREDICTION_CONFIG_SIZE   64
#define DRC_OVERLAP_CONFIG_SIZE 2

typedef enum
{
    DRC_ALARM_LEVEL_ERROR = 0,
    DRC_ALARM_LEVEL_FATAL = 1,
}DRC_ALARM_LEVEL_E;

typedef enum
{
    DRC_ALARM_TRIG_EVENT = 0,
    DRC_ALARM_TRIG_PERIODIC = 1,
}DRC_ALARM_TRIG_TYPE_E;

typedef struct
{
    DOUBLE dbx = 0;  //in meters.
    DOUBLE dby = 0;  // in meters.
    DOUBLE dbz = 0;  // in meters.
}drc_vector3d;

typedef struct
{
    S32 ulx;  //in meters.
    S32 uly;  // in meters.
    S32 ulz;  // in meters.
    U32 ulPadding;
}drc_vector3i;

typedef struct
{
    U32 ulGlobalDrsuId;
    U32 ulResult; //0:success
}drc_fail_cfg_drsu_info;

typedef struct
{
    U32 ulId;
    U32 ulResult; //0:success
}drc_fail_device_info;

typedef struct
{
    U32 ulalarm_sn;
    U32 ulalarm_level;  //DRC_ALARM_LEVEL_E
    U32 ulmodeIdFlag; //0: ulmodeId invalid
    U32 ulmodeId;
    U32 ulPadding;
    U32 aucalarm_discriptionLen;
    U8 aucalarm_discription[DRC_ALARM_DISCRIPTION_LEN];
}drc_alarm_info;

typedef struct
{
    U32 ulalarm_sn;
    U32 ulmodeIdFlag; //0: ulmodeId invalid
    U32 ulmodeId;
    U32 ulPadding;
}drc_alarm_sn_info;

typedef struct
{
	DOUBLE tmpK;
	DOUBLE tmpB;
}JAM_SCORE_COEFF_STRU;

#define FLUENT_SCORE_LOWER_LIMIT		0.0
#define FLUENT_SCORE_UPPER_LIMIT		20.0
#define BASIC_FLUENT_SCORE_LOWER_LIMIT	20.0
#define BASIC_FLUENT_SCORE_UPPER_LIMIT	40.0
#define LIGHT_JAM_SCORE_LOWER_LIMIT		40.0
#define LIGHT_JAM_SCORE_UPPER_LIMIT		60.0
#define MODERATE_JAM_SCORE_LOWER_LIMIT	60.0
#define MODERATE_JAM_SCORE_UPPER_LIMIT	80.0
#define SEVERE_JAM_SCORE_LOWER_LIMIT	80.0
#define SEVERE_JAM_SCORE_UPPER_LIMIT	100.0
typedef struct
{
	U32 ulLaneLimitSpeed;// km/h
	U32 ulFluentSpeed;// cm/s
	U32 ulBasicFluentSpeed;// cm/s
	U32 ulLightJamSpeed;// cm/s
	U32 ulModerateJamSpeed;// cm/s
	JAM_SCORE_COEFF_STRU stFluentCoeff;
	JAM_SCORE_COEFF_STRU stBasicFluentCoeff;
	JAM_SCORE_COEFF_STRU stLightJamCoeff;
	JAM_SCORE_COEFF_STRU stModerateJamCoeff;
	JAM_SCORE_COEFF_STRU stSevereJamCoeff;
}JAM_LEVEL_THRES_INFO_STRU;


/******************** common end********************/

/******************** drsu related start********************/
#define DRC_DRSU_NUM_MAX 100
#define DRC_RCER_DRSU_NUM_MAX 8
#define MIN_DRSU_SESSION_INDEX 0
#define DRSU_GLOBAL_ID_BEGAIN 16384
#define DRSU_GLOBAL_ID_END 32767

#define DRC_IP_ADDRESS_LEN 32
#define DRSU_AUTO_OPEN_BOX_ALARM_DOWN  0
#define DRSU_AUTO_OPEN_BOX_ALARM_UP    480

#define BOOTUP_DRC_DRSU_ETH_NAME  CONFIG_BOOTUP_PATH"drc_drsu_eth_name"
#define BOOTUP_DRC_CRM_ETH_NAME  CONFIG_BOOTUP_PATH"drc_crm_eth_name"
#define BOOTUP_DRC_ACU_ETH_NAME  CONFIG_BOOTUP_PATH"drc_acu_eth_name"

#define DRC_SLAVE_TRACKID_NUM_MAX 10
#define DRC_MASTER_DRSU_NUM_MAX 6
#define DRC_OBS_DRSUID_TRACKID_MAX_NUM 20
#define DRC_LANE_NUM_MAX 16
#define DRC_HD_LANE_NUM_MAX 16
#define DRC_AUC_LANE_NUM_MAX 16
#define DRC_HD_CONNECTION_NUM_MAX 16
#define DRC_MAC_ADDRESS_LEN 6
#define DRC_DEVICE_MODE_LEN 128
#define DRC_DRSU_SUB_DEV_NUM_MAX 10
#define DRC_DRSU_OPERATION_TIME_LEN 12
#define DRC_BUSLANE_TIME_MAX 6 //Configure the maximum number of the bus lane time
#define DRC_JAM_LEVEL_INFO_MAX_NUM 6



#define RCER_WAIT_RCTP_RSP_MAX_TIME_MS 100


typedef struct
{
    DOUBLE dbx;  //East from the origin, in meters.
    DOUBLE dby;  //North from the origin, in meters.
    DOUBLE dbz;  //Up from the WGS-84 ellipsoid, in meters.
}drc_pointutm;

typedef enum
{
    DRC_OBSTACLE_CTR_OFF = 0,
    DRC_OBSTACLE_CTR_ON = 1,
}DRC_OBSTACLE_CTR_SWITCH_E;

typedef enum
{
    DRC_OBST_REC_ALG_BASIC = 0,
    DRC_OBST_REC_ALG_ADV = 1,
}DRC_OBST_REC_ALG_E;

typedef enum
{
    DRC_DRSU_SUBDEV_NULL = 0,
    DRC_DRSU_SUBDEV_LIDAR,
    DRC_DRSU_SUBDEV_CAMERA,
    DRC_DRSU_SUBDEV_TRAFFIC_LIGHT,
    DRC_DRSU_SUBDEV_GPS,
    DRC_DRSU_SUBDEV_BUZZER,
    DRC_DRSU_SUBDEV_RADIATOR_FAN,
}DRC_DRSU_SUBDEVICE_TYPE_E;

typedef enum
{
    DRC_DRSU_SUBDEV_WORKING_OFF = 0,   /* poweroff */
    DRC_DRSU_SUBDEV_WORKING_ON,        /* poweron and working normal */
    DRC_DRSU_SUBDEV_WORKING_ABNORMAL,  /* poweron but working abnormal  */
    DRC_DRSU_SUBDEV_WORKING_FRAME_RATE_ABNORMAL,  	/* poweron but frame rate abnormal  */
    DRC_DRSU_SUBDEV_WORKING_PAUSE,     /* poweron but working pause */
}DRC_DRSU_SUBDEV_WORKING_STATE_E;

typedef enum
{
    DRC_ALARM_REPORT_PERIOD_60S = 60,
    DRC_ALARM_REPORT_PERIOD_120S = 120,
    DRC_ALARM_REPORT_PERIOD_180S = 180
}DRC_DRSU_ALARM_REPORT_PERIOD_E;

typedef enum
{
    DRC_DRSU_DEVICE_POWER_NULL = 0,
    DRC_DRSU_DEVICE_POWER_ON ,
    DRC_DRSU_DEVICE_POWER_ALL_ON ,
    DRC_DRSU_DEVICE_POWER_OFF,
    DRC_DRSU_DEVICE_POWER_ALL_OFF,
}DRC_DRSU_DEVICE_POWER_TYPE_E;

typedef struct
{
    U32 uldeviceId;
    U32 ulWorkingStatus; //DRC_DRSU_SUBDEV_WORKING_STATE_E
}drc_drsu_subdevice_status;

typedef struct
{
    U32 ulDeviceIdFlag = 0;
    U32 ulDeviceId;
    U32 ulSubDeviceTypeFlag = 0;
    U32 ulSubDeviceType; //DRC_DRSU_SUBDEVICE_TYPE_E
    U32 ulDeviceModelFlag = 0;
    U32 ulDeviceVersionFlag = 0;
    U32 ulWorkingStatusFlag = 0;
    U32 ulWorkingStatus; //DRC_DRSU_SUBDEV_WORKING_STATE_E
    U8  aucDeviceModel[DRC_DEVICE_MODE_LEN];
    U8  aucDeviceVersion[DRC_VERSION_NUMBER_LEN];
}drc_drsu_subdevice_info;

typedef struct
{
    U32 ulPeriodOfDataRptFlag;
    U32 ulPeriodOfDataRpt;  //ms
    U32 ulPeriodOfAlarmRptFlag;
    U32 ulPeriodOfAlarmRpt;  //DRC_DRSU_ALARM_REPORT_PERIOD_E
    U32 ulRecAlgorithmFlag;
    U32 ulRecAlgorithm;  //DRC_OBST_REC_ALG_E
    U32 ulMapUpdateFlag;
    U32 ulPadding; //DRC use it as padding
}drc_drsu_cfg_struct;

typedef struct
{
    U32 ulGlobalDrsuId;
    U32 ulPadding; //DRC use it as padding
    drc_drsu_cfg_struct stDrsuDeviceCfg;
}drc_drsu_cfg_info;

typedef enum
{
    DRC_DEVFUNC_SWITCH_OFF = 0,
    DRC_DEVFUNC_SWITCH_ON = 1
}DRC_DEV_FUNC_SWITCH_E;

/******************** drsu related end********************/

/******************** acu start ********************/
#define DRC_ACU_NUMBER_MAX  100
#define DRC_NUMBER_PLATE_LEN 16
#define DRC_ENGINE_NUMBER_LEN 16
#define DRC_ACU_GLOBAL_ID_START 32768 //ACU ID:32768~65535(0x8000~0xFFFF),the higher 4 types reserved
#define DRC_ACU_GLOBAL_ID_END 65535

typedef enum
{
    DRC_LIGHT_COLOR_UNKNOWN,
    DRC_LIGHT_COLOR_RED,
    DRC_LIGHT_COLOR_GREEN,
    DRC_LIGHT_COLOR_YELLOW,
    DRC_LIGHT_COLOR_BLACK,
    DRC_LIGHT_COLOR_INIT = 99,
}DRC_LIGHT_COLOR_E;

/******************** acu end ********************/

/************************ smart expressway start***************************/
#define DEFAULTTHRESHOLD 1
#define DEFAULTMERGETHRESHOLD 20
#define EMERGENCY_VEHICLE_LANE 99
#define DRC_OCCUPIED_GRIDS_NUMBLER   256
#define DRC_MATRIX_XD_DATA_LEN 30
#define DRC_TRAFFIC_LIGHT_ID_LEN  256
#define DRC_ROAD_HEADER_MODULE_NAME_LEN  256
#define MAX_2D_DELAY_TIME 90


//0 is motor lane, 1 is non-motor lane, 2 emergency lane, 3 ramp, 4 bus lane
typedef enum
{
	MOTOR_LANE = 0,
	NON_MOTOR_LANE = 1,
	EMERGENCY_LANE = 2,
	RAMP_LANE = 3,
	BUS_LANE = 4,
}HDLANE_TYPE_E;




typedef enum  {
    PLATFORM_CTR_PTZ_UP = 1,
    PLATFORM_CTR_PTZ_DOWN,
    PLATFORM_CTR_PTZ_LEFT,
    PLATFORM_CTR_PTZ_RIGHT,
    PLATFORM_CTR_PTZ_ZOOM_ADD,
    PLATFORM_CTR_PTZ_ZOOM_DEC,
    PLATFORM_CTR_PTZ_FOCUS_ADD,
    PLATFORM_CTR_PTZ_FOCUS_DEC,
    PLATFORM_CTR_PTZ_PERSET_POINT_MOVE,
    PLATFORM_CTR_PTZ_PERSET_POINT_SET,
    PLATFORM_CTR_PTZ_PERSET_POINT_DEL,
    PLATFORM_CTR_EXTPTZ_FASTGOTO,
} CAMER_PLATFORM_CTRL_E;

typedef enum
{
    DRC_LIGHT_TYPE_UNKNOWN_TYPE,
    DRC_LIGHT_TYPE_CYCLE,
    DRC_LIGHT_TYPE_LEFT,
    DRC_LIGHT_TYPE_STRAIGHT,
    DRC_LIGHT_TYPE_RIGHT,
    DRC_LIGHT_TYPE_TURN,
    DRC_LIGHT_TYPE_TURNLEFT,
    DRC_LIGHT_TYPE_STRAIGHT_LEFT,
    DRC_LIGHT_TYPE_COUNT_DOWN,
}DRC_LIGHT_TYPE_E;

typedef enum
{
    DRC_HD_LANE_TYPE_NONE = 0,
    DRC_HD_LANE_TYPE_STRAIGHT = 1,
    DRC_HD_LANE_TYPE_LEFT = 2,
    DRC_HD_LANE_TYPE_STRAIGHT_AND_LEFT = 3,
    DRC_HD_LANE_TYPE_RIGHT = 4,
    DRC_HD_LANE_TYPE_STRAIGHT_AND_RIGHT = 5,
    DRC_HD_LANE_TYPE_TURN = 8,
}DRC_HD_LANE_TYPE_E;

typedef enum
{
    DRC_HD_CONNECTION_PAIR_LEFT = 0,
    DRC_HD_CONNECTION_PAIR_STRAIGHT = 1,
    DRC_HD_CONNECTION_PAIR_RIGHT = 2,
    DRC_HD_CONNECTION_PAIR_TRUN = 3,
}DRC_HD_CONNECTION_PAIR_TYPE_E;

typedef enum
{
    DRC_OBJECT_TYPE_UNKNOWN = 0,
    DRC_OBJECT_TYPE_UNKNOWN_MOVABLE = 1,
    DRC_OBJECT_TYPE_UNKNOWN_UNMOVABLE = 2,
    DRC_OBJECT_TYPE_PEDESTRIAN = 3,
    DRC_OBJECT_TYPE_BICYCLE = 4,
    DRC_OBJECT_TYPE_VEHICLE = 5,
    DRC_OBJECT_TYPE_CAR = 6,
    DRC_OBJECT_TYPE_TRUCK = 7,
    DRC_OBJECT_TYPE_BUS = 8,
    DRC_OBJECT_TYPE_TRICYCLE = 9,
    DRC_OBJECT_TYPE_BLOCK = 10,
    DRC_OBJECT_TYPE_CONE_BARREL = 11,
    DRC_OBJECT_TYPE_ABANDONED_BOX = 12,
    DRC_OBJECT_TYPE_ABANDONED_TYRE = 13,
    DRC_OBJECT_TYPE_NEW_PEDESTRIAN = 21,
    DRC_OBJECT_TYPE_NEW_OBJECT = 22,
    DRC_OBJECT_TYPE_NEW_VEHICLE = 23,
    DRC_OBJECT_TYPE_NEW_NONMOTOR = 24,
    DRC_OBJECT_TYPE_NEW_GOODS = 25,
    DRC_OBJECT_TYPE_NEW_SCENE = 26,
}DRC_OBJECT_TYPE_E;

typedef enum
{
	DRC_UNIVERSAL_TYPE_UNKNOWN = 0,
	DRC_UNIVERSAL_TYPE_MICRO_THING = 1,
	DRC_UNIVERSAL_TYPE_HUMAN_LIKE = 2,
	DRC_UNIVERSAL_TYPE_VEHICLE = 3,
}DRC_UNIVERSAL_TYPE_E;

typedef struct
{
    U32 ulVolume;
    U32 ulFreeSpace;
    U32 ulStatus;
    U32 ulPadding;
}DRSU_NET_DEV_DISK_STATE_STRU;


typedef struct
{
    U32 ulIceSwitch;
	U32 ulIceFlag;
    U32 ulWetSwitch;
	U32 ulWetFlag;
    U32 ulFogSwitch;
	U32 ulFogFlag;
}DRC_DRSU_AI_CFG_STRU;

typedef struct
{
    U32 ulX;
    U32 ulY;
}DRC_CAMERA_POINT_COORDINATE_STRU;

typedef struct
{
    DOUBLE xmin;
    DOUBLE ymin;
    DOUBLE xmax;
    DOUBLE ymax;
}drc_Box2d;

typedef struct
{
    DOUBLE dbDis_from_lane_center;
    DOUBLE dbdis_from_ego;
    U32 ulin_lane_id;
    typedef enum
    {
        BG = 0,
        CAR = 1,
        BUS = 2,
        TRUCK = 3,
        PERSON = 4,
        BICYCLE = 5,
        TRICYCLE = 6,
        BLOCK = 7,
        MOTOR = 8,
    }CAMERAOBJECTTYPE_E;

    U32 camera_object_type; //CAMERAOBJECTTYPE_E: default = BG
    drc_Box2d stimg_box;
    DOUBLE dbconfidence;
}drc_CameraSupplement;

typedef struct
{
    U32 ulaOccupied_grids[DRC_OCCUPIED_GRIDS_NUMBLER];
}drc_LidarSupplement;

typedef struct
{
    U32 is_background;  //default = false
    U32 ulPadding;
}drc_RadarSupplement;

typedef struct
{
    U32 ulDataLen;
    U32 ulPadding;
    DOUBLE adbData[DRC_MATRIX_XD_DATA_LEN];// Matrix data. The storage order of data is column major.
    S32 slRows;      // The row number of the matrix. default = 0
    S32 slCols;      // The col number of the matrix. default = 0
}drc_MatrixXd;


typedef struct
{
    U32 ulId;
    U32 stObj_type;   //DRC_OBJECT_TYPE_E
    DOUBLE dbTimestamp;
    drc_vector3d stCenter;   //UTM vector
    drc_MatrixXd stPosition_uncertainty;
    drc_vector3d stEgo_center;
    drc_vector3d stDirection;         // Direction of movement
    DOUBLE dbTheta;               //Deflection angle
    DOUBLE dblength;
    DOUBLE dbwidth;
    DOUBLE dbheight;
    DOUBLE dbdet_confidence;    //detection confidence
    DOUBLE dbType_prob;         //probility
    drc_vector3d stvelocity;        //Line speed
    drc_MatrixXd stVelocity_uncertainty; //
    DOUBLE dbangular_velocity;   //angular speed
    drc_vector3d stAcceleration;
    drc_MatrixXd stAcceleration_uncertainty;
    DOUBLE dbDetection_time;
    DOUBLE dbTracking_time;
    DOUBLE dbLatest_tracked_time;
    U32 ultrack_id;

    // Lane IDs
    U32 ulLaneNum;
    S32 aulLaneIDs[DRC_LANE_NUM_MAX];
	// HD Lane IDs
    U32 ulHDLaneNum;
    U32 aulHDLaneIDs[DRC_HD_LANE_NUM_MAX];
	// HD Connection IDs
    U32 ulHDConnectionNum;
    U32 aulHDConnectionIDs[DRC_HD_CONNECTION_NUM_MAX];

	U32 ulDrsuNum;
	U32 aulDrsuIDs[DRC_RCER_DRSU_NUM_MAX];

    //other
    U32 stSensorSource; //SENSOR_SOURCE_E
    U32 is_valid;   //default = true
    U32 is_on_hdmap;  //default = false
    U32 is_stationary;  //default = false
    drc_CameraSupplement stCameraSupplement;
    drc_LidarSupplement stLidarSupplement;
    drc_RadarSupplement stRadarSupplement;

    std::string strVehicleClass; // Vehicle type
    std::string strVehicleColor; // Vehicle Color
    std::string strVehicleBrand; // Vehicle Brand
    std::string strVehicleModel; // Vehicle Model
    std::string strVehicleStyles; // What year is the car, for example:2014,2015,2016
    std::string strPlateClass; // Plate type
    std::string strPlateColor; // Plate Color
    std::string strPlateNo; // Plate Number
    bool bCarDirection;
    U32 ulCarDirection; // Whether to photograph the front of the car
    bool bSpecialVehicleType;
    U32 ulSpecialVehicleType; // Special car
	U32 ulHaveOtherDataFlag = 0;//0代表没有其他数据信息，指车牌等，1代表有
	U32 ulHavePosDataFlag = 1;//0代表没有位置数据信息，1代表有

    S32 slSpeed; // 标量速度
    S32 slHeading; // 航向角 0.0125°
    U32 ulSpeedConfidence;
    U32 ulPosConfidence;

    //------------------------gf add 2022.06.06---------------------------
    std::string targetCameraIP;
    U32 targetCameraDis;
	//--------------------------------------------------------------------

}drc_dr_obstacle_stru;

typedef enum
{
	GENERIC_TYPE_UNKNOWN = 0,
	GENERIC_TYPE_OTHERS = 1,
	GENERIC_TYPE_HUMAN_LIKE = 2,
	GENERIC_TYPE_VEHICLE = 3,
    GENERIC_TYPE_OBJECT = 4,
    GENERIC_TYPE_NONMOTOR = 5,
    GENERIC_TYPE_GOODS = 6,
    GENERIC_TYPE_SCENE = 7,
	GENERIC_TYPE_MAX_SIZE = 8,
}GENERIC_TYPE_E;


typedef struct
{
	DOUBLE dbTheta;
	drc_dr_obstacle_stru stRawData;
    VectorXd vObvState; // 观测状态，4维向量，前两维是位置坐标，后两维是速度矢量 TODO 需要加代码设置这个值
	U32 ulFrame;//当前帧
	U32 ulSubFrame;//当前子帧
	U32 ulCheckState = 0;//0代表该观测值在当前帧，未找到关联数据。1代表找到
	U32 ulUndetNum = 0; //该观测值连续未检测到的帧数
	U32 ulIsNewOne = TRUE;//这个版本不用了
	U32 ulBeUsedBy = 0; //用于CSV输出
	U32 ulDetecNum = 0; //该观测值在系统中检测到的累计值
	U32 ulHaveBeenUpdated = 0;//0代表这个观测值在当前帧，没有新的数据更新，即数据不可用。1代表有更新，即数据可用
	U32 ulDrsuId;//该观测值所属的DRSU
    U32 ulSubNodeId;
    U64 llCreateTime; //时间戳
}FUSION_RAW_DATA_STRU;



typedef struct
{
    U32 ulError_code; //ACU_ERROR_COdE_E,default = OK
    U32 ulPadding;
    U8 aucDscription[DRC_ALARM_DISCRIPTION_LEN];
}drc_status_pb;

typedef struct
{
    DOUBLE dbTimestamp_sec;// Message index or query time in seconds. It is recommended
    // to obtain timestamp_sec from sensor time, it is used as a query
    // time in GetExpectedObserved.
    U8 aucModule_name[DRC_ROAD_HEADER_MODULE_NAME_LEN];
    U32 ulSequence_num; // Sequence number for each message. Each module maintains its own counter for
    // sequence_num, always starting from 0 on boot.
    U32 ulVersion;          // data version,default = 1]
    U64 ullLidar_timestamp; // Lidar Sensor timestamp for nano-second.
    U64 ullCamera_timestamp;// Camera Sensor timestamp for nano-second.
    U64 ullRadar_timestamp; // Radar Sensor timestamp for nano-second.
    drc_status_pb stStatus;           //
    DOUBLE dbPub_timestamp_sec; //Message publishing time in seconds. It is recommended to obtain
    // timestamp_sec from ros::Time::now(), right before calling
    // SerializeToString() and publish().
}drc_dr_obstacle_header_stru;

typedef struct
{
    std::string aucId;
    U32 ulColor;  //DRC_LIGHT_COLOR_E
    U32 ulLight_type; //DRC_LIGHT_TYPE_E
    U32 ulIs_twinkle; //bool
    U32 ulCountdown_time;
    DOUBLE dbTracking_time;
    U32 ulColorLastFrame;
    std::unordered_map<U32, U32> astLast_time;
    U32 ulDrsuId;
}drc_dr_traffic_light_stru;

typedef struct
{
    U32 ulIs_countdown; //TRUE:countdown
    U32 ulLast_time;
    std::list<U32> stNew_Last_time;
}drc_dr_traffic_light_countdown_config;
/************************ smart expressway end***************************/

/*************************DRSU RAIL INFO start********************************************/
enum{
	HEAD_ID,
	TAIL_ID,
	NORMAL_ID,
};


class DRSU_RAIL_INFO
{
public:
	U32	ulSequenceId;
	std::unordered_map<U64,drc_dr_obstacle_stru> astObsInfo;//key is UUID
    std::list<drc_dr_traffic_light_stru> astTrafficInfo;
    GlobalMap astVTGlobalMap; // <UTM_X, <UTM_Y, <obs list>>>
public:
    std::mutex mMutexForTraverse;
};


typedef struct
{
    U32 ulStarthoursOfBusLane; //start hours
    U32 ulStartminutesOfBusLane; //start minutes
    U32 ulEndhoursOfBusLane; //end hours
    U32 ulEndminutesOfBusLane; //end minutes
}BUSLANE_TIME_STRU;

/**
 * @brief 多RCTP之间的预测配置
 */
typedef struct
{
    U32 ulDrsuId;
    DOUBLE dbDrsuUtmX;
    DOUBLE dbDrsuUtmY;
    U32 ulDistance;
    U32 ulPredictedFrames;
}FUSION_PREDICTION_CONFIG_STRU;

typedef enum
{
    NO_FUSION_TYPE = 0,
    RCTP_SEND_TYPE,
    RCTP_RECV_TYPE,
}OVERLAP_AREA_TYPE_E;

/**
 * @brief 多RCTP之间的融合配置
 */
typedef struct
{
    U32 ulRctpId;       //RCTP_N的配置
    U32 ulType;         //0表示不需要多RCTP之间的融合，1表示发送数据，2表示接收数据并处理数据
    // U32 ulSendRctpId;   //发送给哪个RCTP
    // U32 ulRecvRctpId;   //从哪个RCTP接收
    U32 ulDrsuId;       //发送哪个DRSU下的数据/接收哪个DRSU下的数据
    DOUBLE dbDrsuUtmX;     //DRSU的UTMX
    DOUBLE dbDrsuUtmY;     //DRSU的UTMY
    DOUBLE dbDistance;     //超过DRSU多远才会发送
}FUSION_OVERLAP_AREA_CONFIG_STRU;

//typedef unordered_map<U32,DRSU_RAIL_INFO> DRSU_RAIL_INFO_MAP;//key is drsu id
//typedef Map<U32,DRSU_RAIL_INFO> DRSU_RAIL_INFO_MAP;//key is drsu id
typedef std::list<if_drc_drsu::DRSU_DRC_DATA_INFO_IND_MSG> RAW_OBS_INFO_LIST;
typedef std::unordered_map<U32, RAW_OBS_INFO_LIST> DRSU_RAW_OBS_INFO_MAP;
/*************************DRSU RAIL INFO end********************************************/


typedef struct
{
    drc_event::VEHICLE_CONVERSE_RUNNING_WARNING  astVehicleConversePb; //车辆逆行 6
    drc_event::ULTRA_LOW_SPEED_WARNING  astUltraLowSpeedPb; //超低速 8
    drc_event::LOW_SPEED_WARNING astLowSpeedPb; //低速 9
    drc_event::SPOILING_WARNING astSpoilingPb;  //抛洒物 10
    drc_event::EXCEED_SPEED_WARNING astExceedSpeed; //超速 17
    drc_event::ROAD_MAINTENANCE_WARNING astRoadMaintenancePb; //道路维护 18
    drc_event::OCCUPY_EMERGENCY_LANE_WARNING  astOccupyEmeragencyLanePb; //占用应急车道 19
    drc_event::OCCUPY_BUS_LANE_WARNING   astOccupyBusLanePb; //占用公交车道 27
}DRC_EVENT_CFG_PB;


typedef enum
{
    DRC_SINGLE_VEHICLE_TRAFFIC_ACCIDENT = 1,
    DRC_TWO_VEHICLES_TRAFFIC_ACCIDENT = 2,
    DRC_MULTIPLE_VEHICLES_TRAFFIC_ACCIDENT = 3,
    DRC_SINGLE_VEHICLE_ABNORMAL_PARKING = 4,
    DRC_TWO_VEHICLES_ABNORMAL_PARKING = 5,
    DRC_VEHICLE_CONVERSE_RUNNING= 6, 
    DRC_VIOLATION_PERSON =7, 
    DRC_ULTRA_LOW_SPEED = 8, 
    DRC_LOW_SPEED = 9, 
    DRC_SPOILING = 10,
    DRC_TRAFFIC_JAM = 11,
    DRC_ROAD_CLOSE = 12,  
    DRC_CROSS_LANE_DRIVING = 13, 
    DRC_ICING = 14,
    DRC_WET_SLIDING =15,
    DRC_CLUSTER_FOG =16, 
    DRC_EXCEED_SPEED = 17,
    DRC_ROAD_MAINTENANCE = 18,
	DRC_OCCUPY_EMERGENCY_LANE = 19,
	DRC_TRUCK_OCCUPY_THE_FAST_LANE = 20,
    //drc占用事件类型21紧急刹车，请从22继续，23,24用于应急车道柔性复用
    DRC_BLOCK_PEDESTRIANS = 22, //行人被遮挡事件（用于演示）结构体同违规行人
    DRC_TRAFFIC_ACCIDENT= 25,
    DRC_ABNORMAL_PARKING = 26,
    DRC_OCCUPY_BUS_LANE = 27,
	DRC_ABNORMAL_CHANGE_LINE = 28,
    DRC_EVENT_MAX ,
}DRC_EVENT_TPYE_E;


typedef enum
{
    GENERAL_CONFIG = 0,
    VEHICLE_CONVERSE_RUNNING_WARNING = 6,
    VIOLATION_PERSON_WARNING = 7,
    ULTRA_LOW_SPEED_WARNING = 8,
    LOW_SPEED_WARNING = 9,
    SPOILING_WARNING = 10,
    TRAFFIC_JAM_WARNING = 11,
    EXCEED_SPEED_WARNING = 17,
    ROAD_MAINTENANCE_WARNING = 18,
    OCCUPY_EMERGENCY_LANE_WARNING = 19,
    TRUCK_OCCUPY_THE_FAST_LANE_WARNING = 20,
    TRAFFIC_ACCIDENT_WARNING = 25,
    ABNORMAL_PARKING_WARNING = 26,
    OCCUPY_BUS_LANE_WARNING = 27,
}CONFIG_TYPE_E;

typedef struct
{
    CONFIG_TYPE_E eConfigType;
    U32 ulDataLength;
    U32 ulPadding;
    U8  pData[0];
}NACOS_CONFIG_UPDATE_IND_MSG;



//经纬度结构体
struct Geolocation
{
    double latitude;  //纬度
    double longitude; //经度
};



typedef struct
{
    S32 slLng;              //分辨率为1e-7
    S32 slLat;              //分辨率为1e-7
}GpsLoc;

typedef struct
{
    std::string number;          //桩号
    S32 slDistance;         //距离 m
}PileLoc;

typedef struct
{
    S64 s64UtmX;            //分辨率为1e-7
    S64 s64UtmY;            //分辨率为1e-7
    S64 s64UtmZ;            //分辨率为1e-7
}UtmLoc;



//LaneID 和 长度
struct DRC_LANE_STRU
{
	U32 ulDrcLaneId;
	U32 ulLength;
};


typedef enum
{
  DRSU_DATA_IND = 0,
  EVENT_IND = 1,
  TCI_IND = 2,
  FUSED_TRAFFIC_DATA_IND = 3,
  TRAFFICLIGHT_DATA_IND = 4,
  TRAFFIC_INFO_IND = 5,
  KEY_VEHICLE_MAP_IND = 6,
  DEBUG_ABNORMAL_PARKING = 7,
  DEBUG_TRAFFIC_ACCIDENT = 8,
  DEBUG_VIOLATION_PERSON = 9,
  DEBUG_JAM = 10,
  DEBUG_ROAD_MAINTENANCE = 11,
  DEBUG_ULTRA_LOW_SPEED = 12,
  DEBUG_LOW_SPEED = 13,
  DEBUG_EXCEED_SPEED = 14,
  DEBUG_CONVERSE_RUNNING = 15,
  DEBUG_DETECTOR_INFO = 16,
  VT_POSITION_IND = 17,
  SERVICE_ALARM_IND = 18,
  DEBUG_FILTER_STATISTICS_IND = 30,
  DEBUG_ETOD_CHECK = 31,
  TBOX_TRANSMIT_IND = 35,
  UAV_FLIGHT_DATA_REQ = 50003,
  UAV_STARTUP_REQ = 50001,
  UAV_HEARTBEAT_REQ = 50002,
  UAV_STARTUP_RSP = 60001,
  UAV_COMMAND_CTRL = 60003,
  UAV_RADAR_DATA_REQ = 80003
}KAFKA_DATA_TYPE;



typedef struct
{
    U32      ulFrame;
    U32      ulSubFrame;
    double   UtmX;
    double   UtmY;
    double   AvgSpeed; //平均速度
    U32      ulLaneId;
    U32      ulLaneType;
    U32      ulHdLaneId;
    U32      ulHdConnectionId;
}JAM_DETECTOR_INFO;

#endif/*IF_DRC_COMMON_H*/
