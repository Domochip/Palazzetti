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

int Palazzetti::iGetStoveConfigurationAtech()
{
    byte_46DC65 = 2;
    byte_46DC61 = 0;

    uint16_t buf; //var_10
    int res = 0;  //var_24
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
        if ((buf2[2] & 0x80) > 0)
            byte_46DC63 = 2;
        else
        {
            byte_46DC61 = 4;
            if ((buf & 0x8000) > 0)
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
    pdword_46DC14 = pdword_46DC40[0xA / 2];
    pdword_46DC18 = pdword_46DC40[0xC / 2];
    pdword_46DC1C = pdword_46DC40[0xE / 2];
    pdword_46DC34 = pdword_46DC40[0x1E / 2];
    pdword_46DC28 = pdword_46DC40[0x88 / 2];
    pdword_46DC2C = pdword_46DC40[0x8C / 2];
    pdword_46DC30 = pdword_46DC40[0x8E / 2];

    res = iGetStoveConfigurationAtech(); //TODO
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
            pdword_46DC54 = (uint8_t)((double)int8_t(pdword_46DC54) / 5.0);
            pdword_46DC58 = (uint8_t)((double)int8_t(pdword_46DC58) / 5.0);
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
    ////sendmsg 'GET STDT' which call again iUpdateStaticData()

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

int Palazzetti::iSetSetPointAtech(int16_t setPoint)
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
    res = fumisComReadBuff(0x200A, &buf, 8);
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

    if (m_isInitialized)
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
    m_isInitialized = true;
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
    return iSetSetPointAtech(setPoint) >= 0;
}

bool Palazzetti::readTemperature(float *T1, float *T2, float *T3, float *T4, float *T5)
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