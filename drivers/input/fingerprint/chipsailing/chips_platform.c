/*************************************************************************
  版权： ShenZhen ChipSailing Technology Co., Ltd. All rights reserved.
  文件名称: chips_platform.c
  文件描述: 与硬件平台相关的函数接口
  作者: zwp    ID:58    版本:2.0   日期:2016/10/16
  其他:
  历史:
      1. 日期:           作者:          ID:
	     修改说明:
		 
	  2.
 *************************************************************************/

#include <linux/slab.h>
#include <linux/of.h>
#include <linux/err.h>
#include <linux/of_platform.h>
#include <linux/pinctrl/consumer.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/of_irq.h>
#include "./inc/chips_main.h"
#include <linux/errno.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#if defined(SPDM_PLAT)
#include <soc/sprd/sci.h>
#include <soc/sprd/sci_glb_regs.h>
#include <soc/sprd/adi.h>
#include <soc/sprd/adc.h>
#include <linux/regulator/consumer.h>
#include <soc/sprd/board.h>
#include <soc/sprd/sci_glb_regs.h>
#include <soc/sprd/regulator.h>
#include <linux/completion.h>
#include <linux/version.h>
#include <linux/miscdevice.h>  //
#include <linux/power_supply.h> //
#include <soc/sprd/gpio.h>   //"You must define GPIO_FP_INT in __board-XXXXX.h.
#endif

#if defined(CONFIG_MACH_SP7731CEA_FS286)||defined(CONFIG_MACH_SP7731CEB_FS286)
#include <soc/sprd/adi.h>
#include <soc/sprd/adc.h>
#include <soc/sprd/sci_glb_regs.h>

#define KPLED_CTL_A              (ANA_REGS_GLB_BASE + 0xf4)
#endif
int platform_power(int on)
{

    if(on)
    {
#if defined(SPDM_PLAT)

        static struct regulator *vdd3v3=NULL;
        //vdd3v3 = regulator_get(NULL, "vddsim2");
        vdd3v3 = regulator_get(NULL, "vddsdio");
        if (IS_ERR(vdd3v3))
        {
            printk("[chipsailing]rf2351 could not find the vdd3v3 regulator\n");
            vdd3v3 = NULL;
            return EIO;
        }
        regulator_set_voltage(vdd3v3, 3300000, 3300000);
        regulator_enable(vdd3v3);
#endif
    }
    printk("chipsailing fp dev_power sucess.\n");
    return 0;

}
 /**
 *  @brief chips_parse_dts 解析dts、获取硬件参数信息
 *  
 *  @param [in] chips_data chips_data结构体指针
 *  
 *  @return 成功返回0，失败返回非0
 */
