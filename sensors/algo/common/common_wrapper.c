/*--------------------------------------------------------------------------
Copyright (c) 2014, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--------------------------------------------------------------------------*/

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <CalibrationModule.h>
#include <sensors.h>

#define LOG_TAG "sensor_cal.common"
#include <utils/Log.h>

#include "st480/st480_algo.h"

#define SENSOR_CAL_ALGO_VERSION 1
#define CSPEC_HNAVE_V   8

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

struct sensor_vec {
	union {
		struct {
			float data[4];
		};
		struct {
			float x;
			float y;
			float z;
		};
	};
};

struct sensor_cal_module_t SENSOR_CAL_MODULE_INFO;
static struct sensor_cal_algo_t algo_list[];

static float last_pocket = -1.0f;
static float last_light = -1.0f;
static float last_proximity = -1.0f;

static int convert_magnetic(sensors_event_t *raw, sensors_event_t *result,
		struct sensor_algo_args *args __attribute__((unused)))
{
	if (raw->type == SENSOR_TYPE_MAGNETIC_FIELD) {
		st480_t st480_raw;
		int level = -1;
		float values[3];

		st480_raw.mag_x = raw->magnetic.x;
		st480_raw.mag_y = raw->magnetic.y;
		st480_raw.mag_z = raw->magnetic.z;

		st480_run_library(st480_raw);
		get_calilevel_value(&level);
		get_magnetic_values(values);

		result->magnetic.x = values[0];
		result->magnetic.y = values[1];
		result->magnetic.z = values[2];
		result->magnetic.status = level;

		return 0;
	}

	return -EAGAIN;
}

static int convert_orientation(sensors_event_t *raw, sensors_event_t *result,
		struct sensor_algo_args *args __attribute__((unused)))
{
	float av;
	float pitch, roll, azimuth;
	const float rad2deg = 180 / M_PI;
	float values[3] = {0};
	float acc_m[3] = {0};
	float mag_m[3] = {0};

	static struct sensor_vec mag, acc;

	if (raw->type == SENSOR_TYPE_MAGNETIC_FIELD) {
		mag.x = raw->magnetic.x;
		mag.y = raw->magnetic.y;
		mag.z = raw->magnetic.z;
	}

	if (raw->type == SENSOR_TYPE_ACCELEROMETER) {
		acc.x = raw->acceleration.x;
		acc.y = raw->acceleration.y;
		acc.z = raw->acceleration.z;
	}

	av = sqrtf(acc.x*acc.x + acc.y*acc.y + acc.z*acc.z);
	if (av >= DBL_EPSILON) {
		acc_m[0] = acc.x;
		acc_m[1] = acc.y;
		acc_m[2] = acc.z;

		mag_m[0] = mag.x;
		mag_m[1] = mag.y;
		mag_m[2] = mag.z;

		get_oritation_values(mag_m, acc_m, values);
		result->orientation.azimuth = values[0];
		result->orientation.pitch = values[1];
		result->orientation.roll = values[2];

		result->orientation.status = 3;
	}

	if (raw->type != SENSOR_TYPE_MAGNETIC_FIELD)
		return -EAGAIN;

	return 0;

}

static int convert_rotation_vector(sensors_event_t *raw, sensors_event_t *result,
		struct sensor_algo_args *args __attribute__((unused)))
{
	float av;
	float pitch, roll, azimuth;
	int i;

	static struct sensor_vec mag, acc;

	if (raw->type == SENSOR_TYPE_MAGNETIC_FIELD) {
		mag.x = raw->magnetic.x;
		mag.y = raw->magnetic.y;
		mag.z = raw->magnetic.z;
	}

	if (raw->type == SENSOR_TYPE_ACCELEROMETER) {
		acc.x = raw->acceleration.x;
		acc.y = raw->acceleration.y;
		acc.z = raw->acceleration.z;
	}


