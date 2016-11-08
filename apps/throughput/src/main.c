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
#include "bsp/bsp.h"
#include "hal/hal_gpio.h"
//#include "hal/hal_cputime.h"

#include "sysinit/sysinit.h"
#include <console/console.h>
#include <shell/shell.h>
#include <log/log.h>
#include <imgmgr/imgmgr.h>

#include "adafruit/adautil.h"

/* BLE */
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "host/ble_hs_adv.h"
#include "host/ble_uuid.h"
#include "host/ble_att.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_l2cap.h"
#include "host/ble_sm.h"
#include "controller/ble_ll.h"

/* RAM HCI transport. */
#include "transport/ram/ble_hci_ram.h"

/* Newtmgr include */
#include "newtmgr/newtmgr.h"
#include "nmgrble/newtmgr_ble.h"

/* RAM persistence layer. */
#include "store/ram/ble_store_ram.h"

/* Mandatory services. */
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

/* Crash test ... just because */
//#include "crash_test/crash_test.h"

/* Adafruit libraries and helpers */
#include "adafruit/bledis.h"
#include "adafruit/bleuart.h"


/** Default device name */
#define CFG_GAP_DEVICE_NAME     "Adafruit Bluefruit"

/*------------------------------------------------------------------*/
/* TASK Settings
 *------------------------------------------------------------------*/

/* BLE peripheral task settings */
#define BLE_TASK_PRIO                 1
#define BLE_STACK_SIZE                (OS_STACK_ALIGN(336))
struct  os_eventq  btle_evq;
struct  os_task    btle_task;
bssnz_t os_stack_t btle_stack[BLE_STACK_SIZE];

/* Shell task settings */
#define SHELL_TASK_PRIO               (3)
#define SHELL_MAX_INPUT_LEN           (256)
#define SHELL_TASK_STACK_SIZE         (OS_STACK_ALIGN(384))
os_stack_t shell_stack[SHELL_TASK_STACK_SIZE];

/* newtmgr task settings */
#define NEWTMGR_TASK_PRIO             (4)
#define NEWTMGR_TASK_STACK_SIZE       (OS_STACK_ALIGN(512))
os_stack_t newtmgr_stack[NEWTMGR_TASK_STACK_SIZE];

/* Blinky task settings */
#define BLINKY_TASK_PRIO              (10)
#define BLINKY_STACK_SIZE             OS_STACK_ALIGN(128)

struct os_task blinky_task;
os_stack_t blinky_stack[BLINKY_STACK_SIZE];

/*------------------------------------------------------------------*/
/* Global values
 *------------------------------------------------------------------*/
static char serialnumber[16 + 1];

uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;

/*------------------------------------------------------------------*/
/* Functions prototypes
 *------------------------------------------------------------------*/
static int cmd_nustest_exec(int argc, char **argv);
static int btle_gap_event(struct ble_gap_event *event, void *arg);

/*------------------------------------------------------------------*/
/* Functions
 *------------------------------------------------------------------*/
/**
 *  Converts 'tick' to milliseconds
 */
static inline uint32_t tick2ms(os_time_t tick)
{
  return ((uint64_t) (tick*1000)) / OS_TICKS_PER_SEC;
}

static struct shell_cmd cmd_nustest =
{
    .sc_cmd      = "nustest",
    .sc_cmd_func = cmd_nustest_exec
};

/**
 *  'nustest' shell command handler
 */
static int cmd_nustest_exec(int argc, char **argv)
{
  /* 1st arg is number of packet (default 100)
   * 2nd arg is size of each packet (deault 20)
   */

  uint32_t count = (argc > 1) ? strtoul(argv[1], NULL, 10) : 100;
  uint32_t size  = (argc > 2) ? strtoul(argv[2], NULL, 10) : 20;

  if ( count > 100 )
  {
    printf("count must not exceed 100\n");
    return -1;
  }

  if ( size > 240 )
  {
    printf("size must not exceed 240\n");
    return -1;
  }

  uint32_t total = count * size;

  char *data = malloc(size);
  if(!data) return (-1);

  for(uint8_t i=0; i<size; i++)
  {
    data[i] = i%10 + '0';
  }

  /* Negotiate a larger MTU if size > 20 */
  if ( size > 20 )
  {
    ble_gattc_exchange_mtu(conn_handle, NULL, NULL);
    /* wait for the MTU procedure to complete. We could use a callback */
    /* but for now we simply delay 500 ms */
    os_time_delay(500);
  }

  os_time_t tick = os_time_get();

  for(uint8_t i=0; i<count; i++)
  {
    bleuart_write(data, size);
  }

  tick = os_time_get() -  tick;
  uint32_t ms = tick2ms(tick);

  free(data);

  /* Print the results */
  printf("Queued %lu bytes (%lu packets of %lu size) in %lu milliseconds\n", total, count, size, ms);

  return 0;
}

