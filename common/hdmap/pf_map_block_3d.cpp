/************************************************************
  Copyright (C), BroadXT Inc  2019
  FileName   :pf_map_block_3d.cpp
  Author     :zhangyan     
  Version    :v0.1         
  Date       :2025-06-05
  Description:
  History:
  <author>       <time>       <version >   <desc>
  zhangyan       2025-06-05   v0.1         create 
***********************************************************/
#define THIS_MODULE MODULE_COMM

#define PF_MAP_BLOCK_3D_CPP

#include "pf_map_block_3d.h"
#include "utm.h"
#include <fstream>

using namespace std;



PF_MAP_INFO_ST     gMap={0};

PF_MAP_AREA_ST*    pstRestriction = NULL;
PF_MAP_AREA_ST*    pstGround = NULL;
PF_MAP_XYZ_ST*     pstNode = NULL;
PF_MAP_BLOCK_ST*   pstBlock = NULL;

// PF_NODE_ST gPolygon[POLYGON_NODE_COUNT_MAX] = {0};
// S32 gPolygonNodeCount = 0;

U32 gu32MapErrCount = 0;

S32 check_block_around_lonlat(DOUBLE lat,DOUBLE lon,DOUBLE z,DOUBLE dis_xy ,DOUBLE dis_z);

#define LINE_MAX  1024
S32 map_get_map_info_count(ifstream &stIFile)
{
    DOUBLE dbx = 0;
    DOUBLE dby = 0;
    DOUBLE dbz = 0;
    S32    memSize = 0;
    S16    s16ret = 0;

    gMap.nodeCount = 0;
    gMap.resdictionCount= 0;
    gMap.groundCount= 0;
    gMap.block_count_x = 0;
    gMap.block_count_y = 0;
    gMap.block_count_z = 0;

    while(!stIFile.eof())
    {
        CHAR   lineBuf[LINE_MAX] = {0};
        stIFile.getline(lineBuf, LINE_MAX);

        if(NULL != strstr(lineBuf,"<node x="))
        {
            gMap.nodeCount++;
        }
        else if(NULL != strstr(lineBuf,"<restriction_area id="))
        {
            gMap.resdictionCount++;
        }
        else if(NULL != strstr(lineBuf,"<ground_area id="))
        {
            gMap.groundCount++;
        }
        else if(NULL != strstr(lineBuf,"<utm_min "))
        {
            s16ret = sscanf(lineBuf, " <utm_min x=\"%lf\" y=\"%lf\" z=\"%lf\"", &dbx,&dby,&dbz);
            
            if(s16ret == 3)
            {
                gMap.utm_min.x = dbx;
                gMap.utm_min.y = dby;
                gMap.utm_min.z = dbz;
            }
            else
            {
                gu32MapErrCount++;
                pf_map_log("\n MapErr_%d:   get <utm_min>  fail!  s16ret = %d \n%s \n\n",
                                gu32MapErrCount,s16ret,lineBuf);
            }        
        }
        else if(NULL != strstr(lineBuf,"<utm_max "))
        {
            s16ret = sscanf(lineBuf, " <utm_max x=\"%lf\" y=\"%lf\" z=\"%lf\"", &dbx,&dby,&dbz);
            
            if(s16ret == 3)
            {
                gMap.utm_max.x = dbx;
                gMap.utm_max.y = dby;
                gMap.utm_max.z = dbz;
            }
            else
            {
                gu32MapErrCount++;
                pf_map_log("\n MapErr_%d:   get <utm_max>  fail!  s16ret = %d \n%s \n\n",
                                gu32MapErrCount,s16ret,lineBuf);
            }        
        }
        else if(NULL != strstr(lineBuf,"<block_min "))
        {
            s16ret = sscanf(lineBuf, " <block_min x=\"%lf\" y=\"%lf\" z=\"%lf\"", &dbx,&dby,&dbz);
            
            if(s16ret == 3)
            {
                gMap.block_min.x = dbx;
                gMap.block_min.y = dby;
                gMap.block_min.z = dbz;
            }
            else
            {
                gu32MapErrCount++;
                pf_map_log("\n MapErr_%d:   get <block_min>  fail!  s16ret = %d \n%s \n\n",
                                gu32MapErrCount,s16ret,lineBuf);
            }        
        }
        else if(NULL != strstr(lineBuf,"<block_max "))
        {
            s16ret = sscanf(lineBuf, " <block_max x=\"%lf\" y=\"%lf\" z=\"%lf\"", &dbx,&dby,&dbz);
            
            if(s16ret == 3)
            {
                gMap.block_max.x = dbx;
                gMap.block_max.y = dby;
                gMap.block_max.z = dbz;
            }
            else
            {
                gu32MapErrCount++;
                pf_map_log("\n MapErr_%d:   get <block_max>  fail!  s16ret = %d \n%s \n\n",
                                gu32MapErrCount,s16ret,lineBuf);
            }        
        }
        else if(NULL != strstr(lineBuf,"<block_size "))
        {
            s16ret = sscanf(lineBuf, " <block_size x=\"%lf\" y=\"%lf\" z=\"%lf\"", &dbx,&dby,&dbz);
            
            if(s16ret == 3)
            {
                gMap.block_size.x = dbx;
                gMap.block_size.y = dby;
                gMap.block_size.z = dbz;
            }
            else
            {
                gu32MapErrCount++;
                pf_map_log("\n MapErr_%d:   get <block_size>  fail!  s16ret = %d \n%s \n\n",
                                gu32MapErrCount,s16ret,lineBuf);
            }        
        }


    }


    gMap.block_count_x = S32((gMap.block_max.x - gMap.block_min.x+gMap.block_size.x/2)/gMap.block_size.x);
    gMap.block_count_y = S32((gMap.block_max.y - gMap.block_min.y+gMap.block_size.y/2)/gMap.block_size.y);
    gMap.block_count_z = S32((gMap.block_max.z - gMap.block_min.z+gMap.block_size.z/2)/gMap.block_size.z);

    memSize = gMap.resdictionCount*sizeof(PF_MAP_AREA_ST); 
    memSize += gMap.groundCount*sizeof(PF_MAP_AREA_ST); 
    memSize += gMap.nodeCount*sizeof(PF_MAP_XYZ_ST); 
    memSize += gMap.block_count_x*gMap.block_count_y*sizeof(PF_MAP_BLOCK_ST); 


    pf_map_log("\n ****** MAP_INFO ****** ");
    pf_map_log("\n utm_min:[ %lf , %lf , %lf ]",gMap.utm_min.x,gMap.utm_min.y,gMap.utm_min.z);
    pf_map_log("\n utm_max:[ %lf , %lf , %lf ]",gMap.utm_max.x,gMap.utm_max.y,gMap.utm_max.z);
    pf_map_log("\n block_min:[ %lf , %lf , %lf ]",gMap.block_min.x,gMap.block_min.y,gMap.block_min.z);
    pf_map_log("\n block_max:[ %lf , %lf , %lf ]",gMap.block_max.x,gMap.block_max.y,gMap.block_max.z);
    pf_map_log("\n block_size:[ %lf , %lf , %lf ]",gMap.block_size.x,gMap.block_size.y,gMap.block_size.z);
    pf_map_log("\n block_count:[ %d , %d , %d ]",gMap.block_count_x,gMap.block_count_y,gMap.block_count_z);
    pf_map_log("\n resdictionCount:  %d ",gMap.resdictionCount);
    pf_map_log("\n groundCount:  %d ",gMap.groundCount);
    pf_map_log("\n nodeCount:  %d ",gMap.nodeCount);
    pf_map_log("\n memSize:  %d ",memSize);
    pf_map_log("\n ********************** \n");
    
    if(gu32MapErrCount > 0)
    {
        return RET_ERR;
    }

    U8* pmaproot = (U8*)malloc(memSize);

    
    if(pmaproot == NULL)
    {
        pf_map_log("\n Err: malloc fail!  size_%ld ",memSize);
        return RET_ERR;
    }
    
    memset(pmaproot,0,memSize);

    pstRestriction = (PF_MAP_AREA_ST*)pmaproot;
    pstGround = (PF_MAP_AREA_ST*)(pstRestriction + gMap.resdictionCount); 
    pstNode = (PF_MAP_XYZ_ST*)(pstGround + gMap.groundCount); 
    pstBlock = (PF_MAP_BLOCK_ST*)(pstNode + gMap.nodeCount); 

    return RET_OK;
        
}






