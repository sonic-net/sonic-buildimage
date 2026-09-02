#!/usr/bin/python3.9

# Front panel interfaces
import yaml
import sys
import os
import copy
import shutil 
import subprocess
import re

class Pbo:
    def __init__(self):
        self.defaultMtu = 9126
        self.deviceDir = "/usr/share/sonic/device/x86_64-supermicro_sse_t7132s-r0"
#        self.deviceDir = "./"
        self.configDbDotJson = "/etc/sonic/config_db.json"
        self.refferenceSkuDir = "{}/Supermicro_sse_t7132s".format(self.deviceDir)
        self.CustomSkuDir = "{}/t7132CustomSKU".format(self.deviceDir) 
        self.default_sku = "{}/default_sku".format(self.deviceDir)
        self.CustomConfigDotYamlName = "config_t7132CustomSKU.yaml"
        self.reffConfigDotYamlName = "config_32x400G_sse_t7132s.yaml"
        self.configDotYaml =  "{}/{}".format(self.CustomSkuDir, self.CustomConfigDotYamlName)
        self.portConfigDotIni = "{}/port_config.ini".format(self.CustomSkuDir)
        self.ivmSaiConfigDotYaml = "{}/ivm.sai.config.yaml".format(self.CustomSkuDir)
        self.buffersJsonDotJ2 = "{}/buffers.json.j2".format(self.CustomSkuDir)
        self.buffersDefT1DotJ2 = "{}/buffers_defaults_t1.j2".format(self.CustomSkuDir)
        self.preDefinedSkus = { 
                                '32x400G': 'Supermicro_sse_t7132s',
                                '128x100G': 'Supermicro_sse_t7132s_128x100',
                                '16x400G-64x100G': 'Supermicro_sse_t7132s_16x400_64x100',
                                '32x100G': 'Supermicro_sse_t7132s_32x100',
                                '64x100G': 'Supermicro_sse_t7132s_64x100',
                                '64x200G': 'Supermicro_sse_t7132s_64x200'
                                }
        self.fixedBoPort = ['no', 'no', 'no', 'no', 'no', 'no', 'no', 'no', 'no', 'no',
                            'no', 'no', 'no', 'no', 'no', 'no', 'no', 'no', 'no',  'no',
                            'no', 'no', 'no', 'no', 'no', 'no', 'no', 'no', 'no', 'no',
                            'no',  'no','2x10G']

        self.startLaneId = [241, 249, 225, 233, 217, 209, 201, 193, 185, 177,
                            169, 161, 153, 145, 137, 129, 121, 113, 105,  97,
                            89,   81,  73,  65,  57,  49,  41,  33,  25,  17,
                             9,    1, 257]


        self.serdesgroup = ['30','31','28','29','27','26','25','24','23','22',
                            '21','20','19','18','17','16','15','14','13','12',
                            '11','10', '9', '8', '7', '6', '5', '4', '3', '2',
                             '1', '0', '32']
        self.maxPboLimit = 168
        self.confPboCount = 0 
        self.numFpi = len(self.startLaneId)
        self.profiles = {"1x400G":1, #DO NOT CHANGE THIS MAPPING. You may change the order in the list though.
                          "1x200G":2,
                          "2x200G":3,
                          "1x100G":4,
                          "2x100G":5,
                          "4x100G":6, 
                          "1x40G":7,
                          "2x40G":8,
                          "8x10G":9,
                          "4x10G":10,
                          "4x25G":11,
                          "8x25G":12
                          }
        self.FixedProfiles = {"2x10G":13 }
        
        #breakout count for above profile map
        self.breakoutCountIdMap = [1,1,2,1,2,4,1,2,8,4,4,8,2] 


        self.fpiList = [{}]

        self.confyaml = { 'ifcs': {'options': {'log_level': 'info'}},
                          'nodes': [ { 'node_id': '0',
                                       'options' : { 
                                         'sd_low_power_mode_global_default': 'true',
                                         'sku': 'configs/sku/innovium.77700_A',
                                         'netdev': [ { 'auto_create': 'no',
                                                       'multi_interface': 'yes'}],
                                         'pcie_attn': '10, 0, 0, 0',
                                         'pcie_post': '10, 0, 0, 0',
                                         'pcie_pre1': '0, 0, 0, 0' ,
                                         'buffer_management_mode': 'api_driven',
                                         'wred_cr_ip_proto_list': '17',
                                         'cr_assignment_mode': '1',
                                         'max_lossless_tc': '2',
                                         'ilpm_enable': '1',
                                         'forward_profile': 'IFCS_FORWARD_PROFILE_ID_PROFILE_E',
                                         'led_cfg_sck_rate': '0x5',
                                         'led_refresh_precliff_timer': '0x18eec2',
                                         'led_refresh_cliff_timer': '0x15e',
                                         'led_cfg_pic_stream_mode': '1',
                                         'led_refresh_tmr_ctl_enable': '1',
                                         'txring': [ { 
                                                       'txring_id': '0',
                                                        'desc_count': '1024',
                                                       'prio': '1',
                                                       'netdev': 'true'
                                                       },
                                                     {
                                                       'txring_id': '1',
                                                       'desc_count': '1024',
                                                       'prio': '1',
                                                       'netdev': 'true'
                                                       },
                                                     {
                                                       'txring_id': '2',
                                                         'desc_count': '1024',
                                                       'prio': '1',
                                                       'netdev': 'true'
                                                       },
                                                     { 
                                                       'txring_id': '3',
                                                        'desc_count': '1024',
                                                       'prio': '1',
                                                       'netdev': 'true'
                                                       }],

                                         'rxring': [ {
                                                       'rxring_id': '0',
                                                       'desc_count': '1024',
                                                       'prio': '1',
                                                       'netdev': 'true',
                                                       'queues': '0, 3, 6, 9, 12, 15, 18, '
                                                                 '21, 24, 27, 30, 33, 36, '
                                                                 '39'
                                                       },
                                                     { 
                                                       'rxring_id': '1',
                                                       'desc_count': '1024',
                                                       'prio': '1',
                                                       'netdev': 'true',
                                                       'queues': '1, 4, 7, 10, 13, 16, 19, '
                                                                 '22, 25, 28, 31, 34, 37, '
                                                                 '40'
                                                       },
                                                     { 
                                                       'rxring_id': '2',
                                                       'desc_count': '1024',
                                                       'prio': '1',
                                                       'netdev': 'true',
                                                       'queues': '2, 5, 8, 11, 14, 17, 20, '
                                                                 '23, 26, 29, 32, 35, 38, '
                                                                 '41, 47'
                                                       },
                                                     { 
                                                       'rxring_id': '3',
                                                       'desc_count': '1024',
                                                       'prio': '1',
                                                       'queues': '42, 43, 44, 45, 46'
                                                       }],

                                         'sys_clk': '1720',
                                         'ifc_clk': '1200',
                                         'mac_clk': '1340' },

                                      'devports': [ {'id': '0', 'sysport': '1000', 'type': 'cpu'} ],
                                      'isg': [ { 'id': '0',
                                                 'tx_polarity': '10100000',
                                                 'rx_polarity': '11111011',
                                                 'lane_swap': '37250416'
                                                 },
                                               { 'id': '1',
                                                 'tx_polarity': '01010011',
                                                 'rx_polarity': '00000100',
                                                 'lane_swap': '52407613'
                                                 },
                                               { 'id': '2',
                                                 'tx_polarity': '11010001',
                                                 'rx_polarity': '01111100',
                                                 'lane_swap': '06153427'
                                                 },
                                               { 'id': '3',
                                                 'tx_polarity': '00100000',
                                                 'rx_polarity': '10001001',
                                                 'lane_swap':   '74501263' 
                                                 },
                                               { 'id': '4',
                                                 'tx_polarity': '10100000',
                                                 'rx_polarity': '11101000',
                                                 'lane_swap':   '05471632' 
                                                 },
                                               { 'id': '5',
                                                 'tx_polarity': '00010100',
                                                 'rx_polarity': '00111100',
                                                 'lane_swap':   '72604351' 
                                                 },
                                               { 'id': '6',
                                                 'tx_polarity': '11011001',
                                                 'rx_polarity': '00011001',
                                                 'lane_swap':   '16340725' 
                                                 },
                                               { 'id': '7',
                                                 'tx_polarity': '11010000',
                                                 'rx_polarity': '11000010',
                                                 'lane_swap':   '70615324' 
                                                 },
                                               { 'id': '8',
                                                 'tx_polarity': '00111101',
                                                 'rx_polarity': '11011000',
                                                 'lane_swap':   '25074613' 
                                                 },
                                               { 'id': '9',
                                                 'tx_polarity': '00001010',
                                                 'rx_polarity': '01000011',
                                                 'lane_swap':   '32706451' 
                                                 },
                                               { 'id': '10',
                                                 'tx_polarity': '00100010',
                                                 'rx_polarity': '01001011',
                                                 'lane_swap':   '07162543' 
                                                 },
                                               { 'id': '11',
                                                 'tx_polarity': '01101001',
                                                 'rx_polarity': '11110001',
                                                 'lane_swap':   '41706253' 
                                                 },
                                               { 'id': '12',
                                                 'tx_polarity': '11001000',
                                                 'rx_polarity': '11000011',
                                                 'lane_swap': '07136524'
                                                 },
                                               { 'id': '13',
                                                 'tx_polarity': '01100001',
                                                 'rx_polarity': '10010000',
                                                 'lane_swap': '73506412'
                                                 },
                                               { 'id': '14',
                                                 'tx_polarity': '01010001',
                                                 'rx_polarity': '10110110',
                                                 'lane_swap': '26143705'
                                                 },
                                               { 'id': '15',
                                                 'tx_polarity': '00001000',
                                                 'rx_polarity': '11101100',
                                                 'lane_swap': '51602437'
                                                 },
                                               { 'id': '16',
                                                 'tx_polarity': '00010000',
                                                 'rx_polarity': '11101011',
                                                 'lane_swap': '45076312'
                                                 },
                                               { 'id': '17',
                                                 'tx_polarity': '01011000',
                                                 'rx_polarity': '00000000',
                                                 'lane_swap': '50642371'
                                                 },
                                               { 'id': '18',
                                                 'tx_polarity': '01010100',
                                                 'rx_polarity': '00011001',
                                                 'lane_swap': '07436125'
                                                 },
                                               { 'id': '19',
                                                 'tx_polarity': '00011010',
                                                 'rx_polarity': '01001011',
                                                 'lane_swap': '61734250'
                                                 },
                                               { 'id': '20',
                                                 'tx_polarity': '00111110',
                                                 'rx_polarity': '10011100',
                                                 'lane_swap': '04275631'
                                                 },
                                               { 'id': '21',
                                                 'tx_polarity': '10110100',
                                                 'rx_polarity': '01110110',
                                                 'lane_swap': '41620573'
                                                 },
                                               { 'id': '22',
                                                 'tx_polarity': '01100110',
                                                 'rx_polarity': '10010000',
                                                 'lane_swap': '17240635'
                                                 },
                                               { 'id': '23',
                                                 'tx_polarity': '01010000',
                                                 'rx_polarity': '11110101',
                                                 'lane_swap': '52704631'
                                                 },
                                               { 'id': '24',
                                                 'tx_polarity': '00010001',
                                                 'rx_polarity': '10100100',
                                                 'lane_swap': '16253704'
                                                 },
                                               { 'id': '25',
                                                 'tx_polarity': '01000101',
                                                 'rx_polarity': '00010000',
                                                 'lane_swap': '53607241'
                                                 },
                                               { 'id': '26',
                                                 'tx_polarity': '00110101',
                                                 'rx_polarity': '11101110',
                                                 'lane_swap': '16074325'
                                                 },
                                               { 'id': '27',
                                                 'tx_polarity': '10000111',
                                                 'rx_polarity': '01011110',
                                                 'lane_swap': '75604231'
                                                 },
                                               { 'id': '28',
                                                 'tx_polarity': '01010100',
                                                 'rx_polarity': '01010101',
                                                 'lane_swap': '70614235'
                                                 },
                                               { 'id': '29',
                                                 'tx_polarity': '01010001',
                                                 'rx_polarity': '01000001',
                                                 'lane_swap': '24610537'
                                                 },
                                               { 'id': '30',
                                                 'tx_polarity': '01101011',
                                                 'rx_polarity': '01010011',
                                                 'lane_swap': '70614352'
                                                 },
                                               { 'id': '31',
                                                 'tx_polarity': '01101001',
                                                 'rx_polarity': '10100000',
                                                 'lane_swap': '34250716'
                                                 },
                                               { 'id': '32',
                                                 'tx_polarity': '00000000',
                                                 'rx_polarity': '00000000',
                                                 'lane_swap': '01234567'
                                                 }]}]}

        self.yamlKeys = ['sd_low_power_mode_global_default','wred_cr_ip_proto_list','cr_assignment_mode', 'txring_id', 'prio', 'netdev', 'desc_count', 'txring', 'sys_clk', 'sku', 'rxring_id', 'rxring', 'queues', 'pcie_pre1', 'pcie_post', 'pcie_attn','multi_interface', 'auto_create', 'max_lossless_tc', 'mac_clk', 'led_refresh_tmr_ctl_enable' , 'led_refresh_precliff_timer', 'led_refresh_cliff_timer', 'led_cfg_sck_rate', 'led_cfg_pic_stream_mode', 'ilpm_enable', 'ifc_clk', 'forward_profile', 'ecn_stats_enable', 'buffer_management_mode', 'options', 'node_id', 'tx_polarity', 'rx_polarity', 'lane_swap', 'id','isg', 'fec', 'lanes', 'serdes_group', 'speed', 'sysport', 'type', 'devports', 'nodes', 'log_level', 'options', 'ifcs']

        self.portInnoBlockMap = [2, 0, 2, 2 ,2, 2,
                                 4, 4, 4, 4, 4,
                                 3, 3, 3, 3, 3,
                                 1, 1, 1, 1, 1, 1,
                                 5, 5, 5, 5, 5,
                                 0, 0, 0, 0, 0]

        self.innoBlockInfoDefault = {0 : {'maxLp': 32, 
                                    'minLp': 6,
                                     'breakoutIdMenu': {}},
                              1 : {'maxLp': 32, 
                                    'minLp': 6,
                                     'breakoutIdMenu': {}},
                              2 : {'maxLp': 32, 
                                    'minLp': 6,
                                     'breakoutIdMenu': {}},
                              3 : {'maxLp': 32, 
                                    'minLp': 6,
                                     'breakoutIdMenu': {}},
                              4 : {'maxLp': 20, 
                                    'minLp': 5,
                                     'breakoutIdMenu': {}},
                              5 : {'maxLp': 20, 
                                    'minLp': 5,
                                     'breakoutIdMenu': {}}
                               }


        self.intfNamesMgmt = []
        self.intfNames400G = []
        self.intfNames200G = []
        self.intfNames100G = []
        self.intfNames50G = []
        self.intfNames40G = []
        self.intfNames25G = []
        self.intfNames10G = []
        self.innoBlockInfo = dict() 
        self.initTables()
        self.bufIntfName_re = re.compile(r"(^\s*{%(\s*set\s+)(mgmt_port_name|port_name_400G|port_name_200G|port_name_100G|port_name_50G|port_name_40G|port_name_25G|port_name_10G)[^}%]+%}\s*)+", re.MULTILINE )

    def initTables (self):
        self.innoBlockInfo = copy.deepcopy(self.innoBlockInfoDefault) 
        self.getDefaultIntfList()
        self.setDefaultBreakOutIdMenu()

    def setDefaultBreakOutIdMenu(self):
        for k, ib in self.innoBlockInfo.items():
            ib['breakoutIdMenu'] = set(range(1,13))

    def cmdHelp(self):
        print("\n{:<10}:{}\n{:<10}:{}\n{:<10}:{}\n{:<10}:{}".format("show",
                 "Shows port breakout configured in this session.Shows the default if nothing configured ",
                 "config", "Configure Port breakout and hold it in a temporary memory",
                 "write","Permanently write this breakout configurations to device files",
                 "quit", "Exit from this session"
                 )) 

    def updateInterfaceNames(self, intfName, speed, flag = "externalPorts" ):
        if (flag == "internalMgmtPort"):
            self.intfNamesMgmt.append(intfName)
        elif int(speed) == 400000:
            self.intfNames400G.append(intfName)
        elif int(speed) == 200000:
            self.intfNames200G.append(intfName)
        elif int(speed) == 100000:
            self.intfNames100G.append(intfName)
        elif int(speed) == 50000:
            self.intfNames50G.append(intfName)
        elif int(speed) == 40000:
            self.intfNames40G.append(intfName)
        elif int(speed) == 25000:
            self.intfNames25G.append(intfName)
        elif int(speed) == 10000:
            self.intfNames10G.append(intfName)
        else :
            print("Error: Port List issue!! Unknowin speed {} for intf {} ".format(speed, intfName))

    def writeBufferJ2(self):
        delimit = ","
        list2str = lambda l, s : s.join( "'" + str(i) + "'" for i in l) 
        #appendList = lambda b, l, s, n:  b + "{% set " + n + " = [" + list2str(l, s) + "] %}\n" if len(l) > 0 else b
        appendList = lambda b, l, s, n:  b + "{% set " + n + " = [" + list2str(l, s) + "] %}\n" 
        bufFiles = [self.buffersJsonDotJ2, self.buffersDefT1DotJ2]

        newStr = "{% set mgmt_port_name = [" + list2str (self.intfNamesMgmt,delimit) + "] %}\n" 
        newStr = appendList(newStr, self.intfNames400G , delimit, "port_name_400G")
        newStr = appendList(newStr, self.intfNames200G , delimit, "port_name_200G")
        newStr = appendList(newStr, self.intfNames100G , delimit, "port_name_100G")
        newStr = appendList(newStr, self.intfNames50G , delimit, "port_name_50G")
        newStr = appendList(newStr, self.intfNames40G , delimit, "port_name_40G")
        newStr = appendList(newStr, self.intfNames25G , delimit, "port_name_25G")
        newStr = appendList(newStr, self.intfNames10G , delimit, "port_name_10G")
      
        for fileName in bufFiles:
            with open(fileName) as bufFile:
                bufFileStr = bufFile.read()

            if self.bufIntfName_re.search(bufFileStr) is None:
                prinf("Error buffers.json.j2 file missing the default port names")
                sys.exit()
            
            new_bufFileStr = self.bufIntfName_re.sub(newStr , bufFileStr, count=1)

            with open(fileName, 'w') as bufFile:
                bufFile.write(new_bufFileStr)

    def getLanes(self, start, count):
        lns=""
        end = start + count
        for ln in range(start,end):
            lns += "{},".format(ln)
        return (lns[:-1])
      
