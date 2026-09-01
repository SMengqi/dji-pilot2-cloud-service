/************************************************************
  Copyright (C), BroadXT Inc  2019
  FileName   :pf_map_block.cpp
  Author     :zhangyan     
  Version    :v0.1         
  Date       :2021-03-15
  Description:
  History:
  <author>       <time>       <version >   <desc>
  zhangyan       2021-03-15   v0.1         create 
***********************************************************/
#define THIS_MODULE MODULE_COMM

#define PF_MAP_BLOCK_CPP

#include "pf_map_block.h"
#include <fstream>

using namespace std;

#ifdef GTEST_EN
extern S32 gdebugflag;


char *loglevelstr[]=  {
    "DNU",
    "FAT",
    "ERR",
    "WRN",
    "INF",
    "TRC",
    "UIF"
};


#undef  pl_log(ulLogLevel, format, ...)        
#define pl_log(ulLogLevel, format, ...)                         printf("\n %s: ",loglevelstr[ulLogLevel]);printf(  format, ## __VA_ARGS__);printf("\n");        

#endif


typedef struct
{
    DOUBLE      x;                 
    DOUBLE      y;                 
    DOUBLE      dis;                 
    S32         index;               
    S32         side_f;      
}DIS_POINT_LINE;



/*
                           .P0(x0,y0)
                           |
                           |
                           | 
      .--------------------.--------------------.     
    P1(x1,y1)              P(x,y)                P2(x2,y2)  
	
	a = x2 - x1
	b = y2 - y1
	x = (a*a*x0 + b*b*x1 + a*b*(y0-y1))/(a*a + b*b);
	y = (a*a*y1 + b*b*y0 + a*b*(x0-x1))/(a*a + b*b);
	
    f = b*(x0-x1)-a*(y0-y1)
    f>0 : P0 is on the right of P1P2 
    f<0 : P0 is on the left of P1P2 
    f==0: P0 is on P1P2         
*/
DIS_POINT_LINE distance_node2line(DOUBLE p0_x,DOUBLE p0_y, DOUBLE p1_x,DOUBLE p1_y, DOUBLE p2_x,DOUBLE p2_y)
{
    DIS_POINT_LINE retSt = {0 , 0, 0 , 0 , 0};
    
    DOUBLE x0 = p0_x;
    DOUBLE y0 = p0_y;
    DOUBLE x1 = p1_x;
    DOUBLE y1 = p1_y;
    DOUBLE x2 = p2_x;
    DOUBLE y2 = p2_y;
                
    DOUBLE a = x2 - x1;
    DOUBLE b = y2 - y1;
    DOUBLE aa = a*a;
    DOUBLE bb = b*b;
    
    if( aa + bb != 0)
    {
        DOUBLE bx0_x1 = b*(x0-x1);
        DOUBLE ay0_y1 = a*(y0-y1);

        DOUBLE x = (aa*x0 + bb*x1 + b*ay0_y1)/(aa + bb);
        DOUBLE y = (aa*y1 + bb*y0 + a*bx0_x1)/(aa + bb);
        
        DOUBLE x_x1 = x-x1;
        DOUBLE y_y1 = y-y1;
        DOUBLE x_x2 = x-x2;
        DOUBLE y_y2 = y-y2;
        
        if((x_x1*x_x2<=0.000001)&&(y_y1*y_y2<=0.000001))
        {
            retSt.x = x;
            retSt.y = y;
            retSt.dis = sqrt((x - x0)*(x - x0)+(y - y0)*(y - y0));
            retSt.side_f = bx0_x1 - ay0_y1;
        }
    }
    
    return retSt;   
}




DOUBLE get_nodeList(DOUBLE p0_x,DOUBLE p0_y, S32 lanemarker_i,DOUBLE len,PF_MAP_NODE_STRUCT* pNodeList, S32* s32Count)
{
    PF_MAP_NODE_LINE_ST* pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastLanemarker[lanemarker_i])+sizeof(PF_MAP_LINE_INFO_ST));
    S32 node_count = pastLanemarker[lanemarker_i]->nodeCount;
    DOUBLE dblen = 0;
    S32 s32Index = *s32Count;
    for(S32 i=node_count-2;i>=0;i--)
    {
        if(s32Index == 0)
        {
            DIS_POINT_LINE retSt = distance_node2line(p0_x,p0_y,pnode[i+1].node_x,pnode[i+1].node_y,pnode[i].node_x,pnode[i].node_y);
            if(retSt.x>0)
            {
                pNodeList[0].x=retSt.x;
                pNodeList[0].y=retSt.y;
                s32Index++;
                i++;
            }
            else
            {
                DOUBLE dis1=(p0_x-pnode[i+1].node_x)*(p0_x-pnode[i+1].node_x)+(p0_y-pnode[i+1].node_y)*(p0_y-pnode[i+1].node_y);
                DOUBLE dis2=(p0_x-pnode[i].node_x)*(p0_x-pnode[i].node_x)+(p0_y-pnode[i].node_y)*(p0_y-pnode[i].node_y);
                if(dis1<dis2)
                {
                    pNodeList[0].x=pnode[i+1].node_x;
                    pNodeList[0].y=pnode[i+1].node_y;
                    s32Index++;
                    i++;
                }
            }
        }
        else
        {
            DOUBLE disX = pnode[i].node_x - pNodeList[s32Index-1].x;
            DOUBLE disY = pnode[i].node_y - pNodeList[s32Index-1].y;
            DOUBLE tmpdis = sqrt(disX*disX+disY*disY);
            if(dblen + tmpdis < len) 
            {
                if(tmpdis>1)
                {
                    dblen+=tmpdis;
                    pNodeList[s32Index].x=pnode[i].node_x;
                    pNodeList[s32Index].y=pnode[i].node_y;
                    s32Index++;
                }
            }
            else
            {
                tmpdis=(len-dblen)/tmpdis;
                pNodeList[s32Index].x=pNodeList[s32Index-1].x + disX*tmpdis;
                pNodeList[s32Index].y=pNodeList[s32Index-1].y + disY*tmpdis;
                s32Index++;
                *s32Count=s32Index;
                return len;
            }
        }
    }
    
    *s32Count=s32Index;
    return dblen;
}



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

S32 pf_map_get_centreNodeList(DOUBLE p0_x,DOUBLE p0_y,DOUBLE maxlen, S32 minLaneNum, S32 maxLaneNum,PF_MAP_NODE_STRUCT* pNodeList,DOUBLE* width)
{
    S32 lane_i=0;
    S32 laneNum=0;
    S32 type=0;
    S32 leftNodeCount=0;
    S32 rightNodeCount=0;
    PF_MAP_NODE_STRUCT leftNodeList[1000]={0};
    PF_MAP_NODE_STRUCT rightNodeList[1000]={0};
    DOUBLE dblen = maxlen;
    
    S32 s32ret = PF_MAP_ERR;
    s32ret = pf_map_get_ID_by_utm(p0_x,p0_y,&type,NULL,&lane_i,NULL,&laneNum);

    if(PF_MAP_ERR == s32ret)
    {
        return PF_MAP_ERR;
    }
    
    if(type == MAP_LANE)
    {
        S32 Section_i = pstLane[lane_i].section_i;
        S32 firstLane_i = pstSection[Section_i].lane_i;
        S32 left_i = -1;
        S32 right_i = -1;
        S32 count = 0;
        S32 centreNodeCount = 0;

        if((minLaneNum<=0)||(maxLaneNum<=0))
        {
            minLaneNum = pstLane[lane_i].laneNumber;
            maxLaneNum = pstLane[lane_i].laneNumber;
        }

        for(int i=0;i<pstSection[Section_i].laneCount;i++)
        {
            if(pstLane[firstLane_i+i].laneNumber == minLaneNum)
            {
                left_i=firstLane_i+i;
            }
            if(pstLane[firstLane_i+i].laneNumber == maxLaneNum)
            {
                right_i=firstLane_i+i;
            }
        }

        while(left_i>=0)
        {
            DOUBLE tmpdb = get_nodeList(p0_x,p0_y,pstLane[left_i].leftLanemarker_i,dblen,leftNodeList,&leftNodeCount);

            if(tmpdb < dblen)
            {
                dblen-=tmpdb;
                left_i = pf_map_get_preLane_by_index(left_i);
            }
            else
            {
                break;
            }
        }

        dblen = maxlen;
        while(right_i>=0)
        {
            DOUBLE tmpdb = get_nodeList(p0_x,p0_y,pstLane[right_i].rightLanemarker_i,dblen,rightNodeList,&rightNodeCount);

            if(tmpdb < dblen)
            {
                dblen-=tmpdb;
                right_i = pf_map_get_preLane_by_index(right_i);
            }
            else
            {
                break;
            }
        }

#ifdef GTEST_EN
if(gdebugflag == 1)
{
    pf_map_log("\n\nleftNode[%d]:",leftNodeCount);
    left_i = 0;
    while(left_i<leftNodeCount)
    {
        pf_map_log("\n%lf,%lf",leftNodeList[left_i].x,leftNodeList[left_i].y);
        left_i++;
    }

    pf_map_log("\n\nrightNode[%d]:",rightNodeCount);
    right_i = 0;
    while(right_i<rightNodeCount)
    {
        pf_map_log("\n%lf,%lf",rightNodeList[right_i].x,rightNodeList[right_i].y);
        right_i++;
    }
}
#endif


        left_i = 0;
        right_i = 0;
        dblen = 0;
        s32ret=1;
        while(s32ret==1)
        {
            DOUBLE x = (leftNodeList[left_i].x+rightNodeList[right_i].x)/2;
            DOUBLE y = (leftNodeList[left_i].y+rightNodeList[right_i].y)/2;
            DIS_POINT_LINE tmpSt = {0}; 

            pNodeList[centreNodeCount].x=x;
            pNodeList[centreNodeCount].y=y;
            centreNodeCount++;
            
            if(left_i<leftNodeCount-1)
            {
                tmpSt = distance_node2line(x,y, leftNodeList[left_i].x,leftNodeList[left_i].y,leftNodeList[left_i+1].x,leftNodeList[left_i+1].y);
            }

            if((left_i>0)&&(tmpSt.x<=0.0001))
            {
                tmpSt = distance_node2line(x,y, leftNodeList[left_i].x,leftNodeList[left_i].y,leftNodeList[left_i-1].x,leftNodeList[left_i-1].y);
            }

            if(tmpSt.x>0.0001)
            {
                dblen += tmpSt.dis;
                count++;
            }

            s32ret=0;
            if(left_i<leftNodeCount-1)
            {
                left_i++;
                s32ret=1;
            }
            if(right_i<rightNodeCount-1)
            {
                right_i++;
                s32ret=1;
            }
        }
        
        *width = dblen/count;

#ifdef GTEST_EN
if(gdebugflag == 1)
{
        pf_map_log("\n\ncentreNode[%d]:",centreNodeCount);
        right_i = 0;
        while(right_i<centreNodeCount)
        {
            pf_map_log("\n%lf,%lf",pNodeList[right_i].x,pNodeList[right_i].y);
            right_i++;
        }
        pf_map_log("\nwidth: %lf \n",*width);
}
#endif
                
        return centreNodeCount;

    }

    return PF_MAP_ERR;
}




/**********************************************************************************************
 * @function      pf_map_utmZ_file_read
 * @brief         read  map height file  
 * @input         char*    map height file (include path)
 * @output        void
 * @return        S32      PF_MAP_ERR: read fail   
 *                         PF_MAP_SUCCESS: read success
 *********************************************************************************************/
S32 pf_map_utmZ_file_read(char* filename)

{
    S32    s32ret = PF_MAP_ERR;
    S16    s16ret = PF_MAP_ERR;
    
    char   linestr[1024] = {0};
    char   tmpstr[128] = {0};
    
    U32    index = 0;
    U32    lineCount = 0;
    
    DOUBLE dbx=0;
    DOUBLE dby=0;
    DOUBLE dbz=0;
    U64    tmpsize=0;
    
    ifstream stIFile(filename);

    if(!stIFile.good())
    {
        pl_log(ERR," open file:  %s  fail ",  filename);
        return PF_MAP_ERR;
    }

    pstMapUtmXYZ = NULL;
    gs32MapUtmXYZCount = 0;
    gs32MapUtmX_Y = 1;
    
    stIFile.getline(linestr, 1023);

    s16ret = sscanf(linestr, "POINT_X,POINT_Y,grid_code,%d",&gs32MapUtmXYZCount);
    
    if(s16ret != 1)
    {
        pl_log(ERR," get MAP_UTM_XYZ fail , s16ret_%d , '%s' ", s16ret,linestr);
        
        s32ret = PF_MAP_ERR;
        gs32MapUtmXYZCount = 0;
       
        goto end;
    }

    tmpsize = ((U64)gs32MapUtmXYZCount)*sizeof(PF_MAP_UTM_XYZ_ST);

    pstMapUtmXYZ = (PF_MAP_UTM_XYZ_ST*)malloc(tmpsize);

    if(pstMapUtmXYZ == NULL)
    {
        pl_log(ERR," pstMapUtmXYZ  malloc fail!  size_(%d*%d)=%ld ",gs32MapUtmXYZCount,sizeof(PF_MAP_UTM_XYZ_ST),tmpsize);
        s32ret = PF_MAP_ERR;
        goto end;
    }

    memset(pstMapUtmXYZ,0,tmpsize);

    index = 0;
    lineCount = 1;
    
    while( !stIFile.eof())
    {
        stIFile.getline(linestr, 1023);
        
        lineCount++;
                               // x   y   z    
        s16ret = sscanf(linestr,"%lf,%lf,%lf ",&dbx,&dby,&dbz);

        if(s16ret != 3)
        {
            if(NULL == strstr(linestr, ","))
            {
                continue;
            }
            
            pl_log(ERR," pf_map_utmZ_file_read fail!  s16ret_%d ,  line_%d: '%s' ",s16ret,lineCount,linestr);
            s32ret = PF_MAP_ERR;
            goto end;
        }


        if(index >= gs32MapUtmXYZCount)
        {
            pl_log(ERR," pf_map_utmZ_file_read:   count_%d > malloc_count_%d , '%s' ",index,gs32MapUtmXYZCount,linestr);
            s32ret = PF_MAP_ERR;
            goto end;
        }
        
        pstMapUtmXYZ[index].x = S32(dbx);
        pstMapUtmXYZ[index].y = S32(dby);
        pstMapUtmXYZ[index].z = dbz;
        
        index++;

    }

    if(index < gs32MapUtmXYZCount)
    {
        gs32MapUtmXYZCount = index;
    }

    gs32MapUtmX_Y = index/(pstMapUtmXYZ[index-1].x - pstMapUtmXYZ[0].x);

    if(gs32MapUtmX_Y < 1)
    {
        gs32MapUtmX_Y = 1;
    }
    
    s32ret = PF_MAP_SUCCESS;
    
    pl_log(INF," pf_map_utmZ_file_read: %s over! , count_%d ",filename,index);
    
end:

    stIFile.close();

    return s32ret;
}






/**********************************************************************************************
 * @function      pf_map_block_file_read
 * @brief         read  .mapblock file  
 * @input         char*    .mapblock file name (include path)
 * @output        void
 * @return        S32      PF_MAP_ERR: read fail   
 *                         PF_MAP_SUCCESS: read success
 *********************************************************************************************/
