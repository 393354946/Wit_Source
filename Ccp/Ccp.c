/*============================================================================*/
/* Copyright (C) 2015-2016, Chengdu WeiTe Dianpen CO.,LTD.
 *  
 *  All rights reserved. This software is WIT property. Duplication 
 *  or disclosure without WIT written authorization is prohibited.
 *  
 *  
 *  @file            :<Ccp.c>
 *  @brief           :<Ccp for MPC5634> 
 *  @author          :<Mingdong.Yuan>
 *  @date            :<2016-4-22>
 *  @Current revision: $Revision: 1.0
 */
/*============================================================================*/

/*************************************************************************
Packages inclusion
*************************************************************************/
#include "Ccp.h"
#include "CanIf.h"
#include <string.h>

/*************************************************************************
EXPORTED variables declaration
*************************************************************************/
EXPORTED CCP_ST_PARAMETER_T       CCP_stParameter;
EXPORTED CAL_ST_PARAMETER_T       CAL_stParameter;
EXPORTED CCP_ST_BUFFER_T   Ccp_RxBuffer;
EXPORTED CCP_ST_BUFFER_T   Ccp_TxBuffer;
EXPORTED uint8             CCP_u8ResourceMask;
EXPORTED uint32            CCP_u32CcpSeed;
EXPORTED uint8             CCP_au8TestArea[5];

/*for debug*/
EXPORTED uint8             CCP_au8Data[9];

EXPORTED	uint8  CCP_u8DaqListNum;
EXPORTED	uint8  CCP_u8OdtNum;
EXPORTED	uint8  CCP_u8ElementNum; 

/*Timing counter used by DAQ*/
EXPORTED uint8  DAQ_u8TimingCounter;

/*Timing flag fo DAQ event*/
EXPORTED boolean DAQ_bTimer10msFlag;
EXPORTED boolean DAQ_bTimer100msFlag;

/**/
uint8 CCP_au8SlaveDeviceID[] = CCP_SLAVE_DEVICE_ID;
/*************************** COMMAND PROCESSOR ****************************/

/*************************************************************************
LOCAL constant data declaration
*************************************************************************/
/*Structure holding a map between command codes and the function implementing the command*/
LOCAL Ccp_CmdListType Ccp_CmdList[CCP_CMD_NUMBER] = 
{
	{                   CCP_CMD_CONNECT,    /* CCP command Connect:0x01    */
		                 M_u8CmdConnect},    /* Connect function            */               
	{  
	                    CCP_CMD_SET_MTA,    /* CCP command SetMTA:0x02     */
		                  M_u8CmdSetMTA},    /* SetMTA function             */
	{  
	                   CCP_CMD_DOWNLOAD,    /* CCP command Download:0x03   */
		                M_u8CmdDownload},    /* Download function           */
	{  
	                     CCP_CMD_UPLOAD,    /* CCP command Upload:0x04     */
		                  M_u8CmdUpload},    /* Upload function             */
	{  
	                 CCP_CMD_START_STOP,    /* CCP command StartStop:0x06  */
		               M_u8CmdStartStop},    /* StartStop function          */
	{  
	                  CCP_CMD_DISCONNECT,   /* CCP command Disconnect:0x07 */
		               M_u8CmdDisconnect},   /* Disconnect function         */	    
	{  
	         CCP_CMD_GET_ACTIVE_CAL_PAGE,   /* CCP command Connect:0x09  */
		         M_u8CmdGetActiveCalPage},   /* GetActiveCalPage function */
	{  
	                CCP_CMD_SET_S_STATUS,   /* CCP command Connect:0x0C  */
		               M_u8CmdSetSStatus},   /* SetSStatus function       */
	{  
	                CCP_CMD_GET_S_STATUS,   /* CCP command Connect:0x0D  */
		               M_u8CmdGetSStatus},   /* GetSStatus function       */

	{	             CCP_CMD_BUILD_CHKSUM, /* CCP command Connect:0x0E  */
	               M_u8CmdBuildChecksum},/* Build Checksum function   */
	{  
	             CCP_CMD_SELECT_CAL_PAGE,   /* CCP command Connect:0x11  */
		            M_u8CmdSelectCalPage},   /* SelectCalPage function    */
	{  
	                    CCP_CMD_GET_SEED,   /* CCP command Connect:0x12  */
		                  M_u8CmdGetSeed},   /* GetSeed function          */
	{  
	                      CCP_CMD_UNLOCK,   /* CCP command Connect:0x13  */
		                   M_u8CmdUnlock},   /* Unlock function           */
	{  
	                CCP_CMD_GET_DAQ_SIZE,   /* CCP command Connect:0x14  */
		               M_u8CmdGetDaqSize},   /* GetDaqSize function       */
	{  
	                 CCP_CMD_SET_DAQ_PTR,   /* CCP command Connect:0x15  */
		                M_u8CmdSetDaqPtr},   /* SetDaqPtr function        */
	{  
	                   CCP_CMD_WRITE_DAQ,   /* CCP command Connect:0x16  */
		                 M_u8CmdWriteDaq},   /* WriteDaq function         */
	{  
	                 CCP_CMD_EXCHANGE_ID,   /* CCP command Connect:0x17  */
		               M_u8CmdExchangeID},   /* ExchangeID function       */
	{  
	             CCP_CMD_GET_CCP_VERSION,   /* CCP command Connect:0x1B  */
		            M_u8CmdGetCcpVersion},   /* GetCcpVersion function    */
	{  
	                     CCP_CMD_DNLOAD6,   /* CCP command Connect:0x23  */
		                  M_u8CmdDnload6},   /* Dnload6 function          */
		               
};
/**************************************************************************
Private Functions
**************************************************************************/

