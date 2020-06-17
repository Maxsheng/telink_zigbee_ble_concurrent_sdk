/********************************************************************************************************
 * @file     zb_appCb.c
 *
 * @brief    call back function for zigbee
 *
 * @author
 * @date     Dec. 1, 2016
 *
 * @par      Copyright (c) 2016, Telink Semiconductor (Shanghai) Co., Ltd.
 *           All rights reserved.
 *
 *			 The information contained herein is confidential and proprietary property of Telink
 * 		     Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *			 of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *			 Co., Ltd. and the licensee in separate contract or the terms described here-in.
 *           This heading MUST NOT be removed from this file.
 *
 * 			 Licensees are granted free, non-transferable use of the information in this
 *			 file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided.
 *
 *******************************************************************************************************/

#if (__PROJECT_TL_CONCURRENT_SWITCH__)

/**********************************************************************
 * INCLUDES
 */
#include "tl_common.h"
#include "zb_api.h"
#include "zcl_include.h"
#include "bdb.h"
#include "ota.h"
#include "sampleSwitch.h"
#include "app_ui.h"

/**********************************************************************
 * LOCAL CONSTANTS
 */


/**********************************************************************
 * TYPEDEFS
 */


/**********************************************************************
 * LOCAL FUNCTIONS
 */
void zbdemo_bdbInitCb(u8 status, u8 joinedNetwork);
void zbdemo_bdbCommissioningCb(u8 status, void *arg);
void zbdemo_bdbIdentifyCb(u8 endpoint, u16 srcAddr, u16 identifyTime);
void zbdemo_bdbFindBindSuccessCb(findBindDst_t *pDstInfo);

#if PM_ENABLE
extern void app_zigbeePollRateRecovery(void);
#endif
/**********************************************************************
 * LOCAL VARIABLES
 */
bdb_appCb_t g_zbDemoBdbCb =
{
	zbdemo_bdbInitCb,
	zbdemo_bdbCommissioningCb,
	zbdemo_bdbIdentifyCb,
	zbdemo_bdbFindBindSuccessCb
};

#ifdef ZCL_OTA
ota_callBack_t sampleSwitch_otaCb =
{
	sampleSwitch_otaProcessMsgHandler,
};
#endif


volatile u8 T_zbdemoBdbInfo[8] = {0};




/**********************************************************************
 * FUNCTIONS
 */
void sampleSwitch_bdbRejoinStart(void *arg){
	zb_rejoin_mode_set(REJOIN_INSECURITY);
	bdb_init((af_simple_descriptor_t *)&sampleSwitch_simpleDesc, &g_bdbCommissionSetting, &g_zbDemoBdbCb,1);
}

s32 sampleSwitch_bdbNetworkSteerStart(void *arg){
	bdb_networkSteerStart();

	return -1;
}

#if FIND_AND_BIND_SUPPORT
s32 sampleSwitch_bdbFindAndBindStart(void *arg){
	BDB_ATTR_GROUP_ID_SET(0x1234);//only for initiator
	bdb_findAndBindStart(BDB_COMMISSIONING_ROLE_INITIATOR);

	g_switchAppCtx.bdbFBTimerEvt = NULL;
	return -1;
}
#endif

/*********************************************************************
  * @fn      zbdemo_bdbInitCb
  *
  * @brief   application callback for bdb initiation
  *
  * @param   status - the status of bdb init BDB_INIT_STATUS_SUCCESS or BDB_INIT_STATUS_FAILURE
  *
  * @param   joinedNetwork  - 1: node is on a network, 0: node isn't on a network
  *
  * @return  None
  */
