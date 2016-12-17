/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * resarding copyright ownership.  The ASF licenses this file
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

#include <string.h>
#include <errno.h>
#include <assert.h>

#include "defs/error.h"
#include "os/os.h"
#include "sysinit/sysinit.h"
#include "hal/hal_i2c.h"
#include "sensor/sensor.h"
#include "sensor/accel.h"
#include "lsm303dlhc/lsm303dlhc.h"
#include "lsm303dlhc_priv.h"

#if MYNEWT_VAL(LSM303DLHC_LOG)
#include "log/log.h"
#endif

#if MYNEWT_VAL(LSM303DLHC_LOG)
#define LOG_MODULE_LSM303DLHC (303)
#define LSM303DLHC_INFO(...)  LOG_INFO(&_log, LOG_MODULE_LSM303DLHC, __VA_ARGS__)
#define LSM303DLHC_ERR(...)   LOG_ERROR(&_log, LOG_MODULE_LSM303DLHC, __VA_ARGS__)
static struct log _log;
#else
#define LSM303DLHC_INFO(...)
#define LSM303DLHC_ERR(...)
#endif

/* Exports for the sensor interface.
 */
static void *lsm303dlhc_sensor_get_interface(struct sensor *, sensor_type_t);
static int lsm303dlhc_sensor_read(struct sensor *, sensor_type_t,
        sensor_data_func_t, void *, uint32_t);
static int lsm303dlhc_sensor_get_config(struct sensor *, sensor_type_t,
        struct sensor_cfg *);

static const struct sensor_driver g_lsm303dlhc_sensor_driver = {
    lsm303dlhc_sensor_get_interface,
    lsm303dlhc_sensor_read,
    lsm303dlhc_sensor_get_config
};

/**
 * Writes a single byte to the specified register
 *
 * @param The I2C address to use
 * @param The register address to write to
 * @param The value to write
 *
 * @return 0 on success, non-zero error on failure.
 */
int
lsm303dlhc_write8(uint8_t addr, uint8_t reg, uint32_t value)
{
    int rc;
    uint8_t payload[2] = { reg, value & 0xFF };

    struct hal_i2c_master_data data_struct = {
        .address = addr,
        .len = 2,
        .buffer = payload
    };

    rc = hal_i2c_master_write(MYNEWT_VAL(LSM303DLHC_I2CBUS), &data_struct,
                              OS_TICKS_PER_SEC / 10, 1);
    if (rc) {
        LSM303DLHC_ERR("Failed to write @0x%02X with value 0x%02X\n",
                       reg, value);
    }

    return rc;
}

/**
 * Reads a single byte from the specified register
 *
 * @param The I2C address to use
 * @param The register address to read from
 * @param Pointer to where the register value should be written
 *
 * @return 0 on success, non-zero error on failure.
 */
int
lsm303dlhc_read8(uint8_t addr, uint8_t reg, uint8_t *value)
{
    int rc;
    uint8_t payload;

    struct hal_i2c_master_data data_struct = {
        .address = addr,
        .len = 1,
        .buffer = &payload
    };

    /* Register write */
    payload = reg;
    rc = hal_i2c_master_write(MYNEWT_VAL(LSM303DLHC_I2CBUS), &data_struct,
                              OS_TICKS_PER_SEC / 10, 1);
    if (rc) {
        LSM303DLHC_ERR("Failed to address sensor\n");
        goto error;
    }

    /* Read one byte back */
    payload = 0;
    rc = hal_i2c_master_read(MYNEWT_VAL(LSM303DLHC_I2CBUS), &data_struct,
                             OS_TICKS_PER_SEC / 10, 1);
    *value = payload;
    if (rc) {
        LSM303DLHC_ERR("Failed to read @0x%02X\n", reg);
    }

error:
    return rc;
}

/**
 * Expects to be called back through os_dev_create().
 *
 * @param The device object associated with this accellerometer
 * @param Argument passed to OS device init, unused
 *
 * @return 0 on success, non-zero error on failure.
 */
