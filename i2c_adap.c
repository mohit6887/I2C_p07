#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/platform_data/i2c-omap.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include "i2c_adap.h"

#define DRIVER_NAME "I2C_PLDRV"

static struct omap_i2c_dev i2c_dev;

inline void omap_i2c_write_reg(struct omap_i2c_dev *i2c_dev,
                                      int reg, u16 val)
{       
        __raw_writew(val, i2c_dev->base + i2c_dev->regs[reg]);
}

inline u16 omap_i2c_read_reg(struct omap_i2c_dev *i2c_dev, int reg)
{
        return __raw_readw(i2c_dev->base + i2c_dev->regs[reg]);
}

inline void omap_i2c_ack_stat(struct omap_i2c_dev *dev, u16 stat)
{
        omap_i2c_write_reg(dev, OMAP_I2C_STAT_REG, stat);
}

/*
 * Waiting on Bus Busy
 */
int omap_i2c_wait_for_bb(struct omap_i2c_dev *dev)
{
        unsigned long timeout;

        timeout = jiffies + OMAP_I2C_TIMEOUT;
        while (omap_i2c_read_reg(dev, OMAP_I2C_STAT_REG) & OMAP_I2C_STAT_BB) {
                if (time_after(jiffies, timeout)) {
                        printk("timeout waiting for bus ready\n");
                        return -ETIMEDOUT;
                }
                msleep(1);
        }

        return 0;
}

void flush_fifo(struct omap_i2c_dev *dev)
{
	unsigned long timeout;
	u32 status;
	timeout = jiffies + OMAP_I2C_TIMEOUT;
        while ((status = omap_i2c_read_reg(dev, OMAP_I2C_STAT_REG)) & OMAP_I2C_STAT_RRDY) {
                omap_i2c_read_reg(dev, OMAP_I2C_DATA_REG);
                omap_i2c_ack_stat(dev, OMAP_I2C_STAT_RRDY);
                if (time_after(jiffies, timeout)) {
                        printk(KERN_ALERT "timeout waiting for bus ready\n");
                        break;
                }
                msleep(1);
        }
}

u16 wait_for_event(struct omap_i2c_dev *dev)
{
        unsigned long timeout = jiffies + OMAP_I2C_TIMEOUT;
        u16 status;

        while (!((status = omap_i2c_read_reg(dev, OMAP_I2C_STAT_REG)) &
                                (OMAP_I2C_STAT_ROVR | OMAP_I2C_STAT_XUDF |
                                 OMAP_I2C_STAT_XRDY | OMAP_I2C_STAT_RRDY |
                                 OMAP_I2C_STAT_ARDY | OMAP_I2C_STAT_NACK |
                                 OMAP_I2C_STAT_AL))) {
                if (time_after(jiffies, timeout)) {
                        printk("time-out waiting for event\n");
                        omap_i2c_write_reg(dev, OMAP_I2C_STAT_REG, 0XFFFF);
                        return 0;
                }
                mdelay(1);
        }
        return status;
}

static void omap_i2c_resize_fifo(u8 size, bool is_rx) 
{ 
	u16	buf; 
 
	/* 
	 * Set up notification threshold based on message size. We're doing 
	 * this to try and avoid draining feature as much as possible. Whenever 
	 * we have big messages to transfer (bigger than our total fifo size) 
	 * then we might use draining feature to transfer the remaining bytes. 
	 */ 
 
	i2c_dev.threshold = clamp(size, (u8) 1, i2c_dev.fifo_size); 
 
	buf = omap_i2c_read_reg(&i2c_dev, OMAP_I2C_BUF_REG); 
 
	if (is_rx) { 
		/* Clear RX Threshold */ 
		buf &= ~(0x3f << 8); 
		buf |= ((i2c_dev.threshold - 1) << 8) | OMAP_I2C_BUF_RXFIF_CLR; 
	} else { 
		/* Clear TX Threshold */ 
		buf &= ~0x3f; 
		buf |= (i2c_dev.threshold - 1) | OMAP_I2C_BUF_TXFIF_CLR; 
	} 
	omap_i2c_write_reg(&i2c_dev, OMAP_I2C_BUF_REG, buf); 
}

