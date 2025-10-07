/******************/
/**** Includes ****/
/******************/
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>



/****************************/
/**** Macro definitions *****/
/****************************/
#define UPPER_THRESHOLD_TEMP_SIMULATION_MILI_C 50000U
#define LOWER_THRESHOLD_TEMP_SIMULATION_MILI_C 20000U
#define TEMP_SIMULATION_INCREMENTS 10U



/********************************/
/**** Data Types Definition *****/
/********************************/
typedef struct simtemp_sample {
    __u64 timestamp_ns;
    __u32 temp_mC;
    __u32 flags;
} simtemp_sample_type;



/****************************/
/**** Function prototypes ***/
/****************************/
enum hrtimer_restart simtemp_timer_callback(struct hrtimer *timer);
__u32 simtemp_get_temperature(void);
static ssize_t simtemp_read_operation(struct file *f, char __user *u, size_t l, loff_t *o);
static int __init simtemp_module_start(void);
static void __exit simtemp_module_exit(void);



/***************************************/
/**** Static variables definitions *****/
/***************************************/
/* Timer Variables */
static struct hrtimer simtemp_sampling_timer;
static ktime_t simtemp_timer_period;
/* Temperature sensing variables */
static __s32 simtemp_temperature_sensor_reading = 32000U;
static bool  simtemp_temperature_sensor_increment_flag = true; // True:temperature is incremented, False:temperature is decremented
/* binary record */
static simtemp_sample_type simtemp_binary_record;
/* Variables for timestamp measurements */
static ktime_t simtemp_timestamp_start;
static ktime_t simtemp_timestamp_stop;
/* Character device variables */
static int simtemp_major_char_dev;
static struct file_operations simtemp_fops = {
    .read = simtemp_read_operation
};



/****************************/
/**** Function defintions ***/
/****************************/
/* Read operation for file operations */
static ssize_t simtemp_read_operation(struct file *f, char __user *u, size_t l, loff_t *o)
{
    printk(KERN_INFO "Read operation");
    return 0;
}



/* Module Start */
static int __init simtemp_module_start(void)
{
    printk(KERN_INFO "Initializing simtemp module.\n");

    /* Register character device, use a free device number  */
    simtemp_major_char_dev = register_chrdev(0, "simtemp", &simtemp_fops);
    if (simtemp_major_char_dev < 0)
    {
        printk(KERN_ERR "simtemp - Error registering chrdev\n");
        return simtemp_major_char_dev;
    }
    else
    {
        printk(KERN_ERR "simtemp - simtemp_Major_char_dev Device Number: %d\n", simtemp_major_char_dev);
    }

    /* Define the delay time (e.g., 5 seconds) */
    simtemp_timer_period = ktime_set(5, 0);
    /* Initialize the hrtimer */
    hrtimer_init(&simtemp_sampling_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    /* Set the callback function */
    simtemp_sampling_timer.function = simtemp_timer_callback;
    /* Start the hrtimer */
    hrtimer_start(&simtemp_sampling_timer, simtemp_timer_period, HRTIMER_MODE_REL);

    return 0;
}



/* Module Exit */
static void __exit simtemp_module_exit(void)
{
    /* Unregister the character device */
    unregister_chrdev(simtemp_major_char_dev, "simtemp");

    /* Cancel the hrtimer if it's still active */
    hrtimer_cancel(&simtemp_sampling_timer);
    printk(KERN_INFO "simtemp module unloaded.\n");
}



/* Timer callback function */
enum hrtimer_restart simtemp_timer_callback(struct hrtimer *timer)
{
    /* timesatmp measurement */
    simtemp_timestamp_start = ktime_get();
    simtemp_binary_record.timestamp_ns = ktime_to_ns(ktime_sub(simtemp_timestamp_start, simtemp_timestamp_stop));
    simtemp_timestamp_stop = simtemp_timestamp_start;
    printk(KERN_INFO "The timestamp is: %llu\n", simtemp_binary_record.timestamp_ns);
    /* Get the temperature reading and the timestamp and store it into the binary record */
    simtemp_binary_record.temp_mC = simtemp_get_temperature();
    printk(KERN_INFO "The temperature is: %u\n", simtemp_binary_record.temp_mC);
    /* Restarting timer using the indicated time on simtemp_timer_period variable */
    hrtimer_forward_now(timer, simtemp_timer_period);
    return HRTIMER_RESTART;
}



/* This function simulates the process to obtain temperature samples */
__u32 simtemp_get_temperature(void)
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

    return simtemp_temperature_sensor_reading;
}



module_init(simtemp_module_start);
module_exit(simtemp_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alejandro Padilla");
MODULE_DESCRIPTION("Temperature sampling module");