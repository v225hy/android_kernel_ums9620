/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef _SPRD_SENSOR_K_H_
#define _SPRD_SENSOR_K_H_

#include <linux/types.h>
#include <linux/ioctl.h>

struct sensor_i2c_tag {
	__u8  *i2c_data;
	__u16 i2c_count;
	__u16 slave_addr;
	__u16 read_len;
};

struct sensor_i2c_seq_tag {
	__u8  *i2c_data;
	__u16 i2c_count;
	__u16 slave_addr;
	__u16 wr_cnt;
	__u16 rd_cnt;
};

struct sensor_reg_tag {
	__u16 reg_addr;
	__u16 reg_value;
};

struct sensor_reg_bits_tag {
	__u16 reg_addr;
	__u16 reg_value;
	__u32 reg_bits;
};

struct sensor_reg_tab_tag {
	struct sensor_reg_tag *sensor_reg_tab_ptr;
	__u32 reg_count;
	__u32 reg_bits;
	__u32 burst_mode;
};

struct sensor_flash_level {
	__u32 low_light;
	__u32 high_light;
};

enum INTERFACE_OP_ID {
	INTERFACE_OPEN = 0,
	INTERFACE_CLOSE
};

enum INTERFACE_TYPE_ID {
	INTERFACE_CCIR = 0,
	INTERFACE_MIPI
};

struct sensor_if_cfg_tag {
	__u32 is_open;
	__u32 if_type;
	__u32 phy_id;
	__u32 lane_num;
	__u32 bps_per_lane;
	__u32 is_cphy;
	__u64 lane_seq;
};

struct sensor_socid_tag {
	__u32 d_die;
	__u32 a_die;
};

struct sensor_power {
	__u32 af_volt_level;
	__u32 avdd_volt_level;
	__u32 dvdd_volt_level;
	__u32 iovdd_volt_level;
	__u32 power_down_level;
	__u32 reset_level;
	__u32 default_mclk;
};

struct sensor_power_info_tag {
	__u32            is_on;
	__u32            op_sensor_id;
	struct sensor_power dev0;
	struct sensor_power dev1;
	struct sensor_power dev2;
};

enum sensor_otp_param_type_t {
	SENSOR_OTP_PARAM_NORMAL = 0x00,
	SENSOR_OTP_PARAM_CHECKSUM,
	SENSOR_OTP_PARAM_READBYTE,
	SENSOR_OTP_PARAM_FW_VERSION,
	SENSOR_OTP_PARAM_TYPE_MAX
};

struct _sensor_otp_data_info_tag {
	__u32 size;
	void    *data_ptr;
};

struct _sensor_otp_param_tag {
	__u32 type;
	__u32 start_addr;
	__u32 len;
	__u8 *buff;
	struct _sensor_otp_data_info_tag golden;
	struct _sensor_otp_data_info_tag awb;
	struct _sensor_otp_data_info_tag lsc;
};

struct sensor_muti_aec_i2c_tag {
	__u16 *sensor_id;
	__u16 id_size;
	__u16 *i2c_slave_addr;
	__u16 i2c_slave_len;
	__u16 *addr_bits_type;
	__u16 addr_bits_type_len;
	__u16 *data_bits_type;
	__u16 data_bits_type_len;
	struct sensor_reg_tag *master_i2c_tab;
	__u16 msize;
	struct sensor_reg_tag *slave_i2c_tab;
	__u16 ssize;
	/* TODO optimize this later */
	struct sensor_reg_tag *slave_i2c_tab_2;
	__u16 ssize_2;
};

#define AEC_I2C_SETTINGS_MAX 64
#define AEC_I2C_SENSOR_MAX 3

#define SENSOR_IOC_MAGIC			'R'

#define SENSOR_IO_PD	\
	 _IOW(SENSOR_IOC_MAGIC,  0,  __u8)
#define SENSOR_IO_SET_AVDD	\
	 _IOW(SENSOR_IOC_MAGIC,  1,  __u32)
#define SENSOR_IO_SET_DVDD	\
	 _IOW(SENSOR_IOC_MAGIC,  2,  __u32)
#define SENSOR_IO_SET_IOVDD \
	_IOW(SENSOR_IOC_MAGIC,  3,  __u32)
#define SENSOR_IO_SET_MCLK	\
	_IOW(SENSOR_IOC_MAGIC,  4,  __u32)
#define SENSOR_IO_RST	\
	_IOW(SENSOR_IOC_MAGIC,  5,  __u32)
#define SENSOR_IO_I2C_INIT	\
	_IOW(SENSOR_IOC_MAGIC,  6,  __u32)
#define SENSOR_IO_I2C_DEINIT	\
	_IOW(SENSOR_IOC_MAGIC,  7,  __u32)
#define SENSOR_IO_SET_ID	\
	_IOW(SENSOR_IOC_MAGIC,  8,  __u32)
#define SENSOR_IO_RST_LEVEL	\
	_IOW(SENSOR_IOC_MAGIC,  9,  __u32)
#define SENSOR_IO_I2C_ADDR	\
	_IOW(SENSOR_IOC_MAGIC,  10, __u16)
#define SENSOR_IO_I2C_READ	\
	_IOWR(SENSOR_IOC_MAGIC, 11, struct sensor_reg_bits_tag)
#define SENSOR_IO_I2C_WRITE	\
	_IOW(SENSOR_IOC_MAGIC,  12, struct sensor_reg_bits_tag)
#define SENSOR_IO_SET_FLASH	\
	_IOW(SENSOR_IOC_MAGIC,  13, __u32)
#define SENSOR_IO_I2C_WRITE_REGS	\
	_IOW(SENSOR_IOC_MAGIC,  14, struct sensor_reg_tab_tag)
#define SENSOR_IO_SET_CAMMOT	\
	_IOW(SENSOR_IOC_MAGIC,  15, __u32)
#define SENSOR_IO_SET_I2CCLOCK	\
	_IOW(SENSOR_IOC_MAGIC,  16, __u32)
#define SENSOR_IO_GET_FLASH_LEVEL	\
	_IOWR(SENSOR_IOC_MAGIC, 17, struct sensor_flash_level)
#define SENSOR_IO_IF_CFG	\
	_IOW(SENSOR_IOC_MAGIC,  18, struct sensor_if_cfg_tag)
#define SENSOR_IO_GRC_I2C_WRITE	\
	_IOW(SENSOR_IOC_MAGIC,  19, struct sensor_i2c_tag)
#define SENSOR_IO_GRC_I2C_READ	\
	_IOWR(SENSOR_IOC_MAGIC, 20, struct sensor_i2c_tag)
#define SENSOR_IO_POWER_CFG	\
	_IOWR(SENSOR_IOC_MAGIC, 21, struct sensor_power_info_tag)
#define SENSOR_IO_SET_MIPI_SWITCH	\
	_IOW(SENSOR_IOC_MAGIC,  22,  __u32)
#define SENSOR_IO_MUTI_I2C_WRITE	\
	_IOW(SENSOR_IOC_MAGIC,  23, struct sensor_muti_aec_i2c_tag)
#define SENSOR_IO_IF_SWITCH            \
	_IOW(SENSOR_IOC_MAGIC,  24, struct sensor_if_cfg_tag)
#define SENSOR_IO_READ_OTPDATA	\
	_IOWR(SENSOR_IOC_MAGIC, 254, struct _sensor_otp_param_tag)
#define SENSOR_IO_GET_SOCID	\
	_IOWR(SENSOR_IOC_MAGIC, 255, struct sensor_socid_tag)

#endif