#    def writeLinePortConfigDotIni(self, f, intf, index, speed, lanes , fec):
#        line ="Ethernet{0:<12}{1:<36}{2:<12}{3:<12}{4:<8}{5:<9}{6}\n"
#        for bo in range (intf['breakOutCount']):
#            if intf['breakOutCount'] == 1 or intf['fixedBoPort'] == 'True':
#                alias = "{}{}".format(intf['alias'], index + bo +1) 
#                f.write(line.format(index*8 + bo, 
#                            self.getLanes(intf['startLaneId'] + (bo * lanes), lanes),
#                            alias, speed,index + bo + 1, intf['mtu'], fec ))
#            else:
#                alias = "{}{}-{}".format(intf['alias'], index + 1, bo +1) 
#                f.write(line.format(index*8 + bo, 
#                            self.getLanes(intf['startLaneId'] + (bo * lanes), lanes),
#                            alias, speed,index + 1, intf['mtu'], fec ))
#  
#            if intf['fixedBoPort'] == 'True':
#                self.updateInterfaceNames("Ethernet{}".format(index*8 + bo), speed, flag = "internalMgmtPort" )
#            else:
#                self.updateInterfaceNames("Ethernet{}".format(index*8 + bo), speed )
     
    def writeLinePortConfigDotIni(self, f, intf, index, speed, lanes , fec):
        line ="Ethernet{0:<12}{1:<36}{2:<12}{3:<12}{4:<8}{5:<9}{6}\n"
        step = 1 if intf['fixedBoPort'] == 'True'else 8//intf['breakOutCount']
        for bo in range (intf['breakOutCount']):
            if intf['breakOutCount'] == 1 or intf['fixedBoPort'] == 'True':
                alias = "{}{}".format(intf['alias'], index + bo +1) 
                f.write(line.format(index*8 + (bo * step), 
                            self.getLanes(intf['startLaneId'] + (bo * lanes), lanes),
                            alias, speed,index + bo + 1, intf['mtu'], fec ))
            else:
                alias = "{}{}-{}".format(intf['alias'], index + 1, bo +1) 
                f.write(line.format(index*8 + (bo * step), 
                            self.getLanes(intf['startLaneId'] + (bo * lanes), lanes),
                            alias, speed,index + 1, intf['mtu'], fec ))

            if intf['fixedBoPort'] == 'True':
                self.updateInterfaceNames("Ethernet{}".format(index*8 + (bo * step)), speed, flag = "internalMgmtPort" )
            else:
                self.updateInterfaceNames("Ethernet{}".format(index*8 + (bo * step)), speed )
     
    def writePortConfigDotIni(self):
        f = open(self.portConfigDotIni, "w")
        f.write("{0:<20}{1:<36}{2:<12}{3:<12}{4:<8}{5:<9}{6}\n".format("# name",
                                    "lanes","alias","speed","index", "mtu" , "fec"))
        index = 0
        for intf in self.fpiList :
            if intf['speed'] == "400G":
                self.writeLinePortConfigDotIni(f, intf, index, "400000",8 , "rs")
            elif intf['speed'] == "200G":
                if (intf['breakOutCount'] == 1):
                    if intf['encoding'] == 'NRZ': #this case is obsoleted
                        self.writeLinePortConfigDotIni(f, intf, index, "200000",8 , "rs")
                    else: #PAM4
                        self.writeLinePortConfigDotIni(f, intf, index, "200000",4 , "rs")
                elif (intf['breakOutCount'] == 2):#PAM4
                    self.writeLinePortConfigDotIni(f, intf, index, "200000",4 , "rs")
     
            elif intf['speed'] == "100G":
                if (intf['breakOutCount'] == 1):
                    if intf['encoding'] == 'NRZ':
                        self.writeLinePortConfigDotIni(f, intf, index, "100000",4 , "rs")
                    else: #PAM4
                        self.writeLinePortConfigDotIni(f, intf, index, "100000",2 , "rs")
                elif (intf['breakOutCount'] == 2):#always NRZ
                    self.writeLinePortConfigDotIni(f, intf, index, "100000",4 , "rs")
                elif (intf['breakOutCount'] == 4):#always PAM4 
                    self.writeLinePortConfigDotIni(f, intf, index, "100000",2 , "rs")
     
            elif intf['speed'] == "40G":
                if (intf['breakOutCount'] == 1) or (intf['breakOutCount'] == 2):
                    self.writeLinePortConfigDotIni(f, intf, index, "40000",4 , "none")
     
            elif intf['speed'] == "25G":#always NRZ
                if (intf['breakOutCount'] == 8):
                    self.writeLinePortConfigDotIni(f, intf, index, "25000",1 , "rs")
                elif (intf['breakOutCount'] == 4):
                    self.writeLinePortConfigDotIni(f, intf, index, "25000",1 , "rs")
            elif intf['speed'] == "10G":#always NRZ
                if (intf['breakOutCount'] == 8):
                    self.writeLinePortConfigDotIni(f, intf, index, "10000",1 , "none")
                elif (intf['breakOutCount'] == 4):
                    self.writeLinePortConfigDotIni(f, intf, index, "10000",1 , "none")
                elif (intf['breakOutCount'] == 2):
                    self.writeLinePortConfigDotIni(f, intf, index, "10000",1 , "none")
            index +=  1
        f.close()

    def confirmExit(self):
        yn = ["yes", "no"]
        a = input("Do you want to proceed? (yes/no)")
        if a in yn:
            if a == "no":
                sys.exit()
            return a
        else:
            print ("\'{}\' is not one of the listed option".format(a))
            return self.confirmExit() 

    def menu(self):
        k = ["show", "config", "write", "quit"]
        i = input("Enter config/write/show/quit: ")
        if i in k:
            return i
        else:
            print ("\'{}\' is not one of the listed option".format(i))
            return self.menu()
     
    def getDefaultIntfList(self):
        self.confPboCount = 0
        self.fpiList = [ {} for i in range(self.numFpi)]
        for i in range(self.numFpi):
            if self.fixedBoPort[i] == 'no': #not a fixed breakout port 
                self.fpiList[i]['alias'] = "Eth"
                self.fpiList[i]['speed'] = "400G"
                self.fpiList[i]['breakOutCount'] = 1 
                self.fpiList[i]['encoding'] = "PAM4"
                self.fpiList[i]['mtu'] = self.defaultMtu
                self.fpiList[i]['breakOutId'] = 1 
                self.fpiList[i]['fixedBoPort'] = 'False' 
                self.fpiList[i]['startLaneId'] = self.startLaneId[i] 
                self.confPboCount += self.fpiList[i]['breakOutCount'] 

        for i in range(self.numFpi):
            if self.fixedBoPort[i] == '2x10G':
                self.fpiList[i]['alias'] = "Eth"
                self.fpiList[i]['speed'] = "10G"
                self.fpiList[i]['breakOutCount'] = 2 
                self.fpiList[i]['encoding'] = "none"
                self.fpiList[i]['mtu'] = self.defaultMtu 
                self.fpiList[i]['breakOutId'] = 13 
                self.fpiList[i]['fixedBoPort'] = 'True' 
                self.fpiList[i]['startLaneId'] = self.startLaneId[i] 
                self.confPboCount += 2 #incremented here since fixed ports are not user configured 

        return self.fpiList
     
    def getBreakOutId(self,port):
        boIdMenu = self.innoBlockInfo[self.portInnoBlockMap[port - 1]]['breakoutIdMenu']
        maxLp = self.innoBlockInfo[self.portInnoBlockMap[port - 1]]['maxLp']
        print ("Port{}".format(port))
        print ("       Resource Usage:\n\t Global {}/{}\n\t Port Group's {}".format(self.confPboCount, self.maxPboLimit, maxLp))

        ni = input("       Select ID from Menu:{} :".format(boIdMenu))
        if ni in self.profiles.keys():  #'name' has been configured
            boId = self.profiles[ni]
        elif ni.isdigit() is True and int(ni) in self.profiles.values(): #'id' has been configured
            boId = int(ni)
        else:
            print("--------{} is not a valid breakout ID/Name -----".format(ni)) 
            return self.getBreakOutId(port)

        if boId not in boIdMenu:
            print("--------{} is not an option in the ID menu------".format(ni)) 
            return self.getBreakOutId(port)
      
        # subtract 1 Lp from the garunteedLp(minLp) for the configured port 
        self.innoBlockInfo[self.portInnoBlockMap[port - 1]]['minLp'] -= 1

        #update maxLp
        freeLp = self.innoBlockInfo[self.portInnoBlockMap[port - 1]]['maxLp']
        freeLp -= self.breakoutCountIdMap[boId - 1] 
        self.innoBlockInfo[self.portInnoBlockMap[port - 1]]['maxLp'] = freeLp

        return boId
     

    def configFpiPort(self, fpi,  boId):
        if boId==1:
          fpi['speed'] = "400G" 
          fpi['encoding'] = "PAM4"
      # elif boId == 2:
      #   fpi['speed'] = "200G" 
      #   fpi['encoding'] = "NRZ"
        elif boId == 2:
          fpi['speed'] = "200G" 
          fpi['encoding'] = "PAM4"
        elif boId == 3:
          fpi['speed'] = "200G" 
          fpi['encoding'] = "PAM4"
        elif boId == 4:
          fpi['speed'] = "100G" 
          fpi['encoding'] = "NRZ"
        elif boId == 5:
          fpi['speed'] = "100G" 
          fpi['encoding'] = "NRZ"
        elif boId == 6:
          fpi['speed'] = "100G" 
          fpi['encoding'] = "PAM4"
        elif boId == 7:
          fpi['speed'] = "40G" 
          fpi['encoding'] = "NRZ"
        elif boId == 8:
          fpi['speed'] = "40G" 
          fpi['encoding'] = "NRZ"
        elif boId == 9:
          fpi['speed'] = "10G" 
          fpi['encoding'] = "NRZ"
        elif boId == 10:
          fpi['speed'] = "10G" 
          fpi['encoding'] = "NRZ"
        elif boId == 11:
          fpi['speed'] = "25G" 
          fpi['encoding'] = "NRZ"
        elif boId == 12:
          fpi['speed'] = "25G" 
          fpi['encoding'] = "NRZ"
        elif boId == 13:
          fpi['speed'] = "10G" 
          fpi['encoding'] = "none"
        else:
            print("Oops..invalid breakout configured {} ".format(boId))
            return
        
        fpi['breakOutCount'] = self.breakoutCountIdMap[boId -1] 

        fpi['breakOutId'] = boId 
        self.confPboCount += fpi['breakOutCount']
     
    def config(self):
        print ("{:<5}{}".format("ID", "Name"))
        print ("---------------")
        valsetsort = set(self.profiles.values())
        valset = list(self.profiles.values())
        keyset = list(self.profiles.keys())
        for v in valsetsort:
            print ("{:<5}{}".format( v, keyset[valset.index(v)]))
     
        print ("Enter breakout Name or ID for front pannel ports\n") 
        self.confPboCount = 2 #breakout count for fixed port 
        for p in range(self.numFpi):
            if self.fixedBoPort[p] == 'no': # Not a fixed port 
                boId = self.getBreakOutId(p+1) 
                self.updateBreakoutMenuOptions(p)
                self.configFpiPort(self.fpiList[p], boId) 
     
     
    def writeIni(self): 
        self.intfNamesMgmt = []
        self.intfNames400G = []
        self.intfNames200G = []
        self.intfNames100G = []
        self.intfNames50G = []
        self.intfNames40G = []
        self.intfNames25G = []
        self.intfNames10G = []

        self.writePortConfigDotIni()

    def dumpFpiList(self):
        idset = list(self.profiles.values())
        keyset = list(self.profiles.keys())
        for i in range(self.numFpi):
            if self.fixedBoPort[i] == 'no': # Not a fixed port 
                boId = self.fpiList[i]['breakOutId']
                print ("Port {:<5}: {}".format( i+1, keyset[idset.index(boId)]))

     