int chips_parse_dts(struct chips_data* chips_data)
{
#if defined(MTK_PLAT)


	struct device_node *node = NULL;
	struct platform_device *pdev = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,cs_finger");
	if (IS_ERR(node)) {
		chips_dbg("device node is null\n");
		return PTR_ERR(node);
	}
	pdev = of_find_device_by_node(node);
	if (IS_ERR(pdev)) {
		chips_dbg("platform device is null\n");
		return PTR_ERR(pdev);
	}
	chips_dbg("pdev name=%s,\n",pdev->name);

	chips_data->pinctrl = devm_pinctrl_get(&pdev->dev);
	//chips_data->pinctrl = devm_pinctrl_get(&chips_data->spi->dev);
	if (IS_ERR(chips_data->pinctrl)) {
		chips_dbg("devm_pinctrl_get error\n");
		return  PTR_ERR(chips_data->pinctrl);
	}

	/* cs、clk、miso、mosi、int、rst */
	chips_data->rst_output1 = pinctrl_lookup_state(chips_data->pinctrl, "cs_finger_reset_en1");
	if (IS_ERR(chips_data->rst_output1)) {
		chips_dbg("Cannot find cs_finger_reset_en1\n");
		return PTR_ERR(chips_data->rst_output1);
	}
	chips_data->rst_output0 = pinctrl_lookup_state(chips_data->pinctrl, "cs_finger_reset_en0");
	if (IS_ERR(chips_data->rst_output0)) {
		chips_dbg("Cannot find cs_finger_reset_en0\n");
		return PTR_ERR(chips_data->rst_output0);
	}
	
	chips_data->cs_finger_spi0_mi_as_spi0_mi = pinctrl_lookup_state(chips_data->pinctrl, "cs_finger_spi0_mi_as_spi0_mi");
	if (IS_ERR(chips_data->cs_finger_spi0_mi_as_spi0_mi)) {
		chips_dbg("Cannot find cs_finger_spi0_mi_as_spi0_mi\n");
		return PTR_ERR(chips_data->cs_finger_spi0_mi_as_spi0_mi);
	}
	
	chips_data->cs_finger_spi0_mi_as_gpio = pinctrl_lookup_state(chips_data->pinctrl, "cs_finger_spi0_mi_as_gpio");
	if (IS_ERR(chips_data->cs_finger_spi0_mi_as_gpio)) {
		chips_dbg("Cannot find cs_finger_spi0_mi_as_gpio\n");
		return PTR_ERR(chips_data->cs_finger_spi0_mi_as_gpio);
	}
	
	chips_data->cs_finger_spi0_mo_as_spi0_mo = pinctrl_lookup_state(chips_data->pinctrl, "cs_finger_spi0_mo_as_spi0_mo");
	if (IS_ERR(chips_data->cs_finger_spi0_mo_as_spi0_mo)) {
		chips_dbg("Cannot find cs_finger_spi0_mo_as_spi0_mo\n");
		return PTR_ERR(chips_data->cs_finger_spi0_mo_as_spi0_mo);
	}
	
	chips_data->cs_finger_spi0_mo_as_gpio = pinctrl_lookup_state(chips_data->pinctrl, "cs_finger_spi0_mo_as_gpio");
	if (IS_ERR(chips_data->cs_finger_spi0_mo_as_gpio)) {
		chips_dbg("Cannot find cs_finger_spi0_mo_as_gpio\n");
		return PTR_ERR(chips_data->cs_finger_spi0_mo_as_gpio);
	}
	
	chips_data->cs_finger_spi0_clk_as_spi0_clk = pinctrl_lookup_state(chips_data->pinctrl, "cs_finger_spi0_clk_as_spi0_clk");
	if (IS_ERR(chips_data->cs_finger_spi0_clk_as_spi0_clk)) {
		chips_dbg("Cannot find cs_finger_spi0_clk_as_spi0_clk\n");
		return PTR_ERR(chips_data->cs_finger_spi0_clk_as_spi0_clk);
	}
	
	chips_data->cs_finger_spi0_clk_as_gpio = pinctrl_lookup_state(chips_data->pinctrl, "cs_finger_spi0_clk_as_gpio");
	if (IS_ERR(chips_data->cs_finger_spi0_clk_as_gpio)) {
		chips_dbg("Cannot find cs_finger_spi0_clk_as_gpio\n");
		return PTR_ERR(chips_data->cs_finger_spi0_clk_as_gpio);
	}
	
	chips_data->cs_finger_spi0_cs_as_spi0_cs = pinctrl_lookup_state(chips_data->pinctrl, "cs_finger_spi0_cs_as_spi0_cs");
	if (IS_ERR(chips_data->cs_finger_spi0_cs_as_spi0_cs)) {
		chips_dbg("Cannot find cs_finger_spi0_cs_as_spi0_cs\n");
		return PTR_ERR(chips_data->cs_finger_spi0_cs_as_spi0_cs);
	}
	
	chips_data->cs_finger_spi0_cs_as_gpio = pinctrl_lookup_state(chips_data->pinctrl, "cs_finger_spi0_cs_as_gpio");
	if (IS_ERR(chips_data->cs_finger_spi0_cs_as_gpio)) {
		chips_dbg("Cannot find cs_finger_spi0_cs_as_gpio\n");
		return PTR_ERR(chips_data->cs_finger_spi0_cs_as_gpio);
	}
	
	chips_data->eint_as_int = pinctrl_lookup_state(chips_data->pinctrl, "cs_finger_int_as_int");
	if (IS_ERR(chips_data->eint_as_int)) {
		chips_dbg("Cannot find s_finger_int_as_int\n");
		return PTR_ERR(chips_data->eint_as_int);
	}
#endif	
	
	chips_dbg("get pinctrl success\n");
	return 0;
}


 /**
 *  @brief chips_release_gpio 释放中断以及复位gpio
 *  
 *  @param [in] chips_data chips_data结构体指针
 *  
 *  @return 无返回值
 */
