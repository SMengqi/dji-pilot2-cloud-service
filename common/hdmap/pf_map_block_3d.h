/************************************************************
  Copyright (C), BroadXT Inc  2025
  FileName   :pf_map_block_3d.h
  Author     :zhangyan     
  Version    :v0.1         
  Date       :2025-06-04
  Description:
  History:
  <author>       <time>       <version >   <desc>
  zhangyan       2025-06-04   v0.1         create 
***********************************************************/
#ifndef PF_MAP_BLOCK_3D_H

#define PF_MAP_BLOCK_3D_H

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#undef GLOBAL
#ifdef PF_MAP_BLOCK_3D_CPP
#define GLOBAL    
#else
#define GLOBAL   extern 
#endif


#define PAI                       3.1415926


//#define pf_map_log(ucLogLevel, format, ...)                     pf_log(ucLogLevel, THIS_LOG_MODULE, format, ## __VA_ARGS__)
#define pf_map_log( format, ...)                                printf(  format, ## __VA_ARGS__)


typedef signed char             S8;
typedef signed short            S16;
typedef signed int              S32;
typedef signed long long        S64;
typedef unsigned char           U8;
typedef unsigned short          U16;
typedef unsigned int            U32;
typedef unsigned long long      U64;

typedef char                    CHAR;
typedef unsigned int            BOOL;
typedef float                   F32;
typedef double                  DOUBLE;

typedef struct
{
    DOUBLE   x;   
    DOUBLE   y;   
}PF_NODE_ST;

typedef struct {
    const char* filename;
    PF_NODE_ST* polygon;
    int* nodeCount;
}PF_POLYGON_DATA;


#define POLYGON_NODE_COUNT_MAX  1024 
GLOBAL PF_NODE_ST gPolygon[POLYGON_NODE_COUNT_MAX];
GLOBAL S32 gPolygonNodeCount;


/*************** 事件响应区域 ***************/
// 3号楼露台 - 异常人员
GLOBAL PF_NODE_ST gPolygonBuild3Terrace[POLYGON_NODE_COUNT_MAX];
GLOBAL S32 gPolygonBuild3TerraceNodeCount;

// 3号楼锥桶 - 异常物品
GLOBAL PF_NODE_ST gPolygonBuild3ConeBucket[POLYGON_NODE_COUNT_MAX];
GLOBAL S32 gPolygonBuild3ConeBucketNodeCount;

// 3号楼纸箱 - 异常物品
GLOBAL PF_NODE_ST gPolygonBuild3Carton[POLYGON_NODE_COUNT_MAX];
GLOBAL S32 gPolygonBuild3CartonNodeCount;

// 信息港锥桶 - 异常物品
GLOBAL PF_NODE_ST gPolygonInfoportConeBucket[POLYGON_NODE_COUNT_MAX];
GLOBAL S32 gPolygonInfoportConeBucketNodeCount;

// 篮球场 - 抛撒物
GLOBAL PF_NODE_ST gPolygonBasketballCourt[POLYGON_NODE_COUNT_MAX];
GLOBAL S32 gPolygonBasketballCourtNodeCount;
/*******************************************/

/*************** 抛投区域 ***************/
// 篮球场抛投区域
GLOBAL PF_NODE_ST gPolygonBasketballCourtThrow[POLYGON_NODE_COUNT_MAX];
GLOBAL S32 gPolygonBasketballCourtNodeCountThrow;
/*******************************************/

/*************** 飞行场地 ***************/
// 宁波研究院
GLOBAL PF_NODE_ST gPolygonAcademy[POLYGON_NODE_COUNT_MAX];
GLOBAL S32 gPolygonAcademyNodeCount;
// 杭州信息港
GLOBAL PF_NODE_ST gPolygonInfoport[POLYGON_NODE_COUNT_MAX];
GLOBAL S32 gPolygonInfoportNodeCount;
// 东方理工大学
GLOBAL PF_NODE_ST gPolygonEitech[POLYGON_NODE_COUNT_MAX];
GLOBAL S32 gPolygonEitechNodeCount;
/*******************************************/

