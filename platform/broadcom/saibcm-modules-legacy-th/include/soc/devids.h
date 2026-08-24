/*
 * $Id: devids.h,v 1.309 Broadcom SDK $
 * $Copyright: 2017-2024 Broadcom Inc. All rights reserved.
 * 
 * Permission is granted to use, copy, modify and/or distribute this
 * software under either one of the licenses below.
 * 
 * License Option 1: GPL
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 * 
 * 
 * License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
 * 
 * This software is governed by the Broadcom Open Network Switch APIs license:
 * https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
 * 
 * 
 * 
 */

#ifndef _SOC_DEVIDS_H
#define _SOC_DEVIDS_H

/*
 * Defines PCI device and revision ID for every recognized device.
 * All driver routines refer to this ID only.
 */

#define BROADCOM_VENDOR_ID      0x14e4

/* Draco */
#define BCM5690_DEVICE_ID       0x5690
#define BCM5690_A0_REV_ID       1
#define BCM5690_A1_REV_ID       2
#define BCM5690_A2_REV_ID       3

/* Draco without HG (Medusa) */
#define BCM5691_DEVICE_ID       0x5691
#define BCM5691_A0_REV_ID       1
#define BCM5691_A1_REV_ID       2
#define BCM5691_A2_REV_ID       3

/* Draco without L3 */
#define BCM5692_DEVICE_ID       0x5692
#define BCM5692_A0_REV_ID       1
#define BCM5692_A1_REV_ID       2
#define BCM5692_A2_REV_ID       3

/* Draco without HG or L3 */
#define BCM5693_DEVICE_ID       0x5693
#define BCM5693_A0_REV_ID       1
#define BCM5693_A1_REV_ID       2
#define BCM5693_A2_REV_ID       3

/* Draco 1.5 */
#define BCM5695_DEVICE_ID       0x5695
#define BCM5695_A0_REV_ID       1
#define BCM5695_A1_REV_ID       2
#define BCM5695_B0_REV_ID       0x11

/* Draco 1.5 without HG */
#define BCM5696_DEVICE_ID       0x5696
#define BCM5696_A0_REV_ID       1
#define BCM5696_A1_REV_ID       2
#define BCM5696_B0_REV_ID       0x11

/* Draco 1.5 without L3 */
#define BCM5697_DEVICE_ID       0x5697
#define BCM5697_A0_REV_ID       1
#define BCM5697_A1_REV_ID       2
#define BCM5697_B0_REV_ID       0x11

/* Draco 1.5 without HG or L3 */
#define BCM5698_DEVICE_ID       0x5698
#define BCM5698_A0_REV_ID       1
#define BCM5698_A1_REV_ID       2
#define BCM5698_B0_REV_ID       0x11

/* Hercules with 8 ports */
#define BCM5670_DEVICE_ID       0x5670
#define BCM5670_A0_REV_ID       1
#define BCM5670_A1_REV_ID       2

/* Hercules with 4 ports */
#define BCM5671_DEVICE_ID       0x5671
#define BCM5671_A0_REV_ID       1
#define BCM5671_A1_REV_ID       2
#define BCM5671_A2_REV_ID       3       /* Maxxus */

/* Hercules 1.5 with 8 ports */
#define BCM5675_DEVICE_ID       0x5675
#define BCM5675_A0_REV_ID       1
#define BCM5675_A1_REV_ID       2

/* Hercules 1.5 with 4 ports */
#define BCM5676_DEVICE_ID       0x5676
#define BCM5676_A0_REV_ID       1
#define BCM5676_A1_REV_ID       2

/* Lynx */
#define BCM5673_DEVICE_ID       0x5673
#define BCM5673_A0_REV_ID       1
#define BCM5673_A1_REV_ID       2
#define BCM5673_A2_REV_ID       3

/* Lynx 1.5 */
#define BCM5674_DEVICE_ID       0x5674
#define BCM5674_A0_REV_ID       1

/* Felix */
#define BCM56100_DEVICE_ID      0xb100
#define BCM56100_A0_REV_ID      1
#define BCM56100_A1_REV_ID      2
#define BCM56101_DEVICE_ID      0xb101
#define BCM56101_A0_REV_ID      1
#define BCM56101_A1_REV_ID      2
#define BCM56102_DEVICE_ID      0xb102
#define BCM56102_A0_REV_ID      1
#define BCM56102_A1_REV_ID      2
#define BCM56105_DEVICE_ID      0xb105
#define BCM56105_A0_REV_ID      1
#define BCM56105_A1_REV_ID      2
#define BCM56106_DEVICE_ID      0xb106
#define BCM56106_A0_REV_ID      1
#define BCM56106_A1_REV_ID      2
#define BCM56107_DEVICE_ID      0xb107
#define BCM56107_A0_REV_ID      1
#define BCM56107_A1_REV_ID      2

/* Felix 1.5 */
#define BCM56110_DEVICE_ID      0xb110
#define BCM56110_A0_REV_ID      1
#define BCM56111_DEVICE_ID      0xb111
#define BCM56111_A0_REV_ID      1
#define BCM56112_DEVICE_ID      0xb112
#define BCM56112_A0_REV_ID      1
#define BCM56115_DEVICE_ID      0xb115
#define BCM56115_A0_REV_ID      1
#define BCM56116_DEVICE_ID      0xb116
#define BCM56116_A0_REV_ID      1
#define BCM56117_DEVICE_ID      0xb117
#define BCM56117_A0_REV_ID      1

/* Helix */
#define BCM56300_DEVICE_ID      0xb300
#define BCM56300_A0_REV_ID      1
#define BCM56300_A1_REV_ID      2
#define BCM56300_B0_REV_ID      0x11
#define BCM56300_B1_REV_ID      0x12
#define BCM56301_DEVICE_ID      0xb301
#define BCM56301_A0_REV_ID      1
#define BCM56301_A1_REV_ID      2
#define BCM56301_B0_REV_ID      0x11
#define BCM56301_B1_REV_ID      0x12
#define BCM56302_DEVICE_ID      0xb302
#define BCM56302_A0_REV_ID      1
#define BCM56302_A1_REV_ID      2
#define BCM56302_B0_REV_ID      0x11
#define BCM56302_B1_REV_ID      0x12
#define BCM56303_DEVICE_ID      0xb303
#define BCM56303_A1_REV_ID      2
#define BCM56303_A0_REV_ID      1
#define BCM56303_B0_REV_ID      0x11
#define BCM56303_B1_REV_ID      0x12
#define BCM56304_DEVICE_ID      0xb304
#define BCM56304_A0_REV_ID      1
#define BCM56304_A1_REV_ID      2
#define BCM56304_B0_REV_ID      0x11
#define BCM56304_B1_REV_ID      0x12
#define BCM56404_DEVICE_ID      0xb404
#define BCM56404_A0_REV_ID      1
#define BCM56404_A1_REV_ID      2
#define BCM56305_DEVICE_ID      0xb305
#define BCM56305_A0_REV_ID      1
#define BCM56305_A1_REV_ID      2
#define BCM56305_B0_REV_ID      0x11
#define BCM56305_B1_REV_ID      0x12
#define BCM56306_DEVICE_ID      0xb306
#define BCM56306_A0_REV_ID      1
#define BCM56306_A1_REV_ID      2
#define BCM56306_B0_REV_ID      0x11
#define BCM56306_B1_REV_ID      0x12
#define BCM56307_DEVICE_ID      0xb307
#define BCM56307_A0_REV_ID      1
#define BCM56307_A1_REV_ID      2
#define BCM56307_B0_REV_ID      0x11
#define BCM56307_B1_REV_ID      0x12
#define BCM56308_DEVICE_ID      0xb308
#define BCM56308_A0_REV_ID      1
#define BCM56308_A1_REV_ID      2
#define BCM56308_B0_REV_ID      0x11
#define BCM56308_B1_REV_ID      0x12
#define BCM56309_DEVICE_ID      0xb309
#define BCM56309_A0_REV_ID      1
#define BCM56309_A1_REV_ID      2
#define BCM56309_B0_REV_ID      0x11
#define BCM56309_B1_REV_ID      0x12

/* Helix 1.5 */
#define BCM56310_DEVICE_ID      0xb310
#define BCM56310_A0_REV_ID      1
#define BCM56311_DEVICE_ID      0xb311
#define BCM56311_A0_REV_ID      1
#define BCM56312_DEVICE_ID      0xb312
#define BCM56312_A0_REV_ID      1
#define BCM56313_DEVICE_ID      0xb313
#define BCM56313_A0_REV_ID      1
#define BCM56314_DEVICE_ID      0xb314
#define BCM56314_A0_REV_ID      1
#define BCM56315_DEVICE_ID      0xb315
#define BCM56315_A0_REV_ID      1
#define BCM56316_DEVICE_ID      0xb316
#define BCM56316_A0_REV_ID      1
#define BCM56317_DEVICE_ID      0xb317
#define BCM56317_A0_REV_ID      1
#define BCM56318_DEVICE_ID      0xb318
#define BCM56318_A0_REV_ID      1
#define BCM56319_DEVICE_ID      0xb319
#define BCM56319_A0_REV_ID      1

#ifndef EXCLUDE_BCM56324
/* Helix 2 */
#define BCM56322_DEVICE_ID      0xb322
#define BCM56322_A0_REV_ID      1
#define BCM56324_DEVICE_ID      0xb324
#define BCM56324_A0_REV_ID      1
#endif /* EXCLUDE_BCM56324 */

#define BCM53300_DEVICE_ID      0xb006
#define BCM53300_A0_REV_ID      0x11
#define BCM53300_A1_REV_ID      0x12
#define BCM53301_DEVICE_ID      0xb206
#define BCM53301_A0_REV_ID      0x11
#define BCM53301_A1_REV_ID      0x12
#define BCM53302_DEVICE_ID      0xb008
#define BCM53302_A0_REV_ID      0x11
#define BCM53302_A1_REV_ID      0x12

/* Firebolt */
#define BCM56500_DEVICE_ID      0xb500
#define BCM56500_A0_REV_ID      1
#define BCM56500_A1_REV_ID      2
#define BCM56500_B0_REV_ID      0x11
#define BCM56500_B1_REV_ID      0x12
#define BCM56500_B2_REV_ID      0x13
#define BCM56501_DEVICE_ID      0xb501
#define BCM56501_A0_REV_ID      1
#define BCM56501_A1_REV_ID      2
#define BCM56501_B0_REV_ID      0x11
#define BCM56501_B1_REV_ID      0x12
#define BCM56501_B2_REV_ID      0x13
#define BCM56502_DEVICE_ID      0xb502
#define BCM56502_A0_REV_ID      1
#define BCM56502_A1_REV_ID      2
#define BCM56502_B0_REV_ID      0x11
#define BCM56502_B1_REV_ID      0x12
#define BCM56502_B2_REV_ID      0x13
#define BCM56503_DEVICE_ID      0xb503
#define BCM56503_A0_REV_ID      1
#define BCM56503_A1_REV_ID      2
#define BCM56503_B0_REV_ID      0x11
#define BCM56503_B1_REV_ID      0x12
#define BCM56503_B2_REV_ID      0x13
#define BCM56504_DEVICE_ID      0xb504
#define BCM56504_A0_REV_ID      1
#define BCM56504_A1_REV_ID      2
#define BCM56504_B0_REV_ID      0x11
#define BCM56504_B1_REV_ID      0x12
#define BCM56504_B2_REV_ID      0x13
#define BCM56505_DEVICE_ID      0xb505
#define BCM56505_A0_REV_ID      1
#define BCM56505_A1_REV_ID      2
#define BCM56505_B0_REV_ID      0x11
#define BCM56505_B1_REV_ID      0x12
#define BCM56505_B2_REV_ID      0x13
#define BCM56506_DEVICE_ID      0xb506
#define BCM56506_A0_REV_ID      1
#define BCM56506_A1_REV_ID      2
#define BCM56506_B0_REV_ID      0x11
#define BCM56506_B1_REV_ID      0x12
#define BCM56506_B2_REV_ID      0x13
#define BCM56507_DEVICE_ID      0xb507
#define BCM56507_A0_REV_ID      1
#define BCM56507_A1_REV_ID      2
#define BCM56507_B0_REV_ID      0x11
#define BCM56507_B1_REV_ID      0x12
#define BCM56507_B2_REV_ID      0x13
#define BCM56508_DEVICE_ID      0xb508
#define BCM56508_A0_REV_ID      1
#define BCM56508_A1_REV_ID      2
#define BCM56508_B0_REV_ID      0x11
#define BCM56508_B1_REV_ID      0x12
#define BCM56508_B2_REV_ID      0x13
#define BCM56509_DEVICE_ID      0xb509
#define BCM56509_A0_REV_ID      1
#define BCM56509_A1_REV_ID      2
#define BCM56509_B0_REV_ID      0x11
#define BCM56509_B1_REV_ID      0x12
#define BCM56509_B2_REV_ID      0x13

