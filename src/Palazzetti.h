#ifndef Palazzetti_h
#define Palazzetti_h

#include <Arduino.h>

// Based on version 2.2.1 2022-10-24 11:13:21

class Palazzetti
{
public:
    enum class CommandResult : byte
    {
        COMMUNICATION_ERROR, // default value for negative result if it doesn't match following ones
        BUSY,                // = -20
        UNSUPPORTED,         // = -10
        PARSER_ERROR,        // = -3
        ERROR,               // = -1 and -5
        OK                   // = 0
    };

private:
    uint16_t wAddrFeederActiveTime = 0;
    uint32_t fumisComStatus = 0;                    // sFumisComData.4
    CommandResult dword_433248 = CommandResult::OK; // sFumisComData.8 (DEBUG?/Last Com Result?)

    uint16_t serialPortModel = 2; // myData.32  depend of board name : 1=omni-emb; 2=others
    // time_t lastGetAllStatusTime;  // myData.36  keep track of last iGetAllStatus
    // // msgbuf structure used by msgrcv function
    // struct msgbufR
    // {
    //     long mtype;       // myData.40
    //     char mtext[2048]; // myData.44
    // } msgbufReceive;
    // // msgbuf structure used by msgsnd function
    // struct msgbufS
    // {
    //     long mtype;       // myData.2092
    //     char mtext[2048]; // myData.2096
    // } msgbufSend;
    // char path[31] = "/tmp";     // myData.4144  file path to generate msgkey
    // uint16_t verboseLevel = 0;  // myData.4176
    uint16_t comPortNumber = 0; // myData.4180  COM Port path selector (0=>myData.4184; 1=>myData.4188; etc.)
    // myData.4184  pointer to string "/dev/ttyS0"
    // myData.4188  pointer to string "/dev/ttyUSB0"
    // myData.4192  pointer to string "/dev/ttyUSB1"
    // myData.4196  pointer to string "/dev/ttyACM0"
    int _MBTYPE = 0; // myData.4200  0 for Fumis; 100(0x64) for Micronova
    // uint16_t _MBTYPEMicronova = 0; // myData.4204  MBTYPE Micronova Only
    byte _HWTYPE = 0; // myData.4208
    // myData.4212
    unsigned long selectSerialTimeoutMs = 2300; // myData.4216 + myData.4220 (timeval type originially)
    char _SN[28] = {0};                         // myData.4224

    // char _LABEL[32]; //myData.4280  Not Used

    float _T1 = 0;     // myData.4312
    float _T2 = 0;     // myData.4316
    float _T3 = 0;     // myData.4320
    float _T4 = 0;     // myData.4324
    float _T5 = 0;     // myData.4328
    byte _IN_I01 = 0;  // myData.4332
    byte _IN_I02 = 0;  // myData.4333
    byte _IN_I03 = 0;  // myData.4334
    byte _IN_I04 = 0;  // myData.4335
    byte _OUT_O01 = 0; // myData.4336
    byte _OUT_O02 = 0; // myData.4337
    byte _OUT_O03 = 0; // myData.4338
    byte _OUT_O04 = 0; // myData.4339
    byte _OUT_O05 = 0; // myData.4340
    byte _OUT_O06 = 0; // myData.4341
    byte _OUT_O07 = 0; // myData.4342

    uint16_t _F1V = 0;   // myData.4344
    uint16_t _F2V = 0;   // myData.4348
    uint16_t _F1RPM = 0; // myData.4352
    uint16_t _F2L = 0;   // myData.4356 (transcode needed)
    uint16_t _F3L = 0;   // myData.4360
    uint16_t _F4L = 0;   // myData.4364

    byte _BECO = 0; // myData.4368

    byte _PWR = 0; // myData.4369
    // byte _RPWR = 0; // myData.4370 (Micronova Only)

    float _FDR = 0; // myData.4372

    byte _PUMP = 0;          // myData.4376
    uint16_t _DP_TARGET = 0; // myData.4380
    uint16_t _DP_PRESS = 0;  // myData.4384

