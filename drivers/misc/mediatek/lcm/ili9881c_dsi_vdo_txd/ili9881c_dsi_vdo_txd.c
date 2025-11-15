/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2015. All rights reserved.
*
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
* RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
* AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
* NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
* SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
* SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
* THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
* THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
* CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
* SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
* CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
* AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
* OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
* MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*/
#define LOG_TAG "LCM"

#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif

#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
/*#include <mach/mt_pm_ldo.h>*/
#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_gpio.h>
#else
#include <mt-plat/mtk_gpio.h>
#include <mt-plat/mt6763/include/mach/gpio_const.h>
//#include "mach/gpio_const.h" 
#endif
#endif
#ifdef CONFIG_MTK_LEGACY
#include <cust_gpio_usage.h>
#endif
#ifndef CONFIG_FPGA_EARLY_PORTING
#if defined(CONFIG_MTK_LEGACY)
#include <cust_i2c.h>
#endif
#endif

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif


static const unsigned int BL_MIN_LEVEL = 20;
static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))
#define MDELAY(n)       (lcm_util.mdelay(n))
#define UDELAY(n)       (lcm_util.udelay(n))

#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
    lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
        lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
        lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
      lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
        lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
    
#define set_gpio_lcd_enp(cmd) \
    lcm_util.set_gpio_lcd_enp_bias(cmd)
#define set_gpio_lcd_enn(cmd) \
    lcm_util.set_gpio_lcd_enn_bias(cmd)

#ifndef BUILD_LK

extern int NT50358A_write_byte(unsigned char addr, unsigned char value);

#endif

/* static unsigned char lcd_id_pins_value = 0xFF; */
static const unsigned char LCD_MODULE_ID = 0x01;
#define LCM_DSI_CMD_MODE                                    0
#define FRAME_WIDTH                                     (720)
#define FRAME_HEIGHT                                    (1440)

#define LCM_PHYSICAL_WIDTH                  (64800)
#define LCM_PHYSICAL_HEIGHT                    (115200)
#define REGFLAG_DELAY       0xFFFC
#define REGFLAG_UDELAY  0xFFFB
#define REGFLAG_END_OF_TABLE    0xFFFD
#define REGFLAG_RESET_LOW   0xFFFE
#define REGFLAG_RESET_HIGH  0xFFFF

//static LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

struct LCM_setting_table {
    unsigned int cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
    {0x28, 0, {} },
    {REGFLAG_DELAY, 20, {} },
    {0x10, 0, {} },
    {REGFLAG_DELAY, 120, {} },
};

