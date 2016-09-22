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

#include <shell/shell.h>
#include "bleuart/bleuart.h"
#include "fifo/fifo.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+

// UART Serivce: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
// UART RXD    : 6E400002-B5A3-F393-E0A9-E50E24DCCA9E
// UART TXD    : 6E400003-B5A3-F393-E0A9-E50E24DCCA9E

const uint8_t BLEUART_UUID_SERVICE[] =
{
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E
};

const uint8_t BLEUART_UUID_CHR_RXD[] =
{
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E
};

const uint8_t BLEUART_UUID_CHR_TXD[] =
{
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E
};

#define UUID16_RXD  0x0002
#define UUID16_TXD  0x0003

//--------------------------------------------------------------------+
// VARIABLE DECLARATION
//--------------------------------------------------------------------+
int bleuart_char_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

static struct
{
  uint16_t conn_hdl;
  uint16_t txd_hdl;
}_bleuart;

static const struct ble_gatt_svc_def _service_bleuart[] =
{
  {
      .type            = BLE_GATT_SVC_TYPE_PRIMARY,
      .uuid128         = BLEUART_UUID_SERVICE,
      .characteristics = (struct ble_gatt_chr_def[])
      {
        { /*** Characteristic: TXD */
          .uuid128    = BLEUART_UUID_CHR_TXD,
          .val_handle = &_bleuart.txd_hdl,
          .access_cb  = bleuart_char_access,
          .flags      = BLE_GATT_CHR_F_NOTIFY,
        },
        { /*** Characteristic: RXD. */
          .uuid128    = BLEUART_UUID_CHR_RXD,
          .access_cb  = bleuart_char_access,
          .flags      = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
        },
        { 0 } /* No more characteristics. */
      }
  }

  , { 0 } /* No more services. */
};


FIFO_DEF(bleuart_ffin , CFG_BLE_UART_BUFSIZE, char, true );
//FIFO_DEF(bleuart_ffout, CFG_BLE_UART_BUFSIZE, char, true );
//uint8_t bleuart_xact_buf[64];

static int bleuart_tx_exec(int argc, char **argv);
static int bleuart_rx_exec(int argc, char **argv);

static struct shell_cmd cmd_bleuart[] =
{
    { .sc_cmd = "bleuarttx", .sc_cmd_func = bleuart_tx_exec },
    { .sc_cmd = "bleuartrx", .sc_cmd_func = bleuart_rx_exec },
};


int bleuart_init(struct ble_hs_cfg *cfg)
{
  varclr(_bleuart);

  ASSERT_STATUS( ble_gatts_count_cfg(_service_bleuart, cfg) );
  return ble_gatts_add_svcs(_service_bleuart);
}

void bleuart_set_conn_handle(uint16_t conn_handle)
{
  _bleuart.conn_hdl = conn_handle;
}

int bleuart_write(void const* buffer, uint32_t size)
{
  struct os_mbuf *om = ble_hs_mbuf_from_flat(buffer, size);
  return (0 == ble_gattc_notify_custom(_bleuart.conn_hdl, _bleuart.txd_hdl, om)) ? size : 0;
}

int bleuart_read(uint8_t* buffer, uint32_t size)
{
  return fifo_read_n(bleuart_ffin, buffer, size);
}

int bleuart_getc(void)
{
  char ch;
  return fifo_read(bleuart_ffin, &ch) ? ch : EOF;
}

int bleuart_shell_register(void)
{
  ASSERT_STATUS( shell_cmd_register(&cmd_bleuart[0]) );
  ASSERT_STATUS( shell_cmd_register(&cmd_bleuart[1]) );

  return 0;
}

//--------------------------------------------------------------------+
//
//--------------------------------------------------------------------+
int bleuart_char_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
  uint16_t uuid16 = uuid_extract_128_to_16(ctxt->chr->uuid128);
  struct os_mbuf *om = ctxt->om;

  switch (uuid16)
  {
    case UUID16_RXD:
      VERIFY(ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR, 0);
      fifo_write_n(bleuart_ffin, om->om_data, om->om_len);
    break;

    case UUID16_TXD:
//      assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR);
//      int len = fifo_read_n(bleuart_ffout, bleuart_xact_buf, sizeof(bleuart_xact_buf));
//
//      if (len > 0) ctxt->att.read.data = bleuart_xact_buf;
//      ctxt->att.read.len  = len;
    break;

    default: return -1;
  }

  return 0;
}


static int bleuart_tx_exec(int argc, char **argv)
{
  // skip command name
  for(int i=1; i<argc; i++)
  {
    // send space as well
    if (i > 1) bleuart_putc(' ');

    bleuart_puts(argv[i]);
  }

  return 0;
}

static int bleuart_rx_exec(int argc, char **argv)
{
  (void) argc;
  (void) argv;

  int ch;
  while( EOF != (ch = bleuart_getc()) )
  {
    putchar(ch);
  }

  return 0;
}