	av = sqrtf(acc.x*acc.x + acc.y*acc.y + acc.z*acc.z);
	pitch = asinf(-acc.y / av);
	roll = asinf(acc.x / av);
	azimuth = atan2(-(mag.x) * cosf(roll) + mag.z * sinf(roll),
			mag.x*sinf(pitch)*sinf(roll) + mag.y*cosf(pitch) + mag.z*sinf(pitch)*cosf(roll));

	float halfAzi = azimuth / 2;
	float halfPitch = pitch / 2;
	float halfRoll = -roll / 2;

	float c1 = cosf(halfAzi);
	float s1 = sinf(halfAzi);
	float c2 = cosf(halfPitch);
	float s2 = sinf(halfPitch);
	float c3 = cosf(halfRoll);
	float s3 = sinf(halfRoll);

	result->data[0] = c1*c2*c3 - s1*s2*s3;
	result->data[1] = c1*s2*c3 - s1*c2*s3;
	result->data[2] = c1*c2*s3 + s1*s2*c3;
	result->data[3] = s1*c2*c3 + c1*s2*s3;

	if (halfAzi < M_PI / 2) {
		result->data[1] = -result->data[1];
		result->data[3] = -result->data[3];
	} else {
		result->data[2] = -result->data[2];
	}

	if (raw->type != SENSOR_TYPE_MAGNETIC_FIELD)
		return -1;

	return 0;
}

static int config_magnetic(int cmd, struct sensor_algo_args *args)
{
	struct compass_algo_args *param = (struct compass_algo_args*)args;

	switch (cmd) {
		case CMD_ENABLE:
			ALOGD("Enable status changed to %d\n", param->common.enable);
			break;
		case CMD_DELAY:
			ALOGD("Polling rate changed to %d\n", param->common.delay_ms);
			break;
		case CMD_BATCH:
			break;
	}

	return 0;
}

/* The magnetic field raw data is supposed to store at the sensors_event_t:data[4~6]*/
static int convert_uncalibrated_magnetic(sensors_event_t *raw, sensors_event_t *result,
		struct sensor_algo_args *args __attribute__((unused)))
{
	if (raw->type == SENSOR_TYPE_MAGNETIC_FIELD) {
		result->uncalibrated_magnetic.x_uncalib = raw->data[4];
		result->uncalibrated_magnetic.y_uncalib = raw->data[5];
		result->uncalibrated_magnetic.z_uncalib = raw->data[6];

		result->uncalibrated_magnetic.x_bias = raw->data[4] - raw->data[0];
		result->uncalibrated_magnetic.y_bias = raw->data[5] - raw->data[1];
		result->uncalibrated_magnetic.z_bias = raw->data[6] - raw->data[2];

		return 0;
	}

	return -1;
}

static int convert_pocket(sensors_event_t *raw, sensors_event_t *result,
		struct sensor_algo_args *args __attribute__((unused)))
{
	float inside;

	*result = *raw;
	if (raw->type == SENSOR_TYPE_PROXIMITY) {
		last_proximity = raw->data[0];
	} else if (raw->type == SENSOR_TYPE_LIGHT) {
		last_light = raw->data[0];
	} else {
		ALOGE("type error:%d\n", raw->type);
		return -1;
	}

	ALOGD("last_light:%f last_proximity:%f\n", last_light, last_proximity);

	if (last_proximity < 0.0f) {
		if (last_light < 0.0f) {
			ALOGE("sensor data error\n");
			return -1;
		} else if (last_light > 1000.0f) {
			inside = 0;
		} else {
			inside = 1;
		}
	} else if (last_proximity < 5.0f) {
		inside = 1;
	} else {
		inside = 0;
	}

	if (last_pocket != inside) {
		last_pocket = inside;
		result->data[0] = inside;
		return 0;
	}

	return -1;
}

static int config_pocket(int cmd, struct sensor_algo_args *args)
{
	struct compass_algo_args *param = (struct compass_algo_args*)args;

	switch (cmd) {
		case CMD_ENABLE:
			ALOGD("Enable status changed to %d\n", param->common.enable);
			if (param->common.enable) {
				last_pocket = -1.0f;
				last_light = -1.0f;
				last_proximity = -1.0f;
			}
			break;
	}

	return 0;
}