void zbdemo_bdbInitCb(u8 status, u8 joinedNetwork){
	if(status == BDB_INIT_STATUS_SUCCESS){
		T_zbdemoBdbInfo[0]++;

		/*
		 * for non-factory-new device:
		 * 		load zcl data from NV, start poll rate, start ota query, bdb_networkSteerStart
		 *
		 * for factory-new device:
		 * 		if ZBHCI_EN == 1, do noting until receiving a steer command
		 * 		if ZBHCI_EN == 0, steer a network or initiate touch-link
		 *
		 * */
		if(joinedNetwork){
			app_zigbeePollRateRecovery();

#ifdef ZCL_OTA
			ota_queryStart(OTA_CHECK_PERIOD_MIN);
#endif

#ifdef ZCL_POLL_CTRL
			sampleSwitch_zclCheckInStart();
#endif
		}else{
			g_switchAppCtx.steerTriesNum = SAMPLE_SWITCH_STEER_TRIESNUM;
			u16 jitter = zb_random();
			jitter &= 0xfff;
			TL_ZB_TIMER_SCHEDULE(sampleSwitch_bdbNetworkSteerStart, NULL, jitter * 1000);
		}
	}else{
		T_zbdemoBdbInfo[1]++;
		if(joinedNetwork){
			T_zbdemoBdbInfo[2]++;
			//TL_SCHEDULE_TASK(sampleSwitch_bdbRejoinStart, NULL);
		}
	}
}

/*********************************************************************
  * @fn      zbdemo_bdbCommissioningCb
  *
  * @brief   application callback for bdb commissioning
  *
  * @param   status - the status of bdb commissioning
  *
  * @param   arg
  *
  * @return  None
  */
void zbdemo_bdbCommissioningCb(u8 status, void *arg){
	T_zbdemoBdbInfo[3]++;
	T_zbdemoBdbInfo[4] = status;

	g_switchAppCtx.state = APP_STATE_NORMAL;

	if(status == BDB_COMMISSION_STA_SUCCESS){
		T_zbdemoBdbInfo[5]++;
		app_zigbeePollRateRecovery();


#ifdef ZCL_POLL_CTRL
		sampleSwitch_zclCheckInStart();
#endif

		light_blink_start(2, 200, 200);

#ifdef ZCL_OTA
        ota_queryStart(OTA_CHECK_PERIOD_MIN);
#endif

#if FIND_AND_BIND_SUPPORT
        //start Finding & Binding
		if(!g_switchAppCtx.bdbFBTimerEvt){
			g_switchAppCtx.bdbFBTimerEvt = TL_ZB_TIMER_SCHEDULE(sampleSwitch_bdbFindAndBindStart, NULL, 50 * 1000);
		}
#endif
	}else if(status == BDB_COMMISSION_STA_IN_PROGRESS){

	}else if(status == BDB_COMMISSION_STA_NOT_AA_CAPABLE){

	}else if((status == BDB_COMMISSION_STA_NO_NETWORK)||(status == BDB_COMMISSION_STA_TCLK_EX_FAILURE)){
		if(g_switchAppCtx.steerTriesNum-- <= 0){
			return;
		}
		if(g_switchAppCtx.installCodeAvailable){
			/* Switch the two kinds of link keys to attempt join network. */
			if(g_switchAppCtx.useInstallCodeFlg){
				g_bdbCommissionSetting.linkKey.tcLinkKey.keyType = g_switchAppCtx.linkKey.tcLinkKey.keyType;
				g_bdbCommissionSetting.linkKey.tcLinkKey.key = g_switchAppCtx.linkKey.tcLinkKey.key;
			}else{
				g_bdbCommissionSetting.linkKey.tcLinkKey.keyType = SS_GLOBAL_LINK_KEY;
				g_bdbCommissionSetting.linkKey.tcLinkKey.key = (u8 *)tcLinkKeyCentralDefault;
			}
			extern void bdb_linkKeyCfg( bdb_commissionSetting_t *setting, u8 fn);
			bdb_linkKeyCfg(&g_bdbCommissionSetting, TRUE);
			g_switchAppCtx.useInstallCodeFlg = !g_switchAppCtx.useInstallCodeFlg;
		}
		TL_ZB_TIMER_SCHEDULE(sampleSwitch_bdbNetworkSteerStart, NULL, 100 * 1000);
	}else if(status == BDB_COMMISSION_STA_TARGET_FAILURE){

	}else if(status == BDB_COMMISSION_STA_FORMATION_FAILURE){

	}else if(status == BDB_COMMISSION_STA_NO_IDENTIFY_QUERY_RESPONSE){

	}else if(status == BDB_COMMISSION_STA_BINDING_TABLE_FULL){

	}else if(status == BDB_COMMISSION_STA_NO_SCAN_RESPONSE){

	}else if(status == BDB_COMMISSION_STA_NOT_PERMITTED){

	}else if(status == BDB_COMMISSION_STA_TCLK_EX_FAILURE){

	}else if(status == BDB_COMMISSION_STA_PARENT_LOST){
		T_zbdemoBdbInfo[7]++;
	}
}


