/*
 *
 * FocalTech TouchScreen driver.
 *
 * Copyright (c) 2010-2017, Focaltech Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*****************************************************************************
*
* File Name: focaltech_gestrue.c
*
* Author: Focaltech Driver Team
*
* Created: 2016-08-08
*
* Abstract:
*
* Reference:
*
*****************************************************************************/

/*****************************************************************************
* 1.Included header files
*****************************************************************************/
#include "focaltech_core.h"
#include "base.h"
#if FTS_GESTURE_EN
/******************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
#define FTS_GESTURE_POINTS                  255
#define FTS_GESTURE_POINTS_ONETIME          62
#define FTS_GESTURE_POINTS_HEADER           8
#define FTS_GESTURE_OUTPUT_ADRESS           0xD3
#define FTS_GESTURE_OUTPUT_UNIT_LENGTH      4

#define UnkownGesture       0
#define DouTap              1   // double tap
#define UpVee               2   // V
#define DownVee             3   // ^
#define LeftVee             4   // >
#define RightVee            5   // <
#define Circle              6   // O
#define DouSwip             7   // ||
#define Left2RightSwip      8   // -->
#define Right2LeftSwip      9   // <--
#define Up2DownSwip         10  // |v
#define Down2UpSwip         11  // |^
#define Mgesture            12  // M
#define Wgesture            13  // W

#define GESTURE_LEFT                        0x20
#define GESTURE_RIGHT                       0x21
#define GESTURE_UP                          0x22
#define GESTURE_DOWN                        0x23
#define GESTURE_DOUBLECLICK                 0x24
#define GESTURE_DOUBLELINE                  0x28
#define GESTURE_CW_DOUBLELINE               0x27

#define GESTURE_O                           0x30
#define GESTURE_W                           0x31
#define GESTURE_M                           0x32
#define GESTURE_E                           0x33
#define GESTURE_L                           0x44
#define GESTURE_S                           0x46
#define GESTURE_V                           0x54
#define GESTURE_Z                           0x65
#define GESTURE_C                           0x34
#define GESTURE_CW_O                        0x57 // Clockwise O
#define GESTURE_LEFT_V                      0x51
#define GESTURE_RIGHT_V                     0x52

#define UNKOWN_GESTURE_ID                   0x00

#define UINT16 unsigned short
#define UINT8 unsigned char
static UINT16 usPointXMax;
static UINT16 usPointYMax;
static UINT16 usPointXMin;
static UINT16 usPointYMin;
static UINT16 usPointXMaxatY;
static UINT16 usPointYMinatX;
static UINT16 usPointXMinatY;
static UINT16 usPointYMaxatX;

unsigned short coordinate_report[14] = {0};
static uint32_t gesture;
static uint32_t gesture_upload;
unsigned short coordinate_x[256]    = {0};
unsigned short coordinate_y[256]    = {0};
unsigned short coordinate_doblelion_1_x[256] = {0};
unsigned short coordinate_doblelion_2_x[256] = {0};
unsigned short coordinate_doblelion_1_y[256] = {0};
unsigned short coordinate_doblelion_2_y[256] = {0};

static int global_gesture_id = 0;
static int gesture_direction = 0;

/* [liliwen start] optimize gesture */
#define RAW_DATA_LENGTH (FTS_GESTURE_POINTS * 5)
static unsigned char raw_data_buf[RAW_DATA_LENGTH] = { 0 };
/* [liliwen end] */

