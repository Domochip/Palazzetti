#include "Palazzetti.h"

void Palazzetti::SERIALCOM_CloseComport()
{
    m_serial.close();
}

int Palazzetti::SERIALCOM_Flush()
{
    return m_serial.flush();
}

// original signature is (filename,baudrate,databitnumber,stopbitnumber,)
// always 8 data bits and 1 stop bits
int Palazzetti::SERIALCOM_OpenComport(uint32_t baudrate)
{
    return m_serial.open(baudrate);
}

int Palazzetti::SERIALCOM_ReceiveBuf(void *buf, size_t count)
{
    int res = m_serial.select(selectSerialTimeoutMs);
    if (res < 0)
        return -1;

    if (!res)
        return res;

    res = m_serial.read(buf, count);

    if (res < 1)
        return -1;

    if (serialPortModel == 2)
        m_serial.uSleep(1);
    return res;
}

int Palazzetti::SERIALCOM_ReceiveByte(uint8_t *buf)
{
    int res = SERIALCOM_ReceiveBuf(buf, 1);
    if (res < 0)
        return res;

    return 0;
}

int Palazzetti::SERIALCOM_SendBuf(void *buf, size_t count)
{
    size_t uint8_tsSent = 0;
    size_t totalBytesWritten = 0;

    while (totalBytesWritten < count)
    {
        uint8_tsSent = m_serial.write((void *)((uint8_t *)buf + totalBytesWritten), count - totalBytesWritten);
        totalBytesWritten += uint8_tsSent;
    }

    m_serial.drain();

    if (!comPortNumber && !_MBTYPE)
    {
        size_t uint8_tsReaded = 0;
        size_t totalBytesReaded = 0;

        while (totalBytesReaded < count)
        {
            if (m_serial.select(selectSerialTimeoutMs) <= 0)
                return -1;

            uint8_tsReaded = m_serial.read((void *)((uint8_t *)buf + totalBytesReaded), count - totalBytesReaded);
            if (uint8_tsReaded < 0)
                return uint8_tsReaded;
            totalBytesReaded += uint8_tsReaded;
        }
    }

    return uint8_tsSent;
}

void Palazzetti::SERIALCOM_SendByte(uint8_t *buf)
{
    SERIALCOM_SendBuf(buf, 1);
}

int Palazzetti::iChkSum(uint8_t *datasToCheck)
{
    uint8_t chk = 0; // var_10
    for (uint8_t i = 0; i < 0xA; i++)
        chk += datasToCheck[i];

    if (chk != datasToCheck[0xA])
        return -508;
    return 0;
}

int Palazzetti::isValidSerialNumber(char *SN)
{
    if (!SN)
        return 0;

    size_t snLength = strlen(SN);

    if (snLength < 1)
        return 0;

    if (snLength < 10)
        return 1;

    if (SN[0] == 'L' && SN[1] == 'T' && snLength < 0x17)
        return 0;

    if (SN[0] == 'F' && SN[1] == 'F' && (strcmp(SN + snLength - 4, "0000") == 0 || strcmp(SN + snLength - 4, "FFFF") == 0))
        return 0;

    if (SN[0] == 'L' && SN[1] == 'T' && strcmp(SN + snLength - 4, "0000") == 0)
        return 0;

    if (SN[0] == '0' && SN[1] == '0' && strcmp(SN + snLength - 4, "0000") == 0)
        return 0;

    return 1;
}

int Palazzetti::parseRxBuffer(uint8_t *rxBuffer)
{
    int res; // var_10;
    switch (fumisComStatus)
    {
    case 2:
        if (rxBuffer[0] != 0)
            return -1;
        res = iChkSum(rxBuffer);
        if (res < 0)
            return res;
        fumisComStatus = 3;
        return 0;
        break;
    case 4:
        if (rxBuffer[0] != 2)
            return -1;
        res = iChkSum(rxBuffer);
        if (res < 0)
            return res;
        fumisComStatus = 5;
        return 0;
        break;
    }
    return -1;
}

uint16_t Palazzetti::transcodeRoomFanSpeed(uint16_t roomFanSpeed, bool decode)
{
    if (roomFanSpeed == 0)
        return 7;
    if (roomFanSpeed == 7)
        return 0;
    // if (roomFanSpeed == 6)
    //     return 6;

    return roomFanSpeed;
}

