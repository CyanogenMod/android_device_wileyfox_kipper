/*******************************************************************************
*  Copyright (C) 2014 Senodia
*
*  Copyright Statement:
*  ------------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Senodia Inc.
*  ------------------------
*
*  FileName: RunAlgorithm.h
*  Author : Tori Xu <xuezhi_xu@senodia.com,tori.xz.xu@gmail.com>
*  Reversion : V1.0.0
*******************************************************************************/

#ifndef RUN_ALGORITHM_H
#define RUN_ALGORITHM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct st480_vec_t {
     float mag_x;
     float mag_y;
     float mag_z;
} st480_t;

int st480_run_library(st480_t st480);
void get_magnetic_offset(float offset[3]);
void get_magnetic_values(float values[3]);
void get_oritation_values(float mag[3], float acc[3], float values[3]);
void get_calilevel_value(int *level);

#ifdef __cplusplus
}
#endif

#endif