static int cal_init(const struct sensor_cal_module_t *module __attribute__((unused)),
		struct sensor_algo_args *args __attribute__((unused)))
{
	return 0;
}

static void cal_deinit()
{
	ALOGI("%s called\n", __func__);
}

static int cal_get_algo_list(const struct sensor_cal_algo_t **algo)
{
	*algo = algo_list;
	return 0;
}

static struct sensor_algo_methods_t compass_methods = {
	.convert = convert_magnetic,
	.config = config_magnetic,
};

static const char* compass_match_table[] = {
	COMPASS_NAME,
	"st480",
	NULL
};

static struct sensor_algo_methods_t orientation_methods = {
	.convert = convert_orientation,
	.config = NULL,
};

static const char* orientation_match_table[] = {
	ORIENTATION_NAME,
	NULL
};

static struct sensor_algo_methods_t rotation_vector_methods = {
	.convert = convert_rotation_vector,
	.config = NULL,
};

static const char* rotation_vector_match_table[] = {
	ROTATION_VECTOR_NAME,
	NULL
};

static struct sensor_algo_methods_t mag_uncalib_methods = {
	.convert = convert_uncalibrated_magnetic,
	.config = NULL,
};

static const char* mag_uncalib_match_table[] = {
	MAGNETIC_FIELD_UNCALIBRATED_NAME,
	NULL
};

static struct sensor_algo_methods_t pocket_methods = {
	.convert = convert_pocket,
	.config = config_pocket,
};

static const char* pocket_match_table[] = {
	"ltr553-pocket",
	"ap3426-pocket",
	"oem-pocket",
	NULL
};

static struct sensor_cal_algo_t algo_list[] = {
	{
		.tag = SENSOR_CAL_ALGO_TAG,
		.version = SENSOR_CAL_ALGO_VERSION,
		.type = SENSOR_TYPE_MAGNETIC_FIELD,
		.compatible = compass_match_table,
		.module = &SENSOR_CAL_MODULE_INFO,
		.methods = &compass_methods,
	},

	{
		.tag = SENSOR_CAL_ALGO_TAG,
		.version = SENSOR_CAL_ALGO_VERSION,
		.type = SENSOR_TYPE_ORIENTATION,
		.compatible = orientation_match_table,
		.module = &SENSOR_CAL_MODULE_INFO,
		.methods = &orientation_methods,
	},

	{
		.tag = SENSOR_CAL_ALGO_TAG,
		.version = SENSOR_CAL_ALGO_VERSION,
		.type = SENSOR_TYPE_ROTATION_VECTOR,
		.compatible = rotation_vector_match_table,
		.module = &SENSOR_CAL_MODULE_INFO,
		.methods = &rotation_vector_methods,
	},

	{
		.tag = SENSOR_CAL_ALGO_TAG,
		.version = SENSOR_CAL_ALGO_VERSION,
		.type = SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED,
		.compatible = mag_uncalib_match_table,
		.module = &SENSOR_CAL_MODULE_INFO,
		.methods = &mag_uncalib_methods,
	},

	{
		.tag = SENSOR_CAL_ALGO_TAG,
		.version = SENSOR_CAL_ALGO_VERSION,
		.type = SENSOR_TYPE_POCKET,
		.compatible = pocket_match_table,
		.module = &SENSOR_CAL_MODULE_INFO,
		.methods = &pocket_methods,
	},

};

static struct sensor_cal_methods_t cal_methods = {
	.init = cal_init,
	.deinit = cal_deinit,
	.get_algo_list = cal_get_algo_list,
};

struct sensor_cal_module_t SENSOR_CAL_MODULE_INFO = {
	.tag = SENSOR_CAL_MODULE_TAG,
	.id = "cal_module_common",
	.version = SENSOR_CAL_MODULE_VERSION,
	.vendor = "common",
	.dso = NULL,
	.number = ARRAY_SIZE(algo_list),
	.methods = &cal_methods,
	.reserved = {0},
};