#---------------------------

    def devportsYamlAdd(self, dpList,fpi,fpiIndex,laneCount,fec):
        s = 0
        for bo in range(fpi['breakOutCount']):
             dp = {}
             dp['fec'] = fec 
             dp['id'] = str(fpi['startLaneId'] + s )
             dp['lanes'] = '{}:{}'.format(s,laneCount) 
             s += laneCount
             dp['serdes_group'] = self.serdesgroup[fpiIndex] 
             dp['speed'] = fpi['speed'] 
             #dp['sysport'] =str(fpi['startLaneId'])
             dp['sysport'] = dp['id']
             if fpi['fixedBoPort'] == 'True': 
                 dp['type'] =  'mgmt {}'.format(bo)
             else: 
                 dp['type'] = 'eth' 
             dpList.append(dp)
      
    def yamlKeyValFormat(self, dumper, data):
        if data in self.yamlKeys:
            return dumper.represent_scalar('tag:yaml.org,2002:str', data, style='')
        else:
            return dumper.represent_scalar('tag:yaml.org,2002:str', data, style='"')
     
      
    def writeYaml(self):
        fpiIndex = 0 
        
        dpList = [{'id': '0', 'sysport':'1000', 'type':'cpu'}]
        for fpi in self.fpiList:
            boId = fpi['breakOutId']
            if boId == 1:#1x400G
                self.devportsYamlAdd(dpList,fpi,fpiIndex,8,"KPFEC")
           #elif boId == 2:#1x200G 8lanes
           #    self.devportsYamlAdd(dpList,fpi,fpiIndex,8,"KRFEC")
            elif boId == 2:#1x200G 4lanes
                self.devportsYamlAdd(dpList,fpi,fpiIndex,4,"KPFEC")
            elif boId == 3:#2x200G
                self.devportsYamlAdd(dpList,fpi,fpiIndex,4,"KPFEC")
            elif boId == 4:#1x100G
                self.devportsYamlAdd(dpList,fpi,fpiIndex,4,"KRFEC")
            elif boId == 5:#2x100G
                self.devportsYamlAdd(dpList,fpi,fpiIndex,4,"KRFEC")
            elif boId == 6:#4x100G
                self.devportsYamlAdd(dpList,fpi,fpiIndex,2,"KPFEC")
            elif boId == 7:#1x40G
                self.devportsYamlAdd(dpList,fpi,fpiIndex,4,"NONE")
            elif boId == 8:#2x40G
                self.devportsYamlAdd(dpList,fpi,fpiIndex,4,"NONE")
            elif boId == 9:#8x10G
                self.devportsYamlAdd(dpList,fpi,fpiIndex,1,"NONE")
            elif boId == 10:#4x10G
                self.devportsYamlAdd(dpList,fpi,fpiIndex,1,"NONE")
            elif boId == 11:#4x25G
                self.devportsYamlAdd(dpList,fpi,fpiIndex,1,"KRFEC")
            elif boId == 12:#8x25G
                self.devportsYamlAdd(dpList,fpi,fpiIndex,1,"KRFEC")
            elif boId == 13:#2x10G
                self.devportsYamlAdd(dpList,fpi,fpiIndex,1,"NONE")
            else:
                print("Yaml: Wrong Breakout Id")
                return
            fpiIndex += 1
        self.confyaml['nodes'][0]['devports'] = dpList
        yaml.add_representer(str, self.yamlKeyValFormat)
        yf = open(self.configDotYaml, "w")
        yaml.dump(self.confyaml, yf, default_flow_style=False, sort_keys = False)
        yf.close()
     
