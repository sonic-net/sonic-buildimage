/*
 * This file is part of Ciena’s Siril mod
 *
 * Copyright (C) 2022 Ciena Corporation
 * Author: Nikhil sahu <nsahu@ciena.com>
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if
 * not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <linux/module.h>
#include "rudra40_cic.h"

#ifdef __KERNEL__
#include <linux/stddef.h>
#else
#include <stddef.h>
#endif

/* -------------------------------------------------------------------------- */
#define QUOTE(str) #str
#define QUOTE_PIN(str) QUOTE(str)

static const char * const gpio_labels[SIRIL_INT_MAX] = {
	// ---- RUDRA40_GLUE_ISR_MISC ----			//  bit hwirq
	NULL,							//    0     0
	QUOTE_PIN(MISC_J2CPA_INT_N),				//    1     1
	NULL,							//    2     2
	QUOTE_PIN(MISC_J2CP_SI5345_50M_INTR_N),			//    3     3
	QUOTE_PIN(MISC_J2CPA_SI5345_156M_INTR_N),		//    4     4
	NULL,							//    5     5
	QUOTE_PIN(MISC_CFPGA_IOEXP_LOCK_INT_N_3V3),		//    6     6
	QUOTE_PIN(MISC_CFPGA_IOEXP_PG_INT_N),			//    7     7
	NULL,							//    8     8
	NULL,							//    9     9
	QUOTE_PIN(MISC_SW_UART_DATA_RCVD),			//   10    10
	QUOTE_PIN(MISC_SHIFTED_PPS_LOCATION),			//   11    11
	QUOTE_PIN(MISC_GPS_SIRIL_1PPS),				//   12    12
	QUOTE_PIN(MISC_PPS_SRC_SEL),				//   13    13
	QUOTE_PIN(MISC_J2CA_TS_SYNC),				//   14    14
	NULL,							//   15    15
	QUOTE_PIN(MISC_RJ45_UART_DATA_RCVD),			//   16    16
	QUOTE_PIN(MISC_HBM_THERM_ALARM_J2CPA),			//   17    17
	NULL,							//   18    18
	QUOTE_PIN(MISC_ZL30603_IRQ),				//   19    19
	QUOTE_PIN(MISC_MB_WARM_ALERT_N),			//   20    20
	QUOTE_PIN(MISC_MB_HOT_ALERT_N),				//   21    21
	QUOTE_PIN(MISC_CPU_BOARD_SEATED_N),			//   22    22
	QUOTE_PIN(MISC_I2C_STATUS2_BUS_INT1_N),			//   23    23
	QUOTE_PIN(MISC_I2C_STATUS2_BUS_INT2_N),			//   24    24
	QUOTE_PIN(MISC_I2C_STATUS2_BUS_INT3_N),			//   25    25
	QUOTE_PIN(MISC_I2C_STATUS2_BUS_INT4_N),			//   26    26
	QUOTE_PIN(MISC_I2C_STATUS2_BUS_INT5_N),			//   27    27
	QUOTE_PIN(MISC_I2C_STATUS2_BUS_INT6_N),			//   28    28
	QUOTE_PIN(MISC_MUX81356_INTR_N_0),			//   29    29
	QUOTE_PIN(MISC_MUX81356_INTR_N_1),			//   30    30
	NULL,							//   31    31

	// ---- RUDRA40_GLUE_ISR_SW_I2C ----			//  bit hwirq
	QUOTE_PIN(SW_I2C_MB_DONE),				//    0    32
	QUOTE_PIN(SW_I2C_SFP_DONE),				//    1    33
	QUOTE_PIN(SW_I2C_J2C_DONE),				//    2    34
	QUOTE_PIN(SW_I2C_PWRGD_DONE),				//    3    35
	NULL,							//    4    36
	NULL,							//    5    37
	NULL,							//    6    38
	NULL,							//    7    39
	NULL,							//    8    40
	NULL,							//    9    41
	NULL,							//   10    42
	NULL,							//   11    43
	NULL,							//   12    44
	NULL,							//   13    45
	NULL,							//   14    46
	NULL,							//   15    47
	NULL,							//   16    48
	NULL,							//   17    49
	NULL,							//   18    50
	NULL,							//   19    51
	NULL,							//   20    52
	NULL,							//   21    53
	NULL,							//   22    54
	NULL,							//   23    55
	NULL,							//   24    56
	NULL,							//   25    57
	NULL,							//   26    58
	NULL,							//   27    59
	NULL,							//   28    60
	NULL,							//   29    61
	NULL,							//   30    62
	NULL,							//   31    63

	// ---- RUDRA40_OPTICS_ISR_QSFP_PRESENT_0 ----		//  bit hwirq
	QUOTE_PIN(Q28_PRES_P0),					//    0    64
	QUOTE_PIN(Q28_PRES_P1),					//    1    65
	QUOTE_PIN(Q28_PRES_P2),					//    2    66
	QUOTE_PIN(Q28_PRES_P3),					//    3    67
	QUOTE_PIN(Q28_PRES_P4),					//    4    68
	QUOTE_PIN(Q28_PRES_P5),					//    5    69
	QUOTE_PIN(Q28_PRES_P6),					//    6    70
	QUOTE_PIN(Q28_PRES_P7),					//    7    71
	NULL,							//    8    72
	NULL,							//    9    73
	NULL,							//   10    74
	NULL,							//   11    75
	NULL,							//   12    76
	NULL,							//   13    77
	NULL,							//   14    78
	NULL,							//   15    79
	NULL,							//   16    80
	NULL,							//   17    81
	NULL,							//   18    82
	NULL,							//   19    83
	NULL,							//   20    84
	NULL,							//   21    85
	NULL,							//   22    86
	NULL,							//   23    87
	NULL,							//   24    88
	NULL,							//   25    89
	NULL,							//   26    90
	NULL,							//   27    91
	NULL,							//   28    92
	NULL,							//   29    93
	NULL,							//   30    94
	NULL,							//   31    95

	// ---- RUDRA40_OPTICS_ISR_QSFP_LOS_0 ----		//  bit hwirq
	QUOTE_PIN(QSFP_LOS_CHG_0),				//    0    96
	QUOTE_PIN(QSFP_LOS_CHG_1),				//    1    97
	QUOTE_PIN(QSFP_LOS_CHG_2),				//    2    98
	QUOTE_PIN(QSFP_LOS_CHG_3),				//    3    99
	QUOTE_PIN(QSFP_LOS_CHG_4),				//    4   100
	QUOTE_PIN(QSFP_LOS_CHG_5),				//    5   101
	QUOTE_PIN(QSFP_LOS_CHG_6),				//    6   102
	QUOTE_PIN(QSFP_LOS_CHG_7),				//    7   103
	NULL,							//    8   104
	NULL,							//    9   105
	NULL,							//   10   106
	NULL,							//   11   107
	NULL,							//   12   108
	NULL,							//   13   109
	NULL,							//   14   110
	NULL,							//   15   111
	NULL,							//   16   112
	NULL,							//   17   113
	NULL,							//   18   114
	NULL,							//   19   115
	NULL,							//   20   116
	NULL,							//   21   117
	NULL,							//   22   118
	NULL,							//   23   119
	NULL,							//   24   120
	NULL,							//   25   121
	NULL,							//   26   122
	NULL,							//   27   123
	NULL,							//   28   124
	NULL,							//   29   125
	NULL,							//   30   126
	NULL,							//   31   127

	// ---- RUDRA40_OPTICS_ISR_QSFP_PWR_GD_0 ----		//  bit hwirq
	QUOTE_PIN(QSFP_PWR_GD_CHG_0),				//    0   128
	QUOTE_PIN(QSFP_PWR_GD_CHG_1),				//    1   129
	QUOTE_PIN(QSFP_PWR_GD_CHG_2),				//    2   130
	QUOTE_PIN(QSFP_PWR_GD_CHG_3),				//    3   131
	QUOTE_PIN(QSFP_PWR_GD_CHG_4),				//    4   132
	QUOTE_PIN(QSFP_PWR_GD_CHG_5),				//    5   133
	QUOTE_PIN(QSFP_PWR_GD_CHG_6),				//    6   134
	QUOTE_PIN(QSFP_PWR_GD_CHG_7),				//    7   135
	NULL,							//    8   136
	NULL,							//    9   137
	NULL,							//   10   138
	NULL,							//   11   139
	NULL,							//   12   140
	NULL,							//   13   141
	NULL,							//   14   142
	NULL,							//   15   143
	NULL,							//   16   144
	NULL,							//   17   145
	NULL,							//   18   146
	NULL,							//   19   147
	NULL,							//   20   148
	NULL,							//   21   149
	NULL,							//   22   150
	NULL,							//   23   151
	NULL,							//   24   152
	NULL,							//   25   153
	NULL,							//   26   154
	NULL,							//   27   155
	NULL,							//   28   156
	NULL,							//   29   157
	NULL,							//   30   158
	NULL,							//   31   159

	// ---- RUDRA40_OPTICS_ISR_SFP_RX_LOS ----		//  bit hwirq
	QUOTE_PIN(SFP_RXLOS_P0),				//    0   160
	QUOTE_PIN(SFP_RXLOS_P1),				//    1   161
	QUOTE_PIN(SFP_RXLOS_P2),				//    2   162
	QUOTE_PIN(SFP_RXLOS_P3),				//    3   163
	QUOTE_PIN(SFP_RXLOS_P4),				//    4   164
	QUOTE_PIN(SFP_RXLOS_P5),				//    5   165
	QUOTE_PIN(SFP_RXLOS_P6),				//    6   166
	QUOTE_PIN(SFP_RXLOS_P7),				//    7   167
	QUOTE_PIN(SFP_RXLOS_P8),				//    8   168
	QUOTE_PIN(SFP_RXLOS_P9),				//    9   169
	QUOTE_PIN(SFP_RXLOS_P10),				//   10   170
	QUOTE_PIN(SFP_RXLOS_P11),				//   11   171
	QUOTE_PIN(SFP_RXLOS_P12),				//   12   172
	QUOTE_PIN(SFP_RXLOS_P13),				//   13   173
	QUOTE_PIN(SFP_RXLOS_P14),				//   14   174
	QUOTE_PIN(SFP_RXLOS_P15),				//   15   175
	QUOTE_PIN(SFP_RXLOS_P16),				//   16   176
	QUOTE_PIN(SFP_RXLOS_P17),				//   17   177
	QUOTE_PIN(SFP_RXLOS_P18),				//   18   178
	QUOTE_PIN(SFP_RXLOS_P19),				//   19   179
	QUOTE_PIN(SFP_RXLOS_P20),				//   20   180
	QUOTE_PIN(SFP_RXLOS_P21),				//   21   181
	QUOTE_PIN(SFP_RXLOS_P22),				//   22   182
	QUOTE_PIN(SFP_RXLOS_P23),				//   23   183
	QUOTE_PIN(SFP_RXLOS_P24),				//   24   184
	QUOTE_PIN(SFP_RXLOS_P25),				//   25   185
	QUOTE_PIN(SFP_RXLOS_P26),				//   26   186
	QUOTE_PIN(SFP_RXLOS_P27),				//   27   187
	QUOTE_PIN(SFP_RXLOS_P28),				//   28   188
	QUOTE_PIN(SFP_RXLOS_P29),				//   29   189
	QUOTE_PIN(SFP_RXLOS_P30),				//   30   190
	QUOTE_PIN(SFP_RXLOS_P31),				//   31   191

	// ---- RUDRA40_OPTICS_ISR_SFP_RX_LOS_2 ----		//  bit hwirq
	QUOTE_PIN(SFP_RXLOS_P32),				//    0   192
	QUOTE_PIN(SFP_RXLOS_P33),				//    1   193
	QUOTE_PIN(SFP_RXLOS_P34),				//    2   194
	QUOTE_PIN(SFP_RXLOS_P35),				//    3   195
	QUOTE_PIN(SFP_RXLOS_P36),				//    4   196
	QUOTE_PIN(SFP_RXLOS_P37),				//    5   197
	QUOTE_PIN(SFP_RXLOS_P38),				//    6   198
	QUOTE_PIN(SFP_RXLOS_P39),				//    7   199
	NULL,							//    8   200
	NULL,							//    9   201
	NULL,							//   10   202
	NULL,							//   11   203
	NULL,							//   12   204
	NULL,							//   13   205
	NULL,							//   14   206
	NULL,							//   15   207
	NULL,							//   16   208
	NULL,							//   17   209
	NULL,							//   18   210
	NULL,							//   19   211
	NULL,							//   20   212
	NULL,							//   21   213
	NULL,							//   22   214
	NULL,							//   23   215
	NULL,							//   24   216
	NULL,							//   25   217
	NULL,							//   26   218
	NULL,							//   27   219
	NULL,							//   28   220
	NULL,							//   29   221
	NULL,							//   30   222
	NULL,							//   31   223

	// ---- RUDRA40_OPTICS_ISR_SFP_TX_FAULT ----		//  bit hwirq
	QUOTE_PIN(SFP_TXFLT_P0),				//    0   224
	QUOTE_PIN(SFP_TXFLT_P1),				//    1   225
	QUOTE_PIN(SFP_TXFLT_P2),				//    2   226
	QUOTE_PIN(SFP_TXFLT_P3),				//    3   227
	QUOTE_PIN(SFP_TXFLT_P4),				//    4   228
	QUOTE_PIN(SFP_TXFLT_P5),				//    5   229
	QUOTE_PIN(SFP_TXFLT_P6),				//    6   230
	QUOTE_PIN(SFP_TXFLT_P7),				//    7   231
	QUOTE_PIN(SFP_TXFLT_P8),				//    8   232
	QUOTE_PIN(SFP_TXFLT_P9),				//    9   233
	QUOTE_PIN(SFP_TXFLT_P10),				//   10   234
	QUOTE_PIN(SFP_TXFLT_P11),				//   11   235
	QUOTE_PIN(SFP_TXFLT_P12),				//   12   236
	QUOTE_PIN(SFP_TXFLT_P13),				//   13   237
	QUOTE_PIN(SFP_TXFLT_P14),				//   14   238
	QUOTE_PIN(SFP_TXFLT_P15),				//   15   239
	QUOTE_PIN(SFP_TXFLT_P16),				//   16   240
	QUOTE_PIN(SFP_TXFLT_P17),				//   17   241
	QUOTE_PIN(SFP_TXFLT_P18),				//   18   242
	QUOTE_PIN(SFP_TXFLT_P19),				//   19   243
	QUOTE_PIN(SFP_TXFLT_P20),				//   20   244
	QUOTE_PIN(SFP_TXFLT_P21),				//   21   245
	QUOTE_PIN(SFP_TXFLT_P22),				//   22   246
	QUOTE_PIN(SFP_TXFLT_P23),				//   23   247
	QUOTE_PIN(SFP_TXFLT_P24),				//   24   248
	QUOTE_PIN(SFP_TXFLT_P25),				//   25   249
	QUOTE_PIN(SFP_TXFLT_P26),				//   26   250
	QUOTE_PIN(SFP_TXFLT_P27),				//   27   251
	QUOTE_PIN(SFP_TXFLT_P28),				//   28   252
	QUOTE_PIN(SFP_TXFLT_P29),				//   29   253
	QUOTE_PIN(SFP_TXFLT_P30),				//   30   254
	QUOTE_PIN(SFP_TXFLT_P31),				//   31   255

	// ---- RUDRA40_OPTICS_ISR_SFP_TX_FAULT_2 ----		//  bit hwirq
	QUOTE_PIN(SFP_TXFLT_P32),				//    0   256
	QUOTE_PIN(SFP_TXFLT_P33),				//    1   257
	QUOTE_PIN(SFP_TXFLT_P34),				//    2   258
	QUOTE_PIN(SFP_TXFLT_P35),				//    3   259
	QUOTE_PIN(SFP_TXFLT_P36),				//    4   260
	QUOTE_PIN(SFP_TXFLT_P37),				//    5   261
	QUOTE_PIN(SFP_TXFLT_P38),				//    6   262
	QUOTE_PIN(SFP_TXFLT_P39),				//    7   263
	NULL,							//    8   264
	NULL,							//    9   265
	NULL,							//   10   266
	NULL,							//   11   267
	NULL,							//   12   268
	NULL,							//   13   269
	NULL,							//   14   270
	NULL,							//   15   271
	NULL,							//   16   272
	NULL,							//   17   273
	NULL,							//   18   274
	NULL,							//   19   275
	NULL,							//   20   276
	NULL,							//   21   277
	NULL,							//   22   278
	NULL,							//   23   279
	NULL,							//   24   280
	NULL,							//   25   281
	NULL,							//   26   282
	NULL,							//   27   283
	NULL,							//   28   284
	NULL,							//   29   285
	NULL,							//   30   286
	NULL,							//   31   287

	// ---- RUDRA40_OPTICS_ISR_SFP_PRESENT ----		//  bit hwirq
	QUOTE_PIN(SFP_PRES_P0),					//    0   288
	QUOTE_PIN(SFP_PRES_P1),					//    1   289
	QUOTE_PIN(SFP_PRES_P2),					//    2   290
	QUOTE_PIN(SFP_PRES_P3),					//    3   291
	QUOTE_PIN(SFP_PRES_P4),					//    4   292
	QUOTE_PIN(SFP_PRES_P5),					//    5   293
	QUOTE_PIN(SFP_PRES_P6),					//    6   294
	QUOTE_PIN(SFP_PRES_P7),					//    7   295
	QUOTE_PIN(SFP_PRES_P8),					//    8   296
	QUOTE_PIN(SFP_PRES_P9),					//    9   297
	QUOTE_PIN(SFP_PRES_P10),				//   10   298
	QUOTE_PIN(SFP_PRES_P11),				//   11   299
	QUOTE_PIN(SFP_PRES_P12),				//   12   300
	QUOTE_PIN(SFP_PRES_P13),				//   13   301
	QUOTE_PIN(SFP_PRES_P14),				//   14   302
	QUOTE_PIN(SFP_PRES_P15),				//   15   303
	QUOTE_PIN(SFP_PRES_P16),				//   16   304
	QUOTE_PIN(SFP_PRES_P17),				//   17   305
	QUOTE_PIN(SFP_PRES_P18),				//   18   306
	QUOTE_PIN(SFP_PRES_P19),				//   19   307
	QUOTE_PIN(SFP_PRES_P20),				//   20   308
	QUOTE_PIN(SFP_PRES_P21),				//   21   309
	QUOTE_PIN(SFP_PRES_P22),				//   22   310
	QUOTE_PIN(SFP_PRES_P23),				//   23   311
	QUOTE_PIN(SFP_PRES_P24),				//   24   312
	QUOTE_PIN(SFP_PRES_P25),				//   25   313
	QUOTE_PIN(SFP_PRES_P26),				//   26   314
	QUOTE_PIN(SFP_PRES_P27),				//   27   315
	QUOTE_PIN(SFP_PRES_P28),				//   28   316
	QUOTE_PIN(SFP_PRES_P29),				//   29   317
	QUOTE_PIN(SFP_PRES_P30),				//   30   318
	QUOTE_PIN(SFP_PRES_P31),				//   31   319

	// ---- RUDRA40_OPTICS_ISR_SFP_PRESENT_2 ----		//  bit hwirq
	QUOTE_PIN(SFP_PRES_P32),				//    0   320
	QUOTE_PIN(SFP_PRES_P33),				//    1   321
	QUOTE_PIN(SFP_PRES_P34),				//    2   322
	QUOTE_PIN(SFP_PRES_P35),				//    3   323
	QUOTE_PIN(SFP_PRES_P36),				//    4   324
	QUOTE_PIN(SFP_PRES_P37),				//    5   325
	QUOTE_PIN(SFP_PRES_P38),				//    6   326
	QUOTE_PIN(SFP_PRES_P39),				//    7   327
	NULL,							//    8   328
	NULL,							//    9   329
	NULL,							//   10   330
	NULL,							//   11   331
	NULL,							//   12   332
	NULL,							//   13   333
	NULL,							//   14   334
	NULL,							//   15   335
	NULL,							//   16   336
	NULL,							//   17   337
	NULL,							//   18   338
	NULL,							//   19   339
	NULL,							//   20   340
	NULL,							//   21   341
	NULL,							//   22   342
	NULL,							//   23   343
	NULL,							//   24   344
	NULL,							//   25   345
	NULL,							//   26   346
	NULL,							//   27   347
	NULL,							//   28   348
	NULL,							//   29   349
	NULL,							//   30   350
	NULL,							//   31   351

	// ---- RUDRA40_OPTICS_ISR_SFP_PWR_GD ----		//  bit hwirq
	QUOTE_PIN(SFP_PWR_GD_CHG_0),				//    0   352
	QUOTE_PIN(SFP_PWR_GD_CHG_1),				//    1   353
	QUOTE_PIN(SFP_PWR_GD_CHG_2),				//    2   354
	QUOTE_PIN(SFP_PWR_GD_CHG_3),				//    3   355
	QUOTE_PIN(SFP_PWR_GD_CHG_4),				//    4   356
	QUOTE_PIN(SFP_PWR_GD_CHG_5),				//    5   357
	QUOTE_PIN(SFP_PWR_GD_CHG_6),				//    6   358
	QUOTE_PIN(SFP_PWR_GD_CHG_7),				//    7   359
	QUOTE_PIN(SFP_PWR_GD_CHG_8),				//    8   360
	QUOTE_PIN(SFP_PWR_GD_CHG_9),				//    9   361
	QUOTE_PIN(SFP_PWR_GD_CHG_10),				//   10   362
	QUOTE_PIN(SFP_PWR_GD_CHG_11),				//   11   363
	QUOTE_PIN(SFP_PWR_GD_CHG_12),				//   12   364
	QUOTE_PIN(SFP_PWR_GD_CHG_13),				//   13   365
	QUOTE_PIN(SFP_PWR_GD_CHG_14),				//   14   366
	QUOTE_PIN(SFP_PWR_GD_CHG_15),				//   15   367
	QUOTE_PIN(SFP_PWR_GD_CHG_16),				//   16   368
	QUOTE_PIN(SFP_PWR_GD_CHG_17),				//   17   369
	QUOTE_PIN(SFP_PWR_GD_CHG_18),				//   18   370
	QUOTE_PIN(SFP_PWR_GD_CHG_19),				//   19   371
	QUOTE_PIN(SFP_PWR_GD_CHG_20),				//   20   372
	QUOTE_PIN(SFP_PWR_GD_CHG_21),				//   21   373
	QUOTE_PIN(SFP_PWR_GD_CHG_22),				//   22   374
	QUOTE_PIN(SFP_PWR_GD_CHG_23),				//   23   375
	QUOTE_PIN(SFP_PWR_GD_CHG_24),				//   24   376
	QUOTE_PIN(SFP_PWR_GD_CHG_25),				//   25   377
	QUOTE_PIN(SFP_PWR_GD_CHG_26),				//   26   378
	QUOTE_PIN(SFP_PWR_GD_CHG_27),				//   27   379
	QUOTE_PIN(SFP_PWR_GD_CHG_28),				//   28   380
	QUOTE_PIN(SFP_PWR_GD_CHG_29),				//   29   381
	QUOTE_PIN(SFP_PWR_GD_CHG_30),				//   30   382
	QUOTE_PIN(SFP_PWR_GD_CHG_31),				//   31   383

	// ---- RUDRA40_OPTICS_ISR_SFP_PWR_GD_2 ----		//  bit hwirq
	QUOTE_PIN(SFP_PWR_GD_CHG_32),				//    0   384
	QUOTE_PIN(SFP_PWR_GD_CHG_33),				//    1   385
	QUOTE_PIN(SFP_PWR_GD_CHG_34),				//    2   386
	QUOTE_PIN(SFP_PWR_GD_CHG_35),				//    3   387
	QUOTE_PIN(SFP_PWR_GD_CHG_36),				//    4   388
	QUOTE_PIN(SFP_PWR_GD_CHG_37),				//    5   389
	QUOTE_PIN(SFP_PWR_GD_CHG_38),				//    6   390
	QUOTE_PIN(SFP_PWR_GD_CHG_39),				//    7   391
	NULL,							//    8   392
	NULL,							//    9   393
	NULL,							//   10   394
	NULL,							//   11   395
	NULL,							//   12   396
	NULL,							//   13   397
	NULL,							//   14   398
	NULL,							//   15   399
	NULL,							//   16   400
	NULL,							//   17   401
	NULL,							//   18   402
	NULL,							//   19   403
	NULL,							//   20   404
	NULL,							//   21   405
	NULL,							//   22   406
	NULL,							//   23   407
	NULL,							//   24   408
	NULL,							//   25   409
	NULL,							//   26   410
	NULL,							//   27   411
	NULL,							//   28   412
	NULL,							//   29   413
	NULL,							//   30   414
	NULL,							//   31   415

	// ---- RUDRA40_OPTICS_ISR_SFPDD_RX_LOS ----		//  bit hwirq
	QUOTE_PIN(SFPDD_RX_LOS_CHG_0),				//    0   416
	QUOTE_PIN(SFPDD_RX_LOS_CHG_1),				//    1   417
	QUOTE_PIN(SFPDD_RX_LOS_CHG_2),				//    2   418
	QUOTE_PIN(SFPDD_RX_LOS_CHG_3),				//    3   419
	QUOTE_PIN(SFPDD_RX_LOS_CHG_4),				//    4   420
	QUOTE_PIN(SFPDD_RX_LOS_CHG_5),				//    5   421
	QUOTE_PIN(SFPDD_RX_LOS_CHG_6),				//    6   422
	QUOTE_PIN(SFPDD_RX_LOS_CHG_7),				//    7   423
	QUOTE_PIN(SFPDD_RX_LOS_CHG_8),				//    8   424
	QUOTE_PIN(SFPDD_RX_LOS_CHG_9),				//    9   425
	QUOTE_PIN(SFPDD_RX_LOS_CHG_10),				//   10   426
	QUOTE_PIN(SFPDD_RX_LOS_CHG_11),				//   11   427
	QUOTE_PIN(SFPDD_RX_LOS_CHG_12),				//   12   428
	QUOTE_PIN(SFPDD_RX_LOS_CHG_13),				//   13   429
	QUOTE_PIN(SFPDD_RX_LOS_CHG_14),				//   14   430
	QUOTE_PIN(SFPDD_RX_LOS_CHG_15),				//   15   431
	QUOTE_PIN(SFPDD_RX_LOS_CHG_16),				//   16   432
	QUOTE_PIN(SFPDD_RX_LOS_CHG_17),				//   17   433
	QUOTE_PIN(SFPDD_RX_LOS_CHG_18),				//   18   434
	QUOTE_PIN(SFPDD_RX_LOS_CHG_19),				//   19   435
	QUOTE_PIN(SFPDD_RX_LOS_CHG_20),				//   20   436
	QUOTE_PIN(SFPDD_RX_LOS_CHG_21),				//   21   437
	QUOTE_PIN(SFPDD_RX_LOS_CHG_22),				//   22   438
	QUOTE_PIN(SFPDD_RX_LOS_CHG_23),				//   23   439
	QUOTE_PIN(SFPDD_RX_LOS_CHG_24),				//   24   440
	QUOTE_PIN(SFPDD_RX_LOS_CHG_25),				//   25   441
	QUOTE_PIN(SFPDD_RX_LOS_CHG_26),				//   26   442
	QUOTE_PIN(SFPDD_RX_LOS_CHG_27),				//   27   443
	QUOTE_PIN(SFPDD_RX_LOS_CHG_28),				//   28   444
	QUOTE_PIN(SFPDD_RX_LOS_CHG_29),				//   29   445
	QUOTE_PIN(SFPDD_RX_LOS_CHG_30),				//   30   446
	QUOTE_PIN(SFPDD_RX_LOS_CHG_31),				//   31   447

	// ---- RUDRA40_OPTICS_ISR_SFPDD_RX_LOS_2 ----		//  bit hwirq
	QUOTE_PIN(SFPDD_RX_LOS_CHG_32),				//    0   448
	QUOTE_PIN(SFPDD_RX_LOS_CHG_33),				//    1   449
	QUOTE_PIN(SFPDD_RX_LOS_CHG_34),				//    2   450
	QUOTE_PIN(SFPDD_RX_LOS_CHG_35),				//    3   451
	QUOTE_PIN(SFPDD_RX_LOS_CHG_36),				//    4   452
	QUOTE_PIN(SFPDD_RX_LOS_CHG_37),				//    5   453
	QUOTE_PIN(SFPDD_RX_LOS_CHG_38),				//    6   454
	QUOTE_PIN(SFPDD_RX_LOS_CHG_39),				//    7   455
	NULL,							//    8   456
	NULL,							//    9   457
	NULL,							//   10   458
	NULL,							//   11   459
	NULL,							//   12   460
	NULL,							//   13   461
	NULL,							//   14   462
	NULL,							//   15   463
	NULL,							//   16   464
	NULL,							//   17   465
	NULL,							//   18   466
	NULL,							//   19   467
	NULL,							//   20   468
	NULL,							//   21   469
	NULL,							//   22   470
	NULL,							//   23   471
	NULL,							//   24   472
	NULL,							//   25   473
	NULL,							//   26   474
	NULL,							//   27   475
	NULL,							//   28   476
	NULL,							//   29   477
	NULL,							//   30   478
	NULL,							//   31   479

	// ---- RUDRA40_OPTICS_ISR_SFPDD_TX_FAULT ----		//  bit hwirq
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_0),			//    0   480
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_1),			//    1   481
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_2),			//    2   482
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_3),			//    3   483
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_4),			//    4   484
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_5),			//    5   485
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_6),			//    6   486
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_7),			//    7   487
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_8),			//    8   488
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_9),			//    9   489
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_10),			//   10   490
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_11),			//   11   491
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_12),			//   12   492
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_13),			//   13   493
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_14),			//   14   494
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_15),			//   15   495
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_16),			//   16   496
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_17),			//   17   497
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_18),			//   18   498
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_19),			//   19   499
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_20),			//   20   500
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_21),			//   21   501
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_22),			//   22   502
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_23),			//   23   503
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_24),			//   24   504
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_25),			//   25   505
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_26),			//   26   506
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_27),			//   27   507
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_28),			//   28   508
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_29),			//   29   509
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_30),			//   30   510
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_31),			//   31   511

	// ---- RUDRA40_OPTICS_ISR_SFPDD_TX_FAULT_2 ----	//  bit hwirq
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_32),			//    0   512
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_33),			//    1   513
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_34),			//    2   514
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_35),			//    3   515
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_36),			//    4   516
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_37),			//    5   517
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_38),			//    6   518
	QUOTE_PIN(SFPDD_TX_FAULT_CHG_39),			//    7   519
	NULL,							//    8   520
	NULL,							//    9   521
	NULL,							//   10   522
	NULL,							//   11   523
	NULL,							//   12   524
	NULL,							//   13   525
	NULL,							//   14   526
	NULL,							//   15   527
	NULL,							//   16   528
	NULL,							//   17   529
	NULL,							//   18   530
	NULL,							//   19   531
	NULL,							//   20   532
	NULL,							//   21   533
	NULL,							//   22   534
	NULL,							//   23   535
	NULL,							//   24   536
	NULL,							//   25   537
	NULL,							//   26   538
	NULL,							//   27   539
	NULL,							//   28   540
	NULL,							//   29   541
	NULL,							//   30   542
	NULL,							//   31   543
}; // end gpio_labels

const char *rudra40_cic_pin_to_str(uint32_t pin, uint32_t level)
{
	if (pin < SIRIL_INT_MAX)
		return gpio_labels[pin];
	else
		return NULL;
}

bool rudra40_cic_add_gpio_lkup(uint32_t pin, uint32_t level)
{
	switch (pin) {
	case RUDRA40_INT_SW_I2C_MB_DONE:
	case RUDRA40_INT_SW_I2C_SFP_DONE:
	case RUDRA40_INT_SW_I2C_J2C_DONE:
	case RUDRA40_INT_SW_I2C_PWRGD_DONE:
	case RUDRA40_INT_MISC_RJ45_UART_DATA_RCVD:
	case RUDRA40_INT_MISC_SW_UART_DATA_RCVD:
		return true;
	default:
		return false;
	}
}
