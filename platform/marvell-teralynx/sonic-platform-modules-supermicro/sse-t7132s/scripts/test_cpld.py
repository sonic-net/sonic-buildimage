import pytest

CPLD_INFO_PATH='/sys/devices/platform/switchboard/CPLD{}'

@pytest.mark.parametrize(
                    "cpld, offset, default, valid_mask, writeable_mask, test_mask",
    [
        pytest.param(   1,   0x00,    0x00,       0x00,           0x00,       0x00, id="CPLD1 0x00 Reserved"),
        pytest.param(   1,   0x01,    0x00,       0x1f,           0x00,       0x00, id="CPLD1 0x01 Switch model ID"),
        pytest.param(   1,   0x02,    0x10,       0xff,           0x00,       0x00, id="CPLD1 0x02 HW/CPLD version"),
        pytest.param(   1,   0x03,    0xff,       0xff,           0x00,       0x00, id="CPLD1 0x03 Power sequence module status"),
        pytest.param(   1,   0x04,    0x00,       0xff,           0x00,       0x00, id="CPLD1 0x04 Voltage Regulator Module ALERT/ Thermal"),
        pytest.param(   1,   0x05,    0x7f,       0xff,           0x98,       0x18, id="CPLD1 0x05 Enable/ Reset misc. devices"),
        pytest.param(   1,   0x06,    0xfe,       0xff,           0xff,       0x01, id="CPLD1 0x06 Enable/ Reset misc. devices"),
        pytest.param(   1,   0x07,    0x0a,       0x0f,           0x0a,       0x0a, id="CPLD1 0x07 FACTORY_BTN event log and clear"),
        pytest.param(   1,   0x09,    0xe0,       0xe0,           0xe0,       0xe0, id="CPLD1 0x09 System reset records"),
        pytest.param(   1,   0x0B,    0xff,       0xff,           0x3f,       0x00, id="CPLD1 0x0B PCA9548 I2C bus switch RSTn"),
        pytest.param(   1,   0x0C,    0x07,       0x07,           0x07,       0x07, id="CPLD1 0x0C Transceiver Power Enable"),
        pytest.param(   1,   0x0D,    0xff,       0xff,           0xff,       0xff, id="CPLD1 0x0D Transceiver Power Enable"),
        pytest.param(   1,   0x0E,    0x07,       0x07,           0x00,       0x00, id="CPLD1 0x0E Transceiver Power Good"),
        pytest.param(   1,   0x0F,    0xff,       0xff,           0x00,       0x00, id="CPLD1 0x0F Transceiver Power Good"),
        pytest.param(   1,   0x10,    0xff,       0xff,           0xff,       0xff, id="CPLD1 0x10 QSFP-DD Reset signals for Port QT16 to QT9"),
        pytest.param(   1,   0x11,    0xff,       0xff,           0xff,       0xff, id="CPLD1 0x11 QSFP-DD Reset signals for Port QT8 to QT1"),
        pytest.param(   1,   0x12,    0x00,       0xff,           0xff,       0xff, id="CPLD1 0x12 QSFP-DD LPMODE signals for Port QT16 to QT9"),
        pytest.param(   1,   0x13,    0x00,       0xff,           0xff,       0xff, id="CPLD1 0x13 QSFP-DD LPMODE signals for Port QT8 to QT1"),
        pytest.param(   1,   0x14,    0x00,       0x00,           0x00,       0x00, id="CPLD1 0x14 QSFP-DD INT signals from Port QT16 to QT9"),
        pytest.param(   1,   0x15,    0x00,       0x00,           0x00,       0x00, id="CPLD1 0x15 QSFP-DD INT signals from Port QT8 to QT1"),
        pytest.param(   1,   0x16,    0x00,       0x00,           0x00,       0x00, id="CPLD1 0x16 QSFP-DD MODPRS signals from Port QT16 to QT9"),
        pytest.param(   1,   0x17,    0x00,       0x00,           0x00,       0x00, id="CPLD1 0x17 QSFP-DD MODPRS signals from Port QT8 to QT1"),
        pytest.param(   1,   0x18,    0xff,       0xff,           0xff,       0xff, id="CPLD1 0x18 QSFP-DD Reset signals for Port QB16 to QB9"),
        pytest.param(   1,   0x19,    0xff,       0xff,           0xff,       0xff, id="CPLD1 0x19 QSFP-DD Reset signals for Port QB8 to QB1"),
        pytest.param(   1,   0x1A,    0x00,       0x00,           0xff,       0xff, id="CPLD1 0x1A QSFP-DD LPMODE signals for Port QB16 to QB9"),
        pytest.param(   1,   0x1B,    0x00,       0x00,           0xff,       0xff, id="CPLD1 0x1B QSFP-DD LPMODE signals for Port QB8 to QB1"),
        pytest.param(   1,   0x1C,    0x00,       0x00,           0x00,       0x00, id="CPLD1 0x1C QSFP-DD INT signals from Port QB16 to QB9"),
        pytest.param(   1,   0x1D,    0x00,       0x00,           0x00,       0x00, id="CPLD1 0x1D QSFP-DD INT signals from Port QB8 to QB1"),
        pytest.param(   1,   0x1E,    0x00,       0x00,           0x00,       0x00, id="CPLD1 0x1E QSFP-DD MODPRS signals from Port QB16 to QB9"),
        pytest.param(   1,   0x1F,    0x00,       0x00,           0x00,       0x00, id="CPLD1 0x1F QSFP-DD MODPRS signals from Port QB8 to QB1"),
        pytest.param(   1,   0x20,    0x06,       0x00,           0x07,       0x07, id="CPLD1 0x20 Top side SFPP"),
        pytest.param(   1,   0x21,    0x06,       0x00,           0x07,       0x07, id="CPLD1 0x21 Bottom side SFPP"),
        pytest.param(   1,   0x22,    0x00,       0x00,           0xff,       0xff, id="CPLD1 0x22 Watch Dog Maximum count setting, Least significant 8 bits"),
        pytest.param(   1,   0x23,    0x00,       0x00,           0xff,       0xff, id="CPLD1 0x23 Watch Dog Maximum count setting, Most significant 8 bits"),
        pytest.param(   1,   0x24,    0x00,       0x00,           0x00,       0x00, id="CPLD1 0x24 Watch Dog Current count value, Least significant 8 bits"),
        pytest.param(   1,   0x25,    0x00,       0x00,           0x00,       0x00, id="CPLD1 0x25 Watch Dog Current count value, Most significant 8 bits"),
        pytest.param(   1,   0xF0,    0x00,       0xff,           0x00,       0x00, id="CPLD1 0xF0 Version as BMC I2C Registers #0"),
        pytest.param(   1,   0xF1,    0x48,       0xff,           0x00,       0x00, id="CPLD1 0xF1 Version as BMC I2C Registers #1"),
        pytest.param(   1,   0xF2,    0x00,       0xff,           0x00,       0x00, id="CPLD1 0xF2 Version as BMC I2C Registers #2"),
        pytest.param(   1,   0xF3,    0x02,       0xff,           0x00,       0x00, id="CPLD1 0xF3 Version as BMC I2C Registers #3"),
        pytest.param(   1,   0xFE,    0x10,       0xff,           0x00,       0x00, id="CPLD1 0xFE CPLD JED Released Date Month"),
        pytest.param(   1,   0xFF,    0x26,       0xff,           0x00,       0x00, id="CPLD1 0xFF CPLD JED Released Date Day"),
        pytest.param(   2,   0x00,    0x00,       0x00,           0x00,       0x00, id="CPLD2 0x00 Reserved"),
        pytest.param(   2,   0x01,    0x00,       0x00,           0x00,       0x00, id="CPLD2 0x01 Reserved"),
        pytest.param(   2,   0x02,    0x00,       0x0f,           0x00,       0x00, id="CPLD2 0x02 CPLD2 FW version"),
        pytest.param(   2,   0x03,    0x2c,       0x7f,           0x37,       0x33, id="CPLD2 0x03 System Ready/Reset Status and LED control"),
        pytest.param(   2,   0x04,    0x00,       0xff,           0x1f,       0x1f, id="CPLD2 0x04 All transceiver LEDs control"),
        pytest.param(   2,   0x05,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x05 SFPP LED manual control TSFPP"),
        pytest.param(   2,   0x06,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x06 SFPP LED manual control BSFPP"),
        pytest.param(   2,   0x07,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x07 QSFP-DD LED manual control QT1_Pn_G"),
        pytest.param(   2,   0x08,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x08 QSFP-DD LED manual control QT1_Pn_Y"),
        pytest.param(   2,   0x09,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x09 QSFP-DD LED manual control QB1_P1_G"),
        pytest.param(   2,   0x0A,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x0A QSFP-DD LED manual control QB1_P1_Y"),
        pytest.param(   2,   0x0B,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x0B QT2 Green 4 LEDs"),
        pytest.param(   2,   0x0C,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x0C QT2 Yellow 4 LEDs"),
        pytest.param(   2,   0x0D,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x0D QB2 Green 4 LEDs"),
        pytest.param(   2,   0x0E,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x0E QB2Yellow 4 LEDs"),
        pytest.param(   2,   0x0F,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x0F QT3 Green 4 LEDs"),
        pytest.param(   2,   0x10,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x10 QT3 Yellow 4 LEDs"),
        pytest.param(   2,   0x11,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x11 QB3 Green 4 LEDs"),
        pytest.param(   2,   0x12,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x12 QB3Yellow 4 LEDs"),
        pytest.param(   2,   0x13,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x13 QT4 Green 4 LEDs"),
        pytest.param(   2,   0x14,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x14 QT4 Yellow 4 LEDs"),
        pytest.param(   2,   0x15,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x15 QB4 Green 4 LEDs"),
        pytest.param(   2,   0x16,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x16 QB4Yellow 4 LEDs"),
        pytest.param(   2,   0x17,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x17 QT5 Green 4 LEDs"),
        pytest.param(   2,   0x18,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x18 QT5 Yellow 4 LEDs"),
        pytest.param(   2,   0x19,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x19 QB5 Green 4 LEDs"),
        pytest.param(   2,   0x1A,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x1A QB5Yellow 4 LEDs"),
        pytest.param(   2,   0x1B,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x1B QT6 Green 4 LEDs"),
        pytest.param(   2,   0x1C,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x1C QT6 Yellow 4 LEDs"),
        pytest.param(   2,   0x1D,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x1D QB6 Green 4 LEDs"),
        pytest.param(   2,   0x1E,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x1E QB6 Yellow 4 LEDs"),
        pytest.param(   2,   0x1F,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x1F QT7 Green 4 LEDs"),
        pytest.param(   2,   0x20,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x20 QT7 Yellow 4 LEDs"),
        pytest.param(   2,   0x21,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x21 QB7 Green 4 LEDs"),
        pytest.param(   2,   0x22,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x22 QB7 Yellow 4 LEDs"),
        pytest.param(   2,   0x23,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x23 QT8 Green 4 LEDs"),
        pytest.param(   2,   0x24,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x24 QT8 Yellow 4 LEDs"),
        pytest.param(   2,   0x25,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x25 QB8 Green 4 LEDs"),
        pytest.param(   2,   0x26,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x26 QB8 Yellow 4 LEDs"),
        pytest.param(   2,   0x27,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x27 QT9 Green 4 LEDs"),
        pytest.param(   2,   0x28,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x28 QT9 Yellow 4 LEDs"),
        pytest.param(   2,   0x29,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x29 QB9 Green 4 LEDs"),
        pytest.param(   2,   0x2A,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x2A QB9 Yellow 4 LEDs"),
        pytest.param(   2,   0x2B,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x2B QT10 Green 4 LEDs"),
        pytest.param(   2,   0x2C,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x2C QT10 Yellow 4 LEDs"),
        pytest.param(   2,   0x2D,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x2D QB10 Green 4 LEDs"),
        pytest.param(   2,   0x2E,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x2E QB10 Yellow 4 LEDs"),
        pytest.param(   2,   0x2F,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x2F QT11 Green 4 LEDs"),
        pytest.param(   2,   0x30,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x30 QT11 Yellow 4 LEDs"),
        pytest.param(   2,   0x31,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x31 QB11 Green 4 LEDs"),
        pytest.param(   2,   0x32,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x32 QB11 Yellow 4 LEDs"),
        pytest.param(   2,   0x33,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x33 QT12 Green 4 LEDs"),
        pytest.param(   2,   0x34,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x34 QT12 Yellow 4 LEDs"),
        pytest.param(   2,   0x35,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x35 QB12 Green 4 LEDs"),
        pytest.param(   2,   0x36,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x36 QB12 Yellow 4 LEDs"),
        pytest.param(   2,   0x37,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x37 QT13 Green 4 LEDs"),
        pytest.param(   2,   0x38,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x38 QT13 Yellow 4 LEDs"),
        pytest.param(   2,   0x39,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x39 QB13 Green 4 LEDs"),
        pytest.param(   2,   0x3A,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x3A QB13 Yellow 4 LEDs"),
        pytest.param(   2,   0x3B,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x3B QT14 Green 4 LEDs"),
        pytest.param(   2,   0x3C,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x3C QT14 Yellow 4 LEDs"),
        pytest.param(   2,   0x3D,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x3D QB14 Green 4 LEDs"),
        pytest.param(   2,   0x3E,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x3E QB14 Yellow 4 LEDs"),
        pytest.param(   2,   0x3F,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x3F QT15 Green 4 LEDs"),
        pytest.param(   2,   0x40,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x40 QT15 Yellow 4 LEDs"),
        pytest.param(   2,   0x41,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x41 QB15 Green 4 LEDs"),
        pytest.param(   2,   0x42,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x42 QB15 Yellow 4 LEDs"),
        pytest.param(   2,   0x43,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x43 QT16 Green 4 LEDs"),
        pytest.param(   2,   0x44,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x44 QT16 Yellow 4 LEDs"),
        pytest.param(   2,   0x45,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x45 QB16 Green 4 LEDs"),
        pytest.param(   2,   0x46,    0x00,       0xff,           0xff,       0xff, id="CPLD2 0x46 QB16 Yellow 4 LEDs"),
        pytest.param(   2,   0xF0,    0x00,       0xff,           0x00,       0x00, id="CPLD2 0xF0 Version as BMC I2C Registers #0"),
        pytest.param(   2,   0xF1,    0x48,       0xff,           0x00,       0x00, id="CPLD2 0xF1 Version as BMC I2C Registers #1"),
        pytest.param(   2,   0xF2,    0x20,       0xff,           0x00,       0x00, id="CPLD2 0xF2 Version as BMC I2C Registers #2"),
        pytest.param(   2,   0xF3,    0x02,       0xff,           0x00,       0x00, id="CPLD2 0xF3 Version as BMC I2C Registers #3"),
        pytest.param(   2,   0xFE,    0x10,       0xff,           0x00,       0x00, id="CPLD2 0xFE CPLD JED Released Date Month"),
        pytest.param(   2,   0xFF,    0x27,       0xff,           0x00,       0x00, id="CPLD2 0xFF CPLD JED Released Date Day"),
    ],
)
class TestClass:
    def test_cpld_read_default(self, cpld, offset, default, valid_mask, writeable_mask, test_mask):
        reg_offset = "/".join([CPLD_INFO_PATH.format(cpld), "testee_offset"])
        reg_value =  "/".join([CPLD_INFO_PATH.format(cpld), "testee_value"])

        # write offset
        with open(reg_offset, "r+") as file_offset:
            file_offset.write(hex(offset))

        for i in range(2):
            with open(reg_value, "r+") as file_value:
                content = file_value.readline().strip()
                content_value = int(content, 16)
                assert hex(content_value & valid_mask) == hex(default & valid_mask)

    def test_cpld_read_stable(self, cpld, offset, default, valid_mask, writeable_mask, test_mask):
        reg_offset = "/".join([CPLD_INFO_PATH.format(cpld), "testee_offset"])
        reg_value =  "/".join([CPLD_INFO_PATH.format(cpld), "testee_value"])

        # write offset
        with open(reg_offset, "r+") as file_offset:
            file_offset.write(hex(offset))

        # read current value
        with open(reg_value, "r+") as file_value:
            content = file_value.readline().strip()
            last_value = int(content, 16)

        for i in range(100):
            with open(reg_value, "r+") as file_value:
                content = file_value.readline().strip()
                content_value = int(content, 16)
                assert hex(content_value) == hex(last_value)

    def test_cpld_write_stable(self, cpld, offset, default, valid_mask, writeable_mask, test_mask):
        reg_offset = "/".join([CPLD_INFO_PATH.format(cpld), "testee_offset"])
        reg_value =  "/".join([CPLD_INFO_PATH.format(cpld), "testee_value"])

        # write offset
        with open(reg_offset, "r+") as file_offset:
            file_offset.write(hex(offset))

        # read and save current value
        with open(reg_value, "r+") as file_value:
            content = file_value.readline().strip()
            last_value = int(content, 16)

        for i in range(100):
            for j in range(8):
                if (1<<j) & test_mask == 0:
                    continue

                # test bit value=0
                write_value = last_value & ~(1<<j) & 0xff
                # write value
                with open(reg_value, "r+") as file_value:
                    file_value.seek(0)
                    file_value.write(hex(write_value))
                # read value
                with open(reg_value, "r+") as file_value:
                    content = file_value.readline().strip()
                    read_value = int(content, 16)
                    assert hex(read_value & valid_mask) == hex(write_value & valid_mask)

                # test bit value=1
                write_value = last_value | (1<<j)
                # write value
                with open(reg_value, "r+") as file_value:
                    file_value.seek(0)
                    file_value.write(hex(write_value))
                # read value
                with open(reg_value, "r+") as file_value:
                    content = file_value.readline().strip()
                    read_value = int(content, 16)
                    assert hex(read_value & valid_mask) == hex(write_value & valid_mask)

        # resotre save current value
        with open(reg_value, "r+") as file_value:
            file_value.seek(0)
            file_value.write(hex(last_value))