S32 pf_map_block_file_read(char* mapfilename)
{
    S32   elementSize[MAP_ELEMENT_MAX] = {0};

    S32   element_i[MAP_ELEMENT_MAX] = {0};
    S32   elementCurSize[MAP_ELEMENT_MAX] = {0};


    U32   validblock_i = 0;
    U64   totalmemsize = 0;

    S32   s32ret = PF_MAP_ERR;
    S16   s16ret = PF_MAP_ERR;
    
    S32   s32data[20] = {0};
   
    char  linestr[1024] = {0};
    char  tmpstr[128] = {0};
    
    U32   nextpos = 0;
    U32   curtpos = 0;

    U8*   proot = NULL;
    
    ifstream stIFile(mapfilename);

    if(!stIFile.good())
    {
        pl_log(ERR," open file:  %s  fail ",  mapfilename);
        return PF_MAP_ERR;
    }

    pstMap = NULL;
    pstRoad = NULL;
    pstConnection = NULL;
    pstLink = NULL;
    pstLaneLink = NULL;
    pstSection = NULL;
    pstLane = NULL;
    pstWaitingLane = NULL;
    pstTrafficLight = NULL;

    pastExtrapoint = NULL;
    pastLanemarker = NULL;
    pastPavement = NULL;
    pastNoRunZone = NULL;
    pastParkingZone = NULL;
    pastDividerLine = NULL;
    pastRoadEdge = NULL;


    memset(gas32ElementCount,0,sizeof(gas32ElementCount));


    while( !stIFile.eof())
    {
        stIFile.getline(linestr, 1023);
        s16ret = sscanf(linestr, " %s %n ", tmpstr, &nextpos);

        if(s16ret != 1)
        {
            continue;
        }
        
        if(0==strcmp("meminfo" , tmpstr ))  //  meminfo  totalmemsize  utmx_min  utmy_min
        {
            s16ret = sscanf(&(linestr[nextpos]), " %ld  %lx  %lx ",&gu64TotalMemSize,&gdbUtmOffset_x,&gdbUtmOffset_y);

            if(s16ret != 3)
            {
                pl_log(ERR," get meminfo fail , s16ret_%d , '%s' ", s16ret,linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }
        }
        else if(0==strcmp("elementcount" , tmpstr ))  //  elementcount  MAP_INFO ...  MAP_ROADEDGE
        {
            curtpos = 0;
            for(int i=0;i<MAP_ELEMENT_MAX;i++)
            {
                curtpos += nextpos;
                s16ret = sscanf(&(linestr[curtpos]), " %d %n ",&s32ret,&nextpos);
                
                if(s16ret != 1)
                {
                    pl_log(ERR," get elementcount fail , i_%d , '%s' ", i,linestr);
                    s32ret = PF_MAP_ERR;
                    goto end;
                }

                gas32ElementCount[i]= s32ret;
            }
        }
        else if(0==strcmp("elementmemsize" , tmpstr ))  //  elementmemsize:  MAP_INFO ...  MAP_ROADEDGE
        {
            curtpos = 0;
            for(int i=0;i<MAP_ELEMENT_MAX;i++)
            {
                curtpos += nextpos;
                s16ret = sscanf(&(linestr[curtpos]), " %d %n ",&s32ret,&nextpos);
                
                if(s16ret != 1)
                {
                    pl_log(ERR," get elementmemsize fail , i_%d , '%s' ", i,linestr);
                    s32ret = PF_MAP_ERR;
                    goto end;
                }

                elementSize[i]= s32ret;
            }
        }
        else if(0==strcmp("block_count" , tmpstr ))  
        {            
                            // block_count  validblockcount  block_count_x  block_count_y  block_w  block_h  blockTotalSize
            s16ret = sscanf(&(linestr[nextpos]), " %d  %d  %d  %d  %d  %ld ",&gu32BlockValidCount,
                            &gu32BlockCount_x,&gu32BlockCount_y,&gu32Block_w,&gu32Block_h,&gu64BlockTotalSize);

            if(s16ret != 6)
            {
                pl_log(ERR," get block_count fail , s16ret_%d , '%s' ", s16ret,linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }

            for(int i=0;i<MAP_ELEMENT_MAX;i++)
            {
                if((gas32ElementCount[i] < 0)||(elementSize[i] < 0)||
                    ((gas32ElementCount[i] == 0)&&(elementSize[i] > 0))||((gas32ElementCount[i] > 0)&&(elementSize[i] == 0)))
                {
                    pl_log(ERR," element[%d]:  count_%d   memsize_%ld ",i,gas32ElementCount[i], elementSize[i]);
                    s32ret = PF_MAP_ERR;
                    goto end;
                }
                
                totalmemsize += elementSize[i];
            }

            if(gu64TotalMemSize <= totalmemsize)
            {
                pl_log(ERR," gu64TotalMemSize_%ld  <=  totalmemsize_%ld ",gu64TotalMemSize, totalmemsize);
                s32ret = PF_MAP_ERR;
                goto end;
            }
            
            proot = (U8*)malloc(gu64TotalMemSize);

            if(proot == NULL)
            {
                pl_log(ERR," proot  malloc fail!  size_%ld ",gu64TotalMemSize);
                s32ret = PF_MAP_ERR;
                goto end;
            }

            memset(proot,0,gu64TotalMemSize);

            
            totalmemsize = 0;

            if(elementSize[MAP_INFO] > 0)
            {
                pstMap = (PF_MAP_INFO_ST*)((U8*)proot);
                totalmemsize +=  elementSize[MAP_INFO];
            }


            if(elementSize[MAP_ROAD] > 0)
            {
                pstRoad = (PF_MAP_ROAD_ST*)((U8*)proot + totalmemsize);
                totalmemsize +=  elementSize[MAP_ROAD];
            }

            if(elementSize[MAP_CONNECTION] > 0)
            {
                pstConnection = (PF_MAP_CONNECTION_ST*)((U8*)proot + totalmemsize);
                totalmemsize +=  elementSize[MAP_CONNECTION];
            }

            if(elementSize[MAP_LINK] > 0)
            {
                pstLink = (PF_MAP_LINK_ST*)((U8*)proot + totalmemsize);
                totalmemsize +=  elementSize[MAP_LINK];
            }
            
            if(elementSize[MAP_LANE_LINK] > 0)
            {
                pstLaneLink = (PF_MAP_LANE_LINK_ST*)((U8*)proot + totalmemsize);
                totalmemsize +=  elementSize[MAP_LANE_LINK];
            }
            
            if(elementSize[MAP_SECTION] > 0)
            {
                pstSection = (PF_MAP_SECTION_ST*)((U8*)proot + totalmemsize);
                totalmemsize +=  elementSize[MAP_SECTION];
            }

            if(elementSize[MAP_LANE] > 0)
            {
                pstLane = (PF_MAP_LANE_ST*)((U8*)proot + totalmemsize);
                totalmemsize +=  elementSize[MAP_LANE];
            }
            
            if(elementSize[MAP_WAITING_LANE] > 0)
            {
                pstWaitingLane = (PF_MAP_WAITING_LANE_ST*)((U8*)proot + totalmemsize);
                totalmemsize +=  elementSize[MAP_WAITING_LANE];
            }

            if(elementSize[MAP_TRAFFICLIGHT] > 0)
            {
                pstTrafficLight = (PF_MAP_TRAFFICLIGHT_ST*)((U8*)proot + totalmemsize);
                totalmemsize +=  elementSize[MAP_TRAFFICLIGHT];
            }

            if(elementSize[MAP_EXTRAPOINT] > 0)
            {
                pastExtrapoint = (PF_MAP_LINE_INFO_ST**)((U8*)proot + totalmemsize);
                pastExtrapoint[0] = (PF_MAP_LINE_INFO_ST*)(pastExtrapoint + gas32ElementCount[MAP_EXTRAPOINT]);
                totalmemsize +=  elementSize[MAP_EXTRAPOINT];
            }
            
            if(elementSize[MAP_LANEMARKER] > 0)
            {
                pastLanemarker = (PF_MAP_LINE_INFO_ST**)((U8*)proot + totalmemsize);
                pastLanemarker[0] = (PF_MAP_LINE_INFO_ST*)(pastLanemarker + gas32ElementCount[MAP_LANEMARKER]);
                totalmemsize +=  elementSize[MAP_LANEMARKER];
            }

            if(elementSize[MAP_PAVEMENT] > 0)
            {
                pastPavement = (PF_MAP_PAVEMENT_INFO_ST**)((U8*)proot + totalmemsize);
                pastPavement[0] = (PF_MAP_PAVEMENT_INFO_ST*)(pastPavement + gas32ElementCount[MAP_PAVEMENT]);
                totalmemsize +=  elementSize[MAP_PAVEMENT];
            }
            
            if(elementSize[MAP_NO_RUN_ZONE] > 0)
            {
                pastNoRunZone = (PF_MAP_NORUNZONE_INFO_ST**)((U8*)proot + totalmemsize);
                pastNoRunZone[0] = (PF_MAP_NORUNZONE_INFO_ST*)(pastNoRunZone + gas32ElementCount[MAP_NO_RUN_ZONE]);
                totalmemsize +=  elementSize[MAP_NO_RUN_ZONE];
            }

            if(elementSize[MAP_PARKING_ZONE] > 0)
            {
                pastParkingZone = (PF_MAP_LINE_INFO_ST**)((U8*)proot + totalmemsize);
                pastParkingZone[0] = (PF_MAP_LINE_INFO_ST*)(pastParkingZone + gas32ElementCount[MAP_PARKING_ZONE]);
                totalmemsize +=  elementSize[MAP_PARKING_ZONE];
            }
            
            if(elementSize[MAP_CENTER_LINE] > 0)
            {
                pastDividerLine = (PF_MAP_LINE_INFO_ST**)((U8*)proot + totalmemsize);
                pastDividerLine[0] = (PF_MAP_LINE_INFO_ST*)(pastDividerLine + gas32ElementCount[MAP_CENTER_LINE]);
                totalmemsize +=  elementSize[MAP_CENTER_LINE];
            }
            
            if(elementSize[MAP_ROADEDGE] > 0)
            {
                pastRoadEdge = (PF_MAP_ROADEDGE_INFO_ST**)((U8*)proot + totalmemsize);
                pastRoadEdge[0] = (PF_MAP_ROADEDGE_INFO_ST*)(pastRoadEdge + gas32ElementCount[MAP_ROADEDGE]);
                totalmemsize +=  elementSize[MAP_ROADEDGE];
            }


            if(elementSize[MAP_MILEAGE] > 0)
            {
                pstMileage = (PF_MAP_MILEAGE_ST*)((U8*)proot + totalmemsize);
                totalmemsize +=  elementSize[MAP_MILEAGE];
            }

            
            pastMapBlock = (PF_MAP_BLOCK_INFO_ST**)((U8*)proot + totalmemsize);
            pastMapBlock[0] = (PF_MAP_BLOCK_INFO_ST*)(pastMapBlock + gu32BlockCount_x*gu32BlockCount_y);
            totalmemsize = 0;

        }
        else if(0==strcmp("MAP_INFO" , tmpstr ))  
        {
            CHAR* ptmpstr0 = NULL;
            CHAR* ptmpstr1 = NULL;
            
            s16ret = sscanf(&(linestr[nextpos]), " %d  %d  %lf  %lf  %lf  %lf ",
                    &(pstMap[element_i[MAP_INFO]].mapId),&(pstMap[element_i[MAP_INFO]].type),
                    &(pstMap[element_i[MAP_INFO]].utm_x_min),&(pstMap[element_i[MAP_INFO]].utm_y_min),
                    &(pstMap[element_i[MAP_INFO]].utm_x_max),&(pstMap[element_i[MAP_INFO]].utm_y_max));
            
            if(s16ret != 6)
            {
                pl_log(ERR," get MAP_INFO fail , s16ret_%d , '%s' ", s16ret,linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }

            ptmpstr1 = strstr(linestr,"\"");

            for(int i=0;i<6;i++)
            {
                ptmpstr0 = strstr(ptmpstr1,"\"");
                    
                if(ptmpstr0 == NULL)
                {
                    pl_log(ERR," get MAP_INFO fail , s16ret_%d , '%s' ", 6+i,linestr);
                    s32ret = PF_MAP_ERR;
                    goto end;
                }
                
                ptmpstr0++;
                ptmpstr1 = strstr(ptmpstr0,"\"");
                    
                if(ptmpstr1 == NULL)
                {
                    pl_log(ERR," get MAP_INFO fail , s16ret_%d , '%s' ", 6+i,linestr);
                    s32ret = PF_MAP_ERR;
                    goto end;
                }
                
                *ptmpstr1=0;
                
                if(strlen(ptmpstr0) > 0)
                {
                    if(i == 0)
                    {
                        sprintf(pstMap->mapname,"%s\0",ptmpstr0);
                    }
                    else if(i == 1)
                    {
                        sprintf(pstMap->createTime,"%s\0",ptmpstr0);
                    }
                    else if(i == 2)
                    {
                        sprintf(pstMap->version,"%s\0",ptmpstr0);
                    }
                    else if(i == 3)
                    {
                        sprintf(pstMap->producer,"%s\0",ptmpstr0);
                    }
                    else if(i == 4)
                    {
                        sprintf(pstMap->sourceFile,"%s\0",ptmpstr0);
                    }
                    else if(i == 5)
                    {
                        sprintf(pstMap->utm_zone,"%s\0",ptmpstr0);
                    }
                }
                
                ptmpstr1++;

            }
            
            element_i[MAP_INFO]++;
            elementCurSize[MAP_INFO] += sizeof(PF_MAP_INFO_ST);
            if((element_i[MAP_INFO] > gas32ElementCount[MAP_INFO])||(elementCurSize[MAP_INFO] > elementSize[MAP_INFO]))
            {
                pl_log(ERR," MAP_INFO overflow , element_i_%d , gas32ElementCount_%d , curSize_%d , elementSize_%d /n%s/n/n", 
                        element_i[MAP_INFO],gas32ElementCount[MAP_INFO],elementCurSize[MAP_INFO],elementSize[MAP_INFO],linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }
        }
        else if(0==strcmp("MAP_ROAD" , tmpstr ))  
        {
            CHAR* ptmpstr0 = NULL;
            CHAR* ptmpstr1 = NULL;
            
            s16ret = sscanf(&(linestr[nextpos]), " %d  %d  %d  %d  %d ",&(s32data[0]),&(s32data[1]),
                            &(s32data[2]),&(s32data[3]),&(s32data[4]));
            
            if(s16ret != 5)
            {
                pl_log(ERR," get MAP_ROAD fail , s16ret_%d , '%s' ", s16ret,linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }

            pstRoad[element_i[MAP_ROAD]].ID = s32data[0];
            pstRoad[element_i[MAP_ROAD]].length = s32data[1];
            pstRoad[element_i[MAP_ROAD]].laneWidth = s32data[2];
            pstRoad[element_i[MAP_ROAD]].type = (S8)s32data[3];
            pstRoad[element_i[MAP_ROAD]].dividerLine_i = (S8)s32data[4];

            
            ptmpstr0 = strstr(linestr,"\"");
            
            if(ptmpstr0 == NULL)
            {
                pl_log(ERR," get MAP_ROAD  road name fail ,  '%s' ", linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }
            
            ptmpstr0++;
            ptmpstr1 = strstr(ptmpstr0,"\"");
                
            if(ptmpstr1 == NULL)
            {
                pl_log(ERR," get MAP_ROAD  road name fail ,  '%s' ", linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }
            
            *ptmpstr1=0;
            if(strlen(ptmpstr0) > 0)
            {
                sprintf(pstRoad[element_i[MAP_ROAD]].roadName,"%s\0",ptmpstr0);
            }

            element_i[MAP_ROAD]++;
            elementCurSize[MAP_ROAD] += sizeof(PF_MAP_ROAD_ST);
            if((element_i[MAP_ROAD] > gas32ElementCount[MAP_ROAD])||(elementCurSize[MAP_ROAD] > elementSize[MAP_ROAD]))
            {
                pl_log(ERR," MAP_ROAD overflow , element_i_%d , gas32ElementCount_%d , curSize_%d , elementSize_%d /n%s/n/n", 
                        element_i[MAP_ROAD],gas32ElementCount[MAP_ROAD],elementCurSize[MAP_ROAD],elementSize[MAP_ROAD],linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }
        }
        else if(0==strcmp("MAP_CONNECTION" , tmpstr ))  
        {
            s16ret = sscanf(&(linestr[nextpos]), " %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d",
                    &(s32data[0]),&(s32data[1]),&(s32data[2]),&(s32data[3]),&(s32data[4]),&(s32data[5]),&(s32data[6]),&(s32data[7]),&(s32data[8]),
                    &(s32data[9]),&(s32data[10]),&(s32data[11]),&(s32data[12]),&(s32data[13]),&(s32data[14]),&(s32data[15]));
            
            if(s16ret == 14) 
            {
                pstConnection[element_i[MAP_CONNECTION]].ID = s32data[0];
                pstConnection[element_i[MAP_CONNECTION]].s16angle = (S16)s32data[1];
                pstConnection[element_i[MAP_CONNECTION]].extrapoint_i = (S16)s32data[2];
                pstConnection[element_i[MAP_CONNECTION]].link_i = (S16)s32data[3];
                pstConnection[element_i[MAP_CONNECTION]].pavement_i = (S16)s32data[4];
                pstConnection[element_i[MAP_CONNECTION]].linkCount= (S8)s32data[5];
                pstConnection[element_i[MAP_CONNECTION]].pavementCount= (S8)s32data[6];
                pstConnection[element_i[MAP_CONNECTION]].type= (S8)s32data[7];
                pstConnection[element_i[MAP_CONNECTION]].road_count= (S8)s32data[8];
                pstConnection[element_i[MAP_CONNECTION]].road_i[0] = (S8)s32data[9];
                pstConnection[element_i[MAP_CONNECTION]].road_i[1] = (S8)s32data[10];
                pstConnection[element_i[MAP_CONNECTION]].road_i[2] = (S8)s32data[11];
                pstConnection[element_i[MAP_CONNECTION]].laneLinkCount = (S8)s32data[12];
                pstConnection[element_i[MAP_CONNECTION]].laneLink_i = (S16)s32data[13];
                pstConnection[element_i[MAP_CONNECTION]].road_i[3] = (S8)INVALID_VALUE;
                pstConnection[element_i[MAP_CONNECTION]].road_i[4] = (S8)INVALID_VALUE;

            }
            else if(s16ret == 16)
            {
                pstConnection[element_i[MAP_CONNECTION]].ID = s32data[0];
                pstConnection[element_i[MAP_CONNECTION]].s16angle = (S16)s32data[1];
                pstConnection[element_i[MAP_CONNECTION]].extrapoint_i = (S16)s32data[2];
                pstConnection[element_i[MAP_CONNECTION]].link_i = (S16)s32data[3];
                pstConnection[element_i[MAP_CONNECTION]].laneLink_i = (S16)s32data[4];
                pstConnection[element_i[MAP_CONNECTION]].pavement_i = (S16)s32data[5];
                pstConnection[element_i[MAP_CONNECTION]].linkCount= (S8)s32data[6];
                pstConnection[element_i[MAP_CONNECTION]].laneLinkCount = (S8)s32data[7];
                pstConnection[element_i[MAP_CONNECTION]].pavementCount= (S8)s32data[8];
                pstConnection[element_i[MAP_CONNECTION]].type= (S8)s32data[9];
                pstConnection[element_i[MAP_CONNECTION]].road_count= (S8)s32data[10];
                pstConnection[element_i[MAP_CONNECTION]].road_i[0] = (S8)s32data[11];
                pstConnection[element_i[MAP_CONNECTION]].road_i[1] = (S8)s32data[12];
                pstConnection[element_i[MAP_CONNECTION]].road_i[2] = (S8)s32data[13];
                pstConnection[element_i[MAP_CONNECTION]].road_i[3] = (S8)s32data[14];
                pstConnection[element_i[MAP_CONNECTION]].road_i[4] = (S8)s32data[15];

            }
            else
            {
                pl_log(ERR," get MAP_CONNECTION fail , s16ret_%d , '%s' ", s16ret,linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }


            element_i[MAP_CONNECTION]++;
            elementCurSize[MAP_CONNECTION] += sizeof(PF_MAP_CONNECTION_ST);
            if((element_i[MAP_CONNECTION] > gas32ElementCount[MAP_CONNECTION])||(elementCurSize[MAP_CONNECTION] > elementSize[MAP_CONNECTION]))
            {
                pl_log(ERR," MAP_CONNECTION overflow , element_i_%d , gas32ElementCount_%d , curSize_%d , elementSize_%d /n%s/n/n", 
                        element_i[MAP_CONNECTION],gas32ElementCount[MAP_CONNECTION],elementCurSize[MAP_CONNECTION],elementSize[MAP_CONNECTION],linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }
        }
        else if(0==strcmp("MAP_LINK" , tmpstr ))  
        {
            s16ret = sscanf(&(linestr[nextpos]), " %d  %d  %d ",&(s32data[0]),&(s32data[1]),&(s32data[2]));
            
            if(s16ret != 3)
            {
                pl_log(ERR," get MAP_LINK fail , s16ret_%d , '%s' ", s16ret,linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }

            pstLink[element_i[MAP_LINK]].type = (S16)s32data[0];
            pstLink[element_i[MAP_LINK]].fromSection_i = (S16)s32data[1];
            pstLink[element_i[MAP_LINK]].toSection_i = (S16)s32data[2];

            element_i[MAP_LINK]++;
            elementCurSize[MAP_LINK] += sizeof(PF_MAP_LINK_ST);
            if((element_i[MAP_LINK] > gas32ElementCount[MAP_LINK])||(elementCurSize[MAP_LINK] > elementSize[MAP_LINK]))
            {
                pl_log(ERR," MAP_LINK overflow , element_i_%d , gas32ElementCount_%d , curSize_%d , elementSize_%d /n%s/n/n", 
                        element_i[MAP_LINK],gas32ElementCount[MAP_LINK],elementCurSize[MAP_LINK],elementSize[MAP_LINK],linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }
        }
        else if(0==strcmp("MAP_LANE_LINK" , tmpstr ))  
        {
            s16ret = sscanf(&(linestr[nextpos]), " %d  %d  %d ",&(s32data[0]),&(s32data[1]),&(s32data[2]));
            
            if(s16ret != 3)
            {
                pl_log(ERR," get MAP_LANE_LINK fail , s16ret_%d , '%s' ", s16ret,linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }

            pstLaneLink[element_i[MAP_LANE_LINK]].type = (S16)s32data[0];
            pstLaneLink[element_i[MAP_LANE_LINK]].fromLane_i = (S16)s32data[1];
            pstLaneLink[element_i[MAP_LANE_LINK]].toLane_i = (S16)s32data[2];

            element_i[MAP_LANE_LINK]++;
            elementCurSize[MAP_LANE_LINK] += sizeof(PF_MAP_LINK_ST);
            if((element_i[MAP_LANE_LINK] > gas32ElementCount[MAP_LANE_LINK])||(elementCurSize[MAP_LANE_LINK] > elementSize[MAP_LANE_LINK]))
            {
                pl_log(ERR," MAP_LANE_LINK overflow , element_i_%d , gas32ElementCount_%d , curSize_%d , elementSize_%d /n%s/n/n", 
                        element_i[MAP_LANE_LINK],gas32ElementCount[MAP_LANE_LINK],elementCurSize[MAP_LANE_LINK],elementSize[MAP_LANE_LINK],linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }
        }
        else if(0==strcmp("MAP_SECTION" , tmpstr ))  
        {
            s16ret = sscanf(&(linestr[nextpos]), " %d  %d  %d  %d  %d  %d  %d  %d  %d ",
                    &(s32data[0]),&(s32data[1]),&(s32data[2]),&(s32data[3]),&(s32data[4]),
                    &(s32data[5]),&(s32data[6]),&(s32data[7]),&(s32data[8]));
            
            if(s16ret != 9)
            {
                pl_log(ERR," get MAP_SECTION fail , s16ret_%d , '%s' ", s16ret,linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }

            pstSection[element_i[MAP_SECTION]].ID = s32data[0];
            pstSection[element_i[MAP_SECTION]].fromConnection_i = (S16)s32data[1];
            pstSection[element_i[MAP_SECTION]].toConnection_i = (S16)s32data[2];
            pstSection[element_i[MAP_SECTION]].tracficLight_i = (S16)s32data[3];
            pstSection[element_i[MAP_SECTION]].lane_i = (S16)s32data[4];
            pstSection[element_i[MAP_SECTION]].tracficLightCount = (S8)s32data[5];
            pstSection[element_i[MAP_SECTION]].laneCount = (S8)s32data[6];
            pstSection[element_i[MAP_SECTION]].road_i = (S8)s32data[7];
            pstSection[element_i[MAP_SECTION]].type = (S8)s32data[8];

            element_i[MAP_SECTION]++;
            elementCurSize[MAP_SECTION] += sizeof(PF_MAP_SECTION_ST);
            if((element_i[MAP_SECTION] > gas32ElementCount[MAP_SECTION])||(elementCurSize[MAP_SECTION] > elementSize[MAP_SECTION]))
            {
                pl_log(ERR," MAP_SECTION overflow , element_i_%d , gas32ElementCount_%d , curSize_%d , elementSize_%d /n%s/n/n", 
                        element_i[MAP_SECTION],gas32ElementCount[MAP_SECTION],elementCurSize[MAP_SECTION],elementSize[MAP_SECTION],linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }
        }        
        else if(0==strcmp("MAP_LANE" , tmpstr ))  
        {
            s16ret = sscanf(&(linestr[nextpos]), " %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d ",
                    &(s32data[0]),&(s32data[1]),&(s32data[2]),&(s32data[3]),&(s32data[4]),&(s32data[5]),
                    &(s32data[6]),&(s32data[7]),&(s32data[8]),&(s32data[9]),&(s32data[10]),&(s32data[11]),&(s32data[12]));
            
            if(s16ret != 13)
            {
                pl_log(ERR," get MAP_LANE fail , s16ret_%d , '%s' ", s16ret,linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }

            pstLane[element_i[MAP_LANE]].ID = s32data[0];
            pstLane[element_i[MAP_LANE]].type = (S8)s32data[1];
            pstLane[element_i[MAP_LANE]].turnType = (S8)s32data[2];
            pstLane[element_i[MAP_LANE]].limitSpeedMax = (U8)s32data[3];
            pstLane[element_i[MAP_LANE]].limitSpeedMin = (U8)s32data[4];
            pstLane[element_i[MAP_LANE]].leftLanemarker_i = (S16)s32data[5];
            pstLane[element_i[MAP_LANE]].centreLanemarker_i = (S16)s32data[6];
            pstLane[element_i[MAP_LANE]].rightLanemarker_i = (S16)s32data[7];
            pstLane[element_i[MAP_LANE]].section_i = (S16)s32data[8];
            pstLane[element_i[MAP_LANE]].waitingLane_i = (S16)s32data[9];
            pstLane[element_i[MAP_LANE]].s32angle = s32data[10];
            pstLane[element_i[MAP_LANE]].laneNumber= (S16)s32data[11];
            pstLane[element_i[MAP_LANE]].roadSide= (S8)s32data[12];

            element_i[MAP_LANE]++;
            elementCurSize[MAP_LANE] += sizeof(PF_MAP_LANE_ST);
            if((element_i[MAP_LANE] > gas32ElementCount[MAP_LANE])||(elementCurSize[MAP_LANE] > elementSize[MAP_LANE]))
            {
                pl_log(ERR," MAP_LANE overflow , element_i_%d , gas32ElementCount_%d , curSize_%d , elementSize_%d /n%s/n/n", 
                        element_i[MAP_LANE],gas32ElementCount[MAP_LANE],elementCurSize[MAP_LANE],elementSize[MAP_LANE],linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }
        } 
        else if(0==strcmp("MAP_WAITING_LANE" , tmpstr ))  
        {
            s16ret = sscanf(&(linestr[nextpos]), " %d  %d  %d  %d ",
                    &(s32data[0]),&(s32data[1]),&(s32data[2]),&(s32data[3]));
            
            if(s16ret != 4)
            {
                pl_log(ERR," get MAP_WAITING_LANE fail , s16ret_%d , '%s' ", s16ret,linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }

            pstWaitingLane[element_i[MAP_WAITING_LANE]].lane_i = (S16)s32data[0];
            pstWaitingLane[element_i[MAP_WAITING_LANE]].leftLanemarker_i = (S16)s32data[1];
            pstWaitingLane[element_i[MAP_WAITING_LANE]].rightLanemarker_i = (S16)s32data[2];
            pstWaitingLane[element_i[MAP_WAITING_LANE]].centreLanemarker_i = (S16)s32data[3];

            element_i[MAP_WAITING_LANE]++;
            elementCurSize[MAP_WAITING_LANE] += sizeof(PF_MAP_WAITING_LANE_ST);
            if((element_i[MAP_WAITING_LANE] > gas32ElementCount[MAP_WAITING_LANE])||(elementCurSize[MAP_WAITING_LANE] > elementSize[MAP_WAITING_LANE]))
            {
                pl_log(ERR," MAP_WAITING_LANE overflow , element_i_%d , gas32ElementCount_%d , curSize_%d , elementSize_%d /n%s/n/n", 
                        element_i[MAP_WAITING_LANE],gas32ElementCount[MAP_WAITING_LANE],elementCurSize[MAP_WAITING_LANE],elementSize[MAP_WAITING_LANE],linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }
        }
        else if(0==strcmp("MAP_TRAFFICLIGHT" , tmpstr ))  
        {
            S32 lampGroupCount = 0;
            S32 sectionCount = 0;
        
                             // MAP_TRAFFICLIGHT  ID  start_x  start_y  end_x  end_y  lampGroupCount sectionCount
            s16ret = sscanf(&(linestr[nextpos]), " %d  %lx   %lx   %lx   %lx   %d   %d ",&(s32data[0]),
                    &(pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].start_utm_x),
                    &(pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].start_utm_y),
                    &(pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].end_utm_x),
                    &(pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].end_utm_y),
                    &lampGroupCount,&sectionCount);
            
            if(s16ret != 7)
            {
                pl_log(ERR," get MAP_TRAFFICLIGHT fail , s16ret_%d , '%s' ", s16ret,linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }

            pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].ID = s32data[0];
            pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].lampGroupCount = (S8)lampGroupCount;
            pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].sectionCount = (S8)sectionCount;


            stIFile.getline(linestr, 1023);

            s16ret = sscanf(linestr," %d   %d   %d   %d   %d  ",&(s32data[0]),&(s32data[1]),
                            &(s32data[2]),&(s32data[3]),&(s32data[4]));
                            
            if(s16ret != 5)
            {
                pl_log(ERR," trafficLight_%d  get  sectionIndexList  fail , s16ret_%d , '%s' ",
                        pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].ID, s16ret,linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }

            pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].sectionIndexList[0] = (S16)(s32data[0]);
            pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].sectionIndexList[1] = (S16)(s32data[1]);
            pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].sectionIndexList[2] = (S16)(s32data[2]);
            pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].sectionIndexList[3] = (S16)(s32data[3]);
            pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].sectionIndexList[4] = (S16)(s32data[4]);

            for(int j=0;j<lampGroupCount;j++)
            {         
                stIFile.getline(linestr, 1023);
                                       //index  utm_x  utm_y   entrance   light_type                              
                s16ret = sscanf(linestr, " %d   %lx   %lx   %d   %d ",&(s32data[0]),
                        &(pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].lamp_group[j].utm_x),
                        &(pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].lamp_group[j].utm_y),
                        &(s32data[1]),&(s32data[2]));

                if(s16ret != 5)
                {
                    pl_log(ERR," trafficLight_%d  get  lampGroup[%d]_1  fail , s16ret_%d , '%s' ",
                            pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].ID,j,s16ret,linestr);
                    s32ret = PF_MAP_ERR;
                    goto end;
                }

                pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].lamp_group[j].index = (S8)(s32data[0]);
                pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].lamp_group[j].entrance = (S8)(s32data[1]);
                pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].lamp_group[j].light_type = (S8)(s32data[2]);




                stIFile.getline(linestr, 1023);
                // turn_type  number  combination_type  countdown_number_width  independent_pole  is_waiting_area_light                             
                s16ret = sscanf(linestr, " %d   %d   %d   %d   %d   %d ",&(s32data[0]),
                        &(s32data[1]),&(s32data[2]),&(s32data[3]),&(s32data[4]),&(s32data[5]));
                
                if(s16ret != 6)
                {
                    pl_log(ERR," trafficLight_%d  get  lampGroup[%d]_2  fail , s16ret_%d , '%s' ",
                            pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].ID,j,s16ret,linestr);
                    s32ret = PF_MAP_ERR;
                    goto end;
                }
                
                pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].lamp_group[j].turn_type = (S8)(s32data[0]);
                pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].lamp_group[j].number = (S8)(s32data[1]);
                pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].lamp_group[j].combination_type = (S8)(s32data[2]);
                pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].lamp_group[j].countdown_number_width = (S8)(s32data[3]);
                pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].lamp_group[j].independent_pole = (S8)(s32data[4]);
                pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].lamp_group[j].is_waiting_area_light = (S8)(s32data[5]);



                
                stIFile.getline(linestr, 1023);
                           // bind_lane_count  lane_0  lane_1 lane_2  lane_3  lane_4  lane_5  
                s16ret = sscanf(linestr, " %d   %d   %d   %d   %d   %d  %d",&(s32data[0]),
                        &(s32data[1]),&(s32data[2]),&(s32data[3]),&(s32data[4]),&(s32data[5]),&(s32data[6]));

                if(s16ret != 7)
                {
                    pl_log(ERR," trafficLight_%d  get  lampGroup[%d]  bind_lane_index  fail , s16ret_%d , '%s' ",
                            pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].ID,j,s16ret,linestr);
                    s32ret = PF_MAP_ERR;
                    goto end;
                }
                
                pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].lamp_group[j].bind_lane_count = (S8)(s32data[0]);
                pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].lamp_group[j].bind_lane_index[0] = (S8)(s32data[1]);
                pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].lamp_group[j].bind_lane_index[1] = (S8)(s32data[2]);
                pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].lamp_group[j].bind_lane_index[2] = (S8)(s32data[3]);
                pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].lamp_group[j].bind_lane_index[3] = (S8)(s32data[4]);
                pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].lamp_group[j].bind_lane_index[4] = (S8)(s32data[5]);
                pstTrafficLight[element_i[MAP_TRAFFICLIGHT]].lamp_group[j].bind_lane_index[5] = (S8)(s32data[6]);
                
            }  
                
            element_i[MAP_TRAFFICLIGHT]++;
            elementCurSize[MAP_TRAFFICLIGHT] += sizeof(PF_MAP_TRAFFICLIGHT_ST);
            if((element_i[MAP_TRAFFICLIGHT] > gas32ElementCount[MAP_TRAFFICLIGHT])||(elementCurSize[MAP_TRAFFICLIGHT] > elementSize[MAP_TRAFFICLIGHT]))
            {
                pl_log(ERR," MAP_TRAFFICLIGHT overflow , element_i_%d , gas32ElementCount_%d , curSize_%d , elementSize_%d /n%s/n/n", 
                        element_i[MAP_TRAFFICLIGHT],gas32ElementCount[MAP_TRAFFICLIGHT],elementCurSize[MAP_TRAFFICLIGHT],elementSize[MAP_TRAFFICLIGHT],linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }
        }
        else if(0==strcmp("MAP_MILEAGE" , tmpstr ))  
        {
            CHAR* ptmpstr0 = NULL;
            CHAR* ptmpstr1 = NULL;

            s16ret = sscanf(&(linestr[nextpos]), " %lx  %lx  %lx  %d  %d  %d  %d ",
                    &(pstMileage[element_i[MAP_MILEAGE]].utm_x),
                    &(pstMileage[element_i[MAP_MILEAGE]].utm_y),
                    &(pstMileage[element_i[MAP_MILEAGE]].distance),
                    &(s32data[0]),&(s32data[1]),&(s32data[2]),&(s32data[3]));
            
            if(s16ret != 7)
            {
                pl_log(ERR," get MAP_MILEAGE fail , s16ret_%d , '%s' ", s16ret,linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }

            pstMileage[element_i[MAP_MILEAGE]].ID = s32data[0];
            pstMileage[element_i[MAP_MILEAGE]].number = s32data[1];
            pstMileage[element_i[MAP_MILEAGE]].road_i = (S8)s32data[2];
            pstMileage[element_i[MAP_MILEAGE]].side = (S8)s32data[3];
            
            ptmpstr0 = strstr(linestr,"\"");
            
            if(ptmpstr0 == NULL)
            {
                pl_log(ERR," get MAP_MILEAGE  content fail ,  '%s' ", linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }
            
            ptmpstr0++;
            ptmpstr1 = strstr(ptmpstr0,"\"");
                
            if(ptmpstr1 == NULL)
            {
                pl_log(ERR," get MAP_MILEAGE  content fail ,  '%s' ", linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }
            
            *ptmpstr1=0;
            if(strlen(ptmpstr0) > 0)
            {
                sprintf(pstMileage[element_i[MAP_MILEAGE]].content,"%s\0",ptmpstr0);
            }
                
            element_i[MAP_MILEAGE]++;
            elementCurSize[MAP_MILEAGE] += sizeof(PF_MAP_MILEAGE_ST);
            if((element_i[MAP_MILEAGE] > gas32ElementCount[MAP_MILEAGE])||(elementCurSize[MAP_MILEAGE] > elementSize[MAP_MILEAGE]))
            {
                pl_log(ERR," MAP_MILEAGE overflow , element_i_%d , gas32ElementCount_%d , curSize_%d , elementSize_%d /n%s/n/n", 
                        element_i[MAP_MILEAGE],gas32ElementCount[MAP_MILEAGE],elementCurSize[MAP_MILEAGE],elementSize[MAP_MILEAGE],linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }
        }    
        else if(0==strcmp("block" , tmpstr ))
        {
            PF_MAP_BLOCK_INFO_ST* pblock = NULL;
            S32 block_j = 0;
            S32 block_i = 0;
            
                                        // block  j  i  state  connectionCount  laneCount  parkingZone_i[0 , 1 , 2]  noRunZone_i[0 , 1 , 2]
            s16ret = sscanf(&(linestr[nextpos])," %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d",&block_j,
                            &block_i,&(s32data[0]),&(s32data[1]),&(s32data[2]),&(s32data[3]),&(s32data[4]),&(s32data[5]),&(s32data[6]),&(s32data[7]),&(s32data[8]));

            if(s16ret != 11)
            {
                pl_log(ERR," get block info fail , s16ret_%d , '%s' ", s16ret,linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }

            if((validblock_i >= gu32BlockValidCount)||(totalmemsize >= gu64BlockTotalSize))
            {
                pl_log(ERR," block overflow , validblock_i_%d , gu32BlockValidCount_%d , curSize_%ld , gu64BlockTotalSize_%ld /n%s/n/n", 
                        validblock_i,gu32BlockValidCount,totalmemsize,gu64BlockTotalSize,linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }

            if(validblock_i == 0)
            {
                totalmemsize = gu32BlockCount_x*gu32BlockCount_y*sizeof(PF_MAP_BLOCK_INFO_ST*);
            }

            pblock = (PF_MAP_BLOCK_INFO_ST*)((U8*)pastMapBlock + totalmemsize);
            
            pblock->blockSate = (S8)(s32data[0]);
            pblock->connectionCount = (S8)(s32data[1]);
            pblock->laneCount = (S16)(s32data[2]);
            pblock->parkingZone_i[0] = (S16)(s32data[3]);
            pblock->parkingZone_i[1] = (S16)(s32data[4]);
            pblock->parkingZone_i[2] = (S16)(s32data[5]);
            pblock->noRunZone_i[0] = (S16)(s32data[6]);
            pblock->noRunZone_i[1] = (S16)(s32data[7]);
            pblock->noRunZone_i[2] = (S16)(s32data[8]);
            
            pastMapBlock[gu32BlockCount_x*block_j+block_i] = pblock;
            
            totalmemsize += sizeof(PF_MAP_BLOCK_INFO_ST);

            for(int j=0;j<pblock->connectionCount;j++)
            {
                PF_BLOCK_CONNECTION_ST* pconnection = (PF_BLOCK_CONNECTION_ST*)((U8*)pastMapBlock + totalmemsize);

                stIFile.getline(linestr, 1023);
                
                                   // connectionId  extrapoint_i  nodeLine_i  nodeLineCount    
                s16ret = sscanf(linestr," %d  %d  %d  %d ",&(s32data[0]),&(s32data[1]),&(s32data[2]),&(s32data[3]));

                if(s16ret != 4)
                {
                    pl_log(ERR," block[%d][%d]  connection_%d , s16ret_%d , '%s' ",
                                block_j,block_i,j,s16ret,linestr);
                    s32ret = PF_MAP_ERR;
                    goto end;
                }

                pconnection->connection_i = (S16)(s32data[0]);
                pconnection->extrapoint_i = (S16)(s32data[1]);
                pconnection->nodeLine_i = (S16)(s32data[2]);
                pconnection->nodeLineCount = (S16)(s32data[3]);
                
                totalmemsize += sizeof(PF_BLOCK_CONNECTION_ST);

            }

            for(int j=0;j<pblock->laneCount;j++)
            {
                PF_BLOCK_LNAE_ST* plane = (PF_BLOCK_LNAE_ST*)((U8*)pastMapBlock + totalmemsize);
                
                stIFile.getline(linestr, 1023);

                           // laneId  leftLanemarker_i  leftNodeLine_i  leftNodeLineCount  rightLanemarker_i  rightNodeLine_i  rightNodeLineCount
                s16ret = sscanf(linestr," %d  %d  %d  %d  %d  %d  %d ",&(s32data[0]),&(s32data[1]),
                            &(s32data[2]),&(s32data[3]),&(s32data[4]),&(s32data[5]),&(s32data[6]));

                if(s16ret != 7)
                {
                    pl_log(ERR," block[%d][%d]  lane_%d , s16ret_%d , '%s' ",
                            block_j,block_i,j,s16ret,linestr);
                    s32ret = PF_MAP_ERR;
                    goto end;
                }
                
                plane->lane_i = (S16)(s32data[0]);
                plane->leftLanemarker_i = (S16)(s32data[1]);
                plane->leftNodeLine_i = (S16)(s32data[2]);
                plane->leftNodeLineCount = (S16)(s32data[3]);
                plane->rightLanemarker_i = (S16)(s32data[4]);
                plane->rightNodeLine_i = (S16)(s32data[5]);
                plane->rightNodeLineCount = (S16)(s32data[6]);
                
                totalmemsize += sizeof(PF_BLOCK_LNAE_ST);
            }

            validblock_i++;
            
        }
        else if(0==strcmp("over" , tmpstr ))
        {
            s32ret = PF_MAP_SUCCESS;

            if((validblock_i != gu32BlockValidCount)||(totalmemsize != gu64BlockTotalSize))
            {
                pl_log(ERR," block:  validblock_i_%d , gu32BlockValidCount_%d , curSize_%ld , gu64BlockTotalSize_%ld /n", 
                        validblock_i,gu32BlockValidCount,totalmemsize,gu64BlockTotalSize);
                s32ret = PF_MAP_ERR;
            }

        
            for(int i=0;i<MAP_ELEMENT_MAX;i++)
            {
                if((element_i[i] != gas32ElementCount[i])||(elementCurSize[i] != elementSize[i]))
                {
                    pl_log(ERR," elementType_%d:  element_i_%d , gas32ElementCount_%d , elementCurSize_%d , elementSize_%d ", 
                                i,element_i[i],gas32ElementCount[i], elementCurSize[i],elementSize[i]);

                    s32ret = PF_MAP_ERR;
                }

                totalmemsize += elementCurSize[i];
            }
        
            if(totalmemsize != gu64TotalMemSize)
            {
                pl_log(ERR," gu64TotalMemSize_%ld , totalmemsize_%ld \n",gu64TotalMemSize,totalmemsize);

                s32ret = PF_MAP_ERR;
            }
            
            if(s32ret == PF_MAP_SUCCESS)
            {
                pl_log(FATAL," %s over!  totalMemSize_%ld , blockMemSize_%d ", 
                                mapfilename,gu64TotalMemSize,gu64BlockTotalSize);
            }

            for(int i=0;i<MAP_ELEMENT_MAX;i++)
            {
                pl_log(FATAL," emement[%d]:   count_%d , MemSize_%d ", 
                                i,gas32ElementCount[i],elementSize[i]);
            }
            
            if(pstMap != NULL)
            {
                pl_log(FATAL," mapInfo:  ID_%d , name_%s , version_%s , creatTime_%s ", 
                                pstMap->mapId,pstMap->mapname,pstMap->version,pstMap->createTime);
                pl_log(FATAL," mapInfo:  prodecer_%s , sourceFile_%s , type_%d , utm_zone_%s ", 
                                pstMap->producer,pstMap->sourceFile,pstMap->type,pstMap->utm_zone);
                pl_log(FATAL," mapInfo:  left_%lf , right_%lf , top_%lf , bottom_%lf \n", 
                                pstMap->utm_x_min,pstMap->utm_x_max,pstMap->utm_y_max,pstMap->utm_y_min);

            }


        
            goto end;
        }
        else 
        {
            U8** pelement = NULL;
            PF_MAP_NODE_LINE_ST* pnode_new = NULL;
            DOUBLE* pdistance = NULL;

            CHAR elementName[32] = {0};
            S32  elementType = 0;

            if(0==strcmp("MAP_EXTRAPOINT" , tmpstr ))
            {
                pelement = (U8**)pastExtrapoint;
                elementType = MAP_EXTRAPOINT;
                sprintf(elementName,"%s\0","MAP_EXTRAPOINT");
            }
            else if(0==strcmp("MAP_LANEMARKER" , tmpstr ))
            {
                pelement = (U8**)pastLanemarker;
                elementType = MAP_LANEMARKER;
                sprintf(elementName,"%s\0","MAP_LANEMARKER");
            }
            else if(0==strcmp("MAP_PAVEMENT" , tmpstr ))
            {
                pelement = (U8**)pastPavement;
                elementType = MAP_PAVEMENT;
                sprintf(elementName,"%s\0","MAP_PAVEMENT");
            }
            else if(0==strcmp("MAP_PARKING_ZONE" , tmpstr ))
            {
                pelement = (U8**)pastParkingZone;
                elementType = MAP_PARKING_ZONE;
                sprintf(elementName,"%s\0","MAP_PARKING_ZONE");
            }
            else if(0==strcmp("MAP_NO_RUN_ZONE" , tmpstr ))
            {
                pelement = (U8**)pastNoRunZone;
                elementType = MAP_NO_RUN_ZONE;
                sprintf(elementName,"%s\0","MAP_NO_RUN_ZONE");
            }
            else if(0==strcmp("MAP_CENTER_LINE" , tmpstr ))
            {
                pelement = (U8**)pastDividerLine;
                elementType = MAP_CENTER_LINE;
                sprintf(elementName,"%s\0","MAP_CENTER_LINE");
            }
            else if(0==strcmp("MAP_ROADEDGE" , tmpstr ))
            {
                pelement = (U8**)pastRoadEdge;
                elementType = MAP_ROADEDGE;
                sprintf(elementName,"%s\0","MAP_ROADEDGE");
            }
            else
            {
                continue;
            }


            if(elementType == MAP_NO_RUN_ZONE)
            {
                s16ret = sscanf(&(linestr[nextpos]), " %d  %d  %d  %d  %d  %d  %d", &(s32data[0]),&(s32data[1]),
                                &(s32data[2]),&(s32data[3]),&(s32data[4]),&(s32data[5]),&(s32data[6]));

                if(s16ret != 7)
                {
                    pl_log(ERR," '%s' , s16ret_%d ",linestr,s16ret);
                    s32ret = PF_MAP_ERR;
                    goto end;
                }

            
                PF_MAP_NORUNZONE_INFO_ST* pnoRunZone = (PF_MAP_NORUNZONE_INFO_ST*)(pelement[element_i[elementType]]);
                pnode_new = (PF_MAP_NODE_LINE_ST*)((U8*)pnoRunZone + sizeof(PF_MAP_NORUNZONE_INFO_ST));

                pnoRunZone->ID = s32data[0];
                pnoRunZone->nodeCount = (S16)(s32data[1]);
                pnoRunZone->type = (S8)(s32data[2]);
                pnoRunZone->insideMode = (S8)(s32data[3]);
                pnoRunZone->road_i = s32data[4];
                pnoRunZone->angle = (S16)(s32data[5]);
                pnoRunZone->roadSide = (S16)(s32data[6]);

                if(element_i[elementType] == 0)
                {
                    elementCurSize[elementType] += gas32ElementCount[elementType]*sizeof(PF_MAP_NORUNZONE_INFO_ST*);
                }
                elementCurSize[elementType] += sizeof(PF_MAP_NORUNZONE_INFO_ST);
            } 
            else
            {
                s16ret = sscanf(&(linestr[nextpos]), " %d  %d  %d  %d ", &(s32data[0]),&(s32data[1]),&(s32data[2]),&(s32data[3]));

                if(s16ret != 4)
                {
                    pl_log(ERR," '%s' , s16ret_%d ",linestr,s16ret);
                    s32ret = PF_MAP_ERR;
                    goto end;
                }


                if((elementType == MAP_EXTRAPOINT)||(elementType == MAP_LANEMARKER)||
                    (elementType == MAP_PARKING_ZONE)||(elementType == MAP_CENTER_LINE))
                {
                    PF_MAP_LINE_INFO_ST* plineinfo = (PF_MAP_LINE_INFO_ST*)(pelement[element_i[elementType]]);
                    pnode_new = (PF_MAP_NODE_LINE_ST*)((U8*)plineinfo + sizeof(PF_MAP_LINE_INFO_ST));

                    if(elementType == MAP_CENTER_LINE)
                    {
                        pdistance = (DOUBLE*)(pnode_new+s32data[1]);
                    }

                    plineinfo->ID = s32data[0];
                    plineinfo->nodeCount = (S16)(s32data[1]);
                    plineinfo->type = (S8)(s32data[2]);
                    plineinfo->insideMode = (S8)(s32data[3]);
                }
                else if(elementType == MAP_PAVEMENT)
                {
                    PF_MAP_PAVEMENT_INFO_ST* ppavement = (PF_MAP_PAVEMENT_INFO_ST*)(pelement[element_i[elementType]]);
                    pnode_new = (PF_MAP_NODE_LINE_ST*)((U8*)ppavement + sizeof(PF_MAP_PAVEMENT_INFO_ST));

                    ppavement->ID = s32data[0];
                    ppavement->nodeCount = (S8)(s32data[1]);
                    ppavement->section_i = (S16)(s32data[2]);
                    ppavement->insideMode = (S8)(s32data[3]);
                }    
                else if(elementType == MAP_ROADEDGE)
                {
                    PF_MAP_ROADEDGE_INFO_ST* proadedge = (PF_MAP_ROADEDGE_INFO_ST*)(pelement[element_i[elementType]]);
                    pnode_new = (PF_MAP_NODE_LINE_ST*)((U8*)proadedge + sizeof(PF_MAP_ROADEDGE_INFO_ST));

                    proadedge->ID = s32data[0];
                    proadedge->nodeCount = (S16)(s32data[1]);
                    proadedge->acrossType = (S8)(s32data[2]);
                    proadedge->type = (S8)(s32data[3]);
                }    

                if(element_i[elementType] == 0)
                {
                    elementCurSize[elementType] += gas32ElementCount[elementType]*sizeof(PF_MAP_LINE_INFO_ST*);
                }
                elementCurSize[elementType] += sizeof(PF_MAP_LINE_INFO_ST);

            }



            for(int j=0;j<s32data[1];j++)
            {
                stIFile.getline(linestr, 1023);

                if(elementType == MAP_CENTER_LINE)
                {
                    s16ret = sscanf(linestr," %lx  %lx  %lx  %lx  %lx ",&(pnode_new->node_x),&(pnode_new->node_y),&(pnode_new->k),&(pnode_new->b),pdistance);

                    if(s16ret != 5)
                    {
                        pl_log(ERR," %s_%d  nodeCount_%d , get node[%d] info fail , s16ret_%d , '%s' ",
                                    elementName,s32data[0],s32data[1],j,s16ret,linestr);
                        s32ret = PF_MAP_ERR;
                        goto end;
                    }
                    pnode_new++;
                    pdistance++;
                    elementCurSize[elementType] += (sizeof(PF_MAP_NODE_LINE_ST)+sizeof(DOUBLE));
                }
                else
                {
                    s16ret = sscanf(linestr," %lx  %lx  %lx  %lx ",&(pnode_new->node_x),&(pnode_new->node_y),&(pnode_new->k),&(pnode_new->b));

                    if(s16ret != 4)
                    {
                        pl_log(ERR," %s_%d  nodeCount_%d , get node[%d] info fail , s16ret_%d , '%s' ",
                                    elementName,s32data[0],s32data[1],j,s16ret,linestr);
                        s32ret = PF_MAP_ERR;
                        goto end;
                    }
                    pnode_new++;
                    elementCurSize[elementType] += sizeof(PF_MAP_NODE_LINE_ST);
                }
            }
            
            element_i[elementType]++;
            
            if((element_i[elementType] > gas32ElementCount[elementType])||(elementCurSize[elementType] > elementSize[elementType]))
            {
                pl_log(ERR," %s overflow , element_i_%d , gas32ElementCount_%d , curSize_%d , elementSize_%d \n%s\n\n",elementName, 
                        element_i[elementType],gas32ElementCount[elementType],elementCurSize[elementType],elementSize[elementType],linestr);
                s32ret = PF_MAP_ERR;
                goto end;
            }

            if(element_i[elementType] < gas32ElementCount[elementType])
            {
                pelement[element_i[elementType]] = ((U8*)pelement + elementCurSize[elementType]);
            }
        }

    }
    
    s32ret = PF_MAP_ERR;
    pl_log(ERR," pf_map_block_file_read: %s fail ",  mapfilename);

end:

    stIFile.close();

    gStrUtmZone = pstMap->utm_zone;

    return s32ret;
}


S32 pf_map_check_nodeline_side(DOUBLE pos_x,DOUBLE pos_y,PF_MAP_NODE_LINE_ST* pnode_start,PF_MAP_NODE_LINE_ST* pnode_end,S32 direction_x,DOUBLE* distance,S32 unlimited)
{
    DOUBLE line_start = 0;
    DOUBLE line_end = 0;
    DOUBLE line_test = 0;
    DOUBLE pos_test = 0;
    DOUBLE dbdistance = 0;

    DOUBLE dbx0 = 0;
    DOUBLE dbx1 = 0;
    DOUBLE dby0 = 0;
    DOUBLE dby1 = 0;

    S32 s32ret = PF_MAP_ERR;

    if((pnode_start == NULL)||(pnode_end == NULL))
    {
        pl_log(ERR," pnode_start or pnode_end is NULL ");
        return PF_MAP_ERR;
    }

    dbx0 = pnode_start->node_x;
    dby0 = pnode_start->node_y;

    dbx1 = pnode_end->node_x;
    dby1 = pnode_end->node_y;


    if(dbx0 < dbx1)
    {
        dbx0 -= DB_OFFSET;
        dbx1 += DB_OFFSET;
    }
    else
    {
        dbx0 += DB_OFFSET;
        dbx1 -= DB_OFFSET;
    }

    if(dby0 < dby1)
    {
        dby0 -= DB_OFFSET;
        dby1 += DB_OFFSET;
    }
    else
    {
        dby0 += DB_OFFSET;
        dby1 -= DB_OFFSET;
    }


    if(pnode_start->k == 0)
    {
        if(pnode_start->b == 0)
        {
            if((((pos_x >= dbx0)&&(pos_x <= dbx1))||((pos_x <= dbx0)&&(pos_x >= dbx1)))&&
                ((unlimited == 1)||(((pos_y >= dby0)&&(pos_y <= dby1))||((pos_y <= dby0)&&(pos_y >= dby1)))))
            {
                return ON_LINE;
            }
            
            if((direction_x == 0)&&(((pos_y >= dby0)&&(pos_y <= dby1))||((pos_y <= dby0)&&(pos_y >= dby1))||(unlimited == 1)))
            {
                line_test = pnode_start->node_x;
                pos_test = pos_x;
                line_start = dby0;
                line_end = dby1;
            }
            else
            {
                return PF_MAP_ERR;
            }
        }
        else 
        {
            if(((unlimited == 1)||(((pos_x >= dbx0)&&(pos_x <= dbx1))||((pos_x <= dbx0)&&(pos_x >= dbx1))))&&
                (((pos_y >= dby0)&&(pos_y <= dby1))||((pos_y <= dby0)&&(pos_y >= dby1))))
            {
                return ON_LINE;
            }
            
            if((direction_x == 1)&&(((pos_x >= dbx0)&&(pos_x <= dbx1))||((pos_x <= dbx0)&&(pos_x >= dbx1))||(unlimited == 1)))
            {
                line_test = pnode_start->b;
                pos_test = pos_y;
                line_start = dbx0;
                line_end = dbx1;
            }
            else
            {
                return PF_MAP_ERR;
            }
        }
    }
    else 
    {
        if(direction_x == 1)
        {
            if(((pos_x >= dbx0)&&(pos_x <= dbx1))||((pos_x <= dbx0)&&(pos_x >= dbx1))||(unlimited == 1))
            {
                line_test = pnode_start->k*pos_x + pnode_start->b;
                pos_test = pos_y;
                line_start = dbx0;
                line_end = dbx1;

#ifdef GTEST_EN
if(gdebugflag == 1)
{
pf_map_log("\r\n y = %lf * %lf + %lf  = %lf , y0_%lf , ",pnode_start->k,pos_x,pnode_start->b,line_test,pos_test);
}
#endif
                
            }
            else
            {
                return PF_MAP_ERR;
            }
        }
        else
        {
            if(((pos_y >= dby0)&&(pos_y <= dby1))||((pos_y <= dby0)&&(pos_y >= dby1))||(unlimited == 1))
            {
                line_test = (pos_y-pnode_start->b)/pnode_start->k;
                pos_test = pos_x;
                line_start = dby0;
                line_end = dby1;

#ifdef GTEST_EN
if(gdebugflag == 1)
{
pf_map_log("\r\n x = (%lf-%lf)/%lf = %lf , x0_%lf , ",pos_y,pnode_start->b,pnode_start->k,line_test,pos_test);

}
#endif                
            }
            else
            {
                return PF_MAP_ERR;
            }
        }
    }

    if(pos_test < line_test)
    {
        dbdistance = line_test - pos_test;
    }
    else
    {
        dbdistance = pos_test - line_test;
    }


    *distance = dbdistance;

    if(dbdistance <= ONLINE_OFFSET)
    {

#ifdef GTEST_EN
if(gdebugflag == 1)
{
pf_map_log(" dbdistance_%lf , ON_LINE \n\n ",dbdistance);
}
#endif

        return ON_LINE;
    }
    else if(direction_x == 1)
    {
        if(((line_start <= line_end)&&(line_test < pos_test))||
            ((line_start >= line_end)&&(line_test > pos_test)))
        {
            s32ret = LEFT_SIDE;
        }
        else
        {
            s32ret = RIGHT_SIDE;
        }
    }
    else
    {
        if(((line_start <= line_end)&&(line_test > pos_test))||
            ((line_start >= line_end)&&(line_test < pos_test)))
        {
            s32ret = LEFT_SIDE;
        }
        else
        {
            s32ret = RIGHT_SIDE;
        }
    }

#ifdef GTEST_EN
if(gdebugflag == 1)
{
    pf_map_log(" dbdistance_%lf , s32ret_%d  ",dbdistance,s32ret);
}
#endif
    
    return s32ret;
}



S32 pf_map_check_line_side(DOUBLE pos_x,DOUBLE pos_y,PF_MAP_NODE_LINE_ST* pnode0,S32 lineID,S32 nodeCount,
                                S16 line_i,S16 linecount, DOUBLE* pdbinstance, S32 ispolygon)
{
    DOUBLE distance_min = 0;
    DOUBLE dbdistance = 0;
    S32    lineside = PF_MAP_ERR;
    S32    s32ret = PF_MAP_ERR;
    U32    tmpoffset = 0;
    S32    i = 0;
    S32    j = 0;
    
    PF_MAP_NODE_LINE_ST* pnode_cur = NULL;
    PF_MAP_NODE_LINE_ST* pnode_next = NULL;
        
    if((line_i < 0)||(line_i >= nodeCount)||(linecount > nodeCount))
    {
        pl_log(ERR," lineID_%d: nodecount_%d  can not match  line_i_%d , linecount_%d ",
                     lineID,nodeCount,line_i,linecount);
        return PF_MAP_ERR;
    }
    
    for( j=0;j<2;j++ )
    {
        s32ret = PF_MAP_ERR;
        
        tmpoffset = line_i;
        
        for( i=0;i<linecount;i++ )
        {
            pnode_cur = pnode0 + tmpoffset;
            
            if((i+line_i+1) == nodeCount)
            {
                tmpoffset = 0;
            }
            else
            {
                tmpoffset++;
            }
            
            pnode_next = pnode0 + tmpoffset;

            s32ret = pf_map_check_nodeline_side(pos_x,pos_y,pnode_cur,pnode_next,j,&dbdistance,0);
#ifdef GTEST_EN
if(gdebugflag == 1)
{
    pf_map_log("\n lineID_%d, j_%d , i_%d , line_i_%d , linecount_%d , nodecount_%d , dbdistance_%lf , s32ret_%d \n",
                lineID,j,i,line_i,linecount,nodeCount,dbdistance,s32ret);
}
#endif

            if(s32ret == ON_LINE)
            {
                *pdbinstance = 0;
                return s32ret;
            }

            if((s32ret > ON_LINE)&&((dbdistance < distance_min)||(distance_min == 0)))
            {
                lineside = s32ret;
                distance_min = dbdistance;
            }

        }


    }

    if(lineside == PF_MAP_ERR)
    {
        DOUBLE tmpdb = -1;
        S32    node_i = 0;

        PF_MAP_NODE_LINE_ST* pnode_pre = NULL;
        
        distance_min = -1;
        
        tmpoffset = line_i;
        pnode_cur = pnode0 + line_i;
        j = line_i;
        
        for( i=0;i<=linecount;i++ )
        {
            tmpdb = (pnode_cur->node_x - pos_x)*(pnode_cur->node_x - pos_x)+(pnode_cur->node_y - pos_y)*(pnode_cur->node_y - pos_y);

            if((tmpdb < distance_min)||(distance_min == -1))
            {
                distance_min = tmpdb;
                node_i = j;
            }

            if(j < nodeCount-1)
            {
                pnode_cur++;
                j++;
            }
            else
            {
                pnode_cur = pnode0;
                j = 0;
            }
        }


        pnode_cur = pnode0 + node_i;

        if(node_i < nodeCount-1)
        {
            pnode_next = pnode0 + node_i + 1;
        }
        else if(ispolygon == 1)
        {
            pnode_next = pnode0;
        }
        else
        {
            pnode_next = pnode_cur;
        }

        if(node_i > 0)
        {
            pnode_pre = pnode0 + node_i - 1;
        }
        else if(ispolygon == 1)
        {
            pnode_pre = pnode0 + nodeCount - 1;
        }
        else
        {
            pnode_pre = pnode_cur;
        }

        // y3 = kx3 + b = ((x3-x1)*y2 + (x2-x3)*y1)/(x2-x1)
        tmpdb = ((pos_x - pnode_pre->node_x)*pnode_next->node_y + (pnode_next->node_x - pos_x)*pnode_pre->node_y)/(pnode_next->node_x-pnode_pre->node_x);

        if(pnode_next->node_x > pnode_pre->node_x)
        {
            if(pos_y < tmpdb)
            {
                lineside = RIGHT_SIDE;
            }
            else if(pos_y > tmpdb)
            {
                lineside = LEFT_SIDE;
            }
        }
        else if(pnode_next->node_x < pnode_pre->node_x)
        {
            if(pos_y > tmpdb)
            {
                lineside = RIGHT_SIDE;
            }
            else if(pos_y < tmpdb)
            {
                lineside = LEFT_SIDE;
            }
        }
        else if(pnode_next->node_y > pnode_pre->node_y)
        {
            if(pos_x < pnode_pre->node_x)
            {
                lineside = LEFT_SIDE;
            }
            else if(pos_x > pnode_pre->node_x)
            {
                lineside = RIGHT_SIDE;
            }
        }
        else if(pnode_next->node_y < pnode_pre->node_y)
        {
            if(pos_x > pnode_pre->node_x)
            {
                lineside = LEFT_SIDE;
            }
            else if(pos_x < pnode_pre->node_x)
            {
                lineside = RIGHT_SIDE;
            }
        }
    }

    *pdbinstance = distance_min;
    return lineside;
}




S32 pf_map_check_utm_in_polygon(DOUBLE utm_x,DOUBLE utm_y, S32 type ,S32 index, DOUBLE* distance)
{
    PF_MAP_NODE_LINE_ST* pnode = NULL;
    S32 lineID = 0;
    S32 nodecount = 0;
    S32 insidemode = 0;
    S32 s32ret = 0;
    S32 i = 0;
    
    DOUBLE dbx_min = 0;
    DOUBLE dby_min = 0;
    DOUBLE dbx_max = 0;
    DOUBLE dby_max = 0;
    DOUBLE dbx = 0;
    DOUBLE dby = 0;
    
    if(type == MAP_EXTRAPOINT)
    {
        lineID = pastExtrapoint[index]->ID;
        nodecount = pastExtrapoint[index]->nodeCount;
        insidemode = pastExtrapoint[index]->insideMode;
        pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastExtrapoint[index]) + LINE_INFO_HEAD_LEN);
    }
    else if(type == MAP_PAVEMENT)
    {
        lineID = pastPavement[index]->ID;
        nodecount = pastPavement[index]->nodeCount;
        insidemode = pastPavement[index]->insideMode;
        pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastPavement[index]) + sizeof(PF_MAP_PAVEMENT_INFO_ST));
    }
    else if(type == MAP_PARKING_ZONE)
    {
        lineID = pastParkingZone[index]->ID;
        nodecount = pastParkingZone[index]->nodeCount;
        insidemode = pastParkingZone[index]->insideMode;
        pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastParkingZone[index]) + LINE_INFO_HEAD_LEN);
    }
    else if(type == MAP_NO_RUN_ZONE)
    {
        lineID = pastNoRunZone[index]->ID;
        nodecount = pastNoRunZone[index]->nodeCount;
        insidemode = pastNoRunZone[index]->insideMode;
        pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastNoRunZone[index]) + sizeof(PF_MAP_NORUNZONE_INFO_ST));
    }

    dbx_min = pnode[0].node_x;
    dbx_max = pnode[0].node_x;
    dby_min = pnode[0].node_y;
    dby_max = pnode[0].node_y;
    
    for(i=1; i<nodecount; i++)
    {
        if(pnode[i].node_x < dbx_min)
        {
            dbx_min = pnode[i].node_x;
        }
        else if(pnode[i].node_x > dbx_max) 
        {
            dbx_max = pnode[i].node_x;
        }

        if(pnode[i].node_y < dby_min)
        {
            dby_min = pnode[i].node_y;
        }
        else if(pnode[i].node_y > dby_max) 
        {
            dby_max = pnode[i].node_y;
        }
    }

    if((utm_x < dbx_min)||(utm_x > dbx_max)||(utm_y < dby_min)||(utm_y > dby_max))
    {
        if(distance!= NULL)
        {
            s32ret = pf_map_check_line_side(utm_x,utm_y,pnode,lineID,nodecount,0,nodecount,&dbx,1);
        
            *distance = dbx;
        }
        
        return 0;
    }


    s32ret = pf_map_check_line_side(utm_x,utm_y,pnode,lineID,nodecount,0,nodecount,&dbx,1);

    if(distance!= NULL)
    {
        *distance = dbx;
    }
    
    if((insidemode == s32ret)||(s32ret == ON_LINE))
    {
        return 1;
    }

    return 0;

}


