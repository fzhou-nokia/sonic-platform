// Driver for Nokia-7220-IXR-H6-128 Router
/*
 * Copyright (C) 2026 Accton Technology Corporation.
 * Copyright (C) 2026 Nokia Corporation. 
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * see <http://www.gnu.org/licenses/>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/leds.h>
#include <linux/slab.h>
#include <linux/platform_data/i2c-ocores.h>
#include <linux/delay.h>
#include <linux/dmi.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/mfd/core.h>
#include <linux/pci.h>

#define PORT_NUM  (128 + 3)  /*128 OSFPs + 1 QSFP28 + SFP + redriver*/

/*
 * PCIE BAR0 address
 */
#define BAR0_NUM                       0
#define BAR1_NUM                       1
#define BAR2_NUM                       2
#define REGION_LEN                     0xFF
#define FPGA_PCI_VENDOR_ID             0x10ee
#define FPGA_PCI_DEVICE_ID             0x7021

/* CPLD 1 */
#define CPLD1_PCIE_START_OFFSET        0x2000

/* CPLD 2 */
#define CPLD2_PCIE_START_OFFSET        0x3000

static uint param_i2c_khz = 400;
module_param(param_i2c_khz, uint, S_IRUGO);
MODULE_PARM_DESC(param_i2c_khz, "Target clock speed of i2c bus, in KHz.");

static struct pci_dev *g_pcidev = NULL;

