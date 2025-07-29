//======================================================================
// Driver for the Dapu RTC module INS5699 SA
//
// Copyright(C) DAPU Telecom CORPORATION 2021. All rights reserved.
//
// Derived from INS5699 driver:
// Copyright (C) 2020 Dptel
//
//
// Modified by wangzn at https://www.dptel.com/
// 20220921 V1.4 update the ins5699_init_client function and ins5699_recover function
// Modified by zengjl at https://www.dptel.com/
// 20221107 V1.5 update i2c_driver struct, Assign values to of_match_table of i2c_driver
// 20221107 V1.5 update insxxx_recover function
//
// This driver software is distributed as is, without any warranty of any kind,
// either express or implied as further specified in the GNU Public License. This
// software may be used and distributed according to the terms of the GNU Public
// License, version 2 as published by the Free Software Foundation.
// See the file COPYING in the main directory of this archive for more details.
//
// You should have received a copy of the GNU General Public License along with
// this program. If not, see <http://www.gnu.org/licenses/>.
//======================================================================

#if 0
#define DEBUG
#include <linux/device.h>
#undef DEBUG
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/bcd.h>
#include <linux/i2c.h>
#include <linux/list.h>
#include <linux/rtc.h>
#include <linux/of_gpio.h>

#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/delay.h>

#define mem_clear(data, size) memset((data), 0, (size))

//#define  CHARGEBAT

// INS5699 Basic Time and Calendar Register definitions
#define INS5699_BTC_SEC					0x00
#define INS5699_BTC_MIN					0x01
#define INS5699_BTC_HOUR					0x02
#define INS5699_BTC_WEEK					0x03
#define INS5699_BTC_DAY					0x04
#define INS5699_BTC_MONTH				0x05
#define INS5699_BTC_YEAR					0x06
#define INS5699_BTC_RAM					0x07
#define INS5699_BTC_ALARM_MIN			0x08
#define INS5699_BTC_ALARM_HOUR			0x09
#define INS5699_BTC_ALARM_WEEK_OR_DAY	0x0A
#define INS5699_BTC_TIMER_CNT_0			0x0B
#define INS5699_BTC_TIMER_CNT_1			0x0C
#define INS5699_BTC_EXT					0x0D
#define INS5699_BTC_FLAG					0x0E
#define INS5699_BTC_CTRL					0x0F

// INS5699 Extension Register 1 definitions
#define INS5699_EXT_SEC					0x10
#define INS5699_EXT_MIN					0x11
#define INS5699_EXT_HOUR					0x12
#define INS5699_EXT_WEEK					0x13
#define INS5699_EXT_DAY					0x14
#define INS5699_EXT_MONTH				0x15
#define INS5699_EXT_YEAR					0x16
#define INS5699_EXT_TEMP					0x17
#define INS5699_EXT_BACKUP				0x18

#define INS5699_EXT_TIMER_CNT_0			0x1B
#define INS5699_EXT_TIMER_CNT_1			0x1C
#define INS5699_EXT_EXT					0x1D
#define INS5699_EXT_FLAG					0x1E
#define INS5699_EXT_CTRL					0x1F


// Flag INS5699_BTC_EXT Register bit positions
#define INS5699_BTC_EXT_TSEL0		(1 << 0)
#define INS5699_BTC_EXT_TSEL1		(1 << 1)
#define INS5699_BTC_EXT_FSEL0		(1 << 2)
#define INS5699_BTC_EXT_FSEL1		(1 << 3)
#define INS5699_BTC_EXT_TE 			(1 << 4)
#define INS5699_BTC_EXT_USEL			(1 << 5)
#define INS5699_BTC_EXT_WADA			(1 << 6)
#define INS5699_BTC_EXT_TEST			(1 << 7)

// Flag INS5699_BTC_FLAG Register bit positions
#define INS5699_BTC_FLAG_VDET 		(1 << 0)
#define INS5699_BTC_FLAG_VLF 		(1 << 1)

#define INS5699_BTC_FLAG_AF 			(1 << 3)
#define INS5699_BTC_FLAG_TF 			(1 << 4)
#define INS5699_BTC_FLAG_UF 			(1 << 5)