S32 pf_map_get_connection_lane_i(S32 connection_i,DOUBLE utm_x, S32 utm_y)
{
    S32 s32ret = 0;
    S32 s32rightside = 0;
    S32 s32leftside = 0;
    S32 s32dir_x = 0;
    S32 link_i = 0;
    S32 lane_i = -1;
    S32 lanemarker_i = 0;
    DOUBLE dbdistance = 0;
    DOUBLE dbdistanceMin = 1000;
    PF_MAP_NODE_LINE_ST* pnode = NULL;
    
    if((connection_i<0)||(connection_i >= gas32ElementCount[MAP_CONNECTION]))
    {
        pl_log(ERR," connection_i_%d  is invalid , connectionCount_%d",connection_i,gas32ElementCount[MAP_CONNECTION]);
        return PF_MAP_ERR;
    }

    if(pstConnection[connection_i].s16angle > 180 )
    {
//        pl_log(ERR," connection[%d]_ID_%d , s32angle_%d",connection_i,pstConnection[connection_i].ID,pstConnection[connection_i].s16angle);
        return PF_MAP_ERR;
    }
    
    if(((pstConnection[connection_i].s16angle > 45 )&&(pstConnection[connection_i].s16angle < 135 ))||
        ((pstConnection[connection_i].s16angle < -45 )&&(pstConnection[connection_i].s16angle > -135 )))
    {
        s32dir_x = 1;
    }

    for(int i=0;i<pstConnection[connection_i].linkCount;i++)
    {
        link_i = pstConnection[connection_i].link_i;
        if( pstLink[link_i+i].type == LINK_STRAIGHT)
        {
            for(int k=0;k<pstSection[pstLink[link_i+i].fromSection_i].laneCount;k++)
            {
                lanemarker_i = pstLane[pstSection[pstLink[link_i+i].fromSection_i].lane_i+k].leftLanemarker_i;
                
                pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastLanemarker[lanemarker_i])+sizeof(PF_MAP_LINE_INFO_ST));
                pnode = pnode + (pastLanemarker[lanemarker_i]->nodeCount-2);

                s32ret = pf_map_check_nodeline_side(utm_x,utm_y,pnode,pnode+1,s32dir_x,&dbdistance,1);
                if(s32ret == ON_LINE)
                {
                    return pstSection[pstLink[link_i+i].fromSection_i].lane_i+k;
                }
                else if(s32ret == RIGHT_SIDE)
                {
                    s32leftside = 1;
                }
                else if(s32ret == LEFT_SIDE)
                {
                    if(dbdistanceMin > dbdistance)
                    {
                        lane_i = pstSection[pstLink[link_i+i].fromSection_i].lane_i+k;
                        dbdistanceMin = dbdistance;
                    }
                    break;
                }
                else
                {
                    continue;
                }
                
                lanemarker_i = pstLane[pstSection[pstLink[link_i+i].fromSection_i].lane_i+k].rightLanemarker_i;
                
                pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastLanemarker[lanemarker_i])+sizeof(PF_MAP_LINE_INFO_ST));
                pnode = pnode + (pastLanemarker[lanemarker_i]->nodeCount-2);

                s32ret = pf_map_check_nodeline_side(utm_x,utm_y,pnode,pnode+1,s32dir_x,&dbdistance,1);
                if(s32ret == ON_LINE)
                {
                    return pstSection[pstLink[link_i+i].fromSection_i].lane_i+k;
                }
                else if(s32ret == LEFT_SIDE)
                {
                    s32rightside= 1;
                }
                else
                {
                    s32rightside = 0;
                }

                if((s32leftside == 1)&&(s32rightside == 1))
                {
                    return pstSection[pstLink[link_i+i].fromSection_i].lane_i+k;
                }
            }
        }
    }

    if(lane_i >= 0)
    {
        return lane_i;
    }
    
    return PF_MAP_ERR;
}



