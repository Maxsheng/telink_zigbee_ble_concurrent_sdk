/********************************************************************************************************
 * @file    irq_handler.c
 *
 * @brief   This is the source file of irq_handler for tl321x
 *
 * @author  Zigbee Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/

#include "tl_common.h"


volatile u8 T_DBG_testIrq[16] = {0};

#include "zigbee_ble_switch.h"
extern u8 rf_busyFlag;
//#include "ble.h"
extern void blc_sdk_irq_handler(void);
extern void switch_to_ble_context(void);
extern void rf_rx_irq_handler(void);
extern void rf_tx_irq_handler(void);


_attribute_ram_code_ void rf_irq_handler(void)
{
	T_DBG_testIrq[0]++;
	if(CURRENT_SLOT_GET() == DUALMODE_SLOT_ZIGBEE){
		if(rf_get_irq_status(FLD_RF_IRQ_RX)){
			rf_clr_irq_status(FLD_RF_IRQ_RX);
			T_DBG_testIrq[1]++;
			rf_rx_irq_handler();
		}else if(rf_get_irq_status(FLD_RF_IRQ_TX)){
			rf_clr_irq_status(FLD_RF_IRQ_TX);
			T_DBG_testIrq[2]++;
			rf_tx_irq_handler();
		}else{
			T_DBG_testIrq[3]++;
			rf_clr_irq_status(0xffff);
		}
	}else{
		T_DBG_testIrq[12]++;
		blc_sdk_irq_handler ();
	}
}

void timer0_irq_handler(void)
{
	if(timer_get_irq_status(FLD_TMR0_MODE_IRQ)){
		timer_clr_irq_status(FLD_TMR0_MODE_IRQ);
		drv_timer_irq0_handler();
	}
}

void timer1_irq_handler(void)
{
	if(timer_get_irq_status(FLD_TMR1_MODE_IRQ)){
		timer_clr_irq_status(FLD_TMR1_MODE_IRQ);
		drv_timer_irq1_handler();
	}
}

_attribute_ram_code_ void stimer_irq_handler(void)
{
	if(CURRENT_SLOT_GET() == DUALMODE_SLOT_ZIGBEE){
		if(stimer_get_irq_status(FLD_SYSTEM_IRQ)){
			if(rf_busyFlag != BIT(1)){ 	//rf_busyFlag != TX_BUSY
				/* need switch to ble mode */
				switch_to_ble_context();
				blc_sdk_irq_handler();
				DBG_ZIGBEE_STATUS(0x23);
			}else{
				if(rf_get_irq_status(FLD_RF_IRQ_TX)){
					rf_clr_irq_status(FLD_RF_IRQ_TX);
					rf_tx_irq_handler();
				}
			}
		}
	}else{
		blc_sdk_irq_handler ();
		T_DBG_testIrq[10]++;
	}
}

void gpio0_irq_handler(void)
{
	T_DBG_testIrq[5]++;
	gpio_clr_irq_status(GPIO_IRQ_IRQ0);
	drv_gpio_irq_handler(GPIO_IRQ0);
}

void gpio1_irq_handler(void)
{
	T_DBG_testIrq[5]++;
	gpio_clr_irq_status(GPIO_IRQ_IRQ1);
	drv_gpio_irq_handler(GPIO_IRQ1);
}

void gpio2_irq_handler(void)
{
	T_DBG_testIrq[5]++;
	gpio_clr_irq_status(GPIO_IRQ_IRQ2);
	drv_gpio_irq_handler(GPIO_IRQ2);
}

void uart0_irq_handler(void)
{
	if(uart_get_irq_status(UART0, UART_TXDONE_IRQ_STATUS)){
		T_DBG_testIrq[8]++;
		drv_uart_tx_irq_handler();
	}

#if 0 //tl321x uses dma rx irq instead of uart irq
	if(uart_get_irq_status(UART0, UART_RXDONE_IRQ_STATUS)){
		T_DBG_testIrq[9]++;
		drv_uart_rx_irq_handler();
	}
#endif
}

void dma_irq_handler(void)
{
	//uart dma rx
	if(dma_get_tc_irq_status(BIT(UART_DMA_CHANNEL_RX))){
		T_DBG_testIrq[9]++;
		drv_uart_rx_irq_handler();
	}
}