    uint16_t _STATUS = 0;   // myData.4388
    uint16_t _FSTATUS = 0;  // myData.4392
    uint16_t _LSTATUS = 0;  // myData.4396
    uint16_t _MFSTATUS = 0; // myData.4400

    float _SETP = 0;          // myData.4420  aka SetPoint
    float _F3S = 0;           // myData.4424
    float _F4S = 0;           // myData.4428
    float _SECO = 0;          // myData.4432
    char _STOVE_DATETIME[25]; // myData.4436 //increased to 25 instead of 20 to avoid compilation warning
    uint16_t _STOVE_WDAY = 0; // myData.4456
    byte _CHRSTATUS = 0;      // myData.4460
    uint16_t _EFLAGS = 0;     // myData.4464
    uint16_t _PQT = 0;        // myData.4468
    uint16_t _PLEVEL = 0;     // myData.4472
    uint16_t _PSENSCSTA = 0;  // myData.4476
    uint16_t _PSENSLEMP = 0;  // myData.4480

    // myData.4484  contains pointer from malloc(0xD0) used to store ChronoData
    // 0->5 : P1->P6
    struct chronoDataProgram
    {
        float CHRSETP; // chronoData[0->5 * 0x14]
        byte STARTH;   // chronoData[0->5 * 0x14 + 4]
        byte STARTM;   // chronoData[0->5 * 0x14 + 8]
        byte STOPH;    // chronoData[0->5 * 0x14 + 0x0C]
        byte STOPM;    // chronoData[0->5 * 0x14 + 0x10]
    };
    chronoDataProgram chronoDataPrograms[6];
    // 0->6 : D1->D7
    struct chronoDataDay
    {
        byte M1; // chronoData[0x78 + (0->6 * 3 + 0) * 4]
        byte M2; // chronoData[0x78 + (0->6 * 3 + 1) * 4]
        byte M3; // chronoData[0x78 + (0->6 * 3 + 2) * 4]
    };
    chronoDataDay chronoDataDays[7];
    uint16_t chronoDataStatus; // chronoData[0xCC]

    // space of 0x16 size reserved by malloc in iInit
    uint16_t _IGN = 0;           // myData.4488[0]
    uint16_t _POWERTIMEH = 0;    // myData.4488[1]
    uint16_t _POWERTIMEM = 0;    // myData.4488[2]
    uint16_t _HEATTIMEH = 0;     // myData.4488[3]
    uint16_t _HEATTIMEM = 0;     // myData.4488[4]
    uint16_t _SERVICETIMEM = 0;  // myData.4488[5]
    uint16_t _SERVICETIMEH = 0;  // myData.4488[6]
    uint16_t _ONTIMEM = 0;       // myData.4488[7]
    uint16_t _ONTIMEH = 0;       // myData.4488[8]
    uint16_t _OVERTMPERRORS = 0; // myData.4488[9]
    uint16_t _IGNERRORS = 0;     // myData.4488[10]

    // dword_471C5C // myData.4492 contains pointer from malloc(0x69) used to store Logs

    byte staticDataLoaded = 0; // myData.4496  (address in psStaticData) : indicates that Static Data are loaded

    uint16_t _FWDATED = 0; // myData.4500 (psStaticData[4])
    uint16_t _FWDATEM = 0; // myData.4504 (psStaticData[8])
    uint16_t _FWDATEY = 0; // myData.4508 (psStaticData[0xC])

    uint16_t _MOD = 0;  // myData.4512 (psStaticData[0x10])
    uint16_t _VER = 0;  // myData.4516 (psStaticData[0x14])
    uint16_t _CORE = 0; // myData.4520 (psStaticData[0x18])
    // uint16_t pdword_471044 = 0; // myData.4524  Unused variable (always set but never get)
    // uint16_t pdword_471048 = 0; // myData.4528  Unused variable (always set but never get)
    // uint16_t pdword_47104C = 0; // myData.4532  Unused variable (always set but never get)
    // uint16_t pdword_471050 = 0; // myData.4536  Unused variable (always set but never get)

