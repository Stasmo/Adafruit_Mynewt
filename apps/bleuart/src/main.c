/**************************************************************************/
/*!
    @file     main.c

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2016, Adafruit Industries (adafruit.com)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**************************************************************************/

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "bsp/bsp.h"
#include "os/os.h"
#include "hal/hal_gpio.h"

#include "sysinit/sysinit.h"
#include <console/console.h>
#include <shell/shell.h>
#include <log/log.h>
#include <imgmgr/imgmgr.h>

/* BLE */
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"

/* Newtmgr include */
#include "newtmgr/newtmgr.h"
#include "nmgrble/newtmgr_ble.h"

/* RAM persistence layer. */
#include "store/ram/ble_store_ram.h"

/* Adafruit libraries and helpers */
#include "adafruit/adautil.h"
#include "adafruit/bledis.h"
#include "adafruit/bleuart.h"


/** Default device name */
#define CFG_GAP_DEVICE_NAME     "Adafruit Bluefruit"

/*------------------------------------------------------------------*/
/* Global values
 *------------------------------------------------------------------*/
static char serialnumber[16 + 1];

/*------------------------------------------------------------------*/
/* TASK Settings
 *------------------------------------------------------------------*/

/* BLE peripheral task settings */
#define BLE_TASK_PRIO                 1
#define BLE_STACK_SIZE                (OS_STACK_ALIGN(336))
struct  os_eventq  btle_evq;
struct  os_task    btle_task;
bssnz_t os_stack_t btle_stack[BLE_STACK_SIZE];

/* Blinky task settings */
#define BLINKY_TASK_PRIO              (10)
#define BLINKY_STACK_SIZE             OS_STACK_ALIGN(128)

struct os_task blinky_task;
os_stack_t blinky_stack[BLINKY_STACK_SIZE];

/* BLEUART to UART bridge task */
#define BLEUART_BRIDGE_TASK_PRIO      5
#define BLEUART_BRIDGE_STACK_SIZE     OS_STACK_ALIGN(256)

struct os_task bleuart_bridge_task;
os_stack_t bleuart_bridge_stack[BLEUART_BRIDGE_STACK_SIZE];

/*------------------------------------------------------------------*/
/* ADA Config
 *------------------------------------------------------------------*/
/* Group config Data to one struct */
struct
{
  char ble_devname[32];
}cfgdata =
{
    .ble_devname = CFG_GAP_DEVICE_NAME
};

/* Note when saving to flash group name will be added as prefix to each variable name
 * e.g
 * - group = "adafruit", variable is "ble/devname"
 * - "adafruit/ble/devname" will be save to flash
 */
const adacfg_info_t cfg_info[] =
{
    /* Name, Type, Size, Buffer */
    { "ble/devname", CONF_STRING, 32, cfgdata.ble_devname },

    { 0 } /* Zero for null-terminator */
};

/*------------------------------------------------------------------*/
/* Functions prototypes
 *------------------------------------------------------------------*/
static int btle_gap_event(struct ble_gap_event *event, void *arg);

/**
 * Enables advertising with the following parameters:
 *     o General discoverable mode.
 *     o Undirected connectable mode.
 */
static void btle_advertise(void)
{
  struct ble_hs_adv_fields fields =
  {
      /* Indicate that the flags field should be included; specify a value of 0
       * to instruct the stack to fill the value in for us. */
      .flags_is_present      = 1,
      .flags                 = 0,

      /* Indicate that the TX power level field should be included; have the
       * stack fill this one automatically as well.  This is done by assiging the
       * special value BLE_HS_ADV_TX_PWR_LVL_AUTO. */
      .tx_pwr_lvl_is_present = 1,
      .tx_pwr_lvl            = BLE_HS_ADV_TX_PWR_LVL_AUTO,

      .uuids128              = (void*) BLEUART_UUID_SERVICE,
      .num_uuids128          = 1,
      .uuids128_is_complete  = 0,
  };

  VERIFY_STATUS(ble_gap_adv_set_fields(&fields), RETURN_VOID);

  /*------------- Scan response data -------------*/
  const char *name = ble_svc_gap_device_name();
  struct ble_hs_adv_fields rsp_fields =
  {
      .name = (uint8_t*) name,
      .name_len = strlen(name),
      .name_is_complete = 1
  };
  ble_gap_adv_rsp_set_fields(&rsp_fields);

  /* Begin advertising. */
  struct ble_gap_adv_params adv_params;
  memset(&adv_params, 0, sizeof adv_params);
  adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
  VERIFY_STATUS(ble_gap_adv_start(BLE_ADDR_TYPE_PUBLIC, 0, NULL, BLE_HS_FOREVER, &adv_params, btle_gap_event, NULL),
                RETURN_VOID);
}

