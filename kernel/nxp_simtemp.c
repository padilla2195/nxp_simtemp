/**
 * @file nxp_simtemp.c
 * @brief This source file implements the NXP Systems Software Engineer Candidate Challenge.
 *        Goal: Build a small system that simulates a hardware sensor in the linux Kernel and exposes
 *        it to user space.
 * @author Enrique Alejandro Padilla Sanchez
 * @date 23/Oct/2025
 */

/******************/
/**** Includes ****/
/******************/
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/rtc.h>
#include <linux/random.h>
#include <linux/poll.h>
#include <linux/wait.h>



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
#define MAX_DEV                                    1U



/*****************************/
/**** Struct definitions *****/
/*****************************/
struct char_device_data {
    struct cdev cdev;
};



/****************************/
/**** Function prototypes ***/
/****************************/
/* Call-back functions */
static enum hrtimer_restart simtemp_timer_callback(struct hrtimer *timer);
static unsigned int simtemp_new_event_poll(struct file *file, poll_table *wait);
/* Temperature sensor functions */
static __u32 simtemp_get_temperature(void);
static void simtemp_get_timestamp(void);
/* Init and Exit module functions */
static int __init simtemp_module_start(void);
static void __exit simtemp_module_exit(void);
/* Sysfs functions */
static ssize_t simtemp_sysfs_sampling_time_show(struct device *d, struct device_attribute *attr, char *buf);
static ssize_t simtemp_sysfs_sampling_time_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t simtemp_sysfs_temperature_threshold_show(struct device *d, struct device_attribute *attr, char *buf);
static ssize_t simtemp_sysfs_temperature_threshold_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t simtemp_sysfs_timestamp_show(struct device *d, struct device_attribute *attr, char *buf);
static ssize_t simtemp_sysfs_timestamp_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t simtemp_sysfs_temp_mc_show(struct device *d, struct device_attribute *attr, char *buf);
static ssize_t simtemp_sysfs_temp_mc_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t simtemp_sysfs_flags_show(struct device *d, struct device_attribute *attr, char *buf);
static ssize_t simtemp_sysfs_flags_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t simtemp_sysfs_mode_show(struct device *d, struct device_attribute *attr, char *buf);
static ssize_t simtemp_sysfs_mode_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count);



/***************************************/
/**** Static variables definitions *****/
/***************************************/

/* hrtimer variables */
static struct hrtimer simtemp_sampling_timer;
static ktime_t simtemp_timer_period;
/* Temperature sensing variables */
static __s32 simtemp_temperature_sensor_reading = 32000U;
static bool  simtemp_temperature_sensor_increment_flag = true; // True:temperature is incremented, False:temperature is decremented
/* Character device variables */
static dev_t dev_nr;
/* Variables for sysfs */
static struct class *simtemp_class;
static struct device *simtemp_dev;
static __u32 simtemp_sysfs_sampling_time = 200; /* sampling time initialized to 200ms */
static __s32 simtemp_sysfs_temperature_threshold = 40000; /* Threshold in mC */
static char simtemp_sysfs_timestamp[100]; /* Timestamp */
static __u32 simtemp_sysfs_temp_mC; /* Measured temperature in mC */
static __u32 simtemp_sysfs_flags; /* Flags */
static __u32 simtemp_sysfs_mode = 0; /* Mode, possible values: 0 = Normal, 1 = Noisy, 2 = Ramp */
/* Variables for polling */
static wait_queue_head_t wait_queue_new_sampling_available;
static wait_queue_head_t wait_queue_thres_cross;
/* Character device data */
static struct char_device_data simtemp_char_dev_data;
/* File Operations */
static const struct file_operations chardev_fops = {
    .poll = simtemp_new_event_poll
};



/************************************/
/**** Device attribute asignation ***/
/************************************/
DEVICE_ATTR(simtemp_sysfs_sampling_time, 0660, simtemp_sysfs_sampling_time_show, simtemp_sysfs_sampling_time_store);
DEVICE_ATTR(simtemp_sysfs_temperature_threshold, 0660, simtemp_sysfs_temperature_threshold_show, simtemp_sysfs_temperature_threshold_store);
DEVICE_ATTR(simtemp_sysfs_timestamp, 0660, simtemp_sysfs_timestamp_show, simtemp_sysfs_timestamp_store);
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

    /* allocate chardev region and indicate the number of devices  */
    chr_dev_status = alloc_chrdev_region(&dev_nr, 0, MAX_DEV, "nxp_simtemp");
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
        unregister_chrdev_region(dev_nr, MAX_DEV);
        return chr_dev_status;
    }
    
    /* Init a new device */
    cdev_init(&simtemp_char_dev_data.cdev, &chardev_fops);

    /* Add device to the system */
    cdev_add(&simtemp_char_dev_data.cdev, MKDEV(MAJOR(dev_nr), 0), 1);

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
    chr_dev_status = device_create_file(simtemp_dev, &dev_attr_simtemp_sysfs_timestamp);
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

    /* Init the waitqueue */
    init_waitqueue_head(&wait_queue_new_sampling_available);
    init_waitqueue_head(&wait_queue_thres_cross);

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
    unregister_chrdev_region(dev_nr, MAX_DEV);
}



