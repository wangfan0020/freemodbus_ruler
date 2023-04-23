/*
 * FreeModbus Libary: user callback functions and buffer define in slave mode
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: user_mb_app.c,v 1.60 2013/11/23 11:49:05 Armink $
 */
#include "user_mb_app.h"

/*------------------------Slave mode use these variables----------------------*/
//Slave mode:DiscreteInputs variables
static USHORT   usSDiscInStart                               = S_DISCRETE_INPUT_START;
//Slave mode:Coils variables
static USHORT   usSCoilStart                                  = S_COIL_START;
//Slave mode:InputRegister variables
static USHORT   usSRegInStart                                = S_REG_INPUT_START;
//Slave mode:HoldingRegister variables
static USHORT   usSRegHoldStart                              = S_REG_HOLDING_START;


static UCHAR    *ucSDiscInBuf=0;
static UCHAR    *ucSCoilBuf=0;
static USHORT   *usSRegInBuf=0;
static USHORT   *usSRegHoldBuf=0;


static USHORT   REG_INPUT_NREGS = 0;
static USHORT   REG_HOLDING_NREGS = 0;
static USHORT   COIL_NCOILS = 0;
static USHORT   DISCRETE_INPUT_NDISCRETES = 0;




eMBErrorCode eMBUserInitCfg( eMBFuncCofg cfg)
{

    ucSDiscInBuf =cfg.ucSDiscInBuf;
    DISCRETE_INPUT_NDISCRETES =cfg.DiscInSize;

    ucSCoilBuf = cfg.ucSCoilBuf;
    COIL_NCOILS= cfg.CoilSize;

    usSRegInBuf = cfg.usSRegInBuf;
    REG_INPUT_NREGS = cfg.RegInSize;

    usSRegHoldBuf = cfg.usSRegHoldBuf;
    REG_HOLDING_NREGS = cfg.RegHoldSize;
}
/**
 * Modbus slave input register callback function.
 *
 * @param pucRegBuffer input register buffer
 * @param usAddress input register address
 * @param usNRegs input register number
 *
 * @return result
 */
