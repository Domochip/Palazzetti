#ifndef Palazzetti_h
#define Palazzetti_h

#include <Arduino.h>

class Palazzetti
{
    uint16_t wAddrFeederActiveTime = 0;
    uint32_t fumisComStatus = 0; //sFumisComData.4
    uint16_t dword_432618 = 0; //sFumisComData.8  Unused variable (always set but never get : DEBUG?)

    uint16_t serialPortModel = 2; //myData.32  depend of board name : 1=omni-emb; 2=others

    //myData.4176 = verbose level
    uint16_t comPortNumber = 0; //myData.4180  COM Port Number

    int _MBTYPE = 0; //myData.4200  0 for Fumis; 100(0x64) for Micronova
    byte _HWTYPE = 0; //myData.4208 
    uint16_t dword_470F0C = 0; //myData.4204  MBTYPE (Micronova)

    unsigned long selectSerialTimeoutms = 3000;

    char _SN[28]; //myData.4224

    // char _LABEL[32]; //myData.4280  Not Used

    float _T1 = 0; //myData.4312
    float _T2 = 0; //myData.4316
    float _T3 = 0; //myData.4320
    float _T4 = 0; //myData.4324
    float _T5 = 0; //myData.4328
    byte _IN_I01 = 0;   //myData.4332
    byte _IN_I02 = 0;   //myData.4333
    byte _IN_I03 = 0;   //myData.4334
    byte _IN_I04 = 0;   //myData.4335
    byte _OUT_O01 = 0;   //myData.4336
    byte _OUT_O02 = 0;   //myData.4337
    byte _OUT_O03 = 0;   //myData.4338
    byte _OUT_O04 = 0;   //myData.4339
    byte _OUT_O05 = 0;   //myData.4340
    byte _OUT_O06 = 0;   //myData.4341
    byte _OUT_O07 = 0;   //myData.4342

    uint16_t _F1V = 0; //myData.4344
    uint16_t _F2V = 0; //myData.4348
    uint16_t _F1RPM = 0; //myData.4352
    uint16_t _F2L = 0; //myData.4356 (transcode needed)
    uint16_t _F3L = 0; //myData.4360
    uint16_t _F4L = 0; //myData.4364

    byte _PWR = 0; //myData.4368

    float _FDR = 0; //myData.4372  (FeederActiveTime)

    byte _PUMP = 0; //myData.4376
    uint16_t _DP_TARGET = 0; //myData.4380
    uint16_t _DP_PRESS = 0; //myData.4384

    uint16_t _STATUS = 0; //myData.4388
    uint16_t _LSTATUS = 0; //myData.4392
    uint16_t _MFSTATUS = 0; //myData.4396

    float _SETP = 0;    //myData.4416  aka SetPoint
    char _STOVE_DATETIME[20]; //myData.4428
    uint16_t _STOVE_WDAY = 0; //myData.4448
    byte _CHRSTATUS = 0; //myData.4452
    uint16_t _PQT = 0; //myData.4460

    //dword_47101C //myData.4476  contains pointer from malloc(0xD0) used to store ChronoData

    //space of 0x16 size reserved by malloc in iInit
    uint16_t _IGN = 0; //myData.4480[0]
    uint16_t _POWERTIMEH = 0; //myData.4480[1]
    uint16_t _POWERTIMEM = 0; //myData.4480[2]
    uint16_t _HEATTIMEH = 0; //myData.4480[3]
    uint16_t _HEATTIMEM = 0; //myData.4480[4]
    uint16_t _SERVICETIMEM = 0; //myData.4480[5]
    uint16_t _SERVICETIMEH = 0; //myData.4480[6]
    uint16_t _ONTIMEM = 0; //myData.4480[7]
    uint16_t _ONTIMEH = 0; //myData.4480[8]
    uint16_t _OVERTMPERRORS = 0; //myData.4480[9]
    uint16_t _IGNERRORS = 0; //myData.4480[10]

    //dword_471024 //myData.4484 contains pointer from malloc(0x69) used to store Logs

    byte staticDataLoaded = 0; //myData.4488  (psStaticData) : indicates that Static Data are loaded

    uint16_t _FWDATEY = 0; //myData.4492
    uint16_t _FWDATEM = 0; //myData.4496
    uint16_t _FWDATED = 0; //myData.4500

    uint16_t _MOD = 0; //myData.4504
    uint16_t _VER = 0; //myData.4508
    uint16_t _CORE = 0; //myData.4512
    // uint16_t pdword_471044 = 0; //myData.4516  Unused variable (always set but never get)
    // uint16_t pdword_471048 = 0; //myData.4520  Unused variable (always set but never get)
    // uint16_t pdword_47104C = 0; //myData.4524  Unused variable (always set but never get)
    // uint16_t pdword_471050 = 0; //myData.4528  Unused variable (always set but never get)
    
