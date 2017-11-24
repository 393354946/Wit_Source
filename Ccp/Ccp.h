/*============================================================================*/
/* Copyright (C) 2015-2016, Chengdu WeiTe Dianpen CO.,LTD.
 *  
 *  All rights reserved. This software is WIT property. Duplication 
 *  or disclosure without WIT written authorization is prohibited.
 *  
 *  
 *  @file       <Ccp.c>
 *  @brief      <Ccp for MPC5634> 
 *  @author     <Mingdong.Yuan>
 *  @date       <2016-4-22>
 */
/*============================================================================*/
#include "Std_Types.h"
#include "Can.h"

/*************************************************************************
Declaration of constants
*************************************************************************/
#define COM_U8_POS_DATA0                0x00
#define COM_U8_POS_DATA1                0x01
#define COM_U8_POS_DATA2                0x02
#define COM_U8_POS_DATA3                0x03
#define COM_U8_POS_DATA4                0x04
#define COM_U8_POS_DATA5                0x05
#define COM_U8_POS_DATA6                0x06
#define COM_U8_POS_DATA7                0x07
#define COM_U8_POS_DATA8                0x08

#define COM_U8_SHIFT_1_BIT          	 0x01
#define COM_U8_SHIFT_2_BIT          	 0x02
#define COM_U8_SHIFT_4_BIT          	 0x04
#define COM_U8_SHIFT_6_BIT          	 0x06
#define COM_U8_SHIFT_8_BIT          	 0x08
#define COM_U8_SHIFT_16_BIT         	 0x10
#define COM_U8_SHIFT_24_BIT         	 0x18

/* STANDARD COMMANDS */                                                    
#define CCP_CMD_CONNECT                 0x01    
#define CCP_CMD_SET_MTA                 0x02
#define CCP_CMD_DOWNLOAD                0x03
#define CCP_CMD_UPLOAD                  0x04  
#define CCP_CMD_TEST                    0x05 
#define CCP_CMD_START_STOP              0x06 
#define CCP_CMD_DISCONNECT              0x07
#define CCP_CMD_GET_ACTIVE_CAL_PAGE     0x09
#define CCP_CMD_SET_S_STATUS            0x0C
#define CCP_CMD_GET_S_STATUS            0x0D
#define CCP_CMD_BUILD_CHKSUM				 0x0E
#define CCP_CMD_SELECT_CAL_PAGE         0x11
#define CCP_CMD_GET_SEED                0x12
#define CCP_CMD_UNLOCK                  0x13
#define CCP_CMD_GET_DAQ_SIZE            0x14
#define CCP_CMD_SET_DAQ_PTR             0x15
#define CCP_CMD_WRITE_DAQ               0x16
#define CCP_CMD_EXCHANGE_ID             0x17
#define CCP_CMD_GET_CCP_VERSION         0x1B
#define CCP_CMD_DNLOAD6                 0x23

#define CCP_CMD_NUMBER                  0x13 
#define CCP_MAX_DTO                     0xFF

/*CCP PDU and DAQ ID to be define*/
#define CCP_PDU_ID_TX						 0x02
#define CCP_DAQ_ID_TX						 0x03

/*CCP message send flag */
#define CCP_MSG_SEND					       0x01
#define CCP_MSG_SEND_CLEAR					 0x00

/* Bitmasks for ccp.SendStatus */
#define CCP_CRM_REQUEST  0x01
#define CCP_DTM_REQUEST  0x02
#define CCP_USR_REQUEST  0x04
#define CCP_CMD_PENDING  0x08
#define CCP_CRM_PENDING  0x10
#define CCP_DTM_PENDING  0x20
#define CCP_USR_PENDING  0x40
#define CCP_TX_PENDING   0x80
#define CCP_SEND_PENDING (CCP_DTM_PENDING|CCP_CRM_PENDING|CCP_USR_PENDING)
/************** CCP parameters *************/
#define CCP_SLAVE_DEVICE_ID    "ECU001"

#define PACKET_ID              0xFF
/* Command Return codes */
#define CRC_OK                 0x00          /*acknowledge / no error*/			

/* Error category :C1 */
#define CRC_CMD_BUSY           0x10
#define CRC_DAQ_BUSY           0x11
#define CRC_KEY_REQUEST        0x18
#define CRC_STATUS_REQUEST     0x19

/* Error category :C2 */
#define CRC_COLD_START_REQUEST 0x20
#define CRC_CAL_INIT_REQUEST   0x21
#define CRC_DAQ_INIT_REQUEST   0x22
#define CRC_CODE_REQUEST       0x23

/* Error category :C3 */
#define CRC_CMD_UNKNOWN        0x30
#define CRC_CMD_SYNTAX         0x31
#define CRC_OUT_OF_RANGE       0x32
#define CRC_ACCESS_DENIED      0x33			/*access denied*/
#define CRC_OVERLOAD           0x34
#define CRC_ACCESS_LOCKED      0x35

/* CCP Identifiers and Address */
#define CCP_STATION_ADDR  0x0200      /* Define CCP_STATION_ADDR in Intel Format */
#define CCP_DATA_LENGTH   0x08

