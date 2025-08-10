####################################################################################################
# 2020-09-07, Credo BALD EAGLE SDK Software scripts
#
# CREDO Semiconductors Inc. Confidential
# 
####################################################################################################
# These sets of scripts are for use with Bald Eagle, with the Binary FW
#2
# To be able to run the functions inside this file, execute this file at the python prompt:
#
#      >>> execfile("CredoSdk.py")
# 
#
####################################################################################################
from __future__ import division
import re, sys, os, time, datetime, struct
from sys import *
#import numpy as np
import math
import ctypes
sys.path.insert(0,os.getcwd())
cameolibpath=os.getenv('CAMEOLIB')
fw_path=os.getenv('CREDO_100G_PATH')
#libcameo = ctypes.CDLL("libcameo_mdio.so")
ctypes.cdll.LoadLibrary(cameolibpath)
libcameo = ctypes.CDLL(cameolibpath)

#import /usr/lib/python2.7/site-packages/DosComponent/BabbageLib/baldeagle_phoenix_reg as phoenix
#import /usr/lib/python2.7/site-packages/DosComponent/BabbageLib/baldeagle_eagle_reg as eagle

import baldeagle_phoenix_reg as Pam4Reg
import baldeagle_eagle_reg as NrzReg
try:
    reload(NrzReg)
    reload(Pam4Reg)
except NameError:
    pass
        
##########################[   0       1       2       3      4      5      6       7       8       9     10      11     12      13     14      15 ] 
lane_name_list =          [ 'A0',   'A1',   'A2',   'A3',  'A4',  'A5',   'A6',   'A7',  'B0',   'B1',  'B2',   'B3',  'B4',   'B5',  'B6',   'B7']
#lane_rx_input_mode_list = [ 'dc',   'dc',   'dc',
try:
    lane_rx_input_mode_list
except NameError:
    lane_rx_input_mode_list = ['dc']*16
try:
    lane_mode_list
except NameError:
    lane_mode_list = ['pam4'] *16
try:
    chan_est_list
except NameError:
    chan_est_list = [1.010,63,62] *16

    
lane_offset = { 'A0': 0x7000,'A1': 0x7200,'A2': 0x7400,'A3': 0x7600,
                'A4': 0x7800,'A5': 0x7a00,'A6': 0x7c00,'A7': 0x7e00,
                'B0': 0x8000,'B1': 0x8200,'B2': 0x8400,'B3': 0x8600,
                'B4': 0x8800,'B5': 0x8a00,'B6': 0x8c00,'B7': 0x8e00,}

gPrbsPrevCount  = { 'A0': 0,'A1': 0,'A2': 0,'A3': 0,
                    'A4': 0,'A5': 0,'A6': 0,'A7': 0,
                    'B0': 0,'B1': 0,'B2': 0,'B3': 0,
                    'B4': 0,'B5': 0,'B6': 0,'B7': 0,}
                    
    
MDIO_CONNECTED       = 0
MDIO_DISCONNECTED    = 1
MDIO_LOST_CONNECTION = 2
gCard                   = None
gChipName               = 'Baldeagle'
gSetupFileName          = 'Baldeagle_RegSetup.txt'
gUsbPort                = 0 # eval board USB port0 or port 1, Can run 2 Credo boards in 2 python shells on the same PC
gSlice                  = 0 # Slice 0 or 1
gRefClkFreq             = 195.3125  # 156.25
gNrzLanes               = [8,9,10,11,12,13,14,15] #[0,1,2,3,4,5,6,7]
gPam4Lanes              = [0,1,2,3,4,5,6,7] #[8,9,10,11,12,13,14,15]
gPrbsEn                 = True
gPam4_En                = 1
gLane                   = list(range(16)) #[0,1,2,3,4,5,6,7] #or 'ALL'
gDebugPrint             = 1
gNrzTxSourceIsCredo     = 1 # 0: means do not touch  NRZ lane's TX taps, as it's driving another NRZ lane's RX
gPam4TxSourceIsCredo    = 1 # 0: means do not touch PAM4 lane's TX taps, as it's driving another NRZ lane's RX
gFecThresh              = 15
gSltVer                 = 0.0
gDualSliceMode          = False
c                       = Pam4Reg
# RxPolarityMap
# TxPolarityMap
# RxGrayCodeMap
# TxGrayCodeMap
# RxPrecoderMap
# TxPrecoderMap
# RxMsbLsbMap;   
# TxMsbLsbMap
# gLanePartnerMap
gFwFileName              = fw_path + '/BE2.fw.2.18.43.bin';
gFwFileNameLastLoaded    = None
gFecStatusPrevTimeStamp  = []
gFecStatusCurrTimeStamp  = []
gMode_sel                = True #MDIO:True,  I2C:False
gSlice_grop              = []

for i in range(32):
    gFecStatusPrevTimeStamp.append(time.time());
    gFecStatusCurrTimeStamp.append(time.time());

#gFecStatusPrevTimeStamp[0] = time.time() # Slice 0
#gFecStatusPrevTimeStamp[1] = time.time() # Slice 1
#gFecStatusCurrTimeStamp[0] = time.time() # Slice 0
#gFecStatusCurrTimeStamp[1] = time.time() # Slice 1


gPrbsResetTime=time.time()
gDebugTuning = False
try:
    gDebugTuning
except NameError:
    gDebugTuning = False
try:
    gPrint
except NameError:
    gPrint = True
try:
    gLane
except NameError:
    gLane = [0]


##########################
# Define Globals here once
########################## SMK Board's TX Source for each lane's RX is its own TX (assuming loop-back)
#gLanePartnerMap;
gLanePartnerMap=[[[0,0],[0,1],[0,2],[0,3],[0,4],[0,5],[0,6],[0,7],  [0,8],[0,9],[0,10],[0,11],[0,12],[0,13],[0,14],[0,15]], # Slice 0, TX Source for each lane's RX
                 [[1,0],[1,1],[1,2],[1,3],[1,4],[1,5],[1,6],[1,7],  [1,8],[1,9],[1,10],[1,11],[1,12],[1,13],[1,14],[1,15]], # Slice 1, TX Source for each lane's RX
                 [[2,0],[2,1],[2,2],[2,3],[2,4],[2,5],[2,6],[2,7],  [2,8],[2,9],[2,10],[2,11],[2,12],[2,13],[2,14],[2,15]], # Slice 2, TX Source for each lane's RX
                 [[3,0],[3,1],[3,2],[3,3],[3,4],[3,5],[3,6],[3,7],  [3,8],[3,9],[3,10],[3,11],[3,12],[3,13],[3,14],[3,15]], # Slice 3, TX Source for each lane's RX
                 [[4,0],[4,1],[4,2],[4,3],[4,4],[4,5],[4,6],[4,7],  [4,8],[4,9],[4,10],[4,11],[4,12],[4,13],[4,14],[4,15]], # Slice 4, TX Source for each lane's RX
                 [[5,0],[5,1],[5,2],[5,3],[5,4],[5,5],[5,6],[5,7],  [5,8],[5,9],[5,10],[5,11],[5,12],[5,13],[5,14],[5,15]], # Slice 5, TX Source for each lane's RX
                 [[6,0],[6,1],[6,2],[6,3],[6,4],[6,5],[6,6],[6,7],  [6,8],[6,9],[6,10],[6,11],[6,12],[6,13],[6,14],[6,15]], # Slice 6, TX Source for each lane's RX
                 [[7,0],[7,1],[7,2],[7,3],[7,4],[7,5],[7,6],[7,7],  [7,8],[7,9],[7,10],[7,11],[7,12],[7,13],[7,14],[7,15]], # Slice 7, TX Source for each lane's RX
                 [[8,0],[8,1],[8,2],[8,3],[8,4],[8,5],[8,6],[8,7],  [8,8],[8,9],[8,10],[8,11],[8,12],[8,13],[8,14],[8,15]], # Slice 8, TX Source for each lane's RX
                 [[9,0],[9,1],[9,2],[9,3],[9,4],[9,5],[9,6],[9,7],  [9,8],[9,9],[9,10],[9,11],[9,12],[9,13],[9,14],[9,15]], # Slice 9, TX Source for each lane's RX

                 [[10,0],[10,1],[10,2],[10,3],[10,4],[10,5],[10,6],[10,7],  [10,8],[10,9],[10,10],[10,11],[10,12],[10,13],[10,14],[10,15]], # Slice 10, TX Source for each lane's RX
                 [[11,0],[11,1],[11,2],[11,3],[11,4],[11,5],[11,6],[11,7],  [11,8],[11,9],[11,10],[11,11],[11,12],[11,13],[11,14],[11,15]], # Slice 11, TX Source for each lane's RX
                 [[12,0],[12,1],[12,2],[12,3],[12,4],[12,5],[12,6],[12,7],  [12,8],[12,9],[12,10],[12,11],[12,12],[12,13],[12,14],[12,15]], # Slice 12, TX Source for each lane's RX
                 [[13,0],[13,1],[13,2],[13,3],[13,4],[13,5],[13,6],[13,7],  [13,8],[13,9],[13,10],[13,11],[13,12],[13,13],[13,14],[13,15]], # Slice 13, TX Source for each lane's RX
                 [[14,0],[14,1],[14,2],[14,3],[14,4],[14,5],[14,6],[14,7],  [14,8],[14,9],[14,10],[14,11],[14,12],[14,13],[14,14],[14,15]], # Slice 14, TX Source for each lane's RX
                 [[15,0],[15,1],[15,2],[15,3],[15,4],[15,5],[15,6],[15,7],  [15,8],[15,9],[15,10],[15,11],[15,12],[15,13],[15,14],[15,15]], # Slice 15, TX Source for each lane's RX
                 [[16,0],[16,1],[16,2],[16,3],[16,4],[16,5],[16,6],[16,7],  [16,8],[16,9],[16,10],[16,11],[16,12],[16,13],[16,14],[16,15]], # Slice 16, TX Source for each lane's RX
                 [[17,0],[17,1],[17,2],[17,3],[17,4],[17,5],[17,6],[17,7],  [17,8],[17,9],[17,10],[17,11],[17,12],[17,13],[17,14],[17,15]], # Slice 17, TX Source for each lane's RX
                 [[18,0],[18,1],[18,2],[18,3],[18,4],[18,5],[18,6],[18,7],  [18,8],[18,9],[18,10],[18,11],[18,12],[18,13],[18,14],[18,15]], # Slice 18, TX Source for each lane's RX
                 [[19,0],[19,1],[19,2],[19,3],[19,4],[19,5],[19,6],[19,7],  [19,8],[19,9],[19,10],[19,11],[19,12],[19,13],[19,14],[19,15]], # Slice 19, TX Source for each lane's RX

                 [[20,0],[20,1],[20,2],[20,3],[20,4],[20,5],[20,6],[20,7],  [20,8],[20,9],[20,10],[20,11],[20,12],[20,13],[20,14],[20,15]], # Slice 20, TX Source for each lane's RX
                 [[21,0],[21,1],[21,2],[21,3],[21,4],[21,5],[21,6],[21,7],  [21,8],[21,9],[21,10],[21,11],[21,12],[21,13],[21,14],[21,15]], # Slice 21, TX Source for each lane's RX
                 [[22,0],[22,1],[22,2],[22,3],[22,4],[22,5],[22,6],[22,7],  [22,8],[22,9],[22,10],[22,11],[22,12],[22,13],[22,14],[22,15]], # Slice 22, TX Source for each lane's RX
                 [[23,0],[23,1],[23,2],[23,3],[23,4],[23,5],[23,6],[23,7],  [23,8],[23,9],[23,10],[23,11],[23,12],[23,13],[23,14],[23,15]], # Slice 23, TX Source for each lane's RX
                 [[24,0],[24,1],[24,2],[24,3],[24,4],[24,5],[24,6],[24,7],  [24,8],[24,9],[24,10],[24,11],[24,12],[24,13],[24,14],[24,15]], # Slice 24, TX Source for each lane's RX
                 [[25,0],[25,1],[25,2],[25,3],[25,4],[25,5],[25,6],[25,7],  [25,8],[25,9],[25,10],[25,11],[25,12],[25,13],[25,14],[25,15]], # Slice 25, TX Source for each lane's RX
                 [[26,0],[26,1],[26,2],[26,3],[26,4],[26,5],[26,6],[26,7],  [26,8],[26,9],[26,10],[26,11],[26,12],[26,13],[26,14],[26,15]], # Slice 26, TX Source for each lane's RX
                 [[27,0],[27,1],[27,2],[27,3],[27,4],[27,5],[27,6],[27,7],  [27,8],[27,9],[27,10],[27,11],[27,12],[27,13],[27,14],[27,15]], # Slice 27, TX Source for each lane's RX
                 [[28,0],[28,1],[28,2],[28,3],[28,4],[28,5],[28,6],[28,7],  [28,8],[28,9],[28,10],[28,11],[28,12],[28,13],[28,14],[28,15]], # Slice 28, TX Source for each lane's RX
                 [[29,0],[29,1],[29,2],[29,3],[29,4],[29,5],[29,6],[29,7],  [29,8],[29,9],[29,10],[29,11],[29,12],[29,13],[29,14],[29,15]], # Slice 29, TX Source for each lane's RX

                 [[30,0],[30,1],[30,2],[30,3],[30,4],[30,5],[30,6],[30,7],  [30,8],[30,9],[30,10],[30,11],[30,12],[30,13],[30,14],[30,15]], # Slice 30, TX Source for each lane's RX
                 [[31,0],[31,1],[31,2],[31,3],[31,4],[31,5],[31,6],[31,7],  [31,8],[31,9],[31,10],[31,11],[31,12],[31,13],[31,14],[31,15]]] # Slice 31, TX Source for each lane's RX


########################## SMK Board's RX and TX Polarities for Slice 0 and 1
RxPolarityMap = []
TxPolarityMap = []
for i in range(32): RxPolarityMap.append([]);
         #Slice  [A0.............A7, B0,..........B7]
RxPolarityMap[0]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 0 lanes, RX Polarity
RxPolarityMap[1]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 1 lanes, RX Polarity
RxPolarityMap[2]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 2 lanes, RX Polarity
RxPolarityMap[3]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 3 lanes, RX Polarity
RxPolarityMap[4]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 4 lanes, RX Polarity
RxPolarityMap[5]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 5 lanes, RX Polarity
RxPolarityMap[6]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 6 lanes, RX Polarity
RxPolarityMap[7]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 7 lanes, RX Polarity
RxPolarityMap[8]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 8 lanes, RX Polarity
RxPolarityMap[9]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 9 lanes, RX Polarity
RxPolarityMap[10]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 10 lanes, RX Polarity
RxPolarityMap[11]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 11 lanes, RX Polarity
RxPolarityMap[12]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 12 lanes, RX Polarity
RxPolarityMap[13]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 13 lanes, RX Polarity
RxPolarityMap[14]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 14 lanes, RX Polarity
RxPolarityMap[15]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 15 lanes, RX Polarity
RxPolarityMap[16]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 16 lanes, RX Polarity
RxPolarityMap[17]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 17 lanes, RX Polarity
RxPolarityMap[18]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 18 lanes, RX Polarity
RxPolarityMap[19]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 19 lanes, RX Polarity
RxPolarityMap[20]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 20 lanes, RX Polarity
RxPolarityMap[21]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 21 lanes, RX Polarity
RxPolarityMap[22]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 22 lanes, RX Polarity
RxPolarityMap[23]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 23 lanes, RX Polarity
RxPolarityMap[24]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 24 lanes, RX Polarity
RxPolarityMap[25]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 25 lanes, RX Polarity
RxPolarityMap[26]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 26 lanes, RX Polarity
RxPolarityMap[27]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 27 lanes, RX Polarity
RxPolarityMap[28]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 28 lanes, RX Polarity
RxPolarityMap[29]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 29 lanes, RX Polarity
RxPolarityMap[30]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 30 lanes, RX Polarity
RxPolarityMap[31]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 31 lanes, RX Polarity

for i in range(32): TxPolarityMap.append([]);
         #Slice  [A0.............A7, B0,..........B7]
TxPolarityMap[0]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 0 lanes, TX Polarity
TxPolarityMap[1]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 1 lanes, TX Polarity
TxPolarityMap[2]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 2 lanes, TX Polarity
TxPolarityMap[3]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 3 lanes, TX Polarity
TxPolarityMap[4]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 4 lanes, TX Polarity
TxPolarityMap[5]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 5 lanes, TX Polarity
TxPolarityMap[6]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 6 lanes, TX Polarity
TxPolarityMap[7]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 7 lanes, TX Polarity
TxPolarityMap[8]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 8 lanes, TX Polarity
TxPolarityMap[9]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 9 lanes, TX Polarity
TxPolarityMap[10]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 10 lanes, TX Polarity
TxPolarityMap[11]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 11 lanes, TX Polarity
TxPolarityMap[12]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 12 lanes, TX Polarity
TxPolarityMap[13]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 13 lanes, TX Polarity
TxPolarityMap[14]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 14 lanes, TX Polarity
TxPolarityMap[15]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 15 lanes, TX Polarity
TxPolarityMap[16]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 16 lanes, TX Polarity
TxPolarityMap[17]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 17 lanes, TX Polarity
TxPolarityMap[18]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 18 lanes, TX Polarity
TxPolarityMap[19]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 19 lanes, TX Polarity
TxPolarityMap[20]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 20 lanes, TX Polarity
TxPolarityMap[21]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 21 lanes, TX Polarity
TxPolarityMap[22]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 22 lanes, TX Polarity
TxPolarityMap[23]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 23 lanes, TX Polarity
TxPolarityMap[24]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 24 lanes, TX Polarity
TxPolarityMap[25]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 25 lanes, TX Polarity
TxPolarityMap[26]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 26 lanes, TX Polarity
TxPolarityMap[27]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 27 lanes, TX Polarity
TxPolarityMap[28]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 28 lanes, TX Polarity
TxPolarityMap[29]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 29 lanes, TX Polarity
TxPolarityMap[30]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 30 lanes, TX Polarity
TxPolarityMap[31]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1] # Slice 31 lanes, TX Polarity


########################## Channel Estimates per lane per Slice
gChanEst = []
for i in range(32): gChanEst.append([]);
         # [Chan Est, OF, HF]
gChanEst[0]=[[0.0,0,0]]*16 # Slice 0, Channel Estimates for each lane's RX
gChanEst[1]=[[0.0,0,0]]*16 # Slice 1, Channel Estimates for each lane's RX
gChanEst[2]=[[0.0,0,0]]*16 # Slice 2, Channel Estimates for each lane's RX
gChanEst[3]=[[0.0,0,0]]*16 # Slice 3, Channel Estimates for each lane's RX
gChanEst[4]=[[0.0,0,0]]*16 # Slice 4, Channel Estimates for each lane's RX
gChanEst[5]=[[0.0,0,0]]*16 # Slice 5, Channel Estimates for each lane's RX
gChanEst[6]=[[0.0,0,0]]*16 # Slice 6, Channel Estimates for each lane's RX
gChanEst[7]=[[0.0,0,0]]*16 # Slice 7, Channel Estimates for each lane's RX
gChanEst[8]=[[0.0,0,0]]*16 # Slice 8, Channel Estimates for each lane's RX
gChanEst[9]=[[0.0,0,0]]*16 # Slice 9, Channel Estimates for each lane's RX
gChanEst[10]=[[0.0,0,0]]*16 # Slice 10, Channel Estimates for each lane's RX
gChanEst[11]=[[0.0,0,0]]*16 # Slice 11, Channel Estimates for each lane's RX
gChanEst[12]=[[0.0,0,0]]*16 # Slice 12, Channel Estimates for each lane's RX
gChanEst[13]=[[0.0,0,0]]*16 # Slice 13, Channel Estimates for each lane's RX
gChanEst[14]=[[0.0,0,0]]*16 # Slice 14, Channel Estimates for each lane's RX
gChanEst[15]=[[0.0,0,0]]*16 # Slice 15, Channel Estimates for each lane's RX
gChanEst[16]=[[0.0,0,0]]*16 # Slice 16, Channel Estimates for each lane's RX
gChanEst[17]=[[0.0,0,0]]*16 # Slice 17, Channel Estimates for each lane's RX
gChanEst[18]=[[0.0,0,0]]*16 # Slice 18, Channel Estimates for each lane's RX
gChanEst[19]=[[0.0,0,0]]*16 # Slice 19, Channel Estimates for each lane's RX
gChanEst[20]=[[0.0,0,0]]*16 # Slice 20, Channel Estimates for each lane's RX
gChanEst[21]=[[0.0,0,0]]*16 # Slice 21, Channel Estimates for each lane's RX
gChanEst[22]=[[0.0,0,0]]*16 # Slice 22, Channel Estimates for each lane's RX
gChanEst[23]=[[0.0,0,0]]*16 # Slice 23, Channel Estimates for each lane's RX
gChanEst[24]=[[0.0,0,0]]*16 # Slice 24, Channel Estimates for each lane's RX
gChanEst[25]=[[0.0,0,0]]*16 # Slice 25, Channel Estimates for each lane's RX
gChanEst[26]=[[0.0,0,0]]*16 # Slice 26, Channel Estimates for each lane's RX
gChanEst[27]=[[0.0,0,0]]*16 # Slice 27, Channel Estimates for each lane's RX
gChanEst[28]=[[0.0,0,0]]*16 # Slice 28, Channel Estimates for each lane's RX
gChanEst[29]=[[0.0,0,0]]*16 # Slice 29, Channel Estimates for each lane's RX
gChanEst[30]=[[0.0,0,0]]*16 # Slice 30, Channel Estimates for each lane's RX
gChanEst[31]=[[0.0,0,0]]*16 # Slice 31, Channel Estimates for each lane's RX


########################## PRBS and BER Statistics per lane per Slice
gLaneStats = []
for i in range(32): gLaneStats.append([]);
                 #[PrbsCount, PrbsCount-1, PrbsCount-2, PrbsRstTime, PrbsLastReadoutTime]
gLaneStats[0]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 0, PRBS Statistics for each lane's RX
gLaneStats[1]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 1, PRBS Statistics for each lane's RX
gLaneStats[2]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 2, PRBS Statistics for each lane's RX
gLaneStats[3]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 3, PRBS Statistics for each lane's RX
gLaneStats[4]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 4, PRBS Statistics for each lane's RX
gLaneStats[5]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 5, PRBS Statistics for each lane's RX
gLaneStats[6]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 6, PRBS Statistics for each lane's RX
gLaneStats[7]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 7, PRBS Statistics for each lane's RX
gLaneStats[8]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 8, PRBS Statistics for each lane's RX
gLaneStats[9]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 9, PRBS Statistics for each lane's RX
gLaneStats[10]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 10, PRBS Statistics for each lane's RX
gLaneStats[11]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 11, PRBS Statistics for each lane's RX
gLaneStats[12]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 12, PRBS Statistics for each lane's RX
gLaneStats[13]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 13, PRBS Statistics for each lane's RX
gLaneStats[14]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 14, PRBS Statistics for each lane's RX
gLaneStats[15]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 15, PRBS Statistics for each lane's RX
gLaneStats[16]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 16, PRBS Statistics for each lane's RX
gLaneStats[17]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 17, PRBS Statistics for each lane's RX
gLaneStats[18]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 18, PRBS Statistics for each lane's RX
gLaneStats[19]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 19, PRBS Statistics for each lane's RX
gLaneStats[20]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 20, PRBS Statistics for each lane's RX
gLaneStats[21]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 21, PRBS Statistics for each lane's RX
gLaneStats[22]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 22, PRBS Statistics for each lane's RX
gLaneStats[23]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 23, PRBS Statistics for each lane's RX
gLaneStats[24]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 24, PRBS Statistics for each lane's RX
gLaneStats[25]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 25, PRBS Statistics for each lane's RX
gLaneStats[26]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 26, PRBS Statistics for each lane's RX
gLaneStats[27]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 27, PRBS Statistics for each lane's RX
gLaneStats[28]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 28, PRBS Statistics for each lane's RX
gLaneStats[29]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 29, PRBS Statistics for each lane's RX
gLaneStats[30]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 30, PRBS Statistics for each lane's RX
gLaneStats[31]=[[0,0,0,0,0,0,0,0,0,0,0]]*16 # Slice 31, PRBS Statistics for each lane's RX

########################## Line Encoding mode and Data Rate per lane per Slice
gEncodingMode = []
for i in range(32): gEncodingMode.append([]);
                 #[PAM4/NRZ, 53.125/25.78125/20.625/10.3125]
gEncodingMode[0]=[['pam4', 53.125]]*16 # Slice 0, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[1]=[['pam4', 53.125]]*16 # Slice 1, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[2]=[['pam4', 53.125]]*16 # Slice 2, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[3]=[['pam4', 53.125]]*16 # Slice 3, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[4]=[['pam4', 53.125]]*16 # Slice 4, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[5]=[['pam4', 53.125]]*16 # Slice 5, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[6]=[['pam4', 53.125]]*16 # Slice 6, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[7]=[['pam4', 53.125]]*16 # Slice 7, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[8]=[['pam4', 53.125]]*16 # Slice 8, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[9]=[['pam4', 53.125]]*16 # Slice 9, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[10]=[['pam4', 53.125]]*16 # Slice 10, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[11]=[['pam4', 53.125]]*16 # Slice 11, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[12]=[['pam4', 53.125]]*16 # Slice 12, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[13]=[['pam4', 53.125]]*16 # Slice 13, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[14]=[['pam4', 53.125]]*16 # Slice 14, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[15]=[['pam4', 53.125]]*16 # Slice 15, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[16]=[['pam4', 53.125]]*16 # Slice 16, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[17]=[['pam4', 53.125]]*16 # Slice 17, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[18]=[['pam4', 53.125]]*16 # Slice 18, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[19]=[['pam4', 53.125]]*16 # Slice 19, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[20]=[['pam4', 53.125]]*16 # Slice 20, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[21]=[['pam4', 53.125]]*16 # Slice 21, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[22]=[['pam4', 53.125]]*16 # Slice 22, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[23]=[['pam4', 53.125]]*16 # Slice 23, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[24]=[['pam4', 53.125]]*16 # Slice 24, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[25]=[['pam4', 53.125]]*16 # Slice 25, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[26]=[['pam4', 53.125]]*16 # Slice 26, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[27]=[['pam4', 53.125]]*16 # Slice 27, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[28]=[['pam4', 53.125]]*16 # Slice 28, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[29]=[['pam4', 53.125]]*16 # Slice 29, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[30]=[['pam4', 53.125]]*16 # Slice 30, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz
gEncodingMode[31]=[['pam4', 53.125]]*16 # Slice 31, Data Rate for each lane, will be updated by pll() or init_pam4/init_nrz

gCameo400G_EQ = []
gCameo400G_EQ.append([ \
{'card':8,'chip':1,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':1,'lane':2,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':1,'lane':3,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':1,'lane':6,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':1,'lane':7,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':0,'lane':2,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':0,'lane':3,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':0,'lane':6,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':0,'lane':7,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':1,'lane':2,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':1,'lane':3,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':1,'lane':6,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':1,'lane':7,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':0,'lane':2,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':0,'lane':3,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':0,'lane':6,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':0,'lane':7,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':1,'lane':2,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':1,'lane':3,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':1,'lane':6,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':1,'lane':7,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':0,'lane':2,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':0,'lane':3,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':0,'lane':6,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':0,'lane':7,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':1,'lane':2,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':1,'lane':3,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':1,'lane':6,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':1,'lane':7,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':0,'lane':2,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':0,'lane':3,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':0,'lane':6,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':0,'lane':7,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':1,'lane':2,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':1,'lane':3,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':1,'lane':6,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':1,'lane':7,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':0,'lane':2,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':0,'lane':3,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':0,'lane':6,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':0,'lane':7,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':1,'lane':2,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':1,'lane':3,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':1,'lane':6,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':1,'lane':7,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':0,'lane':2,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':0,'lane':3,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':0,'lane':6,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':0,'lane':7,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':5,'chip':1,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':1,'lane':2,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':1,'lane':3,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':1,'lane':6,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':1,'lane':7,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':0,'lane':2,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':0,'lane':3,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':0,'lane':6,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':0,'lane':7,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':1,'lane':2,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':1,'lane':3,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':1,'lane':6,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':1,'lane':7,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':0,'lane':2,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':0,'lane':3,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':0,'lane':6,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':0,'lane':7,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':1,'lane':2,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':1,'lane':3,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':1,'lane':6,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':1,'lane':7,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':0,'lane':2,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':0,'lane':3,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':0,'lane':6,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':0,'lane':7,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':1,'lane':2,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':1,'lane':3,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':1,'lane':6,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':1,'lane':7,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':0,'lane':2,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':0,'lane':3,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':0,'lane':6,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':0,'lane':7,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':3,'chip':1,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':1,'lane':2,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':1,'lane':3,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':1,'lane':6,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':1,'lane':7,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':0,'lane':2,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':0,'lane':3,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':0,'lane':6,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':0,'lane':7,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':1,'lane':2,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':1,'lane':3,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':1,'lane':6,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':1,'lane':7,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':0,'lane':2,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':0,'lane':3,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':0,'lane':6,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':0,'lane':7,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':1,'lane':2,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':1,'lane':3,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':1,'lane':6,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':1,'lane':7,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':0,'lane':2,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':0,'lane':3,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':0,'lane':6,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':0,'lane':7,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':1,'lane':2,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':1,'lane':3,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':1,'lane':6,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':1,'lane':7,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':0,'lane':2,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':0,'lane':3,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':0,'lane':6,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':0,'lane':7,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':1,'lane':2,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':1,'lane':3,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':1,'lane':6,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':1,'lane':7,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':0,'lane':2,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':0,'lane':3,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':0,'lane':6,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':0,'lane':7,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':1,'lane':2,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':1,'lane':3,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':1,'lane':6,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':1,'lane':7,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':0,'lane':2,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':0,'lane':3,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':0,'lane':6,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':0,'lane':7,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0}])

gCameo100G_EQ = []
gCameo100G_EQ.append([ \
{'card':1,'chip':1,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':1,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':1,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':1,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':1,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':1,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':1,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':1,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':1,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':2,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':2,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':2,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':2,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':2,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':2,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':2,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':2,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':2,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':3,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':3,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':3,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':3,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':3,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':3,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':3,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':3,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':3,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':3,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':3,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':3,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':3,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':3,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':3,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':3,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':4,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':4,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':4,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':4,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':4,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':4,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':4,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':4,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':4,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':4,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':4,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':4,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':4,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':4,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':1,'chip':4,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':1,'chip':4,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':1,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':1,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':1,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':1,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':1,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':1,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':1,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':1,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':1,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':2,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':2,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':2,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':2,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':2,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':2,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':2,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':2,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':2,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':3,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':3,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':3,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':3,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':3,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':3,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':3,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':3,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':3,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':3,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':3,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':3,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':3,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':3,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':3,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':3,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':4,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':4,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':4,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':4,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':4,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':4,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':4,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':4,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':4,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':4,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':4,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':4,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':4,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':4,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':2,'chip':4,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':2,'chip':4,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':1,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':1,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':1,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':1,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':1,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':1,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':1,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':1,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':1,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':2,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':2,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':2,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':2,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':2,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':2,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':2,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':2,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':2,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':3,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':3,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':3,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':3,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':3,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':3,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':3,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':3,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':3,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':3,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':3,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':3,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':3,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':3,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':3,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':3,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':4,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':4,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':4,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':4,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':4,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':4,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':4,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':4,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':4,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':4,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':4,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':4,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':4,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':4,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':3,'chip':4,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':3,'chip':4,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':1,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':1,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':1,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':1,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':1,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':1,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':1,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':1,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':1,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':2,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':2,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':2,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':2,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':2,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':2,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':2,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':2,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':2,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':3,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':3,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':3,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':3,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':3,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':3,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':3,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':3,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':3,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':3,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':3,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':3,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':3,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':3,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':3,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':3,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':4,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':4,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':4,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':4,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':4,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':4,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':4,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':4,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':4,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':4,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':4,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':4,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':4,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':4,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':4,'chip':4,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':4,'chip':4,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':1,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':1,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':1,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':1,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':1,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':1,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':1,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':1,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':1,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':2,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':2,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':2,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':2,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':2,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':2,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':2,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':2,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':2,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':3,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':3,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':3,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':3,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':3,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':3,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':3,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':3,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':3,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':3,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':3,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':3,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':3,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':3,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':3,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':3,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':4,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':4,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':4,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':4,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':4,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':4,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':4,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':4,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':4,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':4,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':4,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':4,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':4,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':4,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':18,'tap4':9,'tap5':0},
{'card':5,'chip':4,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':5,'chip':4,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':1,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':1,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':1,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':1,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':1,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':1,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':1,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':1,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':1,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':2,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':2,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':2,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':2,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':2,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':2,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':2,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':2,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':2,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':3,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':3,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':3,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':3,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':3,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':3,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':3,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':3,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':3,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':3,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':3,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':3,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':3,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':3,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':3,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':3,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':4,'slice':1,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':4,'slice':1,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':4,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':4,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':4,'slice':1,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':4,'slice':1,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':4,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':4,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':4,'slice':0,'lane':0,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':4,'slice':0,'lane':1,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':4,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':4,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':4,'slice':0,'lane':4,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':4,'slice':0,'lane':5,'tap1':0,'tap2':-4,'tap3':19,'tap4':8,'tap5':0},
{'card':6,'chip':4,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':6,'chip':4,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':1,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':1,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':1,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':1,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':1,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':1,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':1,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':1,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':1,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':2,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':2,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':2,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':2,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':2,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':2,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':2,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':2,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':2,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':3,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':3,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':3,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':3,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':3,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':3,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':3,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':3,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':3,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':3,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':3,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':3,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':3,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':3,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':3,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':3,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':4,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':4,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':4,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':4,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':4,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':4,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':4,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':4,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':4,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':4,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':4,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':4,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':4,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':4,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':7,'chip':4,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':7,'chip':4,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':1,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':1,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':1,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':1,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':1,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':1,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':1,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':1,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':1,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':2,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':2,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':2,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':2,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':2,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':2,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':2,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':2,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':2,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':3,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':3,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':3,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':3,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':3,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':3,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':3,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':3,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':3,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':3,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':3,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':3,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':3,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':3,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':3,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':3,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':4,'slice':1,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':4,'slice':1,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':4,'slice':1,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':4,'slice':1,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':4,'slice':1,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':4,'slice':1,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':4,'slice':1,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':4,'slice':1,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':4,'slice':0,'lane':0,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':4,'slice':0,'lane':1,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':4,'slice':0,'lane':2,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':4,'slice':0,'lane':3,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':4,'slice':0,'lane':4,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':4,'slice':0,'lane':5,'tap1':0,'tap2':-3,'tap3':20,'tap4':8,'tap5':0},
{'card':8,'chip':4,'slice':0,'lane':6,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0},
{'card':8,'chip':4,'slice':0,'lane':7,'tap1':0,'tap2':0,'tap3':0,'tap4':0,'tap5':0}])

####################################################################################################
def slice_power_up_init(slice=0):
    
    slices = get_slice_list(slice)
    need_to_load_fw = False
    
    if slice == [0,1]: 
        hard_reset()
    
    for slc in slices:
        print("\n...Slice %d Power-Up-Initialization Started..."%slc),
        sel_slice(slc)
        slice_reset()
        set_bandgap('on', range(16)) 
        set_top_pll(pll_side='both', freq=195.3125)
        pll_cal_top()
        print (get_top_pll())
        if not fw_loaded():
            need_to_load_fw = True
    if need_to_load_fw:
        fw_load(slice=slices)


    return slices

def convert_to_loopback():
    fw_config_cmd(config_cmd=0x9090,config_detail=0x0000) #Destroy Gearbox mode [A0:A1] to [B0:B3] 
    fw_config_cmd(config_cmd=0x9091,config_detail=0x0000) #Destroy Gearbox mode [A2:A3] to [B4:B7]
    for ln in range(4):
        fw_config_cmd(config_cmd=0x80F0+ln,config_detail=0x0000) # Set [A0:A3] as PAM4 loopback mode
    for ln in range(8,16):
        fw_config_cmd(config_cmd=0x80E0+ln,config_detail=0x0000) # Set [B0:B7] as NRZ loopback mode
        
def convert_to_gearbox():
    for ln in range(4):
        fw_config_cmd(config_cmd=0x90F0+ln,config_detail=0x0000) # Destroy [A0:A3] PAM4 loopback mode
    for ln in range(8,16):
        fw_config_cmd(config_cmd=0x90E0+ln,config_detail=0x0000) # Destroy [B0:B7] NRZ loopback mode
    fw_config_cmd(config_cmd=0x8090,config_detail=0x0000) #Set Gearbox mode [A0:A1] to [B0:B3] 
    fw_config_cmd(config_cmd=0x8091,config_detail=0x0000) #Set Gearbox mode [A2:A3] to [B4:B7] 

def check_gearbox():
    slices=[2,3,6,7,14,15,30,31]
    for idx in range(8):
        sel_slice(slices[idx])
        print ("Slice: " + str(slices[idx]) + ", " +str(MdioRdh(0)))

####################################################################################################
# config_baldeagle(slice=[0,1], mode='nrz-28G', input_mode='ac', lane='all', cross_mode=False, chip_reset=True)
# config_baldeagle(slice=[0,1], mode='nrz-28G-RETIMER', input_mode='ac', lane='all', cross_mode=False, chip_reset=True)
#
#
####################################################################################################
def config_baldeagle(slice=[0,1], mode='nrz', input_mode='ac', lane='all', cross_mode=False, chip_reset=True):
    
    lanes = get_lane_list(lane) 
    if 'RETIME' in mode.upper():
        all_lanes = get_retimer_lane_list(lane, cross_mode)
    else: # PHY Mode
        all_lanes = lanes 
    global gLane; gLane=all_lanes
    
    if chip_reset==True:
        slices = slice_power_up_init(slice)
    else:
        slices = get_slice_list(slice)

    #auto_pol_en=0


    for slc in slices:
        sel_slice(slc)
        init_lane_for_fw (mode=mode,input_mode=input_mode,lane=all_lanes) 

    for slc in slices:
        sel_slice(slc)
        #fw_reg(115,0xffff)  # Disable FFE Adaptation
        #reg(0x20,0x380)     # CDR BW Kp=6
        #reg(0x1d6,0xcc80)   # az short pulse
        #reg(0x7b,0x0004)    # Enable BLWC
        if 'RETIMER' in mode.upper(): # if Retimer Mode
            prbs_mode_select(lane=all_lanes, prbs_mode='functional')
            fw_config_retimer (mode=mode, lane=lanes, cross_mode=cross_mode)
            #fw_config_wait (max_wait=None, lane=all_lanes, print_en=0)
        elif 'LOOPBACK' in mode.upper(): # if Retimer Mode
            prbs_mode_select(lane=all_lanes, prbs_mode='functional')
            fw_config_loopback (mode=mode, lane=lanes,print_en=0)
            for ln in range(16):
                prbs_mode_select(lane=ln,prbs_mode='prbs')
                tx_prbs_mode(lane=ln,patt='PRBS31')
                rx_prbs_mode(lane=ln,patt='PRBS31')
                msblsb(lane=ln,print_en=0,rx_msblsb=0,tx_msblsb=0)
            #fw_config_wait (max_wait=None, lane=all_lanes, print_en=0)
        else: # Phy mode or OFF mode  
            fw_config_lane (mode=mode,lane=all_lanes)
            for ln in range(16):
                prbs_mode_select(lane=ln,prbs_mode='prbs')
                tx_prbs_mode(lane=ln,patt='PRBS31')
                rx_prbs_mode(lane=ln,patt='PRBS31')
                msblsb(lane=ln,print_en=0,rx_msblsb=0,tx_msblsb=0)
            
    #if 'OFF' not in mode.upper():
    #    for slc in slices:
    #        sel_slice(slc)
    #        fw_adapt_wait (max_wait=None, lane=all_lanes, print_en=1)
    #        fw_config_wait(max_wait=None, lane=all_lanes, print_en=1)
        
        #for slc in slices:
        #    sel_slice(slc)
        #    fw_serdes_params(lane=all_lanes)
        
        #if auto_pol_en:
        #    for slc in slices:
        #        sel_slice(slc)
        #        if 'RETIMER' in mode.upper():                
        #            auto_pol(tx_prbs='dis') # Put TX in FUNCTIONAL mode if in RETIMER mode, then run auto RX polarity correction
        #        else:            
        #            auto_pol(tx_prbs='en') # Enable TX PRBS Gen if in PHY mode, then run auto RX polarity correction
                
    # for slc in slices:
        # sel_slice(slc)
        # fw_slice_params(lane='ALL') 

    #for slc in slices:
    #    sel_slice(slc)
    #    if gSltVer>0: slt_mode(slt_ver=gSltVer)        
    #for slc in slices:
    #    sel_slice(slc)
    #    rx_monitor(rst=1,lane=all_lanes)
  
    sel_slice(slices[0])
    
####################################################################################################
# config_pam4_nrz_phy(slice=[2,3,6,7,14,15,30,31], mode='nrz-28G', input_mode='ac', lane='all', cross_mode=False, chip_reset=True)
# config baldeage A lane [0,1,2,3] to Pam4-phy, B lane [8,9,10,11,12,13,14,15] to NRZ-phy
#
#
####################################################################################################
#def config_pam4_nrz_phy(slice=[2,3,6,7,14,15,30,31], mode='nrz', lane=range(4)+range(8,16), cross_mode=False, chip_reset=True):
def config_pam4_nrz_phy(slice=[2,3,6,7,14,15,30,31], mode='nrz', lane=[0,1,2,3,8,9,10,11,12,13,14,15], cross_mode=False, chip_reset=True): 
 
    lanes = get_lane_list(lane) 
    if 'RETIME' in mode.upper():
        all_lanes = get_retimer_lane_list(lane, cross_mode)
    else: # PHY Mode
        all_lanes = lanes 
    global gLane; gLane=all_lanes
    
    if chip_reset==True:
        slices = slice_power_up_init(slice)
    else:
        slices = get_slice_list(slice)

    #auto_pol_en=0

    for slc in slices:
        sel_slice(slc)
        init_lane_for_fw (mode='nrz-phy',input_mode='ac',lane=[8,9,10,11,12,13,14,15]) 
        init_lane_for_fw (mode='pam4-phy',input_mode='dc',lane=[0,1,2,3])


    for slc in slices:
        sel_slice(slc)
        if 'RETIMER' in mode.upper(): # if Retimer Mode
            fw_config_retimer (mode=mode, lane=lanes, cross_mode=cross_mode)
            prbs_mode_select(lane=all_lanes, prbs_mode='functional')
            fw_config_wait (max_wait=None, lane=all_lanes, print_en=0)
        else: # Phy mode or OFF mode  
            #fw_config_lane (mode=mode,lane=all_lanes)
            fw_config_lane (mode='pam4-phy', lane=[0,1,2,3])
            fw_config_lane (mode='nrz-phy', lane=[8,9,10,11,12,13,14,15])
            #prbs_mode_select(lane=all_lanes, prbs_mode='functional')

    if 'OFF' not in mode.upper():
        for slc in slices:
            sel_slice(slc)
            fw_adapt_wait (max_wait=None, lane=all_lanes, print_en=1)
            fw_config_wait(max_wait=None, lane=all_lanes, print_en=1)
        
        for slc in slices:
            sel_slice(slc)
            fw_serdes_params(lane=all_lanes)

        # if auto_pol_en:
            # for slc in slices:
                # sel_slice(slc)
                # if 'RETIMER' in mode.upper():                
                    # auto_pol(tx_prbs='dis') # Put TX in FUNCTIONAL mode if in RETIMER mode, then run auto RX polarity correction
                # else:            
                    # auto_pol(tx_prbs='en') # Enable TX PRBS Gen if in PHY mode, then run auto RX polarity correction
                
    # for slc in slices:
        # sel_slice(slc)
        # fw_slice_params(lane='ALL') 
      
    for slc in slices:
        sel_slice(slc)
        rx_monitor(rst=1,lane=all_lanes)
  
    sel_slice(slices[0])    
    
    
####################################################################################################
def config_active_switchover(mode=None):

    #slc=0


    A_lanes=[0,1,2,3]
    B_lanes0=[8,9,10,11]
    B_lanes1=[12,13,14,15]
    cross_mode = True if mode!=None and 'cross' in mode else False
    if cross_mode==True:
        main_B_lanes    = B_lanes1
        standby_B_lanes = B_lanes0
    else:
        main_B_lanes    = B_lanes0
        standby_B_lanes = B_lanes1
    
    all_lanes= A_lanes + main_B_lanes + standby_B_lanes
    global gLane; gLane=all_lanes
        
    if mode!=None and 'init' in mode.lower():
        #slices = slice_power_up_init(slc)
        init_lane (mode='nrz',lane=all_lanes)
        for ln in range(16): # Set the polarities of the lanes        
            pol(TxPolarityMap[gSlice][ln],RxPolarityMap[gSlice][ln],ln,0)
        fw_reg_wr(9,0xffff)        
        fw_reg_wr(8,0xffff)        
        fw_reg_wr(128,0xffff)        
        fw_reg_wr(115,0)        
        prbs_mode_select(lane= all_lanes, prbs_mode='functional')
        prbs_mode_select(lane= standby_B_lanes, prbs_mode='prbs')
        fw_config_retimer (mode ='nrz', lane = A_lanes, cross_mode = cross_mode)
        fw_config_lane    (mode ='nrz', lane = standby_B_lanes)
        fw_adapt_wait  (max_wait=None, lane=all_lanes, print_en=1)                              
        fw_config_wait (max_wait=None, lane=A_lanes,   print_en=1)    
        fw_serdes_params(lane=all_lanes)            
        fw_slice_params(lane=all_lanes) 
        fw_reg_wr(8,0)
        fw_reg_wr(128,0)
    else:
        #fw_config_lane    (mode ='nrz', lane = standby_B_lanes)
        if mode is None and rreg([0x9855,[15,12]])==0xF:  # Already in Crossed mode, switch to Direct mode
            print("\n Swithing from Cross Mode to Direct Mode")
            cross_mode= False
        else:
            print("\n Swithing from Direct Mode to Cross Mode")
            cross_mode= True
        fw_config_retimer (mode ='nrz', lane = A_lanes, cross_mode = cross_mode)
     
    reg([0x9855,0x985f,0x98d1,0x98d2,0x98d3,0x98d4])
 ##############################################################################
def config_baldeagle_gearbox(slice=[2,3,6,7,14,15,30,31], A_lanes=[0,1,2,3], gearbox_type='100G-1', gearbox_by_fw=True, fec_b_bypass=False):
    #fw_load(gFwFileName)
    if '50' in gearbox_type:
        # For 50G-2 Gearbox mode, 3 options supported for A-Lane groups 
        group0_50G=[0] # A_lanes group 1 -> [A0] <-> [ 8, 9]
        group1_50G=[1] # A_lanes group 2 -> [A1] <-> {10,11]
        group2_50G=[2] # A_lanes group 3 -> [A2] <-> [12,13]
        group3_50G=[3] # A_lanes group 4 -> [A3] <-> [14,15]
        group4_50G=[4] # A_lanes group 4 -> [A4] <-> [12,13]
        group5_50G=[5] # A_lanes group 5 -> [A5] <-> [14,15]
          
        #Determine the corresponding B-Lanes for each group of A-Lanes
        group0_selected=0
        group1_selected=0
        group2_selected=0
        group3_selected=0
        group4_selected=0
        group5_selected=0

        #Determine the corresponding B-Lanes for each group of A-Lanes
        B_lanes=[]
        if all(elem in A_lanes for elem in group0_50G): # If A_lanes contains [0]
            B_lanes+=[ 8, 9]                        
            group0_selected=1
        if all(elem in A_lanes for elem in group1_50G): # If A_lanes contains [1]
            B_lanes+=[10,11]                      
            group1_selected=1
        if all(elem in A_lanes for elem in group2_50G): # If A_lanes contains [2]
            B_lanes+=[12,13]
            group2_selected=1
        if all(elem in A_lanes for elem in group3_50G): # If A_lanes contains [3]
            B_lanes+=[14,15]
            group3_selected=1
        if all(elem in A_lanes for elem in group4_50G): # If A_lanes contains [2]
            B_lanes+=[12,13]
            group4_selected=1
        if all(elem in A_lanes for elem in group5_50G): # If A_lanes contains [3]
            B_lanes+=[14,15]                
            group5_selected=1
        if group0_selected==0 and group1_selected==0 and group2_selected==0 and group3_selected==0 and group4_selected==0 and group5_selected==0:
            print("\n*** 50G-1 Gearbox Setup: Invalid Target A-Lanes specified!"),
            print("\n*** Options: A_lanes=[0]"),
            print("\n***          A_lanes=[1]"),
            print("\n***          A_lanes=[2]"),
            print("\n***          A_lanes=[3]"),
            print("\n***          A_lanes=[4]"),
            print("\n***          A_lanes=[5]"),
            print("\n***          A_lanes=[0,1,2,3]")
            print("\n***          A_lanes=[0,1,4,5]")
            return
    else: # '100' in gearbox_type
        # For 100G-2 Gearbox mode, 3 options supported for A-Lane groups 
        group0_100G=[0,1] # A_lanes group 1 -> [A0,A1] <-> [ 8, 9,10,11]
        group1_100G=[2,3] # A_lanes group 2 -> [A2,A3] <-> [12,13,14,15]
        group2_100G=[4,5] # A_lanes group 3 -> [A4,A5] <-> [12,13,14,15]
        
        #Determine the corresponding B-Lanes for each group of A-Lanes
        #group0_selected=0
        group1_selected=0
        group2_selected=0
        B_lanes=[]
        if all(elem in A_lanes for elem in group0_100G):  # If A_lanes contains [0,1]
            B_lanes+=[8,9,10,11]
            #group0_selected=1
        if all(elem in A_lanes for elem in group1_100G):  # If A_lanes contains [2,3]
            B_lanes+=[12,13,14,15]
            group1_selected=1
        elif all(elem in A_lanes for elem in group2_100G): # If A_lanes contains [4,5]
            B_lanes+=[12,13,14,15]
            group2_selected=1
        if group1_selected==0 and group2_selected==0: # can only select group 1 or 2, not both
            print("\n*** 100G-1 Gearbox Setup: Invalid Target A-Lanes specified!"),
            print("\n*** Options: A_lanes=[0,1]"),
            print("\n***          A_lanes=[2,3]"),
            print("\n***          A_lanes=[4,5]"),
            print("\n***          A_lanes=[0,1,2,3]"),
            print("\n***          A_lanes=[0,1,4,5]")
            return
        
    lanes = get_lane_list(lane=A_lanes+B_lanes)
    global gLane; gLane=lanes    
    slices = slice_power_up_init(slice)
    
    if gearbox_by_fw: # use FW Command to setup and monitor Gearbox
        #fw_reg_wr=(14,gb_fec_test_wait)
        for slc in slices:
            sel_slice(slc)
            init_lane_for_fw(mode='pam4',input_mode='dc',lane=A_lanes)
            init_lane_for_fw(mode='nrz' ,input_mode='ac',lane=B_lanes)
            for ln in range(16):
                pol(TxPolarityMap[gSlice][ln],RxPolarityMap[gSlice][ln],ln,0)
            if '50' in gearbox_type:        
                fw_config_gearbox_50G (A_lanes,fec_b_byp=fec_b_bypass)
            else:
                if 'LT' in gearbox_type:
                    print("Link training Enable!!!!!!!!!!!!")
                    fw_config_gearbox_100G_LT(A_lanes,fec_b_byp=fec_b_bypass)
                else:
                    fw_config_gearbox_100G(A_lanes,fec_b_byp=fec_b_bypass)
            
        for slc in slices:
            sel_slice(slc)
            #start_time=time.time()   # start timer to get gearbox-ready time
            #fw_adapt_wait(max_wait=10, lane=A_lanes+B_lanes, print_en=1)  
            #print("\n...Waiting for FEC Status Lock (fec_wait=%d) ...  "%(fw_reg_rd(14))),

    else: # use FW to adapt lanes and use software to setup and monitor Gearbox
        for slc in slices:
            sel_slice(slc)
            opt_lane (mode='pam4',input_mode='dc',lane=A_lanes)
            opt_lane (mode='nrz' ,input_mode='ac',lane=B_lanes)
            for ln in range(16):
                pol(TxPolarityMap[gSlice][ln],RxPolarityMap[gSlice][ln],ln,0)          
            if '50' in gearbox_type:        
                sw_config_gearbox_50G (A_lanes,fec_b_byp=fec_b_bypass)
            else:
                sw_config_gearbox_100G(A_lanes,fec_b_byp=fec_b_bypass)        
            #start_time=time.time()   # start timer to get gearbox-ready time
        
    #for slc in slices:
    #    sel_slice(slc)
    #    fw_gearbox_wait (max_wait=10, print_en=1)
    #for slc in slices:
    #    sel_slice(slc)
    #    fw_serdes_params(lane=lanes)
    #for slc in slices:
    #    sel_slice(slc)
    #    fec_status()
    #
####################################################################################################
# Initialize and Optimize Baldeagle in BITMUX mode
# 
# This function configures and Optimizes Baldeagle A-lanes in NRZ 20G and B lanes in 10G modes
#
# It assumes QSFP cables connected between: Slice 0: cable between A0-3 to A4-7 
#                                           Slice 0: loopback modules on B0-7 
#                                           Slice 1: cable between A0-3 to A4-7
#                                           Slice 1: loopback modules on B0-7
#
# The main idea behind the bitmux setup is to make sure all three receivers (the one on the A side and two on the B side) are ready together. If one or more are not ready, the traffic on both directions (A-to-B and B-to-A) stops.
#
# The bitmux setup sequence:
# 1. Call any software init to the lanes going to be set up in bitmux mode (do not call software init to the lanes from this step forward)
# 2. Enable Bitmux mode configuration
# 3. Issue Configure Commands to the lanes, 20G and 10G (and start adaptation)
# 4. while (!rxAdaptationComplete ) ( timeout = 2sec ) --- Stay in the while loop until all three lanes adaptations done
#      Keep PRBS Generator ON (this is needed if Credo is TX source, such as loopback module or cable between two ports)
#      On timeout, issue HW lane-reset (or go back to step 3 and reconfigure the lanes) 
# 5. All three lanes adaptation complete ---> Disable PRBS Generator
#
####################################################################################################
def config_baldeagle_bitmux(slice=[0,1], A_lanes=[0,1,2,3], bitmux_type='20G',print_en=1, chip_reset=True):

    sel_slice(slice)
    global gLane
    
    #auto_pol_en=0
    global RxPolarityMap; RxPolarityMap=[]
                                       #Slice  [A0.............A7, B0,..........B7]
    RxPolarityMap.append([]); RxPolarityMap[0]=[ 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0 ] # Slice 0 lanes, RX Polarity
    RxPolarityMap.append([]); RxPolarityMap[1]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0] # Slice 1 lanes, RX Polarity

    #### edit by Jeff for Falcon ####
    #RxPolarityMap.append([]); RxPolarityMap[0]=[1,1,0,0,1,1,0,0,   0,1,1,0,0,1,1,0] # Slice 0 lanes, RX Polarity
    #RxPolarityMap.append([]); RxPolarityMap[1]=[0,0,0,0,0,0,0,0,   0,1,0,0,1,1,1,0] # Slice 1 lanes, RX Polarity
    #TxPolarityMap.append([]); TxPolarityMap[0]=[0,0,0,0,0,0,0,0,   0,1,1,1,0,0,0,0] # Slice 0 lanes, TX Polarity
    #TxPolarityMap.append([]); TxPolarityMap[1]=[1,0,1,0,1,0,1,0,   0,1,1,0,0,0,0,1] # Slice 1 lanes, TX Polarity
    
    #for ln in gLane: # Set Polarity of each lane according the pol_list
        #pol(TxPolarityMap[gSlice][ln],RxPolarityMap[gSlice][ln],ln,0)
    # For Bitmux mode, 3 options supported for A-Lane groups 
    group1_bitmux=[0,1] # A_lanes group 1 -> [A0,A1] <-> [B0,B1,B2,B3]
    group2_bitmux=[2,3] # A_lanes group 2 -> [A2,A3] <-> [B4,B5,B6,B7]
    group3_bitmux=[4,5] # A_lanes group 3 -> [A4,A5] <-> [B4,B5,B6,B7]
    #Determine the corresponding B-Lanes for each group of A-Lanes
    group1_selected=0
    group2_selected=0
    B_lanes=[]
    if all(elem in A_lanes for elem in group1_bitmux):  # If A_lanes contains [0,1]
        B_lanes +=[8,9,10,11]
        group1_selected=1
    if all(elem in A_lanes for elem in group2_bitmux):  # If A_lanes contains [2,3]
        B_lanes+=[12,13,14,15]
        group2_selected=1
    elif all(elem in A_lanes for elem in group3_bitmux): # If A_lanes contains [4,5]
        B_lanes+=[12,13,14,15]
        group2_selected=1
    if group1_selected==0 and group2_selected==0:
        print("\n*** Bitmux Setup: Invalid Target A-Lanes specified!"),
        print("\n*** Options: A_lanes=[0,1]"),
        print("\n***          A_lanes=[2,3]"),
        print("\n***          A_lanes=[4,5]"),
        print("\n***          A_lanes=[0,1,2,3]"),
        print("\n***          A_lanes=[0,1,4,5]"),
        return
        
    lanes = get_lane_list(lane=A_lanes+B_lanes)
    gLane=lanes    
    if chip_reset==True:
        slice_power_up_init(slice) #slices = slice_power_up_init(slice)
    # else:
        # slices = get_slice_list(slice)

    #init_lane (mode='nrz20',input_mode='ac',lane=A_lanes)
    #init_lane (mode='nrz10',input_mode='ac',lane=B_lanes)
    #global gPrbsEn; gPrbsEn=False
    if '53' in bitmux_type:        
        init_lane_for_fw('pam4',input_mode='ac',lane=A_lanes)
    elif '51' in bitmux_type:
        init_lane_for_fw('pam4',input_mode='ac',lane=A_lanes)
    else:
        init_lane_for_fw('nrz',input_mode='ac',lane=A_lanes)
        
    init_lane_for_fw('nrz',input_mode='ac',lane=B_lanes)

    #for ln in lanes: # Set Polarity of each lane according the pol_list
    for ln in range(16):
        for Slices in range(slice):
            pol(TxPolarityMap[Slices][ln],RxPolarityMap[Slices][ln],ln,0)

    prbs_mode_select(lane=lanes, prbs_mode='functional')

    if '53' in bitmux_type:        
        fw_config_bitmux_53G(A_lanes)
    elif '51' in bitmux_type:
        fw_config_bitmux_51G(A_lanes)
    else:
        fw_config_bitmux_20G(A_lanes)
    
    prbs_mode_select(lane=lanes, prbs_mode='functional')
    #pll_cap(81,150,lane=14)
    fw_bitmux_wait(lane=A_lanes, max_wait=10, print_en=print_en)

    prbs_mode_select(lane=lanes, prbs_mode='functional')
    #if auto_pol_en: auto_pol(tx_prbs='dis') # Do not enable TX PRBS Gen if in RETIMER mode, for auto polarity correction
    fw_serdes_params(lane=lanes)
    rx_monitor(rst=1,lane=lanes)

####################################################################################################
def opt_tx_taps(slice=0, mode='nrz', input_mode='ac', lane=None):
    global gLaneStats #[per Slice][per lane], [PrbsCount, PrbsCount-1,PrbsCount-2, PrbsRstTime, PrbsLastReadoutTime]
    pre2 = 2
    pre1 = -8
    main = 17
    post1 = 0
    post2 = 0
    tx_taps(pre2, pre1, main, post1, post2)
    Eye_best = eye_pam4(lane)
    Ber_best, PostBerBest = ber(lane,rst=1,t=5)[lane]
    pre2_best = pre2
    pre1_best = pre1
    main_best = main
    post1_best = post1
    post2_best = post2
    #pre2_next = pre2
    pre1_next = pre1
    main_next = main
    post1_next = post1
    post2_next = post2
    
    for x in range(-6,6): #pre2 range
        tx_taps(x, pre1_next, main_next, post1_next, post2_next)
        if mode == 'pam4':
            Eye_post = eye_pam4(lane)
        elif mode == 'nrz':
            Eye_post =  eye_nrz(lane)
        Ber_post, PostFecBer = ber(lane,rst=1,t=5)[lane]    
        serdes_params(lane)  
        time.sleep(1)            
        if (Eye_post > Eye_best and Ber_post < Ber_best):
            Eye_best = Eye_post
            Ber_best = Ber_post
            pre2_best = x
        print('\nBest pre 2 setting so far is : %d' %pre2_best)
        pre2_next = x
        #serdes_params()
        #time.sleep(1)
        
        for y in range(-12,0): #pre1 range
            tx_taps(pre2_next, y, main_next, post1_next, post2_next)
            if mode == 'pam4':
                Eye_post = eye_pam4(lane)
            elif mode == 'nrz':
                Eye_post =  eye_nrz(lane)
            Ber_post, PostFecBer = ber(lane,rst=1,t=5)[lane]    
            serdes_params(lane)  
            time.sleep(1)            
            if (Eye_post > Eye_best and Ber_post < Ber_best):
                Eye_best = Eye_post
                Ber_best = Ber_post
                pre1_best = y
            print('\nBest pre 1 setting so far is : %d' %pre1_best)
            pre1_next = y
            #serdes_params()
            #time.sleep(1)   
            
            for z in range(-16,21): #main range
                tx_taps(pre2_next, pre1_next, z, post1_next, post2_next)
                if mode == 'pam4':
                    Eye_post = eye_pam4(lane)
                elif mode == 'nrz':
                    Eye_post =  eye_nrz(lane)
                Ber_post,PostFecBer = ber(lane,rst=1,t=5)[lane]    
                serdes_params(lane)  
                time.sleep(1)            
                if (Eye_post > Eye_best and Ber_post < Ber_best):
                    Eye_best = Eye_post
                    Ber_best = Ber_post
                    main_best = z
                print('\nBest main setting so far is : %d' %main_best)
                main_next = z
                #serdes_params()
                #time.sleep(1)            
            
                for p in range(-7,0): #post1 range
                    tx_taps(pre2_next, pre1_next, main_next, p, post2)
                    if mode == 'pam4':
                        Eye_post = eye_pam4(lane)
                    elif mode == 'nrz':
                        Eye_post =  eye_nrz(lane)
                    Ber_post,PostFecBer = ber(lane,rst=1,t=5)[lane]    
                    serdes_params(lane)  
                    time.sleep(1)            
                    if (Eye_post > Eye_best and Ber_post < Ber_best):
                        Eye_best = Eye_post
                        Ber_best = Ber_post
                        post1_best = p
                    print('\nBest post1 setting so far is : %d' %post1_best)
                    post1_next = p
                        #time.sleep(1)
                
                    for q in range(-3,3): #post2 range
                        tx_taps(pre2_next, pre1_next, main_next, post1_next, q)
                        if mode == 'pam4':
                            Eye_post = eye_pam4(lane)
                        elif mode == 'nrz':
                            Eye_post =  eye_nrz(lane)
                        Ber_post,PostFecBer = ber(lane,rst=1,t=5)[lane]    
                        serdes_params(lane)  
                        time.sleep(1)            
                        if (Eye_post > Eye_best and Ber_post < Ber_best):
                            Eye_best = Eye_post
                            Ber_best = Ber_post
                            post2_best = q
                        print('\nBest post2 setting so far is : %d' %post2_best)
                        post2_next = q
                
    print ('\nOptimal precursor 2 value is: %d' %pre2_best)
    print ('\nOptimal precursor 1 value is: %d' %pre1_best)
    print ('\nOptimal main cursor value is: %d' %main_best)
    print ('\nOptimal post cursor 1 value is: %d' %post1_best)
    print ('\nOptimal post cursor 2 value is: %d' %post2_best)
      
    tx_taps(pre2, pre1, main, post1, post2)
    serdes_params(lane)


####################################################################################################
# First step inside a python shell is to establish connection 
# to Credo Serdes Eval boards+Dongle+USB Cable to PC
#
# Usage:
#  *** mdio()                 : Show current status of Credo MDIO connection
#
#  *** mdio('connect',0)      : Connect to Credo Slice 0 (on USB port 0)
#  *** mdio('connect',1)      : Connect to Credo Slice 1 (on USB port 0)
#  *** mdio('connect',0,1)    : Connect to Credo Slice 0 (on USB port 1)
#  *** mdio('connect',1,1)    : Connect to Credo Slice 1 (on USB port 1)
#
#  *** mdio('disconnect',0)   : Disconnect MDIO to Credo Slice 0 (on USB port 0)
#  *** mdio('disconnect',1)   : Disconnect MDIO to Credo Slice 1 (on USB port 0)
#  *** mdio('disconnect',0,1) : Disconnect MDIO to Credo Slice 0 (on USB port 1)
#  *** mdio('disconnect',1,1) : Disconnect MDIO to Credo Slice 1 (on USB port 1)
#
####################################################################################################
def mdio(connect=None, Slice=None, usb_port=None):
    time.sleep(0.5)
    global gUsbPort
    global gSlice
    global gMode_sel
    
    #SWAPPING_USB_PORT=0
    SWAPPING_SLICE=0
                    
    if (connect is None): # only checking MDIO connection status
        mdio_status = get_mdio_status()
        if (mdio_status == MDIO_DISCONNECTED): 
            print ('\n...Credo MDIO Already Disconnected from USB Port %d Slice %d'%(gUsbPort,gSlice)),
        elif(mdio_status == MDIO_CONNECTED):
            print ('\n...Credo MDIO Already Connected to USB Port %d Slice %d'%(gUsbPort,gSlice)),
        else: # mdio_status = MDIO_LOST_CONNECTION
            val0 = rreg(0x0,0)
            print ('\n***> Credo MDIO Connection is LOST on USB Port %d Slice %d'%(gUsbPort,gSlice)),
            print ('\n***> Reading back invalid value 0x%04X for Credo Registers' % val0),
            print ('\n***> Disconnecting MDIO on USB Port %d Slice %d'%(gUsbPort,gSlice))
            chip.disconnect()
            #mdio_status = MDIO_DISCONNECTED
        
    else: # asked to connect/dis-connect
        if usb_port!=None and usb_port!=gUsbPort:
            #SWAPPING_USB_PORT=1
            gUsbPort=usb_port
        
        if Slice!=None and Slice!=gSlice:
            SWAPPING_SLICE=1
            gSlice=Slice

        if connect == 1 or connect =='connect':
            mdio_status= get_mdio_status()
            if ( SWAPPING_SLICE==0 and mdio_status == MDIO_CONNECTED): # Already connected and reading valid values
                print ('\n...Credo MDIO Already Connected to USB Port %d Slice %d'%(gUsbPort,gSlice)),
            else: # changing Slice or not connected to any Slice yet, issue connect() command
                #if (SWAPPING_SLICE and mdio_status == MDIO_CONNECTED)
                #chip.connect(phy_addr=gSlice, usb_sel=gUsbPort)
                mdio_status= get_mdio_status()
                if (mdio_status == MDIO_CONNECTED): 
                    print ('\n...Credo MDIO Connected to USB Port %d Slice %d'%(gUsbPort,gSlice)),
                else:
                    #val0 = rregLane(0x0)
                    print ('\n***> Credo MDIO Connection Attempt Failed on USB Port %d Slice %d'%(gUsbPort,gSlice)),
                    #print ('\n***> Reading back 0x%04X! for SerDes Registers' % val0),
                    print ('\n***> Disconnecting Python from MDIO on USB Port %d Slice %d'%(gUsbPort,gSlice)),
                    print ('\n***> Fix Dongle Connection Issue and Try again!')
                    chip.disconnect()
                    #mdio_status = MDIO_DISCONNECTED
                    
        elif connect == 0 or connect =='disconnect': # connect == 0 (i.e. Disconnect MDIO)
            #mdio_status = get_mdio_status()
            #if (mdio_status != MDIO_CONNECTED):
                #chip.connect(phy_addr=gSlice, usb_sel=gUsbPort)
            #chip.disconnect()
            print ('\n...Credo MDIO Disconnected from USB Port %d Slice %d'%(gUsbPort,gSlice))
            #mdio_status = MDIO_DISCONNECTED
    time.sleep(0.5)
####################################################################################################
# Select Slice/slicer inside the part to work with.
# Sets Global variable "gSlice" to the selected Slice
#
# Usage:
#  *** sel_slice()   : Show current Slice conencted to. returns 0: Slice 0, 1: Slice 1
#  *** sel_slice(0)  : Connect to Slice/slicer 0 
#  *** sel_slice(1)  : Connect to Slice/slicer 1
####################################################################################################
def sel_slice(slice=None,mdio=None): 
    global gSlice
    global gMode_sel
    global gUsbPort

    if (slice!=None): # asked to connect to a particular Slice inside the same chip, sharing MDIO/I2c bus
        gSlice=slice 
        # try:
            # #chip.connect(phy_addr=slice, usb_sel=gUsbPort,mdio=gMode_sel)
            # gSlice=slice
        # except:
            # #chip.connect(phy_addr=slice, usb_sel=gUsbPort,mdio=gMode_sel)
            # gSlice=slice
                                
    return gSlice
####################################################################################################
def get_mdio_status():
  mdio_status = MDIO_LOST_CONNECTION
  val0 = rreg(0x0,0)
  val1 = rreg(0x1,0)
  
  if ((val0==0xffff or val0==0x7fff) and val1==val0):
    mdio_status = MDIO_LOST_CONNECTION
    
  elif ((val0==0x0001) and val1==val0):
    mdio_status = MDIO_DISCONNECTED
    
  elif (val0!=0x0000 and val0!=0x0001 and val0!=0xffff and val1!=val0): # Already connected and reading valid values
      mdio_status = MDIO_CONNECTED
     
  return mdio_status

####################################################################################################
# 
# Process the specific lane(s) passed 
# and return an integer list of "lanes" to the calling function to loop through list of lanes
#
###################################################################################################
def get_slice_list(slice=None):

    if slice is None:        slices=[gSlice] # single slice download mode on the Slice already selected
    elif type(slice)== int:  slices=[slice]     # single slice download mode 
    elif type(slice)== list: slices=slice       # single slice download mode 
    else:                    slices=[0,1]    # broadcast download mode to both slices
    
    return slices
####################################################################################################
# 
# Process the specific lane(s) passed 
# and return an integer list of "lanes" to the calling function to loop through list of lanes
#
####################################################################################################
def get_lane_list(lane=None):
    if lane is None:          lanes=gLane
    if type(lane)==int:       lanes=[lane]
    elif type(lane)==list:    lanes=lane
    elif type(lane)==str and lane.upper()=='ALL': 
        lanes=list(range(len(lane_name_list)))

    return lanes
####################################################################################################
# register write/read
# intended for engineering mode use only
# specifically made for use in python shell command
# and not be called from other functions
####################################################################################################
def reg(addr, val=None, lane=None,slice=None,print_en=1):

    full_addr=False
    lanes  = get_lane_list(lane)
    slices = get_slice_list(slice)

    if   type(addr)==int:  addr_list=[addr]
    elif type(addr)==list: addr_list=addr

    if addr_list[0]>= 0x1000: #  less than 0x1000 taken as per-lane registers
        full_addr=True
        lanes=[0]
    
    if print_en:
        ##### Print Headers
        print("\nSlice ADDR:"),
        #print("\n ADDR:"),
        if full_addr: 
            print("VALUE"),
        else:
            for ln in lanes:
                print("  %s" %(lane_name_list[ln])),
   
    ##### Write registers first, if value given
    if val!=None:
        for slc in slices:
            sel_slice(slc)
            for add in addr_list:
                for ln in lanes:
                    wreg(add,val,ln)
    ##### Read registers, one address per row
    for add in addr_list:
        for slc in slices:
            sel_slice(slc)
            if print_en:
                if full_addr: print("\n   S%d %04X:" % (gSlice,add) ),
                else:         print("\n   S%d  %03X:"% (gSlice,add) ),
                for ln in lanes:
                    print("%04X" % (rreg(add,ln)) ),
            
####################################################################################################
def rreg(addr, lane = 0):
    '''
    read from a register address, register offset or register field
    '''
    #if lane is None: lane=gLane
    
    #lane_mode_list
    if type(addr) == int:
        if (addr & 0xf000) == 0: addr += (0x7000 + lane*0x200) #addr += lane_offset[lane_name_list[lane]]
        return MdioRd(addr)
    elif type(addr) == list:
        val = 0
        i = 0
        while(i  < (len(addr)-1)):
            full_addr = addr[i]
            if (addr[i] & 0xf000) == 0: full_addr += (0x7000 + lane*0x200)#lane_offset[lane_name_list[lane]]
            val_tmp = MdioRd(full_addr)
            i += 1
            mask = sum([1<<bit for bit in range(addr[i][0], addr[i][-1]-1, -1)])
            val_tmp = (val_tmp & mask)>>addr[i][-1]
            val = (val<<(addr[i][0]-addr[i][-1]+1)) + val_tmp
            i += 1
        return val
    else:
        print("\n***Error reading register***")
        return -1
        
####################################################################################################
def wreg_new(addr, val, lane=None, slice=None):
    #mdio_log_en=0
    lanes  = get_lane_list(lane)
    if lane is None: lanes=[gLane[0],gLane[0]+1]
    slices = get_slice_list(slice)

    if type(addr) == list: # Address and bits given               
        reg_addr = addr[0]
        bit_hi   = addr[1][0]
        bit_lo   = addr[1][-1]
    else: # Address given, but no bit, meaning all bits [15:0]
        reg_addr = addr
        bit_hi   = 15
        bit_lo   = 0
        

    for slc in slices:
        if slice!=None: sel_slice(slc)
        for ln in lanes:            
            if (reg_addr & 0xf000) == 0: # per-lane address
                full_addr = reg_addr + lane_offset[lane_name_list[ln]]
            else:
                full_addr = reg_addr
                
            if type(addr) == int: # Address given, but no bit, meaning all bits [15:0]
                MdioWr(full_addr, val)
                #if mdio_log_en: print("wreg( [0x%04X, [%2d,%2d]], 0x%04X, lane=%2d ) # Slice %d "%(full_addr,bit_hi,bit_lo,val,ln, slc))
            elif type(addr) == list: # Address and bits given               
                curr_val = MdioRd(full_addr)
                mask = sum([1<<bit for bit in range(bit_hi, bit_lo-1, -1)])
                new_val = (curr_val & ~mask) + (val<<bit_lo & mask)
                MdioWr(full_addr, new_val)
                #if mdio_log_en: print("wreg( [0x%04X, [%2d,%2d]], 0x%04X, lane=%2d ) # Slice %d "%(full_addr,bit_hi,bit_lo,val,ln,slc))
            #else:
                #if mdio_log_en: print("wreg( [0x%04X, [15, 0]], 0x%04X, lane=%2d ) # Slice %d #### Error "%(full_addr,val,ln,slc))
            
            if (reg_addr & 0xf000) != 0: # full address given, exit after writing
                break

     
####################################################################################################
def wreg(addr, val, lane = 0):
    '''
    write to a register address, register offset or register field
    '''
    #global chip
    #if lane is None: lane=gLane[0]
    if type(addr) == int:
        if (addr & 0xf000) == 0: addr += (0x7000 + lane*0x200)#lane_offset[lane_name_list[lane]]
        MdioWr(addr, val)
    elif type(addr) == list:
        full_addr = addr[0]
        if (addr[0] & 0xf000) == 0: full_addr += (0x7000 + lane*0x200)#lane_offset[lane_name_list[lane]]
        curr_val = MdioRd(full_addr)
        mask = sum([1<<bit for bit in range(addr[1][0], addr[1][-1]-1, -1)])
        new_val = (curr_val & ~mask) + (val<<addr[1][-1] & mask)
        MdioWr(full_addr, new_val)
    else:
        print("\n***Error writing register***")
     
          
####################################################################################################
def wregBits(addr, bits, val, lane = None):
    '''
    write to a register field
    '''

    addr = [addr, bits]    
    wreg(addr, val, lane)
    
####################################################################################################
def wregMask(addr, mask, val, lane = None):
    '''
    write multiple bit fields, as defined by mask, to a register address
    '''
    #if lane==None: lane=gLane[0]
    #value_old = rreg(addr)
    #value_new = (value_old & ~mask) | (value & mask)
    wreg(addr, value)

####################################################################################################
def rregBits(addr, bits, lane = None):
    '''
    read from a register field
    '''
    if lane is None: lane=gLane[0]
    addr = [addr, bits]
    return rreg(addr, lane)

####################################################################################################
# Similar to the function in main script
# Added lane_reset at the end 
####################################################################################################
def load_setup(filename = None):

    global gSetupFileName
    if filename is None:
        filename=gSetupFileName # Use the same filename used last time save_setup() was called.

    try:
        f=open(filename, 'r')
        script = f.read()
        f.close()
    except IOError:
        print ("\n***Error: Can't Find Register Setup File: '%s' <<<\n" %filename)
        return
    insts = re.findall("^[\da-fA-F]{4}[ ]{1}[\da-fA-F]{4}", script, flags = re.MULTILINE)
    #insts = re.findall("^[\da-fA-F]{4}", script, flags = re.MULTILINE)
    insts = map(lambda t: (int(t[0:4], 16), int(t[5:], 16)), insts) # convert extracted texts to 16-bit integers addr and data
    for inst in insts:
        MdioWr(inst[0], inst[1])
        
    #for lane in lane_name_list:
    #   lane_reset(lane)
    
    print ("\n...Loaded Slice %d Registers from File: %s" %(gSlice,filename)),
     
####################################################################################################
# Similar to the function in main script
# changed the order of addresses saved to be sequential from 0x7000 to 0x8FFF 
####################################################################################################
def save_setup(filename = None, lane=None):
    
    global gSetupFileName
    global gChipName

    if lane is None: 
        lanes = range(0,len(lane_name_list)) # Save all lanes' settings by default
    else:
        lanes = get_lane_list(lane)
  
    try:
        f=open(filename, 'r')
        #script = f.read()
        f.close()
        print ("\n*** A file with the same name already exists. Choose another file name!  <<<<"),
        return
    except IOError:
        print (" ")

    if len(lanes)==1: 
        lanes_str='_Device%d_Lane%d_%s_' %(gSlice,lanes[0],gEncodingMode[gSlice][lanes[0]][0].upper())
    else:
        lanes_str='_Device%d_Lanes%d-%d_%s_' %(gSlice,lanes[0],lanes[-1],gEncodingMode[gSlice][lanes[0]][0].upper())

    timestr = time.strftime("%Y%m%d_%H%M%S")
    if filename is None:
        filename = gChipName + lanes_str + timestr + '.txt'
    gSetupFileName=filename
    log = open(filename, 'w')
    log.write('\n#---------------------------------------------'),
    log.write('\n# File: %s' % filename),
    log.write("\n# %s Rev %2.1f Registers" %(gChipName, chip_rev())),
    log.write("\n# %s" % time.asctime())
    log.write('\n#---------------------------------------------\n'),
    log.close()
    
    ##### Save FW SERDES PARAMS
    log_file = open(filename, 'a+')
    temp_stdout = sys.stdout
    sys.stdout = log_file
    serdes_params(lanes)
    sys.stdout = temp_stdout
    log_file.close()

    if lanes==range(0,len(lane_name_list)): # First, save Top PLL Settings only if all lanes are going to be saved
        reg_group_dump(0x9500, range(0x00, 0x16, 1), 'PLL Fsyn1 registers', filename)
        reg_group_dump(0x9600, range(0x00, 0x16, 1), 'PLL Fsyn0 registers', filename)
    for lane in lanes:
        reg_group_dump(lane_offset[lane_name_list[lane]], range(0x00, 0x1ff+1, 1), 'per lane registers', filename)
        
    reg_group_dump(0x3000, range(0x000,0x3FF, 1), 'Link Training Registers', filename)
    reg_group_dump(0x4000, range(0x000,0xBFF, 1), 'FEC Registers A', filename)
    reg_group_dump(0x5000, range(0x000,0xBFF, 1), 'FEC Registers B', filename)
    reg_group_dump(0x9800, range(0x000,0x0DA, 1), 'Top Registers', filename)
    reg_group_dump(0xB000, range(0x000,0x00F, 1), 'FEC Analyzer Registers', filename)
    reg_group_dump(0xB000, range(0x03F,0x0FF, 1), 'TSensor, VSensor Registers', filename)
    
    ##### Save FW Register values
    log_file = open(filename, 'a+')
    temp_stdout = sys.stdout
    sys.stdout = log_file
    fw_reg()
    sys.stdout = temp_stdout
    log_file.close()

    lanes_str='Slice%d, Lanes %d-%d,' %(gSlice,lanes[0],lanes[-1])
    print ("\n...Saved Slice %s Registers to Setup File: %s" %(lanes_str,filename))


####################################################################################################
# Similar to the function in main script
# changed the order of addresses saved to be sequential from 0x7000 to 0x8FFF 
####################################################################################################   
def reg_group_dump(base_addr, addr_range, addr_name, filename):
    log = open(filename, "a+")
    log.write('\n\n#---------------------------------------------')
    log.write('\n#%s (R%04X to R%04X)' % (addr_name, base_addr + addr_range[0], base_addr + addr_range[-1]) )
    log.write('\n#Addr Value')
    #print('\n%s (R%04X to R%04X)' % (addr_name, base_addr + addr_range[0], base_addr + addr_range[-1]) ),
    log.write('\n#---------------------------------------------\n'),

    for i in addr_range:
        #print("\nR%04X: " % (base_addr + i)),
        #log.write("\n%04X " % (base_addr + i)),
        for j in range(1):
            addr = base_addr + j * 0x100 + i
            val = MdioRd(addr)
            #print(" %04X" % val)
            if addr == 0x9501 or addr == 0x9601:
                val_01 = val
                val = val & 0xfffb
            if addr == 0x9512 or addr == 0x9612:
                val_12 = val
                val = val & 0x7fff
            log.write("%04X %04X\n" % (addr, val))
    if base_addr == 0x9500 or base_addr == 0x9600:
        log.write("%04X %04X # TOP PLL POR TX [15] toggle\n" % (base_addr+0x12, val_12))
    if base_addr == 0x9500 or base_addr == 0x9600:
        log.write("%04X %04X # TOP PLL PU [2] toggle\n" % (base_addr+0x01, val_01))
    log.write("\n")
    log.close()
####################################################################################################
# 
# prints or saves values for same address of each lane in one row, for quick comparison and debugging.
# Example:
#--------------------------------------------------------------------------------------------------
#   ,   A0,   A1,   A2,   A3,   A4,   A5,   A6,   A7,   B0,   B1,   B2,   B3,   B4,   B5,   B6,   B7
#Addr,7000 ,7200 ,7400 ,7600 ,7800 ,7A00 ,7C00 ,7E00 ,8000 ,8200 ,8400 ,8600 ,8800 ,8A00 ,8C00 ,8E00 
#--------------------------------------------------------------------------------------------------
#000 ,106B ,106B ,106B ,106B ,106B ,106B ,106B ,106B ,106B ,106B ,106B ,106B ,106B ,106B ,106B ,106B 
#001 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 
#
# Note, this is a log file for debug only and not suitable to be called by "load_setup()"
# 
# reg_group_dump_debug_mode(range(0x7000,0x8F00+1,0x200), range(0,0x1FF+1, 1), "SerDes Lane Registers")
# reg_group_dump_debug_mode(range(0x3000,0x3F00+1,0x100), range(0,0x063+1, 1), "Link Training Registers")
# reg_group_dump_debug_mode(range(0x9500,0x9600+1,0x100), range(0,0x015+1, 1), "Top PLL Registers")
####################################################################################################   
def reg_group_dump_debug_mode(base_addr_range, lane_addr_range, addr_name, filename=None):
    
    if filename!=None:
        log_file = open(filename, 'a+')
        temp_stdout = sys.stdout
        sys.stdout = log_file

    separator='\n-----'
    # for lane_base_addr in base_addr_range:
        # separator += '------'
        
    ### header
    print('\n %s   (Addr: 0x%04X to 0x%04X)' % (addr_name, base_addr_range[0], base_addr_range[-1]+lane_addr_range[-1]) ),
    print('%s'%separator),
    print("\n     "),
    for lane in lane_name_list:
        print("%4s " % (lane)),   
    print("\nAddr:"),
    # for lane_base_addr in base_addr_range:
        # print("%04X " % (lane_base_addr)),
    print('%s'%separator),
    
    ### Data
    for pre_lane_addr in lane_addr_range:
        print("\n %03X:" % (pre_lane_addr)),
        prev_val = 0xeeeee
        for base_addr in base_addr_range:
            val = MdioRd (base_addr + pre_lane_addr)
            diff= '<' if (base_addr!=base_addr_range[0] and val!=prev_val) else ' '
            print("%04X%s" % (val,diff)),
            prev_val = val
            
    print('%s'%separator),
    print("\n"),
    
    if filename!=None:
        sys.stdout = temp_stdout
        log_file.close()
        #print ("\n...Saved Registers as Debug Setup File: %s" %filename)
####################################################################################################
# 
# saves the same address values for all lanes in one row, for quick comparison and debugging.
# Example:
#--------------------------------------------------------------------------------------------------
#   ,   A0,   A1,   A2,   A3,   A4,   A5,   A6,   A7,   B0,   B1,   B2,   B3,   B4,   B5,   B6,   B7
#Addr,7000 ,7200 ,7400 ,7600 ,7800 ,7A00 ,7C00 ,7E00 ,8000 ,8200 ,8400 ,8600 ,8800 ,8A00 ,8C00 ,8E00 
#--------------------------------------------------------------------------------------------------
#000 ,106B ,106B ,106B ,106B ,106B ,106B ,106B ,106B ,106B ,106B ,106B ,106B ,106B ,106B ,106B ,106B 
#001 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 ,0800 
#
# Note, this is a log file for debug only and not suitable to be called by "load_setup()"
#
####################################################################################################   
def save_setup_debug_mode(filename = None):
    '''
    saves the same address values for all lanes in one row, for quick comparison and debugging.
    '''
    global gChipName

    timestr = time.strftime("%Y%m%d_%H%M%S")
    if filename is None:
        filename = gChipName + '_Reg_Setup_Debug_Mode' + timestr + '.txt'
    log = open(filename, 'w')
    log.write('\n----------------------------------------------------------------------'),
    log.write('\n File: %s' % filename),
    log.write('\n %s Rev %2.1f' % (gChipName, chip_rev(print_en=0))),
    log.write("\n %s" % time.asctime())
    log.write('\n----------------------------------------------------------------------\n\n'),
    log.close()
    
    reg_group_dump_debug_mode(range(0x7000,0x8F00+1,0x200), range(0,0x1FF+1, 1), "SerDes Lane Registers", filename )
    reg_group_dump_debug_mode(range(0x3000,0x3F00+1,0x100), range(0,0x063+1, 1), "Link Training Registers", filename)
    reg_group_dump_debug_mode(range(0x9500,0x9600+1,0x100), range(0,0x015+1, 1), "Top PLL Registers", filename)
    
    ##### Save FW Serdes Params and FW Regiters
    log_file = open(filename, 'a+')
    temp_stdout = sys.stdout
    sys.stdout = log_file
    serdes_params(lane=range(16)) #####
    fw_reg()                      #####
    sys.stdout = temp_stdout
    log_file.close()

    print ("\n...Saved Registers as Debug Setup File: %s" %filename)

####################################################################################################
# 
# Reset Slice completely (ready to be initialized next)
####################################################################################################
def slice_reset(t=0.010):

    #global gSlice
    #chip.connect(phy_addr=Slice); gSlice=Slice
    get_lane_mode('all') # update the Encoding modes of all lanes for this Slice
    soft_reset()
    time.sleep(0.1)
    set_bandgap('on', 'all') 
    set_top_pll(pll_side='both', freq=195.3125)
    all_lanes=range(len(lane_name_list))
    
    # put lanes in PAM4 mode and do a logic reset
    set_lane_mode(mode='pam4',lane=all_lanes)
    time.sleep(0.1)
    logic_reset()
    time.sleep(0.1)    
    
    # put lanes in NRZ mode and do a logic reset
    set_lane_mode(mode='nrz',lane=all_lanes)
    time.sleep(0.1)
    logic_reset()
    time.sleep(0.1)
    
    for ln in range(16):
        ####disable AN/LT registers as default
        wreg(0x3000+(0x100*ln),0x0000,lane=0)
        wreg(0xe000+(0x100*ln),0xc000,lane=0)
        wreg([0x041,[15]], 0,ln) # Toggle PAM4_RX_ENABLE
        wreg([0x041,[15]], 1,ln) # Toggle PAM4_RX_ENABLE
        time.sleep(t)
        wreg([0x041,[15]], 0,ln) # Toggle PAM4_RX_ENABLE
        wreg([0x0b0,[11]], 0,ln) # Toggle NRZ_PRBS_ENABLE
        wreg([0x0b0,[11]], 1,ln) # Toggle NRZ_PRBS_ENABLE
        time.sleep(t)
        wreg([0x0b0,[11]], 0,ln) # Toggle NRZ_PRBS_ENABLE

    fec_reset()
    
   
    #set_bandgap('off', 'all')
    #chip.disconnect()
 
    print("\n...Slice %d is FULLY reset!" % gSlice),

####################################################################################################
def hard_reset():
    print("\n...Chip is Hard Reset!"),
    sel_slice(0); wreg(Pam4Reg.chip_rst_addr, Pam4Reg.chip_cpu_rst_val) # Slice 0, CPU in Reset
    sel_slice(1); wreg(Pam4Reg.chip_rst_addr, Pam4Reg.chip_cpu_rst_val) # Slice 1, CPU in Reset
    #crs('dut','pin_reset', 0) # toggle HW_RST Pin
    time.sleep(0.010)
    sel_slice(0); wreg(Pam4Reg.chip_rst_addr, Pam4Reg.chip_cpu_rst_val) # Slice 0, CPU in Reset
    sel_slice(1); wreg(Pam4Reg.chip_rst_addr, Pam4Reg.chip_cpu_rst_val) # Slice 1, CPU in Reset
    #crs('dut','pin_reset', 0) # toggle HW_RST Pin one more time
    time.sleep(0.010)
    
def soft_reset():
    wreg(Pam4Reg.chip_rst_addr, Pam4Reg.chip_soft_rst_val)
    wreg(Pam4Reg.chip_rst_addr, 0x0)

def logic_reset(lane=None):
    if lane is None: # Logic reset the whole slice
        wreg(Pam4Reg.chip_rst_addr, Pam4Reg.chip_logic_rst_val)
        wreg(Pam4Reg.chip_rst_addr, 0x0)
    else: # Logic reset selected lane(s) only
        lane_logic_reset_addr = 0x980F
        lanes = get_lane_list(lane)
        for ln in lanes:
            wreg(lane_logic_reset_addr,   1<<ln)
        for ln in lanes:
            wreg(lane_logic_reset_addr, ~(1<<ln))
    

def cpu_reset():
    wreg(Pam4Reg.chip_rst_addr, Pam4Reg.chip_cpu_rst_val) 
    wreg(Pam4Reg.chip_rst_addr, 0x0)

def reg_reset():
    wreg(Pam4Reg.chip_rst_addr, Pam4Reg.chip_reg_rst_val) 
    wreg(Pam4Reg.chip_rst_addr, 0x0)
    
####################################################################################################
# FW_LOAD_FILE_INIT()
#
# Downloads FW to one slice or both slices of a chip at the same time
#
# usage:
#       fw_load('1.19.00')           # load FW ver 1.19.00 to slice already selected
#       fw_load('1.19.00', slice=[0])   # load FW ver 1.19.00 to slice 0
#       fw_load('1.19.00', slice=[1])   # load FW ver 1.19.00 to slice 1
#       fw_load('1.19.00', slice=[0,1]) # load FW ver 1.19.00 to both slices in parallel
####################################################################################################
def fw_load_file_init (file_name=None, path_name=None):

    global gFwFileName
    global gFwFileNameLastLoaded
    
    #### Define FW file folder and the filename according to chip revision
    file_path = '/usr/share/DosFirmwareImages/'
    file_name_prefix_rev_1 = 'BE.fw.'
    file_name_prefix_rev_2 = 'BE2.fw.'
    file_name_extension = '.bin'    
    if file_name is None:
        try:
            gFwFileName
        except NameError:
            print("\n*** FW Not loaded. No FW file name is defined ***\n\n")
            return False
        else:
            fw_file_name=gFwFileName
    else:
        if not (file_name_extension in file_name) and not ('.fw.' in file_name): 
            file_name_prefix = file_name_prefix_rev_2 if chip_rev()==2.0 else file_name_prefix_rev_1
            fw_file_name = file_name_prefix + file_name + file_name_extension
        elif not (file_name_extension in file_name) and ('.fw' in file_name): 
            fw_file_name = file_name + file_name_extension
        else:
            fw_file_name = file_name # use the filename exactly as passed to function
    if path_name !=None:    
        fw_file_name = file_path + fw_file_name
    gFwFileNameLastLoaded = fw_file_name # update this. Used by fw_info()
    
    if not os.path.exists(fw_file_name):
        print("\n...FW LOAD ERROR: Error Opening FW File: %s\n" %(fw_file_name))      
        return False
    
    return  fw_file_name         
####################################################################################################
def fw_load_init ():
    ##### Before download, make sure Top PLL is set, cal'ed and FW is Unloaded in each slice
    #for slice_num in slices:
    #sel_slice(slice_num)
    set_top_pll(pll_side='both') # Setup Top PLLs first
    pll_cal_top()                # Calibrate Top PLLs 
    fw_unload(print_en=0)        # Unload FW from each slice
           
####################################################################################################
def fw_load_broadcast_mode (mode='on'):
    
    FW3 = 0x985A # Broadcast-Mode Download control register

    if mode.upper() =='ON':
        wreg(FW3,0x8888)
    else:
        wreg(FW3,0x0000)    
           
####################################################################################################
def fw_load_main (fw_file_name=None,broadcast_mode='OFF',wait=0.001):
    # print_en=0

    ##### FW Download Register addresses
    FW0 = 0x9F00
    FW1 = 0x9802
    FW2 = 0x9805
    
    ##### FW Download starts here
    fw_file_ptr = open(fw_file_name, 'rb')
    fw_data = fw_file_ptr.read()
    start = 4096
    # file_hash_code = struct.unpack_from('>I', fw_data[start   :start+4 ])[0]
    file_crc_code  = struct.unpack_from('>H', fw_data[start+4 :start+6 ])[0]
    #file_date_code = struct.unpack_from('>H', fw_data[start+6 :start+8 ])[0]
    entryPoint     = struct.unpack_from('>I', fw_data[start+8 :start+12])[0]
    length         = struct.unpack_from('>I', fw_data[start+12:start+16])[0]
    ramAddr        = struct.unpack_from('>I', fw_data[start+16:start+20])[0]
    data           = fw_data[start+20:]

    # d=datetime.date(1970, 1, 1) + datetime.timedelta(file_date_code)

    # if print_en: print ("fw_load Hash Code : 0x%06x" % file_hash_code)
    # if print_en: print ("fw_load Date Code : 0x%02x (%04d-%02d-%02d)" % (file_date_code, d.year, d.month, d.day))
    # if print_en: print ("fw_load  CRC Code : 0x%04x" % file_crc_code)
    # if print_en: print ("fw_load    Length : %d" % length)
    # if print_en: print ("fw_load     Entry : 0x%08x" % entryPoint)
    # if print_en: print ("fw_load       RAM : 0x%08x" % ramAddr)

    dataPtr = 0
    sections = (length+23)/24

    ##### FW Unload
    wreg(FW2, 0xFFF0)
    wreg(FW1, 0x0AAA)
    wreg(FW1, 0x0000)    
    
    if broadcast_mode.upper()=='ON': # broadcast download mode, use fixed delay
        time.sleep(.01)              
    else:                # single-slice download mode, status read back
        #start_time=time.time()
        checkTime = 0
        status = rreg(FW2)
        while status != 0:
            status = rreg(FW2)
            checkTime += 1
            if checkTime > 100000:
                print ('\n...FW LOAD ERROR: : Wait for FW2=0 Timed Out! FW2 = 0x%X'% status) #Wait for FW2=0: 0.000432 sec
                break
        #stop_time=time.time()    
    wreg(FW2, 0x0000)
    ##### FW Unload Done
    
    download_start_time=time.time()

    ##### Main FW Download loop
    i = 0
    while i < sections:
        checkSum = 0x800c
        wreg(FW0+12, ramAddr>>16)
        wreg(FW0+13, (ramAddr & 0xFFFF))
        checkSum += ( ramAddr >> 16 ) + ( ramAddr & 0xFFFF )
        for j in range( 12 ):                                               
            if (dataPtr > length):
                mdioData = 0x0000
            else: 
                mdioData = struct.unpack_from('>H', data[dataPtr:dataPtr+2])[0]
            wreg(FW0+j, mdioData)
            checkSum += mdioData
            dataPtr += 2
            ramAddr += 2
 
        wreg(FW0 + 14, (~checkSum+1) & 0xFFFF)
        wreg(FW0 + 15, 0x800C)
        #print '\nDEBUG fw_load: section %d, Checksum %X' % (i, checkSum),
 
        
        if broadcast_mode.upper()=='ON': # broadcast download mode, use fixed delay passed to the function
            time.sleep(wait)      
        else:                # single-slice download mode, status read back per section
            checkTime = 0
            status = rreg(FW0 + 15) 
            while status == 0x800c:
                status = rreg(FW0 + 15)
                checkTime += 1
                if checkTime > 1000:
                    print ('\n...FW LOAD ERROR: Write to Ram Timed Out! FW0 = %x'% status)
                    break
                    
            #if checkTime>0:
                #if print_en: print ('\nfw_load: section %d, CheckTime= %d' % (i, checkTime))

            if status != 0:
                print("\n...FW LOAD ERROR: Invalid Write to RAM, section %d, Status %d"%(i,status))
        i += 1
        print("\b\b\b\b\b%3.0f%%"%(100.0 *i/sections)), # Display % download progressed
    ##### End of Main FW Download loop
    
    ##### Last steps of FW Download
    wreg(FW0 + 12, entryPoint>>16)
    wreg(FW0 + 13, ( entryPoint & 0xFFFF ))
    checkSum = ( entryPoint >> 16 ) + ( entryPoint & 0xFFFF ) + 0x4000
    wreg(FW0 + 14, ( ~checkSum+1 ) & 0xFFFF)
    wreg(FW0 + 15, 0x4000)    
    time.sleep(.1)
    
    download_time = time.time()-download_start_time     
    fw_file_ptr.close()
    
    return download_time, file_crc_code
####################################################################################################
def cameo_fw_unload_all():
    cameo_drv_open() 
    slice=[0,1]
    slices = get_slice_list(slice)    
    for card in range(1,9):
        for num in range(1,5):
            cameo_change_chip_card(num,card)
            mdio_status = get_mdio_status()
            #print("\ncard %d chip %d\n" % (card,num))
            if (mdio_status == MDIO_CONNECTED):    
                for slice_num in slices:
                    sel_slice(slice_num) 
                    fw_unload(print_en=1)
    cameo_drv_close()   

def cameo_fw_unload(card):
    cameo_drv_open() 
    slice=[0,1]
    slices = get_slice_list(slice)    
    for num in range(1,5):
        cameo_change_chip_card(num,card)
        mdio_status = get_mdio_status()
        #print("\ncard %d chip %d\n" % (card,num))
        if (mdio_status == MDIO_CONNECTED):    
            for slice_num in slices:
                sel_slice(slice_num) 
                fw_unload(print_en=1)
    cameo_drv_close()     
    
def cameo_fw_load_status_all() : 
    cameo_drv_open() 
    slice=[0,1]
    slices = get_slice_list(slice)

    print("+--------------------------------------------------+")
    print("| Card | Chip | Slice |   Status   |    Version    |")
    print("+--------------------------------------------------+")    
    for card in range(1,9):
        for num in range(1,5):
            cameo_change_chip_card(num,card)
            mdio_status = get_mdio_status()
               
            if (mdio_status == MDIO_CONNECTED):
                for slice_num in slices:
                    sel_slice(slice_num) 
                    ver_code  = fw_ver(print_en=0) 
                    ver_str    = "VER_%02d.%02d.%02d" %(ver_code>>16&0xFF,ver_code>>8&0xFF,ver_code&0xFF)                    
                    if not fw_loaded(print_en=0):                        
                        print("|   %d  |   %d  |   %d   | Not Loaded | N/A           |" % (card,num,slice_num))
                    else:
                        print("|   %d  |   %d  |   %d   | Loaded     | %s  |" % (card,num,slice_num,ver_str))
            else:
                for slice_num in slices:
                    sel_slice(slice_num)
                    print("|   %d  |   %d  |   %d   | N/A        | N/A           |" % (card,num,slice_num))       
            print("+--------------------------------------------------+")
    cameo_drv_close()            
def fw_load_status (file_crc_code, slice=0):
    ##### Check FW CRC for each slice and confirm a good download
    fw_info(slice=slice)
    crc_code = fw_crc (print_en=0)
    if crc_code != file_crc_code:
        print("\n\n...Slice %d FW LOAD ERROR: CRC Code Not Matching, Expected CRC: 0x%04x -- Actual CRC: 0x%04x\n\n" %(slice,file_crc_code,crc_code))
        return False
    else:
        return True
    #return hash_code, crc_code, date_code
def cameo_drv_open():
    libcameo.lscpcie_open()
    cameo_change_chip_card(1,1)
    sel_slice(0)
def cameo_drv_close():
    libcameo.lscpcie_close()
def cameo_change_chip_card(chip,card):
    if card > 8 or card < 1:
        print ("card out of range")
        return
    if chip > 4 or chip < 1:
        print ("chip out of range")
        return
    #print "change to card %d" % card
    libcameo.cm_sw_phy_card(chip,card)
    sel_slice(0)
def cameo_fw_load_2_card (card=1,file_name=None,path_name=None,slice=[0,1], wait=0.0001):
    if card==2 or card==4 or card==6 or card==8:
        card = card - 1    
    if card>8 or card<1:
        return
    cameo_drv_open()    
    global gMode_sel
    
    slices = get_slice_list(slice)    
   
    cameo_change_chip_card(1,card)
    mdio_status = get_mdio_status()   
    if (mdio_status == MDIO_CONNECTED):
        sel_slice(slices[0])   
        present_card = card
    else:
        cameo_change_chip_card(1,card+1)
        mdio_status = get_mdio_status()
        if (mdio_status == MDIO_CONNECTED):
            sel_slice(slices[0])          
            present_card = card+1
        else:
            print ("Card MDIO fail!")
            return
  
    if gMode_sel== True:
        #if len(slices) > 1:
            broadcast_mode='ON'
            print("\n...Broadcast-Mode Downloading FW to both slices...   "),
    #### 1. Define FW file folder and the filename according to chip revision
    fw_file_name = fw_load_file_init(file_name,path_name)
    if fw_file_name == False:
        return 
        
    ##### 2. Before download, make sure Top PLL is set, cal'ed and FW is Unloaded in each slice
    cameo_change_chip_card(1,card)
    sel_slice(slices[0])
    mdio_status = get_mdio_status()            
    if (mdio_status == MDIO_CONNECTED): 
        for num in range(1,5):
            cameo_change_chip_card(num,card)
            mdio_status = get_mdio_status()            
            if (mdio_status == MDIO_CONNECTED):     
                for slice_num in slices:
                    sel_slice(slice_num) 
                    mdio_status = get_mdio_status()            
                    if (mdio_status == MDIO_CONNECTED):
                        fw_load_init()
                        time.sleep(0.1)
                        if gMode_sel== True:
                            fw_load_broadcast_mode(broadcast_mode)                        
    cameo_change_chip_card(1,card+1)
    sel_slice(slices[0])
    mdio_status = get_mdio_status()            
    if (mdio_status == MDIO_CONNECTED): 
        for num in range(1,5):
            cameo_change_chip_card(num,card+1)
            mdio_status = get_mdio_status()            
            if (mdio_status == MDIO_CONNECTED):     
                for slice_num in slices:
                    sel_slice(slice_num) 
                    mdio_status = get_mdio_status()            
                    if (mdio_status == MDIO_CONNECTED):            
                        fw_load_init()
                        time.sleep(0.1)
                        if gMode_sel== True:
                            fw_load_broadcast_mode(broadcast_mode)                      
    time.sleep(0.5)

    ##### 3. if broadcast FW download  mode, enable it in each slice first before moving forward
    #if gMode_sel== True:
    #    cameo_change_chip_card(1,card)
    #    sel_slice(slices[0])
    #    mdio_status = get_mdio_status()
    #    
    #    if (mdio_status == MDIO_CONNECTED):
    #        for num in range(1,5):
    #            cameo_change_chip_card(num,card)
    #            mdio_status = get_mdio_status()            
    #            if (mdio_status == MDIO_CONNECTED):         
    #                for slice_num in slices:
    #                    sel_slice(slice_num) 
    #                    mdio_status = get_mdio_status()            
    #                    if (mdio_status == MDIO_CONNECTED):                    
    #                        fw_load_broadcast_mode(broadcast_mode)
    #    cameo_change_chip_card(1,card+1)        
    #    sel_slice(slices[0])
    #    mdio_status = get_mdio_status()        
    #    if (mdio_status == MDIO_CONNECTED):
    #        for num in range(1,5):
    #            cameo_change_chip_card(num,card)
    #            mdio_status = get_mdio_status()            
    #            if (mdio_status == MDIO_CONNECTED):         
    #                for slice_num in slices:
    #                    sel_slice(slice_num) 
    #                    mdio_status = get_mdio_status()            
    #                    if (mdio_status == MDIO_CONNECTED):
    #                        print "Set slice %d to bcast" % slice_num
    #                        fw_load_broadcast_mode(broadcast_mode) 

    cameo_change_chip_card(1,present_card)
    ##### 4. FW Download starts here
    if gMode_sel== True:
        sel_slice(slices[0])
        download_time, expected_crc_code = fw_load_main(fw_file_name,broadcast_mode,wait)
    
    else:
        for slice_num in slices:
            sel_slice(slice_num)
            download_time, expected_crc_code = fw_load_main(fw_file_name,broadcast_mode,wait)
    
    ##### 5. if in broadcast FW download mode mode, disable it right away in each slice
    if gMode_sel== True:
        cameo_change_chip_card(1,card)
        sel_slice(slices[0])
        mdio_status = get_mdio_status()
        
        if (mdio_status == MDIO_CONNECTED):
            for num in range(1,5):
                cameo_change_chip_card(num,card)
                mdio_status = get_mdio_status()            
                if (mdio_status == MDIO_CONNECTED):         
                    for slice_num in slices:
                        sel_slice(slice_num) 
                        mdio_status = get_mdio_status()            
                        if (mdio_status == MDIO_CONNECTED):
                            fw_load_broadcast_mode('OFF')
        cameo_change_chip_card(1,card+1)
        sel_slice(slices[0])
        mdio_status = get_mdio_status()
        
        if (mdio_status == MDIO_CONNECTED):
            for num in range(1,5):
                cameo_change_chip_card(num,card+1)
                mdio_status = get_mdio_status()            
                if (mdio_status == MDIO_CONNECTED):         
                    for slice_num in slices:
                        sel_slice(slice_num)
                        mdio_status = get_mdio_status()            
                        if (mdio_status == MDIO_CONNECTED):
                            fw_load_broadcast_mode('OFF') 
                    
    print(' (%2.2f sec)'% (download_time))
    #print("\r"),
    
    ##### 6. Check FW CRC for each slice and confirm a good download
    for slice_num in slices:
        sel_slice(slice_num) 
        fw_load_status(expected_crc_code,slice_num)
       
    sel_slice(slices[0]) # select the first slice before exiting
    cameo_drv_close()
####################################################################################################
# FW_LOAD()
#
# Downloads FW to one slice or both slices of a chip at the same time
#
# usage:
#       fw_load('1.19.00')           # load FW ver 1.19.00 to slice already selected
#       fw_load('1.19.00', slice=[0])   # load FW ver 1.19.00 to slice 0
#       fw_load('1.19.00', slice=[1])   # load FW ver 1.19.00 to slice 1
#       fw_load('1.19.00', slice=[0,1]) # load FW ver 1.19.00 to both slices in parallel
####################################################################################################
def fw_load (file_name=None,path_name=None,slice=[0,1], wait=0.0001):
    global gMode_sel
    
    slices = get_slice_list(slice)


    if gMode_sel== True:
        if len(slices) > 1:
            broadcast_mode='ON'
            print("\n...Broadcast-Mode Downloading FW to both slices...   "),
        else:
            broadcast_mode='OFF'
            print("\n...Downloading FW to slice %d...    "%slices[0]),
    else:
        broadcast_mode ='OFF'
    #### 1. Define FW file folder and the filename according to chip revision
    fw_file_name = fw_load_file_init(file_name,path_name)
    if fw_file_name == False:
        return False 
        
    ##### 2. Before download, make sure Top PLL is set, cal'ed and FW is Unloaded in each slice
    for slice_num in slices:
        sel_slice(slice_num) 
        fw_load_init()
    
    ##### 3. if broadcast FW download  mode, enable it in each slice first before moving forward
    if gMode_sel== True:
        for slice_num in slices:
            sel_slice(slice_num) 
            fw_load_broadcast_mode(broadcast_mode)
        
    ##### 4. FW Download starts here
    if gMode_sel== True:
        sel_slice(slices[0])
        download_time, expected_crc_code = fw_load_main(fw_file_name,broadcast_mode,wait)
    
    else:
        for slice_num in slices:
            sel_slice(slice_num)
            download_time, expected_crc_code = fw_load_main(fw_file_name,broadcast_mode,wait)
    
    ##### 5. if in broadcast FW download mode mode, disable it right away in each slice
    if gMode_sel== True:
        for slice_num in slices:
            sel_slice(slice_num) 
            fw_load_broadcast_mode('OFF')
    
    print(' (%2.2f sec)'% (download_time))
    #print("\r"),

    ##### 6. Check FW CRC for each slice and confirm a good download
    for slice_num in slices:
        sel_slice(slice_num) 
        ret_fw_load_st = fw_load_status(expected_crc_code,slice_num)
        if ret_fw_load_st is False:
            return False
       
    sel_slice(slices[0]) # select the first slice before exiting
    
####################################################################################################
# This function checks real time for ALL FW related parameters 
# 
####################################################################################################
def fw_info(slice=[0,1],print_en=True):
    global gFwFileName
    global gFwFileNameLastLoaded
    try:
        gFwFileNameLastLoaded
    except NameError:
        fw_bin_filename = 'FW_FILENAME_UNKNOWN'
    else:
        fw_bin_filename = gFwFileNameLastLoaded
        
    slices = get_slice_list(slice)
    for slc in slices:
        sel_slice(slc)        
        magic_code = fw_magic(print_en=0) # Expected FW magic word = 0x6A6A
        ver_code   = fw_ver(print_en=0)     # 
        hash_code  = fw_hash(print_en=0)
        crc_code   = fw_crc(print_en=0)
        date_code  = fw_date(print_en=0)
        d=datetime.date(1970, 1, 1) + datetime.timedelta(date_code)
        
        ver_str    = "VER_%02d.%02d.%02d" %(ver_code>>16&0xFF,ver_code>>8&0xFF,ver_code&0xFF)
        date_str   = "DATE_%04d%02d%02d"%(d.year, d.month, d.day)
        hash_str   = "HASH_0x%06X" %(hash_code)
        crc_str    = "CRC_0x%04X" %(crc_code)
        magic_str  = "MAGIC_0x%04X" %(magic_code)
        
        if print_en:
            if hash_code==0 or hash_code==0xFFFFFF:                # A quick check for zero value
                print("\n...Slice %d has no FW Loaded!"%slc),
            else:
                print("\n...Slice %d FW Info:"%(slc))
                print("'%s'," %(fw_bin_filename))
                print("%s," %(ver_str))
                print("%s," % (date_str))
                print("%s," %(hash_str))
                print("%s," %(crc_str))
                print("%s" %(magic_str))

    if not print_en:
        return fw_bin_filename, ver_str, date_str, hash_str, crc_str,  magic_str
####################################################################################################
# This function reads MAGIC WORD if a valid FW already loaded in Serdes
# 
####################################################################################################
def fw_magic(print_en=1):
    magic_word = rreg(Pam4Reg.fw_load_magic_word_addr) # reading back FW MAGIC Code
    if (magic_word == Pam4Reg.fw_load_magic_word):
        if print_en==1: print("\n...FW Magic Word : 0x%04X (Download Successful)\n" % magic_word),
        return magic_word
    else:
        if print_en==1: print("\n...FW Magic Word : 0x%04X (INVALID!!! Expected: 0x%04X)\n" % (magic_word, Pam4Reg.fw_load_magic_word)),
        return 0
    #return magic_word
####################################################################################################
# This function reads SerDes Chips Revision version number for the FW already loaded in Serdes
# 
# This routine check for Chip Revsion ID using know register and value
# If ID in efuse is supported, it also read efuse ID 
# 
# Returns: 1.0 for rev A0
#          1.1 for rev A1
#          1.2 for rev A2
#          2.0 for rev B0
#          2.1 for rev B1
#          3.0 for rev C0
#
####################################################################################################
def chip_rev(print_en=0):
    rev_id_addr = [0x91,[15,0]]
  
    rev_id_reg_val = rreg(rev_id_addr,lane=8) # readback chip rev regsiter 
    
    if rev_id_reg_val == 0x0100:      
        rev_id = 2.0
        if print_en==1: print("\n...Credo Silicon Revision : B0"),
    elif rev_id_reg_val == 0x0000:      
        rev_id = 1.0
        if print_en==1: print("\n...Credo Silicon Revision : A0"),
    else: 
        rev_id = -1.0
        if print_en==1: print("\n...Credo Silicon Revision Invalid ==> (%04X)\n" %(rev_id_reg_val)),
        
    return rev_id
####################################################################################################
# This function reads FW version number for the FW already loaded in Serdes
# 
# This routine check for FW version support.
# Earlier FW did not support version command (0xF003) and return the same 
# value for date_code and ver_code. 
# 
####################################################################################################
def fw_ver(print_en=1):
    #date_code = fw_date(print_en=0) # readback date code (cmd = 0xF002). 
    # wreg(Pam4Reg.fw_ver_word_lo_addr, 0x0000) # clear readback-low value first before gets updated by next cmd
    # wreg(Pam4Reg.fw_ver_word_hi_addr, 0x0000) # clear readback-high value first before gets updated by next cmd
    wreg(Pam4Reg.fw_ver_read_en_addr, Pam4Reg.fw_ver_read_en) # enable reading back FW VERSION code
    # cnt=0
    # while(rreg(Pam4Reg.fw_ver_read_en_addr)& Pam4Reg.fw_ver_read_status != Pam4Reg.fw_ver_read_status):
        # cnt+=1
        # if (cnt > 100):    break
        # pass
    high_word = rreg(Pam4Reg.fw_ver_word_hi_addr) # upper byte
    low_word  = rreg(Pam4Reg.fw_ver_word_lo_addr) # lower word
    ver_code = (high_word <<16) + low_word
    
    # if date_code == low_word:      # if this is an older FW version that didn't support version command 0xF003. put month/day of date as version number  
        # d=datetime.date(1970, 1, 1) + datetime.timedelta(date_code)
        # ver_code = ((d.month)<<8) + d.day    
        # if print_en==1: print("\n...FW Version : %02d.%02d.%02d\n" %(ver_code>>16&0xFF,ver_code>>8&0xFF,ver_code&0xFF))
    # else: # if this is a FW that support version command 0xF003. get the actual FW version
    if print_en==1: print("\n...FW Version : %02d.%02d.%02d\n" %(ver_code>>16&0xFF,ver_code>>8&0xFF,ver_code&0xFF))
    
    return ver_code
####################################################################################################
# This function reads HASH CODE for the FW already loaded in Serdes
# 
####################################################################################################
def fw_hash(print_en=1):
    wreg(Pam4Reg.fw_hash_word_lo_addr, 0x0000) # clear readback value first before gets updated by next cmd
    wreg(Pam4Reg.fw_hash_read_en_addr, Pam4Reg.fw_hash_read_en) # enable reading back FW HASH code
    cnt=0
    while(rreg(Pam4Reg.fw_hash_read_en_addr)& Pam4Reg.fw_hash_read_status != Pam4Reg.fw_hash_read_status):
        cnt+=1
        if (cnt > 100):    break
        #pass
    high_word = rreg(Pam4Reg.fw_hash_word_hi_addr) # upper byte
    low_word  = rreg(Pam4Reg.fw_hash_word_lo_addr) # lower word
    hash_code = (high_word <<16) + low_word
    if print_en==1: print("\n...FW Hash Code : 0x%06X\n" %(hash_code)),
    return hash_code
####################################################################################################
# This function reads CRC CODE for the FW already loaded in Serdes
# 
####################################################################################################
def fw_crc(print_en=1):
    wreg(Pam4Reg.fw_crc_word_addr, 0x0000) # clear readback value first before gets updated by next cmd
    wreg(Pam4Reg.fw_crc_read_en_addr, Pam4Reg.fw_crc_read_en)  # Enable reading back FW CRC Code
    time.sleep(.01)
    checksum_code  = rreg(Pam4Reg.fw_crc_word_addr)
    if print_en==1: print("\n...FW  CRC Code    : 0x%04X\n" %(checksum_code)) ,
    return checksum_code
####################################################################################################
# This function Calculates CRC CODE for the Code+ReadOnly Section of the FW in Memory
# 
####################################################################################################
def fw_crc_calc(print_en=1):
    wreg(0x9806, 0xA0F0)  # Enable reading back FW CRC Code
    time.sleep(.1)
    
    for i in range(1000):
        checksum_cmd_status = rreg(0x9806) # wait for 0x0A01 indicating command done
        if checksum_cmd_status==0x0A01: # ==0x0A01
            break

    if checksum_cmd_status != 0x0A01:
        checksum_code=-1
        if print_en==1: print("\n...CRC Calc for Code+ReadOnly Execution Failed. Expected: 0x0A01, Actual: 0x%04X\n" %(checksum_cmd_status)) ,
    else:
        checksum_code  = rreg(0x9807)
        if print_en==1: print("\n...CRC Calc for Code+ReadOnly Memory: 0x%04X\n" %(checksum_code)),
    return checksum_code
####################################################################################################
# This function reads DATE CODE for the FW already loaded in Serdes
# 
####################################################################################################
def fw_date(print_en=1):
    wreg(Pam4Reg.fw_date_word_addr, 0x0000) # clear readback value first before gets updated by next cmd
    wreg(Pam4Reg.fw_date_read_en_addr, Pam4Reg.fw_date_read_en)  # Enable Reading back FW Date Code
    time.sleep(.01)
    datecode = rreg(Pam4Reg.fw_date_word_addr)
    d=datetime.date(1970, 1, 1) + datetime.timedelta(datecode)

    if print_en==1: print ("\n...FW Date Code : %04d-%02d-%02d\n" % (d.year, d.month, d.day))
    return datecode
####################################################################################################
# This function reads WATCHDOG Counter value
#
# The watchdog timer is incremented only by the FW, if FW is loaded in Serdes
# 
####################################################################################################
def fw_watchdog(count=None):
    if count != None:
        wreg(Pam4Reg.fw_watchdog_timer_addr,0x0000) # clear the counter if asked
        
    watchdog_count = rreg(Pam4Reg.fw_watchdog_timer_addr)
    return watchdog_count
    
fw_tick=fw_watchdog

####################################################################################################
# This function checks if a FW is loaded
#
# Returns 0: if FW is not loaded
# Returns 1: if FW is loaded
#
####################################################################################################
def fw_loaded(print_en=1):    
    val0 = fw_hash(print_en=0)           # check the hash code to see if it valid counter to see if FW is incrementing it
    if val0==0 or val0==0xFFFFFF:                # A quick check for zero value
        fw_loaded_stat=0       # hash code counter is zero, FW is not loaded
        if print_en==1: print("\n...Slice %d has no FW Loaded!"%gSlice)
    else:
        fw_loaded_stat=1       # hash code is not zero, a FW not loaded
        if print_en==1: 
            print("\n...Slice %d has FW Loaded" %(gSlice)),
            print (fw_info(slice=gSlice, print_en=0))
        
    return fw_loaded_stat 
####################################################################################################
# This function checks if a FW is loaded and is running?
#
# Returns 0: if FW is not running or not loaded
# Returns 1: if FW is running
#
####################################################################################################
def fw_running():    
    val0 = fw_watchdog()       # check the watchdog counter to see if FW is incrementing it
    if val0==0:                # A quick check for zero value
        fw_running_stat=0      # watchdog counter is zero, FW is not loaded       
    else:
        time.sleep(1.8)        # wait more than one second
        val1 = fw_watchdog()   # check the watchdog counter once more
        if val1 > val0:        # watchdog counter is moving, then FW is loaded and running
            #wreg(fw_load_magic_word_addr, fw_load_magic_word) # if FW is loaded but magic word is corrupted, correct it
            fw_running_stat=1  # watchdog counter is moving, then FW is loaded and running
        else:
            fw_running_stat=0   # watchdog counter is not moving, then FW is not loaded or halted
            fw_watchdog(0)      # clear the counter so the next fw_running check goes faster            

    return fw_running_stat 
####################################################################################################
# This function removes any FW already loaded in Serdes
# 
####################################################################################################
def fw_unload(print_en=1,slice=None):
    slices = get_slice_list(slice)
    for slc in slices:
        sel_slice(slc)      
        wreg(Pam4Reg.fw_unload_addr, Pam4Reg.fw_unload_word) # Enable Unloading the FW from Slice
        cpu_reset() # Need to reset CPU 
        time.sleep(0.1)
        #wreg(Pam4Reg.fw_unload_addr, 0) # Clear the Unload register after cpu reset
        fw_unload_status = rreg(Pam4Reg.fw_unload_addr) # Read and check to make sure Boot ROM register is cleared after cpu reset
        if fw_unload_status != 0:
            if print_en: print("\n*** Slice %d FW Unload FAILED or CPU Not Running!  ***"%slc)
            #result = False
        else:        
            for ln in range(len(lane_name_list)):
                for core in [Pam4Reg, NrzReg]:
                    wreg(core.rx_bp1_st_addr, 0, ln)
                    wreg(core.rx_bp1_en_addr, 0, ln)
                    wreg(core.rx_sm_cont_addr, 0, ln)
                    time.sleep(0.001)
                    wreg(core.rx_sm_cont_addr, 1, ln)
            if print_en: print("\n...Slice %d FW Unloaded Successfully"%slc),

####################################################################################################
def fw_cmd(cmd):
    wreg(c.fw_cmd_addr, cmd) # fw_cmd_addr = 0x9806
    for i in range(1000):
        result=rreg(c.fw_cmd_addr) # fw_cmd_addr = 0x9806
        if result!=cmd: # ==0x800 or result==0xb00:
            break
    if result == 0x302:
        result=-1
        #print("\n*** fw_cmd %04x Failed. Return Status = 0x%04X" %(cmd, result))
    #print("\n*** FW Command %04x failed. is firmware loaded?" % cmd)
    return result

####################################################################################################
# Get FW Debug Information
####################################################################################################
def fw_debug_info(section=2, index=2,lane=None):
    lanes = get_lane_list(lane)
    result = {}
    #timeout=0.2 
    for ln in lanes:
        result[ln] = fw_debug_cmd(section, index, ln)
        
    return result
####################################################################################################
# Get FW Debug Information
####################################################################################################
def fw_debug(section=None, index=None,lane=None):

    if section is None:         sections=[2]
    if type(section)==int:    sections=[section]
    elif type(section)==list: sections=section
    
    if index is None:           indices=[2]
    if type(index)==int:      indices=[index]
    elif type(index)==list:   indices=index
    
    lanes = get_lane_list(lane)
    result = {}
    #timeout=0.2 
    for sec in sections:
        for idx in indices:
            for ln in lanes:
                result[ln].append(fw_debug_cmd(sec, idx, ln))
        
    return result
####################################################################################################
# Get FW Debug Information
####################################################################################################
def fw_debug_cmd(section=2, index=7, lane=0):
    #timeout=0.2
    result=0    
    cmd = 0xB000 + ((section&0xf)<<4) + lane
    wreg(c.fw_cmd_detail_addr, index,lane) # fw_cmd_detail_addr = 0x9807
    status = fw_cmd(cmd)   # fw_cmd_addr = 0x9806
    if(status!=0x0b00+section):
        #print("FW Debug CMD Section %d, Index %d, for Lane %s failed with code 0x%04x" %(section, index, lane_name_list[lane],status))
        result = -1
    else:
        result = rreg(c.fw_cmd_detail_addr,lane) # fw_cmd_detail_addr = 0x9807
        
    return result
####################################################################################################
# Get FW config Information
####################################################################################################
def fw_config_cmd(config_cmd=0x8090,config_detail=0x0000):  
    wreg(c.fw_cmd_detail_addr, config_detail) # fw_cmd_detail_addr = 0x9807
    status = fw_cmd(config_cmd)   # fw_cmd_addr = 0x9806
    if(status==0x302): # Retry sending config command one more time
        #print("FW Config CMD 0x%04X, Config Detail: 0x%04X, Failed with Status 0x%04X. RETRYING..."%(config_cmd, config_detail ,status))
        status = fw_cmd(config_cmd)   # fw_cmd_addr = 0x9806
    
    if(status==0x302): # Failed again on the second retry 
        status=-1
        #print("FW Config CMD 0x%04X, Config Detail: 0x%04X, Failed with Status 0x%04X. AFTER RETRY"%(config_cmd, config_detail ,status))

        
    return status
####################################################################################################
# 
# lane_reset fully managed by FW
####################################################################################################
def fw_lane_reset (lane=None):

    # if not fw_loaded(print_en=0):
        # print("\n*** No FW Loaded. Skipping fw_lane_reset().\n")
        # return
    
    fw_lane_reset_cmd = 0xA000   
    lanes = get_lane_list(lane)
                
    for ln in lanes:             
        fw_config_cmd(config_cmd=fw_lane_reset_cmd+ln,config_detail=0x0000) 
        
    global gLaneStats #lane statistics, used in rx_monitor()
    for ln in lanes:  gLaneStats[gSlice][ln][3]=0 
####################################################################################################
# 
# lane_reset by toggling HW bit (but when FW is loaded)
####################################################################################################
def hw_lane_reset(lane=None):

    lanes = get_lane_list(lane)
    get_lane_mode(lanes) # update the Encoding modes of all lanes for this Slice
    
    for ln in lanes:
        c = Pam4Reg if gEncodingMode[gSlice][ln][0].upper() == 'PAM4' else NrzReg  # Lane is in PAM4 or NRZ mode?        
        wreg(c.rx_lane_rst_addr, 0x1, ln)
    time.sleep(.050)        
    for ln in lanes:
        c = Pam4Reg if gEncodingMode[gSlice][ln][0].upper() == 'PAM4' else NrzReg  # Lane is in PAM4 or NRZ mode?        
        wreg(c.rx_lane_rst_addr, 0x0, ln)
    time.sleep(.050)        
            
    global gLaneStats #lane statistics, used in rx_monitor()
    for ln in lanes:  gLaneStats[gSlice][ln][3]=0 
        
####################################################################################################
# 
# lane_reset fully managed by SDK on HW State Machine (used only when FW is NOT loaded)
####################################################################################################
def hw_lane_reset_no_fw(lane=None):

    lanes = get_lane_list(lane)
    get_lane_mode(lanes) # update the Encoding modes of all lanes for this Slice
    
    for lane in lanes:
        if gEncodingMode[gSlice][lane][0] == 'pam4': # Lane is in PAM4 mode
            c=Pam4Reg

            #print"\nSlice %d Lane %s (PAM4) is Reset"%(gSlice,lane_name_list[lane]),
            #wreg(0x002,0xc000,lane) # final cntr target
            super_cal = rreg(c.rx_mu_ow_addr,lane)
            if super_cal != 0:
                wreg(c.rx_mu_owen_addr,1,lane) # super-cal disable        
                wreg(c.rx_mu_ow_addr,0,lane) # super-cal disable
            updn = rreg(c.rx_theta_update_mode_addr,lane)
            if updn != 0:
                wreg(c.rx_theta_update_mode_addr, 0, lane)
            blc = rreg([0x07b,[8,6]],lane) # BLC?
            wreg([0x07b,[8,6]],0,lane) # BLC off 
            wreg(c.rx_theta_update_mode_addr,0,lane) # up/dn mode disable - rajan        
            
            wreg(c.rx_lane_rst_addr, 0x1, lane)
            time.sleep(.050)        
            wreg(c.rx_lane_rst_addr, 0x0, lane)
            
            # Before exiting Lane Reset, Restore parameters
            if updn != 0: wreg(c.rx_theta_update_mode_addr,updn,lane) # up/dn mode enable - rajan        
            wreg(c.rx_mu_owen_addr,1,lane) # super-cal enable      
            if super_cal != 0: wreg(c.rx_mu_ow_addr,super_cal,lane) # super-cal enable        
            wreg([0x07b,[8,6]],blc,lane) # BLC restored
            
        else: # Lane is in NRZ mode
            c=NrzReg
            #print"\nSlice %d Lane %s (NRZ) is Reset"%(gSlice,lane_name_list[lane]),
            wreg([0x07b,[8,6]],0,lane) # BLC off
            wreg(c.rx_cntr_target_addr, 0x100, lane)            
            wreg(c.rx_lane_rst_addr, 0x1, lane)
            time.sleep(.050)        
            wreg(c.rx_lane_rst_addr, 0x0, lane)
            wreg(c.rx_cntr_target_addr, 0x002, lane)            

    # Clear lane statistics, used in rx_monitor()
    #rx_monitor_clear(lane=lanes,fec_thresh=fec_thresh) 
    global gLaneStats #lane statistics, used in rx_monitor()
    for ln in lanes:  gLaneStats[gSlice][ln][3]=0 
   
####################################################################################################
def lane_reset (lane=None):

    if (not fw_loaded(print_en=0)) or (fw_reg_rd(128)==0):  # FW not loaded or loaded but halted
        #lane_reset_method = 'sw_manage_lane_reset_with_no_fw'
        hw_lane_reset_no_fw(lane=lane)
    else:                                                   # FW loaded and running
        if fw_date(print_en=0)<18268:                       # FW Date is older than 2020-01-07
            #lane_reset_method = 'sw_manage_lane_reset_with_fw'
            hw_lane_reset (lane=lane)
        else:                                               # FW Date is on or after 2020-01-07
            l#ane_reset_method = 'fw_manage_lane_reset_fw_fully'
            fw_lane_reset (lane=lane)  
    
####################################################################################################
# 
# lane_reset (used when FW is loaded)
####################################################################################################
def lr(lane=None):
    lane_reset(lane)
    
####################################################################################################
#
# Here is how to get a lane speed
# 1. For address 0x9807, write 0x0004
# 2. For address 0x9806, write 0xB00x (x is the lane number, 0 to 0xF)
# 3. Read 0x9807 for return value.
#
# The return value would be 0x00~0x06.
# 0x00 -> ??? 
# 0x01 -> 10G 
# 0x02 -> 20G
# 0x03 -> 25G
# 0x04 -> 26G
# 0x05 -> 28G
# 0x06 -> ???
# 0x07 -> ???
# 0x08 -> 50G
# 0x09 -> 53G
# 0x0A -> 56G
#
####################################################################################################
def fw_lane_speed(lane=None):
              #[ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,  0x08,  0x09,  0x0A, 0x0B ]
    speed_list=['OFF','10G','20G','25G','26G','28G','53G','07?', '51G', '53G', '56G','0x0B?']
    mode_list =['off','nrz','nrz','nrz','nrz','nrz','pam4','07?','pam4','pam4','pam4','????']
    lanes = get_lane_list(lane)
    result = {}    
    for ln in lanes:
        speed_index= fw_debug_cmd(section=0,index=4,lane=ln)   
        result[ln] = [mode_list[speed_index],speed_list[speed_index]]
        #result[ln] = speed_index
    return result
####################################################################################################
# show current mode assigned to the lane by the FW
#
# index of the mode:
# 0: set_mode before opt
# 1: opt_mode
#
# mode:
# 0: lane in OFF
# 1: lane is in NRZ mode
# 2: lane is in PAM4 mode
####################################################################################################
def fw_lane_mode (lane=None):

    lanes = get_lane_list(lane)
    result = {}
    # print current fw_modes of the lanes
    #dbg_mode=0
    #dbg_cmd=c.fw_debug_info_cmd # 0xb000
    mode_list=['OFF','NRZ','PAM4']
    mode_idx=[0,1]
    opt_status_list=['-','OPT']
    #opt_status_bit=[0,1]
  
    for ln in lanes:
        for idx in range(2): # get both fw modes [0]:set_mode [1]: opt_mode
            mode_idx[idx] = fw_debug_cmd(section=0, index=idx, lane=ln) # for index=1 ===> 0: OFF, 1: NRZ, 2: PAM4
        opt_status_bit = (rreg(c.fw_opt_done_addr) >> ln) & 0x0001
        result[ln] = (mode_list[mode_idx[0]], mode_list[mode_idx[1]], opt_status_list[opt_status_bit])
    return result
####################################################################################################
# pause/restart FW in target lane(s)
#
#
####################################################################################################
def fw_pause (fw_mode=None,lane=None, print_en=1):

    if not fw_loaded(print_en=0):
        print("\n*** No FW Loaded. Skipping fw_pause() \n")
        return

    top_fw_addr      = 9
    serdes_fw_addr   = 8
    tracking_fw_addr = 128
    ffe_fw_addr      = 115
    
    off_list = ['OFF','DIS','PAUSE','STOP']
    mode_list= ['off','ON']

    lanes = get_lane_list(lane)
    result = {}

    if fw_mode!=None: # Write FW Registers to pause/restart FW
        en = 0 if any(i in fw_mode.upper() for i in off_list ) else 1
        if print_en:
            if en==0: 
                print ("\n...PAUSED FW on Lanes: " + str(lanes))
            else: 
                print ("\n...Restarted FW on Lanes: " + str(lanes))

        for ln in lanes:
            val1=fw_reg(addr=top_fw_addr, print_en=0)[top_fw_addr   ]
            val2=fw_reg(addr=serdes_fw_addr,print_en=0)[serdes_fw_addr]
            val3=fw_reg(addr=tracking_fw_addr, print_en=0)[tracking_fw_addr   ]
            val4=fw_reg(addr=ffe_fw_addr, print_en=0)[ffe_fw_addr   ]

            ####### Proper sequence to Pause FW    
            if en==0: 
                ### Pause Top FW     
                if any(i in fw_mode.upper() for i in ['TOP','ALL']):   
                    val = (val1 & ~(1<<ln)) | (en<<ln)
                    fw_reg(addr=top_fw_addr, data=val, print_en=0)
                ### Pause FFE FW ONLY (Keep Tracking FW 128 on to get ISI or PAM4 DFE taps)           
                if any(i in fw_mode.upper() for i in ['FFE']):   
                    val = (val4 & ~(1<<ln)) | (~en<<ln) # set lane bit to '1' to disable FFE FW
                    fw_reg(addr=ffe_fw_addr, data=val, print_en=0)
                ### Pause TRACKING FW
                ### if Pausing Serdes FW, Pause TRACKING FW first                 
                if any(i in fw_mode.upper() for i in ['SERDES','PHY','TRK','TRACKING','ALL']):   
                    val = (val3 & ~(1<<ln)) | (en<<ln)
                    fw_reg(addr=tracking_fw_addr, data=val, print_en=0)
                ### Pause Serdes FW     
                if any(i in fw_mode.upper() for i in ['SERDES','PHY','ALL']):             
                    val = (val2 & ~(1<<ln)) | (en<<ln)
                    fw_reg(addr=serdes_fw_addr, data=val, print_en=0)
                    ### Other preprations for "No-Serdes FW" mode
                    bp1(0,0,lane=ln) # Clear Breakpoint 1
                    bp2(0,0,lane=ln) # Clear Breakpoint 2
                    sm_cont(lane=ln) # Continue state machine
                    
            ####### Proper sequence to Restart FW    
            else: 
                ### Restart Serdes FW     
                ### if Restarting Serdes FW, Start TRACKING FW "after" Serdes FW    
                if any(i in fw_mode.upper() for i in ['SERDES','PHY','ALL']):             
                    val = (val2 & ~(1<<ln)) | (en<<ln)
                    fw_reg(addr=serdes_fw_addr, data=val, print_en=0)
                ### Restart TRACKING FW                      
                if any(i in fw_mode.upper() for i in ['SERDES','PHY','TRK','TRACKING','ALL']):   
                    val = (val3 & ~(1<<ln)) | (en<<ln)
                    fw_reg(addr=tracking_fw_addr, data=val, print_en=0)
                ### Restart Top FW     
                if any(i in fw_mode.upper() for i in ['TOP','ALL']):   
                    val = (val1 & ~(1<<ln)) | (en<<ln)
                    fw_reg(addr=top_fw_addr, data=val, print_en=0)
                ### Restart FFE FW last if it was off ( Tracking FW 128 asked to be back on)           
                if any(i in fw_mode.upper() for i in ['FFE','ALL','SERDES','TRK','TRACKING']):   
                    val = (val4 & ~(1<<ln)) | (~en<<ln) # set lane bit to '0' to enable FFE FW
                    fw_reg(addr=ffe_fw_addr, data=val, print_en=0)
    
    ### Read the updated values for the FW Registers        
    for ln in range(16): 
        status1=(fw_reg(addr=top_fw_addr,   print_en=0)[top_fw_addr   ]>>ln) & 1
        status2=(fw_reg(addr=serdes_fw_addr,print_en=0)[serdes_fw_addr]>>ln) & 1
        status3=(fw_reg(addr=tracking_fw_addr,   print_en=0)[tracking_fw_addr   ]>>ln) & 1
        status4=(fw_reg(addr=ffe_fw_addr,   print_en=0)[ffe_fw_addr   ]>>ln) & 1
        result[ln]=status1,status2,status3,status4
        
    ### Print FW Pause/Active Status for all lanes     
    if print_en: 
        print("\n------------------------"),
        print("  FW Pause/Off or Active/ON Status Per Lane"),
        print("  ------------------------"),
        print("\n...       Lane:"),
        for ln in range(16):
            print(" %3s" %(lane_name_list[ln])),

        print("\n...     Top FW:"),
        for ln in range(16):
            print(" %3s" %(mode_list[result[ln][0]])),
        print("\n...  Serdes FW:"),
        for ln in range(16):
            print(" %3s" %(mode_list[result[ln][1]])),
        print("\n...Tracking FW:"),
        for ln in range(16):
            print(" %3s" %(mode_list[result[ln][2]])),
        print("\n...     FFE FW:"),
        for ln in range(16):
            print(" %3s" %(mode_list[~result[ln][3]])),
        fw_reg(addr=[top_fw_addr,serdes_fw_addr,tracking_fw_addr,ffe_fw_addr])
    else:
        return result
####################################################################################################
# Use FW Command to Squelch or Unsquelch TX output
#
#
####################################################################################################
def fw_tx_squelch(mode=None,lane=None, print_en=1):

    if not fw_loaded(print_en=0):
        print("\n*** No FW Loaded. Skipping fw_tx_squelch().\n")
        return

    squelch_en_cmd      = 0x7011
    squelch_dis_cmd     = 0x7010
    squelch_status_addr = 0x98cd
    hw_tx_control_addr  = 0xA0
    
    squelch_en_list  = ['ON','EN','SQ','SQUELCH']
    squelch_dis_list = ['OFF','DIS','UNSQ','UNSQUELCH']
    mode_list= ['dis','EN']

    lanes = get_lane_list(lane)
    result = {}
    
    if print_en: print("\n-----------------------------"),
    if print_en: print("  FW TX Squelch/En or Unsquelch/Dis Status Per Lane"),
    if print_en: print("  -----------------------------"),
    if print_en: print("\n           Lane:"),
    if print_en: 
        for ln in range(16):
            print(" %4s" %(lane_name_list[ln])),

    if mode!=None: # Write FW Commands
        for ln in lanes:
            if any(i in mode.upper() for i in squelch_en_list):   
                val = 1<<ln
                fw_config_cmd(squelch_en_cmd, val)
            if any(i in mode.upper() for i in squelch_dis_list):   
                val = 1<<ln
                fw_config_cmd(squelch_dis_cmd, val)

    for ln in range(16): # Read FW and HW Registers
        status1=(rreg(squelch_status_addr)>>ln) & 1
        status2=rreg(addr=hw_tx_control_addr,lane=ln)
        result[ln]=status1,status2
        
    if print_en: # Print Status
        print("\n FW TX Squelch :"),
        for ln in range(16):
            print(" %4s"  %(mode_list[result[ln][0]])),
        print("\n HW TX Reg 0xA0:"),
        for ln in range(16):
            print(" %04X" %(result[ln][1])),
    else:
        return result
##############################################################################
# Use FW Command to Configure a lane in Optic Mode
#
#
##############################################################################
def fw_optic_mode(mode=None,lane=None, print_en=1):

    if not fw_loaded(print_en=0):
        print("\n*** No FW Loaded. Skipping fw_tx_squelch().\n")
        return

    optic_fw_reg_addr = 20
    optic_mode_reg_val = fw_reg_rd(optic_fw_reg_addr)
    
    optic_en_list  = ['ON','EN','OPTIC']
    optic_dis_list = ['OFF','DIS','COPPER']
    mode_list= ['dis','EN']

    lanes = get_lane_list(lane)
    result = {}
    
    if print_en: print("\n-----------------------------------"),
    if print_en: print("  FW Optic Mode Status Per Lane"),
    if print_en: print("  -----------------------------------"),
    if print_en: print("\n           Lane:"),
    if print_en: 
        for ln in range(16):
            print(" %4s" %(lane_name_list[ln])),
            
        
    if mode!=None: # Write FW Commands
        for ln in lanes:
            if any(i in mode.upper() for i in optic_en_list):
                optic_mode_reg_val = (optic_mode_reg_val | (1<<ln))
            if any(i in mode.upper() for i in optic_dis_list):   
                optic_mode_reg_val = (optic_mode_reg_val & ~(1<<ln))
        fw_reg_wr (optic_fw_reg_addr, optic_mode_reg_val)
        
    for ln in range(16): # Read relavant Registers
        lane_optic_mode =(fw_reg_rd(optic_fw_reg_addr) >> ln) & 1
        result[ln]=lane_optic_mode
        
    if print_en: # Print Status
        print("\n FW Optic Mode :"),
        for ln in range(16):
            print(" %4s"  %(mode_list[result[ln]])),
    else:
        return result
##############################################################################
# wait until all lanes are done with FW-based RX ADAPTATION 
#
# Status of all lanes RX and FECs set up as gearbox:
# 0: lane's RX adaptation not done or incomplete
# 1: lane's RX adaptation complete
##############################################################################
def fw_gearbox_wait (max_wait=2000, print_en=0):

    fec_b_bypass = True if rreg([0x9857,[7,4]])==0 else False
    start_time=time.time()
    
    cnt=1
    gearbox_done=0
    final_state_count=0
    final_gearbox_state = 6 if fec_b_bypass==True else 12  # When FECB Bypassed, final Gearbox FW State = 6, else 12
    if print_en: print("\n...Waiting for FEC Locks (fec_wait=%d)      "%(fw_reg_rd(14))),
    while ( not gearbox_done ):
        if print_en: print("\b\b\b\b\b\b%4.1fs"%(time.time()-start_time)),
        if print_en: print("\n%4.1fs"%(time.time()-start_time)),
        cnt+=1
        time.sleep(.1)
        gb_state0  = fw_debug_cmd(section=3,index=0, lane=0)
        gb_state2  = fw_debug_cmd(section=3,index=0, lane=2)
        gb_state8  = fw_debug_cmd(section=3,index=0, lane=8)
        gb_state12 = fw_debug_cmd(section=3,index=0, lane=12)
        
        if print_en: 
            # tx_output_a0 = hex(rreg(0xa0, 0))
            # tx_output_a2 = hex(rreg(0xa0, 2))
            # tx_output_b0 = hex(rreg(0xa0, 8))
            # tx_output_b4 = hex(rreg(0xa0,12))
            fec_stat, fec_counts = fec_status(print_en=0)
            adapt_stat = hex(rreg(0x98c9))
            reg1=hex(rreg(0x4880));reg2=hex(rreg(0x4a80));reg3=hex(rreg(0x5880));reg4=hex(rreg(0x5a80))
            #print gb_state0,gb_state2,gb_state8,gb_state12, tx_output_a0, tx_output_a2, tx_output_b0, tx_output_b4, adapt_stat, reg1,reg2,reg3,reg4, fec_stat,
            if print_en: print (gb_state0,gb_state2,gb_state8,gb_state12, adapt_stat, reg1,reg2,reg3,reg4, fec_stat)
            
        if gb_state0==final_gearbox_state and gb_state2==final_gearbox_state and gb_state8==final_gearbox_state and gb_state12==final_gearbox_state:
            if final_state_count<3:
                final_state_count+=1
            else:
                gearbox_done=1
        if cnt>max_wait :
            gearbox_done=1
    #fec_status()
    if print_en: print("...Done!\n")

    global gFecStatusPrevTimeStamp
    global gFecStatusCurrTimeStamp
    gFecStatusPrevTimeStamp[gSlice] = time.time()
    gFecStatusCurrTimeStamp[gSlice] = time.time()

    return gearbox_done
####################################################################################################
# wait until all lanes are done with FW-based RX ADAPTATION 
#
# Status of all lanes RX and FECs set up as gearbox:
# 0: lane's RX adaptation not done or incomplete
# 1: lane's RX adaptation complete
####################################################################################################
def fw_gearbox_wait_orig (max_wait=None, lane=None, print_en=1):

    lanes = get_lane_list(lane)
    start=time.time()
    adapt_done_total_prev = 0
    adapt_done_total = 0
    adapt_done_lane_prev = [0]*16
    adapt_done_lane_curr = [0]*16
    
    if max_wait is None:
        maxWait = 30
    else:
        maxWait = max_wait

    if(print_en==1):
        print("\n...Slice %d Lanes %d-%d RX Adaptation In Progress. . . \n"%(gSlice,lanes[0],lanes[-1]))
        for ln in lanes:
            print("%2s" %(lane_name_list[ln]) ),
    
    if(print_en==2): print("\n...Slice %d Lanes %d-%d RX Adaptation In Progress. . ."%(gSlice,lanes[0],lanes[-1])),
    
    while adapt_done_total < len(lanes):
        if(print_en==1): print("")
        if(print_en==2): print("."),
        adapt_done_total=0
        for ln in lanes:
            adapt_done_lane_curr[ln] = (rreg(c.fw_opt_done_addr) >> ln) & 1
            adapt_done_total += adapt_done_lane_curr[ln]
            if(print_en==2 and adapt_done_total_prev==0 and adapt_done_total>0):
                print("\n"),
            if(print_en==2 and adapt_done_lane_curr[ln]==1 and adapt_done_lane_prev[ln]==0 ):
                print("%s"%(lane_name_list[ln])),
            if(print_en==1): 
                print("%2d" %(adapt_done_lane_curr[ln]) ),
            adapt_done_lane_prev[ln] = adapt_done_lane_curr[ln]
            adapt_done_total_prev += adapt_done_total
            
        t2 = time.time() - start
        if(print_en==1): print(" %2.2fs"%t2),
        if (t2 > maxWait ):
            break

        time.sleep(.1)
                
    if(print_en==1):
        print ("")
        for ln in lanes:
            print("%2s" %(lane_name_list[ln]) ),
        print ("\n")

    adapt_done_all = 1 if adapt_done_total==len(lanes) else 0
    
    return [adapt_done_all, t2]
    
    
           
####################################################################################################
# wait until all lnaes are done with FW-based RX ADAPTATION 
#
# Status of each lane:
# 0: lane's RX adaptation not done or incomplete
# 1: lane's RX adaptation complete
####################################################################################################
def fw_config_wait (max_wait=None, lane=None, print_en=1):

    lanes = get_lane_list(lane)
    config_done_this_lane = [0]*16
    config_done_all_lanes=0
    
    if max_wait is None:
        maxWait = 3.0
        # if any of the lanes is PAM4 mode, set the max wait to 20 seconds, otherwise 3 seconds max 
        for ln in lanes:
            #lane_speed_index = fw_debug_cmd(section=0, index=4,  lane=ln)
            curr_fw_lane_mode = fw_lane_mode (ln)
            if any('PAM4' in s for s in curr_fw_lane_mode[ln]): maxWait =8
    else:
        maxWait = max_wait

    start=time.time()
    while config_done_all_lanes < len(lanes):
        config_done_all_lanes=0
        for ln in lanes:
            config_code_this_lane= fw_debug_cmd(section=0, index=38, lane=ln)
            ### Config is GB, BM or Retimer but not PHY mode
            if config_code_this_lane<0xA0:
                config_done_this_lane[ln] = (fw_debug_cmd(section=0, index=40, lane=ln) >> ln) & 1
            #### Config is PHY Mode. Get its status register 0x98c9
            else: 
                config_done_this_lane[ln] = (rreg(c.fw_opt_done_addr) >> ln) & 1   

            config_done_all_lanes += config_done_this_lane[ln]
        t2 = time.time() - start
        if (t2 > maxWait ):
            break
        time.sleep(.1)

    config_done_all = 1 if config_done_all_lanes==len(lanes) else 0
    
    return [config_done_all, t2]

####################################################################################################
# wait until all lnaes are done with FW-based RX ADAPTATION 
#
# Status of each lane:
# 0: lane's RX adaptation not done or incomplete
# 1: lane's RX adaptation complete
####################################################################################################
def fw_adapt_wait (max_wait=None, lane=None, print_en=1):

    lanes = get_lane_list(lane)
    start=time.time()
    adapt_done_total_prev = 0
    adapt_done_total = 0
    adapt_done_reg=0
    adapt_done_reg_prev=0
    adapt_done_lane_prev = [0]*16
    adapt_done_lane_curr = [0]*16
    
    if max_wait is None:
        maxWait = 3.0
        # if any of the lanes is PAM4 mode, set the max wait to 20 seconds, otherwise 3 seconds max 
        for ln in lanes:
            curr_fw_lane_mode = fw_lane_mode (ln)
            if any('PAM4' in s for s in curr_fw_lane_mode[ln]): maxWait =8
    else:
        maxWait = max_wait

    if(print_en==1):
        print("\n...Slice %d Lanes %d-%d RX Adaptation In Progress... \n"%(gSlice,lanes[0],lanes[-1]))
        print("  "),
        for ln in lanes:
            print("%2s" %(lane_name_list[ln]) ),
        print("")
    if(print_en==2): print("\n...Slice %d Lanes %d-%d RX Adaptation In Progress..."%(gSlice,lanes[0],lanes[-1])),
    
    while adapt_done_total < len(lanes):
        adapt_done_reg_prev = adapt_done_reg
        adapt_done_reg = rreg(c.fw_opt_done_addr)
        if(print_en==1):
            print("  "),
        if(print_en==2): 
            print("\b."),            
        adapt_done_total=0
        for ln in lanes:
            adapt_done_lane_curr[ln] = (adapt_done_reg >> ln) & 1
            adapt_done_total += adapt_done_lane_curr[ln]
            if(print_en==2 and adapt_done_total_prev==0 and adapt_done_total>0):
                print("\r...Slice %d Lanes %d-%d RX Adaptation In Progress..."%(gSlice,lanes[0],lanes[-1])),
            if(print_en==2 and adapt_done_lane_curr[ln]==1 and adapt_done_lane_prev[ln]==0 ):
                print("\b%s."%(lane_name_list[ln])),
            if(print_en==1): 
                print("%2d" %(adapt_done_lane_curr[ln]) ),
            adapt_done_lane_prev[ln] = adapt_done_lane_curr[ln]
            adapt_done_total_prev += adapt_done_total

            
        t2 = time.time() - start
        if(print_en==1): 
            print(" %4.1fs"%t2),
            if(adapt_done_reg!=adapt_done_reg_prev): 
                print('\n'), # ERASE_LINE = '\x1b[2K, CURSOR_UP_ONE = '\x1b[1A'
            else:
                print('\r'),
        if (t2 > maxWait ):
            break
        time.sleep(.1)
                
    if(print_en==1):
        print("  "),
        for ln in lanes:
            print("%2s" %(lane_name_list[ln]) ),
        print("\n"),

    adapt_done_all = 1 if adapt_done_total==len(lanes) else 0
    
    return [adapt_done_all, t2]
####################################################################################################
# wait until all Bitmux Config are done with FW-based BITMUX
#
# Status of each Bitmux:
# 0: lane's Bitmux Config not done or incomplete
# 1: lane's Bitmux Config complete
####################################################################################################
def fw_bitmux_wait (lane=[0,1,2,3], max_wait=None, print_en=1):

    lanes = get_lane_list(lane)
    b_lanes=[[8,9],[10,11],[12,13],[14,15],[12,13],[14,15]]
    #print b_lanes
    start=time.time()
    bitmux_done_total = 0
    done_total_prev = 0
    done_total = 0

    bitmux_tx_reg_lane_curr = [0]*16
    bitmux_done_lane_prev = [0]*16
    bitmux_state_lane_curr = [0]*16
    bitmux_done_lane_curr = [0]*16
    #adapt_done_lane_prev = [0]*16
    adapt_done_lane_curr = [0]*16
    
    if max_wait is None:
        maxWait = 10.0
    else:
        maxWait = max_wait

    if(print_en!=0):
        print("\n...Slice %d Lanes %d-%d Bitmux Config In Progress..."%(gSlice,lanes[0],lanes[-1])),

    if(print_en==1):
        print("\n  "),
        for a_ln in lanes:
            print("BM%d" %(a_ln) ),
            print("%2s" %(lane_name_list[a_ln]) ),
            for b_ln in b_lanes[a_ln]:
                print("%2s" %(lane_name_list[b_ln]) ),
            print(""),
        print("")
    
    while bitmux_done_total < len(lanes):
        if(print_en==1):
            print("  "),
        if(print_en==2): 
            print("\b."),            
            if(bitmux_done_total==0 ):
                print("\b\b\b\b..."),        
        bitmux_done_total=0
        done_total=0
        
        
        for a_ln in lanes:
            bitmux_tx_reg_lane_curr[a_ln] = rreg(0x0A0,lane=14)  # get bitmux TX registers 0xA0
            bitmux_state_lane_curr[a_ln] = fw_debug_cmd(4, 0,a_ln)   # get bitmux state number
            bitmux_done_lane_curr[a_ln] = 1 if fw_debug_cmd(8, 99,a_ln)==1 else 0   # get bitmux done for this group
            bitmux_done_total += bitmux_done_lane_curr[a_ln]
            done_total += bitmux_done_lane_curr[a_ln]
            
            adapt_done_lane_curr[a_ln] = (rreg(c.fw_opt_done_addr) >> a_ln) & 1 # get adapt done for this group
            done_total += adapt_done_lane_curr[a_ln]
            
            for b_ln in b_lanes[a_ln]:                                      # get adapt done for this group
                adapt_done_lane_curr[b_ln] = (rreg(c.fw_opt_done_addr) >> b_ln)  & 1
                done_total += adapt_done_lane_curr[b_ln]
                                          
            if(print_en==1): 
                print("%d" %(bitmux_state_lane_curr[a_ln])),
                print("%d" %(bitmux_done_lane_curr[a_ln])),
                print("%2d"  %(adapt_done_lane_curr[a_ln])),
                #print("0x%04x"  %(bitmux_tx_reg_lane_curr[a_ln])),
                for b_ln in b_lanes[a_ln]:
                    print("%2d" %(adapt_done_lane_curr[b_ln]) ),
                print(""),
            if(print_en==2):
                if(bitmux_done_lane_curr[a_ln]==1 and bitmux_done_lane_prev[a_ln]==0 ):
                    print("\b\bBM%d.."%(a_ln)),
                bitmux_done_lane_prev[a_ln]=bitmux_done_lane_curr[a_ln]
                
        t2 = time.time() - start
        if(print_en==1): 
            print(" %4.1fs"%t2),
            if(done_total!=done_total_prev): 
                print('\n'), # ERASE_LINE = '\x1b[2K, CURSOR_UP_ONE = '\x1b[1A'
            else:                
                print('\r'),
        done_total_prev = done_total
        if (t2 > maxWait ):
            break
        time.sleep(.01)
                
    if(print_en==1):
        print("  "),
        for a_ln in lanes:
            print("BM%d" %(a_ln) ),
            print("%2s" %(lane_name_list[a_ln]) ),
            for b_ln in b_lanes[a_ln]:
                print("%2s" %(lane_name_list[b_ln]) ),
            print(""),

    bitmux_done_all = 1 if bitmux_done_total==len(lanes) else 0
    
    return [bitmux_done_all, t2]
####################################################################################################
#
# "Adaptation" counter for SerDes PHY. Rolls over at 0xFFFF. 
#
####################################################################################################
def fw_adapt_cnt(lane=None):
    lanes = get_lane_list(lane)
    result = {} 
    for ln in lanes:
        if not fw_loaded(print_en=0):
            result[ln] =-1
            continue
        get_lane_mode(ln)
        if gEncodingMode[gSlice][ln][0]=='pam4':  # Lane is in PAM4 mode
            result[ln] = fw_debug_cmd(section=2,index=7,lane=ln)
        else:                                     # Lane is in NRZ mode
            result[ln] = fw_debug_cmd(section=1,index=10,lane=ln)

    return result
####################################################################################################
#
# "Re-adaptation" counter for SerDes PHY. Clears on read, saturates to 0xFFFF. 
# This increments with fw_adapt_cnt (FW does a lane-restart)
#
####################################################################################################
def fw_readapt_cnt(lane=None):
    lanes = get_lane_list(lane)
    result = {}    
    for ln in lanes:
        if not fw_loaded(print_en=0):
            result[ln] =-1
            continue
        get_lane_mode(ln)
        result[ln] = fw_debug_cmd(section=8,index=1,lane=ln)

    return result
####################################################################################################
#
# "Link lost" counter for SerDes PHY. Clears on read, saturates to 0xFFFF.
#
####################################################################################################
def fw_link_lost_cnt(lane=None):
    lanes = get_lane_list(lane)
    result = {}        
    for ln in lanes:
        if not fw_loaded(print_en=0):
            result[ln] =-1
            continue
        get_lane_mode(ln)
        result[ln] = fw_debug_cmd(section=8,index=0,lane=ln)

    return result
####################################################################################################
#
# "Gearbox FEC link lost" count, Clears on read, saturates to 0xFFFF.
#
####################################################################################################
def fw_gearbox_lost_cnt(lane=None):
    lanes = get_lane_list(lane)
    result = {}    
    for ln in lanes:
        if not fw_loaded(print_en=0):
            result[ln] =-1
            continue
        get_lane_mode(ln)
        result[ln] = fw_debug_cmd(section=11,index=0,lane=ln) 
    return result
####################################################################################################
#
# "Adaptation" counter for SerDes PHY. Rolls over at 0xFFFF. 
#
####################################################################################################
def fw_chan_est(lane=None):
    lanes = get_lane_list(lane)
    result = {}    
    for ln in lanes:
        if not fw_loaded(print_en=0):
            result[ln] =gChanEst[gSlice][ln]
            continue
        get_lane_mode(ln)
        [lane_mode,lane_speed]=fw_lane_speed(ln)[ln]
        sect = 2 if lane_mode.upper()=='PAM4' else 1
        chan_est =(fw_debug_info(section=sect, index=2,lane=ln)[ln]) / 256.0
        of       = fw_debug_info(section=sect, index=4,lane=ln)[ln]
        hf       = fw_debug_info(section=sect, index=5,lane=ln)[ln]            
        gChanEst[gSlice][ln]=[chan_est,of,hf]
        result[ln] = [chan_est,of,hf]
    return result
####################################################################################################
def fw_reg_rd(addr):
    c = Pam4Reg
    wreg(c.fw_cmd_detail_addr, addr,lane=0) # fw_cmd_detail_addr = 0x9807
    result=fw_cmd(0xe010)            # fw_cmd_addr = 0x9806
    if result!=0x0e00:
        print("\n*** FW Register read error, code=%04x" % result)
    return rreg(c.fw_cmd_status_addr) # fw_cmd_status_addr = 0x98C7

####################################################################################################
def fw_reg_wr(addr, data):
    c = Pam4Reg
    wreg(c.fw_cmd_detail_addr, addr,lane=0) # fw_cmd_detail_addr = 0x9807
    wreg(c.fw_cmd_status_addr, data,lane=0) # fw_cmd_status_addr = 0x98C7
    result=fw_cmd(0xe020)            # fw_cmd_addr = 0x9806
    if result!=0x0e00:
        print("\n*** FW Register write error, code=%04x" % result)
        
####################################################################################################
def fw_reg(addr=None, data=None, print_en=1):
    
    
    if addr is None:
        addr_list=range(201)    # read all FW registers
        data=None               # Make sure not to write more than one address at a time. Just read FW registers and exit!
    elif type(addr)==int:       
        addr_list=[addr]        # read single FW register
    elif type(addr)==list:    
        addr_list=addr          # read list of FW registers
        data=None               # Make sure not to write more than one address at a time. Just read FW registers and exit!
 
    result = {}
    c = Pam4Reg

    str=""
    #### FW Reg Write if data-to-write is not given
    if data!=None:
        wreg(c.fw_cmd_detail_addr, addr_list[0],lane=0) # fw_cmd_detail_addr = 0x9807
        wreg(c.fw_cmd_status_addr, data,lane=0) # fw_cmd_status_addr = 0x98C7
        cmd_status=fw_cmd(0xe020)            # fw_cmd_addr = 0x9806
        if cmd_status!=0x0e00:
            print("\n*** FW Register write error: Addr %d Code=0x%04x" %(addr, cmd_status))
            return False
    
    line_separator= "\n#+-------------------------+"
    title         = "\n#+ FWReg |      Value      |"
    str += line_separator + title + line_separator
    #### FW Reg Read 
    for reg_addr in addr_list:
        wreg(c.fw_cmd_detail_addr, reg_addr,lane=0) # fw_cmd_detail_addr = 0x9807
        cmd_status=fw_cmd(0xe010)            # fw_cmd_addr = 0x9806
        if cmd_status!=0x0e00:
            # Undefined FW register address or the read of a "defined" FW register failed
            #print("\n*** FW Register read error: Addr %d Code=0x%04x" % (reg_addr,cmd_status))
            break
        else:
            result[reg_addr] = rreg(c.fw_cmd_status_addr)
            str += ("\n#|  %3d  | 0x%04x  (%-5d) |"%(reg_addr,result[reg_addr],result[reg_addr]))
    
    str += line_separator
    if print_en == 1: 
        print (str)
    else:
        return result

####################################################################################################
# Instruct the FW to configure a lane (PHY) to a mode (and a speed)
# 
# Options for 'mode': 'nrz'      :  NRZ 25G (25.78125Gbps)
#                   : 'pam4'     : PAM4 53G (53.125  Gbps)
#
#                   : 'nrz-10G'  :  NRZ 10G (10.3125 Gbps)
#                   : 'nrz-20G'  :  NRZ 20G (20.6250 Gbps)
#                   : 'nrz-25G'  :  NRZ 25G (25.78125Gbps)
#                   : 'nrz-26G'  :  NRZ 26G (26.5625 Gbps)
#                   : 'nrz-28G'  :  NRZ 28G (28.125  Gbps)
#
#                   : 'pam4-51G' : PAM4 50G (51.5625 Gbps)
#                   : 'pam4-50G' : PAM4 53G (53.125  Gbps)
#                   : 'pam4-53G' : PAM4 53G (53.125  Gbps)
#                   : 'pam4-56G' : PAM4 56G (56.25   Gbps)
#
####################################################################################################
def fw_config_lane (mode=None, datarate=None, lane=None):

    if not fw_loaded(print_en=0):
        print("\n*** No FW Loaded. Skipping fw_config_lane().\n")
        return
    
    lanes = get_lane_list(lane)
    curr_fw_lane_mode = fw_lane_mode (lanes) # learn which mode (nrz/pam4/off) each lane is in now
    
    speed_str_list =['10','20','25','26','28','51','50','53','56'] # speed as part of mode argument
    speed_code_list=[0x11,0x22,0x33,0x44,0x55,0x88,0x99,0x99,0xAA] # speed codes to be written to 0x9807[7:0]
    
    if mode is None: # no arguments? then just print current fw_modes of the lanes and exit
        print ("")
        for ln in lanes:
            print(" %4s" %(lane_name_list[ln])),
        for idx in [1,2]: # show both fw modes [0]:set_mode [1]: opt_mode [2]: adapt_done flag
            print ("")
            for ln in lanes:
                print(" %4s"%curr_fw_lane_mode[ln][idx]),
    else:  # configure (either activate or turn off) the lanes          
        if 'NRZ' in mode.upper():
            mode_code_cmd = 0x80C0               # command to activate lane in NRZ
            speed_code_cmd = 0x33                # default NRZ speed is 25G                      
            if datarate!=None:  mode=mode+str(int(round((datarate-1.0)/5.0)*5.0)) # add datarate to mode so we can find it in the speed codes
            for i in range(len(speed_str_list)): # if NRZ speed is specified, take it
                if speed_str_list[i] in mode: speed_code_cmd = speed_code_list[i]
  
        elif 'PAM4' in mode.upper():
            mode_code_cmd = 0x80D0               # command to activate lane in PAM4
            speed_code_cmd = 0x99                # default PAM4 speed is 53G
            for i in range(len(speed_str_list)): # if PAM4 speed is specified, take it
                if speed_str_list[i] in mode: speed_code_cmd = speed_code_list[i]
                
        elif 'OFF' in mode.upper():              # command to deactivate lane (Turn it OFF)
            mode_code_cmd = 0x90D0 if(curr_fw_lane_mode[lanes[0]][1] == 'PAM4') else 0x90C0
            speed_code_cmd = 0x00                # For deactivating the lane, speed code does not matter

        else:
            for ln in lanes:
                print("\n***Slice %d Lane %s FW Config: selected mode is invalid  => '%s'" %(gSlice,lane_name_list[ln],mode.upper())),
            return
                
        ################ Destroy the target lanes before programming them to target mode
        for ln in lanes:             
            off_cmd = 0x90D0 if(curr_fw_lane_mode[ln][1] == 'PAM4') else 0x90C0
            result=fw_config_cmd(config_cmd=off_cmd+ln,config_detail=0x0000) 
            #print("\n...Slice %d Lane %s FW freed up lane before reconfiguring it. (OffCode 0x%04X, Error Code 0x%04x)" %(gSlice,lane_name_list[ln],off_cmd+ln,result)),
            if (result!=c.fw_config_lane_status): # fw_config_lane_status=0x800
                print("\n***Slice %d Lane %s: FW could not free up lane before reconfiguring it. (Error Code 0x%04x)" %(gSlice,lane_name_list[ln],result)),
        
        for ln in lanes:             
            wreg([0xa0,[15,11]],0x1d,ln) # '11101' Make sure TX output is not squelched
            
        ############# Now, configure the lane per user's target mode (either activate or deactivate)
        for ln in lanes:             
            result=fw_config_cmd(config_cmd=mode_code_cmd+ln,config_detail=speed_code_cmd) 
            #print("\n...Slice %d Lane %s FW_CONFIG_LANE to Active Mode. (SpeedCode 0x9807=0x%04X, ActivateCode 0x9806=0x%04X, ExpectedStatus:0x%0X, ActualStatus=0x%04x)" %(gSlice,lane_name_list[ln],speed_code_cmd, mode_code_cmd+ln,c.fw_config_lane_status,result)),
            if (result!=c.fw_config_lane_status): # fw_config_lane_status=0x800
                print("\n***Slice %d Lane %s FW_CONFIG_LANE to Active Mode Failed. (SpeedCode 0x9807=0x%04X, ActiveCode 0x9806=0x%04X, ExpectedStatus:0x%0X, ActualStatus=0x%04x)" %(gSlice,lane_name_list[ln],speed_code_cmd, mode_code_cmd+ln,c.fw_config_lane_status,result)),

                     
####################################################################################################  
# FW to program Gearbox mode, A-side PAM4, B-side: NRZ
#
#
####################################################################################################
def fw_config_gearbox_100G(A_lanes=[0,1], fec_b_byp=0):
    
    if not fw_loaded(print_en=0):
        print("\n...FW Gearbox 100G-2 : FW not loaeded. Not executed!"),
        return
        
    print_en=1
    #fec_reset() # reset all 8 FECs and clear their Align Markers
    
    # For 100G-2 Gearbox mode, 3 options supported for A-Lane groups 
    group0_100G=[0,1] # A_lanes group 1 -> [A0,A1] <-> [ 8, 9,10,11]
    group1_100G=[2,3] # A_lanes group 2 -> [A2,A3] <-> [12,13,14,15]
    group2_100G=[4,5] # A_lanes group 3 -> [A4,A5] <-> [12,13,14,15]
    
    #Determine the corresponding B-Lanes for each group of A-Lanes
    B_lanes=[]
    if all(elem in A_lanes for elem in group0_100G):  # If A_lanes contains [0,1]
        B_lanes+=[8,9,10,11]
    if all(elem in A_lanes for elem in group1_100G):  # If A_lanes contains [2,3]
        B_lanes+=[12,13,14,15]
    elif all(elem in A_lanes for elem in group2_100G): # If A_lanes contains [4,5]
        B_lanes+=[12,13,14,15]
    #else:
    #    print("\n*** 100G-2 Gearbox Setup: Invalid Target A-Lanes specified!\n")
    #    return
    
    lanes = sorted(list(set(A_lanes + B_lanes)))
    prbs_mode_select(lane=lanes, prbs_mode='functional')

    if all(elem in A_lanes for elem in group0_100G):  # If A_lanes contains [0,1]
        fw_config_cmd(config_cmd=0x9090,config_detail=0x0000) # 0x9090 = First, FW destroy any instances of these lanes being already used      
    if all(elem in A_lanes for elem in group1_100G):  # If A_lanes contains [2,3]
        fw_config_cmd(config_cmd=0x9091,config_detail=0x0000) # 0x9091 = First, FW destroy any instances of these lanes being already used
        fw_config_cmd(config_cmd=0x9092,config_detail=0x0000) # 0x9092 = First, FW destroy any instances of these lanes being already used     
    elif all(elem in A_lanes for elem in group2_100G): # If A_lanes contains [4,5]
        fw_config_cmd(config_cmd=0x9092,config_detail=0x0000) # 0x9091 = First, FW destroy any instances of these lanes being already used
        fw_config_cmd(config_cmd=0x9091,config_detail=0x0000) # 0x9092 = First, FW destroy any instances of these lanes being already used

    if all(elem in A_lanes for elem in group0_100G):  # If A_lanes contains [0,1]
        if fec_b_byp==False:
            if print_en: print("\n...FW Gearbox 100G-2: Lanes A0-A1 to B0-B3 with FEC_A0/FEC_B0..."),
            fw_config_cmd(config_cmd=0x8090,config_detail=0x0000) # 0x8090 = FW Activate Gearbox 100G-2 for Lanes A0/A1        
        else:
            if print_en: print("\n...FW Gearbox 100G-2: Lanes A0-A1 to B0-B3 with FEC_A0 (FEC_B0 Bypassed)..."),
            fw_config_cmd(config_cmd=0x8098,config_detail=0x0000) # no fec 0x8098 = FW Activate Gearbox 100G-2 for Lanes A0/A1        
    if all(elem in A_lanes for elem in group1_100G):  # If A_lanes contains [2,3]
        if fec_b_byp==False:
            if print_en: print("\n...FW Gearbox 100G-2: Lanes A2-A3 to B4-B7 with FEC_A2/FEC_B2..."),
            fw_config_cmd(config_cmd=0x8091,config_detail=0x0000) # 0x8091 = FW Activate Gearbox 100G-2 for Lanes A2/A3       
        else:
            if print_en: print("\n...FW Gearbox 100G-2: Lanes A2-A3 to B4-B7 with FEC_A2 (FEC_B2 Bypassed)..."),
            fw_config_cmd(config_cmd=0x8099,config_detail=0x0000) # no fec 0x8099 = FW Activate Gearbox 100G-2 for Lanes A2/A3       
    elif all(elem in A_lanes for elem in group2_100G): # If A_lanes contains [4,5]
        if fec_b_byp==False:
            if print_en: print("\n...FW Gearbox 100G-2: Lanes A4-A5 to B4-B7 with FEC_A2/FEC_B2..."),
            fw_config_cmd(config_cmd=0x8092,config_detail=0x0000)# 0x9092 = FW Activate Gearbox 100G-2 for Lanes A4/A5
        else:
            if print_en: print("\n...FW Gearbox 100G-2: Lanes A4-A5 to B4-B7 with FEC_A2 (FEC_B2 Bypassed)..."),
            fw_config_cmd(config_cmd=0x809A,config_detail=0x0000) # no fec 0x809A = FW Activate Gearbox 100G-2 for Lanes A4/A5       
    
    if print_en: print("Done!")
    #fec_status()
##############################################################################  
# FW to program Gearbox mode, A-side PAM4, B-side: NRZ
#
#
##############################################################################
def fw_config_gearbox_100G_LT(A_lanes=[0,1], fec_b_byp=0):
    
    if not fw_loaded(print_en=0):
        print("\n...FW Gearbox 100G-2 to 25G NRZ_ANLT: FW not loaded. Not executed!"),
        return
    print_en=1
    #fec_reset() # reset all 8 FECs and clear their Align Markers
    
    # For 100G-2 Gearbox mode, 3 options supported for A-Lane groups 
    group0_100G=[0,1] # A_lanes group 1 -> [A0,A1] <-> [ 8, 9,10,11]
    group1_100G=[2,3] # A_lanes group 2 -> [A2,A3] <-> [12,13,14,15]
    group2_100G=[4,5] # A_lanes group 3 -> [A4,A5] <-> [12,13,14,15]
    
    #Determine the corresponding B-Lanes for each group of A-Lanes
    B_lanes=[]
    if all(elem in A_lanes for elem in group0_100G):  # If A_lanes contains [0,1]
        B_lanes+=[8,9,10,11]
    if all(elem in A_lanes for elem in group1_100G):  # If A_lanes contains [2,3]
        B_lanes+=[12,13,14,15]
    elif all(elem in A_lanes for elem in group2_100G): # If A_lanes contains [4,5]
        B_lanes+=[12,13,14,15]
    #else:
    #    print("\n*** 100G-2 Gearbox Setup: Invalid Target A-Lanes specified!\n")
    #    return
    
    lanes = sorted(list(set(A_lanes + B_lanes)))
    prbs_mode_select(lane=lanes, prbs_mode='functional')

    if all(elem in A_lanes for elem in group0_100G):  # If A_lanes contains [0,1]     
        fw_config_cmd(config_cmd=0x9090,config_detail=0x0000) # 0x9090 = First, FW destroy any instances of these lanes being already used      
    if all(elem in A_lanes for elem in group1_100G):  # If A_lanes contains [2,3]
        fw_config_cmd(config_cmd=0x9091,config_detail=0x0000) # 0x9091 = First, FW destroy any instances of these lanes being already used
        fw_config_cmd(config_cmd=0x9092,config_detail=0x0000) # 0x9092 = First, FW destroy any instances of these lanes being already used     
    elif all(elem in A_lanes for elem in group2_100G): # If A_lanes contains [4,5]
        fw_config_cmd(config_cmd=0x9092,config_detail=0x0000) # 0x9091 = First, FW destroy any instances of these lanes being already used
        fw_config_cmd(config_cmd=0x9091,config_detail=0x0000) # 0x9092 = First, FW destroy any instances of these lanes being already used

    if all(elem in A_lanes for elem in group0_100G):  # If A_lanes contains [0,1]
        if fec_b_byp==False:
            if print_en: print("\n...FW Gearbox 100G-2 to 25G NRZ_ANLT: Lanes A0-A1 to B0-B3 with FEC_A0/FEC_B0..."),
            fw_config_cmd(config_cmd=0x8090,config_detail=0x0200) # 0x8090 = FW Activate Gearbox 100G-2 for Lanes A0/A1        
        else:
            if print_en: print("\n...FW Gearbox 100G-2 to 25G NRZ_ANLT: Lanes A0-A1 to B0-B3 with FEC_A0 (FEC_B0 Bypassed)..."),
            fw_config_cmd(config_cmd=0x8098,config_detail=0x0200) # no fec 0x8098 = FW Activate Gearbox 100G-2 for Lanes A0/A1        
    if all(elem in A_lanes for elem in group1_100G):  # If A_lanes contains [2,3]
        if fec_b_byp==False:
            if print_en: print("\n...FW Gearbox 100G-2 to 25G NRZ_ANLT: Lanes A2-A3 to B4-B7 with FEC_A2/FEC_B2..."),
            fw_config_cmd(config_cmd=0x8091,config_detail=0x0200) # 0x8091 = FW Activate Gearbox 100G-2 for Lanes A2/A3       
        else:
            if print_en: print("\n...FW Gearbox 100G-2 to 25G NRZ_ANLT: Lanes A2-A3 to B4-B7 with FEC_A2 (FEC_B2 Bypassed)..."),
            fw_config_cmd(config_cmd=0x8099,config_detail=0x0200) # no fec 0x8099 = FW Activate Gearbox 100G-2 for Lanes A2/A3       
    elif all(elem in A_lanes for elem in group2_100G): # If A_lanes contains [4,5]
        if fec_b_byp==False:
            if print_en: print("\n...FW Gearbox 100G-2 to 25G NRZ_ANLT: Lanes A4-A5 to B4-B7 with FEC_A2/FEC_B2..."),
            fw_config_cmd(config_cmd=0x8092,config_detail=0x0200)# 0x9092 = FW Activate Gearbox 100G-2 for Lanes A4/A5
        else:
            if print_en: print("\n...FW Gearbox 100G-2 to 25G NRZ_ANLT: Lanes A4-A5 to B4-B7 with FEC_A2 (FEC_B2 Bypassed)..."),
            fw_config_cmd(config_cmd=0x809A,config_detail=0x0200) # no fec 0x809A = FW Activate Gearbox 100G-2 for Lanes A4/A5       
    
    if print_en: print("Done!")
    #fec_status()
def fw_config_gearbox_50G(A_lanes=[0,1,2,3], fec_b_byp=False):
    
    if not fw_loaded(print_en=0):
        print("\n...FW Gearbox 50G-1 : FW is not loaded. Not executed!"),
        return
        
    print_en=1
    
    # For 50G-2 Gearbox mode, 3 options supported for A-Lane groups 
    group0_50G=[0] # A_lanes group 1 -> [A0] <-> [ 8, 9]
    group1_50G=[1] # A_lanes group 2 -> [A1] <-> {10,11]
    group2_50G=[2] # A_lanes group 3 -> [A2] <-> [12,13]
    group3_50G=[3] # A_lanes group 4 -> [A3] <-> [14,15]
    group4_50G=[4] # A_lanes group 4 -> [A4] <-> [12,13]
    group5_50G=[5] # A_lanes group 5 -> [A5] <-> [14,15]
    
    #Determine the corresponding B-Lanes for each group of A-Lanes
    B_lanes=[]
    if all(elem in A_lanes for elem in group0_50G): # If A_lanes contains [0]
        B_lanes+=[ 8, 9]                        
    if all(elem in A_lanes for elem in group1_50G): # If A_lanes contains [1]
        B_lanes+=[10,11]                      
    if all(elem in A_lanes for elem in group2_50G): # If A_lanes contains [2]
        B_lanes+=[12,13]
    if all(elem in A_lanes for elem in group3_50G): # If A_lanes contains [3]
        B_lanes+=[14,15]
    if all(elem in A_lanes for elem in group4_50G): # If A_lanes contains [2]
        B_lanes+=[12,13]
    if all(elem in A_lanes for elem in group5_50G): # If A_lanes contains [3]
        B_lanes+=[14,15]        
    #else:
    #    print("\n*** 50G-1 Gearbox Setup: Invalid Target A-Lanes specified!\n")
    #    return
    
    lanes = sorted(list(set(A_lanes + B_lanes)))
    prbs_mode_select(lane=lanes, prbs_mode='functional')

    if all(elem in A_lanes for elem in group0_50G):  # If A_lanes contains [0]
        fw_config_cmd(config_cmd=0x90b0,config_detail=0x0000) # 0x90b0 = First, FW destroy any instances of these lanes being already used      
    if all(elem in A_lanes for elem in group1_50G):  # If A_lanes contains [1]
        fw_config_cmd(config_cmd=0x90b1,config_detail=0x0000) # 0x90b1 = First, FW destroy any instances of these lanes being already used
    if all(elem in A_lanes for elem in group2_50G): # If A_lanes contains [2]
        fw_config_cmd(config_cmd=0x90b2,config_detail=0x0000) # 0x90b2 = First, FW destroy any instances of these lanes being already used
    if all(elem in A_lanes for elem in group3_50G): # If A_lanes contains [3]
        fw_config_cmd(config_cmd=0x90b3,config_detail=0x0000) # 0x90b3 = First, FW destroy any instances of these lanes being already used
    if all(elem in A_lanes for elem in group4_50G): # If A_lanes contains [4]
        fw_config_cmd(config_cmd=0x90b4,config_detail=0x0000) # 0x90b4 = First, FW destroy any instances of these lanes being already used
    if all(elem in A_lanes for elem in group5_50G): # If A_lanes contains [5]
        fw_config_cmd(config_cmd=0x90b5,config_detail=0x0000) # 0x90b5 = First, FW destroy any instances of these lanes being already used

    if all(elem in A_lanes for elem in group0_50G):  # If A_lanes contains [0]
        if fec_b_byp==False:
            if print_en: print("\n...FW Gearbox 50G-1: Lanes A0 to B0-B1 with FEC_A0/FEC_B0..."),
            fw_config_cmd(config_cmd=0x80b0,config_detail=0x0000) # 0x80b0 = FW Activate Gearbox 50G-1 for Lane A0
        else:
            if print_en: print("\n...FW Gearbox 50G-1: Lanes A0 to B0-B1 with FEC_A0 (FEC_B0 Bypassed)..."),
            fw_config_cmd(config_cmd=0x80b8,config_detail=0x0000) # no fec 0x80b8 = FW Activate Gearbox 50G-1 for Lane A0   
   
    if all(elem in A_lanes for elem in group1_50G):  # If A_lanes contains [1]
        if fec_b_byp==False:
            if print_en: print("\n...FW Gearbox 50G-1: Lanes A1 to B2-B3 with FEC_A1/FEC_B1..."),
            fw_config_cmd(config_cmd=0x80b1,config_detail=0x0000) # 0x80b1 = FW Activate Gearbox 50G-1 for Lane A1
        else:
            if print_en: print("\n...FW Gearbox 50G-1: Lanes A1 to B2-B3 with FEC_A1 (FEC_B1 Bypassed)..."),
            fw_config_cmd(config_cmd=0x80b9,config_detail=0x0000) # no fec 0x80b9 = FW Activate Gearbox 50G-1 for Lane A1   
    
    if all(elem in A_lanes for elem in group2_50G):  # If A_lanes contains [2]
        if fec_b_byp==False:
            if print_en: print("\n...FW Gearbox 50G-1: Lanes A2 to B4-B5 with FEC_A0/FEC_B0..."),
            fw_config_cmd(config_cmd=0x80b2,config_detail=0x0000) # 0x80b0 = FW Activate Gearbox 50G-1 for Lane A2
        else:
            if print_en: print("\n...FW Gearbox 50G-1: Lanes A2 to B4-B5 with FEC_A2 (FEC_B2 Bypassed)..."),
            fw_config_cmd(config_cmd=0x80ba,config_detail=0x0000) # no fec 0x80ba = FW Activate Gearbox 50G-1 for Lane A2   
    
    if all(elem in A_lanes for elem in group3_50G):  # If A_lanes contains [3]
        if fec_b_byp==False:
            if print_en: print("\n...FW Gearbox 50G-1: Lanes A3 to B6-B7 with FEC_A3/FEC_B3..."),
            fw_config_cmd(config_cmd=0x80b3,config_detail=0x0000) # 0x80b0 = FW Activate Gearbox 50G-1 for Lane A3
        else:
            if print_en: print("\n...FW Gearbox 50G-1: Lanes A3 to B6-B7 with FEC_A3 (FEC_B3 Bypassed)..."),
            fw_config_cmd(config_cmd=0x80bb,config_detail=0x0000) # no fec 0x80bb = FW Activate Gearbox 50G-1 for Lane A3
            
    if all(elem in A_lanes for elem in group4_50G):  # If A_lanes contains [4]
        if fec_b_byp==False:
            if print_en: print("\n...FW Gearbox 50G-1: Lanes A4 to B4-B5 with FEC_A0/FEC_B0..."),
            fw_config_cmd(config_cmd=0x80b4,config_detail=0x0000) # 0x80b0 = FW Activate Gearbox 50G-1 for Lane A4
        else:
            if print_en: print("\n...FW Gearbox 50G-1: Lanes A4 to B4-B5 with FEC_A2 (FEC_B2 Bypassed)..."),
            fw_config_cmd(config_cmd=0x80bc,config_detail=0x0000) # no fec 0x80bc = FW Activate Gearbox 50G-1 for Lane A4   
    
    if all(elem in A_lanes for elem in group5_50G):  # If A_lanes contains [5]
        if fec_b_byp==False:
            if print_en: print("\n...FW Gearbox 50G-1: Lanes A5 to B6-B7 with FEC_A3/FEC_B3..."),
            fw_config_cmd(config_cmd=0x80b5,config_detail=0x0000) # 0x80b0 = FW Activate Gearbox 50G-1 for Lane A5
        else:
            if print_en: print("\n...FW Gearbox 50G-1: Lanes A5 to B6-B7 with FEC_A3 (FEC_B3 Bypassed)..."),
            fw_config_cmd(config_cmd=0x80bd,config_detail=0x0000) # no fec 0x80bd = FW Activate Gearbox 50G-1 for Lane A5
    
    if print_en: print("Done!")
    #fec_status()
####################################################################################################
def watch(function,*args):
    #import os
    os.system('cls' if os.name == 'nt' else 'clear')
    try:
        while True:            
            print('\033[0;0H') # go to top left corner of screen
            print('\033[f') # go to top left corner of screen
            function(*args)
            time.sleep(1)            
    except KeyboardInterrupt:
        pass
####################################################################################################
def twos_to_int(twos_val, bitWidth):
    '''
    return a signed decimal number
    '''
    mask = 1<<(bitWidth - 1)
    return -(twos_val & mask) + (twos_val & ~mask)
    
####################################################################################################
def int_to_twos(value, bitWidth):
    '''
    return a twos complement number
    '''
    return (2**bitWidth)+value if value < 0 else value
####################################################################################################
def dec2bin(x):
  x -= int(x)
  bins = []
  for i in range(8):
    x *= 2
    bins.append(1 if x>=1. else 0)
    x -= int(x)
    #print bins
  value = 0
  for a in range(8):
    value = value+ bins[7-a]*pow(2,a)
    
  return value
####################################################################################################
# 
# Gray Code to Binary Conversion
####################################################################################################
def Bin_Gray(bb=0):
    gg=bb^(bb>>1)
    return gg

####################################################################################################
def Gray_Bin(gg=0): # up to 7-bit Gray Number
    bb1=(gg&0x40)
    bb2=(gg^(bb1>>1))&(0x20)
    bb3=(gg^(bb2>>1))&(0x10)
    bb4=(gg^(bb3>>1))&(0x8)
    bb5=(gg^(bb4>>1))&(0x4)
    bb6=(gg^(bb5>>1))&(0x2)
    bb7=(gg^(bb6>>1))&(0x1)
    bb=bb1+bb2+bb3+bb4+bb5+bb6+bb7
    return bb
####################################################################################################
def pol (tx_pol=None, rx_pol=None, lane=None, print_en=1):

    lanes = get_lane_list(lane)    
    #Slice=gSlice
    #get_lane_mode('all')
    result={}
           
    for ln in lanes:
        get_lane_mode(ln)
        c = Pam4Reg if lane_mode_list[ln] == 'pam4' else NrzReg
        if (tx_pol!=None):
            wreg(c.tx_pol_addr, tx_pol,ln)
        if (rx_pol!=None):
            wreg(c.rx_pol_addr, rx_pol,ln)

        tx_pol_this_lane= rreg(c.tx_pol_addr,ln)
        rx_pol_this_lane= rreg(c.rx_pol_addr,ln)
        result[ln] = tx_pol_this_lane, rx_pol_this_lane
        #if(print_en): print("\nSlice %d Lane %s (%4s) Polarity: TX: %d -- RX: %d"%(Slice,lane_name_list[ln],gEncodingMode[gSlice][ln][0].upper(),tx_pol_this_lane, rx_pol_this_lane)),

    if print_en: # Print Status
        get_lane_mode('all')
        print("\nSlice %d, Lane:"%(sel_slice())),
        for ln in range(len(lane_name_list)):
            print(" %2s" %(lane_name_list[ln])),
        print("\n  TX Polarity:"),
        for ln in range(len(lane_name_list)):
            c = Pam4Reg if lane_mode_list[ln] == 'pam4' else NrzReg
            print(" %2d"  %(rreg(c.tx_pol_addr,ln))),
        print("\n  RX Polarity:"),
        for ln in range(len(lane_name_list)):
            c = Pam4Reg if lane_mode_list[ln] == 'pam4' else NrzReg
            print(" %2d" %(rreg(c.rx_pol_addr,ln))),
    else:
        return tx_pol_this_lane, rx_pol_this_lane
####################################################################################################
def gc (tx_gc=None, rx_gc=None, lane=None, print_en=None):

    lanes = get_lane_list(lane)
        
    Slice=gSlice
    #get_lane_mode('all')
    
    for ln in lanes:
        get_lane_mode(ln)
        if gEncodingMode[gSlice][ln][0] == 'nrz': c=NrzReg
        else:                           c=Pam4Reg

        if (tx_gc!=None and rx_gc is None):
            wreg(c.tx_gray_en_addr, tx_gc,ln)
            if gEncodingMode[gSlice][ln][0] == 'pam4': wreg(c.rx_gray_en_addr, tx_gc,ln)
        elif (tx_gc!=None and rx_gc!=None):
            wreg(c.tx_gray_en_addr, tx_gc,ln)
            if gEncodingMode[gSlice][ln][0] == 'pam4': wreg(c.rx_gray_en_addr, rx_gc,ln)
        else:
            if print_en is None: print_en=1 # if no arguments, readout the current polarity settings

        tx_gc_this_lane= rreg(c.tx_gray_en_addr,ln)
        if gEncodingMode[gSlice][ln][0] == 'pam4': rx_gc_this_lane= rreg(c.rx_gray_en_addr,ln)
        else: rx_gc_this_lane=0
        
        if(print_en): print("\nSlice %d Lane %s (%4s) GrayCode: TX: %d -- RX: %d"%(Slice,lane_name_list[ln],gEncodingMode[gSlice][ln][0].upper(),tx_gc_this_lane, rx_gc_this_lane)),
    if(print_en==0): return tx_gc_this_lane, rx_gc_this_lane
####################################################################################################
def pc (tx_pc=None, rx_pc=None, lane=None, print_en=None):

    lanes = get_lane_list(lane)
        
    Slice=gSlice
    #get_lane_mode('all')
    
    for ln in lanes:
        get_lane_mode(ln)
        if gEncodingMode[gSlice][ln][0] == 'nrz': c=NrzReg
        else:                           c=Pam4Reg

        if (tx_pc!=None and rx_pc is None):
            wreg(c.tx_precoder_en_addr, tx_pc,ln)
            if gEncodingMode[gSlice][ln][0] == 'pam4': wreg(c.rx_precoder_en_addr, tx_pc,ln)
        elif (tx_pc!=None and rx_pc!=None):
            wreg(c.tx_precoder_en_addr, tx_pc,ln)
            if gEncodingMode[gSlice][ln][0] == 'pam4': wreg(c.rx_precoder_en_addr, rx_pc,ln)
        else:
            if print_en is None: print_en=1 # if no arguments, readout the current polarity settings

        tx_pc_this_lane= rreg(c.tx_precoder_en_addr,ln)
        if gEncodingMode[gSlice][ln][0] == 'pam4': rx_pc_this_lane= rreg(c.rx_precoder_en_addr,ln)
        else: rx_pc_this_lane= 0
        
        if(print_en): print("\nSlice %d Lane %s (%4s) Precoder: TX: %d -- RX: %d"%(Slice,lane_name_list[ln],gEncodingMode[gSlice][ln][0].upper(),tx_pc_this_lane, rx_pc_this_lane)),
    if(print_en==0): return tx_pc_this_lane, rx_pc_this_lane
####################################################################################################
def msblsb (tx_msblsb=None, rx_msblsb=None, lane=None, print_en=None):

    lanes = get_lane_list(lane)

    #Slice=gSlice
    #get_lane_mode('all')
    
    for ln in lanes:
        get_lane_mode(ln)
        if gEncodingMode[gSlice][ln][0] == 'nrz': c=NrzReg
        else:                           c=Pam4Reg

        if (tx_msblsb!=None and rx_msblsb is None):
            wreg(c.tx_msb_swap_addr, tx_msblsb,ln)
            if gEncodingMode[gSlice][ln][0] == 'pam4': wreg(c.rx_msb_swap_addr, tx_msblsb,ln)
        elif (tx_msblsb!=None and rx_msblsb!=None):
            wreg(c.tx_msb_swap_addr, tx_msblsb,ln)
            if gEncodingMode[gSlice][ln][0] == 'pam4': wreg(c.rx_msb_swap_addr, rx_msblsb,ln)
        else:
            if print_en is None: print_en=1 # if no arguments, readout the current polarity settings

        tx_msblsb_this_lane= rreg(c.tx_msb_swap_addr,ln)
        if gEncodingMode[gSlice][ln][0] == 'pam4': rx_msblsb_this_lane= rreg(c.rx_msb_swap_addr,ln)
        else: rx_msblsb_this_lane =0
        
        if(print_en): print("\nSlice %d Lane %s (%4s) MSB-LSB Swap: TX: %d -- RX: %d"%(gSlice,lane_name_list[ln],gEncodingMode[gSlice][ln][0].upper(),tx_msblsb_this_lane, rx_msblsb_this_lane)),
   
    if(print_en==0): return tx_msblsb_this_lane, rx_msblsb_this_lane

####################################################################################################
# reset_lane_pll ()
#
# Power Down/Up (Toggle PU bits of) TXPLL or RXPLL 
# Meanwhile, toggle FRAC_EN of each PLL
#
#################################################################################################### 
def reset_lane_pll (tgt_pll='both', lane=None):
   
    lanes = get_lane_list(lane)      # determine lanes to work on
    #get_lane_mode(lanes)         # Get current PAM4/NRZ modes settings of the specified lane(s)

    
    for ln in lanes:
        get_lane_mode(ln)
        c = Pam4Reg if lane_mode_list[ln].lower() == 'pam4' else NrzReg
  
        tx_frac_en = rreg(c.tx_pll_frac_en_addr, ln)# save, 0x0D7   [13] TX PLL FRAC_EN=x
        rx_frac_en = rreg(c.rx_pll_frac_en_addr, ln)# save, 0x1F0   [13] RX PLL FRAC_EN=y

        wreg(c.rx_pll_pu_addr,               0, ln) # Power down RX PLL while prgramming PLL
        wreg(c.tx_pll_pu_addr,               0, ln) # Power down RX PLL while prgramming PLL
                                          
        wreg(c.tx_pll_frac_en_addr,          0, ln) #  0x0D7   [13] TX PLL FRAC_EN=0
        wreg(c.rx_pll_frac_en_addr,          0, ln) #  0x1F0   [13] RX PLL FRAC_EN=0

        wreg(c.tx_pll_frac_en_addr,          1, ln) #  0x0D7   [13] TX PLL FRAC_EN=1
        wreg(c.rx_pll_frac_en_addr,          1, ln) #  0x1F0   [13] RX PLL FRAC_EN=1
                                          
        wreg(c.tx_pll_frac_en_addr,          0, ln) #  0x0D7   [13] TX PLL FRAC_EN=0
        wreg(c.rx_pll_frac_en_addr,          0, ln) #  0x1F0   [13] RX PLL FRAC_EN=0
                                          
        wreg(c.rx_pll_pu_addr,               1, ln) # Power up RX PLL after toggling FRAC_EN
        wreg(c.tx_pll_pu_addr,               1, ln) # Power up TX PLL after toggling FRAC_EN

        #### Enable Fractional PLLs after programming PLLs, if needed
        wreg(c.tx_pll_frac_en_addr, tx_frac_en, ln) # restore 0x0D7   [13] TX PLL FRAC_EN=x
        wreg(c.rx_pll_frac_en_addr, rx_frac_en, ln) # restore 0x1F0   [13] RX PLL FRAC_EN=y

####################################################################################################
# set/get pll caps
#
#
#################################################################################################### 
def pll_cap (tx_cap=None, rx_cap=None, lane=None, print_en=1):
   
    lanes = get_lane_list(lane)      # determine lanes to work on
    result = {}

    if(print_en): print("\n+-------------------------------------------------------------------+"),
    top_pll_A_cap = rreg([0x9501,[12,6]])
    top_pll_B_cap = rreg([0x9601,[12,6]])
    if(print_en): print("\n| Dev%d TopPLL A/B |     |  A-cap: %3d         |   B-cap: %3d        |"%(gSlice, top_pll_A_cap, top_pll_B_cap)),
    if(print_en): print("\n+-------------------------------------------------------------------+"),
    if(print_en): print("\n|      |          | Cal |      TX PLL Cal     |     RX PLL Cal      |"),
    if(print_en): print("\n| Lane | DataRate | Done| Min  Wr/Rd/Curr Max | Min  Wr/Rd/Curr Max |"),
    if(print_en): print("\n+-------------------------------------------------------------------+"),

    pll_params = get_lane_pll(lanes)

    for ln in lanes:
        pll_cal_done= (fw_debug_cmd(0,5,ln)>>ln)&1 

        tx_cap_min = fw_debug_cmd(0,10,ln)
        tx_cap_max = fw_debug_cmd(0,11,ln)
        rx_cap_min = fw_debug_cmd(0,12,ln)
        rx_cap_max = fw_debug_cmd(0,13,ln)
        tx_cap_write = fw_debug_cmd(0,14,ln)
        tx_cap_read  = fw_debug_cmd(0,15,ln)
        rx_cap_write = fw_debug_cmd(0,16,ln)
        rx_cap_read  = fw_debug_cmd(0,17,ln)

        if(tx_cap!=None): wreg(c.tx_pll_lvcocap_addr, tx_cap,ln)
        if(rx_cap!=None): wreg(c.rx_pll_lvcocap_addr, rx_cap,ln)
        tx_cap_this_lane= rreg(c.tx_pll_lvcocap_addr,ln)
        rx_cap_this_lane= rreg(c.rx_pll_lvcocap_addr,ln)
        result[ln] = [tx_cap_min,tx_cap_this_lane,tx_cap_max, rx_cap_min, rx_cap_this_lane,rx_cap_max]
        pll_params = get_lane_pll(lanes)
        fvco=pll_params[ln][0][0]
        if(print_en): print("\n|  %2s  | %8.5f | %2d  | %3d  %2d/%2d/%2d   %2d  | %3d  %2d/%2d/%2d   %2d  |"%(lane_name_list[ln],fvco,pll_cal_done,tx_cap_min,tx_cap_write, tx_cap_read,tx_cap_this_lane,tx_cap_max, rx_cap_min, rx_cap_write, rx_cap_read,rx_cap_this_lane,rx_cap_max)),
        if(print_en) and (ln==lanes[-1] or ln==7):
            print("\n+-------------------------------------------------------------------+"),

    if(print_en==0): return result

####################################################################
# pll cal for TOP PLL  (REFCLK Source)
#
# sweeps pll cap and checks pll lock
#################################################################### 
def pll_cal_top0( side = 'A'):
    window=0x3fff
    #toppll_lock = 0
    if side.upper() == 'A':
        Base_addr = 0x9500
    else:
        Base_addr = 0x9600
    # out = 0
    # inside = 0
    f2 = open('pll_cal_top_log.txt','w')
    wregBits(Base_addr + 0x01,[12,6],0,16)
    wregBits(Base_addr + 0x0D,[14,0],window,16)
    setting = rregBits(Base_addr + 0x0D,[14,0],16) # set counter to the reference frequency
    while 1:
        wregBits(Base_addr + 0x12, [3], 0,16)  # PD_CAL_FCAL = 0  
        wregBits(Base_addr + 0x10, [7], 0,16) # LO_OPEN = 0 
        wregBits(Base_addr + 0x0D, [15], 0,16) # fcal_start = 0  
        wregBits(Base_addr + 0x0D, [15], 1,16) # fcal_start = 1  
        while(rregBits(Base_addr + 0x0F, [15],16) == 0):  #check fcal_done. If it's done, continue to the next
            pass
        readout = rregBits(Base_addr + 0x0E,[15,0],16)  # read out the counter value(fcal_cnt_op[15:0]).
        vcocap = rregBits(Base_addr + 0x01,[12,6],16)
        if(abs(readout-setting) > 2):  
            if (vcocap == 0x7f):
                vcocap_min = 0
                #toppll_lock = 1
                print >> f2, "The frequency setting is out of pll list(range(min))"
                break
            else:
                vcocap = rregBits(Base_addr + 0x01,[12,6],16) + 0x1
                wregBits(Base_addr + 0x01, [12,6], vcocap,16)
        else:
            vcocap_min = rregBits(Base_addr + 0x01,[12,6],16)
            print >> f2, "Min vcocap = %s" % (bin(vcocap_min))
            break

    wregBits(Base_addr + 0x01,[12,6],0x7f,16)
    wregBits(Base_addr + 0x0D,[14,0],window,16)
    while 1:
        wregBits(Base_addr + 0x12, [3], 0,16)
        wregBits(Base_addr + 0x10, [7], 0,16)
        wregBits(Base_addr + 0x0D, [15], 0,16)
        wregBits(Base_addr + 0x0D, [15], 1,16)
        while(rregBits(Base_addr + 0x0F, [15],16) == 0):
            pass

        readout = rregBits(Base_addr + 0x0E,[15,0],16)
        vcocap = rregBits(Base_addr + 0x01,[12,6],16)
        if(abs(readout-window) > 2):
            if (vcocap == 0x0):
                vcocap_max = 0
                #toppll_lock = 1
                print >> f2, "The frequency setting is out of pll list(range(max))"
                break
            else:
                vcocap = rregBits(Base_addr + 0x01,[12,6],16) - 0x1
                wregBits(Base_addr + 0x01, [12,6], vcocap,16)
        else:
            vcocap_max = rregBits(Base_addr + 0x01,[12,6],16)
            print >> f2,  "Max vcocap = %s" % (bin(vcocap_max))
            break
           
    new_vcocap = int((vcocap_max + vcocap_min)/2)
    if ((vcocap_max == 0x7f) & ((vcocap_max-vcocap_min)<4)):
        new_vcocap = vcocap_max
    else:
        pass
    if ((vcocap_min == 0x00) & ((vcocap_max-vcocap_min)<4)):
        new_vcocap = vcocap_min
    else:
        pass

    print >> f2, "new_vcocap = %s" % (new_vcocap)
    print >> f2, "Min CAP value : %s" % (vcocap_min)
    print >> f2, "Max CAP value : %s" % (vcocap_max)
    wregBits(Base_addr + 0x01, [12,6], new_vcocap,16)
    print("CAP value : %s" % (hex(rregBits(Base_addr + 0x01, [12,6]))))
    f2.close()
    return new_vcocap
####################################################################
# pll cal for TOP PLL  (REFCLK Source)
#
# sweeps pll cap and checks pll lock
#################################################################### 
def pll_cal_top1(tgt_pll=None, lane=None, print_en=1):
    
    if tgt_pll is None: tgt_pll='both' # set both top PLLs, A and B
    pll_list=['A','B'] if tgt_pll=='both' else tgt_pll.upper()

    result = {}
    pll_cal_result=[[1,2,3],[4,5,6]] # TX_PLL[vcocap_min,new_vcocap,vcocap_max], RX_PLL[vcocap_min,new_vcocap,vcocap_max],
    log_en=1
    window=0x2000
    # out = 0
    # inside = 0
    
    if(log_en): f2 = open('pll_cal_TOP_log.txt','w')
    if print_en: print("\n     -- Top PLL Cap --",)
    if print_en: print("\nPLL, Min, Old/New, Max,",)

    for top_pll_side in pll_list:
        if print_en: print("\n%3s,"%top_pll_side,)
        if top_pll_side == 'A':
            Base_addr = 0x9500
            top_pll_index=0
        else:
            Base_addr = 0x9600
            top_pll_index=1

        toppll_lock = 0

        if(log_en): print >> f2, "TopPLL  ,Dir, Cap, Target, ReadOut, Delta"
        orig_vcocap = rregBits(Base_addr + 0x01,[12,6],16)  # original cap value
        
        ### Sweep upward and find min vcocap
        wregBits(Base_addr + 0x01,[12,6],0,16)
        wregBits(Base_addr + 0x0D,[14,0],window,16)
        target_cnt = rregBits(Base_addr + 0x0D,[14,0],16) # set counter to the reference frequency
        while 1:
            wregBits(Base_addr + 0x12, [3], 0,16)  # PD_CAL_FCAL = 0  
            wregBits(Base_addr + 0x10, [7], 0,16) # LO_OPEN = 0 
            wregBits(Base_addr + 0x0D, [15], 0,16) # fcal_start = 0  
            wregBits(Base_addr + 0x0D, [15], 1,16) # fcal_start = 1  
            while(rregBits(Base_addr + 0x0F, [15],16) == 0):  #check fcal_done. If it's done, continue to the next
                pass
            readout = rregBits(Base_addr + 0x0E,[15,0],16)  # read out the counter value(fcal_cnt_op[15:0]).
            vcocap = rregBits(Base_addr + 0x01,[12,6],16)
            if(log_en): print >> f2, "TopPLL-%s, Up, %3d,   %04X,   %04X,   %04X"%(top_pll_side, vcocap,target_cnt,readout, target_cnt-readout)
            if(abs(readout-target_cnt) > 2):  
                if (vcocap == 0x7f):
                    vcocap_min = 0
                    toppll_lock = 1
                    if(log_en): print >> f2, "Top PLL Side %s,  UP: The frequency target_cnt is out of pll list(range(min))"%top_pll_side
                    break
                else:
                    vcocap = rregBits(Base_addr + 0x01,[12,6],16) + 0x1
                    wregBits(Base_addr + 0x01, [12,6], vcocap,16)
            else:
                vcocap_min = rregBits(Base_addr + 0x01,[12,6],16)
                #if(log_en): print >> f2,  "Min vcocap = %d" % (vcocap_min)
                break

        ### Sweep downward and find max vcocap
        wregBits(Base_addr + 0x01,[12,6],0x7f,16)
        wregBits(Base_addr + 0x0D,[14,0],window,16)
        while 1:
            wregBits(Base_addr + 0x12, [3], 0,16)
            wregBits(Base_addr + 0x10, [7], 0,16)
            wregBits(Base_addr + 0x0D, [15], 0,16)
            # fcal start pulse delay
            wregBits(Base_addr + 0x0D, [15], 1,16)
            # fcal done check delay
            while(rregBits(Base_addr + 0x0F, [15],16) == 0):
                pass

            readout = rregBits(Base_addr + 0x0E,[15,0],16)
            vcocap = rregBits(Base_addr + 0x01,[12,6],16)
            if(log_en): print >> f2, "TopPLL-%s, Dn, %3d,   %04X,   %04X,   %04X"%(top_pll_side, vcocap,target_cnt,readout, target_cnt-readout)
            if(abs(readout-target_cnt) > 2):
                if (vcocap == 0x0):
                    vcocap_max = 0
                    toppll_lock = 1
                    if(log_en): print >> f2, "Top PLL Side %s,  DN: The frequency target_cnt is out of pll list(range(min))"%top_pll_side
                    break
                else:
                    vcocap = rregBits(Base_addr + 0x01,[12,6],16) - 0x1
                    wregBits(Base_addr + 0x01, [12,6], vcocap,16)
            else:
                vcocap_max = rregBits(Base_addr + 0x01,[12,6],16)
                #if(log_en): print >> f2,  "Max vcocap = %d" % (vcocap_max)
                break
               
        ### Find the optimum cap from min and max
        if (vcocap_max == 0x7f) and (vcocap_min == 0x00): # Search FAILED
            new_vcocap = orig_vcocap
        else: # Search SUCCESSFUL
            new_vcocap = int((vcocap_max + vcocap_min)/2) 
            if   ((vcocap_max == 0x7f) and ((vcocap_max-vcocap_min)<4)):
                new_vcocap = vcocap_max
            elif ((vcocap_min == 0x00) and ((vcocap_max-vcocap_min)<4)):
                new_vcocap = vcocap_min

        wregBits(Base_addr + 0x01, [12,6], new_vcocap,16)
        pll_cal_result[top_pll_index]=[vcocap_min,new_vcocap,vcocap_max]
        flag= '>' if orig_vcocap!=new_vcocap else ','
        if print_en: print("%3d, %3d%s%3d, %3d, " % (vcocap_min,orig_vcocap,flag,new_vcocap,vcocap_max),)
        if(log_en): print >> f2, "\nTop PLL %s VCOCAPS: Min/Center(Orig)/Max: %2d / %2d(%2d) / %2d\n" % (top_pll_side,vcocap_min,new_vcocap,orig_vcocap,vcocap_max)

        wregBits(Base_addr + 0x10,  [7],  0,16) # LO_OPEN = 0 
        wregBits(Base_addr + 0x0D,  [15], 0,16) # fcal_start = 0  
        wregBits(Base_addr + 0x0D,[14,0], 0,16) # facl window=0
        wregBits(Base_addr + 0x12,   [3], 1,16) # PD_CAL_FCAL = 1
        #### End of this Top PLL      
        
    #### End of both Top PLL
    result=[pll_cal_result[0],pll_cal_result[1]]  # [0]=A and [1]=B
    if print_en: print("\n")
    if(log_en): f2.close()
    if print_en==0: return result

####################################################################  
def pll_cal_top(side ='both'): 
    #print ("\npll_cal_top\n")

    if side.upper() == 'A' or side.upper() == 'BOTH':
        Base_addr = 0x9500
        capA = pll_cal_top3(base=Base_addr)
    if side.upper() == 'B' or side.upper() == 'BOTH':
        Base_addr = 0x9600
        capB = pll_cal_top3(base=Base_addr)

    if side.upper() == 'A':
        return capA
    if side.upper() == 'B':
        return capB
    if side.upper() == 'BOTH':  
        return [capA,capB]

####################################################################
# pll cal for TOP PLL  (REFCLK Source)
#
# sweeps pll cap and checks pll lock
####################################################################  
def pll_cal_top3(window=0x3fff, base=0x9500):
    wregBits(base+0x10, [6, 4], 4)
    wregBits(base+0x10, [7], 1)        
    wregBits(base+0xd, [14, 0], window)
    wregBits(base+0x12, [3], 0)
    wregBits(base+0x0d, [15], 0)
    vco_min = 0
    vco_max = 0
    no_cross = True
    for vco in list(range(0, 128, 2)):
        wregBits(base+1, [12, 6], vco)        
        wregBits(base+0x0d, [15], 1)
        count=0
        while rregBits(base+0xf, [15])==0 and count<1000: count+=1
        readout = rregBits(base+0xe, [15, 0])
        wregBits(base+0xd, [15], 0)
        if (count>=1000):   
            print("cal_done timeout at %d", vco)
            continue
        else:
            if readout > window :
                vco_min = vco
            elif no_cross :
                vco_max = vco
                no_cross = False          
            #print("scan, %3d, %04x, %04x" % (vco, window, readout))
    vco = int((vco_min+vco_max)/2)
    wregBits(base+1, [12, 6], vco)
    wregBits(base+0x10, [7], 0)
    wregBits(base+0xd, [14, 0], 0)
    wregBits(base+0x12, [3], 1)
    #print("Top PLL %04X final vco = %3d" % (base,vco)    )

    return vco

####################################################################
# pll cal for TOP PLL  (REFCLK Source)
#
# sweeps pll cap and checks pll lock
####################################################################     
def pll_cal_top2(lane=0, window=0x3fff, openLoop=False, scanAll=None, dir_down=False):
    # Prolog
    base=0x9500+lane*0x100
    
    if openLoop:
        wregBits(base+0x10, [6, 4], 4)
        wregBits(base+0x10, [7], 1)
    else:
        wregBits(base+0x10, [7], 0)
        
    wregBits(base+0xd, [14, 0], window)
    wregBits(base+0x12, [3], 0)
    wregBits(base+0x0d, [15], 0)
    vco_max = 128
    if scanAll is not None:
        if not isinstance(scanAll, list):
            scanAll=range(vco_max)
        for vco in scanAll:
            if dir_down :
                wregBits(base+1, [12, 6], vco)
            else :
                wregBits(base+1, [12, 6], vco_max-vco)           
            wregBits(base+0x0d, [15], 1)
            count=0
            while rregBits(base+0xf, [15])==0 and count<1000: count+=1
            readout = rregBits(base+0xe, [15, 0])
            wregBits(base+0xd, [15], 0)
            if (count>=1000):
                print("cal_done timeout at %d", vco)
            else:
                print("lane %d, scan, %3d, %04x, %04x" % (lane, vco, window, readout))
        return
    vco=0
    while vco<0x80:
        wregBits(base+1, [12, 6], vco)
        wregBits(base+0x0d, [15], 1)
        count=0
        while rregBits(base+0xf, [15])==0 and count<1000: count+=1
        readout = rregBits(base+0xe, [15, 0])
        wregBits(base+0x0d, [15], 0)
        if (count>=1000):
            print("cal_done timeout at %d", vco)
            return
        print("lane %d, up, %3d, %04x, %04x" % (lane, vco, window, readout))
        if abs(readout-window)>2:
            if (vco==0x7f):
                # vco_min = 0
                print("top pll scan up no lock")
                return
            else:
                vco+=1
                continue
        else:
            vcomin = vco
            print("lane %d vcomin = %3d" % (lane, vcomin))
            break
    vco=0x7f
    while vco>=0:
        wregBits(base+1, [12, 6], vco)
        wregBits(base+0xd, [15], 1)
        count=0
        while rregBits(base+0xf, [15])==0 and count<1000: count+=1
        readout = rregBits(base+0xe, [15, 0])
        wregBits(base+0xd, [15], 0)
        if (count>=1000):
            print("cal_done timeout at %d", vco)
            return
        print("lane %d, down, %3d, %04x, %04x" % (lane, vco, window, readout))
        if abs(readout-window)>2:
            if (vco==0):
                print("top pll scan down no lock")
                return
            else:
                vco-=1
                continue
        else:
            vcomax = vco
            print("lane %d vcomax = %3d" % (lane, vcomax))
            break
    vco = int((vcomin+vcomax)/2)
    wregBits(base+1, [12, 6], vco)
    # Epilog
    wregBits(base+0x10, [7], 0)
    wregBits(base+0xd, [14, 0], 0)
    wregBits(base+0x12, [3], 1)
    print("lane %d final vco = %3d" % (lane, vco))

####################################################################################################
# pll cal for RX PLL, TX PLL, or both 
#
# sweeps pll cap and checks pll lock
#################################################################################################### 
def pll_cal(tgt_pll=None, lane=None, print_en=1):

    if tgt_pll is None: tgt_pll='both' # set both PLLs with datarate passed
    pll_list=['tx','rx'] if tgt_pll=='both' else tgt_pll

    lanes = get_lane_list(lane)
    result = {}
    pll_cal_result=[[1,2,3],[4,5,6]] # TX_PLL[vcocap_min,new_vcocap,vcocap_max], RX_PLL[vcocap_min,new_vcocap,vcocap_max],
    #log_en=0
    window=0x2000
    # out = 0
    # inside = 0
    rx_lock = 0
       
    #if(log_en): f2 = open('pll_cal_log.txt','w')
    if print_en: print ("\n    --- TX PLL Cap ---  --- RX PLL Cap ---")
    if print_en: print ("\nLn, Min, Old/New, Max,  Min, Old/New, Max")

    for ln in lanes:
        if print_en: print ("\n%s,"%lane_name_list[ln])
        for lane_pll in pll_list:
            index=-1
            if 'tx' in lane_pll: # TX PLL CAL
                index=0
                vcocap_addr      = [0xDB, [14,8]]
                fcal_window_addr = [0xD1, [14,0]]
                fcal_cnt_op_addr = [0xD0, [15,0]]
                fcal_pd_cal_addr = [0xDB,    [6]]
                fcal_lo_open_addr= [0xD9,   [11]]
                fcal_start_addr  = [0xD1,   [15]]
                fcal_done_addr   = [0xCF,   [15]]
            else: # RX PLL CAL 
                index=1
                vcocap_addr      = [0x1F5,[15,9]]
                fcal_window_addr = [0x1EA,[14,0]]
                fcal_cnt_op_addr = [0x1E9,[15,0]]
                fcal_pd_cal_addr = [0x1F5,   [6]]
                fcal_lo_open_addr= [0x1F3,  [11]]
                fcal_start_addr  = [0x1EA,  [15]]
                fcal_done_addr   = [0x1E8,  [15]]

            
            #if(log_en): print >> f2, "Lane,Dir, Cap, Target, ReadOut"          
            orig_vcocap = rreg(vcocap_addr,ln)  # original cap value

            ### Sweep upward and find min vcocap
            wreg(vcocap_addr,0x0,ln)
            wreg(fcal_window_addr,window,ln)
            target_cnt = rreg(fcal_window_addr,ln) # set counter to the reference frequency
            while 1:
                wreg(fcal_pd_cal_addr, 0,ln)  # PD_CAL_FCAL = 0  
                wreg(fcal_lo_open_addr, 0,ln) # LO_OPEN = 0 
                wreg(fcal_start_addr, 0,ln) # fcal_start = 0  
                wreg(fcal_start_addr, 1,ln) # fcal_start = 1 
                
                while(rreg(fcal_done_addr,ln) == 0):  #check fcal_done. If it's done, continue to the next
                    pass
                readout = rreg(fcal_cnt_op_addr,ln)  # read out the counter value(fcal_cnt_op[15:0]).
                vcocap = rreg(vcocap_addr,ln)
                #if(log_en): print >> f2, "%4d, Up, %3d,    %04X,   %04X"%(ln, vcocap,target_cnt,readout)
                if(abs(readout-target_cnt) > 2):  
                    if (vcocap == 0x7f):
                        vcocap_min = 0
                        rx_lock = 1
                        #if(log_en): print >> f2, "Lane %d,  UP: The frequency target_cnt is out of pll range(min)"%ln
                        break
                    else:
                        vcocap = rreg(vcocap_addr,ln) + 0x1
                        wreg(vcocap_addr, vcocap,ln)
                else:
                    vcocap_min = rreg(vcocap_addr,ln)
                    #if(log_en): print >> f2,  "Min vcocap = %d" % (vcocap_min)
                    break

            ### Sweep downward and find max vcocap
            wreg(vcocap_addr,0x7f,ln)
            wreg(fcal_window_addr,window,ln)
            while 1:
                wreg(fcal_pd_cal_addr, 0,ln)
                wreg(fcal_lo_open_addr, 0,ln)
                wreg(fcal_start_addr, 0,ln)
                # fcal start pulse delay
                wreg(fcal_start_addr, 1,ln)
                # fcal done check delay
                while(rreg(fcal_done_addr,ln) == 0):
                    pass

                readout = rreg(fcal_cnt_op_addr,ln)
                vcocap = rreg(vcocap_addr,ln)
                #if(log_en): print >> f2, "%4d, Dn, %3d,    %04X,   %04X"%(ln, vcocap,target_cnt,readout)

                if(abs(readout-target_cnt) > 2):
                    if (vcocap == 0x0):
                        vcocap_max = 0
                        rx_lock = 1
                        #if(log_en): print >> f2, "Lane %d,Down: The frequency target_cnt is out of pll range(max)"%ln
                        break
                    else:
                        vcocap = rreg(vcocap_addr,ln) - 0x1
                        wreg(vcocap_addr, vcocap,ln)
                else:
                    vcocap_max = rreg(vcocap_addr,ln)
                    #if(log_en): print >> f2,  "Max vcocap = %d" % (vcocap_max)
                    break
                    
            ### Find the optimum cap from min and max
            if (vcocap_max == 0x7f) and (vcocap_min == 0x00): # Search FAILED
                new_vcocap = orig_vcocap
            else: # Search SUCCESSFUL
                new_vcocap = int((vcocap_max + vcocap_min)/2) 
                if   ((vcocap_max == 0x7f) and ((vcocap_max-vcocap_min)<4)):
                    new_vcocap = vcocap_max
                elif ((vcocap_min == 0x00) and ((vcocap_max-vcocap_min)<4)):
                    new_vcocap = vcocap_min

            wreg(vcocap_addr, new_vcocap,ln)
            pll_cal_result[index]=[vcocap_min,new_vcocap,vcocap_max]
            flag= '>' if orig_vcocap!=new_vcocap else ','
            if print_en: print ("%3d, %3d%s%3d, %3d, " % (vcocap_min,orig_vcocap,flag,new_vcocap,vcocap_max))
            #if(log_en): print ("\nLane %2d %s VCOCAPS: Min/Center(Orig)/Max: %2d / %2d(%2d) / %2d\n" % (ln,lane_pll,vcocap_min,new_vcocap,orig_vcocap,vcocap_max),file=f2)
            
            ### Disable TX or RX PLL CAL circuit after done with this Lane's PLL
            wreg(fcal_lo_open_addr, 0,ln)
            wreg(fcal_start_addr,   0,ln)
            wreg(fcal_window_addr,  0,ln)
            wreg(fcal_pd_cal_addr,  1,ln)
            ### end of this PLL, TX or RX
            
        result[ln]=[pll_cal_result[0],pll_cal_result[1]]
        #### End of this lane
    
    if print_en: print ("\n")
    #if(log_en): f2.close()
    if print_en==0: return result

####################################################################################################
# set_lane_pll ()
#
# Programs TXPLL or RXPLL according to PLL parameter(s) passed
#
# returns status of PLL programming (success/fail)
#
#################################################################################################### 
def set_lane_pll (tgt_pll=None, datarate=None, fvco=None, cap=None, n=None, div4=None, div2=None, refclk=None, frac_en=None, frac_n=None, lane=None):

    if tgt_pll is None:
        if datarate is None:
            print(" \tEnter one or more of the following arguments:")
            print(" \tset_lane_pll(tgt_pll='both'/'rx'/'tx', datarate, fvco, cap, n, div4, div2, refclk, lane)")
            rn
        else:
            tgt_pll='both' # set both PLLs with datarate passed
    
    fvco_max = 33.0
    fvco_min = 15.0
    pll_n_max = 511 # (0x1FF)

    lanes = get_lane_list(lane)      # determine lanes to work on
    #get_lane_mode(lanes)         # Get current PAM4/NRZ modes settings of the specified lane(s)
    pll_params = get_lane_pll(lanes) # Get current PLL settings of the specified lane(s)

    #pll_params = [list(x) for x in pll_params_curr]
    if 'tx' in tgt_pll:
        desired_pll = [0]
    elif 'rx' in tgt_pll:
        desired_pll = [1]
    else: #if tgt_pll=='both':
        desired_pll = [0,1]
    
    # Temporary holders for each lane/pll
    # lane_datarate=[0,1]
    # lane_fvco    =[0,1]
    lane_cap     =[0,1]
    lane_n       =[0,1]
    lane_div4    =[0,1]
    lane_div2    =[0,1]
    lane_refclk  =[0,1]
    lane_frac_n  =[0,1]
    lane_frac_en =[0,1]
  
    ####### determine desired PLL parameters if passed, otherwise re-use current values from chip
    for ln in lanes:
        get_lane_mode(ln)
        #### Do this per PLL (TXPLL and/or RXPLL) of each lane
        for pll in desired_pll:
            lane_cap   [pll] = pll_params[ln][pll][2] if    cap  is  None else     cap
            lane_n     [pll] = pll_params[ln][pll][3] if      n  is  None else       n
            lane_div4  [pll] = pll_params[ln][pll][4] if   div4  is  None else    div4
            lane_div2  [pll] = pll_params[ln][pll][5] if   div2  is  None else    div2
            lane_refclk[pll] = pll_params[ln][pll][6] if refclk  is  None else  refclk
            lane_frac_n[pll] = pll_params[ln][pll][7] if frac_n  is  None else  frac_n
            lane_frac_en[pll]= pll_params[ln][pll][8] if frac_en is  None else  frac_en

            ####### Calculate 'desired_n' per lane per pll (only if Data Rate or Fvco is passed)
            if datarate != None or fvco != None:
                if datarate != None:
                    desired_data_rate = float(datarate)
                    if desired_data_rate > fvco_max:
                        data_rate_to_fvco_ratio = 2.0    ##### PAM4 Data Rate, > 33 Gbps
                        wreg(c.rx_pam4_en_addr,    1,ln) # RX_PAM4_EN=1
                        wreg(c.tx_nrz_mode_addr,   0,ln) # TX_NRZ_EN=0
                        wreg(c.tx_mode10g_en_addr, 0,ln) # TX_NRZ_10G_EN=0 (or Disable NRZ Half-Rate Mode)
                        wreg(c.rx_mode10g_addr,    0,ln) # RX_NRZ_10G_EN=0 (or Disable NRZ Half-Rate Mode)
                    elif desired_data_rate < fvco_min:
                        data_rate_to_fvco_ratio = 0.5    ##### NRZ Half-Rate Data Rate, < 15 Gbps
                        wreg(c.rx_pam4_en_addr,    0,ln) # RX_PAM4_EN=0
                        wreg(c.tx_nrz_mode_addr,   1,ln) # TX_NRZ_EN=1
                        wreg(c.tx_mode10g_en_addr, 1,ln) # TX_NRZ_10G_EN=1 (or Enable NRZ Half-Rate Mode)
                        wreg(c.rx_mode10g_addr,    1,ln) # RX_NRZ_10G_EN=1 (or Enable NRZ Half-Rate Mode)
                    else: 
                        data_rate_to_fvco_ratio = 1.0    ##### NRZ Full-Rate Data Rate, 15 Gbps to 33 Gbps
                        wreg(c.rx_pam4_en_addr,    0,ln) # RX_PAM4_EN=0
                        wreg(c.tx_nrz_mode_addr,   1,ln) # TX_NRZ_EN=1
                        wreg(c.tx_mode10g_en_addr, 0,ln) # TX_NRZ_10G_EN=0 (or Disable NRZ Half-Rate Mode)
                        wreg(c.rx_mode10g_addr,    0,ln) # RX_NRZ_10G_EN=0 (or Disable NRZ Half-Rate Mode)
                        
                    desired_fvco = desired_data_rate / data_rate_to_fvco_ratio
                else:
                    desired_fvco = float(fvco)
                    
                if desired_fvco >fvco_max: desired_fvco=fvco_max
                if desired_fvco <fvco_min: desired_fvco=fvco_min
                
                ### Now, calculate PLL_N per lane, from desired_fvco
                div_by_4 = 1.0 if lane_div4[pll]==0 else 4.0
                mul_by_2 = 1.0 if lane_div2[pll]==1 else 2.0
                
                desired_n = int( (1000.0 * desired_fvco * div_by_4 ) / ( 2.0 * mul_by_2 * lane_refclk[pll] ) )
                if desired_n > pll_n_max: desired_n = pll_n_max
                lane_n[pll]=desired_n

        #update this lane's TXPLL and RXPLL params
        pll_params[ln] = [(0,0,lane_cap[0],lane_n[0],lane_div4[0],lane_div2[0],lane_refclk[0],lane_frac_n[0],lane_frac_en[0]),(0,0,lane_cap[1],lane_n[1],lane_div4[1],lane_div2[1],lane_refclk[1],lane_frac_n[1],lane_frac_en[1])]
            
    ###### Now program both PLLs of the specified lane(s)
    for ln in lanes:
        wreg(c.rx_pll_pu_addr, 0, ln) # Power down RX PLL while prgramming PLL
        wreg(c.tx_pll_pu_addr, 0, ln) # Power down TX PLL while prgramming PLL
        
        if 'tx' in tgt_pll or tgt_pll=='both':
            wreg(c.tx_pll_lvcocap_addr, pll_params[ln][0][2], ln)
            wreg(c.tx_pll_n_addr,       int(pll_params[ln][0][3]), ln)
            wreg(c.tx_pll_div4_addr,    pll_params[ln][0][4], ln)
            wreg(c.tx_pll_div2_addr,    pll_params[ln][0][5], ln)
            wreg(c.tx_pll_frac_n_addr,  pll_params[ln][0][7], ln)
            wreg(c.tx_pll_frac_en_addr, pll_params[ln][0][8], ln)
    
        if 'rx' in tgt_pll or tgt_pll=='both':
            wreg(c.rx_pll_lvcocap_addr, pll_params[ln][1][2], ln)
            wreg(c.rx_pll_n_addr,       int(pll_params[ln][1][3]), ln)
            wreg(c.rx_pll_div4_addr,    pll_params[ln][1][4], ln)
            wreg(c.rx_pll_div2_addr,    pll_params[ln][1][5], ln)
            wreg(c.rx_pll_frac_n_addr,  pll_params[ln][1][7], ln)
            wreg(c.rx_pll_frac_en_addr, pll_params[ln][1][8], ln)
            
        wreg(c.rx_pll_pu_addr, 1, ln) # Power up RX PLL after prgramming PLL
        wreg(c.tx_pll_pu_addr, 1, ln) # Power up TX PLL after prgramming PLL

    
####################################################################################################
# get_lane_pll ()
#
# reads all PLL related registers and computes PLL Frequencies and Data Rates
#
# returns all TXPLL and RXPLL parameters for the specified lane(s)
#
####################################################################################################    
def get_lane_pll (lane=None):

    lanes = get_lane_list(lane)
    #get_lane_mode(lanes)

    pll_params = {}
    
    #ref_clk=195.3125
    #ref_clk=156.25
    ref_clk = gRefClkFreq
    

    for ln in lanes:
        tx_div4_en        = rreg(c.tx_pll_div4_addr,       ln)
        tx_div2_bypass    = rreg(c.tx_pll_div2_addr,       ln)
        tx_pll_n          = rreg(c.tx_pll_n_addr,          ln)
        tx_pll_cap        = rreg(c.tx_pll_lvcocap_addr,    ln)
        tx_10g_mode_en    = rreg(c.tx_mode10g_en_addr,     ln) # NRZ 10G (or NRZ Half-Rate Mode)
        tx_pll_frac_n     = rreg(c.tx_pll_frac_n_addr,     ln) # Fractional_PLL_N
        if chip_rev()==2.0: # For R2.0, TX Frac-N is 20 bits
            tx_pll_frac_n_hi = rreg([0x0d9,[3,0]],         ln) # Fractional_PLL_N[19:16]
            tx_pll_frac_n_lo = rreg(c.tx_pll_frac_n_addr,  ln) # Fractional_PLL_N[15:0]
            tx_pll_frac_n = (tx_pll_frac_n_hi << 16) + tx_pll_frac_n_lo
        #tx_pll_frac_order = rreg(c.tx_pll_frac_order_addr, ln) # Fractional_PLL_ORDER
        tx_pll_frac_en    = rreg(c.tx_pll_frac_en_addr,    ln) # Fractional_PLL_EN
        
        rx_div4_en        = rreg(c.rx_pll_div4_addr,       ln)
        rx_div2_bypass    = rreg(c.rx_pll_div2_addr,       ln)
        rx_pll_n          = rreg(c.rx_pll_n_addr,          ln)
        rx_pll_cap        = rreg(c.rx_pll_lvcocap_addr,    ln)
       #rx_10g_mode_en    = rreg(c.rx_mode10g_addr,        ln) # checking TX 10G is enough
        rx_pll_frac_n     = rreg(c.rx_pll_frac_n_addr,     ln) # Fractional_PLL_N
        #rx_pll_frac_order = rreg(c.rx_pll_frac_order_addr, ln) # Fractional_PLL_ORDER
        rx_pll_frac_en    = rreg(c.rx_pll_frac_en_addr,    ln) # Fractional_PLL_EN
        
        tx_div_by_4 = 1.0 if tx_div4_en==0     else 4.0
        rx_div_by_4 = 1.0 if rx_div4_en==0     else 4.0
        tx_mul_by_2 = 1.0 if tx_div2_bypass==1 else 2.0
        rx_mul_by_2 = 1.0 if rx_div2_bypass==1 else 2.0
        
        pam4_mode_en = 1 if (rreg(c.tx_nrz_mode_addr,ln) == 0 and rreg(c.rx_pam4_en_addr,ln) == 1) else 0
        if pam4_mode_en: ########## Lane is in PAM4 mode (i.e. 33G =< data rate < 63G)
            data_rate_to_fvco_ratio = 2.0 
        else: ##################### Lane is in NRZ mode
            if tx_10g_mode_en==0: # Lane is in NRZ Full Rate mode (i.e. 15G < data rate < 33G)
                data_rate_to_fvco_ratio = 1.0
            else:                 # Lane is in NRZ Half Rate mode (i.e. data rate =< 15G) 
                data_rate_to_fvco_ratio = 0.5
            
        if chip_rev() == 2.0:
            tx_pll_n_float = float(tx_pll_n) + float(tx_pll_frac_n/1048575.0) if tx_pll_frac_en else float(tx_pll_n)
        else:
            tx_pll_n_float = float(tx_pll_n) + float(tx_pll_frac_n/65535.0)  if tx_pll_frac_en else float(tx_pll_n)        

        rx_pll_n_float = float(rx_pll_n) + float(rx_pll_frac_n/65535.0) if rx_pll_frac_en else float(rx_pll_n)
        
        tx_fvco = (ref_clk * tx_pll_n_float * 2.0 * tx_mul_by_2) / tx_div_by_4 / 1000.0  # in GHz
        rx_fvco = (ref_clk * rx_pll_n_float * 2.0 * rx_mul_by_2) / rx_div_by_4 / 1000.0  # in GHz

        tx_data_rate = tx_fvco * data_rate_to_fvco_ratio 
        rx_data_rate = rx_fvco * data_rate_to_fvco_ratio 
        
        tx_pll_params = tx_data_rate, tx_fvco, tx_pll_cap, tx_pll_n_float, tx_div4_en, tx_div2_bypass, ref_clk, tx_pll_frac_en, tx_pll_frac_n
        rx_pll_params = rx_data_rate, rx_fvco, rx_pll_cap, rx_pll_n_float, rx_div4_en, rx_div2_bypass, ref_clk, rx_pll_frac_en, rx_pll_frac_n
        
        pll_params[ln] = [tx_pll_params,rx_pll_params]
        
    return pll_params    #data_rate, fvco, pll_cap, pll_n, div4, div2, ref_clk
#################################################################################################### 
# pll()
#
# This is intended for python command line usage, to get or set PLL
# 
# See get_lane_pll() and  set_lane_pll() for actual PLL functions
#
####################################################################################################
def pll (tgt_pll=None, datarate=None, fvco=None, cap=None, n=None, div4=None, div2=None, refclk=None, frac_en=None, frac_n=None, lane=None):

    lanes = get_lane_list(lane)
    
    if datarate!=None or fvco!=None or cap!=None or n!=None or div4!=None or div2!=None or refclk!=None or frac_en!=None or frac_n!=None: # desired_pll_n or desired_data_rate, then program PLL registers
        set_lane_pll(tgt_pll, datarate, fvco, cap, n, div4, div2, refclk, frac_en, frac_n, lane)
    
    #get_lane_mode(lanes) # Now update the lanes' modes (PAM4 or NRZ) before printing them out
    pll_params = get_lane_pll(lanes)
    
    ##### Print Headers
    print("\n PLL Parameters for Slice %d with RefClk: %8.4f MHz\n"%(gSlice,pll_params[lanes[0]][0][6])),
    print("\n+------------+------------- T X  P L L --------------------+------------- R X  P L L --------------------+"),
    print("\n|Lane | mode | DataRate,     Fvco, CAP,       N, DIV4, DIV2| DataRate,     Fvco, CAP,       N, DIV4, DIV2|"),
    print("\n+------------+---------------------------------------------+---------------------------------------------+"),
    
    for ln in lanes:
        get_lane_mode(ln)
        print("\n|S%d_%2s| %4s |"%(gSlice,lane_name_list[ln],gEncodingMode[gSlice][ln][0].upper())),
        for pll in [0,1]:
            print("%8.5f, %8.5f, %3d, %7.3f, %3d , %3d |" %(pll_params[ln][pll][0], pll_params[ln][pll][1], pll_params[ln][pll][2], pll_params[ln][pll][3],  pll_params[ln][pll][4],  pll_params[ln][pll][5])),
        if ln==lanes[-1] or ln==7:
            print("\n+------------+---------------------------------------------+---------------------------------------------+"),
####################################################################################################
## EYE Margin, by Python directly accessing the HW registers
####################################################################################################
def eye(lane = None):
    lanes = get_lane_list(lane)
    result = {}
    fw_eye_en = fw_loaded(print_en=0) and fw_date(print_en=0)>=18015 and fw_reg_rd(128)!=0 # FW Date 20190429 and later
    for ln in lanes:    
        get_lane_mode(ln)
        line_encoding = lane_mode_list[ln].lower()
        c = Pam4Reg if line_encoding == 'pam4' else NrzReg
        x = 100 if line_encoding == 'pam4' else 200
        rdy = sum(ready(ln)[ln])        
        em=[-1,-1,-1]
        ##### PAM4 EYE
        if line_encoding == 'pam4' and rdy == 3:
            ####### FW-based Eye margin
            # if FW is loaded and the Background FW is not paused
            # if fw_loaded and fw_pause(print_en=0)[0][2] == 1:
            if fw_eye_en == True:        
                fw_debug_cmd(section=10, index=5, lane=ln)
                em = [rreg(0x9f00+eye_index) for eye_index in range(3)]
            ####### SW-based Eye margin. Direct access to HW registers
            else:
                eye_margin = []
                dac_val = dac(lane=ln)[ln]
                for eye_index in range (0,3):
                    result1 = 0xffff
                    for y in range (0,4):
                        sel = 3 * y + eye_index
                        wreg(c.rx_plus_margin_sel_addr, sel, ln)
                        plus_margin = rreg(c.rx_plus_margin_addr, ln)
                        if (plus_margin > 0x7ff):
                            plus_margin = plus_margin - 0x1000
                        wreg(c.rx_minus_margin_sel_addr, sel, ln)
                        minus_margin = (rreg(c.rx_minus_margin_addr, ln))
                        if (minus_margin > 0x7ff):
                            minus_margin = minus_margin - 0x1000
                        diff = plus_margin - minus_margin
                        if ( diff < result1):
                            result1 = diff
                        # else:
                            # result1 = result1
                    eye_margin.append((result1))
                    em[eye_index] = (float(eye_margin[eye_index])/2048.0) * (x + (50.0 * float(dac_val)))
        ##### NRZ EYE
        elif line_encoding == 'nrz' and rdy == 3: 
            dac_val = dac(lane=ln)[ln]
            eye_reg_val = rreg(c.rx_em_addr, ln)
            em[0] = (float(eye_reg_val) / 2048.0) * (x + (50.0 * float(dac_val)))            
            em[1]=0;em[2]=0
 
        result[ln] = int(em[0]),int(em[1]),int(em[2])        
    return result
####################################################################################################
## EYE Margin, by Python directly accessing the HW registers
####################################################################################################
def sw_eye(lane = None):
    lanes = get_lane_list(lane)
    result = {}
    for ln in lanes:    
        get_lane_mode(ln)
        line_encoding = lane_mode_list[ln].lower()
        c = Pam4Reg if line_encoding == 'pam4' else NrzReg
        x = 100 if line_encoding == 'pam4' else 200
        rdy = sum(ready(ln)[ln])
        em=[-1,-1,-1]
        ##### PAM4 EYE
        if line_encoding == 'pam4' and rdy == 3:
            eye_margin = []
            ####### FW-based Eye margin
            # if FW is loaded and the Background FW is not paused
            # if fw_loaded and fw_pause(print_en=0)[0][2] == 1:
            # if False:# fw_loaded and fw_reg_rd(128)!=0:
                # print ("FW EYE MARGIN Lane %d"%ln)
                # fw_debug_cmd(section=10, index=5, lane=ln)
                # em = [rreg(0x9f00+eye_index) for eye_index in range(3)]
            # ####### SW-based Eye margin. Direct access to HW registers
            # else:
            dac_val = dac(lane=ln)[ln]
            for eye_index in range (0,3):
                result1 = 0xffff
                for y in range (0,4):
                    sel = 3 * y + eye_index
                    wreg(c.rx_plus_margin_sel_addr, sel, ln)
                    plus_margin = rreg(c.rx_plus_margin_addr, ln)
                    if (plus_margin > 0x7ff):
                        plus_margin = plus_margin - 0x1000
                    wreg(c.rx_minus_margin_sel_addr, sel, ln)
                    minus_margin = (rreg(c.rx_minus_margin_addr, ln))
                    if (minus_margin > 0x7ff):
                        minus_margin = minus_margin - 0x1000
                    diff = plus_margin - minus_margin
                    if ( diff < result1):
                        result1 = diff
                    # else:
                        # result1 = result1
                eye_margin.append((result1))
                em[eye_index] = (float(eye_margin[eye_index])/2048.0) * (x + (50.0 * float(dac_val)))
        ##### NRZ EYE
        elif line_encoding == 'nrz' and rdy == 3: 
            dac_val = dac(lane=ln)[ln]
            eye_reg_val = rreg(c.rx_em_addr, ln)
            em[0] = (float(eye_reg_val) / 2048.0) * (x + (50.0 * float(dac_val)))            
            em[1]=0;em[2]=0
 
        result[ln] = int(em[0]),int(em[1]),int(em[2])        
    return result
####################################################################################################
## EYE Margin, by Firmware
####################################################################################################
def fw_eye(lane = None):

   #fw_eye_en = fw_loaded(print_en=0) and fw_date(print_en=0)>=18015 and fw_reg_rd(128)!=0 # FW Date 20190429 or later
    fw_eye_en = fw_loaded(print_en=0) and fw_ver(print_en=0)>=0x20900 and fw_reg_rd(128)!=0 # FW 2.09.00 and later
    if fw_eye_en == False:
        print("\n*** FW Not Loaded, or "),
        print("\n*** FW Eye Margin function Not Available In This Release (Need DateCode 20190429 or later), or"),
        print("\n*** FW Background Functions are Disabled (FW REG 128 = 0 instead of 0xFFFF)\n"),
        return -1, -1, -1
        
    lanes = get_lane_list(lane)
    result = {}
    for ln in lanes:    
        get_lane_mode(ln)
        line_encoding = lane_mode_list[ln].lower()
        c = Pam4Reg if line_encoding == 'pam4' else NrzReg
        x = 100 if line_encoding == 'pam4' else 200
        rdy = sum(ready(ln)[ln])
        em=[-1,-1,-1]
        ##### PAM4 EYE
        if line_encoding == 'pam4' and rdy == 3:
            fw_debug_cmd(section=10, index=5, lane=ln)
            em = [rreg(0x9f00+eye_index) for eye_index in range(3)]
        ##### NRZ EYE
        elif line_encoding == 'nrz' and rdy == 3: # NRZ EYE
            dac_val = dac(lane=ln)[ln]
            eye_reg_val = rreg(c.rx_em_addr, ln)
            em[0] = (float(eye_reg_val) / 2048.0) * (x + (50.0 * float(dac_val)))            
            em[1] = 0
            em[2] = 0
        result[ln] = int(em[0]),int(em[1]),int(em[2])                
    return result

####################################################################################################
## PAM4 EYE Margin, by FW or by Python directly accessing HW registers
####################################################################################################
def eye_pam4(lane=None):
    #c = Pam4Reg
    #c.rx_plus_margin_sel_addr  = [0x88,[15,12]]
    #c.rx_minus_margin_sel_addr = [0x88,[11,8]]
    #c.rx_plus_margin_addr = [0x32,[15,4]]
    #c.rx_minus_margin_addr = [0x32, [3,0], 0x33, [15,8]]
    #c.rx_minus_margin_upper_addr = [0x32,[3,0]]
    #c.rx_minus_margin_lower_addr = [0x33,[15,8]]

    fw_eye_en = fw_loaded(print_en=0) and fw_date(print_en=0)>=18015 and fw_reg_rd(128)!=0 # FW Date 20190429 and later
    
    lanes = get_lane_list(lane)
    eyes = {}
    for ln in lanes:     
        get_lane_mode(ln)
        line_encoding = lane_mode_list[ln].lower()
        c = Pam4Reg if line_encoding == 'pam4' else NrzReg
        x = 100 if line_encoding == 'pam4' else 200
        rdy = sum(ready(ln)[ln])
        em=[-1,-1,-1]
        if line_encoding == 'pam4' and rdy == 3:
            ####### FW-based PAM4 Eye margin
            # if FW is loaded and the Background FW is not paused
            # if fw_loaded and fw_pause(print_en=0)[0][2] == 1:
            if fw_eye_en == True:      
                fw_debug_cmd(section=10, index=5, lane=ln)
                em = [rreg(0x9f00+eye_index) for eye_index in range(3)]
                eyes[ln] = em[0], em[1], em[2]
            ####### SW-based PAM4 Eye margin. Direct access to HW registers
            else:
                eye_margin = []        
                dac_val = dac(lane=ln)[ln]
                for eye_index in range (3):
                    result1 = 0xffff
                    for y in range (4):
                        sel = 3 * y + eye_index
                        wreg(c.rx_minus_margin_sel_addr, sel, ln)
                        wreg(c.rx_plus_margin_sel_addr, sel, ln)
                        plus_margin = rreg(c.rx_plus_margin_addr, ln)
                        if (plus_margin > 0x7ff):
                            plus_margin = plus_margin - 0x1000
                        minus_margin = (rreg(c.rx_minus_margin_addr, ln))
                        if (minus_margin > 0x7ff):
                            minus_margin = minus_margin - 0x1000
                        diff = plus_margin - minus_margin
                        if ( diff < result1):
                            result1 = diff
                        # else:
                            # result1 = result1
                        #result1 = result1 + diff
                    eye_margin.append((result1))
                em0 = (float(eye_margin[0])/2048.0) * (x + (50.0 * float(dac_val)))
                em1 = (float(eye_margin[1])/2048.0) * (x + (50.0 * float(dac_val)))
                em2 = (float(eye_margin[2])/2048.0) * (x + (50.0 * float(dac_val)))
                eyes[ln] = em0, em1, em2
    return eyes
 ####################################################################################################
# Resets the PRBS Error Counter for the selected lane
# 
####################################################################################################
def prbs_rst(lane = None):
    lanes = get_lane_list(lane)
    for lane in lanes:
        c = Pam4Reg if lane_mode_list[lane].lower() == 'pam4' else NrzReg
        wreg(c.rx_err_cntr_rst_addr, 0, lane)
        time.sleep(0.001)
        wreg(c.rx_err_cntr_rst_addr, 1, lane)
        time.sleep(0.001)
        wreg(c.rx_err_cntr_rst_addr, 0, lane)
        
        
####################################################################################################
# Collects the PRBS Error Counter for the selected lane
# 
# It resets the counter first if rst=1
####################################################################################################
def get_prbs(lane = None, rst=0, print_en=1):
    lanes = get_lane_list(lane)
    
    ###### 1. Clear Rx PRBS Counter for these lanes, if requested
    if (rst==1): 
        prbs_rst(lane=lanes)
        
    result = {}
    ###### 2. Capture PRBS Counter for this lane
    for ln in lanes:
        c = Pam4Reg if lane_mode_list[ln].lower() == 'pam4' else NrzReg
        result[ln] = long(rreg(c.rx_err_cntr_msb_addr, ln)<<16) + rreg(c.rx_err_cntr_lsb_addr, ln)
 
    return result

####################################################################################################
# 
# flips the RX polarity, but user can ask to flip the TX polarity instead
#
####################################################################################################
def flip_pol(lane=None, port='rx', print_en=1):

    if lane is None and (type(lane)!=int or type(lane)!=list):
        print("\n   ...Usage.................................................................")
        print("\n      flip_pol (lane=0)            # flip RX polarity of lane 0"),
        print("\n      flip_pol (lane=0, port='rx') # flip RX polarity of lane 0"),
        print("\n      flip_pol (lane=0, port='TX') # flip TX polarity of lane 0"),
        print("\n      flip_pol (lane=[0,3,5])      # flip RX polarities of lanes 0, 3 and 5"),
        print("\n   .........................................................................")
        return

    lanes = get_lane_list(lane)
    result={}
    for ln in lanes:
        orig_tx_pol,orig_rx_pol =  pol(lane=ln, print_en=0)
        print("\nLane %d, TX Pol: (%d -> %d), RX Pol: (%d -> %d)"%(ln,orig_tx_pol,int(not orig_tx_pol),orig_rx_pol,int( not orig_rx_pol))),
        if port.upper() != 'TX': # if asked to reverse RX Polarity (default selection)
            new_tx_pol= orig_tx_pol
            new_rx_pol= int(not orig_rx_pol)
        else: # if asked to reverse TX Polarity
            new_tx_pol= int(not orig_tx_pol)
            new_rx_pol= orig_rx_pol
        
        pol(tx_pol=new_tx_pol, rx_pol= new_rx_pol, lane=ln, print_en=0)
            
        result[ln] = pol(lane=ln, print_en=0)
        #print result[ln]
    
    if print_en: 
        pol(lane=range(len(lane_name_list)), print_en=1)
        
    if print_en==0: return result
####################################################################################################
#
####################################################################################################
def scan_lane_partner(slice=[0,1], lane='all', print_en=True):

    global gLanePartnerMap
    
    TX_lanes  = get_lane_list(lane)
    TX_slices = get_slice_list(slice)
    rx_slices=[0,1]
    rx_lanes=range(16)
    rx_partner=[-1,-1]

    ### Create List for gLanePartnerMap
    # tx_map_slice_lane=[]
    # for slc in range(2):
        # tx_map_slice_lane.append([])
        # for ln in range(16):
            # tx_map_slice_lane[slc].append([])
    
    ### Turn all target lanes' TX output
    for TX_slc in TX_slices:
        sel_slice(TX_slc)
        tx_output(mode='on', lane=TX_lanes, print_en=0)

    if print_en:
        print("\n Scanning for Lane Partners...\n"),
        print("\n Slice_Lane               Slice_Lane"),
        print("\n         TX <-----------> RX "),

    ### Loop through all target slices/lanes (and find each one's RX partner)
    for TX_slc in TX_slices:
        for TX_ln in TX_lanes:
            if print_en:
                if TX_ln%4==0: print ("")
                print("\n    [S%d_%s]"%(TX_slc,lane_name_list[TX_ln])),
            num_lanes_checked=0
            lane_not_rdy_cnt=0
            ### Turn off this lane's TX
            sel_slice(TX_slc)
            tx_output(mode='off', lane=TX_ln, print_en=0)
            
            ### Search for RX partner of this TX
            for rx_slc in rx_slices:
                sel_slice(rx_slc)
                rdy_rx_ln = {}
                for rx_ln in rx_lanes:
                    rdy_rx_ln[rx_ln] = phy_rdy(rx_ln)[rx_ln]
                    #print("\n RX Slice %d Lane %d RDY = %d"%(rx_slc,rx_ln,rdy_rx_ln[rx_ln])),
                    if rdy_rx_ln[rx_ln]==0:# and lane_not_rdy_cnt==0:
                        rx_partner=[rx_slc,rx_ln]
                        conn = '<-LoopBack-->' if [rx_slc,rx_ln] == [TX_slc, TX_ln] else '<---Cable--->'
                        print ("%s [S%d_%s]" % (conn, rx_slc,lane_name_list[rx_ln]))
                        gLanePartnerMap[TX_slc][TX_ln] = [rx_slc,rx_ln]
                        lane_not_rdy_cnt+=1
                    # else:
                        # print "\nSlice %d Lane %d" % (rx_slc,rx_ln),                
                    num_lanes_checked+=1
                    
            ### Found this lane's partner. Turn its TX back on. Wait for its partner's RX to come ready before moving on to next TX_lane     
            sel_slice(TX_slc)
            tx_output(mode='on', lane=TX_ln, print_en=0)
            start=time.time()
            wait_time=time.time()-start
            sel_slice(rx_partner[0])
            while phy_rdy(rx_partner[1])[rx_partner[1]] == 0:
                wait_time=time.time()-start
                if wait_time>2.0:
                    break
            if print_en and lane_not_rdy_cnt!=1: 
                print(" (%d found for this lane, wait time = %2.1f)"%(lane_not_rdy_cnt, wait_time)),               
    if print_en==0:
        return gLanePartnerMap
####################################################################################################
# Automatically determines the correct polarity of the RX input, based on PRBS Error Counter
# 
# By default flips the RX polarity if detects a wrong polarity, but user can ask to flip the TX polarity instead
#
####################################################################################################
def auto_pol(port='rx', tx_prbs='en', print_en=1):
    #print'here'
    lanes = range(0,16) #get_lane_list(lane)
    #get_lane_mode(lanes)
    
    result={}

    if print_en:
        tx_gen = 'TX PRBS Gen Forced Enabled' if 'en' in tx_prbs else 'TX PRBS Gen NOT Forced Enabled'
        print("\n\n...Slice %d Lanes %d-%d Auto Polarity with %s..."%(gSlice,lanes[0],lanes[-1], tx_gen)),
        print("\n                              #   Lane No: [A0.A1.A2.A3.A4.A5.A6.A7,B0.B1.B2.B3.B4.B5.B6.B7 ]"),
        if port.upper() != 'TX':
            print("\nRxPolarityMap.append([]); RxPolarityMap[%d]=["%(gSlice) ),
        else:
            print("\nTxPolarityMap.append([]); TxPolarityMap[%d]=["%(gSlice) ),
    
    # Make sure both PRBS TX Generator and RX Checker are enabled and both are set to PRBS31
    for ln in lanes:
        get_lane_mode(ln)
        #c = Pam4Reg if lane_mode_list[ln].lower() == 'pam4' else NrzReg
        
        tx_prbs_gen_en = rreg([0x0a0,[14]],ln) # NRZ/PAM4 mode, PRBS Gen clock en
        #rx_prbs_checker_en = rreg([0x043, [3]],ln) if c==Pam4Reg else rreg([0x161,[10]],ln) # PAM4 or NRZ mode, PRBS Sync Checker powered up
        if (tx_prbs =='en' and tx_prbs_gen_en==0): # or rx_prbs_checker_en==0:
            prbs_mode_select(lane=ln, prbs_mode='prbs')
    
    for ln in lanes:
        #c = Pam4Reg if lane_mode_list[ln].lower() == 'pam4' else NrzReg
        
        if (tx_prbs =='en'): 
            prbs_mode_select(lane=ln, prbs_mode='prbs')
        
        ##Clear and read Rx PRBS Counter for this lane
        prbs_cnt_before = get_prbs(lane = ln, rst=1, print_en=0)[ln]
        
        orig_tx_pol,orig_rx_pol =  pol(tx_pol=None, rx_pol=None, lane=ln, print_en=0)

        if port.upper() != 'TX': # if asked to reverse RX Polarity (default selection)
            pol(tx_pol= orig_tx_pol, rx_pol=(int(not orig_rx_pol)), lane=ln, print_en=0)
        else: # if asked to reverse TX Polarity
            pol(tx_pol=(int(not orig_tx_pol)), rx_pol= orig_rx_pol, lane=ln, print_en=0)
        
        prbs_cnt_after = get_prbs(lane = ln, rst=1, print_en=0)[ln]
            
        if (prbs_cnt_before==0) or (prbs_cnt_before * 10 <= prbs_cnt_after): # if at least one order not improved, keep the orginal polarity
            pol(tx_pol= orig_tx_pol, rx_pol=orig_rx_pol, lane=ln, print_en=0)

        result[ln] = pol(tx_pol=None, rx_pol=None, lane=ln, print_en=0)
          
        #print prbs_cnt_before, prbs_cnt_after
        if print_en:
            if port.upper() != 'TX':
                if ln != lanes[-1]: print('%d,'%(result[ln][1])),
                else:               print('%d' %(result[ln][1])),
            else:
                if ln != lanes[-1]: print('%d,'%(result[ln][0])),
                else:               print('%d' %(result[ln][0])),
                
    #### Update the global polarity array for this Slice, os it can be used next time init() is called
    for ln in lanes:
        if port.upper() != 'TX':            
            RxPolarityMap[gSlice][ln]=result[ln][1] # gSlice lanes, RX Polarity
        else:
            TxPolarityMap[gSlice][ln]=result[ln][0] # gSlice lanes, TX Polarity
        
    if print_en:
        print("] # Slice %d lanes, %s Polarity"%(gSlice,port.upper()) )
    else:
        return result
    
####################################################################################################
# flips RX or TX polarities until fec_status is clean
# 
# call this only when set up in Gearbox mode
#
####################################################################################################
def auto_pol_fec(port='rx',lanes=range(0,16),print_en=1):
    #get_lane_list(lane)
    #get_lane_mode(lanes)
    
    result={}
    
    if print_en:
        print("\n                              #   Lane No: [A0.A1.A2.A3.A4.A5.A6.A7,B0.B1.B2.B3.B4.B5.B6.B7 ]"),
        if port.upper() != 'TX':
            print("\nRxPolarityMap.append([]); RxPolarityMap[%d]=["%(gSlice) ),
        else:
            print("\nTxPolarityMap.append([]); TxPolarityMap[%d]=["%(gSlice) ),
    
  
    for ln in lanes:
        get_lane_mode(ln)
        #c = Pam4Reg if lane_mode_list[ln].lower() == 'pam4' else NrzReg
        if port.upper() != 'TX':
            ##read FEC Status and FIFO Counters for this gearbox
            overall_fec_stat, fec_statistics = fec_status(print_en=0)
            
            # if FEC Status is clean, done
            if not (-1 in overall_fec_stat):
                break
            
            fw_reg_wr(9,0x0000) # disable gearbox FW top control
            fw_reg_wr(8,0x0000) # disable FW PHY adaptation control
            #### 1 #### start with set all polarities to default pol(1,0)
            #### if all B-side adapt-done=1, flip RX pol until all FEC AMlock bits are '1'
            #### if all A-side adapt-done=1, flip RX pol until all FEC AMlock bits are '1'
            #### if any of A-side or B-side adapt-done=0, go to next and set FW-reg8=0xFFFF

            fw_reg_wr(9,0x0000) # disable gearbox FW top control
            fw_reg_wr(8,0xffff) # enable FW PHY adaptation control        
            #### 2 #### flip RX polarities until all lanes' RX adapt-done. If A-side adapt-done=0, wait long enough for it
            
            fw_reg_wr(9,0xffff) # enable back gearbox FW top control
            fw_reg_wr(8,0x0000) # disable FW PHY adaptation control
            #### 3 #### flip RX polarities until all remaining FEC AMlock bits are '1'
            
            fw_reg_wr(9,0xffff) # enable back gearbox FW top control
            fw_reg_wr(8,0xffff) # enable FW PHY adaptation control   
            #### 4 #### wait until all remaining FEC FIFO pointers fall into place
        #else:
        #### 5 #### flip TX polarities if partner FEC is not locked        
        
        #orig_tx_pol,orig_rx_pol =  pol(tx_pol=None, rx_pol=None, lane=ln, print_en=0)

        if port.upper() != 'TX': # if asked to reverse RX Polarity (default selection)
            #pol(tx_pol= orig_tx_pol, rx_pol=~orig_rx_pol, lane=ln, print_en=0)
            flip_pol(lane=ln, port='rx', print_en=1)
        else: # if asked to reverse TX Polarity
            #pol(tx_pol=~orig_tx_pol, rx_pol= orig_rx_pol, lane=ln, print_en=0)
            flip_pol(lane=ln, port='tx', print_en=1)
        
        #prbs_cnt_after = get_prbs(lane = ln, rst=1, print_en=0)[ln]
            
        #if (prbs_cnt_before==0) or (prbs_cnt_before * 10 <= prbs_cnt_after): # if at least one order not improved, keep the orginal polarity
        #    pol(tx_pol= orig_tx_pol, rx_pol=orig_rx_pol, lane=ln, print_en=0)

        result[ln] = pol(tx_pol=None, rx_pol=None, lane=ln, print_en=0)
          
        #print prbs_cnt_before, prbs_cnt_after
        if print_en:
            if port.upper() != 'TX':
                if ln != lanes[-1]: print('%d,'%(result[ln][1])),
                else:               print('%d' %(result[ln][1])),
            else:
                if ln != lanes[-1]: print('%d,'%(result[ln][0])),
                else:               print('%d' %(result[ln][0])),
                
    #### Update the global polarity array for this Slice, os it can be used next time init() is called
    for ln in lanes:
        if port.upper() != 'TX':            
            RxPolarityMap[gSlice][ln]=result[ln][1] # gSlice lanes, RX Polarity
        else:
            TxPolarityMap[gSlice][ln]=result[ln][0] # gSlice lanes, TX Polarity
        
    if print_en:
        print("] # Slice %d lanes, %s Polarity"%(gSlice,port.upper()) )
    else:
        return result
    
####################################################################################################
def rx_prbs_mode(patt = None, lane = None):
    lanes = get_lane_list(lane)
    nrz_prbs_pat  = ['PRBS9', 'PRBS15', 'PRBS23', 'PRBS31'] 
    pam4_prbs_pat = ['PRBS9', 'PRBS13', 'PRBS15', 'PRBS31']
    result = {}
    for lane in lanes:
        c = Pam4Reg if lane_mode_list[lane].lower() == 'pam4' else NrzReg
        if patt is None:
            checker = rx_checker(lane=lane)[lane]
            patt_v = rreg(c.rx_prbs_mode_addr, lane)   
            if lane_mode_list[lane].lower() == 'pam4':
                pat_sel = pam4_prbs_pat[patt_v]
            else:
                pat_sel = nrz_prbs_pat[patt_v]
            result[lane] = checker, pat_sel
        elif type(patt) == int:
            rx_checker(1,lane)
            wreg(c.rx_prbs_mode_addr, patt, lane)
        elif type(patt) == str:
            val = pam4_prbs_pat.index(patt) if gPam4_En else nrz_prbs_pat.index(patt)
            rx_checker(1,lane)
            wreg(c.rx_prbs_mode_addr, val, lane)
    # else:
    if result != {}: return result
def rx_checker(status = None, lane = None):
    lanes = get_lane_list(lane)
    result = {}
    for lane in lanes:
        c = Pam4Reg if lane_mode_list[lane].lower() == 'pam4' else NrzReg
        if status is None:
            result[lane] = rreg(c.rx_prbs_checker_pu_addr, lane)
        else:
            wreg(c.rx_prbs_checker_pu_addr, status, lane)
    # else:
    if result != {}: return result
            
#####################################################################################################
# Command to Disable (Squelch) or Enable (Unsquelch) TX output
#
# 'Disable' means put TX output in electrical idle mode
# 
####################################################################################################
def tx_output(mode=None,lane=None, print_en=1):

    hw_tx_control_addr  = 0xA0
    
    output_en_list  = ['ON','EN','UNSQ','UNSQUELCH']
    output_dis_list = ['OFF','DIS','SQ','SQUELCH']
    mode_list= ['EN','dis']

    lanes = get_lane_list(lane)
    result = {}
    
    if print_en: print("\n------------------------------"),
    if print_en: print("  Slice %d TX Output En or Dis Status Per Lane"%gSlice),
    if print_en: print("  ------------------------------"),
    if print_en: print("\n        Lane:"),
    if print_en: 
        for ln in range(16):
            print(" %4s" %(lane_name_list[ln])),

    if mode!=None: # Command to dis/en certain lanes' TX output
        for ln in lanes:
            if any(i in mode.upper() for i in output_en_list):   
                wreg([hw_tx_control_addr,[15]],1, ln) # stop using test pattern 0x0000
                wreg([hw_tx_control_addr,[14]],1, ln) # stop using test pattern 0x0000
                wreg([hw_tx_control_addr,[13]],1, ln) # stop using test pattern 0x0000
            if any(i in mode.upper() for i in output_dis_list):
                wreg(c.tx_test_patt4_addr, 0x0000, ln)
                wreg(c.tx_test_patt3_addr, 0x0000, ln)
                wreg(c.tx_test_patt2_addr, 0x0000, ln)
                wreg(c.tx_test_patt1_addr, 0x0000, ln)
                wreg([hw_tx_control_addr,[15]],0, ln) # use test pattern = 0x0000 to output electrical idle pattern
                wreg([hw_tx_control_addr,[14]],0, ln) # use test pattern = 0x0000 to output electrical idle pattern
                wreg([hw_tx_control_addr,[13]],1, ln) # use test pattern = 0x0000 to output electrical idle pattern


    for ln in range(16): # Read TX Output On/Off HW Register bit
        status2=rreg(hw_tx_control_addr,ln)
        status1=rreg([hw_tx_control_addr,[13]],ln)
        result[ln]=status1,status2
           
    if print_en: # Print Status
        print("\n TX Output  :"),
        for ln in range(16):
            print(" %4s"  %(mode_list[result[ln][0]])),
        print("\n TX Reg 0xA0:"),
        for ln in range(16):
            print(" %04X" %(result[ln][1])),
    else:
        return result
        
####################################################################################################
def tx_status(mode='func', lane = None): # mode= (1) 'off' or 'idle', (2) 'on' or 'func' or 'functional', (3) 'prbs'
    lanes = get_lane_list(lane)
    c = Pam4Reg
    for lane in lanes:
        #### OFF
        if mode.upper() == 'OFF' or mode.upper() == 'IDLE': # test pattern mode =0000       
            wreg(c.tx_test_patt4_addr, 0x0000, lane)
            wreg(c.tx_test_patt3_addr, 0x0000, lane)
            wreg(c.tx_test_patt2_addr, 0x0000, lane)
            wreg(c.tx_test_patt1_addr, 0x0000, lane)
            wreg(c.tx_test_patt_sc_addr,   0, lane) # 0xA0[15]=0, TX test signal source = Test pattern memory, not PRBS generator
            wreg(c.tx_test_patt_en_addr,   1, lane) # 0xA0[13]=1, TX data = test data, not traffic data
            wreg(c.tx_prbs_clk_en_addr,    0, lane) # 0xA0[14]=0, TX PAM4 PRBS Gen Clock disabled
            wreg(c.tx_prbs_en_addr,        0, lane) # 0xA0[11]=0, TX PAM4 PRBS Gen disabled 
            wreg(c.tx_prbs_clk_en_nrz_addr,0, lane) # 0xb0[14]=0, TX  NRZ PRBS gen clock disabled
            wreg(c.tx_prbs_en_nrz_addr,    0, lane) # 0xb0[11]=0, TX  NRZ PRBS gen disabled
        #### PRBS 
        elif 'PRBS' in mode.upper():      
            wreg(c.tx_test_patt_sc_addr,   1, lane) # 0xA0[15]=1, TX test signal source = Test pattern memory, not PRBS generator
            wreg(c.tx_test_patt_en_addr,   1, lane) # 0xA0[13]=1, TX data = test data, not functional/traffic mode
            wreg(c.tx_prbs_clk_en_addr,    1, lane) # 0xA0[14]=1, TX PAM4 PRBS GEN Clock enabled
            wreg(c.tx_prbs_en_addr,        1, lane) # 0xA0[11]=1, TX PAM4 PRBS Gen enabled 
            wreg(c.tx_prbs_clk_en_nrz_addr,1, lane) # 0xb0[14]=0, TX  NRZ PRBS gen clock disabled
            wreg(c.tx_prbs_en_nrz_addr,    1, lane) # 0xb0[11]=1, TX  NRZ PRBS gen enabled
        #### FUNCTIONAL (normal traffic mode)
        else:                    
            wreg(c.tx_test_patt_sc_addr,   0, lane) # 0xA0[15]=0, TX test signal source = not PRBS generator
            wreg(c.tx_test_patt_en_addr,   0, lane) # 0xA0[13]=0, TX data = functional mode, not test data
            wreg(c.tx_prbs_clk_en_addr,    0, lane) # 0xA0[14]=0, TX PAM4 PRBS Gen CLock disabled
            wreg(c.tx_prbs_en_addr,        0, lane) # 0xA0[11]=0, TX PAM4 PRBS Gen disabled 
            wreg(c.tx_prbs_clk_en_nrz_addr,0, lane) # 0xb0[14]=0, TX  NRZ PRBS gen clock disabled
            wreg(c.tx_prbs_en_nrz_addr,    0, lane) # 0xb0[11]=0, TX  NRZ PRBS gen disabled
                                     
####################################################################################################
def tx_test_patt(en=None, lane = None, tx_test_patt4_val=0x0000,tx_test_patt3_val=0x0000,tx_test_patt2_val=0x0000,tx_test_patt1_val=0x0000):
    lanes = get_lane_list(lane)
    c = Pam4Reg
    result = {}
    for lane in lanes:
        if en is None:
            prbs_en = rreg(c.tx_prbs_en_addr, lane)
            test_patt_en = rreg(c.tx_test_patt_en_addr, lane)
            prbs_clk_en = rreg(c.tx_prbs_clk_en_addr, lane)
            test_patt_sc = rreg(c.tx_test_patt_sc_addr, lane)
            val = prbs_en & test_patt_en & prbs_clk_en & (1-test_patt_sc)
            result[lane] = val
        elif en == 1:
            # test pattern mode        
            wreg(c.tx_test_patt_en_addr, en, lane)
            wreg(c.tx_test_patt_sc_addr, 1-en, lane)
            wreg(c.tx_test_patt4_addr, tx_test_patt4_val, lane)
            wreg(c.tx_test_patt3_addr, tx_test_patt3_val, lane)
            wreg(c.tx_test_patt2_addr, tx_test_patt2_val, lane)
            wreg(c.tx_test_patt1_addr, tx_test_patt1_val, lane)
        else:
            # normal traffic mode
            wreg(c.tx_test_patt_en_addr, 0x0, lane)
    # else:
    if result != {}: return result
        
####################################################################################################
def tx_prbs_en(en = None, lane = None):
    lanes = get_lane_list(lane)
    c = Pam4Reg
    result = {}
    for lane in lanes:
        if en is None:
            prbs_en = rreg(c.tx_prbs_en_addr, lane)
            test_patt_en = rreg(c.tx_test_patt_en_addr, lane)
            prbs_clk_en = rreg(c.tx_prbs_clk_en_addr, lane)
            test_patt_sc = rreg(c.tx_test_patt_sc_addr, lane)
            val = prbs_en & test_patt_en & prbs_clk_en & test_patt_sc
            result[lane] = val
        else:
            wreg(c.tx_prbs_en_addr, en, lane)
            wreg(c.tx_test_patt_en_addr, en, lane)
            wreg(c.tx_prbs_clk_en_addr, en, lane)
            wreg(c.tx_test_patt_sc_addr, en, lane)
    
####################################################################################################
def tx_prbs_mode(patt = None, lane = None):
    lanes = get_lane_list(lane)
    c = Pam4Reg
    result = {}
    nrz_prbs_pat  = ['PRBS9', 'PRBS15', 'PRBS23', 'PRBS31'] 
    pam4_prbs_pat = ['PRBS9', 'PRBS13', 'PRBS15', 'PRBS31']
    for lane in lanes:
        if patt is None:
            prbs_en = rreg(c.tx_prbs_en_addr, lane)
            test_patt_en = rreg(c.tx_test_patt_en_addr, lane)
            prbs_clk_en = rreg(c.tx_prbs_clk_en_addr, lane)
            test_patt_sc = rreg(c.tx_test_patt_sc_addr, lane)
            patt_v = rreg(c.tx_prbs_patt_sel_addr, lane)
            if lane_mode_list[lane].lower() == 'pam4':
                pat_sel = pam4_prbs_pat[patt_v]
            else:
                pat_sel = nrz_prbs_pat[patt_v]
            result[lane] = (prbs_en & test_patt_en & prbs_clk_en & test_patt_sc, pat_sel)
        elif type(patt) == int:
            tx_prbs_en(0, lane)
            wreg(c.tx_prbs_patt_sel_addr, patt, lane)
            tx_prbs_en(1, lane)
        elif type(patt) == str:
            tx_prbs_en(0, lane)
            val = pam4_prbs_pat.index(patt) if lane_mode_list[lane].lower() == 'pam4' else nrz_prbs_pat.index(patt)
            wreg(c.tx_prbs_patt_sel_addr, val, lane)
            tx_prbs_en(1, lane)
    #else:
    if result != {}: return result
        
####################################################################################################
def err_inject(lane = None):
    lanes = get_lane_list(lane)
    c = Pam4Reg
    for lane in lanes:
        wreg(c.tx_prbs_1b_err_addr, 0x0, lane)
        time.sleep(0.001)
        wreg(c.tx_prbs_1b_err_addr, 0x1, lane)
        time.sleep(0.001)
        wreg(c.tx_prbs_1b_err_addr, 0x0, lane)
    
####################################################################################################
def tx_taps(tap1 = None,tap2 = None,tap3 = None, tap4 = None, tap5 = None, tap6 = None, tap7 = None, tap8 = None, tap9 = None, tap10 = None, tap11 = None, lane = None):
    lanes = get_lane_list(lane)
    c = Pam4Reg
    result = {}
    for lane in lanes:
        if tap1 is tap2 is tap3 is tap4 is tap5 is tap6 is tap7 is tap8 is tap9 is tap10 is tap11 is None:
            tap1_v = twos_to_int(rreg(c.tx_tap1_addr, lane), 8)
            tap2_v = twos_to_int(rreg(c.tx_tap2_addr, lane), 8)
            tap3_v = twos_to_int(rreg(c.tx_tap3_addr, lane), 8)
            tap4_v = twos_to_int(rreg(c.tx_tap4_addr, lane), 8)
            tap5_v = twos_to_int(rreg(c.tx_tap5_addr, lane), 8)
            tap6_v = twos_to_int(rreg(c.tx_tap6_addr, lane), 4)
            tap7_v = twos_to_int(rreg(c.tx_tap7_addr, lane), 4)
            tap8_v = twos_to_int(rreg(c.tx_tap8_addr, lane), 4)
            tap9_v = twos_to_int(rreg(c.tx_tap9_addr, lane), 4)
            tap10_v = twos_to_int(rreg(c.tx_tap10_addr, lane), 4)
            tap11_v = twos_to_int(rreg(c.tx_tap11_addr, lane), 4)
            result[lane] = tap1_v, tap2_v, tap3_v, tap4_v, tap5_v, tap6_v, tap7_v, tap8_v, tap9_v, tap10_v, tap11_v
        else:
            if tap1 != None: wreg(c.tx_tap1_addr, int_to_twos(tap1,8), lane)
            if tap2 != None: wreg(c.tx_tap2_addr, int_to_twos(tap2,8), lane)
            if tap3 != None: wreg(c.tx_tap3_addr, int_to_twos(tap3,8), lane)
            if tap4 != None: wreg(c.tx_tap4_addr, int_to_twos(tap4,8), lane)
            if tap5 != None: wreg(c.tx_tap5_addr, int_to_twos(tap5,8), lane)
            if tap6 != None: wreg(c.tx_tap6_addr, int_to_twos(tap6,4), lane)
            if tap7 != None: wreg(c.tx_tap7_addr, int_to_twos(tap7,4), lane)
            if tap8 != None: wreg(c.tx_tap8_addr, int_to_twos(tap8,4), lane)
            if tap9 != None: wreg(c.tx_tap9_addr, int_to_twos(tap9,4), lane)
            if tap10 != None: wreg(c.tx_tap10_addr, int_to_twos(tap10,4), lane)
            if tap11 != None: wreg(c.tx_tap11_addr, int_to_twos(tap11,4), lane)
    #else:
    if result != {}: return result
    
####################################################################################################
def tx_taps_rule_check(lane = None):
    lanes = get_lane_list(lane)
    c = Pam4Reg
    result = {}
    for lane in lanes:
        taps = tx_taps(lane = lane)[lane]
        taps = [abs(taps[i]) for i in range(len(taps))]
        hf = rreg(c.tx_taps_hf_addr, lane)
        tx_taps_sum = sum(taps)/2.0 + sum([taps[i]*((hf>>i)&1) for i in range(5)])/2.0
        result[lane] = (tx_taps_sum <= c.tx_taps_sum_limit), tx_taps_sum
    #else:
    if result != {}: return result
####################################################################################################
def tx(lane = None):
    lanes = get_lane_list(lane)
    for lane in lanes:
        test_patt_en = tx_test_patt(lane = lane)[lane]
        prbs_en, prbs_sel = tx_prbs_mode(lane = lane)[lane]
        taps = tx_taps(lane = lane)[lane]
        sum = tx_taps_rule_check(lane = lane)[lane][-1]
        print ("%s, PRBS enable: %s %s, test pattern enable: %s, taps: %s, %d"%(lane_name_list[lane], bool(prbs_en), prbs_sel, bool(test_patt_en), ','.join((map(str,taps))), sum))

####################################################################################################
#
#   tx_sj( A = 6 ,f = 20, lane = 0 )
#
####################################################################################################
def tx_sj( A = 0.3, f = 2500, lane = None):
    lanes = get_lane_list(lane)
    result = {}
    
    if (A!=None and A>0.0):
        sj_en = 1 
    elif (A!=None):
        sj_en = 0

    
    if A!=None:
        for ln in lanes:        
            if sj_en:
                tx_sj_en(en=1,lane=ln)
                tx_sj_ampl(A,ln)
                tx_sj_freq(f,ln)
            else: # Disable SJ
                tx_sj_en(en=0,lane=ln)
                
    for ln in lanes:       
        sj_en_status  = 'dis' if rregBits(0xf3,[5],ln)==1 else 'en'
        sj_amp =float(rregBits(0xf9,[15,13],ln))+ (float(rregBits(0xf9,[12,5],ln))/256.0)
        sj_freq=int(float(rregBits(0xf5,[15,2],ln)) * 3.2)
        result[ln]=sj_en_status,sj_amp,sj_freq
        
    return result
####################################################################################################
def tx_sj_range( A = 3, lane = 0):
    tx_sj_en(en=1,lane=lane)
    tx_sj_ampl(A,lane)
    wregBits(0xf5,[15,2],0x3fff,lane) # sj_freq = max
    
####################################################################################################
def tx_sj_en( en=1,lane=0):
    if en: 
        wregBits(0xf4,[0],0,lane)  # disable TRF before enabling injecting SJ 
        wregBits(0xf3,[5],0,lane)  # enable tx_rotator to inject SJ 
    else: # Disable SJ 
        wregBits(0xf3,[5],1,lane)  # disable tx_rotator
        tx_sj_ampl(A=0,lane=lane)  # sj_ampl_0123  = 0
        tx_sj_freq(f=0,lane=lane)  # sj_freq    = 0

    return en
####################################################################################################
def tx_sj_ampl( A = 3, lane = 0 ):
    addr = [0xf9,0xf8,0xf7,0xf6] # sj_ampl_0/1/2/3
    #           sj_ampl_cos_0,  sj_ampl_cos_1,         sj_ampl_cos_2,         sj_ampl_cos_3
    phase_value =[math.cos(0), math.cos((math.pi)/8), math.cos((math.pi)/4), math.cos(3*(math.pi)/8)]
    for i in range(4):
        value = A*phase_value[i]
        int_A = int(value)
        float_A = dec2bin(value - int_A)        
        wregBits(addr[i],[15,13], int_A,lane)
        wregBits(addr[i],[12,5],float_A,lane)

####################################################################################################
def tx_sj_freq( f = 250, lane = 0):
    data_f = int(round((f/3.2),0))   
    wregBits(0xf5,[15,2],data_f,lane) # sj_freq value
    
        
def wait_rdy(lane = None, t = 1):
    lanes = get_lane_list(lane)
    i = 0
    while(i<t):
        rdy = ready(lanes)
        tmp = [sum(rdy[lane]) == 2 for lane in lanes]
        em = [eye(lane)[lane][-1] for lane in lanes]
        if (False not in tmp) and (0.0 not in em): 
            break
        time.sleep(0.001)
        i += 0.001
    return rdy
    
def sm_cont(lane = None):
    lanes = get_lane_list(lane)
    for lane in lanes:
        c = Pam4Reg if lane_mode_list[lane].lower() == 'pam4' else NrzReg  
        wreg(c.rx_sm_cont_addr, 0x0, lane)
        time.sleep(0.001)
        wreg(c.rx_sm_cont_addr, 0x1, lane)
        #time.sleep(0.001)
        #wreg(c.rx_sm_cont_addr, 0x0, lane)
    
def bp1(en = None, st = 0, lane = None):
    lanes = get_lane_list(lane)
    result = {}
    for lane in lanes:
        c = Pam4Reg if lane_mode_list[lane].lower() == 'pam4' else NrzReg  
        if en is None:
            en_v = rreg(c.rx_bp1_en_addr, lane)
            st_v = rreg(c.rx_bp1_st_addr, lane)
            reached = rreg(c.rx_bp1_reached_addr, lane)
            result[lane] = en_v, st_v, reached
        else:
            wreg(c.rx_bp1_st_addr, st, lane)
            wreg(c.rx_bp1_en_addr, en, lane)
    #else:
    if result != {}: return result
        
def bp2(en = None, st = 0, lane = None):
    lanes = get_lane_list(lane)
    result = {}
    for lane in lanes:
        c = Pam4Reg if lane_mode_list[lane].lower() == 'pam4' else NrzReg  
        if en is None:
            en_v = rreg(c.rx_bp2_en_addr, lane)
            st_v = rreg(c.rx_bp2_st_addr, lane)
            reached = rreg(c.rx_bp2_reached_addr, lane)
            result[lane] = en_v, st_v, reached
        else:
            wreg(c.rx_bp2_st_addr, st, lane)
            wreg(c.rx_bp2_en_addr, en, lane)
    #else:
    if result != {}: return result   
        
def ctle(val = None, lane = None):
    lanes = get_lane_list(lane)
    result = {}
    for lane in lanes:
        c = Pam4Reg if lane_mode_list[lane].lower() == 'pam4' else NrzReg  
        if val is None:
            v = rreg(c.rx_agc_ow_addr, lane)
            result[lane] = v
        else:
            wreg(c.rx_agc_ow_addr, val, lane)
            wreg(c.rx_agc_owen_addr, 0x1, lane)
    #else:
    if result != {}: return result
        
def ctle12 (ctle = None, ctle1 = None, ctle2 = None, lane = None):
    lanes = get_lane_list(lane)
    result = {}
    for ln in lanes:
        c = Pam4Reg if lane_mode_list[ln].lower() == 'pam4' else NrzReg  
        if ctle is None: # Read current CTLE settings
            ctle_idx = rreg(c.rx_agc_ow_addr, lane)
            ctle_1_bit4=rreg([0x1d7,[3]],ln)
            ctle_2_bit4=rreg([0x1d7,[2]],ln)
            ctle_1 = ctle_map(ctle_idx, lane=ln)[ln][0] + (ctle_1_bit4*8)
            ctle_2 = ctle_map(ctle_idx, lane=ln)[ln][1] + (ctle_2_bit4*8)            
            result[ln] = ctle_idx,ctle_1,ctle_2
        else:# Select requested CTLE index 
            wreg(c.rx_agc_ow_addr, ctle, ln)
            wreg(c.rx_agc_owen_addr, 0x1, ln)
            # overwrite its CTLE1/2 values if given
            if ctle1 != None and ctle2 != None: 
                if ctle1>7:
                    ctle1_w = ctle1 - 8
                    wreg([0x1d7,[3]],1,ln)
                else:
                    ctle1_w = ctle1
                    wreg([0x1d7,[3]],0,ln)
            
                if ctle2>7:
                    ctle2_w = ctle2 - 8
                    wreg([0x1d7,[2]],1,ln)
                else:
                    ctle2_w = ctle2
                    wreg([0x1d7,[2]],0,ln)       
                ctle_map(ctle, ctle1_w, ctle2_w, lane=ln)

    #else:
    if result != {}: return result

def skef(en = None, val = None, lane = None):
    lanes = get_lane_list(lane)
    result = {}
    c = Pam4Reg
    for lane in lanes:
        if en is None:
            en_v = rreg(c.rx_skef_en_addr, lane)
            v = rreg(c.rx_skef_degen_addr, lane)
            result[lane] = en_v, v
        else:
            wreg(c.rx_skef_degen_addr, val, lane)
            wreg(c.rx_skef_en_addr, en, lane)
    #else:
    if result != {}: return result
        
def agcgain(agcgain1 = None, agcgain2 = None, lane = None):
    lanes = get_lane_list(lane)
    result = {}
    c = Pam4Reg
    for lane in lanes:
        if agcgain1 is agcgain2 is None:
            agcgain1_v = Gray_Bin(rreg(c.rx_agcgain1_addr, lane))
            agcgain2_v = Gray_Bin(rreg(c.rx_agcgain2_addr, lane))
            result[lane] = agcgain1_v, agcgain2_v
        else:
            if agcgain1 != None:
                wreg(c.rx_agcgain1_addr, Bin_Gray(agcgain1), lane)
            if agcgain2 != None:
                wreg(c.rx_agcgain2_addr, Bin_Gray(agcgain2), lane)
    #else:
    if result != {}: return result
        
def ffegain(ffegain1 = None, ffegain2 = None, lane = None):
    lanes = get_lane_list(lane)
    result = {}
    c = Pam4Reg
    for lane in lanes:
        if ffegain1 is ffegain2 is None:
            ffegain1_v = Gray_Bin(rreg(c.rx_ffe_sf_msb_addr, lane))
            ffegain2_v = Gray_Bin(rreg(c.rx_ffe_sf_lsb_addr, lane))
            result[lane] = ffegain1_v, ffegain2_v
        else:
            if ffegain1 != None:
                wreg(c.rx_ffe_sf_msb_addr, Bin_Gray(ffegain1), lane)
            if ffegain2 != None:
                wreg(c.rx_ffe_sf_lsb_addr, Bin_Gray(ffegain2), lane)
    #else:
    if result != {}: return result
         
def dc_gain(agcgain1=None, agcgain2=None, ffegain1=None, ffegain2=None, lane=None):    
    if agcgain1 is agcgain2 is ffegain1 is ffegain2 is None:
        agcgain_val = agcgain(lane=lane)
        ffegain_val = ffegain(lane=lane)
        result = {}
        for i in agcgain_val.keys():
            result[i] = agcgain_val[i][0], agcgain_val[i][1], ffegain_val[i][0], ffegain_val[i][1]
        #else:
        if result != {}: return result
    else:
        if agcgain1 != None or agcgain2 != None:
            agcgain(agcgain1, agcgain2, lane)
        if ffegain1 != None or ffegain2 != None:
            ffegain(ffegain1, ffegain2, lane)
 
def ctle_map(sel = None, val1 = None, val2 = None, lane = None):
    lanes = get_lane_list(lane)
    result = {}
    for lane in lanes:
        c = Pam4Reg if lane_mode_list[lane].lower() == 'pam4' else NrzReg  
        if None in (sel, val1, val2):
            map0 = rreg(c.rx_agc_map0_addr, lane)
            map1 = rreg(c.rx_agc_map1_addr, lane)
            map2 = rreg(c.rx_agc_map2_addr, lane)
            agc = { 0: [map0>>13, (map0>>10) & 0x7],
                    1: [(map0>>7) & 0x7, (map0>>4) & 0x7],
                    2: [(map0>>1) & 0x7, ((map0 & 0x1)<<2) + (map1>>14)],
                    3: [(map1>>11) & 0x7, (map1>>8) & 0x7],
                    4: [(map1>>5) & 0x7, (map1>>2) & 0x7],
                    5: [((map1 & 0x3)<<1) + (map2>>15), (map2>>12) & 0x7],
                    6: [(map2>>9) & 0x7, (map2>>6) & 0x7],
                    7: [(map2>>3) & 0x7, map2 & 0x7]
                    }
            if sel is None:
                result[lane] = agc
            else:
                result[lane] = agc[sel]
        else:
            if sel != 2 and sel != 5:
                val = (val1<<3) + val2
                if sel == 0:
                    map0 = (rreg(c.rx_agc_map0_addr, lane) | 0xfc00) & (val<<10 | 0x3ff) 
                    wreg(c.rx_agc_map0_addr, map0, lane)
                elif sel == 1:
                    map0 = (rreg(c.rx_agc_map0_addr, lane) | 0x03f0) & (val<<4 | 0xfc0f) 
                    wreg(c.rx_agc_map0_addr, map0, lane)
                elif sel == 3:
                    map1 = (rreg(c.rx_agc_map1_addr, lane) | 0x3f00) & (val<<8 | 0xc0ff) 
                    wreg(c.rx_agc_map1_addr, map1, lane)
                elif sel == 4:
                    map1 = (rreg(c.rx_agc_map1_addr, lane) | 0x00fc) & (val<<2 | 0xff03) 
                    wreg(c.rx_agc_map1_addr, map1, lane)
                elif sel == 6:
                    map2 = (rreg(c.rx_agc_map2_addr, lane) | 0x0fc0) & (val<<6 | 0xf03f) 
                    wreg(c.rx_agc_map2_addr, map2, lane)
                elif sel == 7:
                    map2 = (rreg(c.rx_agc_map2_addr, lane) | 0x003f) & (val | 0xffc0) 
                    wreg(c.rx_agc_map2_addr, map2, lane)
            elif sel == 2:
                val = (val1<<1) + ((val2 & 0x4)>>2)
                map0 = (rreg(c.rx_agc_map0_addr, lane) | 0x000f) & (val | 0xfff0)
                wreg(c.rx_agc_map0_addr, map0, lane)
                val = val2 & 0x3
                map1 = (rreg(c.rx_agc_map1_addr, lane) | 0xc000) & ((val<<14) | 0x3fff)
                wreg(c.rx_agc_map1_addr, map1, lane)
            else:# sel == 5:
                val = (val1 >>1 & 0x3)
                map1 = (rreg(c.rx_agc_map1_addr, lane) & 0xfffc) | (val & 0x0003)
                wreg(c.rx_agc_map1_addr, map1, lane)
                val = ((val1 & 0x1)<<3) + val2
                #map2 = (rreg(c.rx_agc_map2_addr, lane) | 0xf) & ((val<<12) | 0x0fff)
                map2 = (rreg(c.rx_agc_map2_addr, lane) & 0x0fff) | ((val<<12) & 0xf000)
                wreg(c.rx_agc_map2_addr, map2, lane)
    #else:
    if result != {}:
        if sel is None:
            for key in range(8):
                print (key)
                for lane in lanes:
                    print (result[lane][key])
                print (' ')
        else:
            return result
 
                
def delta(val = None, lane = None):
    lanes = get_lane_list(lane)
    result = {}
    for lane in lanes:
        c = Pam4Reg if lane_mode_list[lane].lower() == 'pam4' else NrzReg  
        if val is None:
            result[lane] = twos_to_int(rreg(c.rx_delta_addr, lane), 7)
        else:
            wreg(c.rx_delta_ow_addr, int_to_twos(val, 7), lane)
            wreg(c.rx_delta_owen_addr, 0x1, lane)
    #else:
    if result != {}: return result
        
def dac(val = None, lane = None):
    lanes = get_lane_list(lane)
    result = {}
    for lane in lanes:
        get_lane_mode(lane)
        c = Pam4Reg if lane_mode_list[lane].lower() == 'pam4' else NrzReg  
        if val != None:
            wreg(c.rx_dac_ow_addr, val, lane)
            wreg(c.rx_dac_owen_addr, 1, lane)
        
        dac_v = rreg(c.rx_dac_addr, lane)
        result[lane] = dac_v
    #else:
    if result != {}: return result        
def kp(val = None, lane = None, print_en=1):
    lanes = get_lane_list(lane)
    result = {}
    for ln in lanes:
        get_lane_mode(ln)
        c = Pam4Reg if lane_mode_list[ln].lower() == 'pam4' else NrzReg  
        if val != None:
            wreg(c.rx_kp_ow_addr, val, ln)
            wreg(c.rx_kp_owen_addr, 1, ln)
        
        ted_v = rreg(c.rx_ted_en_addr, ln)
        kp_v  = rreg(c.rx_kp_ow_addr, ln)
        result[ln] = ted_v, kp_v
        
    if print_en: # Print Status
        print("\nSlice %d, Lane:"%(sel_slice())),
        for ln in range(len(lane_name_list)):
            print(" %2s" %(lane_name_list[ln])),
        get_lane_mode('all')
        print("\n      CDR TED:"),
        for ln in range(len(lane_name_list)):
            c = Pam4Reg if lane_mode_list[ln] == 'pam4' else NrzReg
            ted_v = rreg(c.rx_ted_en_addr, ln)
            if lane_mode_list[ln] == 'pam4':
                print(" %2d"  %(ted_v)),
            else:
                print("  -"),
            
        print("\n      CDR Kp :"),
        for ln in range(len(lane_name_list)):
            c = Pam4Reg if lane_mode_list[ln] == 'pam4' else NrzReg
            kp_v  = rreg(c.rx_kp_ow_addr, ln)
            print(" %2d" %(kp_v)),
    else:
        if result != {}: return result        

def ted(val = None, lane = None, print_en=1):
    lanes = get_lane_list(lane)
    result = {}
    for ln in lanes:
        #get_lane_mode(ln)
        c = Pam4Reg  
        if val != None:
            wreg(c.rx_ted_en_addr, val, ln)
        
        ted_v = rreg(c.rx_ted_en_addr, ln)
        kp_v  = rreg(c.rx_kp_ow_addr, ln)
        result[ln] = ted_v, kp_v
        
    if print_en: # Print Status
        get_lane_mode('all')
        print("\nSlice %d, Lane:"%(sel_slice())),
        for ln in range(len(lane_name_list)):
            print(" %2s" %(lane_name_list[ln])),
        print("\n      CDR TED:"),
        for ln in range(len(lane_name_list)):
            c = Pam4Reg if lane_mode_list[ln] == 'pam4' else NrzReg
            ted_v = rreg(c.rx_ted_en_addr, ln)
            if lane_mode_list[ln] == 'pam4':
                print(" %2d"  %(ted_v)),
            else:
                print("  -"),
        print("\n      CDR Kp :"),
        for ln in range(len(lane_name_list)):
            c = Pam4Reg if lane_mode_list[ln] == 'pam4' else NrzReg
            kp_v  = rreg(c.rx_kp_ow_addr, ln)
            print(" %2d" %(kp_v)),
    else:
        if result != {}: return result        

def trf(val = None, lane = None, print_en=1):
    lanes = get_lane_list(lane)
    result = {}
    for ln in lanes:
        #get_lane_mode(ln)
        #c = Pam4Reg  
        if val != None:
            wreg([0xf4,[0]], val, ln)
        
        trf_v = rreg([0xf4,[0]], ln)
        result[ln] = trf_v
        
    if print_en: # Print Status
        get_lane_mode('all')
        print("\nSlice %d, Lane:"%(sel_slice())),
        for ln in range(len(lane_name_list)):
            print(" %2s" %(lane_name_list[ln])),
        print("\n       TRF En:"),
        for ln in range(len(lane_name_list)):
            trf_v  = rreg([0xf4,[0]], ln)
            print(" %2d" %(trf_v)),
    else:
        if result != {}: return result        

def ready(lane = None):
    lanes = get_lane_list(lane)
    result = {}
    for lane in lanes:
        get_lane_mode(lane)
        c = Pam4Reg if lane_mode_list[lane].lower() == 'pam4' else NrzReg  
        sd = rreg(c.rx_sd_addr, lane)
        rdy = rreg(c.rx_rdy_addr, lane)
        adapt_done=1
        if fw_loaded(print_en=0):
            adapt_done = (rreg(c.fw_opt_done_addr) >> lane) & 1
        result[lane] = sd, rdy, adapt_done
    return result
def sig_det(lane = None):
    lanes = get_lane_list(lane)
    result = {}
    for lane in lanes:
        get_lane_mode(lane)
        c = Pam4Reg if lane_mode_list[lane].lower() == 'pam4' else NrzReg  
        sd_bit = rreg(c.rx_sd_addr, lane)
        #rdy = rreg(c.rx_rdy_addr, lane)
        result[lane] = sd_bit
    return result
def phy_rdy(lane = None):
    lanes = get_lane_list(lane)
    result = {}
    for lane in lanes:
        get_lane_mode(lane)
        c = Pam4Reg if lane_mode_list[lane].lower() == 'pam4' else NrzReg  
        #sd = rreg(c.rx_sd_addr, lane)
        rdy = rreg(c.rx_rdy_addr, lane)
        result[lane] = rdy
    return result
def adapt_done(lane = None):
    lanes = get_lane_list(lane)
    result = {}
    for lane in lanes:
        get_lane_mode(lane)
        c = Pam4Reg if lane_mode_list[lane].lower() == 'pam4' else NrzReg
        adapt_done=1
        if fw_loaded(print_en=0):
            adapt_done = (rreg(c.fw_opt_done_addr) >> lane) & 1
        result[lane] = adapt_done
    return result
def ppm(lane = None):
    lanes = get_lane_list(lane)
    result = {}
    for lane in lanes:
        get_lane_mode(lane)
        c = Pam4Reg if lane_mode_list[lane].lower() == 'pam4' else NrzReg  
        ppm = twos_to_int(rreg(c.rx_freq_accum_addr, lane), 11)
        result[lane] = ppm
    return result
    
def state(lane = None):
    lanes = get_lane_list(lane)
    result = {}
    for lane in lanes:
        get_lane_mode(lane)
        c = Pam4Reg if lane_mode_list[lane].lower() == 'pam4' else NrzReg  
        result[lane] = rreg(c.rx_state_addr, lane)
    return result
    
def edge(edge1 = None, edge2 = None, edge3 = None, edge4 = None, lane = None):
    lanes = get_lane_list(lane)
    result = {}
    c = Pam4Reg
    for lane in lanes:
        if edge1 is edge2 is edge3 is edge4 is None:
            edge1_v = rreg(c.rx_edge1_addr, lane)
            edge2_v = rreg(c.rx_edge2_addr, lane)
            edge3_v = rreg(c.rx_edge3_addr, lane)
            edge4_v = rreg(c.rx_edge4_addr, lane)
            result[lane] = edge1_v, edge2_v, edge3_v, edge4_v
        else:
            if edge1 != None:
                wreg(c.rx_edge1_addr, edge1, lane)
            if edge2 != None:
                wreg(c.rx_edge2_addr, edge2, lane)
            if edge3 != None:
                wreg(c.rx_edge3_addr, edge3, lane)
            if edge4 != None:
                wreg(c.rx_edge4_addr, edge4, lane)
    #else:
    if result != {}: return result
        
def get_err(lane = None):
    
    lanes = get_lane_list(lane)
    result = {}
    for lane in lanes:
        c = NrzReg if lane_mode_list[lane].lower() == 'nrz' else Pam4Reg
        # check if prbs checker is on
        checker = rreg(c.rx_prbs_checker_pu_addr, lane)
        #rdy = sum(ready(lane)[lane])
        if checker == 0:
            print("\n***Lane %s PRBS checker is off***"%lane_name_list[lane])
            result[lane] = -1
        else:
            err = long(rreg(c.rx_err_cntr_msb_addr, lane)<<16) + rreg(c.rx_err_cntr_lsb_addr, lane)
            result[lane] = err
    #else:
    if result != {}: return result
####################################################################################################
# 
# GET ISI Residual Taps
####################################################################################################
def sw_pam4_isi(b0=None, dir=0, isi_tap_range=range(0,9), lane=None, print_en=0):
    if lane is None: lane=gLane[0]
    result={}
    if fw_loaded(print_en=0):
        #print("\n...sw_nrz_isi: Slice %d Lane %2d has FW Loaded. Exiting!"%(gSlice,lane))
        result[lane] = [-1]*len(isi_tap_range)
        return result
    if type(lane) != int:
        print ("\n...sw_pam4_isi: This is a single-lane function")
        result[lane] = [-1]*len(isi_tap_range)
        return result

        
    #print_en2=0
    
    if b0 is None:
        print_en=1
        b0=2
    else:
        print_en=0
        
    org1 = rreg(c.rx_margin_patt_dis_owen_addr, lane)
    org2 = rreg(c.rx_margin_patt_dis_ow_addr  , lane)
    org3 = rreg(c.rx_mu_ow_addr               , lane)
    org4 = rreg(c.rx_iter_s6_addr             , lane)
    org5 = rreg(c.rx_timer_meas_s6_addr       , lane)
    org6 = rreg(c.rx_bp1_en_addr              , lane)
    org7 = rreg(c.rx_bp1_st_addr              , lane)
    org8 = rreg(c.rx_bp2_en_addr              , lane)
    org9 = rreg(c.rx_bp2_st_addr              , lane)
    org10= rreg(c.rx_sm_cont_addr             , lane)
    #bp1_org = bp1(lane=lane)[lane][0:2]
    
    # program registers for , b0, dir, pattern_p and pattern_n
    isi_tap_list = []
    #tap_saturated = []
    ffe_index_list = ['   ','Del','k1','k2','k3','k4','  ','  ','  ','  ','   ','   ','   ','   ','   ','   ','   ','   ','   ','   ','   ']
    #ffe_index_list = ['   ','k-1','k12','k3','k4','  ','  ','  ','  ','  ','   ','   ','   ','   ','   ','   ','   ','   ','   ','   ','   ']
    tap_index_list = ['f-2','f-1','f2' ,'f3','f4','f5','f6','f7','f8','f9','f10','f11','f12','f13','f14','f15','f16','f17','f18','f19','f20']
    wreg(c.rx_fixed_patt_b0_addr, b0, lane)
    wreg(c.rx_fixed_patt_dir_addr, dir, lane)
    wreg(c.rx_timer_meas_s6_addr, 8, lane)
    
    #### Clear BP2, Toggle BP1 and Continue to BP1 at state 0x12
    bp2(0,0x12,lane=lane)
    bp1(0,0x12,lane=lane)
    sm_cont_01(lane=lane)
    bp1(1,0x12,lane=lane)    

    wait_for_bp1_timeout = 0
    while True:
        if bp1(lane=lane)[lane][-1]: 
            #ma = rreg(c.rx_state_cntr_addr, lane) # wait for the whole 32-bit counter to be '0x00000000', not good
            ma = rreg(c.rx_mj_addr,lane) # wait for the lowest 2 bits to become '00'
            if ma == 0:
                break
            else:
                #print("0x%x,"%ma),
                bp1(0,0x12,lane)
                sm_cont_01(lane=lane)
                bp1(1,0x12,lane)
        else:
            wait_for_bp1_timeout += 1
            if wait_for_bp1_timeout > 5000:
                print("\nGet Tap Value FULL (1)***>> Timed out waiting for read_state_counter=0, before starting Getting Taps")
                #if print_en2: print (bp1(lane=lane)[lane][-1])
                bp2(0,0x12,lane=lane)
                bp1(0,0x12,lane=lane)
                break
                
                
    wreg(c.rx_margin_patt_dis_owen_addr, 1, lane)
    wreg(c.rx_margin_patt_dis_ow_addr, 0, lane)
    wreg(c.rx_mu_ow_addr, 0, lane)
    wreg(c.rx_iter_s6_addr, 1, lane)
    bp1(0,0x12,lane=lane)
    sm_cont_01(lane=lane)
    
    for i in isi_tap_range:
        wreg(c.rx_fixed_patt_mode_addr, i, lane)
        time.sleep(0.2)
        bp1(1,0x12,lane=lane)
        wait_for_bp1_timeout = 0
        while True:
            if bp1(lane=lane)[lane][-1]: break
            else:
                wait_for_bp1_timeout += 1
                if wait_for_bp1_timeout > 5000:
                    print("\nGet Tap Value FULL (2) ***>> Timed out waiting to reach BP1 for tap %d"%i)
                    bp2(0,0x12,lane=lane)
                    bp1(0,0x12,lane=lane)
                    break
        # read back fixed pattern margins (plus margin and minus margin)reg_addr1 = 0x36
        plus = rreg(c.rx_fixed_patt_plus_margin_addr, lane)
        minus = rreg(c.rx_fixed_patt_minus_margin_addr, lane)
    
        if (plus>2047): plus = plus - 4096
        if (minus>2047): minus = minus - 4096
        diff_margin = plus - minus 
        #diff_margin_f = ((float(diff_margin & 0x0fff)/2048)+1)%2-1
        #if print_en2: print ("%3d,%2d,%2d,%5d,%5d,%5d,%8.4f "  % (i, b0, dir, plus, minus, diff_margin, diff_margin_f))
    
        if abs(plus) == 2048 or abs(minus) == 2048:
            print("\nGet Tap Value FULL ***>> Margin saturated to +/-2048 for tap %d"%i)
            diff_margin = 2222
            
        isi_tap_list.append(diff_margin)
        bp1(0,0x12,lane=lane)
        sm_cont_01(lane=lane)
    
    bp1(1,0x12,lane=lane)
    wait_for_bp1_timeout = 0
    while True:
        if bp1(lane=lane)[lane][-1]: break
        else:
            wait_for_bp1_timeout += 1
            if wait_for_bp1_timeout > 5000:
                print("\nGet Tap Value FULL (3) ***>> Timed out waiting to reach BP1")
                break
    bp1(0,0x12,lane=lane)#debug
    sm_cont_01(lane=lane)#debug           
    wreg(c.rx_margin_patt_dis_owen_addr, org1 ,lane)
    wreg(c.rx_margin_patt_dis_ow_addr  , org2 ,lane)
    wreg(c.rx_mu_ow_addr               , org3 ,lane)
    wreg(c.rx_iter_s6_addr             , org4 ,lane)
    wreg(c.rx_timer_meas_s6_addr       , org5 ,lane)
    wreg(c.rx_bp1_en_addr              , org6 ,lane)
    wreg(c.rx_bp1_st_addr              , org7 ,lane)
    wreg(c.rx_bp2_en_addr              , org8 ,lane)
    wreg(c.rx_bp2_st_addr              , org9 ,lane)
    wreg(c.rx_sm_cont_addr             , org10,lane)
    sm_cont_01(lane=lane)
    
    #print 
    #rregBits(0x28,[13,9],1)
    if print_en:
        print("\n...SW Residual ISI Taps: Lane %s\n\n|"%lane_name_list[lane]),
        
        for i in isi_tap_range:
            print("%4s |"%ffe_index_list[i]),
        print("\n|"),
        [ffe_k1_bin,ffe_k2_bin,ffe_k3_bin,ffe_k4_bin,ffe_s1_bin,ffe_s2_bin,ffe_sf_bin] = ffe_taps(lane=lane)[lane]
        delta_val = delta(lane=lane)[lane]
        print('     | %3d  | %4d | %4d | %4d | %4d |      |      |      |\n|' %(delta_val,ffe_k1_bin,ffe_k2_bin,ffe_k3_bin,ffe_k4_bin)),

        for i in isi_tap_range:
            print("%4s |"%tap_index_list[i]),
        print("\n|"),
        for i in range(len(isi_tap_list)):
            print("%4s |"%isi_tap_list[i]),
        print("\n")   
    else:
        return isi_tap_list

Gettapvalue_full = sw_pam4_isi
####################################################################################################
# 
# GET Residual ISI by the FW
####################################################################################################
def fw_pam4_isi(lane=None, print_en=0):
    isi_tap_range=range(0,16)
    lanes = get_lane_list(lane)
    result = {}
    
    for ln in lanes:
        get_lane_mode(ln)
        # Lane is in PAM4 mode, get the ISI taps
        if lane_mode_list[ln] == 'pam4' and phy_rdy(ln)[ln] == 1: 
            readout = BE_info_signed(ln, 10, 0, len(isi_tap_range))
        # Lane is in NRZ mode, or phy not ready, Skip
        else: 
            readout = [-1*ln]*len(isi_tap_range)
        result[ln] = readout
        
    return result
####################################################################################################
# ISI Residual Taps for NRZ
#
# For Chip Rev 2.0 only
# Does not require a FW pause because it is using BP 5, not used by FW
####################################################################################################
def sw_nrz_isi(lane=None,print_en=0):
    isi_tap_range=range(0,16)
    lanes = get_lane_list(lane)
    result={}

    for ln in lanes:
        isi_tap_list = []; 
        for unused in isi_tap_range: isi_tap_list.append(-1)
        if lane_mode_list[ln].upper() != 'NRZ' or phy_rdy(ln)[ln]==0:
            result[ln] = isi_tap_list
            continue
        bp1_state_orig = rregBits(0x17d, [12, 8],ln)
        bp1_en_orig = rregBits(0x17d, [15],ln)
        wregBits(0x17d, [12, 8], 24,ln)
        wregBits(0x17d, [15], 0, ln)
        wregBits(0x10c, [15], 0, ln)
        wregBits(0x10c, [15], 1, ln)

        for i in isi_tap_range:
            wregBits(0x165, [4,1],i,ln)
            time.sleep(0.01)
            wregBits(0x17d, [15], 1, ln)
            wait_for_bp1_timeout=0
            while True:
                if rregBits(0x18f, [15],ln):
                    break
                else:
                    wait_for_bp1_timeout+=1
                    if wait_for_bp1_timeout>500:
                        if print_en>2:print (" Get Tap Value >>>>> Timed out 2 waiting for BP1")
                        break
            plus = (rregBits(0x127, [3,0],ln)<<8) + rregBits(0x128, [15,8],ln)
            minus = (rregBits(0x128, [7,0],ln)<<4) + rregBits(0x129, [15,12],ln)

            if (plus>2047): plus = plus - 4096
            if (minus>2047): minus = minus - 4096
            diff_margin = plus - minus 
            diff_margin_f = ((float(diff_margin & 0x0fff)/2048)+1)%2-1

            if print_en>2: print ("\n%8d, %8d, %8d, %11d, %11.4f "  % (i, plus, minus, diff_margin, diff_margin_f))
            isi_tap_list[i] = diff_margin
           
            wregBits(0x17d, [15], 0, ln)
            wregBits(0x10c, [15], 0, ln)
            wregBits(0x10c, [15], 1, ln)

        wregBits(0x17d, [12, 8], bp1_state_orig, ln)
        wregBits(0x17d, [15], bp1_en_orig, ln)
        result[ln] = isi_tap_list 

    return result
####################################################################################################
# 
# GET Residual ISI by the FW if loaded, or by software
####################################################################################################
def pam4_isi(lane=None,print_en=0):
    result = {}
    if fw_loaded(print_en=0):
        result = fw_pam4_isi(lane,print_en)
    else:
        result = sw_pam4_isi(lane,print_en)
    return result
####################################################################################################
# 
# GET Residual ISI by the FW if loaded, or by software
####################################################################################################
def nrz_isi(lane=None,print_en=0):
    result = {}
    if (chip_rev() == 1.0):
        result = sw_nrz_isi_BE1(lane,print_en)
    else:
        result = sw_nrz_isi(lane,print_en)
    return result
####################################################################################################
# 
# GET Residual ISI in NRZ or PAM4
####################################################################################################
def isi(lane=None,print_en=1):
    lanes = get_lane_list(lane)
    result = {}
    nrz_lane=False
    pam4_lane=False
    for ln in lanes:
        get_lane_mode(ln)
        if lane_mode_list[ln] == 'pam4':
            result[ln] = pam4_isi(ln,print_en)
            pam4_lane=True
        else: # NRZ
            result[ln] = nrz_isi(ln,print_en)
            nrz_lane=True
    
    if print_en:
        isi_tap_range=range(16)
        line_separator= "\n+--------------------------------------------------------------------------------------------------------------+"
        ffe_index_list = ['  ','Del','k1','k2','k3','k4','  ','  ','  ','  ','   ','   ','   ','   ','   ','   ','   ']
        tap_index_list = ['f-2','f-1','f2','f3','f4','f5','f6','f7','f8','f9','f10','f11','f12','f13','f14','f15','f16']
        tap_index_list_nrz = ['f-1','f4','f5','f6','f7','f8','f9','f10','f11','f12','f13','f14','f15','f16','f16','f17','f17']
        print (line_separator)
        if print_en==2 and lane_mode_list[ln] == 'pam4':
            print("\n|   "),            
            for i in isi_tap_range:
                print("|%4s"%ffe_index_list[i])
            print("|")
        if pam4_lane==True:
            print("\n|  PAM4 ISI ->")
            for i in isi_tap_range:
                print("|%4s"%tap_index_list[i])
            print("|")
        if nrz_lane==True:
            print("\n|   NRZ ISI ->")
            for i in isi_tap_range:
                print("|%4s"%tap_index_list_nrz[i])
            print("|")
        print (line_separator)

        for ln in lanes:
            if print_en==2 and lane_mode_list[ln] == 'pam4':
                # if lane_mode_list[ln] == 'pam4':
                [ffe_k1_bin,ffe_k2_bin,ffe_k3_bin,ffe_k4_bin,ffe_s1_bin,ffe_s2_bin,ffe_sf_bin] = ffe_taps(lane=ln)[ln]
                # else:
                    # [ffe_k1_bin,ffe_k2_bin,ffe_k3_bin,ffe_k4_bin,ffe_s1_bin,ffe_s2_bin,ffe_sf_bin] = [0,0,0,0,0,0,0] 
                delta_val = delta(lane=ln)[ln]            
                print("\n             %3d  %4d  %4d  %4d  %4d                                                             |" %(delta_val,ffe_k1_bin,ffe_k2_bin,ffe_k3_bin,ffe_k4_bin)),
            print("\n| S%d_%2s -"%(gSlice,lane_name_list[ln])),            
            if lane_mode_list[ln] == 'pam4':
                print("PAM4"),
            else:
                print("NRZ "), 
            for i in isi_tap_range:
                print(" %4d"%(result[ln][ln][i])),
            print("|"),
        print (line_separator)
    else:
        return result
   
####################################################################################################
## NRZ specific functions
####################################################################################################
def nrz_dfe(lane = None):
    lanes = get_lane_list(lane)

    c = NrzReg
    result = {}
    for lane in lanes:
        f1 = twos_to_int(rreg(c.rx_f1_addr, lane),7)
        f2 = twos_to_int(rreg(c.rx_f2_addr, lane),7)
        f3 = twos_to_int(rreg(c.rx_f3_addr, lane),7)
        result[lane] = f1, f2, f3
    return result
    
def bb_nrz(en = None, lane = None):
    lanes = get_lane_list(lane)
    c = NrzReg
    result = {}
    for lane in lanes:
        if en != None:
            wreg(c.rx_bb_en_addr, en, lane) # bb_en
            wreg(c.rx_delta_adapt_en_addr, 1-en, lane) # delta_adapt_en
        else:
            bb = rreg(c.rx_bb_en_addr, lane)
            delta_en = rreg(c.rx_delta_adapt_en_addr, lane)
        result[lane] = bb, delta_en
    #else:
    if result != {}: return result

def ffe_pu(en = None, lane = None):
    lanes = get_lane_list(lane)
    c = Pam4Reg
    result = {}
    for lane in lanes:
        if en is None:
            pu1 = rreg(c.rx_ffe_k1_pu_addr, lane)
            pu2 = rreg(c.rx_ffe_k2_pu_addr, lane)
            pu3 = rreg(c.rx_ffe_k3_pu_addr, lane)
            pu4 = rreg(c.rx_ffe_k4_pu_addr, lane)
            result[lane] = pu1 & pu2 & pu3 & pu4
        else:
            wreg(c.rx_ffe_k1_pu_addr, en, lane)
            wreg(c.rx_ffe_k2_pu_addr, en, lane)
            wreg(c.rx_ffe_k3_pu_addr, en, lane)
            wreg(c.rx_ffe_k4_pu_addr, en, lane)
    #else:
    if result != {}: return result
    
def ffe_taps (k1=None, k2=None, k3=None, k4=None, s1=None, s2=None, sf=None, lane=None):

    lanes = get_lane_list(lane)
    c = Pam4Reg
    result = {}
    for lane in lanes:
        if k1!=None:
            pol1 = 1 if k1<0 else 0
            k11 = abs(k1)
            rx_ffe_k1_gray_msb = Bin_Gray(k11 >> 4)
            rx_ffe_k1_gray_lsb = Bin_Gray(k11&0xf)
            #rx_ffe_k1_gray = ((rx_ffe_k1_gray_msb << 4) + rx_ffe_k1_gray_lsb)&0xFF
            wreg(c.rx_ffe_k1_msb_addr, rx_ffe_k1_gray_msb, lane)
            wreg(c.rx_ffe_k1_lsb_addr, rx_ffe_k1_gray_lsb, lane)
            wreg(c.rx_ffe_pol1_addr, pol1, lane)
        if k2!=None:
            pol2 = 1 if k2<0 else 0
            k22 = abs(k2)
            rx_ffe_k2_gray_msb = Bin_Gray(k22 >> 4)
            rx_ffe_k2_gray_lsb = Bin_Gray(k22&0xf)
            rx_ffe_k2_gray = ((rx_ffe_k2_gray_msb << 4) + rx_ffe_k2_gray_lsb)&0xFF
            wreg(c.rx_ffe_k2_msb_addr, rx_ffe_k2_gray_msb, lane)
            wreg(c.rx_ffe_k2_lsb_addr, rx_ffe_k2_gray_lsb, lane)
            wreg(c.rx_ffe_pol2_addr, pol2, lane)
        if k3!=None:
            pol3 = 1 if k3<0 else 0
            k33 = abs(k3)
            rx_ffe_k3_gray_lsb = Bin_Gray(k33&0xf)
            rx_ffe_k3_gray_msb = Bin_Gray(k33 >> 4)
            rx_ffe_k3_gray = ((rx_ffe_k3_gray_msb << 4) + rx_ffe_k3_gray_lsb)&0xFF
            wreg(c.rx_ffe_k3_msb_addr, rx_ffe_k3_gray_msb, lane)
            wreg(c.rx_ffe_k3_lsb_addr, rx_ffe_k3_gray_lsb, lane)
            wreg(c.rx_ffe_pol3_addr, pol3, lane)
        if k4!=None:
            pol4 = 1 if k4<0 else 0
            k44 = abs(k4)
            rx_ffe_k4_gray_lsb = Bin_Gray(k44&0xf)
            rx_ffe_k4_gray_msb = Bin_Gray(k44 >> 4)
            rx_ffe_k4_gray = ((rx_ffe_k4_gray_msb << 4) + rx_ffe_k4_gray_lsb)&0xFF
            wreg(c.rx_ffe_k4_msb_addr, rx_ffe_k4_gray_msb, lane)
            wreg(c.rx_ffe_k4_lsb_addr, rx_ffe_k4_gray_lsb, lane)
            wreg(c.rx_ffe_pol4_addr, pol4, lane)
        if s2!=None:
            rx_ffe_s2_gray_lsb = Bin_Gray(s2&0x0f)
            rx_ffe_s2_gray_msb = Bin_Gray((s2 >> 4)&0x0f)
            rx_ffe_s2_gray = ((rx_ffe_s2_gray_msb << 4) + rx_ffe_s2_gray_lsb)&0xFF
            wreg(c.rx_ffe_s2_msb_addr, rx_ffe_s2_gray_msb, lane)
            wreg(c.rx_ffe_s2_lsb_addr, rx_ffe_s2_gray_lsb, lane)
        if s1!=None:
            rx_ffe_s1_gray_lsb = Bin_Gray(s1&0x0f)
            rx_ffe_s1_gray_msb = Bin_Gray((s1 >> 4)&0x0f)
            rx_ffe_s1_gray = ((rx_ffe_s1_gray_msb << 4) + rx_ffe_s1_gray_lsb)&0xFF
            wreg(c.rx_ffe_s1_msb_addr, rx_ffe_s1_gray_msb, lane)
            wreg(c.rx_ffe_s1_lsb_addr, rx_ffe_s1_gray_lsb, lane)
            
        if sf!=None:
            rx_ffe_sf_gray_lsb = Bin_Gray(sf&0x0f)
            rx_ffe_sf_gray_msb = Bin_Gray((sf >> 4)&0x0f)
            rx_ffe_sf_gray = ((rx_ffe_sf_gray_msb << 4) + rx_ffe_sf_gray_lsb)&0xFF
            wreg(c.rx_ffe_sf_msb_addr, rx_ffe_sf_gray_msb, lane)
            wreg(c.rx_ffe_sf_lsb_addr, rx_ffe_sf_gray_lsb, lane)
            
        if k1 is None and k2 is None and k3 is None and k4 is None and s1 is None and s2 is None and sf is None:
            pol1 = rreg(c.rx_ffe_pol1_addr, lane)
            rx_ffe_k1_msb = Gray_Bin(rreg(c.rx_ffe_k1_msb_addr, lane))
            rx_ffe_k1_lsb = Gray_Bin(rreg(c.rx_ffe_k1_lsb_addr, lane))
            rx_ffe_k1_bin = (1-2*pol1)*(((rx_ffe_k1_msb << 4) + rx_ffe_k1_lsb)&0xFF)
        
            pol2 = rreg(c.rx_ffe_pol2_addr, lane)
            rx_ffe_k2_msb = Gray_Bin(rreg(c.rx_ffe_k2_msb_addr,  lane))
            rx_ffe_k2_lsb = Gray_Bin(rreg(c.rx_ffe_k2_lsb_addr,  lane))
            rx_ffe_k2_bin = (1-2*pol2)*(((rx_ffe_k2_msb << 4) + rx_ffe_k2_lsb)&0xFF)

            pol3 = rreg(c.rx_ffe_pol3_addr, lane)
            rx_ffe_k3_msb = Gray_Bin(rreg(c.rx_ffe_k3_msb_addr,  lane))
            rx_ffe_k3_lsb = Gray_Bin(rreg(c.rx_ffe_k3_lsb_addr,  lane))
            rx_ffe_k3_bin = (1-2*pol3)*(((rx_ffe_k3_msb << 4) + rx_ffe_k3_lsb)&0xFF)
            
            pol4 = rreg(c.rx_ffe_pol4_addr, lane)
            rx_ffe_k4_msb = Gray_Bin(rreg(c.rx_ffe_k4_msb_addr, lane))
            rx_ffe_k4_lsb = Gray_Bin(rreg(c.rx_ffe_k4_lsb_addr, lane))
            rx_ffe_k4_bin = (1-2*pol4)*(((rx_ffe_k4_msb << 4) + rx_ffe_k4_lsb)&0xFF)
            
            rx_ffe_s1_msb = Gray_Bin(rreg(c.rx_ffe_s1_msb_addr,  lane))
            rx_ffe_s1_lsb = Gray_Bin(rreg(c.rx_ffe_s1_lsb_addr,  lane))
            rx_ffe_s1_bin = ((rx_ffe_s1_msb << 4) + rx_ffe_s1_lsb)&0xFF
            
            rx_ffe_s2_msb = Gray_Bin(rreg(c.rx_ffe_s2_msb_addr,  lane))
            rx_ffe_s2_lsb = Gray_Bin(rreg(c.rx_ffe_s2_lsb_addr,  lane))
            rx_ffe_s2_bin = ((rx_ffe_s2_msb << 4) + rx_ffe_s2_lsb)&0xFF
            
            rx_ffe_sf_msb = Gray_Bin(rreg(c.rx_ffe_sf_msb_addr,  lane))
            rx_ffe_sf_lsb = Gray_Bin(rreg(c.rx_ffe_sf_lsb_addr,  lane))
            rx_ffe_sf_bin = ((rx_ffe_sf_msb << 4) + rx_ffe_sf_lsb)&0xFF
            
            result[lane] = rx_ffe_k1_bin, rx_ffe_k2_bin, rx_ffe_k3_bin, rx_ffe_k4_bin, rx_ffe_s1_bin, rx_ffe_s2_bin, rx_ffe_sf_bin
        
    #else:
    if result != {}: return result
        

####################################################################################################  
# FEC Parameters Readback functions
####################################################################################################
#           [FEC_A0,FEC_A1,FEC_A2,FEC_A2, FEC_B0,FEC_B1,FEC_B2,FEC_B3]
fec_class = [0x4000,0x4100,0x4200,0x4300, 0x5000,0x5100,0x5200,0x5300]
fec_class2= [0x4400,0x4500,0x4600,0x4700, 0x5400,0x5500,0x5600,0x5700]
fec_class3= [0x4800,0x4900,0x4a00,0x4b00, 0x5800,0x5900,0x5a00,0x5b00]

####################################################################################################

def fec_status_en(index):
    counter = rregBits(0x9857, [index])
    return counter
    
def fec_status_traffic_gen_en(index): #wreg(0x48f0,0x1095)
    status = rregBits(fec_class3[index]+0xF0,[7])
    return status
        
def fec_status_aligned_rx(index):
    status = rregBits(fec_class[index]+0xc9,[14])
    return status

def fec_status_ampsLock(index):
    status = rregBits(fec_class[index]+0xc9,[11,8])
    return status

def fec_status_uncorrected(index):
    counter_lower = rregBits(fec_class[index]+0xcc,[15,0])
    counter_upper = rregBits(fec_class[index]+0xcd,[15,0])
    counter = (counter_upper<<16)+counter_lower
    return counter

def fec_status_corrected(index):
    counter_lower = rregBits(fec_class[index]+0xca,[15,0])
    counter_upper = rregBits(fec_class[index]+0xcb,[15,0])
    counter = (counter_upper<<16)+counter_lower
    return counter

def fec_status_rx_fifo_cnt(index):
    counter = rregBits(fec_class3[index]+0x9f,[10,0])
    return counter
def fec_status_rx_fifo_min(index):
    counter = rregBits(fec_class3[index]+0xa0,[10,0])
    return counter
def fec_status_rx_fifo_max(index):
    counter = rregBits(fec_class3[index]+0xa1,[10,0])
    return counter
def fec_status_tx_fifo_cnt(index):
    counter = rregBits(fec_class3[index]+0x9c,[10,0])
    return counter
def fec_status_tx_fifo_min(index):
    counter = rregBits(fec_class3[index]+0x9d,[10,0])
    return counter
def fec_status_tx_fifo_max(index):
    counter = rregBits(fec_class3[index]+0x9e,[10,0])
    return counter

####################################################################################################  
# Checks the status of all 8 FECs
#
# Returns: A list of eight FEC Status flags, one for each FEC_IDX 0 to 7
#
#         FEC Status = [FECA0,FECA1,FECA2,FECA3, FECB0,FECB1,FECB2,FECB3]
#
#         FEC Status[idx] =  0: If FEC is not powered up (unused or never configured)
#         FEC Status[idx] =  1: If FEC was able to LOCK (pass)
#         FEC Status[idx] = -1: If FEC was not able to lock (fail)
#
####################################################################################################
def fec_status(print_en=True, fifo_check_en=False):
    length_1 = 22
    length = 17
    
    if fifo_check_en: # [Acceptable range for each FIFO counter]
        rx_fec_fifo_min_limit = [354,  965]
        rx_fec_fifo_max_limit = [354,  965]
        rx_fec_fifo_del_limit = [  0,  514]
        
        tx_fec_fifo_min_limit = [320,  931]
        tx_fec_fifo_max_limit = [320,  931]
        tx_fec_fifo_del_limit = [  0,  514]
    else:
        rx_fec_fifo_min_limit = [  1, 1279]
        rx_fec_fifo_max_limit = [  1, 1279]
        rx_fec_fifo_del_limit = [  1, 1279]
        
        tx_fec_fifo_min_limit = [  1, 1279]
        tx_fec_fifo_max_limit = [  1, 1279]
        tx_fec_fifo_del_limit = [  1, 1279]
        
        
    data={}
    for index in range(8):
        data[index]=[]
        data[index].append(fec_status_aligned_rx(index))
        data[index].append(fec_status_ampsLock(index))
        data[index].append(fec_status_uncorrected(index))
        data[index].append(fec_status_corrected(index))
        data[index].append(fec_status_rx_fifo_min(index))
        data[index].append(fec_status_rx_fifo_cnt(index))
        data[index].append(fec_status_rx_fifo_max(index))
        data[index].append(fec_status_tx_fifo_min(index))
        data[index].append(fec_status_tx_fifo_cnt(index))
        data[index].append(fec_status_tx_fifo_max(index))
        data[index].append(fec_status_en(index))
    
    global gFecStatusPrevTimeStamp
    global gFecStatusCurrTimeStamp
    gFecStatusPrevTimeStamp[gSlice] = gFecStatusCurrTimeStamp[gSlice]
    gFecStatusCurrTimeStamp[gSlice] = time.time()
    
    rx_adapt_done=[]

    aligned=[]
    ampslock=[]
    uncorrected=[]
    corrected=[]
    
    rx_fifo_min=[]
    rx_fifo_cnt=[]
    rx_fifo_max=[]
    rx_fifo_del=[]
    
    tx_fifo_min=[]
    tx_fifo_cnt=[]
    tx_fifo_max=[]
    tx_fifo_del=[]
    
    fec_en=[]
    fec_status=[]
    fec_statistics=[]; idx=0
    
    # Check all lanes' RX Adaptation Status
    rx_adapt_done_lane = [0]*(16+4)
    for ln in range(16):
        rx_adapt_done_lane[ln] = (rreg(c.fw_opt_done_addr) >> ln) & 1
           
    for index in range(8): # 4 A-FECs and 4 B-FECs
        # This FEC lanes' RX Adapt status
        if index < 4: # Adapt status of the A side lanes, related to this FEC
            rx_adapt_done.append([])
            if(rreg([0x9858,  [5,4]])== 2): # Check FEC_A2 Lane Mapping. If [5:4]=2, it means FEC_A2 is connected to lanes A2/A3, else A4/A5
                rx_adapt_done[index].append(rx_adapt_done_lane[index])     # adapt flag for lane  0 or 2
                rx_adapt_done[index].append(rx_adapt_done_lane[index+1])   # adapt flag for lane  1 or 3
            else:
                rx_adapt_done[index].append(rx_adapt_done_lane[index*2])   # adapt flag for lane  0 or 4
                rx_adapt_done[index].append(rx_adapt_done_lane[index*2+1]) # adapt flag for lane  1 or 5
            
        else:         # Adapt status of the B side lanes, related to this FEC
            rx_adapt_done.append([])
            rx_adapt_done[index].append(rx_adapt_done_lane[index*2])   # adapt flag for lane  8 or 12
            rx_adapt_done[index].append(rx_adapt_done_lane[index*2+1]) # adapt flag for lane  9 or 13
            rx_adapt_done[index].append(rx_adapt_done_lane[index*2+2]) # adapt flag for lane 10 or 14
            rx_adapt_done[index].append(rx_adapt_done_lane[index*2+3]) # adapt flag for lane 11 or 15        
        # This FEC status
        aligned.append(data[index][0])
        ampslock.append(data[index][1])
        uncorrected.append(data[index][2])
        corrected.append(data[index][3])
        rx_fifo_min.append(data[index][4])
        rx_fifo_cnt.append(data[index][5])
        rx_fifo_max.append(data[index][6])
        rx_fifo_del.append(rx_fifo_max[index] - rx_fifo_min[index])
        tx_fifo_min.append(data[index][7])
        tx_fifo_cnt.append(data[index][8])
        tx_fifo_max.append(data[index][9])
        tx_fifo_del.append(tx_fifo_max[index] - tx_fifo_min[index])
        fec_en.append(data[index][10])
        fec_status.append(1)
        
        # Return values for this FEC
        if fec_en[index]:
            fec_statistics.append([])
            fec_statistics[idx].append(rx_adapt_done[index])
            fec_statistics[idx].append(format(ampslock[index],'04b'))
            fec_statistics[idx].append(aligned[index])
            fec_statistics[idx].append(corrected[index])
            fec_statistics[idx].append(uncorrected[index])
            fec_statistics[idx].append(rx_fifo_min[index])
            fec_statistics[idx].append(rx_fifo_max[index])
            fec_statistics[idx].append(rx_fifo_del[index])
            fec_statistics[idx].append(tx_fifo_min[index])
            fec_statistics[idx].append(tx_fifo_max[index])
            fec_statistics[idx].append(tx_fifo_del[index])
            idx+=1
        
    num_fec_en = fec_en.count(1)
    #for testing
    #num_fec_en=8# = fec_en.count(1)
    #fec_en=[1,1,1,1,1,1,1,1]
    
    if print_en:
        separator='\n |'
        for i in range(length_1+(length)*num_fec_en):
            separator+='-'
        separator += '|'   

        print ('\n\n | Gearbox FEC STATUS:  Slice %d  - Elapsed Time: %2.3f sec'%(gSlice, gFecStatusCurrTimeStamp[gSlice] - gFecStatusPrevTimeStamp[gSlice])),
        print (separator)

        print ('\n | FEC Parameters       |')
        for index in range(8):
            if fec_en[index]:
                fec_type = 'PAM4 FEC A'   if index<4 else ' NRZ FEC B'
                macro    = index if index<4 else index-4
                print(' %s[%d] |'%(fec_type,macro)),

        print (separator)

        print ('\n | RX Lanes Adapt Done  |')
        for index in range(8):
            if fec_en[index]:
                flag='<<' if (0 in rx_adapt_done[index]) else '  '
                if flag=='<<': fec_status[index]=-1
                if index < 4: print("        %d,%d %s |"%(rx_adapt_done[index][0],rx_adapt_done[index][1],flag)),
                if index > 3: print("    %d,%d,%d,%d %s |"%(rx_adapt_done[index][0],rx_adapt_done[index][1],rx_adapt_done[index][2],rx_adapt_done[index][3],flag)),
            
        print ('\n | RX FEC AmLck 0,1,2,3 |')
        for index in range(8):
            if fec_en[index]:
                flag='<<' if (ampslock[index] != 0xF) else '  '
                if flag=='<<': fec_status[index]=-1
                print("    %d,%d,%d,%d %s |"%(ampslock[index]>>0&1,ampslock[index]>>1&1,ampslock[index]>>2&1,ampslock[index]>>3&1,flag)),

        print ('\n | RX FEC Aligned       |')
        for index in range(8):
            if fec_en[index]:
                flag='<<' if (aligned[index]) != 1 else '  '
                if flag=='<<': fec_status[index]=-1
                print(" %10d %s |"%(aligned[index],flag)),
            else:
                fec_status[index]=0
            
        print ('\n | RX FEC Corr   (cw)   |')
        for index in range(8):
            if fec_en[index]:
                if index < 4: flag='  ' if corrected[index] == 0 else '  '
                if index >=4: flag='<<' if corrected[index] != 0 else '  '
                if flag=='<<': fec_status[index]=-1
                print(" %10d %s |"%(corrected[index],flag)),

        print ('\n | RX FEC Uncorr (cw)   |')
        for index in range(8):
            if fec_en[index]:
                flag='<<' if uncorrected[index] != 0 else '  '
                if flag=='<<': fec_status[index]=-1
                print(" %10d %s |"%(uncorrected[index],flag)),

        print (separator)

        print ('\n | RX FEC FIFO Min      |')
        for index in range(8):
            if fec_en[index]:
                flag='<<' if not (rx_fec_fifo_min_limit[0] <= rx_fifo_min[index] <= rx_fec_fifo_min_limit[1]) else '  '
                if flag=='<<': fec_status[index]=-1
                print(" %10d %s |"%(rx_fifo_min[index],flag)),
        print ('\n | RX FEC FIFO Cnt      |')
        for index in range(8):
            if fec_en[index]:
                flag='<<' if not (rx_fifo_min[index] <= rx_fifo_cnt[index] <= rx_fifo_max[index]) else '  '
                #if flag=='<<': fec_status[index]=-1 # do not fail for live count value
                print(" %10d %s |"%(rx_fifo_cnt[index],flag)),
        print ('\n | RX FEC FIFO Max      |')
        for index in range(8):
            if fec_en[index]:
                flag='<<' if not (rx_fec_fifo_max_limit[0] <= rx_fifo_max[index] <= rx_fec_fifo_max_limit[1]) else '  '
                if flag=='<<': fec_status[index]=-1
                print(" %10d %s |"%(rx_fifo_max[index],flag)),
        print ('\n | RX FEC FIFO Delta    |')
        for index in range(8):
            if fec_en[index]:
                flag='<<' if not (rx_fec_fifo_del_limit[0] <= rx_fifo_del[index] <= rx_fec_fifo_del_limit[1]) else '  '
                if flag=='<<': fec_status[index]=-1
                print(" %10d %s |"%(rx_fifo_del[index],flag)),

        print (separator)
        
        print ('\n | TX FEC FIFO Min      |')
        for index in range(8):
            if fec_en[index]:
                flag='<<' if not (tx_fec_fifo_min_limit[0] <= tx_fifo_min[index] <= tx_fec_fifo_min_limit[1]) else '  '
                if flag=='<<': fec_status[index]=-1
                print(" %10d %s |"%(tx_fifo_min[index],flag)),
        print ('\n | TX FEC FIFO Cnt      |')
        for index in range(8):
            if fec_en[index]:
                flag='<<' if not (tx_fifo_min[index] <= tx_fifo_cnt[index] <= tx_fifo_max[index]) else '  '
                #if flag=='<<': fec_status[index]=-1 # do not fail for live count value
                print(" %10d %s |"%(tx_fifo_cnt[index],flag)),
        print ('\n | TX FEC FIFO Max      |')
        for index in range(8):
            if fec_en[index]:
                flag='<<' if not (tx_fec_fifo_max_limit[0] <= tx_fifo_max[index] <= tx_fec_fifo_max_limit[1]) else '  '
                if flag=='<<': fec_status[index]=-1
                print(" %10d %s |"%(tx_fifo_max[index],flag)),
        print ('\n | TX FEC FIFO Delta    |')
        for index in range(8):
            if fec_en[index]:
                flag='<<' if not (tx_fec_fifo_del_limit[0] <= tx_fifo_del[index] <= tx_fec_fifo_del_limit[1]) else '  '
                if flag=='<<': fec_status[index]=-1
                print(" %10d %s |"%(tx_fifo_del[index],flag)),

        print (separator)

        print ('\n | Overall FEC STATUS   |')
        for index in range(8):
            if fec_en[index]:
                fec_overall_status='    LOCK     ' if fec_status[index]==1 else '*** FAIL *** '
                print(" %12s |"%(fec_overall_status))
             

        print (separator)
    else: 
        return fec_status, fec_statistics
##################################################################################################
#
# fec_hist(hist_time = 5, print_en=True, fecs = [0,2,4,6] )
#
##################################################################################################
# Collects FEC Correctable Error Count Histogram over a period of time
# for each FEC block selected
#
# Returns tuple of all FECs statistical information (if print_en=False):
#   
# {FEC_IDX:[DataRate,Time, AM Bits, TotalUncorrCnt, TotalCorrCnt, Bin0,...,Bin15]
#   {0: [53.125,   10, 1111, 0, 157, 151, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], 
#    2: [53.125,   10, 1111, 0,  30,  24, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], 
#    4: [25.78125, 10, 1111, 0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], 
#    6: [25.78125, 10, 1111, 0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], 
#
# Prints out histogram per FEC (if print_en=True):
#
# FEC Histogram (100 sec)
# +------------------------------------------------------------------------------------------+
# |  Parameters          |     FEC_A[0]   |     FEC_A[2]   |     FEC_B[0]   |     FEC_B[2]   |
# +------------------------------------------------------------------------------------------+
# | RX FEC AmLck 0,1,2,3 |        1111    |        1111    |        1111    |        1111    |
# | RX FEC Corr   (cw)   |        1706    |         211    |           0    |           0    |
# | RX FEC Uncorr (cw)   |           0    |           0    |           0    |           0    |
# +------------------------------------------------------------------------------------------+
# | RX FEC Bin  1        |        1648    |         193    |           0    |           0    |
# | RX FEC Bin  2        |          37    |          14    |           0    |           0    |
# | RX FEC Bin  3        |          12    |           4    |           0    |           0    |
# | RX FEC Bin  4        |           6    |           0    |           0    |           0    |
# | RX FEC Bin  5        |           1    |           0    |           0    |           0    |
# | RX FEC Bin  6        |           1    |           0    |           0    |           0    |
# | RX FEC Bin  7        |           1    |           0    |           0    |           0    |
# | RX FEC Bin  8        |           0    |           0    |           0    |           0    |
# | RX FEC Bin  9        |           0    |           0    |           0    |           0    |
# | RX FEC Bin 10        |           0    |           0    |           0    |           0    |
# | RX FEC Bin 11        |           0    |           0    |           0    |           0    |
# | RX FEC Bin 12        |           0    |           0    |           0    |           0    |
# | RX FEC Bin 13        |           0    |           0    |           0    |           0    |
# | RX FEC Bin 14        |           0    |           0    |           0    |           0    |
# | RX FEC Bin 15        |           0    |           0    |           0    |           0    |
# +------------------------------------------------------------------------------------------+
# / dsh >>> fec_hist(3600)
#
# FEC Histogram (3600 sec)
# +------------------------------------------------------------------------------------------+
# |  Parameters          |     FEC_A[0]   |     FEC_A[2]   |     FEC_B[0]   |     FEC_B[2]   |
# +------------------------------------------------------------------------------------------+
# | RX FEC AmLck 0,1,2,3 |        1111    |        1111    |        1111    |        1111    |
# | RX FEC Corr   (cw)   |       60416    |       11302    |           0    |           0    |
# | RX FEC Uncorr (cw)   |           0    |           0    |           0    |           0    |
# +------------------------------------------------------------------------------------------+
# | RX FEC Bin  1        |       58821    |       10029    |           0    |           0    |
# | RX FEC Bin  2        |         986    |         836    |           0    |           0    |
# | RX FEC Bin  3        |         382    |         285    |           0    |           0    |
# | RX FEC Bin  4        |         197    |         141    |           0    |           0    |
# | RX FEC Bin  5        |          15    |           7    |           0    |           0    |
# | RX FEC Bin  6        |          13    |           4    |           0    |           0    |
# | RX FEC Bin  7        |           2    |           0    |           0    |           0    |
# | RX FEC Bin  8        |           0    |           0    |           0    |           0    |
# | RX FEC Bin  9        |           0    |           0    |           0    |           0    |
# | RX FEC Bin 10        |           0    |           0    |           0    |           0    |
# | RX FEC Bin 11        |           0    |           0    |           0    |           0    |
# | RX FEC Bin 12        |           0    |           0    |           0    |           0    |
# | RX FEC Bin 13        |           0    |           0    |           0    |           0    |
# | RX FEC Bin 14        |           0    |           0    |           0    |           0    |
# | RX FEC Bin 15        |           0    |           0    |           0    |           0    |
# +------------------------------------------------------------------------------------------+
#
##################################################################################################
def fec_hist(hist_time = 5, print_en=True, fecs = [0,2,4,6] ):
    if chip_rev == 1.0:
        print ("\n*** fec_hist() function is not supported in Babbage A0 ***\n")
        return
    
    fec_class1= [0x4000,0x4100,0x4200,0x4300, 0x5000,0x5100,0x5200,0x5300]
    #fec_class2= [0x4400,0x4500,0x4600,0x4700, 0x5400,0x5500,0x5600,0x5700]
    fec_class3= [0x4800,0x4900,0x4a00,0x4b00, 0x5800,0x5900,0x5a00,0x5b00]

    result={}
    bins=range(0,17)

    #### Start the FEC Counters
    for fec_idx in fecs:
        wregBits(fec_class3[fec_idx]+0x63, [0], 1) # clear error counter
        wregBits(fec_class3[fec_idx]+0x63, [0], 0)
        wregBits(fec_class3[fec_idx]+0x63, [1], 0) # unfreeze the counter
    
    #### Wait for FEC Stat Collection Time
    time.sleep(hist_time)
    
    #### Stop the FEC Counters and Histograms
    for fec_idx in fecs:        
        wregBits(fec_class3[fec_idx]+0x63, [1], 1) # freeze the counter
        
    #### Now process the FEC Counters and Histograms
    for fec_idx in fecs:
        result[fec_idx]=[]
        cw_size = 5440 if fec_idx < 4 else 5280
        data_rate = 53.125 if fec_idx < 4 else 25.78125
        bits_transferred = float((hist_time*data_rate*pow(10,9)))  
               
        ampslock = fec_status_ampsLock(fec_idx)
        total_uncorr_cnt= (rreg(fec_class1[fec_idx] +0xcd)<<16) + rreg(fec_class1[fec_idx] +0xcc) # read hist uncorr count
        total_corr_cnt  = (rreg(fec_class1[fec_idx] +0xcb)<<16) + rreg(fec_class1[fec_idx] +0xca) # read hist corr count
        result[fec_idx].append(data_rate)  # fec_idx][0]
        result[fec_idx].append(hist_time) # fec_idx][1]
        result[fec_idx].append((ampslock>>3&1)*1000 + (ampslock>>3&1)*100 + (ampslock>>1&1)*10 + (ampslock>>0&1) )  # fec_idx][2]
        result[fec_idx].append(total_uncorr_cnt) # fec_idx][3]
        result[fec_idx].append(total_corr_cnt) # fec_idx][4]
        
        corr_cnt_per_bin = [0]*17
        corr_ber_per_bin = [0]*17
        for bin in bins: # fec_idx][6++]
            wregBits(fec_class3[fec_idx]+0x60, [4,0], bin) # select hist bin
            corr_cnt_per_bin[bin] = (rreg(fec_class3[fec_idx]+0x62)<<16) + rreg(fec_class3[fec_idx]+0x61) # read hist corr count
            if bin == 0:
                wregBits(fec_class3[fec_idx]+0x60, [4,0], bin+17)
                corr_cnt_per_bin[bin] += (rreg(fec_class3[fec_idx]+0x61)<<32)
                total_cw_transferred = corr_cnt_per_bin[0] + total_corr_cnt + total_uncorr_cnt 
                result[fec_idx].append(total_cw_transferred) # fec_idx][5]

            result[fec_idx].append(corr_cnt_per_bin[bin]) # fec_idx][6++]
            
    #### Now turn off the FEC Counters and Histograms
    for fec_idx in fecs:
       wregBits(fec_class3[fec_idx]+0x63, [1], 0) # unfreeze the counter

    #### Print out Final results and Histograms
    if print_en:
        length_1 = 22
        length = 17

        print("\n  FEC Histogram (%d sec)"%(hist_time)),
        num_fec_en=0
        fec_en=[0]*8
        for fec_idx in fecs:
            if(fec_status_en(fec_idx)):
                fec_en[fec_idx]=1
                num_fec_en+=1
            
        separator='\n +'
        for i in range(length_1):
            separator+='-'
        for i in range(length*num_fec_en):
            separator+='-'*print_en
        separator += '+'   
        
        print (separator)
        print ('\n |  Parameters          |')
        for fec_idx in fecs:
            if fec_en[fec_idx]:
                fec_type = 'A'   if fec_idx<4 else 'B'
                macro    = fec_idx if fec_idx<4 else fec_idx-4
                print('    FEC_%s[%d]   |'%(fec_type,macro)),
                if print_en==2: print('    FEC_%s[%d]   |'%(fec_type,macro)),

        print (separator)           
        print ('\n | RX FEC AmLck 0,1,2,3 |')
        for fec_idx in fecs:
            if fec_en[fec_idx]:
                print("       %04d    |"%(result[fec_idx][2])),
                if print_en==2: print("       %04d    |"%(result[fec_idx][2])),
                
        print ('\n | RX FEC Uncorr (cw)   |')
        for fec_idx in fecs:
            if fec_en[fec_idx]:
                print(" %10d    |"%(result[fec_idx][3])),
                if print_en==2: print(" %10.1e    |"%(float(result[fec_idx][3])/float(result[fec_idx][5]))),
        print ('\n | RX FEC Corr   (cw)   |')
        for fec_idx in fecs:
            if fec_en[fec_idx]:
                print(" %10d    |"%(result[fec_idx][4])),
                if print_en==2: print(" %10.1e    |"%(float(result[fec_idx][4])/float(result[fec_idx][5]))),
        print (separator)
        for bin in bins:
            print('\n | RX FEC Bin %2d        |'%(bin )),
            for fec_idx in fecs:
                if fec_en[fec_idx]:
                    print(" %10d    |"%(result[fec_idx][6+bin])),
                    if print_en==2: print(" %10.1e    |"%(float(result[fec_idx][6+bin])/float(result[fec_idx][5]))),
        print (separator)

    if not print_en:
        return result 
####################################################################################################
def fec_ber(fecs=[0,2,4,6]):
    # fec_class = [0x4000,0x4100,0x4200,0x4300, 0x5000,0x5100,0x5200,0x5300]
    # fec_class2= [0x4400,0x4500,0x4600,0x4700, 0x5400,0x5500,0x5600,0x5700]
    # fec_class3= [0x4800,0x4900,0x4a00,0x4b00, 0x5800,0x5900,0x5a00,0x5b00]
    print_en=1
    if chip_rev == 1.0:
        print ("\n*** fec_status_pre_fec_ber() This function is not supported in Babbage A0 ***\n")
        return
    fec_type=['A','A','A','A','B','B','B','B']
    if print_en: print('\n...Gearbox Pre-FEC BER')
    for fec_idx in fecs:
        if print_en: print('\n   FEC_%s[%d] : '%(fec_type[fec_idx],fec_idx)),
        fec_base_addr = fec_class3[fec_idx]
        cw_size = 5440 if fec_idx < 4 else 5280
        
        wreg(fec_base_addr+0x63,0x8000) # turn the clock on for reading counters bit[4]=0
        time.sleep(0.10)
        wreg(fec_base_addr+0x63,0x8002) # freeze the counters bit[1]=1, turn the clock on for reading counters bit[4]=0
        time.sleep(0.010)
        total_errors_hi   = rreg(fec_base_addr+0x7f)
        total_errors_mid  = rreg(fec_base_addr+0x7e)
        total_errors_lo   = rreg(fec_base_addr+0x7d)
     
        total_cw_hi  = rreg(fec_base_addr+0x5f)
        total_cw_mid = rreg(fec_base_addr+0x5e)
        total_cw_lo  = rreg(fec_base_addr+0x5d) 
       
        total_errors = long( (total_errors_hi << 32) + (total_errors_mid<<16) + (total_errors_lo) )
        total_cw     = long( (total_cw_hi     << 32) + (total_cw_mid    <<16) + (total_cw_lo    ) )
        
        #if print_en: print ("\n...FEC %d , BaseAddr: 0x%04X, CW Size: %d, Pre-FEC BER"%(fec_idx,fec_base_addr,cw_size))
        #if print_en: print ("\n...Bits In Error      : %04X_%04X_%04X = %11d bits"%(total_errors_hi,total_errors_mid,total_errors_lo,total_errors))
        #if print_en: print ("\n...Total CWs Received : %04X_%04X_%04X = %11d CWs"%(total_cw_hi,total_cw_mid,total_cw_lo, total_cw))
        #if print_en: print ("%04X_%04X_%04X (%11d bits),"%(total_errors_hi,total_errors_mid,total_errors_lo,total_errors)),
        #if print_en: print ("%04X_%04X_%04X (%11d CWs),"%(total_cw_hi,total_cw_mid,total_cw_lo, total_cw)),
        if total_errors == 0 and total_cw != 0:
            #ber = 0.0 
            if print_en: print ("0"),
        elif total_cw == 0:
            #ber = -1.0 
            if print_en: print ("ERROR. CW Count =0 !!!"),
        else:
            ber = float(total_errors)/float(total_cw)/float(cw_size)    
            if print_en: print ("%2.2e"%(ber)),
        
        wreg(fec_base_addr+0x63,0x8000) # unfreeze the counters, bit[1]=0, 
        time.sleep(0.010)
        wreg(fec_base_addr+0x63,0x8001) # clear the counters for next time, bit[0]=1
        time.sleep(0.010)
        wreg(fec_base_addr+0x63,0x8000) # clear the counters for next time, bit[0]=0
        #time.sleep(0.010)
        #wreg(fec_base_addr+0x63,0x8010) # turn the clock off to save power, bit[4]=1
    #return total_errors, total_cw, ber    
####################################################################################################
# Set any lane in "shallow loop-back mode internally":
#
# A0-RX looped back out to A0-TX
# A1-RX looped back out to A1-TX
# ...
# B6-RX looped back out to B6-TX 
# B7-RX looped back out to B7-TX 
#
####################################################################################################
def sw_config_loopback_mode(lane=None, enable='en'):

    lanes = get_lane_list(lane)

    #### TOP PLL: need to set dvi2 mode
    #wreg(0x9500,0x1a68) ## top pll
    #wreg(0x9600,0x1a68)
    
    #get_lane_mode(lanes) # update current Encoding modes of all lanes for this Slice
    loopback_bit = 1 if enable == 'en' else 0
    
    if loopback_bit==1: # loopback_mode enabled, TX in functional mode  
        wreg(0x9857,0x0000,lane) # Disable FEC
        wreg(0x9858,0x0000,lane) # Disable any lane mapping for Gearbox modes
        wreg(0x9859,0x0000,lane) # Disable BM (BitMux)
        wreg(0x9855,0x0000,lane) # Disable any lane mapping for Gearbox modes
        wreg(0x985f,0x0000,lane) # Disable AN
                   
        wreg(0x98d1,0x8888,lane) # [15:12]=8: B7RX->B7TX, [11:8]=8: B6RX->B6TX, [7:4]=8: B5RX->B5TX, [3:0]=8: B4RX->B4TX
        wreg(0x98d2,0x8888,lane) # [15:12]=8: B3RX->B3TX, [11:8]=8: B2RX->B2TX, [7:4]=8: B1RX->B1TX, [3:0]=8: B0RX->B0TX
        wreg(0x98d3,0x8888,lane) # [15:12]=8: A7RX->A7TX, [11:8]=8: A6RX->A6TX, [7:4]=8: A5RX->A5TX, [3:0]=8: A4RX->A4TX
        wreg(0x98d4,0x8888,lane) # [15:12]=8: A3RX->A3TX, [11:8]=8: A2RX->A2TX, [7:4]=8: A1RX->A1TX, [3:0]=8: A0RX->A0TX
    else:                        # loopback_mode disabled, TX in PRBS mode
        wreg(0x98d1,0x0000,lane) # [15:12]=0: B7_RXPLL->B7TX, [11:8]=0: B6_RXPLL->B6TX, [7:4]=0: B5_RXPLL->B5TX, [3:0]=0: B4_RXPLL->B4TX
        wreg(0x98d2,0x0000,lane) # [15:12]=0: B3_RXPLL->B3TX, [11:8]=0: B2_RXPLL->B2TX, [7:4]=0: B1_RXPLL->B1TX, [3:0]=0: B0_RXPLL->B0TX
        wreg(0x98d3,0x0000,lane) # [15:12]=0: A7_RXPLL->A7TX, [11:8]=0: A6_RXPLL->A6TX, [7:4]=0: A5_RXPLL->A5TX, [3:0]=0: A4_RXPLL->A4TX
        wreg(0x98d4,0x0000,lane) # [15:12]=0: A3_RXPLL->A3TX, [11:8]=0: A2_RXPLL->A2TX, [7:4]=0: A1_RXPLL->A1TX, [3:0]=0: A0_RXPLL->A0TX            

 
    for ln in lanes:      
        get_lane_mode(ln)
        wreg(0x3000+(0x100*ln),0x0000,lane=0) ####disable link training of this lane
        wreg(0xe000+(0x100*ln),0xc000,lane=0) ####disable AutoNeg of this lane  
        
        #### TX rotator and pi enable
        if loopback_bit==0: # Do this first thing if disabling loop-back mode
            wreg([0x0f4,   [0]], loopback_bit, ln) ## TX_PLL refclk Source 0: crystal 1:RX_PLL recovered clock
        wreg([0x0f3,[14,8]], 0x00        , ln) ## [15]->1, TX phase rotator value
        wreg([0x0f3,  [15]], loopback_bit, ln) ## [15]->1, TX phase rotator enable
       #wreg([0x0f3,   [2]], loopback_bit, ln) # 0 or 1 for pam4
        wreg([0x0f3,   [2]],            0, ln) # TRF pol is controlled by register 0x79 or 0x17a


        if loopback_bit==1: # loopback_mode enabled, TX in functional mode
            prbs_mode_select(lane=ln, prbs_mode='functional') # Put TX in Functional Mode
        else:                         # loopback_mode disabled, TX in PRBS mode
            prbs_mode_select(lane=ln, prbs_mode='prbs')    # Put TX in PRBS Mode7
            
        ##### Enable loop-back for this lane
        if ln==0:    #A0
            wreg([0x9811,[1]],loopback_bit,lane=0); wreg([0x9811,[3]],loopback_bit,lane=0);
        elif ln==1:  #A1
            wreg([0x9819,[1]],loopback_bit,lane=0); wreg([0x9819,[3]],loopback_bit,lane=0);
        elif ln==2:  #A2
            wreg([0x9821,[1]],loopback_bit,lane=0); wreg([0x9821,[3]],loopback_bit,lane=0);
        elif ln==3:  #A3
            wreg([0x9829,[1]],loopback_bit,lane=0); wreg([0x9829,[3]],loopback_bit,lane=0);
        elif ln==4:  #A4
            wreg([0x9831,[1]],loopback_bit,lane=0); wreg([0x9831,[3]],loopback_bit,lane=0);
        elif ln==5:  #A5
            wreg([0x9839,[1]],loopback_bit,lane=0); wreg([0x9839,[3]],loopback_bit,lane=0);
        elif ln==6:  #A6
            wreg([0x9841,[1]],loopback_bit,lane=0); wreg([0x9841,[3]],loopback_bit,lane=0);
        elif ln==7:  #A7
            wreg([0x9849,[1]],loopback_bit,lane=0); wreg([0x9849,[3]],loopback_bit,lane=0);
            
        elif ln==8:  #B0
            wreg([0x9811,[0]],loopback_bit,lane=0); wreg([0x9811,[2]],loopback_bit,lane=0);
        elif ln==9:  #B1
            wreg([0x9819,[0]],loopback_bit,lane=0); wreg([0x9819,[2]],loopback_bit,lane=0);
        elif ln==10: #B2
            wreg([0x9821,[0]],loopback_bit,lane=0); wreg([0x9821,[2]],loopback_bit,lane=0);
        elif ln==11: #B3
            wreg([0x9829,[0]],loopback_bit,lane=0); wreg([0x9829,[2]],loopback_bit,lane=0);
        elif ln==12: #B4
            wreg([0x9831,[0]],loopback_bit,lane=0); wreg([0x9831,[2]],loopback_bit,lane=0);
        elif ln==13: #B5
            wreg([0x9839,[0]],loopback_bit,lane=0); wreg([0x9839,[2]],loopback_bit,lane=0);
        elif ln==14: #B6
            wreg([0x9841,[0]],loopback_bit,lane=0); wreg([0x9841,[2]],loopback_bit,lane=0);
        elif ln==15: #B7
            wreg([0x9849,[0]],loopback_bit,lane=0); wreg([0x9849,[2]],loopback_bit,lane=0);
        else:
            print("\nsw_config_loopback_mode: ***> Lane # must be between 0 to 15")
            return
        


        ##### TX peer for this lane's RX is lane_name_list[gLanePartnerMap[gSlice][ln]]
        #
        
        if gEncodingMode[gSlice][ln][0] == 'pam4':
            if loopback_bit==1: # PAM4 53G PLL_N
               #wreg(0x1fd,0x113f,ln)  ## perlane RXPLL pll_n
               #wreg([0x1f5,[8]],0,ln) ## perlane RXPLLpll[8]->0: div2 not bypassed
               #wreg(0x079,0x20a4,ln)  ## PAM4 TRF
                wreg(0x079,0x3084,ln)  ## PAM4 TRF
            else:
               #wreg(0x1fd,0x223f,ln)  ## perlane RXPLLpll_n, same as in the init_pam4/init_nrz
               #wreg([0x1f5,[8]],1,ln) ## perlane RXPLLpll[8]->1: div2 bypassed
                wreg(0x079,0x00a4,ln)  ## PAM4 TRF, set to the same value as in the init_pam4/init_nrz

        else: # NRZ mode (TBD, not completed yet)
            if loopback_bit==1: # NRZ 25.78125G PLL_N
               #wreg(0x1fd,0x113f,ln)  ## perlane RXPLL pll_n???
               #wreg([0x1f6,[8]],0,ln) ## perlane RXPLL pll[8]->0: div2 not bypassed
               #wreg(0x17a,0x116C,ln) ##  NRZ TRF
                wreg(0x17a,0x3084,ln) ##  NRZ TRF
            else:
               #wreg(0x1FD,0x213f,ln) # NRZ mode, RX_PLLN= 33*2*195.3125 |  [3:1]  //vcoi=0x7
               #wreg(0x1f6,0x71b0,ln) # NRZ mode, vagcbufdac
                wreg(0x17a,0x00a4,ln) ##  NRZ TRF, set to the same value as in the init_pam4/init_nrz
        
        if loopback_bit==1: # Do this as the last thing when enabling loopback mode
            wreg([0x0f4,   [0]], loopback_bit, ln) ## TX_PLL refclk Source 0: crystal 1:RX_PLL recovered clock
        
####################################################################################################
# 
# Processes the lane list passed
# Based on number of A-lanes in the list returns...
# ... all lanes connected from A side to B side in retimer mode
#
####################################################################################################
def get_retimer_lane_list(lane=None, cross_mode=False):

    lanes=get_lane_list(lane)
    A_lanes=[x for x in lanes if x < 8]
    lanes=A_lanes[:]
    for ln in A_lanes:
        if cross_mode==False:
            lanes.append(ln+8)  # retimer straight mode lanes A0-A7 to B0-B7
        else:
            if ln<4:  
                lanes.append(ln+12) # retimer cross mode lanes A0-A3 to B4-B7
            else:# ln>=4: 
                lanes.append(ln+4)  # retimer cross mode lanes A4-A7 to B0-B3
    lanes.sort()            
    return lanes
####################################################################################################
def retimer_status(lane=None,print_en=True):
    retimer_status = True
    lanes = get_lane_list(lane)
    A_lanes=[x for x in lanes if x < 8]
    if print_en: print("\n...Retimer FIFO Status")
    for ln in A_lanes:
        fifo_addr = 0x9813 + (ln%8)*8
        fifo_1 = Gray_Bin(rreg([fifo_addr,[15,13]],ln));  
        fifo_2 = Gray_Bin(rreg([fifo_addr,[12,10]],ln));  
        fifo_3 = Gray_Bin(rreg([fifo_addr,[ 9, 7]],ln));  
        fifo_4 = Gray_Bin(rreg([fifo_addr,[ 6, 4]],ln));  
        
        if fifo_1 != 5 or fifo_2 != 5 or fifo_3 != 5 or fifo_4 != 5 :  retimer_status = False

        if print_en: print("\n   A%d Rx -->[%d,%d]--> Tx B%d  %s"%(ln,fifo_1,fifo_2,ln,'<<' if fifo_1 !=5 or fifo_2 !=5 else '' )),
        if print_en: print("\n      Tx <--[%d,%d]<-- Rx     %s"%(fifo_3,fifo_4,'<<' if fifo_3 !=5 or fifo_4 !=5 else '' ))
    if print_en: print("\n")
    return retimer_status
####################################################################################################
# Instruct the FW to configure an A+B lane in retimer mode
# 
# Options:
#     mode    = 'pam4', 'pam4-LT', 'nrz'/'nrz25'/'nrz-LT', 'nrz10', 'nrz20'
#     lanes = any of the lanes, i.e. [0,1,2,3,4,5,6,7] or [8,9,10,11,12,13,14,15]"),
#
##############################################################################
def fw_config_retimer (mode=None, lane=range(8), cross_mode=False,print_en=0):

    if not fw_loaded(print_en=0):
        print("\n*** No FW Loaded. Skipping fw_config_retimer().\n")
        return
    
    retimer_en=1;
    LT_en=1 if 'LT' in mode.upper() else 0

    if mode is None: # or all(ln >= 8 for ln in A_lanes): # if any of the A-lanes > 8
        print("\n*** Instruct the FW to configure an A+B lane in retimer mode"),
        print("\n*** Options: "),
        print("\n***         mode = 'pam4', 'pam4-LT', 'nrz'/'nrz25'/'nrz-LT', 'nrz10', 'nrz20'"),
        print("\n***         lane = any of the A lanes, i.e. [0,1,2,3,4,5,6,7]"),
        return
    
    lanes = get_lane_list(lane)
    A_lanes=[x for x in lanes if x < 8]
    
    speed_str_list =['10','20','25','26','28','51','50','53','56'] # speed as part of mode argument
    speed_code_list=[0x11,0x22,0x33,0x44,0x55,0x88,0x99,0x99,0xAA] # speed codes to be written to 0x9807[7:0]
    
    if 'NRZ' in mode.upper():
        destroy_cmd = 0x9000 if cross_mode==False else 0x9020  # command to destroy a NRZ lane
        config_cmd  = 0x8000 if cross_mode==False else 0x8020  # command to activate lane in NRZ      
        speed_code_cmd = 0x33                                  # default NRZ speed is 25G
        for i in range(len(speed_str_list)): # if NRZ speed is specified as part of the mode, take it
            if speed_str_list[i] in mode: speed_code_cmd = speed_code_list[i]
        if LT_en:
            speed_code_cmd  += 0x200
        
    elif 'PAM4' in mode.upper():
        destroy_cmd = 0x9010 if cross_mode==False else 0x9030  # command to destroy a PAM4 lane
        config_cmd  = 0x8010 if cross_mode==False else 0x8030  # command to activate lane in PAM4
        speed_code_cmd = 0x66                                  # default PAM4 speed is 53G
        for i in range(len(speed_str_list)): # if PAM4 speed is specified as part of the mode, take it
            if speed_str_list[i] in mode: speed_code_cmd = speed_code_list[i]
        if LT_en:
            speed_code_cmd  += 0x200
            
    elif any(i in mode.upper() for i in ['OFF','DIS','DISABLE']): 
        destroy_cmd = 0x9010 if cross_mode==False else 0x9030  # command to destroy a retimer lane
        retimer_en=0
    else:
        for ln in A_lanes:
            print("\n*** fw_config_retimer_mode: selected mode '%s' for lane A%d is invalid" %(mode.upper(), ln)),
        return
        
    ######## First, do all the destroys #############
    for ln in A_lanes:
        result = fw_config_cmd (config_cmd=destroy_cmd+ln,config_detail=0x0000) 
        if (result!=c.fw_config_lane_status): # fw_config_lane_status=0x800
            print("\n***Lane %s: FW could not free up lane before reconfiguring it as retimer. (Error Code 0x%04x)" %(lane_name_list[ln],result)),
    #for ln in A_lanes:
    #    wreg([0xa0,[13]],0,ln)    # Make sure TX output is not squelched on A-side
    #    if cross_mode==False:
    #        wreg([0xa0,[13]],0,ln+8)  # Make sure TX output is not squelched on B-side
    #    else:
    #        wreg([0xa0,[13]],0,ln+12) # Make sure TX output is not squelched on B-side
        
    ######## Next, do all the activates if retimer_en=1 #############
    for ln in A_lanes:
        if cross_mode==False:
            b_ln = ln
            tag = '<--->'
            tag2 = 'Direct non-LT Mode' if LT_en==0 else 'Direct LT Mode'
        elif ln<4:
            b_ln = ln+4
            tag = '<-X->'
            tag2 = 'Crossed non-LT Mode' if LT_en==0 else 'Crossed LT Mode'
        elif ln>=4:
            b_ln = ln-4
            tag = '<-X->'          
            tag2 = 'Crossed non-LT Mode' if LT_en==0 else 'Crossed LT Mode'
        if retimer_en==1:
            if print_en: print("\n...FW Retimer: Enabled Retimer in %s between Lanes A%d %s B%d"%(tag2,ln,tag,b_ln)),
            result = fw_config_cmd (config_cmd=config_cmd+ln,config_detail=speed_code_cmd) 
            if (result!=c.fw_config_lane_status): # fw_config_lane_status=0x800
                print("\n***Slice %d Lane %s FW_CONFIG_RETIMER Failed. (SpeedCode 0x9807=0x%04X, ActiveCode 0x9806=0x%04X, ExpectedStatus:0x%0X, ActualStatus=0x%04x)" %(gSlice,lane_name_list[ln],speed_code_cmd, config_cmd+ln,c.fw_config_lane_status,result)),
        else: # retimer disabled
            if print_en: print("\n...FW Retimer: Disabled Retimer on Lane A%d"%(ln)),
    #for ln in A_lanes:
    #    wreg([0xa0,[13]],0,ln)    # Make sure TX output is not squelched on A-side
    #    if cross_mode==False:
    #        wreg([0xa0,[13]],0,ln+8)  # Make sure TX output is not squelched on B-side
    #    else:
    #        wreg([0xa0,[13]],0,ln+12) # Make sure TX output is not squelched on B-side

####################################################################################################
# Instruct the FW to configure an A+B lane in Loopback mode
# 
# Options:
#     mode    = 'pam4', 'pam4-LT', 'nrz'/'nrz25'/'nrz-LT', 'nrz10', 'nrz20'
#     lanes = any of the lanes, i.e. [0,1,2,3,4,5,6,7] or [8,9,10,11,12,13,14,15]"),
#
##############################################################################
def fw_config_loopback (mode=None, lane=range(16), print_en=0):

    if not fw_loaded(print_en=0):
        print("\n*** No FW Loaded. Skipping fw_config_loopback().\n")
        return
    
    Loopback_en=1;

    if mode is None: 
        print("\n*** Instruct the FW to configure an A+B lane in Loopback mode"),
        print("\n*** Options: "),
        print("\n***         mode = 'pam4', 'nrz'/'nrz25'/'nrz10'/'nrz20'"),
        print("\n***         lane = any of anes, i.e. [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16]"),
        return
    
    lanes = get_lane_list(lane)

    
    speed_str_list =['10','20','25','26','28','51','50','53','56'] # speed as part of mode argument
    speed_code_list=[0x11,0x22,0x33,0x44,0x55,0x88,0x99,0x99,0xAA] # speed codes to be written to 0x9807[7:0]
    
    if 'NRZ' in mode.upper():
        destroy_cmd = 0x90E0                                   # command to destroy a NRZ lane
        config_cmd  = 0x80E0                                   # command to activate lane in NRZ      
        speed_code_cmd = 0x33                                  # default NRZ speed is 25G
        for i in range(len(speed_str_list)): # if NRZ speed is specified as part of the mode, take it
            if speed_str_list[i] in mode: speed_code_cmd = speed_code_list[i]
        
    elif 'PAM4' in mode.upper():
        destroy_cmd = 0x90F0                                   # command to destroy a PAM4 lane
        config_cmd  = 0x80F0                                   # command to activate lane in PAM4
        speed_code_cmd = 0x66                                  # default PAM4 speed is 53G
        for i in range(len(speed_str_list)): # if PAM4 speed is specified as part of the mode, take it
            if speed_str_list[i] in mode: speed_code_cmd = speed_code_list[i]
            
    elif any(i in mode.upper() for i in ['OFF','DIS','DISABLE']): 
        destroy_cmd = 0x90F0                                   # command to destroy a Loopback lane
        Loopback_en=0
    else:
        for ln in lanes:
            print("\n*** fw_config_retimer_mode: selected mode '%s' for lane A%d is invalid" %(mode.upper(), ln)),
        return
        
    ######## First, do all the destroys #############
    for ln in lanes:
        result = fw_config_cmd (config_cmd=destroy_cmd+ln,config_detail=0x0000) 
        if (result!=c.fw_config_lane_status): # fw_config_lane_status=0x800
            print("\n***Lane %s: FW could not free up lane before reconfiguring it as Loopback. (Error Code 0x%04x)" %(lane_name_list[ln],result)),

    for ln in lanes:
        if Loopback_en==1:
            if print_en: print("\n...FW Loopback: Enabled Loopback on Lane %d"%(ln)),
            result = fw_config_cmd (config_cmd=config_cmd+ln,config_detail=speed_code_cmd) 
            if (result!=c.fw_config_lane_status): # fw_config_lane_status=0x800
                print("\n***Slice %d Lane %s FW_CONFIG_LOOPBACK Failed. (SpeedCode 0x9807=0x%04X, ActiveCode 0x9806=0x%04X, ExpectedStatus:0x%0X, ActualStatus=0x%04x)" %(gSlice,lane_name_list[ln],speed_code_cmd, config_cmd+ln,c.fw_config_lane_status,result)),
        else: # Loopback disabled
            if print_en: print("\n...FW Loopback: Disabled Loopback on Lane %d"%(ln)),

####################################################################################################  
# FW to Configure lanes in Bitmux A1:B2 NRZ-NRZ mode
#
# A-side NRZ 2x20G  to  B-side: NRZ 4x10G
# A-lanes are running exactly at 2X the data rate of the B-lanes
# Must initialize and optimize the lanes in at proper data rate before calling this routine
#
# Options: A_lanes=[0,1]
#          A_lanes=[2,3]
#          A_lanes=[4,5]
#          A_lanes=[0,1,2,3]
#          A_lanes=[0,1,4,5]
####################################################################################################
def fw_config_bitmux_20G(A_lanes=[0,1],print_en=0):

    if not fw_loaded(print_en=0):
        print("\n...FW Bitmux 20G: FW not loaded. BITMUX Not Configured!"),
        return
    
    
    
    # For 2x20G to 4x10G Bitmux mode, 3 options supported for A-Lane groups 
    group1_bitmux=[0,1] # A_lanes group 1 -> [A0,A1] <-> [B0,B1,B2,B3]
    group2_bitmux=[2,3] # A_lanes group 2 -> [A2,A3] <-> [B4,B5,B6,B7]
    group3_bitmux=[4,5] # A_lanes group 3 -> [A4,A5] <-> [B4,B5,B6,B7]
    
    #Determine the corresponding B-Lanes for each group of A-Lanes
    group1_selected=0
    group2_selected=0
    B_lanes=[]
    if all(elem in A_lanes for elem in group1_bitmux):  # If A_lanes contains [0,1]
        B_lanes +=[8,9,10,11]
        group1_selected=1
    if all(elem in A_lanes for elem in group2_bitmux):  # If A_lanes contains [2,3]
        B_lanes+=[12,13,14,15]
        group2_selected=1
    elif all(elem in A_lanes for elem in group3_bitmux): # If A_lanes contains [4,5]
        B_lanes+=[12,13,14,15]
        group2_selected=1
    
    if group1_selected==0 and group2_selected==0:
        print("\n*** Bitmux Setup: Invalid Target A-Lanes specified!"),
        print("\n*** Options: A_lanes=[0,1]"),
        print("\n***          A_lanes=[2,3]"),
        print("\n***          A_lanes=[4,5]"),
        print("\n***          A_lanes=[0,1,2,3]"),
        print("\n***          A_lanes=[0,1,4,5]"),
        return
    
    #lanes = sorted(list(set(A_lanes + B_lanes)))
    #prbs_mode_select(lane=lanes, prbs_mode='functional')
            
    ######## First, do all the destroys #############
    if all(elem in A_lanes for elem in group2_bitmux):  # If A_lanes contains [0,1]
        fw_config_cmd(config_cmd=0x9040,config_detail=0x0000) # 0x9040 = First, FW destroy any instances of these lanes being already used      
        fw_config_cmd(config_cmd=0x9041,config_detail=0x0000) # 0x9041 = First, FW destroy any instances of these lanes being already used
    if all(elem in A_lanes for elem in group2_bitmux):  # If A_lanes contains [2,3]
        fw_config_cmd(config_cmd=0x9042,config_detail=0x0000) # 0x9041 = First, FW destroy any instances of these lanes being already used
        fw_config_cmd(config_cmd=0x9043,config_detail=0x0000) # 0x9042 = First, FW destroy any instances of these lanes being already used     
        fw_config_cmd(config_cmd=0x9044,config_detail=0x0000) # 0x9043 = First, FW destroy any instances of these lanes being already used
        fw_config_cmd(config_cmd=0x9045,config_detail=0x0000) # 0x9044 = First, FW destroy any instances of these lanes being already used
    elif all(elem in A_lanes for elem in group3_bitmux): # If A_lanes contains [4,5]
        fw_config_cmd(config_cmd=0x9042,config_detail=0x0000) # 0x9041 = First, FW destroy any instances of these lanes being already used
        fw_config_cmd(config_cmd=0x9043,config_detail=0x0000) # 0x9042 = First, FW destroy any instances of these lanes being already used     
        fw_config_cmd(config_cmd=0x9044,config_detail=0x0000) # 0x9043 = First, FW destroy any instances of these lanes being already used
        fw_config_cmd(config_cmd=0x9045,config_detail=0x0000) # 0x9044 = First, FW destroy any instances of these lanes being already used


    ######## Next, do all the activates #############
    
    ######## Bitmux A2/A3 to B4/B5/B6/B7    
    if all(elem in A_lanes for elem in group1_bitmux):  # If A_lanes contains [0,1]
        if print_en: print("\n...FW BitMux 20G : Setting Up Lane A0/A1 to B0/B1/B2/B3..."),
        fw_config_cmd(config_cmd=0x8040,config_detail=0x0021) # 0x8040 = FW to Activate Bitmux A1B2 NRZ for Lane A0, 0x0021 = FW Bitmux A1B2 NRZ command        
        fw_config_cmd(config_cmd=0x8041,config_detail=0x0021) # 0x8041 = FW to Activate Bitmux A1B2 NRZ for Lane A1, 0x0021 = FW Bitmux A1B2 NRZ command       

    ######## Bitmux A2/A3 to B4/B5/B6/B7
    if all(elem in A_lanes for elem in group2_bitmux):  # If A_lanes contains [2,3]
        if print_en: print("\n...FW BitMux 20G : Setting Up Lane A2/A3 to B4/B5/B6/B7..."),
        fw_config_cmd(config_cmd=0x8042,config_detail=0x0021) # 0x8042 = FW to Activate Bitmux A1B2 NRZ for Lane A2, 0x0021 = FW Bitmux A1B2 NRZ command       
        fw_config_cmd(config_cmd=0x8043,config_detail=0x0021) # 0x8043 = FW to Activate Bitmux A1B2 NRZ for Lane A3, 0x0021 = FW Bitmux A1B2 NRZ command       

    ######## Bitmux A4/A5 to B4/B5/B6/B7    
    elif all(elem in A_lanes for elem in group3_bitmux): # If A_lanes contains [4,5]
        if print_en: print("\n...FW BitMux 20G : Setting Up Lane A4/A5 to B4/B5/B6/B7..."),
        fw_config_cmd(config_cmd=0x8044,config_detail=0x0021) # 0x8044 = FW to Activate Bitmux A1B2 NRZ for Lane A4, 0x0021 = FW Bitmux A1B2 NRZ command
        fw_config_cmd(config_cmd=0x8045,config_detail=0x0021) # 0x8045 = FW to Activate Bitmux A1B2 NRZ for Lane A5, 0x0021 = FW Bitmux A1B2 NRZ command

    #prbs_mode_select(lane=lanes, prbs_mode='functional')
    #fw_adapt_wait(max_wait=20, lane=A_lanes+B_lanes, print_en=1)

    return A_lanes, B_lanes
####################################################################################################  
# FW to Configure lanes in Bitmux A1:B2 NRZ-NRZ mode
#
# A-side NRZ 2x20G  to  B-side: NRZ 4x10G
# A-lanes are running exactly at 2X the data rate of the B-lanes
# Must initialize and optimize the lanes in at proper data rate before calling this routine
#
# Options: A_lanes=[0,1]
#          A_lanes=[2,3]
#          A_lanes=[4,5]
#          A_lanes=[0,1,2,3]
#          A_lanes=[0,1,4,5]
####################################################################################################
def fw_config_bitmux_40G_LT(A_lanes=[0,1,2,3],print_en=0):

    if not fw_loaded(print_en=0):
        print("\n...FW Bitmux 20G: FW not loaded. BITMUX Not Configured!"),
        return
    
    
    
    # For 2x20G to 4x10G Bitmux mode, 3 options supported for A-Lane groups 
    group1_bitmux=[0,1] # A_lanes group 1 -> [A0,A1] <-> [B0,B1,B2,B3]
    group2_bitmux=[2,3] # A_lanes group 2 -> [A2,A3] <-> [B4,B5,B6,B7]
    group3_bitmux=[4,5] # A_lanes group 3 -> [A4,A5] <-> [B4,B5,B6,B7]
    
    #Determine the corresponding B-Lanes for each group of A-Lanes
    group1_selected=0
    group2_selected=0
    B_lanes=[]
    if all(elem in A_lanes for elem in group1_bitmux):  # If A_lanes contains [0,1]
        B_lanes +=[8,9,10,11]
        group1_selected=1
    if all(elem in A_lanes for elem in group2_bitmux):  # If A_lanes contains [2,3]
        B_lanes+=[12,13,14,15]
        group2_selected=1
    elif all(elem in A_lanes for elem in group3_bitmux): # If A_lanes contains [4,5]
        B_lanes+=[12,13,14,15]
        group2_selected=1
    
    if group1_selected==0 and group2_selected==0:
        print("\n*** 40G-ANLT Bitmux Setup: Invalid Target A-Lanes specified!"),
        print("\n*** Options: A_lanes=[0,1]"),
        print("\n***          A_lanes=[2,3]"),
        print("\n***          A_lanes=[4,5]"),
        print("\n***          A_lanes=[0,1,2,3]"),
        print("\n***          A_lanes=[0,1,4,5]"),
        return
    
    #lanes = sorted(list(set(A_lanes + B_lanes)))
    #prbs_mode_select(lane=lanes, prbs_mode='functional')
            
    ######## First, do all the destroys #############
    if all(elem in A_lanes for elem in group2_bitmux):  # If A_lanes contains [0,1]
        fw_config_cmd(config_cmd=0x9046,config_detail=0x0000) # 0x9046 = First, FW destroy any instances of these lanes being already used
    if all(elem in A_lanes for elem in group2_bitmux):  # If A_lanes contains [2,3]
        fw_config_cmd(config_cmd=0x9047,config_detail=0x0000) # 0x9046 = First, FW destroy any instances of these lanes being already used
        fw_config_cmd(config_cmd=0x9048,config_detail=0x0000) # 0x9047 = First, FW destroy any instances of these lanes being already used
    elif all(elem in A_lanes for elem in group3_bitmux): # If A_lanes contains [4,5]
        fw_config_cmd(config_cmd=0x9047,config_detail=0x0000) # 0x9047 = First, FW destroy any instances of these lanes being already used
        fw_config_cmd(config_cmd=0x9048,config_detail=0x0000) # 0x9048 = First, FW destroy any instances of these lanes being already used


    ######## Next, do all the activates #############
    
    ######## Bitmux A2/A3 to B4/B5/B6/B7    
    if all(elem in A_lanes for elem in group1_bitmux):  # If A_lanes contains [0,1]
        if print_en: print("\n...FW BitMux 40G-ANLT : Setting Up Lane A0/A1 to B0/B1/B2/B3..."),
        fw_config_cmd(config_cmd=0x8046,config_detail=0x0221) # 0x8046 = FW to Activate ANLT Bitmux A1B2 NRZ for Lane A0, 0x0021 = FW Bitmux A1B2 NRZ command        

    ######## Bitmux A2/A3 to B4/B5/B6/B7
    if all(elem in A_lanes for elem in group2_bitmux):  # If A_lanes contains [2,3]
        if print_en: print("\n...FW BitMux 40G-ANLT : Setting Up Lane A2/A3 to B4/B5/B6/B7..."),
        fw_config_cmd(config_cmd=0x8047,config_detail=0x0221) # 0x8047 = FW to Activate ANLT Bitmux A1B2 NRZ for Lane A2, 0x0021 = FW Bitmux A1B2 NRZ command       

    ######## Bitmux A4/A5 to B4/B5/B6/B7    
    elif all(elem in A_lanes for elem in group3_bitmux): # If A_lanes contains [4,5]
        if print_en: print("\n...FW BitMux 40G-ANLT : Setting Up Lane A4/A5 to B4/B5/B6/B7..."),
        fw_config_cmd(config_cmd=0x8048,config_detail=0x0221) # 0x8044 = FW to Activate Bitmux A1B2 NRZ for Lane A4, 0x0021 = FW Bitmux A1B2 NRZ command

    #prbs_mode_select(lane=lanes, prbs_mode='functional')
    #fw_adapt_wait(max_wait=20, lane=A_lanes+B_lanes, print_en=1)

    return A_lanes, B_lanes
####################################################################################################  
# FW to Configure lanes in Bitmux A1:B2 PAM4-NRZ mode
#
# A-side NRZ 2x53G  to  B-side: NRZ 4x26G
# A-lanes are running exactly at 2X the data rate of the B-lanes
# Must initialize and optimize the lanes in at proper data rate before calling this routine
#
# Options: A_lanes=[0,1]
#          A_lanes=[2,3]
#          A_lanes=[4,5]
#          A_lanes=[0,1,2,3]
#          A_lanes=[0,1,4,5]
####################################################################################################
def fw_config_bitmux_53G(A_lanes=[0,1],print_en=0):

    if not fw_loaded(print_en=0):
        print("\n...FW Bitmux 53G: FW not loaded. BITMUX Not Configured!"),
        return

    # For 2x20G to 4x10G Bitmux mode, 3 options supported for A-Lane groups 
    group1_bitmux=[0,1] # A_lanes group 1 -> [A0,A1] <-> [B0,B1,B2,B3]
    group2_bitmux=[2,3] # A_lanes group 2 -> [A2,A3] <-> [B4,B5,B6,B7]
    group3_bitmux=[4,5] # A_lanes group 3 -> [A4,A5] <-> [B4,B5,B6,B7]
    
    #Determine the corresponding B-Lanes for each group of A-Lanes
    group1_selected=0
    group2_selected=0
    B_lanes=[]
    if all(elem in A_lanes for elem in group1_bitmux):  # If A_lanes contains [0,1]
        B_lanes +=[8,9,10,11]
        group1_selected=1
    if all(elem in A_lanes for elem in group2_bitmux):  # If A_lanes contains [2,3]
        B_lanes+=[12,13,14,15]
        group2_selected=1
    elif all(elem in A_lanes for elem in group3_bitmux): # If A_lanes contains [4,5]
        B_lanes+=[12,13,14,15]
        group2_selected=1
    
    if group1_selected==0 and group2_selected==0:
        print("\n*** Bitmux Setup: Invalid Target A-Lanes specified!"),
        print("\n*** Options: A_lanes=[0,1]"),
        print("\n***          A_lanes=[2,3]"),
        print("\n***          A_lanes=[4,5]"),
        print("\n***          A_lanes=[0,1,2,3]"),
        print("\n***          A_lanes=[0,1,4,5]"),
        return
    
    #lanes = sorted(list(set(A_lanes + B_lanes)))
    #prbs_mode_select(lane=lanes, prbs_mode='functional')

        
    ######## First, do all the destroys #############
    if all(elem in A_lanes for elem in group2_bitmux):  # If A_lanes contains [0,1]
        fw_config_cmd(config_cmd=0x9050,config_detail=0x0000) # 0x9050 = First, FW destroy any instances of these lanes being already used      
        fw_config_cmd(config_cmd=0x9051,config_detail=0x0000) # 0x9051 = First, FW destroy any instances of these lanes being already used
    if all(elem in A_lanes for elem in group2_bitmux):  # If A_lanes contains [2,3]
        fw_config_cmd(config_cmd=0x9052,config_detail=0x0000) # 0x9052 = First, FW destroy any instances of these lanes being already used
        fw_config_cmd(config_cmd=0x9053,config_detail=0x0000) # 0x9053 = First, FW destroy any instances of these lanes being already used     
        fw_config_cmd(config_cmd=0x9054,config_detail=0x0000) # 0x9054 = First, FW destroy any instances of these lanes being already used
        fw_config_cmd(config_cmd=0x9055,config_detail=0x0000) # 0x9055 = First, FW destroy any instances of these lanes being already used
    elif all(elem in A_lanes for elem in group3_bitmux): # If A_lanes contains [4,5]
        fw_config_cmd(config_cmd=0x9052,config_detail=0x0000) # 0x9052 = First, FW destroy any instances of these lanes being already used
        fw_config_cmd(config_cmd=0x9053,config_detail=0x0000) # 0x9053 = First, FW destroy any instances of these lanes being already used     
        fw_config_cmd(config_cmd=0x9054,config_detail=0x0000) # 0x9054 = First, FW destroy any instances of these lanes being already used
        fw_config_cmd(config_cmd=0x9055,config_detail=0x0000) # 0x9055 = First, FW destroy any instances of these lanes being already used


    
    ######## Next, do all the activates #############
    
    ######## Bitmux A2/A3 to B4/B5/B6/B7    
    if all(elem in A_lanes for elem in group1_bitmux):  # If A_lanes contains [0,1]
        if print_en: print("\n...FW BitMux 53G : Setting Up Lane A0/A1 to B0/B1/B2/B3..."),
        fw_config_cmd(config_cmd=0x8050,config_detail=0x0064) # 0x8050 = FW to Activate Bitmux 53G for Lane A0, 0x0064 = FW Bitmux 53G command        
        fw_config_cmd(config_cmd=0x8051,config_detail=0x0064) # 0x8051 = FW to Activate Bitmux 53G for Lane A1, 0x0064 = FW Bitmux 53G command       

                                                                                                   
    ######## Bitmux A2/A3 to B4/B5/B6/B7                                                          
    if all(elem in A_lanes for elem in group2_bitmux):  # If A_lanes contains [2,3]               
        if print_en: print("\n...FW BitMux 53G : Setting Up Lane A2/A3 to B4/B5/B6/B7..."),       
        fw_config_cmd(config_cmd=0x8052,config_detail=0x0064) # 0x8052 = FW to Activate Bitmux 53G for Lane A2, 0x0064 = FW Bitmux 53G command       
        fw_config_cmd(config_cmd=0x8053,config_detail=0x0064) # 0x8053 = FW to Activate Bitmux 53G for Lane A3, 0x0064 = FW Bitmux 53G command       
    
    
    ######## Bitmux A4/A5 to B4/B5/B6/B7                                                                      
    elif all(elem in A_lanes for elem in group3_bitmux): # If A_lanes contains [4,5]                          
        if print_en: print("\n...FW BitMux 53G : Setting Up Lane A4/A5 to B4/B5/B6/B7..."),                   
        fw_config_cmd(config_cmd=0x8054,config_detail=0x0064) # 0x8054 = FW to Activate Bitmux 53G for Lane A4, 0x0064 = FW Bitmux 53G command
        fw_config_cmd(config_cmd=0x8055,config_detail=0x0064) # 0x8055 = FW to Activate Bitmux 53G for Lane A5, 0x0064 = FW Bitmux 53G command


    #fw_adapt_wait(max_wait=20, lane=A_lanes+B_lanes, print_en=1)


    return A_lanes, B_lanes
    
def fw_config_bitmux_51G(A_lanes=[0,1],print_en=0):

    if not fw_loaded(print_en=0):
        print("\n...FW Bitmux 53G: FW not loaded. BITMUX Not Configured!"),
        return

    # For 2x20G to 4x10G Bitmux mode, 3 options supported for A-Lane groups 
    group1_bitmux=[0,1] # A_lanes group 1 -> [A0,A1] <-> [B0,B1,B2,B3]
    group2_bitmux=[2,3] # A_lanes group 2 -> [A2,A3] <-> [B4,B5,B6,B7]
    group3_bitmux=[4,5] # A_lanes group 3 -> [A4,A5] <-> [B4,B5,B6,B7]
    
    #Determine the corresponding B-Lanes for each group of A-Lanes
    group1_selected=0
    group2_selected=0
    B_lanes=[]
    if all(elem in A_lanes for elem in group1_bitmux):  # If A_lanes contains [0,1]
        B_lanes +=[8,9,10,11]
        group1_selected=1
    if all(elem in A_lanes for elem in group2_bitmux):  # If A_lanes contains [2,3]
        B_lanes+=[12,13,14,15]
        group2_selected=1
    elif all(elem in A_lanes for elem in group3_bitmux): # If A_lanes contains [4,5]
        B_lanes+=[12,13,14,15]
        group2_selected=1
    
    if group1_selected==0 and group2_selected==0:
        print("\n*** Bitmux Setup: Invalid Target A-Lanes specified!"),
        print("\n*** Options: A_lanes=[0,1]"),
        print("\n***          A_lanes=[2,3]"),
        print("\n***          A_lanes=[4,5]"),
        print("\n***          A_lanes=[0,1,2,3]"),
        print("\n***          A_lanes=[0,1,4,5]"),
        return
    
    #lanes = sorted(list(set(A_lanes + B_lanes)))
    #prbs_mode_select(lane=lanes, prbs_mode='functional')

        
    ######## First, do all the destroys #############
    if all(elem in A_lanes for elem in group2_bitmux):  # If A_lanes contains [0,1]
        fw_config_cmd(config_cmd=0x9060,config_detail=0x0000) # 0x9050 = First, FW destroy any instances of these lanes being already used      
        fw_config_cmd(config_cmd=0x9061,config_detail=0x0000) # 0x9051 = First, FW destroy any instances of these lanes being already used
    if all(elem in A_lanes for elem in group2_bitmux):  # If A_lanes contains [2,3]
        fw_config_cmd(config_cmd=0x9062,config_detail=0x0000) # 0x9052 = First, FW destroy any instances of these lanes being already used
        fw_config_cmd(config_cmd=0x9063,config_detail=0x0000) # 0x9053 = First, FW destroy any instances of these lanes being already used     
        fw_config_cmd(config_cmd=0x9064,config_detail=0x0000) # 0x9054 = First, FW destroy any instances of these lanes being already used
        fw_config_cmd(config_cmd=0x9065,config_detail=0x0000) # 0x9055 = First, FW destroy any instances of these lanes being already used
    elif all(elem in A_lanes for elem in group3_bitmux): # If A_lanes contains [4,5]
        fw_config_cmd(config_cmd=0x9062,config_detail=0x0000) # 0x9052 = First, FW destroy any instances of these lanes being already used
        fw_config_cmd(config_cmd=0x9063,config_detail=0x0000) # 0x9053 = First, FW destroy any instances of these lanes being already used     
        fw_config_cmd(config_cmd=0x9064,config_detail=0x0000) # 0x9054 = First, FW destroy any instances of these lanes being already used
        fw_config_cmd(config_cmd=0x9065,config_detail=0x0000) # 0x9055 = First, FW destroy any instances of these lanes being already used


    
    ######## Next, do all the activates #############
    
    ######## Bitmux A2/A3 to B4/B5/B6/B7    
    if all(elem in A_lanes for elem in group1_bitmux):  # If A_lanes contains [0,1]
        if print_en: print("\n...FW BitMux 53G : Setting Up Lane A0/A1 to B0/B1/B2/B3..."),
        fw_config_cmd(config_cmd=0x8060,config_detail=0x0083) # 0x8050 = FW to Activate Bitmux 53G for Lane A0, 0x0064 = FW Bitmux 53G command        
        fw_config_cmd(config_cmd=0x8061,config_detail=0x0083) # 0x8051 = FW to Activate Bitmux 53G for Lane A1, 0x0064 = FW Bitmux 53G command       

                                                                                                   
    ######## Bitmux A2/A3 to B4/B5/B6/B7                                                          
    if all(elem in A_lanes for elem in group2_bitmux):  # If A_lanes contains [2,3]               
        if print_en: print("\n...FW BitMux 53G : Setting Up Lane A2/A3 to B4/B5/B6/B7..."),       
        fw_config_cmd(config_cmd=0x8062,config_detail=0x0083) # 0x8052 = FW to Activate Bitmux 53G for Lane A2, 0x0064 = FW Bitmux 53G command       
        fw_config_cmd(config_cmd=0x8063,config_detail=0x0083) # 0x8053 = FW to Activate Bitmux 53G for Lane A3, 0x0064 = FW Bitmux 53G command       
    
    
    ######## Bitmux A4/A5 to B4/B5/B6/B7                                                                      
    elif all(elem in A_lanes for elem in group3_bitmux): # If A_lanes contains [4,5]                          
        if print_en: print("\n...FW BitMux 53G : Setting Up Lane A4/A5 to B4/B5/B6/B7..."),                   
        fw_config_cmd(config_cmd=0x8064,config_detail=0x0083) # 0x8054 = FW to Activate Bitmux 53G for Lane A4, 0x0064 = FW Bitmux 53G command
        fw_config_cmd(config_cmd=0x8065,config_detail=0x0083) # 0x8055 = FW to Activate Bitmux 53G for Lane A5, 0x0064 = FW Bitmux 53G command


    #fw_adapt_wait(max_wait=20, lane=A_lanes+B_lanes, print_en=1)


    return A_lanes, B_lanes

####################################################################################################
# 
# Configure specified lane(s) in NRZ or PAM4 mode
#
# Note: this function sets minimum requirement for a lane to be in PAM4 or NRZ mode
#       It does not do a complete initialization of the lane in a NRZ/PAM4 mode
#       See init_lane() for complete initialization.
####################################################################################################
def set_lane_mode(mode='pam4',lane=None):

    lanes = get_lane_list(lane)

    for ln in lanes:
        if mode.upper()!='PAM4': # put lanes in NRZ mode
            wreg([0x041,[15]],0, ln) # Disable PAM4 mode
            wreg([0x0a0,[13]],0, ln) # Disable PAM4 PRBS Generator
            wreg([0x0b0, [1]],1, ln) # Enable NRZ mode
            wreg([0x0b0,[11]],1, ln) # Enable NRZ PRBS Generator
        else: #################### put lanes in PAM4 mode
            wreg([0x0b0, [1]],0, ln) # Disable NRZ mode
            wreg([0x0b0,[11]],0, ln) # Disable NRZ PRBS Generator
            wreg([0x041,[15]],1, ln) # Enable PAM4 mode
            wreg([0x0a0,[13]],1, ln) # Enable PAM4 PRBS Generator    
####################################################################################################
# 
# Checks to see if a lane is in PAM4 or NRZ mode
#
# updates the global gEncodingMode variable for this Slice and this lane
# returns: list of [NRZ/PAM4, speed] per lane
#
####################################################################################################
def get_lane_mode(lane=None):
    
    lanes = get_lane_list(lane)
 
    global gEncodingMode
    
    for ln in lanes:

        if rreg([0x0ff,[12]],ln)==0 or rreg([0x1ff,[12]],ln)==0: # Lane's bandgap is OFF
            #print ("\n Slice %d lane %2d is OFF"%(gSlice,ln)),
            data_rate= 1.0
            gEncodingMode[gSlice][ln] = ['off',data_rate]
            lane_mode_list[ln] = 'off'

        elif rreg([0xb0,[1]],ln) == 0 and rreg([0x41,[15]],ln) == 1:
            #print ("\n Slice %d lane %2d is PAM4"%(gSlice,ln)),
            data_rate= get_lane_pll(ln)[ln][0][0]
            gEncodingMode[gSlice][ln] = ['pam4',data_rate]
            lane_mode_list[ln] = 'pam4'
        else:
            #print ("\n Slice %d lane %2d is NRZ"%(gSlice,ln)),
            data_rate= get_lane_pll(ln)[ln][0][0]
            gEncodingMode[gSlice][ln] = ['nrz',data_rate]
            lane_mode_list[ln] = 'nrz'
            
    return gEncodingMode
####################################################################################################
# 
# Initialize lane(s) in NRZ or PAM4 mode
#
# Note: this function does a complete initialization of the lane in a NRZ/PAM4 mode
#       It does not adapt the RX of the lane to the connected channel
#       See opt_lane() for optimization of lane
####################################################################################################
def init_lane(mode='pam4',datarate=None,input_mode='ac',lane=None):

    lanes = get_lane_list(lane)

    for ln in lanes:
        if 'NRZ' in mode.upper(): # put lanes in NRZ mode
            if datarate!=None:  init_lane_nrz (datarate,input_mode,ln) # NRZ at exactly the requested datarate
            elif '10' in mode:  init_lane_nrz (10.3125, input_mode,ln) # NRZ-10G
            elif '20' in mode:  init_lane_nrz (20.6250, input_mode,ln) # NRZ-20G
            elif '25' in mode:  init_lane_nrz (25.78125,input_mode,ln) # NRZ-25G
            else:               init_lane_nrz (25.78125,input_mode,ln) # NRZ-25G, or exactly the requested datarate      
        else: #################### put lanes in PAM4 mode
            init_lane_pam4(datarate,input_mode,ln)  
####################################################################################################
# 
# Initialize and adapt lane(s) in NRZ or PAM4 mode
#
# Note: this function does a complete initialization and adaptation of lane in a NRZ/PAM4 mode
#       It optimizes the RX of the lane to the connected channel
####################################################################################################
def opt_lane(mode='pam4',datarate=None, input_mode='ac',lane=None):

    lanes = get_lane_list(lane)
    set_bandgap('on', 'all') 
    set_top_pll(pll_side='both', freq=195.3125)
    
    #### FW-based Adaptation
    if fw_loaded(print_en=0):
        for ln in lanes:
            #init_lane_for_fw(mode = mode,tx_pol=1, rx_pol=RxPolarityMap[gSlice][ln],input_mode = input_mode,lane=ln)
            init_lane_for_fw(mode,datarate,input_mode,ln)
        fw_config_lane(mode,datarate,lanes)
        fw_adapt_wait (lane=lanes, print_en=2)
        
    #### Python-based Adaptation
    else:
        for ln in lanes:
            if 'NRZ' in mode.upper(): # put lanes in NRZ mode and optimize them
                if datarate!=None:  opt_lane_nrz (datarate,input_mode,ln) # NRZ-25G, or exactly the requested datarate
                elif '10' in mode:  opt_lane_nrz (10.3125, input_mode,ln) # NRZ-10G
                elif '20' in mode:  opt_lane_nrz (20.6250, input_mode,ln) # NRZ-20G
                elif '25' in mode:  opt_lane_nrz (25.78125,input_mode,ln) # NRZ-25G
                else:               opt_lane_nrz (25.78125,input_mode,ln) # NRZ-25G, or exactly the requested datarate
            else: #################### put lanes in PAM4 mode and optimize them
                opt_lane_pam4(datarate,input_mode,ln)  
            
####################################################################################################
# 
# Initialize target lane(s) in NRZ mode
#
# Note: this function does a complete initialization of the lane in a NRZ mode
#       It does not adapt the RX of the lane to the connected channel
#       See opt_lane() for complete initialization.
####################################################################################################
def init_lane_for_fw(mode='nrz',tx_pol=1,rx_pol=0, input_mode='ac',lane=None):
    #print'\n...init_lane_for_fw'
    lanes = get_lane_list(lane)
    global gEncodingMode


    for ln in lanes:
        set_lane_mode(mode=mode,lane=ln)
        ####################### put lane in PAM4 mode
        if ('PAM4' in mode.upper()):
            wreg([0x0af,[5,1]],0x4,lane=gLanePartnerMap[gSlice][ln][1]) # TX Taps scales = (0.5,0.5, 1, 0.5,0.5)
            tx_taps(2,-8,17,0,0,lane=ln)
            # PAM4 mode, gc=en | Pre=dis | masblsb= default
            gc(1,1,lane=ln)
            pc(0,0,lane=ln)
            msblsb(0,0,lane=ln)

            if gPrbsEn:
                prbs_mode_select(lane=ln,prbs_mode='prbs') # PAM4 mode, tx pam4 prbs31
                #wreg(0x042,0xb3fd,ln) 
            else:
                prbs_mode_select(lane=ln,prbs_mode='functional') # PAM4 mode, TX pam4 FUNCTIONAL MODE
                #wreg(0x042,0xb3fd,ln) # PAM4 mode, Rx PAM4 gc=en | Pre=dis

        ####################### put lane in NRZ mode
        if ('NRZ' in mode.upper()):
            wreg([0x0af,[5,1]],0x4,lane=gLanePartnerMap[gSlice][ln][1]) # TX Taps scales = (0.5,0.5, 1, 0.5,0.5)
            #tx_taps(0,-8,17,0,0,lane=ln)
            #tx_taps(0,0,31,0,0,lane=ln)
            #tx_taps(0,-2,25,0,-2,lane=ln)
            if ((sel_slice() == 3 or sel_slice() == 2) and ln<=11):#Port 1,2
                tx_taps(0,-2,25,0,-5,lane=ln)
            elif (sel_slice() == 7 and ln<=11):#Port3
                tx_taps(0,-4,24,0,0,lane=ln)
            elif ((sel_slice() == 6 or sel_slice() == 14 or sel_slice() == 15 or sel_slice() == 31) and ln <=11):#Port4,5,6,7
                tx_taps(0,-4,27,0,-2,lane=ln)
            elif((sel_slice() == 6 or sel_slice() == 7 or sel_slice() == 15) and (ln >= 12 and ln <=15)):#Port 10,11,12,13,14
                tx_taps(0,-3,27,0,-5,lane=ln)
            elif(sel_slice() == 30 and (ln >= 8 and ln <=13)): #Port 8 and Port 9(lane 1~2)
                tx_taps(0,-3,27,0,-5,lane=ln)
            elif(sel_slice() == 31 and (ln >= 14 and ln <=15)): #Port 9(lane 3~4)
                tx_taps(0,-3,26,0,-5,lane=ln)
            else:
                tx_taps(0,-2,25,0,-2,lane=ln)
            
            # NRZ mode, gc=dis | Pre=dis | masblsb= default (none applies in NRZ mode)
            gc(0,0,lane=ln)
            pc(0,0,lane=ln)
            msblsb(0,0,lane=ln)
            #wreg([0x161,[8]],1,ln) # NRZ freq_loop enable ## FW set this during init_lane
            if gPrbsEn: #print'PRBS_ON'
                prbs_mode_select(lane=ln,prbs_mode='prbs')
                #wreg(0x161,0x7520,ln) # NRZ mode, Rx checker
            else:      #print'FUNCTIONAL_ON'
                prbs_mode_select(lane=ln,prbs_mode='functional')
                #wreg(0x161,0x3520,ln) # NRZ mode, Rx checker off
          
        if chip_rev()==2.0:
            wreg([0x07b,[3]],  1, ln) # R2.0 PAM4 BLWC Pol = 1
            wreg([0x17c,[15]], 0, ln) # R2.0 NRZ BLWC en = 0 (disabled)
                     
        if (input_mode=='dc'):
            wreg(c.rx_vcomrefsel_ex_addr,1,ln) # DC-Coupled mode 0x1DD[9]=1
            wreg(c.rx_en_vcominbuf_addr ,1,ln) # DC-Coupled mode 0x1E7[11]=1
        else:
            wreg(c.rx_vcomrefsel_ex_addr,0,ln) # AC-Coupled mode 0x1DD[9]=0
            wreg(c.rx_en_vcominbuf_addr ,1,ln) # AC-Coupled mode 0x1E7[11]=1

        # Set the polarities of the lanes
        pol(TxPolarityMap[gSlice][ln],RxPolarityMap[gSlice][ln],ln,0)
        

def init_lane_nrz(datarate=None, input_mode='ac',lane=None):

    lanes = get_lane_list(lane)
    global gEncodingMode
    c=NrzReg
    
    if datarate is None:  datarate=25.78125
    # if datarate!=None:  datarate=datarate # NRZ at exactly the requested datarate
    # elif '10' in mode:  datarate=10.3125  # NRZ-10G
    # elif '20' in mode:  datarate=20.6250  # NRZ-20G
    # elif '25' in mode:  datarate=25.78125 # NRZ-25G
    # else:               datarate=25.78125 # NRZ-25G   

    for ln in lanes:
        #print("\ninit_lane_nrz(): Lane %s, Target DataRate=%3.5fG, Actual DataRate=%3.5f G"%(lane_name_list[ln],datarate,get_lane_pll(ln)[ln][0][0])),
        gEncodingMode[gSlice][ln] = ['nrz',datarate]
        lane_mode_list[ln] = 'nrz'
        ####disable AN/LT registers as default
        wreg(0x3000+(0x100*ln),0x0000,lane=0)
        wreg(0xe000+(0x100*ln),0xc000,lane=0)            
        # ---------------- LANE Q NRZ init begin

        #wreg(c.rx_lane_rst_addr, 1, ln) # Keep lane in Reset while programming lane
        #wreg(c.rx_pll_pu_addr,   0, ln) # Power down RX PLL while prgramming PLL
        #wreg(c.tx_pll_pu_addr,   0, ln) # Power down RX PLL while prgramming PLL
        ####################### NRZ mode, Per-Lane PLLs ###
        wreg(0x0D7,0x0000,ln) # NRZ mode, TX_PLL, Fractional PLL off  
        wreg(0x0D8,0x0000,ln) # NRZ mode, TX_PLL, Fractional PLL off  
        wreg(0x1F0,0x0000,ln) # NRZ mode, RX_PLL, Fractional PLL off  
        wreg(0x1F1,0x0000,ln) # NRZ mode, RX_PLL, Fractional PLL off    
        
       #wreg(0x0FF,0x5df6,ln) # NRZ mode, TX_BG bandgap | div4=0
       #wreg(0x0FE,0x213e,ln) # NRZ mode, TX PLLN = 66*195.3125 | [3:1] //vcoi=0x7
        wreg(0x0FF,0x5df4,ln) # NRZ mode, TX_BG bandgap | div4=0, div2_pass=0
        wreg(0x0FE,0x10bf,ln) # NRZ mode, TX PLLN = 33*195.3125 | [3:1] //vcoi=0x7
        wreg(0x0FD,0x5636,ln) # NRZ mode, TX PLL,bypass_pll=0
       #wreg(0x0FD,0x5436,ln) # NRZ mode, TX PLL, clear bypass 1p7 regulator 0xFD[9]
        wreg(0x0FC,0x7236,ln) # NRZ mode, TX PLL,vvco_reg
        wreg(0x0FB,0x7fb7,ln) # NRZ mode, TX PLL,vref_intp
        wreg(0x0FA,0x8010,ln) # NRZ mode, TX PLL,testmode_tx_pll
        wreg(0x0DB,0x2100,ln) # NRZ mode, TX PLL,[14:8] // tx_vcocap=rx_vcocap=33 or 34 for fvco=25.78125G
        wreg(0x0DA,0x7de6,ln) # NRZ mode, TX PLL,[14:10]// bm_vco=0x1f
        wreg(0x0D9,0xb760,ln) # NRZ mode,
                  
        wreg(0x1FF,0x5db1,ln) # NRZ mode, RX_BG bandgap | [15:13]//vbg=0
      
        wreg(0x1FD,0x213f,ln) # NRZ mode, RX_PLLN= 33*2*195.3125 |  [3:1]  //vcoi=0x7
       #wreg(0x1FC,0x1048,ln) # NRZ mode, RX PLL, clear bypass 1p7 regulator 0x1FC[9]
        wreg(0x1F5,0x4340,ln) # NRZ mode, RX PLL, lcvco_cap [15:9] //rx_vcocap=33 or 34 for fvco=25.78125G
        wreg(0x1F4,0x7de6,ln) # NRZ mode, RX PLL, en_div2=0 | [14:10]//bm_vco=0x1f
        wreg(0x1F3,0xb760,ln) # NRZ mode, RX PLL, pu_intp_rx



        ####################### analog ###

        wreg(0x0f1,0x000f,ln) # NRZ mode, pu_adc|pu_clkcomp|pu_clkreg
        wreg(0x0ef,0x9230,ln) # NRZ mode, vrefadc
        wreg(0x0ee,0x691c,ln) # NRZ mode, calnbs_top|bot
        wreg(0x0ed,0x7777,ln) # NRZ mode, edge
        wreg(0x0eb,0x67fc,ln) # NRZ mode, TX Swing 
                    
        wreg(0x1f9,0x7686,ln) # NRZ mode, vrefagcdegen
        wreg(0x1f6,0x71b0,ln) # NRZ mode, vagcbufdac
        wreg(0x1e7,0xc34c,ln) # NRZ mode, pu_agc|pu_agcdl | en_vcominbuf=0
        wreg(0x1e6,0x88a5,ln) # NRZ mode, bypass_agcreg
        wreg(0x1e5,0x6c38,ln) # NRZ mode, 4e3c # vrefagc1p5reg
                  
        wreg(0x1dd,0xc162,ln) # NRZ mode, c1e2 # ffe_delay | en_skef | skef_value
        wreg(0x1da,0x6f00,ln) # NRZ mode, 6d80 # clkphase0
                  
        wreg(0x1d4,0x01c0,ln) # NRZ mode, agcgain1 | agacgain2
        wreg(0x1d5,0x0100,ln) # NRZ mode, blwc_en
        wreg(0x1d6,0xee00,ln) # NRZ mode, en_az_short_pulse

        #######################  NRZ Mode Configuration
        wreg(0x041,0x0000,ln) # NRZ mode, Toggle PAM4_RX_ENABLE
        wreg(0x041,0x8000,ln) # NRZ mode, Toggle PAM4_RX_ENABLE
        wreg(0x041,0x0000,ln) # NRZ mode, Toggle PAM4_RX_ENABLE
        wreg(0x0b0,0x4802,lane=gLanePartnerMap[gSlice][ln][1]) # NRZ mode (4802) | Toggle PAM4_TX_ENABLE
        wreg(0x0b0,0x0000,lane=gLanePartnerMap[gSlice][ln][1]) # NRZ mode (4802) | Toggle PAM4_TX_ENABLE
        wreg(0x0b0,0x4802,lane=gLanePartnerMap[gSlice][ln][1]) # NRZ mode (4802) | Toggle PAM4_TX_ENABLE
        wreg(0x1e7,0x836c,ln) # NRZ mode, ffe disable in nrz mode (agc_dl=0)

        #######################  NRZ SM Configuration
        #wreg(0x101,0x4201,ln)
        wreg(0x101,0x0201,ln) # NRZ mode, delta_freeze  
        wreg(0x102,0x1006,ln) # NRZ mode, cntr_target=0x100 during link-up, change to 0x002 after
        wreg(0x103,0x5933,ln)
        wreg(0x104,0x700a,ln)
       #wreg(0x105,0x8682,ln)  #8e82, # UPDATE 20180521
       #wreg(0x105,0x8e82,ln)  #8e82, 
        wreg(0x106,0xA100,ln) 
        wreg(0x108,0x688B,ln)  
        wreg(0x10C,0x8000,ln)  
        wreg(0x13B,0xe000,ln) # NRZ mode, Set KP=6, # UPDATE 20180521
        #wreg(0x145,0x8080,ln) # commented out CHANGE_FOR_10G, # UPDATE 20180521
        wreg(0x14D,0x8000,ln) # NRZ mode, OW INIT_FREQ=0  
        wreg([0x014D, [10,0]], 0x7D8, ln) #set one initial freq to be -160ppm, # CHANGE_FOR_10G, # UPDATE 20180521
        wreg(0x14F,0x0100,ln)
        wreg(0x150,0x5040,ln)
        wreg(0x161,0x0120,ln) # NRZ mode, Rx checker power-down.
        wreg(0x163,0x0080,ln)
        wreg(0x164,0x8080,ln)
        wreg(0x179,0x8874,ln) # 887e bb_mode=1
        wreg(0x180,0x3500,ln)  
        wreg(0x181,0x1000,ln)  
        wreg([0x0181, [10,0]], 0x7D8, ln) #set one initial freq to be -160ppm, # CHANGE_FOR_10G, # UPDATE 20180521
        wreg(0x182,0xD800,ln)  
                
        #wreg(0x18C,0x0040,ln) # CHANGE_FOR_10G # UPDATE 20180521         
        wreg(0x176,0x418A,ln) #209a ctle_map
        wreg(0x177,0x3837,ln) 
        wreg(0x178,0x97EE,ln)
        wreg(0x14E,0x3800,ln) #CTLE=7

        wreg(0x17C,0x0000,ln) # REV_2.0 default  = 0x8001
        
        ####################### NRZ mode, TX setting
        if gNrzTxSourceIsCredo:
            wreg(0x0a5,0x0000,lane=gLanePartnerMap[gSlice][ln][1]) # NRZ mode, pre2
            wreg(0x0a7,0xf800,lane=gLanePartnerMap[gSlice][ln][1]) # NRZ mode, pre1
            wreg(0x0a9,0x1100,lane=gLanePartnerMap[gSlice][ln][1]) # NRZ mode, main
            wreg(0x0ab,0x0000,lane=gLanePartnerMap[gSlice][ln][1]) # NRZ mode, post1
            wreg(0x0ad,0x0000,lane=gLanePartnerMap[gSlice][ln][1]) # NRZ mode, post2
            wreg(0x0af,0xfc08,lane=gLanePartnerMap[gSlice][ln][1]) # NRZ mode, tx_automode|no gc|nopre |SHmode

        ####################### NRZ mode,  TX PRBS gen and Rx PRBS checker
        if gPrbsEn:
            wreg(0x0a0,0xeb20,lane=gLanePartnerMap[gSlice][ln][1]) # NRZ mode, tx prbs31
            wreg(0x0a0,0xe320,lane=gLanePartnerMap[gSlice][ln][1]) # NRZ mode, tx prbs31
            wreg(0x0a0,0xeb20,lane=gLanePartnerMap[gSlice][ln][1]) # NRZ mode, tx prbs31
            wreg(0x161,0x7520,ln) # NRZ mode, Rx checker
        else:
            wreg(0x0a0,0x0120,lane=gLanePartnerMap[gSlice][ln][1]) # NRZ mode, TX functional mode
            wreg(0x0a0,0x0120,lane=gLanePartnerMap[gSlice][ln][1]) # NRZ mode, TX functional mode
            wreg(0x0a0,0x0120,lane=gLanePartnerMap[gSlice][ln][1]) # NRZ mode, TX functional mode
            wreg(0x161,0x3520,ln) # NRZ mode, Rx checker off
        
       #set_lane_pll(tgt_pll='both',datarate=datarate, div2=0, lane=ln)
        
        if chip_rev()==2.0:
            wreg([0x07b,[3]],  1, ln) # R2.0 PAM4 BLWC Pol = 1
            wreg([0x17c,[15]], 0, ln) # R2.0 NRZ BLWC en = 0 (disabled)
            if datarate < 24.0: # If 10G or 20G using Frac-N, expanded to 20 bits for TX-PLL 
                wreg([0x0d9,[3,0]],0x6, ln) # NRZ-10G/20G mode, 0x0D9[3:0] TX PLL FRAC_N[19:16]=0x6 = 0.4 (R2.0: 20 bits, R1.0: 16 bits)

        if datarate < 15.0: # NRZ 10.3125G , NRZ Half-Rate Mode
            #### disable Fractional PLLs while programming PLLs,  # UPDATE 20180521
            wreg(c.tx_pll_frac_en_addr,     0, ln) # NRZ-10G mode, 0x0D7   [13] TX PLL FRAC_EN=0 while programming PLLs
            wreg(c.rx_pll_frac_en_addr,     0, ln) # NRZ-10G mode, 0x1F0   [13] RX PLL FRAC_EN=0 while programming PLLs
            
            wreg(c.tx_pll_lvcocap_addr,    85, ln) # NRZ-10G mode, 0x0DB [14:8] TX PLL VCOCAP=84 or 85 for fvco=20.625G                
            wreg(c.tx_pll_n_addr,          26, ln) # NRZ-10G mode, 0x0FE [15:7] TX_PLL N= 26.4*2*195.3125
            wreg(c.tx_pll_div4_addr,        0, ln) # NRZ-10G mode, 0x0FF    [0] TX_PLL DIV4=0
            wreg(c.tx_pll_div2_addr,        0, ln) # NRZ-10G mode, 0x0FF    [1] TX PLL DIV2=0
            if chip_rev()==2.0:
                wreg([0x0d9,[3,0]],          0x6, ln) # NRZ-10G mode, 0x0D9  [3:0] TX PLL FRAC_N[19:16]=0x6    = 0.4 (R2.0: 20 bits, R1.0: 16 bits)
                wreg(c.tx_pll_frac_n_addr, 0x6666, ln) # NRZ-10G mode, 0x0D8 [15:0] TX PLL FRAC_N[15: 0]=0x6666 = 0.4
            else:
                wreg([0x0d9,[3,0]],          0x0, ln) # NRZ-10G mode, 0x0D9  [3:0] TX PLL FRAC_N[19:16]=0x0    = 0.4 (R2.0: 20 bits, R1.0: 16 bits)
                wreg(c.tx_pll_frac_n_addr, 0x6666, ln) # NRZ-10G mode, 0x0D8 [15:0] TX PLL FRAC_N[15: 0]=0x6666 = 0.4
            
            wreg(c.rx_pll_frac_order_addr,  2, ln) # NRZ-10G mode, 0x1F0[15:14] RX PLL FRAC_ORDER =10
            
            wreg(c.rx_pll_lvcocap_addr,    85, ln) # NRZ-10G mode, 0x1F5 [15:9] RX PLL VCOCAP=84 or 85 for fvco=20.625G                
            wreg(c.rx_pll_n_addr,          52, ln) # NRZ-10G mode, 0x1FD [15:7] RX_PLL N= 52.8*1*195.3125
            wreg(c.rx_pll_div4_addr,        0, ln) # NRZ-10G mode, 0x1FF    [6] RX_PLL DIV4=0
            wreg(c.rx_pll_div2_addr,        1, ln) # NRZ-10G mode, 0x1F4    [8] RX PLL DIV2=1
            wreg(c.rx_pll_frac_n_addr, 0xCCCC, ln) # NRZ-10G mode, 0x1F1 [15:0] RX PLL FRAC_N=0xCCCC = 0.8
            wreg(c.tx_pll_frac_order_addr,  2, ln) # NRZ-10G mode, 0x0D7[15:14] TX PLL FRAC_ORDER =10

            wreg(c.rx_pll_pu_addr,          1, ln) # NRZ-10G mode,Power up RX PLL after prgramming PLL and before toggling FRAC_EN
            wreg(c.tx_pll_pu_addr,          1, ln) # NRZ-10G mode,Power up TX PLL after prgramming PLL and before toggling FRAC_EN

            #### Enable Fractional PLLs after programming PLLs, # UPDATE 20180521
            wreg(c.tx_pll_frac_en_addr,     1, ln) # NRZ-10G mode, 0x0D7   [13] TX PLL FRAC_EN=1 after programming PLLs
            wreg(c.rx_pll_frac_en_addr,     1, ln) # NRZ-10G mode, 0x1F0   [13] RX PLL FRAC_EN=1 after programming PLLs               
                                                        
            wreg(c.tx_mode10g_en_addr,      1, ln) # NRZ-10G mode, TX_NRZ_10G_EN=1 (or Enable NRZ Half-Rate Mode)
            wreg(c.rx_mode10g_addr,         1, ln) # NRZ-10G mode, RX_NRZ_10G_EN=1 (or Enable NRZ Half-Rate Mode)
                                                        
            wreg(c.rx_delta_adapt_en_addr,  0, ln) # NRZ-10G mode, Disable Delta Adaptation loop(0x0101=0x0201)
            wreg(0x0165,0x0001, ln)                # NRZ-10G Mode, INTP Half Rate mode [0] = 1
            wreg(0x0175,0xC6AF, ln)                # NRZ-10G Mode, Margin Counter Phases 4321 en [7:4]=1010
            wreg(0x0100,0x4010, ln)                # NRZ-10G mode, F1F2_INIT_0, 0x5185
            wreg(0x0107,0x4013, ln)                # NRZ-10G mode, F1F2_INIT_1, 0x5185
            wreg(0x0183,0x4013, ln)                # NRZ-10G mode, F1F2_INIT_2, 0x5185
            wreg(0x0184,0x8007, ln)                # NRZ-10G mode, F1F2_INIT_3, 0x9185
            wreg(0x0185,0x4013, ln)                # NRZ-10G mode, F1F2_INIT_4, 0x5185
            wreg(0x0186,0xC014, ln)                # NRZ-10G mode, F1F2_INIT_5, 0xD185
            wreg(0x0187,0xC012, ln)                # NRZ-10G mode, F1F2_INIT_6, 0x5185
            wreg(0x0188,0x4006, ln)                # NRZ-10G mode, F1F2_INIT_7, 0x5185
            
        elif datarate < 24.0: # NRZ 20.6250G, , NRZ Full-Rate Mode
            #### disable Fractional PLLs while programming PLLs,# UPDATE 20180521
            wreg(c.tx_pll_frac_en_addr,     0, ln) # NRZ-20G mode, 0x0D7   [13] TX PLL FRAC_EN=0 while programming PLLs
            wreg(c.rx_pll_frac_en_addr,     0, ln) # NRZ-20G mode, 0x1F0   [13] RX PLL FRAC_EN=0 while programming PLLs
        
            wreg(c.tx_pll_lvcocap_addr,    85, ln) # NRZ-20G mode, 0x0DB [14:8] TX PLL VCOCAP=84 or 85 for fvco=20.625G                
            wreg(c.tx_pll_n_addr,          26, ln) # NRZ-20G mode, 0x0FE [15:7] TX_PLL N= 26.4*2*195.3125
            wreg(c.tx_pll_div4_addr,        0, ln) # NRZ-20G mode, 0x0FF    [0] TX_PLL DIV4=0
            wreg(c.tx_pll_div2_addr,        0, ln) # NRZ-20G mode, 0x0FF    [1] TX PLL DIV2=0
            if chip_rev()==2.0:
                wreg([0x0d9,[3,0]],          0x6, ln) # NRZ-10G mode, 0x0D9  [3:0] TX PLL FRAC_N[19:16]=0x6    = 0.4 (R2.0: 20 bits, R1.0: 16 bits)
                wreg(c.tx_pll_frac_n_addr, 0x6666, ln) # NRZ-10G mode, 0x0D8 [15:0] TX PLL FRAC_N[15: 0]=0x6666 = 0.4
            else:
                wreg([0x0d9,[3,0]],          0x0, ln) # NRZ-10G mode, 0x0D9  [3:0] TX PLL FRAC_N[19:16]=0x0    = 0.4 (R2.0: 20 bits, R1.0: 16 bits)
                wreg(c.tx_pll_frac_n_addr, 0x6666, ln) # NRZ-10G mode, 0x0D8 [15:0] TX PLL FRAC_N[15: 0]=0x6666 = 0.4
            wreg(c.tx_pll_frac_order_addr,  2, ln) # NRZ-20G mode, 0x0D7[15:14] TX PLL FRAC_ORDER =10
                                                        
            wreg(c.rx_pll_lvcocap_addr,    85, ln) # NRZ-20G mode, 0x1F5 [15:9] RX PLL VCOCAP=84 or 85 for fvco=20.625G                
            wreg(c.rx_pll_n_addr,          52, ln) # NRZ-20G mode, 0x1FD [15:7] RX_PLL N= 52.8*1*195.3125
            wreg(c.rx_pll_div4_addr,        0, ln) # NRZ-20G mode, 0x1FF    [6] RX_PLL DIV4=0
            wreg(c.rx_pll_div2_addr,        1, ln) # NRZ-20G mode, 0x1F4    [8] RX PLL DIV2=1
            wreg(c.rx_pll_frac_n_addr, 0xCCCC, ln) # NRZ-20G mode, 0x1F1 [15:0] RX PLL FRAC_N=0xCCCC = 0.8
            wreg(c.rx_pll_frac_order_addr,  2, ln) # NRZ-20G mode, 0x1F0[15:14] RX PLL FRAC_ORDER =10
            
            wreg(c.rx_pll_pu_addr,          1, ln) # NRZ-20G mode,Power up RX PLL after prgramming PLL and before toggling FRAC_EN
            wreg(c.tx_pll_pu_addr,          1, ln) # NRZ-20G mode,Power up TX PLL after prgramming PLL and before toggling FRAC_EN

            #### Enable Fractional PLLs after programming PLLs,# UPDATE 20180521
            wreg(c.tx_pll_frac_en_addr,     1, ln) # NRZ-20G mode, 0x0D7   [13] TX PLL FRAC_EN=1 after programming PLLs
            wreg(c.rx_pll_frac_en_addr,     1, ln) # NRZ-20G mode, 0x1F0   [13] RX PLL FRAC_EN=1 after programming PLLs   
            
            wreg(c.tx_mode10g_en_addr,      0, ln) # NRZ-20G mode, TX_NRZ_10G_EN=0 (or Disable NRZ Half-Rate Mode)
            wreg(c.rx_mode10g_addr,         0, ln) # NRZ-20G mode, RX_NRZ_10G_EN=0 (or Disable NRZ Half-Rate Mode)
            wreg(0x165,0x0000,ln)                  # NRZ-FullRate Mode, INTP Half Rate mode [0] = 0
            wreg(0x175,0xC6FF,ln)                  # NRZ-FullRate Mode, Margin Counter Phases 4321 en [7:4]=1111
            wreg(0x100,0x4900,ln)                  # NRZ-FullRate mode, F1F2_INIT_0, 0x5185
            wreg(0x107,0x44FB,ln)                  # NRZ-FullRate mode, F1F2_INIT_1, 0x5185
            wreg(0x183,0x4B08,ln)                  # NRZ-FullRate mode, F1F2_INIT_2, 0x5185
            wreg(0x184,0x82FA,ln)                  # NRZ-FullRate mode, F1F2_INIT_3, 0x9185
            wreg(0x185,0x4B82,ln)                  # NRZ-FullRate mode, F1F2_INIT_4, 0x5185
            wreg(0x186,0xCB88,ln)                  # NRZ-FullRate mode, F1F2_INIT_5, 0xD185
            wreg(0x187,0xC97A,ln)                  # NRZ-FullRate mode, F1F2_INIT_6, 0x5185
            wreg(0x188,0x417a,ln)                  # NRZ-FullRate mode, F1F2_INIT_7, 0x5185

        else: # NRZ 25.78125G, , NRZ Full-Rate Mode
            #### disable Fractional PLLs while programming PLLs,# UPDATE 20180521
            wreg(c.tx_pll_frac_en_addr,     0, ln) # NRZ-25G mode, 0x0D7   [13] TX PLL FRAC_EN=0 while programming PLLs
            wreg(c.rx_pll_frac_en_addr,     0, ln) # NRZ-25G mode, 0x1F0   [13] RX PLL FRAC_EN=0 while programming PLLs
        
            wreg(c.tx_pll_lvcocap_addr,    34, ln) # NRZ-25G mode, 0x0DB [14:8] TX PLL VCOCAP=33 or 34 for fvco=25.78125G                
            wreg(c.tx_pll_n_addr,          33, ln) # NRZ-25G mode, 0x0FE [15:7] TX_PLL N= 33*2*195.3125
            wreg(c.tx_pll_div4_addr,        0, ln) # NRZ-25G mode, 0x0FF    [0] TX_PLL DIV4=0
            wreg(c.tx_pll_div2_addr,        0, ln) # NRZ-25G mode, 0x0FF    [1] TX PLL DIV2=0
            wreg([0x0d9,[3,0]],             0, ln) # NRZ-25G mode, 0x0D9  [3:0] TX PLL FRAC_N[19:16]=0 (R2.0: 20 bits, R1.0: 16 bits)
            wreg(c.tx_pll_frac_n_addr,      0, ln) # NRZ-25G mode, 0x0D8 [15:0] TX PLL FRAC_N=0
            wreg(c.tx_pll_frac_order_addr,  0, ln) # NRZ-25G mode, 0x0D7[15:14] TX PLL FRAC_ORDER =0
                                                       
            wreg(c.rx_pll_lvcocap_addr,    34, ln) # NRZ-25G mode, 0x1F5 [15:9] RX PLL VCOCAP=33 or 34 for fvco=25.78125G                
            wreg(c.rx_pll_n_addr,          66, ln) # NRZ-25G mode, 0x1FD [15:7] RX_PLL N= 66*1*195.3125
            wreg(c.rx_pll_div4_addr,        0, ln) # NRZ-25G mode, 0x1FF    [6] RX_PLL DIV4=0
            wreg(c.rx_pll_div2_addr,        1, ln) # NRZ-25G mode, 0x1F4    [8] RX PLL DIV2=0
            wreg(c.rx_pll_frac_n_addr,      0, ln) # NRZ-25G mode, 0x1F1 [15:0] RX PLL FRAC_N=0
            wreg(c.rx_pll_frac_order_addr,  0, ln) # NRZ-25G mode, 0x1F0[15:14] RX PLL FRAC_ORDER =0
                                                       
            wreg(c.rx_pll_pu_addr,          1, ln) # NRZ-25G mode,Power up RX PLL after prgramming PLL and before toggling FRAC_EN
            wreg(c.tx_pll_pu_addr,          1, ln) # NRZ-25G mode,Power up TX PLL after prgramming PLL and before toggling FRAC_EN

            #### Enable Fractional PLLs after programming PLLs, if needed   # UPDATE 20180521
            wreg(c.tx_pll_frac_en_addr,     0, ln) # NRZ-25G mode, 0x0D7   [13] TX PLL FRAC_EN=0 kept off for 25G 
            wreg(c.rx_pll_frac_en_addr,     0, ln) # NRZ-25G mode, 0x1F0   [13] RX PLL FRAC_EN=0 kept off for 25G   

            wreg(c.tx_mode10g_en_addr,      0, ln) # NRZ-25G mode, TX_NRZ_10G_EN=0 (or Disable NRZ Half-Rate Mode)
            wreg(c.rx_mode10g_addr,         0, ln) # NRZ-25G mode, RX_NRZ_10G_EN=0 (or Disable NRZ Half-Rate Mode)
            wreg(0x165,0x0000,ln)                  # NRZ-FullRate Mode, INTP Half Rate mode [0] = 0
            wreg(0x175,0xC6FF,ln)                  # NRZ-FullRate Mode, Margin Counter Phases 4321 en [7:4]=1111
            wreg(0x100,0x4900,ln)                  # NRZ-FullRate mode, F1F2_INIT_0, 0x5185
            wreg(0x107,0x44FB,ln)                  # NRZ-FullRate mode, F1F2_INIT_1, 0x5185
            wreg(0x183,0x4B08,ln)                  # NRZ-FullRate mode, F1F2_INIT_2, 0x5185
            wreg(0x184,0x82FA,ln)                  # NRZ-FullRate mode, F1F2_INIT_3, 0x9185
            wreg(0x185,0x4B82,ln)                  # NRZ-FullRate mode, F1F2_INIT_4, 0x5185
            wreg(0x186,0xCB88,ln)                  # NRZ-FullRate mode, F1F2_INIT_5, 0xD185
            wreg(0x187,0xC97A,ln)                  # NRZ-FullRate mode, F1F2_INIT_6, 0x5185
            wreg(0x188,0x417a,ln)                  # NRZ-FullRate mode, F1F2_INIT_7, 0x5185
        '''
        wreg(0x165,0x0000,ln)                  # NRZ-FullRate Mode, INTP Half Rate mode [0] = 0
        wreg(0x175,0xC6FF,ln)                  # NRZ-FullRate Mode, Margin Counter Phases 4321 en [7:4]=1111
        wreg(0x100,0x4900,ln)                  # NRZ-FullRate mode, F1F2_INIT_0, 0x5185
        wreg(0x107,0x44FB,ln)                  # NRZ-FullRate mode, F1F2_INIT_1, 0x5185
        wreg(0x183,0x4B08,ln)                  # NRZ-FullRate mode, F1F2_INIT_2, 0x5185
        wreg(0x184,0x82FA,ln)                  # NRZ-FullRate mode, F1F2_INIT_3, 0x9185
        wreg(0x185,0x4B82,ln)                  # NRZ-FullRate mode, F1F2_INIT_4, 0x5185
        wreg(0x186,0xCB88,ln)                  # NRZ-FullRate mode, F1F2_INIT_5, 0xD185
        wreg(0x187,0xC97A,ln)                  # NRZ-FullRate mode, F1F2_INIT_6, 0x5185
        wreg(0x188,0x417a,ln)                  # NRZ-FullRate mode, F1F2_INIT_7, 0x5185
        '''
        
        if ln%2: # ODD lanes
            set_bandgap(bg_val=7,lane=ln)
        else:      # Even lanes
            set_bandgap(bg_val=2,lane=ln)
        
        ####################### NRZ mode, ln Reset                   
        if (input_mode=='dc'):
            wreg(c.rx_vcomrefsel_ex_addr,1,ln) # DC-Coupled mode 0x1DD[9]=1
            wreg(c.rx_en_vcominbuf_addr ,1,ln) # DC-Coupled mode 0x1E7[11]=1
        else:
            wreg(c.rx_vcomrefsel_ex_addr,0,ln) # AC-Coupled mode 0x1DD[9]=0
            wreg(c.rx_en_vcominbuf_addr ,1,ln) # AC-Coupled mode 0x1E7[11]=1
        
        # ---------------- LANE Q NRZ init end
        
        # Set the polarities of the lanes
        pol(TxPolarityMap[gSlice][ln],RxPolarityMap[gSlice][ln],ln,0)

        #print("\ninit_lane_nrz(): Lane %s, Target DataRate=%3.5fG, Actual DataRate=%3.5f G"%(lane_name_list[ln],datarate,get_lane_pll(ln)[ln][0][0])),
        
    get_lane_mode(lanes) # update the Encoding modes of all lanes for this Slice        
    #for ln in lanes:
    #    print("\n init_lane_nrz(): Lane %s, Target DataRate=%3.5fG, Actual DataRate=%3.5f G"%(lane_name_list[ln],datarate,get_lane_pll(ln)[ln][0][0])),

    if not fw_loaded(print_en=0):
        for ln in lanes:
            lr(lane=ln)  

        
        #rx_monitor_clear(ln)
####################################################################################################
# 
# Initialize the target lane, or list of lanes, to PAM4 mode
####################################################################################################
def init_lane_pam4(datarate=None,input_mode='ac',lane=None):

    lanes = get_lane_list(lane)    
    global gEncodingMode
    c=Pam4Reg
    ### First put the target lanes' State Machine in reset mode while programming lane registers and PLLs
    for ln in lanes:
        wreg(c.rx_lane_rst_addr, 1, ln) # Keep lane in Reset while programming lane
        wreg(c.rx_pll_pu_addr,   0, ln) # Power down RX PLL while prgramming PLL
        wreg(c.tx_pll_pu_addr,   0, ln) # Power down RX PLL while prgramming PLL
    
    for ln in lanes:
        gEncodingMode[gSlice][ln] = ['pam4',53.125]
        lane_mode_list[ln] = 'pam4'
        ####disable AN/LT registers as default
        wreg(0x3000+(0x100*ln),0x0000,lane=0)
        wreg(0xe000+(0x100*ln),0xc000,lane=0)
        # ---------------- LANE Q PAM4 init begin
        ####################### Per-Lane PLLs ###
        wreg(0x1F0,0x0000,ln) # PAM4 mode, RX_PLL, Fractional PLL off  
        wreg(0x1F1,0x0000,ln) # PAM4 mode, RX_PLL, Fractional PLL off          
        wreg(0x0D7,0x0000,ln) # PAM4 mode, TX_PLL, Fractional PLL off  
        wreg(0x0D8,0x0000,ln) # PAM4 mode, TX_PLL, Fractional PLL off  

       #wreg(0x0FF,0x5df6,ln) # PAM4 mode, TX_BG bandgap | div4=0
       #wreg(0x0FE,0x223e,ln) # PAM4 mode, TX PLLN = 68*195.3125*4*2 | [3:1] //vcoi=0x7
        wreg(0x0FF,0x5df4,ln) # PAM4 mode, TX_BG bandgap | div4=0
        wreg(0x0FE,0x113e,ln) # PAM4 mode, TX PLLN = 34*195.3125*4 | [3:1] //vcoi=0x7
       #wreg(0x0FD,0x5636,ln) # PAM4 mode, TX PLL, bypass_pll=0
       #wreg(0x0FD,0x5436,ln) # PAM4 mode, TX PLL, bypass_pll=0, clear bypass 1p7 regulator 0xFD[9]
        wreg(0x0FC,0x7236,ln) # PAM4 mode, TX PLL, vvco_reg
        wreg(0x0FB,0x7fb6,ln) # PAM4 mode, TX PLL, vref_intp
        wreg(0x0FA,0x8010,ln) # PAM4 mode, TX PLL, testmode_tx_pll
        wreg(0x0DB,0x1e00,ln) # PAM4 mode, TX PLL, [14:8] // tx_vcocap=rx_vcocap=29 or 30 for fvco=26.5625G
        wreg(0x0DA,0x7de6,ln) # PAM4 mode, TX PLL, [14:10]// bm_vco=0x1f
        wreg(0x0D9,0xb760,ln)
                  
        wreg(0x1FF,0x5db1,ln) # PAM4 mode, RX_BG bandgap | [15:13]//vbg=0
       #wreg(0x1FC,0x1048,ln) # PAM4 mode, RX PLL, clear bypass 1p7 regulator 0x1FC[9]
       #wreg(0x1FD,0x213e,ln) # PAM4 mode, RX_PLLN= 33*2*195.3125 |  [3:1]  //vcoi=0x7
        wreg(0x1F5,0x3d40,ln) # PAM4 mode, RX PLL, lcvco_cap [15:9] //tx_vcocap=rx_vcocap=29 or 30 for fvco=26.5625G
        wreg(0x1FD,0x223e,ln) # PAM4 mode, RX_PLLN= 34*195.3125*4*2 [3:1]  //vcoi=0x7
       #wreg(0x1F5,0x3c40,ln) # PAM4 mode, RX PLL, lcvco_cap [15:9] //tx_vcocap=rx_vcocap=29 or 30 for fvco=26.5625G
       #wreg(0x1FD,0x113e,ln) # PAM4 mode, RX_PLLN= 34*195.3125*4*2 [3:1]  //vcoi=0x7
        wreg(0x1F4,0x7de6,ln) # PAM4 mode, RX PLL, en_div2=0 | [14:10]//bm_vco=0x1f
        wreg(0x1F3,0xb760,ln) # PAM4 mode, RX PLL, pu_intp_rx
        

        #### From Xiaofan 20171204
        wreg(0x0FD,0x1436,ln) # PAM4 mode, bypass_pll=1, bypass1p7reg=0
        wreg(0x0FB,0x6fb6,ln) # PAM4 mode, vref_intp, vref1p3vcodiv=3
        wreg(0x1FC,0x1448,ln) # PAM4 mode, bypass_pll=1, bypass1p7reg=0

        if ln%2: # ODD lanes
            set_bandgap(bg_val=7,lane=ln)
        else:      # Even lanes
            set_bandgap(bg_val=2,lane=ln)

        ####################### analog ###
        wreg(0x0f1,0x0007,ln) # PAM4 mode, pu_adc|pu_clkcomp|pu_clkreg
        wreg(0x0ef,0x9230,ln) # PAM4 mode, vrefadc
        wreg(0x0ee,0x691c,ln) # PAM4 mode, calnbs_top|bot
        wreg(0x0ed,0x8888,ln) # PAM4 mode, edge
        wreg(0x0eb,0x67fc,ln) # PAM4 mode, TX Swing
                  
        wreg(0x1f9,0x7e86,ln) # PAM4 mode, vrefagcdegen (was 0x7686)
        wreg(0x1f6,0x71b0,ln) # PAM4 mode, vagcbufdac
        wreg(0x1e7,0xc34c,ln) # PAM4 mode, pu_agc|pu_agcdl | en_vcominbuf=0
        
       
        wreg(0x1e6,0x88a5,ln) # PAM4 mode, bypass_agcreg
        wreg(0x1e5,0x6c38,ln) # PAM4 mode, 4e3c # PAM4 mode, vrefagc1p5reg
                  
        wreg(0x1da,0x6f00,ln) # PAM4 mode, 6d80 # clkphase0
                  
        wreg(0x1d4,0x01c0,ln) # PAM4 mode, agcgain1 | agacgain2
        wreg(0x1d5,0x0100,ln) # PAM4 mode, blwc_en
        wreg(0x1d6,0xcc00,ln) # PAM4 mode, en_az_short_pulse #rajan

        #######################  PAM4 Mode Configuration
        wreg(0x041,0x83df,ln) # PAM4 mode, up/dn mode OFF | pam4_en=1
        wreg(0x0b0,0x4000,lane=gLanePartnerMap[gSlice][ln][1]) # PAM4 mode, (4000)
        wreg(0x0b0,0x4802,lane=gLanePartnerMap[gSlice][ln][1]) # PAM4 mode, (4000)
        wreg(0x0b0,0x4000,lane=gLanePartnerMap[gSlice][ln][1]) # PAM4 mode, (4000)
        wreg(0x1e7,0xc36c,ln) # PAM4 mode, ffe enabled in pam4 mode (agc_dl=1)

        #######################  PAM4 SM Configuration
        wreg(0x000,0x286b,ln)
        wreg(0x001,0xc000,ln) # changed from 0x8000 to 0xc000 per Yifei 20171203
        wreg(0x002,0x4000,ln)
        wreg(0x003,0x7873,ln)
        wreg(0x005,0xbd2a,ln)
        wreg(0x006,0x762b,ln)
        wreg(0x007,0x3ac2,ln)
        wreg(0x008,0xc001,ln)
        wreg(0x00a,0xe5b1,ln)
        wreg(0x00b,0x3d15,ln)
        wreg(0x00c,0x0080,ln)
        wreg(0x044,0x1035,ln)
        wreg(0x04b,0xe802,ln)
        wreg(0x079,0x00a4,ln)  #--- Turn On TED Qualifier 4/24/2018
        wreg(0x07b,0x4004,ln) #---- Set BLWC MU=4 4/24/2018
        wreg(0x087,0x0800,ln)
        
        # Updated rajan - need to verify
        wreg(0x005 ,0xbd29, ln)
        wreg(0x007 ,0x32bf, ln) 
        wreg(0x009 ,0x7665, ln)   
        
        #######################  PAM4 Rx optimize parameters
        sel_ctle_map(IL='ALL', lane=ln)
        ctle(7,ln)
       #wreg(0x048,0x2518,ln) # PAM4 mode, ctle_map
       #wreg(0x049,0x79eb,ln) # PAM4 mode, ctle_map
       #wreg(0x04a,0xbf3f,ln) # PAM4 mode, ctle_map, CTLE7=(7,7)
       #wreg(0x021,0x00f0,ln) # PAM4 mode, CTLE=7      
        wreg(0x020,0x03c0,ln) # PAM4 mode, timing loop|Kp=7 | Kf Set Kp=7 to compensate TED Qualifier ON 4/24/2018
        
        wreg(0x1d5,0x0100,ln) # PAM4 mode, bit8=1, see 0x007[14], baseline_ow en
        wreg(0x1e0,0xfc40,ln) # PAM4 mode, ffe_pol | pu_sum
        wreg(0x1e1,0x1000,ln) # PAM4 mode, ffe_gain12|ffe_sum4
        wreg(0x1e2,0x1601,ln) # PAM4 mode, ffe_k1 | ffe_sum3
        wreg(0x1e3,0x0101,ln) # PAM4 mode, ffe_k2 | ffe_sum2
        wreg(0x1e4,0x0101,ln) # PAM4 mode, ffe_s2| ffe_sum1      
        wreg(0x1df,0x6666,ln) # PAM4 mode, ffe1234_delay
        wreg(0x1de,0x77cc,ln) # PAM4 mode, ffe5678_delay
        
        ####################### Direct Connect (XSR)
        wreg(0x1dd,0xc1c2,ln) # PAM4 mode, ffe9_delay | en_skef = 0 | skef_value
        wreg(0x1d4,0x0260,ln) # PAM4 mode, 0d00 # PAM4 mode, agcgain1[15:9] (bin=4) / agcgain2 [8:4] (bin=31)
        wreg(0x004,0xb029,ln) # PAM4 mode, b029 f1_over_init
        wreg(0x012,0x2500,ln) # PAM4 mode, 3f80 Delta  
        wreg(0x0ed,0x7777,ln) # PAM4 mode, 7777 Edge
        
        ######################## Super Cal
        #wreg(0x004,0xb029,ln) # bit 0 =1
        wreg(0x077,0x4e5c,ln)
        wreg(0x078,0xe080,ln)
        wreg(0x009,0x8666,ln)
        wreg(0x087,0x0e00,ln) # super-cal enable | 0800 disable
        
        ####################### PAM4 mode, TX setting
        if gPam4TxSourceIsCredo:
            wreg(0x0a5,0x0200,lane=gLanePartnerMap[gSlice][ln][1]) # PAM4 mode, pre2
            wreg(0x0a7,0xf800,lane=gLanePartnerMap[gSlice][ln][1]) # PAM4 mode, pre1
            wreg(0x0a9,0x1100,lane=gLanePartnerMap[gSlice][ln][1]) # PAM4 mode, main
            wreg(0x0ab,0x0000,lane=gLanePartnerMap[gSlice][ln][1]) # PAM4 mode, post1
            wreg(0x0ad,0x0000,lane=gLanePartnerMap[gSlice][ln][1]) # PAM4 mode, post2
            wreg(0x0af,0xfa08,lane=gLanePartnerMap[gSlice][ln][1]) # PAM4 mode, tx_automode| MSB first|gc=en|Pre off|SHmode
            
        #######################  TX PRBS gen and Rx PRBS checker
        if gPrbsEn:
            wreg(0x0a0,0xe320,lane=gLanePartnerMap[gSlice][ln][1]) # PAM4 mode, PRBS clock en before Patt en
            wreg(0x0a0,0xeb20,lane=gLanePartnerMap[gSlice][ln][1]) # PAM4 mode, tx pam4 prbs31
            wreg(0x043,0x0cfa,ln) # PAM4 mode, Rx PAM4 prbs31 checker | MSB first
            wreg(0x042,0xb3fd,ln) # PAM4 mode, Rx PAM4 gc=en | Pre=dis
        else:
            wreg(0x0a0,0xe320,lane=gLanePartnerMap[gSlice][ln][1]) # PAM4 mode, PRBS clock en before Patt en
            wreg(0x0a0,0x8320,lane=gLanePartnerMap[gSlice][ln][1]) # PAM4 mode, TX pam4 FUNCTIONAL MODE
            wreg(0x043,0x0ce2,ln) # PAM4 mode, Rx PAM4 FUNCTIONAL MODE | MSB first
            wreg(0x042,0xb3fd,ln) # PAM4 mode, Rx PAM4 gc=en | Pre=dis
        
        ####################### PAM4 mode, ln Reset
        wreg(0x087,0x0800,ln) # super-cal disable        
        
        #if datarate!=None:
            #set_lane_pll(tgt_pll='both',datarate=datarate, div2=0, lane=ln)
        
        if chip_rev()==2.0:
            wreg([0x07b,[3]],  1, ln) # R2.0 PAM4 BLWC Pol = 1
            wreg([0x17c,[15]], 0, ln) # R2.0 NRZ BLWC en = 0 (disabled)
            
        # PAM4 50G: PLL = 53.125Gbps        
        #### disable Fractional PLLs while programming PLLs
        wreg(c.tx_pll_frac_en_addr,     0, ln) # PAM4-53G mode, 0x0D7   [13] TX PLL FRAC_EN=0
        wreg(c.rx_pll_frac_en_addr,     0, ln) # PAM4-53G mode, 0x1F0   [13] RX PLL FRAC_EN=0

        wreg(c.tx_pll_lvcocap_addr,    30, ln) # PAM4-53G mode, 0x0DB [14:8] TX PLL VCOCAP=30 for fvco=26.5625G                
        wreg(c.tx_pll_n_addr,          34, ln) # PAM4-53G mode, 0x0FE [15:7] TX_PLL N= 34*2*195.3125
        wreg(c.tx_pll_div4_addr,        0, ln) # PAM4-53G mode, 0x0FF    [0] TX_PLL DIV4=0
        wreg(c.tx_pll_div2_addr,        0, ln) # PAM4-53G mode, 0x0FF    [1] TX PLL DIV2=0
        wreg(c.tx_pll_frac_n_addr,      0, ln) # PAM4-53G mode, 0x0D8 [15:0] TX PLL FRAC_N=0
        wreg(c.tx_pll_frac_order_addr,  0, ln) # PAM4-53G mode, 0x0D7[15:14] TX PLL FRAC_ORDER =10

        wreg(c.rx_pll_lvcocap_addr,    30, ln) # PAM4-53G mode, 0x1F5 [15:9] RX PLL VCOCAP=30 for fvco=26.5625G                
        wreg(c.rx_pll_n_addr,          68, ln) # PAM4-53G mode, 0x1FD [15:7] RX_PLL N= 68*1*195.3125
        wreg(c.rx_pll_div4_addr,        0, ln) # PAM4-53G mode, 0x1FF    [6] RX_PLL DIV4=0
        wreg(c.rx_pll_div2_addr,        1, ln) # PAM4-53G mode, 0x1F4    [8] RX PLL DIV2=0
        wreg(c.rx_pll_frac_n_addr,      0, ln) # PAM4-53G mode, 0x1F1 [15:0] RX PLL FRAC_N=0
        wreg(c.rx_pll_frac_order_addr,  0, ln) # PAM4-53G mode, 0x1F0[15:14] RX PLL FRAC_ORDER =10
        
        wreg(c.rx_pll_pu_addr,          1, ln) # PAM4-53G mode,Power up RX PLL after prgramming PLL and before toggling FRAC_EN
        wreg(c.tx_pll_pu_addr,          1, ln) # PAM4-53G mode,Power up TX PLL after prgramming PLL and before toggling FRAC_EN

        #### Enable Fractional PLLs after programming PLLs, if needed
        wreg(c.tx_pll_frac_en_addr,     0, ln) # PAM4-53G mode, 0x0D7   [13] TX PLL FRAC_EN=0 kept off for 50G 
        wreg(c.rx_pll_frac_en_addr,     0, ln) # PAM4-53G mode, 0x1F0   [13] RX PLL FRAC_EN=0 kept off for 50G                

        if (input_mode=='dc'):
            wreg(c.rx_vcomrefsel_ex_addr,1,ln) # DC-Coupled mode 0x1DD[9]=1
            wreg(c.rx_en_vcominbuf_addr ,1,ln) # DC-Coupled mode 0x1E7[11]=1
        else:
            wreg(c.rx_vcomrefsel_ex_addr,0,ln) # AC-Coupled mode 0x1DD[9]=0
            wreg(c.rx_en_vcominbuf_addr ,1,ln) # AC-Coupled mode 0x1E7[11]=1
        
        # Set the polarities of the lanes
        pol(TxPolarityMap[gSlice][ln],RxPolarityMap[gSlice][ln],ln,0)
        # ---------------- LANE Q PAM4 init end
        
    get_lane_mode(lanes) # update the Encoding modes of all lanes for this Slice
    if not fw_loaded(print_en=0):
        for ln in lanes:
            lr(lane=ln)   
#################################################################################################### 
def lane_power(lane=None, slice=[0,1], rx_off=1, tx_off=1, rx_bg_off=1, tx_bg_off=1):

    tx_bg_val=7
    rx_bg_val=7
    
    slices = get_slice_list(slice)
    lanes = get_lane_list(lane)
    get_lane_mode(lanes)
    
    for slc in slices:
        sel_slice(slc)
        for ln in lanes:
            ### First step: Band Gap Power UP
            if rx_bg_off == 0:
                wregBits(0x1FF, [12], 1, ln)
            if tx_bg_off == 0:
                wregBits(0x0FF, [12], 1, ln)
                
            ### RX Power DOWN
            if rx_off == 1: 
                wregBits(0x181, [11],      1, ln) # NRZ state machine reset
                wregBits(0x000, [15],      1, ln) # PAM4 state machine reset
                wregBits(0x1FF, [11],      0, ln)
                wregBits(0x1FF, [15, 13],  0, ln) # RX Bandgap setting to 0
                wregBits(0x1FF, [7],       0, ln)          
                wregBits(0x1FF, [5],       0, ln)
                wregBits(0x1FE, [15, 14],  0, ln)
                wregBits(0x1FD, [0],       0, ln) # RX PLL down
                wregBits(0x1FC, [6, 1],    0, ln)
                wregBits(0x1F8, [15, 13],  0, ln)
                wregBits(0x1F8, [9],       0, ln)
                wregBits(0x1F3, [5],       0, ln)
                wregBits(0x1E7, [15, 14],  0, ln)
                wregBits(0x1E0, [15, 10],  0, ln) # PU FFE DEGEN
                wregBits(0x1DD, [1],       0, ln) # PU_INTP
                wregBits(0x0F1, [2, 0],    0, ln) # PU_ADC,CLKCOMP,CLKCOMPREG
                wregBits(0x091, [8],       0, ln) # XCORR OFF
                wregBits(0x1C1, [1,0],     0, ln) # PU_FEC_ANA
                wreg(0x980c, 0xffff,  lane=0)     # disable low freq Bitmux fifo
                wreg(0x980d, 0xffff,  lane=0)     # disable low freq Bitmux fifo
                wreg(0x3000+(0x100*ln),0x0000,lane=0) # Disable LT
                if   ln<4: wreg(0xF002+(0x010*ln    ),0x0000,lane=0) # Disable AN lanes 0 to 3
                elif ln<8: wreg(0xF102+(0x010*(ln-4)),0x0000,lane=0) # Disable AN lanes 4 to 7

            ### RX Power UP
            else: 
                wregBits(0x1FF, [15, 13], rx_bg_val,ln)     # RX Bandgap setting to 7
                wregBits(0x1FF, [11],      1, ln)
                wregBits(0x1FF, [7],       1, ln)          
                wregBits(0x1FF, [5],       1, ln)
                wregBits(0x1FE, [15],      1, ln)
                wregBits(0x1FD, [0],       1, ln) # RX PLL UP
                wregBits(0x1FC, [6, 1], 0x24, ln)
                wregBits(0x1F8, [15,13],   7, ln)
                wregBits(0x1F8, [9],       1, ln)
                wregBits(0x1F3, [5],       1, ln)
                wregBits(0x1E7, [15],      1, ln)
                if gEncodingMode[ln][0].upper() == 'PAM4': 
                    wregBits(0x1FE, [14],  1, ln)    # PAM4: FFE MA PU
                    wregBits(0x1E7, [14],  1, ln)    # PAM4: FFE on
                    wregBits(0x1E0, [15,10],0x3F,ln) # PAM4: PU FFE DEGEN
                    wregBits(0x091, [8],   1, ln)    # XCORR ON
                else:                                       
                    wregBits(0x1FE, [14],  0, ln)    # NRZ: FFE MA PU
                    wregBits(0x1E7,[14],   0, ln)    # NRZ: FFE off
                    wregBits(0x1E0,[15,10],0, ln)    # NRZ: PU FFE DEGEN
                    wregBits(0x091, [8],   0, ln)    # XCORR OFF
                wregBits(0x1DD, [1],       1, ln) # PU_INTP
                wregBits(0x0F1, [2,0],     7, ln) # PU_ADC,CLKCOMP,CLKCOMPREG
                wregBits(0x1C1, [1,0],     3, ln) # PU_FEC_ANA
                wreg(0x980c,0x0000,lane=0)        # enable low freq Bitmux fifo
                wreg(0x980d,0x0000,lane=0)        # enable low freq Bitmux fifo


            ### TX Power DOWN
            if tx_off == 1:         
                wregBits(0x0FF, [15, 13],  0, ln) # TX Bandgap 0
                wregBits(0x0FF, [11],      0, ln) # TX RVDD PU
                wregBits(0x0FF, [10],      0, ln) # TX RVDDVCO PU
                wregBits(0x0FF, [7],       0, ln) # TX RVDDLOOP PU
                wregBits(0x0FE, [0],       0, ln) # TX PLL PU
                wregBits(0x0FA, [15],      0, ln) # TX PU DRV MA
                wregBits(0x0EB, [13],      0, ln) # TX DRV PU
                wregBits(0x0EA, [6],       0, ln) # TX HIMODE PU 
                wregBits(0x0FD, [6, 1],    0, ln) # TX DRV-PLLPMP PU
            ### TX Power UP
            else:
                wregBits(0x0FF, [15, 13], tx_bg_val,ln) # TX Bandgap 7
                wregBits(0x0FF, [11],      1, ln) # TX RVDD PU
                wregBits(0x0FF, [10],      1, ln) # TX RVDDVCO PU
                wregBits(0x0FF, [7],       1, ln) # TX RVDDLOOP PU
                wregBits(0x0FE, [0],       1, ln) # TX PLL UP
                wregBits(0x0FA, [15],      1, ln) # TX PU DRV MA
                wregBits(0x0EB, [13],      1, ln) # TX DRV UP
                wregBits(0x0EA, [6],       0, ln) # TX HIMODE PU
                wregBits(0x0FD, [6,1],  0x1B, ln) # TX DRV-PLLPMP PU
            
            ### Last step: Band Gap Power DOWN
            if rx_bg_off == 1:
                wregBits(0x1FF, [12], 0, ln)
            if tx_bg_off == 1:
                wregBits(0x0FF, [12], 0, ln)
#################################################################################################### 
def analog_low_power():
    PRNT_EN=0
    regg(0xfe,0x1167,lane=0,print_en=PRNT_EN) # PAM4 TX PLL LVCOI = 7 -> 3
    regg(0xfe,0x1167,lane=1,print_en=PRNT_EN)
    regg(0xfe,0x1167,lane=2,print_en=PRNT_EN)   
    regg(0xfe,0x1167,lane=3,print_en=PRNT_EN)
    regg(0xfe,0x10b7,lane=8,print_en=PRNT_EN)
    regg(0xfe,0x10b7,lane=9,print_en=PRNT_EN)
    regg(0xfe,0x10b7,lane=10,print_en=PRNT_EN) # NRZ TX PLL LVCOI = 7 -> 3
    regg(0xfe,0x10b7,lane=11,print_en=PRNT_EN) 
    regg(0xfe,0x10b7,lane=12,print_en=PRNT_EN)
    regg(0xfe,0x10b7,lane=13,print_en=PRNT_EN)
    regg(0xfe,0x10b7,lane=14,print_en=PRNT_EN)
    regg(0xfe,0x10b7,lane=15,print_en=PRNT_EN)
    regg(0x0da,0x3de6,print_en=PRNT_EN)  # PAM4/NRZ TX PLL BM_VCOI = 7 -> 3
    regg(0x1f6,0x70b0,print_en=PRNT_EN) # RX analog bufdac, acomp, was 0xd1b0
    regg(0x0eb,0x6248,print_en=PRNT_EN)  # TX vdrv reg1/2/3 = 7/7/7 -> 2/2/2 was 0x67fc
    for slc in range(2):
        sel_slice(slc)
        for ln in range(16):
            wreg([0x043, [6,0]],0,ln) # Power Down PAM4 RX PRBS Checker
            wreg([0x161,  [10]],0,ln) # Power Down NRZ  RX PRBS Checker
            wreg([0x0A0,[15,8]],0,ln) # Power Down PAM4 TX PRBS Generator
            wreg([0x0B0,[15,2]],0,ln) # Power Down NRZ  TX PRBS Generator
    sel_slice(0)
#################################################################################################### 
def gearbox_power_save_mode(unused_lanes=[4,5,6,7]):
    PRNT_EN=0
    print("\n...Applied Gearbox Power Saving Features!")
    ################################ disable bandgap of unused lanes
    regg(0x0ff,0x0df4,lane=unused_lanes,print_en=PRNT_EN);
    regg(0x1ff,0x0db1,lane=unused_lanes,print_en=PRNT_EN)

    ################################# shut down analog setting of unused lanes
    regg(0x1ff ,0x4111,lane=unused_lanes,print_en=PRNT_EN)
    regg(0x1fe ,0x0000,lane=unused_lanes,print_en=PRNT_EN)
    regg(0x1fd ,0x223e,lane=unused_lanes,print_en=PRNT_EN)
    regg(0x1f8 ,0x0000,lane=unused_lanes,print_en=PRNT_EN)
    regg(0x1f3 ,0xb740,lane=unused_lanes,print_en=PRNT_EN)
    regg(0x1e7 ,0x036c,lane=unused_lanes,print_en=PRNT_EN)
    regg(0x1dd ,0x6160,lane=unused_lanes,print_en=PRNT_EN)
    regg(0x0ff ,0x4166,lane=unused_lanes,print_en=PRNT_EN)
    regg(0x0fe ,0x223e,lane=unused_lanes,print_en=PRNT_EN)
    regg(0x0f1 ,0x0000,lane=unused_lanes,print_en=PRNT_EN)
    regg(0x0eb ,0x436c,lane=unused_lanes,print_en=PRNT_EN)

    ########### disable xcorr-gate
    regg(0x91,lane=range(8,16)+unused_lanes,print_en=PRNT_EN)

    ############## disable BLWC
    regg(0x07b,0,print_en=PRNT_EN)
    regg(0x17c,0,print_en=PRNT_EN)

    ############## disable fec analyzer
    regg(0x1c1,0,print_en=PRNT_EN)

    ####################### disable linktraining module
    regg(0x3400,0,print_en=PRNT_EN)
    regg(0x3500,0,print_en=PRNT_EN)
    regg(0x3600,0,print_en=PRNT_EN)
    regg(0x3700,0,print_en=PRNT_EN)

    ########################## AN bypass
    regg(0xf002,0x8000,print_en=PRNT_EN)
    regg(0xf012,0x8000,print_en=PRNT_EN)
    regg(0xf022,0x8000,print_en=PRNT_EN)
    regg(0xf032,0x8000,print_en=PRNT_EN)
                      
    regg(0xf102,0x8000,print_en=PRNT_EN)
    regg(0xf112,0x8000,print_en=PRNT_EN)
    regg(0xf122,0x8000,print_en=PRNT_EN)
    regg(0xf132,0x8000,print_en=PRNT_EN)

    ########################## disable low freq fifo
    regg(0x980c,0xffff,print_en=PRNT_EN)
    regg(0x980d,0xffff,print_en=PRNT_EN)
    
    analog_low_power()
    
#################################################################################################### 
def gearbox_power_save_mode_new(unused_lanes=[2,3,6,7]):
    for slc in range(2):
        sel_slice(slc)
        ################################ disable bandgap of unused lanes
        #reg(0xff,0x0df4,lane=unused_lanes);reg(0x1ff,0x0db1,lane=unused_lanes)
        lane_power(lane=unused_lanes, slice=[0,1], rx_off=1, tx_off=1, rx_bg_off=1, tx_bg_off=1)

        ################################# shut down analog setting of unused lanes
        wreg(0x1ff ,0x4111,lane=unused_lanes)
        wreg(0x1fe ,0x0000,lane=unused_lanes)
        wreg(0x1fd ,0x223e,lane=unused_lanes)
        wreg(0x1f8 ,0x0000,lane=unused_lanes)
        wreg(0x1f3 ,0xb740,lane=unused_lanes)
        wreg(0x1e7 ,0x036c,lane=unused_lanes)
        wreg(0x1dd ,0x6160,lane=unused_lanes)
        wreg(0x0ff ,0x4166,lane=unused_lanes)
        wreg(0x0fe ,0x223e,lane=unused_lanes)
        wreg(0x0f1 ,0x0000,lane=unused_lanes)
        wreg(0x0eb ,0x436c,lane=unused_lanes)

        ########### disable xcorr-gate
        wreg(0x91,0,lane=range(8,16)+unused_lanes)

        ############## disable BLWC
        wreg(0x07b,0,lane=range(8,16)+unused_lanes); 
        wreg(0x17c,0,lane=range(8,16)+unused_lanes)

        ############## disable fec analyzer
        wreg(0x1c1,0,lane='all')

        ####################### disable linktraining module
        wreg(0x3400,0,lane='all')
        wreg(0x3500,0,lane='all')
        wreg(0x3600,0,lane='all')
        wreg(0x3700,0,lane='all')

        ########################## AN bypass
        wreg(0xf002,0x8000,lane='all')
        wreg(0xf012,0x8000,lane='all')
        wreg(0xf022,0x8000,lane='all')
        wreg(0xf032,0x8000,lane='all')

        wreg(0xf102,0x8000,lane='all')
        wreg(0xf112,0x8000,lane='all')
        wreg(0xf122,0x8000,lane='all')
        wreg(0xf132,0x8000,lane='all')

        ########################## disable low freq fifo
        wreg(0x980c,0xffff,lane='all')
        wreg(0x980d,0xffff,lane='all')
#################################################################################################### 
####################################################################################################
# Initialize lane to NRZ mode and then adapt RX
#
# 
#
####################################################################################################
def opt_lane_nrz(datarate=None, input_mode='ac',lane=None):
   
    global gLanePartnerMap  # TX Source for RX, one  per lane per Slice
    global gChanEst; # Channel Estimates, one set per lane per Slice  
    lanes = get_lane_list(lane)
    #get_lane_mode(lanes) # update the Encoding modes of all lanes for this Slice
    #c=NrzReg
    for ln in lanes:
        get_lane_mode(ln)
        
        if fw_loaded(print_en=0) == 1: 
            init_lane_for_fw(mode = 'nrz', rx_pol=RxPolarityMap[gSlice][ln],input_mode = input_mode,lane=ln)
            print ('init_lane_for_fw_nrz')
        else:init_lane_nrz(datarate, input_mode,ln) # NRZ mode
        line_encoding_mode = gEncodingMode[gSlice][ln][0]
        peer_encoding_mode = gEncodingMode[gSlice][gLanePartnerMap[gSlice][ln][1]][0]
        ctle_map(0,1,3, lane=ln)
        ctle_map(1,1,5, lane=ln)
        ctle_map(2,1,7, lane=ln)
        ctle_map(3,2,7, lane=ln)
        ctle_map(4,3,7, lane=ln)
        ctle_map(5,4,7, lane=ln)
        ctle_map(6,5,7, lane=ln)
        ctle_map(7,7,7, lane=ln)
        
        if gNrzTxSourceIsCredo:  # Make sure TxPeer Lane is also the same mode and data rate as this lane (NRZ or PAM4)        
            if line_encoding_mode != peer_encoding_mode :
                print ("\nSlice %d Lane %s is in %s mode. Its TX Peer (%s) is in %s Mode"%(gSlice, lane_name_list[ln],line_encoding_mode.upper(),  lane_name_list[gLanePartnerMap[gSlice][ln][1]],peer_encoding_mode.upper()))
                continue
            else:            
                tx_taps(0,-8,17,0,0,lane=gLanePartnerMap[gSlice][ln][1])
                time.sleep(1)

        of, hf, chan_est = channel_analyzer_nrz(lane=ln)
        gChanEst[gSlice][ln]=[chan_est,of,hf]
        if chan_est == 0.0: # LANE FAILED Channel Estimation
            print ("\nSlice %d Lane %s (NRZ) Channel Analyzer: ChanEst: %6.3f <<< FAILED"%(gSlice, lane_name_list[ln],chan_est)),               
        else:
            print ("\nSlice %d Lane %s (NRZ) Channel Analyzer: ChanEst : %6.3f, OF: %2d, HF: %2d"%(gSlice,lane_name_list[ln],chan_est, of, hf)),       
                    
        if chan_est == 0.0:       # LANE FAILED
            agcgain(1,1,lane=ln) # initial DC Gain
        elif chan_est < 1.20:             # DIRECT LOOPBACK  ############## USE SR TABLE #################
            ctle(7, lane=ln)
            agcgain(1,28,lane=ln) # initial DC Gain
            #agcgain(1,4,lane=ln) # initial DC Gain
        elif chan_est < 1.40:
            ctle(6, lane=ln)
            agcgain(5,31,lane=ln)
        elif chan_est < 1.55:
            ctle(5, lane=ln)
            agcgain(10,31,lane=ln)
        elif chan_est < 1.80:
            ctle(4, lane=ln)
            agcgain(15,31,lane=ln)
        elif chan_est < 2.09:
            ctle(3, lane=ln)
            agcgain(25,31,lane=ln)
        elif chan_est < 2.70:
            ctle(2, lane=ln)
            agcgain(40,31,lane=ln)
        else:
            ctle(1, lane=ln)
            agcgain(60,31,lane=ln)
    
        tx_taps_table(chan_est,ln) # select TX Taps based on channel estimate
        if chan_est != 0: # Only if Lane passed Channel Estimation, finish the rest of adaptation
            lr(lane=ln) # reset lane
            #if (sig_det(lane=ln)[ln])==1: # if PHY RDY = 0, save time and skip the rest of adaptation
            ctle_search_nrz(lane=ln, t=0.02) 
            lr(lane=ln)
            cntr_tgt_nrz(tgt_val='low', lane=ln)
                
        serdes_params(lane=ln)         

####################################################################################################
def opt_lane_pam4(datarate=None,input_mode='ac',lane=None):

    global gLanePartnerMap  # TX Source for RX, one  per lane per Slice
    global gChanEst; # Channel Estimates, one set per lane per Slice  
    lanes = get_lane_list(lane)
    #get_lane_mode(lanes) # update the Encoding modes of all lanes for this Slice
    c=Pam4Reg
    ########## DEVICE 0 EXTERNAL LOOPBACK LANES
    for ln in lanes:
        get_lane_mode(ln)
        init_lane_pam4(datarate,input_mode,ln) # PAM4 mode, BER ~1e7
        line_encoding_mode = gEncodingMode[gSlice][ln][0]
        peer_encoding_mode = gEncodingMode[gSlice][gLanePartnerMap[gSlice][ln][1]][0]

   
        if gPam4TxSourceIsCredo:  # Make sure TxPeer Lane is also the same mode and data rate as this lane (NRZ or PAM4)    
            if line_encoding_mode != peer_encoding_mode :
                print ("\nSlice %d Lane %s is in %s mode. Its TX Peer (%s) is in %s Mode"%(gSlice, lane_name_list[ln],line_encoding_mode.upper(),  lane_name_list[gLanePartnerMap[gSlice][ln][1]],peer_encoding_mode.upper()))
                continue
            else:  # Initialize This lane's TX Partner before getting Channel Estimate
                tx_taps(+5,-16,17,0,0,lane=gLanePartnerMap[gSlice][ln][1])
                time.sleep(.1)
                
        dc_gain(22,31,8,1, ln)
        delta_ph(-1,ln)
        f13(3,ln)        
        lr(ln)
        opt_agc1,opt_agc2 = dc_gain_search(lane=ln, target_dac_val = 10)
        
        of,hf,chan_est= channel_analyzer_pam4(gain1_val = opt_agc1, gain2_val = opt_agc2, lane=ln)
        gChanEst[gSlice][ln]=[chan_est,of,hf]
        if chan_est == 0.0: # LANE FAILED Channel Estimation
            print ("\nSlice %d Lane %s (PAM4) Channel Analyzer: ChanEst FAILED <<<"%(gSlice, lane_name_list[ln])),
        else:
            print ("\nSlice %d Lane %s (PAM4) Channel Analyzer: ChanEst: %5.3f, OF: %2d, HF: %2d"%(gSlice, lane_name_list[ln],chan_est, of, hf)),             

        if chan_est < 1.80:  ############## USE SR TABLE #################
            ctle_map(0,7,1, lane=ln)
            ctle_map(1,3,4, lane=ln)
            ctle_map(2,3,5, lane=ln)
            ctle_map(3,6,3, lane=ln)
            ctle_map(4,4,6, lane=ln)
            ctle_map(5,7,5, lane=ln)
            ctle_map(6,6,7, lane=ln)
            ctle_map(7,7,7, lane=ln)
        
        else:               ############## USE LR TABLE #################
            ctle_map(0,2,1, lane=ln)  # unusual value  <--- just for testing
            ctle_map(1,3,1, lane=ln)
            ctle_map(2,2,2, lane=ln)
            ctle_map(3,4,1, lane=ln)
            ctle_map(4,5,1, lane=ln)
            ctle_map(5,6,1, lane=ln)
            ctle_map(6,7,1, lane=ln)
            ctle_map(7,3,4, lane=ln)
         
        if chan_est == 0.0:         # LANE FAILED Channel Estimation
            agcgain(1,1,lane=ln)    # initial DC Gain
        elif chan_est <1.25:        # DIRECT LOOPBACK  ############## USE SR TABLE #################
            ctle(7, lane=ln)
            delta_ph(4,ln)
            f13(1,ln)
            edge(4,4,4,4,ln)
            skef(1,3,ln)
            dc_gain(1,1,8,8, ln) # initial DC Gain
            ffe_taps(0x66,0x11,-0x11,0x11,0x01,0x01,ln) # initial FFE Taps 
        elif chan_est <1.36: # Artek 0%, SR, less than 10dB (7,5)
            ctle(7, lane=ln)
            delta_ph(2,ln)
            f13(3,ln)
            edge(4,4,4,4,ln)
            skef(1,4,ln)
            dc_gain(1,10,8,8, ln) # initial DC Gain
            ffe_taps(0x33,0x11,-0x11,0x12,0x01,0x01,ln)   
        elif chan_est <1.47: # Artek 0%, SR, less than 10dB (7,5)
            ctle(6, lane=ln)
            delta_ph(-6,ln)
            f13(4,ln)
            edge(6,6,6,6,ln)
            skef(1,5,ln)
            dc_gain(1,10,8,8, ln) # initial DC Gain
            ffe_taps(0x44,0x00,0x00,-0x11,0x01,0x01,ln)   
        elif chan_est <1.55:  # Artek 10%, MR - 1.57 (4,6)
            ctle(5, lane=ln)
            delta_ph(-6,ln)
            f13(4,ln)
            edge(6,6,6,6,ln)
            skef(1,5,ln)
            dc_gain(1,25,8,8, ln) # initial DC Gain
            ffe_taps(0x77,0x01,0x11,-0x55,0x01,0x01,ln) # initial FFE Taps 
        elif chan_est <1.59:  # Artek 20%, MR,  ctle(4,6)
            ctle(4, lane=ln)
            delta_ph(-6,ln)
            f13(4,ln)
            edge(6,6,6,6,ln)
            skef(1,5,ln)
            dc_gain(8,31,8,8, ln) # initial DC Gain
            ffe_taps(0x77,0x01,0x11,-0x55,0x01,0x01,ln) # initial FFE Taps
        elif chan_est <1.68:  # Artek 30%, MR, ctle(3,7)
            ctle(3, lane=ln)
            delta_ph(-6,ln)
            f13(5,ln)
            edge(9,9,9,9,ln)
            skef(1,6,ln)
            #dc_gain(10,31,8,8, ln) 
            ffe_taps(0x77,0x01,0x11,-0x55,0x01,0x01,ln) # initial FFE Taps
        elif chan_est <1.80:  # Artek 40%,  ctle(3,5) ###### SR TABLE #############
            ctle(2, lane=ln)
            delta_ph(-8,ln)
            f13(5,ln)
            edge(10,10,10,10,ln)
            skef(1,6,ln)
            #dc_gain(15,31,8,8, ln)
            dc_gain(30,31,8,8, ln)             
            ffe_taps(0x77,0x11,0x11,-0x55,0x01,0x01,ln) # initial FFE Taps
        
        ################# NEW CTLE TABLE USED ####################### after this condition > 1.80
        elif chan_est <1.95:  # Artek 50% ctle(7,2)     
            ctle(6, lane=ln)
            delta_ph(-5,ln)
            f13(6,ln)
            edge(10,10,10,10,ln)
            skef(1,7,ln)
            #dc_gain(20,31,8,8, ln) 
            dc_gain(30,31,8,8, ln) 
            ffe_taps(0x77,0x11,0x11,-0x55,0x01,0x01,ln)
        elif chan_est <2.05:  # Artek 60% ctle(3,4)  #### NEW TABLE USED #####
            ctle(5, lane=ln)
            delta_ph(-5,ln)
            f13(6,ln)
            edge(11,11,11,11,ln)
            skef(1,7,ln)
            dc_gain(20,31,8,8, ln) 
            ffe_taps(0x77,0x01,0x11,-0x55,0x01,0x01,ln)                       
        elif chan_est <2.25:  # Artek 70%, (6,2)
            ctle(4, lane=ln)
            delta_ph(-6,ln)
            f13(6,ln)
            edge(12,12,12,12,ln)
            skef(1,7,ln)
            #dc_gain(25,31,8,2, ln) 
            ffe_taps(0x77,0x01,0x11,-0x55,0x01,0x01,ln) 
        elif chan_est <2.45:  # Artek 80% , (5,2)
            ctle(3, lane=ln)
            delta_ph(-7,ln)
            f13(7,ln)
            edge(12,12,12,12,ln)
            skef(1,7,ln)
            dc_gain(30,31,8,2, ln) 
            ffe_taps(0x77,0x01,0x11,-0x55,0x01,0x01,ln)                 
        elif chan_est <2.7:  # Artek 90%, (4,2)
            ctle(2, lane=ln)
            delta_ph(-8,ln)
            f13(8,ln)
            edge(13,13,13,13,ln)
            skef(1,7,ln)
            #dc_gain(35,31,8,2, ln)
            dc_gain(50,31,8,2, ln)            
            ffe_taps(0x77,0x01,0x11,0x11,0x01,0x01,ln) 
        elif chan_est <3.25: # Artek 100%, (6,1)
            ctle(1, lane=ln)
            delta_ph(-14,ln)
            f13(9,ln)
            edge(13,13,13,13,ln)
            skef(1,7,ln)
            #dc_gain(40,31,15,1, ln) 
            dc_gain(70,31,15,1, ln) 
            ffe_taps(0x11,0x11,0x11,0x11,0x32,0x30,ln)                   
        else: #if chan_est >=3.25: # Artek 100%, (6,1)
            #if gPam4TxSourceIsCredo: tx_taps(+3,-12,17,0,0,lane=gLanePartnerMap[gSlice][ln][1])
            ctle(0, lane=ln)
            delta_ph(-14,ln)
            f13(9,ln)
            edge(13,13,13,13,ln)
            skef(1,7,ln)
            dc_gain(90,31,15,1, ln) 
            ffe_taps(0x11,0x55, 0x01,0x11,0x01,0x01,ln)
        
        print ("\nSlice %d Lane %s (PAM4) CTLE selection in EQ1: %d"%(gSlice, lane_name_list[ln],ctle2(lane=ln)))
        tx_taps_table(chan_est,ln) # select TX Taps based on channel estimate
        if chan_est != 0.0:         # LANE PASSED Channel Estimation
            #f13_table(lane=ln)
            #dc_gain_search(lane=ln,target_dac_val=10)
            lr(ln)
            
            #if sig_det(lane=ln)[ln]: # if PHY RDY = 0, save time and skip the rest of adaptation
            #lane_reset_fast(ln) # reset lanes
            time.sleep(1)
            
            wreg(c.rx_theta_update_mode_addr,0,ln) # Disable updn Mode
            delta_search(lane=ln, print_en=0) 
            #ctle_search(lane=ln, t=0.5)
            ctle_fine_search(lane=ln)
            if chan_est > 1.6:
                ctle_fine_search(lane=ln)
            #ctle_search(lane=ln, t=0.5)
            dc_gain_search(lane=ln)
            lr(ln)
            time.sleep(1)
            print ("\nSlice %d Lane %s (PAM4) CTLE selection in ctle_search: %d"%(gSlice, lane_name_list[ln],ctle2(lane=ln)))
            
            wreg(c.rx_theta_update_mode_addr,7,ln) # Enable updn Mode
            time.sleep(.1)
            delta_search(lane=ln, print_en=0) 
            f13_table(lane=ln)
            lr(ln)
            time.sleep(.5)            
            i=0
            list = [1,1,1,-1,-1,-1]
            while i < 5:
                if eye_check(lane=ln)[0] != 0:
                    i+=1
                    f13(val=(f13(lane=ln)[0]+list[i]),lane=ln)
                    lane_reset_fast(ln)
                    time.sleep(.3)
                else:
                    break
            print("\nSlice %d Lane %s final f13 value: %d"%(gSlice, lane_name_list[ln],f13(lane=ln)[0])), 
                
            if chan_est >=1.47: 
                ffe_search_a1_orig(lane=ln, print_en=0)
        
            lr(ln) # reset lanes
            time.sleep(.5)
            background_cal(enable='en', lane=ln)
            time.sleep(.5)
         
        serdes_params(lane=ln)   
          
####################################################################################################
def tx_taps_table(chan_est=None,lane=None):

    global gLanePartnerMap  # TX Source for RX, one  per lane per Slice
    global gChanEst;  # Channel Estimates, one set per lane per Slice  
    lanes = get_lane_list(lane)
    #get_lane_mode(lanes) # update the Encoding modes of all lanes for this Slice
   
    for ln in lanes:
        get_lane_mode(ln)
        ##### Check if this lane is PAM4 or NRZ
        line_encoding_mode = gEncodingMode[gSlice][ln][0]
        peer_encoding_mode = gEncodingMode[gSlice][gLanePartnerMap[gSlice][ln][1]][0]
        
        ##### Check if this lane's TX Peer is from Credo TX and both ends' mode (PAM4 or NRZ) match
        if (gPam4TxSourceIsCredo==1 or gNrzTxSourceIsCredo==1) and (line_encoding_mode!=peer_encoding_mode) :
            print ("\n***tx_taps_chan_est(): Slice %d Lane %s is in %s mode. Its TX Peer (%s) is in %s Mode"%(gSlice, lane_name_list[ln],line_encoding_mode.upper(),  lane_name_list[gLanePartnerMap[gSlice][ln][1]],peer_encoding_mode.upper()))
            continue

        ##### If user does not pass Chan Estimate, get the chan estimate from most recent Rx Adaptation
        if chan_est is None:
            if fw_loaded(print_en=0):
                dbg_md = 2 if  line_encoding_mode=='pam4' else 1
                chanEst =(fw_debug_info(section=dbg_md, index=2,lane=ln)[ln]) / 256.0
                of      = fw_debug_info(section=dbg_md, index=4,lane=ln)[ln]
                hf      = fw_debug_info(section=dbg_md, index=5,lane=ln)[ln]            
                gChanEst[gSlice][ln]=[chanEst,of,hf]
                
            #print ("%5.3f,%2d,%2d" %(gChanEst[gSlice][ln][0],gChanEst[gSlice][ln][1],gChanEst[gSlice][ln][2])),
            chanEst = gChanEst[gSlice][ln][0]
        else:
            chanEst=chan_est
        
        ##### This lane is PAM4. Set the TX if the TX source for this lane is Credo TX
        if line_encoding_mode=='pam4' and gPam4TxSourceIsCredo==1:
            if   chanEst <0.90:  tx_taps(+2, -8,17,0,0,lane=gLanePartnerMap[gSlice][ln][1]) # default or used for channel analyzer
            elif chanEst <1.25:  tx_taps(+1, -6,16,0,0,lane=gLanePartnerMap[gSlice][ln][1]) # DIRECT LOOPBACK
            elif chanEst <1.36:  tx_taps(+1, -6,16,0,0,lane=gLanePartnerMap[gSlice][ln][1]) # Artek 0%, SR, less than 10dB
            elif chanEst <1.47:  tx_taps(+2, -8,17,0,0,lane=gLanePartnerMap[gSlice][ln][1]) # Artek 0%, SR, less than 10dB 
            elif chanEst <1.55:  tx_taps(+3,-12,17,0,0,lane=gLanePartnerMap[gSlice][ln][1]) # Artek 10%, MR
            elif chanEst <1.59:  tx_taps(+3,-12,17,0,0,lane=gLanePartnerMap[gSlice][ln][1]) # Artek 20%, MR
            elif chanEst <1.68:  tx_taps(+3,-12,17,0,0,lane=gLanePartnerMap[gSlice][ln][1]) # Artek 30%, MR
            elif chanEst <1.80:  tx_taps(+3,-12,17,0,0,lane=gLanePartnerMap[gSlice][ln][1]) # Artek 40%,
            elif chanEst <1.95:  tx_taps(+4,-12,17,0,0,lane=gLanePartnerMap[gSlice][ln][1]) # Artek 50%    
            elif chanEst <2.05:  tx_taps(+4,-12,17,0,0,lane=gLanePartnerMap[gSlice][ln][1]) # Artek 60%                   
            elif chanEst <2.25:  tx_taps(+4,-15,18,0,0,lane=gLanePartnerMap[gSlice][ln][1]) # Artek 70%
            elif chanEst <2.45:  tx_taps(+4,-15,18,0,0,lane=gLanePartnerMap[gSlice][ln][1]) # Artek 80%  LR
            elif chanEst <2.70:  tx_taps(+4,-15,18,0,0,lane=gLanePartnerMap[gSlice][ln][1]) # Artek 90%  LR
            elif chanEst <3.20:  tx_taps(+4,-15,18,0,0,lane=gLanePartnerMap[gSlice][ln][1]) # Artek 100% LR
            else:                tx_taps(+4,-15,18,0,0,lane=gLanePartnerMap[gSlice][ln][1]) # 
                
        ###### This lane is NRZ. Set the TX if the TX source for this lane is Credo TX
        elif line_encoding_mode=='nrz' and gNrzTxSourceIsCredo==1:            
            if   chanEst < 0.90: tx_taps( 0, -8,17,0,0,lane=gLanePartnerMap[gSlice][ln][1]) # default
            elif chanEst < 1.20: tx_taps( 0, -8,17,0,0,lane=gLanePartnerMap[gSlice][ln][1])
            elif chanEst < 1.40: tx_taps( 0, -8,17,0,0,lane=gLanePartnerMap[gSlice][ln][1])
            elif chanEst < 1.55: tx_taps( 0, -8,17,0,0,lane=gLanePartnerMap[gSlice][ln][1])
            elif chanEst < 1.80: tx_taps( 0, -8,17,0,0,lane=gLanePartnerMap[gSlice][ln][1])
            elif chanEst < 2.09: tx_taps( 0, -8,17,0,0,lane=gLanePartnerMap[gSlice][ln][1])
            elif chanEst < 4.00: tx_taps( 0, -8,17,0,0,lane=gLanePartnerMap[gSlice][ln][1])
            else:                tx_taps( 0, -8,17,0,0,lane=gLanePartnerMap[gSlice][ln][1]) # default or used for channel analyzer

        tt=tx_taps(lane=gLanePartnerMap[gSlice][ln])[gLanePartnerMap[gSlice][ln][1]]
        diff = '-' if ln==gLanePartnerMap[gSlice][ln][1] else '*'
        print ("\n  (%2d,%3d,%2d,%d,%d) %s-TX %s-->  %s-RX (ChanEst: %5.3f) "%(tt[0],tt[1],tt[2],tt[3],tt[4],lane_name_list[gLanePartnerMap[gSlice][ln]], diff, lane_name_list[ln], chanEst)),   
####################################################################################################
def fw_serdes_params(lane=None,slice=None,print_en=1):

    fullstring = ""
    
    if not fw_loaded(print_en=0):
        print ("\n######## FW is Not Loaded ! #########"),
    
    lanes = get_lane_list(lane)
    
    if slice is None: # if slice is not defined used gSlice
        slice=gSlice 
    
    top_pll_A_cap = rreg([0x9501,[12,6]])
    top_pll_B_cap = rreg([0x9601,[12,6]])
    
    pam4_lane_in_this_slice=0
    for ln in lanes:
        if fw_loaded(print_en=0):
            [lane_mode,lane_speed]=fw_lane_speed(ln)[ln]
            gEncodingMode[gSlice][ln][0] = lane_mode
        else: # avoid crash if FW not loaded
            get_lane_mode(ln)
        if gEncodingMode[gSlice][ln][0] == 'pam4':
            pam4_lane_in_this_slice=1
            
    die_temp = fw_temp_sensor(print_en=0)        
    die_rev = chip_rev(print_en=0)
    
    ver_code = fw_ver(print_en=0)
    ver_str  = "v%02d.%02d.%02d" %(ver_code>>16&0xFF,ver_code>>8&0xFF,ver_code&0xFF)
    crc_code = fw_crc(print_en=0)
    crc_str  = "CRC-0x%04X" %(crc_code)

    #top_line      = "\n#+------------------------------------ S E R D E S  P A R A M S -------- Rev %s, Slice %d, Temp %5.1fC ----------------------------------------------------------------+"%('A0' if die_rev==1.0 else 'B0',slice,die_temp)
    top_line      = "\n# SERDES PARAMS --- Rev %s, Slice %d, FW %s %s, Temp %5.1f C "%('A0' if die_rev==1.0 else 'B0',slice,ver_str,crc_str,die_temp)
    line_separator= "\n#+--------------------------------------------------------------------------------------------------------------------------------------------------------------------+"
    fullstring += top_line
    fullstring += line_separator
    fullstring += ("\n#|     |    |  FW COUNTERS    |SD,Rdy,|FRQ |PLL Cal|  CHANNEL   |      CTLE      |  |   | EYE MARGIN  |            DFE          | TIMING |           FFE Taps         |")
    if pam4_lane_in_this_slice:
        fullstring += ("\n#|Lane |Mode| Adp ,ReAdp,LLost|AdpDone|PPM | %d,%d | Est ,OF,HF | Peaking, G1,G2 |SK|DAC|  1 , 2 , 3  | F0 , F1 ,F1/F0,F1+F0,F13|Del,Edge| K1 , K2 , K3 , K4 ,S1,S2,Sf|"%(top_pll_A_cap,top_pll_B_cap))
    else:
        fullstring += ("\n#|Lane |Mode| Adp ,ReAdp,LLost|AdpDone|PPM | %d,%d | Est ,OF,HF | Peaking, G1,G2 |SK|DAC|  1 , 2 , 3  |  F1,  F2,  F3           |Del,Edge| K1 , K2 , K3 , K4 ,S1,S2,Sf|"%(top_pll_A_cap,top_pll_B_cap))
    fullstring += line_separator
    
    for ln in lanes:
        c = Pam4Reg if lane_mode_list[ln].lower() == 'pam4' else NrzReg  
        if fw_loaded(print_en=0):
            [lane_mode,lane_speed]=fw_lane_speed(ln)[ln]
            gEncodingMode[gSlice][ln][0] = lane_mode
            gEncodingMode[gSlice][ln][1] = lane_speed
        else: # avoid crash if FW not loaded
            get_lane_mode(ln)
            lane_mode = gEncodingMode[gSlice][ln][0]
            lane_speed= int(gEncodingMode[gSlice][ln][1])
        
        if lane_mode.upper()=='OFF': # Lane is OFF
            fullstring += ("\n#|S%d_%s| %3s|"%(slice, lane_name_list[ln], lane_speed))
            fullstring += ("                 |       |    |       |            |                |  |   |             |                         |        |                            |")

        else: # Lane is active, in PAM4 or NRZ mode            
            adapt_cnt = fw_adapt_cnt(ln)[ln]
            readapt_cnt = fw_readapt_cnt(ln)[ln]
            linklost_cnt = fw_link_lost_cnt(ln)[ln]
            sd = sig_det(ln)[ln]
            rdy = phy_rdy(ln)[ln]
            adapt_done = (rreg(c.fw_opt_done_addr) >> ln) & 1
            sd_flag  = '*' if ( sd!=1) else ' '
            rdy_flag = '*' if (rdy!=1 or adapt_done!=1) else ' '          
            ppm_val = ppm(ln)[ln]
            tx_cap = rreg(c.tx_pll_lvcocap_addr,ln)
            rx_cap = rreg(c.rx_pll_lvcocap_addr,ln)
            chan_est,of,hf = fw_chan_est(ln)[ln]
            gChanEst[gSlice][ln] = [chan_est,of,hf]
            ctle_val = ctle(lane=ln)[ln]
            ctle_1_bit4=rreg([0x1d7,[3]],ln)
            ctle_2_bit4=rreg([0x1d7,[2]],ln)
            ctle_1 = ctle_map(ctle_val, lane=ln)[ln][0] + (ctle_1_bit4*8)
            ctle_2 = ctle_map(ctle_val, lane=ln)[ln][1] + (ctle_2_bit4*8)
            #if (lane_mode.upper()=='PAM4' and chan_est < 1.80): ctle_val+=8
            if lane_mode.upper()=='PAM4' and (ctle_1_bit4==1 or ctle_2_bit4==1 or ctle_map(7)[ln][0]==7): ctle_val+=8
            skef_val = skef(lane=ln)[ln][1]
            dac_val  =  dac(lane=ln)[ln]

            delta_val = delta_ph(lane=ln)[0]
            edge1,edge2,edge3,edge4 = edge(lane=ln)[ln]
            eyes = eye(lane=ln)[ln]
            #pre_ber, post_ber = ber(rst=1,lane=ln, t=0.1)[ln]
            
            fullstring += ("\n#|S%d_%s| %3s|%5d,%5d,%4d |%s%d,%d,%d%s|%3d | %2d,%2d "%(slice, lane_name_list[ln], lane_speed,adapt_cnt,readapt_cnt,linklost_cnt,sd_flag,sd,rdy,adapt_done,rdy_flag,ppm_val,tx_cap,rx_cap))  
            fullstring += ("|%5.2f,%2d,%2d " %(chan_est,of,hf))        
            fullstring += ("|%2d(%d,%-2d),%3d,%2d " %(ctle_val,ctle_1,ctle_2,dc_gain(lane=ln)[ln][0],dc_gain(lane=ln)[ln][1]))
            fullstring += ("|%d |%2d " %(skef_val,dac_val))
            
            if lane_mode.upper()=='PAM4': # Lane is in PAM4 mode
                f0,f1,f1f0_ratio = pam4_dfe(lane=ln)[ln]
                f13_val  = f13(lane=ln)[0]
                fullstring += ("|%4.0f,%3.0f,%3.0f "%(eyes[0],eyes[1],eyes[2]))               
                fullstring += ("|%4.2f,%4.2f,%5.2f,%5.2f,%2d " %(f0,f1,f1f0_ratio,f0+f1,f13_val)) 
                fullstring += ("|%3d,%X%X%X%X| " %(delta_val,edge1,edge2,edge3,edge4))
                [ffe_k1_bin,ffe_k2_bin,ffe_k3_bin,ffe_k4_bin,ffe_s1_bin,ffe_s2_bin,ffe_sf_bin] = ffe_taps(lane=ln)[ln]
                fullstring +=('\b%4d,%4d,%4d,%4d,%02X,%02X,%02X| ' %(ffe_k1_bin,ffe_k2_bin,ffe_k3_bin,ffe_k4_bin,ffe_s1_bin,ffe_s2_bin,ffe_sf_bin))
            else: # Lane is in NRZ mode
                # hf_0= fw_debug_cmd(section=1, index=250,lane=ln)
                # hf_1= fw_debug_cmd(section=1, index=251,lane=ln)
                # hf_2= fw_debug_cmd(section=1, index=252,lane=ln)
                     
                # of_0= fw_debug_cmd(section=1, index=255,lane=ln)
                # of_1= fw_debug_cmd(section=1, index=256,lane=ln)
                # of_2= fw_debug_cmd(section=1, index=257,lane=ln)
                
                # adc_cal_done_0= fw_debug_cmd(section=1, index=530,lane=ln)
                # adc_cal_done_1= fw_debug_cmd(section=1, index=531,lane=ln)
                # adc_cal_done_2= fw_debug_cmd(section=1, index=532,lane=ln)
                
                # read_B0_q_0= fw_debug_cmd(section=1, index=425, lane=ln)
                # read_B0_q_1= fw_debug_cmd(section=1, index=426, lane=ln)
                # read_B0_q_2= fw_debug_cmd(section=1, index=427, lane=ln)
                
                # agc_peaking_0= fw_debug_cmd(section=1, index=420, lane=ln)
                # agc_peaking_1= fw_debug_cmd(section=1, index=421, lane=ln)
                # agc_peaking_2= fw_debug_cmd(section=1, index=422, lane=ln)
                
                f1,f2,f3 = nrz_dfe(lane=ln)[ln]
                fullstring += ("|%4.0f         "%(eyes[0]))             
                fullstring += ("|%4d,%4d,%4d           " %(f1,f2,f3))
                fullstring += ("|%3d,%X%X%X%X| " %(delta_val,edge1,edge2,edge3,edge4)) 
                fullstring += ("                           | ")
                # print('|%2d %2d %2d %2d %2d %2d       |' %(hf_0, hf_1, hf_2, of_0, of_1, of_2) ),
                # if(hf_0<10 or hf_1<10 or hf_2<10 or of_0<10 or of_1<10 or of_2<10):
                    # print('\n<<<<< adc_cal_done = %2d %2d %2d <<<<<')%(adc_cal_done_0, adc_cal_done_1, adc_cal_done_2),
                    # print('\n<<<<< read_B0_q    = %2d %2d %2d <<<<<')%(read_B0_q_0, read_B0_q_1, read_B0_q_2),
                    # print('\n<<<<< agc peaking  = %2d %2d %2d <<<<<\n')%(agc_peaking_0, agc_peaking_1, agc_peaking_2),
                    # nrz_dump_fw(lane=ln)

            
        if (ln<lanes[-1] and ln==7) or (ln==lanes[-1]):
            fullstring += line_separator # Line separator between A lines and B lines
            
    if print_en == 1: 
        print (fullstring)
    else:
        return fullstring

####################################################################################################
def serdes_params(lane=None):

    top_pll_A_cap = rreg([0x9501,[12,6]])
    top_pll_B_cap = rreg([0x9601,[12,6]])

    line_separator= "\n#+--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+"
    print (line_separator)
    print ("\n#|   |    |Line | Data | RX |      TX Taps       |                 | SD, | FRQ |PLL Cal|  CHANNEL  |      CTLE     |   |   | EYE MARGIN  |         DFE       | TIMING  |           FFE Taps         |"),
    print ("\n#|Dev|Lane| Enc | Rate | In |Ln(-2, -1, M,+1,+2) | Pol,Gry,PrC,MSB | Rdy | PPM | %d,%d | Est,OF,HF |Peaking, G1,G2 |SK |DAC|  1 , 2 , 3  | F0 , F1 ,F1/F0,F13|Del,Edge | K1 , K2 , K3 , K4 ,S1,S2,Sf|"%(top_pll_A_cap,top_pll_B_cap)),
    print (line_separator)

    lanes = get_lane_list(lane)
    get_lane_mode('all')
    
    for ln in lanes:
        c = Pam4Reg if lane_mode_list[ln].lower() == 'pam4' else NrzReg  
        line_encoding = gEncodingMode[gSlice][ln][0].upper()
        #lane_speed= str(int(round((gEncodingMode[gSlice][ln][1]-1.0)/5.0)*5.0))+'G'
        lane_speed= gEncodingMode[gSlice][ln][1]
        tt=tx_taps(lane=gLanePartnerMap[gSlice][ln][1])[gLanePartnerMap[gSlice][ln][1]]
        #tx_peer_marker = '<' if gLanePartnerMap[gSlice][ln][1]==ln else '/'
        rx_mode='DC' if(rreg(c.rx_vcomrefsel_ex_addr,ln)==0) else 'ac'
        tx_pol, rx_pol = pol   (lane=ln,print_en=0)
        if line_encoding=='PAM4': # Lane is in PAM4 mode
            tx_gc , rx_gc  = gc    (lane=ln,print_en=0)
            tx_pc , rx_pc  = pc    (lane=ln,print_en=0)
            tx_msb, rx_msb = msblsb(lane=ln,print_en=0)
        sd=sig_det(ln)[ln]
        rdy=phy_rdy(ln)[ln]
        sd_flag ='*' if ( sd!=1) else ' '
        rdy_flag='*' if (rdy!=1) else ' '          
        ppm_val=ppm(ln)[ln]
        tx_cap= rreg(c.tx_pll_lvcocap_addr,ln)
        rx_cap= rreg(c.rx_pll_lvcocap_addr,ln)
        if fw_loaded(print_en=0): 
            fw_chan_est(ln)[ln]
        chan_est,of,hf = gChanEst[gSlice][ln]
        ctle_val = ctle(lane=ln)[ln]; ctle_1= ctle_map(ctle_val, lane=ln)[ln][0]; ctle_2= ctle_map(ctle_val, lane=ln)[ln][1]        
        skef_val = skef(lane=ln)[ln][1]
        dac_val  =  dac(lane=ln)[ln]
        delta_val= delta_ph(lane=ln)[0]
        edge1,edge2,edge3,edge4 = edge(lane=ln)[ln]
        eyes=eye(lane=ln)[ln]
        
        print ("\n#| %d | %s"%(gSlice, lane_name_list[ln])),  
        print ("|%4s |%6.3f| %s"%(line_encoding,lane_speed,rx_mode)),  
        print ("|%2s(%2d,%3d,%2d,%2d,%2d)"%(lane_name_list[gLanePartnerMap[gSlice][ln][1]],tt[0],tt[1],tt[2],tt[3],tt[4])),   
       
        if line_encoding=='PAM4': # Lane is in PAM4 mode
            print ("| %d/%d,%d/%d,%d/%d,%d/%d" %(tx_pol, rx_pol,tx_gc , rx_gc,tx_pc , rx_pc,tx_msb, rx_msb)),
        else:
            print ("| %d/%d            " %(tx_pol, rx_pol)),
        print ("|%s%d,%d%s|%4d | %2d,%2d"%(sd_flag,sd,rdy,rdy_flag,ppm_val,tx_cap,rx_cap)),  
        print ("|%4.2f,%2d,%2d" %(abs(chan_est),of,hf)),        
        print ("| %d(%d,%d),%3d,%2d" %(ctle_val,ctle_1,ctle_2,dc_gain(lane=ln)[ln][0],dc_gain(lane=ln)[ln][1])),   
        print ("| %d |%2d" %(skef_val,dac_val)),   
        
        if line_encoding=='PAM4': # Lane is in PAM4 mode
            f0,f1,f1f0_ratio = pam4_dfe(lane=ln)[ln]
            f13_val  = f13(lane=ln)[0]
            print ("|%4.0f,%3.0f,%3.0f"%(eyes[0],eyes[1],eyes[2])),                
            print ("|%4.2f,%4.2f,%5.2f,%2d" %(abs(f0),abs(f1),abs(f1f0_ratio),f13_val)),   
            print ("|%3d,%X%X%X%X" %(delta_val,edge1,edge2,edge3,edge4)),   
            [ffe_k1_bin,ffe_k2_bin,ffe_k3_bin,ffe_k4_bin,ffe_s1_bin,ffe_s2_bin,ffe_sf_bin] = ffe_taps(lane=ln)[ln]
            print('|%4d,%4d,%4d,%4d,%02X,%02X,%02X|' %(ffe_k1_bin,ffe_k2_bin,ffe_k3_bin,ffe_k4_bin,ffe_s1_bin,ffe_s2_bin,ffe_sf_bin)),
        else: # Lane is in NRZ mode
            print ("|%4.0f        "%(eyes[0])),                
            print ("|                   |        "),
            print('|                            |' ),
            
        if (ln<lanes[-1] and ln==7) or (ln==lanes[-1]):
            print (line_separator) # Line separator between A lines and B lines
         
####################################################################################################
def fw_slice_params(lane=None,slice=None,print_en=1):

    if slice is None: # if slice number is not given use gSlice
        slice=gSlice
    else:
        sel_slice(slice)
        
    if not fw_loaded(print_en=0):
        print ("\n...Error in 'fw_slice_params': Slice %d has no FW!"%slice),
        return
        

    lanes = get_lane_list(lane)
           
              #[ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 ,  0x08,  0x09,  0x0A, 0x0B]
    #speed_list=['OFF','10G','20G','25G','26G','28G', '50G','07?', '50G', '50G', '56G', '0x0B?']
    mode_list =['OFF','NRZ-10G','NRZ-20G','NRZ-25G','NRZ-26G','NRZ-28G','PAM4-50G','pam4-07?','pam4-51G','PAM4-50G','pam4-56G','pam4-0B?']
    config_list={#0x00: 'Not Configured', 
                 ## Retimer config codes
                 0x00: 'RetimerNrz - 0',        0x01: 'RetimerNrz - 1',        0x02: 'RetimerNrz - 2',       0x03: 'RetimerNrz - 3',
                 0x04: 'RetimerNrz - 4',        0x05: 'RetimerNrz - 5',        0x06: 'RetimerNrz - 6',       0x07: 'RetimerNrz - 7',
                 0x10: 'RetimerPam4 - 0',       0x11: 'RetimerPam4 - 1',       0x12: 'RetimerPam4 - 2',      0x13: 'RetimerPam4 - 3',
                 0x14: 'RetimerPam4 - 4',       0x15: 'RetimerPam4 - 5',       0x16: 'RetimerPam4 - 6',      0x17: 'RetimerPam4 - 7',
                 ## Retimer Cross-Mode config codes
                 0x20: 'RetimerNrz X 0',        0x21: 'RetimerNrz X 1',        0x22: 'RetimerNrz X 2',       0x23: 'RetimerNrz X 3',                    
                 0x24: 'RetimerNrz X 4',        0x25: 'RetimerNrz X 5',        0x26: 'RetimerNrz X 6',       0x27: 'RetimerNrz X 7',
                 0x30: 'RetimerPam4 X 0',       0x31: 'RetimerPam4 X 1',       0x32: 'RetimerPam4 X 2',      0x33: 'RetimerPam4 X 3',
                 0x34: 'RetimerPam4 X 4',       0x35: 'RetimerPam4 X 5',       0x36: 'RetimerPam4 X 6',      0x37: 'RetimerPam4 X 7',
                 ## Bitmux config codes
                 0x40: 'BitMux-20G - 0',        0x41: 'BitMux-20G - 1',        0x42: 'BitMux-20G - 2',       0x43: 'BitMux-20G - 3',        0x44: 'BitMux-20G - 4',       0x45: 'BitMux-20G - 5', 
                 0x50: 'BitMux-53G - 0',        0x51: 'BitMux-53G - 1',        0x52: 'BitMux-53G - 2',       0x53: 'BitMux-53G - 3',        0x54: 'BitMux-53G - 4',       0x55: 'BitMux-53G - 5', 
                 ## Retimer Cross-Mode config codes
                 0x90: 'GearBox100G - 0',       0x91: 'GearBox100G - 1',       0x92: 'GearBox100G - 2', 
                 0x98: 'GearBox100gNoFecB - 0', 0x99: 'GearBox100gNoFecB - 1', 0x9a: 'GearBox100gNoFecB - 2',0x9b: 'GearBox100gNoFecB - 3',
                 0xb0: 'GearBox50G - 0',        0xb1: 'GearBox50G - 1',        0xb2: 'GearBox50G - 2',       0xb3: 'GearBox50G - 3',        0xb4: 'GearBox50G - 4',       0xb5: 'GearBox50G - 5',
                 0xb8: 'GearBox50gNoFecB - 0',  0xb9: 'GearBox50gNoFecB - 1',  0xba: 'GearBox50gNoFecB - 2', 0xbb: 'GearBox50gNoFecB - 3',  0xbc: 'GearBox50gNoFecB - 4', 0xbd: 'GearBox50gNoFecB - 5',
                 ## Serdes-Only config codes
                 0xC0: 'PhyOnlyNRZ', 0xD0: 'PhyOnlyPAM4',0xF0:'LoopBack', 
                }
    lane_status_list=['NotRdy*','RDY']
    lane_optic_mode_list=['Off','ON']

    result = {} 

    line_separator=    "\n +-----------------------------------------------------------------+"
    if print_en: print("\n              FW Configuration For Slice: %d"%(slice)),
    #if print_en: print lanes,
    if print_en: print (line_separator)
    if print_en: print("\n | Lane  |     Config - Group    | Mode-Speed | OpticBit |  Status |"),
    if print_en: print (line_separator)
    
    for ln in lanes:    
        speed_index     = fw_debug_cmd(section=0,index=4,lane=ln)
        #if speed_index > len(speed_list)-1: speed_index=0
        #lane_speed      = speed_list[speed_index]
        lane_mode_speed = mode_list[speed_index]
        config_code_this_lane= fw_debug_cmd(section=0, index=38, lane=ln)        
        lane_config     = config_list[config_code_this_lane] if config_code_this_lane<0xC0 else config_list[config_code_this_lane & 0xF0]
        lane_optic_mode = lane_optic_mode_list[(fw_reg_rd(20) >> ln) & 1]
        
        if 'OFF' in lane_mode_speed: lane_config = 'None' # overwrite config to 'None' is lane is 'OFF'
        
        #### GB or BM's status for all lanes is obtained through fw debug command section 0 index 40
        if 'GearBox' in lane_config or 'BitMux' in lane_config:
            lane_rdy   = lane_status_list[(fw_debug_cmd(section=0, index=40, lane=ln) >> ln) & 1]
        #### PHY Mode's status for all lanes is obtained through register 0x98c9
        else: 
            lane_rdy   =  lane_status_list[(rreg(c.fw_opt_done_addr) >> ln) & 1]
            

        result[ln]  = [lane_config,lane_mode_speed,lane_optic_mode,lane_rdy]
        
        if print_en: print("\n | S%d_%2s | %s |  %s  | %s | %s |"%(slice,lane_name_list[ln],lane_config.center(21,' '),lane_mode_speed.center(8,' '),lane_optic_mode.center(8,' '),lane_rdy.center(7,' '))),
    
    if print_en: print (line_separator)

    if not print_en: return result
####################################################################################################
# 
# Set Lanes in PRBS or Functional Mode
# prbs_mode options: 'functional'  : disable prbs, used in loopback, retimer or gearbox modes with no FEC
#                    'prbs'        : enable prbs generator, leaves the prbs pattern unchanged
#                    'prbs31       : enable prbs generator, select prbs pattern PRBS31
#                    'prbs23       : enable prbs generator, select prbs pattern PRBS23 (NRZ only)
#                    'prbs15       : enable prbs generator, select prbs pattern PRBS15
#                    'prbs13       : enable prbs generator, select prbs pattern PRBS13 (PAM4 only)
#                    'prbs9        : enable prbs generator, select prbs pattern PRBS9
####################################################################################################
def prbs_mode_select(lane=None, prbs_mode='prbs'):
    
    lanes = get_lane_list(lane)
    if prbs_mode.upper() == 'PRBS': prbs_mode ='PRBS31'
    
    #global gPrbsEn;  gPrbsEn = prbs_mode
   
    nrz_prbs_pat  = ['PRBS9', 'PRBS15', 'PRBS23', 'PRBS31'] 
    pam4_prbs_pat = ['PRBS9', 'PRBS13', 'PRBS15', 'PRBS31']
    
    # if a specific prbs pattern is requested, program the PRBS type register bits
    if 'PRBS' in prbs_mode.upper() :
        for ln in lanes:
            wreg([0x0a0,  [9,8]],pam4_prbs_pat.index(prbs_mode.upper()),ln) # PAM4/NRZ TX PRBS Pattern
            if gEncodingMode[gSlice][ln][0]=='pam4':
                wreg([0x043,  [6,5]],pam4_prbs_pat.index(prbs_mode.upper()),ln) # PAM4 RX PRBS Pattern
            else:
                wreg([0x161,[13,12]], nrz_prbs_pat.index(prbs_mode.upper()),ln) # NRZ RX PRBS Pattern
    
    for ln in lanes:
        get_lane_mode(ln)
        ###### Lane is in PAM4 mode ####################
        if gEncodingMode[gSlice][ln][0]=='pam4':
            wreg([0x0b0,[14]],0,ln)    # PAM4 mode, NRZ PRBS Gen clock dis
            wreg([0x0b0,[11]],0,ln)    # PAM4 mode, NRZ PRBS dis
            wreg([0x0b0, [1]],0,ln)    # PAM4 mode, NRZ TX dis
            wreg([0x0b0, [0]],0,ln)    # PAM4 mode, NRZ 10G dis
            
           #wreg([0x0a0, [6]],0,ln)    # PAM4 mode, TX pol flip = 0, leave polarity as it was
           #wreg([0x0a0, [5]],0,ln)    # PAM4 mode, TX ana pol flip = 0, leave polarity as it was
            
            ######## Credo PAM4 TX in Functional Mode, used in loopback, retimer and gearbox modes
            if 'PRBS' not in prbs_mode.upper(): 
                wreg([0x0a0,[15]],0,ln)# PAM4 mode, PRBS Gen de-selected
                wreg([0x0a0,[14]],0,ln)# PAM4 mode, PRBS Gen clock dis
                wreg([0x0a0,[13]],0,ln)# PAM4 mode, PRBS Gen test data dis
                wreg([0x0a0,[11]],0,ln)# PAM4 mode, PRBS Gen dis
                
               #wreg([0x043,  [7]],1,ln)# PAM4 mode, PRBS Checker flip = 1, leave polarity as it was
               #wreg([0x043,[6,5]],3,ln)# PAM4 mode, PRBS Checker PRBS pattern, done already, see above
                wreg([0x043,  [4]],1,ln)# PAM4 mode, PRBS Checker powered up
                wreg([0x043,  [3]],1,ln)# PAM4 mode, PRBS Sync Checker powered up
                wreg([0x043,  [1]],1,ln)# PAM4 mode, PRBS Sync Checker auto-sync en  
                #wreg(0x043,0x0ce2,ln) # PAM4 mode, Rx PAM4 FUNCTIONAL MODE | MSB first
            
            ######## Credo PAM4 TX and RX in PRBS Mode
            else: 
                wreg([0x0a0,[15]],1,ln) # PAM4 mode, PRBS Gen selected
                wreg([0x0a0,[14]],1,ln) # PAM4 mode, PRBS Gen clock en
                wreg([0x0a0,[13]],1,ln) # PAM4 mode, PRBS Gen test data en
                wreg([0x0a0,[11]],1,ln) # PAM4 mode, PRBS Gen en
                
               #wreg([0x043,  [7]],1,ln)# PAM4 mode, PRBS Checker flip = 1, leave polarity as it was
               #wreg([0x043,[6,5]],3,ln)# PAM4 mode, PRBS Checker PRBS pattern, done already, see above
                wreg([0x043,  [4]],1,ln)# PAM4 mode, PRBS Checker powered up
                wreg([0x043,  [3]],1,ln)# PAM4 mode, PRBS Sync Checker powered up
                wreg([0x043,  [1]],1,ln)# PAM4 mode, PRBS Sync Checker auto-sync en  
                
                #wreg(0x043,0x0cfa,ln)  # PAM4 mode, Rx PAM4 prbs31 checker | MSB first

        ###### Lane is in NRZ mode #######################
        else: 
            wreg([0x0b0,[14]],1,ln)    # NRZ mode, NRZ PRBS Gen clock en
            wreg([0x0b0,[11]],1,ln)    # NRZ mode, NRZ PRBS en
            wreg([0x0b0, [1]],1,ln)    # NRZ mode, NRZ TX en
           #wreg([0x0b0, [0]],0,ln)    # NRZ mode, NRZ TX 10G leave as it was
           
           #wreg([0x0a0, [6]],1,ln)    # NRZ mode, TX pol flip1 = leave as it was
           #wreg([0x0a0, [5]],0,ln)    # NRZ mode, TX pol flip2 = leave as it was
            
            ######## Credo NRZ TX in Functional Mode, used in loopback, retimer and gearbox modes
            if 'PRBS' not in prbs_mode.upper(): 
                wreg([0x0a0,   [15]],0,ln)# NRZ mode, PRBS Gen de-selected
                wreg([0x0a0,   [14]],0,ln)# NRZ mode, PRBS Gen clock dis
                wreg([0x0a0,   [13]],0,ln)# NRZ mode, PRBS Gen test data dis
                wreg([0x0a0,   [11]],0,ln)# NRZ mode, PRBS Gen dis
               #wreg([0x161,   [14]],1,ln)# NRZ mode, PRBS Checker flip = 1, leave as it was
               #wreg([0x161,[13,12]],3,ln)# NRZ mode, PRBS Checker PRBS pattern, done already, see above
               #wreg([0x161,   [10]],0,ln)# NRZ mode, PRBS Checker powered off

                #wreg(0x0a0,0x0120,ln) # NRZ mode, TX functional mode
                #wreg(0x161,0x7120,ln) # NRZ mode, PRBS Rx checker off, bit 10
                
            ######## Credo NRZ TX and RX in PRBS Mode
            else: 
                wreg([0x0a0,   [15]],1,ln)# NRZ mode, PRBS Gen selected
                wreg([0x0a0,   [14]],1,ln)# NRZ mode, PRBS Gen clock en
                wreg([0x0a0,   [13]],1,ln)# NRZ mode, PRBS Gen test data en
                wreg([0x0a0,   [11]],1,ln)# NRZ mode, PRBS Gen en
               #wreg([0x161,   [14]],1,ln)# NRZ mode, PRBS Checker flip = 1, leave as it was
               #wreg([0x161,[13,12]],3,ln)# NRZ mode, PRBS Checker PRBS pattern, done already, see above
                wreg([0x161,   [10]],1,ln)# NRZ mode, PRBS Checker powered up
                #wreg(0x161,0x7520,ln)    # NRZ mode, PRBS checker up, bit 10

####################################################################################################
def rx_monitor_clear(lane=None, fec_thresh=gFecThresh,mode=None):
    global gLaneStats #[per Slice][per lane], [PrbsCount, PrbsCount-1,PrbsCount-2, PrbsRstTime, PrbsLastReadoutTime]
    global gSlice
    gSlice=sel_slice()
    lanes = get_lane_list(lane)
    
    for ln in lanes:
        get_lane_mode(ln)
        c = Pam4Reg if gEncodingMode[gSlice][ln][0].upper() == 'PAM4' else NrzReg

        ###### 1. Initialize FEC Analyzer for this lane
        if gEncodingMode[gSlice][ln][0].lower() == 'pam4':
            fec_ana_init(lane=ln, delay=.1, err_type=2, T=fec_thresh, M=10, N=5440, print_en=0, mode=mode)
        else: # Lane is in NRZ mode 
            fec_ana_init(lane=ln, delay=.1, err_type=2, T=fec_thresh, M=10, N=5280, print_en=0, mode=mode)

        ###### 2. Clear Rx PRBS Counter for this lane
        wreg(c.rx_err_cntr_rst_addr, 0, ln)
        time.sleep(0.001)
        wreg(c.rx_err_cntr_rst_addr, 1, ln)
        time.sleep(0.001)
        wreg(c.rx_err_cntr_rst_addr, 0, ln)
        
        ###### 3. Capture Time Stamp for Clearing FEC and PRBS Counters. Used for Calculating BER
        prbs_reset_time = time.time() # get the time-stamp for the counter clearing

        ###### 4. Clear Stats for this lane
        #                     prbs1/2/3                               eye123     fec1,2
        gLaneStats[gSlice][ln]=[long(0),long(0),long(0),prbs_reset_time, prbs_reset_time,0,0,0,'CLR',0,0]
                               #[0,0,0,prbs_reset_time, prbs_reset_time,0,0,0,'RDY',0,0,0,0]
    return gLaneStats[gSlice]
    
#################################################################################################### 
def rx_monitor_capture (lane=None): 
    global gLaneStats #[per Slice][per lane], [PrbsCount, PrbsCount-1,PrbsCount-2, PrbsRstTime, PrbsLastReadoutTime]
    global gSlice
    gSlice=sel_slice()
    lanes = get_lane_list(lane)
   
    for ln in lanes:
        get_lane_mode(ln)
        c = Pam4Reg if gEncodingMode[gSlice][ln][0].lower() == 'pam4' else NrzReg
        
        ###### 1. Capture FEC Analyzer Data for this lane
        tei = fec_ana_tei(lane=ln)
        teo = fec_ana_teo(lane=ln)
        #sei = fec_ana_sei(lane=ln)
        #bei = fec_ana_bei(lane=ln)

        ###### 2. Capture PRBS Counter for this lane
        cnt1 = long(rreg(c.rx_err_cntr_msb_addr, ln)<<16) + rreg(c.rx_err_cntr_lsb_addr, ln)
        cnt = long(rreg(c.rx_err_cntr_msb_addr, ln)<<16) + rreg(c.rx_err_cntr_lsb_addr, ln)
        if cnt<cnt1: 
            cnt=cnt1 # check for LBS 16-bit wrap around
        
        ###### 3. Capture Time Stamp for the FEC and PRBS Counters. Used for Calculating BER
        cnt_time = time.time() # PrbsReadoutTime = get the time stamp ASAP for valid prbs count
        
        sd = sig_det(ln)[ln]
        rdy = phy_rdy(ln)[ln]
        adapt_done = (rreg(c.fw_opt_done_addr) >> ln) & 1
        cnt_n1 = long(gLaneStats[gSlice][ln][0]) # PrevPrbsCount-1
        cnt_n2 = long(gLaneStats[gSlice][ln][1]) # PrevPrbsCount-2
        if gLaneStats[gSlice][ln][3] != 0:  #if the PRBS count was cleared at least once before, use the time of last clear
            prbs_reset_time = gLaneStats[gSlice][ln][3]
        else:
            prbs_reset_time = cnt_time #if the PRBS counter was never cleared before, use the current time as the new 'clear time'
        link_status = 'RDY'
        if(sd==0 or rdy ==0 or adapt_done==0): # check for PHY_RDY first
            tei=0xEEEEEEEE
            teo=0xEEEEEEEE
            cnt=0xEEEEEEEE # return an artificially large number if RDY=0
            cnt_n1 = long(0) # PrevPrbsCount-1
            cnt_n2 = long(0) # PrevPrbsCount-2
            link_status = 'NOT_RDY'
            eyes=[-1,-1,-1]
        else: # RDY=1, then consider the PRBS count value
            eyes=eye(lane=ln)[ln]          
            # if(cnt < cnt_n1): # check for PRBS counter wrap-around
                # cnt = 0xFFFFFFFEL # return an artificially large number if counter rolls over
                # cnt_n1 = long(0) # PrevPrbsCount-1
                # cnt_n2 = long(0) # PrevPrbsCount-2

                
        # if RDY=1 and PRBS count value is considered a valid count
        gLaneStats[gSlice][ln]=[cnt,cnt_n1,cnt_n2,prbs_reset_time, cnt_time,eyes[0],eyes[1],eyes[2],link_status,tei,teo]

    return gLaneStats[gSlice]

#################################################################################################### 
def rx_monitor_print (lane=None,slice=None): 
    global gLaneStats #[per Slice][per lane], [PrbsCount, PrbsCount-1,PrbsCount-2, PrbsRstTime, PrbsLastReadoutTime]
    myber=99e-1
    global gSlice
    gSlice=sel_slice()
    
    lanes = get_lane_list(lane)

    #Slice=gSlice
    get_lane_mode(lanes)
    if slice is None: # if slice is not defined used gSlice
        slice=gSlice
    
    skip="       -"
    line="\n------------- "
    for ln in lanes:
        if ln == 8 :
            line+="|------- "
        else:
            line+="-------- "
    print (line)
    
    print("\n   Slice_Lane"),
    for ln in lanes:  print("   S%d_%s" %(slice,lane_name_list[ln])),
        
    print("\n     Encoding"),
    for ln in lanes:  print("%8s" %(gEncodingMode[gSlice][ln][0].upper())),
    print("\nDataRate Gbps"),
    for ln in lanes:  print("%8.4f" %(gEncodingMode[gSlice][ln][1])),

    print (line)
    print("\n  Link Status"),
    for ln in lanes:  print("%8s" %(gLaneStats[gSlice][ln][8])),
    print("\n    Eye1 (mV)"),
    for ln in lanes:  
        if(gLaneStats[gSlice][ln][8] == 'RDY'):
            print ("%8.0f"%(gLaneStats[gSlice][ln][5])),                
        else:
            print (skip)
    print("\n    Eye2 (mV)"),
    for ln in lanes:  
        if(gEncodingMode[gSlice][ln][0].upper()== 'PAM4' and gLaneStats[gSlice][ln][8] == 'RDY'):
            print ("%8.0f"%(gLaneStats[gSlice][ln][6])),                
        else:
            print (skip)
    print("\n    Eye3 (mV)"),
    for ln in lanes:  
        if(gEncodingMode[gSlice][ln][0].upper()== 'PAM4' and gLaneStats[gSlice][ln][8] == 'RDY'):
            print ("%8.0f"%(gLaneStats[gSlice][ln][7])),                
        else:
            print (skip)
        
    print (line)
    print("\n Elapsed Time"),
    for ln in lanes:
        if(gLaneStats[gSlice][ln][8] == 'RDY'):
            elapsed_time=gLaneStats[gSlice][ln][4] - gLaneStats[gSlice][ln][3]    
            if elapsed_time < 1000.0:
                print("%6.1f s" % (elapsed_time) ),
            else:
                print("%6.0f s" % (elapsed_time) ),
        else:
            print (skip)
    
    # print("\nPrev PRBS    "),
    # for ln in lanes:  print("%8X " % (gLaneStats[gSlice][ln][1]) ),
    #print("\nCurr PRBS    "),
    print("\n     PRBS Cnt"),
    for ln in lanes:
        if(gLaneStats[gSlice][ln][8] == 'RDY'):
            print("%8X" % (gLaneStats[gSlice][ln][0]) ),
        else:
            print (skip)
    # print("\nDeltaPRBS    "),
    # for ln in lanes:  print("%8X " % (gLaneStats[gSlice][ln][0] - gLaneStats[gSlice][ln][1]) ),

    print("\n     PRBS BER"),
    for ln in lanes:
        curr_prbs_cnt = float(gLaneStats[gSlice][ln][0])
        prbs_accum_time = gLaneStats[gSlice][ln][4] - gLaneStats[gSlice][ln][3]
        data_rate = gEncodingMode[gSlice][ln][1]
        bits_transferred = float((prbs_accum_time*data_rate*pow(10,9)))       
        if curr_prbs_cnt==0 or bits_transferred==0: 
            ber_val = 0
            print("%8d" % (ber_val) ),
            myber= ((ber_val) )
        else:
            ber_val= curr_prbs_cnt / bits_transferred
            if(gLaneStats[gSlice][ln][8] == 'RDY'):
                 print("%8.1e" % (ber_val) ),
                 myber= ((ber_val) )
            else:
                print (skip)

            
    print (line)
    #print "\n  User has set the FEC Threshold to less than max"
    if (gEncodingMode[gSlice][ln][0].lower() == 'pam4' and gFecThresh<15) or\
       (gEncodingMode[gSlice][ln][0].lower() != 'pam4' and gFecThresh<7):
        print("\n   FEC Thresh"),
        for ln in lanes:
            print("%8d" % (gFecThresh) ),

    print("\n  pre-FEC Cnt"),
    for ln in lanes:  
        if(gLaneStats[gSlice][ln][8] == 'RDY'):
            print("%8X" % (gLaneStats[gSlice][ln][9]) ),
        else:
            print (skip)

    print("\n Post-FEC Cnt"),
    for ln in lanes:  
        if(gLaneStats[gSlice][ln][8] == 'RDY'):
            print("%8X" % (gLaneStats[gSlice][ln][10]) ),
        else:
            print (skip)
    print("\n  pre-FEC BER"),
    for ln in lanes:
        curr_prbs_cnt = float(gLaneStats[gSlice][ln][9])
        prbs_accum_time = gLaneStats[gSlice][ln][4] - gLaneStats[gSlice][ln][3]
        data_rate = gEncodingMode[gSlice][ln][1]
        bits_transferred = float((prbs_accum_time*data_rate*pow(10,9)))       
        if curr_prbs_cnt==0 or bits_transferred==0: 
            ber_val = 0
            print("%8d" % (ber_val) ),
        else:
            ber_val= curr_prbs_cnt / bits_transferred
            if(gLaneStats[gSlice][ln][8] == 'RDY'):
                print("%8.1e" % (ber_val) ),
            else:

                print (skip)
    print("\n Post-FEC BER"),
    for ln in lanes:
        curr_prbs_cnt = float(gLaneStats[gSlice][ln][10])
        prbs_accum_time = gLaneStats[gSlice][ln][4] - gLaneStats[gSlice][ln][3]
        data_rate = gEncodingMode[gSlice][ln][1]
        bits_transferred = float((prbs_accum_time*data_rate*pow(10,9)))       
        if curr_prbs_cnt==0 or bits_transferred==0: 
            ber_val = 0
            print("%8d" % (ber_val) ),
        else:
            ber_val= curr_prbs_cnt / bits_transferred
            if(gLaneStats[gSlice][ln][8] == 'RDY'):
                print("%8.1e" % (ber_val) ),
            else:
                print (skip)

    print (line)
    
    return myber
def rx_monitor_summary (rst=None, slice='all', lane='all', fec_thresh=gFecThresh):
    global gLaneStats  # [per tile][per group][per lane], [PrbsCount, PrbsCount-1,PrbsCount-2, PrbsRstTime, PrbsLastReadoutTime]
    slices = get_slice_list(slice)
    lanes = get_lane_list(lane)    
    get_lane_mode(lanes)
        
    if rst != None:
        print("\n..%sCollecting BER Statistics Summary"%('.Clearing and ' if rst==1 else '.'))
        for slc in slices:
            sel_slice(slc)
            rx_monitor (rst=rst, slice=slc, lane=lanes, fec_thresh=fec_thresh, print_en=0)
    else:
        print("\n...BER Statistics Summary")
    
    elapsed_time=gLaneStats[slices[0]][lanes[0]][4] - gLaneStats[slices[0]][lanes[0]][3] 
    print("\n...Elapsed Time: %1.1f sec" % (elapsed_time))

    pre_fec_ber_bins = [1e-4, 1e-5, 1e-6, 1e-7, 1e-8, 1e-9, 1e-10, 1e-11, 0 ]
    pre_fec_ber_cnt  = []
    pre_fec_ber_sl   = []
    pre_fec_ber_ln   = []
    post_fec_ber_cnt = []
    post_fec_ber_sl  = []
    post_fec_ber_ln  = []
    lane_not_rdy_cnt = []
    lane_not_rdy_sl  = []
    lane_not_rdy_ln  = []
    for slc in slices:
        lane_not_rdy_cnt.append(0)
        post_fec_ber_cnt.append(0)
        pre_fec_ber_cnt.append([])
        for bin in range(len(pre_fec_ber_bins)):
            pre_fec_ber_cnt[slc].append(0)
            
    for slc in slices:
        for ln in lanes:
            if(gLaneStats[slc][ln][8] == 'RDY'):
                #### PRBS BER binning
                curr_prbs_cnt = float(gLaneStats[slc][ln][0])
                prbs_accum_time = gLaneStats[slc][ln][4] - gLaneStats[slc][ln][3]
                data_rate = gEncodingMode[slc][ln][1]
                bits_transferred = float((prbs_accum_time * data_rate * pow(10, 9)))
                if curr_prbs_cnt == 0 or bits_transferred == 0:
                    pre_fec_ber_cnt[slc][-1] += 1 # if clean PRBS BER, add to last bin
                else:
                    ber_val = curr_prbs_cnt / bits_transferred
                    ber_bin = next(x for x, val in enumerate(pre_fec_ber_bins) if ber_val >= val)
                    pre_fec_ber_cnt[slc][ber_bin] += 1
                    if ber_bin == 0: 
                        pre_fec_ber_sl.append(slc)                    
                        pre_fec_ber_ln.append(ln)                    

                #### Non-zero Post-FEC Cnt         
                if gLaneStats[slc][ln][10] > 0:
                    post_fec_ber_cnt[slc] += 1
                    post_fec_ber_sl.append(slc)                    
                    post_fec_ber_ln.append(ln)                    
            #### lane NOT_RDY cnt           
            else:
                lane_not_rdy_cnt[slc] += 1
                lane_not_rdy_sl.append(slc)                    
                lane_not_rdy_ln.append(ln)                    

    separator="\n +--------------+--------+--------+--------+"
    print (separator)
    print("\n | Metric       | Slice0 | Slice1 | Total  |"),
    print (separator)

    print("\n | Link Not Rdy |"),
    all_slices_cnt=0
    for slc in slices:
        print("%4d %s|"%(lane_not_rdy_cnt[slc],'<<' if lane_not_rdy_cnt[slc] !=0 else '  ' )),
        all_slices_cnt+=lane_not_rdy_cnt[slc]
    print("%4d %s|"%(all_slices_cnt,'<<' if all_slices_cnt !=0 else '  ' )),
    for sl_ln in range(len(lane_not_rdy_sl)):
        print("[S%d_%s]"%(lane_not_rdy_sl[sl_ln],lane_name_list[lane_not_rdy_ln[sl_ln]])),
        if sl_ln > 8: break # show up to 8 bad lanes
        
    for bin in range(len(pre_fec_ber_bins)):
        print("\n | > %1.1e    |"%(pre_fec_ber_bins[bin])),
        all_slices_cnt=0
        for slc in slices:
            print("%4d %s|"%(pre_fec_ber_cnt[slc][bin],'<<' if bin==0 and pre_fec_ber_cnt[slc][bin] !=0 else '  ' )),
            all_slices_cnt+=pre_fec_ber_cnt[slc][bin]
        print("%4d %s|"%(all_slices_cnt,'<<' if bin==0 and all_slices_cnt !=0 else '  ' )),
        if bin==0:
            for sl_ln in range(len(pre_fec_ber_sl)):
                print("[S%d_%s]"%(pre_fec_ber_sl[sl_ln],lane_name_list[pre_fec_ber_ln[sl_ln]])),
                if sl_ln > 8: break # show up to 8 bad lanes

    print("\n | Post-FEC (%2d)|"%gFecThresh),
    all_slices_cnt=0
    for slc in slices:
        print("%4d %s|"%(post_fec_ber_cnt[slc],'<<' if post_fec_ber_cnt[slc] !=0 else '  ')),
        all_slices_cnt+=post_fec_ber_cnt[slc]
    print("%4d %s|"%(all_slices_cnt,'<<' if all_slices_cnt !=0 else '  ' )),
    for sl_ln in range(len(post_fec_ber_sl)):
        print("[S%d_%s]"%(post_fec_ber_sl[sl_ln],lane_name_list[post_fec_ber_ln[sl_ln]])),
        if sl_ln > 8: break # show up to 8 bad lanes
    print (separator)
             
#################################################################################################### 
def rx_monitor_log (linecard_num=0,phy_num=0,slice_num=None,lane=None, rst=0, adapt_time=-1, header=1, logfile='rx_monitor_log.txt'): 
    global gLaneStats #[per Slice][per lane], [PrbsCount, PrbsCount-1,PrbsCount-2, PrbsRstTime, PrbsLastReadoutTime]
    if slice_num is None: 
        slice_num = gSlice
    post_fec_ber={}

   
    lanes = get_lane_list(lane)
    get_lane_mode(lanes)
 
    rx_monitor_capture(lane=lanes)
    
    log_file = open(logfile, 'a+')
    if header:
        ##### First Line of Header
        log_file.write("\nLc/Ph/Sl,Adapt,  BER"),
        for ln in lanes:  
            log_file.write(",%7s" %(lane_name_list[ln])),   # post-FEC BER            
        for ln in lanes:  
            log_file.write(",%7s" %(lane_name_list[ln])),   #  pre-FEC BER 
        for ln in lanes:  
            if(gEncodingMode[gSlice][ln][0].upper()== 'PAM4'):
                log_file.write(",%4s" %(lane_name_list[ln])),   # PAM4 Eye1
                log_file.write(",%4s" %(lane_name_list[ln])),   # PAM4 Eye2   
                log_file.write(",%4s" %(lane_name_list[ln])),   # PAM4 Eye3
            else:
                log_file.write(",%4s" %(lane_name_list[ln])),   # NRZ Eye
            
        ##### Second Line of Header
        log_file.write("\nLc/Ph/Sl, Time, Time"),
        for ln in lanes:  
            log_file.write(",Fec-BER"), # post-FEC BER         
        for ln in lanes:  
            log_file.write(",Raw-BER"), #  pre-FEC BER 
        for ln in lanes:  
            if(gEncodingMode[gSlice][ln][0].upper()== 'PAM4'):
                log_file.write(",Eye1"),    # PAM4 Eye1
                log_file.write(",Eye2" ),   # PAM4 Eye2   
                log_file.write(",Eye3" ),   # PAM4 Eye3
            else:
                log_file.write(", Eye"),    # NRZ Eye


    ##### Third Line is data, Linecard/Phy/Slice
    log_file.write("\n%2d/%2d/%2d" %(linecard_num,phy_num,slice_num)),   
    log_file.write(",%5.1f"%(adapt_time)),
    ber_time=gLaneStats[gSlice][lanes[0]][4] - gLaneStats[gSlice][lanes[0]][3]
    log_file.write(",%5.0f"% (ber_time) ),            # BER Collection time
    
    ##### Third Line is data, post-FEC BER
    for ln in lanes:
        if(gLaneStats[gSlice][ln][8] == 'RDY'):                
            curr_prbs_cnt = float(gLaneStats[gSlice][ln][10])
            prbs_accum_time = gLaneStats[gSlice][ln][4] - gLaneStats[gSlice][ln][3]
            data_rate = gEncodingMode[gSlice][ln][1]
            bits_transferred = float((prbs_accum_time*data_rate*pow(10,9)))       

            if curr_prbs_cnt==0 or bits_transferred==0: 
                ber_val = 0
                log_file.write(",%7d" % (ber_val) ), # post-FEC BER
            else:
                ber_val= curr_prbs_cnt / bits_transferred
                log_file.write(",%7.1e" % (ber_val) ),# post-FEC BER
            post_fec_ber[ln] = ber_val
        else: # Lane's PHY RDY = 0
            log_file.write(",      -"),  # post-FEC BER
            
    ##### Third Line is data, pre-FEC BER
    for ln in lanes:
        if(gLaneStats[gSlice][ln][8] == 'RDY'):             
            curr_prbs_cnt = float(gLaneStats[gSlice][ln][0])
            prbs_accum_time = gLaneStats[gSlice][ln][4] - gLaneStats[gSlice][ln][3]
            data_rate = gEncodingMode[gSlice][ln][1]
            bits_transferred = float((prbs_accum_time*data_rate*pow(10,9)))       
            if curr_prbs_cnt==0 or bits_transferred==0: 
                ber_val = 0
                log_file.write(",%7d" % (ber_val) ), # pre-FEC BER
            else:
                ber_val= curr_prbs_cnt / bits_transferred
                log_file.write(",%7.1e" % (ber_val) ), # pre-FEC BER
        else: # Lane's PHY RDY = 0
            log_file.write(",      -"),  # pre-FEC BER

    ##### Third Line is data, EYE MARGINS
    for ln in lanes:
        if(gLaneStats[gSlice][ln][8] == 'RDY'):
            if(gEncodingMode[gSlice][ln][0].upper()== 'PAM4'):
                log_file.write (",%4.0f"%(gLaneStats[gSlice][ln][5])),  # Eye1 
                log_file.write (",%4.0f"%(gLaneStats[gSlice][ln][6])),  # Eye2
                log_file.write (",%4.0f"%(gLaneStats[gSlice][ln][7])),  # Eye3
            else:
                log_file.write (",%4.0f"%(gLaneStats[gSlice][ln][5])),  # Eye
        else: # Lane's PHY RDY = 0
            if(gEncodingMode[gSlice][ln][0].upper()== 'PAM4'):
                log_file.write(",   -"),  # PAM4 Eye1
                log_file.write(",   -"),  # PAM4 Eye2   
                log_file.write(",   -"),  # PAM4 Eye3   
            else:
                log_file.write(",   -"),  # NRZ Eye
            

    if header:
        log_file.write("\n")
    log_file.close()
    return post_fec_ber , ber_time
#################################################################################################### 
def rx_monitor(rst=0, lane=None,  slice=None, fec_thresh=gFecThresh, print_en=1, returnon = 0, mode='PRBS31'):

    lanes = get_lane_list(lane)
        
    if (rst>0): 
        if rst>1:
            print("BER Accumulation Time: %1.1f sec"%(float(rst)))
        rx_monitor_clear(lane=lanes,fec_thresh=fec_thresh, mode=mode)
        if rst>1:
            time.sleep(float(rst-(len(lanes)*0.05)))
    else: # rst=0
        # in case any of the lanes' PRBS not cleared or FEC Analyzer not initialized, clear its Stats first
        for ln in lanes:
            if gLaneStats[gSlice][ln][3]==0 or gLaneStats[gSlice][ln][8] != 'RDY':
                rx_monitor_clear(lane=ln,fec_thresh=fec_thresh)
                #print("\nSlice %d Lane %s RX_MONITOR Initialized First!"%(gSlice,lane_name_list[ln])),
        
    rx_monitor_capture(lane=lane)
    if print_en:
        mydata = rx_monitor_print(lane=lane,slice=slice)
    if returnon ==1: return mydata
####################################################################################################    
def ber(rst=0,lane=None, t=None,fec_thresh=gFecThresh):

    lanes = get_lane_list(lane)
        
    if (rst>0): 
        if rst>1:
            print("BER Accumulation Time: %1.1f sec"%(float(rst)))
        rx_monitor_clear(lane=lanes,fec_thresh=fec_thresh)
        if rst>1:
            time.sleep(float(rst-(len(lanes)*0.05)))
    else: # rst=0
        # in case any of the lanes' PRBS not cleared or FEC Analyzer not initialized, clear its Stats first
        for ln in lanes:
            if gLaneStats[gSlice][ln][3]==0 or gLaneStats[gSlice][ln][8] != 'RDY':
                rx_monitor_clear(lane=ln,fec_thresh=fec_thresh)
                #print("\nSlice %d Lane %s RX_MONITOR Initialized First!"%(gSlice,lane_name_list[ln])),
        
    if (t!=None):
        time.sleep(t)
    rx_monitor_capture(lane=lane)
    result={}
    for ln in lanes:
        prbs_accum_time = gLaneStats[gSlice][ln][4] - gLaneStats[gSlice][ln][3]
        data_rate = gEncodingMode[gSlice][ln][1]
        bits_transferred = float((prbs_accum_time*data_rate*pow(10,9)))       
        
        prbs_err_cnt = float(gLaneStats[gSlice][ln][0])
        #pre_fec_cnt  = float(gLaneStats[gSlice][ln][9])
        post_fec_cnt = float(gLaneStats[gSlice][ln][10])
        if(gLaneStats[gSlice][ln][8] != 'RDY'):
            prbs_ber = 1
            #pre_fec_ber = 1
            post_fec_ber = 1
        else:
            #### PRBS BER
            if prbs_err_cnt==0 or bits_transferred==0: 
                prbs_ber = 0
            else:
                prbs_ber= prbs_err_cnt / bits_transferred
            #### Pre-FEC BER
            #if pre_fec_cnt==0 or bits_transferred==0: 
                #pre_fec_ber = 0
            #else:
                #pre_fec_ber= pre_fec_cnt / bits_transferred
            #### Post-FEC BER
            if post_fec_cnt==0 or bits_transferred==0: 
                post_fec_ber = 0
            else:
                post_fec_ber= post_fec_cnt / bits_transferred

        result[ln] = prbs_ber, post_fec_ber
    return result
####################################################################################################    
def ber(rst=0,lane=None, t=None,fec_thresh=gFecThresh):

    lanes = get_lane_list(lane)
        
    if (rst>0): 
        if rst>1:
            print("BER Accumulation Time: %1.1f sec"%(float(rst)))
        rx_monitor_clear(lane=lanes,fec_thresh=fec_thresh)
        if rst>1:
            time.sleep(float(rst-(len(lanes)*0.05)))
    else: # rst=0
        # in case any of the lanes' PRBS not cleared or FEC Analyzer not initialized, clear its Stats first
        for ln in lanes:
            if gLaneStats[gSlice][ln][3]==0 or gLaneStats[gSlice][ln][8] != 'RDY':
                rx_monitor_clear(lane=ln,fec_thresh=fec_thresh)
                #print("\nSlice %d Lane %s RX_MONITOR Initialized First!"%(gSlice,lane_name_list[ln])),
        
    if (t!=None):
        time.sleep(t)
    rx_monitor_capture(lane=lane)
    result={}
    for ln in lanes:
        prbs_accum_time = gLaneStats[gSlice][ln][4] - gLaneStats[gSlice][ln][3]
        data_rate = gEncodingMode[gSlice][ln][1]
        bits_transferred = float((prbs_accum_time*data_rate*pow(10,9)))       
        
        prbs_err_cnt = float(gLaneStats[gSlice][ln][0])
        #pre_fec_cnt  = float(gLaneStats[gSlice][ln][9])
        post_fec_cnt = float(gLaneStats[gSlice][ln][10])
        if(gLaneStats[gSlice][ln][8] != 'RDY'):
            prbs_ber = 1
            #pre_fec_ber = 1
            post_fec_ber = 1
        else:
            #### PRBS BER
            if prbs_err_cnt==0 or bits_transferred==0: 
                prbs_ber = 0
            else:
                prbs_ber= prbs_err_cnt / bits_transferred
            #### Pre-FEC BER
            #if pre_fec_cnt==0 or bits_transferred==0: 
                #pre_fec_ber = 0
            #else:
                #pre_fec_ber= pre_fec_cnt / bits_transferred
            #### Post-FEC BER
            if post_fec_cnt==0 or bits_transferred==0: 
                post_fec_ber = 0
            else:
                post_fec_ber= post_fec_cnt / bits_transferred

        result[ln] = prbs_ber, post_fec_ber
    return result
####################################################################################################    
def ber_test(rst=0,lane=None, t=None, nrz_limit=0, pam4_limit=1e-6 ):

    pre_fec_ber_limit = 0
    post_fec_ber_limit = 0
    result={}
    lanes = get_lane_list(lane)
    ber_data = ber(lane, rst, t)
    for ln in lanes:        
        pre_fec_ber_limit = nrz_limit if lane_mode_list[ln].lower() == 'nrz' else pam4_limit
        if (ber_data[ln][0] > pre_fec_ber_limit) or (ber_data[ln][1] > post_fec_ber_limit):
                result[ln]= 'Fail'
        else:
            result[ln]= 'Pass'
    return result
####################################################################################################    
# copy_lane(source_lane, destination_lane)
#
# This function copies one lanes register to another lane, addr by addr
####################################################################################################    
def copy_lane(lane1, lane2):

    first_addr=0x000
    last_addr =0x1FF
     
    for addr in range(first_addr, last_addr+1, +1):
        val=rreg(addr,lane1)
        wreg(addr, val,lane2)   
        
    if lane2%2: # destination lane is an ODD lane
        set_bandgap(bg_val=7,lane=lane2)
    else:      # destination lane is an Even lane
        set_bandgap(bg_val=2,lane=lane2)

    lr(lane=lane2)
    print("\n....Slice %d Lane %s all registers copied to Lane %s "%(gSlice,lane_name_list[lane1],lane_name_list[lane2]))

####################################################################################################
# get_top_pll ()
#
# reads TOP-PLL registers and computes TOP-PLL Frequencies 
#
# returns all parameters for the specified top pll
#
####################################################################################################    
def get_top_pll ():

    top_pll_params = {}
    
    ext_ref_clk=156.25
    
    ## TOP PLL A (0x9500) & TOP PLL B (0x9600)
    # top_pll_en_refclk_addr = [0x9500, [7]]
    # top_pll_pu_addr = [0x9501, [2]]
    # top_pll_div4_addr = [0x9500, [6]]
    # top_pll_n_addr = [0x9507, [12,4]]
    # top_pll_lvcocap_addr = [0x9501, [12,6]] 
    # top_pll_refclk_div_addr = [0x9513, [15,7]]
    # top_pll_div2_addr = [0x9513, [6]]

    for pll_idx in [0,1]:
        div4_en     = rregBits(c.top_pll_div4_addr[0]+(pll_idx*0x100)       ,c.top_pll_div4_addr[1]   )
        div2_bypass = rregBits(c.top_pll_div2_addr[0]+(pll_idx*0x100)       ,c.top_pll_div2_addr[1]   )
        refclk_div  = rregBits(c.top_pll_refclk_div_addr[0]+(pll_idx*0x100) ,c.top_pll_refclk_div_addr[1])
        pll_n       = rregBits(c.top_pll_n_addr[0]+(pll_idx*0x100)          ,c.top_pll_n_addr[1]      )
        pll_cap     = rregBits(c.top_pll_lvcocap_addr[0]+(pll_idx*0x100)    ,c.top_pll_lvcocap_addr[1])
      
        div_by_4 = 1.0 if div4_en==0     else 4.0
        mul_by_2 = 1.0 if div2_bypass==1 else 2.0
       
        fvco = (ext_ref_clk * float(pll_n) *2.0 * mul_by_2) / (div_by_4 * float(refclk_div) * 8.0) # in MHz 
    
        top_pll_params[pll_idx] = fvco, pll_cap, pll_n, div4_en, refclk_div, div2_bypass, ext_ref_clk
        
    return top_pll_params    #data_rate, fvco, pll_cap, pll_n, div4, div2, ext_ref_clk
####################################################################################################
# 
# Top PLL Setup Configuration
####################################################################################################
def set_top_pll(pll_side='both', freq=195.3125):

    global gRefClkFreq; gRefClkFreq=195.3125
    
    if pll_side is None: pll_side='both'
    if type(pll_side)==int: 
        if pll_side==0: pll_side='A'
        else: pll_side='B'
    if type(pll_side)==str: pll_side=pll_side.upper()

   
    ##### If FW already loaded and pll_top_cal done, then save the cap values to restore them
    #if fw_loaded(print_en=0):
    top_pll_A_cap = rreg([0x9501,[12,6]])
    top_pll_B_cap = rreg([0x9601,[12,6]])

    ##### Top PLL B-side lanes
    if pll_side == 'A' or pll_side == 'BOTH':
        wreg(0x9500, 0x1aa0)
        wreg(0x9500, 0x0aa0) # [12]=0, do not bypass 1p7 regulator
        wreg(0x9501, 0x8a5b) # Top PLL A powered down while programming it, bit [2]=0
        wreg(0x9502, 0x03e6) # 0x03b6
        wreg(0x9503, 0x6d86)
        wreg(0x9504, 0x7180)
        wreg(0x9505, 0x9000)
        wreg(0x9506, 0x0000)
        wreg(0x9507, 0x0500)
        wreg(0x950a, 0x4040)
        wreg(0x950b, 0x0000)
        wreg(0x950d, 0x0000)
        wreg(0x9510, 0xb670)
        wreg(0x9511, 0x0000)
        wreg(0x9512, 0x7de8)
        wreg(0x9513, 0x0840)
        
        wreg(0x9512, 0xfde8)
        wreg(0x9501, 0x8a5f) # Top PLL A on after programming it, bit [2]=1
        
        wreg(0x9501, 0x89df) # default top_pll_A_cap = 0x27        
        if fw_loaded(print_en=0):      # if FW loaded, restore cal'ed value for top_pll_A_cap
            wreg([0x9501,[12,6]], top_pll_A_cap)

        if type(freq)==str and freq.upper()=='OFF':
            wregBits(0x9501, [2], 0)
            
    ##### Top PLL B-side lanes   
    if pll_side == 'B' or pll_side == 'BOTH':   
        wreg(0x9600, 0x1aa0)
        wreg(0x9600, 0x0aa0) # [12]=0, do not bypass 1p7 regulator
        wreg(0x9601, 0x8a5b) # Top PLL B powered down while programming it, bit [2]=0
        wreg(0x9602, 0x03e6) # 0x03b6
        wreg(0x9603, 0x6d86)
        wreg(0x9604, 0x7180)
        wreg(0x9605, 0x9000)
        wreg(0x9606, 0x0000)
        wreg(0x9607, 0x0500)
        wreg(0x960a, 0x4040)
        wreg(0x960b, 0x0000)
        wreg(0x960d, 0x0000)
        wreg(0x9610, 0xb670)
        wreg(0x9611, 0x0000)
        wreg(0x9612, 0x7de8)
        wreg(0x9613, 0x0840)
        
        wreg(0x9612, 0xfde8)
        wreg(0x9601, 0x8a5f) # Top PLL A powered up after programming it, bit [2]=1
        
        wreg(0x9601, 0x89df) # default top_pll_B_cap = 0x27        
        if fw_loaded(print_en=0):      # if FW loaded, restore cal'ed value for top_pll_B_cap
            wreg([0x9601,[12,6]], top_pll_B_cap)
            
        if type(freq)==str and freq.upper()=='OFF':
            wregBits(0x9601, [2], 0)
            
####################################################################################################
def set_top_pll_bypass(pll_side='both', freq=195.3125):

    global gRefClkFreq; gRefClkFreq=195.3125
    
    if pll_side is None: pll_side='both'
    if type(pll_side)==int: 
        if pll_side==0: pll_side='A'
        else: pll_side='B'
    if type(pll_side)==str: pll_side=pll_side.upper()
    
    ##### If FW already loaded and pll_top_cal done, then save the cap values to restore them
    if fw_loaded(print_en=0):
        top_pll_A_cap = rreg([0x9501,[12,6]])
        top_pll_B_cap = rreg([0x9601,[12,6]])

    ##### Top PLL B-side lanes
    if pll_side == 'A' or pll_side == 'BOTH':
        wreg(0x9500, 0x1a20) # Bypass Top PLL A
        wreg(0x9500, 0x0a20) # [12]=0, do not bypass 1p7 regulator
        #wreg(0x9500, 0x1aa0)
        #wreg(0x9500, 0x0aa0) # [12]=0, do not bypass 1p7 regulator
        wreg(0x9501, 0x8a5b) # Top PLL A poewred down while programming it, bit [2]=0
        wreg(0x9502, 0x03e6) # 0x03b6
        wreg(0x9503, 0x6d86)
        wreg(0x9504, 0x7180)
        wreg(0x9505, 0x9000)
        wreg(0x9506, 0x0000)
        wreg(0x9507, 0x0500)
        wreg(0x950a, 0x4040)
        wreg(0x950b, 0x0000)
        wreg(0x950d, 0x0000)
        wreg(0x9510, 0xb670)
        wreg(0x9511, 0x0000)
        wreg(0x9512, 0x7de8)
        wreg(0x9513, 0x0840)
        
        wreg(0x9512, 0xfde8)
        wreg(0x9501, 0x8a5f) # Top PLL A on after programming it, bit [2]=1
        
        wreg(0x9501, 0x89df) # default top_pll_A_cap = 0x27        
        if fw_loaded(print_en=0):      # if FW loaded, restore cal'ed value for top_pll_A_cap
            wreg([0x9501,[12,6]], top_pll_A_cap)

        if type(freq)==str and freq.upper()=='OFF':
            wregBits(0x9501, [2], 0)
            
    ##### Top PLL B-side lanes   
    if pll_side == 'B' or pll_side == 'BOTH':
        wreg(0x9600, 0x1a20) 
        wreg(0x9600, 0x0a20) # [12]=0, do not bypass 1p7 regulator    
        #wreg(0x9600, 0x1aa0)
        #wreg(0x9600, 0x0aa0) # [12]=0, do not bypass 1p7 regulator
        wreg(0x9601, 0x8a5b) # Top PLL B powered down while programming it, bit [2]=0
        wreg(0x9602, 0x03e6) # 0x03b6
        wreg(0x9603, 0x6d86)
        wreg(0x9604, 0x7180)
        wreg(0x9605, 0x9000)
        wreg(0x9606, 0x0000)
        wreg(0x9607, 0x0500)
        wreg(0x960a, 0x4040)
        wreg(0x960b, 0x0000)
        wreg(0x960d, 0x0000)
        wreg(0x9610, 0xb670)
        wreg(0x9611, 0x0000)
        wreg(0x9612, 0x7de8)
        wreg(0x9613, 0x0840)
        
        wreg(0x9612, 0xfde8)
        wreg(0x9601, 0x8a5f) # Top PLL A powered up after programming it, bit [2]=1
        
        wreg(0x9601, 0x89df) # default top_pll_B_cap = 0x27        
        if fw_loaded(print_en=0):      # if FW loaded, restore cal'ed value for top_pll_B_cap
            wreg([0x9601,[12,6]], top_pll_B_cap)
            
        if type(freq)==str and freq.upper()=='OFF':
            wregBits(0x9601, [2], 0)

##############################################################################
def fec_reset(fec_list=[0,1,2,3,4,5,6,7]):

    for fec_idx in fec_list:
        wreg([0x9857,   [fec_idx]], 1) # FEC_x enabled
        time.sleep(0.01)
        wreg([fec_class3[fec_idx]+0x80,[7,6]], 0x3) # Reset FEC x
        time.sleep(0.01)
        wreg([fec_class3[fec_idx]+0x80,[11,8]], 0xf) # FEC_A: TX forced to value 1, RX forced to reset
        wreg([fec_class3[fec_idx]+0x80,[11,8]], 0xf) # FEC_B: TX forced to value 1, RX forced to reset
        wreg([0x9857,   [fec_idx]], 0) # FEC_x disabled
  
    #wreg(0x9858, 0x0000) # clear FEC-to-SerdesLanes Mapping
       
  
####################################################################################################
# 
# Top PLL Setup Configuration
####################################################################################################
def set_bandgap(bg_val=None, lane=None):
   
    lanes = get_lane_list(lane)

    if bg_val!=None:
        for lane in lanes:
            if type(bg_val) == int:
                bg_val = max(0, min(bg_val, 7))
                wregBits(0x0ff,[12],1,lane) # power up TX BG
                wregBits(0x1ff,[12],1,lane) # power up RX BG
                wregBits(0x0ff,[15,13],bg_val,lane)
                wregBits(0x1ff,[15,13],bg_val,lane)
            elif bg_val.upper()=='OFF':     # power down BG
                wregBits(0x0ff,[12],0,lane)
                wregBits(0x1ff,[12],0,lane)
            else: #if bg_val.upper()=='ON': # power up BG, leave BG value as is
                wregBits(0x0ff,[12],1,lane)
                wregBits(0x1ff,[12],1,lane)
                
    if bg_val is None:
        tx_bg_pu  =[] 
        rx_bg_pu  =[] 
        tx_bg_val =[]                
        rx_bg_val =[]                
        for lane in range(0,len(lane_name_list)):
            tx_bg_pu.append (rreg([0x0ff,   [12]],lane)) # TX BG powered up/down?
            rx_bg_pu.append (rreg([0x1ff,   [12]],lane)) # RX BG powered up/down?
            tx_bg_val.append(rreg([0x0ff,[15,13]],lane)) # TX BG value
            rx_bg_val.append(rreg([0x1ff,[15,13]],lane)) # RX BG value

        print ("Lanes    :",lanes)
        print ("TX BG PU :",tx_bg_pu)
        print ("RX BG PU :",rx_bg_pu)
        print ("TX BG Val:",tx_bg_val)
        print ("RX BG Val:",rx_bg_val)

####################################################################################################
# Program Clock Output Test Points (CLKOUT0 +/- DIFF, CLKOUT1 single-ended or CLKOUT2 single-ended)
#
#
####################################################################################################
def set_clock_out(clkout='???', clkdiv=32, lane=0,print_en=0):
   
    CLKOUT_REG = [0x98D5,0x98D6,0x98D7,0x98D8,0x98D9]
    clkout_div_list=[32,64,128,256,512,1024,2048,4096]
    if clkdiv in clkout_div_list:
        div_index = clkout_div_list.index(clkdiv)
    else:
        div_index=0
        clkdiv=clkout_div_list[div_index]
    
    ###### CLKOUT0 Differential output pins
    if clkout.lower() == '0': 
        wreg([CLKOUT_REG[0],[ 3, 0]],lane)      # select target lane's clock to send to clock out 0
        wreg([CLKOUT_REG[1],[   12]],1)         # enable clock out 0 mux
        wreg([CLKOUT_REG[1],[ 2, 0]],div_index) # select clock out 0 divider
        wreg([CLKOUT_REG[3],[14,13]],3)         # enable clock out main driver
        wreg([CLKOUT_REG[3],[11, 8]],0xF)       # enable clock out 0 driver
        wreg([CLKOUT_REG[4],[13, 0]],0x3FFF)    # power up bandgap for clock out driver
        pll_params = get_lane_pll(lane)
        data_rate=pll_params[lane][0][0]
        fvco=pll_params[lane][0][1]
        fclkout=float(1000*fvco/float(clkdiv))        
        print("\n...Turned On CLKOUT0 Diff pins, PLL Lane: %d, ClkOutDivider: %d, BitRate: %2.5f Gbps, PLL: %2.5f GHz, FclkOut0: %2.5f MHz"%(lane,clkdiv,data_rate,fvco,fclkout))
        
    ###### CLKOUT1 single-ended output pin
    elif clkout.lower() == '1': 
        wreg([CLKOUT_REG[0],[ 7, 4]],lane)      # select target lane's clock to send to clock out 1
        wreg([CLKOUT_REG[1],[   13]],1)         # enable clock out 1 mux
        wreg([CLKOUT_REG[1],[ 5, 3]],div_index) # select clock out 1 divider
        wreg([CLKOUT_REG[3],[14,13]],3)         # enable clock out main driver
        wreg([CLKOUT_REG[3],[ 7, 4]],0xF)       # enable clock out 1 driver
        wreg([CLKOUT_REG[4],[13, 0]],0x3FFF)    # power up bandgap for clock out driver
        pll_params = get_lane_pll(lane)
        data_rate=pll_params[lane][0][0]
        fvco=pll_params[lane][0][1]
        fclkout=float(1000*fvco/float(clkdiv))        
        print("\n...Turned On CLKOUT1 pin, PLL Lane: %d, ClkOutDivider: %d, BitRate: %2.5f Gbps, PLL: %2.5f GHz, FclkOut1: %2.5f MHz"%(lane,clkdiv,data_rate,fvco,fclkout))
    ###### CLKOUT2 single-ended output pin
    elif clkout.lower() == '2': 
        wreg([CLKOUT_REG[0],[11, 8]],lane)      # select target lane's clock to send to clock out 2
        wreg([CLKOUT_REG[1],[   14]],1)         # enable clock out 2 mux
        wreg([CLKOUT_REG[1],[ 8, 6]],div_index) # select clock out 2 divider
        wreg([CLKOUT_REG[3],[14,13]],3)         # enable clock out main driver
        wreg([CLKOUT_REG[3],[ 3, 0]],0xF)       # enable clock out 2 driver
        wreg([CLKOUT_REG[4],[13, 0]],0x3FFF)    # power up bandgap for clock out driver
        pll_params = get_lane_pll(lane)
        data_rate=pll_params[lane][0][0]
        fvco=pll_params[lane][0][1]
        fclkout=float(1000*fvco/float(clkdiv))        
        print("\n...Turned On CLKOUT2 pin, PLL Lane: %d, ClkOutDivider: %d, BitRate: %2.5f Gbps, PLL: %2.5f GHz, FclkOut2: %2.5f MHz"%(lane,clkdiv,data_rate,fvco,fclkout))
    elif clkout.lower() == 'off': 
        wreg(CLKOUT_REG[0],0x0000)             # back to the default lane 0 for all clock out pins
        wreg(CLKOUT_REG[1],0x0000)             # disable all clock out mux'es
        wreg(CLKOUT_REG[2],0x0000)             # default 
        wreg(CLKOUT_REG[3],0x0000)             # disable clock out main driver
        wreg(CLKOUT_REG[4],0x0000)             # power down bandgap for clock out driver
        print("\n...Turned Off ALL Three Clock Output Pins!")
    else:
        print("\n...Usage:")
        print("\n   set_clock_out (clkout='0', clkdiv=32, lane=0)"),
        print("\n   set_clock_out (clkout='1', clkdiv=32, lane=0)"),
        print("\n   set_clock_out (clkout='2', clkdiv=32, lane=0)"),
        print("\n   set_clock_out ('OFF') ")
        print("\n...Parameters:")
        print("\n   clkout : selects clock-out pin to program"),
        print("\n           '0'   :Outout to differential clock-out pins CKOP/CKON "),
        print("\n           '1'   :Output to Single-ended clock-out pin  CLK_OUT1  "),
        print("\n           '2'   :Output to Single-ended clock-out pin  CLK_OUT2  "),
        print("\n           'off' :Turn OFF all three clock-out pins")
        print("\n   clkdiv : selects clock-out divider (from target lane's PLL) "),
        print("\n            choices: 32,64,128,256,1024,2048 or 4096 ")
        print("\n   lane   : selects target lane's PLL as source for clock-out pin"),
        print("\n            choices: : 0 to 15")
    
    if print_en: reg([0x98D5,0x98D6,0x98D7,0x98D8,0x98D9])
####################################################################################################
# Read On-Chip Temperature Sensor by FIRMWARE
# 
# FW Register 170 contain die temp value. It is updated by FW every few mSec
#
####################################################################################################
def fw_temp_sensor(print_en=1):
    
    Yds = 237.7
    Kds = 79.925
    value = fw_reg_rd(170) # FW REG 170 contains temp sensor readback value by FW
    realVal = value*Yds/4096-Kds
    if print_en: print('FW Temp Sensor: %3.1f C'%(realVal)),
    else: return realVal
####################################################################################################
# Read On-Chip Temperature Sensor
# 
####################################################################################################
def temp_sensor_fast(sel=0):
    
    base = 0xb000
    Yds = 237.7
    Kds = 79.925
    addr= [base+0x39,base+0x3a,base+0x3b,base+0x3c]
    MdioWr(base+0x37, 0x2d) # set write ack. Do this to make the first read correct
    MdioWr(base+0x37, 0x0d) # set read with ack
    #time.sleep(1)
    rdy = MdioRd(base+0x38)&0x01
    time1=time.time()
    #time2=time.time()
    while(rdy==0): # wait for rdy
        rdy = MdioRd(base+0x38)&0x01
        time2=time.time()
        if (time2-time1)>=2:
            print ('TSensor Read-back Timed Out!')
            break

    value = MdioRd(addr[sel])
    realVal = value*Yds/4096-Kds
    #print('TempSensor: %3.1f C'%(realVal)),
    #print('tempsensor%d: %d,realVal:%f'%(sel,value,realVal))
    MdioWr(base+0x37, 0x2d) #write ack
    return realVal
####################################################################################################
# Read On-Chip Temperature Sensor
# 
####################################################################################################
def temp_sensor(print_en=True):
    temp_sensor_start()    
    value1=temp_sensor_read()
    if print_en: print('Device TempSensor: %3.1f C'%(value1)),
    return value1

####################################################################################################
# Set up On-Chip Temperature Sensor (to be read later)
# 
####################################################################################################
def temp_sensor_start():
    base = 0xB000 
    #MdioWr(0x81f7,0x4d80) #set tst0 value
    MdioWr(base+0x3e, 0x10d) #set clock
    time.sleep(0.1)
    MdioWr(base+0x37, 0x0) #reset sensor
    time.sleep(0.1)
    MdioWr(base+0x37, 0xc) #set no ack      
####################################################################################################
# Read back On-Chip Temperature Sensor (after it's been set up aready)
# 
####################################################################################################
def temp_sensor_read():
    base = 0xB000 
    sense_addr= base+0x39
    Yds = 237.7
    Kds = 79.925
    
    rdy = MdioRd(base+0x38)>>8 
    while(rdy==0): # wait for rdy
        rdy = MdioRd(base+0x38)>>8 
    value1 = MdioRd(sense_addr)*Yds/4096-Kds
    #print('%3.1f C'%(value1)),
    return value1
####################################################################################################    
# background_cal()  # returns status of current gLane
# background_cal(lane=11)  # returns status of current gLane
# background_cal('en', lane=11)  #  enable for lane 11 (B0)
# background_cal('dis',lane=11)  # disable for lane 11 (B0)
# background_cal()  # returns status of current gLane
# background_cal()  # returns status of current gLane
####################################################################################################    
def background_cal (enable=None,lane=None,print_en=1):

    lanes = get_lane_list(lane)

    get_lane_mode(lanes)
    background_cal_status=[]
    lane_cntr=0
    for ln in lanes:
        if gEncodingMode[gSlice][ln][0]=='pam4':  # Lane is in PAM4 mode
            c=Pam4Reg
        else:
            print("\nSlice %d Lane %s (NRZ) Background Cal: ***>> Lane is in NRZ Mode (No Background Cal!)\n"%(gSlice,lane_name_list[lane]))
            return

        # Asked to enable or disable background cal
        if enable!=None: 
            #### Clear BP2, Toggle BP1 and Continue to BP1 at state 0x12
            bp2(0,0x0,lane=ln)
            bp1(0,0x0,lane=ln)
            sm_cont_01(lane=ln)
            bp1(1,0x12,lane=ln) 
            wait_for_bp1_timeout=0
            # Wait for BP1 state 0x12 to be reached
            while True:
                if bp1(lane=ln)[ln][-1]: #BP1 state 0x12 is reached
                    break
                else:
                    wait_for_bp1_timeout+=1
                    if wait_for_bp1_timeout>5000:
                        if(print_en):print("\nSlice %d Lane %s (PAM4) Background Cal: ***>> Timed out waiting for BP1\n"%(gSlice,lane_name_list[ln]))
                        #save_setup('background_cal_timeout.txt')
                        break

            if enable=='en' or enable==1: # asked to enable background cal
                wreg ([0x4, [0]],  1, ln)
                wreg (0x077, 0x4e5c, ln)
                wreg (0x078, 0xe080, ln)
                wreg (c.rx_iter_s6_addr, 6, ln)
                wreg (c.rx_mu_ow_addr,   3, ln)
            else: # asked to disable background cal
                wreg (c.rx_iter_s6_addr, 1, ln)
                wreg (c.rx_mu_ow_addr,   0, ln)        
                #wreg (c.rx_iter_s6_addr, 1, ln)
                #wreg (c.rx_mu_ow_addr,   0, ln)
                
            bp1(0,0x12,lane=ln) # clear BP1
            sm_cont_01(lane=ln) # toggle SM Continue
            
        else:
            print_en=1
            
        # return Status of background cal for this lane
        if(print_en):print("\nSlice %d Lane %s (PAM4) Background Cal: "%(gSlice,lane_name_list[ln]))

        background_cal_status.append(rreg(c.rx_mu_ow_addr,ln) & 0x01)
        if background_cal_status[lane_cntr] == 0:
            if(print_en):print ("OFF")
        else:
            if(print_en):print ("ON")
        lane_cntr+=1 # next lane, if applicable
    
    return background_cal_status
####################################################################################################
# 
# DC_GAIN 
####################################################################################################
def dc_gain2(ctle_gain1=None, ctle_gain2=None, ffe_gain1=None, ffe_gain2=None, lane=None):
    
    if ctle_gain1=='?':
        print("\n ***> Usage: dc_gain(ctle_gain1, ffe_gain1, ctle_gain2, ffe_gain2, lane#)")
        ctle_gain1=None; ctle_gain2=None; ffe_gain1=None;  ffe_gain2=None; lane=None
        print("\n ***> Current settings shown below:")

    if lane is None: lane=gLane
    if type(lane)==int:     lanes=[lane]
    elif type(lane)==list:  lanes=list(lane)
    elif type(lane)==str and lane.upper()=='ALL': 
        lanes=range(0,len(lane_name_list))
        
    for lane in lanes:
        ## Convert from binary to Gray and write dc gain to registers
        if ctle_gain1 !=None:
            ctle_gain1_gray = Bin_Gray(ctle_gain1)
            wreg(c.rx_agcgain1_addr, ctle_gain1_gray, lane)
                
        if ctle_gain2 !=None:
            ctle_gain2_gray = Bin_Gray(ctle_gain2)
            wreg(c.rx_agcgain2_addr, ctle_gain2_gray, lane)
                
        if ffe_gain1 !=None:
            ffe_gain1_gray = Bin_Gray(ffe_gain1)
            wreg(c.rx_ffe_sf_msb_addr, ffe_gain1_gray, lane)
            
        if ffe_gain2 !=None:
            ffe_gain2_gray = Bin_Gray(ffe_gain2)
            wreg(c.rx_ffe_sf_lsb_addr, ffe_gain2_gray, lane)
        
        ##read DC gain from register and convert to binary from gray code
        ctle_gain1_bin = Gray_Bin (rreg(c.rx_agcgain1_addr, lane))
        ctle_gain2_bin = Gray_Bin (rreg(c.rx_agcgain2_addr, lane))
        ffe_gain1_bin  = Gray_Bin (rreg(c.rx_ffe_sf_msb_addr, lane))
        ffe_gain2_bin  = Gray_Bin (rreg(c.rx_ffe_sf_lsb_addr, lane))
    
    return ctle_gain1_bin, ctle_gain2_bin, ffe_gain1_bin, ffe_gain2_bin
####################################################################################################
# 
# set or get CTLE PEAKING index selection 
####################################################################################################
def ctle2(val = None, lane = None):
    lanes = get_lane_list(lane)
        
    for lane in lanes:
        if val != None:
            wreg(c.rx_agc_ow_addr, val, lane)
            wreg(c.rx_agc_owen_addr, 0x1, lane)
            
        val = rreg(c.rx_agc_ow_addr, lane)
        
    return val
####################################################################################################
# 
# set or get CTLE PEAKING 1/2 settings
####################################################################################################
def ctle_map2(sel = None, val1 = None, val2 = None, lane = None):

    lanes = get_lane_list(lane)

    for lane in lanes:
        if not None in (sel, val1, val2):
            if sel != 2 and sel != 5:
                val = (val1<<3) + val2
                if sel == 0:
                    map0 = (rreg(c.rx_agc_map0_addr, lane) | 0xfc00) & (val<<10 | 0x3ff) 
                    wreg(c.rx_agc_map0_addr, map0, lane)
                elif sel == 1:
                    map0 = (rreg(c.rx_agc_map0_addr, lane) | 0x03f0) & (val<<4 | 0xfc0f) 
                    wreg(c.rx_agc_map0_addr, map0, lane)
                elif sel == 3:
                    map1 = (rreg(c.rx_agc_map1_addr, lane) | 0x3f00) & (val<<8 | 0xc0ff) 
                    wreg(c.rx_agc_map1_addr, map1, lane)
                elif sel == 4:
                    map1 = (rreg(c.rx_agc_map1_addr, lane) | 0x00fc) & (val<<2 | 0xff03) 
                    wreg(c.rx_agc_map1_addr, map1, lane)
                elif sel == 6:
                    map2 = (rreg(c.rx_agc_map2_addr, lane) | 0x0fc0) & (val<<6 | 0xf03f) 
                    wreg(c.rx_agc_map2_addr, map2, lane)
                elif sel == 7:
                    map2 = (rreg(c.rx_agc_map2_addr, lane) | 0x003f) & (val | 0xffc0) 
                    wreg(c.rx_agc_map2_addr, map2, lane)
            elif sel == 2:
                val = (val1<<1) + (val2 & 0x4)
                map0 = (rreg(c.rx_agc_map0_addr, lane) | 0x000f) & (val | 0xfff0)
                wreg(c.rx_agc_map0_addr, map0, lane)
                val = val2 & 0x3
                map1 = (rreg(c.rx_agc_map1_addr, lane) | 0xc000) & (val | 0x3fff)
                wreg(c.rx_agc_map1_addr, map1, lane)
            else:# sel == 5:
                val = val1 & 0x6
                map1 = (rreg(c.rx_agc_map1_addr, lane) | 0x3) & (val | 0xfffc)
                wreg(c.rx_agc_map1_addr, map1, lane)
                val = ((val1 & 0x1)<<3) + val2
                map2 = (rreg(c.rx_agc_map2_addr, lane) | 0xf) & (val | 0x0fff)
                wreg(c.rx_agc_map2_addr, map2, lane)
                
        # Read CTLE Peaking table values for each/this lane
        map0 = rreg(c.rx_agc_map0_addr, lane)
        map1 = rreg(c.rx_agc_map1_addr, lane)
        map2 = rreg(c.rx_agc_map2_addr, lane)
        agc = { 0: [map0>>13, (map0>>10) & 0x7],
                1: [(map0>>7) & 0x7, (map0>>4) & 0x7],
                2: [(map0>>1) & 0x7, ((map0 & 0x1)<<2) + (map1>>14)],
                3: [(map1>>11) & 0x7, (map1>>8) & 0x7],
                4: [(map1>>5) & 0x7, (map1>>2) & 0x7],
                5: [((map1 & 0x3)<<1) + (map2>>15), (map2>>12) & 0x7],
                6: [(map2>>9) & 0x7, (map2>>6) & 0x7],
                7: [(map2>>3) & 0x7, map2 & 0x7]
                }
                
    if sel is None:
        for key in agc.keys():
            print (key, agc[key])
    else:
        return agc[sel]
####################################################################################################    
def sel_ctle_map(IL='ALL', lane=None):

    lanes = get_lane_list(lane)

    for lane in lanes:
        if IL == 'SR' or IL == 'ALL' :
            ctle_map(0,1,2,lane)  
            ctle_map(1,2,3,lane)  
            ctle_map(2,3,3,lane)  
            ctle_map(3,3,4,lane)  
            ctle_map(4,3,6,lane)  
            ctle_map(5,4,5,lane)  
            ctle_map(6,5,6,lane)  
            ctle_map(7,7,7,lane)  
        elif IL == 'MR':
            wreg(0x048,0x2518,lane) # PAM4 mode, ctle_map
            wreg(0x049,0x79eb,lane) # PAM4 mode, ctle_map
            wreg(0x04a,0xbf3f,lane) # PAM4 mode, ctle_map, CTLE7=(7,7)
        elif IL == 'VSR':
            wreg(0x048,0x2518,lane) # PAM4 mode, ctle_map
            wreg(0x049,0x79eb,lane) # PAM4 mode, ctle_map
            wreg(0x04a,0xbf3f,lane) # PAM4 mode, ctle_map, CTLE7=(7,7)
        
      
####################################################################################################
# 
# Set or get f1over3 
#
# (1) f13()                get f1over3 value of gLane
# (1) f13('all')           get f1over3 values of all lanes
# (2) f13(2)               set f1over3 value of gLane to 2
# (3) f13(10, [0,15])      set f1over3 values of lanes 0 and 15  to the same value of 10
# (4) f13(10, 'all')       set delta values of all lanes to the same value of 10
#
# returns: a list of f1over3 values, for the lane numbers passed
####################################################################################################
def f13(val=None, lane=None):

    lanes = get_lane_list(lane)

    if type(val)==str and val.upper()=='ALL': 
        lanes=range(0,len(lane_name_list))
        val=None
   
    val_list_out=[]# return values goes in here
    for lane in lanes:
        if val!=None: 
            val_to_write = (val<0) and (val+0x80) or val
            wreg(c.rx_f1over3_addr, val_to_write, lane)
            #if(print_en):print ('\nF1o3 write %d, read back %d' % (val, f13())),

        f1o3 = rreg(c.rx_f1over3_addr, lane) 
        val_list_out.append((f1o3>0x40) and (f1o3-0x80) or f1o3)
    
    return val_list_out

####################################################################################################
# 
# GET DFE Taps when FW not loaded
####################################################################################################
def sw_pam4_dfe(lane=None, print_en=0):
    global gPam4_En; gPam4_En=1
    lanes = get_lane_list(lane)
    result = {}
    for ln in lanes:
        get_lane_mode(ln)
        if lane_mode_list[ln] != 'pam4':
            result[ln] = -1,-1,-1
        else: # PAM4
            ths_list = []
            for val in range(12):
                wreg(c.rx_pam4_dfe_sel_addr, val, ln)
                time.sleep(0.01)
                readout = rreg(c.rx_pam4_dfe_rd_addr, ln)
                readout2 = rreg(c.rx_pam4_dfe_rd_addr, ln)
                if readout == readout2:
                    this_ths = ((float(readout)/2048.0)+1.0) % 2.0 - 1.0
                    if print_en: print(" %2d 0x%04X %6.3f"%(val,readout,this_ths))
                else:
                    readout = rreg(c.rx_pam4_dfe_rd_addr, ln)
                    this_ths = ((float(readout)/2048.0)+1.0) % 2.0 - 1.0
                    if print_en: print("*%2d 0x%04X %6.3f"%(val,readout,this_ths))
                ths_list.append(this_ths)
              
            f0 = (-3/16)*((ths_list[0]-ths_list[2])+(ths_list[3]-ths_list[5])+(ths_list[6]-ths_list[8])+(ths_list[9]-ths_list[11]))
            f1 = (-3/20)*((ths_list[0]+ths_list[1]+ths_list[2]-ths_list[9]-ths_list[10]-ths_list[11])+(1/3)*(ths_list[3]+ths_list[4]+ths_list[5]-ths_list[6]-ths_list[7]-ths_list[8]))
            f1_f0 = 0 if f0==0 else f1/f0
            result[ln] = f0, f1, f1_f0
        
    return result   
####################################################################################################
# 
# GET DFE Taps by the FW
####################################################################################################
def fw_pam4_dfe(lane=None,print_en=0):
    lanes = get_lane_list(lane)
    result = {}
    for ln in lanes:
        get_lane_mode(ln)
        if lane_mode_list[ln] != 'pam4':
            result[ln] = -1,-1,-1
        else: # PAM4
            readout = BE_info_signed(ln, 10, 1, 12)
            ths_list = []      
            for val in range(12):
                ths_list.append(((float(readout[val])/2048.0)+1.0) % 2.0 - 1.0)
                if print_en: print("%2d 0x%04X %6.3f"%(val,readout,result))
            
            f0 = (-3/16)*((ths_list[0]-ths_list[2])+(ths_list[3]-ths_list[5])+(ths_list[6]-ths_list[8])+(ths_list[9]-ths_list[11]))
            f1 = (-3/20)*((ths_list[0]+ths_list[1]+ths_list[2]-ths_list[9]-ths_list[10]-ths_list[11])+(1/3)*(ths_list[3]+ths_list[4]+ths_list[5]-ths_list[6]-ths_list[7]-ths_list[8]))
            f1_f0 = 0 if f0==0 else f1/f0
            result[ln] = f0, f1, f1_f0
        
    return result
####################################################################################################
# 
# GET DFE Taps by the FW if loaded, or by software
####################################################################################################
def pam4_dfe(lane=None,print_en=0):
    result = {}
    if fw_loaded(print_en=0):
        result = fw_pam4_dfe(lane,print_en)
    else:
        result = sw_pam4_dfe(lane,print_en)
    return result
####################################################################################################
# 
# GET DFE Taps, NRZ or PAM4
####################################################################################################
def dfe(lane=None,print_en=0):
    lanes = get_lane_list(lane)
    result = {}
    for ln in lanes:
        get_lane_mode(ln)
        if lane_mode_list[ln] == 'pam4':
            result[ln] = pam4_dfe(ln,print_en)
        else: # NRZ
            result[ln] = nrz_dfe(ln)
        
    return result
####################################################################################################
# 
# Set or get DELTA Phase
#
# (1) delta_ph()                get delta value of gLane
# (1) delta_ph('all')           get delta values of all lanes
# (2) delta_ph(2)               set delta value of gLane to 2
# (3) delta_ph(10, [0,15])      set delta values of lanes 0 and 15  to the same value of 10
# (4) delta_ph(10, 'all')       set delta values of all lanes to the same value of 10
#
# returns: a list of delta values, for the lane numbers passed
####################################################################################################
def delta_ph(val=None, lane=None):

    lanes = get_lane_list(lane)

    if type(val)==str and val.upper()=='ALL': 
        lanes=range(0,len(lane_name_list))
        val=None

    val_list_out=[]# return values goes in here
    for ln in lanes:
        get_lane_mode(ln)
        this_lane_mode = gEncodingMode[gSlice][ln][0]
        c = NrzReg if this_lane_mode == 'nrz' else Pam4Reg
        ######### PAM4 Mode
        if this_lane_mode.upper()=='PAM4':
            if val!=None:
                delta_to_write= (val<0) and (val+0x80) or val
                wreg(c.rx_delta_owen_addr, 1, ln)
                wreg(c.rx_delta_ow_addr, delta_to_write, ln)
                
            delta_val= rreg(c.rx_delta_ow_addr,  ln)
            #val_list_out.append((delta_val>0x40) and (delta_val-0x80) or delta_val)

        ######### NRZ Mode
        else: 
            if val!=None:
                wreg(c.rx_delta_addr, val, ln)                
            delta_val = rreg(c.rx_delta_addr,  ln)
                
        val_list_out.append((delta_val>0x40) and (delta_val-0x80) or delta_val)
        
    return val_list_out
        
####################################################################################################
# Initialize FEC Analyzer function
#
# This is a per-lane function
####################################################################################################
def fec_ana_init(lane=None, delay=.1, err_type=2, T=gFecThresh, M=10, N=5440, print_en=1, mode=None):
    lanes = get_lane_list(lane)

    FEC_ANA_BASE_ADDR = 0x1C0
    global gFecThresh
    
    if T!=None: gFecThresh=T
        
    for ln in lanes:
        thresh=T
        if gEncodingMode[gSlice][ln][0].lower() != 'pam4':
            if thresh>7: thresh=7 ## if lane is in NRZ mode, max T is 7 

        #err_type: tei ctrl teo ctrl
        wreg(FEC_ANA_BASE_ADDR+0x9, 0x6202,lane=ln) #set PRBS mode: force reload
        
        #wreg(FEC_ANA_BASE_ADDR+0x8, ( rreg(FEC_ANA_BASE_ADDR+0x8,lane=ln) | 0x0018), lane=ln) #set PRBS mode:PRBS31
        if mode == 'PRBS31':
            #print("\n PRBS31")
            wreg(FEC_ANA_BASE_ADDR+0x8, ( rreg(FEC_ANA_BASE_ADDR+0x8,lane=ln) | 0x0018), lane=ln) #set PRBS mode:PRBS31
        elif mode == 'PRBS13':
            #print("\n PRBS13")
            wreg(FEC_ANA_BASE_ADDR+0x8, ( rreg(FEC_ANA_BASE_ADDR+0x8,lane=ln) | 0x0008), lane=ln) #set PRBS mode:PRBS13
        elif mode == 'PRBS9':
            #print("\n PRBS9")
            wreg(FEC_ANA_BASE_ADDR+0x8, ( rreg(FEC_ANA_BASE_ADDR+0x8,lane=ln) | 0x0000), lane=ln) #set PRBS mode:PRBS9        
        elif mode == 'PRBS15':
            #print("\n PRBS15")
            wreg(FEC_ANA_BASE_ADDR+0x8, ( rreg(FEC_ANA_BASE_ADDR+0x8,lane=ln) | 0x0010), lane=ln) #set PRBS mode:PRBS15
        
        wreg(FEC_ANA_BASE_ADDR+0x0, ((rreg(FEC_ANA_BASE_ADDR+0x0,lane=ln) & 0xFFF0) | M),lane=ln)  #set M = 10
        wreg(FEC_ANA_BASE_ADDR+0x4, N,lane=ln) #set N = 5440
        wreg(FEC_ANA_BASE_ADDR+0x5, (rreg(FEC_ANA_BASE_ADDR+0x5,lane=ln) & 0xf007) | ((thresh<<7)+(thresh<<3)),lane=ln)
        wreg(FEC_ANA_BASE_ADDR+0x1, 0x000b,  lane=ln) #reset FEC_counter
        wreg(FEC_ANA_BASE_ADDR+0x1, 0x0003,  lane=ln) #release the reset 
        wreg(FEC_ANA_BASE_ADDR+0xc, err_type,lane=ln) #set TEO error type = 2 means bits in error
        wreg(FEC_ANA_BASE_ADDR+0xb, err_type,lane=ln) #set TEi error type = 2 means bits in error
        if print_en: print ('\n....Lane %s: FEC Analyzer Initialized' %(lane_name_list[ln]))
        
    #time.sleep(delay)
####################################################################################################
def fec_ana_read(lane=None):
    lanes = get_lane_list(lane)

    print ('\n....FEC Analyzer Status....'),
    print ('\nLane, RAW Errors, Uncorr Frames, Uncorr Symbols, Uncorr PRBS'),
    for ln in lanes:         
        tei = fec_ana_tei(lane=ln)
        teo = fec_ana_teo(lane=ln)
        #sei = fec_ana_sei(lane=ln)
        #bei = fec_ana_bei(lane=ln)
        #print ('\n%4s, %10d, %13d, %14d, %11d'%(lane_name_list[ln],tei,teo,sei,bei)),
        print ('\n%4s, %10d, %13d'%(lane_name_list[ln],tei,teo)),
    
####################################################################################################
def fec_ana_tei(lane=None):
    FEC_ANA_BASE_ADDR = 0x1C0
    if lane is None: lane=gLane
   
    wreg(FEC_ANA_BASE_ADDR+0xD,4,lane) #set reading data of TEi low 16 bit
    tei_l = rreg(FEC_ANA_BASE_ADDR+0x7,lane)#read data
    wreg(FEC_ANA_BASE_ADDR+0xD,5,lane) #set reading data of TEi high 16 bit
    tei_h = rreg(FEC_ANA_BASE_ADDR+0x7,lane)#read data
    tei = tei_h*65536+tei_l #combinate the data
    #print '\n....Lane %s: TEi counter: %d' %(lane_name_list[lane],tei),
    return tei 

def fec_ana_teo(lane=None):
    FEC_ANA_BASE_ADDR = 0x1C0
    if lane is None: lane=gLane
    wreg(FEC_ANA_BASE_ADDR+0xD,6,lane ) #set reading data of TEo low 16 bit
    teo_l = rreg(FEC_ANA_BASE_ADDR+0x7,lane)#read data
    wreg(FEC_ANA_BASE_ADDR+0xD,7,lane) #set reading data of TEo high 16 bit
    teo_h = rreg(FEC_ANA_BASE_ADDR+0x7,lane)#read data
    teo = teo_h*65536+teo_l#combinate the data
    #print '\n....Lane %s: TEo counter: %d' %(lane_name_list[lane],teo),
    return teo 

def fec_ana_sei(lane=None):
    FEC_ANA_BASE_ADDR = 0x1C0
    if lane is None: lane=gLane
    wreg(FEC_ANA_BASE_ADDR+0xD,0,lane) #set reading data of SEi low 16 bit
    sei_l = rreg(FEC_ANA_BASE_ADDR+0x7,lane)#read data
    wreg(FEC_ANA_BASE_ADDR+0xD,1 ) #set reading data of SEi high 16 bit
    sei_h = rreg(FEC_ANA_BASE_ADDR+0x7,lane)#read data
    sei = sei_h*65536+sei_l#combinate the data
    #print '\n....Lane %s: SEi counter: %d' %(lane_name_list[lane],sei),
    return sei 

def fec_ana_bei(lane=None):
    FEC_ANA_BASE_ADDR = 0x1C0
    if lane is None: lane=gLane
    wreg(FEC_ANA_BASE_ADDR+0xD,2,lane) #set reading data of BEi low 16 bit
    bei_l = rreg(FEC_ANA_BASE_ADDR+0x7,lane)#read data
    wreg(FEC_ANA_BASE_ADDR+0xD,3 ,lane) #set reading data of BEi high 16 bit
    bei_h = rreg(FEC_ANA_BASE_ADDR+0x7,lane)#read data
    bei = bei_h*65536+bei_l#combinate the data
    #print '\n....Lane %s: BEi counter: %d' %(lane_name_list[lane],bei),
    return bei
    
def fec_ana_hist_DFE(lane=None):
    lanes = get_lane_list(lane)
    f0=[]
    f1=[]
    ratio=[]
    for ln in lanes: 
        #ln = lane_offset/(0x200)
        f0.append(pam4_dfe(lane=ln)[ln][0])
        f1.append(pam4_dfe(lane=ln)[ln][1])
        ratio.append(pam4_dfe(lane=ln)[ln][2])
        print ('A%d: DFE %4.2f, %4.2f, %4.2f'%(ln, f0[ln], f1[ln], ratio[ln]))
        #fmt = 'A%d: DFE %4.2f, %4.2f, %4.2f'%(ln, f0[ln], f1[ln], ratio[ln])
        #file.obj.write(fmt)
        lane_hist_file_ptr = open('S%s_%s_data.txt'%(gSlice,lane_name_list[ln]),'a')
        lane_hist_file_ptr.write("%s\n%s\n"%(f0[ln],f1[ln]))
        lane_hist_file_ptr.close()
        
    return f0, f1, ratio
    
def fec_ana_hist(hist_time=10,lane=None, slice=None, pass_fail_bin=6, file_name='fec_ana_histogram_all.txt'):

    #per_lane_data_files_en=0    # en/dis to save individual files per lanes
    #pass_fail_result_print_en=1
    
    lanes = get_lane_list(lane)
    slices = get_slice_list(slice)
    
    ### FEC Analyzer Parameters
    FEC_ANA_BASE_ADDR = 0x1C0
    num_groups=4
    bins_per_group=4

    ### Create empty list for Hist Data
    hist_slice_lane_bin=[] # list to contain each lane's hist counts
    hist_pass_fail_list=[] # list to conatian each lane's pass-fail status
    for slc in range(2):
        hist_slice_lane_bin.append([])
        hist_pass_fail_list.append([])
        for ln in range(16):
            hist_slice_lane_bin[slc].append([])
            hist_pass_fail_list[slc].append([])
            hist_pass_fail_list[slc][ln] = 0
            for bin in range(16):
                hist_slice_lane_bin[slc][ln].append([])
            
    ### Header for Histogram Data
    timestr = time.strftime("%Y%m%d_%H%M%S")
    fmt=('\n\n...FEC Analyzer Histogram, %d sec per bin, Slice%s '%(hist_time,'s' if len(slices)>1 else ''))
    fmt+= str(slices)
    fmt+= ', TimeStamp [%s]'%timestr
    print (fmt)
    hist_file_ptr = open(file_name, 'a')
    hist_file_ptr.write(fmt)
    hist_file_ptr.close()
    
    ### Start Histogram Collection for all target slices/lanes
    print("\n...Histogram Collection in Progress...Error"),
    for bin_grp in range(num_groups):
        print("Bins[%d,%d,%d,%d]:"%(bin_grp*4,bin_grp*4+1,bin_grp*4+2,bin_grp*4+3)),        
        
        ### Initialize FEC Analyzers for all target slices/lanes
        for slc in slices:
            sel_slice(slc)       
            if len(slices)>1: print("S%d%s"%(slc,' -' if slc==slices[0] else ',')),        
            #fec_ana_hist_DFE(lanes)    
            #fec_ana_hist_one_bin_grp_setup(bin_grp,fec_ana_err_type, fec_ana_T,fec_ana_M,fec_ana_N,lanes) # setup the FEC
            for ln in lanes:
                if gEncodingMode[slc][ln][0].upper() == 'PAM4':
                    fec_ana_err_type=0
                    fec_ana_T=15
                    fec_ana_M=10
                    fec_ana_N=5440
                else:
                    fec_ana_err_type=0
                    fec_ana_T=7
                    fec_ana_M=10
                    fec_ana_N=5280
                wreg([FEC_ANA_BASE_ADDR+0x5,[12]],1,ln)     #reset Histogram
                wreg([FEC_ANA_BASE_ADDR+0x5,[12]],0,ln)     #reset Histogram
                wreg([FEC_ANA_BASE_ADDR+0x5,[2,0]], bin_grp, ln)  #select which group of histogram to read
                wreg( FEC_ANA_BASE_ADDR+0x9, 0x6202,ln) #set PRBS mode: force reload
                wreg( FEC_ANA_BASE_ADDR+0x8, (rreg(FEC_ANA_BASE_ADDR+0x8,ln) | 0x0018),ln) #set PRBS mode:PRBS31
                wreg( FEC_ANA_BASE_ADDR+0x0, ((rreg(FEC_ANA_BASE_ADDR+0x0,ln) & 0xFFF0) | fec_ana_M),ln)  #set M = 10
                wreg( FEC_ANA_BASE_ADDR+0x4, fec_ana_N,ln) #set N = 5440
                wreg( FEC_ANA_BASE_ADDR+0x5, (rreg(FEC_ANA_BASE_ADDR+0x5,ln) & 0xf007) | ((fec_ana_T<<7)+(fec_ana_T<<3)) ,ln) #set T = 2
                wreg( FEC_ANA_BASE_ADDR+0x1, 0x000b,ln) #reset FEC_counter
                wreg( FEC_ANA_BASE_ADDR+0x1, 0x0003,ln) #release the reset 
                wreg( FEC_ANA_BASE_ADDR+0xc, fec_ana_err_type,ln) #set TEO error type
                wreg( FEC_ANA_BASE_ADDR+0xb, fec_ana_err_type,ln) #set TEi error type
            
            
        ### Wait Time for Histogram Data Collection
        time.sleep(hist_time)

        ### Capture FEC Analyzers Histogram for all target slices/lanes
        for slc in slices:
            sel_slice(slc)       
            for ln in lanes:
                for bin in range(bins_per_group):
                    wreg(FEC_ANA_BASE_ADDR+0xD, 12+bin*2 ,ln)   # set reading data of histogram 0 lower 16 bit
                    histo_lo = rreg(FEC_ANA_BASE_ADDR+0x7,ln)   # read data
                    wreg(FEC_ANA_BASE_ADDR+0xD, 13+bin*2 ,ln)   # set reading data of histogram 0 upper 16 bit
                    histo_hi = rreg(FEC_ANA_BASE_ADDR+0x7,ln)   # read data
                    hist_cnt = (histo_hi*65536+histo_lo)
                    hist_slice_lane_bin[slc][ln][bin_grp*bins_per_group+bin]=hist_cnt   # get the 32-bit data                   
                    if (bin_grp*bins_per_group+bin) >= pass_fail_bin and hist_cnt > 0:
                        hist_pass_fail_list[slc][ln] = 1
    
    ### Finished Histogram Collection for all 16 bins. Print/save results
    for slc in slices:
        sel_slice(slc)
        
        #### Print Histogram Data for each slice
        fmt=("\n\nBin")
        for ln in lanes:
            fmt+=("   S%d_%s  " %(slc,lane_name_list[ln]))
        fmt+=("\n---")
        for unused in lanes:
            fmt+=(" ---------")
        for bin_grp in range(num_groups):
            for bin in range(bins_per_group):
                fmt+= '\n%-3d' %(bin_grp*bins_per_group+bin)
                for ln in lanes:
                    cnt = hist_slice_lane_bin[slc][ln][bin_grp*bins_per_group+bin]
                    fmt+= " %-9s" %(str(cnt) if cnt!=0 else '.')                
        ##### Print Pass/Fail Results per lane
        fmt+="\n   "
        for ln in lanes:
            fmt+=("%-10s" %('' if hist_pass_fail_list[slc][ln] == 0 else ' **FAIL**'))
        print (fmt)
        
        #### Save Histogram Data for both slices in the 'fec_ana_hist.txt' file
        hist_file_ptr = open(file_name,'a')
        hist_file_ptr.write(fmt)
        hist_file_ptr.close()
        
        #### Save Histogram Data per-slice per-lane in individual files (if enabled)
        # if per_lane_data_files_en:
            # for ln in lanes:
                # fmt=('\n...FEC Analyzer Histogram Slice %d Lane %s, %d sec per bin, TimeStamp [%s]\n'%(slc,lane_name_list[ln],hist_time,timestr))
                # for bin_grp in range(num_groups):
                    # for bin in range(bins_per_group):
                        # fmt+=("%d\n"%hist_slice_lane_bin[slc][ln][bin_grp*bins_per_group+bin])
                # lane_hist_file_ptr = open('fec_ana_hist_S%s_%s.txt'%(gSlice,lane_name_list[ln]),'a')
                # lane_hist_file_ptr.write(fmt)
                # lane_hist_file_ptr.close()

    return (sum([i.count(1) for i in hist_pass_fail_list]))
####################################################################################################

#################################################################################################### 
CMD=0x9806
CMD_DETAIL=0x9807
REG_DATA=0x9f00
    
def MdioRd(addr):
    #print "MdioRd addr 0x%x " % (addr) 
    libcameo.mdio_read.restype=ctypes.c_ushort   
    val=libcameo.mdio_read(gSlice,0x1,addr)
    #print "MdioRd val 0x%x " % (val)
    return val
    #chip.MdioRd(addr)

def MdioWr(addr, value):
    #print "MdioWr addr 0x%x value 0x%x" % (addr,value)
    libcameo.mdio_write(gSlice,0x1,addr, value)
    #chip.MdioWr(addr, value)
    

def MdioRdh(addr):
    return "0x%04x" % MdioRd(addr)
    
#################################################################################################### 
def BE_debug(lane, mode, index):
    MdioWr(CMD_DETAIL, index)
    cmd = 0xB000 + ((mode&0xf)<<4) + lane
    result = fw_cmd(cmd)
    if (result!=0x0b00+mode):
        return -1
        #raise IOError("Debug command failed with code %04x" % result)
        #print "Debug command failed with code %04x" % result
    return MdioRd(CMD_DETAIL)    
def BE_debug_signed(lane, mode, index):
    value = BE_debug(lane, mode, index)
    return value if value<0x8000 else value-0x10000
def BE_debug1(lane, mode, index, length):
    return BE_debugs(lane, mode, range(index, index+length))
def BE_debug1_signed(lane, mode, index, length):
    return BE_debugs_signed(lane, mode, range(index, index+length))
def BE_debug2(lane, mode, index, l1, l2):
    return [BE_debugs(lane, mode, range(index+i*l2, index+(i+1)*l2))
        for i in range(l1)]
def BE_debug2_signed(lane, mode, index, l1, l2):
    return [BE_debugs_signed(lane, mode, range(index+i*l2, index+(i+1)*l2))
        for i in range(l1)]
def BE_debugs(lane, mode, index):
    return [BE_debug(lane, mode, i) for i in index]
def BE_debugs_signed(lane, mode, index):
    return [BE_debug_signed(lane, mode, i) for i in index]
def BE_debug32(lane, mode, index):
    h = BE_debug(lane, mode, index)
    l = BE_debug(lane, mode, index+1)
    return (h<<16)+l
def BE_debug32_signed(lane, mode, index):
    value = BE_debug32(lane, mode, index)
    return value if value<0x80000000 else value-0x100000000


def BE_info(lane, mode, index, size):
    MdioWr(CMD_DETAIL, index)
    mode |= 8
    cmd = 0xB000 + ((mode&0xf)<<4) + lane
    result = fw_cmd(cmd)
    if (result!=0x0b00+mode):
        #result=-1
        return [-1 for i in range(size)]
        #raise IOError("Info command failed with code %04x" % result)
    return [MdioRd(REG_DATA+i) for i in range(size)]

def BE_info_signed(lane, mode, index, size):
    return [x if x<0x8000 else x-0x10000 for x in BE_info(lane, mode, index, size)]   

def pam4_info_fw(lane):
    ISI = BE_info_signed(lane, 10, 0, 16)
    ths = BE_info_signed(lane, 10, 1, 12)
    ffe_accu = BE_info(lane, 10, 2, 8)
    ffe_accu = [(ffe_accu[i]<<16) + ffe_accu[i+1] for i in range(0, 8, 2)]
    ffe_accu = [x if x<0x80000000 else x-0x100000000 for x in ffe_accu]
    ffe_accu = [x/32768.0 for x in ffe_accu]
    ffe = BE_info_signed(lane, 10, 3, 7)
    ffe_flips = BE_debugs(lane, 2, range(660, 664))
    print ("ISI =", ISI)
    print ("ths =", ths)
    print ("FFE_accu =", ffe_accu)
    print ("FFE: K   =", ffe[0:4], ", S = [%d %d], nbias = %d" % (ffe[5], ffe[6], ffe[4]))
    print ("FFE: K Pol flips=", ffe_flips[0:4])
    
    
def dump_fw(lane):
    exit_codes = BE_debugs(lane, 0, range(100, 100+16))
    print ("exit codes =", [hex(x) for x in exit_codes])
    
def nrz_dump_fw(lane):
    agcgain1_dc1 = BE_debugs(lane, 1, range(80, 80+9))
    agcgain2_dc1 = BE_debugs(lane, 1, range(100, 100+9))
    index_dc1 = BE_debugs(lane, 1, range(140, 140+9))
    of_cnt_dc1 = BE_debugs(lane, 1, range(120, 120+9))
    print ("-----------------FIRST DAC SEARCH------------------")
    print ("|init agcgains|index number|of_cnt|result agcgains|")
    for i in range(8) : 
        print ("| (%3d,  %3d) |    %3d     |%5d |  (%3d,  %3d)  |"%(agcgain1_dc1[i], agcgain2_dc1[i], index_dc1[i], of_cnt_dc1[i], agcgain1_dc1[i+1], agcgain2_dc1[i+1]))
    print ("---------------------------------------------------")
    
    of_cnt_ca1 = BE_debugs(lane, 1, range(330, 330+8))
    of_thre_ca1 = BE_debugs(lane, 1, range(300, 300+8))
    hf_cnt_ca1 = BE_debugs(lane, 1, range(390, 390+8))
    hf_thre_ca1 = BE_debugs(lane, 1, range(360, 360+8))
    
    of_cnt_ca2 = BE_debugs(lane, 1, range(338, 338+8))
    of_thre_ca2 = BE_debugs(lane, 1, range(308, 308+8))
    hf_cnt_ca2 = BE_debugs(lane, 1, range(398, 398+8))
    hf_thre_ca2 = BE_debugs(lane, 1, range(368, 368+8))
    
    of_cnt_ca3 = BE_debugs(lane, 1, range(346, 346+8))
    of_thre_ca3 = BE_debugs(lane, 1, range(316, 316+8))
    hf_cnt_ca3 = BE_debugs(lane, 1, range(406, 406+8))
    hf_thre_ca3 = BE_debugs(lane, 1, range(376, 376+8))
    
    print ("----------------------CHANNEL ANALYZER-----------------------")
    print ("|        CA1        |        CA2        |        CA3        |")
    print ("|of_cnt of hf_cnt hf|of_cnt of hf_cnt hf|of_cnt of hf_cnt hf|")
    for i in range(8) :
        print ("|%5d %2d %5d %2d  |%5d %2d %5d %2d  |%5d %2d %5d %2d  |"%(of_cnt_ca1[i], of_thre_ca1[i], hf_cnt_ca1[i], hf_thre_ca1[i], of_cnt_ca2[i], of_thre_ca2[i], hf_cnt_ca2[i], hf_thre_ca2[i], of_cnt_ca3[i], of_thre_ca3[i], hf_cnt_ca3[i], hf_thre_ca3[i]))
    print ("-------------------------------------------------------------")
    
    ctle = BE_debugs(lane, 1, range(505, 505+4))
    dfe_ctle1 = [BE_debug_signed(lane, 1, 510+i) for i in range(3)]
    dfe_ctle2 = [BE_debug_signed(lane, 1, 513+i) for i in range(3)]
    dfe_ctle3 = [BE_debug_signed(lane, 1, 516+i) for i in range(3)]
    dfe_ctle4 = [BE_debug_signed(lane, 1, 519+i) for i in range(3)]
    print ("-------CTLE FINE SEARCH--------")
    print ("| ctle   |%4d|%4d|%4d|%4d|"%(ctle[0], ctle[1], ctle[2], ctle[3]))
    print ("| dfe F1 |%4d|%4d|%4d|%4d|"%(dfe_ctle1[0], dfe_ctle2[0], dfe_ctle3[0], dfe_ctle4[0]))
    print ("| dfe F2 |%4d|%4d|%4d|%4d|"%(dfe_ctle1[1], dfe_ctle2[1], dfe_ctle3[1], dfe_ctle4[1]))
    print ("| dfe F3 |%4d|%4d|%4d|%4d|"%(dfe_ctle1[2], dfe_ctle2[2], dfe_ctle3[2], dfe_ctle4[2]))
    print ("-------------------------------")
    
    agcgain1_dc2 = BE_debugs(lane, 1, range(89, 89+9))
    agcgain2_dc2 = BE_debugs(lane, 1, range(109, 109+9))
    index_dc2 = BE_debugs(lane, 1, range(149, 149+9))
    of_cnt_dc2 = BE_debugs(lane, 1, range(129, 129+9))
    print ("----------------SECOND DAC SEARCH------------------")
    print ("|init agcgains|index number|of_cnt|result agcgains|")
    for i in range(8) : 
        print ("| (%3d,  %3d) |    %3d     |%5d |  (%3d,  %3d)  |"%(agcgain1_dc2[i], agcgain2_dc2[i], index_dc2[i], of_cnt_dc2[i], agcgain1_dc2[i+1], agcgain2_dc2[i+1]))
    print ("---------------------------------------------------")
    
    debug_states = BE_debugs(lane, 1, range(160, 160+8))
    print ("stats = [")
    for i in range(8) : 
        print ("%2d  "%(debug_states[i]))
    print ("]")
    
def pam4_dump_fw_linear_fit(lane):
    pam4_state = BE_debug(lane, 2, 0)
    error_exit = False
    if pam4_state==0xeeef:
        error_exit = True
        pam4_state = BE_debug(lane, 0, 3)
    try:
        adapt_mode = BE_debug(lane, 2, 3)
    except CoreException:
        adapt_mode = 0
    if adapt_mode==0:
        print ("Auto adapt mode")
    elif adapt_mode==1:
        print ("Factory fixed setting mode")
    elif adapt_mode==2:
        print ("User fixed setting mode")
    else:
        print ("Unknown adapt mode")
        return

    agcgain1 = BE_debug(lane, 2, 23)
    agcgain2 = BE_debug(lane, 2, 24)
    agcgain1_dc1 = BE_debug(lane, 2, 8)
    agcgain2_dc1 = BE_debug(lane, 2, 9)
    final_of = BE_debug(lane, 2, 4)
    final_hf = BE_debug(lane, 2, 5)

    dc_search_agcgain = BE_debug2(lane, 2, 300, 2, 15)
    ratio = BE_debug(lane, 2, 2)
    agc_index = BE_debug(lane, 2, 1)
    restart_count = BE_debug(lane, 2, 7)
    #delta = BE_debug_signed(lane, 2, 27)
    delta_times0 = BE_debug_signed(lane, 2, 35)+1
    delta_dump0 = [BE_debug_signed(lane, 2, 110+i) for i in range(10)]
    fm1_dump0 = [BE_debug_signed(lane, 2, 150+i) for i in range(10)]
    ctle = BE_debug(lane, 2, 33)
    next_f13 = BE_debug(lane, 2, 34)
    dc_search_agcgain[0] = [(x>>8, x&0xff) for x in dc_search_agcgain[0]]
    dc_search_agcgain[1] = [(x>>8, x&0xff) for x in dc_search_agcgain[1]]

    ctle_isi = BE_debug2_signed(lane, 2, 330, 2, 4)
    ctle_record = BE_debug1_signed(lane, 2, 340, 2)
    ctle_freq_accu =  BE_debug_signed(lane, 2, 200)
    ctle_freq_accu1 =  BE_debug_signed(lane, 2, 201)
    # dump_lf=False
    # if dump_lf:
        # lf_dmp_size = 2
    # else:
        # lf_dmp_size = 7
    lf_dmp_size = 7
    smart_check_result = BE_debug2_signed(lane, 2, 350, lf_dmp_size, 5)
    smart_check_ths = BE_debug2_signed(lane, 2, 700, lf_dmp_size, 12)
    #smart_check1_ths = BE_debug2_signed(lane, 2, 800, lf_dmp_size, 12)
    lf_result = BE_debug2_signed(lane, 2, 400, lf_dmp_size, 5)

    # if dump_lf:
        # force_ths = BE_debug2_signed(lane, 2, 900, lf_dmp_size, 12)
        # plus_margin = BE_debug2_signed(lane, 2, 1000, lf_dmp_size, 12)
        # minus_margin = BE_debug2_signed(lane, 2, 1100, lf_dmp_size, 12)
    em_debug = BE_debug2_signed(lane, 2, 470, lf_dmp_size, 3)
    ffe_adapt = BE_debug2_signed(lane, 2, 600, 1, 4)
    nbias_adapt = BE_debug1_signed(lane, 2, 640, 1)
    if error_exit:
        print ("Error exited")
    print ("PAM4 state = 0x%04x" % pam4_state)
    print ("restart count = %d" % restart_count)
    if adapt_mode!=0:
        fixed_f13 = BE_debug_signed(lane, 2, 40)
        fixed_CTLE = BE_debug(lane, 2, 41)
        fixed_Kp = BE_debug(lane, 2, 42)
        fixed_agcgain = BE_debugs(lane, 2, range(43, 45))
        fixed_delta = BE_debug_signed(lane, 2, 45)
        fixed_skef = BE_debug(lane, 2, 46)
        fixed_edge = BE_debug(lane, 2, 47)
        fixed_kf = BE_debug(lane, 2, 48)
        fixed_sf = BE_debug(lane, 2, 49)
        ctle_temp = BE_debugs(lane, 2, range(50, 53))
        fixed_ffe = BE_debugs_signed(lane, 2, range(53, 53+7))
        # Reconstruct CTLE table
        fixed_ctle_all = (ctle_temp[0]<<32) | (ctle_temp[1]<<16) | ctle_temp[2]
        fixed_ctle_table = [(fixed_ctle_all>>(i*6))&0x3f for i in range(7, -1, -1)]
        fixed_ctle_table = [(i>>3, i&7) for i in fixed_ctle_table]
        print ("Fixed settings:")
        print ("    F1/3 = %d" % fixed_f13)
        print ("    CTLE = %d" % fixed_CTLE)
        print ("    Kp = %d" % fixed_Kp)
        print ("    agcgain = %d, %d" % (fixed_agcgain[0], fixed_agcgain[1]))
        print ("    delta = %d" % fixed_delta)
        print ("    Skef = %d (%s)" % (fixed_skef&7, "enabled" if fixed_skef&8 else "disabled"))
        print ("    Edge = 0x%04x" % fixed_edge)
        print ("    Kf = %d, Sf = %d" % (fixed_kf, fixed_sf))
        print ("    CTLE table =", fixed_ctle_table)
        print ("    FFE: K1=%d, K2=%d, K3=%d, K4=%d, S1=%d, S1=%d, nbias=%d" % (fixed_ffe[0],
                fixed_ffe[1], fixed_ffe[2], fixed_ffe[3], fixed_ffe[4], fixed_ffe[5], fixed_ffe[6]))
    if adapt_mode<1:
        print ("AGCgain = %d, %d" % (agcgain1, agcgain2))
        print ("=== DC search 1 ===")
        print ("   agcgain record =", dc_search_agcgain[0])
        print ("   AGCgain after DC search1 = %d, %d" % (agcgain1_dc1, agcgain2_dc1))
        print ("=== Channel analyzer === ")
        print ("   OF =", final_of)
        print ("   HF =", final_hf)
        print ("   Ratio = %d (%5.3f)" % (ratio, ratio/256.0))
        print ("   CTLE search index = %d" % (agc_index))
        print ("=== DC search 2 === ")
        print ("   agcgain record =", dc_search_agcgain[1])
        print ("=== Smart check and linear fit ===")
        print ("   smart_chk=", smart_check_result)
        print ("   smart_check_ths=", smart_check_ths)
        #print "   smart_check1_ths=", smart_check1_ths
        print ("   LF_result =", lf_result)
        # if dump_lf:
            # print ("   force_ths=", force_ths)
            # print ("   plus_margin=", plus_margin)
            # print ("   minus_margin=", minus_margin)
        print ("   EM_debug =", em_debug)
        print ("   Final F1/3 init = %d" % next_f13)
        print ("=== CTLE search ===")
        print ("   CTLE before search =", ctle_record[0])
        print ("   CTLE search ISI1 =", ctle_isi[0])
        print ("   Freq accu before CTLE search =", ctle_freq_accu)
        print ("   Freq accu after CTLE search =", ctle_freq_accu1)
        if (ctle_record[1]!=-1):
            print ("   CTLE after search 1 =", ctle_record[1])
            print ("   CTLE search ISI2 =", ctle_isi[1])
        else:
            print ("   CTLE search2 is skipped")
        print ("   Final CTLE =", ctle)
        print ("=== Delta search ===")
        print ("   Searched %d times. Search process:" % (delta_times0), delta_dump0)
        print ("   F(-1) =", fm1_dump0)

    print ("FFE adapt:")
    for i in range(1):
        print ("FFE adapt iteration %d: [%d, %d, %d, %d], nbias=%d" % (i,
                ffe_adapt[i][0], ffe_adapt[i][1], ffe_adapt[i][2], ffe_adapt[i][3], nbias_adapt[i]))


    # timers
    timers = BE_debugs(lane, 2, range(10, 18))
    t0 = timers[1]
    timers = [x-t0 for x in timers]
    timers = [timers[0]] + [x if x>=0 else x+65536 for x in timers[1:]]
    start_time, sd_time, ca_done_time, dc_search_time, link_time, ctle_search_time, delta_search_time, done_time = timers
    print ("Timers: ")
    print ("Start time: %d" % start_time)
    print ("SD time: %d" % sd_time)
    print ("CA done: %d" % ca_done_time)
    print ("DC search: %d" % dc_search_time)
    print ("Initial link: %d" % link_time)
    print ("CTLE search: %d" % ctle_search_time)
    print ("Delta search1: %d" % delta_search_time)
    print ("Done: %d" % done_time)

def pam4_dump_chip(lane):
    #dac_val, en = dac_pam4(lane)
    dac_val = dac(lane)[lane]
    #ctle_val = ctle_pam4(lane)
    ctle_val = ctle(lane)[lane]
    #agcgain1, agcgain2 = agcgain(lane)
    agcgain1, agcgain2, g3,g4 = dc_gain(lane)[lane]
    
    f1over3 = f13(lane)[lane]
    print ("DAC = %d%s" % (dac_val, " (forced)" if en else ""))
    print ("CTLE = %d" % ctle_val)
    print ("AGCgain = %d, %d" % (agcgain1, agcgain2))
    print ("F1/3 = %d" % f1over3)
#################################################################################################### 
def fw_adapt_dump(lane=None):
    lanes = get_lane_list(lane)
    for ln in lanes:
        get_lane_mode(ln)
        #c = Pam4Reg if lane_mode_list[ln] == 'pam4' else NrzReg
        if lane_mode_list[ln] == 'pam4':
            print("\n...Lane %d: PAM4 Adaptation Info...."%ln)
            pam4_dump_fw_linear_fit(ln)
        else:
            print("\n...Lane %d: NRZ Adaptation Info...."%ln)
            nrz_dump_fw(ln)
####################################################################################################
def fw_info_dump(lane=None):
    lanes = get_lane_list(lane)
    for ln in lanes:
        get_lane_mode(ln)
        #c = Pam4Reg if lane_mode_list[ln] == 'pam4' else NrzReg
        if lane_mode_list[ln] == 'pam4':
            print("\n...Lane %d: PAM4 ISI and FFE Info\n"%ln)
            pam4_info_fw(ln)
        else:
            print("\n*** Lane %d: ISI and FFE Info Available for PAM4 Lane Only! ***\n"%ln)
#################################################################################################### End of FW DUMP

def bitmux_status(print_en=True, fifo_check_en=1):
    bitmux_return_status=True
    if fifo_check_en: # [Acceptable range for each FIFO pointer]
        buf_ptr_min_limit = [1,  13]
        buf_ptr_max_limit = [1,  13]
        buf_ptr_del_limit = [0,  10]        
    else:       
        buf_ptr_min_limit = [0,  15]
        buf_ptr_max_limit = [0,  15]
        buf_ptr_del_limit = [0,  10]
    
    buf_type = ['--Split-->','       -->','       ---','<--Merge--']      
    if print_en: print("\n -- BitMux Buffers: Min,Curr,Max,Delta")
    for a_side_buf in range(0,4):
        wreg([0x984d,[14,12]], a_side_buf)
        for b_side_buf in range(4,8):
            wreg([0x984d,[10,8]], b_side_buf)
            buf_ptr_min  = rreg([0x984e, [11,8]])
            buf_ptr_max  = rreg([0x984e,  [7,4]])
            buf_ptr_cur  = rreg([0x984e,[15,12]])
            buf_ptr_del  = buf_ptr_max - buf_ptr_min
            
            buf_ptr_min_status = 1 if  buf_ptr_min_limit[0] <= buf_ptr_min <= buf_ptr_min_limit[1] else -1
            buf_ptr_max_status = 1 if  buf_ptr_max_limit[0] <= buf_ptr_max <= buf_ptr_max_limit[1] else -1
            buf_ptr_del_status = 1 if  buf_ptr_del_limit[0] <= buf_ptr_del <= buf_ptr_del_limit[1] else -1
            #buf_ptr_cur_status = 1 if  buf_ptr_del_limit[0] <= buf_ptr_cur <= buf_ptr_del_limit[1] else -1
            status_flag = '<<' if buf_ptr_min_status<0 or buf_ptr_max_status<0 or buf_ptr_del_status<0 else ' '
            if status_flag=='<<':
                bitmux_return_status=False
            if print_en: print("\n A%d %s B%d :"%(a_side_buf,buf_type[b_side_buf-4],a_side_buf*2+(b_side_buf%2))),
            if print_en: print("%3d %3d  %3d %3d"%(buf_ptr_min,buf_ptr_cur,buf_ptr_max, buf_ptr_del)),
            if print_en: print("%s"%(status_flag)),
        if print_en: print("")

    return bitmux_return_status

def fw_bitmux_status(print_en=True, fifo_check_en=1):
    return_status=True
    return_retries_config=0
    return_retries_runtime=0
    if fifo_check_en: # [Acceptable range for each FIFO pointer]
        buf_ptr_min_limit = [2,  13]
        buf_ptr_max_limit = [2,  13]
        buf_ptr_del_limit = [1,  10]        
    else:       
        buf_ptr_min_limit = [0,  15]
        buf_ptr_max_limit = [0,  15]
        buf_ptr_del_limit = [0,  10]
    
    buf_type = ['--Split-->','       -->','       ---','<--Merge--']
    for a_side_buf in range(0,4):
    
        bitmux_state  = BE_debug(a_side_buf, 4, 0)           # Bitmux state (5 means 'Done with bitmux config')
        bitmux_retries_config  = BE_debug(a_side_buf, 4, 24) # Bitmux FIFO retry counter during bitmux config
        bitmux_retries_runtime = BE_debug(a_side_buf, 4, 25) # Bitmux FIFO retry counter after bitmux config (runtime FIFO check)
       
        print("\n FW BitMux Buffers: Min,Max,Delta [Min,Max,Delta]"),
        bitmux_state_char = '=' if bitmux_state==5 else '->'
        print(",State%s %d" % (bitmux_state_char,bitmux_state)),
        bitmux_retry_char = '=' if (bitmux_retries_config==0 and bitmux_retries_runtime==0) else '->'
        print(",Retries%s %d %d" % (bitmux_retry_char, bitmux_retries_config,bitmux_retries_runtime)),
        if bitmux_retries_config > return_retries_config:
            return_retries_config = bitmux_retries_config
        if bitmux_retries_runtime > return_retries_runtime:
            return_retries_runtime = bitmux_retries_runtime
                
        for b_side_buf in range(4,8):
            buf_ptr_max  = BE_debug(a_side_buf, 4, (b_side_buf)+6)
            buf_ptr_min  = BE_debug(a_side_buf, 4, (b_side_buf)+10)
            #buf_ptr_cur  = 0
            buf_ptr_del  = buf_ptr_max - buf_ptr_min
            
            buf_ptr_max_during_config  = BE_debug(a_side_buf, 4, (b_side_buf)+26)
            buf_ptr_min_during_config  = BE_debug(a_side_buf, 4, (b_side_buf)+30)
            buf_ptr_del_during_config  = buf_ptr_max_during_config - buf_ptr_min_during_config
            
            buf_ptr_min_status = 1 if  buf_ptr_min_limit[0] <= buf_ptr_min <= buf_ptr_min_limit[1] else -1
            buf_ptr_max_status = 1 if  buf_ptr_max_limit[0] <= buf_ptr_max <= buf_ptr_max_limit[1] else -1
            buf_ptr_del_status = 1 if  buf_ptr_del_limit[0] <= buf_ptr_del <= buf_ptr_del_limit[1] else -1
            #buf_ptr_cur_status = 1 if  buf_ptr_del_limit[0] <= buf_ptr_cur <= buf_ptr_del_limit[1] else -1
            status_flag = '<<' if buf_ptr_min_status<0 or buf_ptr_max_status<0 or buf_ptr_del_status<0 else ' '
            
            print("\n A%d %s B%d :"%(a_side_buf,buf_type[b_side_buf-4],a_side_buf*2+(b_side_buf%2))),
            print("%3d %3d %3d  "%(buf_ptr_min,buf_ptr_max,buf_ptr_del)),
            print("[%3d %3d %3d  ] "%(buf_ptr_min_during_config,buf_ptr_max_during_config, buf_ptr_del_during_config)),
            print("%s"%(status_flag)),
            if status_flag=='<<':
                return_status=False


        print("")

    return return_status, return_retries_config, return_retries_runtime
###################################################################################################
# 
# GET ISI Residual Taps for NRZ
####################################################################################################
def sw_nrz_isi(lane=None,print_en=0):
    isi_tap_range=range(0,16)
    lanes = get_lane_list(lane)
    result={}
    #fw_pause_en=1

    # if fw_pause_en:
        # if fw_loaded(print_en=0):
            # fw_reg(addr=8,data=0x0000, print_en=0) # pause serdes FW

    for ln in lanes:
        bp1_ori = rregBits(0x17d, [12, 8],ln)
        bp1_en_ori = rregBits(0x17d, [15],ln)
        wregBits(0x17d, [12, 8], 24,ln)
        tap_list = []
        # wregBits(0x17d, [15], 0, ln)
        # wregBits(0x10c, [15], 0, ln)
        # wregBits(0x10c, [15], 1, ln)

        wregBits(0x17d, [15], 0, ln)
        wregBits(0x10c, [15], 0, ln)
        wregBits(0x10c, [15], 1, ln)

        for i in isi_tap_range:
            wregBits(0x165, [4,1],i,ln)
            time.sleep(0.01)
            wregBits(0x17d, [15], 1, ln)

            wait_for_bp1_timeout=0
            while True:
                if rregBits(0x18f, [15],ln):
                    break
                else:
                    wait_for_bp1_timeout+=1
                    if wait_for_bp1_timeout>5000:
                        if print_en==2:print (" Get Tap Value >>>>> Timed out 2 waiting for BP1")
                        break
            plus = (rregBits(0x127, [3,0],ln)<<8) + rregBits(0x128, [15,8],ln)
            minus = (rregBits(0x128, [7,0],ln)<<4) + rregBits(0x129, [15,12],ln)

            if (plus>2047): plus = plus - 4096
            if (minus>2047): minus = minus - 4096
            diff_margin = plus - minus 
            diff_margin_f = ((float(diff_margin & 0x0fff)/2048)+1)%2-1

            if print_en==2: print ("\n%8d, %8d, %8d, %11d, %11.4f "  % (i, plus, minus, diff_margin, diff_margin_f))
            tap_list.append(diff_margin)

            # wregBits(0x17d, [15], 0, ln)
            # wregBits(0x10c, [15], 0, ln)
            # wregBits(0x10c, [15], 1, ln)
            
            wregBits(0x17d, [15], 0, ln)
            wregBits(0x10c, [15], 0, ln)
            wregBits(0x10c, [15], 1, ln)

        wregBits(0x17d, [12, 8], bp1_ori, ln)
        wregBits(0x17d, [15], bp1_en_ori, ln)
        result[ln] = tap_list 
        
    # if fw_pause_en:
        # if fw_loaded(print_en=0):
            # fw_reg(addr=8,data=0xffff, print_en=0) # un-pause serdes FW
    return result
####################################################################################################   
def read_plus_minus_margin_nrz(lane=None):
    lanes = get_lane_list(lane)
    result = {}
    for lane in lanes:
        plus_0 = rregBits(0x11a,[15,4],lane)
        plus_1 = (rregBits(0x11a,[3,0],lane)<<8)+rregBits(0x11b,[15,8],lane)
        plus_2 = (rregBits(0x11b,[7,0],lane)<<4)+rregBits(0x11c,[15,12],lane)
        plus_3 = rregBits(0x11c,[11,0],lane)
        plus_4 = rregBits(0x11d,[15,4],lane)
        plus_5 = (rregBits(0x11d,[3,0],lane)<<8)+rregBits(0x11e,[15,8],lane)
        plus_6 = (rregBits(0x11e,[7,0],lane)<<4)+rregBits(0x11f,[15,12],lane)
        plus_7 = rregBits(0x11f,[11,0],lane)
        if plus_0 > 2047: plus_0 = plus_0 - 4096
        if plus_1 > 2047: plus_1 = plus_1 - 4096
        if plus_2 > 2047: plus_2 = plus_2 - 4096
        if plus_3 > 2047: plus_3 = plus_3 - 4096
        if plus_4 > 2047: plus_4 = plus_4 - 4096
        if plus_5 > 2047: plus_5 = plus_5 - 4096
        if plus_6 > 2047: plus_6 = plus_6 - 4096
        if plus_7 > 2047: plus_7 = plus_7 - 4096
        plus_margin = [plus_0,plus_1,plus_2,plus_3,plus_4,plus_5,plus_6,plus_7]
        minus_0 = rregBits(0x120,[15,4],lane)
        minus_1 = (rregBits(0x120,[3,0],lane)<<8)+rregBits(0x121,[15,8],lane)
        minus_2 = (rregBits(0x121,[7,0],lane)<<4)+rregBits(0x122,[15,12],lane)
        minus_3 = rregBits(0x122,[11,0],lane)
        minus_4 = rregBits(0x123,[15,4],lane)
        minus_5 = (rregBits(0x123,[3,0],lane)<<8)+rregBits(0x124,[15,8],lane)
        minus_6 = (rregBits(0x124,[7,0],lane)<<4)+rregBits(0x125,[15,12],lane)
        minus_7 = rregBits(0x125,[11,0],lane)
        if minus_0 > 2047: minus_0 = minus_0 - 4096
        if minus_1 > 2047: minus_1 = minus_1 - 4096
        if minus_2 > 2047: minus_2 = minus_2 - 4096
        if minus_3 > 2047: minus_3 = minus_3 - 4096
        if minus_4 > 2047: minus_4 = minus_4 - 4096
        if minus_5 > 2047: minus_5 = minus_5 - 4096
        if minus_6 > 2047: minus_6 = minus_6 - 4096
        if minus_7 > 2047: minus_7 = minus_7 - 4096
        minus_margin = [minus_0,minus_1,minus_2,minus_3,minus_4,minus_5,minus_6,minus_7]
        #print 'plus_margin=', plus_margin
        #print 'minus_margin=',minus_margin
        result[lane] = plus_margin, minus_margin
    #else:
    if result != {}: return result

####################################################################################################
def v_sensor(on_chip_sensor=1, print_en=1):
    # function used to measure voltage
    # if on chip sensor deactivated, use external multimeter (currently commented out)
    Vsensor_base_addr = 0xB000

    if on_chip_sensor==0: # Use Bench Multimeter connected to laptop through GPIB/USB
        #v=mm.v
        v=-1
        return v*1000
        #pass
    else: # Use On-Chip Voltage Sensor
        
        chip.MdioWr((Vsensor_base_addr + 0x3f),0x010d) 
        chip.MdioWr((Vsensor_base_addr + 0xf6),0x1040) #sel[6:3]=8,pd[12]=1
        time.sleep(0.1)
        chip.MdioWr((Vsensor_base_addr + 0xf6),0x0040) #sel[6:3]=8,pd[12]=0
        time.sleep(0.1)
        chip.MdioWr((Vsensor_base_addr + 0xf6),0x0040) 
        time.sleep(0.1)
        chip.MdioWr((Vsensor_base_addr + 0xf6),0x0440) #sel[6:3]=8,pd[12]=0;rstn[10]=1
        time.sleep(0.1)
        chip.MdioWr((Vsensor_base_addr + 0xf6),0x0c40) #sel[6:3]=8,pd[12]=0;rstn[10]=1,run[11]=1
        time.sleep(0.1)

        k = 1
        for i in range(0,k): 
            chip.MdioWr((Vsensor_base_addr + 0xf6),0x0c40+(i<<3)) #sel[6:3]=8,pd[12]=0;rstn[10]=1,run[11]=1
            time.sleep(0.3)
            rdatah = chip.MdioRd(Vsensor_base_addr + 0xf5) 
            rdatah = (rdatah& 0x0fff) 
            #if print_en: print "Voltage sensor register data:0x%04x" % (rdatah) 
             
            rdata = ((rdatah+1.0)/ 256.0) * 1.224
        if print_en: 
            print ("On-Chip Vsense Voltage: %4.0f mV" % (rdata*1000.0))
        else: 
            return rdata*1000


def Retimer_temp():
    for RT_address in range(0,10):
        #chip.disconnect()
        #chip.connect(dev_addr=1,phy_addr=RT_address,usb_sel=0)
        temperature = temp_sensor(print_en=True)
        print("SLice %2d temperature is %2.1f"%(RT_address,temperature))

def test_init():           # Add by Klaus

    for RT_address in range(0,10):
        print("SLice "+ str(RT_address))
        #chip.disconnect()
        #chip.connect(dev_addr=1,phy_addr=RT_address,usb_sel=0)
        rx_monitor(rst=1)
        
##############################################################################
# FW Exit Code Lane information
#
##############################################################################
def exit_codes(lane=range(16)):
    #s=sel_slice(slice)
    lanes = get_lane_list(lane)    
    
    
    print ("\n===================================================================================================="),
    print ("\nLane | Exit Codes => "),

    for ln in lanes:
        print ('\n %3d'%ln),
        #lane_mode = s.get_lane_mode(ln)
        #if s.lane_mode_list[ln] == 'pam4':
        for index in range(100, 116):
            exit_code = fw_debug_cmd(0, index, ln) # index=100 is the oldest record, 115 is newest
            print ('-> %04X' % (exit_code)),          
    print ("\n====================================================================================================")

def rxm(rst=0, fec_thresh=gFecThresh):
    sel_slice(0);temp_sensor_fast();rx_monitor(rst=rst, fec_thresh=fec_thresh);sel_slice(1);rx_monitor(rst=rst, fec_thresh=fec_thresh)

def kpp(val=None,val2=None):
    sel_slice(0);
    if val2!=None: ted(val2,print_en=0)
    kp(val);
    sel_slice(1);
    if val2!=None: ted(val2,print_en=0)
    kp(val)
    
def regg(addr,val=None,lane=None,print_en=1):
    reg(addr,val,lane,slice=[0,1],print_en=print_en)
    
def auto_poll():
    sel_slice(0);auto_pol();sel_slice(1);auto_pol()
 
def ser():
    sel_slice(0);fw_serdes_params();sel_slice(1);fw_serdes_params()
    
def fecc(print_en=1):
    sel_slice(0)
    slice0= fec_status(print_en=print_en)
    sel_slice(1)
    slice1 = fec_status(print_en=print_en)
    if not print_en: return slice0, slice1

def plll(tgt_pll=None, datarate=None, fvco=None, cap=None, n=None, div4=None, div2=None, refclk=None, frac_en=None, frac_n=None, lane=None):
    sel_slice(0);pll(tgt_pll, datarate, fvco, cap, n, div4, div2, refclk, frac_en, frac_n, lane)
    sel_slice(1);pll(tgt_pll, datarate, fvco, cap, n, div4, div2, refclk, frac_en, frac_n, lane)

def lrr():
    sel_slice(0);lr();fw_adapt_wait(max_wait=12);sel_slice(1);lr();fw_adapt_wait(max_wait=12)
    #sel_slice(0);fw_adapt_wait();sel_slice(1);fw_adapt_wait()

def hist(hist_time=1, lane='all', slice=[0,1]):
    #start_time=time.time()
    result = fec_ana_hist(hist_time=hist_time,lane=lane,slice=slice)
    #stop_time=time.time()
    #print("\n...Total Histogram time: %2.2f sec"%(stop_time-start_time))
    print("\n\n...Lanes Failed: %d\n"%(result))

def fw_stop():
    fw_reg_wr(128,0)

def fw_start():
    fw_reg_wr(128,0xFFFF) 
def cameo_400G_card_8():     
    libcameo.cameo_switch_phy_id(1,8)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(2,8)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=7)
def cameo_400G_card_7(): 
    libcameo.cameo_switch_phy_id(1,7)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(2,7)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=7)
def cameo_400G_card_6(): 
    libcameo.cameo_switch_phy_id(1,6)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(2,6)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=7)

def cameo_400G_card_5():
    libcameo.cameo_switch_phy_id(1,5)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(2,5)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=7)
def cameo_400G_card_4(): 
    libcameo.cameo_switch_phy_id(1,4)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(2,4)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=7)

def cameo_400G_card_3(): 
    libcameo.cameo_switch_phy_id(1,3)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(2,3)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=7)    

def cameo_400G_card_2():    
    libcameo.cameo_switch_phy_id(1,2)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(2,2)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=7)
def cameo_400G_card_1():
    libcameo.cameo_switch_phy_id(1,1)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(2,1)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=7)



def cameo_eq_setting_400G(card):
    global gCameo400G_EQ
    max_count = 32
    count = 0
    for first_col in gCameo400G_EQ:
        for item in first_col:
            if item['card'] == card :  
                cameo_change_chip_card(item['chip'],card)
                sel_slice(item['slice'])
                tx_taps(tap1=item['tap1'],tap2=item['tap2'] ,tap3=item['tap3'] ,tap4=item['tap4'],tap5=item['tap5'],lane=item['lane'])
                count = count + 1
                if count == max_count:
                    return
def cameo_100G_card_8(): 
    libcameo.cameo_switch_phy_id(1,8)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(2,8)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    
    libcameo.cameo_switch_phy_id(3,8)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(4,8)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
def cameo_100G_card_7(): 
    libcameo.cameo_switch_phy_id(1,7)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(2,7)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    
    libcameo.cameo_switch_phy_id(3,7)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(4,7)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
def cameo_100G_card_6(): 
    libcameo.cameo_switch_phy_id(1,6)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(2,6)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    
    libcameo.cameo_switch_phy_id(3,6)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(4,6)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
def cameo_100G_card_5():
    libcameo.cameo_switch_phy_id(1,5)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(2,5)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(3,5)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(4,5)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7) 
def cameo_100G_card_4(): 
    libcameo.cameo_switch_phy_id(1,4)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(2,4)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    
    libcameo.cameo_switch_phy_id(3,4)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(4,4)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=18,tap4=9,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
def cameo_100G_card_3():     
    libcameo.cameo_switch_phy_id(1,3)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(2,3)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    
    libcameo.cameo_switch_phy_id(3,3)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(4,3)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-4,tap3=19,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
def cameo_100G_card_2():    
    libcameo.cameo_switch_phy_id(1,2)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(2,2)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(3,2)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(4,2)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
def cameo_100G_card_1():
    libcameo.cameo_switch_phy_id(1,1)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(2,1)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    
    
    libcameo.cameo_switch_phy_id(3,1)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    libcameo.cameo_switch_phy_id(4,1)
    
    
    sel_slice(1)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)
    
    
    sel_slice(0)
    
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=0)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=1)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=2)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=3)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=4)
    tx_taps(tap1=0,tap2=-3,tap3=20,tap4=8,tap5=0,lane=5)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=6)
    tx_taps(tap1=0,tap2=0 ,tap3=0 ,tap4=0,tap5=0,lane=7)    
def cameo_eq_setting_100G(card):
    global gCameo100G_EQ
    max_count = 64
    count = 0
    for first_col in gCameo100G_EQ:
        for item in first_col:
            if item['card'] == card :  
                cameo_change_chip_card(item['chip'],card)
                sel_slice(item['slice'])
                tx_taps(tap1=item['tap1'],tap2=item['tap2'] ,tap3=item['tap3'] ,tap4=item['tap4'],tap5=item['tap5'],lane=item['lane'])
                count = count + 1
                if count == max_count:
                    return


def cameo_main_100G(card,A_lanes=[0,1,4,5],gearbox_type='100G-1', fec_b_bypass=False):    
    #gSlice=0
    #slices=[0,1]
    global gCard
    gCard = card    
    cameo_drv_open()
                         
    for num in range(1,5):
        cameo_change_chip_card(num,card)        
        mdio_status = get_mdio_status()

        if (mdio_status == MDIO_CONNECTED):            
            sel_slice(0)
            if not fw_loaded(print_en=0):
                print("\n*** No FW Loaded. %d\n" % num)             
                fw_load(gFwFileName,slice=[0,1])
                
            sel_slice(0)
            soft_reset()
            sel_slice(1)
            soft_reset()  
            config_baldeagle_gearbox(slice=[0,1], A_lanes=A_lanes,gearbox_type=gearbox_type, gearbox_by_fw=True, fec_b_bypass=fec_b_bypass)
        else:
            print ("Failed to access MDIO")

    cameo_eq_setting_100G(card);
    cameo_drv_close()   
    res = cameo_get_result(card,'GearBox100G')
    cameo_write_result (card,res)
    return res
def cameo_main_50G_gearbox(card):    
    #gSlice=0
    #slices=[0,1]
    global gCard
    gCard = card    
    cameo_drv_open()
                         
    for num in range(1,5):
        cameo_change_chip_card(num,card)        
        mdio_status = get_mdio_status()

        if (mdio_status == MDIO_CONNECTED):            
            sel_slice(0)
            if not fw_loaded(print_en=0):
                print("\n*** No FW Loaded. %d\n" % num)             
                fw_load(gFwFileName,slice=[0,1])
                
            sel_slice(0)
            soft_reset()
            sel_slice(1)
            soft_reset()  
            config_baldeagle_gearbox(slice=[0,1], A_lanes=[0,1,4,5],gearbox_type='50G', gearbox_by_fw=True, fec_b_bypass=False)
        else:
            print ("Failed to access MDIO")

    cameo_eq_setting_100G(card);
    cameo_drv_close()          
    res = cameo_get_result(card,'GearBox50G')
    cameo_write_result (card,res)
    return res    
def cameo_main_400G(card,mode='retimer_pam4'):       
    gSlice=0
    #slices=[0,1]
    global gCard
    gCard = card
    cameo_drv_open()
                 
    for num in range(1,3):
        cameo_change_chip_card(num,card)        
        mdio_status = get_mdio_status()
        if (mdio_status == MDIO_CONNECTED):
            sel_slice(0)            
            if not fw_loaded(print_en=0):
                print("\n*** No FW Loaded.\n")
                fw_load(gFwFileName,slice=[0,1])
            sel_slice(0)
            soft_reset()
            sel_slice(1)
            soft_reset()            
            config_baldeagle(slice=[0,1], mode='retimer_pam4',input_mode='ac',lane=None,cross_mode=False)
            RxPolarityMap.append([]); RxPolarityMap[0]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0]
            TxPolarityMap.append([]); TxPolarityMap[0]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1]
            RxPolarityMap.append([]); RxPolarityMap[1]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0]
            TxPolarityMap.append([]); TxPolarityMap[1]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1]
            sel_slice(0)
            for ln in gLane:
                pol(TxPolarityMap[gSlice][ln],RxPolarityMap[gSlice][ln],ln,0)
            sel_slice(1)
            for ln in gLane:
                pol(TxPolarityMap[gSlice][ln],RxPolarityMap[gSlice][ln],ln,0)

        else:
            print ("Failed to access MDIO")

    cameo_eq_setting_400G(card)
    cameo_drv_close()        
    res = cameo_get_result(card,'RetimerPam4')
    cameo_write_result (card,res)
    return res        
def cameo_main_loopback(card,mode='LoopbackPam4',speed='100G'):
    gSlice=0
    #slices=[0,1]
    global gCard
    gCard = card
    cameo_drv_open()
                 
    for num in range(1,5):
        cameo_change_chip_card(num,card)        
        mdio_status = get_mdio_status()
        if (mdio_status == MDIO_CONNECTED):
            sel_slice(0)            
            if not fw_loaded(print_en=0):
                print("\n*** No FW Loaded.\n")
                fw_load(gFwFileName,slice=[0,1])
            sel_slice(0)
            soft_reset()
            sel_slice(1)
            soft_reset()            
            config_baldeagle(slice=[0,1], mode='LoopbackPam4',input_mode='ac',lane=None,cross_mode=False)

            sel_slice(0)
            if speed == '400G':
                print("\n*** 400G change polarity mapping\n")
                RxPolarityMap.append([]); RxPolarityMap[0]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0]
                TxPolarityMap.append([]); TxPolarityMap[0]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1]
                RxPolarityMap.append([]); RxPolarityMap[1]=[0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0]
                TxPolarityMap.append([]); TxPolarityMap[1]=[1,1,1,1,1,1,1,1,   1,1,1,1,1,1,1,1]
                sel_slice(0)
                for ln in gLane:
                    pol(TxPolarityMap[gSlice][ln],RxPolarityMap[gSlice][ln],ln,0)
                sel_slice(1)
                for ln in gLane:
                    pol(TxPolarityMap[gSlice][ln],RxPolarityMap[gSlice][ln],ln,0)

    if speed == '400G':
        cameo_eq_setting_400G(card)
    elif speed == '100G':
        cameo_eq_setting_100G(card)
    cameo_drv_close()
    #return cameo_get_result(card,'RetimerPam4')      
def cameo_main_10Gx2_retimer(card):       
    #gSlice=0
    #slices=[0,1]
    global gCard
    gCard = card
    cameo_drv_open()
                 
    for num in range(1,5):
        cameo_change_chip_card(num,card)        
        mdio_status = get_mdio_status()
        if (mdio_status == MDIO_CONNECTED):
            sel_slice(0)            
            if not fw_loaded(print_en=0):
                print("\n*** No FW Loaded.\n")
                fw_load(gFwFileName,slice=[0,1])
            sel_slice(0)
            soft_reset()
            sel_slice(1)
            soft_reset()            
            config_baldeagle(slice=[0,1], mode='nrz-retimer-10G',input_mode='ac',lane=None,cross_mode=False)
        else:
            print ("Failed to access MDIO")

    cameo_eq_setting_100G(card)        
    cameo_drv_close()    
    res = cameo_get_result(card,'RetimerNrz')
    cameo_write_result (card,res)
    return res          
def cameo_rx_monitor(card):
    cameo_drv_open()
    slice=[0,1]
    slices = get_slice_list(slice) 
    for chip in range(1,5):
        cameo_change_chip_card(chip,card)
        mdio_status = get_mdio_status()
           
        if (mdio_status == MDIO_CONNECTED):
            for slice_num in slices:
                sel_slice(slice_num)            
                print("+-------------------------------+" )
                print("|     Card %02d Chip %02d Sl %02d     |" % (card,chip,slice_num))
                print("+-------------------------------+" )
                rx_monitor()  
    cameo_drv_close()  
def cameo_get_result(card,mode):
    cameo_drv_open()
    slice=[0,1]
    slices = get_slice_list(slice) 
    check_result = True
    card_exist = False
    for chip in range(1,5):
        cameo_change_chip_card(chip,card)
        mdio_status = get_mdio_status()           
        if (mdio_status == MDIO_CONNECTED):  
            card_exist = True
    if card_exist is True:
        if mode.upper() == 'GEARBOX100G':
            config_lane = [0,1,4,5,8,9,10,11,12,13,14,15]
        elif mode.upper() == 'GEARBOX50G':
            config_lane = [0,1,4,5,8,9,10,11,12,13,14,15]            
        elif mode.upper() == 'RETIMERPAM4': 
            config_lane = get_lane_list('all')
        elif mode.upper() == 'RETIMERNRZ': 
            config_lane = get_lane_list('all')            
        else:
            print ("Invalid mode! Mode should be RetimerPam4,GearBox100G,GearBox50G, RetimerNrz")
            return False
        for chip in range(1,5):
            cameo_change_chip_card(chip,card)
            mdio_status = get_mdio_status()
            if (mdio_status == MDIO_CONNECTED):  
                for slice_num in slices:
                    sel_slice(slice_num)            
                    result = fw_slice_params(lane=config_lane,slice=None,print_en=0)                    
                    if not fw_loaded(print_en=0):
                        #print ("\n...Error in 'fw_slice_params': Slice %d has no FW!"%slice_num),
                        check_result = False    
                    else:
                        for ln in config_lane: 
                            #print result[ln]
                            #print("+-------------------------------+" )
                            #print result[ln][0] 
                            #print result[ln][3]
                            #print '\n'
                            if mode.upper() not in result[ln][0].upper() : 
                                check_result = False 
    else:
        check_result = False 
    cameo_change_chip_card(1,card)
    #fw_slice_params(lane=config_lane,slice=0,print_en=1) 
    cameo_drv_close()          
    #print check_result    
    return check_result
def optic_mode_print(lane=None):

    optic_fw_reg_addr = 20
    #optic_mode_reg_val = fw_reg_rd(optic_fw_reg_addr)
    
    #optic_en_list  = ['ON','EN','OPTIC']
    #optic_dis_list = ['OFF','DIS','COPPER']
    mode_list= ['dis','EN']

    #lanes = get_lane_list(lane)
    result = {}
    
    print("-----------------------------------"),
    print("  FW Optic Mode Status Per Lane"),
    print("  -----------------------------------"),
    print("\n           Lane:"),
    
    for ln in range(16):
        print(" %4s" %(lane_name_list[ln])),
        
    for ln in range(16): # Read relavant Registers
        lane_optic_mode =(fw_reg_rd(optic_fw_reg_addr) >> ln) & 1
        result[ln]=lane_optic_mode
        
    
    print("\n FW Optic Mode :"),
    for ln in range(16):
        print(" %4s"  %(mode_list[result[ln]])),
#
# Reg 20 is optical mode register
# Bit0 = lane0
# 0xFF00 is lane 8-15
# tx_taps do not need to change by mode.
#
def cameo_set_Bside_optical_mode(card):

    cameo_drv_open()
    slice=[0,1]
    slices = get_slice_list(slice) 
            
    for chip in range(1,5):
        cameo_change_chip_card(chip,card)
        mdio_status = get_mdio_status()          
        if (mdio_status == MDIO_CONNECTED):
            for slice_num in slices:
                sel_slice(slice_num)   
                print("\n\n")
                print("Card %02d Chip %02d Sl %02d" % (card,chip,slice_num))                          
                if fw_loaded(print_en=0):
                    fw_reg_wr(20,0xFF00)
                    lane_reset() 
                    optic_mode_print()                                                       
    cameo_drv_close()                
def cameo_set_Bside_copper_mode(card):

    cameo_drv_open()
    slice=[0,1]
    slices = get_slice_list(slice) 
    for chip in range(1,5):
        cameo_change_chip_card(chip,card)
        mdio_status = get_mdio_status()          
        if (mdio_status == MDIO_CONNECTED):
            for slice_num in slices:
                sel_slice(slice_num)  
                print("\n\n")
                print("Card %02d Chip %02d Sl %02d" % (card,chip,slice_num))                               
                if fw_loaded(print_en=0):
                    fw_reg_wr(20,0x0000)
                    lane_reset()  
                    optic_mode_print()
    cameo_drv_close()                
def cameo_write_result(card,result):
    
    if result is True:
        result_str = "Card " + str(card) + " : Init Success.\n"
    else:
        result_str = "Card " + str(card) + " : Init Failed.\n"
        
    resultfilename = "card_" + str(card) + "_status"
    f = open(resultfilename,'w')
    f.write(result_str)
    f.close
def cameo_lib_set_taps(card,speed):    
    max_count = 64
    count = 0
    #MAX_LANE_NUM = 16
    #MAX_PARAM_NUM = 5
    # if speed == '400g':
        # MAX_CHIP = 2
    # elif speed == '100g':
        # MAX_CHIP = 4
    for first_col in gCameo100G_EQ:
        for item in first_col:
            if item['card'] == card :  
                cameo_change_chip_card(item['chip'],card)
                sel_slice(item['slice'])
                tx_taps(tap1=item['tap1'],tap2=item['tap2'] ,tap3=item['tap3'] ,tap4=item['tap4'],tap5=item['tap5'],lane=item['lane'])
                count = count + 1
                if count == max_count:
                    return
                    
    # for i in range(MAX_LANE_NUM):        
        # for j in range(MAX_PARAM_NUM):

#################################################################################################### 
#################################################################################################### 
# 
#                  END of all Bald Eagle Scripts
#
# Every time executing this file:
# Call the following functions first to establish basic connection 
#                or to define certain global reference parameters
#   
####################################################################################################
####################################################################################################
if __name__ == '__main__':
    filename = ' '
    #filename=sys.argv[0]
    if len(sys.argv)>1:
        var1=sys.argv[1]
    if len(sys.argv)>2:
        var2=sys.argv[2]        
    if len(sys.argv)>3:
        var3=sys.argv[3]           
    
    if len(sys.argv) == 3 and var1 =='fw_load':
#        gFwFileName = fw_path + '/BE2.fw.2.18.43.bin'
        card=int(var2)
        if card>8 or card<1:
            print ("card must be 1-8")
        cameo_fw_load_2_card(card)
    elif len(sys.argv) == 3 and var1 =='rx_monitor':
        card=int(var2)
        if card>8 or card<1:
            print ("card must be 1-8")
        cameo_rx_monitor(card)       
    elif len(sys.argv) == 3 and var1 =='fw_unload':
        card=int(var2)
        if card>8 or card<1:
            print ("card must be 1-8")
        cameo_fw_unload(card)            
    elif len(sys.argv) == 2 and var1 =='fw_status':
        cameo_fw_load_status_all()
    elif len(sys.argv) == 2 and var1 =='fw_unload':
        cameo_fw_unload_all()        
    elif len(sys.argv) == 3 and var1 == '100G':
        card=int(var2)
        if card>8 or card<1:
            print ("card must be 1-8")
        cameo_main_100G(card)
    elif len(sys.argv) == 3 and var1 == '100G-LT':
        card=int(var2)
        if card>8 or card<1:
            print ("card must be 1-8")
        cameo_main_100G(card,gearbox_type='100G-1-LT')
    elif len(sys.argv) == 3 and var1 == '400G':
        card=int(var2)
        if card>8 or card<1:
            print ("card must be 1-8")
        cameo_main_400G(card)
    elif len(sys.argv) == 3 and var1 == '400G-LT':
        card=int(var2)
        if card>8 or card<1:
            print ("card must be 1-8")
        cameo_main_400G(card,mode='retimer_pam4-LT')
    elif len(sys.argv) == 3 and var1 == 'optical_mode':
        card=int(var2)
        if card>8 or card<1:
            print ("card must be 1-8")
        cameo_set_Bside_optical_mode(card)        
    elif len(sys.argv) == 3 and var1 == 'copper_mode':
        card=int(var2)
        if card>8 or card<1:
            print ("card must be 1-8")
        cameo_set_Bside_copper_mode(card)
    elif len(sys.argv) == 3 and var1 == '10G_retimer':
        card=int(var2)
        if card>8 or card<1:
            print ("card must be 1-8")
        cameo_main_10Gx2_retimer(card) 
    elif len(sys.argv) == 3 and var1 == '50G_gearbox':
        card=int(var2)
        if card>8 or card<1:
            print ("card must be 1-8")
        cameo_main_50G_gearbox(card)   
    elif len(sys.argv) == 3 and var1 == 'lb_100G':
        card=int(var2)
        if card>8 or card<1:
            print ("card must be 1-8")
        cameo_main_loopback(card)   
    elif len(sys.argv) == 3 and var1 == 'lb_400G':
        card=int(var2)
        if card>8 or card<1:
            print ("card must be 1-8")
        cameo_main_loopback(card,mode='LoopbackPam4',speed='400G')        
    elif len(sys.argv) == 4 and var1 == 'result':
        card=int(var2)
        if card>8 or card<1:
            print ("card must be 1-8")
        get_result = cameo_get_result(card,var3)
        print (get_result)
    else:
        print ("usage: %s fw_load [card]" % filename)
        print ("       %s 100G [card]" % filename)
        print ("       %s 400G [card]" % filename)
        print ("       %s fw_status" % filename)
        print ("       %s fw_unload" % filename)
        print ("       %s fw_unload [card]" % filename)
        print ("       %s rx_monitor [card]" % filename)
        print ("       %s optical_mode [card]" % filename)
        print ("       %s copper_mode [card]" % filename)
        print ("       %s 10G_retimer [card]" % filename)
        print ("       %s 50G_gearbox [card]" % filename)
        print ("       %s lb_100G [card]" % filename)
        print ("       %s lb_400G [card]" % filename)
        print ("       %s result [card] [mode]" % filename)
        
    
   
####################################################################################################    
####################################################################################################    
