#ifndef _PLTYPE_H
#define _PLTYPE_H

#if 0
typedef int                     STATUS;    //this is defined native in vxworks
typedef unsigned char           BYTE;
typedef unsigned short          WORD;
typedef unsigned long           DWORD;
#endif

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



#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#ifndef MAX_UINT16_VALUE
#define MAX_UINT16_VALUE  0xFFFF
#endif

#ifndef MAX_UINT32_VALUE
#define MAX_UINT32_VALUE  0xFFFFFFFF
#endif

#ifndef MAX_UINT64_VALUE
#define MAX_UINT64_VALUE  0xFFFFFFFFFFFFFFFF
#endif

/*UCHAR-->U32*/
#define STATICINLINE  inline
#define EXTERN extern


#ifndef __BYTE_CONVERT_
#define __BYTE_CONVERT_

/*Conve*/
#define UINT8O_UINT32(s1, s2, s3, s4) (((uint32_t)(s1)<<24) + ((uint32_t)(s2)<<16) + ((uint32_t)(s3)<<8) + (s4))
/*U32-->UCHAR*/
#define UINT32_TO_UINT8_HH(u)    ((U8)((u) >>24))
#define UINT32_TO_UINT8_MH(u)    ((U8)((u) >>16))
#define UINT32_TO_UINT8_ML(u)    ((U8)((u) >>8))
#define UINT32_TO_UINT8_LL(u)    ((U8)(u))

#define UINT8O_UINT32_MEM(p) UINT8O_UINT32(((p)[0]), ((p)[1]), ((p)[2]), ((p)[3]))

#define GET_MEM_24(p) UINT8O_UINT32((0), ((p)[0]), ((p)[1]), ((p)[2]))

#define UINT32_TO_UINT8_MEM(p, u) \
    do{\
          (p)[0] = (u) >> 24;\
          (p)[1] = (u) >> 16;\
          (p)[2] = (u) >> 8;\
          (p)[3] = (u);\
      }while(0)

/*UCHAR-->USHORT*/
#define UINT8O_UINT16(s1, s2) (((uint16_t)(s1)<<8) + (s2))
/*USHORT-->UCHAR*/
#define UINT16_TO_UINT8_H(us) ((U8)((us) >>8))
#define UINT16_TO_UINT8_L(us) ((U8)((us) & 0xFF))

#endif /*__BYTE_CONVERT_*/



#ifndef ALIGN_4BYTE_ADDR
#define ALIGN_4BYTE_ADDR(addr) (((addr + 3)>>2)<<2)
#endif

#ifndef ALIGN_2BYTE_ADDR
#define ALIGN_2BYTE_ADDR(addr) (((addr + 1)>>1)<<1)
#endif



#endif