#---------------------------

    def updateIvmSaiConfigYaml(self):
        sc = open(self.ivmSaiConfigDotYaml,'r')
        scData = sc.read()
        sc.close()
        cy = open(self.configDotYaml,'r')
        cyData = cy.read()
        cy.close()

        if self.fpiList[16]['breakOutId'] == 1 or self.fpiList[1]['breakOutId'] == 1 : 
            newScData = scData.replace("innovium.77700_B","innovium.77700_A")
            newCyData = cyData.replace("innovium.77700_B","innovium.77700_A")
        else:
            boId16  = self.fpiList[16]['breakOutId']
            boId1  = self.fpiList[1]['breakOutId']
            boCount16 = self.breakoutCountIdMap[boId16 - 1] 
            boCount1 = self.breakoutCountIdMap[boId1 - 1] 
            freeLp16 = self.innoBlockInfo[3]['maxLp']
            freeLp1 = self.innoBlockInfo[2]['maxLp']
            if freeLp16 == 0 or freeLp1 == 0 or \
               (freeLp16 < (boCount16 / 2)) or (freeLp1 < (boCount1 / 2)) :
                newScData = scData.replace("innovium.77700_B","innovium.77700_A")
                newCyData = cyData.replace("innovium.77700_B","innovium.77700_A")
            else:
                newScData = scData.replace("innovium.77700_A","innovium.77700_B")
                newCyData = cyData.replace("innovium.77700_A","innovium.77700_B")

        sc = open(self.ivmSaiConfigDotYaml,'w')
        sc.write(newScData)
        sc.close()
        cy = open(self.configDotYaml,'w')
        cy.write(newCyData)
        cy.close()

    def applySku(self, skuName):
        