/**
* nodeCount������εĶ�����Ŀ
* px�����x ����
* py�����y ����
* poly������ζ��������
* ����1�����ڶ���η�Χ��
* ����0���㲻�ڶ���η�Χ��
*/
S32 pnpoly(S32 nodeCount, PF_MAP_XYZ_ST *poly, DOUBLE px, DOUBLE py) 
{
    int i, j, c = 0;
    for (i = 0, j = nodeCount - 1; i < nodeCount; j = i++) 
    {
        if (((poly[i].y > py) != (poly[j].y > py)) &&
            (px < (poly[j].x - poly[i].x) * (py - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x))
        {
            c = !c;
        }
    }
    return c;
}




S32 check_in_area(DOUBLE px, DOUBLE py,PF_MAP_AREA_ST parea)
{
    if((px<parea.min_x)||(px>parea.max_x)||(py<parea.min_y)||(py>parea.max_y))
    {
        return 0;
    }

    S32 ret = pnpoly(parea.node_count,pstNode+parea.node_i, px, py);

    return ret;
}





void set_map_block_area(void)
{
    S32 ret = 0;
    PF_MAP_BLOCK_ST (*pblock)[gMap.block_count_x] = (PF_MAP_BLOCK_ST(*)[gMap.block_count_x])pstBlock;
    
    for(int j=0;j<gMap.block_count_y;j++)
    {
        DOUBLE dby = gMap.block_min.y + gMap.block_size.y*j;
        for(int i=0;i<gMap.block_count_x;i++)
        {
            DOUBLE dbx = gMap.block_min.x + gMap.block_size.x*i;
            for(int k=0;k<gMap.resdictionCount;k++)
            {
                ret = check_in_area(dbx, dby,pstRestriction[k]);
                    
                if(ret == 0)
                {
                   ret = check_in_area(dbx+gMap.block_size.x, dby,pstRestriction[k]);
                }
                
                if(ret == 0)
                {
                   ret = check_in_area(dbx+gMap.block_size.x, dby+gMap.block_size.y,pstRestriction[k]);
                }
                
                if(ret == 0)
                {
                   ret = check_in_area(dbx, dby+gMap.block_size.y,pstRestriction[k]);
                }
                
                if(ret == 1)
                {
                    if((pblock[j][i].resdiction_i == 0)||(pstRestriction[k].height_limit == RESTRICT_NO_FLY)||
                        (pstRestriction[k].height_limit > pstRestriction[pblock[j][i].resdiction_i-1].height_limit))
                    {
                        pblock[j][i].resdiction_i = k+1;

                        //pf_map_log("\n %lf  %lf , block[%d][%d] , resdiction[%d]_%d ",
                        //    dbx,dby,j,i,pblock[j][i].resdiction_i,pstRestriction[pblock[j][i].resdiction_i].ID);
                    }
                }
            }

            if(pblock[j][i].ground_i == 0)  
            {
                for(int k=0;k<gMap.groundCount;k++)
                {
                    ret = check_in_area(dbx, dby,pstGround[k]);
                        
                    if(ret == 0)
                    {
                       ret = check_in_area(dbx+gMap.block_size.x, dby,pstGround[k]);
                    }
                    
                    if(ret == 0)
                    {
                       ret = check_in_area(dbx+gMap.block_size.x, dby+gMap.block_size.y,pstGround[k]);
                    }
                    
                    if(ret == 0)
                    {
                       ret = check_in_area(dbx, dby+gMap.block_size.y,pstGround[k]);
                    }
                    
                    if(ret == 1)
                    {
                        pblock[j][i].ground_i= k+1;
                    }
                }
            }
        }
    }
}






/**********************************************************************************************
 * @function      get_map_info
 * @brief           get map info form  .xml  file  
 * @input          char*    mapfilename (include path)
 * @output        void
 * @return         S32      RET_ERR:  fail   
 *                                 RET_OK:   success
 *********************************************************************************************/

S32 get_map_info(CHAR* mapfilename)
{
    S32    s32ret = RET_ERR;
    S16    s16ret = 0;
    DOUBLE dbx = 0;
    DOUBLE dby = 0;
    DOUBLE dbz = 0;
    S32    s32value = 0;
    S32    blockCount = 0;
    S32    nodeCount = 0;
    S32    resdictionCount = 0;
    S32    groundCount = 0;


    ifstream stIFile(mapfilename);

    if(!stIFile.good())
    {
        pf_map_log("\n Err: open file: %s  fail \r\n\r\n",mapfilename);
        return RET_ERR;
    }

    s32ret = map_get_map_info_count(stIFile);
    
    if(s32ret != RET_OK)
    {
        stIFile.close();
        return RET_ERR;
    }

    PF_MAP_BLOCK_ST (*pblock)[gMap.block_count_x] = (PF_MAP_BLOCK_ST(*)[gMap.block_count_x])pstBlock;

    stIFile.clear();
    stIFile.seekg(0,ios_base::beg);

    while(!stIFile.eof())
    {
        CHAR   lineBuf[LINE_MAX] = {0};
        stIFile.getline(lineBuf, LINE_MAX);

        if(NULL != strstr(lineBuf,"<block id="))
        {
            CHAR*  pstr = strstr(lineBuf," x=");
            s16ret = sscanf(pstr, " x=\"%lf\" y=\"%lf\" z=\"%lf\"", &dbx,&dby,&dbz);
            
            if(s16ret == 3)
            {
                S32 xi = S32((dbx-gMap.block_min.x+gMap.block_size.x/2)/gMap.block_size.x);
                S32 yi = S32((dby-gMap.block_min.y+gMap.block_size.y/2)/gMap.block_size.y);
                //S32 xi = S32((dbx-gMap.block_min.x)/gMap.block_size.x);
                //S32 yi = S32((dby-gMap.block_min.y)/gMap.block_size.y);
                if ((xi>=gMap.block_count_x)||(yi>=gMap.block_count_y))
                {
                    //gu32MapErrCount++;
                    pf_map_log("\n Err:   block[%d][%d]  not in  blockCount[%d][%d]  \n%s \n\n",
                                    yi,xi,gMap.block_count_y,gMap.block_count_x,lineBuf);
                    //goto end;
                }
                else
                {
                    pblock[yi][xi].z_i = S32((dbz-gMap.block_min.z+gMap.block_size.z/2)/gMap.block_size.z) + 1;
                    pblock[yi][xi].zmax_mm =  S32(dbz*1000);
                    blockCount++;
                }
            }
            else
            {
                gu32MapErrCount++;
                pf_map_log("\n MapErr_%d:   get <block>  fail!  s16ret = %d \n%s \n\n",
                                gu32MapErrCount,s16ret,lineBuf);
                goto end;
            }        
        }
        else if(NULL != strstr(lineBuf,"<points id="))
        {
            s16ret = sscanf(lineBuf, " <points id=\"%d\"", &s32value);
            if(s16ret == 1)
            {
                S32 node_i = nodeCount;
                DOUBLE max_x = 0;
                DOUBLE max_y = 0;
                DOUBLE min_x = 999999999.9;
                DOUBLE min_y = 999999999.9;
                
                while(!stIFile.eof())
                {
                    stIFile.getline(lineBuf, LINE_MAX);
                    if(NULL != strstr(lineBuf,"<node x="))
                    {
                        s16ret = sscanf(lineBuf, " <node x=\"%lf\" y=\"%lf\" z=\"%lf\"", &dbx,&dby,&dbz);
                        
                        if(s16ret == 3)
                        {
                            pstNode[nodeCount].x = dbx;
                            pstNode[nodeCount].y = dby;
                            pstNode[nodeCount].z = dbz;
                            nodeCount++;

                            if(max_x < dbx) 
                            {
                                max_x = dbx;
                            }
                            
                            if(max_y < dby) 
                            {
                                max_y = dby;
                            }
                            
                            if(min_x > dbx) 
                            {
                                min_x = dbx;
                            }
                            
                            if(min_y > dby) 
                            {
                                min_y = dby;
                            }
                            
                        }
                        else
                        {
                            gu32MapErrCount++;
                            pf_map_log("\n MapErr_%d:   get <node>  fail!  s16ret = %d \n%s \n\n",
                                            gu32MapErrCount,s16ret,lineBuf);
                        }     
                    }
                    else if(NULL != strstr(lineBuf,"</points>"))
                    {
                        break;
                    }
                }

                s16ret = 0;
                for(int i=0;i<resdictionCount;i++)
                {
                    if(pstRestriction[i].extrapointsID == s32value) 
                    {
                        pstRestriction[i].node_i = node_i;
                        pstRestriction[i].node_count = nodeCount - node_i;
                        pstRestriction[i].max_x = max_x;
                        pstRestriction[i].max_y = max_y;
                        pstRestriction[i].min_x = min_x;
                        pstRestriction[i].min_y = min_y;
                        s16ret++;
                    }
                }

                for(int i=0;i<groundCount;i++)
                {
                    if(pstGround[i].extrapointsID == s32value) 
                    {
                        pstGround[i].node_i = node_i;
                        pstGround[i].node_count = nodeCount - node_i;
                        pstGround[i].max_x = max_x;
                        pstGround[i].max_y = max_y;
                        pstGround[i].min_x = min_x;
                        pstGround[i].min_y = min_y;
                        s16ret++;
                    }
                }

                if(s16ret == 0)
                {
                    gu32MapErrCount++;
                    pf_map_log("\n MapErr_%d:  <points id = %d > is not used! \n\n",
                                    gu32MapErrCount,s32value);
                } 
                
            }
            else
            {
                gu32MapErrCount++;
                pf_map_log("\n MapErr_%d:   get <points id>  fail!  s16ret = %d \n%s \n\n",
                                gu32MapErrCount,s16ret,lineBuf);
            }     
        }
        else if(NULL != strstr(lineBuf,"<restriction_area id="))
        {
            s16ret = sscanf(lineBuf, " <restriction_area id=\"%d\"", &s32value);
            
            if(s16ret == 1)
            {
                pstRestriction[resdictionCount].ID = s32value;

                while(!stIFile.eof())
                {
                    stIFile.getline(lineBuf, LINE_MAX);
                
                    if(NULL != strstr(lineBuf,"<name val="))
                    {
                        s16ret = sscanf(lineBuf, " <name val=\"%s\"", pstRestriction[resdictionCount].name);
                        
                        if(s16ret != 1)
                        {
                            gu32MapErrCount++;
                            pf_map_log("\n MapErr_%d:   get <restriction_area name>  fail!  s16ret = %d \n%s \n\n",
                                            gu32MapErrCount,s16ret,lineBuf);
                        }
                    }
                    else if(NULL != strstr(lineBuf,"<type val="))
                    {
                        s16ret = sscanf(lineBuf, " <type val=\"%d\"", &s32value);
                        
                        if(s16ret == 1)
                        {
                            pstRestriction[resdictionCount].type = s32value;
                        }
                        else
                        {
                            gu32MapErrCount++;
                            pf_map_log("\n MapErr_%d:   get <restriction_area type>  fail!  s16ret = %d \n%s \n\n",
                                            gu32MapErrCount,s16ret,lineBuf);
                        }
                    }
                    else if(NULL != strstr(lineBuf,"<height_limit val="))
                    {
                        s16ret = sscanf(lineBuf, " <height_limit val=\"%lf\"", &dbx);
                        
                        if(s16ret == 1)
                        {
                            pstRestriction[resdictionCount].height_limit = dbx;
                        }
                        else
                        {
                            gu32MapErrCount++;
                            pf_map_log("\n MapErr_%d:   get <restriction_area height_limit>  fail!  s16ret = %d \n%s \n\n",
                                            gu32MapErrCount,s16ret,lineBuf);
                        }
                    }
                    else if(NULL != strstr(lineBuf,"<extra_points ref="))
                    {
                        s16ret = sscanf(lineBuf, " <extra_points ref=\"%d\"", &s32value);
                        
                        if(s16ret == 1)
                        {
                            pstRestriction[resdictionCount].extrapointsID = s32value;
                        }
                        else
                        {
                            gu32MapErrCount++;
                            pf_map_log("\n MapErr_%d:   get <restriction_area extra_points>  fail!  s16ret = %d \n%s \n\n",
                                            gu32MapErrCount,s16ret,lineBuf);
                        }
                    }
                    else if(NULL != strstr(lineBuf,"</restriction_area>"))
                    {
                        break;
                    }
                }
                
                resdictionCount++;
            }
            else
            {
                gu32MapErrCount++;
                pf_map_log("\n MapErr_%d:   get <restriction_area id>  fail!  s16ret = %d \n%s \n\n",
                                gu32MapErrCount,s16ret,lineBuf);
            }    

            
        }
        else if(NULL != strstr(lineBuf,"<ground_area id="))
        {
            s16ret = sscanf(lineBuf, " <ground_area id=\"%d\"", &s32value);
            
            if(s16ret == 1)
            {
                pstGround[groundCount].ID = s32value;
        
                while(!stIFile.eof())
                {
                    stIFile.getline(lineBuf, LINE_MAX);
                
                    if(NULL != strstr(lineBuf,"<name val="))
                    {
                        s16ret = sscanf(lineBuf, " <name val=\"%s\"", pstGround[groundCount].name);
                        
                        if(s16ret != 1)
                        {
                            gu32MapErrCount++;
                            pf_map_log("\n MapErr_%d:   get <ground_area name>  fail!  s16ret = %d \n%s \n\n",
                                            gu32MapErrCount,s16ret,lineBuf);
                        }
                    }
                    else if(NULL != strstr(lineBuf,"<type val="))
                    {
                        s16ret = sscanf(lineBuf, " <type val=\"%d\"", &s32value);
                        
                        if(s16ret == 1)
                        {
                            pstGround[groundCount].type = s32value;
                        }
                        else
                        {
                            gu32MapErrCount++;
                            pf_map_log("\n MapErr_%d:   get <ground_area type>  fail!  s16ret = %d \n%s \n\n",
                                            gu32MapErrCount,s16ret,lineBuf);
                        }
                    }
                    else if(NULL != strstr(lineBuf,"<extra_points ref="))
                    {
                        s16ret = sscanf(lineBuf, " <extra_points ref=\"%d\"", &s32value);
                        
                        if(s16ret == 1)
                        {
                            pstGround[groundCount].extrapointsID = s32value;
                        }
                        else
                        {
                            gu32MapErrCount++;
                            pf_map_log("\n MapErr_%d:   get <ground_area extra_points>  fail!  s16ret = %d \n%s \n\n",
                                            gu32MapErrCount,s16ret,lineBuf);
                        }
                    }
                    else if(NULL != strstr(lineBuf,"</ground_area>"))
                    {
                        break;
                    }
                }
                
                groundCount++;
            }
            else
            {
                gu32MapErrCount++;
                pf_map_log("\n MapErr_%d:   get <restriction_area id>  fail!  s16ret = %d \n%s \n\n",
                                gu32MapErrCount,s16ret,lineBuf);
            }    
            
        }

    }
        
        
end:
            
    stIFile.close();

    pf_map_log("\n get_map_info over!   blockCount_%d(%d)    gu32MapErrCount_%d \n\n",blockCount,gMap.block_count_x*gMap.block_count_y,gu32MapErrCount);

    if(gu32MapErrCount == 0)
    {
        set_map_block_area();
        return RET_OK;
    }
    
    return RET_ERR;
}




/**********************************************************************************************
 * @function      check_block_around
 * @brief           Check around the input point(x,y,z)  
 * @input          DOUBLE  (x,y,z)  position  UTM
 * @input          DOUBLE  dis_xy  Horizontal detection distance
 * @input          DOUBLE  dis_z    Vertical detection distance
 * @output        void
 * @return         S32      RET_ERR:    detect fail   
 *                                 RET_OVER   detect nothing
 *                                 RET_OK:      detect block
 *********************************************************************************************/

S32 check_block_around(DOUBLE x,DOUBLE y,DOUBLE z,DOUBLE dis_xy ,DOUBLE dis_z)
{
    DOUBLE zmin = z - dis_z;
    S32 xi_min = S32((x-dis_xy-gMap.block_min.x)/gMap.block_size.x);
    S32 yi_min = S32((y-dis_xy-gMap.block_min.y)/gMap.block_size.y);
    S32 xi_max = S32((x+dis_xy-gMap.block_min.x)/gMap.block_size.x);
    S32 yi_max = S32((y+dis_xy-gMap.block_min.y)/gMap.block_size.y);
    
    if((xi_min >= gMap.block_count_x)||(xi_max < 0)||
        (yi_min >= gMap.block_count_y)||(yi_max < 0))
    {
        pf_map_log("\n check_block_around fail!   pos(%lf,%lf,%lf)  disxy_%lf  disz_%lf \n\n",x,y,z,dis_xy,dis_z);
        return RET_ERR;
    }
    
    PF_MAP_BLOCK_ST (*pblock)[gMap.block_count_x] = (PF_MAP_BLOCK_ST(*)[gMap.block_count_x])pstBlock;

    if(xi_min<0)  
    {
        xi_min = 0;
    }
    if(yi_min<0)  
    {
        yi_min = 0;
    }
    if(xi_max > gMap.block_count_x-1)  
    {
        xi_max = gMap.block_count_x-1;
    }
    if(yi_max > gMap.block_count_y-1)  
    {
        yi_max = gMap.block_count_y-1;
    }

    #if 0
    S32 i = S32((x-gMap.block_min.x)/gMap.block_size.x);
    S32 j = S32((y-gMap.block_min.y)/gMap.block_size.y);
    
    pf_map_log("\n [%d][%d]  [%d][%d]  [%d][%d] ",yi_min,xi_min,yi_max,xi_max,j,i);
    for(S32 yi=yi_min;yi<=yi_max;yi++)
    {
        DOUBLE dby = gMap.block_min.y + gMap.block_size.y*yi;
        for(S32 xi=xi_min;xi<=xi_max;xi++)
        {
            DOUBLE dbx = gMap.block_min.x + gMap.block_size.x*xi;
            pf_map_log("\n %lf  %lf ",dbx,dby);
        }
    }
    #endif

    for(S32 yi=yi_min;yi<=yi_max;yi++)
    {
        for(S32 xi=xi_min;xi<=xi_max;xi++)
        {
            DOUBLE blockz = 999999999.9;

            if(pblock[yi][xi].z_i>0)
            {
                blockz = (DOUBLE(pblock[yi][xi].zmax_mm))/1000;
            }
            
            if(pblock[yi][xi].resdiction_i > 0)
            {
                S32 k = pblock[yi][xi].resdiction_i-1;
                if(pstRestriction[k].height_limit == RESTRICT_NO_FLY)
                {
                    blockz = 999999999.9;
                }
                else if(pstRestriction[k].height_limit > z)
                {
                    blockz = pstRestriction[k].height_limit;
                }
            }

            if(blockz > zmin)
            {
                pf_map_log("\n check_block_around( %lf  %lf  %lf  %lf  %lf ), block[%d][%d] , resdiction_%d , zmax_%lf\n\n",
                    x,y,z,dis_xy,dis_z,yi,xi,pblock[yi][xi].resdiction_i-1,(DOUBLE(pblock[yi][xi].zmax_mm))/1000);
                return RET_OK;
            }
        }
    }
    
    return RET_OVER;
}




S32 check_block_around_lonlat(DOUBLE lat,DOUBLE lon,DOUBLE z,DOUBLE dis_xy ,DOUBLE dis_z)
{
    int zone = 51;
    DOUBLE utm_east = 0.0;
    DOUBLE utm_north = 0.0;

    WGS842UTM(lat, lon, utm_east, utm_north, zone);

    S32 result = check_block_around(utm_east, utm_north, z, dis_xy, dis_z);

    return result;
}

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

S32 check_block_around_line(DOUBLE x, DOUBLE y, DOUBLE z, DOUBLE angle, DOUBLE length , DOUBLE dis_xy, DOUBLE dis_z, DOUBLE* safe_dis)
{

    pf_map_log("\n check_block_around_line( %lf, %lf, %lf, %lf, %lf, %lf, %lf )",x,y,z,angle,length,dis_xy,dis_z);

    DOUBLE curLen = dis_xy;
    DOUBLE theta = angle*PAI/180;
    
    if(length < dis_xy) 
    {
        curLen = length;
    }

    
    while(curLen <= length )
    {
        DOUBLE newx = x + curLen*cos(theta);
        DOUBLE newy = y + curLen*sin(theta);

        pf_map_log("\n  %lf  %lf , len: %lf",newx,newy,curLen);

        S32 ret = check_block_around(newx, newy, z, dis_xy, dis_z);
        
        if(ret != RET_OVER) 
        {
            if (safe_dis!=NULL)
            {
                if(curLen < dis_xy)
                {
                    *safe_dis = 0;
                }
                else
                {
                    *safe_dis = curLen - dis_xy;
                }
            }
            return ret;
        }

        curLen += dis_xy;
    }
    
    if (safe_dis!=NULL)
    {
        *safe_dis = curLen;
    }
    return RET_OVER;
}

S32 check_block_around_line_lonlat(DOUBLE lat, DOUBLE lon, DOUBLE z, DOUBLE angle, DOUBLE length , DOUBLE dis_xy, DOUBLE dis_z, DOUBLE* safe_dis)
{
    int zone = 51;
    DOUBLE utm_east = 0.0;
    DOUBLE utm_north = 0.0;
    DOUBLE ul_safe_dis = 0.0;

    WGS842UTM(lat, lon, utm_east, utm_north, zone);
    S32 result = check_block_around_line(utm_east, utm_north, z, angle, length, dis_xy, dis_z, &ul_safe_dis);

    *safe_dis = ul_safe_dis;
    
    return result;
}


/**********************************************************************************************
 * @function      check_in_resdiction
 * @brief           Check whether the input position is in the resdiction
 * @input          DOUBLE  (x,y,z)  position  UTM
 * @output        void
 * @return         S32      RET_OVER   not in resdiction
 *                                 RET_OK:      in resdiction
 *********************************************************************************************/
S32 check_fly_resdiction(DOUBLE x, DOUBLE y, DOUBLE z)
{
    for(int i=0;i<gMap.resdictionCount;i++)
    {
        if((pstRestriction[i].height_limit == -1)||
            ((pstRestriction[i].height_limit >0)&&(pstRestriction[i].height_limit <= z)))
        {
            S32 ret = check_in_area(x, y,pstRestriction[i]);
            if(ret == 1)
            {
                pf_map_log("\n check_in_resdiction( %lf  %lf  %lf )  is in resdiction[%d]_%d , height_limit_%lf\n\n",
                    x,y,z,i,pstRestriction[i].ID,pstRestriction[i].height_limit);
                return RET_OK;
            }
        }
    }
}







S32 print_block_area(S32 areaid)
{
    PF_MAP_BLOCK_ST (*pblock)[gMap.block_count_x] = (PF_MAP_BLOCK_ST(*)[gMap.block_count_x])pstBlock;
    for(int j=0;j<gMap.block_count_y;j++)
    {
        DOUBLE dby = gMap.block_min.y + gMap.block_size.y*j;
        for(int i=0;i<gMap.block_count_x;i++)
        {
            DOUBLE dbx = gMap.block_min.x + gMap.block_size.x*i;

            if(pblock[j][i].resdiction_i > 0)
            {
                S32 k = pblock[j][i].resdiction_i-1;
                if(pstRestriction[k].ID == areaid)
                {
                    pf_map_log("\n %lf  %lf , block[%d][%d] , resdiction[%d]_%d ",
                        dbx,dby,j,i,k,pstRestriction[k].ID);
                }
            }
        }
    }
}

/**********************************************************************************************
 * @function      get_block_min_z
 * @brief           
 * @input          
 * @output        void
 * @return         DOUBLE      block_min_z
 *                                        
 *********************************************************************************************/
DOUBLE get_block_min_z()
{
    return gMap.block_min.z;
}






/**********************************************************************************************
 * @function      get_block_z
 * @input          DOUBLE  (x,y)  horizontal position  UTM
 * @input          
 * @output        DOUBLE      block_z
 * @return         S32            RET_ERR:    fail   
 *                                        RET_OK:      ok
 *********************************************************************************************/
DOUBLE get_block_z(DOUBLE x, DOUBLE y, DOUBLE* block_z)
{
    if(block_z==NULL)
    {
        pf_map_log("\n get_block_z fail!  block_z is NULL  \n\n");
        return RET_ERR;
    }


    S32 xi = S32((x-gMap.block_min.x)/gMap.block_size.x);
    S32 yi = S32((y-gMap.block_min.y)/gMap.block_size.y);

    if((xi<0)||(yi<0)||(xi>=gMap.block_count_x)||(yi>=gMap.block_count_y))
    {
        pf_map_log("\n get_block_z fail!   pos(%lf,%lf) , block[%d][%d] ,  blockMax[%d][%d]  \n\n",x,y,yi,xi,gMap.block_count_y,gMap.block_count_x);
        return RET_ERR;
    }

    PF_MAP_BLOCK_ST (*pblock)[gMap.block_count_x] = (PF_MAP_BLOCK_ST(*)[gMap.block_count_x])pstBlock;

    *block_z = (DOUBLE(pblock[yi][xi].zmax_mm))/1000;
    pf_map_log("\n get_block_z(%lf,%lf) , block[%d][%d] ,  block_z: %lf  \n\n",x,y,yi,xi,*block_z);
    return RET_OK;
}

DOUBLE get_block_min_z_lonlat(DOUBLE lat, DOUBLE lon, DOUBLE* block_z)
{
    int zone = 51;
    DOUBLE utm_east = 0.0;
    DOUBLE utm_north = 0.0;
    DOUBLE ul_block_z = 0.0;

    WGS842UTM(lat, lon, utm_east, utm_north, zone);
    printf("lat: %.7f lon: %.7f utmz-east: %.7f utm_north: %.7f\n", lat, lon, utm_east, utm_north);
    DOUBLE result = get_block_z(utm_east, utm_north, &ul_block_z);
    *block_z = ul_block_z;
    
    return result;
}


BOOL readPolygonFile(CHAR* filename) 
{
	std::ifstream stIFile(filename);
    
    if(!stIFile.good())
    {
        printf("\n open file:  '%s'  fail \n",filename);
        return false;
    }
    else
    {
        CHAR   tmpStrBuf[1024] ;
        DOUBLE dbUtmX = 0;     
        DOUBLE dbUtmY = 0;  
        S32    nodeCount = 0;
        
        gPolygonNodeCount = 0;
        
        while( !stIFile.eof())
        {
            stIFile.getline(tmpStrBuf, 1024);
            
            S16 s16ret = sscanf(tmpStrBuf,"%lf,%lf",&dbUtmX,&dbUtmY);
            
            if(s16ret == 2)
            {
                if(nodeCount<POLYGON_NODE_COUNT_MAX)
                {
                    gPolygon[nodeCount].x = dbUtmX;
                    gPolygon[nodeCount].y = dbUtmY;
                }
                nodeCount++;
            }
        }
        
        stIFile.close();

        if(nodeCount <= POLYGON_NODE_COUNT_MAX)
        {
            gPolygonNodeCount = nodeCount;
            printf("\n read file  '%s'  over, nodeCount_%d \n",filename,nodeCount);
            return true;
        }
        else
        {
            printf("\n ERR: read file  '%s' over, nodeCount_%d > MAX_%d \n",filename,nodeCount,POLYGON_NODE_COUNT_MAX);
            return false;
        }
    }
}

BOOL readPolygonFile2(const CHAR* filename, PF_NODE_ST* golygon, S32* polygonNodeCount) 
{
	std::ifstream stIFile(filename);
    
    if(!stIFile.good())
    {
        printf("\n open file:  '%s'  fail \n",filename);
        return false;
    }
    else
    {
        CHAR   tmpStrBuf[1024] ;
        DOUBLE dbUtmX = 0;     
        DOUBLE dbUtmY = 0;  
        S32    nodeCount = 0;
        
        *polygonNodeCount = 0;
        
        while( !stIFile.eof())
        {
            stIFile.getline(tmpStrBuf, 1024);
            
            S16 s16ret = sscanf(tmpStrBuf,"%lf,%lf",&dbUtmX,&dbUtmY);
            
            if(s16ret == 2)
            {
                if(nodeCount<POLYGON_NODE_COUNT_MAX)
                {
                    golygon[nodeCount].x = dbUtmX;
                    golygon[nodeCount].y = dbUtmY;
                }
                nodeCount++;
            }
        }
        
        stIFile.close();

        if(nodeCount <= POLYGON_NODE_COUNT_MAX)
        {
            *polygonNodeCount = nodeCount;
            printf("\n read file  '%s'  over, nodeCount_%d \n",filename,nodeCount);
            return true;
        }
        else
        {
            printf("\n ERR: read file  '%s' over, nodeCount_%d > MAX_%d \n",filename,nodeCount,POLYGON_NODE_COUNT_MAX);
            return false;
        }
    }
}

void BxtTest_LoadPolygonFiles(PF_POLYGON_DATA* polygons, int polygon_count)
{
    for (size_t i = 0; i <  polygon_count; ++i) {
        PF_POLYGON_DATA* poly = &polygons[i];
        readPolygonFile2(poly->filename, poly->polygon, poly->nodeCount);
    }

}

bool BxtTest_CheckPointInPolygons(PF_POLYGON_DATA* polygons, int polygon_count, PF_NODE_ST point)
{
    printf("\n polygon count: %d \n", polygon_count);
    for (size_t i = 0; i < polygon_count; ++i) {
        PF_POLYGON_DATA* poly = &polygons[i];
        if (isInPolygon(point, poly->polygon, *poly->nodeCount)) {
            printf("\n ( %lf,%lf ) isInPolygon == 1 在%s限制范围内\n", point.x, point.y, poly->filename);
            return true;
        }
    }
    return false;
}


/**
 * 计算水平向右的射线与线段是否相交
 * @param {*} point 射线起点
 * @param {*} startPoint 线段起点
 * @param {*} endPoint 线段终点
 * @returns 
 */
BOOL isRayCrossSeg(PF_NODE_ST point, PF_NODE_ST startPoint, PF_NODE_ST endPoint) 
{
    // 排除水平、重合、线段为点
    if (startPoint.y == endPoint.y) {
        return false;
    }
    // 排除线段在射线上方
    if (startPoint.y > point.y && endPoint.y > point.y) {
        return false;
    }
    // 排除线段在射线下方
    if (startPoint.y < point.y && endPoint.y < point.y) {
        return false;
    }
    // 排除线段在射线上方时，一个端点在射线上
    if (startPoint.y == point.y && endPoint.y > point.y) {
        return false;
    }
    if (endPoint.y == point.y && startPoint.y > point.y) {
        return false;
    }
    // 排除线段在射线左侧
    if (startPoint.x < point.x && endPoint.x < point.x) {
        return false;
    }
    
    DOUBLE xseg = endPoint.x - (endPoint.x - startPoint.x) * (endPoint.y - point.y) / (endPoint.y - startPoint.y);
    // 排除交点在射线左侧
    if (xseg < point.x) {
        return false;
    }
    return true;
}



/**
 * 射线法判断点是否在多边形内
 * @param {*} point 目标点
 * @param {*} polygon 多边形点集
 * @returns 
 */
BOOL isInPolygon(PF_NODE_ST point, PF_NODE_ST* polygon, S32 nodeCount ) 
{
    S32 crossPointNum = 0;
    S32 j = nodeCount - 1;
    for (S32 k = 0; k < nodeCount; k++) 
    {
        PF_NODE_ST endPoint = polygon[k];
        PF_NODE_ST startPoint = polygon[j];
        if (isRayCrossSeg(point, startPoint, endPoint)) 
        {
            crossPointNum ++;
        }
        j = k;
    }
    return crossPointNum % 2 == 1;
}

#undef PF_MAP_BLOCK_3D_CPP


