#include "Palazzetti.h"

// evaluates a CommandResult expression and returns it from the calling function if it is not OK
#define RETURN_IF_ERROR(expr)             \
    do                                    \
    {                                     \
        CommandResult cmdRes_ = (expr);   \
        if (cmdRes_ != CommandResult::OK) \
            return cmdRes_;               \
    } while (0)

// The original SERIALCOM_* layer, the frame part of the fumis* functions and their retries are in FumisLink (serial layer)
// the fumisCom* functions below add what the values mean: date checks, the value expected back after a write

// FumisLink results as CommandResult: the bus errors (Timeout, BadFrame, Collision) are COMMUNICATION_ERROR, Error and Mismatch are ERROR
Palazzetti::CommandResult Palazzetti::fumisResult(FumisLink::Result res)
{
    switch (res)
    {
    case FumisLink::Result::Ok:
        return CommandResult::OK;
    case FumisLink::Result::Error:
    case FumisLink::Result::Mismatch:
        return CommandResult::ERROR;
    default:
        return CommandResult::COMMUNICATION_ERROR;
    }
}

bool Palazzetti::isValidDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
    // basic control
    if (year < 2000 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || minute > 59 || second > 59)
        return false;
    // 30 days month control
    if ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30)
        return false;
    // February leap year control
    if (month == 2 && day > 29)
        return false;
    // February not leap year control
    if (month == 2 && day == 29 && !(((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0)))
        return false;
    return true;
}