#        cg = subprocess.run(["sudo", "sonic-cfggen", "-H", "-p", "--preset", "t1", "-k", skuName ],
#                capture_output = True, text=True, check=True)
        
#        print(cg.stdout)

#        with open(self.configDbDotJson, 'w+') as f:
#            f.write(cg.stdout)

        cg = subprocess.run(["sudo", "rm", "-rf", self.configDbDotJson ],
                capture_output = True, text=True, check=True)
        with open(self.default_sku, 'w+') as s:
            s.write("{} t1".format(skuName))

        #cg = subprocess.run(["sudo", "sonic-cfggen", "-H", "-p", "--preset", "t1", "-k", "t7132CustomSKU"],
        #        stdout=subprocess.PIPE)
        #print(cg.stdout.decode('utf-8'))
        #print("Reloading config takes a while...")

        #cr = subprocess.run(["sudo", "config", "reload", "/dev/stdin", "-y"], input=cg.stdout, stdout=subprocess.PIPE)
        #print(cr.stdout.decode('utf-8'))
       
       

    def createSkuDir(self):
        if not os.path.exists(self.CustomSkuDir):
            if not os.path.exists(self.refferenceSkuDir):
                print("Dirictory {} doesn't exist".format(self.refferenceSkuDir))
                sys.exit()
            shutil.copytree(self.refferenceSkuDir, self.CustomSkuDir)

            shutil.move("{}/{}".format(self.CustomSkuDir, self.reffConfigDotYamlName), 
                        "{}/{}".format(self.CustomSkuDir, self.CustomConfigDotYamlName))

            sc = open(self.ivmSaiConfigDotYaml,'r')
            scData = sc.read()
            sc.close()
            newScData = scData.replace(self.reffConfigDotYamlName, self.CustomConfigDotYamlName)
            sc = open(self.ivmSaiConfigDotYaml,'w')
            sc.write(newScData)
            sc.close()

    def updateBreakoutMenuOptions (self, port):
        ibKey = self.portInnoBlockMap[port]
        ib =  self.innoBlockInfo[ibKey]
        invalidIds = set()
        excessLp = ib['maxLp'] - ib ['minLp']
        for breakoutId in  ib['breakoutIdMenu']:
            if (excessLp + 1) < self.breakoutCountIdMap[breakoutId - 1]:
                invalidIds.add(breakoutId)
        ib['breakoutIdMenu'].difference_update(invalidIds)

    def selectPredefinedSku(self):
        preDefNames = list(self.preDefinedSkus.keys())
        if os.path.exists(self.CustomSkuDir) == True:
            preDefNames.append("custom")
        preDefNames.append("none")

        print (*preDefNames, sep="\n") 
        i = input("Select a port break out option:")
        if i in preDefNames: 
            if i == 'custom':
                return
            if i != 'none':
                self.applySku(skuName = self.preDefinedSkus[i])
                print("Success!\n\nReboot this switch (sudo reboot) for this configuration to be effective")
            sys.exit()
        else:
            print("--------{} is not a valid option ------".format(i)) 
            self.selectPredefinedSku()


