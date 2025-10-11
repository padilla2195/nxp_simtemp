/******************/
/**** Includes ****/
/******************/
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/random.h>



/****************************/
/**** Macro definitions *****/
/****************************/
#define UPPER_THRESHOLD_TEMP_SIMULATION_MILI_C 50000U
#define LOWER_THRESHOLD_TEMP_SIMULATION_MILI_C 20000U
#define NORMAL_TEMPERATURE_VALUE               32000U
#define TEMP_SIMULATION_INCREMENTS               500U
#define MODE_NORMAL                                0U
#define MODE_NOISY                                 1U
#define MODE_RAMP                                  2U



/****************************/
/**** Function prototypes ***/
/****************************/
enum hrtimer_restart simtemp_timer_callback(struct hrtimer *timer);
__u32 simtemp_get_temperature(void);
static int __init simtemp_module_start(void);
static void __exit simtemp_module_exit(void);
ssize_t simtemp_sysfs_sampling_time_show(struct device *d, struct device_attribute *attr, char *buf);
ssize_t simtemp_sysfs_sampling_time_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count);
ssize_t simtemp_sysfs_temperature_threshold_show(struct device *d, struct device_attribute *attr, char *buf);
ssize_t simtemp_sysfs_temperature_threshold_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count);
ssize_t simtemp_sysfs_timestamp_show(struct device *d, struct device_attribute *attr, char *buf);
ssize_t simtemp_sysfs_timestamp_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count);
ssize_t simtemp_sysfs_temp_mc_show(struct device *d, struct device_attribute *attr, char *buf);
ssize_t simtemp_sysfs_temp_mc_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count);
ssize_t simtemp_sysfs_flags_show(struct device *d, struct device_attribute *attr, char *buf);
ssize_t simtemp_sysfs_flags_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count);
ssize_t simtemp_sysfs_mode_show(struct device *d, struct device_attribute *attr, char *buf);
ssize_t simtemp_sysfs_mode_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count);



/***************************************/
/**** Static variables definitions *****/
/***************************************/
/* Timer Variables */
static struct hrtimer simtemp_sampling_timer;
static ktime_t simtemp_timer_period;
/* Temperature sensing variables */
static __s32 simtemp_temperature_sensor_reading = 32000U;
static bool  simtemp_temperature_sensor_increment_flag = true; // True:temperature is incremented, False:temperature is decremented
/* Variables for timestamp measurements */
static ktime_t simtemp_timestamp_start;
static ktime_t simtemp_timestamp_stop;
/* Character device variables */
static dev_t dev_nr;
/* Variables for sysfs */
static struct class *simtemp_class;
static struct device *simtemp_dev;
__u32 simtemp_sysfs_sampling_time = 200; /* sampling time initialized to 200ms */
__s32 simtemp_sysfs_temperature_threshold = 40000; /* Threshold in mC */
__u64 simtemp_sysfs_timestamp_ns; /* Timestamp in ns*/
__u32 simtemp_sysfs_temp_mC; /* Measured temperature in mC */
__u32 simtemp_sysfs_flags; /* Flags */
__u32 simtemp_sysfs_mode = 0; /* Mode, possible values: 0 = Normal, 1 = Noisy, 2 = Ramp */


/************************************/
/**** Device attribute asignation ***/
/************************************/
DEVICE_ATTR(simtemp_sysfs_sampling_time, 0660, simtemp_sysfs_sampling_time_show, simtemp_sysfs_sampling_time_store);
DEVICE_ATTR(simtemp_sysfs_temperature_threshold, 0660, simtemp_sysfs_temperature_threshold_show, simtemp_sysfs_temperature_threshold_store);
DEVICE_ATTR(simtemp_sysfs_timestamp_ns, 0660, simtemp_sysfs_timestamp_show, simtemp_sysfs_timestamp_store);
DEVICE_ATTR(simtemp_sysfs_temp_mC, 0660, simtemp_sysfs_temp_mc_show, simtemp_sysfs_temp_mc_store);
DEVICE_ATTR(simtemp_sysfs_flags, 0660, simtemp_sysfs_flags_show, simtemp_sysfs_flags_store);
DEVICE_ATTR(simtemp_sysfs_mode, 0660, simtemp_sysfs_mode_show, simtemp_sysfs_mode_store);