eMBErrorCode eMBRegInputCB(UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          iRegIndex;
    USHORT *        pusRegInputBuf;

    if(usSRegInBuf == 0)
        return MB_EILLSTATE;
    pusRegInputBuf = usSRegInBuf;
    /* it already plus one in modbus function method. */
    usAddress--;

    if ((usAddress >= usSRegInStart)
            && (usAddress + usNRegs <= usSRegInStart + REG_INPUT_NREGS))
    {
        iRegIndex = usAddress - usSRegInStart;
        while (usNRegs > 0)
        {
            *pucRegBuffer++ = (UCHAR) (pusRegInputBuf[iRegIndex] >> 8);
            *pucRegBuffer++ = (UCHAR) (pusRegInputBuf[iRegIndex] & 0xFF);
            iRegIndex++;
            usNRegs--;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}

/**
 * Modbus slave holding register callback function.
 *
 * @param pucRegBuffer holding register buffer
 * @param usAddress holding register address
 * @param usNRegs holding register number
 * @param eMode read or write
 *
 * @return result
 */
eMBErrorCode eMBRegHoldingCB(UCHAR * pucRegBuffer, USHORT usAddress,
        USHORT usNRegs, eMBRegisterMode eMode)
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          iRegIndex;
    USHORT *        pusRegHoldingBuf;
    if(usSRegHoldBuf == 0)
        return MB_EILLSTATE;
    pusRegHoldingBuf = usSRegHoldBuf;

    /* it already plus one in modbus function method. */
    usAddress--;

    if ((usAddress >= usSRegHoldStart)
            && (usAddress + usNRegs <= usSRegHoldStart + REG_HOLDING_NREGS))
    {
        iRegIndex = usAddress - usSRegHoldStart;
        switch (eMode)
        {
        /* read current register values from the protocol stack. */
        case MB_REG_READ:
            while (usNRegs > 0)
            {
                *pucRegBuffer++ = (UCHAR) (pusRegHoldingBuf[iRegIndex] >> 8);
                *pucRegBuffer++ = (UCHAR) (pusRegHoldingBuf[iRegIndex] & 0xFF);
                iRegIndex++;
                usNRegs--;
            }
            break;

        /* write current register values with new values from the protocol stack. */
        case MB_REG_WRITE:
            while (usNRegs > 0)
            {
                pusRegHoldingBuf[iRegIndex] = *pucRegBuffer++ << 8;
                pusRegHoldingBuf[iRegIndex] |= *pucRegBuffer++;
                iRegIndex++;
                usNRegs--;
            }
            break;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}

/**
 * Modbus slave coils callback function.
 *
 * @param pucRegBuffer coils buffer
 * @param usAddress coils address
 * @param usNCoils coils number
 * @param eMode read or write
 *
 * @return result
 */
eMBErrorCode eMBRegCoilsCB(UCHAR * pucRegBuffer, USHORT usAddress,
        USHORT usNCoils, eMBRegisterMode eMode)
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          iRegIndex , iRegBitIndex , iNReg;
    UCHAR *         pucCoilBuf;

    iNReg =  usNCoils / 8 + 1;
    if(ucSCoilBuf == 0)
        return MB_EILLSTATE;
    pucCoilBuf = ucSCoilBuf;

    /* it already plus one in modbus function method. */
    usAddress--;

    if( ( usAddress >= usSCoilStart ) &&
        ( usAddress + usNCoils <= usSCoilStart + COIL_NCOILS ) )
    {
        iRegIndex = (USHORT) (usAddress - usSCoilStart) / 8;
        iRegBitIndex = (USHORT) (usAddress - usSCoilStart) % 8;
        switch ( eMode )
        {
        /* read current coil values from the protocol stack. */
        case MB_REG_READ:
            while (iNReg > 0)
            {
                *pucRegBuffer++ = xMBUtilGetBits(&pucCoilBuf[iRegIndex++],
                        iRegBitIndex, 8);
                iNReg--;
            }
            pucRegBuffer--;
            /* last coils */
            usNCoils = usNCoils % 8;
            /* filling zero to high bit */
            *pucRegBuffer = *pucRegBuffer << (8 - usNCoils);
            *pucRegBuffer = *pucRegBuffer >> (8 - usNCoils);
            break;

            /* write current coil values with new values from the protocol stack. */
        case MB_REG_WRITE:
            while (iNReg > 1)
            {
                xMBUtilSetBits(&pucCoilBuf[iRegIndex++], iRegBitIndex, 8,
                        *pucRegBuffer++);
                iNReg--;
            }
            /* last coils */
            usNCoils = usNCoils % 8;
            /* xMBUtilSetBits has bug when ucNBits is zero */
            if (usNCoils != 0)
            {
                xMBUtilSetBits(&pucCoilBuf[iRegIndex++], iRegBitIndex, usNCoils,
                        *pucRegBuffer++);
            }
            break;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}

/**
 * Modbus slave discrete callback function.
 *
 * @param pucRegBuffer discrete buffer
 * @param usAddress discrete address
 * @param usNDiscrete discrete number
 *
 * @return result
 */
eMBErrorCode eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          iRegIndex , iRegBitIndex , iNReg;
    UCHAR *         pucDiscreteInputBuf;
    iNReg =  usNDiscrete / 8 + 1;
    if(ucSDiscInBuf == 0)
        return MB_EILLSTATE;
    pucDiscreteInputBuf = ucSDiscInBuf;

    /* it already plus one in modbus function method. */
    usAddress--;

    if ((usAddress >= usSDiscInStart)
            && (usAddress + usNDiscrete    <= usSDiscInStart + DISCRETE_INPUT_NDISCRETES))
    {
        iRegIndex = (USHORT) (usAddress - usSDiscInStart) / 8;
        iRegBitIndex = (USHORT) (usAddress - usSDiscInStart) % 8;

        while (iNReg > 0)
        {
            *pucRegBuffer++ = xMBUtilGetBits(&pucDiscreteInputBuf[iRegIndex++],
                    iRegBitIndex, 8);
            iNReg--;
        }
        pucRegBuffer--;
        /* last discrete */
        usNDiscrete = usNDiscrete % 8;
        /* filling zero to high bit */
        *pucRegBuffer = *pucRegBuffer << (8 - usNDiscrete);
        *pucRegBuffer = *pucRegBuffer >> (8 - usNDiscrete);
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}