int i2c_transmit(struct i2c_msg *msg, size_t count)
{
	//TODO: 3.1 Update the cnt to 3
	u16 status, cnt = 2, w;
	int i2c_error = 0;
	/* Initialize the loop variable */
	int k = 7;
	
	//TODO: 3.2 Declare an array of 3
	// Index 0 - Eeprom address upper 8 bits
	// Index 1 - Eeprom address lower 8 bits
	// Index 2 - data
	u8 a[] = {0x00, 0x30, 0x95};
	int i = 0;

	ENTER();

	//TODO 2.6 Set the TX FIFO Threshold to 0 and clear the FIFO's
	omap_i2c_write_reg(&i2c_dev, OMAP_I2C_BUF_REG, 0);
	//TODO 2.7: Update the slave addresss register with 0x50
	omap_i2c_write_reg(&i2c_dev, OMAP_I2C_SA_REG, 0x50); /* Slave Address */
	//TODO 2.8: Update the count register with 1
	omap_i2c_write_reg(&i2c_dev, OMAP_I2C_CNT_REG, cnt);
	printk("##### Sending %d byte(s) on the I2C bus ####\n", cnt);

	/*
	 * TODO 2.9: Update the configuration register (OMAP_I2C_CON_REG) to start the
	 * transaction with master mode and direction as transmit. Also, enable the
	 * I2c module and set the start and stop bits.
	 * The naming convention for the bits is OMAP_I2C_<REG NAME>_<BIT NAME>
	 * So, for start bit, the macro is OMAP_I2C_CON_STT. Check I2c_char.h for other bits
	 */
	w = (OMAP_I2C_CON_MST | OMAP_I2C_CON_STT | OMAP_I2C_CON_EN | OMAP_I2C_CON_STP
			| OMAP_I2C_CON_TRX);
	omap_i2c_write_reg(&i2c_dev, OMAP_I2C_CON_REG, w); /* Control Register */

	while (k--) {
		// Wait for status to be updated
		status = wait_for_event(&i2c_dev);
		if (status == 0) {
			i2c_error = -ETIMEDOUT;
			goto wr_exit;
		}
		//TODO 2.10: Check the status to verify if XRDY is received
		if (status & OMAP_I2C_STAT_XRDY) {
			printk("Got XRDY\n");
			//TODO 2.11: Update the data register with data to be transmitted
			//TODO 3.3: Write an array into the data register
			omap_i2c_write_reg(&i2c_dev, OMAP_I2C_DATA_REG, a[i++]);
			//TODO 2.12: Clear the XRDY event with omap_i2c_ack_stat
			omap_i2c_ack_stat(&i2c_dev, OMAP_I2C_STAT_XRDY);
			continue;
		}
		//TODO 2.13: Check the status to verify if ARDY is received
		if (status & OMAP_I2C_STAT_ARDY) {
			printk("Got ARDY\n");
			//TODO 2.14: Clear the XRDY event with omap_i2c_ack_stat and break out of loop
			omap_i2c_ack_stat(&i2c_dev, OMAP_I2C_STAT_ARDY);
			break;
		}
	}
	if (k <= 0) {
		printk("TX Timed out\n");
		i2c_error = -ETIMEDOUT;
	}
wr_exit:
	flush_fifo(&i2c_dev);
	omap_i2c_write_reg(&i2c_dev, OMAP_I2C_STAT_REG, 0XFFFF);
	return i2c_error;

}