/* Easyrider */
#define BCM56600_DEVICE_ID      0xb600
#define BCM56600_A0_REV_ID      1
#define BCM56600_B0_REV_ID      0x11
#define BCM56600_C0_REV_ID      0x21
#define BCM56601_DEVICE_ID      0xb601
#define BCM56601_A0_REV_ID      1
#define BCM56601_B0_REV_ID      0x11
#define BCM56601_C0_REV_ID      0x21
#define BCM56602_DEVICE_ID      0xb602
#define BCM56602_A0_REV_ID      1
#define BCM56602_B0_REV_ID      0x11
#define BCM56602_C0_REV_ID      0x21
#define BCM56603_DEVICE_ID      0xb603
#define BCM56603_A0_REV_ID      1
#define BCM56603_B0_REV_ID      0x11
#define BCM56603_C0_REV_ID      0x21
#define BCM56605_DEVICE_ID      0xb605
#define BCM56605_A0_REV_ID      1
#define BCM56605_B0_REV_ID      0x11
#define BCM56605_C0_REV_ID      0x21
#define BCM56606_DEVICE_ID      0xb606
#define BCM56606_A0_REV_ID      1
#define BCM56606_B0_REV_ID      0x11
#define BCM56606_C0_REV_ID      0x21
#define BCM56607_DEVICE_ID      0xb607
#define BCM56607_A0_REV_ID      1
#define BCM56607_B0_REV_ID      0x11
#define BCM56607_C0_REV_ID      0x21
#define BCM56608_DEVICE_ID      0xb608
#define BCM56608_A0_REV_ID      1
#define BCM56608_B0_REV_ID      0x11
#define BCM56608_C0_REV_ID      0x21

/* Goldwing */
#define BCM56580_DEVICE_ID      0xb580
#define BCM56580_A0_REV_ID      1

/* HUMV */
#define BCM56700_DEVICE_ID      0xb700
#define BCM56700_A0_REV_ID      1
#define BCM56701_DEVICE_ID      0xb701
#define BCM56701_A0_REV_ID      1

/* Bradley */
#define BCM56800_DEVICE_ID      0xb800
#define BCM56800_A0_REV_ID      1
#define BCM56801_DEVICE_ID      0xb801
#define BCM56801_A0_REV_ID      1
#define BCM56802_DEVICE_ID      0xb802
#define BCM56802_A0_REV_ID      1
#define BCM56803_DEVICE_ID      0xb803
#define BCM56803_A0_REV_ID      1

/* Raven */
#define BCM56224_DEVICE_ID      0xb224
#define BCM56224_A0_REV_ID      1
#define BCM56224_B0_REV_ID      0x11
#define BCM56225_DEVICE_ID      0xb225
#define BCM56225_A0_REV_ID      1
#define BCM56225_B0_REV_ID      0x11
#define BCM56226_DEVICE_ID      0xb226
#define BCM56226_A0_REV_ID      1
#define BCM56226_B0_REV_ID      0x11
#define BCM56227_DEVICE_ID      0xb227
#define BCM56227_A0_REV_ID      1
#define BCM56227_B0_REV_ID      0x11
#define BCM56228_DEVICE_ID      0xb228
#define BCM56228_A0_REV_ID      1
#define BCM56228_B0_REV_ID      0x11
#define BCM56229_DEVICE_ID      0xb229
#define BCM56229_A0_REV_ID      1
#define BCM56229_B0_REV_ID      0x11
#define BCM56024_DEVICE_ID      0xb024
#define BCM56024_A0_REV_ID      1
#define BCM56024_B0_REV_ID      0x11
#define BCM56025_DEVICE_ID      0xb025
#define BCM56025_A0_REV_ID      1
#define BCM56025_B0_REV_ID      0x11
#define BCM53724_DEVICE_ID      0xc724
#define BCM53724_A0_REV_ID      1
#define BCM53724_B0_REV_ID      0x11
#define BCM53726_DEVICE_ID      0xc726
#define BCM53726_A0_REV_ID      1
#define BCM53726_B0_REV_ID      0x11

/* Hawkeye */
#define BCM53312_DEVICE_ID      0xc312
#define BCM53312_A0_REV_ID      1
#define BCM53312_B0_REV_ID      0x11
#define BCM53313_DEVICE_ID      0xc313
#define BCM53313_A0_REV_ID      1
#define BCM53313_B0_REV_ID      0x11
#define BCM53314_DEVICE_ID      0xc314
#define BCM53314_A0_REV_ID      1
#define BCM53314_B0_REV_ID      0x11

/* Hawkeye EEE */
#define BCM53322_DEVICE_ID      0xc322
#define BCM53322_A0_REV_ID      1
#define BCM53323_DEVICE_ID      0xc323
#define BCM53323_A0_REV_ID      1
#define BCM53324_DEVICE_ID      0xc324
#define BCM53324_A0_REV_ID      1


/* Raptor */
#define BCM56218_DEVICE_ID              0xB218
#define BCM56218_A0_REV_ID              1
#define BCM56218_A1_REV_ID              2
#define BCM56218_A2_REV_ID              3
#define BCM56218X_DEVICE_ID             0xc710
#define BCM56218X_A0_REV_ID             1
#define BCM56218X_A1_REV_ID             2
#define BCM56218X_A2_REV_ID             3
#define BCM56219_DEVICE_ID              0xB219
#define BCM56219_A0_REV_ID              1
#define BCM56219_A1_REV_ID              2
#define BCM56219_A2_REV_ID              3
#define BCM56218R_DEVICE_ID             0xB21A
#define BCM56218R_A0_REV_ID             1
#define BCM56218R_A1_REV_ID             2
#define BCM56218R_A2_REV_ID             3
#define BCM56219R_DEVICE_ID             0xB21B
#define BCM56219R_A0_REV_ID             1
#define BCM56219R_A1_REV_ID             2
#define BCM56219R_A2_REV_ID             3
#define BCM56214_DEVICE_ID              0xB214
#define BCM56214_A0_REV_ID              1
#define BCM56214_A1_REV_ID              2
#define BCM56214_A2_REV_ID              3
#define BCM56215_DEVICE_ID              0xB215
#define BCM56215_A0_REV_ID              1
#define BCM56215_A1_REV_ID              2
#define BCM56215_A2_REV_ID              3
#define BCM56214R_DEVICE_ID             0xB21C
#define BCM56214R_A0_REV_ID             1
#define BCM56214R_A1_REV_ID             2
#define BCM56214R_A2_REV_ID             3
#define BCM56215R_DEVICE_ID             0xB21D
#define BCM56215R_A0_REV_ID             1
#define BCM56215R_A1_REV_ID             2
#define BCM56215R_A2_REV_ID             3
#define BCM56216_DEVICE_ID              0xB216
#define BCM56216_A0_REV_ID              1
#define BCM56216_A1_REV_ID              2
#define BCM56216_A2_REV_ID              3
#define BCM56217_DEVICE_ID              0xB217
#define BCM56217_A0_REV_ID              1
#define BCM56217_A1_REV_ID              2
#define BCM56217_A2_REV_ID              3
#define BCM56212_DEVICE_ID              0xB212
#define BCM56212_A0_REV_ID              1
#define BCM56212_A1_REV_ID              2
#define BCM56212_A2_REV_ID              3
#define BCM56213_DEVICE_ID              0xB213
#define BCM56213_A0_REV_ID              1
#define BCM56213_A1_REV_ID              2
#define BCM56213_A2_REV_ID              3
#define BCM53718_DEVICE_ID              0xC71A
#define BCM53718_A0_REV_ID              1
#define BCM53718_A1_REV_ID              2
#define BCM53718_A2_REV_ID              3
#define BCM53714_DEVICE_ID              0xC71B
#define BCM53714_A0_REV_ID              1
#define BCM53714_A1_REV_ID              2
#define BCM53714_A2_REV_ID              3
#define BCM53716_DEVICE_ID              0xC716
#define BCM53716_A0_REV_ID              1
#define BCM53716_A1_REV_ID              2
#define BCM53716_A2_REV_ID              3
#define BCM56018_DEVICE_ID              0xB018
#define BCM56018_A0_REV_ID              1
#define BCM56018_A1_REV_ID              2
#define BCM56018_A2_REV_ID              3
#define BCM56014_DEVICE_ID              0xB014
#define BCM56014_A0_REV_ID              1
#define BCM56014_A1_REV_ID              2
#define BCM56014_A2_REV_ID              3

/* Firebolt2 */
#define BCM56510_DEVICE_ID      0xb510
#define BCM56510_A0_REV_ID      1
#define BCM56511_DEVICE_ID      0xb511
#define BCM56511_A0_REV_ID      1
#define BCM56512_DEVICE_ID      0xb512
#define BCM56512_A0_REV_ID      1
#define BCM56513_DEVICE_ID      0xb513
#define BCM56513_A0_REV_ID      1
#define BCM56514_DEVICE_ID      0xb514
#define BCM56514_A0_REV_ID      1
#define BCM56516_DEVICE_ID      0xb516
#define BCM56516_A0_REV_ID      1
#define BCM56517_DEVICE_ID      0xb517
#define BCM56517_A0_REV_ID      1
#define BCM56518_DEVICE_ID      0xb518
#define BCM56518_A0_REV_ID      1
#define BCM56519_DEVICE_ID      0xb519
#define BCM56519_A0_REV_ID      1

/* Triumph */
#define BCM56620_DEVICE_ID      0xb620
#define BCM56620_A0_REV_ID      1
#define BCM56620_A1_REV_ID      2
#define BCM56620_B0_REV_ID      0x11
#define BCM56620_B1_REV_ID      0x12
#define BCM56620_B2_REV_ID      0x13
#define BCM56624_DEVICE_ID      0xb624
#define BCM56624_A0_REV_ID      1
#define BCM56624_A1_REV_ID      2
#define BCM56624_B0_REV_ID      0x11
#define BCM56624_B1_REV_ID      0x12
#define BCM56624_B2_REV_ID      0x13
#define BCM56626_DEVICE_ID      0xb626
#define BCM56626_A0_REV_ID      1
#define BCM56626_A1_REV_ID      2
#define BCM56626_B0_REV_ID      0x11
#define BCM56626_B1_REV_ID      0x12
#define BCM56626_B2_REV_ID      0x13
#define BCM56628_DEVICE_ID      0xb628
#define BCM56628_A0_REV_ID      1
#define BCM56628_A1_REV_ID      2
#define BCM56628_B0_REV_ID      0x11
#define BCM56628_B1_REV_ID      0x12
#define BCM56628_B2_REV_ID      0x13
#define BCM56629_DEVICE_ID      0xb629
#define BCM56629_A0_REV_ID      1
#define BCM56629_A1_REV_ID      2
#define BCM56629_B0_REV_ID      0x11
#define BCM56629_B1_REV_ID      0x12
#define BCM56629_B2_REV_ID      0x13

/* Valkyrie */
#define BCM56680_DEVICE_ID      0xb680
#define BCM56680_A0_REV_ID      1
#define BCM56680_A1_REV_ID      2
#define BCM56680_B0_REV_ID      0x11
#define BCM56680_B1_REV_ID      0x12
#define BCM56680_B2_REV_ID      0x13
#define BCM56684_DEVICE_ID      0xb684
#define BCM56684_A0_REV_ID      1
#define BCM56684_A1_REV_ID      2
#define BCM56684_B0_REV_ID      0x11
#define BCM56684_B1_REV_ID      0x12
#define BCM56684_B2_REV_ID      0x13
#define BCM56686_DEVICE_ID      0xb686
#define BCM56686_B0_REV_ID      0x11
#define BCM56686_B1_REV_ID      0x12
#define BCM56686_B2_REV_ID      0x13

/* Scorpion */
#define BCM56820_DEVICE_ID      0xb820
#define BCM56820_A0_REV_ID      1
#define BCM56820_B0_REV_ID      0x11
#define BCM56821_DEVICE_ID      0xb821
#define BCM56821_A0_REV_ID      1
#define BCM56821_B0_REV_ID      0x11
#define BCM56822_DEVICE_ID      0xb822
#define BCM56822_A0_REV_ID      1
#define BCM56822_B0_REV_ID      0x11
#define BCM56823_DEVICE_ID      0xb823
#define BCM56823_A0_REV_ID      1
#define BCM56823_B0_REV_ID      0x11
#define BCM56825_DEVICE_ID      0xb825
#define BCM56825_B0_REV_ID      0x11

/* HUMV Plus */
#define BCM56720_DEVICE_ID      0xb720
#define BCM56720_A0_REV_ID      1
#define BCM56720_B0_REV_ID      0x11
#define BCM56721_DEVICE_ID      0xb721
#define BCM56721_A0_REV_ID      1
#define BCM56721_B0_REV_ID      0x11

/* Conqueror */
#define BCM56725_DEVICE_ID      0xb725
#define BCM56725_A0_REV_ID      1
#define BCM56725_B0_REV_ID      0x11

/* Triumph2 */
#define BCM56630_DEVICE_ID      0xb630
#define BCM56630_A0_REV_ID      1
#define BCM56630_B0_REV_ID      0x11
#define BCM56634_DEVICE_ID      0xb634
#define BCM56634_A0_REV_ID      1
#define BCM56634_B0_REV_ID      0x11
#define BCM56636_DEVICE_ID      0xb636
#define BCM56636_A0_REV_ID      1
#define BCM56636_B0_REV_ID      0x11
#define BCM56638_DEVICE_ID      0xb638
#define BCM56638_A0_REV_ID      1
#define BCM56638_B0_REV_ID      0x11
#define BCM56639_DEVICE_ID      0xb639
#define BCM56639_A0_REV_ID      1
#define BCM56639_B0_REV_ID      0x11

/* Valkyrie2 */
#define BCM56685_DEVICE_ID      0xb685
#define BCM56685_A0_REV_ID      1
#define BCM56685_B0_REV_ID      0x11
#define BCM56689_DEVICE_ID      0xb689
#define BCM56689_A0_REV_ID      1
#define BCM56689_B0_REV_ID      0x11