    byte _DSPFWVER = 0; // myData.4540 (psStaticData[0x2C])
    byte _DSPTYPE = 0;  // myData.4541 (psStaticData[0x2D])

    // myData.4544 contains pointer to malloc(0x6A) (setted up in iInit)
    byte _PARAMS[0x6A]; // myData.4544 (psStaticData[0x30])
    // byte _PARAMS[0x4C] = 0; // CONFIG
    // byte _PARAMS[0x5C] = 0; // PELLETTYPE
    // byte _PARAMS[0x62] = 0; // PSENSLMAX (Pellet Level max)
    // byte _PARAMS[0x63] = 0; // PSENSLTSH (Pellet Level threshold)
    // byte _PARAMS[0x64] = 0; // PSENSLMIN (Pellet Level min)

    byte paramsBufferSize = 0x6A; // myData.4548 (psStaticData[0x34]) setted up in iInit for sizing PARAMS, _LIMMAX and _LIMMIN malloc

    // myData.4552 contains pointer to malloc(0xDE) (setted up in iInit by 0x6F * 2)
    uint16_t _HPARAMS[0x6F]; // myData.4552 (psStaticData[0x38])

    byte hparamsBufferSize = 0x6F; // myData.4556 (psStaticData[0x3C]) setted up in iInit for sizing HPARAMS malloc

    uint16_t _FLUID = 0; // myData.4560 (psStaticData[0x40])

    byte _LIMMAX[0x6A]; // mydata.4564 (psStaticData[0x44])
    // byte _LIMMAX[0x33] or _LIMMAX[0x54] contains SPLMAX

    byte _LIMMIN[0x6A]; // mydata.4568 (psStaticData[0x48])
    // byte _LIMMIN[0x33] or _LIMMIN[0x54] contains SPLMIN

    byte _SPLMIN = 0;        // myData.4572 (psStaticData[0x4C])  (SetPointLimitMin)
    byte _SPLMAX = 0;        // myData.4576 (psStaticData[0x50])  (SetPointLimitMax)
    uint16_t _PSENSTYPE = 0; // myData.4580 (psStaticData[0x54])

    byte _UICONFIG = 0;   // myData.4584 (psStaticData[0x58])
    byte _BLEMBMODE = 0;  // myData.4585 (psStaticData[0x59])
    byte _BLEDSPMODE = 0; // myData.4586 (psStaticData[0x5A])
    byte _MAINTPROBE = 0; // myData.4587 (psStaticData[0x5B])
    byte _STOVETYPE = 0;  // myData.4588 (psStaticData[0x5C])
    byte _FAN2TYPE = 0;   // myData.4589 (psStaticData[0x5D])
    byte _FAN2MODE = 0;   // myData.4590 (psStaticData[0x5E])
    // byte byte_471CBF = 0;   // myData.4591  Unused variable (always set but never get)
    byte _AUTONOMYTYPE = 0; // myData.4592 (psStaticData[0x60])
    byte _NOMINALPWR = 0;   // myData.4593 (psStaticData[0x61])
    byte byte_471CC2 = 0;   // myData.4594 (psStaticData[0x62]) Fan related value used for calculation...
    byte _FAN2LMIN = 0;     // myData.4595 (psStaticData[0x63]) FANLMINMAX[0]
    byte _FAN2LMAX = 0;     // myData.4596 (psStaticData[0x64]) FANLMINMAX[1]
    byte _FAN3LMIN = 0;     // myData.4597 (psStaticData[0x65]) FANLMINMAX[2]
    byte _FAN3LMAX = 0;     // myData.4598 (psStaticData[0x66]) FANLMINMAX[3]
    byte _FAN4LMIN = 0;     // myData.4599 (psStaticData[0x67]) FANLMINMAX[4]
    byte _FAN4LMAX = 0;     // myData.4600 (psStaticData[0x68]) FANLMINMAX[5]