int i2c_receive(struct i2c_msg *msg, size_t count)
{	
	u16 status, cnt = 3, w;
	int i2c_error = 0;
	/* Initialize the loop variable */
	int k = 7;
	u8 a;

	ENTER();

	// Set the TX FIFO Threshold to 0 and clear the FIFO's
	omap_i2c_write_reg(&i2c_dev, OMAP_I2C_BUF_REG, 0);
	// Update the slave addresss register with 0x50
	omap_i2c_write_reg(&i2c_dev, OMAP_I2C_SA_REG, 0x50); /* Slave Address */
	// Update the count register with 1
	omap_i2c_write_reg(&i2c_dev, OMAP_I2C_CNT_REG, cnt);
	printk("##### Receiving %d byte(s) on the I2C bus ####\n", cnt);

	/*
	 * TODO 4.1: Update the configuration register (OMAP_I2C_CON_REG) to start the
	 * transaction with master mode and direction as receive. Also, enable the
	 * I2c module and set the start and stop bits.
	 * The naming convention for the bits is OMAP_I2C_<REG NAME>_<BIT NAME>
	 * So, for start bit, the macro is OMAP_I2C_CON_STT. Check I2c_char.h for other bits
	 */
	w = (OMAP_I2C_CON_MST | OMAP_I2C_CON_STT | OMAP_I2C_CON_EN | OMAP_I2C_CON_STP);
	omap_i2c_write_reg(&i2c_dev, OMAP_I2C_CON_REG, w); /* Control Register */

	while (k--) {
		// Wait for status to be updated
		status = wait_for_event(&i2c_dev);
		if (status == 0) {
			i2c_error = -ETIMEDOUT;
			goto wr_exit;
		}
		//TODO 4.2: Check the status to verify if RRDY is received
		if (status & OMAP_I2C_STAT_RRDY) {
			printk("Got RRDY\n");
			//TODO 4.3: Read Data register
			a = omap_i2c_read_reg(&i2c_dev, OMAP_I2C_DATA_REG);
                    	printk("Received %x\n", a);
			//TODO 4.4: Clear the RRDY event with omap_i2c_ack_stat
			omap_i2c_ack_stat(&i2c_dev, OMAP_I2C_STAT_RRDY);
			continue;
		}
		//TODO 4.5: Check the status to verify if ARDY is received
		if (status & OMAP_I2C_STAT_ARDY) {
			printk("Got ARDY\n");
			//TODO 4.6: Clear the XRDY event with omap_i2c_ack_stat and break out of loop
			omap_i2c_ack_stat(&i2c_dev, OMAP_I2C_STAT_ARDY);
			break;
		}
	}
	if (k <= 0) {
		printk("RX Timed out\n");
		i2c_error = -ETIMEDOUT;
	}
wr_exit:
	flush_fifo(&i2c_dev);
	omap_i2c_write_reg(&i2c_dev, OMAP_I2C_STAT_REG, 0XFFFF);
	return i2c_error;
}

int i2c_txrx(struct i2c_adapter *adap, struct i2c_msg msg[], int count)
{
	u16 w, j = 0;
	int i2c_error = 0;
	unsigned long timeout;

	ENTER();

	while (j < count) {
		i2c_dev.receiver = !!(msg[j].flags & I2C_M_RD);
		omap_i2c_resize_fifo(msg[j].len, i2c_dev.receiver);

		// TODO 10.1: Update buff to point to i2c_msg buf
		i2c_dev.buf = msg[j].buf;
		i2c_dev.buf_len = msg[j].len;
		i2c_error = count;

		/* 
		 * TODO 8.3: Update CNT, FIFO, SA and CON registers as per the
		 * i2c_msg. Set the TRX bit as per the mode of operation
		 */
		omap_i2c_write_reg(&i2c_dev, OMAP_I2C_SA_REG, msg[j].addr); /* Slave Address */
		omap_i2c_write_reg(&i2c_dev, OMAP_I2C_CNT_REG, msg[j].len);

		/* Clear the FIFO Buffers */
		w = omap_i2c_read_reg(&i2c_dev, OMAP_I2C_BUF_REG);
		w |= OMAP_I2C_BUF_RXFIF_CLR | OMAP_I2C_BUF_TXFIF_CLR;
		omap_i2c_write_reg(&i2c_dev, OMAP_I2C_BUF_REG, w);

		printk("##### Sending/Receiving %d byte(s) on the I2C bus ####\n", msg[j].len);

		w = (OMAP_I2C_CON_MST | OMAP_I2C_CON_STT | OMAP_I2C_CON_EN | OMAP_I2C_CON_STP);

		if (!(msg[j].flags & I2C_M_RD))
		{
			w |= OMAP_I2C_CON_TRX;
		}

		// TODO 10.2: Re-initialize the completion data data structure
		reinit_completion(&i2c_dev.cmd_complete);
		omap_i2c_write_reg(&i2c_dev, OMAP_I2C_CON_REG, w); /* Control Register */
		i2c_dev.cmd_err = 0;
		printk("Waiting...\n");
		/* 
		 * TODO: Wait for completion with timeout OMAP_I2C_TIMEOUT. 
		 * Refer include/linux/completion.h
		 */
		timeout = wait_for_completion_timeout(&(i2c_dev.cmd_complete),
				OMAP_I2C_TIMEOUT);
		if (timeout == 0) {
			printk("controller timed out\n");
			return -ETIMEDOUT;
		}
		flush_fifo(&i2c_dev);
		omap_i2c_write_reg(&i2c_dev, OMAP_I2C_STAT_REG, 0XFFFF);
		j++;

	}
	return i2c_error;
}

