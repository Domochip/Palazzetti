#ifndef Palazzetti_h
#define Palazzetti_h

#include <Arduino.h>

class Palazzetti
{
    uint16_t wAddrFeederActiveTime = 0; //dword_42F1F0
    uint32_t dword_42F214 = 0;
    int dword_42F218 = 0;

    uint16_t dword_46CAC0 = 2; //depend of board name : 1=omni-emb; 2=others

    //dword_46DAF0 = verbose mode
    uint16_t dword_46DAF4 = 0; //COM Port Number

    int dword_46DB08 = 0;      //MBTYPE
    uint16_t dword_46DB10 = 0; //HWTYPE
    uint16_t dword_46DB0C = 0; //MBTYPE (Micronova)

    //dword_46DB14 and dword_46DB18 as struct timeval
    unsigned long selectSerialTimeoutms = 3000;

    char byte_46DB1C[28]; //SN

    char byte_46DB54[32]; //LABEL

    float dword_46DB74 = 0; //T1
    float dword_46DB78 = 0; //T2
    float dword_46DB7C = 0; //T3
    float dword_46DB80 = 0; //T4
    float dword_46DB84 = 0; //T5
    byte byte_46DB88 = 0;   //IN_I01
    byte byte_46DB89 = 0;   //IN_I02
    byte byte_46DB8A = 0;   //IN_I03
    byte byte_46DB8B = 0;   //IN_I04
    byte byte_46DB8C = 0;   //OUT_O01
    byte byte_46DB8D = 0;   //OUT_O02
    byte byte_46DB8E = 0;   //OUT_O03
    byte byte_46DB8F = 0;   //OUT_O04
    byte byte_46DB90 = 0;   //OUT_O05
    byte byte_46DB91 = 0;   //OUT_O06
    byte byte_46DB92 = 0;   //OUT_O07

    uint16_t dword_46DB94 = 0; //F1V
    uint16_t dword_46DB98 = 0; //F2V
    uint16_t dword_46DB9C = 0; //F1RPM
    uint16_t dword_46DBA0 = 0; //F2L (transcode needed)
    uint16_t dword_46DBA4 = 0; //F3L
    uint16_t dword_46DBA8 = 0; //F4L

    byte byte_46DBAC = 0; //PWR
    //byte byte_46DBAD = 0; //RPWR (Micronova)

    float dword_46DBB0 = 0; //FDR (FeederActiveTime)

    uint16_t dword_46DBB8 = 0; //DP_TARGET
    uint16_t dword_46DBBC = 0; //DP_PRESS

    uint16_t dword_46DBC0 = 0; //STATUS
    uint16_t dword_46DBC4 = 0; //LSTATUS
    uint16_t dword_46DBC8 = 0; //MFSTATUS

    float dword_46DBDC = 0;    //SETP aka SetPoint
    char byte_46DBE0[20];      //STOVE_DATETIME
    uint16_t dword_46DBF4 = 0; //STOVE_WDAY

    uint16_t dword_46DBFC = 0; //PQT

    //dword_46DC04 contains pointer to malloc(0xD0)

    //space of 0x16 size reserved by malloc in iInit
    uint16_t pdword_46DC08_00 = 0; //IGN
    uint16_t pdword_46DC08_02 = 0; //POWERTIME(hour)
    uint16_t pdword_46DC08_04 = 0; //POWERTIME(minute)
    uint16_t pdword_46DC08_06 = 0; //HEATTIME(hour)
    uint16_t pdword_46DC08_08 = 0; //HEATTIME(minute)
    uint16_t pdword_46DC08_0A = 0; //SERVICETIME(minute)
    uint16_t pdword_46DC08_0C = 0; //SERVICETIME(hour)
    uint16_t pdword_46DC08_0E = 0; //ONTIME(minute)
    uint16_t pdword_46DC08_10 = 0; //ONTIME(hour)
    uint16_t pdword_46DC08_12 = 0; //OVERTMPERRORS
    uint16_t pdword_46DC08_14 = 0; //IGNERRORS