#if  0
S32 pf_map_block_get_index(DOUBLE utm_x, DOUBLE utm_y)
{
    S32 s32ret = PF_MAP_ERR;
    S32 blocki = S32((utm_x - gdbUtmOffset_x)/gu32Block_w);
    S32 blockj = S32((utm_y - gdbUtmOffset_y)/gu32Block_h);

    if((blocki < 0)||(blocki >= gu32BlockCount_x)||(blockj < 0)||(blockj >= gu32BlockCount_y))
    {
        pl_log(ERR," block[%d][%d] is invalid, blockcountmax[%d][%d] ",
                    blockj,blocki,gu32BlockCount_y,gu32BlockCount_x);
        return PF_MAP_ERR;
    }

    for(int i=0;i<gu32BlockValidCount;i++)
    {
        if(pastMapBlock[i]->block_y_j < blockj)
        {
            continue;
        }
        else if(pastMapBlock[i]->block_y_j > blockj)
        {
            return PF_MAP_ERR;
        }

        if(pastMapBlock[i]->block_x_i < blocki)
        {
            continue;
        }
        else if(pastMapBlock[i]->block_x_i > blocki)
        {
            return PF_MAP_ERR;

        }
        
        return i;
    }
}
#endif



S32 pf_map_block_get_laneID(DOUBLE utm_x, DOUBLE utm_y, S32* laneID, S32* connectionID)
{
    S32 i = 0;
    S32 j = 0;
    S32 block_i = 0;

    U32 tmpoffset = 0;
    S32 lanemarkerstate[64][2] = {0};
    DOUBLE lanemarkerdistance[64] = {0};

    DOUBLE dbdistance = 0;

    S32 s32laneID = -1;

    block_i = gu32BlockCount_x*(U32((utm_y - gdbUtmOffset_y)/gu32Block_h)) + U32((utm_x - gdbUtmOffset_x)/gu32Block_w);

    if(block_i < 0)
    {
        return PF_MAP_ERR;
    }

    PF_MAP_BLOCK_INFO_ST* pmapblock = pastMapBlock[block_i];
    
    if(pmapblock == NULL)
    {
        pl_log(ERR," pmapblock is NULL ");
        return PF_MAP_ERR;
    }

    tmpoffset = sizeof(PF_MAP_BLOCK_INFO_ST);

    if((pmapblock->connectionCount == 1)&&(pmapblock->laneCount == 0))
    {
        PF_BLOCK_CONNECTION_ST* pconnection = (PF_BLOCK_CONNECTION_ST*)((U8*)pmapblock + tmpoffset);

        if(pconnection->nodeLineCount == 0)
        {

            *laneID = -1;
            *connectionID = pstConnection[pconnection->connection_i].ID;
            return PF_MAP_CONNECTION;
        }
    }
        

    for(i=0;i<pmapblock->connectionCount;i++)
    {
        PF_BLOCK_CONNECTION_ST* pconnection = (PF_BLOCK_CONNECTION_ST*)((U8*)pmapblock + tmpoffset);
        PF_MAP_NODE_LINE_ST* pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastExtrapoint[pconnection->extrapoint_i]) + sizeof(PF_MAP_LINE_INFO_ST));

        S32 lineside = pf_map_check_line_side(utm_x,utm_y,pnode,pastExtrapoint[pconnection->extrapoint_i]->ID,
                                                pastExtrapoint[pconnection->extrapoint_i]->nodeCount,
                                                pconnection->nodeLine_i,pconnection->nodeLineCount,&dbdistance,1);