#define RET_ERR    -1
#define RET_OVER    0
#define RET_OK      1


/*
restriction_area	
	type		int	
			0:  fly
			1:  control	
			2:  no-fly
				
	height_limit	int	
				  -1:  no-fly	
				    0:   no-limit		
				n>0: flight height limit	
	
			
ground_area		
	
	type		int	
    			0  : unknown
    			1  : civic architecture			
    			2  : Forest greening			
    			3  : high-tension line
    			4  : airport	
    			5  : urban road	
    			6  : land		
    			7  : water area		
    			8  : park		
    			9  : military area		
 			10: railway	

*/


#define RESTRICT_NO_FLY  -1


typedef enum
{
    FLIGHT_AREA = 0,
    CONTROL_AREA = 1,
	NO_FLY_AREA = 2,
    RESTRICTION_TYPE_MAX,
}PF_MAP_RESTRICTION_TYPE_E;


typedef enum
{
    UNKNOWN = 0,
    ARCHITECTURE = 1,
	FOREST_GREEN = 2,
	HIGH_TENSION = 3,
	AIRPORT = 4,
	ROAD = 5,
	LAND = 6,
	WARTER_AREA = 7,
	PARK = 8,
	MILITARY_AREA = 9,
	RAILWAY = 10,
    GROUND_TYPE_MAX,
}PF_MAP_GROUND_TYPE_E;


typedef enum
{
	DIRECTION_NONE = 0,
    DIRECTION_NORTH = 1,
    DIRECTION_NORTH_EAST = 2,
    DIRECTION_EAST = 4,
    DIRECTION_SOUTH_EAST = 8,
    DIRECTION_SOUTH = 16,
    DIRECTION_SOUTH_WEST = 32,
    DIRECTION_WEST = 64,
    DIRECTION_NORTH_WEST = 128,
    DIRECTION_MAX,
}PF_MAP_DIRECTION_E;




typedef struct
{
    DOUBLE   x;        
    DOUBLE   y;        
    DOUBLE   z;      
}PF_MAP_XYZ_ST;




typedef struct
{
    PF_MAP_XYZ_ST   utm_min;        
    PF_MAP_XYZ_ST   utm_max;        
    PF_MAP_XYZ_ST   block_min;      
    PF_MAP_XYZ_ST   block_max; 
    PF_MAP_XYZ_ST   block_size;
    S32      block_count_x;   
    S32      block_count_y;   
    S32      block_count_z;   
    S32      mapId;             
    S32      type;              
    S32      nodeCount;              
    S32      resdictionCount;              
    S32      groundCount;              
    S32      memSize;              
    CHAR     mapname[128];      
    CHAR     createTime[32];    
    CHAR     version[32];       
    CHAR     producer[40];      
    CHAR     sourceFile[40];    
    CHAR     utm_zone[16];      
}PF_MAP_INFO_ST;



typedef struct
{
    S32      ID;                
    S32      extrapointsID;      
    S32      type;              
    S32      node_i;      
    S32      node_count;      
    DOUBLE   height_limit;      
    DOUBLE   min_x;      
    DOUBLE   min_y;      
    DOUBLE   max_x;      
    DOUBLE   max_y;      
    CHAR     name[68];     
}PF_MAP_AREA_ST;







typedef struct
{
    S8    resdiction_i;    // 0: invalid   
    S8    ground_i;        // 0: invalid   
    S16   z_i;             // 0: invalid     
    S32   zmax_mm;    
}PF_MAP_BLOCK_ST;





/**********************************************************************************************
 * @function      get_map_info
 * @brief           get map info form  .xml  file  
 * @input          char*    mapfilename (include path)
 * @output        void
 * @return         S32      RET_ERR:  fail   
 *                         		RET_OK:   success
 *********************************************************************************************/