/* Apollo */
#define BCM56520_DEVICE_ID      0xb520
#define BCM56520_A0_REV_ID      1
#define BCM56520_B0_REV_ID      0x11
#define BCM56521_DEVICE_ID      0xb521
#define BCM56521_A0_REV_ID      1
#define BCM56521_B0_REV_ID      0x11
#define BCM56522_DEVICE_ID      0xb522
#define BCM56522_A0_REV_ID      1
#define BCM56522_B0_REV_ID      0x11
#define BCM56524_DEVICE_ID      0xb524
#define BCM56524_A0_REV_ID      1
#define BCM56524_B0_REV_ID      0x11
#define BCM56526_DEVICE_ID      0xb526
#define BCM56526_A0_REV_ID      1
#define BCM56526_B0_REV_ID      0x11

/* Firebolt 3 */
#define BCM56534_DEVICE_ID      0xb534
#define BCM56534_B0_REV_ID      0x11
#define BCM56538_DEVICE_ID      0xb538
#define BCM56538_B0_REV_ID      0x11

/* Enduro */
#define BCM56331_DEVICE_ID      0xb331
#define BCM56331_A0_REV_ID      1
#define BCM56331_B0_REV_ID      0x11
#define BCM56331_B1_REV_ID      0x12
#define BCM56333_DEVICE_ID      0xb333
#define BCM56333_A0_REV_ID      1
#define BCM56333_B0_REV_ID      0x11
#define BCM56333_B1_REV_ID      0x12
#define BCM56334_DEVICE_ID      0xb334
#define BCM56334_A0_REV_ID      1
#define BCM56334_B0_REV_ID      0x11
#define BCM56334_B1_REV_ID      0x12
#define BCM56338_DEVICE_ID      0xb338
#define BCM56338_A0_REV_ID      1
#define BCM56338_B0_REV_ID      0x11
#define BCM56338_B1_REV_ID      0x12

/* Helix 3 */
#define BCM56320_DEVICE_ID      0xb320
#define BCM56320_A0_REV_ID      1
#define BCM56320_B0_REV_ID      0x11
#define BCM56320_B1_REV_ID      0x12
#define BCM56321_DEVICE_ID      0xb321
#define BCM56321_A0_REV_ID      1
#define BCM56321_B0_REV_ID      0x11
#define BCM56321_B1_REV_ID      0x12


/* FireScout */
#define BCM56548H_DEVICE_ID     0xB54A
#define BCM56548H_A0_REV_ID     1
#define BCM56548_DEVICE_ID      0xb548
#define BCM56548_A0_REV_ID      1
#define BCM56547_DEVICE_ID      0xb547
#define BCM56547_A0_REV_ID      1

/* Helix 4 */
#define BCM56346_DEVICE_ID      0xb346
#define BCM56346_A0_REV_ID      1
#define BCM56345_DEVICE_ID      0xb345
#define BCM56345_A0_REV_ID      1
#define BCM56344_DEVICE_ID      0xb344
#define BCM56344_A0_REV_ID      1
#define BCM56342_DEVICE_ID      0xb342
#define BCM56342_A0_REV_ID      1
#define BCM56340_DEVICE_ID      0xb340
#define BCM56340_A0_REV_ID      1

/* Spiral */
#define BCM56049_DEVICE_ID      0xb049
#define BCM56049_A0_REV_ID      1
#define BCM56048_DEVICE_ID      0xb048
#define BCM56048_A0_REV_ID      1
#define BCM56047_DEVICE_ID      0xb047
#define BCM56047_A0_REV_ID      1

/* Ranger */
#define BCM56042_DEVICE_ID      0xb042
#define BCM56042_A0_REV_ID      1
#define BCM56041_DEVICE_ID      0xb041
#define BCM56041_A0_REV_ID      1
#define BCM56040_DEVICE_ID      0xb040
#define BCM56040_A0_REV_ID      1

/* Stardust */
#define BCM56132_DEVICE_ID      0xb132
#define BCM56132_A0_REV_ID      1
#define BCM56132_B0_REV_ID      0x11
#define BCM56132_B1_REV_ID      0x12
#define BCM56134_DEVICE_ID      0xb134
#define BCM56134_A0_REV_ID      1
#define BCM56134_B0_REV_ID      0x11
#define BCM56134_B1_REV_ID      0x12

/* Dagger */
#define BCM56230_DEVICE_ID      0xb230
#define BCM56230_B1_REV_ID      0x12
#define BCM56231_DEVICE_ID      0xb231
#define BCM56231_B1_REV_ID      0x12

/* Hurricane */
#define BCM56140_DEVICE_ID      0xb140
#define BCM56140_A0_REV_ID      1
#define BCM56142_DEVICE_ID      0xb142
#define BCM56142_A0_REV_ID      1
#define BCM56143_DEVICE_ID      0xb143
#define BCM56143_A0_REV_ID      1
#define BCM56144_DEVICE_ID      0xb144
#define BCM56144_A0_REV_ID      1
#define BCM56146_DEVICE_ID      0xb146
#define BCM56146_A0_REV_ID      1
#define BCM56147_DEVICE_ID      0xb147
#define BCM56147_A0_REV_ID      1
#define BCM56149_DEVICE_ID      0xb149
#define BCM56149_A0_REV_ID      1

/* Trident */
#define BCM56840_DEVICE_ID      0xb840
#define BCM56840_A0_REV_ID      1
#define BCM56840_A1_REV_ID      2
#define BCM56840_A2_REV_ID      3
#define BCM56840_A3_REV_ID      4
#define BCM56840_A4_REV_ID      5
#define BCM56840_B0_REV_ID      0x11
#define BCM56840_B1_REV_ID      0x12
#define BCM56841_DEVICE_ID      0xb841
#define BCM56841_A0_REV_ID      1
#define BCM56841_A1_REV_ID      2
#define BCM56841_A2_REV_ID      3
#define BCM56841_A3_REV_ID      4
#define BCM56841_A4_REV_ID      5
#define BCM56841_B0_REV_ID      0x11
#define BCM56841_B1_REV_ID      0x12
#define BCM56843_DEVICE_ID      0xb843
#define BCM56843_A0_REV_ID      1
#define BCM56843_A1_REV_ID      2
#define BCM56843_A2_REV_ID      3
#define BCM56843_A3_REV_ID      4
#define BCM56843_A4_REV_ID      5
#define BCM56843_B0_REV_ID      0x11
#define BCM56843_B1_REV_ID      0x12
#define BCM56845_DEVICE_ID      0xb845
#define BCM56845_A0_REV_ID      1
#define BCM56845_A1_REV_ID      2
#define BCM56845_A2_REV_ID      3
#define BCM56845_A3_REV_ID      4
#define BCM56845_A4_REV_ID      5
#define BCM56845_B0_REV_ID      0x11
#define BCM56845_B1_REV_ID      0x12

/* Titan */
#define BCM56743_DEVICE_ID      0xb743
#define BCM56743_A0_REV_ID      1
#define BCM56743_A1_REV_ID      2
#define BCM56743_A2_REV_ID      3
#define BCM56743_A3_REV_ID      4
#define BCM56743_A4_REV_ID      5
#define BCM56743_B0_REV_ID      0x11
#define BCM56743_B1_REV_ID      0x12
#define BCM56745_DEVICE_ID      0xb745
#define BCM56745_A0_REV_ID      1
#define BCM56745_A1_REV_ID      2
#define BCM56745_A2_REV_ID      3
#define BCM56745_A3_REV_ID      4
#define BCM56745_A4_REV_ID      5
#define BCM56745_B0_REV_ID      0x11
#define BCM56745_B1_REV_ID      0x12

/* Saber 2 */
#define BCM56260_DEVICE_ID      0xb260
#define BCM56260_A0_REV_ID      1
#define BCM56260_B0_REV_ID      0x11
#define BCM56261_DEVICE_ID      0xb261
#define BCM56261_A0_REV_ID      1
#define BCM56261_B0_REV_ID      0x11
#define BCM56262_DEVICE_ID      0xb262
#define BCM56262_A0_REV_ID      1
#define BCM56262_B0_REV_ID      0x11
#define BCM56263_DEVICE_ID      0xb263
#define BCM56263_A0_REV_ID      1
#define BCM56263_B0_REV_ID      0x11

#define BCM56265_DEVICE_ID      0xb265
#define BCM56265_A0_REV_ID      1
#define BCM56265_B0_REV_ID      0x11
#define BCM56266_DEVICE_ID      0xb266
#define BCM56266_A0_REV_ID      1
#define BCM56266_B0_REV_ID      0x11
#define BCM56267_DEVICE_ID      0xb267
#define BCM56267_A0_REV_ID      1
#define BCM56267_B0_REV_ID      0x11
#define BCM56268_DEVICE_ID      0xb268
#define BCM56268_A0_REV_ID      1
#define BCM56268_B0_REV_ID      0x11

/*Dagger 2*/
#define BCM56233_DEVICE_ID      0xb233
#define BCM56233_B0_REV_ID      0x11

#define BCM56460_DEVICE_ID      0xb460
#define BCM56460_A0_REV_ID      1
#define BCM56460_B0_REV_ID      0x11
#define BCM56461_DEVICE_ID      0xb461
#define BCM56461_A0_REV_ID      1
#define BCM56461_B0_REV_ID      0x11
#define BCM56462_DEVICE_ID      0xb462
#define BCM56462_A0_REV_ID      1
#define BCM56462_B0_REV_ID      0x11
#define BCM56463_DEVICE_ID      0xb463
#define BCM56463_A0_REV_ID      1
#define BCM56463_B0_REV_ID      0x11

#define BCM56465_DEVICE_ID      0xb465
#define BCM56465_A0_REV_ID      1
#define BCM56465_B0_REV_ID      0x11
#define BCM56466_DEVICE_ID      0xb466
#define BCM56466_A0_REV_ID      1
#define BCM56466_B0_REV_ID      0x11
#define BCM56467_DEVICE_ID      0xb467
#define BCM56467_A0_REV_ID      1
#define BCM56467_B0_REV_ID      0x11
#define BCM56468_DEVICE_ID      0xb468
#define BCM56468_A0_REV_ID      1
#define BCM56468_B0_REV_ID      0x11

/* Metrolite */
#define BCM56270_DEVICE_ID      0xb270
#define BCM56270_A0_REV_ID      1
#define BCM56271_DEVICE_ID      0xb271
#define BCM56271_A0_REV_ID      1
#define BCM56272_DEVICE_ID      0xb272
#define BCM56272_A0_REV_ID      1

/* Beagle */
#define BCM53460_DEVICE_ID      0x8460
#define BCM53460_A0_REV_ID      1
#define BCM53461_DEVICE_ID      0x8461
#define BCM53461_A0_REV_ID      1

/* Trident Plus */
#define BCM56842_DEVICE_ID      0xb842
#define BCM56842_A0_REV_ID      1
#define BCM56842_A1_REV_ID      2
#define BCM56844_DEVICE_ID      0xb844
#define BCM56844_A0_REV_ID      1
#define BCM56844_A1_REV_ID      2
#define BCM56846_DEVICE_ID      0xb846
#define BCM56846_A0_REV_ID      1
#define BCM56846_A1_REV_ID      2
#define BCM56549_DEVICE_ID      0xb549
#define BCM56549_A0_REV_ID      1
#define BCM56549_A1_REV_ID      2
#define BCM56053_DEVICE_ID      0xb053
#define BCM56053_A0_REV_ID      1
#define BCM56053_A1_REV_ID      2
#define BCM56831_DEVICE_ID      0xb831
#define BCM56831_A0_REV_ID      1
#define BCM56831_A1_REV_ID      2
#define BCM56835_DEVICE_ID      0xb835
#define BCM56835_A0_REV_ID      1
#define BCM56835_A1_REV_ID      2
#define BCM56838_DEVICE_ID      0xb838
#define BCM56838_A0_REV_ID      1
#define BCM56838_A1_REV_ID      2
#define BCM56847_DEVICE_ID      0xb847
#define BCM56847_A0_REV_ID      1
#define BCM56847_A1_REV_ID      2
#define BCM56847_A2_REV_ID      3
#define BCM56847_A3_REV_ID      4
#define BCM56847_A4_REV_ID      5
#define BCM56847_B0_REV_ID      0x11
#define BCM56847_B1_REV_ID      0x12
#define BCM56849_DEVICE_ID      0xb849
#define BCM56849_A0_REV_ID      1
#define BCM56849_A1_REV_ID      2

/* Titan Plus */
#define BCM56742_DEVICE_ID      0xb742
#define BCM56742_A0_REV_ID      1
#define BCM56742_A1_REV_ID      2
#define BCM56742_A2_REV_ID      3
#define BCM56744_DEVICE_ID      0xb744
#define BCM56744_A0_REV_ID      1
#define BCM56744_A1_REV_ID      2
#define BCM56746_DEVICE_ID      0xb746
#define BCM56746_A0_REV_ID      1
#define BCM56746_A1_REV_ID      2


/* Triumph 3 */
#define BCM56640_DEVICE_ID      0xb640
#define BCM56640_A0_REV_ID      1
#define BCM56640_A1_REV_ID      2
#define BCM56640_B0_REV_ID      0x11
#define BCM56643_DEVICE_ID      0xb643
#define BCM56643_A0_REV_ID      1
#define BCM56643_A1_REV_ID      2
#define BCM56643_B0_REV_ID      0x11
#define BCM56644_DEVICE_ID      0xb644
#define BCM56644_A0_REV_ID      1
#define BCM56644_A1_REV_ID      2
#define BCM56644_B0_REV_ID      0x11
#define BCM56648_DEVICE_ID      0xb648
#define BCM56648_A0_REV_ID      1
#define BCM56648_A1_REV_ID      2
#define BCM56648_B0_REV_ID      0x11
#define BCM56649_DEVICE_ID      0xb649
#define BCM56649_A0_REV_ID      1
#define BCM56649_A1_REV_ID      2
#define BCM56649_B0_REV_ID      0x11