extern void sampleSwitch_zclIdentifyCmdHandler(u8 endpoint, u16 srcAddr, u16 identifyTime);
void zbdemo_bdbIdentifyCb(u8 endpoint, u16 srcAddr, u16 identifyTime){
#if FIND_AND_BIND_SUPPORT
	sampleSwitch_zclIdentifyCmdHandler(endpoint, srcAddr, identifyTime);
#endif
}

/*********************************************************************
  * @fn      zbdemo_bdbFindBindSuccessCb
  *
  * @brief   application callback for finding & binding
  *
  * @param   pDstInfo
  *
  * @return  None
  */
void zbdemo_bdbFindBindSuccessCb(findBindDst_t *pDstInfo){
#if FIND_AND_BIND_SUPPORT
	epInfo_t dstEpInfo;
	TL_SETSTRUCTCONTENT(dstEpInfo, 0);

	dstEpInfo.dstAddrMode = APS_SHORT_DSTADDR_WITHEP;
	dstEpInfo.dstAddr.shortAddr = pDstInfo->addr;
	dstEpInfo.dstEp = pDstInfo->endpoint;
	dstEpInfo.profileId = HA_PROFILE_ID;

	zcl_identify_sendIdentifyCmd(SAMPLE_SWITCH_ENDPOINT, &dstEpInfo, FALSE, 0, 0);
#endif
}



#ifdef ZCL_OTA
void sampleSwitch_otaProcessMsgHandler(u8 evt, u8 status)
{
	//printf("sampleSwitch_otaProcessMsgHandler: status = %x\n", status);
	if(evt == OTA_EVT_START){
		if(status == ZCL_STA_SUCCESS){
			zb_setPollRate(QUEUE_POLL_RATE);
		}else{

		}
	}else if(evt == OTA_EVT_COMPLETE){
		zb_setPollRate(POLL_RATE * 3);

		if(status == ZCL_STA_SUCCESS){
			ota_mcuReboot();
		}else{
			ota_queryStart(OTA_CHECK_PERIOD_MIN);
		}
	}
}
#endif

/*********************************************************************
  * @fn      sampleSwitch_leaveCnfHandler
  *
  * @brief   Handler for ZDO Leave Confirm message.
  *
  * @param   pRsp - parameter of leave confirm
  *
  * @return  None
  */
void sampleSwitch_leaveCnfHandler(void *p)
{
	nlmeLeaveConf_t *pCnf = (nlmeLeaveConf_t *)p;
	//printf("sampleSwitch_leaveCnfHandler, status = %x\n", pCnf->status);
    if(pCnf->status == SUCCESS ){
    	//SYSTEM_RESET();
    }
}

/*********************************************************************
  * @fn      sampleSwitch_leaveIndHandler
  *
  * @brief   Handler for ZDO leave indication message.
  *
  * @param   pInd - parameter of leave indication
  *
  * @return  None
  */

void sampleSwitch_leaveIndHandler(void *p)
{
	//nlmeLeaveInd_t *pInd = (nlmeLeaveInd_t *)p;
    //printf("sampleSwitch_leaveIndHandler, rejoin = %d\n", pInd->rejoin);
    //printfArray(pInd->device_address, 8);
}


#endif  /* __PROJECT_TL_SWITCH_8258__ */