/* [yanlin start] optimize gesture */
#define TP_MAX_X_VALUE		720
#define TP_MAX_Y_VALUE		1440
static int TP_DATA_ERROR = 0;
/* [yanlin end] */

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
static void _get_coordinate(unsigned char *buf, int pointnum)
{
    int i;
    int temp_x1,temp_y1,temp_x2,temp_y2,temp_x3,temp_y3;

    for (i = 0; i < pointnum; i++) {
        coordinate_x[i] =  (((s16) buf[2 + (4 * i)]) & 0x0F) <<
            8 | (((s16) buf[3 + (4 * i)])& 0xFF);
        coordinate_y[i] = (((s16) buf[4 + (4 * i)]) & 0x0F) <<
            8 | (((s16) buf[5 + (4 * i)]) & 0xFF);
        printk("%s:pointx[%d] = %d,pointy[%d] = %d\n",__func__,i,coordinate_x[i],i,coordinate_y[i]);
    }
    if((global_gesture_id==0x31) || (global_gesture_id==0x32)||(global_gesture_id==0x54)) {
        if (coordinate_x[0] < coordinate_x[pointnum-1])
            gesture_direction = 0;
        else
            gesture_direction = 1;
    }else if((global_gesture_id==0x51)||(global_gesture_id==0x52)) {
        if (coordinate_y[0] < coordinate_y[pointnum-1])
            gesture_direction = 0;
        else
            gesture_direction = 1;
    }else if(global_gesture_id==0x30 || global_gesture_id==0x57){
        /* [lichenggang start]modify for gesture "O",
            judging direction is clockwise or anti-clockwise throughing three point coordinates.
            for more accuracy, choosing 1/3 pointnum and 2/3 pointnum.*/
        temp_x1=coordinate_x[0];
        temp_y1=coordinate_y[0];
        temp_x2=coordinate_x[pointnum/3];
        temp_y2=coordinate_y[pointnum/3];
        temp_x3=coordinate_x[pointnum*2/3];
        temp_y3=coordinate_y[pointnum*2/3];
        if(((temp_x2-temp_x1)*(temp_y3-temp_y2)-(temp_y2-temp_y1)*(temp_x3-temp_x2))>0)
            gesture_direction = 0;
        else
            gesture_direction = 1;
    }
    printk("gesture_direction=%d\n",gesture_direction);
    /* [lichenggang end] */
}

static void SpecialGestureTraceMaxAndMin(UINT16 usCurPointX, UINT16 usCurPointY)
{
    if (usCurPointX < usPointXMin) {
        usPointXMin = usCurPointX;
        usPointXMinatY = usCurPointY;
    }

    if (usCurPointX > usPointXMax) {
        usPointXMax = usCurPointX;
        usPointXMaxatY = usCurPointY;
    }

    if (usCurPointY < usPointYMin) {
        usPointYMin = usCurPointY;
        usPointYMinatX = usCurPointX;
    }

    if (usCurPointY > usPointYMax) {
        usPointYMax = usCurPointY;
        usPointYMaxatX = usCurPointX;
    }
}

static void SpecGestureTraceGet(UINT8 uiPointSum)
{
    UINT16 usCurPointX;
    UINT16 usCurPointY;
    UINT8 i = 0;

    usPointXMax = coordinate_x[0];
    usPointXMin = coordinate_x[0];
    usPointYMax = coordinate_y[0];
    usPointYMin = coordinate_y[0];

    for (i = 0; i < uiPointSum; i++) {
        usCurPointX = coordinate_x[i];
        usCurPointY = coordinate_y[i];
        SpecialGestureTraceMaxAndMin(usCurPointX, usCurPointY);
    }
    printk("%s:usPointXMax =%d,usPointXMin =%d,usPointYMax=%d,usPointYMin =%d\n",
                        __func__,usPointXMax,usPointXMin,usPointYMax,usPointYMin);

    /* [yanlin start] optimize gesture data */
    if ((usPointXMax > TP_MAX_X_VALUE) || (usPointXMin == 0) || (usPointYMax > TP_MAX_Y_VALUE) || (usPointYMin == 0)) {
        printk("gesture data is error\n");
        TP_DATA_ERROR = 1;
    }
    /* [yanlin end] */
}

