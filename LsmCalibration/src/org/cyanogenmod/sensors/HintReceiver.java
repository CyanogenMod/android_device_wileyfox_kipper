/*
 * Copyright (C) 2016 The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.cyanogenmod.sensors;

import android.content.Context;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.util.Log;
import android.view.KeyEvent;
import android.widget.Toast;

import org.cyanogenmod.sensors.R;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

public class HintReceiver extends BroadcastReceiver {

    private static final String TAG = "LsmCalibrate";

    private static final String SECRET_CODE_PREFIX = "android_secret_code://";
    private static final String SENSOR_DEV_LSM_ROOT = "/sys/class/sensors";
    private static final String SENSOR_DEV_LSM_ACC = "lsm6ds0_accelerometer";
    private static final String SENSOR_DEV_LSM_GYR = "lsm6ds0_gyroscope";
    private static final String SENSOR_DEV_CALIBRATE = "calibrate";

    public HintReceiver() {
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        String code = intent.getData().getHost();
        int calibrationCompleteResource = -1;
        File sensorFile = null;

        if ("22235".equals(code)) {
            // 22235 spells ACCEL
            sensorFile = new File(SENSOR_DEV_LSM_ROOT + File.separator + SENSOR_DEV_LSM_ACC);
            calibrationCompleteResource = R.string.acc_calibration_complete;
        } else if ("4976".equals(code)) {
            // 4976 spells GYRO
            sensorFile = new File(SENSOR_DEV_LSM_ROOT + File.separator + SENSOR_DEV_LSM_GYR);
            calibrationCompleteResource = R.string.gyr_calibration_complete;
        } else {
            Log.w(TAG, "onReceive: Received unknown code: " + code);
        }

        if (sensorFile != null) {
            doCalibrate(sensorFile, 0);
            Toast.makeText(context, calibrationCompleteResource, Toast.LENGTH_LONG).show();
        }
    }

    private void doCalibrate(File sensorDir, int axis) {
        try {
            FileWriter calibrateWriter = new FileWriter(new File(sensorDir, SENSOR_DEV_CALIBRATE));
            calibrateWriter.write(String.format("%d", axis));
            calibrateWriter.close();
        } catch (IOException e) {
            Log.e(TAG, "Unable to calibrate " + sensorDir, e);
        }
    }
}