#ifdef GTEST_EN
if(gdebugflag == 1)
{
pf_map_log("\r\nconnectionID_%d , inside = %d , lineside_%d  , dbdistance_%lf \r\n\r\n",
pstConnection[pconnection->connection_i].ID,pastExtrapoint[pconnection->extrapoint_i]->insideMode,lineside,dbdistance);
}
#endif
        
        if((pastExtrapoint[pconnection->extrapoint_i]->insideMode == lineside)||(lineside == ON_LINE))
        {
            *laneID = -1;
            *connectionID = pstConnection[pconnection->connection_i].ID;

//pf_map_log("\r\n (%lf,%lf) , connectionID_%d , insidemode_%d , lineside_%d \r\n\r\n",
//            utm_x,utm_y,pconnection->ID,pconnection->insideMode,lineside);

            return PF_MAP_CONNECTION;
        }

        tmpoffset += sizeof(PF_BLOCK_CONNECTION_ST);
    }

    dbdistance = -1;
    
    for(i=0;i<pmapblock->laneCount;i++)
    {
        PF_BLOCK_LNAE_ST* plane = (PF_BLOCK_LNAE_ST*)((U8*)pmapblock + tmpoffset);
        S32 leftlanemarker_state = -1;
        S32 rightlanemarker_state = -1;
        DOUBLE dbdistance_left = -1;
        DOUBLE dbdistance_right = -1;

        for(j=0;j<64;j++)
        {
            if(lanemarkerstate[j][0] == 0)
            {
                break;
            }
            else if((plane->rightNodeLineCount > 0)&&(lanemarkerstate[j][0] == pastLanemarker[plane->rightLanemarker_i]->ID))
            {
                rightlanemarker_state = lanemarkerstate[j][1];
                dbdistance_right = lanemarkerdistance[j];
            }
            else if((plane->leftNodeLineCount > 0)&&(lanemarkerstate[j][0] == pastLanemarker[plane->leftLanemarker_i]->ID))
            {
                leftlanemarker_state = lanemarkerstate[j][1];
                dbdistance_left= lanemarkerdistance[j];
            }
        }

        if(leftlanemarker_state == -1)
        {
            if(plane->leftNodeLineCount == 0)
            {
                leftlanemarker_state = RIGHT_SIDE;
                lanemarkerdistance[j] = -1;
            }
            else
            {
                PF_MAP_NODE_LINE_ST* pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastLanemarker[plane->leftLanemarker_i]) + sizeof(PF_MAP_LINE_INFO_ST));
            
                leftlanemarker_state = pf_map_check_line_side(utm_x,utm_y,pnode,pastLanemarker[plane->leftLanemarker_i]->ID,
                                                    pastLanemarker[plane->leftLanemarker_i]->nodeCount,
                                                    plane->leftNodeLine_i,plane->leftNodeLineCount,&dbdistance_left,0);

                lanemarkerdistance[j] = dbdistance_left;

#ifdef GTEST_EN
if(gdebugflag == 1)
{
pf_map_log("\r\nj_%d: lanemarkerID_%d , lanemarker_state = %d , dbinstance_%lf \n\n",
    j,pastLanemarker[plane->leftLanemarker_i]->ID,leftlanemarker_state,dbdistance_left);
}
#endif
            }

            lanemarkerstate[j][0] = pastLanemarker[plane->leftLanemarker_i]->ID;    
            lanemarkerstate[j][1] = leftlanemarker_state;
            j++;
        }

        if(rightlanemarker_state == -1)
        {
            if(plane->rightNodeLineCount == 0)
            {
                rightlanemarker_state = LEFT_SIDE;
                lanemarkerdistance[j] = -1;

            }
            else
            {
                PF_MAP_NODE_LINE_ST* pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastLanemarker[plane->rightLanemarker_i]) + sizeof(PF_MAP_LINE_INFO_ST));
                
                rightlanemarker_state = pf_map_check_line_side(utm_x,utm_y,pnode,pastLanemarker[plane->rightLanemarker_i]->ID,
                                                    pastLanemarker[plane->rightLanemarker_i]->nodeCount,
                                                    plane->rightNodeLine_i,plane->rightNodeLineCount,&dbdistance_right,0);

                lanemarkerdistance[j] = dbdistance_right;

#ifdef GTEST_EN
if(gdebugflag == 1)
{
pf_map_log("\r\nj_%d: lanemarkerID_%d , lanemarker_state = %d , dbinstance_%lf \n\n",
    j,pastLanemarker[plane->rightLanemarker_i]->ID,rightlanemarker_state,dbdistance_right);
}
#endif
            }

            lanemarkerstate[j][0] = pastLanemarker[plane->rightLanemarker_i]->ID;    
            lanemarkerstate[j][1] = rightlanemarker_state;

            j++;
        }

#ifdef GTEST_EN
if(gdebugflag == 1)
{
pf_map_log("\r\nlaneID_%d : leftlanemarkerID_%d , state_%d , distance_%lf ; rightlanemarkerID_%d , state_%d , distance_%lf \r\n\r\n",
        pstLane[plane->lane_i].ID,pastLanemarker[plane->leftLanemarker_i]->ID,leftlanemarker_state,dbdistance_left,
                    pastLanemarker[plane->rightLanemarker_i]->ID,rightlanemarker_state,dbdistance_right);
}
#endif

        if(((LEFT_SIDE == rightlanemarker_state)&&(RIGHT_SIDE == leftlanemarker_state))||
                (ON_LINE == rightlanemarker_state)||(ON_LINE == leftlanemarker_state))
        {

            if(((dbdistance_right>=0)&&(dbdistance_right<dbdistance))||(dbdistance<0))
            {
                dbdistance = dbdistance_right;
                s32laneID = pstLane[plane->lane_i].ID;
#ifdef GTEST_EN
if(gdebugflag == 1)
{
pf_map_log("\r\n s32laneID: %d -> %d ; dbdistance_%lf > dbdistance_right_%lf \r\n",
            s32laneID,pstLane[plane->lane_i].ID,dbdistance,dbdistance_right);
}
#endif
                
            }

            if((dbdistance_left>=0)&&(dbdistance_left<dbdistance)||(dbdistance<0))
            {
                dbdistance = dbdistance_left;
                s32laneID = pstLane[plane->lane_i].ID;
#ifdef GTEST_EN
if(gdebugflag == 1)
{
pf_map_log("\r\n s32laneID: %d -> %d ; dbdistance_%lf > dbdistance_left_%lf \r\n",
            s32laneID,pstLane[plane->lane_i].ID,dbdistance,dbdistance_left);
}
#endif
            }

        }
        
        tmpoffset += sizeof(PF_BLOCK_LNAE_ST);
    }

    if(s32laneID > 0)
    {
        *laneID = s32laneID;
        *connectionID = -1;

        return PF_MAP_LANE;
    }

    pl_log(ERR," get laneID fail ");
    return PF_MAP_ERR;

}





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

GLOBAL S32 pf_map_get_ID_by_utm(DOUBLE utm_x, DOUBLE utm_y, S32* type, S32* s32ID, S32* index, DOUBLE* dbtheta, S32* laneNumber)

{
    S32 i = 0;
    S32 j = 0;
    S32 block_i = 0;

    U32 tmpoffset = 0;
    S32 lanemarkerstate[64][2] = {0};
    DOUBLE lanemarkerdistance[64] = {0};

    DOUBLE dbdistance = 0;
    DOUBLE dbtmp = 0;

    S32 s32lane_i = -1;
    S32 s32ret = 0;

    if(type == NULL)
    {
        return PF_MAP_ERR;
    }

    block_i = gu32BlockCount_x*(U32((utm_y - gdbUtmOffset_y)/gu32Block_h)) + U32((utm_x - gdbUtmOffset_x)/gu32Block_w);

    if((block_i < 0)||(block_i >= gu32BlockCount_x*gu32BlockCount_y))
    {
        pl_log(ERR," utm(%lf,%lf) is not in block map , block_i_%d , BlockCount(%d,%d) ",
                    utm_x,utm_y,block_i,gu32BlockCount_x,gu32BlockCount_y);
        return PF_MAP_ERR;
    }

    PF_MAP_BLOCK_INFO_ST* pmapblock = pastMapBlock[block_i];
    
    if(pmapblock == NULL)
    {
        pl_log(ERR," pastMapBlock[%d] is NULL , utm(%lf,%lf) ",block_i,utm_x,utm_y);
        return PF_MAP_ERR;
    }

    tmpoffset = sizeof(PF_MAP_BLOCK_INFO_ST);

    if((pmapblock->connectionCount == 1)&&(pmapblock->laneCount == 0))
    {
        PF_BLOCK_CONNECTION_ST* pconnection = (PF_BLOCK_CONNECTION_ST*)((U8*)pmapblock + tmpoffset);

        if(pconnection->nodeLineCount == 0)
        {
            *type = MAP_CONNECTION;
            if(index != NULL)
            {
                *index = pconnection->connection_i;
            }
            
            if(s32ID != NULL)
            {
                *s32ID = pstConnection[pconnection->connection_i].ID;
            }
            
            if(dbtheta != NULL)
            {
                s32ret = pf_map_get_drivingDirection_by_utm(utm_x, utm_y,pconnection->connection_i,dbtheta);
                if(s32ret != PF_MAP_SUCCESS)
                {
                    *dbtheta = INVALID_VALUE;
                }
            }
            
            if(laneNumber != NULL)
            {
                s32ret = pf_map_get_connection_lane_i(pconnection->connection_i,utm_x,utm_y);
                if(s32ret >= 0)
                {
                    *laneNumber = pstLane[s32ret].laneNumber;
                }
                else
                {
                    *laneNumber = INVALID_VALUE;
                }
            }
            
            return PF_MAP_SUCCESS;
        }
    }
        

    for(i=0;i<pmapblock->connectionCount;i++)
    {
        PF_BLOCK_CONNECTION_ST* pconnection = (PF_BLOCK_CONNECTION_ST*)((U8*)pmapblock + tmpoffset);
        PF_MAP_NODE_LINE_ST* pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastExtrapoint[pconnection->extrapoint_i]) + sizeof(PF_MAP_LINE_INFO_ST));

        S32 lineside = pf_map_check_line_side(utm_x,utm_y,pnode,pastExtrapoint[pconnection->extrapoint_i]->ID,
                                        pastExtrapoint[pconnection->extrapoint_i]->nodeCount,
                                        pconnection->nodeLine_i,pconnection->nodeLineCount,&dbdistance,1);

#ifdef GTEST_EN
if(gdebugflag == 1)
{
pf_map_log("\r\nconnectionID_%d , inside = %d , lineside_%d  , dbdistance_%lf \r\n\r\n",
pstConnection[pconnection->connection_i].ID,pastExtrapoint[pconnection->extrapoint_i]->insideMode,lineside,dbdistance);
}
#endif
        
        if((pastExtrapoint[pconnection->extrapoint_i]->insideMode == lineside)||(lineside == ON_LINE))
        {
            *type = MAP_CONNECTION;
            if(index != NULL)
            {
                *index = pconnection->connection_i;
            }
            if(s32ID != NULL)
            {
                *s32ID = pstConnection[pconnection->connection_i].ID;
            }
            if(dbtheta != NULL)
            {
                s32ret = pf_map_get_drivingDirection_by_utm(utm_x, utm_y,pconnection->connection_i,dbtheta);
                if(s32ret != PF_MAP_SUCCESS)
                {
                    *dbtheta = INVALID_VALUE;
                }
            }
            
            if(laneNumber != NULL)
            {
                s32ret = pf_map_get_connection_lane_i(pconnection->connection_i,utm_x,utm_y);
                if(s32ret >= 0)
                {
                    *laneNumber = pstLane[s32ret].laneNumber;
                }
                else
                {
                    *laneNumber = INVALID_VALUE;
                }
            }
            
            return PF_MAP_SUCCESS;
        }

        tmpoffset += sizeof(PF_BLOCK_CONNECTION_ST);
    }


    for(int k=0;k<3;k++)
    {
        if(pmapblock->noRunZone_i[k] >= 0)
        {
            if(1 == pf_map_check_utm_in_polygon(utm_x,utm_y,MAP_NO_RUN_ZONE,pmapblock->noRunZone_i[k],NULL))
            {
                *type = MAP_NO_RUN_ZONE;
                if(index != NULL)
                {
                    *index = (S32)(pmapblock->noRunZone_i[k]);
                }
                if(s32ID != NULL)
                {
                    *s32ID = pastNoRunZone[pmapblock->noRunZone_i[k]]->ID;
                }
                if(dbtheta != NULL)
                {
                    *dbtheta = ((DOUBLE)(pastNoRunZone[pmapblock->noRunZone_i[k]]->angle)*PAI/180);
                }
                
                if(laneNumber != NULL)
                {
                    *laneNumber = (S32)(pastNoRunZone[pmapblock->noRunZone_i[k]]->roadSide);
                }
                
                return PF_MAP_SUCCESS;
            }
        }
        else
        {
            break;
        }
    }


    dbdistance = -1;
    
    for(i=0;i<pmapblock->laneCount;i++)
    {
        PF_BLOCK_LNAE_ST* plane = (PF_BLOCK_LNAE_ST*)((U8*)pmapblock + tmpoffset);
        S32 leftlanemarker_state = -1;
        S32 rightlanemarker_state = -1;
        DOUBLE dbdistance_left = -1;
        DOUBLE dbdistance_right = -1;

        for(j=0;j<64;j++)
        {
            if(lanemarkerstate[j][0] == 0)
            {
                break;
            }
            else if((plane->rightNodeLineCount > 0)&&(lanemarkerstate[j][0] == pastLanemarker[plane->rightLanemarker_i]->ID))
            {
                rightlanemarker_state = lanemarkerstate[j][1];
                dbdistance_right = lanemarkerdistance[j];
            }
            else if((plane->leftNodeLineCount > 0)&&(lanemarkerstate[j][0] == pastLanemarker[plane->leftLanemarker_i]->ID))
            {
                leftlanemarker_state = lanemarkerstate[j][1];
                dbdistance_left= lanemarkerdistance[j];
            }
        }

        if(leftlanemarker_state == -1)
        {
            if(plane->leftNodeLineCount == 0)
            {
                leftlanemarker_state = RIGHT_SIDE;
                lanemarkerdistance[j] = -1;
            }
            else
            {
                PF_MAP_NODE_LINE_ST* pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastLanemarker[plane->leftLanemarker_i]) + sizeof(PF_MAP_LINE_INFO_ST));
                leftlanemarker_state = pf_map_check_line_side(utm_x,utm_y,pnode,pastLanemarker[plane->leftLanemarker_i]->ID,
                                                    pastLanemarker[plane->leftLanemarker_i]->nodeCount,
                                                    plane->leftNodeLine_i,plane->leftNodeLineCount,&dbdistance_left,0);

                lanemarkerdistance[j] = dbdistance_left;

#ifdef GTEST_EN
if(gdebugflag == 1)
{
pf_map_log("\r\nj_%d: lanemarkerID_%d , lanemarker_state = %d , dbinstance_%lf \n\n",
    j,pastLanemarker[plane->leftLanemarker_i]->ID,leftlanemarker_state,dbdistance_left);
}
#endif
            }

            lanemarkerstate[j][0] = pastLanemarker[plane->leftLanemarker_i]->ID;    
            lanemarkerstate[j][1] = leftlanemarker_state;
            j++;
        }

        if(rightlanemarker_state == -1)
        {
            if(plane->rightNodeLineCount == 0)
            {
                rightlanemarker_state = LEFT_SIDE;
                lanemarkerdistance[j] = -1;

            }
            else
            {
                PF_MAP_NODE_LINE_ST* pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastLanemarker[plane->rightLanemarker_i]) + sizeof(PF_MAP_LINE_INFO_ST));
                rightlanemarker_state = pf_map_check_line_side(utm_x,utm_y,pnode,pastLanemarker[plane->rightLanemarker_i]->ID,
                                                    pastLanemarker[plane->rightLanemarker_i]->nodeCount,
                                                    plane->rightNodeLine_i,plane->rightNodeLineCount,&dbdistance_right,0);

                lanemarkerdistance[j] = dbdistance_right;