    // char _MAC[19]; // myData.4601 (psStaticData[0x69])

#define OPENSERIAL_SIGNATURE std::function<int(uint32_t baudrate)>
#define CLOSESERIAL_SIGNATURE std::function<void()>
#define SELECTSERIAL_SIGNATURE std::function<int(unsigned long timeout)>
#define READSERIAL_SIGNATURE std::function<size_t(void *buf, size_t count)>
#define WRITESERIAL_SIGNATURE std::function<size_t(const void *buf, size_t count)>
#define DRAINSERIAL_SIGNATURE std::function<int()>
#define FLUSHSERIAL_SIGNATURE std::function<int()>
#define USLEEP_SIGNATURE std::function<void(unsigned long usec)>

    // Open a Serial
    // Upon successful completion, 0 shall be returned. Otherwise, -1 shall be returned
    OPENSERIAL_SIGNATURE m_openSerial = nullptr;
    // Close Serial
    CLOSESERIAL_SIGNATURE m_closeSerial = nullptr;
    // Indicates that some data are available to read
    // Shall return 1 if some data are available; 0 if no data are available. otherwise -1 for error
    SELECTSERIAL_SIGNATURE m_selectSerial = nullptr;
    // Read from Serial
    // Upon successful completion, shall return a non-negative integer indicating the number of bytes actually read. Otherwise, the functions shall return -1
    READSERIAL_SIGNATURE m_readSerial = nullptr;
    // Write to Serial
    // Upon successful completion, shall return the number of bytes actually written. Otherwise, -1 shall be returned
    WRITESERIAL_SIGNATURE m_writeSerial = nullptr;
    // Wait for transmission of output
    // Upon successful completion, 0 shall be returned. Otherwise, -1 shall be returned
    DRAINSERIAL_SIGNATURE m_drainSerial = nullptr;
    // Flush both non-transmitted output data and non-read input data
    // Upon successful completion, 0 shall be returned. Otherwise, -1 shall be returned
    FLUSHSERIAL_SIGNATURE m_flushSerial = nullptr;
    // Suspend execution for an interval (useconds)
    // Upon successful completion, 0 shall be returned. Otherwise, -1 shall be returned
    USLEEP_SIGNATURE m_uSleep = nullptr;

    void SERIALCOM_CloseComport();
    int SERIALCOM_Flush();
    int SERIALCOM_OpenComport(uint32_t baudrate);
    int SERIALCOM_ReceiveBuf(void *buf, size_t count);
    int SERIALCOM_ReceiveByte(byte *buf);
    int SERIALCOM_SendBuf(void *buf, size_t count);
    void SERIALCOM_SendByte(byte *buf);

    int iChkSum(byte *datasToCheck);
    int isValidSerialNumber(char *SN);
    int parseRxBuffer(byte *rxBuffer);
    uint16_t transcodeRoomFanSpeed(uint16_t roomFanSpeed, bool decode);

    CommandResult fumisCloseSerial();
    CommandResult fumisComRead(uint16_t addrToRead, uint16_t *data, bool wordMode);
    CommandResult fumisComReadBuff(uint16_t addrToRead, void *buf, size_t count);
    CommandResult fumisComReadByte(uint16_t addrToRead, uint16_t *data);
    CommandResult fumisComReadWord(uint16_t addrToRead, uint16_t *data);
    CommandResult fumisComSetDateTime(uint16_t year, byte month, byte day, byte hour, byte minute, byte second);
    CommandResult fumisComWrite(uint16_t addrToWrite, uint16_t data, int wordMode);
    CommandResult fumisComWriteByte(uint16_t addrToWrite, uint16_t data);
    CommandResult fumisComWriteWord(uint16_t addrToWrite, uint16_t data);
    CommandResult fumisOpenSerial();
    CommandResult fumisSendRequest(void *buf);
    CommandResult fumisWaitRequest(void *buf);