/* Apollo 2 */
#define BCM56540_DEVICE_ID      0xb540
#define BCM56540_A0_REV_ID      1
#define BCM56540_A1_REV_ID      2
#define BCM56540_B0_REV_ID      0x11
#define BCM56541_DEVICE_ID      0xb541
#define BCM56541_A0_REV_ID      1
#define BCM56541_A1_REV_ID      2
#define BCM56541_B0_REV_ID      0x11
#define BCM56542_DEVICE_ID      0xb542
#define BCM56542_A0_REV_ID      1
#define BCM56542_A1_REV_ID      2
#define BCM56542_B0_REV_ID      0x11
#define BCM56543_DEVICE_ID      0xb543
#define BCM56543_A0_REV_ID      1
#define BCM56543_A1_REV_ID      2
#define BCM56543_B0_REV_ID      0x11
#define BCM56544_DEVICE_ID      0xb544
#define BCM56544_A0_REV_ID      1
#define BCM56544_A1_REV_ID      2
#define BCM56544_B0_REV_ID      0x11

/* Firebolt 4 */
#define BCM56545_DEVICE_ID      0xb545
#define BCM56545_A0_REV_ID      1
#define BCM56545_A1_REV_ID      2
#define BCM56545_B0_REV_ID      0x11
#define BCM56546_DEVICE_ID      0xb546
#define BCM56546_A0_REV_ID      1
#define BCM56546_A1_REV_ID      2
#define BCM56546_B0_REV_ID      0x11

/* Ranger plus */
#define BCM56044_DEVICE_ID      0xb044
#define BCM56044_B0_REV_ID      0x11
#define BCM56045_DEVICE_ID      0xb045
#define BCM56045_A0_REV_ID      1
#define BCM56045_A1_REV_ID      2
#define BCM56045_B0_REV_ID      0x11
#define BCM56046_DEVICE_ID      0xb046
#define BCM56046_A0_REV_ID      1
#define BCM56046_A1_REV_ID      2
#define BCM56046_B0_REV_ID      0x11


/* Katana */
#define BCM56440_DEVICE_ID      0xb440
#define BCM56440_A0_REV_ID      1
#define BCM56440_B0_REV_ID      0x11
#define BCM56441_DEVICE_ID      0xb441
#define BCM56441_A0_REV_ID      1
#define BCM56441_B0_REV_ID      0x11
#define BCM56442_DEVICE_ID      0xb442
#define BCM56442_A0_REV_ID      1
#define BCM56442_B0_REV_ID      0x11
#define BCM56443_DEVICE_ID      0xb443
#define BCM56443_A0_REV_ID      1
#define BCM56443_B0_REV_ID      0x11
#define BCM56445_DEVICE_ID      0xb445
#define BCM56445_A0_REV_ID      1
#define BCM56445_B0_REV_ID      0x11
#define BCM56446_DEVICE_ID      0xb446
#define BCM56446_A0_REV_ID      1
#define BCM56446_B0_REV_ID      0x11
#define BCM56447_DEVICE_ID      0xb447
#define BCM56447_A0_REV_ID      1
#define BCM56447_B0_REV_ID      0x11
#define BCM56448_DEVICE_ID      0xb448
#define BCM56448_A0_REV_ID      1
#define BCM56448_B0_REV_ID      0x11
#define BCM56449_DEVICE_ID      0xb449
#define BCM56449_A0_REV_ID      1
#define BCM56449_B0_REV_ID      0x11
#define BCM56240_DEVICE_ID      0xb240
#define BCM56240_A0_REV_ID      1
#define BCM56240_B0_REV_ID      0x11
#define BCM56241_DEVICE_ID      0xb241
#define BCM56241_A0_REV_ID      1
#define BCM56241_B0_REV_ID      0x11
#define BCM56242_DEVICE_ID      0xb242
#define BCM56242_A0_REV_ID      1
#define BCM56242_B0_REV_ID      0x11
#define BCM56243_DEVICE_ID      0xb243
#define BCM56243_A0_REV_ID      1
#define BCM56243_B0_REV_ID      0x11
#define BCM56245_DEVICE_ID      0xb245
#define BCM56245_A0_REV_ID      1
#define BCM56245_B0_REV_ID      0x11
#define BCM56246_DEVICE_ID      0xb246
#define BCM56246_A0_REV_ID      1
#define BCM56246_B0_REV_ID      0x11
#define BCM55440_DEVICE_ID      0xa440
#define BCM55440_A0_REV_ID      1
#define BCM55440_B0_REV_ID      0x11
#define BCM55441_DEVICE_ID      0xa441
#define BCM55441_A0_REV_ID      1
#define BCM55441_B0_REV_ID      0x11

/* Katana 2 */
#define BCM55450_DEVICE_ID      0xa450
#define BCM55450_A0_REV_ID      1
#define BCM55450_B0_REV_ID      0x11
#define BCM55450_B1_REV_ID      0x12

#define BCM55455_DEVICE_ID      0xa455
#define BCM55455_A0_REV_ID      1
#define BCM55455_B0_REV_ID      0x11
#define BCM55455_B1_REV_ID      0x12

#define BCM56248_DEVICE_ID      0xb248
#define BCM56248_A0_REV_ID      1
#define BCM56248_B0_REV_ID      0x11
#define BCM56248_B1_REV_ID      0x12

#define BCM56450_DEVICE_ID      0xb450
#define BCM56450_A0_REV_ID      1
#define BCM56450_B0_REV_ID      0x11
#define BCM56450_B1_REV_ID      0x12

#define BCM56452_DEVICE_ID      0xb452
#define BCM56452_A0_REV_ID      1
#define BCM56452_B0_REV_ID      0x11
#define BCM56452_B1_REV_ID      0x12

#define BCM56454_DEVICE_ID      0xb454
#define BCM56454_A0_REV_ID      1
#define BCM56454_B0_REV_ID      0x11
#define BCM56454_B1_REV_ID      0x12

#define BCM56455_DEVICE_ID      0xb455
#define BCM56455_A0_REV_ID      1
#define BCM56455_B0_REV_ID      0x11
#define BCM56455_B1_REV_ID      0x12

#define BCM56456_DEVICE_ID      0xb456
#define BCM56456_A0_REV_ID      1
#define BCM56456_B0_REV_ID      0x11
#define BCM56456_B1_REV_ID      0x12

#define BCM56457_DEVICE_ID      0xb457
#define BCM56457_A0_REV_ID      1
#define BCM56457_B0_REV_ID      0x11
#define BCM56457_B1_REV_ID      0x12

#define BCM56458_DEVICE_ID      0xb458
#define BCM56458_A0_REV_ID      1
#define BCM56458_B0_REV_ID      0x11
#define BCM56458_B1_REV_ID      0x12

/* Trident 2 */
#define BCM56850_DEVICE_ID      0xb850
#define BCM56850_A0_REV_ID      1
#define BCM56850_A1_REV_ID      2
#define BCM56850_A2_REV_ID      3
#define BCM56851_DEVICE_ID      0xb851
#define BCM56851_A0_REV_ID      1
#define BCM56851_A1_REV_ID      2
#define BCM56851_A2_REV_ID      3
#define BCM56852_DEVICE_ID      0xb852
#define BCM56852_A0_REV_ID      1
#define BCM56852_A1_REV_ID      2
#define BCM56852_A2_REV_ID      3
#define BCM56853_DEVICE_ID      0xb853
#define BCM56853_A0_REV_ID      1
#define BCM56853_A1_REV_ID      2
#define BCM56853_A2_REV_ID      3
#define BCM56854_DEVICE_ID      0xb854
#define BCM56854_A0_REV_ID      1
#define BCM56854_A1_REV_ID      2
#define BCM56854_A2_REV_ID      3
#define BCM56855_DEVICE_ID      0xb855
#define BCM56855_A0_REV_ID      1
#define BCM56855_A1_REV_ID      2
#define BCM56855_A2_REV_ID      3
#define BCM56834_DEVICE_ID      0xb834
#define BCM56834_A0_REV_ID      1
#define BCM56834_A1_REV_ID      2
#define BCM56834_A2_REV_ID      3

/* Trident 2+ */
#define BCM56860_DEVICE_ID      0xb860
#define BCM56860_A0_REV_ID      1
#define BCM56860_A1_REV_ID      2
#define BCM56861_DEVICE_ID      0xb861
#define BCM56861_A0_REV_ID      1
#define BCM56861_A1_REV_ID      2
#define BCM56862_DEVICE_ID      0xb862
#define BCM56862_A0_REV_ID      1
#define BCM56862_A1_REV_ID      2
#define BCM56864_DEVICE_ID      0xb864
#define BCM56864_A0_REV_ID      1
#define BCM56864_A1_REV_ID      2
#define BCM56865_DEVICE_ID      0xb865
#define BCM56865_A0_REV_ID      1
#define BCM56865_A1_REV_ID      2
#define BCM56866_DEVICE_ID      0xb866
#define BCM56866_A0_REV_ID      1
#define BCM56866_A1_REV_ID      2
#define BCM56867_DEVICE_ID      0xb867
#define BCM56867_A0_REV_ID      1
#define BCM56867_A1_REV_ID      2
#define BCM56868_DEVICE_ID      0xb868
#define BCM56868_A0_REV_ID      1
#define BCM56868_A1_REV_ID      2
#define BCM56832_DEVICE_ID      0xb832
#define BCM56832_A0_REV_ID      1
#define BCM56832_A1_REV_ID      2
#define BCM56833_DEVICE_ID      0xb833
#define BCM56833_A0_REV_ID      1
#define BCM56833_A1_REV_ID      2
#define BCM56836_DEVICE_ID      0xb836
#define BCM56836_A0_REV_ID      1
#define BCM56836_A1_REV_ID      2


/* Titan 2 */
#define BCM56750_DEVICE_ID      0xb750
#define BCM56750_A0_REV_ID      1
#define BCM56750_A1_REV_ID      2
#define BCM56750_A2_REV_ID      3

/* Scorpion 960 */
#define BCM56830_DEVICE_ID      0xb830
#define BCM56830_A0_REV_ID      1
#define BCM56830_A1_REV_ID      2
#define BCM56830_A2_REV_ID      3

/* Hurricane 2*/
#define BCM56150_DEVICE_ID      0xb150
#define BCM56150_A0_REV_ID      1
#define BCM56151_DEVICE_ID      0xb151
#define BCM56151_A0_REV_ID      1
#define BCM56152_DEVICE_ID      0xb152
#define BCM56152_A0_REV_ID      1

/* Wolfhound*/
#define BCM53342_DEVICE_ID      0x8342
#define BCM53342_A0_REV_ID      1
#define BCM53343_DEVICE_ID      0x8343
#define BCM53343_A0_REV_ID      1
#define BCM53344_DEVICE_ID      0x8344
#define BCM53344_A0_REV_ID      1
#define BCM53346_DEVICE_ID      0x8346
#define BCM53346_A0_REV_ID      1
#define BCM53347_DEVICE_ID      0x8347
#define BCM53347_A0_REV_ID      1

/* Foxhound*/
#define BCM53333_DEVICE_ID      0x8333
#define BCM53333_A0_REV_ID      1
#define BCM53334_DEVICE_ID      0x8334
#define BCM53334_A0_REV_ID      1

/* Deerhound*/
#define BCM53393_DEVICE_ID      0x8393
#define BCM53393_A0_REV_ID      1
#define BCM53394_DEVICE_ID      0x8394
#define BCM53394_A0_REV_ID      1

/* Greyhound , Ranger 2 */
#define BCM53400_DEVICE_ID      0x8400 
#define BCM53400_A0_REV_ID      1
#define BCM56060_DEVICE_ID      0xb060
#define BCM56060_A0_REV_ID      1
#define BCM56062_DEVICE_ID      0xb062
#define BCM56062_A0_REV_ID      1
#define BCM56063_DEVICE_ID      0xb063
#define BCM56063_A0_REV_ID      1
#define BCM56064_DEVICE_ID      0xb064
#define BCM56064_A0_REV_ID      1
#define BCM56065_DEVICE_ID      0xb065
#define BCM56065_A0_REV_ID      1
#define BCM56066_DEVICE_ID      0xb066
#define BCM56066_A0_REV_ID      1
#define BCM53401_DEVICE_ID      0x8401
#define BCM53411_DEVICE_ID      0x8411
#define BCM53401_A0_REV_ID      1
#define BCM53402_DEVICE_ID      0x8402
#define BCM53412_DEVICE_ID      0x8412
#define BCM53402_A0_REV_ID      1
#define BCM53403_DEVICE_ID      0x8403
#define BCM53413_DEVICE_ID      0x8413
#define BCM53403_A0_REV_ID      1
#define BCM53404_DEVICE_ID      0x8404
#define BCM53414_DEVICE_ID      0x8414
#define BCM53404_A0_REV_ID      1
#define BCM53405_DEVICE_ID      0x8405
#define BCM53415_DEVICE_ID      0x8415
#define BCM53405_A0_REV_ID      1
#define BCM53406_DEVICE_ID      0x8406
#define BCM53416_DEVICE_ID      0x8416
#define BCM53406_A0_REV_ID      1
#define BCM53408_DEVICE_ID      0x8408
#define BCM53418_DEVICE_ID      0x8418
#define BCM53408_A0_REV_ID      1
#define BCM53365_DEVICE_ID      0x8365
#define BCM53365_A0_REV_ID      1
#define BCM53369_DEVICE_ID      0x8369
#define BCM53369_A0_REV_ID      1