GLOBAL S32 get_map_info(CHAR* mapfilename);


/**********************************************************************************************
 * @function      check_block_around
 * @brief           Check around the input point(x,y,z)  
 * @input          (x,y,z)  position  UTM
 * @input          dis_xy  Horizontal detection distance
 * @input          dis_z    Vertical detection distance
 * @output        void
 * @return         S32      RET_ERR:    detect fail   
 *                                 RET_OVER   detect nothing
 *                                 RET_OK:      detect block
 *********************************************************************************************/

GLOBAL S32 check_block_around(DOUBLE x,DOUBLE y,DOUBLE z,DOUBLE dis_xy ,DOUBLE dis_z);

GLOBAL S32 check_block_around_lonlat(DOUBLE lat,DOUBLE lon,DOUBLE z,DOUBLE dis_xy ,DOUBLE dis_z);

/**********************************************************************************************
 * @function      check_in_resdiction
 * @brief           Check whether the input position is in the resdiction
 * @input          (x,y,z)  position  UTM
 * @output        void
 * @return         S32      RET_OVER   not in resdiction
 *                                 RET_OK:      in resdiction
 *********************************************************************************************/
GLOBAL S32 check_fly_resdiction(DOUBLE x, DOUBLE y, DOUBLE z);

/**********************************************************************************************
 * @function      check_block_around_line
 * @brief           Check around the input point(x,y,z)  
 * @input          DOUBLE  (x,y,z)        start position  UTM
 * @input          DOUBLE  angle          direction angle 
 * @input          DOUBLE  length         route line length
 * @input          DOUBLE  dis_xy        horizontal detection distance
 * @input          DOUBLE  dis_z          vertical detection distance
 * @output        DOUBLE  safe_dis      safe route length
 * @return         S32        RET_ERR:    detect fail   
 *                                   RET_OVER   detect nothing
 *                                   RET_OK:      detect block
 *********************************************************************************************/

GLOBAL S32 check_block_around_line(DOUBLE x, DOUBLE y, DOUBLE z, DOUBLE angle, DOUBLE length , DOUBLE dis_xy, DOUBLE dis_z, DOUBLE* safe_dis);
GLOBAL S32 check_block_around_line_lonlat(DOUBLE lat, DOUBLE lon, DOUBLE z, DOUBLE angle, DOUBLE length , DOUBLE dis_xy, DOUBLE dis_z, DOUBLE* safe_dis);

/**********************************************************************************************
 * @function      get_block_min_z
 * @brief           
 * @input          
 * @output        void
 * @return         DOUBLE      block_min_z
 *                                        
 *********************************************************************************************/
GLOBAL DOUBLE get_block_min_z();






/**********************************************************************************************
 * @function      get_block_z
 * @input          DOUBLE  (x,y)  horizontal position  UTM
 * @input          
 * @output        DOUBLE      block_z
 * @return         S32            RET_ERR:    fail   
 *                                        RET_OK:      ok
 *********************************************************************************************/
GLOBAL DOUBLE get_block_z(DOUBLE x, DOUBLE y, DOUBLE* block_z);

GLOBAL DOUBLE get_block_min_z_lonlat(DOUBLE lat, DOUBLE lon, DOUBLE* block_z);

GLOBAL BOOL readPolygonFile(CHAR* filename);
GLOBAL BOOL readPolygonFile2(const CHAR* filename, PF_NODE_ST* golygon, S32* polygonNodeCount);
GLOBAL void BxtTest_LoadPolygonFiles(PF_POLYGON_DATA* polygons, int polygon_count);
GLOBAL bool BxtTest_CheckPointInPolygons(PF_POLYGON_DATA* polygons, int polygon_count, PF_NODE_ST point);

GLOBAL BOOL isInPolygon(PF_NODE_ST point, PF_NODE_ST* polygon, S32 nodeCount);

#ifdef __cplusplus
}
#endif

#endif

