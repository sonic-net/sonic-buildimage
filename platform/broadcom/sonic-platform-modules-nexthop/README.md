# Run Python tests (test also run as part of dpkg-buildpackage)
```
python -m pytest
```

# Nexthop tools installed on all SKUs

## idprom

```
root@sonic:~# idprom
Usage: idprom [OPTIONS] COMMAND [ARGS]...

Options:
  --help  Show this message and exit.

Commands:
  clear
  decode
  decode-all
  list
  program
```

### List system idproms
```
root@sonic:~# idprom list
Use one of the following:
/sys/bus/i2c/devices/11-0050/eeprom
```

### Decode idprom
```
root@sonic:~# idprom decode /sys/bus/i2c/devices/11-0050/eeprom
TlvInfo Header:
   Id String:    TlvInfo
   Version:      1
   Total Length: 98
TLV Name             Code Len Value
-------------------- ---- --- -----
Product Name         0x21   7 NH-4010
Part Number          0x22   3 ???
Serial Number        0x23   3 ???
Base MAC Address     0x24   6 00:E1:4C:68:00:C6
Device Version       0x26   1 0
Label Revision       0x27   2 P0
Platform Name        0x28  22 x86_64-nexthop_4010-r0
Manufacturer         0x2B   7 Nexthop
Vendor Name          0x2D   7 Nexthop
Service Tag          0x2F  14 www.nexthop.ai
CRC-32               0xFE   4 0x0FA2C151
```

### Replace select eeprom fields
```
root@sonic:~# idprom program /sys/bus/i2c/devices/11-0050/eeprom --product-name Lotus --part-num 123
TlvInfo Header:
   Id String:    TlvInfo
   Version:      1
   Total Length: 96
TLV Name             Code Len Value
-------------------- ---- --- -----
Product Name         0x21   5 Lotus
Part Number          0x22   3 123
Serial Number        0x23   3 ???
Base MAC Address     0x24   6 00:E1:4C:68:00:C6
Device Version       0x26   1 0
Label Revision       0x27   2 P0
Platform Name        0x28  22 x86_64-nexthop_4010-r0
Manufacturer         0x2B   7 Nexthop
Vendor Name          0x2D   7 Nexthop
Service Tag          0x2F  14 www.nexthop.ai
CRC-32               0xFE   4 0x984CDEB1
```

### Clear eeprom
```
root@sonic:~# idprom clear /sys/bus/i2c/devices/11-0050/eeprom
```

### Program any subset of fields (can set a single field)
```
root@sonic:~# idprom program /sys/bus/i2c/devices/11-0050/eeprom --product-name NH-4010
TlvInfo Header:
   Id String:    TlvInfo
   Version:      1
   Total Length: 15
TLV Name             Code Len Value
-------------------- ---- --- -----
Product Name         0x21   7 NH-4010
CRC-32               0xFE   4 0x5156AA43
```

### Programming Komodo
```
idprom program /sys/bus/i2c/devices/11-0050/eeprom \
    --product-name NH-4010 \
    --part-num ??? \
    --serial-num ??? \
    --mac 00:E1:4C:68:00:C6 \
    --device-version 0 \
    --label-revision P0 \
    --platform-name x86_64-nexthop_4010-r0 \
    --manufacturer-name=Nexthop \
    --vendor-name=Nexthop \
    --service-tag=www.nexthop.ai
TlvInfo Header:
   Id String:    TlvInfo
   Version:      1
   Total Length: 98
TLV Name             Code Len Value
-------------------- ---- --- -----
Product Name         0x21   7 NH-4010
Part Number          0x22   3 ???
Serial Number        0x23   3 ???
Base MAC Address     0x24   6 00:E1:4C:68:00:C6
Device Version       0x26   1 0
Label Revision       0x27   2 P0
Platform Name        0x28  22 x86_64-nexthop_4010-r0
Manufacturer         0x2B   7 Nexthop
Vendor Name          0x2D   7 Nexthop
Service Tag          0x2F  14 www.nexthop.ai
CRC-32               0xFE   4 0x0FA2C151
```