/****************************/
/**** Function defintions ***/
/****************************/
/* @brief Operation that loads the module */
static int __init simtemp_module_start(void)
{
    int chr_dev_status;

    printk(KERN_INFO "Initializing simtemp module.\n");

    /* Register character device, use a free device number  */
    chr_dev_status = alloc_chrdev_region(&dev_nr, 0, MINORMASK + 1, "nxp_simtemp");
    if(chr_dev_status != 0)
    {
        printk(KERN_ERR "simtemp - Error allocating the device number\n");
        return chr_dev_status;
    }
    
    /* Initialize simtemp class for sysfs */
    simtemp_class = class_create("simtemp_class");
    if(IS_ERR(simtemp_class))
    {
        printk(KERN_ERR "simtemp - Error creating class\n");
        chr_dev_status = PTR_ERR(simtemp_class);
        unregister_chrdev_region(dev_nr, MINORMASK + 1);
        return chr_dev_status;
    }
    
    /* Create a device */
    simtemp_dev = device_create(simtemp_class, NULL, dev_nr, NULL, "simtemp_dev%d", MINOR(dev_nr));
    if(IS_ERR(simtemp_dev))
    {
        printk(KERN_ERR "simtemp - Error creating device\n");
        chr_dev_status = PTR_ERR(simtemp_dev);
        class_destroy(simtemp_class);
        return chr_dev_status;
    }

    /* Add the attributes to the device */
    chr_dev_status = device_create_file(simtemp_dev, &dev_attr_simtemp_sysfs_sampling_time);
    if(chr_dev_status != 0)
    {
        printk(KERN_ERR "simtemp - Error creating sampling_time attribute\n");
        device_destroy(simtemp_class, dev_nr);
        return chr_dev_status;
    }

    chr_dev_status = device_create_file(simtemp_dev, &dev_attr_simtemp_sysfs_temperature_threshold);
    if(chr_dev_status != 0)
    {
        printk(KERN_ERR "simtemp - Error creating temperature_threshold attribute\n");
        device_destroy(simtemp_class, dev_nr);
        return chr_dev_status;
    }
    chr_dev_status = device_create_file(simtemp_dev, &dev_attr_simtemp_sysfs_timestamp_ns);
    if(chr_dev_status != 0)
    {
        printk(KERN_ERR "simtemp - Error creating timestamp attribute\n");
        device_destroy(simtemp_class, dev_nr);
        return chr_dev_status;
    }
    chr_dev_status = device_create_file(simtemp_dev, &dev_attr_simtemp_sysfs_temp_mC);
    if(chr_dev_status != 0)
    {
        printk(KERN_ERR "simtemp - Error creating temp_mc attribute\n");
        device_destroy(simtemp_class, dev_nr);
        return chr_dev_status;
    }
    chr_dev_status = device_create_file(simtemp_dev, &dev_attr_simtemp_sysfs_flags);
    if(chr_dev_status != 0)
    {
        printk(KERN_ERR "simtemp - Error creating flags attribute\n");
        device_destroy(simtemp_class, dev_nr);
        return chr_dev_status;
    }
    chr_dev_status = device_create_file(simtemp_dev, &dev_attr_simtemp_sysfs_mode);
    if(chr_dev_status != 0)
    {
        printk(KERN_ERR "simtemp - Error creating mode attribute\n");
        device_destroy(simtemp_class, dev_nr);
        return chr_dev_status;
    }

    /* Define the delay time */
    simtemp_timer_period = ktime_set(0, simtemp_sysfs_sampling_time * 1000000);
    /* Initialize the hrtimer */
    hrtimer_init(&simtemp_sampling_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    /* Set the callback function */
    simtemp_sampling_timer.function = simtemp_timer_callback;
    /* Start the hrtimer */
    hrtimer_start(&simtemp_sampling_timer, simtemp_timer_period, HRTIMER_MODE_REL);

    return 0;
}



/* @brief Operation that unloads the module */
static void __exit simtemp_module_exit(void)
{
    /* Cancel the hrtimer if it's still active */
    hrtimer_cancel(&simtemp_sampling_timer);
    printk(KERN_INFO "simtemp module unloaded.\n");

    /* Release the objects used for sysfs interface */
    device_remove_file(simtemp_dev, &dev_attr_simtemp_sysfs_sampling_time);
    device_destroy(simtemp_class, dev_nr);
    class_destroy(simtemp_class);
    unregister_chrdev_region(dev_nr, MINORMASK + 1);
}



/* @brief Timer callback function */
enum hrtimer_restart simtemp_timer_callback(struct hrtimer *timer)
{
    /* timesatmp measurement */
    simtemp_timestamp_start = ktime_get();
    simtemp_sysfs_timestamp_ns = ktime_to_ns(ktime_sub(simtemp_timestamp_start, simtemp_timestamp_stop));
    simtemp_timestamp_stop = simtemp_timestamp_start;
    printk(KERN_INFO "The timestamp is: %llu\n", simtemp_sysfs_timestamp_ns);
    /* Get the temperature reading and the timestamp and store it into the binary record */
    simtemp_sysfs_temp_mC = simtemp_get_temperature();
    printk(KERN_INFO "The temperature is: %u\n", simtemp_sysfs_temp_mC);
    /* Check if the temperature has crossed the defined threshold */
    if(simtemp_sysfs_temp_mC > simtemp_sysfs_temperature_threshold)
    {
        printk(KERN_INFO "The temperature has crossed the define threshold");
        /* Set bit 1 of flags variable to 1 indicating that the threshold has been crossed */
        simtemp_sysfs_flags = simtemp_sysfs_flags | 0x2;
    }
    else
    {
        simtemp_sysfs_flags = simtemp_sysfs_flags & 0x1;
    }
    /* Restarting timer using the indicated time on simtemp_timer_period variable */
    simtemp_timer_period = ktime_set(0, simtemp_sysfs_sampling_time * 1000000);
    hrtimer_forward_now(timer, simtemp_timer_period);
    return HRTIMER_RESTART;
}



/* @brief This function simulates the process to obtain temperature samples */
__u32 simtemp_get_temperature(void)
{
    __u16 random_value;
    /* Ramp the temperature up until the threshold defined by UPPER_THRESHOLD_TEMP_SIMULATION_MILI_C is reached
     * Then ramp the temperature down until the threshold defined by LOWER_THRESHOLD_TEMP_SIMULATION_MILI_C is reached
    */
    if(simtemp_sysfs_mode == MODE_RAMP)
    {
        if(simtemp_temperature_sensor_reading >= UPPER_THRESHOLD_TEMP_SIMULATION_MILI_C)
        {
            simtemp_temperature_sensor_increment_flag = false;
        }
        else if(simtemp_temperature_sensor_reading <= LOWER_THRESHOLD_TEMP_SIMULATION_MILI_C)
        {
            simtemp_temperature_sensor_increment_flag = true;
        }
        else
        {
            /* Do Nothing */
        }

        if(simtemp_temperature_sensor_increment_flag == true)
        {
            simtemp_temperature_sensor_reading = simtemp_temperature_sensor_reading + TEMP_SIMULATION_INCREMENTS;
        }
        else
        {
            simtemp_temperature_sensor_reading = simtemp_temperature_sensor_reading - TEMP_SIMULATION_INCREMENTS;
        }
    }
    /* Generate a random number and add it to the temperature to simulate a noisy environment */
    else if (simtemp_sysfs_mode == MODE_NOISY)
    {
        get_random_bytes(&random_value, sizeof(random_value));
        simtemp_temperature_sensor_reading = NORMAL_TEMPERATURE_VALUE + (__u32)random_value;
    }
    else
    {
        /* A stable temperature reading will be returned */
        simtemp_temperature_sensor_reading = NORMAL_TEMPERATURE_VALUE;
    }

    return simtemp_temperature_sensor_reading;
}



/* @brief Show function for reading the contents of simtemp_sysfs_sampling_time */
ssize_t simtemp_sysfs_sampling_time_show(struct device *d, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%u", simtemp_sysfs_sampling_time);
}



/* @brief Define the store function for writing to simtemp_sysfs_sampling_time */
ssize_t simtemp_sysfs_sampling_time_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count)
{
    if(sscanf(buf, "%du", &simtemp_sysfs_sampling_time) != 1)
    {
        return -EINVAL;
    }
    return count;
}