#ifdef GTEST_EN
if(gdebugflag == 1)
{
pf_map_log("\r\nj_%d: lanemarkerID_%d , lanemarker_state = %d , dbinstance_%lf \n\n",
    j,pastLanemarker[plane->rightLanemarker_i]->ID,rightlanemarker_state,dbdistance_right);
}
#endif
            }

            lanemarkerstate[j][0] = pastLanemarker[plane->rightLanemarker_i]->ID;    
            lanemarkerstate[j][1] = rightlanemarker_state;

            j++;
        }

#ifdef GTEST_EN
if(gdebugflag == 1)
{
pf_map_log("\r\nlaneID_%d : leftlanemarkerID_%d , state_%d , distance_%lf ; rightlanemarkerID_%d , state_%d , distance_%lf \r\n\r\n",
        pstLane[plane->lane_i].ID,pastLanemarker[plane->leftLanemarker_i]->ID,leftlanemarker_state,dbdistance_left,
                    pastLanemarker[plane->rightLanemarker_i]->ID,rightlanemarker_state,dbdistance_right);
}
#endif

        if(((LEFT_SIDE == rightlanemarker_state)&&(RIGHT_SIDE == leftlanemarker_state))||
                (ON_LINE == rightlanemarker_state)||(ON_LINE == leftlanemarker_state))
        {

            if(((dbdistance_right>=0)&&(dbdistance_right<dbdistance))||(dbdistance<0))
            {
#ifdef GTEST_EN
if(gdebugflag == 1)
{
pf_map_log("\r\n lane[%d]_ID_%d -> lane[%d]_ID_%d ; dbdistance_%lf > dbdistance_right_%lf \r\n",
        s32lane_i,pstLane[s32lane_i].ID,plane->lane_i,pstLane[plane->lane_i].ID,dbdistance,dbdistance_right);
}
#endif
                dbdistance = dbdistance_right;
                s32lane_i = plane->lane_i;
                
            }

            if((dbdistance_left>=0)&&(dbdistance_left<dbdistance)||(dbdistance<0))
            {
#ifdef GTEST_EN
if(gdebugflag == 1)
{
pf_map_log("\r\n lane[%d]_ID_%d -> lane[%d]_ID_%d ; dbdistance_%lf > dbdistance_left_%lf \r\n",
            s32lane_i,pstLane[s32lane_i].ID,plane->lane_i,pstLane[plane->lane_i].ID,dbdistance,dbdistance_left);
}
#endif
                dbdistance = dbdistance_left;
                s32lane_i = plane->lane_i;
            }

        }
        
        tmpoffset += sizeof(PF_BLOCK_LNAE_ST);
    }

    if(s32lane_i >= 0)
    {
        *type = MAP_LANE;
        if(index != NULL)
        {
            *index = s32lane_i;
        }
        if(s32ID != NULL)
        {
            *s32ID = pstLane[s32lane_i].ID;
        }
        if(dbtheta != NULL)
        {
            *dbtheta = ((DOUBLE)(pstLane[s32lane_i].s32angle)*PAI/180);
        }

        if(laneNumber != NULL)
        {
            *laneNumber = (S32)(pstLane[s32lane_i].laneNumber);
        }

        
        return PF_MAP_SUCCESS;
    }



//    pl_log(ERR," get laneID fail ");
    return PF_MAP_ERR;

}






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

GLOBAL S32 pf_map_get_parking_state_by_utm(DOUBLE utm_x, DOUBLE utm_y)
{
    S32 block_i = 0;
    S32 s32ret = -1;
    
    block_i = gu32BlockCount_x*(U32((utm_y - gdbUtmOffset_y)/gu32Block_h)) + U32((utm_x - gdbUtmOffset_x)/gu32Block_w);

    if((block_i < 0)||(block_i >= gu32BlockCount_x*gu32BlockCount_y))
    {
        pl_log(ERR," utm(%lf,%lf) is not in block map , block_i_%d , BlockCount(%d,%d) ",
                    utm_x,utm_y,block_i,gu32BlockCount_x,gu32BlockCount_y);
        return PF_MAP_ERR;
    }

    PF_MAP_BLOCK_INFO_ST* pmapblock = pastMapBlock[block_i];
    
    if(pmapblock != NULL)
    {
        for(int k=0;k<3;k++)
        {
            if(pmapblock->parkingZone_i[k] >= 0)
            {
                if(1 == pf_map_check_utm_in_polygon(utm_x,utm_y,MAP_PARKING_ZONE,pmapblock->parkingZone_i[k],NULL))
                {
                    return PF_MAP_PARKING_YES;
                }
            }
            else
            {
                break;
            }
        }
    }

    return PF_MAP_PARKING_NO;
}





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

GLOBAL S32 pf_map_get_drivingDirection_by_utm(DOUBLE utm_x, DOUBLE utm_y, S32 connection_i,DOUBLE* dbtheta)
{
    if((connection_i<0)||(connection_i >= gas32ElementCount[MAP_CONNECTION]))
    {
        pl_log(ERR," connection_i_%d  is invalid , connectionCount_%d",connection_i,gas32ElementCount[MAP_CONNECTION]);
        return PF_MAP_ERR;
    }
    
    if((pstConnection[connection_i].s16angle <= 180)&&
        (pstConnection[connection_i].s16angle >= -180))
    {
        if(dbtheta != NULL)
        {
            *dbtheta = ((DOUBLE)(pstConnection[connection_i].s16angle)*PAI/180);
        }
        return PF_MAP_SUCCESS;
    }

    return PF_MAP_ERR;

}






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

GLOBAL S32 pf_map_check_pavement_by_utm(DOUBLE utm_x, DOUBLE utm_y, DOUBLE distance, S32* pavement_i, S32* section_i)
{
    S32 s32ret = PF_MAP_ERR;
    S32 type = 0;
    S32 index = -1;
    S32 connection_i = -1;
    DOUBLE tmpdb = -1;
    
    s32ret = pf_map_get_ID_by_utm(utm_x,utm_y,&type,NULL,&index,NULL,NULL);

    if(PF_MAP_ERR == s32ret)
    {
        return PF_MAP_ERR;
    }
    
    if(type == MAP_LANE)
    {
        S32 lastnode_i = pastLanemarker[pstLane[index].leftLanemarker_i]->nodeCount-1;
        DOUBLE db0 = 0;
        DOUBLE db1 = 0;
        PF_MAP_NODE_LINE_ST* pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastLanemarker[pstLane[index].leftLanemarker_i]) + sizeof(PF_MAP_LINE_INFO_ST));
    
        if(((pstLane[index].s32angle > 45)&&(pstLane[index].s32angle < 135))||
            ((pstLane[index].s32angle < -45)&&(pstLane[index].s32angle > -135)))
        {
            db0 = pnode->node_y- utm_y;
            db1 = pnode[lastnode_i].node_y - utm_y;
        }
        else
        {
            db0 = pnode->node_x - utm_x;
            db1 = pnode[lastnode_i].node_x - utm_x;
        }

        if(db0 < 0)
        {
            db0 = -db0;
        }
        
        if(db1 < 0)
        {
            db1 = -db1;
        }

        if(db0 < db1)
        {
            if( db0 > distance)
            {
                return PF_MAP_PAVEMENT_NO;
            }
            connection_i = pstSection[pstLane[index].section_i].fromConnection_i;
        }
        
        if(connection_i < 0)
        {
            if( db1 > distance)
            {
                return PF_MAP_PAVEMENT_NO;
            }
        
            connection_i = pstSection[pstLane[index].section_i].toConnection_i;
            
            if(connection_i < 0)
            {
                if( db0 > distance)
                {
                    return PF_MAP_PAVEMENT_NO;
                }
                connection_i = pstSection[pstLane[index].section_i].fromConnection_i;
            }
        }
        
    }
    else
    {
        connection_i = index;
    }
    
    if((connection_i<0)||(connection_i >= gas32ElementCount[MAP_CONNECTION]))
    {
        pl_log(ERR," utm( %lf , %lf ) , connection_i_%d  is invalid , connectionCount_%d",utm_x,utm_y,connection_i,gas32ElementCount[MAP_CONNECTION]);
        return PF_MAP_ERR;
    }

    for(int i=0;i<pstConnection[connection_i].pavementCount;i++)
    {
        s32ret = pf_map_check_utm_in_polygon(utm_x, utm_y,MAP_PAVEMENT,pstConnection[connection_i].pavement_i + i,&tmpdb);

        if((1 == s32ret)||((tmpdb >= 0)&&(tmpdb < distance)))
        {
            if(pavement_i != NULL)
            {
                *pavement_i = pstConnection[connection_i].pavement_i+i;
            }
            
            if(section_i != NULL)
            {
                *section_i = pastPavement[pstConnection[connection_i].pavement_i+i]->section_i;
            }
            return PF_MAP_PAVEMENT_YES;
        }
    }

    return PF_MAP_PAVEMENT_NO;
}






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
                                                S32* link_i,S32* linkCount, DOUBLE* dbtheta)
{
    if((connection_i<0)||(connection_i >= gas32ElementCount[MAP_CONNECTION]))
    {
        pl_log(ERR," connection_i_%d  is invalid , connectionCount_%d",connection_i,gas32ElementCount[MAP_CONNECTION]);
        return PF_MAP_ERR;
    }

    if(connectionType != NULL)
    {
        *connectionType = pstConnection[connection_i].type;
    }
    
    if(link_i != NULL)
    {
        *link_i = pstConnection[connection_i].link_i;
    }
    
    if(linkCount != NULL)
    {
        *linkCount = pstConnection[connection_i].linkCount;
    }
    
    if(dbtheta != NULL)
    {
        if((pstConnection[connection_i].s16angle <= 180)&&
            (pstConnection[connection_i].s16angle >= -180))
        {
            *dbtheta = ((DOUBLE)(pstConnection[connection_i].s16angle)*PAI/180);
        }
        else
        {
            *dbtheta = INVALID_VALUE;
        }
    }
    
    return PF_MAP_SUCCESS;
}



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

GLOBAL S32 pf_map_get_linkInfo_by_index(S32 link_i,S32* fromSection_i,S32* toSection_i,S32* linkType)
{
    if((link_i<0)||(link_i >= gas32ElementCount[MAP_LINK]))
    {
        pl_log(ERR," link_i_%d  is invalid , linkCount_%d",link_i,gas32ElementCount[MAP_LINK]);
        return PF_MAP_ERR;
    }

    if(fromSection_i != NULL)
    {
        *fromSection_i = pstLink[link_i].fromSection_i;
    }
    
    if(toSection_i != NULL)
    {
        *toSection_i = pstLink[link_i].toSection_i;
    }
    
    if(linkType != NULL)
    {
        *linkType = pstLink[link_i].type;
    }

    return PF_MAP_SUCCESS;
}



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
                                                    S32* toConnection_i,S32* trafficLight_i,S32* trafficLightCount,DOUBLE* dbtheta)
{
    int i = 0;
    
    if((type == MAP_LANE)&&(index < gas32ElementCount[MAP_LANE]))
    {
        i = pstLane[index].section_i;
    }
    else if((type == MAP_SECTION)&&(index < gas32ElementCount[MAP_SECTION]))
    {
        i = index;
    }
    else
    {
        pl_log(ERR," type_%d , index_%d  is invalid , validCount_%d",type,index,gas32ElementCount[type]);
        return PF_MAP_ERR;
    }


    if(sectionType != NULL)
    {
        *sectionType = pstSection[i].type;
    }
    
    if(fromConnection_i != NULL)
    {
        *fromConnection_i = pstSection[i].fromConnection_i;
    }
    
    if(toConnection_i != NULL)
    {
        *toConnection_i = pstSection[i].toConnection_i;
    }

    if(trafficLight_i != NULL)
    {
        *trafficLight_i = pstSection[i].tracficLight_i;
    }

    if(trafficLightCount != NULL)
    {
        *trafficLightCount = pstSection[i].tracficLightCount;
    }

    if(section_i != NULL)
    {
        *section_i = i;
    }

    if(dbtheta != NULL)
    {
        i = pstSection[i].lane_i;
        if((pstLane[i].s32angle <= 180)&&
            (pstLane[i].s32angle >= -180))
        {
            *dbtheta = ((DOUBLE)(pstLane[i].s32angle)*PAI/180);
        }
        else
        {
            *dbtheta = INVALID_VALUE;
        }
    }

    return PF_MAP_SUCCESS;
}






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

GLOBAL S32 pf_map_get_section_lane_lnfo_by_index(S32 type,S32 index,S32* lane_count,S32* first_lane_i,S32* first_lane_num)
{
    int i = 0;
    
    if((type == MAP_LANE)&&(index < gas32ElementCount[MAP_LANE]))
    {
        i = pstLane[index].section_i;
    }
    else if((type == MAP_SECTION)&&(index < gas32ElementCount[MAP_SECTION]))
    {
        i = index;
    }
    else
    {
        pl_log(ERR," type_%d , index_%d  is invalid , validCount_%d",type,index,gas32ElementCount[type]);
        return PF_MAP_ERR;
    }

    if(lane_count != NULL)
    {
        *lane_count = (S32)(pstSection[i].laneCount);
    }
    
    if(first_lane_i != NULL)
    {
        *first_lane_i = (S32)(pstSection[i].lane_i);
    }
    
    if(first_lane_num != NULL)
    {
        *first_lane_num = (S32)(pstLane[pstSection[i].lane_i].laneNumber);
    }

    return PF_MAP_SUCCESS;
}







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

GLOBAL S32 pf_map_get_trafficLightInfo_by_index(S32 trafficLight_i,S32* trafficLightID,DOUBLE* pos_x,DOUBLE* pos_y)
{
    if((trafficLight_i<0)||(trafficLight_i >= gas32ElementCount[MAP_TRAFFICLIGHT]))
    {
        pl_log(ERR," trafficLight_i_%d  is invalid , trafficLightCount_%d",trafficLight_i,gas32ElementCount[MAP_TRAFFICLIGHT]);
        return PF_MAP_ERR;
    }

    if(trafficLightID != NULL)
    {
        *trafficLightID = pstTrafficLight[trafficLight_i].ID;
    }
    
    if(pos_x != NULL)
    {
        *pos_x = pstTrafficLight[trafficLight_i].start_utm_x;
    }
    
    if(pos_y != NULL)
    {
        *pos_y = pstTrafficLight[trafficLight_i].start_utm_y;
    }

    return PF_MAP_SUCCESS;
}






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
                                                S32* laneType, S32* turnType, DOUBLE* dbtheta, S32* section_i)
{
    if((lane_i<0)||(lane_i >= gas32ElementCount[MAP_LANE]))
    {
        pl_log(ERR," lane_i_%d  is invalid , laneCount_%d",lane_i,gas32ElementCount[MAP_LANE]);
        return PF_MAP_ERR;
    }

    if(speedMax != NULL)
    {
        *speedMax = pstLane[lane_i].limitSpeedMax;
    }
    
    if(speedMin != NULL)
    {
        *speedMin = pstLane[lane_i].limitSpeedMin;
    }
    
    if(laneType != NULL)
    {
        *laneType = pstLane[lane_i].type;
    }

    if(turnType != NULL)
    {
        *turnType = pstLane[lane_i].turnType;
    }

    if(dbtheta != NULL)
    {
        if((pstLane[lane_i].s32angle <= 180)&&
            (pstLane[lane_i].s32angle >= -180))
        {
            *dbtheta = ((DOUBLE)(pstLane[lane_i].s32angle)*PAI/180);
        }
        else
        {
            *dbtheta = INVALID_VALUE;
        }
    }

    if(section_i != NULL)
    {
        *section_i = pstLane[lane_i].section_i;
    }
    
    return PF_MAP_SUCCESS;
}










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