/* Elkhound */
#define BCM53454_DEVICE_ID      0x8454
#define BCM53455_DEVICE_ID      0x8455
#define BCM53454_A0_REV_ID      1
#define BCM53456_DEVICE_ID      0x8456
#define BCM53457_DEVICE_ID      0x8457
#define BCM53456_A0_REV_ID      1

/* Bloodhound */
#define BCM53422_DEVICE_ID      0x8422
#define BCM53422_A0_REV_ID      1
#define BCM53424_DEVICE_ID      0x8424
#define BCM53424_A0_REV_ID      1
#define BCM53426_DEVICE_ID      0x8426
#define BCM53426_A0_REV_ID      1

/* Tomahawk */
#define BCM56960_DEVICE_ID      0xb960
#define BCM56960_A0_REV_ID      1
#define BCM56960_B0_REV_ID      0x11
#define BCM56960_B1_REV_ID      0x12
#define BCM56961_DEVICE_ID      0xb961
#define BCM56961_A0_REV_ID      1
#define BCM56961_B0_REV_ID      0x11
#define BCM56961_B1_REV_ID      0x12
#define BCM56962_DEVICE_ID      0xb962
#define BCM56962_A0_REV_ID      1
#define BCM56962_B0_REV_ID      0x11
#define BCM56962_B1_REV_ID      0x12
#define BCM56963_DEVICE_ID      0xb963
#define BCM56963_A0_REV_ID      1
#define BCM56963_B0_REV_ID      0x11
#define BCM56963_B1_REV_ID      0x12
#define BCM56930_DEVICE_ID      0xb930
#define BCM56930_A0_REV_ID      1
#define BCM56930_B0_REV_ID      0x11
#define BCM56930_B1_REV_ID      0x12
#define BCM56930_C0_REV_ID      0x21
#define BCM56931_DEVICE_ID      0xb931
#define BCM56931_A0_REV_ID      1
#define BCM56931_B0_REV_ID      0x11
#define BCM56931_C0_REV_ID      0x21
#define BCM56935_DEVICE_ID      0xb935
#define BCM56935_A0_REV_ID      1
#define BCM56935_B0_REV_ID      0x11
#define BCM56935_C0_REV_ID      0x21
#define BCM56936_DEVICE_ID      0xb936
#define BCM56936_A0_REV_ID      1
#define BCM56936_B0_REV_ID      0x11
#define BCM56936_C0_REV_ID      0x21
#define BCM56939_DEVICE_ID      0xb939
#define BCM56939_A0_REV_ID      1
#define BCM56939_B0_REV_ID      0x11
#define BCM56939_C0_REV_ID      0x21

/* Ranger3+ */
#define BCM56168_DEVICE_ID      0xb168
#define BCM56168_A0_REV_ID      1
#define BCM56168_B0_REV_ID      0x11
#define BCM56168_B1_REV_ID      0x12
#define BCM56169_DEVICE_ID      0xb169
#define BCM56169_A0_REV_ID      1
#define BCM56169_B0_REV_ID      0x11
#define BCM56169_B1_REV_ID      0x12

/* Tomahawk 3 */
#define BCM56980_DEVICE_ID_MASK 0xFFF0
#define BCM56980_DEVICE_ID      0xb980
#define BCM56980_A0_REV_ID      1
#define BCM56980_B0_REV_ID      0x11
#define BCM56981_DEVICE_ID      0xb981
#define BCM56981_A0_REV_ID      1
#define BCM56981_B0_REV_ID      0x11
#define BCM56982_DEVICE_ID      0xb982
#define BCM56982_A0_REV_ID      1
#define BCM56982_B0_REV_ID      0x11
#define BCM56983_DEVICE_ID      0xb983
#define BCM56983_A0_REV_ID      1
#define BCM56983_B0_REV_ID      0x11
#define BCM56984_DEVICE_ID      0xb984
#define BCM56984_A0_REV_ID      1
#define BCM56984_B0_REV_ID      0x11

/* Titanhawk */
#define BCM56968_DEVICE_ID      0xb968
#define BCM56968_A0_REV_ID      1
#define BCM56968_B0_REV_ID      0x11
#define BCM56968_B1_REV_ID      0x12

/* Hurricane 3 */
#define BCM56160_DEVICE_ID      0xb160
#define BCM56160_A0_REV_ID      1
#define BCM56160_B0_REV_ID      0x11
#define BCM56162_DEVICE_ID      0xb162
#define BCM56162_A0_REV_ID      1
#define BCM56162_B0_REV_ID      0x11

/* Hurricane3 Lite */
#define BCM56163_DEVICE_ID      0xb163
#define BCM56163_A0_REV_ID      1
#define BCM56163_B0_REV_ID      0x11
#define BCM56164_DEVICE_ID      0xb164
#define BCM56164_A0_REV_ID      1
#define BCM56164_B0_REV_ID      0x11
#define BCM56166_DEVICE_ID      0xb166
#define BCM56166_A0_REV_ID      1
#define BCM56166_B0_REV_ID      0x11

/* Hurricane4 */
#define BCM56273_DEVICE_ID      0xb273
#define BCM56273_A0_REV_ID      1
#define BCM56273_A1_REV_ID      2

#define BCM56274_DEVICE_ID      0xb274
#define BCM56274_A0_REV_ID      1
#define BCM56274_A1_REV_ID      2

#define BCM56275_DEVICE_ID      0xb275
#define BCM56275_A0_REV_ID      1
#define BCM56275_A1_REV_ID      2

#define BCM56276_DEVICE_ID      0xb276
#define BCM56276_A0_REV_ID      1
#define BCM56276_A1_REV_ID      2

#define BCM56277_DEVICE_ID      0xb277
#define BCM56277_A0_REV_ID      1
#define BCM56277_A1_REV_ID      2

#define BCM56278_DEVICE_ID      0xb278
#define BCM56278_A0_REV_ID      1
#define BCM56278_A1_REV_ID      2

#define BCM56279_DEVICE_ID      0xb279
#define BCM56279_A1_REV_ID      2

#define BCM56575_DEVICE_ID      0xb575
#define BCM56575_A1_REV_ID      2

#define BCM56175_DEVICE_ID      0xb175
#define BCM56175_A1_REV_ID      2

#define BCM56176_DEVICE_ID      0xb176
#define BCM56176_A1_REV_ID      2

/* Buckhound */
#define BCM53440_DEVICE_ID      0x8440
#define BCM53440_A0_REV_ID      1
#define BCM53440_B0_REV_ID      0x11
#define BCM53442_DEVICE_ID      0x8442
#define BCM53442_A0_REV_ID      1
#define BCM53442_B0_REV_ID      0x11
#define BCM53443_DEVICE_ID      0x8443
#define BCM53443_A0_REV_ID      1
#define BCM53443_B0_REV_ID      0x11

/* Foxhound2 */
#define BCM53434_DEVICE_ID      0x8434
#define BCM53434_A0_REV_ID      1
#define BCM53434_B0_REV_ID      0x11

/* Apache */
#define BCM56560_DEVICE_ID      0xb560
#define BCM56560_A0_REV_ID      1
#define BCM56560_B0_REV_ID      0x11

#define BCM56561_DEVICE_ID      0xb561
#define BCM56561_A0_REV_ID      1
#define BCM56561_B0_REV_ID      0x11

#define BCM56562_DEVICE_ID      0xb562
#define BCM56562_A0_REV_ID      1
#define BCM56562_B0_REV_ID      0x11

/* MONTEREY */
#define BCM56670_DEVICE_ID      0xb670
#define BCM56670_A0_REV_ID      1
#define BCM56670_B0_REV_ID      0x11
#define BCM56670_C0_REV_ID      0x21


#define BCM56671_DEVICE_ID      0xb671
#define BCM56671_A0_REV_ID      1
#define BCM56671_B0_REV_ID      0x11
#define BCM56671_C0_REV_ID      0x21

#define BCM56672_DEVICE_ID      0xb672
#define BCM56672_A0_REV_ID      1
#define BCM56672_B0_REV_ID      0x11
#define BCM56672_C0_REV_ID      0x21

#define BCM56675_DEVICE_ID      0xb675
#define BCM56675_A0_REV_ID      1
#define BCM56675_B0_REV_ID      0x11
#define BCM56675_C0_REV_ID      0x21

/* WOLFHOUND3 PLUS */
#define BCM53650_DEVICE_ID      0x8650
#define BCM53650_A0_REV_ID      1
#define BCM53650_B0_REV_ID      0x11
#define BCM53650_C0_REV_ID      0x21

#define BCM53651_DEVICE_ID      0x8651
#define BCM53651_A0_REV_ID      1
#define BCM53651_B0_REV_ID      0x11
#define BCM53651_C0_REV_ID      0x21

#define BCM53652_DEVICE_ID      0x8652
#define BCM53652_A0_REV_ID      1
#define BCM53652_B0_REV_ID      0x11
#define BCM53652_C0_REV_ID      0x21

#define BCM53653_DEVICE_ID      0x8653
#define BCM53653_A0_REV_ID      1
#define BCM53653_B0_REV_ID      0x11
#define BCM53653_C0_REV_ID      0x21

#define BCM53654_DEVICE_ID      0x8654
#define BCM53654_A0_REV_ID      1
#define BCM53654_B0_REV_ID      0x11
#define BCM53654_C0_REV_ID      0x21

/* Firebolt-5 */
#define BCM56565_DEVICE_ID      0xb565
#define BCM56565_A0_REV_ID      1
#define BCM56565_B0_REV_ID      0x11

#define BCM56566_DEVICE_ID      0xb566
#define BCM56566_A0_REV_ID      1
#define BCM56566_B0_REV_ID      0x11

#define BCM56567_DEVICE_ID      0xb567
#define BCM56567_A0_REV_ID      1
#define BCM56567_B0_REV_ID      0x11

#define BCM56568_DEVICE_ID      0xb568
#define BCM56568_A0_REV_ID      1
#define BCM56568_B0_REV_ID      0x11

/* Maverick */
#define BCM56760_DEVICE_ID      0xb760
#define BCM56760_A0_REV_ID      1
#define BCM56760_A1_REV_ID      2
#define BCM56760_B0_REV_ID      0x11


#define BCM56761_DEVICE_ID      0xb761
#define BCM56761_A0_REV_ID      1
#define BCM56761_A1_REV_ID      2
#define BCM56761_B0_REV_ID      0x11
#define BCM56761_B1_REV_ID      0x12

#define BCM56762_DEVICE_ID      0xb762
#define BCM56762_A0_REV_ID      1
#define BCM56762_B0_REV_ID      0x11

#define BCM56764_DEVICE_ID      0xb764
#define BCM56764_A0_REV_ID      1
#define BCM56764_B0_REV_ID      0x11

#define BCM56765_DEVICE_ID      0xb765
#define BCM56765_A0_REV_ID      1
#define BCM56765_B0_REV_ID      0x11

#define BCM56766_DEVICE_ID      0xb766
#define BCM56766_A0_REV_ID      1
#define BCM56766_B0_REV_ID      0x11

#define BCM56768_DEVICE_ID      0xb768
#define BCM56768_A0_REV_ID      1
#define BCM56768_B0_REV_ID      0x11

/* Ranger 2+ */
#define BCM56068_DEVICE_ID      0xb068
#define BCM56068_A0_REV_ID      1
#define BCM56068_B0_REV_ID      0x11
#define BCM56068_B1_REV_ID      0x12

#define BCM56069_DEVICE_ID      0xb069
#define BCM56069_A0_REV_ID      1
#define BCM56069_B0_REV_ID      0x11

/* Hurricane3-MG */
#define BCM56170_DEVICE_ID      0xb170
#define BCM56170_A0_REV_ID      1
#define BCM56170_B0_REV_ID      0x11
#define BCM56172_DEVICE_ID      0xb172
#define BCM56172_A0_REV_ID      1
#define BCM56172_B0_REV_ID      0x11
#define BCM56174_DEVICE_ID      0xb174
#define BCM56174_A0_REV_ID      1
#define BCM56174_B0_REV_ID      0x11

/* Quartz, Greyhound 2 */
#define BCM53570_DEVICE_ID      0x8570
#define BCM53570_A0_REV_ID      1
#define BCM53570_B0_REV_ID      0x11
#define BCM53575_DEVICE_ID      0x8575
#define BCM53575_A0_REV_ID      1
#define BCM53575_B0_REV_ID      0x11

/* FireLight */
#define BCM56070_DEVICE_ID      0xb070
#define BCM56070_A0_REV_ID      1
#define BCM56071_DEVICE_ID      0xb071
#define BCM56071_A0_REV_ID      1
#define BCM56072_DEVICE_ID      0xb072
#define BCM56072_A0_REV_ID      1


/* Tomahawk+ */
#define BCM56965_DEVICE_ID      0xb965
#define BCM56965_A0_REV_ID      1
#define BCM56965_A1_REV_ID      2
#define BCM56969_DEVICE_ID      0xb969        /* Corresponds to TH 56961 */
#define BCM56969_A0_REV_ID      1
#define BCM56966_DEVICE_ID      0xb966        /* Corresponds to TH 56962 */
#define BCM56966_A0_REV_ID      1
#define BCM56967_DEVICE_ID      0xb967        /* Corresponds to TH 56963 */
#define BCM56967_A0_REV_ID      1

