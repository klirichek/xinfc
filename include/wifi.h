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

#pragma once

static const unsigned int ssid_min = 2;
static const unsigned int ssid_max = 28;
static const unsigned int pass_min = 8;
static const unsigned int pass_max = 63;

enum wifi_crypt
{
    crypt_none     = 0x01,
    crypt_wep      = 0x02,
    crypt_tkip     = 0x04,
    crypt_aes      = 0x08,
    crypt_tkip_aes = crypt_tkip | crypt_aes,
};

enum wifi_auth
{
    auth_open              = 0x01,
    auth_wpa_personal      = 0x02,
    auth_shared            = 0x04,
    auth_wpa_enterprise    = 0x08,
    auth_wpa2_enterprise   = 0x10,
    auth_wpa2_personal     = 0x20,
    auth_wpa_wpa2_personal = auth_wpa_personal | auth_wpa2_personal,
};

struct wifi_modes {
    const char* szName;
    int iValid;
    enum wifi_crypt tCrypt;
    enum wifi_auth tAuth;
    const char* szMsg;
};

static const char szNoWPA3[] = "WPA3 encryption modes not supported";
static const char szNoOWE[] = "OWE mode not supported";
static const char szWarnMixedWpa3[] = "Mixed WPA2/WPA3 will be announced as WPA2";
static const char szWarnMixedWpa2[] = "Mixed WPA/WPA2 will be announced as WPA2";

/* taken from https://openwrt.org/docs/guide-user/network/wifi/basic#encryption_modes */
static struct wifi_modes g_dModes[] = {
    { "none",                0, crypt_none,     auth_open,              NULL            },
    { "sae",                -1, crypt_none,     auth_open,              szNoWPA3        },
    { "sae-mixed",           0, crypt_aes,      auth_wpa2_personal,     szWarnMixedWpa3 },
    { "psk2+tkip+ccmp",      0, crypt_tkip_aes, auth_wpa2_personal,     NULL            },
    { "psk2+tkip+aes",       0, crypt_tkip_aes, auth_wpa2_personal,     NULL            },
    { "psk2+tkip",           0, crypt_tkip,     auth_wpa2_personal,     NULL            },
    { "psk2+ccmp",           0, crypt_aes,      auth_wpa2_personal,     NULL            },
    { "psk2+aes",            0, crypt_aes,      auth_wpa2_personal,     NULL            },
    { "psk2",                0, crypt_aes,      auth_wpa2_personal,     NULL            },
    { "psk+tkip+ccmp",       0, crypt_tkip_aes, auth_wpa_personal,      NULL            },
    { "psk+tkip+aes",        0, crypt_tkip_aes, auth_wpa_personal,      NULL            },
    { "psk+tkip",            0, crypt_tkip,     auth_wpa_personal,      NULL            },
    { "psk+ccmp",            0, crypt_aes,      auth_wpa_personal,      NULL            },
    { "psk+aes",             0, crypt_aes,      auth_wpa_personal,      NULL            },
    { "psk",                 0, crypt_aes,      auth_wpa_personal,      NULL            },
    { "psk-mixed+tkip+ccmp", 0, crypt_tkip_aes, auth_wpa_wpa2_personal, NULL            },
    { "psk-mixed+tkip+aes",  0, crypt_tkip_aes, auth_wpa_wpa2_personal, NULL            },
    { "psk-mixed+tkip",      0, crypt_tkip,     auth_wpa_wpa2_personal, NULL            },
    { "psk-mixed+ccmp",      0, crypt_aes,      auth_wpa_wpa2_personal, NULL            },
    { "psk-mixed+aes",       0, crypt_aes,      auth_wpa_wpa2_personal, NULL            },
    { "psk-mixed",           0, crypt_aes,      auth_wpa_wpa2_personal, NULL            },
    { "wep",                 0, crypt_wep,      auth_open,              NULL            },
    { "wep+open",            0, crypt_wep,      auth_open,              NULL            },
    { "wep+shared",          0, crypt_wep,      auth_shared,            NULL            },
    { "wpa3",               -1, crypt_none,     auth_open,              szNoWPA3        },
    { "wpa3-mixed",          0, crypt_aes,      auth_wpa2_enterprise,   szWarnMixedWpa3 },
    { "wpa2+tkip+ccmp",      0, crypt_tkip_aes, auth_wpa2_enterprise,   NULL            },
    { "wpa2+tkip+aes",       0, crypt_tkip_aes, auth_wpa2_enterprise,   NULL            },
    { "wpa2+ccmp",           0, crypt_aes,      auth_wpa2_enterprise,   NULL            },
    { "wpa2+aes",            0, crypt_aes,      auth_wpa2_enterprise,   NULL            },
    { "wpa2",                0, crypt_aes,      auth_wpa2_enterprise,   NULL            },
    { "wpa2+tkip",           0, crypt_tkip,     auth_wpa2_enterprise,   NULL            },
    { "wpa+tkip+ccmp",       0, crypt_tkip_aes, auth_wpa_enterprise,    NULL            },
    { "wpa+tkip+aes",        0, crypt_tkip_aes, auth_wpa_enterprise,    NULL            },
    { "wpa+ccmp",            0, crypt_aes,      auth_wpa_enterprise,    NULL            },
    { "wpa+aes",             0, crypt_aes,      auth_wpa_enterprise,    NULL            },
    { "wpa+tkip",            0, crypt_tkip,     auth_wpa_enterprise,    NULL            },
    { "wpa",                 0, crypt_aes,      auth_wpa_enterprise,    NULL            },
    { "wpa-mixed+tkip+ccmp", 0, crypt_tkip_aes, auth_wpa2_enterprise,   szWarnMixedWpa2 },
    { "wpa-mixed+tkip+aes",  0, crypt_tkip_aes, auth_wpa2_enterprise,   szWarnMixedWpa2 },
    { "wpa-mixed+tkip",      0, crypt_tkip,     auth_wpa2_enterprise,   szWarnMixedWpa2 },
    { "wpa-mixed+ccmp",      0, crypt_aes,      auth_wpa2_enterprise,   szWarnMixedWpa2 },
    { "wpa-mixed+aes",       0, crypt_aes,      auth_wpa2_enterprise,   szWarnMixedWpa2 },
    { "wpa-mixed",           0, crypt_aes,      auth_wpa2_enterprise,   szWarnMixedWpa2 },
    { "owe",                -1, crypt_none,     auth_open,              szNoOWE         },
    { NULL,                 -1, crypt_none,     auth_open,              NULL            },
};