// Flag INS5699_BTC_CTRL Register bit positions
#define INS5699_BTC_CTRL_RESET 		(1 << 0)


#define INS5699_BTC_CTRL_AIE 		(1 << 3)
#define INS5699_BTC_CTRL_TIE 		(1 << 4)
#define INS5699_BTC_CTRL_UIE 		(1 << 5)
#define INS5699_BTC_CTRL_CSEL0 		(1 << 6)
#define INS5699_BTC_CTRL_CSEL1		(1 << 7)

static const struct i2c_device_id ins5699_id[] = {
	{ "ins5699", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ins5699_id);

static const struct of_device_id ins5699_of_match[] = {
	{ .compatible = "dptel,rtc-ins5699" },
	{ }
};
MODULE_DEVICE_TABLE(of, ins5699_of_match);

struct ins5699_data {
	struct i2c_client *client;
	struct rtc_device *rtc;
	struct work_struct work;
	u8 ctrlreg;
	unsigned exiting:1;
};

typedef struct {
	u8 number;
	u8 value;
}reg_data;

#define SE_RTC_REG_READ		_IOWR('p', 0x20, reg_data)
#define SE_RTC_REG_WRITE	_IOW('p',  0x21, reg_data)

//----------------------------------------------------------------------SE_RTC_REG_READ
// ins5699_read_reg()
// reads a ins5699 register (see Register defines)
// See also ins5699_read_regs() to read multiple registers.
//
//----------------------------------------------------------------------
static int ins5699_read_reg(struct i2c_client *client, u8 number, u8 *value)
{
	int ret = i2c_smbus_read_byte_data(client, number) ;

	//check for error
	if (ret < 0) {
		dev_err(&client->dev, "Unable to read register #%d\n", number);
		return ret;
	}

	*value = (u8)ret;
	return 0;
}

//----------------------------------------------------------------------
// ins5699_read_regs()
// reads a specified number of ins5699 registers (see Register defines)
// See also ins5699_read_reg() to read single register.
//
//----------------------------------------------------------------------
static int ins5699_read_regs(struct i2c_client *client, u8 number, u8 length, u8 *values)
{
	int ret = i2c_smbus_read_i2c_block_data(client, number, length, values);

	//check for length error
	if (ret != length) {
		dev_err(&client->dev, "Unable to read registers #%d..#%d\n", number, number + length - 1);
		return ret < 0 ? ret : -EIO;
	}

	return 0;
}

//----------------------------------------------------------------------
// ins5699_write_reg()
// writes a ins5699 register (see Register defines)
// See also ins5699_write_regs() to write multiple registers.
//
//----------------------------------------------------------------------
static int ins5699_write_reg(struct i2c_client *client, u8 number, u8 value)
{
	int ret = i2c_smbus_write_byte_data(client, number, value);

	//check for error
	if (ret)
		dev_err(&client->dev, "Unable to write register #%d\n", number);


	return ret;
}

//----------------------------------------------------------------------
// ins5699_write_regs()
// writes a specified number of ins5699 registers (see Register defines)
// See also ins5699_write_reg() to write a single register.
//
//----------------------------------------------------------------------
static int ins5699_write_regs(struct i2c_client *client, u8 number, u8 length, u8 *values)
{
	int ret = i2c_smbus_write_i2c_block_data(client, number, length, values);

	//check for error
	if (ret)
		dev_err(&client->dev, "Unable to write registers #%d..#%d\n", number, number + length - 1);

	return ret;
}

//----------------------------------------------------------------------
// ins5699_irq()
// irq handler
//
//----------------------------------------------------------------------
static irqreturn_t ins5699_irq(int irq, void *dev_id)
{
	struct i2c_client *client = dev_id;
	struct ins5699_data *ins5699 = i2c_get_clientdata(client);
	disable_irq_nosync(irq);
	schedule_work(&ins5699->work);

	return IRQ_HANDLED;
}

//----------------------------------------------------------------------
// ins5699_work()
//
//----------------------------------------------------------------------
static void ins5699_work(struct work_struct *work)
{
	struct ins5699_data *ins5699 = container_of(work, struct ins5699_data, work);
	struct i2c_client *client = ins5699->client;
	struct mutex *lock = &ins5699->rtc->ops_lock;
	u8 flags;

	mutex_lock(lock);

	if (ins5699_read_reg(client, INS5699_BTC_FLAG, &flags))
		goto out;

	dev_dbg(&client->dev, "%s REG[%02xh]=>%02xh\n", __func__, INS5699_BTC_FLAG, flags);

	if (flags & INS5699_BTC_FLAG_VLF)
		dev_warn(&client->dev, "Data loss is detected. All registers must be initialized.\n");

	if (flags & INS5699_BTC_FLAG_VDET)
		dev_warn(&client->dev, "Temperature compensation stop detected.\n");

	// fixed-cycle timer
	if (flags & INS5699_BTC_FLAG_TF) {
		flags &= ~INS5699_BTC_FLAG_TF;
		local_irq_disable();
		rtc_update_irq(ins5699->rtc, 1, RTC_PF);// | RTC_IRQF);
		local_irq_enable();
		dev_dbg(&client->dev, "%s: fixed-cycle timer function status: %xh\n", __func__, flags);
	}

	// alarm function
	if (flags & INS5699_BTC_FLAG_AF) {
		flags &= ~INS5699_BTC_FLAG_AF;
		local_irq_disable();
		rtc_update_irq(ins5699->rtc, 1, RTC_AF);// | RTC_IRQF);
		local_irq_enable();
		dev_dbg(&client->dev, "%s: alarm function status: %xh\n", __func__, flags);
	}

	// time update function
	if (flags & INS5699_BTC_FLAG_UF) {
		flags &= ~INS5699_BTC_FLAG_UF;
		local_irq_disable();
		rtc_update_irq(ins5699->rtc, 1, RTC_UF);// | RTC_IRQF);
		local_irq_enable();
		dev_dbg(&client->dev, "%s: time update function status: %xh\n", __func__, flags);
	}

	// acknowledge IRQ
	(void)ins5699_write_reg(client, INS5699_BTC_FLAG, 0x0f & flags);

out:
	if (!ins5699->exiting)
		enable_irq(client->irq);

	mutex_unlock(lock);
}

static int ins5699_get_week_day( u8 reg_week_day )
{
	int i, tm_wday = -1;

	for ( i=0; i < 7; i++ )
	{
		if ( reg_week_day & 1 )
		{
			tm_wday = i;
			break;
		}
		reg_week_day >>= 1;
	}

	return 	tm_wday;
}

//----------------------------------------------------------------------
// ins5699_get_time()
// gets the current time from the ins5699 registers
//
//----------------------------------------------------------------------
static int ins5699_get_time(struct device *dev, struct rtc_time *dt)
{
	struct ins5699_data *ins5699 = dev_get_drvdata(dev);
	u8 date[7];
	int err;

    mem_clear(date, sizeof(date));
	err = ins5699_read_regs(ins5699->client, INS5699_BTC_SEC, 7, date);
	if (err)
		return err;

	dev_dbg(dev, "%s: read 0x%02x 0x%02x "
		"0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", __func__,
		date[0], date[1], date[2], date[3], date[4], date[5], date[6]);

	dt->tm_sec  = bcd2bin(date[INS5699_BTC_SEC] & 0x7f);
	dt->tm_min  = bcd2bin(date[INS5699_BTC_MIN] & 0x7f);
	dt->tm_hour = bcd2bin(date[INS5699_BTC_HOUR] & 0x3f);
	dt->tm_wday = ins5699_get_week_day( date[INS5699_BTC_WEEK] & 0x7f );
	dt->tm_mday = bcd2bin(date[INS5699_BTC_DAY] & 0x3f);
	dt->tm_mon  = bcd2bin(date[INS5699_BTC_MONTH] & 0x1f) - 1;
	dt->tm_year = bcd2bin(date[INS5699_BTC_YEAR]);

	if (dt->tm_year < 70)
		dt->tm_year += 100;

	dev_dbg(dev, "%s: date %ds %dm %dh %dwd %dmd %dm %dy\n", __func__,
		dt->tm_sec, dt->tm_min, dt->tm_hour, dt->tm_wday,
		dt->tm_mday, dt->tm_mon, dt->tm_year);

	return rtc_valid_tm(dt);
}

//----------------------------------------------------------------------
// ins5699_set_time()
// Sets the current time in the ins5699 registers
//
//----------------------------------------------------------------------
static int ins5699_set_time(struct device *dev, struct rtc_time *dt)
{
	struct ins5699_data *ins5699 = dev_get_drvdata(dev);
	u8 date[7];
	int ret = 0;

	date[INS5699_BTC_SEC]   = bin2bcd(dt->tm_sec);
	date[INS5699_BTC_MIN]   = bin2bcd(dt->tm_min);
	date[INS5699_BTC_HOUR]  = bin2bcd(dt->tm_hour);
	date[INS5699_BTC_WEEK]  = 1 << (dt->tm_wday);
	date[INS5699_BTC_DAY]   = bin2bcd(dt->tm_mday);
	date[INS5699_BTC_MONTH] = bin2bcd(dt->tm_mon + 1);
	date[INS5699_BTC_YEAR]  = bin2bcd(dt->tm_year % 100);

	dev_dbg(dev, "%s: write 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
		__func__, date[0], date[1], date[2], date[3], date[4], date[5], date[6]);

	ret =  ins5699_write_regs(ins5699->client, INS5699_BTC_SEC, 7, date);

	return ret;
}

//----------------------------------------------------------------------
// ins5699_recover()
// initializes the ins5699
//
//----------------------------------------------------------------------
static int ins5699_recover(struct i2c_client *client)
{
    int err;
	unsigned char value_init = 0;
	unsigned char value_5c = 0;
	unsigned char value_20reg = 0;
	unsigned char value_5c_temp = 0;
	err = ins5699_write_reg(client, 0x30, 0xd1);
	if (err)
		goto out;
	err = ins5699_write_reg(client, 0x40, 0x00);
	if (err)
		goto out;
	err = ins5699_write_reg(client, 0x32, 0x81);
	if (err)
		goto out;
	mdelay(50);

	err = ins5699_write_reg(client, 0x32, 0x80);
	if (err)
		goto out;
	mdelay(50);
	err = ins5699_write_reg(client, 0x32, 0x04);
	if (err)
		goto out;

	//check rtc vertion
	err = ins5699_read_reg(client, 0x20,&value_20reg);
	if (err)
		goto out;
	if(value_20reg == 0XD1)
	{
		goto out;
	}
	else if(value_20reg == 0XD2)
		;
	else
	{
		err = 1;
		goto out;
	}


	//write 5c
	err = ins5699_read_reg(client, 0x5c,&value_init);
	if (err)
		goto out;
	value_5c = value_init|0X08;
	err = ins5699_write_reg(client, 0x5c, value_5c);
	if (err)
		goto out;
	err = ins5699_read_reg(client, 0x5c,&value_5c_temp);
	if (err)
		goto out;
	if(value_5c_temp!=value_5c)
	{
		err = 1;
		goto out;
	}

	mdelay(50);

	value_5c = value_5c & 0XF7;
	err = ins5699_write_reg(client, 0x5c, value_5c);
	if (err)
		goto out;
	err = ins5699_write_reg(client, 0x5c, value_init);
	if (err)
		goto out;
	err = ins5699_read_reg(client, 0x5c,&value_5c_temp);
	if (err)
		goto out;
	if(value_5c_temp != value_init)
	{
		err = 1;
		goto out;
	}
	err = ins5699_write_reg(client, 0x30, 0x00);
	if (err)
		goto out;
out:
	return err;
}


//----------------------------------------------------------------------
// ins5699_init_client()
// initializes the ins5699
//
//----------------------------------------------------------------------
static int ins5699_init_client(struct i2c_client *client, int *need_reset)
{
	u8 flags;
	int need_clear = 0;
	int err = 0;

	err = ins5699_read_reg(client, INS5699_BTC_FLAG, &flags);
	if (err)
		goto out;
//***turn off 0x32 for current  2022-03-10
	err = ins5699_write_reg(client, 0x30, 0xD1);
	if (err)
		goto out;

	err = ins5699_write_reg(client, 0x32, 0x04);
	if (err)
		goto out;

	err = ins5699_write_reg(client, 0x30, 0x00);
	if (err)
		goto out;

//***

	if ( flags & INS5699_BTC_FLAG_VDET )
	{
		dev_warn(&client->dev, "Temperature compensation is stop detected.\n");
		need_clear = 1;
	}

	if ( flags & INS5699_BTC_FLAG_VLF )
	{
		dev_warn(&client->dev, "Data loss is detected. All registers must be initialized.\n");
		*need_reset = 1;
		need_clear = 1;
	}

	if ( flags & INS5699_BTC_FLAG_AF ){
		dev_warn(&client->dev, "Alarm was detected\n");
		need_clear = 1;
	}

	if ( flags & INS5699_BTC_FLAG_TF ){
		dev_warn(&client->dev, "Timer was detected\n");
		need_clear = 1;
	}

	if ( flags & INS5699_BTC_FLAG_UF ){
		dev_warn(&client->dev, "Update was detected\n");
		need_clear = 1;
	}


	if (*need_reset ) {
		//clear ctrl register
		err = ins5699_write_reg(client, INS5699_BTC_CTRL, INS5699_BTC_CTRL_CSEL0);
		if (err)
			goto out;

		//set second update
		err = ins5699_write_reg(client, INS5699_BTC_EXT, 0x00);
		if (err)
			goto out;

	}

	if (need_clear) {
		//clear flag register
		err = ins5699_write_reg(client, INS5699_BTC_FLAG, 0x00);
		if (err)
			goto out;

	}

	/* Solution for drop of vdd&vbat voltage.*/
	err = ins5699_recover(client);
	if (err)
		goto out;

#ifdef  CHARGEBAT
		err = ins5699_write_reg(client, 0x21, 0x81);
		if (err)
			goto out;
#endif


out:
	return err;

}

//----------------------------------------------------------------------
// ins5699_get_alarm()
// reads current Alarm
//
//----------------------------------------------------------------------
static int ins5699_get_alarm(struct device *dev, struct rtc_wkalrm *t)
{
	struct ins5699_data *ins5699 = dev_get_drvdata(dev);
	struct i2c_client *client = ins5699->client;
	u8 alarmvals[3];		//minute, hour, week/day values
	u8 ctrl[3];				//extension, flag
	int err;

	if (client->irq <= 0)
		return -EINVAL;

	//get current minute, hour, week/day alarm values
	mem_clear(alarmvals, sizeof(alarmvals));
	err = ins5699_read_regs(client, INS5699_BTC_ALARM_MIN, 3, alarmvals);
	if (err)
		return err;

	//get current extension, flag, control values
	mem_clear(ctrl, sizeof(ctrl));
	err = ins5699_read_regs(client, INS5699_BTC_EXT, 3, ctrl);
	if (err)
		return err;

	dev_dbg(dev, "%s: minutes:0x%02x hours:0x%02x week/day:0x%02x\n",
		__func__, alarmvals[0], alarmvals[1], alarmvals[2]);

	// Hardware alarm precision is 1 minute
	t->time.tm_sec  = 0;
	t->time.tm_min  = bcd2bin(alarmvals[0] & 0x7f);
	t->time.tm_hour = bcd2bin(alarmvals[1] & 0x3f);

	if ( ctrl[0] & INS5699_BTC_EXT_WADA )
	{   // Day Alarm
		t->time.tm_wday = -1;
		t->time.tm_mday = bcd2bin(alarmvals[2] & 0x3f);
	}
	else
	{   // Week Day Alarm
		t->time.tm_wday = ins5699_get_week_day( alarmvals[2] & 0x7f );
		t->time.tm_mday = -1;
	}

	t->time.tm_mon  = -1;
	t->time.tm_year = -1;

	dev_dbg(dev, "%s: date: %ds %dm %dh %dwd %dmd %dm %dy\n",
		__func__,
		t->time.tm_sec, t->time.tm_min, t->time.tm_hour, t->time.tm_wday,
		t->time.tm_mday, t->time.tm_mon, t->time.tm_year);

	//check if INTR is enabled
	t->enabled = !!(ins5699->ctrlreg & INS5699_BTC_CTRL_AIE);
	//check if flag is triggered
	t->pending = (ctrl[1] & INS5699_BTC_FLAG_AF) && t->enabled;

	dev_dbg(dev, "%s: t->enabled: %d t->pending: %d\n",
		__func__,
		t->enabled, t->pending);

	return err;
}

//----------------------------------------------------------------------
// ins5699_set_alarm()
// sets Alarm
//
//----------------------------------------------------------------------
static int ins5699_set_alarm(struct device *dev, struct rtc_wkalrm *t)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ins5699_data *ins5699 = dev_get_drvdata(dev);
	u8 alarmvals[3];		//minute, hour, day
	u8 ctrl[2];				//ext, flag registers
	int err;

	if (client->irq <= 0)
		return -EINVAL;

	dev_dbg(dev, "%s: date: %ds %dm %dh %dwd %dmd %dm %dy\n",
		__func__,
		t->time.tm_sec, t->time.tm_min, t->time.tm_hour, t->time.tm_wday,
		t->time.tm_mday, t->time.tm_mon, t->time.tm_year);

	//get current flag register
	mem_clear(ctrl, sizeof(ctrl));
	err = ins5699_read_regs(client, INS5699_BTC_FLAG, 2, ctrl);
	if (err <0)
		return err;

	// Hardware alarm precision is 1 minute
	alarmvals[0] = bin2bcd(t->time.tm_min);
	alarmvals[1] = bin2bcd(t->time.tm_hour);

	if ( ctrl[0] & INS5699_BTC_EXT_WADA )
	{   // Day Alarm
		alarmvals[2] = bin2bcd(t->time.tm_mday);
		dev_dbg(dev, "%s: write min:0x%02x hour:0x%02x mday:0x%02x\n",
		__func__, alarmvals[0], alarmvals[1], alarmvals[2]);
	}
	else
	{   // Week Day Alarm
		alarmvals[2] = 1 << (t->time.tm_wday);
		dev_dbg(dev, "%s: write min:0x%02x hour:0x%02x wday:0x%02x\n",
		__func__, alarmvals[0], alarmvals[1], alarmvals[2]);
	}


	//check interrupt enable and disable
	if (ins5699->ctrlreg & (INS5699_BTC_CTRL_AIE | INS5699_BTC_CTRL_UIE) ) {
		ins5699->ctrlreg &= ~(INS5699_BTC_CTRL_AIE | INS5699_BTC_CTRL_UIE);
		err = ins5699_write_reg(ins5699->client, INS5699_BTC_CTRL, ins5699->ctrlreg);
		if (err)
			return err;
	}

	//write the new minute and hour values
	err = ins5699_write_regs(ins5699->client, INS5699_BTC_ALARM_MIN, 3, alarmvals);
	if (err)
		return err;

	//clear Alarm Flag
	ctrl[1] &= ~INS5699_BTC_FLAG_AF;

	err = ins5699_write_reg(ins5699->client, INS5699_BTC_FLAG, ctrl[1]);
	if (err)
		return err;

	//re-enable interrupt if required
	if (t->enabled) {
		//set update interrupt enable
		if ( ins5699->rtc->uie_rtctimer.enabled )
			ins5699->ctrlreg |= INS5699_BTC_CTRL_UIE;
		//set alarm interrupt enable
		if ( ins5699->rtc->aie_timer.enabled )
			ins5699->ctrlreg |= INS5699_BTC_CTRL_AIE | INS5699_BTC_CTRL_UIE;

		err = ins5699_write_reg(ins5699->client, INS5699_BTC_CTRL, ins5699->ctrlreg);
		if (err)
			return err;
	}

	return 0;
}