static struct LCM_setting_table init_setting[] = {
        {0xFF,3,{0x98,0x81,0x03}},     
        {0x01,1,{0x00}},               
        {0x02,1,{0x00}},               
        {0x03,1,{0x53}},               
        {0x04,1,{0x13}},               
        {0x05,1,{0x00}},               
        {0x06,1,{0x04}},               
        {0x07,1,{0x00}},               
        {0x08,1,{0x00}},               
        {0x09,1,{0x21}},               
        {0x0a,1,{0x21}},               
        {0x0b,1,{0x00}},               
        {0x0c,1,{0x01}},               
        {0x0d,1,{0x00}},               
        {0x0e,1,{0x00}},               
        {0x0f,1,{0x21}},               
        {0x10,1,{0x21}},               
        {0x11,1,{0x00}},               
        {0x12,1,{0x00}},               
        {0x13,1,{0x00}},               
        {0x14,1,{0x00}},               
        {0x15,1,{0x00}},               
        {0x16,1,{0x00}},               
        {0x17,1,{0x00}},               
        {0x18,1,{0x00}},               
        {0x19,1,{0x00}},               
        {0x1a,1,{0x00}},               
        {0x1b,1,{0x00}},               
        {0x1c,1,{0x00}},               
        {0x1d,1,{0x00}},               
        {0x1e,1,{0x44}},               
        {0x1f,1,{0x80}},               
        {0x20,1,{0x02}},               
        {0x21,1,{0x03}},               
        {0x22,1,{0x00}},               
        {0x23,1,{0x00}},               
        {0x24,1,{0x00}},               
        {0x25,1,{0x00}},               
        {0x26,1,{0x00}},               
        {0x27,1,{0x00}},               
        {0x28,1,{0x33}},               
        {0x29,1,{0x03}},               
        {0x2a,1,{0x00}},               
        {0x2b,1,{0x00}},               
        {0x2c,1,{0x00}},               
        {0x2d,1,{0x00}},               
        {0x2e,1,{0x00}},               
        {0x2f,1,{0x00}},               
        {0x30,1,{0x00}},               
        {0x31,1,{0x00}},               
        {0x32,1,{0x00}},               
        {0x33,1,{0x00}},               
        {0x34,1,{0x04}},               
        {0x35,1,{0x00}},               
        {0x36,1,{0x00}},               
        {0x37,1,{0x00}},               
        {0x38,1,{0x3C}},               
        {0x39,1,{0x00}},               
        {0x3a,1,{0x40}},               
        {0x3b,1,{0x40}},               
        {0x3c,1,{0x00}},               
        {0x3d,1,{0x00}},               
        {0x3e,1,{0x00}},               
        {0x3f,1,{0x00}},               
        {0x40,1,{0x00}},               
        {0x41,1,{0x00}},               
        {0x42,1,{0x00}},               
        {0x43,1,{0x00}},               
        {0x44,1,{0x00}},               
        {0x50,1,{0x01}},               
        {0x51,1,{0x23}},               
        {0x52,1,{0x45}},               
        {0x53,1,{0x67}},               
        {0x54,1,{0x89}},               
        {0x55,1,{0xAB}},               
        {0x56,1,{0x01}},               
        {0x57,1,{0x23}},               
        {0x58,1,{0x45}},               
        {0x59,1,{0x67}},               
        {0x5a,1,{0x89}},               
        {0x5b,1,{0xAB}},               
        {0x5c,1,{0xCD}},               
        {0x5d,1,{0xEF}},               
        {0x5e,1,{0x11}},               
        {0x5f,1,{0x01}},               
        {0x60,1,{0x00}},               
        {0x61,1,{0x15}},               
        {0x62,1,{0x14}},               
        {0x63,1,{0x0C}},               
        {0x64,1,{0x0D}},               
        {0x65,1,{0x0E}},               
        {0x66,1,{0x0F}},               
        {0x67,1,{0x06}},               
        {0x68,1,{0x02}},               
        {0x69,1,{0x02}},               
        {0x6a,1,{0x02}},               
        {0x6b,1,{0x02}},               
        {0x6c,1,{0x02}},               
        {0x6d,1,{0x02}},               
        {0x6e,1,{0x08}},               
        {0x6f,1,{0x02}},               
        {0x70,1,{0x02}},               
        {0x71,1,{0x02}},               
        {0x72,1,{0x02}},               
        {0x73,1,{0x02}},               
        {0x74,1,{0x02}},               
        {0x75,1,{0x01}},               
        {0x76,1,{0x00}},               
        {0x77,1,{0x15}},               
        {0x78,1,{0x14}},               
        {0x79,1,{0x0C}},               
        {0x7a,1,{0x0D}},               
        {0x7b,1,{0x0E}},               
        {0x7c,1,{0x0F}},               
        {0x7D,1,{0x08}},               
        {0x7E,1,{0x02}},               
        {0x7F,1,{0x02}},               
        {0x80,1,{0x02}},               
        {0x81,1,{0x02}},               
        {0x82,1,{0x02}},               
        {0x83,1,{0x02}},               
        {0x84,1,{0x06}},               
        {0x85,1,{0x02}},               
        {0x86,1,{0x02}},               
        {0x87,1,{0x02}},               
        {0x88,1,{0x02}},               
        {0x89,1,{0x02}},               
        {0x8A,1,{0x02}},               
        {0xFF,3,{0x98,0x81,0x04}},     
        {0x00,1,{0x00}},              
        {0x6C,1,{0x15}},              
        {0x6E,1,{0x2B}},              
        {0x6F,1,{0x35}},              
        {0x35,1,{0x1F}},              
        {0x33,1,{0x14}},              
        {0x3A,1,{0x24}},              
        {0x8D,1,{0x14}},              
        {0x87,1,{0xBA}},              
        {0x26,1,{0x76}},              
        {0xB2,1,{0xD1}},              
        {0xB5,1,{0x06}},
        {0x38,1,{0x01}},
        {0x39,1,{0x00}},
        {0x7A,1,{0x0F}},
       
