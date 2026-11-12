try:
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

use_smbus = False

# If smbus is present, use smbus to read/write i2c data, else use i2cutils commands    
if use_smbus:
    def i2c_read_byte_data(i2c_bus, i2c_addr, reg):
        return smbus.SMBus(i2c_bus).read_byte_data(i2c_addr, reg)
    def i2c_write_byte_data(i2c_bus, i2c_addr, reg, data):
        return smbus.SMBus(i2c_bus).write_byte_data(i2c_addr, reg, data)
else:
    def i2c_read_byte_data(i2c_bus, i2c_addr, reg):
        cmd = ["i2cget", "-y", str(i2c_bus), hex(i2c_addr), hex(reg)]
        cmdstatus, status = getstatusoutput_noshell(cmd)
        if cmdstatus != 0:
            return cmdstatus
        status = int(status, 16)
        return status
    def i2c_write_byte_data(i2c_bus, i2c_addr, reg, data):
        cmd = ["i2cset", "-y", str(i2c_bus), hex(i2c_addr), hex(reg), hex(data)]
        cmdstatus, status = getstatusoutput_noshell(cmd)
        if cmdstatus != 0:
            return cmdstatus
        return status
    

class I2CRegisterBitField:
    def __init__(self, i2c_bus, i2c_addr, reg, mask, states):
        self.i2c_bus = i2c_bus
        self.i2c_addr = i2c_addr
        self.reg = reg
        self.mask = mask
        self.states = states
        self.reversed_states = {v: k for k, v in states.items()}
        
    def set_value(self, value):
        # Read the current register value to preserve the other bits
        reg_val = i2c_read_byte_data(self.i2c_bus, self.i2c_addr, self.reg)
        
        # Make sure the value is masked
        value &= self.mask
        
        # Clear the bits
        reg_val &= ~self.mask
        
        # Set the bits to the value
        reg_val |= value
        
        # Write the new value to the register
        i2c_write_byte_data(self.i2c_bus, self.i2c_addr, self.reg, reg_val)
    
    def get_value(self):
        # Read the current register value
        reg_val = i2c_read_byte_data(self.i2c_bus, self.i2c_addr, self.reg)
        
        # Mask the value
        reg_val &= self.mask
        
        return reg_val
    
    def set_to_state(self, state):        
        # Get the target state, it has to be a valid state passed by the parameter
        try :
            target_state_value = self.states[state]
        except KeyError:
            raise ValueError(f"Invalid state {state}")
        
        self.set_value(target_state_value)
        
    def get_state(self):
        value = self.get_value()
        
        try:
            state = self.reversed_states[value]
        except KeyError:
            state = "INVALID"
        
        return state
        


class I2CLed(object):
    def __init__(self, i2c_register_bit_field):
        self.register_bit_field = i2c_register_bit_field
    
    def set_state(self, state):
        self.register_bit_field.set_to_state(state)
        
    def get_state(self):
        return self.register_bit_field.get_state()