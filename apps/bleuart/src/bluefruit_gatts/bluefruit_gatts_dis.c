/**************************************************************************/
/*!
    @file     bluefruit_gatts_dis.c
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

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+

//--------------------------------------------------------------------+
// INTERNAL OBJECT & FUNCTION DECLARATION
//--------------------------------------------------------------------+

//--------------------------------------------------------------------+
// IMPLEMENTATION
//--------------------------------------------------------------------+
int bf_gatts_dis_char_access(uint16_t conn_handle, uint16_t attr_handle, uint8_t op, union ble_gatt_access_ctxt *ctxt, void *arg)
{
  assert(op == BLE_GATT_ACCESS_OP_READ_CHR);
  uint16_t uuid16 = ble_uuid_128_to_16(ctxt->chr_access.chr->uuid128);

  switch (uuid16)
  {
    case BLE_UUID16_MANUFACTURER_NAME_STRING_CHAR:
      ctxt->chr_access.data = CFG_BLE_DEVICE_INFO_MANUFACTURER;
      ctxt->chr_access.len  = strlen(CFG_BLE_DEVICE_INFO_MANUFACTURER);
    break;

    case BLE_UUID16_MODEL_NUMBER_STRING_CHAR:
      ctxt->chr_access.data = CFG_BLE_DEVICE_INFO_MODEL_NUMBER;
      ctxt->chr_access.len  = strlen(CFG_BLE_DEVICE_INFO_MODEL_NUMBER);
    break;

    case BLE_UUID16_SOFTWARE_REVISION_STRING_CHAR:
      ctxt->chr_access.data = CFG_FIRMWARE_VERSION_STRING;
      ctxt->chr_access.len  = strlen(CFG_FIRMWARE_VERSION_STRING);
    break;

    case BLE_UUID16_FIRMWARE_REVISION_STRING_CHAR:
      ctxt->chr_access.data = CFG_FIRMWARE_VERSION_STRING;
      ctxt->chr_access.len  = strlen(CFG_FIRMWARE_VERSION_STRING);
    break;

    case BLE_UUID16_HARDWARE_REVISION_STRING_CHAR:
      ctxt->chr_access.data = CFG_MCU_STRING;
      ctxt->chr_access.len  = strlen(CFG_MCU_STRING);
    break;

    default: break;
  }

  return 0;
}

err_t bf_gatts_dis_init(void)
{
  return ERROR_NONE;
}
