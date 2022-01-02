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

    if (dword_46CAC0 == 2)
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

    if (!dword_46DAF4 && !dword_46DB08)
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
    switch (dword_42F214)
    {
    case 2:
        if (rxBuffer[0] != 0)
            return -1;
        res = iChkSum(rxBuffer);
        if (res < 0)
            return res;
        dword_42F214 = 3;
        return 0;
        break;
    case 4:
        if (rxBuffer[0] != 2)
            return -1;
        res = iChkSum(rxBuffer);
        if (res < 0)
            return res;
        dword_42F214 = 5;
        return 0;
        break;
    }
    return -1;
}

int Palazzetti::fumisCloseSerial()
{
    SERIALCOM_CloseComport();
    dword_42F214 = 0;
    return 0;
}

int Palazzetti::fumisOpenSerial()
{
    selectSerialTimeoutms = 2300;
    int res = SERIALCOM_OpenComport(0x9600);

    if (res >= 0 && (dword_46CAC0 != 1 || (res = SERIALCOM_Flush()) >= 0))
    {
        dword_42F218 = 0;
        dword_42F214 = 1;
        return 0;
    }
    else
    {
        dword_42F214 = 0;
        dword_42F218 = res;
    }

    return res;
}

int Palazzetti::fumisSendRequest(void *buf)
{
    if (dword_42F214 != 3)
        return -1;

    int totalSentBytes = 0; //var_18
    int sentBytes = 0;      //dummy value for firstrun
    while (totalSentBytes < 0xB && sentBytes != -1)
    {
        sentBytes = SERIALCOM_SendBuf((void *)((uint8_t *)buf + totalSentBytes), 0xB - totalSentBytes);
        totalSentBytes += sentBytes;
    }
    if (sentBytes < 0)
        dword_42F218 = sentBytes;
    return sentBytes; //not totalSentBytes...
}

int Palazzetti::fumisWaitRequest(void *buf)
{
    if (!dword_42F214)
        return -1;

    int nbReceivedBytes = 0; //var_18

    unsigned long startTime; //var_10
    startTime = millis();    //instead of time()
    do
    {
        if (dword_42F214 == 5 || dword_42F214 == 3)
            return nbReceivedBytes;

        nbReceivedBytes = 0;
        bzero(buf, 0xB);
        while ((nbReceivedBytes = SERIALCOM_ReceiveBuf(buf, 0xB)) < 0xB)
        {
            if (millis() - startTime > 3000)
                return -601;

            if (nbReceivedBytes < 0)
            {
                dword_42F218 = nbReceivedBytes;
                if (fumisCloseSerial() < 0)
                    return -1;
                if (fumisOpenSerial() < 0)
                    return -1;
                break;
            }
        };

        if (parseRxBuffer((byte *)buf) >= 0)
            continue;

        if (dword_42F214 == 4)
            return -601;

        if (millis() - startTime > 3000)
            return -601;

        if (dword_46CAC0 == 2 && SERIALCOM_Flush() < 0)
            return -601;

    } while (1);
}

