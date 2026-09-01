/*******************************************************************************************************************
 **                                                                                                                        
 **  Copyright (c)  2009,  Rayfond, Inc.                                                                                 
 **        All    Rights Reserved.                                                                                            
 **                                                                                                                          
 **  Subsystem    : LTE/UE                                                                                             
 **  File        : main.c                                                                                       
 **  Created By    : jzhou                                                                                                    
 **  Created On    : 09/12/24                                                                                                 
 **                                                                                                                         
 **  Purpose:                                                                                                             
 **    This file    contains the platform api and main entry
 **                                                                                                                         
 **  History:                                                                                                             
 **  Programmer        Date    Rev    Description                                                                                 
 **  --------------- ---------- --------    ------------------------------                                                   
 **
 ******************************************************************************************************************/
#ifndef _PF_UUID_H
#define _PF_UUID_H
#include <dlfcn.h>

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************************************
 * @API function  pf_cipher_encrypt_file
 * @brief         encrypt the source file
 * @input         pscSrcFile  The path of source file
                  pscDstFile  The path of destination file
 * @output        pucRst      The encrypt result of the file
 * @return        true:       succuss
                  false:      failure
 *********************************************************************************************/
BOOL pf_cipher_encrypt_file(CHAR* pscSrcFile, CHAR* pscDstFile);

/**********************************************************************************************
 * @API function  pf_cipher_decrypt_file
 * @brief         decrypt the source file
 * @input         pscSrcFile  The path of source file
                  pscDstFile  The path of destination file
 * @output        pucRst      The decrypt result of the file
 * @return        true:       succuss
                  false:      failure
 *********************************************************************************************/
BOOL pf_cipher_decrypt_file(CHAR* pscSrcFile, CHAR* pscDstFile);

/**********************************************************************************************
 * @API function  pf_cipher_enc_md5_file
 * @brief         using md5 and length to pack the source file
 * @input         pscSrcFile  The path of source file
                  pscDstFile  The path of destination file
 * @output        pucRst      The pack result of the file
 * @return        true:       succuss
                  false:      failure
 *********************************************************************************************/
BOOL pf_cipher_enc_md5_file(CHAR* pscSrcFile, CHAR* pscDstFile);

/**********************************************************************************************
 * @API function  pf_cipher_enc_md5_file
 * @brief         using md5 and length to unpack the source file
 * @input         pscSrcFile  The path of source file
                  pscDstFile  The path of destination file
 * @output        pucRst      The unpack result of the file
 * @return        true:       succuss
                  false:      failure
 *********************************************************************************************/
BOOL pf_cipher_dec_md5_file(CHAR* pscSrcFile, CHAR* pscDstFile);

/**********************************************************************************************
 * @API function  pf_cipher_encrypt_mul_file
 * @brief         Multiple files are encrypted into one file
 * @input         ppscArgvFileName  The path of source file
                  ulArgcFileNum     The numbers of source file
 * @output        pscDstFile        The destnation path of file
 * @return        true:             succuss
                  false:            failure
 *********************************************************************************************/
BOOL pf_cipher_encrypt_mul_file(CHAR** ppscArgvFileName, int ulArgcFileNum, CHAR* pscDstFile);


/**********************************************************************************************
 * @API function  pf_cipher_decrypt_mul_file
 * @brief         One file are decrypted into multiple files
 * @input         pscPath           The path of source file
 * @output        pscResultPath     The destnation path of the multiple files
 * @return        true:             succuss
                  false:            failure
 *********************************************************************************************/
BOOL pf_cipher_decrypt_mul_file(CHAR* pscPath, CHAR* pscResultPath);

#ifdef __cplusplus
}
#endif

#endif