        {0xFF,3,{0x98,0x81,0x01}},   
        {0x22,1,{0x0A}},              
        {0x31,1,{0x00}},              
        {0x53,1,{0xAB}},              
        {0x55,1,{0xB6}},               
        {0x50,1,{0xC8}},               
        {0x51,1,{0xC4}},               
        {0x60,1,{0x1D}},               
        {0x61,1,{0x00}},               
        {0x62,1,{0x19}},               
        {0x2e,1,{0xf0}},               
        {0xA0,1,{0x20}},               
        {0xA1,1,{0x49}},               
        {0xA2,1,{0x5B}},               
        {0xA3,1,{0x13}},               
        {0xA4,1,{0x15}},               
        {0xA5,1,{0x28}},               
        {0xA6,1,{0x1D}},               
        {0xA7,1,{0x1E}},               
        {0xA8,1,{0xCD}},               
        {0xA9,1,{0x1B}},               
        {0xAA,1,{0x27}},               
        {0xAB,1,{0xA8}},               
        {0xAC,1,{0x1B}},               
        {0xAD,1,{0x1A}},               
        {0xAE,1,{0x4E}},               
        {0xAF,1,{0x22}},               
        {0xB0,1,{0x29}},               
        {0xB1,1,{0x5C}},               
        {0xB2,1,{0x6A}},               
        {0xB3,1,{0x39}},               
        {0xC0,1,{0x20}},               
        {0xC1,1,{0x49}},               
        {0xC2,1,{0x5B}},               
        {0xC3,1,{0x14}},               
        {0xC4,1,{0x15}},               
        {0xC5,1,{0x28}},               
        {0xC6,1,{0x1C}},               
        {0xC7,1,{0x1E}},               
        {0xC8,1,{0xCC}},               
        {0xC9,1,{0x1C}},               
        {0xCA,1,{0x27}},               
        {0xCB,1,{0xA9}},               
        {0xCC,1,{0x1B}},               
        {0xCD,1,{0x1A}},               
        {0xCE,1,{0x4D}},               
        {0xCF,1,{0x22}},               
        {0xD0,1,{0x29}},               
        {0xD1,1,{0x5D}},               
        {0xD2,1,{0x69}},               
        {0xD3,1,{0x39}},               
        {0xFF,3,{0x98,0x81,0x02}},     
        {0x06,1,{0x40}},               
        {0x07,1,{0x05}},               
        {0xFF,3,{0x98,0x81,0x00}},         
        {0x35,1,{0x00}},               
        {0x36,1,{0x03}},               
        {0x51,2,{0x00,0x00}},          
        {0x53,1,{0x24}},               
        {0x55,1,{0x00}},             
        {0x11,0,{}},                   
    {REGFLAG_DELAY, 120, {} },
    {0x29,0,{}},
    {REGFLAG_DELAY, 20, {} },
    {REGFLAG_END_OF_TABLE, 0x00, {} }

};
static struct LCM_setting_table bl_level[] = {
    {0x51, 2, {0x07,0xFF} },
    {REGFLAG_END_OF_TABLE, 0x00, {} }
};

static void push_table(void *cmdq, struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;
    unsigned cmd;

    for (i = 0; i < count; i++) {
    cmd = table[i].cmd;
    switch (cmd) {
        case REGFLAG_DELAY:
        if (table[i].count <= 10)
            MDELAY(table[i].count);
        else
            MDELAY(table[i].count);
        break;

            case REGFLAG_UDELAY:
                UDELAY(table[i].count);
                break;

            case REGFLAG_END_OF_TABLE:
                break;

    default:
        dsi_set_cmdq_V22(cmdq, cmd, table[i].count, table[i].para_list, force_update);
    }
    }
}


static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    printk(KERN_ERR "%s: TXD_ILI9881C\n",__func__);
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));
    params->type = LCM_TYPE_DSI;
    params->width = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;
    params->physical_width = LCM_PHYSICAL_WIDTH/1000;
    params->physical_height = LCM_PHYSICAL_HEIGHT/1000;
    params->physical_width_um = LCM_PHYSICAL_WIDTH;
    params->physical_height_um = LCM_PHYSICAL_HEIGHT;

#if (LCM_DSI_CMD_MODE)
    params->dsi.mode = CMD_MODE;
    params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
    lcm_dsi_mode = CMD_MODE;