static int _compress_coordinate(unsigned char *buf, int pointnum)
{
    int gesture_id = buf[0];
    int retPointNum=pointnum;
    UINT16 uiTempPointSum = pointnum; // buf[1];

    usPointXMin = 0xFFFF;
    usPointYMin = 0xFFFF;
    usPointXMax = 0;
    usPointYMax = 0;

    SpecGestureTraceGet(pointnum);
    pr_err("%s_1:usPointYMax=%d\n",__func__,usPointYMax);

    if ((gesture_id <= GESTURE_DOWN) && (gesture_id >= GESTURE_LEFT) && pointnum > 2) {
    // buf[1] = 2;
    retPointNum =2;
    coordinate_x[1] = coordinate_x[uiTempPointSum-1];
    coordinate_y[1] = coordinate_y[uiTempPointSum-1];
    } else if ((gesture_id == GESTURE_O) || (gesture_id == GESTURE_CW_O)) { // if char is 'o',make up the PointNum of 6
        // buf[1]=6;
        retPointNum =6;
        coordinate_x[1] = usPointYMinatX;
        coordinate_y[1] = usPointYMin ;
        // Xmin
        coordinate_x[2] = usPointXMin;
        coordinate_y[2] = usPointXMinatY;
        // Ymax
        coordinate_x[3] = usPointYMaxatX;
        coordinate_y[3] = usPointYMax ;
        // xmax
        coordinate_x[4] = usPointXMax;
        coordinate_y[4] = usPointXMaxatY;
        // end point
        coordinate_x[5] = coordinate_x[uiTempPointSum-1];
        coordinate_y[5] = coordinate_y[uiTempPointSum-1];
    } else {
        /* [liliwen] Change to GESTURE_V */
        if ((gesture_id >= GESTURE_LEFT_V) && (gesture_id <= GESTURE_V) && pointnum != 3) {
            if (gesture_id == GESTURE_LEFT_V) // '<'
            {
                coordinate_x[1] = usPointXMin;
                coordinate_y[1] = usPointXMinatY;
            }
            else if (gesture_id == GESTURE_RIGHT_V)// '>'
            {
                coordinate_x[1] = usPointXMax;
                coordinate_y[1] = usPointXMaxatY;
            }
            /* else if (gesture_id == GESTURE_V) // '^'
            {
                coordinate_x[1] = usPointYMinatX;
                coordinate_y[1] = usPointYMin;
            } */
            /* [liliwen] Change to GESTURE_V */
            else if (gesture_id == GESTURE_V) // 'v'
            {
                coordinate_x[1] = usPointYMaxatX;
                coordinate_y[1] = usPointYMax;
            }
            coordinate_x[2] = coordinate_x[uiTempPointSum-1]; // g_stSpecGesStatus
            coordinate_y[2] = coordinate_y[uiTempPointSum-1]; // g_stSpecGesStatus
            //buf[1] = 3;
            retPointNum = 3;
        } else if (((gesture_id == GESTURE_W) || (gesture_id == GESTURE_M)) && (pointnum != 5)) {
            //0x31: 'W'  ,0x32:'M'
            UINT16 usinflectionPointNum = 0;
            UINT16 stepX;
            UINT16 usStartPointX = usPointXMin;
            UINT16 usEndPointX =  usPointXMax;
            usinflectionPointNum = 1;
            stepX = abs(usEndPointX-usStartPointX) / 4;
            if (gesture_id == GESTURE_W) {//  'W'
                coordinate_x[0] = usStartPointX;
                coordinate_y[0] = usPointYMin;
                coordinate_x[1] = usStartPointX+stepX;
                coordinate_y[1] = usPointYMax;
                coordinate_x[2] = usStartPointX+2*stepX;
                coordinate_y[2] = usPointYMin;
                coordinate_x[3] = usStartPointX+3*stepX;
                coordinate_y[3] = usPointYMax;
                coordinate_x[4] = usEndPointX;
                coordinate_y[4] = usPointYMin;
            } else {// 'M'
                coordinate_x[0] = usStartPointX;
                coordinate_y[0] = usPointYMax;
                coordinate_x[1] = usStartPointX+stepX;
                coordinate_y[1] = usPointYMin;
                coordinate_x[2] = usStartPointX+2*stepX;
                coordinate_y[2] = usPointYMax;
                coordinate_x[3] = usStartPointX+3*stepX;
                coordinate_y[3] = usPointYMin;
                coordinate_x[4] = usEndPointX;
                coordinate_y[4] = usPointYMax;
            }
            //buf[1] = 5;
            retPointNum = 5;
        }
    }
    return retPointNum;
}