/**************************************************************************
Object:Ccp_Recieve_Main() is the main process that executes all received commands.
		 The function queues up replies for transmission. Which will be sent
		 when Ccp_Transmit_Main function is called.
Parameters:none
Return:none
**************************************************************************/
LOCAL void Ccp_Recieve_Main(void)
{
	uint8 u8Pid;
	uint8 u8ListIndex;
	Ccp_CmdListType *pCommand;

	u8Pid = 0x00;
	u8Pid = Ccp_RxBuffer.au8Data[0];
	for(u8ListIndex = 0;u8ListIndex < CCP_CMD_NUMBER;u8ListIndex ++)
	{
		/* process standard commands */
      pCommand = &Ccp_CmdList[u8ListIndex]; 
		if(u8Pid == pCommand->Ccp_Cmd)
		{
			pCommand->pfCcpCmdFunc(u8Pid, Ccp_RxBuffer.au8Data, Ccp_RxBuffer.u8Length);
			M_CcpClearRxBuffer();
			return; 
		}
	}
	
	/*if(pCommand->pfCcpCmdFunc)
	{
		pCommand->pfCcpCmdFunc(u8Pid, Ccp_RxBuffer->au8Data + 1, Ccp_RxBuffer->u8Length - 1);
	}*/
}

/**************************************************************************
Object:Ccp_Transmit_Main() transmits queued up replies
Parameters:none
Return:none
**************************************************************************/
LOCAL void Ccp_Transmit_Main(void)
{
	if(CCP_stParameter.SendStatus & CCP_CRM_REQUEST != KUC_NULL)
	{
		CCP_stParameter.SendStatus &= (~CCP_CRM_REQUEST);
		Ccp_Transmit(Ccp_TxBuffer.au8Data, Ccp_TxBuffer.u8Length);
	}
}

/**************************************************************************
Object:Ccp_Transmit()Transport protocol transmit function called by Ccp system
Parameters:data:the data which need to be transmit
           len:the data length which need to be transmit
Return:Std_ReturnType
**************************************************************************/
LOCAL Std_ReturnType Ccp_Transmit(const uint8 *data, int len)
{
    PduInfoType pdu;
	 Std_ReturnType ReValue;
	 uint8 i;
		 
    pdu.sduDataPtr = (uint8*)data;
    pdu.sduLength  = (uint16)len;

	 for(i= 0;i<9;i++)
	 {
	 	CCP_au8Data[i] = pdu.sduDataPtr[i];
	 }
	 CCP_au8Data[8] = pdu.sduLength;
	 
	 ReValue        = CanIf_Transmit(CCP_PDU_ID_TX, &pdu);
    return (ReValue);
}

/**************************************************************************
Object:Receive from CAN interfacelayer,This function is called by the interface
Parameters:u8Data : the data has been received
           u8Len  : data length 
Return:none
**************************************************************************/
LOCAL void Ccp_RxIndication(uint8 *u8Data, uint8 u8Len)
{
   uint8 u8Index;

   Ccp_RxBuffer.u8Length = u8Len;
	for(u8Index = 0;u8Index < u8Len;u8Index++)
	{
		Ccp_RxBuffer.au8Data[u8Index] = u8Data[u8Index];
	}
	
}

/**************************** GENERIC COMMANDS ****************************/

LOCAL Std_ReturnType M_u8CmdConnect(uint8 pid, uint8* data, int len)
{
   uint8  u8CmdCode;
	uint8  u8CmdCTR;
	uint16 u16StationAddr;

	u8CmdCode      = data[COM_U8_POS_DATA0];
	u8CmdCTR       = data[COM_U8_POS_DATA1];
	u16StationAddr = (uint16)data[2] + (uint16)(data[3] << 8);

	/* This station */
   if (u16StationAddr == CCP_STATION_ADDR)
   {
   	if(u8CmdCode == CCP_CMD_CONNECT)
   	{
   		CCP_stParameter.SessionStatus |= SESSION_STATUS_CONNECTED;
         CCP_stParameter.SessionStatus &= ~SESSION_STATUS_TMP_DISCONNECTED;
   	}
		
		/* Responce */
      /* Station addresses in Intel Format */
      Ccp_TxBuffer.au8Data[COM_U8_POS_DATA0] = PACKET_ID;
      Ccp_TxBuffer.au8Data[COM_U8_POS_DATA1] = CRC_OK;
      Ccp_TxBuffer.au8Data[COM_U8_POS_DATA2] = u8CmdCTR;
      Ccp_TxBuffer.au8Data[COM_U8_POS_DATA3] = 0xFF;
		Ccp_TxBuffer.au8Data[COM_U8_POS_DATA4] = 0xFF;
		Ccp_TxBuffer.au8Data[COM_U8_POS_DATA5] = 0xFF;
		Ccp_TxBuffer.au8Data[COM_U8_POS_DATA6] = 0xFF;
		Ccp_TxBuffer.au8Data[COM_U8_POS_DATA7] = 0xFF;

		Ccp_TxBuffer.u8Length = CCP_DATA_LENGTH;
		CCP_stParameter.SendStatus |= CCP_CRM_REQUEST;
   }
	return E_OK;
}

LOCAL Std_ReturnType M_u8CmdSetMTA(uint8 pid, uint8* data, int len)
{
   uint8 u8MTA;
	uint8 u8CmdCTR;
	uint8 u8AddExtension;
	uint32 *pu32Address;

	u8CmdCTR       = data[COM_U8_POS_DATA1];
	u8MTA          = data[COM_U8_POS_DATA2];
	u8AddExtension = data[COM_U8_POS_DATA3];
	pu32Address    = ((uint32*)&data[COM_U8_POS_DATA4]);

	if(u8MTA < CCP_MAX_MTA - 1) 
	{
		CCP_stParameter.pu8MTA[u8MTA] = (uint8*)pu32Address;
   } 

	/* Responce */
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA0] = PACKET_ID;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA1] = CRC_OK;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA2] = u8CmdCTR;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA3] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA4] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA5] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA6] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA7] = 0xFF;
	
	CCP_stParameter.SendStatus |= CCP_CRM_REQUEST;
	return E_OK;
}