    //space of 0x69 size reserved by malloc in iInit
    //dword_46DC0C contains pointer to malloc(0x69)

    uint16_t pdword_46DC14 = 0; //FWDATE(day)
    uint16_t pdword_46DC18 = 0; //FWDATE(month)
    uint16_t pdword_46DC1C = 0; //FWDATE(year)

    uint16_t pdword_46DC20 = 0; //MOD
    uint16_t pdword_46DC24 = 0; //VER
    uint16_t pdword_46DC28 = 0; //CORE
    uint16_t pdword_46DC2C = 0;
    uint16_t pdword_46DC30 = 0;
    uint16_t pdword_46DC34 = 0;
    uint16_t _DSPFWVER = 0;

    //dword_46DC38 contains pointer to malloc(0x6A) (setted up in iInit)
    byte pdword_46DC38[0x6A]; //PARAMS
    // byte pdword_46DC38_4C = 0; //CONFIG
    // byte pdword_46DC38_5C = 0; //PELLETTYPE
    // byte pdword_46DC38_62 = 0; //PSENSLMAX (Pellet Level max)
    // byte pdword_46DC38_63 = 0; //PSENSLTSH (Pellet Level threshold)
    // byte pdword_46DC38_64 = 0; //PSENSLMIN (Pellet Level min)

    byte byte_46DC3C = 0x6A; //setted up in iInit for sizing malloc

    //dword_46DC40 contains pointer to malloc(0xDE) (setted up in iInit by 0x6F*2)
    uint16_t pdword_46DC40[0x6F]; //HPARAMS

    byte byte_46DC44 = 0x6F; //setted up in iInit for sizing malloc

    uint16_t dword_46DC48 = 0; //FLUID

    //dword_46DC4C contains pointer to malloc(0x6A) (setted up in iInit)
    byte pdword_46DC4C[0x6A];

    //dword_46DC50 contains pointer to malloc(0x6A) (setted up in iInit)
    byte pdword_46DC50[0x6A];

    byte pdword_46DC54 = 0;    //SPLMIN (SetPointLimitMin)
    byte pdword_46DC58 = 0;    //SPLMAX (SetPointLimitMax)
    uint16_t dword_46DC5C = 0; //PSENSTYPE (if 1 then Pellet level sensor)

    byte byte_46DC60 = 0; //UICONFIG
    byte byte_46DC61 = 0; //MAINTPROBE
    byte byte_46DC62 = 0; //STOVETYPE
    byte byte_46DC63 = 0; //FAN2TYPE
    byte byte_46DC64 = 0; //FAN2MOD
    byte byte_46DC65 = 0;
    byte byte_46DC66 = 0; //AUTONOMYTYPE
    byte byte_46DC67 = 0; //NOMINALPWR
    byte byte_46DC68 = 0;
    byte byte_46DC69 = 0; //FANLMINMAX[0]
    byte byte_46DC6A = 0; //FANLMINMAX[1]
    byte byte_46DC6B = 0; //FANLMINMAX[2]
    byte byte_46DC6C = 0; //FANLMINMAX[3]
    byte byte_46DC6D = 0; //FANLMINMAX[4]
    byte byte_46DC6E = 0; //FANLMINMAX[5]

    char byte_46DC6F[19]; //Mac address setted up by iUpdateStaticData

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
    int iGetAllStatus();
    int iGetParameterAtech(uint16_t paramToRead, uint16_t *paramValue);
    int iSetParameterAtech(byte paramToWrite, byte paramValue);
    int iGetHiddenParameterAtech(uint16_t hParamToRead, uint16_t *hParamValue);
    int iSetHiddenParameterAtech(uint16_t hParamToWrite, uint16_t hParamValue);