//----------------------------------------------------------------------
// ins5699_alarm_irq_enable()
// sets enables Alarm IRQ
//
//----------------------------------------------------------------------
static int ins5699_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ins5699_data *ins5699 = dev_get_drvdata(dev);
	u8 flagreg;
	u8 ctrl;
	int err;

	//get the current ctrl settings
	ctrl = ins5699->ctrlreg;

	dev_dbg(dev, "%s: %s => 0x%02x\n", __func__, "ctrl", ctrl);

	if (enabled)
	{
		// set update interrupt enable
		if ( ins5699->rtc->uie_rtctimer.enabled )
			ctrl |= INS5699_BTC_CTRL_UIE;
		// set alarm interrupt enable
		if ( ins5699->rtc->aie_timer.enabled )
			ctrl |= (INS5699_BTC_CTRL_AIE | INS5699_BTC_CTRL_UIE);
	}
	else
	{
		//clear update interrupt enable
		if ( ! ins5699->rtc->uie_rtctimer.enabled )
			ctrl &= ~INS5699_BTC_CTRL_UIE;
		//clear alarm interrupt enable
		if ( ! ins5699->rtc->aie_timer.enabled )
		{
			if ( ins5699->rtc->uie_rtctimer.enabled )
				ctrl &= ~INS5699_BTC_CTRL_AIE;
			else
				ctrl &= ~(INS5699_BTC_CTRL_AIE | INS5699_BTC_CTRL_UIE);
		}
	}

	dev_dbg(dev, "%s: %s => 0x%02x\n", __func__, "ctrl", ctrl);

	//clear alarm flag
	err = ins5699_read_reg(client, INS5699_BTC_FLAG, &flagreg);
	if (err <0)
		return err;
	flagreg &= ~INS5699_BTC_FLAG_AF;
	err = ins5699_write_reg(client, INS5699_BTC_FLAG, flagreg);
	if (err)
		return err;

	//update the Control register if the setting changed
	if (ctrl != ins5699->ctrlreg) {
		ins5699->ctrlreg = ctrl;
		err = ins5699_write_reg(client, INS5699_BTC_CTRL, ins5699->ctrlreg);
		if (err)
			return err;
	}

	err = ins5699_read_reg(client, INS5699_BTC_CTRL, &ctrl);
	if (err)
		return err;
	dev_dbg(dev, "%s: REG[0x%02x] => 0x%02x\n", __func__, INS5699_BTC_CTRL, ctrl);

	return 0;
}