LOCAL Std_ReturnType M_u8CmdDownload(uint8 pid, uint8* data, int len)
{
	uint8  u8CmdCTR;
	uint8  u8BlockSize;
	uint8  u8CmdReturnCode;
	uint8  u8ReturnCode;
	uint32 u32MTA0Address;
	BOOL_T bUserAccess_Flag;

	u8CmdCTR       = data[COM_U8_POS_DATA1];
	u8BlockSize    = data[COM_U8_POS_DATA2];
	u32MTA0Address = (uint32)CCP_stParameter.pu8MTA[0];

	if(CCP_stParameter.u8ProtectionStatus & CCP_PL_DAQ != CCP_PL_DAQ)
	{
		u8CmdReturnCode = CRC_ACCESS_DENIED;
	}
	else
	{
		bUserAccess_Flag = M_CcpUserAccess_Manage((uint32)CCP_stParameter.pu8MTA[0]);

		if(bUserAccess_Flag != B_FALSE)
		{
			u8ReturnCode = M_CcpWriteMTA(0x00,u8BlockSize,data);
			CCP_stParameter.pu8MTA[0] = (uint8 *)((uint32)u32MTA0Address + len);
		}
		else
		{
			u8ReturnCode = CRC_ACCESS_DENIED;
		}
	}
	
	if(u8ReturnCode == E_OK)
	{
		/* Responce */
	   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA0] = PACKET_ID;
	   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA1] = CRC_OK;
	   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA2] = u8CmdCTR;
	   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA3] = 0x02;
		Ccp_TxBuffer.au8Data[COM_U8_POS_DATA4] = (uint8)CCP_stParameter.pu8MTA[0]>>24;
		Ccp_TxBuffer.au8Data[COM_U8_POS_DATA5] = (uint8)CCP_stParameter.pu8MTA[0]>>16;
		Ccp_TxBuffer.au8Data[COM_U8_POS_DATA6] = (uint8)CCP_stParameter.pu8MTA[0]>>8;
		Ccp_TxBuffer.au8Data[COM_U8_POS_DATA7] = (uint8)CCP_stParameter.pu8MTA[0];
		
		CCP_stParameter.SendStatus |= CCP_CRM_REQUEST;
	}
	else
	{
				/* Responce */
	   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA0] = PACKET_ID;
	   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA1] = u8ReturnCode;
	   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA2] = u8CmdCTR;
	   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA3] = 0x02;
		Ccp_TxBuffer.au8Data[COM_U8_POS_DATA4] = 0xFF;
		Ccp_TxBuffer.au8Data[COM_U8_POS_DATA5] = 0xFF;
		Ccp_TxBuffer.au8Data[COM_U8_POS_DATA6] = 0xFF;
		Ccp_TxBuffer.au8Data[COM_U8_POS_DATA7] = 0xFF;
		
		CCP_stParameter.SendStatus |= CCP_CRM_REQUEST;
	}
	
	return E_OK;	
}

LOCAL Std_ReturnType M_u8CmdUpload(uint8 pid, uint8* data, int len)
{
	uint8 u8CmdCTR;
}

LOCAL Std_ReturnType M_u8CmdStartStop(uint8 pid, uint8* data, int len)
{
	uint8  u8CmdCTR;         /* Command Counter */
	uint8  u8SSCmd;          /* Start or Stop */
	uint8  u8DaqListNum;     /* DAQ list */
	uint8  u8LastODTNum;     /* Last ODT to send */
	uint8  u8EventChannel;	 /* Event Channel Number */
	uint16 u16Prescaler;     /* Prescaler */
	uint8  u8CmdReturnCode;

	u8CmdCTR       = data[COM_U8_POS_DATA1];
	u8SSCmd        = data[COM_U8_POS_DATA2];
	u8DaqListNum   = data[COM_U8_POS_DATA3];
	u8LastODTNum   = data[COM_U8_POS_DATA4];
	u8EventChannel = data[COM_U8_POS_DATA5];
	u16Prescaler   = (uint16)data[COM_U8_POS_DATA6]<<8 + (uint16)data[COM_U8_POS_DATA7];

	if(CCP_stParameter.u8ProtectionStatus & CCP_PL_DAQ != CCP_PL_DAQ)
	{
		u8CmdReturnCode = CRC_ACCESS_DENIED;
	}
	else if(CCP_stParameter.SessionStatus & SESSION_STATUS_DAQ != SESSION_STATUS_DAQ)
	{
		u8CmdReturnCode = CRC_CAL_INIT_REQUEST;
	}
	else if(u8DaqListNum > CCP_MAX_DAQ)
	{
		/* error info to be define */
		u8CmdReturnCode = CRC_OUT_OF_RANGE;
	}
	else
	{
		switch(u8SSCmd)
		{
			/* stop DAQ */
			case CCP_DAQ_STOP:
				M_CcpStopDaq(u8DaqListNum);
				u8CmdReturnCode = CRC_OK;
				break;
			case CCP_DAQ_START:
				M_CcpPrepareDaq(u8DaqListNum,u8LastODTNum,u8EventChannel,u16Prescaler);
				M_CcpStartDaq(u8DaqListNum);
				u8CmdReturnCode = CRC_OK;
				break;
			case CCP_DAQ_PREPARE:
				M_CcpPrepareDaq(u8DaqListNum,u8LastODTNum,u8EventChannel,u16Prescaler);
				u8CmdReturnCode = CRC_OK;
				break;
			default:
            u8CmdReturnCode = CRC_CMD_SYNTAX;
            break;		
		}
	}

	/* Responce */
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA0] = PACKET_ID;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA1] = u8CmdReturnCode;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA2] = u8CmdCTR;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA3] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA4] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA5] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA6] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA7] = 0xFF;
	
	Ccp_TxBuffer.u8Length = CCP_DATA_LENGTH;
	CCP_stParameter.SendStatus |= CCP_CRM_REQUEST;
	return E_OK;
}