    CommandResult iChkMBType();
    CommandResult iCloseUART();
    CommandResult iGetAllStatus(bool refreshStatus);
    CommandResult iGetChronoDataAtech();
    CommandResult iGetCountersAtech();
    CommandResult iGetDateTimeAtech();
    CommandResult iGetDPressDataAtech();
    void iGetFanLimits();
    CommandResult iGetHiddenParameterAtech(uint16_t hParamToRead, uint16_t *hParamValue);
    CommandResult iGetMBTypeAtech();
    CommandResult iGetParameterAtech(uint16_t paramToRead, uint16_t *paramValue);
    CommandResult iGetPelletQtUsedAtech();
    CommandResult iGetPowerAtech();
    CommandResult iGetPumpRateAtech();
    CommandResult iGetRoomFanAtech();
    CommandResult iGetSetPointAtech();
    CommandResult iGetSNAtech();
    CommandResult iGetStatusAtech();
    CommandResult iGetStoveConfigurationAtech();
    CommandResult iInit();
    CommandResult iReadDataAtech(uint16_t addrToRead, uint16_t *data, bool wordMode);
    CommandResult iReadFansAtech();
    CommandResult iReadIOAtech();
    CommandResult iReadTemperatureAtech();
    CommandResult iSetChronoDayAtech(byte dayNumber, byte memoryNumber, byte programNumber);
    CommandResult iSetChronoPrgAtech(byte prg[6]);
    CommandResult iSetChronoSetpointAtech(byte programNumber, byte setPoint);
    CommandResult iSetChronoStartHHAtech(byte programNumber, byte startHour);
    CommandResult iSetChronoStartMMAtech(byte programNumber, byte startMinute);
    CommandResult iSetChronoStatusAtech(bool chronoStatus);
    CommandResult iSetChronoStopHHAtech(byte programNumber, byte stopHour);
    CommandResult iSetChronoStopMMAtech(byte programNumber, byte stopMinute);
    CommandResult iSetHiddenParameterAtech(uint16_t hParamToWrite, uint16_t hParamValue);
    CommandResult iSetParameterAtech(byte paramToWrite, byte paramValue);
    CommandResult iSetPowerAtech(uint16_t powerLevel);
    CommandResult iSetRoomFan3Atech(uint16_t roomFan3Speed);
    CommandResult iSetRoomFan4Atech(uint16_t roomFan4Speed);
    CommandResult iSetRoomFanAtech(uint16_t roomFanSpeed);
    CommandResult iSetSetPointAtech(byte setPoint);
    CommandResult iSetSetPointAtech(float setPoint);
    CommandResult iSetSilentModeAtech(uint16_t silentMode);
    CommandResult iSwitchOffAtech();
    CommandResult iSwitchOnAtech();
    CommandResult iUpdateStaticData();
    CommandResult iUpdateStaticDataAtech();
    CommandResult iWriteDataAtech(uint16_t addrToWrite, uint16_t data, bool wordMode);

    bool _isInitialized;

public:
    CommandResult initialize(bool loopBack = false);
    CommandResult initialize(OPENSERIAL_SIGNATURE openSerial, CLOSESERIAL_SIGNATURE closeSerial, SELECTSERIAL_SIGNATURE selectSerial, READSERIAL_SIGNATURE readSerial, WRITESERIAL_SIGNATURE writeSerial, DRAINSERIAL_SIGNATURE drainSerial, FLUSHSERIAL_SIGNATURE flushSerial, USLEEP_SIGNATURE uSleep, bool loopBack = false);
    bool isInitialized() { return _isInitialized; };

