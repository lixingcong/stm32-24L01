/********************************************************************************
*
*  File Name     : app_cfg.h
*  Author   	 : 王保礼
*  Create Date   : 2016.06.01
*  Version   	 : 1.0
*  Description   : 应用程序系统配置：宏定义开关等
*  History       : 1. Data:
*                     Author:
*                     Modification:
*
********************************************************************************/

#ifndef  __APP_CFG_H__
#define  __APP_CFG_H__



#define DEBUG 1         //当不需要调试信息时，设置为0

#if DEBUG
#define Log(a) printf(a)
#else
#define Log(a)
#endif



#endif