Palazzetti::CommandResult Palazzetti::fumisCloseSerial()
{
    SERIALCOM_CloseComport();
    fumisComStatus = 0;
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::fumisComRead(uint16_t addrToRead, uint16_t *data, bool wordMode)
{
    uint8_t var_10[8];
    CommandResult cmdRes = fumisComReadBuff(addrToRead, var_10, 8);
    if (cmdRes == CommandResult::OK)
    {
        // *data = 0; //useless

        if (!wordMode)
            *data = var_10[0];
        else
        {
            *data = var_10[1];
            *data <<= 8;
            *data += var_10[0];
        }
    }

    return cmdRes;
}

Palazzetti::CommandResult Palazzetti::fumisComReadBuff(uint16_t addrToRead, void *buf, size_t count)
{
    if (count != 8)
        return CommandResult::ERROR;

    bzero(buf, count);

    if (!fumisComStatus)
        return CommandResult::ERROR;

    uint8_t var_28[32];
    CommandResult cmdRes;

    for (int i = 2; i > 0; i--) // NOTE : the original value is 4
    {
        if (i < 2) // NOTE : the original value is 4
        {
            fumisCloseSerial();
            SERIALCOM_Flush();
            m_serial.uSleep(1);
            fumisOpenSerial();
            m_serial.uSleep(1);
        }

        if (_MBTYPE != 1)
        {
            fumisComStatus = 2;
            cmdRes = fumisWaitRequest((void *)var_28);
            if (cmdRes != CommandResult::OK)
            {
                if (cmdRes == CommandResult::COMMUNICATION_ERROR)
                    continue;
                else
                {
                    dword_433248 = cmdRes;
                    return cmdRes;
                }
            }
        }

        fumisComStatus = 3;
        bzero(var_28, 0xB);
        var_28[0] = 2;
        var_28[1] = addrToRead & 0xFF;
        var_28[2] = (addrToRead >> 8) & 0xFF;

        var_28[10] = var_28[0] + var_28[1] + var_28[2];
        cmdRes = fumisSendRequest(var_28);
        if (cmdRes != CommandResult::OK)
            return cmdRes;

        bzero(var_28, 32);
        fumisComStatus = 4;
        cmdRes = fumisWaitRequest(var_28);
        if (cmdRes == CommandResult::OK)
        {
            memcpy(buf, var_28 + 1, count);
            return cmdRes;
        }

        if (cmdRes != CommandResult::COMMUNICATION_ERROR)
        {
            dword_433248 = cmdRes;
            return cmdRes;
        }
    }

    if (cmdRes == CommandResult::OK)
        memcpy(buf, var_28 + 1, count);
    else
        dword_433248 = cmdRes;
    return cmdRes;
}

Palazzetti::CommandResult Palazzetti::fumisComReadByte(uint16_t addrToRead, uint16_t *data)
{
    return fumisComRead(addrToRead, data, 0);
}

Palazzetti::CommandResult Palazzetti::fumisComReadWord(uint16_t addrToRead, uint16_t *data)
{
    return fumisComRead(addrToRead, data, 1);
}

Palazzetti::CommandResult Palazzetti::fumisComSetDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
    // The content of this function does not match strictly the original one

    // Check if date is valid
    // basic control
    if (year < 2000 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || minute > 59 || second > 59)
        return CommandResult::ERROR;
    // 30 days month control
    if ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30)
        return CommandResult::ERROR;
    // February leap year control
    if (month == 2 && day > 29)
        return CommandResult::ERROR;
    // February not leap year control
    if (month == 2 && day == 29 && !(((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0)))
        return CommandResult::ERROR;

    // weekDay calculation (Tomohiko Sakamoto’s Algorithm)
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    uint16_t calcYear = year;
    if (month < 3)
        calcYear -= 1;
    uint8_t weekDay = (calcYear + calcYear / 4 - calcYear / 100 + calcYear / 400 + t[month - 1] + day) % 7;
    if (weekDay == 0)
        weekDay = 7;

    CommandResult cmdRes; // local_88
    uint8_t buf[0xB];     // local_60

    for (int i = 3; i > 0; i--) // i as local_6c
    {
        if (!fumisComStatus)
            return CommandResult::ERROR;

        fumisComStatus = 2;
        bzero(&buf, 0xB);
        cmdRes = fumisWaitRequest(&buf);
        if (cmdRes != CommandResult::OK)
        {
            dword_433248 = cmdRes;
            continue;
        }
        bzero(&buf, 0xB);
        buf[0] = 6;
        buf[1] = second;
        buf[2] = minute;
        buf[3] = hour;
        buf[4] = weekDay;
        buf[5] = day;
        buf[6] = month;
        buf[7] = year - 2000;
        buf[10] = buf[0] + buf[1] + buf[2] + buf[3] + buf[4] + buf[5] + buf[6] + buf[7];
        cmdRes = fumisSendRequest(&buf);
        if (cmdRes == CommandResult::OK)
            break;
    }

    sprintf(_STOVE_DATETIME, "%d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
    _STOVE_WDAY = weekDay;

    return cmdRes;
}

Palazzetti::CommandResult Palazzetti::fumisComWrite(uint16_t addrToWrite, uint16_t data, int wordMode)
{
    if (!fumisComStatus)
        return CommandResult::ERROR;

    CommandResult cmdRes; // var_38
    uint8_t buf[0xB];     // var_2C

    for (int i = 2; i > 0; i--) // i as var_34
    {
        fumisComStatus = 2;
        bzero(&buf, 0xB);
        cmdRes = fumisWaitRequest(&buf);
        if (cmdRes != CommandResult::OK)
        {
            dword_433248 = cmdRes;
            continue;
        }
        bzero(&buf, 0xB);
        buf[0] = 1;
        buf[1] = addrToWrite & 0xFF;
        buf[2] = addrToWrite >> 8;
        buf[3] = data & 0xFF;
        buf[10] = buf[0] + buf[1] + buf[2] + buf[3];
        cmdRes = fumisSendRequest(&buf);
        if (cmdRes != CommandResult::OK)
            return cmdRes;

        if (!wordMode)
        {
            uint16_t var_30 = 0;
            fumisComRead(addrToWrite, &var_30, 0);
            if (var_30 == data)
                return cmdRes;
            else
                continue;
        }

        fumisComStatus = 2;
        cmdRes = fumisWaitRequest(&buf);
        if (cmdRes != CommandResult::OK)
        {
            dword_433248 = cmdRes;
            return cmdRes;
        }
        bzero(&buf, 0xB);
        buf[0] = 1;
        buf[1] = (addrToWrite + 1) & 0xFF;
        buf[2] = (addrToWrite + 1) >> 8;
        buf[3] = data >> 8;
        buf[10] = buf[0] + buf[1] + buf[2] + buf[3];
        cmdRes = fumisSendRequest(&buf);
        if (cmdRes != CommandResult::OK)
            return cmdRes;

        uint16_t var_30 = 0;
        fumisComRead(addrToWrite, &var_30, 1);
        if (var_30 == data)
            return cmdRes;
    }
    return cmdRes;
}

Palazzetti::CommandResult Palazzetti::fumisComWriteByte(uint16_t addrToWrite, uint16_t data)
{
    return fumisComWrite(addrToWrite, data, 0);
}

Palazzetti::CommandResult Palazzetti::fumisComWriteWord(uint16_t addrToWrite, uint16_t data)
{
    return fumisComWrite(addrToWrite, data, 1);
}

Palazzetti::CommandResult Palazzetti::fumisOpenSerial()
{
    selectSerialTimeoutMs = 100; // NOTE : the original value is 2300 but the longest measured "select" is ~35ms
    int res = SERIALCOM_OpenComport(0x9600);

    if (res >= 0 && (serialPortModel != 1 || (res = SERIALCOM_Flush()) >= 0))
    {
        dword_433248 = CommandResult::OK;
        fumisComStatus = 1;
        res = 0;
    }
    else
    {
        fumisComStatus = 0;
        dword_433248 = (res >= 0) ? CommandResult::OK : CommandResult::ERROR;
    }

    return (res >= 0) ? CommandResult::OK : CommandResult::ERROR;
}

Palazzetti::CommandResult Palazzetti::fumisSendRequest(void *buf)
{
    if (fumisComStatus != 3)
        return CommandResult::ERROR;

    int totalSentBytes = 0; // var_18
    int sentBytes = 0;      // dummy value for firstrun
    while (totalSentBytes < 0xB && sentBytes != -1)
    {
        sentBytes = SERIALCOM_SendBuf((void *)((uint8_t *)buf + totalSentBytes), 0xB - totalSentBytes);
        totalSentBytes += sentBytes;
    }
    if (sentBytes < 0)
        dword_433248 = CommandResult::ERROR;
    return (sentBytes >= 0) ? CommandResult::OK : CommandResult::ERROR;
}

Palazzetti::CommandResult Palazzetti::fumisWaitRequest(void *buf)
{
    if (!fumisComStatus)
        return CommandResult::ERROR;

    // fumisComStatus 2 means waiting for stove "Request" frame (first frame byte == 0)
    // then we should discard RX buffer content
    if (fumisComStatus == 2 && serialPortModel == 2 && SERIALCOM_Flush() < 0)
        return CommandResult::COMMUNICATION_ERROR;

    int nbReceivedBytes = 0; // var_18

    unsigned long startTime; // var_10
    startTime = millis();    // instead of time()
    do
    {
        nbReceivedBytes = 0;
        bzero(buf, 0xB);

        uint8_t bufPosition = 0;

        while (bufPosition < 0xB || parseRxBuffer((uint8_t *)buf) < 0)
        {
            // fumisComStatus 4 means direct read of stove answer
            // if buffer contains 11 bytes and fumisComStatus is still equal to 4, it means that the parseRxBuffer failed
            if (bufPosition == 0xB && fumisComStatus == 4)
                return CommandResult::COMMUNICATION_ERROR;

            nbReceivedBytes = SERIALCOM_ReceiveBuf((uint8_t *)buf + bufPosition, 0xB - bufPosition);

            bufPosition += nbReceivedBytes;

            // Stove "Request" frames are always followed by a 28ms delay
            // longest measured delay for other frames is ~9ms
            // so if we are in fumisComStatus 2, already received 11 bytes and some bytes are received before 18ms have passed
            // we should slide the buffer to the left by one byte to continue reading
            if (fumisComStatus == 2 && bufPosition == 0xB && m_serial.select(18))
            {
                bufPosition--;
                for (int i = 0; i < bufPosition; i++)
                    ((uint8_t *)buf)[i] = ((uint8_t *)buf)[i + 1];
            }

            if (nbReceivedBytes < 0)
            {
                dword_433248 = CommandResult::ERROR;
                if (fumisCloseSerial() != CommandResult::OK)
                    return CommandResult::ERROR;
                if (fumisOpenSerial() != CommandResult::OK)
                    return CommandResult::ERROR;
                if (serialPortModel == 2 && SERIALCOM_Flush() < 0)
                    return CommandResult::COMMUNICATION_ERROR;
                break;
            }

            if (millis() - startTime > 500) // NOTE : the original value is 3000 but 500ms should contains at least ~10 Request frames...
                return CommandResult::COMMUNICATION_ERROR;
        }

        if (fumisComStatus == 5 || fumisComStatus == 3)
            return CommandResult::OK;
    } while (1);
}

Palazzetti::CommandResult Palazzetti::iChkMBType()
{

    // _MBTYPE = 100;
    // No Implementation of Micronova device there
    // skip directly to fumis detection

    _MBTYPE = 0;
    CommandResult cmdRes;
    cmdRes = fumisOpenSerial();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    ///*Trying to find MBTYPE_FUMIS_ALPHA65...*/
    if (iGetStatusAtech() != CommandResult::OK)
    {
        _MBTYPE = -1;
        return CommandResult::ERROR;
    }
    ///*-->Found MBTYPE_FUMIS_ALPHA65 device!*/
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iCloseUART()
{
    if (_MBTYPE < 2)
        return fumisCloseSerial();
    else
        return CommandResult::UNSUPPORTED;
}

Palazzetti::CommandResult Palazzetti::iGetAllStatus(bool refreshStatus)
{

    if (_MBTYPE)
        return CommandResult::UNSUPPORTED;

    CommandResult cmdRes;

    if (refreshStatus)
    {
        cmdRes = iGetDateTimeAtech();
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        cmdRes = iGetStatusAtech();
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        cmdRes = iGetSetPointAtech();
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        cmdRes = iReadFansAtech();
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        cmdRes = iGetPowerAtech();
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        cmdRes = iGetDPressDataAtech();
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        cmdRes = iReadIOAtech();
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        cmdRes = iReadTemperatureAtech();
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        cmdRes = iGetPumpRateAtech();
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        cmdRes = iGetPelletQtUsedAtech();
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        cmdRes = iGetChronoDataAtech();
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        // cmdRes = iGetErrorFlagAtech();
        // if (cmdRes != CommandResult::OK)
        //     return cmdRes;
        // if (_PSENSTYPE)
        // {
        //     cmdRes = iGetPelletLevelAtech();
        //     if (cmdRes != CommandResult::OK)
        //         return cmdRes;
        // }
    }

    if (!staticDataLoaded)
    {
        cmdRes = iUpdateStaticData();
        if (cmdRes != CommandResult::OK)
            return cmdRes;
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iGetChronoDataAtech()
{
    uint8_t CHRSETPList[8];
    CommandResult cmdRes = fumisComReadBuff(0x802D, CHRSETPList, 8);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    uint16_t addrToRead = 0x8000;
    uint8_t programTimes[8];
    for (uint8_t i = 0; i < 6; i++)
    {
        cmdRes = fumisComReadBuff(addrToRead, programTimes, 8);
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        chronoDataPrograms[i].STARTH = programTimes[0];
        chronoDataPrograms[i].STARTM = programTimes[1];
        chronoDataPrograms[i].STOPH = programTimes[2];
        chronoDataPrograms[i].STOPM = programTimes[3];
        chronoDataPrograms[i].CHRSETP = CHRSETPList[i];
        if (!_FLUID)
            chronoDataPrograms[i].CHRSETP /= 5.0;

        addrToRead += 4;
    }

    addrToRead = 0x8018;
    uint8_t dayPrograms[8];
    for (uint8_t i = 0; i < 7; i++)
    {
        cmdRes = fumisComReadBuff(addrToRead, dayPrograms, 8);
        if (cmdRes != CommandResult::OK)
            return cmdRes;

        chronoDataDays[i].M1 = dayPrograms[0];
        chronoDataDays[i].M2 = dayPrograms[1];
        chronoDataDays[i].M3 = dayPrograms[2];

        addrToRead += 3;
    }

    cmdRes = fumisComReadWord(0x207e, &chronoDataStatus);
    if (cmdRes != CommandResult::OK)
        return cmdRes;
    _CHRSTATUS = chronoDataStatus & 0x01;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iGetCountersAtech()
{

    uint8_t var_18[8];
    CommandResult cmdRes = fumisComReadBuff(0x2066, var_18, 8);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    _IGN = var_18[1];
    _IGN <<= 8;
    _IGN += var_18[0];

    _POWERTIMEM = var_18[3];
    _POWERTIMEM <<= 8;
    _POWERTIMEM += var_18[2];

    _POWERTIMEH = var_18[5];
    _POWERTIMEH <<= 8;
    _POWERTIMEH += var_18[4];

    cmdRes = fumisComReadBuff(0x206E, var_18, 8);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    _HEATTIMEM = var_18[1];
    _HEATTIMEM <<= 8;
    _HEATTIMEM += var_18[0];

    _HEATTIMEH = var_18[3];
    _HEATTIMEH <<= 8;
    _HEATTIMEH += var_18[2];

    _SERVICETIMEM = var_18[7];
    _SERVICETIMEM <<= 8;
    _SERVICETIMEM += var_18[6];

    cmdRes = fumisComReadBuff(0x2076, var_18, 8);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    _SERVICETIMEH = var_18[1];
    _SERVICETIMEH <<= 8;
    _SERVICETIMEH += var_18[0];

    _OVERTMPERRORS = var_18[5];
    _OVERTMPERRORS <<= 8;
    _OVERTMPERRORS += var_18[4];

    _IGNERRORS = var_18[7];
    _IGNERRORS <<= 8;
    _IGNERRORS += var_18[6];

    cmdRes = fumisComReadBuff(0x2082, var_18, 8);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    _ONTIMEM = var_18[1];
    _ONTIMEM <<= 8;
    _ONTIMEM += var_18[0];

    _ONTIMEH = var_18[3];
    _ONTIMEH <<= 8;
    _ONTIMEH += var_18[2];

    uint16_t var_10;

    cmdRes = fumisComReadWord(0x2002, &var_10);
    if (cmdRes != CommandResult::OK)
        return cmdRes;
    _PQT = var_10;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iGetDateTimeAtech()
{
    uint8_t buf[8]; // var_14
    CommandResult cmdRes = fumisComReadBuff(0x204E, buf, 8);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    sprintf(_STOVE_DATETIME, "%d-%02d-%02d %02d:%02d:%02d", (uint16_t)buf[6] + 2000, buf[5], buf[4], buf[2], buf[1], buf[0]);

    _STOVE_WDAY = buf[3];

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iGetDPressDataAtech()
{
    uint16_t buf; // var_10
    CommandResult cmdRes = fumisComReadWord(0x2000, &buf);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    _DP_TARGET = buf;

    cmdRes = fumisComReadWord(0x2020, &buf);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    _DP_PRESS = (int16_t)buf;

    return CommandResult::OK;
}

void Palazzetti::iGetFanLimits()
{
    if (byte_471CC2)
    {
        if (byte_471CC2 < _PWR)
            _FAN2LMIN = _PWR - byte_471CC2;
        else
            _FAN2LMIN = 0;
    }
}

Palazzetti::CommandResult Palazzetti::iGetHiddenParameterAtech(uint16_t hParamToRead, uint16_t *hParamValue)
{
    if (hParamToRead > 0x6E)
        return CommandResult::ERROR;

    CommandResult cmdRes = fumisComReadWord((hParamToRead + 0xF00) * 2, hParamValue);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iGetMBTypeAtech()
{
    uint16_t buf;         // var_C
    CommandResult cmdRes; // var_10
    cmdRes = fumisComReadWord(0x204C, &buf);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    _HWTYPE = 6;
    if ((buf & 4) == 0)
    {
        if ((buf & 0x8000) != 0)
            _HWTYPE = 7;
    }
    else
        _HWTYPE = 5;
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iGetParameterAtech(uint16_t paramToRead, uint16_t *paramValue)
{
    if (paramToRead > 0x69)
        return CommandResult::ERROR;

    CommandResult cmdRes = fumisComReadByte(paramToRead + 0x1C00, paramValue);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iGetPelletQtUsedAtech()
{
    uint16_t var_10;
    CommandResult cmdRes = fumisComReadWord(0x2002, &var_10);
    if (cmdRes != CommandResult::OK)
        return cmdRes;
    _PQT = var_10;
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iGetPowerAtech()
{
    uint16_t var_C;

    CommandResult cmdRes = fumisComReadWord(0x202a, &var_C);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    _PWR = var_C & 0xFF;

    cmdRes = fumisComReadWord(wAddrFeederActiveTime, &var_C);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    _FDR = var_C / 10.0f;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iGetPumpRateAtech()
{
    uint16_t buf;
    CommandResult cmdRes = fumisComReadWord(0x2090, &buf);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    _PUMP = buf & 0xFF;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iGetRoomFanAtech()
{
    uint16_t var_C;
    CommandResult cmdRes = fumisComReadByte(0x2036, &var_C);
    if (cmdRes != CommandResult::OK)
        return cmdRes;
    _F2L = var_C;

    cmdRes = fumisComReadWord(0x2004, &var_C);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_FAN2TYPE == 4)
    {
        _F3L = var_C & 1;
        _F4L = ((var_C & 2) != 0);
    }
    else if (_FAN2TYPE == 5)
    {
        _F3L = var_C & 0xFF;
        _F4L = var_C >> 8;
    }
    else if (_FAN2TYPE == 3)
    {
        _F3L = 0;
        _F4L = var_C;
    }
    else
    {
        _F3L = 0;
        _F4L = 0;
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iGetSetPointAtech()
{
    if (_STOVETYPE == 1 && _PARAMS[0x4C] == 2)
    {
        _SETP = 0;
    }
    else if (_FLUID < 2)
    {
        uint8_t buf[8]; // var_1c
        CommandResult cmdRes = fumisComReadBuff(0x1C32, buf, 8);
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        _SECO = buf[0];
        _SETP = buf[1];

        if (!_FLUID)
        {
            _SECO /= 10.0;
            _SETP /= 5.0;
        }

        _BECO = (buf[3] > 0) ? 1 : 0;
    }
    else if (_FLUID == 2)
    {
        uint16_t data;
        CommandResult cmdRes = fumisComReadByte(0x1C54, &data);
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        _SETP = (int16_t)data;
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iGetSNAtech()
{
    CommandResult cmdRes;
    int currentPosInSN = 0;   // var_24
    uint8_t buf[8];           // var_14
    char *pSN = (char *)&_SN; // var_18
    int8_t checkSum = 0;      // must be signed
    while (currentPosInSN < 0xE)
    {
        cmdRes = fumisComReadBuff(0x2100 + currentPosInSN, buf, 8);
        if (cmdRes != CommandResult::OK)
        {
            _SN[0] = 0;
            return cmdRes;
        }
        for (uint8_t i = 0; i < 8; i++)
        {
            sprintf(pSN, "%02X", buf[i]);
            pSN += 2;
            checkSum += buf[i];
        }
        currentPosInSN += 8;
    }

    _SN[27] = 0;
    cmdRes = fumisComReadBuff(0x2100 + currentPosInSN, buf, 8);
    if (cmdRes != CommandResult::OK)
    {
        _SN[0] = 0;
        return cmdRes;
    }
    if (buf[0] == -checkSum)
    {
        _SN[0] = 0;
        return CommandResult::ERROR;
    }
    if (buf[1] != 'U')
    {
        if (strncmp(_SN, "7684", 4) == 0)
        {
            _SN[0] = 'L';
            _SN[1] = 'T';
            strcpy((char *)&_SN + 2, (char *)&_SN + 4);
            _SN[23] = 0;
        }
        return CommandResult::OK;
    }

    _SN[0] = 0;
    return CommandResult::ERROR;
}

Palazzetti::CommandResult Palazzetti::iGetStatusAtech()
{
    CommandResult cmdRes;
    cmdRes = fumisComReadByte(0x201C, &_STATUS);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    _LSTATUS = _STATUS;

    if (200 < _STATUS)
    {
        _LSTATUS = _STATUS + 1000;
        cmdRes = fumisComReadByte(0x2081, &_FSTATUS);
        if (cmdRes != CommandResult::OK)
            return cmdRes;
    }

    if (_STOVETYPE == 3 || _STOVETYPE == 4)
    {
        cmdRes = fumisComReadWord(0x2008, &_MFSTATUS);
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        if (1 < _MFSTATUS)
        {
            _LSTATUS = _MFSTATUS + 500;
            if (_LSTATUS == 0x1FC)
                _LSTATUS = _MFSTATUS + 0x5DC;
        }
    }

    if (_LSTATUS != 9)
        return CommandResult::OK;

    if (_STOVETYPE != 2 || (_UICONFIG != 1 && _UICONFIG != 3 && _UICONFIG != 4))
    {
        _LSTATUS = 0x33;
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iGetStoveConfigurationAtech()
{
    uint16_t buf;         // local_20
    CommandResult cmdRes; // var_24
    cmdRes = fumisComReadByte(0x2006, &buf);
    if (cmdRes != CommandResult::OK)
        return cmdRes;
    _DSPFWVER = buf;
    if (_DSPFWVER > 0)
        _DSPTYPE = ((float)buf) / pow(10, log10(buf));

    cmdRes = iGetMBTypeAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    // byte_471087 = 2; //Not Used elsewhere
    _MAINTPROBE = 0;

    cmdRes = fumisComReadWord(0x1ED4, &buf);
    if (cmdRes != CommandResult::OK)
        return cmdRes;
    uint8_t buf2[8]; // local_28
    cmdRes = fumisComReadBuff(0x1E25, buf2, 8);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    _NOMINALPWR = buf2[0];
    if (buf2[1] & 2)
        _AUTONOMYTYPE = 2;
    else
        _AUTONOMYTYPE = 1;

    uint8_t bVar1 = _PARAMS[0x4C];
    _UICONFIG = bVar1;

    cmdRes = fumisComReadBuff(((bVar1 - 1) * 4) + 0x1E36, buf2, 8);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    bool bVar2 = ((buf2[0] & 0x20) != 0);

    _FAN2TYPE = 1;
    _MAINTPROBE = 0;

    if (buf2[0] & 0x40)
        _MAINTPROBE = 4;

    if (buf2[0] & 8)
    {
        if ((buf2[2] & 0x80) == 0)
            _FAN2TYPE = 2;
        else
        {
            _MAINTPROBE = 4;
            if ((buf & 0x8000) == 0)
            {
                _FAN2TYPE = 4;
                if (_HWTYPE == 7)
                    _FAN2TYPE = 5;
            }
            else
                _FAN2TYPE = 3;
        }
    }

    byte_471CC2 = 0;

    if (_PARAMS[0x69] != 0 && _PARAMS[0x69] < 6)
        byte_471CC2 = _PARAMS[0x69];

    _FAN2LMIN = 1;
    _FAN2LMAX = 5;
    _FAN3LMIN = 0;
    _FAN3LMAX = 1;
    _FAN4LMIN = 0;
    _FAN4LMAX = 1;
    _FAN2MODE = 1;

    if (_HPARAMS[0x26 / 2] & 0x10)
        _FAN2LMIN = 0;

    if (((_HPARAMS[0x38 / 2] + ((bVar1 - 1) * 2)) & 0x800) == 0)
        _FAN2MODE = 3;

    if (_FAN2TYPE == 5 || _FAN2TYPE == 3)
    {
        _FAN3LMAX = 5;
        _FAN4LMAX = 5;
    }

    uint8_t tmp = 1; // local_37
    if (_HWTYPE == 5)
    {
        if (_CORE > 0x13)
            tmp = 0x11;
    }
    else if (_HWTYPE == 7)
    {
        tmp = 3;
        if (_CORE > 0x1D)
            tmp = 0xD;
        if (_CORE > 0x21)
            tmp = 0x11;
    }
    else
    {
        if (_CORE > 0x81)
            tmp = 3;
        if (_CORE > 0x88)
            tmp = 7;
    }
    _BLEMBMODE = tmp;

    if (_DSPTYPE != 2 && (_DSPTYPE != 4 || _DSPFWVER < 0x2E))
    {
        if (_DSPTYPE == 4 && _DSPFWVER > 0x29)
        {
            if (tmp > 6)
                tmp -= 4;
        }
        else
            tmp = 1;
    }
    _BLEDSPMODE = tmp;

    if (bVar2)
        _STOVETYPE = 2;
    else
        _STOVETYPE = 1;

    if (bVar1 < 3)
    {
        cmdRes = fumisComReadBuff(0x1E36, buf2, 8);
        if (cmdRes != CommandResult::OK)
            return cmdRes;

        if (bVar1 == 2)
            buf2[1] = buf2[5];

        if (buf2[1] & 4)
        {
            _FLUID = 0;
            if (bVar2)
                _MAINTPROBE = 4;
        }
        else
            _FLUID = 1;
    }
    else if (bVar1 == 5)
    {
        _FLUID = 0;
        _STOVETYPE = 2;
        _MAINTPROBE = 4;
        _FAN2TYPE = 2;
        _FAN2MODE = 3;
    }
    else
    {
        _FLUID = 2;
        _STOVETYPE = 2;
        _MAINTPROBE = 4;
    }

    if (500 < _MOD && _MOD < 600)
    {
        if (_STOVETYPE == 1)
        {
            _STOVETYPE = 3;
            _MAINTPROBE = 4;
        }
        else
        {
            if (_STOVETYPE != 2)
                return CommandResult::ERROR;

            _STOVETYPE = 4;
            if (bVar1 == 2)
                _MAINTPROBE = 4;
        }
    }

    ///*iGetStoveConfigurationAtech OK*/
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iInit()
{
    if (_MBTYPE < 0)
        return CommandResult::UNSUPPORTED;

    if (_MBTYPE < 2) // if Fumis MB
    {
        // dword_46DC04 = malloc(0xD0);
        paramsBufferSize = 0x6A;
        hparamsBufferSize = 0x6F;
        // dword_46DC38 = malloc(paramsBufferSize aka 0x6A);
        // dword_46DC50 = malloc(paramsBufferSize aka 0x6A);
        // dword_46DC4C = malloc(paramsBufferSize aka 0x6A);
        // dword_46DC40 =  malloc(hparamsBufferSize<<1 aka 0xDE)
        // dword_46DC08 = malloc(0x16)
        // bzero(dword_46DC08,0x16);
        // dword_46DC0C = malloc(0x69);
        // bzero(dword_46DC0C,0x69);
    }

    // rest of iInit concerns Micronova which is not implemented

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iReadDataAtech(uint16_t addrToRead, uint16_t *data, bool wordMode)
{
    if (!wordMode)
        return fumisComReadByte(addrToRead, data);
    else
        return fumisComReadWord(addrToRead, data);
}

Palazzetti::CommandResult Palazzetti::iReadFansAtech()
{
    uint8_t buf[8];
    CommandResult cmdRes = fumisComReadBuff(0x2024, buf, 8);
    if (cmdRes != CommandResult::OK)
    {
        _F1V = 0xFFFF;
        _F2V = 0xFFFF;
        _F1RPM = 0xFFFF;
        _F3S = -1;
        _F4S = -1;
        return cmdRes;
    }
    _F1V = buf[1];
    _F1V <<= 8;
    _F1V += buf[0];

    _F2V = buf[3];
    _F2V <<= 8;
    _F2V += buf[2];

    _F1RPM = buf[5];
    _F1RPM <<= 8;
    _F1RPM += buf[4];

    cmdRes = iGetRoomFanAtech();
    if (cmdRes != CommandResult::OK)
    {
        _F1V = 0xFFFF;
        _F2V = 0xFFFF;
        _F1RPM = 0xFFFF;
        _F3S = -1;
        _F4S = -1;
        return cmdRes;
    }
    if (_BLEMBMODE > 1)
    {
        uint16_t buf2;
        cmdRes = fumisComReadWord(0x20A2, &buf2);
        if (cmdRes != CommandResult::OK)
        {
            _F1V = 0xFFFF;
            _F2V = 0xFFFF;
            _F1RPM = 0xFFFF;
            _F3S = -1;
            _F4S = -1;
            return cmdRes;
        }
        _F3S = buf2 & 0xFF;
        _F3S /= 5.0f; // Code indicates a multiplication by 0.2
        _F4S = buf2 >> 8;
        _F4S /= 5.0f; // Code indicates a multiplication by 0.2
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iReadIOAtech()
{
    uint8_t buf[8];
    CommandResult cmdRes = fumisComReadBuff(0x203c, buf, 8);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    _IN_I01 = (buf[0] & 0x01);
    _IN_I02 = (buf[0] & 0x02) >> 1;
    _IN_I03 = ((buf[0] & 0x04) >> 2) == 0;
    _IN_I04 = (buf[0] & 0x08) >> 3;

    _OUT_O01 = (buf[2] & 0x01);
    _OUT_O02 = (buf[2] & 0x02) >> 1;
    _OUT_O03 = (buf[2] & 0x04) >> 2;
    _OUT_O04 = (buf[2] & 0x08) >> 3;
    _OUT_O05 = (buf[2] & 0x10) >> 4; // 0x16 : original implementation is wrong
    _OUT_O06 = (buf[2] & 0x20) >> 5; // 0x32
    _OUT_O07 = (buf[2] & 0x40) >> 6; // 0x64

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iReadTemperatureAtech()
{
    CommandResult cmdRes; // var_1C
    uint8_t buf[8];       // var_14
    uint16_t conv = 0;
    cmdRes = fumisComReadBuff(0x200A, buf, 8);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    conv = buf[1];
    conv <<= 8;
    conv += buf[0];
    _T3 = (int16_t)conv;

    conv = buf[3];
    conv <<= 8;
    conv += buf[2];
    _T4 = (int16_t)conv;

    conv = buf[5];
    conv <<= 8;
    conv += buf[4];
    _T1 = (int16_t)conv;
    _T1 /= 10.0f;

    conv = buf[7];
    conv <<= 8;
    conv += buf[6];
    _T2 = (int16_t)conv;
    _T2 /= 10.0f;

    uint16_t var_18;
    cmdRes = fumisComReadWord(0x2012, &var_18);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    _T5 = (int16_t)var_18;
    _T5 /= 10.0f;
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iSetChronoDayAtech(uint8_t dayNumber, uint8_t memoryNumber, uint8_t programNumber)
{
    if (programNumber > 6)
        return CommandResult::ERROR;

    if (!dayNumber || dayNumber > 7)
        return CommandResult::ERROR;

    if (!memoryNumber || memoryNumber > 3)
        return CommandResult::ERROR;

    CommandResult cmdRes = fumisComWriteByte((dayNumber - 1) * 3 + memoryNumber + 0x8017, programNumber);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iSetChronoPrgAtech(uint8_t prg[6])
{
    if (!prg[0] || prg[0] > 6)
        return CommandResult::ERROR;

    if (prg[1] < _SPLMIN)
        prg[1] = _SPLMIN;

    if (prg[1] > _SPLMAX)
        prg[1] = _SPLMAX;

    if (!_FLUID)
        prg[1] *= 5;

    if (prg[2] >= 24 || prg[4] >= 24)
        return CommandResult::ERROR;

    if (prg[3] >= 60 || prg[5] >= 60)
        return CommandResult::ERROR;

    CommandResult cmdRes = fumisComWriteByte(prg[0] + 0x802c, prg[1]);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (!_FLUID)
        prg[1] /= 5;

    cmdRes = fumisComWriteByte((prg[0] + 0x1fff) * 4, prg[2]);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    cmdRes = fumisComWriteByte((prg[0] + 0x1fff) * 4 + 1, prg[3]);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    cmdRes = fumisComWriteByte((prg[0] + 0x1fff) * 4 + 2, prg[4]);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    cmdRes = fumisComWriteByte((prg[0] + 0x1fff) * 4 + 3, prg[5]);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iSetChronoSetpointAtech(uint8_t programNumber, uint8_t setPoint)
{
    if (!programNumber || programNumber > 6)
        return CommandResult::ERROR;

    if (setPoint < _SPLMIN)
        setPoint = _SPLMIN;

    if (setPoint > _SPLMAX)
        setPoint = _SPLMAX;

    if (!_FLUID)
        setPoint *= 5;

    CommandResult cmdRes = fumisComWriteByte(0x802C + programNumber, setPoint);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iSetChronoStartHHAtech(uint8_t programNumber, uint8_t startHour)
{
    if (!programNumber || programNumber > 6 || startHour >= 24)
        return CommandResult::ERROR;

    CommandResult cmdRes = fumisComWriteByte((programNumber + 0x1FFF) * 4, startHour);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iSetChronoStartMMAtech(uint8_t programNumber, uint8_t startMinute)
{
    if (!programNumber || programNumber > 6 || startMinute >= 60)
        return CommandResult::ERROR;

    CommandResult cmdRes = fumisComWriteByte((programNumber + 0x1FFF) * 4 + 1, startMinute);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iSetChronoStatusAtech(bool chronoStatus)
{
    uint16_t buf;
    CommandResult cmdRes = fumisComReadWord(0x207e, &buf);
    if (cmdRes != CommandResult::OK)
        return cmdRes;
    if (chronoStatus)
        buf |= 1;
    else
        buf &= 0xfffe;

    cmdRes = fumisComWriteByte(0x207e, buf);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iSetChronoStopHHAtech(uint8_t programNumber, uint8_t stopHour)
{
    if (!programNumber || programNumber > 6 || stopHour >= 24)
        return CommandResult::ERROR;

    CommandResult cmdRes = fumisComWriteByte((programNumber + 0x1FFF) * 4 + 2, stopHour);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iSetChronoStopMMAtech(uint8_t programNumber, uint8_t stopMinute)
{
    if (!programNumber || programNumber > 6 || stopMinute >= 60)
        return CommandResult::ERROR;

    CommandResult cmdRes = fumisComWriteByte((programNumber + 0x1FFF) * 4 + 3, stopMinute);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iSetHiddenParameterAtech(uint16_t hParamToWrite, uint16_t hParamValue)
{
    if (hParamToWrite > 0x6E)
        return CommandResult::ERROR;

    CommandResult cmdRes = fumisComWriteWord((hParamToWrite + 0xF00) * 2, hParamValue);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iSetParameterAtech(uint8_t paramToWrite, uint8_t paramValue)
{
    if (paramToWrite >= 0x6A)
        return CommandResult::ERROR;

    CommandResult cmdRes = fumisComWriteByte(paramToWrite + 0x1C00, paramValue);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iSetPowerAtech(uint16_t powerLevel)
{
    if (powerLevel < 1 || powerLevel > 5)
        return CommandResult::ERROR;

    CommandResult cmdRes = fumisComWriteByte(0x202a, powerLevel);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    _PWR = powerLevel;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iSetRoomFan3Atech(uint16_t roomFan3Speed)
{
    if (roomFan3Speed < 0 || roomFan3Speed > 5)
        return CommandResult::ERROR;

    CommandResult cmdRes;

    if (_FAN2TYPE == 4)
    {
        cmdRes = fumisComWriteWord(0x2004, (!_F4L ? 0 : 2) | (0 < roomFan3Speed));
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        _F3L = (0 < roomFan3Speed);
    }
    else
    {
        if (_FAN2TYPE != 5)
            return CommandResult::ERROR;
        cmdRes = fumisComWriteByte(0x2004, roomFan3Speed);
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        _F3L = roomFan3Speed;
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iSetRoomFan4Atech(uint16_t roomFan4Speed)
{
    if (roomFan4Speed < 0 || roomFan4Speed > 5)
        return CommandResult::ERROR;

    CommandResult cmdRes;

    if (_FAN2TYPE == 4)
    {
        cmdRes = fumisComWriteWord(0x2004, (!roomFan4Speed ? 0 : 2) | (0 < _F3L));
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        _F4L = (0 < roomFan4Speed);
    }
    else
    {
        if (_FAN2TYPE == 5)
            cmdRes = fumisComWriteByte(0x2005, roomFan4Speed);
        else if (_FAN2TYPE == 3)
            cmdRes = fumisComWriteWord(0x2004, roomFan4Speed);
        else
            return CommandResult::ERROR;

        if (cmdRes != CommandResult::OK)
            return cmdRes;
        _F4L = roomFan4Speed;
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iSetRoomFanAtech(uint16_t roomFanSpeed)
{
    if (roomFanSpeed < 0 || roomFanSpeed > 7)
        return CommandResult::ERROR;

    CommandResult cmdRes;

    if (byte_471CC2 == 0 || _PWR < 4 || roomFanSpeed != 7 || (cmdRes = iSetPowerAtech(3)) == CommandResult::OK)
    {
        cmdRes = fumisComWriteByte(0x2036, roomFanSpeed);
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        cmdRes = iGetRoomFanAtech();
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        _F2L = roomFanSpeed;
        return CommandResult::OK;
    }
    return cmdRes;
}

Palazzetti::CommandResult Palazzetti::iSetSetPointAtech(uint8_t setPoint)
{
    return iSetSetPointAtech((float)setPoint);
}

Palazzetti::CommandResult Palazzetti::iSetSetPointAtech(float setPoint)
{
    CommandResult cmdRes; // var_10

    if (setPoint < _SPLMIN)
        setPoint = _SPLMIN;

    if (setPoint > _SPLMAX)
        setPoint = _SPLMAX;

    if (_FLUID < 2)
    {
        if (!_FLUID)
        {
            cmdRes = fumisComWriteByte(0x1C33, setPoint * 5.0f);
            if (cmdRes != CommandResult::OK)
                return cmdRes;
            _SETP = setPoint;
        }
        else
        {
            cmdRes = fumisComWriteByte(0x1C33, setPoint);
            if (cmdRes != CommandResult::OK)
                return cmdRes;
            _SETP = setPoint;
        }
    }
    else if (_FLUID == 2)
    {
        cmdRes = fumisComWriteByte(0x1C54, setPoint);
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        _SETP = setPoint;
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iSetSilentModeAtech(uint16_t silentMode)
{
    if (silentMode > 0)
    {
        CommandResult cmdRes = iSetRoomFanAtech(7);
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        cmdRes = iSetRoomFan3Atech(0);
        if (cmdRes != CommandResult::OK)
            return cmdRes;
        cmdRes = iSetRoomFan4Atech(0);
        if (cmdRes != CommandResult::OK)
            return cmdRes;
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iSwitchOffAtech()
{
    CommandResult cmdRes; // var_10
    if (_MOD < 500 || 599 < _MOD)
        cmdRes = fumisComWriteWord(0x2044, 1);
    else
        cmdRes = fumisComWriteWord(0x2044, 0x11);

    if (cmdRes != CommandResult::OK)
        return CommandResult::ERROR;
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iSwitchOnAtech()
{
    CommandResult cmdRes; // var_10
    if (_MOD < 500 || 599 < _MOD)
        cmdRes = fumisComWriteWord(0x2044, 2);
    else
        cmdRes = fumisComWriteWord(0x2044, 0x12);

    if (cmdRes != CommandResult::OK)
        return CommandResult::ERROR;
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iUpdateStaticData()
{
    // copy mac address in _MAC
    // look for \n in _MAC
    // then replace it by 0

    if (_MBTYPE < 0)
        return CommandResult::UNSUPPORTED;

    if (_MBTYPE < 2) // if fumis MBTYPE
    {
        CommandResult cmdRes = iUpdateStaticDataAtech();
        if (cmdRes != CommandResult::OK)
            return CommandResult::ERROR;
    }
    else if (_MBTYPE == 0x64) // else Micronova not implemented
        return CommandResult::UNSUPPORTED;
    else
        return CommandResult::ERROR;

    // open /etc/appliancelabel
    // if open /etc/appliancelabel is OK Then
    //// _LABEL=0; //to empty current label
    //// read 0x1F char from /etc/appliancelabel into &_LABEL
    //// close etc/appliancelabel
    // Else
    //// run 'touch /etc/appliancelabel'

    staticDataLoaded = 1; // flag that indicates Static Data are loaded

    // sendmsg 'GET STDT'

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iUpdateStaticDataAtech()
{
    iGetSNAtech();

    uint16_t nbTotalBytesReaded = 0; // var_838
    uint8_t buf[8];                  // var_820
    CommandResult cmdRes;            // var_830

    while (nbTotalBytesReaded < paramsBufferSize)
    {
        cmdRes = fumisComReadBuff(0x1C00 + nbTotalBytesReaded, buf, 8);
        if (cmdRes != CommandResult::OK)
            return cmdRes;

        for (uint8_t i = 0; i < 8 && nbTotalBytesReaded < paramsBufferSize; i++)
        {
            _PARAMS[nbTotalBytesReaded] = buf[i];
            nbTotalBytesReaded++;
        }
    }
    ///*read all params OK*/

    nbTotalBytesReaded = 0; // var_838
    while (nbTotalBytesReaded < hparamsBufferSize)
    {
        cmdRes = fumisComReadBuff((nbTotalBytesReaded + 0xF00) * 2, buf, 8);
        if (cmdRes != CommandResult::OK)
            return cmdRes;

        for (uint8_t i = 0; i < 8 && nbTotalBytesReaded < hparamsBufferSize; i += 2)
        {
            // not nbTotalBytesReaded * 2 because _HPARAMS is uint16_t[] type
            _HPARAMS[nbTotalBytesReaded] = buf[i + 1];
            _HPARAMS[nbTotalBytesReaded] <<= 8;
            _HPARAMS[nbTotalBytesReaded] |= buf[i];
            nbTotalBytesReaded++;
        }
    }
    ///*read all hparams OK*/

    _VER = _HPARAMS[4 / 2];
    _MOD = _HPARAMS[6 / 2];
    _CORE = _HPARAMS[8 / 2];
    _FWDATED = _HPARAMS[0xA / 2];
    _FWDATEM = _HPARAMS[0xC / 2];
    _FWDATEY = _HPARAMS[0xE / 2];
    // pdword_471C88 = _HPARAMS[0x1E / 2]; //Not Used
    // pdword_471C7C = _HPARAMS[0x88 / 2]; //Not Used
    // pdword_471C80 = _HPARAMS[0x8C / 2]; //Not Used
    // pdword_471C84 = _HPARAMS[0x8E / 2]; //Not Used

    cmdRes = iGetStoveConfigurationAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MOD < 500 || 599 < _MOD)
    {
        if (_VER < 0x1F)
            wAddrFeederActiveTime = 0x1FAE;
        else if (_VER < 0x28)
            wAddrFeederActiveTime = 0x1FAC;
        else
            wAddrFeederActiveTime = 0x209A;

        uint16_t var_18;
        cmdRes = fumisComReadWord(0x203A, &var_18);
        if (cmdRes != CommandResult::OK)
            return cmdRes;

        _PSENSTYPE = 0;

        if (0 < (_HPARAMS[(_UICONFIG + 0xD) * 4 / 2] & 0x10))
            _PSENSTYPE = 2;
        else if (1 < var_18)
            _PSENSTYPE = 1;
    }
    else
        wAddrFeederActiveTime = 0x209A;

    nbTotalBytesReaded = 0; // var_838
    while (nbTotalBytesReaded < paramsBufferSize)
    {
        cmdRes = fumisComReadBuff((nbTotalBytesReaded + 0x80A2), buf, 8);
        if (cmdRes != CommandResult::OK)
            return cmdRes;

        for (uint8_t i = 0; i < 8 && nbTotalBytesReaded < paramsBufferSize; i++)
        {
            _LIMMIN[nbTotalBytesReaded] = buf[i];
            nbTotalBytesReaded++;
        }
    }

    nbTotalBytesReaded = 0; // var_838
    while (nbTotalBytesReaded < paramsBufferSize)
    {
        cmdRes = fumisComReadBuff((nbTotalBytesReaded + 0x810C), buf, 8);
        if (cmdRes != CommandResult::OK)
            return cmdRes;

        for (uint8_t i = 0; i < 8 && nbTotalBytesReaded < paramsBufferSize; i++)
        {
            _LIMMAX[nbTotalBytesReaded] = buf[i];
            nbTotalBytesReaded++;
        }
    }

    if (_FLUID < 2)
    {
        _SPLMIN = _LIMMIN[0x33];
        _SPLMAX = _LIMMAX[0x33];
        if (!_FLUID)
        {
            _SPLMIN = (uint8_t)((double)_SPLMIN / 5.0);
            _SPLMAX = (uint8_t)((double)_SPLMAX / 5.0);
        }
    }
    else if (_FLUID == 2)
    {
        _SPLMIN = _LIMMIN[0x54];
        _SPLMAX = _LIMMAX[0x54];
    }

    ///*iGetLimitsAtech OK*/

    // JSON not build and so not saved in /tmp/appliance_params.json

    ///*iUpdateStaticDataAtech OK*/

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iWriteDataAtech(uint16_t addrToWrite, uint16_t data, bool wordMode)
{
    if (!wordMode)
        return fumisComWriteByte(addrToWrite, data & 0xFF);
    else
        return fumisComWriteWord(addrToWrite, data);
}

//------------------------------------------
// Public part

Palazzetti::CommandResult Palazzetti::initialize(bool loopBack /* = false*/)
{
    if (!m_serial.open ||
        !m_serial.close ||
        !m_serial.select ||
        !m_serial.read ||
        !m_serial.write ||
        !m_serial.drain ||
        !m_serial.flush ||
        !m_serial.uSleep)
    {
        return CommandResult::ERROR;
    }

    if (_isInitialized)
        return CommandResult::OK;

    // comPortNumber affects loopBack processing
    comPortNumber = loopBack ? 0 : 1;

    CommandResult cmdRes = iChkMBType();
    if (cmdRes != CommandResult::OK)
    {
        iCloseUART();
        return cmdRes;
    }

    cmdRes = iInit();
    if (cmdRes != CommandResult::OK)
    {
        iCloseUART();
        return cmdRes;
    }

    cmdRes = iUpdateStaticData();
    if (cmdRes != CommandResult::OK)
    {
        iCloseUART();
        return cmdRes;
    }

    _isInitialized = true;
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::initialize(SerialAdapter serial, bool loopBack /* = false*/)
{
    m_serial = serial;
    return initialize(loopBack);
}

Palazzetti::CommandResult Palazzetti::getAllHiddenParameters(uint16_t (*hiddenParams)[0x6F])
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    cmdRes = iUpdateStaticDataAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    memcpy(*hiddenParams, _HPARAMS, 0x6F * sizeof(uint16_t));

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getAllParameters(uint8_t (*params)[0x6A])
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    cmdRes = iUpdateStaticDataAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    memcpy(*params, _PARAMS, 0x6A * sizeof(uint8_t));

    return CommandResult::OK;
}

// refreshStatus shoud be true if last call is over ~15sec
Palazzetti::CommandResult Palazzetti::getAllStatus(bool refreshStatus, AllStatusData &out)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    cmdRes = iGetAllStatus(refreshStatus);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    out.MBTYPE = _MBTYPE;
    // MAC not needed
    out.MOD = _MOD;
    out.VER = _VER;
    out.CORE = _CORE;
    snprintf(out.FWDATE, sizeof(out.FWDATE), "%04u-%02u-%02u", _FWDATEY % 10000u, _FWDATEM % 100u, _FWDATED % 100u);
    strcpy(out.APLTS, _STOVE_DATETIME);
    out.APLWDAY = _STOVE_WDAY;
    out.CHRSTATUS = _CHRSTATUS;
    out.STATUS = _STATUS;
    out.LSTATUS = _LSTATUS;
    if (_STOVETYPE == 3 || _STOVETYPE == 4)
    {
        out.isMFSTATUSValid = true;
        out.MFSTATUS = _MFSTATUS;
    }
    else
        out.isMFSTATUSValid = false;
    out.SETP = _SETP;
    out.PUMP = _PUMP;
    out.PQT = _PQT;
    out.F1V = _F1V;
    out.F1RPM = _F1RPM;
    out.F2L = transcodeRoomFanSpeed(_F2L, true);
    uint16_t tmp = transcodeRoomFanSpeed(_F2L, true);
    out.F2LF = (tmp < 6) ? 0 : tmp - 5;
    iGetFanLimits();
    out.FANLMINMAX[0] = _FAN2LMIN;
    out.FANLMINMAX[1] = _FAN2LMAX;
    out.FANLMINMAX[2] = _FAN3LMIN;
    out.FANLMINMAX[3] = _FAN3LMAX;
    out.FANLMINMAX[4] = _FAN4LMIN;
    out.FANLMINMAX[5] = _FAN4LMAX;
    out.F2V = _F2V;
    if (_FAN2TYPE > 2)
    {
        out.isF3LF4LValid = true;
        out.F3L = _F3L;
        out.F4L = _F4L;
    }
    else
        out.isF3LF4LValid = false;
    out.PWR = _PWR;
    out.FDR = _FDR;
    out.DPT = _DP_TARGET;
    out.DP = _DP_PRESS;
    out.IN = _IN_I04 << 3 | _IN_I03 << 2 | _IN_I02 << 1 | _IN_I01;
    out.OUT = _OUT_O07 << 6 | _OUT_O06 << 5 | _OUT_O05 << 4 | _OUT_O04 << 3 | _OUT_O03 << 2 | _OUT_O02 << 1 | _OUT_O01;
    out.T1 = _T1;
    out.T2 = _T2;
    out.T3 = _T3;
    out.T4 = _T4;
    out.T5 = _T5;
    if (isValidSerialNumber(_SN))
    {
        out.isSNValid = true;
        strcpy(out.SN, _SN);
    }
    else
        out.isSNValid = false;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getAllTemps(AllTempsData &out)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    cmdRes = iReadTemperatureAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    out.T1 = _T1;
    out.T2 = _T2;
    out.T3 = _T3;
    out.T4 = _T4;
    out.T5 = _T5;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getChronoData(ChronoData &out)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iGetChronoDataAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    out.CHRSTATUS = _CHRSTATUS; // original code should have been "chronoDataStatus > 0 ? 1 : 0" but _CHRSTATUS is readily available...
    for (uint8_t i = 0; i < 6; i++)
    {
        out.PCHRSETP[i] = chronoDataPrograms[i].CHRSETP;
        out.PSTART[i][0] = chronoDataPrograms[i].STARTH;
        out.PSTART[i][1] = chronoDataPrograms[i].STARTM;
        out.PSTOP[i][0] = chronoDataPrograms[i].STOPH;
        out.PSTOP[i][1] = chronoDataPrograms[i].STOPM;
    }
    for (uint8_t i = 0; i < 7; i++)
    {
        out.DM[i][0] = chronoDataDays[i].M1;
        out.DM[i][1] = chronoDataDays[i].M2;
        out.DM[i][2] = chronoDataDays[i].M3;
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getCounters(CountersData &out)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iGetCountersAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    out.IGN = _IGN;
    out.POWERTIMEh = _POWERTIMEH;
    out.POWERTIMEm = _POWERTIMEM;
    out.HEATTIMEh = _HEATTIMEH;
    out.HEATTIMEm = _HEATTIMEM;
    out.SERVICETIMEh = _SERVICETIMEH;
    out.SERVICETIMEm = _SERVICETIMEM;
    out.ONTIMEh = _ONTIMEH;
    out.ONTIMEm = _ONTIMEM;
    out.OVERTMPERRORS = _OVERTMPERRORS;
    out.IGNERRORS = _IGNERRORS;
    out.PQT = _PQT;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getDateTime(DateTimeData &out)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iGetDateTimeAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    strcpy(out.STOVE_DATETIME, _STOVE_DATETIME);
    out.STOVE_WDAY = _STOVE_WDAY;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getDPressData(DPressData &out)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iGetDPressDataAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    out.DP_TARGET = _DP_TARGET;
    out.DP_PRESS = _DP_PRESS;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getFanData(FanData &out)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iReadFansAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    out.F1V = _F1V;
    out.F2V = _F2V;
    out.F1RPM = _F1RPM;
    out.F2L = transcodeRoomFanSpeed(_F2L, true);
    uint16_t tmp = transcodeRoomFanSpeed(_F2L, true);
    out.F2LF = (tmp < 6) ? 0 : tmp - 5;
    if (_BLEMBMODE > 0xC)
    {
        out.isF3SF4SValid = true;
        out.F3S = _F3S;
        out.F4S = _F4S;
    }
    else
        out.isF3SF4SValid = false;

    // Custom convenient code
    if (_FAN2TYPE > 2)
    {
        out.isF3LF4LValid = true;
        out.F3L = _F3L;
        out.F4L = _F4L;
    }
    else
        out.isF3LF4LValid = false;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getHiddenParameter(uint8_t hParamNumber, uint16_t *hParamValue)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE == 0x64) // if Micronova
        return CommandResult::UNSUPPORTED;

    cmdRes = iGetHiddenParameterAtech(hParamNumber, hParamValue);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getIO(IOData &out)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iReadIOAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    out.IN_I01 = _IN_I01;
    out.IN_I02 = _IN_I02;
    out.IN_I03 = _IN_I03;
    out.IN_I04 = _IN_I04;
    out.OUT_O01 = _OUT_O01;
    out.OUT_O02 = _OUT_O02;
    out.OUT_O03 = _OUT_O03;
    out.OUT_O04 = _OUT_O04;
    out.OUT_O05 = _OUT_O05;
    out.OUT_O06 = _OUT_O06;
    out.OUT_O07 = _OUT_O07;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getModelVersion(ModelVersionData &out)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    out.MOD = _MOD;
    out.VER = _VER;
    out.CORE = _CORE;
    snprintf(out.FWDATE, sizeof(out.FWDATE), "%04u-%02u-%02u", _FWDATEY % 10000u, _FWDATEM % 100u, _FWDATED % 100u);

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getParameter(uint8_t paramNumber, uint8_t *paramValue)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    uint16_t tmpValue;

    cmdRes = iGetParameterAtech(paramNumber, &tmpValue);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    // convert uint16_t to uint8_t
    *paramValue = tmpValue;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getPelletQtUsed(uint16_t *PQT)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    cmdRes = iGetPelletQtUsedAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (PQT)
        *PQT = _PQT;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getPower(PowerData &out)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    cmdRes = iGetPowerAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    out.PWR = _PWR;
    out.FDR = _FDR;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getSetPoint(float *setPoint)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    cmdRes = iGetSetPointAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (setPoint)
        *setPoint = _SETP;
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getSN(char (*SN)[28])
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (SN)
        strcpy(*SN, _SN);
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getStaticData(StaticData &out)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (!staticDataLoaded)
    {
        cmdRes = iUpdateStaticData();
        if (cmdRes != CommandResult::OK)
            return cmdRes;
    }

    // read LABEL : not needed
    // get network infos by running "lua /www/cgi-bin/syscmd.lua \"netdata\" > /dev/null" : not needed

    strcpy(out.SN, _SN);
    out.SNCHK = isValidSerialNumber(_SN);
    out.MBTYPE = _MBTYPE;

    // APLCONN : useless because overwritten by CBox lua code

    out.MOD = _MOD;
    out.VER = _VER;
    out.CORE = _CORE;
    snprintf(out.FWDATE, sizeof(out.FWDATE), "%04u-%02u-%02u", _FWDATEY % 10000u, _FWDATEM % 100u, _FWDATED % 100u);
    out.FLUID = _FLUID;
    out.SPLMIN = _SPLMIN;
    out.SPLMAX = _SPLMAX;
    out.UICONFIG = _UICONFIG;
    out.HWTYPE = _HWTYPE;
    out.DSPTYPE = _DSPTYPE;
    out.DSPFWVER = _DSPFWVER;
    out.CONFIG = _PARAMS[0x4C];
    out.PELLETTYPE = _PARAMS[0x5C];
    out.PSENSTYPE = _PSENSTYPE;
    out.PSENSLMAX = _PARAMS[0x62];
    out.PSENSLTSH = _PARAMS[0x63];
    out.PSENSLMIN = _PARAMS[0x64];
    out.MAINTPROBE = _MAINTPROBE;
    out.STOVETYPE = _STOVETYPE;
    out.FAN2TYPE = _FAN2TYPE;
    out.FAN2MODE = _FAN2MODE;
    out.BLEMBMODE = _BLEMBMODE;
    out.BLEDSPMODE = _BLEDSPMODE;
    out.CHRONOTYPE = 5; // hardcoded value
    out.AUTONOMYTYPE = _AUTONOMYTYPE;
    out.NOMINALPWR = _NOMINALPWR;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getStatus(StatusData &out)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    cmdRes = iGetStatusAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    out.STATUS = _STATUS;
    out.LSTATUS = _LSTATUS;
    out.FSTATUS = _FSTATUS;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::readData(uint16_t addrToRead, bool wordMode, uint16_t *ADDR_DATA)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    return iReadDataAtech(addrToRead, ADDR_DATA, wordMode);
}

Palazzetti::CommandResult Palazzetti::setChronoDay(uint8_t dayNumber, uint8_t memoryNumber, uint8_t programNumber)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iSetChronoDayAtech(dayNumber, memoryNumber, programNumber);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setChronoPrg(uint8_t programNumber, uint8_t setPoint, uint8_t startHour, uint8_t startMinute, uint8_t stopHour, uint8_t stopMinute)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    uint8_t prg[6] = {programNumber, setPoint, startHour, startMinute, stopHour, stopMinute};
    cmdRes = iSetChronoPrgAtech(prg);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setChronoSetpoint(uint8_t programNumber, uint8_t setPoint)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iSetChronoSetpointAtech(programNumber, setPoint);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setChronoStartHH(uint8_t programNumber, uint8_t startHour)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iSetChronoStartHHAtech(programNumber, startHour);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setChronoStartMM(uint8_t programNumber, uint8_t startMinute)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iSetChronoStartMMAtech(programNumber, startMinute);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setChronoStatus(bool chronoStatus, uint8_t *CHRSTATUSReturn /* = nullptr */)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iSetChronoStatusAtech(chronoStatus);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (CHRSTATUSReturn)
        *CHRSTATUSReturn = chronoStatus;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setChronoStopHH(uint8_t programNumber, uint8_t stopHour)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iSetChronoStopHHAtech(programNumber, stopHour);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setChronoStopMM(uint8_t programNumber, uint8_t stopMinute)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iSetChronoStopMMAtech(programNumber, stopMinute);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, DateTimeData *result /* = nullptr */)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = fumisComSetDateTime(year, month, day, hour, minute, second);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (result)
    {
        strcpy(result->STOVE_DATETIME, _STOVE_DATETIME);
        result->STOVE_WDAY = _STOVE_WDAY;
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setHiddenParameter(uint8_t hParamNumber, uint16_t hParamValue)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE >= 2) // if Micronova
        return CommandResult::UNSUPPORTED;

    cmdRes = iSetHiddenParameterAtech(hParamNumber, hParamValue);
    if (cmdRes != CommandResult::OK)
        return cmdRes;
    else if (hParamNumber < hparamsBufferSize)
    {
        _HPARAMS[hParamNumber] = hParamValue;
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setParameter(uint8_t paramNumber, uint8_t paramValue)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iSetParameterAtech(paramNumber, paramValue);
    if (cmdRes != CommandResult::OK)
        return cmdRes;
    else if (paramNumber < paramsBufferSize)
    {
        _PARAMS[paramNumber] = paramValue;
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setPower(uint8_t powerLevel, SetPowerResult *result /* = nullptr */)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iSetPowerAtech(powerLevel);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (byte_471CC2)
    {
        cmdRes = iGetRoomFanAtech();
        if (cmdRes != CommandResult::OK)
            return cmdRes;
    }

    if (result)
    {
        result->PWR = _PWR;
        if (byte_471CC2)
        {
            result->isF2LValid = true;
            result->F2L = transcodeRoomFanSpeed(_F2L, true);
        }
        else
            result->isF2LValid = false;
    }

    // if (_MBTYPEMicronova != 0xB) //Micronova MBTYPE always equals 0
    iGetFanLimits();

    if (result)
    {
        result->FANLMINMAX[0] = _FAN2LMIN;
        result->FANLMINMAX[1] = _FAN2LMAX;
        result->FANLMINMAX[2] = _FAN3LMIN;
        result->FANLMINMAX[3] = _FAN3LMAX;
        result->FANLMINMAX[4] = _FAN4LMIN;
        result->FANLMINMAX[5] = _FAN4LMAX;
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setPowerDown(SetPowerResult *result /* = nullptr */)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iGetPowerAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return setPower(_PWR - 1, result);
}

Palazzetti::CommandResult Palazzetti::setPowerUp(SetPowerResult *result /* = nullptr */)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iGetPowerAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return setPower(_PWR + 1, result);
}

Palazzetti::CommandResult Palazzetti::setRoomFan(uint8_t roomFanSpeed, SetRoomFanResult *result /* = nullptr */)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iSetRoomFanAtech(transcodeRoomFanSpeed(roomFanSpeed, 0));
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (result)
    {
        if (byte_471CC2)
        {
            result->isPWRValid = true;
            result->PWR = _PWR;
        }
        else
            result->isPWRValid = false;
        result->F2L = transcodeRoomFanSpeed(_F2L, true);
        uint16_t tmp = transcodeRoomFanSpeed(_F2L, true);
        result->F2LF = (tmp < 6) ? 0 : tmp - 5;
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setRoomFan3(uint8_t roomFan3Speed, uint16_t *F3LReturn /* = nullptr */)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iSetRoomFan3Atech(roomFan3Speed);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (F3LReturn)
        *F3LReturn = _F3L;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setRoomFan4(uint8_t roomFan4Speed, uint16_t *F4LReturn /* = nullptr */)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iSetRoomFan4Atech(roomFan4Speed);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (F4LReturn)
        *F4LReturn = _F4L;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setRoomFanDown(SetRoomFanResult *result /* = nullptr */)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iReadFansAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return setRoomFan(transcodeRoomFanSpeed(_F2L, true) - 1, result);
}

Palazzetti::CommandResult Palazzetti::setRoomFanUp(SetRoomFanResult *result /* = nullptr */)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iReadFansAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return setRoomFan(transcodeRoomFanSpeed(_F2L, true) + 1, result);
}

Palazzetti::CommandResult Palazzetti::setSetpoint(uint8_t setPoint, float *SETPReturn /* = nullptr */)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iSetSetPointAtech(setPoint);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (SETPReturn)
        *SETPReturn = _SETP;
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setSetpoint(float setPoint, float *SETPReturn /* = nullptr */)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iSetSetPointAtech(setPoint);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (SETPReturn)
        *SETPReturn = _SETP;
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setSetPointDown(float *SETPReturn /* = nullptr */)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iGetSetPointAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return setSetpoint(_SETP - 1.0f, SETPReturn);
}

Palazzetti::CommandResult Palazzetti::setSetPointUp(float *SETPReturn /* = nullptr */)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iGetSetPointAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    return setSetpoint(_SETP + 1.0f, SETPReturn);
}

Palazzetti::CommandResult Palazzetti::setSilentMode(uint8_t silentMode, SetSilentModeResult *result /* = nullptr */)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    cmdRes = iSetSilentModeAtech(silentMode);
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (result)
    {
        result->SLNT = silentMode;
        result->PWR = _PWR;
        result->F2L = transcodeRoomFanSpeed(_F2L, true);
        uint16_t tmp = transcodeRoomFanSpeed(_F2L, true);
        result->F2LF = (tmp < 6) ? 0 : tmp - 5;
        if (_FAN2TYPE > 2)
        {
            result->isF3LF4LValid = true;
            result->F3L = _F3L;
            result->F4L = _F4L;
        }
        else
            result->isF3LF4LValid = false;
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::switchOff(StatusData *result /* = nullptr */)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    cmdRes = iSwitchOffAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    // give the stove time to switch off and reach the final status
    m_serial.uSleep(200000); // maximum measured time is 89ms (from STATUS=9 to STATUS=0)

    StatusData status;
    cmdRes = getStatus(status);
    if (result)
        *result = status;
    return cmdRes;
}

Palazzetti::CommandResult Palazzetti::switchOn(StatusData *result /* = nullptr */)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    cmdRes = iSwitchOnAtech();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    // give the stove time to switch on and reach the "final" status
    // (if stove need to heat up, it stays on STATUS=3 for more than 3.6s)
    // (otherwise it switch to STATUS=9 in less than 0.5s)
    m_serial.uSleep(750000); // maximum measured time is 305ms (from STATUS=0 to STATUS=9)

    StatusData status;
    cmdRes = getStatus(status);
    if (result)
        *result = status;
    return cmdRes;
}

Palazzetti::CommandResult Palazzetti::writeData(uint16_t addrToWrite, uint16_t data, bool wordMode)
{
    CommandResult cmdRes = initialize();
    if (cmdRes != CommandResult::OK)
        return cmdRes;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return CommandResult::UNSUPPORTED;

    return iWriteDataAtech(addrToWrite, data, wordMode);
    // the original code return the data untouched (even if its value is over 255 and wordMode is false...)
    // considered as useless because CommandResult is enough to know if the command was successful
}

//------------------------------------------
// Constructor
Palazzetti::Palazzetti() {}