/* Tomahawk 2 */
#define BCM56970_DEVICE_ID      0xb970
#define BCM56970_A0_REV_ID      1
#define BCM56970_B0_REV_ID      0x11
#define BCM56971_DEVICE_ID      0xb971
#define BCM56971_A0_REV_ID      1
#define BCM56971_B0_REV_ID      0x11
#define BCM56972_DEVICE_ID      0xb972
#define BCM56972_A0_REV_ID      1
#define BCM56972_B0_REV_ID      0x11
#define BCM56974_DEVICE_ID      0xb974
#define BCM56974_A0_REV_ID      1
#define BCM56974_B0_REV_ID      0x11
#define BCM56975_DEVICE_ID      0xb975
#define BCM56975_A0_REV_ID      1
#define BCM56975_B0_REV_ID      0x11

/* Trident3 */
#define BCM56870_DEVICE_ID      0xb870
#define BCM56870_A0_REV_ID      1
#define BCM56873_DEVICE_ID      0xb873
#define BCM56873_A0_REV_ID      1

/* Helix5 */
#define BCM56370_DEVICE_ID      0xb370
#define BCM56370_A0_REV_ID      1
#define BCM56370_A1_REV_ID      0x02
#define BCM56370_A2_REV_ID      0x03

#define BCM56371_DEVICE_ID      0xb371
#define BCM56371_A0_REV_ID      1
#define BCM56371_A1_REV_ID      0x02
#define BCM56371_A2_REV_ID      0x03

#define BCM56372_DEVICE_ID      0xb372
#define BCM56372_A0_REV_ID      1
#define BCM56372_A1_REV_ID      0x02
#define BCM56372_A2_REV_ID      0x03

#define BCM56374_DEVICE_ID      0xb374
#define BCM56374_A0_REV_ID      1
#define BCM56374_A1_REV_ID      0x02
#define BCM56374_A2_REV_ID      0x03

#define BCM56375_DEVICE_ID      0xb375
#define BCM56375_A0_REV_ID      1
#define BCM56375_A1_REV_ID      0x02
#define BCM56375_A2_REV_ID      0x03

#define BCM56376_DEVICE_ID      0xb376
#define BCM56376_A0_REV_ID      1
#define BCM56376_A1_REV_ID      0x02
#define BCM56376_A2_REV_ID      0x03

#define BCM56377_DEVICE_ID      0xb377
#define BCM56377_A0_REV_ID      1
#define BCM56377_A1_REV_ID      0x02
#define BCM56377_A2_REV_ID      0x03

#define BCM56577_DEVICE_ID      0xb577
#define BCM56577_A0_REV_ID      1
#define BCM56577_A1_REV_ID      0x02
#define BCM56577_A2_REV_ID      0x03

#define BCM56578_DEVICE_ID      0xb578
#define BCM56578_A0_REV_ID      1
#define BCM56578_A1_REV_ID      0x02
#define BCM56578_A2_REV_ID      0x03

#define BCM56579_DEVICE_ID      0xb579
#define BCM56579_A0_REV_ID      1
#define BCM56579_A1_REV_ID      0x02
#define BCM56579_A2_REV_ID      0x03

/* Maverick2 */
#define BCM56770_DEVICE_ID      0xb770
#define BCM56770_A0_REV_ID      1

#define BCM56771_DEVICE_ID      0xb771
#define BCM56771_A0_REV_ID      1

/* Firebolt6 */
#define BCM56470_DEVICE_ID      0xb470
#define BCM56470_A0_REV_ID      1
#define BCM56471_DEVICE_ID      0xb471
#define BCM56471_A0_REV_ID      1
#define BCM56472_DEVICE_ID      0xb472
#define BCM56472_A0_REV_ID      1
#define BCM56475_DEVICE_ID      0xb475
#define BCM56475_A0_REV_ID      1
#define BCM56474_DEVICE_ID      0xb474
#define BCM56474_A0_REV_ID      1


/* Wolfhound2 (Hurricane3-CR)*/
#define BCM53540_DEVICE_ID      0x8540
#define BCM53540_A0_REV_ID      1
#define BCM53547_DEVICE_ID      0x8547
#define BCM53547_A0_REV_ID      1
#define BCM53548_DEVICE_ID      0x8548
#define BCM53548_A0_REV_ID      1
#define BCM53549_DEVICE_ID      0x8549
#define BCM53549_A0_REV_ID      1

/* Wolfhound3 */
#define BCM53642_DEVICE_ID      0x8642
#define BCM53642_A0_REV_ID      1

/*
 * BCM5665: Tucana48 (48+4+1)
 * BCM5665L: Tucana24 (24+4+1)
 * BCM5666: Tucana48 (48+4+1) without L3
 * BCM5666L: Tucana24 (24+4+1) without L3
 *
 * The device ID is 0x5665 for all of these parts.  For BCM5665L and
 * BCM5666L, the pbmp_valid property must be set to invalidate fe24-fe47
 * (see $SDK/rc/config.bcm).
 */
#define BCM5665_DEVICE_ID       0x5665
#define BCM5665_A0_REV_ID       1
#define BCM5665_B0_REV_ID       0x11


/*
 * BCM5655: Titanium48 (48+4)
 * BCM5656: Titanium48 (48+4) without L3
 *
 * The device ID is 0x5655 for both parts.
 */
#define BCM5655_DEVICE_ID       0x5655
#define BCM5655_A0_REV_ID       1
#define BCM5655_B0_REV_ID       0x11


/*
 * BCM5650: Titanium-II (24+4)
 * BCM5651: Titanium-II (24+4) without L3
 *
 * The device ID is 0x5650 for both parts.
 * BCM5650C0 is the first spin of a real 24+4 cost-reduced chip.
 */
#define BCM5650_DEVICE_ID       0x5650
#define BCM5650_A0_REV_ID       1
#define BCM5650_B0_REV_ID       0x11
#define BCM5650_C0_REV_ID       0x21

#define BROADCOM_PHYID_HIGH 0x0040

/* Define Dune device IDs */
#define ARAD_DEVICE_ID          0x8650
#define ARAD_A0_REV_ID          0x0000
#define ARAD_B0_REV_ID          0x0011
#define ARAD_B1_REV_ID          0x0012
#define BCM88770_DEVICE_ID      0x8770
#define BCM88770_A1_REV_ID      0x0002
#define BCM88773_DEVICE_ID      0x8773
#define BCM88773_A1_REV_ID      0x0002
#define BCM88774_DEVICE_ID      0x8774
#define BCM88774_A1_REV_ID      0x0002
#define BCM88775_DEVICE_ID      0x8775
#define BCM88775_A1_REV_ID      0x0002
#define BCM88776_DEVICE_ID      0x8776
#define BCM88776_A1_REV_ID      0x0002
#define BCM88777_DEVICE_ID      0x8777
#define BCM88777_A1_REV_ID      0x0002
#define BCM88950_DEVICE_ID      0x8950
#define BCM88950_A0_REV_ID      0x0001
#define BCM88950_A1_REV_ID      0x0002
#define BCM88953_DEVICE_ID      0x8953
#define BCM88953_A1_REV_ID      0x0002
#define BCM88954_DEVICE_ID      0x8954
#define BCM88954_A1_REV_ID      0x0002
#define BCM88955_DEVICE_ID      0x8955
#define BCM88955_A1_REV_ID      0x0002
#define BCM88956_DEVICE_ID      0x8956
#define BCM88956_A1_REV_ID      0x0002
#define DNXC_A0_REV_ID          0x0001
#define DNXC_A1_REV_ID          0x0002
#define DNXC_A2_REV_ID          0x0003
#define DNXC_B0_REV_ID          0x0011
#define DNXC_B1_REV_ID          0x0012
#define DNXC_B2_REV_ID          0x0013
#define DNXC_C0_REV_ID          0x0021
#define DNXC_C1_REV_ID          0x0022
#define DNXC_C2_REV_ID          0x0023
#define DNXC_D0_REV_ID          0x0031
#define DNXC_D1_REV_ID          0x0032
#define DNXC_DEVID_FAMILY_MASK  0xfff0
#define BCM88790_DEVICE_ID      0x8790
#define BCM88790_A0_REV_ID      DNXC_A0_REV_ID
#define BCM88790_B0_REV_ID      DNXC_B0_REV_ID
#define BCM88791_DEVICE_ID      0x8791
#define BCM88792_DEVICE_ID      0x8792
#define BCM88793_DEVICE_ID      0x8793
#define BCM88794_DEVICE_ID      0x8794
#define BCM88795_DEVICE_ID      0x8795
#define BCM88796_DEVICE_ID      0x8796
#define BCM88797_DEVICE_ID      0x8797
#define BCM88798_DEVICE_ID      0x8798
#define BCM88799_DEVICE_ID      0x8799
#define BCM8879A_DEVICE_ID      0x879A
#define BCM8879B_DEVICE_ID      0x879B
#define BCM8879C_DEVICE_ID      0x879C
#define BCM8879D_DEVICE_ID      0x879D
#define BCM8879E_DEVICE_ID      0x879E
#define BCM8879F_DEVICE_ID      0x879F  
#ifdef BCM_DNXF3_SUPPORT
#define RAMON2_DEVICE_ID      0x8910
#define BCM8891F_DEVICE_ID      0x891F
#define RAMON3_DEVICE_ID      0x8920
#endif
#define ARADPLUS_DEVICE_ID      0x8660
#define ARADPLUS_A0_REV_ID      0x0001
#define BCM88660_DEVICE_ID      ARADPLUS_DEVICE_ID
#define BCM88660_A0_REV_ID      ARADPLUS_A0_REV_ID
/* Jericho */
#define JERICHO_DEVICE_ID       0x8675
#define JERICHO_A0_REV_ID       0x0001
#define JERICHO_B0_REV_ID       0x0011
#define JERICHO_A1_REV_ID       0x0002
/* Jericho SKUs */
#define BCM88670_DEVICE_ID      0x8670
#define BCM88670_A0_REV_ID      JERICHO_A0_REV_ID
#define BCM88670_A1_REV_ID      JERICHO_A1_REV_ID
#define BCM88671_DEVICE_ID      0x8671
#define BCM88671_A0_REV_ID      JERICHO_A0_REV_ID
#define BCM88671_A1_REV_ID      JERICHO_A1_REV_ID
#define BCM88671M_DEVICE_ID     0x867A
#define BCM88671M_A0_REV_ID     JERICHO_A0_REV_ID
#define BCM88671M_A1_REV_ID     JERICHO_A1_REV_ID

#define BCM88670_B0_REV_ID      JERICHO_B0_REV_ID
#define BCM88671_B0_REV_ID      JERICHO_B0_REV_ID
#define BCM88671M_B0_REV_ID     JERICHO_B0_REV_ID

#define BCM88672_DEVICE_ID      0x8672
#define BCM88672_A0_REV_ID      JERICHO_A0_REV_ID
#define BCM88672_A1_REV_ID      JERICHO_A1_REV_ID

#define BCM88672_B0_REV_ID      JERICHO_B0_REV_ID

#define BCM88673_DEVICE_ID      0x8673
#define BCM88673_A0_REV_ID      JERICHO_A0_REV_ID
#define BCM88673_A1_REV_ID      JERICHO_A1_REV_ID
#define BCM88674_DEVICE_ID      0x8674
#define BCM88674_A0_REV_ID      JERICHO_A0_REV_ID
#define BCM88674_A1_REV_ID      JERICHO_A1_REV_ID
#define BCM88675_DEVICE_ID      JERICHO_DEVICE_ID
#define BCM88675_A0_REV_ID      JERICHO_A0_REV_ID
#define BCM88675_A1_REV_ID      JERICHO_A1_REV_ID
#define BCM88675M_DEVICE_ID     0x867B
#define BCM88675M_A0_REV_ID     JERICHO_A0_REV_ID
#define BCM88675M_A1_REV_ID     JERICHO_A1_REV_ID
#define BCM88676_DEVICE_ID      0x8676
#define BCM88676_A0_REV_ID      JERICHO_A0_REV_ID
#define BCM88676_A1_REV_ID      JERICHO_A1_REV_ID
#define BCM88676M_DEVICE_ID     0x867C
#define BCM88676M_A0_REV_ID     JERICHO_A0_REV_ID
#define BCM88676M_A1_REV_ID     JERICHO_A1_REV_ID
#define BCM88677_DEVICE_ID      0x8677
#define BCM88677_A0_REV_ID      JERICHO_A0_REV_ID
#define BCM88677_A1_REV_ID      JERICHO_A1_REV_ID
#define BCM88678_DEVICE_ID      0x8678
#define BCM88678_A0_REV_ID      JERICHO_A0_REV_ID
#define BCM88678_A1_REV_ID      JERICHO_A1_REV_ID
#define BCM88679_DEVICE_ID      0x8679
#define BCM88679_A0_REV_ID      JERICHO_A0_REV_ID
#define BCM88679_A1_REV_ID      JERICHO_A1_REV_ID

