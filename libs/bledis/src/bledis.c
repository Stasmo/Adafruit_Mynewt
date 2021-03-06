/**************************************************************************/
/*!
    @file     bledis.c
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

#include "host/ble_hs.h"
#include "adafruit/bledis.h"
#include "adafruit/adautil.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+
enum { BLEDIS_MAX_CHAR = sizeof(bledis_cfg_t)/4 };


//--------------------------------------------------------------------+
// STATISTICS STRUCT DEFINITION
//--------------------------------------------------------------------+
#if MYNEWT_VAL(BLEDIS_STATS)

/* Define the core stats structure */
STATS_SECT_START(bledis_stat_section)
    STATS_SECT_ENTRY(model_reads)
    STATS_SECT_ENTRY(serial_reads)
    STATS_SECT_ENTRY(firmware_rev_reads)
    STATS_SECT_ENTRY(hardware_rev_reads)
    STATS_SECT_ENTRY(software_rev_reads)
    STATS_SECT_ENTRY(manufacturer_reads)
STATS_SECT_END

/* Define the stat names for querying */
STATS_NAME_START(bledis_stat_section)
    STATS_NAME(bledis_stat_section, model_reads)
    STATS_NAME(bledis_stat_section, serial_reads)
    STATS_NAME(bledis_stat_section, firmware_rev_reads)
    STATS_NAME(bledis_stat_section, hardware_rev_reads)
    STATS_NAME(bledis_stat_section, software_rev_reads)
    STATS_NAME(bledis_stat_section, manufacturer_reads)
STATS_NAME_END(bledis_stat_section)

STATS_SECT_DECL(bledis_stat_section) g_bledis_stats;

#endif

#if MYNEWT_VAL(BLEDIS_ADALOG)
  #define _LOG(x)  x
#else
  #define _LOG(x)
#endif


//--------------------------------------------------------------------+
// VARIABLE DECLARATION
//--------------------------------------------------------------------+
#if MYNEWT_VAL(BLEDIS_SERIAL_DYNAMIC)
static char _dis_serial[16 + 1];
#endif

static union
{
  bledis_cfg_t named;
  const char * arrptr[BLEDIS_MAX_CHAR];
} const _dis_cfg =
{
    .named =
    {
        .model        = MYNEWT_VAL(BLEDIS_MODEL_STR),

#if MYNEWT_VAL(BLEDIS_SERIAL_DYNAMIC)
        .serial       = _dis_serial,
#else
        .serial       = MYNEWT_VAL(BLEDIS_SERIAL_STR),
#endif

        .firmware_rev = MYNEWT_VAL(BLEDIS_FIRMWARE_REV_STR),
        .hardware_rev = MYNEWT_VAL(BLEDIS_HARDWARE_REV_STR),
        .software_rev = MYNEWT_VAL(BLEDIS_SOFTWARE_REV_STR),
        .manufacturer = MYNEWT_VAL(BLEDIS_MANUFACTURER_STR),
    }
};

const char * _dis_chr_text[] =
{
    "Model Number",
    "Serial Number",
    "Firmware Revision",
    "Hardware Revision",
    "Software Revision",
    "Manufacturer",
};

static struct ble_gatt_chr_def _dis_chars[BLEDIS_MAX_CHAR+1];

static const struct ble_gatt_svc_def _dis_service[] =
{
  {
      .type = BLE_GATT_SVC_TYPE_PRIMARY,
      .uuid = BLE_UUID16_DECLARE(UUID16_SVC_DEVICE_INFORMATION),
      .characteristics = _dis_chars
  }

  , { 0 } /* No more services. */
};


//--------------------------------------------------------------------+
// FUNCTION DECLARATION
//--------------------------------------------------------------------+
static int bledis_access_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

int bledis_init(void)
{
  _LOG( adalog_init() );

#if MYNEWT_VAL(BLEDIS_STATS)
  /* Initialise the stats section */
  stats_init(
      STATS_HDR(g_bledis_stats),
      STATS_SIZE_INIT_PARMS(g_bledis_stats, STATS_SIZE_32),
      STATS_NAME_INIT_PARMS(bledis_stat_section));

  /* Register the stats section */
  stats_register("ble_svc_dis", STATS_HDR(g_bledis_stats));
#endif

#if MYNEWT_VAL(BLEDIS_SERIAL_DYNAMIC)
  /* Convert MCU Unique Identifier to string as serial number */
  sprintf(_dis_serial, "%08lX%08lX", NRF_FICR->DEVICEID[1], NRF_FICR->DEVICEID[0]);
#endif

  // Include only configured characteristics
  int count = 0;
  for (int i=0; i<BLEDIS_MAX_CHAR; i++)
  {
    if ( _dis_cfg.arrptr[i] != NULL )
    {
      _LOG( ADALOG_INFO("[BLEDIS] %s added\n", _dis_chr_text[i]) );

      ble_uuid16_t* uuid16 = (ble_uuid16_t*) malloc( sizeof(ble_uuid16_t) );
      uuid16->u.type = BLE_UUID_TYPE_16;
      uuid16->value = UUID16_CHR_MODEL_NUMBER_STRING+i;

      _dis_chars[count].uuid      = &uuid16->u;
      _dis_chars[count].access_cb = bledis_access_cb;
      _dis_chars[count].flags     = BLE_GATT_CHR_F_READ;

      count++;
    }
  }

  if ( count )
  {
    // Register Service
    ble_gatts_count_cfg(_dis_service);
    ble_gatts_add_svcs (_dis_service);
  }

  return 0;
}


int bledis_access_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
  if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) return (-1);

  uint16_t uuid16 = ble_uuid_u16(ctxt->chr->uuid);
  if ( !is_within(UUID16_CHR_MODEL_NUMBER_STRING, uuid16, UUID16_CHR_MANUFACTURER_NAME_STRING) ) return (-1);

  const char* str = _dis_cfg.arrptr[uuid16 - UUID16_CHR_MODEL_NUMBER_STRING];

  os_mbuf_append(ctxt->om, str, strlen(str));

#if MYNEWT_VAL(BLEDIS_STATS)
  /* Increment the stats counter */
  switch (uuid16)
  {
    case 0x2A24:  /* Model Number */
      STATS_INC(g_bledis_stats, model_reads);
      break;
    case 0x2A25:  /* Serial Number */
      STATS_INC(g_bledis_stats, serial_reads);
      break;
    case 0x2A26:  /* Firmware Revision */
      STATS_INC(g_bledis_stats, firmware_rev_reads);
      break;
    case 0x2A27:  /* Hardware Revision */
      STATS_INC(g_bledis_stats, hardware_rev_reads);
      break;
    case 0x2A28:  /* Software Revision */
      STATS_INC(g_bledis_stats, software_rev_reads);
      break;
    case 0x2A29:  /* Manufacturer */
      STATS_INC(g_bledis_stats, manufacturer_reads);
      break;
  }
#endif

  return 0;
}
