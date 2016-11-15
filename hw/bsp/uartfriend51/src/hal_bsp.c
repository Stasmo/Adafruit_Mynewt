/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "syscfg/syscfg.h"
#include "hal/hal_bsp.h"
#include "mcu/nrf51_hal.h"
#include "bsp/bsp.h"
#include "nrf_adc.h"
#include "nrf_drv_adc.h"
#include "os/os_dev.h"
#include "os/os_cputime.h"
#include "flash_map/flash_map.h"
#include "hal/hal_flash.h"
#include "hal/hal_spi.h"
#include "hal/hal_i2c.h"
#if MYNEWT_VAL(SPI_0_MASTER)
#include "nrf_drv_spi.h"
#endif
#if MYNEWT_VAL(SPI_1_SLAVE)
#include "nrf_drv_spis.h"
#endif
#include "nrf_drv_config.h"
#include "app_util_platform.h"
#include "uart/uart.h"
#include "uart_hal/uart_hal.h"

#if MYNEWT_VAL(UART_0)
static struct uart_dev os_bsp_uart0;
static const struct nrf51_uart_cfg os_bsp_uart0_cfg = {
    .suc_pin_tx = MYNEWT_VAL(UART_0_PIN_TX),
    .suc_pin_rx = MYNEWT_VAL(UART_0_PIN_RX),
    .suc_pin_rts = MYNEWT_VAL(UART_0_PIN_RTS),
    .suc_pin_cts = MYNEWT_VAL(UART_0_PIN_CTS),
};
#endif

#if MYNEWT_VAL(SPI_0_MASTER)
/*
 * NOTE: do not set the ss pin here! This would cause the nordic SDK
 * to start using the SS pin when configured as a master and this is
 * not what our HAL expects. Our HAL expects that the SS pin, if used,
 * is treated as a gpio line and is handled outside the SPI routines.
 */
static const nrf_drv_spi_config_t os_bsp_spi0m_cfg = {
    .sck_pin      = 29,
    .mosi_pin     = 25,
    .miso_pin     = 28,
    .ss_pin       = NRF_DRV_SPI_PIN_NOT_USED,
    .irq_priority = (1 << __NVIC_PRIO_BITS) - 1,
    .orc          = 0xFF,
    .frequency    = NRF_DRV_SPI_FREQ_4M,
    .mode         = NRF_DRV_SPI_MODE_0,
    .bit_order    = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST
};
#endif

#if MYNEWT_VAL(SPI_1_SLAVE)
static const nrf_drv_spis_config_t os_bsp_spi1s_cfg = {
    .sck_pin      = 29,
    .mosi_pin     = 25,
    .miso_pin     = 28,
    .csn_pin      = 24,
    .miso_drive   = NRF_DRV_SPIS_DEFAULT_MISO_DRIVE,
    .csn_pullup   = NRF_GPIO_PIN_PULLUP,
    .orc          = NRF_DRV_SPIS_DEFAULT_ORC,
    .def          = NRF_DRV_SPIS_DEFAULT_DEF,
    .mode         = NRF_DRV_SPIS_MODE_0,
    .bit_order    = NRF_DRV_SPIS_BIT_ORDER_MSB_FIRST,
    .irq_priority = (1 << __NVIC_PRIO_BITS) - 1
};
#endif

/*
 * What memory to include in coredump.
 */
static const struct hal_bsp_mem_dump dump_cfg[] = {
    [0] = {
        .hbmd_start = &_ram_start,
        .hbmd_size = RAM_SIZE
    }
};

const struct hal_flash *
hal_bsp_flash_dev(uint8_t id)
{
    /*
     * Internal flash mapped to id 0.
     */
    if (id != 0) {
        return NULL;
    }
    return &nrf51_flash_dev;
}

const struct hal_bsp_mem_dump *
hal_bsp_core_dump(int *area_cnt)
{
    *area_cnt = sizeof(dump_cfg) / sizeof(dump_cfg[0]);
    return dump_cfg;
}

int
hal_bsp_power_state(int state)
{
    return (0);
}

/**
 * Returns the configured priority for the given interrupt. If no priority
 * configured, return the priority passed in
 *
 * @param irq_num
 * @param pri
 *
 * @return uint32_t
 */
uint32_t
hal_bsp_get_nvic_priority(int irq_num, uint32_t pri)
{
    uint32_t cfg_pri;

    switch (irq_num) {
    /* Radio gets highest priority */
    case RADIO_IRQn:
        cfg_pri = 0;
        break;
    default:
        cfg_pri = pri;
    }
    return cfg_pri;
}

void
hal_bsp_init(void)
{
    int rc;

#if MYNEWT_VAL(I2C_0)
    struct nrf51_hal_i2c_cfg hal_i2c_cfg;
#endif

    (void)rc;

#if MYNEWT_VAL(UART_0)
    rc = os_dev_create((struct os_dev *) &os_bsp_uart0, "uart0",
      OS_DEV_INIT_PRIMARY, 0, uart_hal_init, (void *)&os_bsp_uart0_cfg);
    assert(rc == 0);
#endif

#if MYNEWT_VAL(TIMER_0)
    rc = hal_timer_init(0, NULL);
    assert(rc == 0);
#endif
#if MYNEWT_VAL(TIMER_1)
    rc = hal_timer_init(1, NULL);
    assert(rc == 0);
#endif
#if MYNEWT_VAL(TIMER_2)
    rc = hal_timer_init(2, NULL);
    assert(rc == 0);
#endif

    /* Set cputime to count at 1 usec increments */
    rc = os_cputime_init(MYNEWT_VAL(CLOCK_FREQ));
    assert(rc == 0);

#if MYNEWT_VAL(SPI_0_MASTER)
    rc = hal_spi_init(0, (void *)&os_bsp_spi0m_cfg, HAL_SPI_TYPE_MASTER);
    assert(rc == 0);
#endif

#if MYNEWT_VAL(SPI_1_SLAVE)
    rc = hal_spi_init(1, (void *)&os_bsp_spi1s_cfg, HAL_SPI_TYPE_SLAVE);
    assert(rc == 0);
#endif

#if MYNEWT_VAL(I2C_0)
    hal_i2c_cfg.scl_pin = TWI0_CONFIG_SCL;
    hal_i2c_cfg.sda_pin = TWI0_CONFIG_SDA;
    hal_i2c_cfg.i2c_frequency = 100;        /* 100 kHz */
    rc = hal_i2c_init(0, &hal_i2c_cfg);
    assert(rc == 0);
#endif

}