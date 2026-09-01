/*******************************************************************************
  Copyright (C), 2012, Innofidei Inc
  File name:    double_list.h

  Author:       Version:        Date: 
  Lou Junqing   1.0             2012-11-15
  
  Description:  This file declares and implements double-linked list.

  Function List:
  DOUBLE_LIST_INIT(macro): initialize a double-linked list.
  DOUBLE_LIST_INSERT_HEAD(macro): insert a node to the head of a double-linked list.
  DOUBLE_LIST_INSERT_TAIL(macro): insert a node to tail of a double-linked list.  
  DOUBLE_LIST_REMOVE(macro): remove a node from a double-linked list.
  DOUBLE_LIST_FIRST(macro): return the first node in a double-linked list.
  DOUBLE_LIST_NEXT(macro): return the next node of current node in a double-linked list.

  History:
  <Author>      <Date>      <Version>   <description>           
  Lou Junqing   2012-11-15  1.0         First implementation of double-linked list

*******************************************************************************/

#ifndef _DOUBLE_LIST_H
#define _DOUBLE_LIST_H

#include <pl_type.h>

typedef struct _DOUBLE_LIST_S 
{
    struct _DOUBLE_LIST_S *pPrev;    /* pointer to previous list node */
    struct _DOUBLE_LIST_S *pNext;    /* pointer to next list node */
    U64 ullData;            /* user maintained data */
} DOUBLE_LIST_S;

/*******************************************************************************
*  Function:    DOUBLE_LIST_INIT
*
*  Description: initialize a double-linked list. 
*
*  Input:       pHead       pointer of list head
*
*  Output:      
*
*  Return:      No Return Value
*
*******************************************************************************/
#define DOUBLE_LIST_INIT(pHead) \
{ \
    (pHead)->pNext = (pHead); \
    (pHead)->pPrev = (pHead); \
}

/*******************************************************************************
*  Function:    DOUBLE_LIST_INSERT_HEAD
*
*  Description: insert a node to head of a double-linked list head
*
*  Input:       pHead       pointer of list head
*               pNode       pointer of the list node to be inserted
*
*  Output:      
*
*  Return:      No Return Value
*
*******************************************************************************/
#define DOUBLE_LIST_INSERT_HEAD(pHead, pNode) \
{ \
    (pNode)->pNext = (pHead)->pNext; \
    (pNode)->pNext->pPrev = (pNode); \
    (pNode)->pPrev = (pHead); \
    (pHead)->pNext = (pNode); \
}

/*******************************************************************************
*  Function:    DOUBLE_LIST_INSERT_TAIL
*
*  Description: insert a node to tail of a double-linked list head
*
*  Input:       pHead       pointer of list head
*               pNode       pointer of the list node to be inserted
*
*  Output:      
*
*  Return:      No Return Value
*
*******************************************************************************/
#define DOUBLE_LIST_INSERT_TAIL(pHead, pNode) \
{ \
    (pNode)->pPrev = (pHead)->pPrev; \
    (pNode)->pNext = (pHead); \
    (pNode)->pPrev->pNext = (pNode); \
    (pHead)->pPrev = (pNode); \
}

/*******************************************************************************
*  Function:    DOUBLE_LIST_REMOVE
*
*  Description: remove a node from a double-linked list
*
*  Input:       pNode       pointer of the list node to be removed
*
*  Output:      
*
*  Return:      No Return Value
*
*******************************************************************************/
#define DOUBLE_LIST_REMOVE(pNode) \
{ \
    (pNode)->pNext->pPrev = (pNode)->pPrev; \
    (pNode)->pPrev->pNext = (pNode)->pNext; \
}

/*******************************************************************************
*  Function:    DOUBLE_LIST_FIRST
*
*  Description: get the first node of a double-linked list
*
*  Input:       pNode       pointer of the list node to be removed
*
*  Output:      
*
*  Return:      if list is not empty, return pointer of the first node.
*               if list is emptry, return pointer of the list head.
*
*******************************************************************************/
#define DOUBLE_LIST_FIRST(pHead) ((pHead)->pNext)

/*******************************************************************************
*  Function:    DOUBLE_LIST_NEXT
*
*  Description: get the next node of current node
*
*  Input:       pNode       pointer of current node
*
*  Output:      
*
*  Return:      if current node is not last one, return pointer of next node.
*               if current node is last one, return pointer of the list head.
*
*******************************************************************************/
#define DOUBLE_LIST_NEXT(pNode) ((pNode)->pNext)

#endif /* #ifndef _DOUBLE_LIST_H */