void chips_release_gpio(struct chips_data* chips_data)
{
	chips_dbg("entry\n");
	if (!IS_ERR(chips_data->pinctrl)) {
		chips_dbg("release pinctrl\n");
		devm_pinctrl_put(chips_data->pinctrl);
		}
	if (gpio_is_valid(chips_data->irq_gpio)){
		gpio_free(chips_data->irq_gpio);
	}
	
	if (gpio_is_valid(chips_data->reset_gpio)){
		gpio_free(chips_data->reset_gpio);
	}
}


 /**
 *  @brief chips_hw_reset IC复位
 *  
 *  @param [in] chips_data chips_data结构体指针
 *  @param [in] delay_ms 延时参数
 *  @return 成功返回0，失败返回负数
 */
int chips_hw_reset(struct chips_data *chips_data, unsigned int delay_ms)
{
/*
	chips_data->reset_gpio = 86;
	if(gpio_request(chips_data->reset_gpio,"reset_gpio")<0){
		gpio_free(chips_data->reset_gpio);
		gpio_request(chips_data->reset_gpio, "reset_gpio");
		gpio_direction_output(chips_data->reset_gpio,1);
	}
	gpio_set_value(chips_data->reset_gpio,1);	

	mdelay((delay_ms > 5)?delay_ms:5);
	gpio_set_value(chips_data->reset_gpio,0);

	mdelay((delay_ms > 5)?delay_ms:5);
	gpio_set_value(chips_data->reset_gpio,1);	
	mdelay(5);

	return 0;
*/

#if defined(MTK_PLAT)
	int ret = -1;
	
	ret = pinctrl_select_state(chips_data->pinctrl, chips_data->rst_output1);
	if(ret < 0)
		return ret;
	
	mdelay((delay_ms > 1)?delay_ms:1);
	
	ret = pinctrl_select_state(chips_data->pinctrl, chips_data->rst_output0);
	if(ret < 0)
		return ret;
	
	mdelay((delay_ms > 1)?delay_ms:1);
	
	ret = pinctrl_select_state(chips_data->pinctrl, chips_data->rst_output1);
	if(ret < 0)
		return ret;
	mdelay(1);
#elif defined(SPDM_PLAT)		
     chips_data->reset_gpio = GPIO_FP_RST;
	if(gpio_request(chips_data->reset_gpio,"reset_gpio")<0){
		gpio_free(chips_data->reset_gpio);
		gpio_request(chips_data->reset_gpio, "reset_gpio");
		gpio_direction_output(chips_data->reset_gpio,1);
	}
	gpio_direction_output(GPIO_FP_RST, 1);
	gpio_set_value(GPIO_FP_RST, 1);    //high
	mdelay((delay_ms > 1)? delay_ms:1);
	gpio_set_value(GPIO_FP_RST, 0);   //low
	mdelay((delay_ms > 1)? delay_ms:1);
	gpio_set_value(GPIO_FP_RST, 1);  //high
	mdelay(1);  	
#elif defined(QCOM_PLAT)

    gpio_set_value(chips_data->reset_gpio, 1);
	
	mdelay((delay_ms > 1)? delay_ms:1);
	
    gpio_set_value(chips_data->reset_gpio, 0);
	
	mdelay((delay_ms > 1)? delay_ms:1);
	
	gpio_set_value(chips_data->reset_gpio, 1);
	
	mdelay(1);  	
#endif
	
	return 0;
}
 /**
 *  @brief chips_hw_reset IC复位
 *  
 *  @param [in] chips_data chips_data结构体指针
 *  @param [in] delay_ms 延时参数
 *  @return 成功返回0，失败返回负数
 */