static void _get_coordinate_report(unsigned char *buf, int pointnum)
{
    int clk;
    int gesture_id = buf[0];
    pr_err("pointnum=%d\n",pointnum);
    _get_coordinate(buf, pointnum);
    pointnum = _compress_coordinate(buf, pointnum);

    if ((gesture_id != GESTURE_O) && (gesture_id != GESTURE_CW_O)) {
        if (!gesture_direction) {
            coordinate_report[1] = coordinate_x[0];
            coordinate_report[2] = coordinate_y[0];
            coordinate_report[3] = coordinate_x[pointnum-1];
            coordinate_report[4] = coordinate_y[pointnum-1];
            coordinate_report[5] = coordinate_x[1];
            coordinate_report[6] = coordinate_y[1];
            coordinate_report[7] = coordinate_x[2];
            coordinate_report[8] = coordinate_y[2];
            coordinate_report[9] = coordinate_x[3];
            coordinate_report[10] = coordinate_y[3];
            coordinate_report[11] = coordinate_x[4];
            coordinate_report[12] = coordinate_y[4];
        } else {
            if (gesture_id == GESTURE_M || gesture_id == GESTURE_W) {
                coordinate_report[1] = coordinate_x[pointnum-1];
                coordinate_report[2] = coordinate_y[pointnum-1];
                coordinate_report[3] = coordinate_x[0];
                coordinate_report[4] = coordinate_y[0];
                coordinate_report[5] = coordinate_x[3];
                coordinate_report[6] = coordinate_y[3];
                coordinate_report[7] = coordinate_x[2];
                coordinate_report[8] = coordinate_y[2];
                coordinate_report[9] = coordinate_x[1];
                coordinate_report[10] = coordinate_y[1];
                coordinate_report[11] = coordinate_x[0];
                coordinate_report[12] = coordinate_y[0];
            } else if (gesture_id == GESTURE_V || gesture_id == GESTURE_LEFT_V || gesture_id == GESTURE_RIGHT_V) {
                coordinate_report[1] = coordinate_x[0];
                coordinate_report[2] = coordinate_y[0];
                coordinate_report[3] = coordinate_x[pointnum-1];
                coordinate_report[4] = coordinate_y[pointnum-1];
                coordinate_report[5] = coordinate_x[1];
                coordinate_report[6] = coordinate_y[1];
                coordinate_report[7] = coordinate_x[2];
                coordinate_report[8] = coordinate_y[2];
                coordinate_report[9] = coordinate_x[3];
                coordinate_report[10] = coordinate_y[3];
                coordinate_report[11] = coordinate_x[4];
                coordinate_report[12] = coordinate_y[4];
            } else {
                coordinate_report[1] = coordinate_x[pointnum-1];
                coordinate_report[2] = coordinate_y[pointnum-1];
                coordinate_report[3] = coordinate_x[0];
                coordinate_report[4] = coordinate_y[0];
                coordinate_report[5] = coordinate_x[3];
                coordinate_report[6] = coordinate_y[3];
                coordinate_report[7] = coordinate_x[2];
                coordinate_report[8] = coordinate_y[2];
                coordinate_report[9] = coordinate_x[1];
                coordinate_report[10] = coordinate_y[1];
                coordinate_report[11] = coordinate_x[0];
                coordinate_report[12] = coordinate_y[0];
            }
        }
    } else {
        coordinate_report[1] = coordinate_x[0];
        coordinate_report[2] = coordinate_y[0];
        coordinate_report[3] = coordinate_x[pointnum-2];
        coordinate_report[4] = coordinate_y[pointnum-2];
        coordinate_report[5] = coordinate_x[1];
        coordinate_report[6] = coordinate_y[1];
        coordinate_report[7] = coordinate_x[2];
        coordinate_report[8] = coordinate_y[2];
        coordinate_report[9] = coordinate_x[3];
        coordinate_report[10] = coordinate_y[3];
        coordinate_report[11] = coordinate_x[4];
        coordinate_report[12] = coordinate_y[4];
        clk = coordinate_x[pointnum-1];
    }
    coordinate_report[13] = gesture_direction;
    if (gesture_id == GESTURE_O) {
        coordinate_report[13] = 0;
    } else if (gesture_id == GESTURE_CW_O) {
        coordinate_report[13] = 1;
    }
}

