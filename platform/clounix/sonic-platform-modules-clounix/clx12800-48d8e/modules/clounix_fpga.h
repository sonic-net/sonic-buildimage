/*******************************************************************************

*  Copyright©[2020-2024] [Hangzhou Clounix Technology Limited]. 

*  All rights reserved.



*  This program is free software: you can redistribute it and/or modify

*  it under the terms of the GNU General Public License as published by

*  the Free Software Foundation, either version 3 of the License, or

*  (at your option) any later version.



*  This program is distributed in the hope that it will be useful,

*  but WITHOUT ANY WARRANTY; without even the implied warranty of

*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the

*  GNU General Public License for more details.



*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 

*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 

*  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 

*  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 

*  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 

*  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 

*  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 

*  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 

*  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 

*  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 

*  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/
#ifndef _CLOUNIX_FPGA_H_
#define _CLOUNIX_FPGA_H_

#define FPGA_VENDOR_ID 0x10ee
#define FPGA_DEVICE_ID 0x7021

/*power cycle*/
#define FPGA_GLOBAL_CFG_BASE 0x100
#define FPGA_RESET_CFG_BASE  (FPGA_GLOBAL_CFG_BASE+0) 
#define P12V_STBY_EN   1
#define RESET_MUX_BIT  4

/*PVT mgr*/
#define FPGA_PVT_BASE  0x900
#define FPGA_PVT_MGR_CFG (FPGA_PVT_BASE +0x0)
#define FPGA_PVT_MGR_DATA (FPGA_PVT_BASE +0x4)
#define FPGA_PVT_MGR_CFG_RST_BIT 31
#define FPGA_PVT_MGR_CFG_EN_BIT  30
#define FPGA_PVT_MGR_DATA_MASK 0x00000FFF

#define GET_BIT(data, bit, value)   value = (data >> bit) & 0x1
#define SET_BIT(data, bit)          data |= (1 << bit)
#define CLEAR_BIT(data, bit)        data &= ~(1 << bit)
/*CPLD init register*/
#define CPLD_BASE_ADDRESS           (0x0300)
#define CPLD_INTF_CONFIG            CPLD_BASE_ADDRESS 

#define CPLD0_RST_BIT      31
#define CPLD0_EN_BIT       30
#define CPLD1_RST_BIT      29
#define CPLD1_EN_BIT       28

extern void __iomem * fpga_ctl_addr;
#endif