/**
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that forms.
 * bleprph uses the same callback for all connections.
 *
 * @param event                 The type of event being signalled.
 * @param ctxt                  Various information pertaining to the event.
 * @param arg                   Application-specified argument; unuesd by
 *                                  bleprph.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */
static int btle_gap_event(struct ble_gap_event *event, void *arg)
{
  switch ( event->type )
  {
    case BLE_GAP_EVENT_CONNECT:
      /* A new connection was established or a connection attempt failed. */
      if ( event->connect.status == 0 )
      {
        bleuart_set_conn_handle(event->connect.conn_handle);
      }
      else
      {
        /* Connection failed; resume advertising. */
        btle_advertise();
      }
      return 0;

    case BLE_GAP_EVENT_DISCONNECT:
      /* Connection terminated; resume advertising. */
      btle_advertise();
      return 0;

  }

  return 0;
}

/**
 * Event loop for the main btle task.
 */
static void btle_task_handler (void *unused)
{
  while (1)
  {
    os_eventq_run(&btle_evq);
  }
}

static void btle_on_sync(void)
{
  /* Begin advertising. */
  btle_advertise();
}

/**
 * Blinky task handler
 */
void blinky_task_handler(void* arg)
{
  hal_gpio_init_out(LED_BLINK_PIN, 1);

  while(1)
  {
    int32_t delay = OS_TICKS_PER_SEC * 1;
    os_time_delay(delay);

    hal_gpio_toggle(LED_BLINK_PIN);
  }
}

void bleuart_bridge_task_handler(void* arg)
{
  // register 'nus' command to send BLEUART
  bleuart_shell_register();

  while(1)
  {
    int ch;

    // Get data from bleuart to hwuart
    if ( (ch = bleuart_getc()) != EOF )
    {
      console_write( (char*)&ch, 1);
    }

    os_time_delay(1);
  }
}


int main(void)
{
  /* Set initial BLE device address. */
  memcpy(g_dev_addr, (uint8_t[6]){0xAD, 0xAF, 0xAD, 0xAF, 0xAD, 0xAF}, 6);

  /* Initialize OS */
  sysinit();

  /* Initialize eventq */
  os_eventq_init(&btle_evq);

  /* Init Config & NFFS */
//  adacfg_init("adafruit");
//  adacfg_add(cfg_info);

  //------------- Task Init -------------//
  os_task_init(&blinky_task, "blinky", blinky_task_handler, NULL,
               BLINKY_TASK_PRIO, OS_WAIT_FOREVER, blinky_stack, BLINKY_STACK_SIZE);

  os_task_init(&bleuart_bridge_task, "bleuart_bridge", bleuart_bridge_task_handler, NULL,
               BLEUART_BRIDGE_TASK_PRIO, OS_WAIT_FOREVER, bleuart_bridge_stack, BLEUART_BRIDGE_STACK_SIZE);

  os_task_init(&btle_task, "bleprph", btle_task_handler, NULL,
               BLE_TASK_PRIO, OS_WAIT_FOREVER, btle_stack, BLE_STACK_SIZE);

  /* Initialize the BLE host. */
  ble_hs_cfg.sync_cb        = btle_on_sync;
//  ble_hs_cfg.store_read_cb  = ble_store_ram_read;
//  ble_hs_cfg.store_write_cb = ble_store_ram_write;

  /* Convert MCU Unique Identifier to string as serial number */
  sprintf(serialnumber, "%08lX%08lX", NRF_FICR->DEVICEID[1], NRF_FICR->DEVICEID[0]);

  /* Device information service (DIS) settings */
  bledis_cfg_t dis_cfg =
  {
      .model        = "Feather52"  ,
      .serial       = serialnumber ,
      .firmware_rev = "0.9.0"      ,
      .hardware_rev = "nRF52832"   ,
      .software_rev = "0.9.0"      ,
      .manufacturer = "Adafruit Industries"
  };
  bledis_init(&dis_cfg);

  /* Nordic UART service (NUS) settings */
  bleuart_init();

  /* Set the default device name. */
  VERIFY_STATUS( ble_svc_gap_device_name_set(cfgdata.ble_devname) );

  /* Set the default eventq for packages that lack a dedicated task. */
  os_eventq_dflt_set(&btle_evq);

  /* Start the OS */
  os_start();

  /* OS start should never return. If it does, this should be an error */
  assert(0);

  return 0;
}