    uint16_t _DSPFWVER = 0; //myData.4532

    //myData.4536 contains pointer to malloc(0x6A) (setted up in iInit)
    byte _PARAMS[0x6A]; //myData.4536
    // byte _PARAMS[0x4C] = 0; //CONFIG
    // byte _PARAMS[0x5C] = 0; //PELLETTYPE
    // byte _PARAMS[0x62] = 0; //PSENSLMAX (Pellet Level max)
    // byte _PARAMS[0x63] = 0; //PSENSLTSH (Pellet Level threshold)
    // byte _PARAMS[0x64] = 0; //PSENSLMIN (Pellet Level min)

    byte paramsBufferSize = 0x6A; //myData.4540  setted up in iInit for sizing PARAMS, splMaxBuffer and splMinBuffer malloc

    //myData.4544 contains pointer to malloc(0xDE) (setted up in iInit by 0x6F * 2)
    uint16_t _HPARAMS[0x6F]; //myData.4544

    byte hparamsBufferSize = 0x6F; //myData.4548  setted up in iInit for sizing HPARAMS malloc

    uint16_t _FLUID = 0; //myData.4552

    byte splMaxBuffer[0x6A]; //mydata.4556
    // byte splMaxBuffer[0x33] or splMaxBuffer[0x33] contains SPLMAX

    byte splMinBuffer[0x6A]; //mydata.4560
    // byte splMinBuffer[0x33] or splMinBuffer[0x33] contains SPLMIN

    byte _SPLMIN = 0; //myData.4564  (SetPointLimitMin)
    byte _SPLMAX = 0;    //myData.4568  (SetPointLimitMax)
    uint16_t _PSENSTYPE = 0; //myData.4572  (if 1 then Pellet level sensor)

    byte _UICONFIG = 0; //myData.4576
    byte _BLEMBMODE = 0; //myData.4577
    byte _BLEDSPMODE = 0; //myData.4578
    byte _MAINTPROBE = 0; //myData.4579
    byte _STOVETYPE = 0; //myData.4580
    byte _FAN2TYPE = 0; //myData.4581
    byte _FAN2MODE = 0; //myData.4582
    // byte byte_471087 = 0; //myData.4583  Unused variable (always set but never get)
    byte _AUTONOMYTYPE = 0; //myData.4584
    byte _NOMINALPWR = 0; //myData.4585
    byte byte_47108A = 0; //myData.4586  Fan related value used for calculation...
    byte _FAN1LMIN = 0; //myData.4587  FANLMINMAX[0]
    byte _FAN1LMAX = 0; //myData.4588  FANLMINMAX[1]
    byte _FAN2LMIN = 0; //myData.4589  FANLMINMAX[2]
    byte _FAN2LMAX = 0; //myData.4590  FANLMINMAX[3]
    byte _FAN3LMIN = 0; //myData.4591  FANLMINMAX[4]
    byte _FAN3LMAX = 0; //myData.4592  FANLMINMAX[5]

    // char byte_471091[19]; //myData.4593  Mac address setted up by iUpdateStaticData

#define OPENSERIAL_SIGNATURE std::function<int(uint32_t baudrate)>
#define CLOSESERIAL_SIGNATURE std::function<void()>
#define SELECTSERIAL_SIGNATURE std::function<int(unsigned long timeout)>
#define READSERIAL_SIGNATURE std::function<size_t(void *buf, size_t count)>
#define WRITESERIAL_SIGNATURE std::function<size_t(const void *buf, size_t count)>
#define DRAINSERIAL_SIGNATURE std::function<int()>
#define FLUSHSERIAL_SIGNATURE std::function<int()>
#define USLEEP_SIGNATURE std::function<void(unsigned long usec)>

    //Open a Serial
    //Upon successful completion, 0 shall be returned. Otherwise, -1 shall be returned
    OPENSERIAL_SIGNATURE m_openSerial = nullptr;
    //Close Serial
    CLOSESERIAL_SIGNATURE m_closeSerial = nullptr;
    //Indicates that some data are available to read
    //Shall return 1 if some data are available; 0 if no data are available. otherwise -1 for error
    SELECTSERIAL_SIGNATURE m_selectSerial = nullptr;
    //Read from Serial
    //Upon successful completion, shall return a non-negative integer indicating the number of bytes actually read. Otherwise, the functions shall return -1
    READSERIAL_SIGNATURE m_readSerial = nullptr;
    //Write to Serial
    //Upon successful completion, shall return the number of bytes actually written. Otherwise, -1 shall be returned
    WRITESERIAL_SIGNATURE m_writeSerial = nullptr;
    //Wait for transmission of output
    //Upon successful completion, 0 shall be returned. Otherwise, -1 shall be returned
    DRAINSERIAL_SIGNATURE m_drainSerial = nullptr;
    //Flush both non-transmitted output data and non-read input data
    //Upon successful completion, 0 shall be returned. Otherwise, -1 shall be returned
    FLUSHSERIAL_SIGNATURE m_flushSerial = nullptr;
    //Suspend execution for an interval (useconds)
    //Upon successful completion, 0 shall be returned. Otherwise, -1 shall be returned
    USLEEP_SIGNATURE m_uSleep = nullptr;