void fts_read_Gesturedata(struct i2c_client *i2c_client)
{
    int ret = -1,i = 0;
    int gesture_id = 0;

    u8 reg_addr = 0xd3;
    short pointnum = 0;

    raw_data_buf[0] = 0xd3;
    printk( "%s\n", __func__);
    ret = fts_i2c_read(fts_i2c_client, raw_data_buf, 1, raw_data_buf, FTS_GESTURE_POINTS_HEADER);
    if (ret < 0)
    {
        printk( "%s read Gesturedata failed.\n", __func__);
        return;
    }
    memset(coordinate_report, 0, sizeof(coordinate_report));
    {
        gesture_id = raw_data_buf[0];
        global_gesture_id = gesture_id;
        printk("read gesture data:gesture_id =0x%x\n",gesture_id);
        pointnum = (short)(raw_data_buf[1]) & 0xff;
        printk( "pointnum=%d\n", pointnum);
        raw_data_buf[0] = 0xd3;
        if ((gesture_id != GESTURE_DOUBLELINE) && (gesture_id != GESTURE_CW_DOUBLELINE)) {
            if ((pointnum * 4 + 2)<255) {
                ret = fts_i2c_read(i2c_client, &reg_addr, 1, raw_data_buf, (pointnum * 4 + 2));
            } else {
                /* [liliwen start] optimize gesture */
                if((pointnum * 4 + 2) > RAW_DATA_LENGTH)
                {
                    pr_err("error::pointnum data will greater than raw_data_buf\n");
                    memset(raw_data_buf, 0, sizeof(raw_data_buf));
                    return;
                }
                /* [liliwen end] */
                ret = fts_i2c_read(i2c_client, &reg_addr, 1, raw_data_buf, 255);
                ret = fts_i2c_read(i2c_client, &reg_addr, 0, raw_data_buf+255, (pointnum * 4 + 2) -255);
            }
            if (ret < 0) {
                pr_err( "[focaltech]:%s read touchdata failed.\n", __func__);
                return;
            }
            _get_coordinate_report(raw_data_buf, pointnum);
            /* [yanlin start] optimize gesture data */
            if (TP_DATA_ERROR == 1) {
                TP_DATA_ERROR = 0;
                memset(raw_data_buf, 0, sizeof(raw_data_buf));
                return;
            }
            /* [yanlin end] */
        } else { // gesture DOUBLELINE
            if ((pointnum * 4 + 4) < 255) {
                ret = fts_i2c_read(i2c_client, &reg_addr, 1, raw_data_buf, (pointnum * 4 + 4));
            } else {
                /* [liliwen start] optimize gesture */
                if((pointnum * 4 + 4) > RAW_DATA_LENGTH)
                {
                    pr_err("error::DOUBLELINE pointnum data will greater than raw_data_buf\n");
                    memset(raw_data_buf, 0, sizeof(raw_data_buf));
                    return;
                }
                /* [liliwen end] */
                ret = fts_i2c_read(i2c_client, &reg_addr, 1, raw_data_buf, 255);
                ret = fts_i2c_read(i2c_client, &reg_addr, 0, raw_data_buf+255, (pointnum * 4 + 4) - 255);
            }

            for (i = 0; i < pointnum; i++) {
                coordinate_doblelion_1_x[i] = (((s16) raw_data_buf[2 + (4 * i)]) & 0x0F) <<
                    8 | (((s16) raw_data_buf[3 + (4 * i)])& 0xFF);
                coordinate_doblelion_1_y[i] = (((s16) raw_data_buf[4 + (4 * i)]) & 0x0F) <<
                    8 | (((s16) raw_data_buf[5 + (4 * i)]) & 0xFF);
                printk("pointx[%d] = %d,pointy[%d] = %d\n",i,coordinate_doblelion_1_x[i],
                    i,coordinate_doblelion_1_y[i]);
                /* [yanlin start] Optimize double line gesture data */
                if ((coordinate_doblelion_1_x[i] == 0)
                        || (coordinate_doblelion_1_x[i] > TP_MAX_X_VALUE)
                        || (coordinate_doblelion_1_y[i] == 0)
                        || (coordinate_doblelion_1_y[i] > TP_MAX_Y_VALUE)) {
                    printk("DOUBLELINE gesture data is error\n");
                    memset(raw_data_buf, 0, sizeof(raw_data_buf));
                    return;
                }
                /* [yanlin end] */
            }
            coordinate_report[5] = coordinate_doblelion_1_x[0];
            coordinate_report[6] = coordinate_doblelion_1_y[0];
            coordinate_report[7] = coordinate_doblelion_1_x[pointnum-1];
            coordinate_report[8] = coordinate_doblelion_1_y[pointnum-1];
            pointnum = raw_data_buf[pointnum * 4 + 2] << 8 |raw_data_buf[pointnum * 4 + 3];
            if ((pointnum * 4 ) < 255) {
                ret = fts_i2c_read(i2c_client, &reg_addr, 0, raw_data_buf, (pointnum * 4));
            } else {
                ret = fts_i2c_read(i2c_client, &reg_addr, 0, raw_data_buf, 255);
                ret = fts_i2c_read(i2c_client, &reg_addr, 0, raw_data_buf+255, (pointnum * 4) -255);
            }
            for (i = 0; i < pointnum; i++) {
                coordinate_doblelion_2_x[i] = (((s16) raw_data_buf[0 + (4 * i)]) & 0x0F) <<
                    8 | (((s16) raw_data_buf[1 + (4 * i)])& 0xFF);
                coordinate_doblelion_2_y[i] = (((s16) raw_data_buf[2 + (4 * i)]) & 0x0F) <<
                    8 | (((s16) raw_data_buf[3 + (4 * i)]) & 0xFF);
                printk("DOUBLELINE::pointx[%d] = %d,pointy[%d] = %d\n",i,coordinate_doblelion_2_x[i],i,coordinate_doblelion_2_y[i]);
            }
            if (ret < 0){
                pr_err( "[focaltech]:%s read touchdata failed.\n", __func__);
                return;
            }
            coordinate_report[1] = coordinate_doblelion_2_x[0];
            coordinate_report[2] = coordinate_doblelion_2_y[0];
            coordinate_report[3] = coordinate_doblelion_2_x[pointnum-1];
            coordinate_report[4] = coordinate_doblelion_2_y[pointnum-1];

            if (gesture_id == GESTURE_DOUBLELINE)
                coordinate_report[13] = 0;
            else
                coordinate_report[13] = 1;
        }
        gesture = (gesture_id == GESTURE_LEFT)          ? Right2LeftSwip :
                 (gesture_id == GESTURE_RIGHT)          ? Left2RightSwip :
                 (gesture_id == GESTURE_UP)             ? Down2UpSwip :
                 (gesture_id == GESTURE_DOWN)           ? Up2DownSwip :
                 (gesture_id == GESTURE_DOUBLECLICK)    ? DouTap:
                 (gesture_id == GESTURE_DOUBLELINE)     ? DouSwip:
                 (gesture_id == GESTURE_CW_DOUBLELINE)  ? DouSwip:
                 (gesture_id == GESTURE_LEFT_V)         ? RightVee:
                 (gesture_id == GESTURE_RIGHT_V)        ? LeftVee:
                 /* [liliwen] GESTURE_V -> UpVee */
                 (gesture_id == GESTURE_V)              ? UpVee:
                 (gesture_id == GESTURE_O)              ? Circle:
                 (gesture_id == GESTURE_CW_O)           ? Circle:
                 (gesture_id == GESTURE_W)              ? Wgesture:
                 (gesture_id == GESTURE_M)              ? Mgesture:
                 UnkownGesture;

        if (gesture != UnkownGesture ) {
            printk("report_gesture::gesture_id=0x%x,gesture=%d\n",gesture_id,gesture);
            global_gesture_id = UNKOWN_GESTURE_ID;
            /* [liliwen] optimize gesture */
            memset(raw_data_buf, 0, sizeof(raw_data_buf));
            gesture_direction = 0;
            gesture_upload = gesture;
            input_report_key(tpd->dev, KEY_F4, 1);
            input_sync(tpd->dev);
            input_report_key(tpd->dev, KEY_F4, 0);
            input_sync(tpd->dev);
        } else {
            printk("......report_gesture::can't judge gesture.....\n");
            global_gesture_id=UNKOWN_GESTURE_ID;
            fts_i2c_write_reg(i2c_client, 0xa5, 0x00);
            fts_i2c_write_reg(i2c_client, 0xd0, 0x01);
            fts_i2c_write_reg(i2c_client, 0xd1, 0x3f);
            fts_i2c_write_reg(i2c_client, 0xd2, 0x07);
            fts_i2c_write_reg(i2c_client, 0xd6, 0xff);
        }
    }
    return;
}