def main():
    if os.geteuid() != 0:
        print("Permission denied! Run this script in root or sudo mode")
        sys.exit()

    os.system('clear')
    print("\n########################################################################") 
    print("## SONIC Port breakout utility, T7132 , Version 18. Supermicro computers Inc.##") 
    print("########################################################################\n") 
    pbo = Pbo()
   
    print("\t\tStarting new port breakout session...\n")
    print("Some of the running configurations could be lost.")
    if os.path.exists(pbo.configDbDotJson) is True:
        print("\nStartup configuration file {} will be DELETED.".format(pbo.configDbDotJson))
    print("Type 'no' if you want to take backup otherwise proceed typing 'yes'")
    pbo.confirmExit()

    pbo.selectPredefinedSku()

    pbo.cmdHelp()
    m = pbo.menu()
    while( m != "quit"):
        if m == "write":
            if (pbo.confPboCount <= pbo.maxPboLimit) :
                #pbo.createSkuDir()
                    
                print("\nWriting to SKU files..")
                
                pbo.writeIni() 
                pbo.writeYaml()
                pbo.updateIvmSaiConfigYaml()
                pbo.writeBufferJ2()
                pbo.applySku( skuName = "t7132CustomSKU")
                print("Breakout configuration Success!")
                print("Quit this session(quit) and  Reboot (sudo reboot) for this configuration to be effective")
            else:
                print("Too many breakouts configured. Limit is {},but configured is {}!\nGenerate Stopped!".format(pbo.maxPboLimit, pbo.confPboCount))
        #elif m == "reset":
        #    pbo.getDefaultIntfList()
        elif m == "config":
            pbo.initTables()
            pbo.config()
        elif m == "show":
            #dumpInfList() 
            pbo.dumpFpiList() 
        m = pbo.menu()


if __name__ == "__main__":
    main()

