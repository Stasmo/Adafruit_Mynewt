/**************************************************************************/
/*!
    @file     bluefruit_gap.c
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

#include "bluefruit_gap.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+

//--------------------------------------------------------------------+
// FUNCTION DECLARATION
//--------------------------------------------------------------------+


//--------------------------------------------------------------------+
// INTERNAL OBJECT
//--------------------------------------------------------------------+
static uint16_t bf_gap_appearance   = SWAP16(CFG_GAP_APPEARANCE);
static uint8_t  bf_gap_privacy_flag = CFG_GAP_PRPH_PRIVACY_FLAG;
static uint8_t bf_gap_reconnect_addr[6];

static uint8_t bf_pref_conn_params[8] = { 0 };

//--------------------------------------------------------------------+
// INTERNAL IMPLEMENTATION
//--------------------------------------------------------------------+
#if 0
const struct ble_gatt_svc_def bluefruit_gap_service =
{
    /*** Service: GAP. */
    .type = BLE_GATT_SVC_TYPE_PRIMARY,
    .uuid128 = BLE_UUID16(BLE_GAP_SVC_UUID16),
    .characteristics = (struct ble_gatt_chr_def[])
    {
      { /*** Characteristic: Device Name. */
          .uuid128 = BLE_UUID16(BLE_GAP_CHR_UUID16_DEVICE_NAME),
          .access_cb = bluefruit_gap_svr_chr_access,
          .flags = BLE_GATT_CHR_F_READ,
      }, {
          /*** Characteristic: Appearance. */
          .uuid128 = BLE_UUID16(BLE_GAP_CHR_UUID16_APPEARANCE),
          .access_cb = bluefruit_gap_svr_chr_access,
          .flags = BLE_GATT_CHR_F_READ,
      }, {
          /*** Characteristic: Peripheral Privacy Flag. */
          .uuid128 = BLE_UUID16(BLE_GAP_CHR_UUID16_PERIPH_PRIV_FLAG),
          .access_cb = bluefruit_gap_svr_chr_access,
          .flags = BLE_GATT_CHR_F_READ,
      }, {
          /*** Characteristic: Reconnection Address. */
          .uuid128 = BLE_UUID16(BLE_GAP_CHR_UUID16_RECONNECT_ADDR),
          .access_cb = bluefruit_gap_svr_chr_access,
          .flags = BLE_GATT_CHR_F_WRITE,
      }, {
          /*** Characteristic: Peripheral Preferred Connection Parameters. */
          .uuid128 = BLE_UUID16(BLE_GAP_CHR_UUID16_PERIPH_PREF_CONN_PARAMS),
          .access_cb = bluefruit_gap_svr_chr_access,
          .flags = BLE_GATT_CHR_F_READ,
      }, {
          0, /* No more characteristics in this service. */
      }
    }
};
#endif

int bluefruit_gap_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle, uint8_t op, union ble_gatt_access_ctxt *ctxt, void *arg)
{
  uint16_t uuid16;

  uuid16 = ble_uuid_128_to_16(ctxt->chr_access.chr->uuid128);
  assert(uuid16 != 0);

  switch (uuid16)
  {
    case BLE_GAP_CHR_UUID16_DEVICE_NAME:
      assert(op == BLE_GATT_ACCESS_OP_READ_CHR);
      ctxt->chr_access.data = CFG_GAP_DEVICE_NAME;
      ctxt->chr_access.len  = strlen(CFG_GAP_DEVICE_NAME);
    break;

    case BLE_GAP_CHR_UUID16_APPEARANCE:
      assert(op == BLE_GATT_ACCESS_OP_READ_CHR);
      ctxt->chr_access.data = &bf_gap_appearance;
      ctxt->chr_access.len  = sizeof(bf_gap_appearance);
    break;

    case BLE_GAP_CHR_UUID16_PERIPH_PRIV_FLAG:
      assert(op == BLE_GATT_ACCESS_OP_READ_CHR);
      ctxt->chr_access.data = &bf_gap_privacy_flag;
      ctxt->chr_access.len  = sizeof(bf_gap_privacy_flag);
    break;

    case BLE_GAP_CHR_UUID16_RECONNECT_ADDR:
      assert(op == BLE_GATT_ACCESS_OP_WRITE_CHR);
      if (ctxt->chr_access.len != sizeof(bf_gap_reconnect_addr) )
      {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
      }
      memcpy(bf_gap_reconnect_addr, ctxt->chr_access.data, sizeof(bf_gap_reconnect_addr));
    break;

    case BLE_GAP_CHR_UUID16_PERIPH_PREF_CONN_PARAMS:
      assert(op == BLE_GATT_ACCESS_OP_READ_CHR);
      ctxt->chr_access.data = &bf_pref_conn_params;
      ctxt->chr_access.len  = sizeof(bf_pref_conn_params);
    break;

    default:
      assert(0);
    break;
  }

  return 0;
}

//--------------------------------------------------------------------+
// PUBLIC API
//--------------------------------------------------------------------+
err_t bluefruit_gap_init(void)
{
//  return ble_gatts_register_svcs(&bluefruit_gap_service, NULL, NULL);

  return ERROR_NONE;
}