GLOBAL S32 pf_map_get_laneMarkerDistance_by_utm(DOUBLE utm_x, DOUBLE utm_y,S32 lane_i,DOUBLE* dis_left, DOUBLE* dis_right)
{
    DOUBLE dis = -1;     
    S32    s32ret = PF_MAP_SUCCESS;
    S32    index = lane_i;
    S32    i = 0;
    
    if(index < 0)
    {
        S32 type = -1;

        if(PF_MAP_ERR == pf_map_get_ID_by_utm(utm_x,utm_y,&type,NULL,&index,NULL,NULL))
        {
            return PF_MAP_ERR;
        }

        if(type != MAP_LANE)
        {
            pl_log(ERR," utm(%lf,%lf) is not on the lane , type_%d , index_%d ",utm_x,utm_y,type,index);
            return PF_MAP_ERR;
        }
    }
    else if(index >= gas32ElementCount[MAP_LANE])
    {
        pl_log(ERR," lane_i_%d  is invalid , laneCount_%d",index,gas32ElementCount[MAP_LANE]);
        return PF_MAP_ERR;
    }
    
    if(dis_left != NULL)
    {
        PF_MAP_NODE_LINE_ST* pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastLanemarker[pstLane[index].leftLanemarker_i])+sizeof(PF_MAP_LINE_INFO_ST));
        S32 node_count = pastLanemarker[pstLane[index].leftLanemarker_i]->nodeCount;

        for(i=1;i<node_count;i++)
        {
            DOUBLE x1 = pnode[i-1].node_x;
            DOUBLE x2 = pnode[i].node_x;
            DOUBLE y1 = pnode[i-1].node_y;
            DOUBLE y2 = pnode[i].node_y;
            
            DOUBLE a = x2 - x1;
            DOUBLE b = y2 - y1;
            DOUBLE aa = a*a;
            DOUBLE bb = b*b;

            DOUBLE x = (aa*utm_x + bb*x1 + a*b*(utm_y-y1))/(aa + bb);
            DOUBLE y = (aa*y1 + bb*utm_y + a*b*(utm_x-x1))/(aa + bb);

            if(((x-x1)*(x-x2)<=0)&&((y-y1)*(y-y2)<=0))
            {
                x = x - utm_x;
                y = y - utm_y;
                
                dis = sqrt(x*x+y*y);
                break;
            }
        }

        *dis_left = dis;

        if(dis < 0)
        {
            pl_log(ERR," utm(%lf,%lf) get dis_left fail , leftLanemarker_%d , nodeCount_%d ",utm_x,utm_y,
                pastLanemarker[pstLane[index].leftLanemarker_i]->ID,pastLanemarker[pstLane[index].leftLanemarker_i]->nodeCount);

            s32ret = PF_MAP_ERR;
        }
    }

    if(dis_right != NULL)
    {
        dis = -1;

        PF_MAP_NODE_LINE_ST* pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastLanemarker[pstLane[index].rightLanemarker_i])+sizeof(PF_MAP_LINE_INFO_ST));
        S32 node_count = pastLanemarker[pstLane[index].rightLanemarker_i]->nodeCount;

        for(i=1;i<node_count;i++)
        {
            DOUBLE x1 = pnode[i-1].node_x;
            DOUBLE x2 = pnode[i].node_x;
            DOUBLE y1 = pnode[i-1].node_y;
            DOUBLE y2 = pnode[i].node_y;
            
            DOUBLE a = x2 - x1;
            DOUBLE b = y2 - y1;
            DOUBLE aa = a*a;
            DOUBLE bb = b*b;

            DOUBLE x = (aa*utm_x + bb*x1 + a*b*(utm_y-y1))/(aa + bb);
            DOUBLE y = (aa*y1 + bb*utm_y + a*b*(utm_x-x1))/(aa + bb);

            if(((x-x1)*(x-x2)<=0)&&((y-y1)*(y-y2)<=0))
            {
                x = x - utm_x;
                y = y - utm_y;
                
                dis = sqrt(x*x+y*y);
                break;
            }
        }

        *dis_right = dis;
        
        if(dis < 0)
        {
            pl_log(ERR," utm(%lf,%lf) get dis_right fail , rightLanemarker_%d , nodeCount_%d ",utm_x,utm_y,
                pastLanemarker[pstLane[index].rightLanemarker_i]->ID,node_count);
                
            s32ret = PF_MAP_ERR;
        }
    }

    return s32ret;
}






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
                                                                          DOUBLE* right_utmx, DOUBLE* right_utmy)
{
    if((lane_i<0)||(lane_i >= gas32ElementCount[MAP_LANE]))
    {
        pl_log(ERR," lane_i_%d  is invalid , laneCount_%d",lane_i,gas32ElementCount[MAP_LANE]);
        return PF_MAP_ERR;
    }

    PF_MAP_NODE_LINE_ST* pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastLanemarker[pstLane[lane_i].leftLanemarker_i])+sizeof(PF_MAP_LINE_INFO_ST));

    if(left_utmx != NULL)
    {
        *left_utmx = pnode->node_x;
    }

    if(left_utmy != NULL)
    {
        *left_utmy = pnode->node_y;
    }
    
    pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastLanemarker[pstLane[lane_i].rightLanemarker_i])+sizeof(PF_MAP_LINE_INFO_ST));

    if(right_utmx != NULL)
    {
        *right_utmx = pnode->node_x;
    }

    if(right_utmy != NULL)
    {
        *right_utmy = pnode->node_y;
    }

    return PF_MAP_SUCCESS;
}




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
                                                                          DOUBLE* right_utmx, DOUBLE* right_utmy)
{
    if((lane_i<0)||(lane_i >= gas32ElementCount[MAP_LANE]))
    {
        pl_log(ERR," lane_i_%d  is invalid , laneCount_%d",lane_i,gas32ElementCount[MAP_LANE]);
        return PF_MAP_ERR;
    }

    PF_MAP_NODE_LINE_ST* pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastLanemarker[pstLane[lane_i].leftLanemarker_i])+sizeof(PF_MAP_LINE_INFO_ST));
    S32 node_i = pastLanemarker[pstLane[lane_i].leftLanemarker_i]->nodeCount - 1;

    if(left_utmx != NULL)
    {
        *left_utmx = pnode[node_i].node_x;
    }

    if(left_utmy != NULL)
    {
        *left_utmy = pnode[node_i].node_y;
    }
    
    pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastLanemarker[pstLane[lane_i].rightLanemarker_i])+sizeof(PF_MAP_LINE_INFO_ST));
    node_i = pastLanemarker[pstLane[lane_i].rightLanemarker_i]->nodeCount - 1;

    if(right_utmx != NULL)
    {
        *right_utmx = pnode[node_i].node_x;
    }

    if(right_utmy != NULL)
    {
        *right_utmy = pnode[node_i].node_y;
    }

    return PF_MAP_SUCCESS;
}









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

GLOBAL S32 pf_map_get_laneMarkerType_by_laneIndex(S32 lane_i,S32* leftLaneMarkerType, S32* rightLaneMarkerType)
{
    if((lane_i<0)||(lane_i >= gas32ElementCount[MAP_LANE]))
    {
        pl_log(ERR," lane_i_%d  is invalid , laneCount_%d",lane_i,gas32ElementCount[MAP_LANE]);
        return PF_MAP_ERR;
    }

    if(leftLaneMarkerType != NULL)
    {
        *leftLaneMarkerType = (S32)(pastLanemarker[pstLane[lane_i].leftLanemarker_i]->type);
    }
    
    if(rightLaneMarkerType != NULL)
    {
        *rightLaneMarkerType = (S32)(pastLanemarker[pstLane[lane_i].rightLanemarker_i]->type);
    }

    return PF_MAP_SUCCESS;
}








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

GLOBAL S32 pf_map_get_preSection_by_index(S32 type, S32 index, S32 linkType)
{
    S32 section_i = INVALID_VALUE;
    S32 connection_i = INVALID_VALUE;
    S32 link_i = INVALID_VALUE;

    if((linkType < 0)||(linkType >= LINK_TYPE_MAX))
    {
        pl_log(ERR," linkType_%d  is invalid ",linkType);
        return PF_MAP_ERR;
    }
    
    if((type == MAP_LANE)&&(index >= 0)&&(index < gas32ElementCount[MAP_LANE]))
    {
        section_i = pstLane[index].section_i;
    }
    else if((type == MAP_SECTION)&&(index >= 0)&&(index < gas32ElementCount[MAP_SECTION]))
    {
        section_i = index;
    }
    else if((type == MAP_CONNECTION)&&(index >= 0)&&(index < gas32ElementCount[MAP_CONNECTION]))
    {
        link_i = pstConnection[index].link_i;
        for(int j=0;j< pstConnection[index].linkCount;j++)
        {
            if((pstLink[link_i].type == linkType)&&(pstLink[link_i].fromSection_i >= 0))
            {
                return pstLink[link_i].fromSection_i;
            }
            link_i++;
        }
        
        return PF_MAP_NO_ELEMENT;
    }
    else
    {
        pl_log(ERR," type_%d , index_%d  is invalid , validCount_%d",type,index,gas32ElementCount[type]);
        return PF_MAP_ERR;
    }

    connection_i = pstSection[section_i].fromConnection_i;

    if(connection_i >= 0)
    {
        link_i = pstConnection[connection_i].link_i;
        for(int j=0;j< pstConnection[connection_i].linkCount;j++)
        {
            if((pstLink[link_i].type == linkType)&&(pstLink[link_i].toSection_i == section_i)&&(pstLink[link_i].fromSection_i >= 0))
            {
                return pstLink[link_i].fromSection_i;
            }
            link_i++;
        }
    }

    return PF_MAP_NO_ELEMENT;
}





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

GLOBAL S32 pf_map_get_nextSection_by_index(S32 type, S32 index, S32 linkType)
{
    int section_i = 0;
    int connection_i = 0;
    S32 link_i = INVALID_VALUE;

    if((linkType < 0)||(linkType >= LINK_TYPE_MAX))
    {
        pl_log(ERR," linkType_%d  is invalid ",linkType);
        return PF_MAP_ERR;
    }

    
    if((type == MAP_LANE)&&(index >= 0)&&(index < gas32ElementCount[MAP_LANE]))
    {
        section_i = pstLane[index].section_i;
    }
    else if((type == MAP_SECTION)&&(index >= 0)&&(index < gas32ElementCount[MAP_SECTION]))
    {
        section_i = index;
    }
    else if((type == MAP_CONNECTION)&&(index >= 0)&&(index < gas32ElementCount[MAP_CONNECTION]))
    {
        link_i = pstConnection[index].link_i;
        for(int j=0;j< pstConnection[index].linkCount;j++)
        {
            if((pstLink[link_i].type == linkType)&&(pstLink[link_i].toSection_i >= 0))
            {
                return pstLink[link_i].toSection_i;
            }
            link_i++;
        }

        return PF_MAP_NO_ELEMENT;
    }
    else
    {
        pl_log(ERR," type_%d , index_%d  is invalid , validCount_%d",type,index,gas32ElementCount[type]);
        return PF_MAP_ERR;
    }

    connection_i = pstSection[section_i].toConnection_i;

    if(connection_i >= 0)
    {
        link_i = pstConnection[connection_i].link_i;
        for(int j=0;j< pstConnection[connection_i].linkCount;j++)
        {
            if((pstLink[link_i].type == linkType)&&(pstLink[link_i].fromSection_i == section_i)&&(pstLink[link_i].toSection_i >= 0))
            {
                return pstLink[link_i].toSection_i;
            }
            link_i++;
        }
    }

    return PF_MAP_NO_ELEMENT;
}







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

GLOBAL S32 pf_map_get_preLane_by_index(S32 index)
{
    S32 connection_i = pstSection[pstLane[index].section_i].fromConnection_i;
    S32 maxLaneNum = -1;
    S32 maxLaneNum_i = -2;

    if(connection_i >= 0)
    {
        S32 link_i = pstConnection[connection_i].link_i;
        for(int j=0;j< pstConnection[connection_i].linkCount;j++)
        {
            if((pstLink[link_i].type == LINK_STRAIGHT)&&(pstLink[link_i].fromSection_i>=0))
            {
                S32 firstLane_i = pstSection[pstLink[link_i].fromSection_i].lane_i;
                for(int i=0;i< pstSection[pstLink[link_i].fromSection_i].laneCount;i++)
                {
                    if(pstLane[firstLane_i+i].laneNumber == pstLane[index].laneNumber)
                    {
                        return firstLane_i+i;
                    }
//                    else if(pstLane[firstLane_i+i].laneNumber > maxLaneNum) 
//                    {
//                        maxLaneNum = pstLane[firstLane_i+i].laneNumber;
//                        maxLaneNum_i = firstLane_i+i;
//                    }
                }
            }
            link_i++;
        }
    }

    return maxLaneNum_i;
}














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

GLOBAL S32 pf_map_get_entrance_distance_by_utm(DOUBLE utm_x, DOUBLE utm_y, S32 road_i, DOUBLE* distance, S32* roadside)
{
    PF_MAP_NODE_LINE_ST* pnode = NULL;
    DOUBLE db_0 = 0;
    DOUBLE db_1 = 0;
    DOUBLE theta = 0;
    DOUBLE* pdistance = 0;
    S32 nodecount = 0;
    S32 s32ret = 0;
    S32 dir_x = 0;
    S32 i = 0;

    if(( road_i >= gas32ElementCount[MAP_ROAD])||( road_i < 0))
    {
        pl_log(ERR," road_i_%d  is invalid , roadCount_%d",road_i,gas32ElementCount[MAP_ROAD]);
        return PF_MAP_ERR;
    }

    if(( pstRoad[road_i].dividerLine_i < 0)||( pstRoad[road_i].dividerLine_i >= gas32ElementCount[MAP_CENTER_LINE]))
    {
        pl_log(ERR," road[%d]_ID_%d  , dividerLine_i_%d  is invalid , dividerLineCount_%d",
                road_i,pstRoad[road_i].ID,pstRoad[road_i].dividerLine_i,gas32ElementCount[MAP_CENTER_LINE]);
        return PF_MAP_ERR;
    }


    pnode = (PF_MAP_NODE_LINE_ST*)((U8*)(pastDividerLine[pstRoad[road_i].dividerLine_i])+sizeof(PF_MAP_LINE_INFO_ST));
    nodecount = pastDividerLine[pstRoad[road_i].dividerLine_i]->nodeCount;

    db_0 = (pnode[0].node_x-utm_x)*(pnode[0].node_x-utm_x) + (pnode[0].node_y-utm_y)*(pnode[0].node_y-utm_y);
    s32ret = 0;

    for(i=1;i<nodecount;i++)
    {
        db_1 = (pnode[i].node_x-utm_x)*(pnode[i].node_x-utm_x) + (pnode[i].node_y-utm_y)*(pnode[i].node_y-utm_y);
        if(db_1 < db_0)
        {
            db_0 = db_1;
            s32ret = i;
        }
    }

    if(s32ret == 0)
    {
        i = 1;
        db_0 = (pnode[1].node_x-utm_x)*(pnode[1].node_x-utm_x) + (pnode[1].node_y-utm_y)*(pnode[1].node_y-utm_y);
    }
    else
    {
        i = s32ret;
    }

    if(distance != NULL)  
    {
        db_1 = sqrt(db_0);
        
        theta = atan2(pnode[i-1].node_y-pnode[i].node_y,pnode[i-1].node_x-pnode[i].node_x) 
                - atan2(utm_y-pnode[i].node_y,utm_x-pnode[i].node_x);

        if(theta < 0)
        {
            theta = -theta;
        }

        pdistance = (DOUBLE*)(pnode+nodecount);

        *distance = pdistance[i] - db_1*cos(theta);
    }

    if(roadside != NULL)
    {
        dir_x = -1;

        if(pnode[i].node_x < utm_x)
        {
            if(pnode[i-1].node_x < pnode[i].node_x)
            {
                if((i < nodecount-1)&&(utm_x <= pnode[i+1].node_x))    // node[i] < utm < node[i+1]
                {
                    i++;
                    dir_x = 1;
                }
                else if((i < nodecount-2)&&(utm_x <= pnode[i+2].node_x)) // node[i+1] < utm < node[i+2]
                {
                    i+=2;
                    dir_x = 1;
                }
            }
            else
            {
                if((i > 0)&&(utm_x <= pnode[i-1].node_x))    // node[i] < utm < node[i-1]
                {
                    dir_x = 1;
                }
                else if((i > 1)&&(utm_x <= pnode[i-2].node_x))  // node[i-1] < utm < node[i-2]
                {
                    i--;
                    dir_x = 1;
                }
            }

        }
        else
        {
            if(pnode[i-1].node_x > pnode[i].node_x)     
            {
                if((i < nodecount-1)&&(utm_x >= pnode[i+1].node_x))    // node[i+1] < utm < node[i]
                {
                    i++;
                    dir_x = 1;
                }
                else if((i < nodecount-2)&&(utm_x >= pnode[i+2].node_x))   // node[i+2] < utm < node[i+1]
                {
                    i+=2;
                    dir_x = 1;
                }
            }
            else
            {
                if((i > 0)&&(utm_x >= pnode[i-1].node_x))   // node[i-1] < utm < node[i]
                {
                    dir_x = 1;
                }
                else if((i > 1)&&(utm_x >= pnode[i-2].node_x))  // node[i-2] < utm < node[i-1]
                {
                    i--;
                    dir_x = 1;
                }
            }
        }

 

        if(dir_x == -1)
        {
            if(pnode[i].node_y < utm_y)
            {
                if(pnode[i-1].node_y < pnode[i].node_y)
                {
                    if((i < nodecount-1)&&(utm_y <= pnode[i+1].node_y))
                    {
                        i++;
                        dir_x = 0;
                    }
                    else if((i < nodecount-2)&&(utm_y <= pnode[i+2].node_y))
                    {
                        i+=2;
                        dir_x = 0;
                    }
                }
                else
                {
                    if((i > 0)&&(utm_y <= pnode[i-1].node_y))
                    {
                        dir_x = 0;
                    }
                    else if((i > 1)&&(utm_y <= pnode[i-2].node_y))
                    {
                        i--;
                        dir_x = 0;
                    }
                }

            }
            else
            {
                if(pnode[i-1].node_y > pnode[i].node_y)
                {
                    if((i < nodecount-1)&&(utm_y >= pnode[i+1].node_y))
                    {
                        i++;
                        dir_x = 0;
                    }
                    else if((i < nodecount-2)&&(utm_y >= pnode[i+2].node_y))
                    {
                        i+=2;
                        dir_x = 0;
                    }
                }
                else
                {
                    if((i > 0)&&(utm_y >= pnode[i-1].node_y))
                    {
                        dir_x = 0;
                    }
                    else if((i > 1)&&(utm_y >= pnode[i-2].node_y))
                    {
                        i--;
                        dir_x = 0;
                    }
                }
            }
        }

        if(dir_x == -1)
        {
            db_0 = pnode[i].node_x - pnode[i-1].node_x;
            db_1 = pnode[i].node_y - pnode[i-1].node_y;

            if(db_0 < 0)
            {
                db_0 = -db_0;
            }
            
            if(db_1 < 0)
            {
                db_1 = -db_1;
            }

            if(db_0 > db_1)
            {
                dir_x = 1;
            }
            else
            {
                dir_x = 0;
            }

            s32ret = pf_map_check_nodeline_side(utm_x,utm_y,pnode+i-1,pnode+i,dir_x,&db_1,1);

        }
        else
        {
            s32ret = pf_map_check_nodeline_side(utm_x,utm_y,pnode+i-1,pnode+i,dir_x,&db_1,0);
        }

        if(s32ret == RIGHT_SIDE)
        {
            *roadside = MAP_ROAD_UP_SIDE;
        }
        else if(s32ret == LEFT_SIDE)
        {
            *roadside = MAP_ROAD_DOWN_SIDE;
        }
        else
        {
                pl_log(ERR," utm( %lf , %lf ) , dividerLine_%d , nodeCount_%d , node[%d](%lf , %lf) , node[%d](%lf , %lf) , dir_x_%d , s32ret_%d ",
                        utm_x,utm_y,pastDividerLine[pstRoad[road_i].dividerLine_i]->ID,nodecount,i-1,pnode[i-1].node_x,pnode[i-1].node_y,i,pnode[i].node_x,pnode[i].node_y,dir_x,s32ret);
                return PF_MAP_ERR;
        }
        
    }

    return PF_MAP_SUCCESS;

}






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

GLOBAL S32 pf_map_get_HighWayMileage_by_utm(DOUBLE utm_x, DOUBLE utm_y, S32* integer_km, S32* remainder_m, S32* roadside)
{
    S32    s32ret = PF_MAP_ERR;
    S32    side = 0;
    S32    km = 0;
    DOUBLE distance = 0;

    if(gas32ElementCount[MAP_MILEAGE] == 0)
    {
        pl_log(ERR,"  MAP_MILEAGE count is 0 ");
        return PF_MAP_ERR;
    }

    s32ret = pf_map_get_entrance_distance_by_utm(utm_x, utm_y, pstMileage[0].road_i, &distance, &side);

    if(s32ret == PF_MAP_SUCCESS)
    {
        distance = distance + pstMileage[0].number - pstMileage[0].distance;
    
        km = (S32)(distance/1000);
        
        if(integer_km != NULL)
        {
            *integer_km = km;
        }
        
        if(remainder_m != NULL)
        {
            *remainder_m = (S32)(distance - km*1000);
        }
        
        if(roadside != NULL)
        {
            *roadside = side;
        }
        
        return PF_MAP_SUCCESS;
        
    }

    return PF_MAP_ERR;

}








/**********************************************************************************************
 * @function      pf_map_get_roadName_by_utm
 * @brief         get road name 
 *
 * @input         DOUBLE  utm_x      UTM coordinates x 
 * @input         DOUBLE  utm_y      UTM coordinates y 
 *
 * @output        CHAR*   type       road name    
 *
 * @return        S32                PF_MAP_ERR:     get road name  fail  
 *                                   PF_MAP_SUCCESS: get road name  success 
 *********************************************************************************************/

S32 pf_map_get_roadName_by_utm(DOUBLE utm_x, DOUBLE utm_y, CHAR* roadName)
{
    S32 s32ret = PF_MAP_ERR;
    S32 type = 0;
    S32 index = 0;

    if(roadName == NULL)
    {
        pl_log(ERR," roadName is NULL ");
        return PF_MAP_ERR;
    }
    
    s32ret =  pf_map_get_ID_by_utm(utm_x, utm_y, &type, NULL, &index,NULL, NULL);

    if(s32ret != PF_MAP_SUCCESS)
    {
//        pl_log(ERR," utm( %lf , %lf ) get laneID fail ",utm_x,utm_y );
        return PF_MAP_ERR;
    }

    if(type == MAP_LANE)
    {
        s32ret = pstSection[pstLane[index].section_i].road_i;
        if(s32ret >=0 )
        {
            sprintf(roadName,"%s\0",pstRoad[s32ret].roadName);
            return PF_MAP_SUCCESS;
        }
    }
    else if((type == MAP_CONNECTION)&&(pstConnection[index].road_count>0))
    {
        s32ret = pstConnection[index].road_i[0];
        if(s32ret >=0 )
        {
            sprintf(roadName,"%s\0",pstRoad[s32ret].roadName);
            return PF_MAP_SUCCESS;
        }
    }

    return PF_MAP_ERR;
}







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