LOCAL Std_ReturnType M_u8CmdDisconnect(uint8 pid, uint8* data, int len)
{
	uint8  u8CmdCTR;
	uint8  u8DisconnectCmd;
	uint16 u16StationAddr;

	u8CmdCTR        = data[COM_U8_POS_DATA1];
	u8DisconnectCmd = data[COM_U8_POS_DATA2];
	u16StationAddr  = (uint16)data[COM_U8_POS_DATA4] 
		             + (uint16)(data[COM_U8_POS_DATA5] << COM_U8_SHIFT_8_BIT);

	CCP_stParameter.SessionStatus &= ~SESSION_STATUS_CONNECTED;

	if(u8DisconnectCmd == DISCONNECT_TEMPORARY)
	{
		/* Temporary */
      CCP_stParameter.SessionStatus |= SESSION_STATUS_TMP_DISCONNECTED;
	}
	else
	{
		/* End of session */
		M_CcpStopAllDaq();
		/* Clear Protection Status */
		CCP_stParameter.u8ProtectionStatus = CCP_PL_CLEAR;
	}

	/* Responce */
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA0] = PACKET_ID;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA1] = CRC_OK;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA2] = u8CmdCTR;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA3] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA4] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA5] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA6] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA7] = 0xFF;
	
	CCP_stParameter.SendStatus |= CCP_CRM_REQUEST;
	return E_OK;
}

/* Select Calibration Page */
LOCAL Std_ReturnType M_u8CmdSelectCalPage(uint8 pid, uint8* data, int len)
{
	uint8  u8CmdCTR;

	u8CmdCTR = data[COM_U8_POS_DATA1];
	M_CcpSetCalPage((uint32)CCP_stParameter.pu8MTA[0]);

	/* Responce */
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA0] = PACKET_ID;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA1] = CRC_OK;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA2] = u8CmdCTR;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA3] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA4] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA5] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA6] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA7] = 0xFF;
	
	CCP_stParameter.SendStatus |= CCP_CRM_REQUEST;
	return E_OK;
}

/* Get Active Calibration Page */
LOCAL Std_ReturnType M_u8CmdGetActiveCalPage(uint8 pid, uint8* data, int len)
{
	uint8  u8CmdCTR;
	uint32 u32CalAddress;

	u8CmdCTR      = data[COM_U8_POS_DATA1];
	/*the start address of the calibration page*/
	u32CalAddress = M_CcpGetCalPage();   

	/* Responce */
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA0] = PACKET_ID;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA1] = CRC_OK;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA2] = u8CmdCTR;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA3] = 0x00;    /* Address Extension */
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA4] = (uint8)(u32CalAddress >> 24);
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA5] = (uint8)(u32CalAddress >> 16);
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA6] = (uint8)(u32CalAddress >> 8);
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA7] = (uint8)u32CalAddress;
	
	CCP_stParameter.SendStatus |= CCP_CRM_REQUEST;
	return E_OK;
}

/* Set Session Status */
LOCAL Std_ReturnType M_u8CmdSetSStatus(uint8 pid, uint8* data, int len)
{
	uint8  u8CmdCTR;
	
	u8CmdCTR        = data[COM_U8_POS_DATA1];
	CCP_stParameter.SessionStatus = data[COM_U8_POS_DATA2]; 

	/* Responce */
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA0] = PACKET_ID;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA1] = CRC_OK;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA2] = u8CmdCTR;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA3] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA4] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA5] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA6] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA7] = 0xFF;
	
	Ccp_TxBuffer.u8Length = CCP_DATA_LENGTH;
	CCP_stParameter.SendStatus |= CCP_CRM_REQUEST;
	return E_OK;
}

LOCAL Std_ReturnType M_u8CmdGetSStatus(uint8 pid, uint8* data, int len)
{
   uint8  u8CmdCTR;
	
	u8CmdCTR = data[COM_U8_POS_DATA1];

	/* Responce */
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA0] = PACKET_ID;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA1] = CRC_OK;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA2] = u8CmdCTR;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA3] = CCP_stParameter.SessionStatus;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA4] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA5] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA6] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA7] = 0xFF;
	
	Ccp_TxBuffer.u8Length = CCP_DATA_LENGTH;
	CCP_stParameter.SendStatus |= CCP_CRM_REQUEST;
	return E_OK;
}

LOCAL Std_ReturnType M_u8CmdBuildChecksum(uint8 pid, uint8* data, int len)
{
	uint8  u8CmdCTR;
	uint8 u8BlockSize;
	uint32 u32CheckSum;
	
	u8CmdCTR = data[COM_U8_POS_DATA1];
	u8BlockSize = 0x04;
	u32CheckSum = 0xFFFFFFFF;

	/* Responce */
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA0] = PACKET_ID;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA1] = CRC_OK;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA2] = u8CmdCTR;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA3] = u8BlockSize;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA4] = (uint8)(u32CheckSum >> 24);
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA5] = (uint8)(u32CheckSum >> 16);
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA6] = (uint8)(u32CheckSum >> 8);
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA7] = (uint8)u32CheckSum;
	
	Ccp_TxBuffer.u8Length = CCP_DATA_LENGTH;
	CCP_stParameter.SendStatus |= CCP_CRM_REQUEST;
	return E_OK;	
}

/* Get Seed for Key */
LOCAL Std_ReturnType M_u8CmdGetSeed(uint8 pid, uint8* data, int len)
{
	uint32 u32Seed;
	uint8  u8ResourceMask;
	uint8  u8ProtectionSts;
	uint8  u8CmdCTR;
	uint8  u8CmdReturnCode;

	u8CmdCTR           = data[COM_U8_POS_DATA1];
	u8ResourceMask     = data[COM_U8_POS_DATA2];
	u8ProtectionSts    = ((uint8)0x00);
	u32Seed            = ((uint32)0x00);

	/* Keys required for CAL or PGM */
	switch(u8ResourceMask)
	{
		case CCP_PL_CAL:
			u8ProtectionSts = (0x00 == (CCP_stParameter.u8ProtectionStatus & CCP_PL_CAL));
			u32Seed         = M_u32CcpGetSeed(CCP_PL_CAL);
			u8CmdReturnCode = CRC_OK;
			break;
			
		case CCP_PL_DAQ:
			u8ProtectionSts = (0x00 == (CCP_stParameter.u8ProtectionStatus & CCP_PL_DAQ));
			u32Seed         = M_u32CcpGetSeed(CCP_PL_DAQ);
			u8CmdReturnCode = CRC_OK;
			break;
			
		case CCP_PL_PGM:
			u8ProtectionSts = (0x00 == (CCP_stParameter.u8ProtectionStatus & CCP_PL_PGM));
			u32Seed         = M_u32CcpGetSeed(CCP_PL_PGM);
			u8CmdReturnCode = CRC_OK;
			break;
		default:
			u8CmdReturnCode = CRC_CMD_SYNTAX;
			break;
	}
	
	/* Responce */
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA0] = PACKET_ID;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA1] = u8CmdReturnCode;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA2] = u8CmdCTR;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA3] = u8ProtectionSts;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA4] = (uint8)(u32Seed >> COM_U8_SHIFT_24_BIT);
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA5] = (uint8)(u32Seed >> COM_U8_SHIFT_16_BIT);
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA6] = (uint8)(u32Seed >> COM_U8_SHIFT_8_BIT);
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA7] = (uint8)u32Seed;
	
	CCP_stParameter.SendStatus |= CCP_CRM_REQUEST;
	return E_OK;
}