    CommandResult getAllHiddenParameters(uint16_t (*hiddenParams)[0x6F]);
    CommandResult getAllParameters(byte (*params)[0x6A]);
    CommandResult getAllStatus(bool refreshStatus, int *MBTYPE, uint16_t *MOD, uint16_t *VER, uint16_t *CORE, char (*FWDATE)[11], char (*APLTS)[20], uint16_t *APLWDAY, byte *CHRSTATUS, uint16_t *STATUS, uint16_t *LSTATUS, bool *isMFSTATUSValid, uint16_t *MFSTATUS, float *SETP, byte *PUMP, uint16_t *PQT, uint16_t *F1V, uint16_t *F1RPM, uint16_t *F2L, uint16_t *F2LF, uint16_t (*FANLMINMAX)[6], uint16_t *F2V, bool *isF3LF4LValid, uint16_t *F3L, uint16_t *F4L, byte *PWR, float *FDR, uint16_t *DPT, uint16_t *DP, byte *IN, byte *OUT, float *T1, float *T2, float *T3, float *T4, float *T5, bool *isSNValid, char (*SN)[28]);
    CommandResult getAllTemps(float *T1, float *T2, float *T3, float *T4, float *T5);
    CommandResult getChronoData(byte *CHRSTATUS, float (*PCHRSETP)[6], byte (*PSTART)[6][2], byte (*PSTOP)[6][2], byte (*DM)[7][3]);
    CommandResult getCounters(uint16_t *IGN, uint16_t *POWERTIMEh, uint16_t *POWERTIMEm, uint16_t *HEATTIMEh, uint16_t *HEATTIMEm, uint16_t *SERVICETIMEh, uint16_t *SERVICETIMEm, uint16_t *ONTIMEh, uint16_t *ONTIMEm, uint16_t *OVERTMPERRORS, uint16_t *IGNERRORS, uint16_t *PQT);
    CommandResult getDateTime(char (*STOVE_DATETIME)[20], byte *STOVE_WDAY);
    CommandResult getDPressData(uint16_t *DP_TARGET, uint16_t *DP_PRESS);
    CommandResult getFanData(uint16_t *F1V, uint16_t *F2V, uint16_t *F1RPM, uint16_t *F2L, uint16_t *F2LF, bool *isF3SF4SValid, float *F3S, float *F4S, bool *isF3LF4LValid, uint16_t *F3L, uint16_t *F4L);
    CommandResult getHiddenParameter(byte hParamNumber, uint16_t *hParamValue);
    CommandResult getIO(byte *IN_I01, byte *IN_I02, byte *IN_I03, byte *IN_I04, byte *OUT_O01, byte *OUT_O02, byte *OUT_O03, byte *OUT_O04, byte *OUT_O05, byte *OUT_O06, byte *OUT_O07);
    CommandResult getModelVersion(uint16_t *MOD, uint16_t *VER, uint16_t *CORE, char (*FWDATE)[11]);
    CommandResult getParameter(byte paramNumber, byte *paramValue);
    CommandResult getPelletQtUsed(uint16_t *PQT);
    CommandResult getPower(byte *PWR, float *FDR);
    CommandResult getSetPoint(float *setPoint);
    CommandResult getSN(char (*SN)[28]);
    CommandResult getStaticData(char (*SN)[28], byte *SNCHK, int *MBTYPE, uint16_t *MOD, uint16_t *VER, uint16_t *CORE, char (*FWDATE)[11], uint16_t *FLUID, uint16_t *SPLMIN, uint16_t *SPLMAX, byte *UICONFIG, byte *HWTYPE, byte *DSPTYPE, byte *DSPFWVER, byte *CONFIG, byte *PELLETTYPE, uint16_t *PSENSTYPE, byte *PSENSLMAX, byte *PSENSLTSH, byte *PSENSLMIN, byte *MAINTPROBE, byte *STOVETYPE, byte *FAN2TYPE, byte *FAN2MODE, byte *BLEMBMODE, byte *BLEDSPMODE, byte *CHRONOTYPE, byte *AUTONOMYTYPE, byte *NOMINALPWR);
    CommandResult getStatus(uint16_t *STATUS, uint16_t *LSTATUS, uint16_t *FSTATUS);
    CommandResult readData(uint16_t addrToRead, bool wordMode, uint16_t *ADDR_DATA);