static u32 i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | (I2C_FUNC_SMBUS_EMUL & ~I2C_FUNC_SMBUS_QUICK) |
	       I2C_FUNC_PROTOCOL_MANGLING;
}

//TODO 9.1: Populate the i2c_algo structure
static const struct i2c_algorithm i2c_algo = {
	.master_xfer	= i2c_txrx,
	.functionality	= i2c_func,
};

static void omap_i2c_set_speed(struct omap_i2c_dev *dev)
{
        u16 psc = 0;
        unsigned long fclk_rate = 48000;
        unsigned long internal_clk = 24000; // Recommended as per TRM
        unsigned long scl, scll, sclh;

        /* Compute prescaler divisor */
        psc = fclk_rate / internal_clk;
        //TODO 2.4: Update the prescalar register with psc - 1
        omap_i2c_write_reg(dev, OMAP_I2C_PSC_REG, psc - 1);

        // Hard coding the speed to 400KHz
        dev->speed = 400;
        scl = internal_clk / dev->speed;
        // 50% duty cycle
        scl /= 2;
        scll = scl - 7;
        sclh = scl - 5;

        //TODO 2.5: Update the SCL low and high registers as per above calculations
        omap_i2c_write_reg(dev, OMAP_I2C_SCLL_REG, scll);
        omap_i2c_write_reg(dev, OMAP_I2C_SCLH_REG, sclh);
}

int omap_i2c_init(struct omap_i2c_dev *dev)
{
        omap_i2c_set_speed(dev);
        omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, 0);

        /* Take the I2C module out of reset: */
        omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, OMAP_I2C_CON_EN);

        // TODO 2.2: Update the 'iestate' field with desired events such as XRDY and ARDY
	// TODO 4.7: Update the 'iestate' to enable RRDY event
	// TODO: Update the 'iestate' field with desired events including XDR and RDR
	dev->iestate = (OMAP_I2C_IE_XRDY | OMAP_I2C_IE_RRDY |
			OMAP_I2C_IE_ARDY | OMAP_I2C_IE_NACK | OMAP_I2C_IE_XDR | OMAP_I2C_IE_RDR |
		       OMAP_I2C_IE_AL);

        // TODO 2.3: Update the OMAP_I2C_IE_REG
	omap_i2c_write_reg(dev, OMAP_I2C_IE_REG, dev->iestate);

        flush_fifo(dev);
        omap_i2c_write_reg(dev, OMAP_I2C_STAT_REG, 0XFFFF);
        omap_i2c_wait_for_bb(dev);

        return 0;
}

static void receive_data(struct omap_i2c_dev *omap, u8 num_bytes)
{
        u8 w;
		printk("Recieving %d bytes of data\n", num_bytes);
        while (num_bytes--) {
                w = omap_i2c_read_reg(omap, OMAP_I2C_DATA_REG);
                *omap->buf++ = w;
                omap->buf_len--;
       }
}

static int transmit_data(struct omap_i2c_dev *omap, u8 num_bytes)
{
	u8 w;
	printk("Transmitting %d bytes of data\n", num_bytes);
	while (num_bytes--) {
		w = *omap->buf++;
		omap->buf_len--;
		omap_i2c_write_reg(omap, OMAP_I2C_DATA_REG, w);
	}
	return 0;
}

