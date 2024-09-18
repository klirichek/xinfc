/*
 * Copyright (C) 2024 Alexey N. Vinogradov <a.n.vinogradov@gmail.com>
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

#include "i2c_nfc_device.h"
#include "version.h"
#include "wifi.h"

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int parse_i2c_address(const char*);
int select_encryption_mode(const char*, enum wifi_crypt*, enum wifi_auth*);
int apply_config(const char*, int, const char*, const char*, enum wifi_crypt, enum wifi_auth);
unsigned int make_wsc_ndef(const char*, const char*, enum wifi_crypt, enum wifi_auth, unsigned char*, unsigned int);
void print_usage(const char*);

int main(int argc, const char* argv[])
{
    fprintf ( stderr, "xinfc version %s\n", XINFC_VERSION );

    if (argc != 6)
    {
        print_usage( argv[0]);
        exit(1);
    }

    const char* szi2cbus = argv[1];
    const char* i2caddr_s = argv[2];
    const char* szSsid = argv[3];
    const char* szPass = argv[4];
    const char* szMode = argv[5];

    if ( strlen ( szi2cbus )!=1 )
    {
        fprintf ( stderr, "Error: Invalid i2c bus parameter!\n" );
        exit(1);
    }

    const int i2caddr = parse_i2c_address(i2caddr_s);
    fprintf ( stderr, "I2c device address is %d.\n", i2caddr );
    if (i2caddr <= 0)
        exit(2);

    const unsigned int ssid_size = strlen( szSsid );

    if ( ssid_size < ssid_min || ssid_size > ssid_max)
    {
        fprintf ( stderr, "Error: ssid must have between %d and %d characters!\n", ssid_min, ssid_max );
        exit(3);
    }

    const unsigned int pass_size = strlen ( szPass );

    if ( pass_size < pass_min || pass_size > pass_max)
    {
        fprintf ( stderr, "Error: password must have between %d and %d characters!\n", pass_min, pass_max );
        exit(4);
    }

    enum wifi_crypt crypt;
    enum wifi_auth auth;

    if ( select_encryption_mode ( szMode, &crypt, &auth )!=0 )
        exit ( 5 );

    int r = apply_config ( szi2cbus, i2caddr, szSsid, szPass, crypt, auth );
    return r;
}

int apply_config(
    const char* szi2cbus,
    int i2caddr,
    const char * szSsid,
    const char * szPass,
    enum wifi_crypt crypt,
    enum wifi_auth auth
)
{
    fprintf ( stderr, "Opening i2c bus %s...\n", szi2cbus );

    struct i2c_nfc_device device;
    i2c_nfc_device ( &device, szi2cbus, i2caddr );

    fprintf ( stderr, "Setting i2c timeout...\n" );
    set_timeout ( &device, 3 );

    fprintf ( stderr, "Setting i2c retries...\n" );
    set_retries ( &device, 2 );

    fprintf ( stderr, "Setting i2c device address %d...\n", i2caddr );
    set_device_address( &device );
    fprintf ( stderr, "I2c device address set.\n" );

    ///////////////////////////////////////////////////////

    const char* szbackup_filename = "nfc_ndef_backup.bin";

    unsigned char ndef_rbuf[max_ndef_buf_size];
    unsigned char ndef_wbuf[max_ndef_buf_size];

    fprintf ( stderr, "Reading existing NDEF data...\n" );

    read_ndef ( &device, ndef_rbuf, sizeof ( ndef_rbuf ) );
    catch ( &device );

    fprintf ( stderr, "Read %d bytes.\n", (int)sizeof ( ndef_rbuf ));

    const int backup_fd = open( szbackup_filename, O_EXCL | O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

    if (backup_fd < 0)
    {
        if (errno != EEXIST)
        {
            closedev ( &device );
            fprintf ( stderr, "Cannot open %s! errno=%d\n", szbackup_filename, errno );
            return 11;
        }

        fprintf ( stderr, "Backup file found. Skipping backup...\n" );
    }
    else
    {
        fprintf ( stderr, "Stock chip data backup not found. Backing up...\n" );

        const size_t to_write = sizeof(ndef_rbuf);
        const size_t written = write(backup_fd, ndef_rbuf, sizeof(ndef_rbuf));
        const int eno = errno;

        close(backup_fd);

        if (written != to_write)
        {
            closedev ( &device );
            fprintf ( stderr, "Cannot write to %s! errno=%d\n", szbackup_filename, eno );
            return 12;
        }

        fprintf ( stderr, "Backup complete.\n" );
    }

    ////////////////////////////////////////////////////////////////

    fprintf ( stderr, "Building new NDEF data...\n" );

    const unsigned int size = make_wsc_ndef(szSsid, szPass, crypt, auth, ndef_wbuf, (unsigned int)sizeof(ndef_wbuf));

    fprintf ( stderr, "New NDEF is %d bytes.\n", size );

    ////////////////////////////////////////////////////////////////

    const int max_retries = 5;
    int retries = 0;

    for (; retries < max_retries; ++retries)
    {
        fprintf ( stderr, "Writing new NDEF data...\n" );

        const unsigned int max_bytes = 4;

        for (unsigned int i = 0; i < size; i += max_bytes)
        {
            const unsigned int s = min ( max_bytes, (unsigned int) size-i );

            const int max_w_retries = 5;
            unsigned int w_retries = 0;

            for (; w_retries <= max_w_retries; ++w_retries)
            {
                write_ndef_at(&device, ndef_wbuf + i, s, i);
                if ( device.iRetCode==0 )
                    break;

                if ( w_retries==max_w_retries )
                {
                    closedev ( &device );
                    fprintf ( stderr, "\n" );
                    exit ( device.iRetCode );
                }

                fprintf ( stderr, "x" );

                // The i2c line / device sometimes hang
                // TODO tune timeout/retries ioctl?
                usleep ( 20000 );
            }

            fprintf ( stderr, "." );
        }

        fprintf ( stderr, "\n" );

        usleep(1000000);

        fprintf ( stderr, "Verifying written NDEF data...\n" );

        {
            memset(ndef_rbuf, 0, size);

            const unsigned int aligned_size = (((size - 1) / 4) + 1) * 4;

            const int max_w_retries = 20;
            unsigned int w_retries = 0;

            for (; w_retries <= max_w_retries; ++w_retries)
            {
                read_ndef ( &device, ndef_rbuf, aligned_size );
                if ( device.iRetCode==0 )
                    break;

                if (w_retries == max_w_retries)
                {
                    closedev ( &device );
                    fprintf ( stderr, "\n" );
                    exit ( 20 );
                }

                // The i2c line / device sometimes hang
                // TODO tune timeout/retries ioctl?
                usleep(40000);
            }

            if ( memcmp ( ndef_wbuf, ndef_rbuf, size )==0 )
            {
                fprintf ( stderr, "Success.\n" );
                break;
            }
        }

        fprintf ( stderr, "Data does not match! Retrying...\n" );
    }

    closedev ( &device );
    if (retries == max_retries)
    {
        fprintf ( stderr, "Error: %s\n", "failed to write new NDEF data" );
        exit(20);
    }
    return 0;
}

unsigned int make_wsc_ndef(
    const char * ssid,
    const char * pass,
    enum wifi_crypt crypt,
    enum wifi_auth auth,
    unsigned char* buf,
    unsigned int max_size
)
{
    const char ndef_app[] = "application/vnd.wfa.wsc";

    size_t ssid_size = strlen (ssid);
    size_t pass_size = strlen (pass);

    if ( 69+ssid_size+pass_size>max_size )
        return 0;

    const size_t payload_len = 35 + ssid_size + pass_size;

    int i = 0;

    buf[i++] = 0x03;
    buf[i++] = 30 + payload_len;
    buf[i++] = 0xd2; buf[i++] = 0x17;
    buf[i++] = 4 + payload_len;

    for (unsigned int j = 0; j < sizeof(ndef_app) - 1; j++)
        buf[i++] = ndef_app[j];

    buf[i++] = 0x10; buf[i++] = 0x0e;     // NDEF credential tag
    buf[i++] = (payload_len >> 8) & 0xFF; // payload length
    buf[i++] =  payload_len       & 0xFF; // payload length
    buf[i++] = 0x10; buf[i++] = 0x26;     // NDEF network idx tag
    buf[i++] = 0x00; buf[i++] = 0x01;     // network idx length
    buf[i++] = 0x01;                      // network idx (always 1)
    buf[i++] = 0x10; buf[i++] = 0x45;     // NDEF network name tag
    buf[i++] = ( ssid_size >> 8) & 0xFF;  // ssid length
    buf[i++] = ssid_size         & 0xFF;  // ssid length

    for (unsigned int j = 0; j < ssid_size; ++j)
        buf[i++] = ssid[j];

    buf[i++] = 0x10; buf[i++] = 0x03;     // NDEF auth type tag
    buf[i++] = 0x00; buf[i++] = 0x02;     // auth type length
    buf[i++] = (auth >> 8) & 0xFF;        // auth
    buf[i++] =  auth       & 0xFF;        // auth
    buf[i++] = 0x10; buf[i++] = 0x0f;     // NDEF crypt type tag
    buf[i++] = 0x00; buf[i++] = 0x02;     // crypt type length
    buf[i++] = (crypt >> 8) & 0xFF;       // crypt
    buf[i++] =  crypt       & 0xFF;       // crypt
    buf[i++] = 0x10; buf[i++] = 0x27;     // NDEF network key tag
    buf[i++] = (pass_size >> 8) & 0xFF; // pass length
    buf[i++] =  pass_size       & 0xFF; // pass length

    for (unsigned int j = 0; j < pass_size; ++j)
        buf[i++] = pass[j];

    buf[i++] = 0x10; buf[i++] = 0x20;     // NDEF mac address tag
    buf[i++] = 0x00; buf[i++] = 0x06;     // mac address length
    buf[i++] = 0xFF; buf[i++] = 0xFF;     // mac address
    buf[i++] = 0xFF; buf[i++] = 0xFF;     // mac address
    buf[i++] = 0xFF; buf[i++] = 0xFF;     // mac address
    buf[i++] = 0xFE;

    return i;
}

int parse_i2c_address(const char* szAddr)
{
    if (strlen(szAddr) != 4 || szAddr[0] != '0' || ( szAddr[1] != 'x' && szAddr[1] != 'X'))
    {
        fprintf ( stderr, "Error: Invalid i2c device!\n" );
        return -1;
    }

    int addr = 0;

    for (int i = 0; i < 2; i++)
    {
        int c = (int) szAddr[2 + i];

        if      (c >= '0' && c <= '9') c = c - (int)'0';
        else if (c >= 'a' && c <= 'f') c = c - (int)'a' + 10;
        else if (c >= 'A' && c <= 'F') c = c - (int)'A' + 10;
        else
        {
            fprintf ( stderr, "Error: Invalid i2c device!\n" );
            return -2;
        }

        addr = (addr << 4) + c;
    }

    return addr;
}


int select_encryption_mode ( const char * szMode, enum wifi_crypt * out_crypt, enum wifi_auth * out_auth )
{
    for ( struct wifi_modes * pMode = g_dModes; pMode->szName; ++pMode )
        if ( strcmp ( szMode, pMode->szName )==0 )
        {
            if ( pMode->szMsg )
                fprintf ( stderr, "%s: %s!\n", ( pMode->iValid==0 ) ? "Warning" : "Error", pMode->szMsg );
            if ( pMode->iValid!=0 )
                return -1;
            *out_crypt = pMode->tCrypt;
            *out_auth = pMode->tAuth;
            return 0;
        }

    fprintf ( stderr, "Error: unknown encryption mode!\n");
    return -1;
}

void print_usage(const char* szName)
{
    fprintf ( stderr,"USAGE: %s i2c-bus i2c-device ssid password mode\n"
    "  i2c-device must be a hex byte in the format 0xMN\n"
    "  ssid must have between %d and %d characters\n"
    "  password must have between %d and %d characters.\n"
    "  mode must be one of the following:\n", szName, ssid_min, ssid_max, pass_min, pass_max );

    for ( struct wifi_modes * pMode = g_dModes; pMode->szName; ++pMode )
        if ( pMode->iValid==0 )
            fprintf ( stderr, "    %s\n", pMode->szName );
}
