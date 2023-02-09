// Copyright 2022 twitch0001 (@twitch0001)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once


#define QUANTUM_PAINTER_SUPPORTS_256_PALETTE true

#define SPI_DRIVER SPID0
#define SPI_SCK_PIN GP18
#define SPI_MOSI_PIN GP19
#define SPI_MISO_PIN GP20

/*
 * Feature disable options
 *  These options are also useful to firmware size reduction.
 */

/* disable debug print */
//#define NO_DEBUG

/* disable print */
//#define NO_PRINT

/* disable action features */
//#define NO_ACTION_LAYER
//#define NO_ACTION_TAPPING
//#define NO_ACTION_ONESHOT