#define CCP_DTO_ID        0x101       /* CAN identifier ECU -> Master */
#define CCP_CRO_ID        0x100       /* CAN identifier Master -> ECU */
#define CCP_DAQ_ID        0x105       /* CAN identifier ECU -> Master */

/* Session Status */
#define SESSION_STATUS_CAL                0x01
#define SESSION_STATUS_DAQ                0x02
#define SESSION_STATUS_RESUME             0x04
#define SESSION_STATUS_TMP_DISCONNECTED   0x10
#define SESSION_STATUS_CONNECTED          0x20
#define SESSION_STATUS_STORE              0x40
#define SESSION_STATUS_RUN                0x80

#define DISCONNECT_TEMPORARY              0x00
#define DISCONNECT_END_SESSION            0x01

#define CCP_MAX_MTA                       0x02
/* Priviledge Level:Resource Mask*/
#define 	CCP_PL_CLEAR 	                  0x00
#define 	CCP_PL_CAL 	                     0x01
#define 	CCP_PL_DAQ 	                     0x02
#define 	CCP_PL_PGM 	                     0x40

/* CCP Data Acuisition Parameters */
#define CCP_DAQ                                   /* Enable synchronous data aquisition in M_CcpDAQProcess() */
#define CCP_MAX_ODT                       0x14    /* Number of ODTs in each DAQ lists */
#define CCP_MAX_DAQ                       0x03    /* Number of DAQ lists */

/* DAQ flag list */
#define DAQ_FLAG_STOP                     0x00
#define DAQ_FLAG_START                    0x01
#define DAQ_FLAG_SEND                     0x02
#define DAQ_FLAG_PREPARED                 0x04
#define DAQ_FLAG_OVERRUN                  0x80

/* Declare CCP-protocol version */
#define CCP_VERSION_MAJOR                 0x02
#define CCP_VERSION_MINOR                 0x01

/* Mode : start / stop / prepare data tranmission */
#define CCP_DAQ_STOP                     0x00
#define CCP_DAQ_START                    0x01
#define CCP_DAQ_PREPARE                  0x02

/*DAQ Event channel define*/
#define DAQ_10MS_EVENT                   0x00u
#define DAQ_100MS_EVENT                  0x01u
#define DAQ_TRIGGER_EVENT                0x02u

/*Calibration define*/
#define CAL_START_ADDRESS                0x4000B800
#define CAL_END_ADDRESS                  0x4000F800

#define CCP_SEED_KEY
#undef  CCP_BOOTLOADER_DOWNLOAD
#define CCP_PROGRAM
#define CCP_WRITE_PROTECTION
/*************************************************************************
Declaration of types
*************************************************************************/
//typedef Std_ReturnType (*CCP_pfCmdFuncType)(uint8, void*, int);
typedef Std_ReturnType (CCP_PF_CMD_FUN)(uint8, uint8*, int);
typedef struct 
{
	 uint8             Ccp_Cmd;
    //uint8           Ccp_len;         /* minimum length of command  */
	 CCP_PF_CMD_FUN    *pfCcpCmdFunc;   /* pointer to function to use */
    //uint8           lock;            /* locked by following types  (Ccp_ProtectType) */
}Ccp_CmdListType;

typedef struct CCP_ST_BUFFER_TAG 
{
    uint8          u8Length;
    uint8          au8Data[CCP_MAX_DTO];
}CCP_ST_BUFFER_T;

/* ODT entry */
typedef struct CCP_ST_ODT_ENTRY_TAG
{
  uint8 *au8OdtData[7];
  uint8 *pu8Ptr;
  uint8  u8Size;
  uint8  u8ODTSetFlag;
}CCP_ST_ODT_ENTRY_T;

/* ODT */
//typedef CCP_ST_ODT_ENTRY_T CCP_ST_ODT_T[7];
typedef CCP_ST_ODT_ENTRY_T CCP_ST_ODT_T;
typedef struct CCP_DAQ_LIST_TAG 
{
  CCP_ST_ODT_T  astOdt[CCP_MAX_ODT];
  uint16        u16Prescaler;
  uint16        u16Cycle;
  uint8         u8EventChannel;
  uint8         u8LastOdtNum;
  uint8         u8Flags;
  //uint8         u8ODTSetFlag;
}CCP_DAQ_LIST_T;

typedef struct CCP_ST_PARAMETER_TAG
{
  uint8                SessionStatus;
  uint8                SendStatus;
  uint8                *pu8MTA[CCP_MAX_MTA];                   /* Memory Transfer Address */
#ifdef CCP_SEED_KEY
  uint8                u8ProtectionStatus;
#endif
  uint8                au8DaqTxBuffer[8];         /* DTM Data Transmission Message buffer */
  CCP_ST_ODT_ENTRY_T   *DaqListPtr;               /* Pointer for SET_DAQ_PTR */
  CCP_DAQ_LIST_T       DaqList[CCP_MAX_DAQ];      /* DAQ list */
	 
}CCP_ST_PARAMETER_T;