static irqreturn_t thread_fn(int this_irq, void *dev_id) {
	u16 bits, status;
	int err = 0, ret = 0;

	do {
		bits = omap_i2c_read_reg(&i2c_dev, OMAP_I2C_IE_REG);
		status = omap_i2c_read_reg(&i2c_dev, OMAP_I2C_STAT_REG);
		status &= bits;
		
		/* If we're in receiver mode, ignore XDR/XRDY */
		if (i2c_dev.receiver)
			status &= ~(OMAP_I2C_STAT_XDR | OMAP_I2C_STAT_XRDY);
		else
			status &= ~(OMAP_I2C_STAT_RDR | OMAP_I2C_STAT_RRDY);
        	if (!status) {
                	goto out;
        	}

		if (status & OMAP_I2C_STAT_XRDY) {
			printk("Got XRDY\n");

			ret = transmit_data(&i2c_dev, i2c_dev.threshold);
 			if (ret < 0)
 				break;
			omap_i2c_ack_stat(&i2c_dev, OMAP_I2C_STAT_XRDY);
			continue;   
		}
		if (status & OMAP_I2C_STAT_RRDY) {
			printk("Got RRDY\n");
			receive_data(&i2c_dev, i2c_dev.threshold);
			omap_i2c_ack_stat(&i2c_dev, OMAP_I2C_STAT_RRDY);
			continue;
        	}
		if (status & OMAP_I2C_STAT_XDR) {
			printk("Got XDR\n");

			ret = transmit_data(&i2c_dev, i2c_dev.buf_len);
			if (ret < 0)
				break;

			omap_i2c_ack_stat(&i2c_dev, OMAP_I2C_STAT_XDR);
			continue;
		}

		if (status & OMAP_I2C_STAT_RDR) {
			printk("Got RDR\n");

			receive_data(&i2c_dev, i2c_dev.buf_len);
			omap_i2c_ack_stat(&i2c_dev, OMAP_I2C_STAT_RDR);
			continue;
		}

		if (status & OMAP_I2C_STAT_ARDY) {	
			printk("Got ARDY\n");
			omap_i2c_ack_stat(&i2c_dev, OMAP_I2C_STAT_ARDY);   
			break;
		}
	} while (status);

	i2c_dev.cmd_err |= err;
	// TODO: Send completion 
	complete(&(i2c_dev.cmd_complete));

out:
	return IRQ_HANDLED;
}

static irqreturn_t irq_fn(int irq, void *dev_id)
{
	struct omap_i2c_dev *omap = dev_id;
	irqreturn_t ret = IRQ_HANDLED;
	u16 mask, stat;

	stat = omap_i2c_read_reg(omap, OMAP_I2C_STAT_REG);
	mask = omap_i2c_read_reg(omap, OMAP_I2C_IE_REG);

	if (stat & mask)
		ret = IRQ_WAKE_THREAD;
	return ret;
}