    CommandResult setChronoDay(byte dayNumber, byte memoryNumber, byte programNumber);
    CommandResult setChronoPrg(byte programNumber, byte setPoint, byte startHour, byte startMinute, byte stophour, byte stopMinute);
    CommandResult setChronoSetpoint(byte programNumber, byte setPoint);
    CommandResult setChronoStartHH(byte programNumber, byte startHour);
    CommandResult setChronoStartMM(byte programNumber, byte startMinute);
    CommandResult setChronoStatus(bool chronoStatus, byte *CHRSTATUSReturn);
    CommandResult setChronoStopHH(byte programNumber, byte stopHour);
    CommandResult setChronoStopMM(byte programNumber, byte stopMinute);
    CommandResult setDateTime(uint16_t year, byte month, byte day, byte hour, byte minute, byte second, char (*STOVE_DATETIMEReturn)[20], byte *STOVE_WDAYReturn);
    CommandResult setHiddenParameter(byte hParamNumber, uint16_t hParamValue);
    CommandResult setParameter(byte paramNumber, byte paramValue);
    CommandResult setPower(byte powerLevel, byte *PWRReturn = nullptr, bool *isF2LReturnValid = nullptr, uint16_t *F2LReturn = nullptr, uint16_t (*FANLMINMAXReturn)[6] = nullptr);
    CommandResult setPowerDown(byte *PWRReturn = nullptr, bool *isF2LReturnValid = nullptr, uint16_t *F2LReturn = nullptr, uint16_t (*FANLMINMAXReturn)[6] = nullptr);
    CommandResult setPowerUp(byte *PWRReturn = nullptr, bool *isF2LReturnValid = nullptr, uint16_t *F2LReturn = nullptr, uint16_t (*FANLMINMAXReturn)[6] = nullptr);
    CommandResult setRoomFan(byte roomFanSpeed, bool *isPWRReturnValid = nullptr, byte *PWRReturn = nullptr, uint16_t *F2LReturn = nullptr, uint16_t *F2LFReturn = nullptr);
    CommandResult setRoomFan3(byte roomFan3Speed, uint16_t *F3LReturn = nullptr);
    CommandResult setRoomFan4(byte roomFan4Speed, uint16_t *F4LReturn = nullptr);
    CommandResult setRoomFanDown(bool *isPWRReturnValid = nullptr, byte *PWRReturn = nullptr, uint16_t *F2LReturn = nullptr, uint16_t *F2LFReturn = nullptr);
    CommandResult setRoomFanUp(bool *isPWRReturnValid = nullptr, byte *PWRReturn = nullptr, uint16_t *F2LReturn = nullptr, uint16_t *F2LFReturn = nullptr);
    CommandResult setSetpoint(byte setPoint, float *SETPReturn = nullptr);
    CommandResult setSetpoint(float setPoint, float *SETPReturn = nullptr);
    CommandResult setSetPointDown(float *SETPReturn = nullptr);
    CommandResult setSetPointUp(float *SETPReturn = nullptr);
    CommandResult setSilentMode(byte silentMode, byte *SLNTReturn = nullptr, byte *PWRReturn = nullptr, uint16_t *F2LReturn = nullptr, uint16_t *F2LFReturn = nullptr, bool *isF3LF4LReturnValid = nullptr, uint16_t *F3LReturn = nullptr, uint16_t *F4LReturn = nullptr);
    CommandResult switchOff(uint16_t *STATUS, uint16_t *LSTATUS, uint16_t *FSTATUS);
    CommandResult switchOn(uint16_t *STATUS, uint16_t *LSTATUS, uint16_t *FSTATUS);
    CommandResult writeData(uint16_t addrToWrite, uint16_t data, bool wordMode) __attribute__((warning("/!\\ writeData is a dangerous function and may arm your stove if not used carefully /!\\")));
    Palazzetti();
};

#endif