#define BCM88673_B0_REV_ID      JERICHO_B0_REV_ID
#define BCM88674_B0_REV_ID      JERICHO_B0_REV_ID
#define BCM88675_B0_REV_ID      JERICHO_B0_REV_ID
#define BCM88675M_B0_REV_ID     JERICHO_B0_REV_ID
#define BCM88676_B0_REV_ID      JERICHO_B0_REV_ID
#define BCM88676M_B0_REV_ID     JERICHO_B0_REV_ID
#define BCM88677_B0_REV_ID      JERICHO_B0_REV_ID
#define BCM88678_B0_REV_ID      JERICHO_B0_REV_ID
#define BCM88679_B0_REV_ID      JERICHO_B0_REV_ID
/* Qumran MX */
#define QMX_DEVICE_ID           0x8375
#define QMX_A0_REV_ID           0x0001
#define QMX_B0_REV_ID           0x0011
#define QMX_A1_REV_ID           0x0002
/* Qumran MX SKUs*/
#define BCM88370_DEVICE_ID      0x8370
#define BCM88370_A0_REV_ID      QMX_A0_REV_ID
#define BCM88370_A1_REV_ID      QMX_A1_REV_ID
#define BCM88371_DEVICE_ID      0x8371
#define BCM88371_A0_REV_ID      QMX_A0_REV_ID
#define BCM88371_A1_REV_ID      QMX_A1_REV_ID
#define BCM88371M_DEVICE_ID     0x837A
#define BCM88371M_A0_REV_ID     QMX_A0_REV_ID
#define BCM88371M_A1_REV_ID     QMX_A1_REV_ID
#define BCM88375_DEVICE_ID      QMX_DEVICE_ID
#define BCM88375_A0_REV_ID      QMX_A0_REV_ID
#define BCM88375_A1_REV_ID      QMX_A1_REV_ID
#define BCM88376_DEVICE_ID      0x8376
#define BCM88376_A0_REV_ID      QMX_A0_REV_ID
#define BCM88376_A1_REV_ID      QMX_A1_REV_ID
#define BCM88376M_DEVICE_ID     0x837B
#define BCM88376M_A0_REV_ID     QMX_A0_REV_ID
#define BCM88376M_A1_REV_ID     QMX_A1_REV_ID
#define BCM88377_DEVICE_ID      0x8377
#define BCM88377_A0_REV_ID      QMX_A0_REV_ID
#define BCM88377_A1_REV_ID      QMX_A1_REV_ID
#define BCM88378_DEVICE_ID      0x8378
#define BCM88378_A0_REV_ID      QMX_A0_REV_ID
#define BCM88378_A1_REV_ID      QMX_A1_REV_ID
#define BCM88379_DEVICE_ID      0x8379
#define BCM88379_A0_REV_ID      QMX_A0_REV_ID
#define BCM88379_A1_REV_ID      QMX_A1_REV_ID

#define BCM88370_B0_REV_ID      QMX_B0_REV_ID
#define BCM88371_B0_REV_ID      QMX_B0_REV_ID
#define BCM88371M_B0_REV_ID     QMX_B0_REV_ID
#define BCM88375_B0_REV_ID      QMX_B0_REV_ID
#define BCM88376_B0_REV_ID      QMX_B0_REV_ID
#define BCM88376M_B0_REV_ID     QMX_B0_REV_ID
#define BCM88377_B0_REV_ID      QMX_B0_REV_ID
#define BCM88378_B0_REV_ID      QMX_B0_REV_ID
#define BCM88379_B0_REV_ID      QMX_B0_REV_ID


#define JERICHO_PLUS_DEVICE_ID           0x8680
#define JERICHO_PLUS_A0_REV_ID           0x0001
#define BCM88680_DEVICE_ID      JERICHO_PLUS_DEVICE_ID
#define BCM88680_A0_REV_ID      JERICHO_PLUS_A0_REV_ID
#define BCM88680_A1_REV_ID               0x0002


#define BCM88681_DEVICE_ID               0x8681
#define BCM88681_A0_REV_ID      JERICHO_PLUS_A0_REV_ID

#define BCM88682_DEVICE_ID               0x8682
#define BCM88682_A0_REV_ID      JERICHO_PLUS_A0_REV_ID

#define BCM88683_DEVICE_ID               0x8683
#define BCM88683_A0_REV_ID      JERICHO_PLUS_A0_REV_ID

#define BCM88684_DEVICE_ID               0x8684
#define BCM88684_A0_REV_ID      JERICHO_PLUS_A0_REV_ID

#define BCM88685_DEVICE_ID               0x8685
#define BCM88685_A0_REV_ID      JERICHO_PLUS_A0_REV_ID

#define BCM88687_DEVICE_ID               0x8687
#define BCM88687_A0_REV_ID      JERICHO_PLUS_A0_REV_ID

#define BCM88380_DEVICE_ID      0x8380
#define BCM88380_A0_REV_ID      JERICHO_PLUS_A0_REV_ID
#define BCM88381_DEVICE_ID      0x8381
#define BCM88381_A0_REV_ID      JERICHO_PLUS_A0_REV_ID

#define JERICHO2_DEVICE_ID           0x8690
#define JERICHO2_A0_REV_ID     DNXC_A0_REV_ID
#define JERICHO2_B0_REV_ID     DNXC_B0_REV_ID
#define JERICHO2_B1_REV_ID     DNXC_B1_REV_ID
#define BCM88690_DEVICE_ID      JERICHO2_DEVICE_ID
#define BCM88690_A0_REV_ID      JERICHO2_A0_REV_ID
#define BCM88690_B0_REV_ID      JERICHO2_B0_REV_ID
#define BCM88690_B1_REV_ID      JERICHO2_B1_REV_ID
#define BCM88691_DEVICE_ID      0x8691
#define BCM88692_DEVICE_ID      0x8692
#define BCM88693_DEVICE_ID      0x8693
#define BCM88694_DEVICE_ID      0x8694
#define BCM88695_DEVICE_ID      0x8695
#define BCM88696_DEVICE_ID      0x8696
#define BCM88697_DEVICE_ID      0x8697
#define BCM88698_DEVICE_ID      0x8698
#define BCM88699_DEVICE_ID      0x8699
#define BCM8869A_DEVICE_ID      0x869A
#define BCM8869B_DEVICE_ID      0x869B
#define BCM8869C_DEVICE_ID      0x869C
#define BCM8869D_DEVICE_ID      0x869D
#define BCM8869E_DEVICE_ID      0x869E
#define BCM8869F_DEVICE_ID      0x869F

#define J2C_DEVICE_ID           0x8800
#define J2C_2ND_DEVICE_ID       0x8820
#define J2C_DEVID_FAMILY_MASK   0xffd0
#define J2C_A0_REV_ID           DNXC_A0_REV_ID
#define J2C_A1_REV_ID           DNXC_A1_REV_ID
#define BCM88800_DEVICE_ID      J2C_DEVICE_ID
#define BCM88820_DEVICE_ID      J2C_2ND_DEVICE_ID
#define BCM88800_A0_REV_ID      J2C_A0_REV_ID
#define BCM88800_A1_REV_ID      J2C_A1_REV_ID
#define BCM88801_DEVICE_ID      0x8801
#define BCM88802_DEVICE_ID      0x8802
#define BCM88803_DEVICE_ID      0x8803
#define BCM88804_DEVICE_ID      0x8804
#define BCM88805_DEVICE_ID      0x8805
#define BCM88806_DEVICE_ID      0x8806
#define BCM88807_DEVICE_ID      0x8807
#define BCM88808_DEVICE_ID      0x8808
#define BCM88809_DEVICE_ID      0x8809
#define BCM8880A_DEVICE_ID      0x880A
#define BCM8880B_DEVICE_ID      0x880B
#define BCM8880C_DEVICE_ID      0x880C
#define BCM8880D_DEVICE_ID      0x880D
#define BCM8880E_DEVICE_ID      0x880E
#define BCM8880F_DEVICE_ID      0x880F
#define BCM88821_DEVICE_ID      0x8821
#define BCM88822_DEVICE_ID      0x8822
#define BCM88823_DEVICE_ID      0x8823
#define BCM88824_DEVICE_ID      0x8824
#define BCM88825_DEVICE_ID      0x8825
#define BCM88826_DEVICE_ID      0x8826
#define BCM88827_DEVICE_ID      0x8827
#define BCM88828_DEVICE_ID      0x8828
#define BCM88829_DEVICE_ID      0x8829
#define BCM8882A_DEVICE_ID      0x882A
#define BCM8882B_DEVICE_ID      0x882B
#define BCM8882C_DEVICE_ID      0x882C
#define BCM8882D_DEVICE_ID      0x882D
#define BCM8882E_DEVICE_ID      0x882E
#define BCM8882F_DEVICE_ID      0x882F

#define J2P_DEVICE_ID                 0x8850
#define J2P_A0_REV_ID           DNXC_A0_REV_ID
#define J2P_A1_REV_ID           DNXC_A1_REV_ID
#define J2P_A2_REV_ID           DNXC_A2_REV_ID
#define BCM88850_DEVICE_ID      J2P_DEVICE_ID
#define BCM88850_A0_REV_ID      J2P_A0_REV_ID
#define BCM88850_A1_REV_ID      J2P_A1_REV_ID
#define BCM88850_A2_REV_ID      J2P_A2_REV_ID
#define BCM88851_DEVICE_ID      0x8851
#define BCM88852_DEVICE_ID      0x8852
#define BCM88853_DEVICE_ID      0x8853
#define BCM88854_DEVICE_ID      0x8854
#define BCM88855_DEVICE_ID      0x8855
#define BCM88856_DEVICE_ID      0x8856
#define BCM88857_DEVICE_ID      0x8857
#define BCM88858_DEVICE_ID      0x8858
#define BCM88859_DEVICE_ID      0x8859
#define BCM8885A_DEVICE_ID      0x885A
#define BCM8885B_DEVICE_ID      0x885B
#define BCM8885C_DEVICE_ID      0x885C
#define BCM8885D_DEVICE_ID      0x885D
#define BCM8885E_DEVICE_ID      0x885E
#define BCM8885F_DEVICE_ID      0x885F

#define BCM88840_DEVICE_ID      0x8840
#define BCM88841_DEVICE_ID      0x8841
#define BCM88842_DEVICE_ID      0x8842
#define BCM88843_DEVICE_ID      0x8843
#define BCM88844_DEVICE_ID      0x8844
#define BCM88845_DEVICE_ID      0x8845
#define BCM88846_DEVICE_ID      0x8846
#define BCM88847_DEVICE_ID      0x8847
#define BCM88848_DEVICE_ID      0x8848
#define BCM88849_DEVICE_ID      0x8849
#define BCM8884A_DEVICE_ID      0x884A
#define BCM8884B_DEVICE_ID      0x884B
#define BCM8884C_DEVICE_ID      0x884C
#define BCM8884D_DEVICE_ID      0x884D
#define BCM8884E_DEVICE_ID      0x884E
#define BCM8884F_DEVICE_ID      0x884F

#define BCM88810_DEVICE_ID      0x8810
#define BCM88811_DEVICE_ID      0x8811
#define BCM88812_DEVICE_ID      0x8812
#define BCM88813_DEVICE_ID      0x8813
#define BCM88814_DEVICE_ID      0x8814
#define BCM88815_DEVICE_ID      0x8815
#define BCM88816_DEVICE_ID      0x8816
#define BCM88817_DEVICE_ID      0x8817
#define BCM88818_DEVICE_ID      0x8818
#define BCM88819_DEVICE_ID      0x8819
#define BCM8881A_DEVICE_ID      0x881A
#define BCM8881B_DEVICE_ID      0x881B
#define BCM8881C_DEVICE_ID      0x881C
#define BCM8881D_DEVICE_ID      0x881D
#define BCM8881E_DEVICE_ID      0x881E
#define BCM8881F_DEVICE_ID      0x881F

#define J2X_DEVICE_ID                 0x8830
#define J2X_A0_REV_ID           DNXC_A0_REV_ID
#define BCM88830_DEVICE_ID      J2X_DEVICE_ID
#define BCM88830_A0_REV_ID      J2X_A0_REV_ID
#define BCM88831_DEVICE_ID      0x8831
#define BCM88832_DEVICE_ID      0x8832
#define BCM88833_DEVICE_ID      0x8833
#define BCM88834_DEVICE_ID      0x8834
#define BCM88835_DEVICE_ID      0x8835
#define BCM88836_DEVICE_ID      0x8836
#define BCM88837_DEVICE_ID      0x8837
#define BCM88838_DEVICE_ID      0x8838
#define BCM88839_DEVICE_ID      0x8839
#define BCM8883A_DEVICE_ID      0x883A
#define BCM8883B_DEVICE_ID      0x883B
#define BCM8883C_DEVICE_ID      0x883C
#define BCM8883D_DEVICE_ID      0x883D
#define BCM8883E_DEVICE_ID      0x883E
#define BCM8883F_DEVICE_ID      0x883F

#define J2Z_DEVICE_ID           0x216
#define J2Z_A0_REV_ID           DNXC_A0_REV_ID
#define BCM8216_DEVICE_ID       J2Z_DEVICE_ID
#define BCM8216_A0_REV_ID       J2Z_A0_REV_ID

#ifdef BCM_DNX3_SUPPORT
#define JERICHO3_DEVICE_ID      0x8860
#define Q3_DEVICE_ID_START      0x886a
#define Q3_DEVICE_ID_END        0x886f
#define J3AI_DEVICE_ID           0x8890
#define Q3D_DEVICE_ID           0x8870
#ifdef BCM_Q3A_SUPPORT
#define Q3A_DEVICE_ID           0x8490
#define Q3U_DEVICE_ID           0x84A0
#define Q3N_DEVICE_ID           0x84A5
#define Q3U_ORIG_DEVICE_ID      0x8400
#define Q3N_ORIG_DEVICE_ID      0x8405
#endif
#endif
#define Q2A_DEVICE_ID           0x8480
#define Q2A_A0_REV_ID           DNXC_A0_REV_ID
#define Q2A_B0_REV_ID           DNXC_B0_REV_ID
#define Q2A_B1_REV_ID           DNXC_B1_REV_ID
#define BCM88480_DEVICE_ID      Q2A_DEVICE_ID
#define BCM88480_A0_REV_ID      Q2A_A0_REV_ID
#define BCM88480_B0_REV_ID      Q2A_B0_REV_ID
#define BCM88480_B1_REV_ID      Q2A_B1_REV_ID
#define BCM88481_DEVICE_ID      0x8481
#define BCM88482_DEVICE_ID      0x8482
#define BCM88483_DEVICE_ID      0x8483
#define BCM88484_DEVICE_ID      0x8484
#define BCM88485_DEVICE_ID      0x8485
#define BCM88486_DEVICE_ID      0x8486
#define BCM88487_DEVICE_ID      0x8487
#define BCM88488_DEVICE_ID      0x8488
#define BCM88489_DEVICE_ID      0x8489
#define BCM8848A_DEVICE_ID      0x848A
#define BCM8848B_DEVICE_ID      0x848B
#define BCM8848C_DEVICE_ID      0x848C
#define BCM8848D_DEVICE_ID      0x848D
#define BCM8848E_DEVICE_ID      0x848E
#define BCM8848F_DEVICE_ID      0x848F