/**
 * Enables advertising with the following parameters:
 *     o General discoverable mode.
 *     o Undirected connectable mode.
 */
static void btle_advertise(void)
{
    /**
     *  Set the advertisement data included in our advertisements:
     *     o Flags (indicates advertisement type and other general info).
     *     o Advertising tx power.
     *     o Device name.
     *     o 16-bit service UUIDs (alert notifications).
     */
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof fields);

    /* Indicate that the flags field should be included; specify a value of 0
     * to instruct the stack to fill the value in for us.
     */
    fields.flags_is_present      = 1;
    fields.flags                 = 0;

    /* Indicate that the TX power level field should be included; have the
     * stack fill this one automatically as well.  This is done by assiging the
     * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
     */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl            = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    fields.uuids128              = (void*) BLEUART_UUID_SERVICE ;
    fields.num_uuids128          = 1;
    fields.uuids128_is_complete  = 0;

    VERIFY_STATUS( ble_gap_adv_set_fields(&fields), RETURN_VOID );

    //------------- Scan response data -------------//
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
static int
btle_gap_event(struct ble_gap_event *event, void *arg)
{
  switch ( event->type )
  {
    case BLE_GAP_EVENT_CONNECT:
      /* A new connection was established or a connection attempt failed. */
      if ( event->connect.status == 0 )
      {
        conn_handle = event->connect.conn_handle;
        bleuart_set_conn_handle(conn_handle);
      }
      else
      {
        /* Connection failed; resume advertising. */
        btle_advertise();

        conn_handle = BLE_HS_CONN_HANDLE_NONE;
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
 * Event loop for the main bleprph task.
 */
static void
btle_task_handler(void *unused)
{
  /* Command usage: nustest <count> <packetsize> */
  shell_cmd_register(&cmd_nustest);

  while (1)
  {
    os_eventq_get(&btle_evq);
  }

#if 0
    struct os_event *ev;
    struct os_callout_func *cf;
    int rc;

    /* Command usage: nustest <count> <packetsize> */
    shell_cmd_register(&cmd_nustest);

    rc = ble_hs_start();
    assert(rc == 0);

    /* Begin advertising. */
//    btle_advertise();

    while (1) {
        ev = os_eventq_get(&btle_evq);

        /* Check if the event is a nmgr ble mqueue event */
        rc = nmgr_ble_proc_mq_evt(ev);
        if (!rc) {
            continue;
        }

        switch (ev->ev_type) {
        case OS_EVENT_T_TIMER:
            cf = (struct os_callout_func *)ev;
            assert(cf->cf_func);
            cf->cf_func(CF_ARG(cf));
            break;
        default:
            assert(0);
            break;
        }
    }
#endif
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


/**
 * main
 *
 * The main function for the project. This function initializes the os, calls
 * init_tasks to initialize tasks (and possibly other objects), then starts the
 * OS. We should not return from os start.
 *
 * @return int NOTE: this function should never return!
 */
int main(void)
{
  /* Set initial BLE device address. */
  memcpy(g_dev_addr, (uint8_t[6]){0xAD, 0xAF, 0xAD, 0xAF, 0xAD, 0xAF}, 6);

  /* Initialize OS */
  sysinit();

  //------------- Task Init -------------//
//  shell_task_init(SHELL_TASK_PRIO, shell_stack, SHELL_TASK_STACK_SIZE, SHELL_MAX_INPUT_LEN);
//  console_init(shell_console_rx_cb);

//  nmgr_task_init(NEWTMGR_TASK_PRIO, newtmgr_stack, NEWTMGR_TASK_STACK_SIZE);
//  imgmgr_module_init();

  os_task_init(&blinky_task, "blinky", blinky_task_handler, NULL,
               BLINKY_TASK_PRIO, OS_WAIT_FOREVER, blinky_stack, BLINKY_STACK_SIZE);
  os_task_init(&btle_task, "bleprph", btle_task_handler, NULL,
               BLE_TASK_PRIO, OS_WAIT_FOREVER, btle_stack, BLE_STACK_SIZE);


  /* Initialize the BLE host. */
  ble_hs_cfg.sync_cb        = btle_on_sync;
  ble_hs_cfg.store_read_cb  = ble_store_ram_read;
  ble_hs_cfg.store_write_cb = ble_store_ram_write;

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

  /* Initialize eventq */
  os_eventq_init(&btle_evq);

  /* Set the default device name. */
  VERIFY_STATUS(ble_svc_gap_device_name_set(CFG_GAP_DEVICE_NAME));

  /* Set the default eventq for packages that lack a dedicated task. */
  os_eventq_dflt_set(&btle_evq);

  /* Start the OS */
  os_start();

  /* OS start should never return. If it does, this should be an error */
  assert(0);

  return 0;
}
