#include "Palazzetti.h"

//original signature is (filename,baudrate,databitnumber,stopbitnumber,)
//always 8 data bits and 1 stop bits
int Palazzetti::SERIALCOM_OpenComport(uint32_t baudrate)
{
    return m_openSerial(baudrate);
}

void Palazzetti::SERIALCOM_CloseComport()
{
    m_closeSerial();
}

int Palazzetti::SERIALCOM_Flush()
{
    return m_flushSerial();
}

int Palazzetti::SERIALCOM_ReceiveBuf(void *buf, size_t count)
{
    int res = m_selectSerial(selectSerialTimeoutms);
    if (res < 0)
        return -1;

    if (!res)
        return res;

    res = m_readSerial(buf, count);

    if (res < 1)
        return -1;

    if (serialPortModel == 2)
        m_uSleep(1);
    return res;
}

int Palazzetti::SERIALCOM_SendBuf(void *buf, size_t count)
{
    size_t bytesSent = 0;
    size_t totalBytesWritten = 0;

    while (totalBytesWritten < count)
    {
        bytesSent = m_writeSerial((void *)((uint8_t *)buf + totalBytesWritten), count - totalBytesWritten);
        totalBytesWritten += bytesSent;
    }

    m_drainSerial();

    if (!comPortNumber && !_MBTYPE)
    {
        size_t bytesReaded = 0;
        size_t totalBytesReaded = 0;

        while (totalBytesReaded < count)
        {
            if (m_selectSerial(selectSerialTimeoutms) <= 0)
                return -1;

            bytesReaded = m_readSerial((void *)((uint8_t *)buf + totalBytesReaded), count - totalBytesReaded);
            if (bytesReaded < 0)
                return bytesReaded;
            totalBytesReaded += bytesReaded;
        }
    }

    return bytesSent;
}

int Palazzetti::SERIALCOM_ReceiveByte(byte *buf)
{
    int res = SERIALCOM_ReceiveBuf(buf, 1);
    if (res < 0)
        return res;

    return 0;
}

void Palazzetti::SERIALCOM_SendByte(byte *buf)
{
    SERIALCOM_SendBuf(buf, 1);
}

int Palazzetti::iChkSum(byte *datasToCheck)
{
    byte chk = 0; //var_10
    for (byte i = 0; i < 0xA; i++)
        chk += datasToCheck[i];

    if (chk != datasToCheck[0xA])
        return -508;
    return 0;
}