int
lsm303dlhc_init(struct os_dev *dev, void *arg)
{
    struct lsm303dlhc *lsm;
    struct sensor *sensor;
    int rc;
    uint8_t reg;

    lsm = (struct lsm303dlhc *) dev;

#if MYNEWT_VAL(LSM303DLHC_LOG)
    log_register("lsm303dlhc", &_log, &log_console_handler, NULL, LOG_SYSLEVEL);
#endif

    /* Enable the accelerometer (100Hz) */
    rc = lsm303dlhc_write8(LSM303DLHC_ADDR_ACCEL,
        LSM303DLHC_REGISTER_ACCEL_CTRL_REG1_A, 0x57);
    if (rc != 0) {
        goto err;
    }

    /* LSM303DLHC has no WHOAMI register so read CTRL_REG1_A back to check */
    /* if we are connected or not */
    rc = lsm303dlhc_read8(LSM303DLHC_ADDR_ACCEL,
        LSM303DLHC_REGISTER_ACCEL_CTRL_REG1_A, &reg);
    if ((rc != 0) || (reg != 0x57)) {
        LSM303DLHC_ERR("No LSM303DLHC detected on I2C bus %d at addr 0x%02X\n",
            MYNEWT_VAL(LSM303DLHC_I2CBUS), LSM303DLHC_ADDR_ACCEL);
        return false;
    }

    sensor = &lsm->sensor;

    rc = sensor_init(sensor, dev);
    if (rc != 0) {
        goto err;
    }

    rc = sensor_set_driver(sensor, SENSOR_TYPE_ACCELEROMETER,
            (struct sensor_driver *) &g_lsm303dlhc_sensor_driver);
    if (rc != 0) {
        goto err;
    }

    rc = sensor_mgr_register(sensor);
    if (rc != 0) {
        goto err;
    }

    return (0);
err:
    return (rc);
}

int
lsm303dlhc_config(struct lsm303dlhc *lsm, struct lsm303dlhc_cfg *cfg)
{
    /* Overwrite the configuration associated with this generic accelleromter. */
    memcpy(&lsm->cfg, cfg, sizeof(*cfg));

    return (0);
}

static void *
lsm303dlhc_sensor_get_interface(struct sensor *sensor, sensor_type_t type)
{
    return (NULL);
}

static int
lsm303dlhc_sensor_read(struct sensor *sensor, sensor_type_t type,
        sensor_data_func_t data_func, void *data_arg, uint32_t timeout)
{
    struct lsm303dlhc *lsm;
    struct sensor_accel_data sad;
    os_time_t now;
    uint32_t num_samples;
    int i;
    int rc;

    /* If the read isn't looking for accel data, then don't do anything. */
    if (!(type & SENSOR_TYPE_ACCELEROMETER)) {
        rc = SYS_EINVAL;
        goto err;
    }

    lsm = (struct lsm303dlhc *) SENSOR_GET_DEVICE(sensor);

    /* When a sensor is "read", we get the last 'n' samples from the device
     * and pass them to the sensor data function.  Based on the sample
     * interval provided to lsm303dlhc_config() and the last time this function
     * was called, 'n' samples are generated.
     */
    now = os_time_get();

    num_samples = (now - lsm->last_read_time) / lsm->cfg.sample_itvl;
    num_samples = min(num_samples, lsm->cfg.nr_samples);

    /* By default only readings are provided for 1-axis (x), however,
     * if number of axises is configured, up to 3-axises of data can be
     * returned.
     */
    sad.sad_x = 0;
    sad.sad_y = 0;
    sad.sad_z = 0;

    /* Call data function for each of the generated readings. */
    for (i = 0; i < num_samples; i++) {
        rc = data_func(sensor, data_arg, &sad);
        if (rc != 0) {
            goto err;
        }
    }

    return (0);
err:
    return (rc);
}

static int
lsm303dlhc_sensor_get_config(struct sensor *sensor, sensor_type_t type,
        struct sensor_cfg *cfg)
{
    int rc;

    if (type != SENSOR_TYPE_ACCELEROMETER) {
        rc = SYS_EINVAL;
        goto err;
    }

    cfg->sc_valtype = SENSOR_VALUE_TYPE_MS2_TRIPLET;

    return (0);
err:
    return (rc);
}