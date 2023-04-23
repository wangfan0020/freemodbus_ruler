/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-06-21     flybreak     first version
 */

#include <rtthread.h>

#include "mb.h"
#include "user_mb_app.h"

#ifdef PKG_MODBUS_SLAVE_SAMPLE
#define SLAVE_ADDR      MB_SAMPLE_SLAVE_ADDR
#define PORT_NUM        MB_SLAVE_USING_PORT_NUM
#define PORT_BAUDRATE   MB_SLAVE_USING_PORT_BAUDRATE
#else
#define SLAVE_ADDR      0x01
#define PORT_NUM        2
#define PORT_BAUDRATE   115200
#endif

#define PORT_PARITY     MB_PAR_NONE

#define MB_POLL_THREAD_PRIORITY  10
#define MB_SEND_THREAD_PRIORITY  RT_THREAD_PRIORITY_MAX - 1

#define MB_POLL_CYCLE_MS 20

UCHAR   CoilBuff[10];
UCHAR   DiscBuff[10];
USHORT  InRegsBuff[10];
USHORT  HoldRegsBuff[10];

void calback(UCHAR ucFunctionCode, eMBException eException)
{
    rt_kprintf("Code:%d  eException:%d \r\n",ucFunctionCode,eException);
}

static void send_thread_entry(void *parameter)
{

    rt_base_t level;

    while (1)
    {
        /* Test Modbus Master */
        level = rt_hw_interrupt_disable();

        InRegsBuff[0] = (USHORT)(rt_tick_get() / 100);
        rt_hw_interrupt_enable(level);

        rt_thread_mdelay(1000);
    }
}

static void mb_slave_poll(void *parameter)
{
    if (rt_strstr(parameter, "RTU"))
    {
        eMBFuncCofg cfg;
#ifdef PKG_MODBUS_SLAVE_RTU
        eMBInit(MB_RTU, SLAVE_ADDR, PORT_NUM, PORT_BAUDRATE, PORT_PARITY,calback);
        cfg.ucSCoilBuf = CoilBuff;
        cfg.CoilSize = 10;

        cfg.ucSDiscInBuf = DiscBuff;
        cfg.DiscInSize = 10;

        cfg.usSRegInBuf = InRegsBuff;
        cfg.RegInSize = 10;

        cfg.usSRegHoldBuf = HoldRegsBuff;
        cfg.RegHoldSize = 10;
        eMBUserInitCfg(cfg);
#else
        rt_kprintf("Error: Please open RTU mode first");
#endif
    }
    else if (rt_strstr(parameter, "ASCII"))
    {
#ifdef PKG_MODBUS_SLAVE_ASCII
        eMBInit(MB_ASCII, SLAVE_ADDR, PORT_NUM, PORT_BAUDRATE, PORT_PARITY);
#else
        rt_kprintf("Error: Please open ASCII mode first");
#endif
    }
    else if (rt_strstr(parameter, "TCP"))
    {
#ifdef PKG_MODBUS_SLAVE_TCP
        eMBTCPInit(0);
#else
        rt_kprintf("Error: Please open TCP mode first");
#endif
    }
    else
    {
        rt_kprintf("Error: unknown parameter");
    }
    eMBEnable();
    while (1)
    {
        eMBPoll();
        rt_thread_mdelay(MB_POLL_CYCLE_MS);
    }
}

static int mb_slave_sample(int argc, char **argv)
{
    static rt_uint8_t is_init = 0;
    rt_thread_t tid1 = RT_NULL, tid2 = RT_NULL;

    if (is_init > 0)
    {
        rt_kprintf("sample is running\n");
        return -RT_ERROR;
    }
    if (argc < 2)
    {
        rt_kprintf("Usage: mb_slave_sample RTU/ASCII/TCP\n");
        return -1;
    }

    tid1 = rt_thread_create("md_s_poll", mb_slave_poll, argv[1], 1024, MB_POLL_THREAD_PRIORITY, 10);
    if (tid1 != RT_NULL)
    {
        rt_thread_startup(tid1);
    }
    else
    {
        goto __exit;
    }

    tid2 = rt_thread_create("md_s_send", send_thread_entry, RT_NULL, 512, MB_SEND_THREAD_PRIORITY, 10);
    if (tid2 != RT_NULL)
    {
        rt_thread_startup(tid2);
    }
    else
    {
        goto __exit;
    }

    is_init = 1;
    return RT_EOK;

__exit:
    if (tid1)
        rt_thread_delete(tid1);
    if (tid2)
        rt_thread_delete(tid2);

    return -RT_ERROR;
}
MSH_CMD_EXPORT(mb_slave_sample, run a modbus slave sample);