    int SERIALCOM_OpenComport(uint32_t baudrate);
    void SERIALCOM_CloseComport();
    int SERIALCOM_Flush();
    int SERIALCOM_ReceiveBuf(void *buf, size_t count);
    int SERIALCOM_SendBuf(void *buf, size_t count);
    int SERIALCOM_ReceiveByte(byte *buf);
    void SERIALCOM_SendByte(byte *buf);
    int iChkSum(byte *datasToCheck);
    int parseRxBuffer(byte *rxBuffer);
    int fumisCloseSerial();
    int fumisOpenSerial();
    int fumisSendRequest(void *buf);
    int fumisWaitRequest(void *buf);
    int fumisComReadBuff(uint16_t addrToRead, void *buf, size_t count);
    int fumisComRead(uint16_t addrToRead, uint16_t *data, int wordMode);
    int fumisComReadByte(uint16_t addrToRead, uint16_t *data);
    int fumisComReadWord(uint16_t addrToRead, uint16_t *data);
    int fumisComWrite(uint16_t addrToWrite, uint16_t data, int wordMode);
    int fumisComWriteByte(uint16_t addrToWrite, uint16_t data);
    int fumisComWriteWord(uint16_t addrToWrite, uint16_t data);
    int iGetStatusAtech();
    int iChkMBType();
    int iInit();
    int isValidSerialNumber(char *SN);
    int iGetSNAtech();
    int iGetMBTypeAtech();
    int iGetStoveConfigurationAtech();
    int iUpdateStaticDataAtech();
    int iUpdateStaticData();
    int iCloseUART();
    int iGetSetPointAtech();
    int iSetSetPointAtech(uint16_t setPoint);
    int iSetSetPointAtech(float setPoint);
    int iReadTemperatureAtech();
    int iSwitchOnAtech();
    int iSwitchOffAtech();
    int iGetPelletQtUsedAtech();
    int iGetRoomFanAtech();
    int iReadFansAtech();
    int iGetPowerAtech();
    int iSetPowerAtech(uint16_t powerLevel);
    void iGetFanLimits();
    uint16_t transcodeRoomFanSpeed(uint16_t roomFanSpeed, bool decode);
    int iSetRoomFanAtech(uint16_t roomFanSpeed);
    int iSetRoomFan3Atech(uint16_t roomFan3Speed);
    int iSetRoomFan4Atech(uint16_t roomFan4Speed);
    int iSetSilentModeAtech(uint16_t silentMode);
    int iGetCounters();
    int iGetDPressDataAtech();
    int iGetDateTimeAtech();
    int iReadIOAtech();
    int iGetPumpRateAtech();
    int iGetChronoDataAtech();
    int iGetAllStatus(bool refreshStatus);
    int iGetParameterAtech(uint16_t paramToRead, uint16_t *paramValue);
    int iSetParameterAtech(byte paramToWrite, byte paramValue);
    int iGetHiddenParameterAtech(uint16_t hParamToRead, uint16_t *hParamValue);
    int iSetHiddenParameterAtech(uint16_t hParamToWrite, uint16_t hParamValue);