ssize_t fts_gesture_coordinate_open(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
    int ret = 0;
    char page[512];

    ret = sprintf(page, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", gesture_upload,
                    coordinate_report[1], coordinate_report[2], coordinate_report[3], coordinate_report[4],
                    coordinate_report[5], coordinate_report[6], coordinate_report[7], coordinate_report[8],
                    coordinate_report[9], coordinate_report[10], coordinate_report[11], coordinate_report[12],
                    coordinate_report[13]);
    ret = simple_read_from_buffer(user_buf, count, ppos, page, strlen(page));
    return ret;
}

/* int fts_gesture_coordinate_show(struct seq_file *m, void *v)
{
    size_t ret = 0;
    char page[512];

    ret = sprintf(page, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", gesture_upload,
                 coordinate_report[1], coordinate_report[2], coordinate_report[3], coordinate_report[4],
                 coordinate_report[5], coordinate_report[6], coordinate_report[7], coordinate_report[8],
                 coordinate_report[9], coordinate_report[10], coordinate_report[11], coordinate_report[12],
                 coordinate_report[13]);
    seq_printf(m,page);
    return ret;
}
 */
int fts_gesture_suspend(struct i2c_client *i2c_client)
{
    int i = 0;
    u8 read_data = 0;
    fts_i2c_write_reg(i2c_client, 0xd0, 0x01);
    fts_i2c_write_reg(i2c_client, 0xd1, 0xff);
    fts_i2c_write_reg(i2c_client, 0xd2, 0xff);
    fts_i2c_write_reg(i2c_client, 0xd5, 0xff);
    fts_i2c_write_reg(i2c_client, 0xd6, 0xff);
    fts_i2c_write_reg(i2c_client, 0xd7, 0xff);
    fts_i2c_write_reg(i2c_client, 0xd8, 0xff);
    msleep(20);
    for (i = 0; i < 10; i++) {
        printk("read_data::time=%d",i);
        fts_i2c_read_reg(i2c_client, 0xd0, &read_data);
        if (read_data == 0x01) {
            printk("fts::gesture write 0x01 successful!\n");
            break;
        } else {
            fts_i2c_write_reg(i2c_client, 0xd0, 0x01);
            fts_i2c_write_reg(i2c_client, 0xd1, 0xff);
            fts_i2c_write_reg(i2c_client, 0xd2, 0xff);
            fts_i2c_write_reg(i2c_client, 0xd5, 0xff);
            fts_i2c_write_reg(i2c_client, 0xd6, 0xff);
            fts_i2c_write_reg(i2c_client, 0xd7, 0xff);
            fts_i2c_write_reg(i2c_client, 0xd8, 0xff);
            msleep(10);
        }
    }
    if (i >= 9) {
        printk("fts gesture write 0x01 to d0 fail \n");
        return -1;
    }
    return 0;
}

#endif