static int i2c_drv_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct device_node *np = pdev->dev.of_node;
	u32 clk_freq = 0;
	struct i2c_adapter *adap;
	int r;
	u16 s;

	// TODO: Get the irq number
	i2c_dev.irq = platform_get_irq(pdev, 0);
	if (i2c_dev.irq < 0) {
		dev_err(&pdev->dev, "no irq resource?\n");
		return -1;
	}
	printk("Irq number is %d\n", i2c_dev.irq);

	// TODO: Initialize the completion data structure
	init_completion(&(i2c_dev.cmd_complete));

	//TODO 5.4: Get the physical address from the platform device
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) 
	{
		pr_err(" Specified Resource Not Available... 1\n");
		return -1;
	}
	printk(KERN_ALERT "\n Memory Area1\n");
	printk(KERN_ALERT "Start:%lx, End:%lx Size:%d\n", (unsigned long)res->start, 
			(unsigned long)res->end, resource_size(res));
	printk("Resource size = %x\n", resource_size(res));

	/*
         * TODO 2.1: Get the virtual address for the i2c0 base address and store it
         * in 'base' field of omap_i2c_dev.
         * Use API void __iomem* ioremap((resource_size_t offset, unsigned long size)
        */

	i2c_dev.base = ioremap(res->start, resource_size(res));
        if (IS_ERR(i2c_dev.base)) {
                printk(KERN_ERR "Unable to ioremap\n");
                return PTR_ERR(i2c_dev.base);
        }

        i2c_dev.regs = (u8 *)reg_map_ip_v2;

	/*
	 * TODO: Get the the total fifo depth from OMAP_I2C_BUFTAT_REG
	 * and accordingly update fifo_size field in i2c_dev to half of
	 * the total fifo depth. This will serve as notification threshold
	 */
	s = (omap_i2c_read_reg(&i2c_dev, OMAP_I2C_BUFSTAT_REG) >> 14) & 0x3;
	i2c_dev.fifo_size = 0x8 << s;
	i2c_dev.fifo_size = (i2c_dev.fifo_size / 2);
	printk("FIFO size = %d\n", i2c_dev.fifo_size);

	/*
	 * TODO 5.5: Get the clock_frequency from the platform_data of
	 * plaform device and assign it to speed field omap_i2c_dev
	 */
	/*
	 * TODO 6.3: Get the clock_frequency from the dtb node
	 * into clk_freq. Overwriting changes from TODO 5.5
	 */
	of_property_read_u32(np, "clock-frequency", &clk_freq);
	i2c_dev.speed = clk_freq;
	printk("Clock frequency is %u\n", i2c_dev.speed);

	omap_i2c_init(&i2c_dev);

	/* 
	 * TODO: Register the threaded irq interrupt with irq_fn as top half
	 * and irq bottom half. Use the flag IRQF_ONESHOT
	 */
	if (request_threaded_irq(i2c_dev.irq, irq_fn, thread_fn, IRQF_ONESHOT, "i2c_irq",
                                  &i2c_dev) < 0) {
		printk("Requesting IRQ failed\n");
		return -1;
	}

	adap = devm_kzalloc(&pdev->dev, sizeof(struct i2c_adapter), GFP_KERNEL);
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_HWMON;
	strlcpy(adap->name, "My I2C adapter", sizeof(adap->name));
	// TODO 9.2: Register the algo 
	adap->algo = &i2c_algo;
	adap->dev.parent = &pdev->dev;
	adap->dev.of_node = pdev->dev.of_node;

	printk("Adap No. is %d\n", adap->nr);
	/* i2c device drivers may be active on return from add_adapter() */
	adap->nr = pdev->id;
	//TODO 9.3: Register the adapter with i2c subsystem
	r = i2c_add_numbered_adapter(adap);
	if (r) {
		printk("Failure adding adapter\n");
		return r;
	}
	platform_set_drvdata(pdev, adap);

	return 0;
}

static int i2c_drv_remove(struct platform_device *pdev)
{
	struct i2c_adapter *adapter = platform_get_drvdata(pdev);
	//TODO 9.4: De-register the adapter from the I2C subsystem
	i2c_del_adapter(adapter);

	return 0;
}

//TODO 6.1: Initialize the compatible property
static const struct of_device_id i2c_drv_dt[] = {
	{ .compatible = "my-i2c", },
	{ }
};

//TODO 5.1: Populate the platform driver structure
static struct platform_driver i2c_pldriver = 
{
	.probe          = i2c_drv_probe,
	.remove         = i2c_drv_remove,
	.driver = 
	{
		.name  = DRIVER_NAME,
		// TODO 6.2 Intialize of_match_table
		.of_match_table = of_match_ptr(i2c_drv_dt),
	},
};

static int __init i2c_init_driver(void)
{
	//TODO 5.2: Register the platform driver
	platform_driver_register(&i2c_pldriver);
	return 0;
}

static void __exit i2c_exit_driver(void)
{
	//TODO 5.3: Un-register the platform driver
	platform_driver_unregister(&i2c_pldriver);
}
module_init(i2c_init_driver);
module_exit(i2c_exit_driver);

MODULE_AUTHOR("Embitude Trainings <info@embitude.in>");
MODULE_DESCRIPTION("Low level I2C driver");
MODULE_LICENSE("GPL");