int Palazzetti::parseRxBuffer(byte *rxBuffer)
{
    int res; //var_10;
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

int Palazzetti::fumisCloseSerial()
{
    SERIALCOM_CloseComport();
    fumisComStatus = 0;
    return 0;
}

int Palazzetti::fumisOpenSerial()
{
    selectSerialTimeoutms = 2300;
    int res = SERIALCOM_OpenComport(0x9600);

    if (res >= 0 && (serialPortModel != 1 || (res = SERIALCOM_Flush()) >= 0))
    {
        dword_432618 = 0;
        fumisComStatus = 1;
        return 0;
    }
    else
    {
        fumisComStatus = 0;
        dword_432618 = res;
    }

    return res;
}

int Palazzetti::fumisSendRequest(void *buf)
{
    if (fumisComStatus != 3)
        return -1;

    int totalSentBytes = 0; //var_18
    int sentBytes = 0;      //dummy value for firstrun
    while (totalSentBytes < 0xB && sentBytes != -1)
    {
        sentBytes = SERIALCOM_SendBuf((void *)((uint8_t *)buf + totalSentBytes), 0xB - totalSentBytes);
        totalSentBytes += sentBytes;
    }
    if (sentBytes < 0)
        dword_432618 = sentBytes;
    return sentBytes; //not totalSentBytes...
}

int Palazzetti::fumisWaitRequest(void *buf)
{
    if (!fumisComStatus)
        return -1;

    int nbReceivedBytes = 0; //var_18

    unsigned long startTime; //var_10
    startTime = millis();    //instead of time()
    do
    {
        if (fumisComStatus == 5 || fumisComStatus == 3)
            return nbReceivedBytes;

        nbReceivedBytes = 0;
        bzero(buf, 0xB);
        while ((nbReceivedBytes = SERIALCOM_ReceiveBuf(buf, 0xB)) < 0xB)
        {
            if (millis() - startTime > 3000)
                return -601;

            if (nbReceivedBytes < 0)
            {
                dword_432618 = nbReceivedBytes;
                if (fumisCloseSerial() < 0)
                    return -1;
                if (fumisOpenSerial() < 0)
                    return -1;
                break;
            }
        };

        if (parseRxBuffer((byte *)buf) >= 0)
            continue;

        if (fumisComStatus == 4)
            return -601;

        if (millis() - startTime > 3000)
            return -601;

        if (serialPortModel == 2 && SERIALCOM_Flush() < 0)
            return -601;

    } while (1);
}

int Palazzetti::fumisComReadBuff(uint16_t addrToRead, void *buf, size_t count)
{
    if (count != 8)
        return -1;

    bzero(buf, count);

    if (!fumisComStatus)
        return -1;

    uint8_t var_28[32];
    int res;

    for (int i = 4; i > 0; i--)
    {
        if (i < 4)
        {
            fumisCloseSerial();
            SERIALCOM_Flush();
            m_uSleep(1);
            fumisOpenSerial();
            m_uSleep(1);
        }

        if (_MBTYPE != 1)
        {
            fumisComStatus = 2;
            res = fumisWaitRequest((void *)var_28);
            if (res < 0)
            {
                if (res == -601)
                    continue;
                else
                {
                    dword_432618 = res;
                    return res;
                }
            }
        }

        fumisComStatus = 3;
        bzero(var_28, 0xB);
        var_28[0] = 2;
        var_28[1] = addrToRead & 0xFF;
        var_28[2] = (addrToRead >> 8) & 0xFF;

        var_28[10] = var_28[0] + var_28[1] + var_28[2];
        res = fumisSendRequest(var_28);
        if (res < 0)
            return res;

        bzero(var_28, 32);
        fumisComStatus = 4;
        res = fumisWaitRequest(var_28);
        if (res >= 0)
        {
            memcpy(buf, var_28 + 1, count);
            return res;
        }

        if (res != -601)
        {
            dword_432618 = res;
            return res;
        }
    }

    if (res >= 0)
        memcpy(buf, var_28 + 1, count);
    else
        dword_432618 = res;
    return res;
}

int Palazzetti::fumisComRead(uint16_t addrToRead, uint16_t *data, int wordMode)
{
    byte var_10[8];
    int res = fumisComReadBuff(addrToRead, var_10, 8);
    if (res < 0)
        return res;

    *data = 0;

    if (!wordMode)
        *data = var_10[0];

    if (wordMode == 1)
    {
        *data = var_10[1];
        *data <<= 8;
        *data += var_10[0];
    }

    return res;
}

int Palazzetti::fumisComReadByte(uint16_t addrToRead, uint16_t *data)
{
    return fumisComRead(addrToRead, data, 0);
}

int Palazzetti::fumisComReadWord(uint16_t addrToRead, uint16_t *data)
{
    return fumisComRead(addrToRead, data, 1);
}

int Palazzetti::fumisComWrite(uint16_t addrToWrite, uint16_t data, int wordMode)
{
    if (!fumisComStatus)
        return -1;

    int res;       //var_38
    byte buf[0xB]; //var_2C

    for (int i = 2; i > 0; i--) //i as var_34
    {
        fumisComStatus = 2;
        bzero(&buf, 0xB);
        res = fumisWaitRequest(&buf);
        if (res < 0)
        {
            dword_432618 = res;
            continue;
        }
        bzero(&buf, 0xB);
        buf[0] = 1;
        buf[1] = addrToWrite & 0xFF;
        buf[2] = addrToWrite >> 8;
        buf[3] = data & 0xFF;
        buf[10] = buf[0] + buf[1] + buf[2] + buf[3];
        res = fumisSendRequest(&buf);
        if (res < 0)
            return res;

        if (!wordMode)
        {
            uint16_t var_30 = 0;
            fumisComRead(addrToWrite, &var_30, 0);
            if (var_30 == data)
                return res;
            else
                continue;
        }

        fumisComStatus = 2;
        res = fumisWaitRequest(&buf);
        if (res < 0)
        {
            dword_432618 = res;
            return res;
        }
        bzero(&buf, 0xB);
        buf[0] = 1;
        buf[1] = (addrToWrite + 1) & 0xFF;
        buf[2] = (addrToWrite + 1) >> 8;
        buf[3] = data >> 8;
        buf[10] = buf[0] + buf[1] + buf[2] + buf[3];
        res = fumisSendRequest(&buf);
        if (res < 0)
            return res;

        uint16_t var_30 = 0;
        fumisComRead(addrToWrite, &var_30, 1);
        if (var_30 == data)
            return res;
    }
    return res;
}

int Palazzetti::fumisComWriteByte(uint16_t addrToWrite, uint16_t data)
{
    return fumisComWrite(addrToWrite, data, 0);
}

int Palazzetti::fumisComWriteWord(uint16_t addrToWrite, uint16_t data)
{
    return fumisComWrite(addrToWrite, data, 1);
}

int Palazzetti::iGetStatusAtech()
{
    int res = 0;
    res = fumisComReadByte(0x201C, &_STATUS);
    if (res < 0)
        return res;

    _LSTATUS = _STATUS;

    if (_STATUS >= 0xC9)
        _LSTATUS = _STATUS + 0x3E8;

    if (_STOVETYPE == 3 || _STOVETYPE == 4)
    {
        res = fumisComReadWord(0x2008, &_MFSTATUS);
        if (res < 0)
            return res;
        if (_MFSTATUS >= 2)
        {
            _LSTATUS = _MFSTATUS + 0x1F4;
            if (_MFSTATUS == 0x1FC)
                _LSTATUS += 0x3e8;
        }
    }

    if (_LSTATUS != 9)
        return 0;

    if (_STOVETYPE != 2 || (_UICONFIG != 1 && _UICONFIG != 3 && _UICONFIG != 4))
    {
        _LSTATUS = 0x33;
    }

    return 0;
}

int Palazzetti::iChkMBType()
{

    _MBTYPE = 0x64;
    //No Implementation of Micronova device there
    //skip directly to fumis detection

    _MBTYPE = 0;
    if (fumisOpenSerial() < 0)
        return -1;

    ///*Trying to find MBTYPE_FUMIS_ALPHA65...*/
    if (iGetStatusAtech() < 0)
    {
        _MBTYPE = -1;
        return -1;
    }
    ///*-->Found MBTYPE_FUMIS_ALPHA65 device!*/
    return 0;
}

int Palazzetti::iInit()
{
    if (_MBTYPE < 0)
        return -10;

    if (_MBTYPE < 2) //if Fumis MB
    {
        //dword_46DC04 = malloc(0xD0);
        paramsBufferSize = 0x6A;
        hparamsBufferSize = 0x6F;
        //dword_46DC38 = malloc(paramsBufferSize aka 0x6A);
        //dword_46DC50 = malloc(paramsBufferSize aka 0x6A);
        //dword_46DC4C = malloc(paramsBufferSize aka 0x6A);
        //dword_46DC40 =  malloc(hparamsBufferSize<<1 aka 0xDE)
        //dword_46DC08 = malloc(0x16)
        //bzero(dword_46DC08,0x16);
        //dword_46DC0C = malloc(0x69);
        //bzero(dword_46DC0C,0x69);
    }

    //rest of iInit concerns Micronova which is not implemented

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

int Palazzetti::iGetSNAtech()
{
    int res = 0;
    int currentPosInSN = 0; //var_24
    byte buf[8];            //var_14
    char *pSN = (char*)&_SN; //var_18
    byte checkSum = 0;
    while (currentPosInSN < 0xE)
    {
        res = fumisComReadBuff(0x2100 + currentPosInSN, buf, 8);
        if (res < 0)
        {
            _SN[0]=0;
            return res;
        }
        for (byte i = 0; i < 8; i++)
        {
            sprintf(pSN, "%02X", buf[i]);
            pSN += 2;
            checkSum += buf[i];
        }
        currentPosInSN += 8;
    }

    _SN[27] = 0;
    res = fumisComReadBuff(0x2100 + currentPosInSN, buf, 8);
    if (res < 0)
    {
        _SN[0]=0;
        return res;
    }
    if (buf[0] == -checkSum)
    {
        _SN[0]=0;
        return -1;
    }
    if (buf[1] != 'U')
    {
        if (strncmp(_SN,"7684",4) == 0)
        {
            _SN[0] = 'L';
            _SN[1] = 'T';
            strcpy((char*)&_SN + 2, (char*)&_SN + 4);
            _SN[23] = 0;
        }
    }

    return 0;
}

int Palazzetti::iGetMBTypeAtech()
{
    uint16_t buf; //var_C
    int res = 0;  //var_10
    res = fumisComReadWord(0x204C, &buf);
    if (res < 0)
        return res;

    _HWTYPE = 6;
    if ((buf & 4) == 0)
    {
        if ((buf & 0x8000) != 0)
            _HWTYPE = 7;
    }
    else
        _HWTYPE = 5;
    return 0;
}

int Palazzetti::iGetStoveConfigurationAtech()
{
    uint16_t buf; //var_10
    int res = 0;  //var_24
    res = fumisComReadByte(0x2006, &buf);
    if (res < 0)
        return res;
    _DSPFWVER = buf;

    iGetMBTypeAtech();

    // byte_471087 = 2; //Not Used elsewhere
    _MAINTPROBE = 0;

    res = fumisComReadWord(0x1ED4, &buf);
    if (res < 0)
        return res;
    byte buf2[8]; //var_18
    res = fumisComReadBuff(0x1E25, buf2, 8);
    if (res < 0)
        return res;

    _NOMINALPWR = buf2[0];
    if (buf2[1] & 2)
        _AUTONOMYTYPE = 2;
    else
        _AUTONOMYTYPE = 1;

    byte var_1F = _PARAMS[0x4C];
    _UICONFIG = var_1F;

    res = fumisComReadBuff(((var_1F - 1) << 2) + 0x1E36, buf2, 8);
    if (res < 0)
        return res;

    byte var_1E = ((buf2[0] & 0x20) > 0);

    _FAN2TYPE = 1;
    _MAINTPROBE = 0;

    if ((buf2[0] & 8) > 0)
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

    byte_47108A = 0;

    if (_PARAMS[0x69] > 0 && _PARAMS[0x69] < 6)
        byte_47108A = _PARAMS[0x69];

    _FAN1LMIN = 1;
    _FAN1LMAX = 5;
    _FAN2LMIN = 0;
    _FAN2LMAX = 1;
    _FAN3LMIN = 0;
    _FAN3LMAX = 1;
    _FAN2MODE = 1;

    if (_HPARAMS[0x26 / 2] & 0x10)
        _FAN1LMIN = 0;

    if (((_HPARAMS[0x38 / 2] + ((var_1F - 1) << 1)) & 0x800) == 0)
        _FAN2MODE = 3;

    if (_FAN2TYPE == 5 || _FAN2TYPE == 3)
    {
        _FAN2LMAX = 5;
        _FAN3LMAX = 5;
    }

    byte tmp = 1; //var_27
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

    if (_DSPFWVER < 0x2E)
    {
        if (_DSPFWVER < 0x2A)
            tmp = 1;
        else if (tmp > 6)
            tmp -= 4;
    }
    _BLEDSPMODE = tmp;

    if (var_1E)
        _STOVETYPE = 2;
    else
        _STOVETYPE = 1;

    if (var_1F < 3)
    {
        res = fumisComReadBuff(0x1E36, buf2, 8);
        if (res < 0)
            return res;
        byte var_28;
        if (var_1F == 2)
            var_28 = buf2[5] & 4;
        else
            var_28 = buf2[1] & 4;

        if (var_28)
        {
            _FLUID = 0;
            if (var_1E)
                _MAINTPROBE = 4;
        }
        else
            _FLUID = 1;
    }
    else
    {
        if (var_1F == 5)
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
    }

    if (_MOD >= 0x1F5 && _MOD < 0x258)
    {
        if (_STOVETYPE == 1)
        {
            _STOVETYPE = 3;
            _MAINTPROBE = 4;
        }
        else
        {
            if (_STOVETYPE == 2)
            {
                _STOVETYPE = 4;
                if (var_1F == 2)
                    _MAINTPROBE = 4;
            }
            else
                return -1;
        }
    }

    ///*iGetStoveConfigurationAtech OK*/
    return 0;
}

int Palazzetti::iUpdateStaticDataAtech()
{
    iGetSNAtech();

    uint16_t nbTotalBytesReaded = 0; //var_838
    byte buf[8];                     //var_820
    int res = 0;                     //var_830

    while (nbTotalBytesReaded < paramsBufferSize)
    {
        res = fumisComReadBuff(0x1C00 + nbTotalBytesReaded, buf, 8);
        if (res < 0)
            return res;

        for (byte i = 0; i < 8 && nbTotalBytesReaded < paramsBufferSize; i++)
        {
            _PARAMS[nbTotalBytesReaded] = buf[i];
            nbTotalBytesReaded++;
        }
    }
    ///*read all params OK*/

    nbTotalBytesReaded = 0; //var_838
    while (nbTotalBytesReaded < hparamsBufferSize)
    {
        res = fumisComReadBuff((nbTotalBytesReaded + 0xF00) << 1, buf, 8);
        if (res < 0)
            return res;

        for (byte i = 0; i < 8 && nbTotalBytesReaded < hparamsBufferSize; i += 2)
        {
            //not nbTotalBytesReaded * 2 because _HPARAMS is uint16_t[] type
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
    // pdword_471050 = _HPARAMS[0x1E / 2]; //Not Used elsewhere
    // pdword_471044 = _HPARAMS[0x88 / 2]; //Not Used elsewhere
    // pdword_471048 = _HPARAMS[0x8C / 2]; //Not Used elsewhere
    // pdword_47104C = _HPARAMS[0x8E / 2]; //Not Used elsewhere

    res = iGetStoveConfigurationAtech();
    if (res < 0)
        return res;

    if (_MOD < 0x1F4 || _MOD >= 0x258)
    {
        if (_VER < 0x1F)
            wAddrFeederActiveTime = 0x1FAE;
        else
        {
            if (_VER < 0x28)
                wAddrFeederActiveTime = 0x1FAC;
            else
                wAddrFeederActiveTime = 0x209A;
        }
        uint16_t var_18;
        res = fumisComReadWord(0x203A, &var_18);
        if (res < 0)
            return res;

        _PSENSTYPE = 0;

        if (0 < (_HPARAMS[(_UICONFIG + 0xD) * 4 / 2] & 0x10))
            _PSENSTYPE = 2;
        else if (1 < var_18)
            _PSENSTYPE = 1;
    }
    else
        wAddrFeederActiveTime = 0x209A;

    nbTotalBytesReaded = 0; //var_838
    while (nbTotalBytesReaded < paramsBufferSize)
    {
        res = fumisComReadBuff((nbTotalBytesReaded + 0x80A2), buf, 8);
        if (res < 0)
            return res;

        for (byte i = 0; i < 8 && nbTotalBytesReaded < paramsBufferSize; i++)
        {
            _LIMMIN[nbTotalBytesReaded] = buf[i];
            nbTotalBytesReaded++;
        }
    }

    nbTotalBytesReaded = 0; //var_838
    while (nbTotalBytesReaded < paramsBufferSize)
    {
        res = fumisComReadBuff((nbTotalBytesReaded + 0x810C), buf, 8);
        if (res < 0)
            return res;

        for (byte i = 0; i < 8 && nbTotalBytesReaded < paramsBufferSize; i++)
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
        else if (_FLUID == 2)
        {
            _SPLMIN = _LIMMIN[0x54];
            _SPLMAX = _LIMMAX[0x54];
        }
    }

    ///*iGetLimitsAtech OK*/

    //JSON not build and so not savec in /tmp/appliance_params.json

    ///*iUpdateStaticDataAtech OK*/

    return 0;
}

int Palazzetti::iUpdateStaticData()
{
    //copy mac address in byte_46DC6F[19]
    //look for \n in byte_46DC6F
    //then replace it by 0

    if (_MBTYPE < 0)
        return -1;

    if (_MBTYPE < 2) //if fumis MBTYPE
    {
        int res = iUpdateStaticDataAtech();
        if (res < 0)
            return -1;
    }
    else if (_MBTYPE == 0x64) //else Micronova not implemented so return -1
        return -1;
    else
        return -1;

    //open /etc/appliancelabel
    //if open /etc/appliancelabel is OK Then
    ////_LABEL=0; //to empty current label
    ////read 0x1F char from /etc/appliancelabel into &_LABEL
    ////close etc/appliancelabel
    //Else
    ////run 'touch /etc/appliancelabel'

    staticDataLoaded = 1; //flag that indicates Static Data are loaded

    //sendmsg 'GET STDT'

    return 0;
}

int Palazzetti::iCloseUART()
{
    if (_MBTYPE < 2)
        return fumisCloseSerial();
    else
        return -1;
}

int Palazzetti::iGetSetPointAtech()
{
    if (_STOVETYPE == 1 && _PARAMS[0x4C] == 2)
    {
        _SETP = 0;
        return 0;
    }

    uint16_t data;
    int res;

    if (_FLUID <= 2)
    {
        res = fumisComReadByte((_FLUID < 2) ? 0x1C33 : 0x1C54, &data);
        if (res < 0)
            return res;
        float dataFloat = (int16_t)data;
        if (!_FLUID)
            dataFloat /= 5.0;
        _SETP = dataFloat;
    }
    return 0;
}

int Palazzetti::iSetSetPointAtech(uint16_t setPoint)
{
    int res; //var_10

    if (setPoint < _SPLMIN)
        setPoint = _SPLMIN;

    if (setPoint > _SPLMAX)
        setPoint = _SPLMAX;

    if (_FLUID > 2)
        return 0;
    if (_FLUID == 2)
    {
        res = fumisComWriteByte(0x1C54, setPoint);
        if (res < 0)
            return res;
        _SETP = setPoint;
    }
    if (!_FLUID)
    {
        res = fumisComWriteByte(0x1C33, setPoint * 5);
        if (res < 0)
            return res;
        _SETP = setPoint;
    }
    else
    {
        res = fumisComWriteByte(0x1C33, setPoint);
        if (res < 0)
            return res;
        _SETP = setPoint;
    }

    return 0;
}

int Palazzetti::iSetSetPointAtech(float setPoint)
{
    int res; //var_10

    if (setPoint < _SPLMIN)
        setPoint = _SPLMIN;

    if (setPoint > _SPLMAX)
        setPoint = _SPLMAX;

    if (_FLUID > 2)
        return 0;
    if (_FLUID == 2)
    {
        res = fumisComWriteByte(0x1C54, setPoint);
        if (res < 0)
            return res;
        _SETP = setPoint;
    }
    if (!_FLUID)
    {
        res = fumisComWriteByte(0x1C33, setPoint * 5);
        if (res < 0)
            return res;
        _SETP = setPoint;
    }
    else
    {
        res = fumisComWriteByte(0x1C33, setPoint);
        if (res < 0)
            return res;
        _SETP = setPoint;
    }

    return 0;
}

int Palazzetti::iReadTemperatureAtech()
{
    int res;     //var_1C
    byte buf[8]; //var_14
    uint16_t conv = 0;
    res = fumisComReadBuff(0x200A, buf, 8);
    if (res < 0)
        return res;
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
    _T1 = conv;
    _T1 /= 10.0f;

    conv = buf[7];
    conv <<= 8;
    conv += buf[6];
    _T2 = conv;
    _T2 /= 10.0f;

    uint16_t var_18;
    res = fumisComReadWord(0x2012, &var_18);
    if (res < 0)
        return res;
    _T5 = (int16_t)var_18;
    _T5 /= 10.0f;
    return 0;
}

int Palazzetti::iSwitchOnAtech()
{
    int res; //var_10
    if (_MOD < 0x1F4 || _MOD >= 0x258)
        res = fumisComWriteWord(0x2044, 2);

    else
        res = fumisComWriteWord(0x2044, 0x12);

    if (res < 0)
        return -1;
    return 0;
}

int Palazzetti::iSwitchOffAtech()
{
    int res; //var_10
    if (_MOD < 0x1F4 || _MOD >= 0x258)
        res = fumisComWriteWord(0x2044, 1);

    else
        res = fumisComWriteWord(0x2044, 0x11);

    if (res < 0)
        return -1;
    return 0;
}

int Palazzetti::iGetPelletQtUsedAtech()
{
    uint16_t var_10;
    int res = fumisComReadWord(0x2002, &var_10);
    if (res < 0)
        return res;
    _PQT = var_10;
    return 0;
}

int Palazzetti::iGetRoomFanAtech()
{
    uint16_t var_C;
    int res = fumisComReadByte(0x2036, &var_C);
    if (res < 0)
        return res;
    _F2L = var_C;

    res = fumisComReadWord(0x2004, &var_C);
    if (res < 0)
        return res;

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

    return 0;
}

int Palazzetti::iReadFansAtech()
{
    byte buf[8];
    int res = fumisComReadBuff(0x2024, buf, 8);
    if (res < 0)
    {
        _F1V = 0xFFFF;
        _F2V = 0xFFFF;
        _F1RPM = 0xFFFF;
        _F3S = -1;
        _F4S = -1;
        return res;
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

    res = iGetRoomFanAtech();
    if (res < 0)
    {
        _F1V = 0xFFFF;
        _F2V = 0xFFFF;
        _F1RPM = 0xFFFF;
        _F3S = -1;
        _F4S = -1;
        return res;
    }
    if (_BLEMBMODE > 1)
    {
        uint16_t buf2;
        res = fumisComReadWord(0x20A2, &buf2);
        if (res < 0)
        {
            _F1V = 0xFFFF;
            _F2V = 0xFFFF;
            _F1RPM = 0xFFFF;
            _F3S = -1;
            _F4S = -1;
            return res;
        }
        _F3S = buf2 & 0xFF;
        _F3S /= 5.0f; //Code indicates a multiplication by 0.2
        _F4S = buf2 >> 8;
        _F4S /= 5.0f; //Code indicates a multiplication by 0.2
    }

    return 0;
}

int Palazzetti::iGetPowerAtech()
{
    uint16_t var_C;

    int res = fumisComReadWord(0x202a, &var_C);
    if (res < 0)
        return res;

    _PWR = var_C & 0xFF;

    res = fumisComReadWord(wAddrFeederActiveTime, &var_C);
    if (res < 0)
        return res;

    _FDR = var_C;
    _FDR /= 10.0f;

    return 0;
}

int Palazzetti::iSetPowerAtech(uint16_t powerLevel)
{
    if (powerLevel < 1 || powerLevel > 5)
        return -1;

    int res = fumisComWriteByte(0x202a, powerLevel);
    if (res < 0)
        return res;

    _PWR = powerLevel;

    return 0;
}

void Palazzetti::iGetFanLimits()
{
    if (byte_47108A)
    {
        if (byte_47108A < _PWR)
        {
            _FAN1LMIN = _PWR - byte_47108A;
        }
        else
        {
            _FAN1LMIN = 0;
        }
    }
}

uint16_t Palazzetti::transcodeRoomFanSpeed(uint16_t roomFanSpeed, bool decode)
{
    if (roomFanSpeed == 0)
        return 7;
    if (roomFanSpeed == 7)
        return 0;
    if (roomFanSpeed == 6)
        return 6;

    return roomFanSpeed;
}

int Palazzetti::iSetRoomFanAtech(uint16_t roomFanSpeed)
{
    if (roomFanSpeed < 0 || roomFanSpeed > 7)
        return -1;

    int res;

    if (byte_47108A == 0 || _PWR < 4 || roomFanSpeed != 7 || (res = iSetPowerAtech(3)) >= 0)
    {
        res = fumisComWriteByte(0x2036, roomFanSpeed);
        if (res < 0)
            return res;
        res = iGetRoomFanAtech();
        if (res < 0)
            return res;
        _F2L = roomFanSpeed;
        return 0;
    }
    return res;
}

int Palazzetti::iSetRoomFan3Atech(uint16_t roomFan3Speed)
{
    if (roomFan3Speed > 5)
        return -1;

    int res;

    if (_FAN2TYPE == 4)
    {
        res = fumisComWriteWord(0x2004, (!_F4L ? 0 : 2) | (0 < roomFan3Speed));
        if (res < 0)
            return res;
        _F3L = (0 < roomFan3Speed);
    }
    else
    {
        if (_FAN2TYPE != 5)
            return -1;
        res = fumisComWriteWord(0x2004, roomFan3Speed);
        if (res < 0)
            return res;
        _F3L = roomFan3Speed;
    }

    return 0;
}

int Palazzetti::iSetRoomFan4Atech(uint16_t roomFan4Speed)
{
    if (roomFan4Speed > 5)
        return -1;

    int res;

    if (_FAN2TYPE == 4)
    {
        res = fumisComWriteWord(0x2004, (!roomFan4Speed ? 0 : 2) | (0 < _F3L));
        if (res < 0)
            return res;
        _F4L = (0 < roomFan4Speed);
    }
    else
    {
        if (_FAN2TYPE != 5 && _FAN2TYPE != 3)
            return -1;

        res = fumisComWriteByte((_FAN2TYPE == 5 ? 0x2005 : 0x2004), roomFan4Speed);
        if (res < 0)
            return res;
        _F4L = roomFan4Speed;
    }

    return 0;
}

int Palazzetti::iSetSilentModeAtech(uint16_t silentMode)
{
    if (silentMode > 0)
    {
        int res = iSetRoomFanAtech(7);
        if (res < 0)
            return res;
        res = iSetRoomFan3Atech(0);
        if (res < 0)
            return res;
        res = iSetRoomFan4Atech(0);
        if (res < 0)
            return res;
    }

    return 0;
}

int Palazzetti::iGetCountersAtech()
{

    byte var_18[8];
    int res = fumisComReadBuff(0x2066, var_18, 8);
    if (res < 0)
        return res;

    _IGN = var_18[1];
    _IGN <<= 8;
    _IGN += var_18[0];

    _POWERTIMEM = var_18[3];
    _POWERTIMEM <<= 8;
    _POWERTIMEM += var_18[2];

    _POWERTIMEH = var_18[5];
    _POWERTIMEH <<= 8;
    _POWERTIMEH += var_18[4];

    res = fumisComReadBuff(0x206E, var_18, 8);
    if (res < 0)
        return res;

    _HEATTIMEM = var_18[1];
    _HEATTIMEM <<= 8;
    _HEATTIMEM += var_18[0];

    _HEATTIMEH = var_18[3];
    _HEATTIMEH <<= 8;
    _HEATTIMEH += var_18[2];

    _SERVICETIMEM = var_18[7];
    _SERVICETIMEM <<= 8;
    _SERVICETIMEM += var_18[6];

    res = fumisComReadBuff(0x2076, var_18, 8);
    if (res < 0)
        return res;

    _SERVICETIMEH = var_18[1];
    _SERVICETIMEH <<= 8;
    _SERVICETIMEH += var_18[0];

    _OVERTMPERRORS = var_18[5];
    _OVERTMPERRORS <<= 8;
    _OVERTMPERRORS += var_18[4];

    _IGNERRORS = var_18[7];
    _IGNERRORS <<= 8;
    _IGNERRORS += var_18[6];

    res = fumisComReadBuff(0x2082, var_18, 8);
    if (res < 0)
        return res;

    _ONTIMEM = var_18[1];
    _ONTIMEM <<= 8;
    _ONTIMEM += var_18[0];

    _ONTIMEH = var_18[3];
    _ONTIMEH <<= 8;
    _ONTIMEH += var_18[2];

    uint16_t var_10;

    res = fumisComReadWord(0x2002, &var_10);
    if (res < 0)
        return res;
    _PQT = var_10;

    return 0;
}

int Palazzetti::iGetDPressDataAtech()
{
    uint16_t buf; //var_10
    int res = fumisComReadWord(0x2000, &buf);
    if (res < 0)
        return res;

    _DP_TARGET = buf;

    res = fumisComReadWord(0x2020, &buf);
    if (res < 0)
        return res;

    _DP_PRESS = buf;

    return 0;
}

int Palazzetti::iGetDateTimeAtech()
{
    byte buf[8]; //var_14
    int res = fumisComReadBuff(0x204E, buf, 8);
    if (res < 0)
        return res;

    sprintf(_STOVE_DATETIME, "%d-%02d-%02d %02d:%02d:%02d", (uint16_t)buf[6] + 2000, buf[5], buf[4], buf[2], buf[1], buf[0]);

    _STOVE_WDAY = buf[3];

    return 0;
}

int Palazzetti::iReadIOAtech()
{
    byte buf[8];
    int res = fumisComReadBuff(0x203c, buf, 8);
    if (res < 0)
        return res;
    
    _IN_I01 = (buf[0] & 0x01);
    _IN_I02 = (buf[0] & 0x02) >> 1;
    _IN_I03 = ((buf[0] & 0x04) >> 2) == 0;
    _IN_I04 = (buf[0] & 0x08) >> 3;

    _OUT_O01 = (buf[2] & 0x01);
    _OUT_O02 = (buf[2] & 0x02) >> 1;
    _OUT_O03 = (buf[2] & 0x04) >> 2;
    _OUT_O04 = (buf[2] & 0x08) >> 3;
    _OUT_O05 = (buf[2] & 0x10) >> 4; //0x16 : original implementation is wrong
    _OUT_O06 = (buf[2] & 0x20) >> 5; //0x32
    _OUT_O07 = (buf[2] & 0x40) >> 6; //0x64

    return 0;
}

int Palazzetti::iGetPumpRateAtech()
{
    uint16_t buf;
    int res = fumisComReadWord(0x2090,&buf);
    if (res < 0)
        return res;

    _PUMP = buf & 0xFF;

    return 0;
}

int Palazzetti::iGetChronoDataAtech()
{
    //TODO : complete implementation (first part is missing)

    uint16_t buf;
    int res = fumisComReadWord(0x207e, &buf);
    if (res < 0)
        return res;
    _CHRSTATUS = buf & 0x01;

    return 0;
}

int Palazzetti::iGetAllStatus(bool refreshStatus)
{
    int res = 0;

    if (refreshStatus)
    {
        res = iGetDateTimeAtech();
        if (res < 0)
            return res;
        res = iGetStatusAtech();
        if (res < 0)
            return res;
        res = iGetSetPointAtech();
        if (res < 0)
            return res;
        res = iReadFansAtech();
        if (res < 0)
            return res;
        res = iGetPowerAtech();
        if (res < 0)
            return res;
        res = iGetDPressDataAtech();
        if (res < 0)
            return res;
        res = iReadIOAtech();
        if (res < 0)
            return res;
        res = iReadTemperatureAtech();
        if (res < 0)
            return res;
        res = iGetPumpRateAtech();
        if (res < 0)
            return res;
        res = iGetPelletQtUsedAtech();
        if (res < 0)
            return res;
        // res = iGetChronoDataAtech();
        // if (res < 0)
        //     return res;
        // res = iGetErrorFlagAtech();
        // if (res < 0)
        //     return res;
        // if (_PSENSTYPE)
        // {
        //     res = iGetPelletLevelAtech();
        //     if (res < 0)
        //         return res;
        // }
    }
    

    if (!staticDataLoaded)
    {
        res = iUpdateStaticData();
        if (res < 0)
            return res;
    }

    return 0;
}

int Palazzetti::iGetParameterAtech(uint16_t paramToRead, uint16_t *paramValue)
{
    if (paramToRead > 0x69)
        return -1;

    int res = fumisComReadByte(paramToRead + 0x1C00, paramValue);
    if (res < 0)
        return res;

    return 0;
}

int Palazzetti::iSetParameterAtech(byte paramToWrite, byte paramValue)
{
    if (paramToWrite >= 0x6A)
        return -1;

    int res = fumisComWriteByte(paramToWrite + 0x1C00, paramValue);
    if (res < 0)
        return res;

    return 0;
}

int Palazzetti::iGetHiddenParameterAtech(uint16_t hParamToRead, uint16_t *hParamValue)
{
    if (hParamToRead > 0x6E)
        return -1;

    int res = fumisComReadWord((hParamToRead + 0xF00) * 2, hParamValue);
    if (res < 0)
        return res;

    return 0;
}

int Palazzetti::iSetHiddenParameterAtech(uint16_t hParamToWrite, uint16_t hParamValue)
{
    if (hParamToWrite >= 0x6E)
        return -1;

    int res = fumisComWriteWord((hParamToWrite + 0xF00) * 2, hParamValue);
    if (res < 0)
        return res;

    return 0;
}

//------------------------------------------
//Public part

bool Palazzetti::initialize()
{
    if (m_openSerial == nullptr ||
        m_closeSerial == nullptr ||
        m_selectSerial == nullptr ||
        m_readSerial == nullptr ||
        m_writeSerial == nullptr ||
        m_drainSerial == nullptr ||
        m_flushSerial == nullptr ||
        m_uSleep == nullptr)
    {
        return false;
    }

    if (_isInitialized)
        return true;

    if (iChkMBType() < 0)
    {
        iCloseUART();
        return false;
    }

    if (iInit() < 0)
    {
        iCloseUART();
        return false;
    }
    if (iUpdateStaticData() < 0)
    {
        iCloseUART();
        return false;
    }
    _isInitialized = true;
    return true;
}

bool Palazzetti::initialize(OPENSERIAL_SIGNATURE openSerial, CLOSESERIAL_SIGNATURE closeSerial, SELECTSERIAL_SIGNATURE selectSerial, READSERIAL_SIGNATURE readSerial, WRITESERIAL_SIGNATURE writeSerial, DRAINSERIAL_SIGNATURE drainSerial, FLUSHSERIAL_SIGNATURE flushSerial, USLEEP_SIGNATURE uSleep)
{
    m_openSerial = openSerial;
    m_closeSerial = closeSerial;
    m_selectSerial = selectSerial;
    m_readSerial = readSerial;
    m_writeSerial = writeSerial;
    m_drainSerial = drainSerial;
    m_flushSerial = flushSerial;
    m_uSleep = uSleep;

    return initialize();
}

bool Palazzetti::getStaticData(char (*SN)[28], byte *SNCHK, int *MBTYPE, uint16_t *MOD, uint16_t *VER, uint16_t *CORE, char (*FWDATE)[11], uint16_t *FLUID, uint16_t *SPLMIN, uint16_t *SPLMAX, byte *UICONFIG, byte *HWTYPE, uint16_t *DSPFWVER, byte *CONFIG, byte *PELLETTYPE, uint16_t *PSENSTYPE, byte *PSENSLMAX, byte *PSENSLTSH, byte *PSENSLMIN, byte *MAINTPROBE, byte *STOVETYPE, byte *FAN2TYPE, byte *FAN2MODE, byte *BLEMBMODE, byte *BLEDSPMODE, byte *CHRONOTYPE, byte *AUTONOMYTYPE, byte *NOMINALPWR)
{
    if (!initialize())
        return false;

    if (!staticDataLoaded)
    {
        if (iUpdateStaticData() < 0)
            return false;
    }

    //read LABEL : not needed
    //get network infos by running nwdata.sh : not needed

    if (SN)
        strcpy(*SN, _SN);
    if (SNCHK)
        *SNCHK = isValidSerialNumber(_SN);
    if (MBTYPE)
        *MBTYPE = _MBTYPE;

    //APLCONN : useless because overwritten by CBox lua code

    if (MOD)
        *MOD = _MOD;
    if (VER)
        *VER = _VER;
    if (CORE)
        *CORE = _CORE;
    if (FWDATE)
        sprintf(*FWDATE, "%d-%02d-%02d", _FWDATEY, _FWDATEM, _FWDATED);
    if (FLUID)
        *FLUID = _FLUID;
    if (SPLMIN)
        *SPLMIN = _SPLMIN;
    if (SPLMAX)
        *SPLMAX = _SPLMAX;
    if (UICONFIG)
        *UICONFIG = _UICONFIG;
    if (HWTYPE)
        *HWTYPE = _HWTYPE;
    if (DSPFWVER)
        *DSPFWVER = _DSPFWVER;
    if (CONFIG)
        *CONFIG = _PARAMS[0x4C];
    if (PELLETTYPE)
        *PELLETTYPE = _PARAMS[0x5C];
    if (PSENSTYPE)
        *PSENSTYPE = _PSENSTYPE;
    if (PSENSLMAX)
        *PSENSLMAX = _PARAMS[0x62];
    if (PSENSLTSH)
        *PSENSLTSH = _PARAMS[0x63];
    if (PSENSLMIN)
        *PSENSLMIN = _PARAMS[0x64];
    if (MAINTPROBE)
        *MAINTPROBE = _MAINTPROBE;
    if (STOVETYPE)
        *STOVETYPE = _STOVETYPE;
    if (FAN2TYPE)
        *FAN2TYPE = _FAN2TYPE;
    if (FAN2MODE)
        *FAN2MODE = _FAN2MODE;
    if (BLEMBMODE)
        *BLEMBMODE = _BLEMBMODE;
    if (BLEDSPMODE)
        *BLEDSPMODE = _BLEDSPMODE;
    if (CHRONOTYPE)
        *CHRONOTYPE = 5; //hardcoded value
    if (AUTONOMYTYPE)
        *AUTONOMYTYPE = _AUTONOMYTYPE;
    if (NOMINALPWR)
        *NOMINALPWR = _NOMINALPWR;
    return true;
}

//refreshStatus shoud be true if last call is over ~15sec
bool Palazzetti::getAllStatus(bool refreshStatus, int *MBTYPE, uint16_t *MOD, uint16_t *VER, uint16_t *CORE, char (*FWDATE)[11], char (*APLTS)[20], uint16_t *APLWDAY, byte *CHRSTATUS, uint16_t *STATUS, uint16_t *LSTATUS, bool *isMFSTATUSValid, uint16_t *MFSTATUS, float *SETP, byte *PUMP, uint16_t *PQT, uint16_t *F1V, uint16_t *F1RPM, uint16_t *F2L, uint16_t *F2LF, uint16_t (*FANLMINMAX)[6], uint16_t *F2V, bool *isF3LF4LValid, uint16_t *F3L, uint16_t *F4L, byte *PWR, float *FDR, uint16_t *DPT, uint16_t *DP, byte *IN, byte *OUT, float *T1, float *T2, float *T3, float *T4, float *T5, bool *isSNValid, char (*SN)[28])
{
    if (!initialize())
        return false;

    if (iGetAllStatus(refreshStatus) < 0)
        return false;

    if (MBTYPE)
        *MBTYPE = _MBTYPE;
    // MAC not needed
    if (MOD)
        *MOD = _MOD;
    if (VER)
        *VER = _VER;
    if (CORE)
        *CORE = _CORE;
    if (FWDATE)
        sprintf(*FWDATE, "%d-%02d-%02d", _FWDATEY, _FWDATEM, _FWDATED);
    if (APLTS)
        strcpy(*APLTS,_STOVE_DATETIME);
    if (APLWDAY)
        *APLWDAY = _STOVE_WDAY;
    if (CHRSTATUS)
        *CHRSTATUS = _CHRSTATUS;
    if (STATUS)
        *STATUS = _STATUS;
    if (LSTATUS)
        *LSTATUS = _LSTATUS;
    if (isMFSTATUSValid && MFSTATUS)
    {
        if (_STOVETYPE == 3 || _STOVETYPE == 4)
        {
            *isMFSTATUSValid = true;
            *MFSTATUS = _MFSTATUS;
        }
        else
            *isMFSTATUSValid = false;
    }
    if (SETP)
        *SETP = _SETP;
    if (PUMP)
        *PUMP = _PUMP;
    if (PQT)
        *PQT = _PQT;
    if (F1V)
        *F1V = _F1V;
    if (F1RPM)
        *F1RPM = _F1RPM;
    if (F2L)
        *F2L = transcodeRoomFanSpeed(_F2L, true);
    if (F2LF)
    {
        uint16_t tmp = transcodeRoomFanSpeed(_F2L, true);
        if (tmp < 6)
            *F2LF = 0;
        else
            *F2LF = tmp - 5;
    }
    iGetFanLimits();
    if (FANLMINMAX)
    {
        (*FANLMINMAX)[0] = _FAN1LMIN;
        (*FANLMINMAX)[1] = _FAN1LMAX;
        (*FANLMINMAX)[2] = _FAN2LMIN;
        (*FANLMINMAX)[3] = _FAN2LMAX;
        (*FANLMINMAX)[4] = _FAN3LMIN;
        (*FANLMINMAX)[5] = _FAN3LMAX;
    }
    if (F2V)
        *F2V = _F2V;
    if (isF3LF4LValid)
    {
        if (_FAN2TYPE > 2)
        {
            *isF3LF4LValid = true;
            if (F3L)
                *F3L = _F3L;
            if (F4L)
                *F4L = _F4L;
        }
        else
            *isF3LF4LValid = false;
    }
    if (PWR)
        *PWR = _PWR;
    if (FDR)
        *FDR = _FDR;
    if (DPT)
        *DPT = _DP_TARGET;
    if (DP)
        *DP = _DP_PRESS;
    if (IN)
        *IN = _IN_I04 << 3 | _IN_I03 << 2 | _IN_I02 << 1 | _IN_I01;
    if (OUT)
        *OUT = _OUT_O07 << 6 | _OUT_O06 << 5 | _OUT_O05 << 4 | _OUT_O04 << 3 | _OUT_O03 << 2 | _OUT_O02 << 1 | _OUT_O01;
    if (T1)
        *T1 = _T1;
    if (T2)
        *T2 = _T2;
    if (T3)
        *T3 = _T3;
    if (T4)
        *T4 = _T4;
    if (T5)
        *T5 = _T5;
    if (isSNValid && SN)
    {
        if (isValidSerialNumber(_SN))
        {
            *isSNValid = true;
            strcpy(*SN, _SN);
        }
        else
            *isSNValid = false;
    }

    return true;
}

bool Palazzetti::getSN(char (*SN)[28])
{
    if (!initialize())
        return false;
    
    if (SN)
        strcpy(*SN, _SN);
    return true;
}

bool Palazzetti::getModelVersion(uint16_t *MOD, uint16_t *VER, uint16_t *CORE, char (*FWDATE)[11])
{
    if (!initialize())
        return false;

    if (MOD)
        *MOD = _MOD;
    if (VER)
        *VER = _VER;
    if (CORE)
        *CORE = _CORE;
    if (FWDATE)
        sprintf(*FWDATE, "%d-%02d-%02d", _FWDATEY, _FWDATEM, _FWDATED);

    return true;
}

bool Palazzetti::getSetPoint(float *setPoint)
{
    if (!initialize())
        return false;

    if (iGetSetPointAtech() < 0)
        return false;
    if (setPoint)
        *setPoint = _SETP;
    return true;
}

bool Palazzetti::setSetpoint(byte setPoint, float *SETPReturn)
{
    if (!initialize())
        return false;

    if (iSetSetPointAtech((uint16_t)setPoint) < 0)
        return false;
    if (SETPReturn)
        *SETPReturn = _SETP;
    return true;
}

bool Palazzetti::getAllTemps(float *T1, float *T2, float *T3, float *T4, float *T5)
{
    if (!initialize())
        return false;

    if (iReadTemperatureAtech() < 0)
        return false;
    if (T1)
        *T1 = _T1;
    if (T2)
        *T2 = _T2;
    if (T3)
        *T3 = _T3;
    if (T4)
        *T4 = _T4;
    if (T5)
        *T5 = _T5;
    return true;
}

bool Palazzetti::getStatus(uint16_t *STATUS, uint16_t *LSTATUS)
{
    if (!initialize())
        return false;

    if (iGetStatusAtech() < 0)
        return false;
    if (STATUS)
        *STATUS = _STATUS;
    if (LSTATUS)
        *LSTATUS = _LSTATUS;
    return true;
}

bool Palazzetti::getPelletQtUsed(uint16_t *PQT)
{
    if (!initialize())
        return false;

    if (iGetPelletQtUsedAtech() < 0)
        return false;
    if (PQT)
        *PQT = _PQT;
    return true;
}

bool Palazzetti::getFanData(uint16_t *F1V, uint16_t *F2V, uint16_t *F1RPM, uint16_t *F2L, uint16_t *F2LF, bool *isF3SF4SValid, float *F3S, float *F4S, bool *isF3LF4LValid, uint16_t *F3L, uint16_t *F4L)
{
    if (!initialize())
        return false;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return false;

    if (iReadFansAtech() < 0)
        return false;

    if (F1V)
        *F1V = _F1V;
    if (F2V)
        *F2V = _F2V;
    if (F1RPM)
        *F1RPM = _F1RPM;
    if (F2L)
        *F2L = transcodeRoomFanSpeed(_F2L, true);

    if (F2LF)
    {
        uint16_t tmp = transcodeRoomFanSpeed(_F2L, true);
        if (tmp < 6)
            *F2LF = 0;
        else
            *F2LF = tmp - 5;
    }

    if (isF3SF4SValid)
    {
        if (_BLEMBMODE > 0xC)
        {
            *isF3SF4SValid = true;
            if (F3S)
                *F3S = _F3S;
            if (F4S)
                *F4S = _F4S;
        }
        else
            *isF3SF4SValid = false;
    }

    //Custom convenient code
    if (isF3LF4LValid)
    {
        if (_FAN2TYPE > 2)
        {
            *isF3LF4LValid = true;
            if (F3L)
                *F3L = _F3L;
            if (F4L)
                *F4L = _F4L;
        }
        else
            *isF3LF4LValid = false;
    }

    return true;
}

bool Palazzetti::getPower(byte *PWR, float *FDR)
{
    if (!initialize())
        return false;

    if (iGetPowerAtech() < 0)
        return false;

    if (PWR)
        *PWR = _PWR;
    if (FDR)
        *FDR = _FDR;

    return true;
}

bool Palazzetti::setPower(byte powerLevel, byte *PWRReturn, bool *isF2LReturnValid, uint16_t *F2LReturn, uint16_t (*FANLMINMAXReturn)[6])
{
    if (!initialize())
        return false;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return false;

    if (iSetPowerAtech(powerLevel) < 0)
        return false;

    if (byte_47108A)
    {
        if (iGetRoomFanAtech() < 0)
            return false;
    }

    if (PWRReturn)
        *PWRReturn = _PWR;
    if (isF2LReturnValid && F2LReturn)
    {
        if (byte_47108A)
        {
            *isF2LReturnValid = true;
            *F2LReturn = transcodeRoomFanSpeed(_F2L, true);
        }
        else
            *isF2LReturnValid = false;
    }

    // if (dword_470F0C != 0xB) //Micronova MBTYPE always equals 0
        iGetFanLimits();
    
    if (FANLMINMAXReturn)
    {
        (*FANLMINMAXReturn)[0] = _FAN1LMIN;
        (*FANLMINMAXReturn)[1] = _FAN1LMAX;
        (*FANLMINMAXReturn)[2] = _FAN2LMIN;
        (*FANLMINMAXReturn)[3] = _FAN2LMAX;
        (*FANLMINMAXReturn)[4] = _FAN3LMIN;
        (*FANLMINMAXReturn)[5] = _FAN3LMAX;
    }

    return true;
}

bool Palazzetti::setRoomFan(byte roomFanSpeed, bool *isPWRReturnValid, byte *PWRReturn, uint16_t *F2LReturn, uint16_t *F2LFReturn)
{
    if (!initialize())
        return false;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return false;

    if (iSetRoomFanAtech(transcodeRoomFanSpeed(roomFanSpeed, 0)) < 0)
        return false;

    if (isPWRReturnValid && PWRReturn)
    {
        if (byte_47108A)
        {
            *isPWRReturnValid = true;
            *PWRReturn = _PWR;
        }
        else
            *isPWRReturnValid = false;
    }
    if (F2LReturn)
        *F2LReturn = transcodeRoomFanSpeed(_F2L, true);
    if (F2LFReturn)
    {
        uint16_t tmp = transcodeRoomFanSpeed(_F2L, true);
        if (tmp < 6)
            *F2LFReturn = 0;
        else
            *F2LFReturn = tmp - 5;
    }

    return true;
}

bool Palazzetti::setRoomFan3(byte roomFan3Speed, uint16_t *F3LReturn)
{
    if (!initialize())
        return false;

    if (_MBTYPE >= 2)
        return false;

    if (iSetRoomFan3Atech(roomFan3Speed) < 0)
        return false;

    if (F3LReturn)
        *F3LReturn = _F3L;

    return true;
}

bool Palazzetti::setRoomFan4(byte roomFan4Speed, uint16_t *F4LReturn)
{
    if (!initialize())
        return false;

    if (_MBTYPE >= 2)
        return false;

    if (iSetRoomFan4Atech(roomFan4Speed) < 0)
        return false;

    if (F4LReturn)
        *F4LReturn = _F4L;

    return true;
}

bool Palazzetti::setSilentMode(byte silentMode, byte *SLNTReturn, byte *PWRReturn, uint16_t *F2LReturn, uint16_t *F2LFReturn, bool *isF3LF4LReturnValid, uint16_t *F3LReturn, uint16_t *F4LReturn)
{
    if (!initialize())
        return false;

    if (_MBTYPE >= 2)
        return false;

    if (iSetSilentModeAtech(silentMode) < 0)
        return false;
    if (SLNTReturn)
        *SLNTReturn = silentMode;
    if (PWRReturn)
        *PWRReturn = _PWR;
    if (F2LReturn)
        *F2LReturn = transcodeRoomFanSpeed(_F2L, true);
    if (F2LFReturn)
    {
        uint16_t tmp = transcodeRoomFanSpeed(_F2L, true);
        if (tmp < 6)
            *F2LFReturn = 0;
        else
            *F2LFReturn = tmp - 5;
    }
    if (isF3LF4LReturnValid)
    {
        if (_FAN2TYPE > 2)
        {
            *isF3LF4LReturnValid = true;
            if (F3LReturn)
                *F3LReturn = _F3L;
            if (F4LReturn)
                *F4LReturn = _F4L;
        }
        else
            *isF3LF4LReturnValid = false;
    }

    return true;
}

bool Palazzetti::getCounters(uint16_t *IGN, uint16_t *POWERTIMEh, uint16_t *POWERTIMEm, uint16_t *HEATTIMEh, uint16_t *HEATTIMEm, uint16_t *SERVICETIMEh, uint16_t *SERVICETIMEm, uint16_t *ONTIMEh, uint16_t *ONTIMEm, uint16_t *OVERTMPERRORS, uint16_t *IGNERRORS, uint16_t *PQT)
{
    if (!initialize())
        return false;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return false;

    if (iGetCountersAtech() < 0)
        return false;

    if (IGN)
        *IGN = _IGN;
    if (POWERTIMEh)
        *POWERTIMEh = _POWERTIMEH;
    if (POWERTIMEm)
        *POWERTIMEm = _POWERTIMEM;
    if (HEATTIMEh)
        *HEATTIMEh = _HEATTIMEH;
    if (HEATTIMEm)
        *HEATTIMEm = _HEATTIMEM;
    if (SERVICETIMEh)
        *SERVICETIMEh = _SERVICETIMEH;
    if (SERVICETIMEm)
        *SERVICETIMEm = _SERVICETIMEM;
    if (ONTIMEh)
        *ONTIMEh = _ONTIMEH;
    if (ONTIMEm)
        *ONTIMEm = _ONTIMEM;
    if (OVERTMPERRORS)
        *OVERTMPERRORS = _OVERTMPERRORS;
    if (IGNERRORS)
        *IGNERRORS = _IGNERRORS;
    if (PQT)
        *PQT = _PQT;

    return true;
}

bool Palazzetti::getDPressData(uint16_t *DP_TARGET, uint16_t *DP_PRESS)
{
    if (!initialize())
        return false;

    if (_MBTYPE >= 2)
        return false;

    if (iGetDPressDataAtech() < 0)
        return false;

    if (DP_TARGET)
        *DP_TARGET = _DP_TARGET;
    if (DP_PRESS)
        *DP_PRESS = _DP_PRESS;

    return true;
}

bool Palazzetti::getDateTime(char (*STOVE_DATETIME)[20], byte *STOVE_WDAY)
{
    if (!initialize())
        return false;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return false;

    if (iGetDateTimeAtech() < 0)
        return false;

    if (STOVE_DATETIME)
        strcpy(*STOVE_DATETIME, _STOVE_DATETIME);

    if (STOVE_WDAY)
        *STOVE_WDAY = _STOVE_WDAY;

    return true;
}

bool Palazzetti::getIO(byte *IN_I01, byte *IN_I02, byte *IN_I03, byte *IN_I04, byte *OUT_O01, byte *OUT_O02, byte *OUT_O03, byte *OUT_O04, byte *OUT_O05, byte *OUT_O06, byte *OUT_O07)
{
    if (!initialize())
        return false;

    if (_MBTYPE >= 2)
        return false;

    if (iReadIOAtech() < 0)
        return false;

    if (IN_I01)
        *IN_I01 = _IN_I01;
    if (IN_I02)
        *IN_I02 = _IN_I02;
    if (IN_I03)
        *IN_I03 = _IN_I03;
    if (IN_I04)
        *IN_I04 = _IN_I04;
    if (OUT_O01)
        *OUT_O01 = _OUT_O01;
    if (OUT_O02)
        *OUT_O02 = _OUT_O02;
    if (OUT_O03)
        *OUT_O03 = _OUT_O03;
    if (OUT_O04)
        *OUT_O04 = _OUT_O04;
    if (OUT_O05)
        *OUT_O05 = _OUT_O05;
    if (OUT_O06)
        *OUT_O06 = _OUT_O06;
    if (OUT_O07)
        *OUT_O07 = _OUT_O07;

    return true;
}

bool Palazzetti::getParameter(byte paramNumber, byte *paramValue)
{
    if (!initialize())
        return false;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return false;

    uint16_t tmpValue;

    if (iGetParameterAtech(paramNumber, &tmpValue) < 0)
        return false;

    //convert uint16_t to byte
    *paramValue = tmpValue;

    return true;
}
bool Palazzetti::setParameter(byte paramNumber, byte paramValue)
{
    if (!initialize())
        return false;

    if (_MBTYPE < 0 || _MBTYPE >= 2)
        return false;

    if (iSetParameterAtech(paramNumber, paramValue) < 0)
        return false;

    return true;
}
bool Palazzetti::getHiddenParameter(byte hParamNumber, uint16_t *hParamValue)
{
    if (!initialize())
        return false;

    if (_MBTYPE == 0x64) //if Micronova
        return false;

    if (iGetHiddenParameterAtech(hParamNumber, hParamValue) < 0)
        return false;

    return true;
}
bool Palazzetti::setHiddenParameter(byte hParamNumber, uint16_t hParamValue)
{
    if (!initialize())
        return false;

    if (_MBTYPE >= 2) //if Micronova
        return false;

    if (iSetHiddenParameterAtech(hParamNumber, hParamValue) < 0)
        return false;
    else if (hParamNumber < hparamsBufferSize)
    {
        _HPARAMS[hParamNumber] = hParamValue;
    }

    return true;
}

bool Palazzetti::getAllParameters(byte (*params)[0x6A])
{
    if (!initialize())
        return false;

    if (iUpdateStaticDataAtech() < 0)
        return false;

    memcpy(*params, _PARAMS, 0x6A * sizeof(byte));

    return true;
}

bool Palazzetti::getAllHiddenParameters(uint16_t (*hiddenParams)[0x6F])
{
    if (!initialize())
        return false;

    if (iUpdateStaticDataAtech() < 0)
        return false;

    memcpy(*hiddenParams, _HPARAMS, 0x6F * sizeof(uint16_t));

    return true;
}

bool Palazzetti::powerOff()
{
    if (!initialize())
        return false;

    if (iSwitchOffAtech() < 0)
        return false;
    return true;
}

bool Palazzetti::powerOn()
{
    if (!initialize())
        return false;

    if (iSwitchOnAtech() < 0)
        return false;
    return true;
}

//------------------------------------------
//Constructor
Palazzetti::Palazzetti() {}