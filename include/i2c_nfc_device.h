/*
 * Copyright (C) 2024 Caian Benedicto <caianbene@gmail.com>
 *
 * This file is part of xinfc.
 *
 * xinfc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * xinfc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xinfc.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <string.h>
#include <stdlib.h>

struct i2c_error
{
    const char* msg;
    int eno;
    int ret;
};

const unsigned int max_ndef_buf_size = 160;
const unsigned short base_ndef_addr = 0x10;

struct i2c_nfc_device
{
	unsigned short _address;
	int _fd;
	int iRetCode;
	struct i2c_error tError;
};

unsigned int min ( unsigned int a, unsigned int b)
{
	if (a<b) return a;
	return b;
}

void i2c_error ( struct i2c_nfc_device * this, const char* szMsg, int eno, int ret )
{
	this->iRetCode = 20;
	this->tError.msg = szMsg;
	this->tError.eno = eno;
	this->tError.ret = ret;
}


void closedev ( struct i2c_nfc_device * this )
{
	if ( this->_fd>=0 )
		close ( this->_fd );
}

void catch ( struct i2c_nfc_device * this )
{
	if ( this->iRetCode==0 )
		return;

	closedev (this);

	fprintf ( stderr, "Error: %s! ret=%d errno=%d\n", this->tError.msg, this->tError.ret, this->tError.eno );
	exit ( this->iRetCode );
}

void i2c_nfc_device ( struct i2c_nfc_device *this, const char * szBus, unsigned short address )
{
	this->_address = address;
	this->iRetCode = 0;
#if !defined(XINFC_DUMMY_OUT)
	char bus_path[100];
	sprintf ( bus_path, "/dev/i2c-%s", szBus );
	this->_fd = open ( bus_path, O_RDWR, 0 );
	if ( this->_fd < 0)
		i2c_error ( this, "failed to open i2c bus", errno, this->_fd );
	catch(this);
#endif
}

void set_timeout( struct i2c_nfc_device * this, unsigned long timeout)
{
#if !defined(XINFC_DUMMY_OUT)
	this->iRetCode = 0;
	if (ioctl( this->_fd, I2C_TIMEOUT, timeout) < 0)
		i2c_error ( this, "failed to set i2c timeout", errno, this->_fd );
	catch ( this );
#endif
}


void set_retries ( struct i2c_nfc_device * this, unsigned long retries )
{
#if !defined(XINFC_DUMMY_OUT)
	this->iRetCode = 0;
	const int r = ioctl ( this->_fd, I2C_RETRIES, retries );
	if ( r<0 )
		i2c_error ( this, "failed to set i2c retries", errno, r );
	catch ( this );
#endif
}


void set_device_address ( struct i2c_nfc_device * this )
{
#if !defined(XINFC_DUMMY_OUT)
	this->iRetCode = 0;
	long laddress = this->_address;
	const int r = ioctl ( this->_fd, I2C_SLAVE, laddress );

	if ( r<0 )
		i2c_error ( this, "failed to set i2c device address", errno, r );
	catch ( this );
#endif
}


void print_rdwr ( const struct i2c_rdwr_ioctl_data * rdwr );

void read_ndef( struct i2c_nfc_device * this, unsigned char* out_buf, unsigned int size_4B_aligned)
{
	this->iRetCode = 0;
	if (size_4B_aligned == 0)
		return;

	if (out_buf == NULL)
		return i2c_error ( this, "invalid ndef buffer", 0, 20 );

	if ((size_4B_aligned % 4) != 0)
		return i2c_error ( this, "invalid read alignment", 0, 20 );

	if (size_4B_aligned > max_ndef_buf_size)
		size_4B_aligned = max_ndef_buf_size;

	const unsigned int read_nmsgs = 2;

	struct i2c_msg msgs[2];
	struct i2c_rdwr_ioctl_data rdwr;

	// The read operation is done by writing
	// the u16 address and then reading

	unsigned char ndef_addr_buf[2] = {
		(base_ndef_addr >> 8) & 0xFF,
		 base_ndef_addr       & 0xFF
	};

	memset(out_buf, 0, size_4B_aligned);

	msgs[0].addr = this->_address;
	msgs[0].flags = 0;
	msgs[0].len = sizeof(ndef_addr_buf);
	msgs[0].buf = ndef_addr_buf;

	msgs[1].addr = this->_address;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = size_4B_aligned;
	msgs[1].buf = out_buf;

	rdwr.msgs = msgs;
	rdwr.nmsgs = read_nmsgs;

#if !defined(XINFC_DUMMY_OUT)
	const int r = ioctl( this->_fd, I2C_RDWR, &rdwr);

	if (r != read_nmsgs)
		return i2c_error (this, "failed to read from i2c device", errno, r );
#else
	print_rdwr(&rdwr);
#endif
}


void write_ndef_at( struct i2c_nfc_device * this, const unsigned char* buf, unsigned int size, unsigned short ndef_off )
{
	this->iRetCode = 0;
	if (size == 0)
		return;

	if (buf == NULL)
		return i2c_error (this, "invalid ndef buffer", 0, 0 );

	if (size > max_ndef_buf_size)
		return i2c_error (this, "invalid ndef buffer size", 0, 0 );

	// Each write operation will only write 4 bytes of data
	const unsigned int write_nmsgs = ((size - 1) / 4) + 1;

	struct i2c_msg msgs[write_nmsgs];
	struct i2c_rdwr_ioctl_data rdwr;

	// Each write operation also needs 2 extra bytes of addressing
	unsigned char ndef_wbuf[write_nmsgs * 6];

	for (unsigned int i = 0; i < write_nmsgs; i++)
	{
		unsigned int curr_buf_off = 4*i;
		const unsigned char * curr_p_buf = buf + curr_buf_off;
		unsigned char * curr_p_wbuf = ndef_wbuf + 6*i;
		unsigned int ndef_addr = base_ndef_addr + ndef_off + curr_buf_off;

		unsigned int len = min(size - curr_buf_off, 4U);

		curr_p_wbuf[0] = (ndef_addr >> 8) & 0xFF;
		curr_p_wbuf[1] =  ndef_addr       & 0xFF;

		for (unsigned int j = 0; j < len; j++)
			curr_p_wbuf[2 + j] = curr_p_buf[j];

		for (unsigned int j = len; j < 4; j++)
			curr_p_wbuf[2 + j] = 0;

		struct i2c_msg* pMsg = &msgs[i];

		pMsg->addr = this->_address;
		pMsg->flags = 0;
		pMsg->len = 6;
		pMsg->buf = curr_p_wbuf;
	}

	rdwr.msgs = msgs;
	rdwr.nmsgs = write_nmsgs;

#if !defined(XINFC_DUMMY_OUT)
	const int r = ioctl(this->_fd, I2C_RDWR, &rdwr);

	if (r != (int)write_nmsgs)
	{
		return i2c_error (this, "failed to write to i2c device", errno, r );
	}
#else
	print_rdwr(&rdwr);
#endif
}

#if defined(XINFC_DUMMY_OUT)
void print_rdwr(const struct i2c_rdwr_ioctl_data* pRdwr)
{
	for (unsigned int i = 0; i <pRdwr->nmsgs; i++)
	{
		const struct i2c_msg* pMsg = &pRdwr->msgs[i];
		const unsigned rd = ( pMsg->flags & I2C_M_RD) != 0;

		if ( rd )
		{
			fprintf ( stderr, "read %d from 0x%02x to 0x%p\n", pMsg->len, pMsg->addr, pMsg->buf );
			return;
		}

		fprintf ( stderr, "write %d to 0x%02x from 0x%p", pMsg->len, pMsg->addr, pMsg->buf );
		for (int i = 0; i < pMsg->len; ++i)
			fprintf ( stderr, " 0x%02x", pMsg->buf[i] );

		fprintf ( stderr, "\n");
	}
}
#endif