/* @brief Timer callback function */
static enum hrtimer_restart simtemp_timer_callback(struct hrtimer *timer)
{
    /* timesatmp measurement */
    simtemp_get_timestamp();
    printk(KERN_INFO "The timestamp is: %s\n", simtemp_sysfs_timestamp);
    /* Get the temperature reading and store it into simtemp_sysfs_temp_mC */
    simtemp_sysfs_temp_mC = simtemp_get_temperature();
    printk(KERN_INFO "The temperature is: %u\n", simtemp_sysfs_temp_mC);
    /* Notify that there is a new sample available by setting bit 0 */
    simtemp_sysfs_flags = simtemp_sysfs_flags | 0x1U;
    wake_up(&wait_queue_new_sampling_available);
    /* Check if the temperature has crossed the defined threshold */
    if(simtemp_sysfs_temp_mC > simtemp_sysfs_temperature_threshold)
    {
        printk(KERN_INFO "The temperature has crossed the define threshold");
        /* Set bit 1 of flags variable to 1 indicating that the threshold has been crossed */
        simtemp_sysfs_flags = simtemp_sysfs_flags | 0x2;
        /* Notify that the threshold has been crossed */
        wake_up(&wait_queue_thres_cross);
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



/* @brief Poll callback function for new sample or error event detection */
static unsigned int simtemp_new_event_poll(struct file *file, poll_table *wait)
{
    int ret_value = 0U;
    poll_wait(file, &wait_queue_new_sampling_available, wait);
    poll_wait(file, &wait_queue_thres_cross, wait);
    /* Check if an error has been detected */
    if(simtemp_sysfs_flags & 0x2U)
    {
        ret_value = ret_value | POLLPRI;
    }
    /* Check if there is a new sample available */
    if(simtemp_sysfs_flags & 0x1U)
    {
        /* Set the flag back to zero */
        simtemp_sysfs_flags = simtemp_sysfs_flags & 0x2U;
        ret_value = ret_value | POLLIN;
    }
    return ret_value;
}



/* @brief This function simulates the process to obtain temperature samples */
static __u32 simtemp_get_temperature(void)
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



static void simtemp_get_timestamp(void)
{
    ktime_t now_time;
    struct rtc_time tm;
    __u64 milliseconds;
    /*  Retrive the current real time of the system*/
    now_time = ktime_get_real();
    tm = rtc_ktime_to_tm(now_time);
    /* Get the corresponding millisecond values */
    milliseconds = ktime_to_ms(now_time) & 0x3E7;
    /* Store the information as string into simtemp_sysfs_timestamp */
    sprintf(simtemp_sysfs_timestamp, "%d-%d-%d, T%d:%d:%d:%lld", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, milliseconds);
}



/* @brief Show function for reading the contents of simtemp_sysfs_sampling_time */
static ssize_t simtemp_sysfs_sampling_time_show(struct device *d, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%u", simtemp_sysfs_sampling_time);
}



/* @brief Define the store function for writing to simtemp_sysfs_sampling_time */
static ssize_t simtemp_sysfs_sampling_time_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count)
{
    if(sscanf(buf, "%du", &simtemp_sysfs_sampling_time) != 1)
    {
        return -EINVAL;
    }
    return count;
}



/* @brief Show function for reading the contents of simtemp_sysfs_temperature_threshold */
static ssize_t simtemp_sysfs_temperature_threshold_show(struct device *d, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%u", simtemp_sysfs_temperature_threshold);
}



/* @brief Define the store function for writing to simtemp_sysfs_temperature_threshold */
static ssize_t simtemp_sysfs_temperature_threshold_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count)
{
    if(sscanf(buf, "%du", &simtemp_sysfs_temperature_threshold) != 1)
    {
        return -EINVAL;
    }
    return count;
}



/* @brief Show function for reading the contents of simtemp_sysfs_timestamp */
static ssize_t simtemp_sysfs_timestamp_show(struct device *d, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%s", simtemp_sysfs_timestamp);
}



/* @brief Define the store function for writing to simtemp_sysfs_timestamp */
static ssize_t simtemp_sysfs_timestamp_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count)
{
    if(sscanf(buf, "%s", simtemp_sysfs_timestamp) != 1)
    {
        return -EINVAL;
    }
    return count;
}



/* @brief Show function for reading the contents of simtemp_sysfs_temp_mC */
static ssize_t simtemp_sysfs_temp_mc_show(struct device *d, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%u", simtemp_sysfs_temp_mC);
}



/* @brief Define the store function for writing to simtemp_sysfs_temp_mC */
static ssize_t simtemp_sysfs_temp_mc_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count)
{
    if(sscanf(buf, "%du", &simtemp_sysfs_temp_mC) != 1)
    {
        return -EINVAL;
    }
    return count;
}



/* @brief Show function for reading the contents of simtemp_sysfs_flags */
static ssize_t simtemp_sysfs_flags_show(struct device *d, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%u", simtemp_sysfs_flags);
}



/* @brief Define the store function for writing to simtemp_sysfs_flags */
static ssize_t simtemp_sysfs_flags_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count)
{
    if(sscanf(buf, "%du", &simtemp_sysfs_flags) != 1)
    {
        return -EINVAL;
    }
    return count;
}



/* @brief Show function for reading the contents of simtemp_sysfs_mode */
static ssize_t simtemp_sysfs_mode_show(struct device *d, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%u", simtemp_sysfs_mode);
}



/* @brief Define the store function for writing to simtemp_sysfs_mode */
static ssize_t simtemp_sysfs_mode_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count)
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