/* Unlock Protection */
LOCAL Std_ReturnType M_u8CmdUnlock(uint8 pid, uint8* data, int len)
{
	uint8  u8CmdCTR;
	uint32 u32Key;
	uint8  u8ResourceMask;

	u8CmdCTR = data[COM_U8_POS_DATA1];
	u32Key   = ((uint32)data[COM_U8_POS_DATA2] << COM_U8_SHIFT_24_BIT)
		      + ((uint32)data[COM_U8_POS_DATA3] << COM_U8_SHIFT_16_BIT)
		      + ((uint32)data[COM_U8_POS_DATA4] << COM_U8_SHIFT_8_BIT)
		      + (uint32)data[COM_U8_POS_DATA5];
	/* Check key */
	/* Reset the appropriate resource protection mask bit */
   CCP_stParameter.u8ProtectionStatus |= M_u8CcpUnlock(u32Key); 
	u8ResourceMask = CCP_stParameter.u8ProtectionStatus;	

	/* Responce */
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA0] = PACKET_ID;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA1] = CRC_OK;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA2] = u8CmdCTR;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA3] = u8ResourceMask;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA4] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA5] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA6] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA7] = 0xFF;
	
	CCP_stParameter.SendStatus |= CCP_CRM_REQUEST;
	return E_OK;
}

/* Return the size of a DAQ list and clear */
LOCAL Std_ReturnType M_u8CmdGetDaqSize(uint8 pid, uint8* data, int len)
{
	uint8  u8DaqListNum;
	uint8  u8CmdCTR;
	uint32 u32DaqID;
	uint8  u8DaqListSize;
	uint8  u8FirstPID;

   u8CmdCTR     = data[COM_U8_POS_DATA1];
	u8DaqListNum = data[COM_U8_POS_DATA2];
	u32DaqID     = ((uint32)data[COM_U8_POS_DATA4] << COM_U8_SHIFT_24_BIT)
		          + ((uint32)data[COM_U8_POS_DATA5] << COM_U8_SHIFT_16_BIT)
		          + ((uint32)data[COM_U8_POS_DATA6] << COM_U8_SHIFT_8_BIT)
		          + (uint32)data[COM_U8_POS_DATA7];

   /* Stop this daq list */
	M_CcpStopDaq(u8DaqListNum);
	/* Number of  ODTs */
	u8DaqListSize = M_CcpClearDaqList(u8DaqListNum);
   /* PID of the first ODT */
	u8FirstPID = u8DaqListNum * CCP_MAX_ODT;

	/* Responce */
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA0] = PACKET_ID;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA1] = CRC_OK;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA2] = u8CmdCTR;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA3] = u8DaqListSize;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA4] = u8FirstPID;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA5] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA6] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA7] = 0xFF;
	
	CCP_stParameter.SendStatus |= CCP_CRM_REQUEST;
	return E_OK;	
}

/* Set DAQ pointer */
LOCAL Std_ReturnType M_u8CmdSetDaqPtr(uint8 pid, uint8* data, int len)
{
	uint8  u8CmdCTR; 
	uint8  u8CmdReturnCode;

	u8CmdCTR     = data[COM_U8_POS_DATA1];
   CCP_u8DaqListNum = data[COM_U8_POS_DATA2];
	CCP_u8OdtNum     = data[COM_U8_POS_DATA3];
	CCP_u8ElementNum = data[COM_U8_POS_DATA4];
	
	u8CmdReturnCode = CRC_OK;
	
	if (CCP_u8DaqListNum >= CCP_MAX_DAQ || CCP_u8OdtNum >= CCP_MAX_ODT || CCP_u8ElementNum > 0x07) 
	{
      u8CmdReturnCode = CRC_CMD_SYNTAX;
      CCP_stParameter.DaqListPtr = 0;
   } 
	
   /* Responce */
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA0] = PACKET_ID;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA1] = u8CmdReturnCode;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA2] = u8CmdCTR;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA3] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA4] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA5] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA6] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA7] = 0xFF;

	Ccp_TxBuffer.u8Length = CCP_DATA_LENGTH;
	CCP_stParameter.SendStatus |= CCP_CRM_REQUEST;
	return E_OK;	
}