static const uint adapt_offset[PORT_NUM]= {
    0x22100,// MESS_TOP_L_CPLD I2C Master OSFP Port1
    0x22120,// MESS_TOP_L_CPLD I2C Master OSFP Port2
    0x22140,// MESS_TOP_L_CPLD I2C Master OSFP Port3
    0x22160,// MESS_TOP_L_CPLD I2C Master OSFP Port4
    0x22180,// MESS_TOP_L_CPLD I2C Master OSFP Port5
    0x221A0,// MESS_TOP_L_CPLD I2C Master OSFP Port6
    0x221C0,// MESS_TOP_L_CPLD I2C Master OSFP Port7
    0x221E0,// MESS_TOP_L_CPLD I2C Master OSFP Port8
    0x22200,// MESS_TOP_L_CPLD I2C Master OSFP Port9
    0x22220,// MESS_TOP_L_CPLD I2C Master OSFP Port10
    0x22240,// MESS_TOP_L_CPLD I2C Master OSFP Port11
    0x22260,// MESS_TOP_L_CPLD I2C Master OSFP Port12
    0x22280,// MESS_TOP_L_CPLD I2C Master OSFP Port13
    0x222A0,// MESS_TOP_L_CPLD I2C Master OSFP Port14
    0x222C0,// MESS_TOP_L_CPLD I2C Master OSFP Port15
    0x222E0,// MESS_TOP_L_CPLD I2C Master OSFP Port16
    0x32100,// MESS_TOP_R_CPLD I2C Master OSFP Port17
    0x32120,// MESS_TOP_R_CPLD I2C Master OSFP Port18
    0x32140,// MESS_TOP_R_CPLD I2C Master OSFP Port19
    0x32160,// MESS_TOP_R_CPLD I2C Master OSFP Port20
    0x32180,// MESS_TOP_R_CPLD I2C Master OSFP Port21
    0x321A0,// MESS_TOP_R_CPLD I2C Master OSFP Port22
    0x321C0,// MESS_TOP_R_CPLD I2C Master OSFP Port23
    0x321E0,// MESS_TOP_R_CPLD I2C Master OSFP Port24
    0x32200,// MESS_TOP_R_CPLD I2C Master OSFP Port25
    0x32220,// MESS_TOP_R_CPLD I2C Master OSFP Port26
    0x32240,// MESS_TOP_R_CPLD I2C Master OSFP Port27
    0x32260,// MESS_TOP_R_CPLD I2C Master OSFP Port28
    0x32280,// MESS_TOP_R_CPLD I2C Master OSFP Port29
    0x322A0,// MESS_TOP_R_CPLD I2C Master OSFP Port30
    0x322C0,// MESS_TOP_R_CPLD I2C Master OSFP Port31
    0x322E0,// MESS_TOP_R_CPLD I2C Master OSFP Port32
//////////////////////////////////////////////////////     
    0x02100,// PORTCPLD0 I2C Master OSFP Port33
    0x02120,// PORTCPLD0 I2C Master OSFP Port34
    0x02180,// PORTCPLD0 I2C Master OSFP Port35
    0x021A0,// PORTCPLD0 I2C Master OSFP Port36
    0x02200,// PORTCPLD0 I2C Master OSFP Port37
    0x02220,// PORTCPLD0 I2C Master OSFP Port38
    0x02280,// PORTCPLD0 I2C Master OSFP Port39
    0x022A0,// PORTCPLD0 I2C Master OSFP Port40
    0x02300,// PORTCPLD0 I2C Master OSFP Port41
    0x02320,// PORTCPLD0 I2C Master OSFP Port42
    0x02380,// PORTCPLD0 I2C Master OSFP Port43
    0x023A0,// PORTCPLD0 I2C Master OSFP Port44
    0x02400,// PORTCPLD0 I2C Master OSFP Port45
    0x02420,// PORTCPLD0 I2C Master OSFP Port46
    0x02480,// PORTCPLD0 I2C Master OSFP Port47
    0x024A0,// PORTCPLD0 I2C Master OSFP Port48
    0x12100,// PORTCPLD1 I2C Master OSFP Port49
    0x12120,// PORTCPLD1 I2C Master OSFP Port50
    0x12180,// PORTCPLD1 I2C Master OSFP Port51
    0x121A0,// PORTCPLD1 I2C Master OSFP Port52
    0x12200,// PORTCPLD1 I2C Master OSFP Port53
    0x12220,// PORTCPLD1 I2C Master OSFP Port54
    0x12280,// PORTCPLD1 I2C Master OSFP Port55
    0x122A0,// PORTCPLD1 I2C Master OSFP Port56
    0x12300,// PORTCPLD1 I2C Master OSFP Port57
    0x12320,// PORTCPLD1 I2C Master OSFP Port58
    0x12380,// PORTCPLD1 I2C Master OSFP Port59
    0x123A0,// PORTCPLD1 I2C Master OSFP Port60
    0x12400,// PORTCPLD1 I2C Master OSFP Port61
    0x12420,// PORTCPLD1 I2C Master OSFP Port62
    0x12480,// PORTCPLD1 I2C Master OSFP Port63
    0x124A0,// PORTCPLD1 I2C Master OSFP Port64
//////////////////////////////////////////////////////     
    0x02140,// PORTCPLD0 I2C Master OSFP Port65
    0x02160,// PORTCPLD0 I2C Master OSFP Port66
    0x021C0,// PORTCPLD0 I2C Master OSFP Port67
    0x021E0,// PORTCPLD0 I2C Master OSFP Port68
    0x02240,// PORTCPLD0 I2C Master OSFP Port69
    0x02260,// PORTCPLD0 I2C Master OSFP Port70
    0x022C0,// PORTCPLD0 I2C Master OSFP Port71
    0x022E0,// PORTCPLD0 I2C Master OSFP Port72
    0x02340,// PORTCPLD0 I2C Master OSFP Port73
    0x02360,// PORTCPLD0 I2C Master OSFP Port74
    0x023C0,// PORTCPLD0 I2C Master OSFP Port75
    0x023E0,// PORTCPLD0 I2C Master OSFP Port76
    0x02440,// PORTCPLD0 I2C Master OSFP Port77
    0x02460,// PORTCPLD0 I2C Master OSFP Port78
    0x024C0,// PORTCPLD0 I2C Master OSFP Port79
    0x024E0,// PORTCPLD0 I2C Master OSFP Port80
    0x12140,// PORTCPLD1 I2C Master OSFP Port81
    0x12160,// PORTCPLD1 I2C Master OSFP Port82
    0x121C0,// PORTCPLD1 I2C Master OSFP Port83
    0x121E0,// PORTCPLD1 I2C Master OSFP Port84
    0x12240,// PORTCPLD1 I2C Master OSFP Port85
    0x12260,// PORTCPLD1 I2C Master OSFP Port86
    0x122C0,// PORTCPLD1 I2C Master OSFP Port87
    0x122E0,// PORTCPLD1 I2C Master OSFP Port88
    0x12340,// PORTCPLD1 I2C Master OSFP Port89
    0x12360,// PORTCPLD1 I2C Master OSFP Port90
    0x123C0,// PORTCPLD1 I2C Master OSFP Port91
    0x123E0,// PORTCPLD1 I2C Master OSFP Port92
    0x12440,// PORTCPLD1 I2C Master OSFP Port93
    0x12460,// PORTCPLD1 I2C Master OSFP Port94
    0x124C0,// PORTCPLD1 I2C Master OSFP Port95
    0x124E0,// PORTCPLD1 I2C Master OSFP Port96
//////////////////////////////////////////////////////    
    0x42100,// MESS_BOT_L_CPLD I2C Master OSFP Port97
    0x42120,// MESS_BOT_L_CPLD I2C Master OSFP Port98
    0x42140,// MESS_BOT_L_CPLD I2C Master OSFP Port99
    0x42160,// MESS_BOT_L_CPLD I2C Master OSFP Port100
    0x42180,// MESS_BOT_L_CPLD I2C Master OSFP Port101
    0x421A0,// MESS_BOT_L_CPLD I2C Master OSFP Port102
    0x421C0,// MESS_BOT_L_CPLD I2C Master OSFP Port103
    0x421E0,// MESS_BOT_L_CPLD I2C Master OSFP Port104
    0x42200,// MESS_BOT_L_CPLD I2C Master OSFP Port105
    0x42220,// MESS_BOT_L_CPLD I2C Master OSFP Port106
    0x42240,// MESS_BOT_L_CPLD I2C Master OSFP Port107
    0x42260,// MESS_BOT_L_CPLD I2C Master OSFP Port108
    0x42280,// MESS_BOT_L_CPLD I2C Master OSFP Port109
    0x422A0,// MESS_BOT_L_CPLD I2C Master OSFP Port110
    0x422C0,// MESS_BOT_L_CPLD I2C Master OSFP Port111
    0x422E0,// MESS_BOT_L_CPLD I2C Master OSFP Port112
    0x52100,// MESS_BOT_R_CPLD I2C Master OSFP Port113
    0x52120,// MESS_BOT_R_CPLD I2C Master OSFP Port114
    0x52140,// MESS_BOT_R_CPLD I2C Master OSFP Port115
    0x52160,// MESS_BOT_R_CPLD I2C Master OSFP Port116
    0x52180,// MESS_BOT_R_CPLD I2C Master OSFP Port117
    0x521A0,// MESS_BOT_R_CPLD I2C Master OSFP Port118
    0x521C0,// MESS_BOT_R_CPLD I2C Master OSFP Port119
    0x521E0,// MESS_BOT_R_CPLD I2C Master OSFP Port120
    0x52200,// MESS_BOT_R_CPLD I2C Master OSFP Port121
    0x52220,// MESS_BOT_R_CPLD I2C Master OSFP Port122
    0x52240,// MESS_BOT_R_CPLD I2C Master OSFP Port123
    0x52260,// MESS_BOT_R_CPLD I2C Master OSFP Port124
    0x52280,// MESS_BOT_R_CPLD I2C Master OSFP Port125
    0x522A0,// MESS_BOT_R_CPLD I2C Master OSFP Port126
    0x522C0,// MESS_BOT_R_CPLD I2C Master OSFP Port127
    0x522E0,// MESS_BOT_R_CPLD I2C Master OSFP Port128
//////////////////////////////////////////////////////    
    0x12500,// QSFP28    port 129
    0x2500,//  SFP+      port 130
    0x2520,//  redriver
};