uint8_t Palazzetti::isoWeekDay(uint16_t year, uint8_t month, uint8_t day)
{
    // Tomohiko Sakamoto's Algorithm (0 = Sunday), then Sunday is turned into 7
    static const uint8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (month < 3)
        year -= 1;
    uint8_t weekDay = (year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % 7;
    return weekDay ? weekDay : 7;
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

uint16_t Palazzetti::transcodeRoomFanSpeed(uint16_t roomFanSpeed, bool decode)
{
    if (roomFanSpeed == 0)
        return 7;
    if (roomFanSpeed == 7)
        return 0;

    return roomFanSpeed;
}

// firmware date as "YYYY-MM-DD"
void Palazzetti::fillFwDate(char (&FWDATE)[11])
{
    sprintf(FWDATE, "%d-%02d-%02d", _FWDATEY % 10000u, _FWDATEM % 100u, _FWDATED % 100u);
}

// (the retries of the original are now in FumisLink::read)
// buf is zeroed on failure (original: buf was a void *)
Palazzetti::CommandResult Palazzetti::fumisComReadBuff(uint16_t addrToRead, uint8_t (&buf)[FumisFrame::DATA_SIZE])
{
    return fumisResult(_link.read(addrToRead, buf));
}

// fumisComReadByte and fumisComReadWord read the first 1 or 2 bytes of the Response (little endian), data is unchanged on failure
// (original: both called fumisComRead(addr, data, wordMode), now FumisLink::readByte and readWord)
Palazzetti::CommandResult Palazzetti::fumisComReadByte(uint16_t addrToRead, uint16_t *data)
{
    return fumisResult(_link.readByte(addrToRead, *data));
}

Palazzetti::CommandResult Palazzetti::fumisComReadWord(uint16_t addrToRead, uint16_t *data)
{
    return fumisResult(_link.readWord(addrToRead, *data));
}

Palazzetti::CommandResult Palazzetti::fumisComSetDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
    // The content of this function does not match strictly the original one

    if (!isValidDateTime(year, month, day, hour, minute, second))
        return CommandResult::ERROR;

    uint8_t weekDay = isoWeekDay(year, month, day);

    // (the retries of the original are now in FumisLink::setDateTime)
    CommandResult cmdRes = fumisResult(_link.setDateTime(second, minute, hour, weekDay, day, month, year - 2000)); // local_88

    // values are already checked by isValidDateTime; the modulos only let the compiler see that the text fits in 20 bytes
    sprintf(_STOVE_DATETIME, "%d-%02d-%02d %02d:%02d:%02d", year % 10000u, month % 100u, day % 100u, hour % 100u, minute % 100u, second % 100u);
    _STOVE_WDAY = weekDay;

    return cmdRes;
}

// (original: fumisComWrite, now FumisLink::writeByte and writeWord with their read-back and retries)
Palazzetti::CommandResult Palazzetti::fumisComWriteByte(uint16_t addrToWrite, uint16_t data)
{
    // Command ON/OFF (0x2044): the stove executes the command then clears the word, so a consumed command reads back as 0
    uint8_t expected = (addrToWrite == 0x2044) ? 0 : (data & 0xFF);
    return fumisResult(_link.writeByte(addrToWrite, data & 0xFF, expected));
}

Palazzetti::CommandResult Palazzetti::fumisComWriteWord(uint16_t addrToWrite, uint16_t data)
{
    return fumisResult(_link.writeWord(addrToWrite, data));
}

Palazzetti::CommandResult Palazzetti::iGetChronoDataAtech()
{
    uint8_t CHRSETPList[8];
    RETURN_IF_ERROR(fumisComReadBuff(0x802D, CHRSETPList));

    uint16_t addrToRead = 0x8000;
    uint8_t programTimes[8];
    for (uint8_t i = 0; i < 6; i++)
    {
        RETURN_IF_ERROR(fumisComReadBuff(addrToRead, programTimes));
        _chronoDataPrograms[i].STARTH = programTimes[0];
        _chronoDataPrograms[i].STARTM = programTimes[1];
        _chronoDataPrograms[i].STOPH = programTimes[2];
        _chronoDataPrograms[i].STOPM = programTimes[3];
        _chronoDataPrograms[i].CHRSETP = CHRSETPList[i];
        if (!_FLUID)
            _chronoDataPrograms[i].CHRSETP /= 5.0;

        addrToRead += 4;
    }

    addrToRead = 0x8018;
    uint8_t dayPrograms[8];
    for (uint8_t i = 0; i < 7; i++)
    {
        RETURN_IF_ERROR(fumisComReadBuff(addrToRead, dayPrograms));

        _chronoDataDays[i].M1 = dayPrograms[0];
        _chronoDataDays[i].M2 = dayPrograms[1];
        _chronoDataDays[i].M3 = dayPrograms[2];

        addrToRead += 3;
    }

    RETURN_IF_ERROR(fumisComReadWord(0x207e, &_chronoDataStatus));
    _CHRSTATUS = _chronoDataStatus & 0x01;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iGetDateTimeAtech()
{
    uint8_t buf[8]; // var_14
    RETURN_IF_ERROR(fumisComReadBuff(0x204E, buf));

    sprintf(_STOVE_DATETIME, "%d-%02d-%02d %02d:%02d:%02d", ((uint16_t)buf[6] + 2000) % 10000u, buf[5] % 100u, buf[4] % 100u, buf[2] % 100u, buf[1] % 100u, buf[0] % 100u);

    _STOVE_WDAY = buf[3];

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iGetDPressDataAtech()
{
    uint16_t buf; // var_10
    RETURN_IF_ERROR(fumisComReadWord(0x2000, &buf));

    _DP_TARGET = buf;

    RETURN_IF_ERROR(fumisComReadWord(0x2020, &buf));

    _DP_PRESS = (int16_t)buf;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iGetErrorFlagAtech()
{
    uint8_t t1Err, t2Err, t3Err, t4Err, t5Err;

    if (_T1 <= 0.0f || (_T1 > 5.0f && _T1 <= 90.0f))
        t1Err = 0;
    else
        t1Err = 1;
    if (_T2 <= 0.0f || (_T2 > 5.0f && _T2 <= 90.0f))
        t2Err = 0;
    else
        t2Err = 1;

    uint16_t eFlags = t1Err + (t2Err * 2);

    if (_STOVETYPE == 3 || _STOVETYPE == 4)
    {
        _EFLAGS = eFlags;
        return CommandResult::OK;
    }

    if (_T3 <= 0.0f || (_T3 > 5.0f && _T3 <= (float)_PARAMS[0x37] * 3.0f))
        t3Err = 0;
    else
        t3Err = 1;

    if (_T4 <= 0.0f || (_T4 > 5.0f && _T4 <= 650.0f))
        t4Err = 0;
    else
        t4Err = 1;

    if (_T5 <= 0.0f || (_T5 > 5.0f && _T5 <= 90.0f))
        t5Err = 0;
    else
        t5Err = 1;

    eFlags = eFlags + (t3Err * 4) + (t4Err * 8) + (t5Err * 16);

    if (_DP_PRESS > 0)
    {
        float dpTarget = (float)_DP_TARGET;
        float dpPress = (float)_DP_PRESS;
        float dpTolerance = dpTarget * ((float)_PARAMS[0x59] / 100.0f);

        if (dpPress > (dpTarget - dpTolerance) && dpPress < (dpTarget + dpTolerance))
        {
            _EFLAGS = eFlags;
            return CommandResult::OK;
        }

        if (dpTarget > 0.0f)
            eFlags = eFlags + 0x0400; // bit 10: DP pressure out of tolerance
    }

    _EFLAGS = eFlags;
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

Palazzetti::CommandResult Palazzetti::iGetMBTypeAtech()
{
    uint16_t buf;         // var_C
    RETURN_IF_ERROR(fumisComReadWord(0x204C, &buf));

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

Palazzetti::CommandResult Palazzetti::iGetPelletLevelAtech()
{
    if (_PSENSTYPE == 1)
    {
        RETURN_IF_ERROR(fumisComReadWord(0x203A, &_PLEVEL)); // direct read to the variable...
    }

    if (_PSENSTYPE == 2)
    {
        uint16_t local_14;
        RETURN_IF_ERROR(fumisComReadByte(0x1E26, &local_14));
        _PSENSCSTA = (local_14 & 0x80) ? 0 : 1;

        if (_HWTYPE == 5)
        {
            uint16_t local_10;
            RETURN_IF_ERROR(fumisComReadByte(0x2030, &local_10));
            if (local_10 > 2)
                _PSENSLEMP = 0;
            else
                _PSENSLEMP = 1;
        }
        else
        {
            uint16_t local_10;
            RETURN_IF_ERROR(fumisComReadWord(0x2014, &local_10));
            if (local_10 < 220 || local_10 > 512)
                _PSENSLEMP = 0;
            else
                _PSENSLEMP = 1;
        }
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iGetPelletQtUsedAtech()
{
    uint16_t var_10;
    RETURN_IF_ERROR(fumisComReadWord(0x2002, &var_10));
    _PQT = var_10;
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iGetPowerAtech()
{
    uint16_t var_C;

    RETURN_IF_ERROR(fumisComReadWord(0x202a, &var_C));

    _PWR = var_C & 0xFF;

    RETURN_IF_ERROR(fumisComReadWord(_wAddrFeederActiveTime, &var_C));

    _FDR = var_C / 10.0f;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iGetPumpRateAtech()
{
    uint16_t buf;
    RETURN_IF_ERROR(fumisComReadWord(0x2090, &buf));

    _PUMP = buf & 0xFF;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iGetRoomFanAtech()
{
    uint16_t var_C;
    RETURN_IF_ERROR(fumisComReadByte(0x2036, &var_C));
    _F2L = var_C;

    RETURN_IF_ERROR(fumisComReadWord(0x2004, &var_C));

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
        RETURN_IF_ERROR(fumisComReadBuff(0x1C32, buf));
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
        RETURN_IF_ERROR(fumisComReadByte(0x1C54, &data));
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
        cmdRes = fumisComReadBuff(0x2100 + currentPosInSN, buf);
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
    cmdRes = fumisComReadBuff(0x2100 + currentPosInSN, buf);
    if (cmdRes != CommandResult::OK)
    {
        _SN[0] = 0;
        return cmdRes;
    }
    if (buf[0] == (uint8_t)-checkSum)
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
            memmove(_SN + 2, _SN + 4, strlen(_SN + 4) + 1); // (original: strcpy, undefined when the strings overlap)
            _SN[23] = 0;
        }
        return CommandResult::OK;
    }

    _SN[0] = 0;
    return CommandResult::ERROR;
}

Palazzetti::CommandResult Palazzetti::iGetStatusAtech()
{
    RETURN_IF_ERROR(fumisComReadByte(0x201C, &_STATUS));

    _LSTATUS = _STATUS;

    if (200 < _STATUS)
    {
        _LSTATUS = _STATUS + 1000;
        RETURN_IF_ERROR(fumisComReadByte(0x2081, &_FSTATUS));
    }

    if (_STOVETYPE == 3 || _STOVETYPE == 4)
    {
        RETURN_IF_ERROR(fumisComReadWord(0x2008, &_MFSTATUS));
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
    RETURN_IF_ERROR(fumisComReadByte(0x2006, &buf));
    _DSPFWVER = buf;
    if (_DSPFWVER > 0)
        _DSPTYPE = (double)buf / pow(10, (int)log10(buf));

    RETURN_IF_ERROR(iGetMBTypeAtech());

    _CHRONOTYPE = 2;
    _MAINTPROBE = 0;

    RETURN_IF_ERROR(fumisComReadWord(0x1ED4, &buf));
    uint8_t buf2[8]; // local_28
    RETURN_IF_ERROR(fumisComReadBuff(0x1E25, buf2));

    _NOMINALPWR = buf2[0];
    if (buf2[1] & 2)
        _AUTONOMYTYPE = 2;
    else
        _AUTONOMYTYPE = 1;

    uint8_t bVar1 = _PARAMS[0x4C];
    _UICONFIG = bVar1;

    RETURN_IF_ERROR(fumisComReadBuff(((bVar1 - 1) * 4) + 0x1E36, buf2));

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
        RETURN_IF_ERROR(fumisComReadBuff(0x1E36, buf2));

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

Palazzetti::CommandResult Palazzetti::iReadFansAtech()
{
    uint8_t buf[8];
    CommandResult cmdRes = fumisComReadBuff(0x2024, buf);
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
    RETURN_IF_ERROR(fumisComReadBuff(0x203c, buf));

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
    uint8_t buf[8];       // var_14
    uint16_t conv = 0;
    RETURN_IF_ERROR(fumisComReadBuff(0x200A, buf));

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
    RETURN_IF_ERROR(fumisComReadWord(0x2012, &var_18));

    _T5 = (int16_t)var_18;
    _T5 /= 10.0f;
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iSetPowerAtech(uint16_t powerLevel)
{
    if (powerLevel < 1 || powerLevel > 5)
        return CommandResult::ERROR;

    RETURN_IF_ERROR(fumisComWriteByte(0x202a, powerLevel));

    _PWR = powerLevel;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iSetRoomFan3Atech(uint16_t roomFan3Speed)
{
    if (roomFan3Speed > 5)
        return CommandResult::ERROR;

    if (_FAN2TYPE == 4)
    {
        RETURN_IF_ERROR(fumisComWriteWord(0x2004, (!_F4L ? 0 : 2) | (0 < roomFan3Speed)));
        _F3L = (0 < roomFan3Speed);
    }
    else
    {
        if (_FAN2TYPE != 5)
            return CommandResult::ERROR;
        RETURN_IF_ERROR(fumisComWriteByte(0x2004, roomFan3Speed));
        _F3L = roomFan3Speed;
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iSetRoomFan4Atech(uint16_t roomFan4Speed)
{
    if (roomFan4Speed > 5)
        return CommandResult::ERROR;

    CommandResult cmdRes;

    if (_FAN2TYPE == 4)
    {
        RETURN_IF_ERROR(fumisComWriteWord(0x2004, (!roomFan4Speed ? 0 : 2) | (0 < _F3L)));
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
    if (roomFanSpeed > 7)
        return CommandResult::ERROR;

    CommandResult cmdRes;

    if (byte_471CC2 == 0 || _PWR < 4 || roomFanSpeed != 7 || (cmdRes = iSetPowerAtech(3)) == CommandResult::OK)
    {
        RETURN_IF_ERROR(fumisComWriteByte(0x2036, roomFanSpeed));
        RETURN_IF_ERROR(iGetRoomFanAtech());
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
    if (setPoint < _SPLMIN)
        setPoint = _SPLMIN;

    if (setPoint > _SPLMAX)
        setPoint = _SPLMAX;

    if (_FLUID < 2)
    {
        if (!_FLUID)
        {
            RETURN_IF_ERROR(fumisComWriteByte(0x1C33, setPoint * 5.0f));
            _SETP = setPoint;
        }
        else
        {
            RETURN_IF_ERROR(fumisComWriteByte(0x1C33, setPoint));
            _SETP = setPoint;
        }
    }
    else if (_FLUID == 2)
    {
        RETURN_IF_ERROR(fumisComWriteByte(0x1C54, setPoint));
        _SETP = setPoint;
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iUpdateStaticData()
{
    // copy mac address in _MAC
    // look for \n in _MAC
    // then replace it by 0

    // (original: only for Fumis, _MBTYPE < 2; Micronova returned UNSUPPORTED)
    CommandResult cmdRes = iUpdateStaticDataAtech();
    if (cmdRes != CommandResult::OK)
        return CommandResult::ERROR;

    // open /etc/appliancelabel
    // if open /etc/appliancelabel is OK Then
    //// _LABEL=0; //to empty current label
    //// read 0x1F char from /etc/appliancelabel into &_LABEL
    //// close etc/appliancelabel
    // Else
    //// run 'touch /etc/appliancelabel'

    _staticDataLoaded = 1; // flag that indicates Static Data are loaded

    // sendmsg 'GET STDT'

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::iUpdateStaticDataAtech()
{
    iGetSNAtech();

    uint16_t nbTotalBytesReaded = 0; // var_838
    uint8_t buf[8];                  // var_820

    while (nbTotalBytesReaded < _paramsBufferSize)
    {
        RETURN_IF_ERROR(fumisComReadBuff(0x1C00 + nbTotalBytesReaded, buf));

        for (uint8_t i = 0; i < 8 && nbTotalBytesReaded < _paramsBufferSize; i++)
        {
            _PARAMS[nbTotalBytesReaded] = buf[i];
            nbTotalBytesReaded++;
        }
    }
    ///*read all params OK*/

    nbTotalBytesReaded = 0; // var_838
    while (nbTotalBytesReaded < _hparamsBufferSize)
    {
        RETURN_IF_ERROR(fumisComReadBuff((nbTotalBytesReaded + 0xF00) * 2, buf));

        for (uint8_t i = 0; i < 8 && nbTotalBytesReaded < _hparamsBufferSize; i += 2)
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

    RETURN_IF_ERROR(iGetStoveConfigurationAtech());

    // Custom: better feeder active time address based on Core and Version
    // (original: based on Model and Version: 0x209A for models 500-599, otherwise 0x1FAE if VER < 31, 0x1FAC if VER < 40, else 0x209A)
    if (_CORE != 0 || _VER >= 40)
        _wAddrFeederActiveTime = 0x209A;
    else if (_VER < 35)
        _wAddrFeederActiveTime = 0x1FAE;
    else
        _wAddrFeederActiveTime = 0x1FAC;

    if (_MOD < 500 || 599 < _MOD)
    {
        uint16_t var_18;
        RETURN_IF_ERROR(fumisComReadWord(0x203A, &var_18));

        _PSENSTYPE = 0;

        if (0 < (_HPARAMS[(_UICONFIG + 0xD) * 4 / 2] & 0x10))
            _PSENSTYPE = 2;
        else if (1 < var_18)
            _PSENSTYPE = 1;
    }

    nbTotalBytesReaded = 0; // var_838
    while (nbTotalBytesReaded < _paramsBufferSize)
    {
        RETURN_IF_ERROR(fumisComReadBuff((nbTotalBytesReaded + 0x80A2), buf));

        for (uint8_t i = 0; i < 8 && nbTotalBytesReaded < _paramsBufferSize; i++)
        {
            _LIMMIN[nbTotalBytesReaded] = buf[i];
            nbTotalBytesReaded++;
        }
    }

    nbTotalBytesReaded = 0; // var_838
    while (nbTotalBytesReaded < _paramsBufferSize)
    {
        RETURN_IF_ERROR(fumisComReadBuff((nbTotalBytesReaded + 0x810C), buf));

        for (uint8_t i = 0; i < 8 && nbTotalBytesReaded < _paramsBufferSize; i++)
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

//------------------------------------------
// Public part

Palazzetti::CommandResult Palazzetti::initialize()
{
    if (!_link.hasSerial())
        return CommandResult::ERROR;

    if (_isInitialized)
        return CommandResult::OK;

    // (original: iChkMBType, which chose between a Micronova and a Fumis motherboard (_MBTYPE),
    //  and iInit, which only sized the static data buffers: see _paramsBufferSize and _hparamsBufferSize)

    // (original: opened the serial port here, and closed it on failure: now opened once by the application)

    // check the stove answers by reading its STATUS (any failure gives ERROR, as in original)
    // this is also the first exchange, so the loopback is detected on 0x201C,
    // an address where the Response cannot look like the echo of the Request (see FumisLink::exchange)
    if (iGetStatusAtech() != CommandResult::OK)
        return CommandResult::ERROR;

    // read the static data (serial number, firmware, parameters, configuration, limits)
    RETURN_IF_ERROR(iUpdateStaticData());

    _isInitialized = true;
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::initialize(Stream &port)
{
    _link.setSerial(port);
    return initialize();
}

Palazzetti::CommandResult Palazzetti::getAllHiddenParameters(uint16_t (*hiddenParams)[0x6F])
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iUpdateStaticDataAtech());

    memcpy(*hiddenParams, _HPARAMS, 0x6F * sizeof(uint16_t));

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getAllParameters(uint8_t (*params)[0x6A])
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iUpdateStaticDataAtech());

    memcpy(*params, _PARAMS, 0x6A * sizeof(uint8_t));

    return CommandResult::OK;
}

// refreshStatus shoud be true if last call is over ~15sec
Palazzetti::CommandResult Palazzetti::getAllStatus(bool refreshStatus, AllStatusData &out)
{
    RETURN_IF_ERROR(initialize());

    // (original: iGetAllStatus)
    if (refreshStatus)
    {
        RETURN_IF_ERROR(iGetDateTimeAtech());
        RETURN_IF_ERROR(iGetStatusAtech());
        RETURN_IF_ERROR(iGetSetPointAtech());
        RETURN_IF_ERROR(iReadFansAtech());
        RETURN_IF_ERROR(iGetPowerAtech());
        RETURN_IF_ERROR(iGetDPressDataAtech());
        RETURN_IF_ERROR(iReadIOAtech());
        RETURN_IF_ERROR(iReadTemperatureAtech());
        RETURN_IF_ERROR(iGetPumpRateAtech());
        RETURN_IF_ERROR(iGetPelletQtUsedAtech());
        RETURN_IF_ERROR(iGetChronoDataAtech());
        RETURN_IF_ERROR(iGetErrorFlagAtech());
        if (_PSENSTYPE)
        {
            RETURN_IF_ERROR(iGetPelletLevelAtech());
        }
    }

    if (!_staticDataLoaded)
    {
        RETURN_IF_ERROR(iUpdateStaticData());
    }

    out.MBTYPE = 0; // Fumis (Micronova is not supported)
    // MAC not needed
    out.MOD = _MOD;
    out.VER = _VER;
    out.CORE = _CORE;
    fillFwDate(out.FWDATE);
    strcpy(out.APLTS, _STOVE_DATETIME);
    out.APLWDAY = _STOVE_WDAY;
    out.CHRSTATUS = _CHRSTATUS;
    out.STATUS = _STATUS;
    out.LSTATUS = _LSTATUS;
    out.FSTATUS = _FSTATUS;
    if (_STOVETYPE == 3 || _STOVETYPE == 4)
    {
        out.isMFSTATUSValid = true;
        out.MFSTATUS = _MFSTATUS;
    }
    else
        out.isMFSTATUSValid = false;
    out.SETP = _SETP;
    out.SECO = _SECO;
    out.BECO = _BECO;
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
    if (_PSENSTYPE == 1)
    {
        out.isPLEVELValid = true;
        out.PLEVEL = _PLEVEL;
    }
    else
        out.isPLEVELValid = false;
    if (_PSENSTYPE == 2)
    {
        out.isPSENSCSTALEMPValid = true;
        out.PSENSCSTA = _PSENSCSTA;
        out.PSENSLEMP = _PSENSLEMP;
    }
    else
        out.isPSENSCSTALEMPValid = false;
    out.OUT = _OUT_O07 << 6 | _OUT_O06 << 5 | _OUT_O05 << 4 | _OUT_O04 << 3 | _OUT_O03 << 2 | _OUT_O02 << 1 | _OUT_O01;
    out.T1 = _T1;
    out.T2 = _T2;
    out.T3 = _T3;
    out.T4 = _T4;
    out.T5 = _T5;
    out.EFLAGS = _EFLAGS;
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
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iReadTemperatureAtech());

    out.T1 = _T1;
    out.T2 = _T2;
    out.T3 = _T3;
    out.T4 = _T4;
    out.T5 = _T5;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getChronoData(ChronoData &out)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iGetChronoDataAtech());

    out.CHRSTATUS = _CHRSTATUS; // original code should have been "_chronoDataStatus > 0 ? 1 : 0" but _CHRSTATUS is readily available...
    for (uint8_t i = 0; i < 6; i++)
    {
        out.PCHRSETP[i] = _chronoDataPrograms[i].CHRSETP;
        out.PSTART[i][0] = _chronoDataPrograms[i].STARTH;
        out.PSTART[i][1] = _chronoDataPrograms[i].STARTM;
        out.PSTOP[i][0] = _chronoDataPrograms[i].STOPH;
        out.PSTOP[i][1] = _chronoDataPrograms[i].STOPM;
    }
    for (uint8_t i = 0; i < 7; i++)
    {
        out.DM[i][0] = _chronoDataDays[i].M1;
        out.DM[i][1] = _chronoDataDays[i].M2;
        out.DM[i][2] = _chronoDataDays[i].M3;
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getCounters(CountersData &out)
{
    RETURN_IF_ERROR(initialize());

    // (original: iGetCountersAtech)
    uint8_t var_18[8];
    RETURN_IF_ERROR(fumisComReadBuff(0x2066, var_18));

    _IGN = var_18[1];
    _IGN <<= 8;
    _IGN += var_18[0];

    _POWERTIMEM = var_18[3];
    _POWERTIMEM <<= 8;
    _POWERTIMEM += var_18[2];

    _POWERTIMEH = var_18[5];
    _POWERTIMEH <<= 8;
    _POWERTIMEH += var_18[4];

    RETURN_IF_ERROR(fumisComReadBuff(0x206E, var_18));

    _HEATTIMEM = var_18[1];
    _HEATTIMEM <<= 8;
    _HEATTIMEM += var_18[0];

    _HEATTIMEH = var_18[3];
    _HEATTIMEH <<= 8;
    _HEATTIMEH += var_18[2];

    _SERVICETIMEM = var_18[7];
    _SERVICETIMEM <<= 8;
    _SERVICETIMEM += var_18[6];

    RETURN_IF_ERROR(fumisComReadBuff(0x2076, var_18));

    _SERVICETIMEH = var_18[1];
    _SERVICETIMEH <<= 8;
    _SERVICETIMEH += var_18[0];

    _OVERTMPERRORS = var_18[5];
    _OVERTMPERRORS <<= 8;
    _OVERTMPERRORS += var_18[4];

    _IGNERRORS = var_18[7];
    _IGNERRORS <<= 8;
    _IGNERRORS += var_18[6];

    RETURN_IF_ERROR(fumisComReadBuff(0x2082, var_18));

    _ONTIMEM = var_18[1];
    _ONTIMEM <<= 8;
    _ONTIMEM += var_18[0];

    _ONTIMEH = var_18[3];
    _ONTIMEH <<= 8;
    _ONTIMEH += var_18[2];

    uint16_t var_10;

    RETURN_IF_ERROR(fumisComReadWord(0x2002, &var_10));
    _PQT = var_10;

    out.IGN = _IGN;
    sprintf(out.POWERTIME, "%02d:%02d", _POWERTIMEH, _POWERTIMEM % 100);
    sprintf(out.HEATTIME, "%02d:%02d", _HEATTIMEH, _HEATTIMEM % 100);
    sprintf(out.SERVICETIME, "%02d:%02d", _SERVICETIMEH, _SERVICETIMEM % 100);
    sprintf(out.ONTIME, "%02d:%02d", _ONTIMEH, _ONTIMEM % 100);
    out.OVERTMPERRORS = _OVERTMPERRORS;
    out.IGNERRORS = _IGNERRORS;
    out.PQT = _PQT;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getDateTime(DateTimeData &out)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iGetDateTimeAtech());

    strcpy(out.STOVE_DATETIME, _STOVE_DATETIME);
    out.STOVE_WDAY = _STOVE_WDAY;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getDPressData(DPressData &out)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iGetDPressDataAtech());

    out.DP_TARGET = _DP_TARGET;
    out.DP_PRESS = _DP_PRESS;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getFanData(FanData &out)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iReadFansAtech());

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
    RETURN_IF_ERROR(initialize());

    // (original: iGetHiddenParameterAtech)
    if (hParamNumber > 0x6E)
        return CommandResult::ERROR;

    return fumisComReadWord((hParamNumber + 0xF00) * 2, hParamValue);
}

Palazzetti::CommandResult Palazzetti::getIO(IOData &out)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iReadIOAtech());

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
    RETURN_IF_ERROR(initialize());

    out.MOD = _MOD;
    out.VER = _VER;
    out.CORE = _CORE;
    fillFwDate(out.FWDATE);

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getParameter(uint8_t paramNumber, uint8_t *paramValue)
{
    RETURN_IF_ERROR(initialize());

    // (original: iGetParameterAtech)
    if (paramNumber > 0x69)
        return CommandResult::ERROR;

    uint16_t tmpValue;
    RETURN_IF_ERROR(fumisComReadByte(paramNumber + 0x1C00, &tmpValue));

    // convert uint16_t to uint8_t
    *paramValue = tmpValue;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getPelletQtUsed(uint16_t *PQT)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iGetPelletQtUsedAtech());

    if (PQT)
        *PQT = _PQT;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getPower(PowerData &out)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iGetPowerAtech());

    out.PWR = _PWR;
    out.FDR = _FDR;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getSetPoint(SetPointData &out)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iGetSetPointAtech());

    out.SETP = _SETP;
    out.SECO = _SECO;
    out.BECO = _BECO;
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getSN(char (*SN)[28])
{
    RETURN_IF_ERROR(initialize());

    if (SN)
        strcpy(*SN, _SN);
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getStaticData(StaticData &out)
{
    RETURN_IF_ERROR(initialize());

    if (!_staticDataLoaded)
    {
        RETURN_IF_ERROR(iUpdateStaticData());
    }

    // read LABEL : not needed
    // get network infos by running "lua /www/cgi-bin/syscmd.lua \"netdata\" > /dev/null" : not needed

    strcpy(out.SN, _SN);
    out.SNCHK = isValidSerialNumber(_SN);
    out.MBTYPE = 0; // Fumis (Micronova is not supported)

    // APLCONN : useless because overwritten by CBox lua code

    out.MOD = _MOD;
    out.VER = _VER;
    out.CORE = _CORE;
    fillFwDate(out.FWDATE);
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
    out.CHRONOTYPE = _CHRONOTYPE;
    out.AUTONOMYTYPE = _AUTONOMYTYPE;
    out.NOMINALPWR = _NOMINALPWR;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::getStatus(StatusData &out)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iGetStatusAtech());

    out.STATUS = _STATUS;
    out.LSTATUS = _LSTATUS;
    out.FSTATUS = _FSTATUS;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::readData(uint16_t addrToRead, bool wordMode, uint16_t *ADDR_DATA)
{
    RETURN_IF_ERROR(initialize());

    // (original: iReadDataAtech)
    if (!wordMode)
        return fumisComReadByte(addrToRead, ADDR_DATA);
    else
        return fumisComReadWord(addrToRead, ADDR_DATA);
}

Palazzetti::CommandResult Palazzetti::setChronoDay(uint8_t dayNumber, uint8_t memoryNumber, uint8_t programNumber)
{
    RETURN_IF_ERROR(initialize());

    // (original: iSetChronoDayAtech)
    if (programNumber > 6)
        return CommandResult::ERROR; // original is PARSER_ERROR

    if (!dayNumber || dayNumber > 7)
        return CommandResult::ERROR; // original is PARSER_ERROR

    if (!memoryNumber || memoryNumber > 3)
        return CommandResult::ERROR; // original is PARSER_ERROR

    return fumisComWriteByte((dayNumber - 1) * 3 + memoryNumber + 0x8017, programNumber);
}

Palazzetti::CommandResult Palazzetti::setChronoPrg(uint8_t programNumber, uint8_t setPoint, uint8_t startHour, uint8_t startMinute, uint8_t stopHour, uint8_t stopMinute)
{
    RETURN_IF_ERROR(initialize());

    // (original: iSetChronoPrgAtech, with the program in an array prg[6])
    if (!programNumber || programNumber > 6)
        return CommandResult::ERROR;

    if (setPoint < _SPLMIN)
        setPoint = _SPLMIN;

    if (setPoint > _SPLMAX)
        setPoint = _SPLMAX;

    if (!_FLUID)
        setPoint *= 5;

    if (startHour >= 24 || stopHour >= 24)
        return CommandResult::ERROR;

    if (startMinute >= 60 || stopMinute >= 60)
        return CommandResult::ERROR;

    RETURN_IF_ERROR(fumisComWriteByte(programNumber + 0x802c, setPoint));

    RETURN_IF_ERROR(fumisComWriteByte((programNumber + 0x1fff) * 4, startHour));

    RETURN_IF_ERROR(fumisComWriteByte((programNumber + 0x1fff) * 4 + 1, startMinute));

    RETURN_IF_ERROR(fumisComWriteByte((programNumber + 0x1fff) * 4 + 2, stopHour));

    return fumisComWriteByte((programNumber + 0x1fff) * 4 + 3, stopMinute);
}

Palazzetti::CommandResult Palazzetti::setChronoSetpoint(uint8_t programNumber, uint8_t setPoint)
{
    RETURN_IF_ERROR(initialize());

    // (original: iSetChronoSetpointAtech)
    if (!programNumber || programNumber > 6)
        return CommandResult::ERROR;

    if (setPoint < _SPLMIN)
        setPoint = _SPLMIN;

    if (setPoint > _SPLMAX)
        setPoint = _SPLMAX;

    if (!_FLUID)
        setPoint *= 5;

    return fumisComWriteByte(0x802C + programNumber, setPoint);
}

Palazzetti::CommandResult Palazzetti::setChronoStartHH(uint8_t programNumber, uint8_t startHour)
{
    RETURN_IF_ERROR(initialize());

    // (original: iSetChronoStartHHAtech)
    if (!programNumber || programNumber > 6 || startHour >= 24)
        return CommandResult::ERROR;

    return fumisComWriteByte((programNumber + 0x1FFF) * 4, startHour);
}

Palazzetti::CommandResult Palazzetti::setChronoStartMM(uint8_t programNumber, uint8_t startMinute)
{
    RETURN_IF_ERROR(initialize());

    // (original: iSetChronoStartMMAtech)
    if (!programNumber || programNumber > 6 || startMinute >= 60)
        return CommandResult::ERROR;

    return fumisComWriteByte((programNumber + 0x1FFF) * 4 + 1, startMinute);
}

Palazzetti::CommandResult Palazzetti::setChronoStatus(bool chronoStatus, uint8_t *CHRSTATUSResult /* = nullptr */)
{
    RETURN_IF_ERROR(initialize());

    // (original: iSetChronoStatusAtech)
    uint16_t buf;
    RETURN_IF_ERROR(fumisComReadWord(0x207e, &buf));
    if (chronoStatus)
        buf |= 1;
    else
        buf &= 0xfffe;

    RETURN_IF_ERROR(fumisComWriteByte(0x207e, buf));

    if (CHRSTATUSResult)
        *CHRSTATUSResult = chronoStatus;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setChronoStopHH(uint8_t programNumber, uint8_t stopHour)
{
    RETURN_IF_ERROR(initialize());

    // (original: iSetChronoStopHHAtech)
    if (!programNumber || programNumber > 6 || stopHour >= 24)
        return CommandResult::ERROR;

    return fumisComWriteByte((programNumber + 0x1FFF) * 4 + 2, stopHour);
}

Palazzetti::CommandResult Palazzetti::setChronoStopMM(uint8_t programNumber, uint8_t stopMinute)
{
    RETURN_IF_ERROR(initialize());

    // (original: iSetChronoStopMMAtech)
    if (!programNumber || programNumber > 6 || stopMinute >= 60)
        return CommandResult::ERROR;

    return fumisComWriteByte((programNumber + 0x1FFF) * 4 + 3, stopMinute);
}

Palazzetti::CommandResult Palazzetti::setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, DateTimeData *result /* = nullptr */)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(fumisComSetDateTime(year, month, day, hour, minute, second));

    if (result)
    {
        strcpy(result->STOVE_DATETIME, _STOVE_DATETIME);
        result->STOVE_WDAY = _STOVE_WDAY;
    }

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setHiddenParameter(uint8_t hParamNumber, uint16_t hParamValue)
{
    RETURN_IF_ERROR(initialize());

    // (original: iSetHiddenParameterAtech)
    if (hParamNumber > 0x6E)
        return CommandResult::ERROR;

    RETURN_IF_ERROR(fumisComWriteWord((hParamNumber + 0xF00) * 2, hParamValue));
    if (hParamNumber < _hparamsBufferSize)
        _HPARAMS[hParamNumber] = hParamValue;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setParameter(uint8_t paramNumber, uint8_t paramValue)
{
    RETURN_IF_ERROR(initialize());

    // (original: iSetParameterAtech)
    if (paramNumber >= 0x6A)
        return CommandResult::ERROR;

    RETURN_IF_ERROR(fumisComWriteByte(paramNumber + 0x1C00, paramValue));
    if (paramNumber < _paramsBufferSize)
        _PARAMS[paramNumber] = paramValue;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setPower(uint8_t powerLevel, SetPowerResult *result /* = nullptr */)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iSetPowerAtech(powerLevel));

    if (byte_471CC2)
    {
        RETURN_IF_ERROR(iGetRoomFanAtech());
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
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iGetPowerAtech());

    return setPower(_PWR - 1, result);
}

Palazzetti::CommandResult Palazzetti::setPowerUp(SetPowerResult *result /* = nullptr */)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iGetPowerAtech());

    return setPower(_PWR + 1, result);
}

Palazzetti::CommandResult Palazzetti::setRoomFan(uint8_t roomFanSpeed, SetRoomFanResult *result /* = nullptr */)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iSetRoomFanAtech(transcodeRoomFanSpeed(roomFanSpeed, 0)));

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

Palazzetti::CommandResult Palazzetti::setRoomFan3(uint8_t roomFan3Speed, uint16_t *F3LResult /* = nullptr */)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iSetRoomFan3Atech(roomFan3Speed));

    if (F3LResult)
        *F3LResult = _F3L;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setRoomFan4(uint8_t roomFan4Speed, uint16_t *F4LResult /* = nullptr */)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iSetRoomFan4Atech(roomFan4Speed));

    if (F4LResult)
        *F4LResult = _F4L;

    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setRoomFanDown(SetRoomFanResult *result /* = nullptr */)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iReadFansAtech());

    return setRoomFan(transcodeRoomFanSpeed(_F2L, true) - 1, result);
}

Palazzetti::CommandResult Palazzetti::setRoomFanUp(SetRoomFanResult *result /* = nullptr */)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iReadFansAtech());

    return setRoomFan(transcodeRoomFanSpeed(_F2L, true) + 1, result);
}

Palazzetti::CommandResult Palazzetti::setSetpoint(uint8_t setPoint, float *SETPResult /* = nullptr */)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iSetSetPointAtech(setPoint));

    if (SETPResult)
        *SETPResult = _SETP;
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setSetpoint(float setPoint, float *SETPResult /* = nullptr */)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iSetSetPointAtech(setPoint));

    if (SETPResult)
        *SETPResult = _SETP;
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setSetPointDown(float *SETPResult /* = nullptr */)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iGetSetPointAtech());

    return setSetpoint(_SETP - 1.0f, SETPResult);
}

Palazzetti::CommandResult Palazzetti::setSetPointFan3(uint8_t setPoint, float *F3SResult /* = nullptr */)
{
    RETURN_IF_ERROR(initialize());

    if (_BLEMBMODE < 13)
        return CommandResult::UNSUPPORTED;

    // (original: iSetSetPointFan3Atech)
    if (setPoint == 0)
        setPoint = _SPLMAX;
    if (setPoint < _SPLMIN)
        setPoint = _SPLMIN;
    if (setPoint > _SPLMAX)
        setPoint = _SPLMAX;
    RETURN_IF_ERROR(fumisComWriteByte(0x20A2, setPoint * 5));
    _F3S = (float)setPoint;

    if (F3SResult)
        *F3SResult = _F3S;
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setSetPointFan4(uint8_t setPoint, float *F4SResult /* = nullptr */)
{
    RETURN_IF_ERROR(initialize());

    if (_BLEMBMODE < 13)
        return CommandResult::UNSUPPORTED;

    // (original: iSetSetPointFan4Atech)
    if (setPoint == 0)
        setPoint = _SPLMAX;
    if (setPoint < _SPLMIN)
        setPoint = _SPLMIN;
    if (setPoint > _SPLMAX)
        setPoint = _SPLMAX;
    RETURN_IF_ERROR(fumisComWriteByte(0x20A3, setPoint * 5));
    _F4S = (float)setPoint;

    if (F4SResult)
        *F4SResult = _F4S;
    return CommandResult::OK;
}

Palazzetti::CommandResult Palazzetti::setSetPointUp(float *SETPResult /* = nullptr */)
{
    RETURN_IF_ERROR(initialize());

    RETURN_IF_ERROR(iGetSetPointAtech());

    return setSetpoint(_SETP + 1.0f, SETPResult);
}

Palazzetti::CommandResult Palazzetti::setSilentMode(uint8_t silentMode, SetSilentModeResult *result /* = nullptr */)
{
    RETURN_IF_ERROR(initialize());

    // (original: iSetSilentModeAtech)
    if (silentMode > 0)
    {
        RETURN_IF_ERROR(iSetRoomFanAtech(7));
        RETURN_IF_ERROR(iSetRoomFan3Atech(0));
        RETURN_IF_ERROR(iSetRoomFan4Atech(0));
    }

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
    RETURN_IF_ERROR(initialize());

    // STATUS before the command, to detect its change
    StatusData status;
    RETURN_IF_ERROR(getStatus(status));
    uint16_t statusBefore = status.STATUS;

    // (original: iSwitchOffAtech)
    // Custom: original is using fumisComWriteWord, and returns -1 for any error
    if (_MOD < 500 || 599 < _MOD)
        RETURN_IF_ERROR(fumisComWriteByte(0x2044, 1));
    else
        RETURN_IF_ERROR(fumisComWriteByte(0x2044, 0x11));

    // poll STATUS for 400ms max while it succeeds and the stove was ON (statusBefore > 1) and hasn't changed yet
    unsigned long startTime = millis();
    CommandResult cmdRes;
    do
    {
        yield();
        cmdRes = getStatus(status);
    } while (cmdRes == CommandResult::OK && statusBefore > 1 && status.STATUS == statusBefore && millis() - startTime < 400);

    if (result)
        *result = status;
    return cmdRes;
}

Palazzetti::CommandResult Palazzetti::switchOn(StatusData *result /* = nullptr */)
{
    RETURN_IF_ERROR(initialize());

    // (original: iSwitchOnAtech)
    // Custom: original is using fumisComWriteWord, and returns -1 for any error
    if (_MOD < 500 || 599 < _MOD)
        RETURN_IF_ERROR(fumisComWriteByte(0x2044, 2));
    else
        RETURN_IF_ERROR(fumisComWriteByte(0x2044, 0x12));

    // poll STATUS for 1200ms max while it succeeds and remains in the start-up states 0-3
    // (if stove doens't need to heat up, it moves from STATUS=3 to 9 (ECO STOP) in 1.2s max)
    StatusData status;
    unsigned long startTime = millis();
    CommandResult cmdRes;
    do
    {
        yield();
        cmdRes = getStatus(status);
    } while (cmdRes == CommandResult::OK && status.STATUS <= 3 && millis() - startTime < 1200);

    if (result)
        *result = status;
    return cmdRes;
}

Palazzetti::CommandResult Palazzetti::writeData(uint16_t addrToWrite, uint16_t data, bool wordMode)
{
    RETURN_IF_ERROR(initialize());

    // (original: iWriteDataAtech)
    if (!wordMode)
        return fumisComWriteByte(addrToWrite, data & 0xFF);
    else
        return fumisComWriteWord(addrToWrite, data);
    // the original code return the data untouched (even if its value is over 255 and wordMode is false...)
    // considered as useless because CommandResult is enough to know if the command was successful
}

//------------------------------------------
// Constructor
Palazzetti::Palazzetti() {}