LOCAL Std_ReturnType M_u8CmdWriteDaq(uint8 pid, uint8* data, int len)
{
	uint32 u32WriteDaqAddr;
	uint8  u8WriteDaqAddrExt;
	uint8  u8WriteDaqSize;    
	uint8  u8CmdCTR;
	uint8  u8CmdReturnCode;
	uint8  *pu8PtrTemp;
	uint8  u8Loop;
	
	u8CmdCTR            = data[COM_U8_POS_DATA1];
	u8WriteDaqSize      = data[COM_U8_POS_DATA2];
	u8WriteDaqAddrExt   = data[COM_U8_POS_DATA3];
	u32WriteDaqAddr     = ((uint32)data[COM_U8_POS_DATA4] << COM_U8_SHIFT_24_BIT)
		                 + ((uint32)data[COM_U8_POS_DATA5] << COM_U8_SHIFT_16_BIT)
		                 + ((uint32)data[COM_U8_POS_DATA6] << COM_U8_SHIFT_8_BIT)
		                 + (uint32)data[COM_U8_POS_DATA7];

	u8CmdReturnCode = CRC_OK;
	pu8PtrTemp = M_CcpGetDaqPointer(u8WriteDaqAddrExt,u32WriteDaqAddr);
	if(((u8WriteDaqSize != 1)&&(u8WriteDaqSize != 2)&&(u8WriteDaqSize != 4))||(u8WriteDaqSize == 0))
	{
		u8CmdReturnCode = CRC_CMD_SYNTAX;
	}
   else
   {
   	for(u8Loop=0;u8Loop<u8WriteDaqSize;u8Loop++)
   	{
   		CCP_stParameter.DaqList[CCP_u8DaqListNum].astOdt[CCP_u8OdtNum].au8OdtData[CCP_u8ElementNum + u8Loop] = (uint8 *)pu8PtrTemp;
		   pu8PtrTemp ++;
   	}
		CCP_stParameter.DaqList[CCP_u8DaqListNum].astOdt[CCP_u8OdtNum].u8ODTSetFlag = 0x01;
		CCP_stParameter.DaqList[CCP_u8DaqListNum].astOdt[CCP_u8OdtNum].u8Size= u8WriteDaqSize;
   }
	/* Responce */
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA0] = PACKET_ID;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA1] = u8CmdReturnCode;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA2] = u8CmdCTR;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA3] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA4] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA5] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA6] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA7] = 0xFF;
	
	Ccp_TxBuffer.u8Length = CCP_DATA_LENGTH;
	CCP_stParameter.SendStatus |= CCP_CRM_REQUEST;
	return E_OK;	
}

LOCAL Std_ReturnType M_u8CmdExchangeID(uint8 pid, uint8* data, int len)
{
   uint8 u8MasterID;
	uint8 u8Index;
	uint8 u8Length;
	uint8 u8CmdCTR;
	uint8 u8CmdReturnCode;
	uint8 u8ResAvailabilityMask;
	uint8 u8ResProtectionMask;

   u8CmdCTR        = data[COM_U8_POS_DATA1];
	u8CmdReturnCode = CRC_OK;
	u8Length        = sizeof(CCP_au8SlaveDeviceID);

	/* Build the Resource Availability and Protection Mask */
	u8ResAvailabilityMask = CCP_PL_CAL;    /* Default: Calibration available */
	u8ResProtectionMask   = 0x00;          /* Default: No Protection */

#ifdef CCP_SEED_KEY
   u8ResProtectionMask |= CCP_PL_CAL;     /* Protected Calibration */
#endif

#ifdef CCP_DAQ
	u8ResAvailabilityMask |= CCP_PL_DAQ;   /* Data Acquisition */
#ifdef CCP_SEED_KEY
	u8ResProtectionMask |= CCP_PL_DAQ;     /* Protected Data Acquisition */
#endif
#endif

#if defined(CCP_PROGRAM) || defined(CCP_BOOTLOADER_DOWNLOAD)
	u8ResAvailabilityMask |= CCP_PL_PGM;   /* Flash Programming */
#ifdef CCP_SEED_KEY
	u8ResProtectionMask |= CCP_PL_PGM;     /* Protected Flash Programming */
#endif
#endif

	CCP_stParameter.pu8MTA[0] = (uint8*)CCP_au8SlaveDeviceID;

	/* Responce */
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA0] = PACKET_ID;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA1] = u8CmdReturnCode;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA2] = u8CmdCTR;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA3] = u8Length;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA4] = 0x00;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA5] = u8ResAvailabilityMask;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA6] = u8ResProtectionMask;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA7] = 0xFF;

	Ccp_TxBuffer.u8Length = CCP_DATA_LENGTH;
	CCP_stParameter.SendStatus |= CCP_CRM_REQUEST;
	return E_OK;	
}

LOCAL Std_ReturnType M_u8CmdGetCcpVersion(uint8 pid, uint8* data, int len)
{
   uint8 u8MainVersion;
	uint8 u8ReleaseVersion;
	uint8 u8CmdCTR;
	uint8 u8CmdReturnCode;

	u8CmdCTR            = data[COM_U8_POS_DATA1];
	u8MainVersion       = data[COM_U8_POS_DATA2];
	u8ReleaseVersion    = data[COM_U8_POS_DATA3];
   u8CmdReturnCode     = CRC_OK;
	
	/* Responce */
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA0] = PACKET_ID;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA1] = u8CmdReturnCode;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA2] = u8CmdCTR;
   Ccp_TxBuffer.au8Data[COM_U8_POS_DATA3] = u8MainVersion;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA4] = u8ReleaseVersion;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA5] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA6] = 0xFF;
	Ccp_TxBuffer.au8Data[COM_U8_POS_DATA7] = 0xFF;
	
	CCP_stParameter.SendStatus |= CCP_CRM_REQUEST;
	return E_OK;	
}

LOCAL Std_ReturnType M_u8CmdDnload6(uint8 pid, uint8* data, int len)
{   pid = 0;
	data = NULLPTR;
	len = 0;
}

LOCAL uint32 M_u32GetRandomValue(void)
{
	uint32 u32RandomValue;

	u32RandomValue = (uint32)rand();
	return(u32RandomValue);
}

LOCAL uint32 M_u32CcpGetSeed(uint8 u8ResourceMask)
{
	CCP_u8ResourceMask = u8ResourceMask;
	CCP_u32CcpSeed = M_u32GetRandomValue();
	return(CCP_u32CcpSeed);
}

LOCAL uint8 M_u8CcpUnlock(uint32 u32CcpKey)
{
	uint32 u32Key;
	uint8  u8ReturnValue;
	
	u32Key = ~CCP_u32CcpSeed;
	/* Check the key */
	if(u32Key != u32CcpKey)
	{
		u8ReturnValue = 0x00;
	}
	else
	{
		u8ReturnValue = CCP_u8ResourceMask;
	}
	return(u8ReturnValue);
	
}

/* Stop DAQ */
LOCAL void M_CcpStopDaq(uint8 u8Daq) 
{

  uint8 u8DaqIndex;

  if(u8Daq >= CCP_MAX_DAQ) 
  {
     return;
  }
  
  CCP_stParameter.DaqList[u8Daq].u8Flags = DAQ_FLAG_STOP;

  /* check if all DAQ lists are stopped */
  for (u8DaqIndex = 0;u8DaqIndex < CCP_MAX_DAQ;u8DaqIndex++) 
  {
     if(CCP_stParameter.DaqList[u8DaqIndex].u8Flags & DAQ_FLAG_START) 
	  {
	     return;
     }
  }

  CCP_stParameter.SessionStatus &= ~SESSION_STATUS_RUN;
}


