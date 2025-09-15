#!/usr/bin/env python3

import sys
import os
import glob

cfg = {
    'nb':{
        'untrimmed_t': [-3.5451E-13, 4.0015E-9, 2.0541E-5, 9.5040E-02, 55.733],
        'k_t': [0.00021375, 0.035818, 2.5039],
        'trimmed_t': [-3.5451E-13, 4.0015E-09, 2.0541E-05, 9.5040E-02, 55.733],
        'trimo':128
    },
    'kg':{
        'untrimmed_t': [-1.8439E-11, 8.0705E-08, 1.8501E-4, 3.2843E-01, 48.69],
        'k_t': [7.000E-5, 9.500E-3, 8.571E-1],
        'trimmed_t': [-1.8439E-11, 8.0705E-08, 1.8501E-4, 3.2843E-01, 48.69],
        'trimo':32
    }
}

def calculate_fpga_pvt_temperature(value, chip_type):
    TRIMO = 0
    TRIMG = 15
    if chip_type not in cfg:
        return round(0, 1)

    calc_cfg = cfg[chip_type]
    untrimmed_t_cfg = calc_cfg['untrimmed_t']
    k_t_cfg = calc_cfg['k_t']
    trimmed_t_cfg = calc_cfg['trimmed_t']

    untrimmed_t = untrimmed_t_cfg[0]*pow(value,4) + untrimmed_t_cfg[1]*pow(value,3) - untrimmed_t_cfg[2]*pow(value,2) + untrimmed_t_cfg[3]*(value) - untrimmed_t_cfg[4] 
    k_t =  k_t_cfg[0]*pow(untrimmed_t,2) + k_t_cfg[1]*untrimmed_t + k_t_cfg[2]
    if TRIMO >= calc_cfg['trimo']:
        trimmed_outdata = value + (TRIMG-15) * k_t +(TRIMO-calc_cfg['trimo'])
    else:
        trimmed_out_data = value + (TRIMG-15) * k_t - TRIMO
    trimmed_t = trimmed_t_cfg[0]*pow(trimmed_out_data,4) + trimmed_t_cfg[1]*pow(trimmed_out_data,3) - trimmed_t_cfg[2]*pow(trimmed_out_data,2) + trimmed_t_cfg[3]*trimmed_out_data - trimmed_t_cfg[4]

    return round(trimmed_t,1)

def main(chip_type):
    paths = glob.glob('/sys/**/pvt_temp*_input', recursive=True)
    if len(paths) == 0:
        print("no pvt temp node find, pls check driver")
        return
    for node in paths:
        try:
            with open(node, 'r', encoding='utf-8') as fd:
                result = fd.read().strip()
        except IOError:
            result = ""
        if len(result) == 0:
            print("read %s fail" % node)
            continue
        val = result
        val = calculate_fpga_pvt_temperature(int(val), chip_type)
        print("%s is %f" % (node, val))
    return

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("%s chip_type" % __file__)
        exit(-1)
    main(sys.argv[1])