    bool _isInitialized;

public:
    bool initialize();
    bool initialize(OPENSERIAL_SIGNATURE openSerial, CLOSESERIAL_SIGNATURE closeSerial, SELECTSERIAL_SIGNATURE selectSerial, READSERIAL_SIGNATURE readSerial, WRITESERIAL_SIGNATURE writeSerial, DRAINSERIAL_SIGNATURE drainSerial, FLUSHSERIAL_SIGNATURE flushSerial, USLEEP_SIGNATURE uSleep);
    bool isInitialized() { return _isInitialized; };
    bool getStaticData(int *MBTYPE, uint16_t *MOD, uint16_t *VER, uint16_t *CORE, char (&FWDATE)[11], uint16_t *FLUID, uint16_t *SPLMIN, uint16_t *SPLMAX, byte *UICONFIG, uint16_t *HWTYPE, uint16_t *DSPFWVER, byte *CONFIG, byte *PELLETTYPE, uint16_t *PSENSTYPE, byte *PSENSLMAX, byte *PSENSLTSH, byte *PSENSLMIN, byte *MAINTPROBE, byte *STOVETYPE, byte *FAN2TYPE, byte *FAN2MOD, byte *CHRONOTYPE, byte *AUTONOMYTYPE, byte *NOMINALPWR);
    bool getAllStatus(int *MBTYPE, uint16_t *MOD, uint16_t *VER, uint16_t *CORE, char (&FWDATE)[11], char (&APLTS)[20], uint16_t *APLWDAY, uint16_t *STATUS, uint16_t *LSTATUS, bool *isMFSTATUSValid, uint16_t *MFSTATUS, float *SETP, uint16_t *PQT, uint16_t *F1V, uint16_t *F1RPM, uint16_t *F2L, uint16_t *F2LF, uint16_t (&FANLMINMAX)[6], uint16_t *F2V, bool *isF3LF4LValid, uint16_t *F3L, uint16_t *F4L, byte *PWR, float *FDR, uint16_t *DPT, uint16_t *DP, byte *IN, byte *OUT, float *T1, float *T2, float *T3, float *T4, float *T5);
    bool getSetPoint(float *setPoint);
    bool setSetpoint(byte setPoint);
    bool getAllTemps(float *T1, float *T2, float *T3, float *T4, float *T5);
    bool getStatus(uint16_t *STATUS, uint16_t *LSTATUS);
    bool getPelletQtUsed(uint16_t *PQT);
    bool getFanData(uint16_t *F1V, uint16_t *F2V, uint16_t *F1RPM, uint16_t *F2L, uint16_t *F2LF, bool *isF3LF4LValid, uint16_t *F3L, uint16_t *F4L);
    bool getPower(byte *PWR, float *FDR);
    bool setPower(byte powerLevel);
    bool setRoomFan(byte roomFanSpeed);
    bool setRoomFan3(byte roomFan3Speed);
    bool setRoomFan4(byte roomFan4Speed);
    bool setSilentMode(byte silentMode);
    bool getCounters(uint16_t *IGN, uint16_t *POWERTIMEh, uint16_t *POWERTIMEm, uint16_t *HEATTIMEh, uint16_t *HEATTIMEm, uint16_t *SERVICETIMEh, uint16_t *SERVICETIMEm, uint16_t *ONTIMEh, uint16_t *ONTIMEm, uint16_t *OVERTMPERRORS, uint16_t *IGNERRORS, uint16_t *PQT);
    bool getDPressData(uint16_t *DP_TARGET, uint16_t *DP_PRESS);
    bool getDateTime(char (&STOVE_DATETIME)[20],byte *STOVE_WDAY);
    bool getParameter(byte paramNumber, byte *paramValue);
    bool setParameter(byte paramNumber, byte paramValue);
    bool getHiddenParameter(byte hParamNumber, uint16_t *hParamValue);
    bool setHiddenParameter(byte hParamNumber, uint16_t hParamValue);
    bool getAllParameters(byte (&params)[0x6A]);
    bool getAllHiddenParameters(uint16_t (&hiddenParams)[0x6F]);
    bool powerOff();
    bool powerOn();
    Palazzetti();
};

#endif