static struct ocores_i2c_platform_data i2c_data = {
    .reg_shift = 2,
    .clock_khz = 25000,
    .bus_khz = 400,
    .devices = NULL,  //trcv_nvm,
    .num_devices = 1  //ARRAY_SIZE(trcv_nvm)
};

static void ftdi_release_platform_dev(struct device *dev)
{
    dev->parent = NULL;
}

static struct platform_device myi2c[PORT_NUM] = {{0}};
static int __init h6_ocore_i2c_init(void)
{
    int i, err = 0;
    static const char *devname = "ocores-i2c";
    static struct resource ocores_resources[PORT_NUM] = {0};
    struct pci_dev *pcidev;
    int status = 0;
    unsigned long bar_base;
    struct resource *res;
    struct platform_device *p = NULL;

    pcidev = pci_get_device(FPGA_PCI_VENDOR_ID, FPGA_PCI_DEVICE_ID, NULL);
     if (!pcidev) {
        pr_err("Cannot found PCI device(%x:%x)\n",
                     FPGA_PCI_VENDOR_ID, FPGA_PCI_DEVICE_ID);
        return -ENODEV;
    }

    g_pcidev = pcidev;

    err = pci_enable_device(pcidev);
    if (err != 0) {
        pr_err("Cannot enable PCI device(%x:%x)\n",
                     FPGA_PCI_VENDOR_ID, FPGA_PCI_DEVICE_ID);
        status = -ENODEV;
        goto exit_pci_put;
    }
    /* enable PCI bus-mastering */
    pci_set_master(pcidev);

    status = pci_enable_msi(pcidev);
    if (status < 0) {
        pr_err("Failed to allocate IRQ vectors: %d\n", status);
        goto exit_pci_disable;
    }

    i2c_data.bus_khz = clamp_val(param_i2c_khz, 50, 400);
    for(i = 0; i < PORT_NUM; i++) {
        p = &myi2c[i];
        p->name                   = devname;
        p->id                     = i;
        p->dev.platform_data      = &i2c_data;
        p->dev.release = ftdi_release_platform_dev;
        res = &ocores_resources[i];
        switch (i)
        {
            case 0 ... PORT_NUM:
                bar_base = pci_resource_start(pcidev, BAR0_NUM);
                break;
            default:
                break;
        }
        res->start = bar_base + adapt_offset[i];
        res->end = res->start + 0x20 - 1;
        res->name = NULL;
        res->flags =IORESOURCE_MEM;
        res->desc = IORES_DESC_NONE;
        p->num_resources          = 1;
        p->resource               = res;
        err = platform_device_register(p);
        if (err)
            goto unload;
    }

    return 0;

unload:
    {
        int j;
        pr_err("[ERROR]rc:%d, unload %u register devices\n", err, i);
        for(j = 0; j < i; j++) {
            platform_device_unregister(&myi2c[j]);
        }
    }
    pci_disable_msi(pcidev);

exit_pci_disable:
    pci_disable_device(pcidev);

exit_pci_put:
    pci_dev_put(pcidev);
    g_pcidev = NULL;

    return status ? status : err;
}

static void __exit h6_ocore_i2c_exit(void)
{
    int i;
    for(i = PORT_NUM ; i > 0; i--) {
        platform_device_unregister(&myi2c[i-1]);
    }
    if (g_pcidev) {
        pci_disable_msi(g_pcidev);
        pci_disable_device(g_pcidev);
        pci_dev_put(g_pcidev);
        g_pcidev = NULL;
    }
}

module_init(h6_ocore_i2c_init);
module_exit(h6_ocore_i2c_exit);

MODULE_AUTHOR("Roy Lee <roy_lee@accton.com.tw>");
MODULE_DESCRIPTION("h6 ocore_i2c platform device driver");
MODULE_LICENSE("GPL");