S32 pf_map_get_road_index_by_utm(DOUBLE utm_x, DOUBLE utm_y)
{
    S32 type = 0;
    S32 index = 0;
    S32 roadIndex = PF_MAP_ERR;
    S32 s32ret =  pf_map_get_ID_by_utm(utm_x, utm_y, &type, NULL, &index,NULL, NULL);
    
    if(s32ret != PF_MAP_SUCCESS)
    {
        pl_log(ERR," pf_map_get_ID_by_utm(%lf,%lf)  fail",utm_x, utm_y);
        return PF_MAP_ERR;
    }

    if(type == MAP_LANE)
    {
        roadIndex = pstSection[pstLane[index].section_i].road_i;
        if(roadIndex < 0 )
        {
            pl_log(ERR," section[%d]_%d.road_i is invalid ",pstLane[index].section_i,pstSection[pstLane[index].section_i].ID);
            return PF_MAP_ERR;
        }
    }
    else if(type == MAP_NO_RUN_ZONE)
    {
        roadIndex = pastNoRunZone[index]->road_i;
        if(roadIndex <0 )
        {
            pl_log(ERR," noRunZone[%d]_%d.road_i is invalid ",index,pastNoRunZone[index]->ID);
            return PF_MAP_ERR;
        }
    }
    else if((type == MAP_CONNECTION)&&(pstConnection[index].road_count>0))
    {
        roadIndex = pstConnection[index].road_i[0];
        if(roadIndex <0 )
        {
            pl_log(ERR," connection[%d]_%d.road_i is invalid ",index,pstConnection[index].ID);
            return PF_MAP_ERR;
        }
    }
    else
    {
        pl_log(ERR," pf_map_get_road_index_by_utm(%lf,%lf)  fail ",utm_x, utm_y);
        return PF_MAP_ERR;
    }

    return roadIndex;
}








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

S32 pf_map_get_roadInfo_by_index(S32 type,S32 index,S32* road_i,S32* roadType,S32* roadLength,CHAR* roadName)
{
    int i = 0;

    if((index < 0)||(index >= gas32ElementCount[type]))
    {
        pl_log(ERR," type_%d , index_%d  is invalid , validCount_%d",type,index,gas32ElementCount[type]);
        return PF_MAP_ERR;
    }

    if(type == MAP_LANE)
    {
        i = pstSection[pstLane[index].section_i].road_i;
    }
    else if(type == MAP_SECTION)
    {
        i = pstSection[index].road_i;
    }
    else if(type == MAP_ROAD)
    {
        i = index;
    }
    else if(type == MAP_CONNECTION)
    {
        if(pstConnection[index].road_count >= 1)
        {
            i = pstConnection[index].road_i[0];
        }
        else
        {
            pl_log(ERR," connection[%d]_%d , roadcount_%d ",index,pstConnection[index].ID,pstConnection[index].road_count);
            return PF_MAP_ERR;
        }
    }
    else if(type == MAP_NO_RUN_ZONE)
    {
        i = pastNoRunZone[index]->road_i;
    }
    else
    {
        pl_log(ERR," type_%d , index_%d  is invalid , validCount_%d ",type,index,gas32ElementCount[type]);
        return PF_MAP_ERR;
    }

    if(i<0)
    {
        return PF_MAP_ERR;
    }

    if(road_i != NULL)
    {
        *road_i = i;
    }

    if(roadType != NULL)
    {
        *roadType = pstRoad[i].type;
    }

    if(roadLength != NULL)
    {
        *roadLength = pstRoad[i].length;
    }

    if(roadName != NULL)
    {
        sprintf(roadName,"%s\0",pstRoad[i].roadName);
    }

    return PF_MAP_SUCCESS;
}









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

S32 pf_map_get_index_by_id(S32 type, S32 id)
{
    S32   count = 0;
    S32   i = 0;


    if(type == MAP_CONNECTION)
    {
        count = gas32ElementCount[type];
        
        for(i=0;i<count;i++)
        {
            if(id == pstConnection[i].ID)
            {
                return i;
            }
        }
    }
    else if(type == MAP_SECTION)
    {
        count = gas32ElementCount[type];
        
        for(i=0;i<count;i++)
        {
            if(id == pstSection[i].ID)
            {
                return i;
            }
        }
    }
    else if(type == MAP_LANE)
    {
        count = gas32ElementCount[type];
        
        for(i=0;i<count;i++)
        {
            if(id == pstLane[i].ID)
            {
                return i;
            }
        }
    }
    else if(type == MAP_TRAFFICLIGHT)
    {
        count = gas32ElementCount[type];
        
        for(i=0;i<count;i++)
        {
            if(id == pstTrafficLight[i].ID)
            {
                return i;
            }
        }
    }
    else if(type == MAP_PAVEMENT)
    {
        count = gas32ElementCount[type];
        
        for(i=0;i<count;i++)
        {
            if(id == pastPavement[i]->ID)
            {
                return i;
            }
        }
    }
    else if(type == MAP_PARKING_ZONE)
    {
        count = gas32ElementCount[type];
        
        for(i=0;i<count;i++)
        {
            if(id == pastParkingZone[i]->ID)
            {
                return i;
            }
        }
    }
    else if(type == MAP_EXTRAPOINT)
    {
        count = gas32ElementCount[type];
        
        for(i=0;i<count;i++)
        {
            if(id == pastExtrapoint[i]->ID)
            {
                return i;
            }
        }
    }
    else if(type == MAP_LANEMARKER)
    {
        count = gas32ElementCount[type];
        
        for(i=0;i<count;i++)
        {
            if(id == pastLanemarker[i]->ID)
            {
                return i;
            }
        }
    }
    else if(type == MAP_ROAD)
    {
        count = gas32ElementCount[type];
        
        for(i=0;i<count;i++)
        {
            if(id == pstRoad[i].ID)
            {
                return i;
            }
        }
    }
    else if(type == MAP_ROADEDGE)
    {
        count = gas32ElementCount[type];
        
        for(i=0;i<count;i++)
        {
            if(id == pastRoadEdge[i]->ID)
            {
                return i;
            }
        }
    }
    else if(type == MAP_NO_RUN_ZONE)
    {
        count = gas32ElementCount[type];
        
        for(i=0;i<count;i++)
        {
            if(id == pastNoRunZone[i]->ID)
            {
                return i;
            }
        }
    }
    else if(type == MAP_CENTER_LINE)
    {
        count = gas32ElementCount[type];
        
        for(i=0;i<count;i++)
        {
            if(id == pastDividerLine[i]->ID)
            {
                return i;
            }
        }
    }
    else if(type == MAP_MILEAGE)
    {
        count = gas32ElementCount[type];
        
        for(i=0;i<count;i++)
        {
            if(id == pstMileage[i].ID)
            {
                return i;
            }
        }
    }
    
    return PF_MAP_ERR;
}



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

S32 pf_map_get_id_by_index(S32 type, S32 index)
{
    S32   count = 0;
    S32   i = index;

    if(type == MAP_CONNECTION)
    {
        if((i>=0)&&(i<gas32ElementCount[type]))
        {
            return pstConnection[i].ID;
        }
    }
    else if(type == MAP_SECTION)
    {
        if((i>=0)&&(i<gas32ElementCount[type]))
        {
            return pstSection[i].ID;
        }
    }
    else if(type == MAP_LANE)
    {
        if((i>=0)&&(i<gas32ElementCount[type]))
        {
            return pstLane[i].ID;
        }
    }
    else if(type == MAP_TRAFFICLIGHT)
    {
        if((i>=0)&&(i<gas32ElementCount[type]))
        {
            return pstTrafficLight[i].ID;
        }
    }
    else if(type == MAP_PAVEMENT)
    {
        if((i>=0)&&(i<gas32ElementCount[type]))
        {
            return pastPavement[i]->ID;
        }
    }
    else if(type == MAP_PARKING_ZONE)
    {
        if((i>=0)&&(i<gas32ElementCount[type]))
        {
            return pastParkingZone[i]->ID;
        }
    }
    else if(type == MAP_EXTRAPOINT)
    {
        if((i>=0)&&(i<gas32ElementCount[type]))
        {
            return pastExtrapoint[i]->ID;
        }
    }
    else if(type == MAP_LANEMARKER)
    {
        if((i>=0)&&(i<gas32ElementCount[type]))
        {
            return pastLanemarker[i]->ID;
        }
    }
    else if(type == MAP_ROAD)
    {
        if((i>=0)&&(i<gas32ElementCount[type]))
        {
            return pstRoad[i].ID;
        }
    }
    else if(type == MAP_ROADEDGE)
    {
        if((i>=0)&&(i<gas32ElementCount[type]))
        {
            return pastRoadEdge[i]->ID;
        }
    }
    else if(type == MAP_NO_RUN_ZONE)
    {
        if((i>=0)&&(i<gas32ElementCount[type]))
        {
            return pastNoRunZone[i]->ID;
        }
    }
    else if(type == MAP_CENTER_LINE)
    {
        if((i>=0)&&(i<gas32ElementCount[type]))
        {
            return pastDividerLine[i]->ID;
        }
    }
    else if(type == MAP_MILEAGE)
    {
        if((i>=0)&&(i<gas32ElementCount[type]))
        {
            return pstMileage[i].ID;
        }
    }
    
    return PF_MAP_ERR;
}






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

DOUBLE pf_map_get_utm_z_by_xy(DOUBLE utm_x, DOUBLE utm_y)
{
    S32   s32x = S32(utm_x);
    S32   s32y = S32(utm_y);
    S32   i = 0;
    S32   step = 0;

    while(i<gs32MapUtmXYZCount)
    {
        step = s32x - pstMapUtmXYZ[i].x;
        if(step > 0)
        {
            i = i+ step*gs32MapUtmX_Y;

            if(i>=gs32MapUtmXYZCount)
            {
                i = gs32MapUtmXYZCount-1;
            }
            continue;
        }
        else 
        {
            step = i;
            
            if(pstMapUtmXYZ[i].x == s32x)
            {
                while(i < gs32MapUtmXYZCount)
                {
                    if(pstMapUtmXYZ[i].x > s32x)
                    {
                        break;
                    }
                    else if((pstMapUtmXYZ[i].x == s32x)&&(pstMapUtmXYZ[i].y == s32y))
                    {
                        return pstMapUtmXYZ[i].z;
                    }
                    
                    i++;
                }
            }

            i = step-1;
            
            while(i >= 0)
            {
                if(pstMapUtmXYZ[i].x < s32x)
                {
                    return PF_MAP_ERR;
                }
                else if((pstMapUtmXYZ[i].x == s32x)&&(pstMapUtmXYZ[i].y == s32y))
                {
                    return pstMapUtmXYZ[i].z;
                }
                
                i--;
            }

            return PF_MAP_ERR;
        }
    }
    
    return PF_MAP_ERR;
}





/**********************************************************************************************
 * @function      pf_map_get_trafficLight_count
 * @brief         get trafficLight count
 *
 * @input         void
 *
 * @return        S32    trafficLight count  
 *                                   
 *********************************************************************************************/

S32 pf_map_get_trafficLight_count(void)
{
    return gas32ElementCount[MAP_TRAFFICLIGHT];
}








/**********************************************************************************************
 * @function      pf_map_get_lampGroup_count_by_index
 * @brief         get lamp group count   
 *
 * @input         S32  trafficLight_i         trafficLight index 
 *
 * @return        S32  lamp group count       PF_MAP_ERR: get lamp group count fail
 *                                            else   lamp group count
 *                                                 
 *********************************************************************************************/

S32 pf_map_get_lampGroup_count_by_index(S32  trafficLight_i)
{
    if((trafficLight_i<0)||(trafficLight_i >= gas32ElementCount[MAP_TRAFFICLIGHT]))
    {
        pl_log(ERR," trafficLight_i_%d  is invalid , trafficLightCount_%d",trafficLight_i,gas32ElementCount[MAP_TRAFFICLIGHT]);
        return PF_MAP_ERR;
    }
   
    return  S32(pstTrafficLight[trafficLight_i].lampGroupCount);
}





/**********************************************************************************************
 * @function      pf_map_get_lampGroup_info_by_index
 * @brief         get lamp group info   
 *
 * @input         S32  trafficLight_i        trafficLight index 
 * @input         S32  lampGroup_i           lampGroup index         注释: 从0开始 
 *
 * @output        PF_API_LAMP_GROUP_ST*      the struct of lamp group info 
 *                                              
 * @return        S32                        PF_MAP_ERR:     fail  
 *                                           PF_MAP_SUCCESS: success
 *********************************************************************************************/

S32 pf_map_get_lampGroup_info_by_index(S32 trafficLight_i,S32 lampGroup_i,PF_API_LAMP_GROUP_ST* pstLampGroupInfo)
{
    S32 bindLaneCount = 0;
    S32 hasBicycleLamp = 0;

    if((trafficLight_i<0)||(trafficLight_i >= gas32ElementCount[MAP_TRAFFICLIGHT]))
    {
        pl_log(ERR," trafficLight_i_%d  is invalid , trafficLightCount_%d",trafficLight_i,gas32ElementCount[MAP_TRAFFICLIGHT]);
        return PF_MAP_ERR;
    }

    if((lampGroup_i<0)||(lampGroup_i >= pstTrafficLight[trafficLight_i].lampGroupCount))
    {
        pl_log(ERR," lampGroup_i_%d  is invalid , trafficLight[%d]_%d  lampGroupCount_%d",lampGroup_i,trafficLight_i,
                pstTrafficLight[trafficLight_i].ID,pstTrafficLight[trafficLight_i].lampGroupCount);
        return PF_MAP_ERR;
    }

    pstLampGroupInfo->index = pstTrafficLight[trafficLight_i].lamp_group[lampGroup_i].index;
    pstLampGroupInfo->light_type = pstTrafficLight[trafficLight_i].lamp_group[lampGroup_i].light_type;
    pstLampGroupInfo->turn_type = pstTrafficLight[trafficLight_i].lamp_group[lampGroup_i].turn_type;

    if(pstLampGroupInfo->light_type == LIGHT_WALK)  // 行人信号灯
    {
        pstLampGroupInfo->bind_lane_count = 0;
        return PF_MAP_SUCCESS;
    }


    for(int k=0;k< pstTrafficLight[trafficLight_i].lampGroupCount;k++)
    {
        if((pstTrafficLight[trafficLight_i].lamp_group[k].light_type == LIGHT_BICYCLE)||
            (pstTrafficLight[trafficLight_i].lamp_group[k].light_type == LIGHT_BICYCLE_STRAIGHT)||
            (pstTrafficLight[trafficLight_i].lamp_group[k].light_type == LIGHT_BICYCLE_LEFT))
        {
            hasBicycleLamp = 1;
            break;
        }
    }
    

    for(int i=0;i< pstTrafficLight[trafficLight_i].sectionCount;i++)
    {
        S32 lanecount = pstSection[pstTrafficLight[trafficLight_i].sectionIndexList[i]].laneCount;
        S32 lane_i = pstSection[pstTrafficLight[trafficLight_i].sectionIndexList[i]].lane_i;
        
        for(int j=0;j<lanecount;j++)
        {
            if(pstTrafficLight[trafficLight_i].lamp_group[lampGroup_i].bind_lane_count > 0)
            {
                for(int k=0;k<pstTrafficLight[trafficLight_i].lamp_group[lampGroup_i].bind_lane_count;k++)
                {
                    if(pstLane[lane_i+j].laneNumber == pstTrafficLight[trafficLight_i].lamp_group[lampGroup_i].bind_lane_index[k])
                    {
                        pstLampGroupInfo->bind_lane_i[bindLaneCount] = (S16)(lane_i+j);
                        bindLaneCount++;
                        break;
                    }
                }
            }
            else if((hasBicycleLamp == 0)||((pstLane[lane_i+j].type != LANE_BICYCLE)&&(pstTrafficLight[trafficLight_i].lamp_group[lampGroup_i].light_type < LIGHT_BICYCLE))||
                    ((pstLane[lane_i+j].type == LANE_BICYCLE)&&((pstTrafficLight[trafficLight_i].lamp_group[lampGroup_i].light_type >= LIGHT_BICYCLE)||
                                                                (pstTrafficLight[trafficLight_i].lamp_group[lampGroup_i].light_type >= LIGHT_BICYCLE_STRAIGHT)||
                                                                (pstTrafficLight[trafficLight_i].lamp_group[lampGroup_i].light_type >= LIGHT_BICYCLE_LEFT))))
            {
                S8 lampTurnType = 0;
                S8 laneTurnType = pstLane[lane_i+j].turnType;

                if((laneTurnType == LANE_NONE)&&(pstLane[lane_i+j].type == LANE_BICYCLE))
                {
                    laneTurnType = LANE_LEFT_STRAIGHT_RIGHT;
                }

                switch(pstTrafficLight[trafficLight_i].lamp_group[lampGroup_i].turn_type)
                {
                    case LIGHT_STRAIGHT:
                        lampTurnType = LANE_STRAIGHT;
                        break;
                    
                    case LIGHT_TURN_LEFT:
                        lampTurnType = LANE_TURN_LEFT;
                        break;

                    case LIGHT_TURN_RIGHT:
                        lampTurnType = LANE_TURN_RIGHT;
                        break;
                        
                    case LIGHT_STRAIGHT_LEFT:
                        lampTurnType = LANE_STRAIGHT_LEFT;
                        break;

                    case LIGHT_STRAIGHT_RIGHT:
                        lampTurnType = LANE_STRAIGHT_RIGHT;
                        break;
                        
                    case LIGHT_LEFT_RIGHT:
                        lampTurnType = LANE_LEFT_RIGHT;
                        break;
                        
                    case LIGHT_LEFT_STRAIGHT_RIGHT:
                        lampTurnType = LANE_LEFT_STRAIGHT_RIGHT;
                        break;
                        
                    case LIGHT_TURN_AROUND:
                        lampTurnType = LANE_TURN_AROUND;
                        break;
                        
                    default:
                        lampTurnType = 0;
                        break;
                }
                
                if(0 != (laneTurnType&lampTurnType))
                {
                    pstLampGroupInfo->bind_lane_i[bindLaneCount] = (S16)(lane_i+j);
                    bindLaneCount++;
                }
            }
        }

    }

    pstLampGroupInfo->bind_lane_count = bindLaneCount;

    return PF_MAP_SUCCESS;
}




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

S32 pf_map_get_lane_end_by_index(S32 lane_i,DOUBLE* left_utmx, DOUBLE* left_utmy,            DOUBLE* right_utmx, DOUBLE* right_utmy)
{
    if((lane_i<0)||(lane_i >= gas32ElementCount[MAP_LANE]))
    {
        pl_log(ERR," lane_i_%d  is invalid , laneCount_%d",lane_i,gas32ElementCount[MAP_LANE]);
        return PF_MAP_ERR;
    }


    PF_MAP_NODE_LINE_ST* pnodeline = (PF_MAP_NODE_LINE_ST*)((U8*)(pastLanemarker[pstLane[lane_i].leftLanemarker_i])+sizeof(PF_MAP_LINE_INFO_ST));

    S32 lastNode_i = pastLanemarker[pstLane[lane_i].leftLanemarker_i]->nodeCount-1;

    
    if(left_utmx != NULL)
    {
        *left_utmx = pnodeline[lastNode_i].node_x;
    }

    if(left_utmy != NULL)
    {
        *left_utmy = pnodeline[lastNode_i].node_y;
    }

    pnodeline = (PF_MAP_NODE_LINE_ST*)((U8*)(pastLanemarker[pstLane[lane_i].rightLanemarker_i])+sizeof(PF_MAP_LINE_INFO_ST));

    lastNode_i = pastLanemarker[pstLane[lane_i].rightLanemarker_i]->nodeCount-1;

    if(right_utmx != NULL)
    {
        *right_utmx = pnodeline[lastNode_i].node_x;
    }

    if(right_utmy != NULL)
    {
        *right_utmy = pnodeline[lastNode_i].node_y;
    }

    return PF_MAP_SUCCESS;

}




/**********************************************************************************************
 * @function      pf_map_get_connection_count
 * @brief         get connection count
 *
 * @input         void
 *
 * @return        S32    connection count  
 *                                   
 *********************************************************************************************/

GLOBAL S32 pf_map_get_connection_count(void)
{
    return gas32ElementCount[MAP_CONNECTION];
}






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

GLOBAL S32 pf_map_get_connection_trafficLight_info(S32 connection_i, S32* ptrafficLightList)
{
    S32 lightCount = 0;
    S32 lightList[16] = {0};
    
    if((connection_i<0)||(connection_i >= gas32ElementCount[MAP_CONNECTION]))
    {
        pl_log(ERR," connection_i_%d  is invalid , connectionCount_%d",connection_i,gas32ElementCount[MAP_CONNECTION]);
        return PF_MAP_ERR;
    }

    for(S32 i=0;i<pstConnection[connection_i].linkCount;i++)
    {
        S32 link_i = pstConnection[connection_i].link_i+i;
        
        if((pstLink[link_i].fromSection_i>=0)&&(pstSection[pstLink[link_i].fromSection_i].tracficLightCount > 0))
        {
            S32 isnew = 1;
            
            for(S32 k=0;k<lightCount;k++)
            {
                if(lightList[k] == pstSection[pstLink[link_i].fromSection_i].tracficLight_i)
                {
                    isnew = 0;
                    break;
                }
            }

            if(isnew == 1)
            {
                lightList[lightCount] = pstSection[pstLink[link_i].fromSection_i].tracficLight_i;
                lightCount++;
            }
        }
    }

    if(ptrafficLightList != NULL)
    {
        for(S32 k=0;k<lightCount;k++)
        {
            ptrafficLightList[k] = lightList[k];
        }
    }

    return lightCount;
}






#undef PF_MAP_BLOCK_CPP