//---------------------------------------------------------------------------
// ins5699_ioctl()
//
//---------------------------------------------------------------------------
static int ins5699_ioctl(struct device *dev, unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = to_i2c_client(dev);
	//struct ins5699_data *ins5699 = dev_get_drvdata(dev);
	//struct mutex *lock = &ins5699->rtc->ops_lock;
	int ret = 0;
	int tmp;
	void __user *argp = (void __user *)arg;
	reg_data reg;

	dev_dbg(dev, "%s: cmd=%x\n", __func__, cmd);

	switch (cmd) {
		case SE_RTC_REG_READ:
			if (copy_from_user(&reg, argp, sizeof(reg)))
				return -EFAULT;
			if ( reg.number > INS5699_EXT_CTRL )
				return -EFAULT;
			//mutex_lock(lock);
			ret = ins5699_read_reg(client, reg.number, &reg.value);
			//mutex_unlock(lock);
			if (! ret )
				return copy_to_user(argp, &reg, sizeof(reg)) ? -EFAULT : 0;
			break;

		case SE_RTC_REG_WRITE:
			if (copy_from_user(&reg, argp, sizeof(reg)))
				return -EFAULT;
			if ( reg.number > INS5699_EXT_CTRL )
				return -EFAULT;
			//mutex_lock(lock);
			ret = ins5699_write_reg(client, reg.number, reg.value);
			//mutex_unlock(lock);
			break;

		case RTC_VL_READ:
			//mutex_lock(lock);
			ret = ins5699_read_reg(client, INS5699_BTC_FLAG, &reg.value);
			//mutex_unlock(lock);
			if (! ret)
			{
				tmp = !!(reg.value & (INS5699_BTC_FLAG_VLF | INS5699_BTC_FLAG_VDET));
				return copy_to_user(argp, &tmp, sizeof(tmp)) ? -EFAULT : 0;
			}
			break;

		case RTC_VL_CLR:
			//mutex_lock(lock);
			ret = ins5699_read_reg(client, INS5699_BTC_FLAG, &reg.value);
			if (! ret)
			{
				reg.value &= ~(INS5699_BTC_FLAG_VLF | INS5699_BTC_FLAG_VDET);
				ret = ins5699_write_reg(client, INS5699_BTC_FLAG, reg.value);
			}
			//mutex_unlock(lock);
			break;

		default:
			return -ENOIOCTLCMD;
	}

	return ret;
}

