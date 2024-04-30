#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/platform_data/serial-omap.h>

struct chrdev_data {
	struct i2c_client *client;
	u8 *write_buff;
	u16 write_max;
	/* Character Driver Files */
	dev_t devt;
	struct cdev cdev;
	struct class *class;
};	

static int my_open(struct inode *i, struct file *f)
{
	struct chrdev_data *dev = container_of(i->i_cdev, struct chrdev_data, cdev);
	if (dev == NULL) {
		printk("Data is null\n");
		return -1;
	}
	f->private_data = dev;

	return 0;
}

static int my_close(struct inode *i, struct file *f)
{
	return 0;
}

static ssize_t my_read(struct file *f, char __user *buf, size_t count, loff_t *off)
{
	struct i2c_msg msg[2];
	int i;
	u8 buff[6] = {0x00, 0x30, 0xaa};

	/*
	 * TODO 9.11: Get chrdev_data from private data field and 
	 * correspondingly get i2c_adapter and i2c_client to 
	 * be used in subsequent APIs
	 */
	struct chrdev_data *dev = (struct chrdev_data *)(f->private_data);
	struct i2c_adapter *adap = dev->client->adapter;
	struct i2c_client *client = dev->client;

	memset(msg, 0, sizeof(msg));

	/* 
	 * TODO 8.8: Since read invokes i2c transmit and i2c receive,
	 * there is a need to send 2 i2c_msg - one for transmit
	 * and other for receive.
	 * Initialize the fields for both i2c_msg
	 */
	//TODO 9.12: Get the slave address from i2c_client data structure
	msg[0].buf = buff;
	msg[0].len = 2;
	msg[0].addr = client->addr;

	msg[1].buf = buff;
	msg[1].len = 3;
	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;

	printk("Initiatiating Transaction\n");
	//TODO 9.13 Invoke the i2c_transfer
	i2c_transfer(adap, msg, 2); 
	printk("The data is:\n");
	for (i = 0; i < 3; i++) {
	       printk("%x\n", msg[1].buf[i]);
	}

	return 0;
}

static ssize_t my_write(struct file *f, const char __user *buf, size_t count, loff_t *off)
{
	// TODO 1.11: Invoke i2c_transmit() from low level driver
	struct i2c_msg msg;
	u8 buff[3] = {0x00, 0x30, 0xaa};

	struct chrdev_data *dev = (struct chrdev_data *)(f->private_data);
	struct i2c_adapter *adap = dev->client->adapter;
	struct i2c_client *client = dev->client;

	memset(&msg, 0, sizeof(msg));

	 //TODO 9.9: Initialize the i2c_msg fields
	msg.buf = buff;
	msg.len = sizeof(buff);
	msg.addr = client->addr;

	//TODO 9.10: Invoke i2c_transfer for 1 message
	if (i2c_transfer(adap, &msg, 1) < 0) {
		printk("Error in transmission\n");
		return -1;
	}

	return count;
}

static struct file_operations driver_fops =
{
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_close,
	.read = my_read,
	.write = my_write
};

static int chrdev_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct chrdev_data *data;
	int init_result;

	printk("Client Driver probe Invoked\n");
	data = devm_kzalloc(&client->dev, sizeof(struct chrdev_data), GFP_KERNEL);
	data->write_max = 32;
	data->write_buff = devm_kzalloc(&client->dev, data->write_max, GFP_KERNEL);
	i2c_set_clientdata(client, data);

	init_result = alloc_chrdev_region(&data->devt, 0, 1, "i2c_drv");

	if (0 > init_result)
	{
		printk(KERN_ALERT "Device Registration failed\n");
		return -1;
	}
	printk("Major Nr: %d\n", MAJOR(data->devt));
	data->client = client;

	// TODO 1.4: Create the class with name i2cdrv
	if ((data->class = class_create(THIS_MODULE, "i2cdrv")) == NULL)
        {
		printk(KERN_ALERT "Class creation failed\n");
                return -1;
        }

	// TODO 1.5: Create the device file
        if (device_create(data->class, NULL, data->devt, NULL, 
				"i2c_drv%d", 0) == NULL) 
        {
                printk( KERN_ALERT "Device creation failed\n" ); 
                unregister_chrdev_region(data->devt, 1);
                return -1;
        }      

	// TODO 1.6: Register the file_operations
	cdev_init(&data->cdev, &driver_fops);                                   

        if (cdev_add(&data->cdev, data->devt, 1) == -1)
        {
                printk( KERN_ALERT "Device addition failed\n" ); 
                device_destroy(data->class, data->devt); 
                unregister_chrdev_region(data->devt, 1);
                return -1;
        }
	return 0;
}

static int chrdev_remove(struct i2c_client *client)
{
	struct chrdev_data *dev;

	printk("Client Driver Remove Invoked\n");
	dev = i2c_get_clientdata(client);

	cdev_del(&dev->cdev);
	device_destroy(dev->class, dev->devt);
	class_destroy(dev->class);
	unregister_chrdev_region(dev->devt, 1);
	return 0;
}

//TODO 9.5: Populate the id table to expose the devices supported
static const struct i2c_device_id chrdev_ids[] = {
	{ "chrdev_client", 0},
	{ } // Don't delete this. Serves as terminator
};
MODULE_DEVICE_TABLE(i2c, chrdev_ids);

// TODO 9.6: Populate the i2c_driver structure
static struct i2c_driver chrdev_driver = {
	.driver = {
		.name = "chrdev_client",
		.owner = THIS_MODULE,
	},
	.probe = chrdev_probe,
	.remove = chrdev_remove,
	.id_table = chrdev_ids,
};

static int __init chrdev_init(void)
{
	//TODO 9.7: Register the dummy client driver
	return i2c_add_driver(&chrdev_driver);
}

static void __exit chrdev_exit(void)
{
	//TODO 9.8: De-register the I2c client driver
	i2c_del_driver(&chrdev_driver);
}

module_init(chrdev_init)
module_exit(chrdev_exit)

MODULE_AUTHOR("Embitude Trainings <info@embitude.in>");
MODULE_DESCRIPTION("I2C Client Driver");
MODULE_LICENSE("GPL");