typedef struct CAL_ST_PARAMETER_TAG
{
	uint32 u32CalibrationMapAddr;                   /* Calibration area MAP start address  */
	uint16 u16CalibrationRamFreeLength;             /* Calibration area free length */
}CAL_ST_PARAMETER_T;

/*************************************************************************
Declaration of variables
*************************************************************************/

extern CCP_ST_PARAMETER_T       CCP_stParameter;
extern CAL_ST_PARAMETER_T       CAL_stParameter;
extern CCP_ST_BUFFER_T   Ccp_RxBuffer; /* CRO :Command Receive Object  buffer */
extern CCP_ST_BUFFER_T   Ccp_TxBuffer; /* CRM :Command Return Message  buffer */
extern uint8             CCP_u8ResourceMask;
extern uint32            CCP_u32CcpSeed;
extern uint8             CCP_au8TestArea[5];
extern uint8             CCP_au8Data[9];

extern uint8  CCP_u8DaqListNum;
extern uint8  CCP_u8OdtNum;
extern uint8  CCP_u8ElementNum;

extern uint8   DAQ_u8TimingCounter;
extern boolean DAQ_bTimer10msFlag;
extern boolean DAQ_bTimer100msFlag;
/*************************************************************************
Declaration of functions
*************************************************************************/
static Std_ReturnType M_u8CmdConnect(uint8 pid, uint8* data, int len);
static Std_ReturnType M_u8CmdSetMTA(uint8 pid, uint8* data, int len);
static Std_ReturnType M_u8CmdDownload(uint8 pid, uint8* data, int len);
static Std_ReturnType M_u8CmdUpload(uint8 pid, uint8* data, int len);
static Std_ReturnType M_u8CmdStartStop(uint8 pid, uint8* data, int len);
static Std_ReturnType M_u8CmdDisconnect(uint8 pid, uint8* data, int len);
static Std_ReturnType M_u8CmdGetActiveCalPage(uint8 pid, uint8* data, int len);
static Std_ReturnType M_u8CmdSetSStatus(uint8 pid, uint8* data, int len);
static Std_ReturnType M_u8CmdGetSStatus(uint8 pid, uint8* data, int len);
static Std_ReturnType M_u8CmdSelectCalPage(uint8 pid, uint8* data, int len);
static Std_ReturnType M_u8CmdGetSeed(uint8 pid, uint8* data, int len);
static Std_ReturnType M_u8CmdUnlock(uint8 pid, uint8* data, int len);
static Std_ReturnType M_u8CmdGetDaqSize(uint8 pid, uint8* data, int len);
static Std_ReturnType M_u8CmdSetDaqPtr(uint8 pid, uint8* data, int len);
static Std_ReturnType M_u8CmdWriteDaq(uint8 pid, uint8* data, int len);
static Std_ReturnType M_u8CmdExchangeID(uint8 pid, uint8* data, int len);
static Std_ReturnType M_u8CmdGetCcpVersion(uint8 pid, uint8* data, int len);
static Std_ReturnType M_u8CmdDnload6(uint8 pid, uint8* data, int len);
static Std_ReturnType M_u8CmdBuildChecksum(uint8 pid, uint8* data, int len);
static uint32 M_u32CcpGetSeed(uint8 u8ResourceMask);
static uint32 M_u32GetRandomValue(void);
static uint8 M_u8CcpUnlock(uint32 u32CcpKey);
static void M_CcpStopDaq(uint8 u8Daq);
static void M_CcpStopAllDaq(void);
static void M_CcpStartDaq(uint8 u8Daq); 
static void M_CcpPrepareDaq(uint8 u8Daq, uint8 u8Last, uint8 u8EventChannel, uint16 u16Prescaler );
static uint8 M_CcpClearDaqList(uint8 u8Daq);
static uint8 *M_CcpGetDaqPointer(uint8 u8Addr_ext, uint32 u32Addr);
static void M_CcpSetCalPage(uint32 u32CalAddress);
static uint32 M_CcpGetCalPage(void);
static uint8 M_CcpWriteMTA(uint8 u8MtaIndex,uint8 u8Size,uint8 *pu8Data);
static void M_CcpClearRxBuffer(void);
static void M_CcpSampleAndTransmitDTM(uint8 u8Pid, uint8 u8DaqNum, uint8 u8OdtNum); 
static void M_CcpDAQProcess(uint8 u8EventChannel);
static BOOL_T M_CcpUserAccess_Manage(uint32 u32CalAddress);

static void Ccp_Recieve_Main(void);
static void Ccp_Transmit_Main(void);
static void Ccp_RxIndication(uint8 *u8Data, uint8 u8Length);
Std_ReturnType Ccp_Transmit(const uint8 *data, int len);

extern void CCP_Init(void);
extern void CCP_DAQTimingProcess(void);
extern void CCP_MainFunction(void);
extern void CCP_CanRxIndication(const PduIdType CcpRxPduId,const PduInfoType *CcpRxPduPtr);
extern void CCP_DAQTransmit(const uint8 *data, int len);
extern void CCP_DAQMainFunction(void);
/*************************************************************************
Evolution of the component

$Log: Ccp.h  $
Revision 1.0 2016/4/25 MingDong Yuan 
First creation

*************************************************************************/

/* end of file */

