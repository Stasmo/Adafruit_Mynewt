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

#include <stats/stats.h>

#include <shell/shell.h>
#include "adafruit/bleuart.h"
#include "adafruit/fifo.h"

/*------------------------------------------------------------------*/
/* MACRO CONSTANT TYPEDEF
 *------------------------------------------------------------------*/
/* UART Serivce: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
 * UART RXD    : 6E400002-B5A3-F393-E0A9-E50E24DCCA9E
 * UART TXD    : 6E400003-B5A3-F393-E0A9-E50E24DCCA9E
 */

const ble_uuid128_t BLEUART_UUID_SERVICE =
    BLE_UUID128_INIT ( 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
                       0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E );

const ble_uuid128_t BLEUART_UUID_CHR_RXD =
    BLE_UUID128_INIT( 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
                      0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E );

const ble_uuid128_t BLEUART_UUID_CHR_TXD =
    BLE_UUID128_INIT( 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
                      0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E );

#define UUID16_RXD  0x0002
#define UUID16_TXD  0x0003


/*------------------------------------------------------------------*/
/* STATISTICS STRUCT DEFINITION
 *------------------------------------------------------------------*/
#if MYNEWT_VAL(BLEUART_STATS)

/* Define the core stats structure */
STATS_SECT_START(bleuart_stat_section)
    STATS_SECT_ENTRY(txd_bytes)
    STATS_SECT_ENTRY(rxd_bytes)
STATS_SECT_END

/* Define the stat names for querying */
STATS_NAME_START(bleuart_stat_section)
    STATS_NAME(bleuart_stat_section, txd_bytes)
    STATS_NAME(bleuart_stat_section, rxd_bytes)
STATS_NAME_END(bleuart_stat_section)

STATS_SECT_DECL(bleuart_stat_section) g_bleuart_stats;

#endif

/*------------------------------------------------------------------*/
/* VARIABLE DECLARATION
 *------------------------------------------------------------------*/
FIFO_DEF(bleuart_ffin, MYNEWT_VAL(BLEUART_BUFSIZE), char, true, NULL);
//FIFO_DEF(bleuart_ffout, MYNEWT_VAL(BLEUART_BUFSIZE), char, true );
//uint8_t bleuart_xact_buf[64];

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
      .uuid            = &BLEUART_UUID_SERVICE.u,
      .characteristics = (struct ble_gatt_chr_def[])
      {
        { /*** Characteristic: TXD */
          .uuid       = &BLEUART_UUID_CHR_TXD.u,
          .val_handle = &_bleuart.txd_hdl,
          .access_cb  = bleuart_char_access,
          .flags      = BLE_GATT_CHR_F_NOTIFY,
        },
        { /*** Characteristic: RXD. */
          .uuid       = &BLEUART_UUID_CHR_RXD.u,
          .access_cb  = bleuart_char_access,
          .flags      = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
        },
        { 0 } /* No more characteristics. */
      }
  }

  , { 0 } /* No more services. */
};

/**
 *
 * @param cfg
 * @return
 */
int bleuart_init(void)
{
  varclr(_bleuart);

#if MYNEWT_VAL(BLEUART_STATS)
  /* Initialise the stats section */
  stats_init( STATS_HDR(g_bleuart_stats),
              STATS_SIZE_INIT_PARMS(g_bleuart_stats, STATS_SIZE_32),
              STATS_NAME_INIT_PARMS(bleuart_stat_section));

  /* Register the stats section */
  stats_register("ble_uart", STATS_HDR(g_bleuart_stats));
#endif

  VERIFY_STATUS( ble_gatts_count_cfg(_service_bleuart) );
  VERIFY_STATUS( ble_gatts_add_svcs(_service_bleuart) );

#if MYNEWT_VAL(BLEUART_CLI)
  int bleuart_shell_register(void);
  bleuart_shell_register();
#endif

  return 0;
}

/**
 *
 * @param conn_handle
 */
void bleuart_set_conn_handle(uint16_t conn_handle)
{
  _bleuart.conn_hdl = conn_handle;
}

/**
 *
 * @param buffer
 * @param size
 * @return
 */
int bleuart_write(void const* buffer, uint32_t size)
{
  struct os_mbuf *om = ble_hs_mbuf_from_flat(buffer, size);

#if MYNEWT_VAL(BLEUART_STATS)
  STATS_INCN(g_bleuart_stats, txd_bytes, size);
#endif

  return (0 == ble_gattc_notify_custom(_bleuart.conn_hdl, _bleuart.txd_hdl, om)) ? size : 0;
}