static struct rtc_class_ops ins5699_rtc_ops = {
	.read_time = ins5699_get_time,
	.set_time = ins5699_set_time,
	.read_alarm = ins5699_get_alarm,
	.set_alarm = ins5699_set_alarm,
	.alarm_irq_enable = ins5699_alarm_irq_enable,
	.ioctl = ins5699_ioctl,
};

//----------------------------------------------------------------------
// ins5699_probe()
// probe routine for the ins5699 driver
//
//----------------------------------------------------------------------
static int ins5699_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct ins5699_data *ins5699;
	int err, need_reset = 0;

	dev_dbg(&client->dev, "IRQ %d supplied\n", client->irq);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA |
											I2C_FUNC_SMBUS_I2C_BLOCK)) {
		dev_err(&adapter->dev, "doesn't support required functionality\n");
		err = -EIO;
		goto errout;
	}

	ins5699 = devm_kzalloc(&client->dev, sizeof(struct ins5699_data), GFP_KERNEL);
	if (!ins5699) {
		dev_err(&adapter->dev, "failed to alloc memory\n");
		err = -ENOMEM;
		goto errout;
	}

	ins5699->client = client;
	i2c_set_clientdata(client, ins5699);

	err = ins5699_init_client(client, &need_reset);
	if (err)
		goto errout;

	if (need_reset) {
		struct rtc_time tm;
		tm = rtc_ktime_to_tm((ktime_t)0);		// set to 1970/1/1
		ins5699_set_time(&client->dev, &tm);
		dev_warn(&client->dev, " - time set to 1970/1/1\n");
	}

	ins5699->rtc = devm_rtc_device_register(&client->dev, client->name,
										&ins5699_rtc_ops, THIS_MODULE);


	if (IS_ERR(ins5699->rtc)) {
		err = PTR_ERR(ins5699->rtc);
		dev_err(&client->dev, "unable to register the class device\n");
		goto errout;
	}

	if (client->irq > 0) {
		dev_info(&client->dev, "IRQ %d supplied\n", client->irq);
		INIT_WORK(&ins5699->work, ins5699_work);
		err = devm_request_threaded_irq(&client->dev,
										client->irq,
										NULL,
										ins5699_irq,
										IRQF_TRIGGER_LOW | IRQF_ONESHOT,
										"ins5699",
										client);

		if (err) {
			dev_err(&client->dev, "unable to request IRQ\n");
			goto errout_reg;
		}
	}

	ins5699->rtc->irq_freq = 1;
	ins5699->rtc->max_user_freq = 1;

	return 0;

errout_reg:

errout:
	dev_err(&adapter->dev, "probing for ins5699 failed\n");
	return err;
}

//----------------------------------------------------------------------
// ins5699_remove()
// remove routine for the ins5699 driver
//
//----------------------------------------------------------------------
static void ins5699_remove(struct i2c_client *client)
{
	struct ins5699_data *ins5699 = i2c_get_clientdata(client);
	struct mutex *lock = &ins5699->rtc->ops_lock;

	if (client->irq > 0) {
		mutex_lock(lock);
		ins5699->exiting = 1;
		mutex_unlock(lock);

		cancel_work_sync(&ins5699->work);
	}

	return;
}

static struct i2c_driver ins5699_driver = {
	.driver = {
		.name = "rtc-ins5699",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ins5699_of_match),
	},
	.probe		= ins5699_probe,
	.remove		= ins5699_remove,
	.id_table	= ins5699_id,
};

module_i2c_driver(ins5699_driver);

MODULE_AUTHOR("Dptel company");
MODULE_DESCRIPTION("INS-5699 SA/LC RTC driver");
MODULE_LICENSE("GPL");