int chips_set_reset_gpio(struct chips_data *chips_data, unsigned int level)
{
	int ret = -1;	
	//seting output high
	if(level != 0){
#if defined(MTK_PLAT)
		ret = pinctrl_select_state(chips_data->pinctrl, chips_data->rst_output1);
		if(ret < 0){
			chips_dbg("pinctrl_select_state error,ret = %d\n",ret);
			return ret;
		}
#elif defined(SPDM_PLAT)		
	gpio_direction_output(GPIO_FP_RST, 1);
	gpio_set_value(GPIO_FP_RST, 1);  

#else  
    gpio_set_value(chips_data->reset_gpio, 1);

#endif
	//seting output low			
	}else{
#if defined(MTK_PLAT)

		ret = pinctrl_select_state(chips_data->pinctrl, chips_data->rst_output0);
		if(ret < 0){
			chips_dbg("pinctrl_select_state error,ret = %d\n",ret);
			return ret;
		 }
#elif defined(SPDM_PLAT)		
	gpio_direction_output(GPIO_FP_RST, 1);
	gpio_set_value(GPIO_FP_RST, 0);  
#else  
    gpio_set_value(chips_data->reset_gpio, 0);

#endif

		}
	
	return 0;
}



 /**
 *  @brief chips_get_irqno 获取中断号
 *  
 *  @param [in] chips_data chips_data结构体指针
 *  
 *  @return 成功返回中断号，失败返回负数
 */
int chips_get_irqno(struct chips_data *chips_data)
{


	u32 ints[2]={0};
	struct device_node *node;

	pinctrl_select_state(chips_data->pinctrl, chips_data->eint_as_int);

	node = of_find_compatible_node(NULL, NULL, "mediatek,cs_finger");
	if (NULL != node) {
		chips_data->irq = irq_of_parse_and_map(node, 0);
		chips_dbg("a irq=%d\n",chips_data->irq);
		of_property_read_u32_array(node,"debounce",ints,ARRAY_SIZE(ints));
		gpio_set_debounce(chips_data->irq,ints[0]);
	}else{
		chips_dbg("of_find_compatible_node error\n");
		return -ENODEV;
	}
	
	return chips_data->irq;

}


 /**
 *  @brief chips_set_spi_mode 设置spi模式
 *  
 *  @param [in] chips_data chips_data结构体指针
 *  
 *  @return 成功返回0，失败返回负数
 */
int chips_set_spi_mode(struct chips_data *chips_data)
{
	int ret = -1;
	
	ret = pinctrl_select_state(chips_data->pinctrl, chips_data->cs_finger_spi0_clk_as_spi0_clk);
	if(ret < 0)
		return ret;
	
	ret = pinctrl_select_state(chips_data->pinctrl, chips_data->cs_finger_spi0_cs_as_spi0_cs);
	if(ret < 0)
		return ret;
	
	ret = pinctrl_select_state(chips_data->pinctrl, chips_data->cs_finger_spi0_mi_as_spi0_mi);
	if(ret < 0)
		return ret;
	
	ret = pinctrl_select_state(chips_data->pinctrl, chips_data->cs_finger_spi0_mo_as_spi0_mo);
	if(ret < 0)
		return ret;
	
	return 0;
}
