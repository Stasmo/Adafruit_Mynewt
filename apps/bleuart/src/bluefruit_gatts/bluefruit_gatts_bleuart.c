/**************************************************************************/
/*!
    @file     bluefruit_gatts_bleuart.c
    @author   hathach

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

#include "bluefruit_gatts.h"
#include "fifo/fifo.h"
#include <shell/shell.h>

//--------------------------------------------------------------------+
//
//--------------------------------------------------------------------+
#define BLEUART_UUID16_RXD  0x0002
#define BLEUART_UUID16_TXD  0x0003

FIFO_DEF(bleuart_ffin , CFG_BLE_UART_BUFSIZE, char, true );
//FIFO_DEF(bleuart_ffout, CFG_BLE_UART_BUFSIZE, char, true );
//uint8_t bleuart_xact_buf[64];

int bf_gatts_bleuart_char_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int cmd_nus_exec(int argc, char **argv);
static struct shell_cmd cmd_nus = {
    .sc_cmd = "nus",
    .sc_cmd_func = cmd_nus_exec
};

//--------------------------------------------------------------------+
//
//--------------------------------------------------------------------+
static const struct ble_gatt_svc_def _service_bleuart[] =
{
  {
      .type            = BLE_GATT_SVC_TYPE_PRIMARY,
      .uuid128         = (uint8_t [])BLEUART_SERVICE_UUID,
      .characteristics = (struct ble_gatt_chr_def[])
      {
        { /*** Characteristic: TXD */
          .uuid128   = (uint8_t []) BLEUART_CHAR_TX_UUID,
          .access_cb = bf_gatts_bleuart_char_access,
          .flags     = BLE_GATT_CHR_F_NOTIFY,
        },
        { /*** Characteristic: RXD. */
          .uuid128   = (uint8_t []) BLEUART_CHAR_RX_UUID,
          .access_cb = bf_gatts_bleuart_char_access,
          .flags     = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
        },
        { 0 } /* No more characteristics in this service. */
      }
  }

  , { 0 } /* No more services. */
};

static struct
{
  uint16_t txd_attr_hdl;
}_bleuart;


//--------------------------------------------------------------------+
//
//--------------------------------------------------------------------+
int bf_gatts_bleuart_init(struct ble_hs_cfg *cfg)
{
  varclr(_bleuart);
  return ble_gatts_count_cfg(_service_bleuart, cfg);;
}

int bf_gatts_bleuart_register(void)
{
//  ASSERT_STATUS( ble_gatts_register_svcs(_service_bleuart, NULL, NULL) );

  ASSERT_STATUS( ble_gatts_add_svcs(_service_bleuart) );

  return 0;
}

int bf_gatts_bleurat_find_tx_hdl(void)
{
  // Find TXD attribute handle
  ASSERT_STATUS( ble_gatts_find_chr(_service_bleuart[0].uuid128, _service_bleuart[0].characteristics[0].uuid128, NULL, &_bleuart.txd_attr_hdl) );

  return 0;
}

extern uint16_t conn_handle;
int bf_gatts_bleuart_putc(char ch)
{
  struct os_mbuf *om = ble_hs_mbuf_from_flat(&ch, 1);
  return (0 == ble_gattc_notify_custom(conn_handle, _bleuart.txd_attr_hdl, om)) ? 1 : 0;
}

int bf_gatts_bleuart_puts(char* str)
{
  int n = strlen(str);
  struct os_mbuf *om = ble_hs_mbuf_from_flat(str, n);
  return (0 == ble_gattc_notify_custom(conn_handle, _bleuart.txd_attr_hdl, om)) ? n : 0;
}

int bf_gatts_bleuart_getc(void)
{
  char ch;
  return fifo_read(bleuart_ffin, &ch) ? ch : EOF;
}

int bf_gatts_bleuart_shell_register(void)
{
  return shell_cmd_register(&cmd_nus);
}

//--------------------------------------------------------------------+
//
//--------------------------------------------------------------------+
int bf_gatts_bleuart_char_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
  uint16_t uuid16 = uuid_extract_128_to_16(ctxt->chr->uuid128);
  struct os_mbuf *om = ctxt->om;

  switch (uuid16)
  {
    case BLEUART_UUID16_RXD:
      VERIFY(ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR, 0);
      fifo_write_n(bleuart_ffin, om->om_data, om->om_len);
    break;

    case BLEUART_UUID16_TXD:
//      assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR);
//      int len = fifo_read_n(bleuart_ffout, bleuart_xact_buf, sizeof(bleuart_xact_buf));
//
//      if (len > 0) ctxt->att.read.data = bleuart_xact_buf;
//      ctxt->att.read.len  = len;
    break;

    default:
      assert(0);
    break;
  }

  return 0;
}

static int cmd_nus_exec(int argc, char **argv)
{
  // skip command name "nus"
  for(int i=1; i<argc; i++)
  {
    // send space as well
    if (i > 1) bf_gatts_bleuart_putc(' ');

    bf_gatts_bleuart_puts(argv[i]);
  }

  return 0;
}