int Palazzetti::fumisComReadBuff(uint16_t addrToRead, void *buf, size_t count)
{
    if (count != 8)
        return -1;

    bzero(buf, count);

    if (!dword_42F214)
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

        if (dword_46DB08 != 1)
        {
            dword_42F214 = 2;
            res = fumisWaitRequest((void *)var_28);
            if (res < 0)
            {
                if (res == -601)
                    continue;
                else
                {
                    dword_42F218 = res;
                    return res;
                }
            }
        }

        dword_42F214 = 3;
        bzero(var_28, 0xB);
        var_28[0] = 2;
        var_28[1] = addrToRead & 0xFF;
        var_28[2] = (addrToRead >> 8) & 0xFF;

        var_28[10] = var_28[0] + var_28[1] + var_28[2];
        res = fumisSendRequest(var_28);
        if (res < 0)
            return res;

        bzero(var_28, 32);
        dword_42F214 = 4;
        res = fumisWaitRequest(var_28);
        if (res >= 0)
        {
            memcpy(buf, var_28 + 1, count);
            return res;
        }

        if (res != -601)
        {
            dword_42F218 = res;
            return res;
        }
    }

    if (res >= 0)
        memcpy(buf, var_28 + 1, count);
    else
        dword_42F218 = res;
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
    if (!dword_42F214)
        return -1;

    int res;       //var_38
    byte buf[0xB]; //var_2C

    for (int i = 2; i > 0; i--) //i as var_34
    {
        dword_42F214 = 2;
        bzero(&buf, 0xB);
        res = fumisWaitRequest(&buf);
        if (res < 0)
        {
            dword_42F218 = res;
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

        dword_42F214 = 2;
        res = fumisWaitRequest(&buf);
        if (res < 0)
        {
            dword_42F218 = res;
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
    res = fumisComReadByte(0x201C, &dword_46DBC0);
    if (res < 0)
        return res;

    dword_46DBC4 = dword_46DBC0;

    if (dword_46DBC0 >= 0xC9)
        dword_46DBC4 = dword_46DBC0 + 0x3E8;

    if (byte_46DC62 == 3 || byte_46DC62 == 4)
    {
        res = fumisComReadWord(0x2008, &dword_46DBC8);
        if (res < 0)
            return res;
        if (dword_46DBC8 >= 2)
        {
            dword_46DBC4 = dword_46DBC8 + 0x1F4;
            if (dword_46DBC8 == 0x1FC)
                dword_46DBC4 += 0x3e8;
        }
    }

    if (dword_46DBC4 != 9)
        return 0;

    if (byte_46DC62 != 2 || (byte_46DC60 != 1 && byte_46DC60 != 3 && byte_46DC60 != 4))
    {
        dword_46DBC4 = 0x33;
    }

    return 0;
}

int Palazzetti::iChkMBType()
{

    dword_46DB08 = 0x64;
    //No Implementation of Micronova device there
    //skip directly to fumis detection

    dword_46DB08 = 0;
    if (fumisOpenSerial() < 0)
        return -1;

    ///*Trying to find MBTYPE_FUMIS_ALPHA65...*/
    if (iGetStatusAtech() < 0)
    {
        dword_46DB08 = -1;
        return -1;
    }
    ///*-->Found MBTYPE_FUMIS_ALPHA65 device!*/
    return 0;
}

int Palazzetti::iInit()
{
    if (dword_46DB08 < 0)
        return -10;

    if (dword_46DB08 < 2) //if Fumis MB
    {
        //dword_46DC04 = malloc(0xD0);
        byte_46DC3C = 0x6A;
        byte_46DC44 = 0x6F;
        //dword_46DC38 = malloc(byte_46DC3C aka 0x6A);
        //dword_46DC50 = malloc(byte_46DC3C aka 0x6A);
        //dword_46DC4C = malloc(byte_46DC3C aka 0x6A);
        //dword_46DC40 =  malloc(byte_46DC44<<1 aka 0xDE)
        //dword_46DC08 = malloc(0x16)
        //bzero(dword_46DC08,0x16);
        //dword_46DC0C = malloc(0x69);
        //bzero(dword_46DC0C,0x69);
    }

    //rest of iInit concerns Micronova which is not implemented

    return 0;
}

int Palazzetti::iGetSNAtech()
{
    //Complex and useless function ...
    //(mine return '000000000000000000000000000' as SN...)

    // int currentPosInSN = 0; //var_24
    // byte buf[8];            //var_14
    // int nbByteReaded = 0;   //var_1C
    // while (currentPosInSN < 0xE)
    // {
    //   nbByteReaded = fumisComReadBuff(0x2100 + currentPosInSN, buf, 8);
    //   if(nbByteReaded<0){
    //     byte_46DB1C[0]=0;
    //     return nbByteReaded;
    //   }
    // }
    return 0;
}

int Palazzetti::iGetMBTypeAtech()
{
    uint16_t buf; //var_C
    int res = 0;  //var_10
    res = fumisComReadWord(0x204C, &buf);
    if (res < 0)
        return res;

    dword_46DB10 = 6;
    if ((buf & 4) == 0)
    {
        if ((buf & 0x8000) != 0)
            dword_46DB10 = 7;
    }
    else
        dword_46DB10 = 5;
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

    byte_46DC65 = 2;
    byte_46DC61 = 0;

    res = fumisComReadWord(0x1ED4, &buf);
    if (res < 0)
        return res;
    byte buf2[8]; //var_18
    res = fumisComReadBuff(0x1E25, buf2, 8);
    if (res < 0)
        return res;

    byte_46DC67 = buf2[0];
    if (buf2[1] & 2)
        byte_46DC66 = 2;
    else
        byte_46DC66 = 1;

    byte var_1F = pdword_46DC38[0x4C];
    byte_46DC60 = var_1F;

    res = fumisComReadBuff(((var_1F - 1) << 2) + 0x1E36, buf2, 8);
    if (res < 0)
        return res;

    byte var_1E = ((buf2[0] & 0x20) > 0);

    byte_46DC63 = 1;
    byte_46DC61 = 0;

    if ((buf2[0] & 8) > 0)
    {
        if ((buf2[2] & 0x80) == 0)
            byte_46DC63 = 2;
        else
        {
            byte_46DC61 = 4;
            if ((buf & 0x8000) == 0)
            {
                byte_46DC63 = 4;
                uint16_t var_C;
                res = fumisComReadWord(0x204C, &var_C);
                if (res < 0)
                    return res;
                if ((var_C & 0x8000) > 0)
                    byte_46DC63 = 5;
            }
            else
                byte_46DC63 = 3;
        }
    }

    byte_46DC68 = 0;

    if (pdword_46DC38[0x69] > 0 && pdword_46DC38[0x69] < 6)
        byte_46DC68 = pdword_46DC38[0x69];

    byte_46DC69 = 1;
    byte_46DC6A = 5;
    byte_46DC6B = 0;
    byte_46DC6C = 1;
    byte_46DC6D = 0;
    byte_46DC6E = 1;
    byte_46DC64 = 1;

    if (pdword_46DC40[0x26 / 2] & 0x10)
        byte_46DC69 = 0;

    if (((pdword_46DC40[0x38 / 2] + ((var_1F - 1) << 1)) & 0x800) == 0)
        byte_46DC64 = 3;

    if (byte_46DC63 == 5 || byte_46DC63 == 3)
    {
        byte_46DC6C = 5;
        byte_46DC6E = 5;
    }

    if (var_1E)
        byte_46DC62 = 2;
    else
        byte_46DC62 = 1;

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
            dword_46DC48 = 0;
            if (var_1E)
                byte_46DC61 = 4;
        }
        else
            dword_46DC48 = 1;
    }
    else
    {
        if (var_1F == 5)
        {
            dword_46DC48 = 0;
            byte_46DC62 = 2;
            byte_46DC61 = 4;
            byte_46DC63 = 2;
            byte_46DC64 = 3;
        }
        else
        {
            dword_46DC48 = 2;
            byte_46DC62 = 2;
            byte_46DC61 = 4;
        }
    }

    if (pdword_46DC20 >= 0x1F5 && pdword_46DC20 < 0x258)
    {
        if (byte_46DC62 == 1)
        {
            byte_46DC62 = 3;
            byte_46DC61 = 4;
        }
        else
        {
            if (byte_46DC62 == 2)
            {
                byte_46DC62 = 4;
                if (var_1F == 2)
                    byte_46DC61 = 4;
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

    while (nbTotalBytesReaded < byte_46DC3C)
    {
        res = fumisComReadBuff(0x1C00 + nbTotalBytesReaded, buf, 8);
        if (res < 0)
            return res;

        for (byte i = 0; i < 8 && nbTotalBytesReaded < byte_46DC3C; i++)
        {
            pdword_46DC38[nbTotalBytesReaded] = buf[i];
            nbTotalBytesReaded++;
        }
    }
    ///*read all params OK*/

    nbTotalBytesReaded = 0; //var_838
    while (nbTotalBytesReaded < byte_46DC44)
    {
        res = fumisComReadBuff((nbTotalBytesReaded + 0xF00) << 1, buf, 8);
        if (res < 0)
            return res;

        for (byte i = 0; i < 8 && nbTotalBytesReaded < byte_46DC44; i += 2)
        {
            //not nbTotalBytesReaded * 2 because pdword_46DC40 is uint16_t[] type
            pdword_46DC40[nbTotalBytesReaded] = buf[i + 1];
            pdword_46DC40[nbTotalBytesReaded] <<= 8;
            pdword_46DC40[nbTotalBytesReaded] |= buf[i];
            nbTotalBytesReaded++;
        }
    }
    ///*read all hparams OK*/

    pdword_46DC24 = pdword_46DC40[4 / 2];
    pdword_46DC20 = pdword_46DC40[6 / 2];
    _CORE = pdword_46DC40[8 / 2];
    pdword_46DC14 = pdword_46DC40[0xA / 2];
    pdword_46DC18 = pdword_46DC40[0xC / 2];
    pdword_46DC1C = pdword_46DC40[0xE / 2];
    pdword_46DC34 = pdword_46DC40[0x1E / 2];
    pdword_46DC28 = pdword_46DC40[0x88 / 2];
    pdword_46DC2C = pdword_46DC40[0x8C / 2];
    pdword_46DC30 = pdword_46DC40[0x8E / 2];

    res = iGetStoveConfigurationAtech();
    if (res < 0)
        return res;

    if (pdword_46DC20 < 0x1F4 || pdword_46DC20 >= 0x258)
    {
        if (pdword_46DC24 < 0x1F)
            wAddrFeederActiveTime = 0x1FAE;
        else
        {
            if (pdword_46DC24 < 0x28)
                wAddrFeederActiveTime = 0x1FAC;
            else
                wAddrFeederActiveTime = 0x209A;
        }
        uint16_t var_18;
        res = fumisComReadWord(0x203A, &var_18);
        if (res < 0)
            return res;

        byte var_82C = (var_18 < 2) ? 0 : 1;

        dword_46DC5C = 0;

        if (0 < (pdword_46DC40[(pdword_46DC38[0x4C] + 0x1B) << 1 / 2] & 0x10))
            dword_46DC5C = 2;
        else if (var_82C)
            dword_46DC5C = 1;
    }
    else
        wAddrFeederActiveTime = 0x209A;

    nbTotalBytesReaded = 0; //var_838
    while (nbTotalBytesReaded < byte_46DC3C)
    {
        res = fumisComReadBuff((nbTotalBytesReaded + 0x80A2), buf, 8);
        if (res < 0)
            return res;

        for (byte i = 0; i < 8 && nbTotalBytesReaded < byte_46DC3C; i++)
        {
            pdword_46DC50[nbTotalBytesReaded] = buf[i];
            nbTotalBytesReaded++;
        }
    }

    nbTotalBytesReaded = 0; //var_838
    while (nbTotalBytesReaded < byte_46DC3C)
    {
        res = fumisComReadBuff((nbTotalBytesReaded + 0x810C), buf, 8);
        if (res < 0)
            return res;

        for (byte i = 0; i < 8 && nbTotalBytesReaded < byte_46DC3C; i++)
        {
            pdword_46DC4C[nbTotalBytesReaded] = buf[i];
            nbTotalBytesReaded++;
        }
    }

    if (dword_46DC48 < 2)
    {
        pdword_46DC54 = pdword_46DC50[0x33];
        pdword_46DC58 = pdword_46DC4C[0x33];
        if (!dword_46DC48)
        {
            pdword_46DC54 = (uint8_t)((double)pdword_46DC54 / 5.0);
            pdword_46DC58 = (uint8_t)((double)pdword_46DC58 / 5.0);
        }
        else if (dword_46DC48 == 2)
        {
            pdword_46DC54 = pdword_46DC50[0x54];
            pdword_46DC58 = pdword_46DC4C[0x54];
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

    if (dword_46DB08 < 0)
        return -1;

    if (dword_46DB08 < 2) //if fumis MBTYPE
    {
        int res = iUpdateStaticDataAtech();
        if (res < 0)
            return -1;
    }
    else if (dword_46DB08 == 0x64) //else Micronova not implemented so return -1
        return -1;
    else
        return -1;

    //open /etc/appliancelabel
    //if open /etc/appliancelabel is OK Then
    ////byte_46DB54=0; //to empty current label
    ////read 0x1F char from /etc/appliancelabel into &byte_46DB54
    ////close etc/appliancelabel
    //Else
    ////run 'touch /etc/appliancelabel'

    staticDataLoaded = 1; //flag that indicates Static Data are loaded

    //sendmsg 'GET STDT'

    return 0;
}

int Palazzetti::iCloseUART()
{
    if (dword_46DB08 < 2)
        return fumisCloseSerial();
    else
        return -1;
}

int Palazzetti::iGetSetPointAtech()
{
    if (byte_46DC62 == 1 && pdword_46DC38[0x4C] == 2)
    {
        dword_46DBDC = 0;
        return 0;
    }

    uint16_t data;
    int res;

    if (dword_46DC48 <= 2)
    {
        res = fumisComReadByte((dword_46DC48 < 2) ? 0x1C33 : 0x1C54, &data);
        if (res < 0)
            return res;
        float dataFloat = (int16_t)data;
        if (!dword_46DC48)
            dataFloat /= 5.0;
        dword_46DBDC = dataFloat;
    }
    return 0;
}

int Palazzetti::iSetSetPointAtech(uint16_t setPoint)
{
    int res; //var_10

    if (setPoint < pdword_46DC54)
        setPoint = pdword_46DC54;

    if (setPoint > pdword_46DC58)
        setPoint = pdword_46DC58;

    if (dword_46DC48 > 2)
        return 0;
    if (dword_46DC48 == 2)
    {
        res = fumisComWriteByte(0x1C54, setPoint);
        if (res < 0)
            return res;
        dword_46DBDC = setPoint;
    }
    if (!dword_46DC48)
    {
        res = fumisComWriteByte(0x1C33, setPoint * 5);
        if (res < 0)
            return res;
        dword_46DBDC = setPoint;
    }
    else
    {
        res = fumisComWriteByte(0x1C33, setPoint);
        if (res < 0)
            return res;
        dword_46DBDC = setPoint;
    }

    return 0;
}

int Palazzetti::iSetSetPointAtech(float setPoint)
{
    int res; //var_10

    if (setPoint < pdword_46DC54)
        setPoint = pdword_46DC54;

    if (setPoint > pdword_46DC58)
        setPoint = pdword_46DC58;

    if (dword_46DC48 > 2)
        return 0;
    if (dword_46DC48 == 2)
    {
        res = fumisComWriteByte(0x1C54, setPoint);
        if (res < 0)
            return res;
        dword_46DBDC = setPoint;
    }
    if (!dword_46DC48)
    {
        res = fumisComWriteByte(0x1C33, setPoint * 5);
        if (res < 0)
            return res;
        dword_46DBDC = setPoint;
    }
    else
    {
        res = fumisComWriteByte(0x1C33, setPoint);
        if (res < 0)
            return res;
        dword_46DBDC = setPoint;
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
    dword_46DB7C = (int16_t)conv;

    conv = buf[3];
    conv <<= 8;
    conv += buf[2];
    dword_46DB80 = (int16_t)conv;

    conv = buf[5];
    conv <<= 8;
    conv += buf[4];
    dword_46DB74 = conv;
    dword_46DB74 /= 10.0f;

    conv = buf[7];
    conv <<= 8;
    conv += buf[6];
    dword_46DB78 = conv;
    dword_46DB78 /= 10.0f;

    uint16_t var_18;
    res = fumisComReadWord(0x2012, &var_18);
    if (res < 0)
        return res;
    dword_46DB84 = (int16_t)var_18;
    dword_46DB84 /= 10.0f;
    return 0;
}

int Palazzetti::iSwitchOnAtech()
{
    int res; //var_10
    if (pdword_46DC20 < 0x1F4 || pdword_46DC20 >= 0x258)
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
    if (pdword_46DC20 < 0x1F4 || pdword_46DC20 >= 0x258)
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
    dword_46DBFC = var_10;
    return 0;
}

int Palazzetti::iGetRoomFanAtech()
{
    uint16_t var_C;
    int res = fumisComReadByte(0x2036, &var_C);
    if (res < 0)
        return res;
    dword_46DBA0 = var_C;

    res = fumisComReadWord(0x2004, &var_C);
    if (res < 0)
        return res;

    if (byte_46DC63 == 4)
    {
        dword_46DBA4 = var_C & 1;
        dword_46DBA8 = ((var_C & 2) != 0);
    }
    else if (byte_46DC63 == 5)
    {
        dword_46DBA4 = var_C & 0xFF;
        dword_46DBA8 = var_C >> 8;
    }
    else if (byte_46DC63 == 3)
    {
        dword_46DBA4 = 0;
        dword_46DBA8 = var_C;
    }
    else
    {
        dword_46DBA4 = 0;
        dword_46DBA8 = 0;
    }

    return 0;
}

int Palazzetti::iReadFansAtech()
{
    byte buf[8];
    int res = fumisComReadBuff(0x2024, buf, 8);
    if (res < 0)
    {
        dword_46DB94 = 0xFFFF;
        dword_46DB98 = 0xFFFF;
        dword_46DB9C = 0xFFFF;
        return res;
    }
    dword_46DB94 = buf[1];
    dword_46DB94 <<= 8;
    dword_46DB94 += buf[0];

    dword_46DB98 = buf[3];
    dword_46DB98 <<= 8;
    dword_46DB98 += buf[2];

    dword_46DB9C = buf[5];
    dword_46DB9C <<= 8;
    dword_46DB9C += buf[4];

    res = iGetRoomFanAtech();
    if (res < 0)
    {
        dword_46DB94 = 0xFFFF;
        dword_46DB98 = 0xFFFF;
        dword_46DB9C = 0xFFFF;
        return res;
    }

    return 0;
}

int Palazzetti::iGetPowerAtech()
{
    uint16_t var_C;

    int res = fumisComReadWord(0x202a, &var_C);
    if (res < 0)
        return res;

    byte_46DBAC = var_C & 0xFF;

    res = fumisComReadWord(wAddrFeederActiveTime, &var_C);
    if (res < 0)
        return res;

    dword_46DBB0 = var_C;
    dword_46DBB0 /= 10.0f;

    return 0;
}

int Palazzetti::iSetPowerAtech(uint16_t powerLevel)
{
    if (powerLevel < 0 || powerLevel > 5)
        return -1;

    int res = fumisComWriteByte(0x202a, powerLevel);
    if (res < 0)
        return res;

    byte_46DBAC = powerLevel;

    return 0;
}

void Palazzetti::iGetFanLimits()
{
    if (byte_46DC68)
    {
        if (byte_46DC68 < byte_46DBAC)
        {
            byte_46DC69 = byte_46DBAC - byte_46DC68;
        }
        else
        {
            byte_46DC69 = 0;
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

    if (byte_46DC68 == 0 || byte_46DBAC < 4 || roomFanSpeed != 7 || (res = iSetPowerAtech(3)) >= 0)
    {
        res = fumisComWriteByte(0x2036, roomFanSpeed);
        if (res < 0)
            return res;
        res = iGetRoomFanAtech();
        if (res < 0)
            return res;
        dword_46DBA0 = roomFanSpeed;
        return 0;
    }
    return res;
}

int Palazzetti::iSetRoomFan3Atech(uint16_t roomFan3Speed)
{
    if (roomFan3Speed > 5)
        return -1;

    int res;

    if (byte_46DC63 == 4)
    {
        res = fumisComWriteWord(0x2004, (!dword_46DBA8 ? 0 : 2) | (0 < roomFan3Speed));
        if (res < 0)
            return res;
        dword_46DBA4 = (0 < roomFan3Speed);
    }
    else
    {
        if (byte_46DC63 == 5)
            return -1;
        res = fumisComWriteWord(0x2004, roomFan3Speed);
        if (res < 0)
            return res;
        dword_46DBA4 = roomFan3Speed;
    }

    return 0;
}

int Palazzetti::iSetRoomFan4Atech(uint16_t roomFan4Speed)
{
    if (roomFan4Speed > 5)
        return -1;

    int res;

    if (byte_46DC63 == 4)
    {
        res = fumisComWriteWord(0x2004, (!roomFan4Speed ? 0 : 2) | (0 < dword_46DBA4));
        if (res < 0)
            return res;
        dword_46DBA8 = (0 < roomFan4Speed);
    }
    else
    {
        if (byte_46DC63 != 5 && byte_46DC63 != 3)
            return -1;

        res = fumisComWriteByte((byte_46DC63 == 5 ? 0x2005 : 0x2004), roomFan4Speed);
        if (res < 0)
            return res;
        dword_46DBA8 = roomFan4Speed;
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

int Palazzetti::iGetCounters()
{

    byte var_18[8];
    int res = fumisComReadBuff(0x2066, var_18, 8);
    if (res < 0)
        return res;

    pdword_46DC08_00 = var_18[1];
    pdword_46DC08_00 <<= 8;
    pdword_46DC08_00 += var_18[0];

    pdword_46DC08_04 = var_18[3];
    pdword_46DC08_04 <<= 8;
    pdword_46DC08_04 += var_18[2];

    pdword_46DC08_02 = var_18[5];
    pdword_46DC08_02 <<= 8;
    pdword_46DC08_02 += var_18[4];

    res = fumisComReadBuff(0x206E, var_18, 8);
    if (res < 0)
        return res;

    pdword_46DC08_08 = var_18[1];
    pdword_46DC08_08 <<= 8;
    pdword_46DC08_08 += var_18[0];

    pdword_46DC08_06 = var_18[3];
    pdword_46DC08_06 <<= 8;
    pdword_46DC08_06 += var_18[2];

    pdword_46DC08_0A = var_18[7];
    pdword_46DC08_0A <<= 8;
    pdword_46DC08_0A += var_18[6];

    res = fumisComReadBuff(0x2076, var_18, 8);
    if (res < 0)
        return res;

    pdword_46DC08_0C = var_18[1];
    pdword_46DC08_0C <<= 8;
    pdword_46DC08_0C += var_18[0];

    pdword_46DC08_12 = var_18[5];
    pdword_46DC08_12 <<= 8;
    pdword_46DC08_12 += var_18[4];

    pdword_46DC08_14 = var_18[7];
    pdword_46DC08_14 <<= 8;
    pdword_46DC08_14 += var_18[6];

    res = fumisComReadBuff(0x2082, var_18, 8);
    if (res < 0)
        return res;

    pdword_46DC08_0E = var_18[1];
    pdword_46DC08_0E <<= 8;
    pdword_46DC08_0E += var_18[0];

    pdword_46DC08_10 = var_18[3];
    pdword_46DC08_10 <<= 8;
    pdword_46DC08_10 += var_18[2];

    uint16_t var_10;

    res = fumisComReadWord(0x2002, &var_10);
    if (res < 0)
        return res;
    dword_46DBFC = var_10;

    return 0;
}

int Palazzetti::iGetDPressDataAtech()
{
    uint16_t buf; //var_10
    int res = fumisComReadWord(0x2000, &buf);
    if (res < 0)
        return res;

    dword_46DBB8 = buf;

    res = fumisComReadWord(0x2020, &buf);
    if (res < 0)
        return res;

    dword_46DBBC = buf;

    return 0;
}

int Palazzetti::iGetDateTimeAtech()
{
    byte buf[8]; //var_14
    int res = fumisComReadBuff(0x204E, buf, 8);
    if (res < 0)
        return res;

    sprintf(byte_46DBE0, "%d-%02d-%02d %02d:%02d:%02d", (uint16_t)buf[6] + 2000, buf[5], buf[4], buf[2], buf[1], buf[0]);

    dword_46DBF4 = buf[3];

    return 0;
}

int Palazzetti::iReadIOAtech()
{
    byte buf[8];
    int res = fumisComReadBuff(0x203c, buf, 8);
    if (res < 0)
        return res;
    
    byte_46DB88 = (buf[0] & 0x01);
    byte_46DB89 = (buf[0] & 0x02) >> 1;
    byte_46DB8A = ((buf[0] & 0x04) >> 2) == 0;
    byte_46DB8B = (buf[0] & 0x08) >> 3;

    byte_46DB8C = (buf[2] & 0x01);
    byte_46DB8D = (buf[2] & 0x02) >> 1;
    byte_46DB8E = (buf[2] & 0x04) >> 2;
    byte_46DB8F = (buf[2] & 0x08) >> 3;
    byte_46DB90 = (buf[2] & 0x10) >> 4; //0x16 : original implementation is wrong
    byte_46DB91 = (buf[2] & 0x20) >> 5; //0x32
    byte_46DB92 = (buf[2] & 0x40) >> 6; //0x64

    return 0;
}

int Palazzetti::iGetPumpRateAtech()
{
    uint16_t buf;
    int res = fumisComReadWord(0x2090,&buf);
    if (res < 0)
        return res;

    dword_46DBB4 = buf & 0xFF;

    return 0;
}

int Palazzetti::iGetChronoDataAtech()
{
    //TODO : complete implementation (first part is missing)

    uint16_t buf;
    int res = fumisComReadWord(0x2020, &buf);
    if (res < 0)
        return res;
    dword_46DBF8 = buf & 0x01;

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
        if (res < 0)
            return res;
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

bool Palazzetti::getStaticData(int *MBTYPE, uint16_t *MOD, uint16_t *VER, uint16_t *CORE, char (&FWDATE)[11], uint16_t *FLUID, uint16_t *SPLMIN, uint16_t *SPLMAX, byte *UICONFIG, uint16_t *HWTYPE, uint16_t *DSPFWVER, byte *CONFIG, byte *PELLETTYPE, uint16_t *PSENSTYPE, byte *PSENSLMAX, byte *PSENSLTSH, byte *PSENSLMIN, byte *MAINTPROBE, byte *STOVETYPE, byte *FAN2TYPE, byte *FAN2MODE, byte *CHRONOTYPE, byte *AUTONOMYTYPE, byte *NOMINALPWR)
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

    // if (SN)
    //     strcpy(SN, byte_46DB1C);
    if (MBTYPE)
        *MBTYPE = dword_46DB08;

    //APLCONN : useless because overwritten by CBox lua code

    if (MOD)
        *MOD = pdword_46DC20;
    if (VER)
        *VER = pdword_46DC24;
    if (CORE)
        *CORE = _CORE;
    sprintf(FWDATE, "%d-%02d-%02d", pdword_46DC1C, pdword_46DC18, pdword_46DC14);
    if (FLUID)
        *FLUID = dword_46DC48;
    if (SPLMIN)
        *SPLMIN = pdword_46DC54;
    if (SPLMAX)
        *SPLMAX = pdword_46DC58;
    if (UICONFIG)
        *UICONFIG = byte_46DC60;
    if (HWTYPE)
        *HWTYPE = dword_46DB10;
    if (DSPFWVER)
        *DSPFWVER = _DSPFWVER;
    if (CONFIG)
        *CONFIG = pdword_46DC38[0x4C];
    if (PELLETTYPE)
        *PELLETTYPE = pdword_46DC38[0x5C];
    if (PSENSTYPE)
        *PSENSTYPE = dword_46DC5C;
    if (PSENSLMAX)
        *PSENSLMAX = pdword_46DC38[0x62];
    if (PSENSLTSH)
        *PSENSLTSH = pdword_46DC38[0x63];
    if (PSENSLMIN)
        *PSENSLMIN = pdword_46DC38[0x64];
    if (MAINTPROBE)
        *MAINTPROBE = byte_46DC61;
    if (STOVETYPE)
        *STOVETYPE = byte_46DC62;
    if (FAN2TYPE)
        *FAN2TYPE = byte_46DC63;
    if (FAN2MODE)
        *FAN2MODE = byte_46DC64;
    if (CHRONOTYPE)
        *CHRONOTYPE = 5; //hardcoded value
    if (AUTONOMYTYPE)
        *AUTONOMYTYPE = byte_46DC66;
    if (NOMINALPWR)
        *NOMINALPWR = byte_46DC67;
    return true;
}

//refreshStatus shoud be true if last call is over ~15sec
bool Palazzetti::getAllStatus(bool refreshStatus, int *MBTYPE, uint16_t *MOD, uint16_t *VER, uint16_t *CORE, char (&FWDATE)[11], char (&APLTS)[20], uint16_t *APLWDAY, byte *CHRSTATUS, uint16_t *STATUS, uint16_t *LSTATUS, bool *isMFSTATUSValid, uint16_t *MFSTATUS, float *SETP, byte *PUMP, uint16_t *PQT, uint16_t *F1V, uint16_t *F1RPM, uint16_t *F2L, uint16_t *F2LF, uint16_t (&FANLMINMAX)[6], uint16_t *F2V, bool *isF3LF4LValid, uint16_t *F3L, uint16_t *F4L, byte *PWR, float *FDR, uint16_t *DPT, uint16_t *DP, byte *IN, byte *OUT, float *T1, float *T2, float *T3, float *T4, float *T5)
{
    if (!initialize())
        return false;

    if (iGetAllStatus(refreshStatus) < 0)
        return false;

    if (MBTYPE)
        *MBTYPE = dword_46DB08;
    // MAC not needed
    if (MOD)
        *MOD = pdword_46DC20;
    if (VER)
        *VER = pdword_46DC24;
    if (CORE)
        *CORE = _CORE;
    sprintf(FWDATE, "%d-%02d-%02d", pdword_46DC1C, pdword_46DC18, pdword_46DC14);
    sprintf(APLTS,byte_46DBE0);
    if (APLWDAY)
        *APLWDAY = dword_46DBF4;
    if (CHRSTATUS)
        *CHRSTATUS = dword_46DBF8;
    if (STATUS)
        *STATUS = dword_46DBC0;
    if (LSTATUS)
        *LSTATUS = dword_46DBC4;
    if (isMFSTATUSValid && MFSTATUS)
    {
        if (byte_46DC62 == 3 || byte_46DC62 == 4)
        {
            *isMFSTATUSValid = true;
            *MFSTATUS = dword_46DBC8;
        }
        else
            *isMFSTATUSValid = false;
    }
    if (SETP)
        *SETP = dword_46DBDC;
    if (PUMP)
        *PUMP = dword_46DBB4;
    if (PQT)
        *PQT = dword_46DBFC;
    if (F1V)
        *F1V = dword_46DB94;
    if (F1RPM)
        *F1RPM = dword_46DB9C;
    if (F2L)
        *F2L = transcodeRoomFanSpeed(dword_46DBA0, true);
    if (F2LF)
    {
        uint16_t tmp = transcodeRoomFanSpeed(dword_46DBA0, true);
        if (tmp < 6)
            *F2LF = 0;
        else
            *F2LF = tmp - 5;
    }
    iGetFanLimits();
    FANLMINMAX[0] = byte_46DC69;
    FANLMINMAX[1] = byte_46DC6A;
    FANLMINMAX[2] = byte_46DC6B;
    FANLMINMAX[3] = byte_46DC6C;
    FANLMINMAX[4] = byte_46DC6D;
    FANLMINMAX[5] = byte_46DC6E;
    if (F2V)
        *F2V = dword_46DB98;
    if (isF3LF4LValid)
    {
        if (byte_46DC63 > 2)
        {
            *isF3LF4LValid = true;
            if (F3L)
                *F3L = dword_46DBA4;
            if (F4L)
                *F4L = dword_46DBA8;
        }
        else
            *isF3LF4LValid = false;
    }
    if (PWR)
        *PWR = byte_46DBAC;
    if (FDR)
        *FDR = dword_46DBB0;
    if (DPT)
        *DPT = dword_46DBB8;
    if (DP)
        *DP = dword_46DBBC;
    if (IN)
        *IN = byte_46DB8B << 3 | byte_46DB8A << 2 | byte_46DB89 << 1 | byte_46DB88;
    if (OUT)
        *OUT = byte_46DB92 << 6 | byte_46DB91 << 5 | byte_46DB90 << 4 | byte_46DB8F << 3 | byte_46DB8E << 2 | byte_46DB8D << 1 | byte_46DB8C;
    if (T1)
        *T1 = dword_46DB74;
    if (T2)
        *T2 = dword_46DB78;
    if (T3)
        *T3 = dword_46DB7C;
    if (T4)
        *T4 = dword_46DB80;
    if (T5)
        *T5 = dword_46DB84;
    return true;
}

bool Palazzetti::getSetPoint(float *setPoint)
{
    if (!initialize())
        return false;

    if (iGetSetPointAtech() < 0)
        return false;
    if (setPoint)
        *setPoint = dword_46DBDC;
    return true;
}

bool Palazzetti::setSetpoint(byte setPoint)
{
    if (!initialize())
        return false;

    return iSetSetPointAtech((uint16_t)setPoint) >= 0;
}

bool Palazzetti::getAllTemps(float *T1, float *T2, float *T3, float *T4, float *T5)
{
    if (!initialize())
        return false;

    if (iReadTemperatureAtech() < 0)
        return false;
    if (T1)
        *T1 = dword_46DB74;
    if (T2)
        *T2 = dword_46DB78;
    if (T3)
        *T3 = dword_46DB7C;
    if (T4)
        *T4 = dword_46DB80;
    if (T5)
        *T5 = dword_46DB84;
    return true;
}

bool Palazzetti::getStatus(uint16_t *STATUS, uint16_t *LSTATUS)
{
    if (!initialize())
        return false;

    if (iGetStatusAtech() < 0)
        return false;
    if (STATUS)
        *STATUS = dword_46DBC0;
    if (LSTATUS)
        *LSTATUS = dword_46DBC4;
    return true;
}

bool Palazzetti::getPelletQtUsed(uint16_t *PQT)
{
    if (!initialize())
        return false;

    if (iGetPelletQtUsedAtech() < 0)
        return false;
    if (PQT)
        *PQT = dword_46DBFC;
    return true;
}

bool Palazzetti::getFanData(uint16_t *F1V, uint16_t *F2V, uint16_t *F1RPM, uint16_t *F2L, uint16_t *F2LF, bool *isF3LF4LValid, uint16_t *F3L, uint16_t *F4L)
{
    if (!initialize())
        return false;

    if (dword_46DB08 < 0 || dword_46DB08 >= 2)
        return false;

    if (iReadFansAtech() < 0)
        return false;

    if (F1V)
        *F1V = dword_46DB94;
    if (F2V)
        *F2V = dword_46DB98;
    if (F1RPM)
        *F1RPM = dword_46DB9C;
    if (F2L)
        *F2L = transcodeRoomFanSpeed(dword_46DBA0, true);

    if (F2LF)
    {
        uint16_t tmp = transcodeRoomFanSpeed(dword_46DBA0, true);
        if (tmp < 6)
            *F2LF = 0;
        else
            *F2LF = tmp - 5;
    }

    if (isF3LF4LValid)
    {
        if (byte_46DC63 > 2)
        {
            *isF3LF4LValid = true;
            if (F3L)
                *F3L = dword_46DBA4;
            if (F4L)
                *F4L = dword_46DBA8;
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
        *PWR = byte_46DBAC;
    if (FDR)
        *FDR = dword_46DBB0;

    return true;
}

bool Palazzetti::setPower(byte powerLevel)
{
    if (!initialize())
        return false;

    if (dword_46DB08 < 0 || dword_46DB08 >= 2)
        return false;

    if (iSetPowerAtech(powerLevel) < 0)
        return false;

    if (byte_46DC68)
    {
        if (iGetRoomFanAtech() < 0)
            return false;
    }

    if (dword_46DB0C != 0xB)
        iGetFanLimits();

    return true;
}

bool Palazzetti::setRoomFan(byte roomFanSpeed)
{
    if (!initialize())
        return false;

    if (dword_46DB08 < 0 || dword_46DB08 >= 2)
        return false;

    if (iSetRoomFanAtech(transcodeRoomFanSpeed(roomFanSpeed, 0)) < 0)
        return false;

    return true;
}

bool Palazzetti::setRoomFan3(byte roomFan3Speed)
{
    if (!initialize())
        return false;

    if (dword_46DB08 >= 2)
        return false;

    if (iSetRoomFan3Atech(roomFan3Speed) < 0)
        return false;

    return true;
}

bool Palazzetti::setRoomFan4(byte roomFan4Speed)
{
    if (!initialize())
        return false;

    if (dword_46DB08 >= 2)
        return false;

    if (iSetRoomFan4Atech(roomFan4Speed) < 0)
        return false;

    return true;
}

bool Palazzetti::setSilentMode(byte silentMode)
{
    if (!initialize())
        return false;

    if (dword_46DB08 >= 2)
        return false;

    if (iSetSilentModeAtech(silentMode) < 0)
        return false;

    return true;
}

bool Palazzetti::getCounters(uint16_t *IGN, uint16_t *POWERTIMEh, uint16_t *POWERTIMEm, uint16_t *HEATTIMEh, uint16_t *HEATTIMEm, uint16_t *SERVICETIMEh, uint16_t *SERVICETIMEm, uint16_t *ONTIMEh, uint16_t *ONTIMEm, uint16_t *OVERTMPERRORS, uint16_t *IGNERRORS, uint16_t *PQT)
{
    if (!initialize())
        return false;

    if (dword_46DB08 < 0 || dword_46DB08 >= 2)
        return false;

    if (iGetCounters() < 0)
        return false;

    if (IGN)
        *IGN = pdword_46DC08_00;
    if (POWERTIMEh)
        *POWERTIMEh = pdword_46DC08_02;
    if (POWERTIMEm)
        *POWERTIMEm = pdword_46DC08_04;
    if (HEATTIMEh)
        *HEATTIMEh = pdword_46DC08_06;
    if (HEATTIMEm)
        *HEATTIMEm = pdword_46DC08_08;
    if (SERVICETIMEh)
        *SERVICETIMEh = pdword_46DC08_0C;
    if (SERVICETIMEm)
        *SERVICETIMEm = pdword_46DC08_0A;
    if (ONTIMEh)
        *ONTIMEh = pdword_46DC08_10;
    if (ONTIMEm)
        *ONTIMEm = pdword_46DC08_0E;
    if (OVERTMPERRORS)
        *OVERTMPERRORS = pdword_46DC08_12;
    if (IGNERRORS)
        *IGNERRORS = pdword_46DC08_14;
    if (PQT)
        *PQT = dword_46DBFC;

    return true;
}

bool Palazzetti::getDPressData(uint16_t *DP_TARGET, uint16_t *DP_PRESS)
{
    if (!initialize())
        return false;

    if (dword_46DB08 >= 2)
        return false;

    if (iGetDPressDataAtech() < 0)
        return false;

    if (DP_TARGET)
        *DP_TARGET = dword_46DBB8;
    if (DP_PRESS)
        *DP_PRESS = dword_46DBBC;

    return true;
}

bool Palazzetti::getDateTime(char (&STOVE_DATETIME)[20], byte *STOVE_WDAY)
{
    if (!initialize())
        return false;

    if (dword_46DB08 < 0 || dword_46DB08 >= 2)
        return false;

    if (iGetDateTimeAtech() < 0)
        return false;

    strcpy(STOVE_DATETIME, byte_46DBE0);

    if (STOVE_WDAY)
        *STOVE_WDAY = dword_46DBF4;

    return true;
}

bool Palazzetti::getIO(byte *IN_I01, byte *IN_I02, byte *IN_I03, byte *IN_I04, byte *OUT_O01, byte *OUT_O02, byte *OUT_O03, byte *OUT_O04, byte *OUT_O05, byte *OUT_O06, byte *OUT_O07)
{
    if (!initialize())
        return false;

    if (dword_46DB08 >= 2)
        return false;

    if (iReadIOAtech() < 0)
        return false;

    if (IN_I01)
        *IN_I01 = byte_46DB88;
    if (IN_I02)
        *IN_I02 = byte_46DB89;
    if (IN_I03)
        *IN_I03 = byte_46DB8A;
    if (IN_I04)
        *IN_I04 = byte_46DB8B;
    if (OUT_O01)
        *OUT_O01 = byte_46DB8C;
    if (OUT_O02)
        *OUT_O02 = byte_46DB8D;
    if (OUT_O03)
        *OUT_O03 = byte_46DB8E;
    if (OUT_O04)
        *OUT_O04 = byte_46DB8F;
    if (OUT_O05)
        *OUT_O05 = byte_46DB90;
    if (OUT_O06)
        *OUT_O06 = byte_46DB91;
    if (OUT_O07)
        *OUT_O07 = byte_46DB92;

    return true;
}

bool Palazzetti::getParameter(byte paramNumber, byte *paramValue)
{
    if (!initialize())
        return false;

    if (dword_46DB08 < 0 || dword_46DB08 >= 2)
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

    if (dword_46DB08 < 0 || dword_46DB08 >= 2)
        return false;

    if (iSetParameterAtech(paramNumber, paramValue) < 0)
        return false;

    return true;
}
bool Palazzetti::getHiddenParameter(byte hParamNumber, uint16_t *hParamValue)
{
    if (!initialize())
        return false;

    if (dword_46DB08 == 0x64) //if Micronova
        return false;

    if (iGetHiddenParameterAtech(hParamNumber, hParamValue) < 0)
        return false;

    return true;
}
bool Palazzetti::setHiddenParameter(byte hParamNumber, uint16_t hParamValue)
{
    if (!initialize())
        return false;

    if (dword_46DB08 >= 2) //if Micronova
        return false;

    if (iSetHiddenParameterAtech(hParamNumber, hParamValue) < 0)
        return false;
    else if (hParamNumber < byte_46DC44)
    {
        pdword_46DC40[hParamNumber] = hParamValue;
    }

    return true;
}

bool Palazzetti::getAllParameters(byte (&params)[0x6A])
{
    if (!initialize())
        return false;

    if (iUpdateStaticDataAtech() < 0)
        return false;

    memcpy(params, pdword_46DC38, 0x6A * sizeof(byte));

    return true;
}

bool Palazzetti::getAllHiddenParameters(uint16_t (&hiddenParams)[0x6F])
{
    if (!initialize())
        return false;

    if (iUpdateStaticDataAtech() < 0)
        return false;

    memcpy(hiddenParams, pdword_46DC40, 0x6F * sizeof(uint16_t));

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