/* @brief Show function for reading the contents of simtemp_sysfs_temperature_threshold */
ssize_t simtemp_sysfs_temperature_threshold_show(struct device *d, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%u", simtemp_sysfs_temperature_threshold);
}



/* @brief Define the store function for writing to simtemp_sysfs_temperature_threshold */
ssize_t simtemp_sysfs_temperature_threshold_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count)
{
    if(sscanf(buf, "%du", &simtemp_sysfs_temperature_threshold) != 1)
    {
        return -EINVAL;
    }
    return count;
}



/* @brief Show function for reading the contents of simtemp_sysfs_timestamp_ns */
ssize_t simtemp_sysfs_timestamp_show(struct device *d, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%llu", simtemp_sysfs_timestamp_ns);
}



/* @brief Define the store function for writing to simtemp_sysfs_timestamp_ns */
ssize_t simtemp_sysfs_timestamp_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count)
{
    if(sscanf(buf, "%lld", &simtemp_sysfs_timestamp_ns) != 1)
    {
        return -EINVAL;
    }
    return count;
}



/* @brief Show function for reading the contents of simtemp_sysfs_temp_mC */
ssize_t simtemp_sysfs_temp_mc_show(struct device *d, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%u", simtemp_sysfs_temp_mC);
}



/* @brief Define the store function for writing to simtemp_sysfs_temp_mC */
ssize_t simtemp_sysfs_temp_mc_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count)
{
    if(sscanf(buf, "%du", &simtemp_sysfs_temp_mC) != 1)
    {
        return -EINVAL;
    }
    return count;
}



/* @brief Show function for reading the contents of simtemp_sysfs_flags */
ssize_t simtemp_sysfs_flags_show(struct device *d, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%u", simtemp_sysfs_flags);
}



/* @brief Define the store function for writing to simtemp_sysfs_flags */
ssize_t simtemp_sysfs_flags_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count)
{
    if(sscanf(buf, "%du", &simtemp_sysfs_flags) != 1)
    {
        return -EINVAL;
    }
    return count;
}



/* @brief Show function for reading the contents of simtemp_sysfs_mode */
ssize_t simtemp_sysfs_mode_show(struct device *d, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%u", simtemp_sysfs_mode);
}



/* @brief Define the store function for writing to simtemp_sysfs_mode */
ssize_t simtemp_sysfs_mode_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count)
{
    if(sscanf(buf, "%du", &simtemp_sysfs_mode) != 1)
    {
        return -EINVAL;
    }
    return count;
}



/*****************************************************/
/**** Assign the module load and unload operations ***/
/*****************************************************/
module_init(simtemp_module_start);
module_exit(simtemp_module_exit);



/*****************************/
/**** Metadata information ***/
/*****************************/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alejandro Padilla");
MODULE_DESCRIPTION("Temperature sampling module");