#else
    params->dsi.mode = SYNC_PULSE_VDO_MODE;
    params->dsi.switch_mode = CMD_MODE;
    lcm_dsi_mode = SYNC_PULSE_VDO_MODE;
#endif
    LCM_LOGI("lcm_get_params lcm_dsi_mode %d\n", lcm_dsi_mode);
    params->dsi.switch_mode_enable = 0;
    /* DSI */
    /* Command mode setting */
    params->dsi.LANE_NUM = LCM_THREE_LANE;
    /* The following defined the fomat for data coming from LCD engine. */
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

    /* Highly depends on LCD driver capability. */
    params->dsi.packet_size = 256;
    /* video mode timing */
    params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
    params->dsi.vertical_sync_active = 2; //old is 2,now is 4
    params->dsi.vertical_backporch = 18; //old is 8,now is 100
    params->dsi.vertical_frontporch = 8; //old is 15,now is 24
    params->dsi.vertical_active_line = FRAME_HEIGHT;
    params->dsi.horizontal_sync_active = 50;
    params->dsi.horizontal_backporch = 50;
    params->dsi.horizontal_frontporch = 50;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;
    params->dsi.ssc_disable = 0;
    params->dsi.ssc_range = 3;
    //params->dsi.HS_TRAIL = 15;
    params->dsi.PLL_CLOCK = 325;    /* this value must be in MTK suggested table */
    params->dsi.PLL_CK_CMD = 325;    
    params->dsi.PLL_CK_VDO = 325;     
    params->dsi.CLK_HS_POST = 36;

    params->dsi.clk_lp_per_line_enable = 0;
    params->dsi.esd_check_enable = 1;
    params->dsi.customization_esd_check_enable = 1;
    params->dsi.lcm_esd_check_table[0].cmd = 0x0a;
    params->dsi.lcm_esd_check_table[0].count = 1;
    params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
  
}

#ifdef BUILD_LK
static struct mt_i2c_t NT50358A_i2c;

static int NT50358A_write_byte(kal_uint8 addr, kal_uint8 value)
{
    kal_uint32 ret_code = I2C_OK;
    kal_uint8 write_data[2];
    kal_uint16 len;

    write_data[0] = addr;
    write_data[1] = value;

    NT50358A_i2c.id = I2C_I2C_LCD_BIAS_CHANNEL; /* I2C2; */
    /* Since i2c will left shift 1 bit, we need to set FAN5405 I2C address to >>1 */
    NT50358A_i2c.addr = LCD_BIAS_ADDR;
    NT50358A_i2c.mode = ST_MODE;
    NT50358A_i2c.speed = 100;
    len = 2;

    ret_code = i2c_write(&NT50358A_i2c, write_data, len);
    /* printf("%s: i2c_write: ret_code: %d\n", __func__, ret_code); */

    return ret_code;
}

#endif

//#define GPIO_LCD_RST_PIN         (GPIO83| 0x80000000)
//#define GPIO_LCD_BIAS_ENN_PIN         (GPIO112 | 0x80000000)
//#define GPIO_LCD_BIAS_ENP_PIN         (GPIO122 | 0x80000000)

static void lcm_reset(void)
{
    //printf("[uboot]:lcm reset start.\n");
#ifdef GPIO_LCD_RST_PIN
    mt_set_gpio_mode(GPIO_LCD_RST_PIN, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCD_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ONE);
    MDELAY(1);
    mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ZERO);
    MDELAY(10);
    mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ONE);
    MDELAY(120);
#else
    SET_RESET_PIN(1);
    MDELAY(1);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);

#endif
    printk("lcm reset end.\n");
}

static void lcm_init_power(void)
{
    /*#ifndef MACH_FPGA
    #ifdef BUILD_LK
        mt6325_upmu_set_rg_vgp1_en(1);
    #else
        LCM_LOGI("%s, begin\n", __func__);
        hwPowerOn(MT6325_POWER_LDO_VGP1, VOL_DEFAULT, "LCM_DRV");
        LCM_LOGI("%s, end\n", __func__);
    #endif
    #endif*/
}