/**
 *
 * @param buffer
 * @param size
 * @return
 */
int bleuart_read(uint8_t* buffer, uint32_t size)
{
  return fifo_read_n(bleuart_ffin, buffer, size);
}

/**
 *
 * @return
 */
int bleuart_getc(void)
{
  char ch;
  return fifo_read(bleuart_ffin, &ch) ? ch : EOF;
}


/**
 *
 * @param conn_handle
 * @param attr_handle
 * @param ctxt
 * @param arg
 * @return
 */
int bleuart_char_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
  struct os_mbuf *om = ctxt->om;

  if( ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR ) return -1;

  fifo_write_n(bleuart_ffin, om->om_data, om->om_len);

#if MYNEWT_VAL(BLEUART_STATS)
  STATS_INCN(g_bleuart_stats, rxd_bytes, om->om_len);
#endif

  return 0;
}

/*------------------------------------------------------------------*/
/* Shell command integration
 * - bleuarttx to send string or bytearray in format AA-BB-CC-DD (hex)
 * - bleuartrx to receive string
 *------------------------------------------------------------------*/
#if MYNEWT_VAL(BLEUART_CLI)

static int bleuart_tx_exec(int argc, char **argv);
static int bleuart_rx_exec(int argc, char **argv);

static struct shell_cmd _bleuart_cmd[] =
{
    { .sc_cmd = "nustx", .sc_cmd_func = bleuart_tx_exec },
    { .sc_cmd = "nusrx", .sc_cmd_func = bleuart_rx_exec },
};

/**
 *
 * @return
 */
int bleuart_shell_register(void)
{
  VERIFY_STATUS( shell_cmd_register(&_bleuart_cmd[0]) );
  VERIFY_STATUS( shell_cmd_register(&_bleuart_cmd[1]) );

  return 0;
}

/**
 * Count the number of bytes in bytearray format AA-BB-CC.
 * Function will strictly check the syntax, any incorrect will
 * cause a zero return.
 * @param str input string in AA-BB-CC bytearray format
 * @return  number of bytes if syntax is correct otherwise 0.
 */
uint16_t get_bytearray_size(char const* str)
{
  uint16_t count = 0;

  while( *str)
  {
    if( !(isxdigit( (int) str[0] ) && isxdigit( (int) str[1])) ) return 0;
    count++;

    if (str[2] == 0  ) break;
    if (str[2] != '-') return 0;

    str += 3;
  }

  return count;
}

/******************************************************************************/
/*!
    @brief    Converts the supplied hexadecimal string to a uint8_t buffer.

    @note     Hex strings must be in the following format: "AA-BB-CC-DD"

    @param    str   The hexadecimal buffer containing the hex string to parser
    @param    p_buf A pointer to the buffer where the converted hex values
                    should be placed. p_buf could be NULL
    @param    size  The number of bytes to convert

    @return   The number of bytes converted

    @note     p_buf is NULL can be used to count the number of bytes converted
              or simply check to see if the format is correct.
*/
/******************************************************************************/
uint16_t parse_bytearray(char const* str, uint8_t* p_buf, uint16_t size)
{
  uint16_t count = 0;
  char *str_end;
  int32_t val = strtol(str, &str_end, 16);

  // Loop until there is a conversion error
  while ( (str < str_end) && (str_end - str <= 2) && (count < size) )
  {
    // Copy the current value to p_buf and increase count
    *p_buf++ = (uint8_t) val;

    count++;

    // Only continue if we find a '-' separator
    if (*str_end != '-') break;

    // Increase to next input
    str = str_end+1;
    val = strtol(str, &str_end, 16);
  }

  return count;
}

/**
 *
 * @param argc
 * @param argv
 * @return
 */
static int bleuart_tx_exec(int argc, char **argv)
{
  const char* str = argv[1];

  // Check if bytearray format
  uint16_t count = get_bytearray_size(str);

  if ( count )
  {
    uint8_t* buf = malloc(count);
    if (!buf) return (-1);

    count = parse_bytearray(str, buf, count);
    bleuart_write(buf, count);

    free(buf);
  }else
  {
    bleuart_puts(str);
  }

//  for(int i=1; i<argc; i++)
//  {
//    // send space as well
//    if (i > 1) bleuart_putc(' ');
//
//    bleuart_puts(argv[i]);
//  }



  return 0;
}

/**
 *
 * @param argc
 * @param argv
 * @return
 */
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

#endif