#define Q2U_DEVICE_ID           0x8280
#define BCM88280_DEVICE_ID      Q2U_DEVICE_ID
#define BCM88281_DEVICE_ID      0x8281
#define BCM88282_DEVICE_ID      0x8282
#define BCM88283_DEVICE_ID      0x8283
#define BCM88284_DEVICE_ID      0x8284
#define BCM88285_DEVICE_ID      0x8285
#define BCM88286_DEVICE_ID      0x8286
#define BCM88287_DEVICE_ID      0x8287
#define BCM88288_DEVICE_ID      0x8288
#define BCM88289_DEVICE_ID      0x8289
#define BCM8828A_DEVICE_ID      0x828A
#define BCM8828B_DEVICE_ID      0x828B
#define BCM8828C_DEVICE_ID      0x828C
#define BCM8828D_DEVICE_ID      0x828D
#define BCM8828E_DEVICE_ID      0x828E
#define BCM8828F_DEVICE_ID      0x828F

#define Q2N_DEVICE_ID           0x8290
#define BCM88290_DEVICE_ID      Q2N_DEVICE_ID
#define BCM88291_DEVICE_ID      0x8291
#define BCM88292_DEVICE_ID      0x8292
#define BCM88293_DEVICE_ID      0x8293
#define BCM88294_DEVICE_ID      0x8294
#define BCM88295_DEVICE_ID      0x8295
#define BCM88296_DEVICE_ID      0x8296
#define BCM88297_DEVICE_ID      0x8297
#define BCM88298_DEVICE_ID      0x8298
#define BCM88299_DEVICE_ID      0x8299
#define BCM8829A_DEVICE_ID      0x829A
#define BCM8829B_DEVICE_ID      0x829B
#define BCM8829C_DEVICE_ID      0x829C
#define BCM8829D_DEVICE_ID      0x829D
#define BCM8829E_DEVICE_ID      0x829E
#define BCM8829F_DEVICE_ID      0x829F

#define QAX_DEVICE_ID           0x8470
#define QAX_A0_REV_ID           0x0001
#define QAX_B0_REV_ID           0x0011
#define BCM88470_DEVICE_ID      QAX_DEVICE_ID
#define BCM88470_B0_REV_ID      QAX_B0_REV_ID
#define BCM88470P_DEVICE_ID     0x847C
#define BCM88471_DEVICE_ID      0x8471
#define BCM88473_DEVICE_ID      0x8473
#define BCM88474_DEVICE_ID      0x8474
#define BCM88474H_DEVICE_ID     0x847B
#define BCM88476_DEVICE_ID      0x8476
#define BCM88477_DEVICE_ID      0x8477

#define BCM88479_DEVICE_ID      0x8479



#define BCM88470_A0_REV_ID      QAX_A0_REV_ID

#define QUX_DEVICE_ID           0x8270
#define QUX_A0_REV_ID           0x0001
#define QUX_A1_REV_ID           0x0002
#define QUX_B0_REV_ID           0x0011
#define BCM88270_DEVICE_ID      QUX_DEVICE_ID
#define BCM88270_A0_REV_ID      QUX_A0_REV_ID
#define BCM88270_A1_REV_ID      QUX_A1_REV_ID
#define BCM88271_DEVICE_ID      0x8271
#define BCM88272_DEVICE_ID      0x8272
#define BCM88273_DEVICE_ID      0x8273
#define BCM88274_DEVICE_ID      0x8274
#define BCM88276_DEVICE_ID      0x8276
#define BCM88278_DEVICE_ID      0x8278
#define BCM88279_DEVICE_ID      0x8279

#define BCM88360_DEVICE_ID      0x8360
#define BCM88360_A0_REV_ID      ARADPLUS_A0_REV_ID
#define BCM88361_DEVICE_ID      0x8361
#define BCM88361_A0_REV_ID      ARADPLUS_A0_REV_ID
#define BCM88363_DEVICE_ID      0x8363
#define BCM88363_A0_REV_ID      ARADPLUS_A0_REV_ID
#define BCM88460_DEVICE_ID      0x8460
#define BCM88460_A0_REV_ID      ARADPLUS_A0_REV_ID
#define BCM88461_DEVICE_ID      0x8461
#define BCM88461_A0_REV_ID      ARADPLUS_A0_REV_ID
#define BCM88560_DEVICE_ID      0x8560
#define BCM88560_A0_REV_ID      ARADPLUS_A0_REV_ID
#define BCM88561_DEVICE_ID      0x8561
#define BCM88561_A0_REV_ID      ARADPLUS_A0_REV_ID
#define BCM88562_DEVICE_ID      0x8562
#define BCM88562_A0_REV_ID      ARADPLUS_A0_REV_ID
#define BCM88661_DEVICE_ID      0x8661
#define BCM88661_A0_REV_ID      ARADPLUS_A0_REV_ID
#define BCM88664_DEVICE_ID      0x8664
#define BCM88664_A0_REV_ID      ARADPLUS_A0_REV_ID


#define BCM88350_DEVICE_ID      0x8350
#define BCM88350_B1_REV_ID      ARAD_B1_REV_ID
#define BCM88351_DEVICE_ID      0x8351
#define BCM88351_B1_REV_ID      ARAD_B1_REV_ID
#define BCM88450_DEVICE_ID      0x8450
#define BCM88450_B1_REV_ID      ARAD_B1_REV_ID
#define BCM88451_DEVICE_ID      0x8451
#define BCM88451_B1_REV_ID      ARAD_B1_REV_ID
#define BCM88550_DEVICE_ID      0x8550
#define BCM88550_B1_REV_ID      ARAD_B0_REV_ID
#define BCM88551_DEVICE_ID      0x8551
#define BCM88551_B1_REV_ID      ARAD_B1_REV_ID
#define BCM88552_DEVICE_ID      0x8552
#define BCM88552_B1_REV_ID      ARAD_B1_REV_ID

#define BCM88772_DEVICE_ID		0x8772
#define BCM88952_DEVICE_ID		0x8952
#define BCM88772_A1_REV_ID		0x0002
#define BCM88952_A0_REV_ID		0x0001
#define BCM88952_A1_REV_ID		0x0002

#define PLX9056_DEVICE_ID       0x9056 /* needed for DNX_TEST_BOARD */

/* Trident4 X11c */
#define BCM56890_DEVICE_ID 0xb890
#define BCM56890_A0_REV_ID 0x0001
#define BCM56891_DEVICE_ID 0xb891
#define BCM56891_A0_REV_ID 0x0001
#define BCM56892_DEVICE_ID 0xb892
#define BCM56892_A0_REV_ID 0x0001
#define BCM56895_DEVICE_ID 0xb895
#define BCM56895_A0_REV_ID 0x0001
#define BCM56896_DEVICE_ID 0xb896
#define BCM56896_A0_REV_ID 0x0001

/* Trident4 X7 */
#define BCM56690_DEVICE_ID 0xb690
#define BCM56690_A0_REV_ID 0x0001
#define BCM56692_DEVICE_ID 0xb692
#define BCM56692_A0_REV_ID 0x0001

/* Trident5 X12 */
#define BCM78800_DEVICE_ID 0xf800
#define BCM78800_A0_REV_ID 0x0001
/* Trident5 X12 8Tbps */
#define BCM78808_DEVICE_ID 0xf808
#define BCM78808_A0_REV_ID 0x0001

/* Trident4 */
#define BCM56880_DEVICE_ID 0xb880
#define BCM56880_A0_REV_ID 0x0001
#define BCM56880_B0_REV_ID 0x0011
#define BCM56881_DEVICE_ID 0xb881
#define BCM56881_A0_REV_ID 0x0001
#define BCM56881_B0_REV_ID 0x0011
#define BCM56883_DEVICE_ID 0xb883
#define BCM56883_A0_REV_ID 0x0001
#define BCM56883_B0_REV_ID 0x0011

/* Trident4 X9 */
#define BCM56780_DEVICE_ID 0xb780
#define BCM56780_A0_REV_ID 0x0001
#define BCM56782_DEVICE_ID 0xb782
#define BCM56782_A0_REV_ID 0x0001
#define BCM56782_A1_REV_ID 0x0002
#define BCM56784_DEVICE_ID 0xb784
#define BCM56784_A0_REV_ID 0x0001
#define BCM56785_DEVICE_ID 0xb785
#define BCM56785_A0_REV_ID 0x0001
#define BCM56786_DEVICE_ID 0xb786
#define BCM56786_A0_REV_ID 0x0001
#define BCM56786_A1_REV_ID 0x0002
#define BCM56787_DEVICE_ID 0xb787
#define BCM56787_A0_REV_ID 0x0001
#define BCM56787_A1_REV_ID 0x0002
#define BCM56788_DEVICE_ID 0xb788
#define BCM56788_A0_REV_ID 0x0001
#define BCM56788_A1_REV_ID 0x0002
#define BCM56789_DEVICE_ID 0xb789
#define BCM56789_A0_REV_ID 0x0001
/* Firelight3 */
#define BCM56089_DEVICE_ID 0xb089
#define BCM56089_A0_REV_ID 0x0001
#define BCM56089_A1_REV_ID 0x0002

/* Tomahawk4 */
#define BCM56990_DEVICE_ID 0xb990
#define BCM56990_A0_REV_ID 0x0001

/* Tomahawk4 */
#ifndef BCM56990_DEVICE_ID
#define BCM56990_DEVICE_ID 0xb990
#define BCM56990_A0_REV_ID 0x0001
#endif
#define BCM56990_B0_REV_ID 0x0011
#define BCM56991_DEVICE_ID 0xb991
#define BCM56991_B0_REV_ID 0x0011
#define BCM56992_DEVICE_ID 0xb992
#define BCM56992_B0_REV_ID 0x0011
#define BCM56993_DEVICE_ID 0xb993
#define BCM56993_B0_REV_ID 0x0011

/* Tomahawk4G */
#define BCM56996_DEVICE_ID 0xb996
#define BCM56996_A0_REV_ID 0x0001
#define BCM56997_DEVICE_ID 0xb997
#define BCM56997_A0_REV_ID 0x0001

/* Tomahawk4G */
#ifndef BCM56996_DEVICE_ID
#define BCM56996_DEVICE_ID 0xb996
#define BCM56996_A0_REV_ID 0x0001
#define BCM56997_DEVICE_ID 0xb997
#endif
#define BCM56996_B0_REV_ID 0x0011
#define BCM56997_B0_REV_ID 0x0011

/* Tomahawk4GT */
#define BCM56998_DEVICE_ID 0xb998
#define BCM56998_A0_REV_ID 0x0001

/* Tomahawk4D */
#define BCM56999_DEVICE_ID 0xb999
#define BCM56999_A0_REV_ID 0x0001
#define BCM56999_B0_REV_ID 0x0011
#define BCM56995_DEVICE_ID 0xb995
#define BCM56995_A0_REV_ID 0x0001
#define BCM56995_B0_REV_ID 0x0011

/* Tomahawk5 */
#define BCM78900_DEVICE_ID 0xf900
#define BCM78900_B0_REV_ID 0x0011
#define BCM78902_DEVICE_ID 0xf902
#define BCM78902_B0_REV_ID 0x0011
#define BCM78903_DEVICE_ID 0xf903
#define BCM78903_B0_REV_ID 0x0011

/* Tomahawk5+ */
#define BCM78905_DEVICE_ID 0xf905
#define BCM78905_A0_REV_ID 0x0001

/* Tomahawk5-512 */
#define BCM78907_DEVICE_ID 0xf907
#define BCM78907_A0_REV_ID 0x0001

/* Firelight2 */
#define BCM56080_DEVICE_ID 0xb080
#define BCM56080_A0_REV_ID 0x0001
#define BCM56080_A1_REV_ID 0x0002
#define BCM56081_DEVICE_ID 0xb081
#define BCM56081_A0_REV_ID 0x0001
#define BCM56081_A1_REV_ID 0x0002
#define BCM56082_DEVICE_ID 0xb082
#define BCM56082_A0_REV_ID 0x0001
#define BCM56082_A1_REV_ID 0x0002
#define BCM56083_DEVICE_ID 0xb083
#define BCM56083_A0_REV_ID 0x0001
#define BCM56083_A1_REV_ID 0x0002
#define BCM56084_DEVICE_ID 0xb084
#define BCM56084_A0_REV_ID 0x0001
#define BCM56084_A1_REV_ID 0x0002
#define BCM56085_DEVICE_ID 0xb085
#define BCM56085_A0_REV_ID 0x0001
#define BCM56085_A1_REV_ID 0x0002

#endif  /* !_SOC_DEVIDS_H */
