/*******************************************************************************************************************
 **                                                                                                                        
 **  Copyright (c)  2009,  Innofidei, Inc.                                                                                 
 **        All    Rights Reserved.                                                                                            
 **                                                                                                                          
 **  Subsystem     : LTE/SMALLCELL                                                                                             
 **  File          : pf_log_link.h                                                                                      
 **  Created By    : roy                                                                                              
 **  Created On    : 2013/12/10                                                                                                 
 **                                                                                                                         
 **  Purpose:                                                                                                             
 **    This file    contains the platform api and main entry
 **                                                                                                                         
 **  History:                                                                                                             
 **  Programmer        Date    Rev    Description                                                                                 
 **  --------------- ---------- --------    ------------------------------                                                   
 **
 ******************************************************************************************************************/

#include <stdio.h> 
#include<malloc.h>
#include<stdlib.h>
#include "pl_type.h"
typedef struct list
{
    U32 ulWriteOffset; 
    U32 ulReadOffset;
    U16 ulFlag; 
    U8 *ucLogAddr;
    struct list *next;
}List;


typedef struct 
{  
    U32 ulBindingNum;      
    U32 ulTotalBindNum;
    U32 ulThreadId;  
    List *WriteList;        
    List *ReadList;                
}PS_LOGLINK_HEADER;

int pf_create_log_link(U32 ThreadId);