    bool _isInitialized;

public:
    bool initialize();
    bool initialize(OPENSERIAL_SIGNATURE openSerial, CLOSESERIAL_SIGNATURE closeSerial, SELECTSERIAL_SIGNATURE selectSerial, READSERIAL_SIGNATURE readSerial, WRITESERIAL_SIGNATURE writeSerial, DRAINSERIAL_SIGNATURE drainSerial, FLUSHSERIAL_SIGNATURE flushSerial, USLEEP_SIGNATURE uSleep);
    bool isInitialized() { return _isInitialized; };
    bool getStaticData(char (*SN)[28], byte *SNCHK, int *MBTYPE, uint16_t *MOD, uint16_t *VER, uint16_t *CORE, char (*FWDATE)[11], uint16_t *FLUID, uint16_t *SPLMIN, uint16_t *SPLMAX, byte *UICONFIG, byte *HWTYPE, uint16_t *DSPFWVER, byte *CONFIG, byte *PELLETTYPE, uint16_t *PSENSTYPE, byte *PSENSLMAX, byte *PSENSLTSH, byte *PSENSLMIN, byte *MAINTPROBE, byte *STOVETYPE, byte *FAN2TYPE, byte *FAN2MODE, byte *CHRONOTYPE, byte *AUTONOMYTYPE, byte *NOMINALPWR);
    bool getAllStatus(bool refreshStatus, int *MBTYPE, uint16_t *MOD, uint16_t *VER, uint16_t *CORE, char (*FWDATE)[11], char (*APLTS)[20], uint16_t *APLWDAY, byte *CHRSTATUS, uint16_t *STATUS, uint16_t *LSTATUS, bool *isMFSTATUSValid, uint16_t *MFSTATUS, float *SETP, byte *PUMP, uint16_t *PQT, uint16_t *F1V, uint16_t *F1RPM, uint16_t *F2L, uint16_t *F2LF, uint16_t (*FANLMINMAX)[6], uint16_t *F2V, bool *isF3LF4LValid, uint16_t *F3L, uint16_t *F4L, byte *PWR, float *FDR, uint16_t *DPT, uint16_t *DP, byte *IN, byte *OUT, float *T1, float *T2, float *T3, float *T4, float *T5, bool *isSNValid, char (*SN)[28]);
    bool getSN(char (*SN)[28]);
    bool getSetPoint(float *setPoint);
    bool setSetpoint(byte setPoint, float *SETPReturn = nullptr);
    bool getAllTemps(float *T1, float *T2, float *T3, float *T4, float *T5);
    bool getStatus(uint16_t *STATUS, uint16_t *LSTATUS);
    bool getPelletQtUsed(uint16_t *PQT);
    bool getFanData(uint16_t *F1V, uint16_t *F2V, uint16_t *F1RPM, uint16_t *F2L, uint16_t *F2LF, bool *isF3LF4LValid, uint16_t *F3L, uint16_t *F4L);
    bool getPower(byte *PWR, float *FDR);
    bool setPower(byte powerLevel, byte *PWRReturn = nullptr, bool *isF2LReturnValid = nullptr, uint16_t *F2LReturn = nullptr, uint16_t (*FANLMINMAXReturn)[6] = nullptr);
    bool setRoomFan(byte roomFanSpeed, bool *isPWRReturnValid = nullptr, byte *PWRReturn = nullptr, uint16_t *F2LReturn = nullptr, uint16_t *F2LFReturn = nullptr);
    bool setRoomFan3(byte roomFan3Speed, uint16_t *F3LReturn = nullptr);
    bool setRoomFan4(byte roomFan4Speed, uint16_t *F4LReturn = nullptr);
    bool setSilentMode(byte silentMode, byte *SLNTReturn = nullptr, byte *PWRReturn = nullptr, uint16_t *F2LReturn = nullptr, uint16_t *F2LFReturn = nullptr, bool *isF3LF4LReturnValid = nullptr, uint16_t *F3LReturn = nullptr, uint16_t *F4LReturn = nullptr);
    bool getCounters(uint16_t *IGN, uint16_t *POWERTIMEh, uint16_t *POWERTIMEm, uint16_t *HEATTIMEh, uint16_t *HEATTIMEm, uint16_t *SERVICETIMEh, uint16_t *SERVICETIMEm, uint16_t *ONTIMEh, uint16_t *ONTIMEm, uint16_t *OVERTMPERRORS, uint16_t *IGNERRORS, uint16_t *PQT);
    bool getDPressData(uint16_t *DP_TARGET, uint16_t *DP_PRESS);
    bool getDateTime(char (*STOVE_DATETIME)[20],byte *STOVE_WDAY);
    bool getIO(byte *IN_I01, byte *IN_I02, byte *IN_I03, byte *IN_I04, byte *OUT_O01, byte *OUT_O02, byte *OUT_O03, byte *OUT_O04, byte *OUT_O05, byte *OUT_O06, byte *OUT_O07);
    bool getParameter(byte paramNumber, byte *paramValue);
    bool setParameter(byte paramNumber, byte paramValue);
    bool getHiddenParameter(byte hParamNumber, uint16_t *hParamValue);
    bool setHiddenParameter(byte hParamNumber, uint16_t hParamValue);
    bool getAllParameters(byte (*params)[0x6A]);
    bool getAllHiddenParameters(uint16_t (*hiddenParams)[0x6F]);
    bool powerOff();
    bool powerOn();
    Palazzetti();
};

#endif