/* Stop all DAQs */
LOCAL void M_CcpStopAllDaq(void) 
{
  uint8 u8DaqIndex;

  for (u8DaqIndex=0;u8DaqIndex<CCP_MAX_DAQ;u8DaqIndex++) 
  {
     CCP_stParameter.DaqList[u8DaqIndex].u8Flags = DAQ_FLAG_STOP;
  }
  CCP_stParameter.SessionStatus &= ~SESSION_STATUS_RUN;
}

/* Start DAQ */
LOCAL void M_CcpStartDaq(uint8 u8Daq) 
{
  CCP_stParameter.DaqList[u8Daq].u8Flags = DAQ_FLAG_START;
  CCP_stParameter.SessionStatus |= SESSION_STATUS_RUN;
}

/* Prepare DAQ */
LOCAL void M_CcpPrepareDaq(uint8 u8Daq, uint8 u8Last, uint8 u8EventChannel, uint16 u16Prescaler) 
{
  CCP_stParameter.DaqList[u8Daq].u8EventChannel = u8EventChannel;
  if(u16Prescaler == 0x00)
  {
     u16Prescaler = 0x01;
  }
  CCP_stParameter.DaqList[u8Daq].u16Prescaler = u16Prescaler;
  CCP_stParameter.DaqList[u8Daq].u16Cycle     = 0x01;
  CCP_stParameter.DaqList[u8Daq].u8LastOdtNum = u8Last;
  CCP_stParameter.DaqList[u8Daq].u8Flags      = DAQ_FLAG_PREPARED;
}

/* Clear DAQ list */
LOCAL uint8 M_CcpClearDaqList(uint8 u8Daq) 
{
  uint8 u8DaqSize;
  uint8 *p;
  uint8 *pl;

  u8DaqSize = CCP_MAX_ODT;
  
  if(u8Daq >= CCP_MAX_DAQ) 
  {
  		u8DaqSize = 0;
  }

  /* Clear this daq list to zero */
  p  = (uint8 *)&CCP_stParameter.DaqList[u8Daq];
  pl = p + sizeof(CCP_DAQ_LIST_T);
  while(p<pl)
  {
     *p ++ = 0x00;
  }
  
  return(u8DaqSize);
}

LOCAL uint8 *M_CcpGetDaqPointer(uint8 u8Addr_ext, uint32 u32Addr)
{
	uint8 *pu8Address;

	pu8Address = (uint8*)u32Addr;
	return((uint8*)pu8Address);
}

LOCAL void M_CcpSetCalPage(uint32 u32CalAddress)
{
	CAL_stParameter.u32CalibrationMapAddr = u32CalAddress;
}

LOCAL uint32 M_CcpGetCalPage(void)
{
	return(CAL_stParameter.u32CalibrationMapAddr);
}

LOCAL uint8 M_CcpWriteMTA(uint8 u8MtaIndex,uint8 u8Size,uint8 *pu8Data)
{
	uint8 u8Index;

	for(u8Index = 0;u8Index < u8Size;u8Index ++)
	{
		*(uint8 *)CCP_stParameter.pu8MTA[u8MtaIndex] = *pu8Data;
		CCP_stParameter.pu8MTA[u8MtaIndex] ++;
		pu8Data ++;
	}

	/*write the data to the memory ,this just for test */
	//memcpy(&CCP_au8TestArea,&pu8Data,u8Size);
	return E_OK;
}

LOCAL void M_CcpGetMTA( uint8 *u8Addr_ext, uint32 *u32Addr ) 
{
	*u8Addr_ext = 0;
	*u32Addr = (uint32)CCP_stParameter.pu8MTA[0];
}

LOCAL void M_CcpClearRxBuffer(void)
{
	uint8 u8Index;
	
	for(u8Index = 0;u8Index < Ccp_RxBuffer.u8Length;u8Index++)
	{
		Ccp_RxBuffer.au8Data[u8Index] = KUC_NULL;
	}
}

/* Sample the DTM */
LOCAL void M_CcpSampleAndTransmitDTM(uint8 u8Pid, uint8 u8DaqNum, uint8 u8OdtNum) 
{
   CCP_ST_ODT_ENTRY_T *pstDaqPtr;
	uint8 u8BuffIndex;
	uint8 u8OdtSize;

	/* PID */
   CCP_stParameter.au8DaqTxBuffer[0] = u8Pid;

   pstDaqPtr = &CCP_stParameter.DaqList[u8DaqNum].astOdt[u8OdtNum];
	u8OdtSize = pstDaqPtr->u8Size;
	
	for(u8BuffIndex = 1;u8BuffIndex < 8;u8BuffIndex++)
	{
		CCP_stParameter.au8DaqTxBuffer[u8BuffIndex] = *pstDaqPtr->au8OdtData[u8BuffIndex];
	}

   CCP_stParameter.SendStatus |= CCP_DTM_REQUEST;
	CCP_DAQTransmit(CCP_stParameter.au8DaqTxBuffer, 8);
}