static void lcm_suspend_power(void)
{
    /*#ifndef MACH_FPGA
    #ifdef BUILD_LK
        mt6325_upmu_set_rg_vgp1_en(0);
    #else
        LCM_LOGI("%s, begin\n", __func__);
        hwPowerDown(MT6325_POWER_LDO_VGP1, "LCM_DRV");
        LCM_LOGI("%s, end\n", __func__);
    #endif
    #endif*/
}

static void lcm_resume_power(void)
{
    /*#ifndef MACH_FPGA
    #ifdef BUILD_LK
        mt6325_upmu_set_rg_vgp1_en(1);
    #else
        LCM_LOGI("%s, begin\n", __func__);
        hwPowerOn(MT6325_POWER_LDO_VGP1, VOL_DEFAULT, "LCM_DRV");
        LCM_LOGI("%s, end\n", __func__);
    #endif
    #endif*/
}

static void lcm_init(void)
{
    unsigned char cmd = 0x0;
    unsigned char data = 0x0F;  //up to +/-5.5V
    int ret = 0;
    printk(KERN_ERR "%s: TXD_ILI9881C\n",__func__);

#if defined(GPIO_LCD_BIAS_ENN_PIN)||defined(GPIO_LCD_BIAS_ENP_PIN)
#ifdef GPIO_LCD_BIAS_ENP_PIN 
    mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
    //mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);
#endif    

#ifdef GPIO_LCD_BIAS_ENN_PIN 
    mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
    //mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ZERO);
#endif

#ifdef GPIO_LCD_BIAS_ENP_PIN 
    MDELAY(20);
    mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
#endif    

#ifdef GPIO_LCD_BIAS_ENN_PIN 
    MDELAY(5);
    mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);
#endif
#else
    set_gpio_lcd_enp(1);
    MDELAY(5);
    set_gpio_lcd_enn(1);
#endif
    ret = NT50358A_write_byte(cmd, data);
    if (ret < 0)
        LCM_LOGI("ili9881c----nt50358a----cmd=%0x--i2c write error----\n", cmd);
    else
        LCM_LOGI("ili9881c----nt50358a----cmd=%0x--i2c write success----\n", cmd);
    cmd = 0x01;
    data = 0x0F;
    ret = NT50358A_write_byte(cmd, data);
    if (ret < 0)
        LCM_LOGI("ili9881c----nt50358a----cmd=%0x--i2c write error----\n", cmd);
    else
        LCM_LOGI("ili9881c----nt50358a----cmd=%0x--i2c write success----\n", cmd);
    lcm_reset();

    push_table(NULL, init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
#ifndef MACH_FPGA

    push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
    MDELAY(10);
#if defined(GPIO_LCD_BIAS_ENN_PIN)||defined(GPIO_LCD_BIAS_ENP_PIN)

#ifdef GPIO_LCD_BIAS_ENN_PIN 
    mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ZERO);
#endif

#ifdef GPIO_LCD_BIAS_ENP_PIN 
    mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);
#endif    
#else
    set_gpio_lcd_enp(0);
    set_gpio_lcd_enn(0);
#endif

#endif
    printk("%s,txd_ili9881clcm_suspend\n", __func__);

    /* SET_RESET_PIN(0);
    MDELAY(10); */
}

static void lcm_resume(void)
{
    lcm_init();
}



static unsigned int lcm_ata_check(unsigned char *buffer)
{
#ifndef BUILD_LK
    unsigned int ret = 0;
    return ret;
#else
    return 0;
#endif
}

static void lcm_setbacklight(void *handle, unsigned int level)
{
   
    bl_level[0].para_list[0] = level>>4;
    bl_level[0].para_list[1] = ((level & 0x0F)<<4)|(level & 0x0F);
    printk("wzx%s,backlight: level = %d, set to 0x%x,0x%x \n", __func__, level,bl_level[0].para_list[0],bl_level[0].para_list[1]);
    push_table(handle, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}



LCM_DRIVER ili9881c_dsi_cmd_lcm_drv_txd = {
    .name = "txd_ili9881c",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params = lcm_get_params,
    .init = lcm_init,
    .suspend = lcm_suspend,
    .resume = lcm_resume,
//    .compare_id = lcm_compare_id,
    .init_power = lcm_init_power,
    .resume_power = lcm_resume_power,
    .suspend_power = lcm_suspend_power,
    .set_backlight_cmdq = lcm_setbacklight,
    .ata_check = lcm_ata_check,

};