LOCAL void M_CcpDAQProcess(uint8 u8EventChannel) 
{
	uint8 u8OdtNum;
	uint8 u8DaqNum;
	uint8 u8OdtIndex;
	
	for (u8OdtNum=0x00,u8DaqNum=0x00; u8DaqNum<CCP_MAX_DAQ; u8OdtNum+=CCP_MAX_ODT,u8DaqNum++)
	{
		if ((CCP_stParameter.DaqList[u8DaqNum].u8Flags & DAQ_FLAG_START) != KUC_NULL) 
		{
			CCP_stParameter.DaqList[u8DaqNum].u8Flags |= DAQ_FLAG_SEND;
			
			for (u8OdtIndex=0;u8OdtIndex<=CCP_stParameter.DaqList[u8DaqNum].u8LastOdtNum;u8OdtIndex++) 
			{
				if(CCP_stParameter.DaqList[u8DaqNum].astOdt[u8OdtIndex].u8ODTSetFlag == 0x01)
				{
					switch(u8EventChannel)
					{

						case DAQ_10MS_EVENT:
							if(DAQ_bTimer10msFlag != B_FALSE)
							{
								/* Sample for every ODT which in the DAQ list */
								M_CcpSampleAndTransmitDTM(u8OdtIndex+u8OdtNum, u8DaqNum, u8OdtIndex);
							}
							break;
								
						case DAQ_100MS_EVENT:
							if(DAQ_bTimer100msFlag != B_FALSE)
							{
								/* Sample for every ODT which in the DAQ list */
								M_CcpSampleAndTransmitDTM(u8OdtIndex+u8OdtNum, u8DaqNum, u8OdtIndex);
							}
							break;
						default:
							break;
					}
				}
	      }
		}
	}
}

LOCAL BOOL_T M_CcpUserAccess_Manage(uint32 u32CalAddress)
{
	BOOL_T bUserAccessFlag;
	
	bUserAccessFlag = B_FALSE;
	
	if((u32CalAddress >= CAL_START_ADDRESS) && (u32CalAddress < CAL_END_ADDRESS))
	{
		bUserAccessFlag = B_TRUE;
	}
	else
	{
		bUserAccessFlag = B_FALSE;
	}

	return(bUserAccessFlag);
}
/**************************************************************************
Exported Functions
**************************************************************************/
/**************************************************************************
Object:Ccp_MainFunction() Scheduled function of the CCP module ,
       It should be called cyclically.
Parameters:none
Return:none
**************************************************************************/
EXPORTED void CCP_Init(void)
{
	uint8 u8Index;
   uint8 u8OdtNum;
	uint8 u8DaqNum;
	uint8 u8OdtIndex;

	DAQ_u8TimingCounter = KUC_NULL;
	DAQ_bTimer10msFlag  = B_FALSE;
	DAQ_bTimer100msFlag = B_FALSE;
	
	for(u8DaqNum = 0x00; u8DaqNum < CCP_MAX_DAQ; u8DaqNum ++)
	{
		for(u8OdtNum = 0x00;u8OdtNum < CCP_MAX_ODT;u8OdtNum ++)
		{
			for(u8OdtIndex=0;u8OdtIndex<=0x07;u8OdtIndex++) 
		   {
				CCP_stParameter.DaqList[u8DaqNum].astOdt[u8OdtNum].au8OdtData[u8OdtIndex] = KUC_NULL;
		   }
		}
	}

	for(u8Index=0;u8Index<5;u8Index++)
	{
		CCP_au8TestArea[u8Index] = 0x22;
	}

	
}

/**************************************************************************
Object	:void CCP_DAQ_TimingProcess(void) 
Parameters:none		
Return:	none
**************************************************************************/
EXPORTED void CCP_DAQTimingProcess(void)
{
	DAQ_u8TimingCounter++;
	if(DAQ_u8TimingCounter > 0x02)
	{
		DAQ_bTimer10msFlag = B_TRUE;
	}
	else if(DAQ_u8TimingCounter > 0x14)
	{
		DAQ_bTimer100msFlag = B_TRUE;
		DAQ_u8TimingCounter = KUC_NULL;
	}
	else
	{
		/*do nothing*/
	}
}
/**************************************************************************
Object:Ccp_MainFunction() Scheduled function of the CCP module ,
       It should be called cyclically.
Parameters:none
Return:none
**************************************************************************/
EXPORTED void CCP_MainFunction(void)
{
    Ccp_Recieve_Main();
    Ccp_Transmit_Main();
}

/**************************************************************************
Object:Receive callback from CAN network layer,This function is called by the 
       lower layers when an AUTOSAR CCP PDU has been received
Parameters:CcpRxPduId  PDU-ID that has been received
           CcpRxPduPtr Pointer to SDU (Buffer of received payload)
Return:none
**************************************************************************/
EXPORTED void CCP_CanRxIndication(const PduIdType CcpRxPduId,const PduInfoType *CcpRxPduPtr)
{
	 Ccp_RxIndication(CcpRxPduPtr->sduDataPtr, (uint8)CcpRxPduPtr->sduLength);
}

/**************************************************************************
Object:CCP DAQ message transmit  
Parameters:data : DAQ data need to be send
           len: data length
Return:none
**************************************************************************/
EXPORTED void CCP_DAQTransmit(const uint8 *data, int len)
{
    PduInfoType pdu;
	 
    pdu.sduDataPtr = (uint8*)data;
    pdu.sduLength  = (uint16)len;
	 if((CCP_stParameter.SendStatus & CCP_DTM_REQUEST )!= KUC_NULL)
	 {
	    CanIf_Transmit(CCP_DAQ_ID_TX, &pdu);
	 }
}

/**************************************************************************
Object:CCP DAQ Data aquisition and transmit  
Parameters:u8EventChannel : event channel ,such as 10ms ,100ms..          
Return:none
**************************************************************************/
EXPORTED void CCP_DAQMainFunction(void) 
{
	uint8 u8DaqNum;
	
	for(u8DaqNum=0x00; u8DaqNum<CCP_MAX_DAQ; u8DaqNum++)
	{
		if(CCP_stParameter.DaqList[u8DaqNum].u8EventChannel == DAQ_10MS_EVENT)
		{
			M_CcpDAQProcess(DAQ_10MS_EVENT); 
		}
		else if(CCP_stParameter.DaqList[u8DaqNum].u8EventChannel == DAQ_100MS_EVENT)
		{
			M_CcpDAQProcess(DAQ_100MS_EVENT);
		}
		else if(CCP_stParameter.DaqList[u8DaqNum].u8EventChannel == DAQ_TRIGGER_EVENT)
		{
			M_CcpDAQProcess(DAQ_TRIGGER_EVENT);
		}
		else
		{
			/*do nothing*/
		}
	}
	
}
/*************************************************************************
Evolution of the component

$Log: Ccp.c  $
Revision 1.0 2016/4/25 MingDong Yuan 
First creation

*************************************************************************/

/* end of file */
