LINUX KERNEL API DEFINITION

------------------------------------------------------------------------------------

Function Prototype: static int __init simtemp_module_start(void)

Brief Description: This function initializes the simtemp kernel device

This function performs the following actions:

1) Allocate a chardev region and assign a number to the device.
2) Initialize simtemp class for sysfs.
3) Initialize the character device.
4) Add attributes to the device.
5) Initialize the wait queue.
6) Set-up hrtimer for temperature sensing.
 
Return value: An error value is returned in case any of the initialization steps fail.

------------------------------------------------------------------------------------

Function Prototype: static void __exit simtemp_module_exit(void)

Brief Description: This function unloads the simtemp kernel device

This function performs the following actions:

1) Cancel the hrtimer if it is still active.
2) Release the objects used for sysfs interface.
 
Return value: void

------------------------------------------------------------------------------------

Function Prototype: static enum hrtimer_restart simtemp_timer_callback(struct hrtimer *timer)

Brief Description: Callback funtion that is executed when the timer value configured by 
simtemp_timer_period is elapsed.

This function performs the following actions:

1) Capture timestamp information.
2) Read temperature sensor value.
3) Notify that a new temperature sample is vailable via poll event.
4) Check if the temperature sensor value has crossed the defined error threshold. If yes
raise a notification via poll event.
5) Re-start the timer value.
 
Return value: HRTIMER_RESTART

------------------------------------------------------------------------------------

Function Prototype: static unsigned int simtemp_new_event_poll(struct file *file, poll_table *wait)

Brief Description: Callback funtion that is executed when a poll event is raised.

This function performs the following actions:

1) Check if the temperature value has crossed the error threshold, if yes, the
return value is or-ed with POLLPRI.
2) Check if there is a new temperature sample available, if yes, the return value
is or/ed with POLLIN. 
 
Return value: 0 - If no events are detected, POLLPRI - If temperature value crossed
the defined error threshold, POLLIN - If a new temperature sensor is available, 
POLLPRI | POLLIN is error threshold is crossed and new sample is available.

------------------------------------------------------------------------------------

Function Prototype: static __u32 simtemp_get_temperature(void)

Brief Description: This function simulates the process of getting the temperature
value from a sensor.

This function performs the following actions:

1) If the value of simtemp_sysfs_mode is MODE_RAMP, the temperature value is incremented
TEMP_SIMULATION_INCREMENTS on every function call until the threshold defined by UPPER_THRESHOLD_TEMP_SIMULATION_MILI_C is reached.
Once the upper threshold is reached the temperature is then decremented until the threshold
defined by LOWER_THRESHOLD_TEMP_SIMULATION_MILI_C is reached.
2) If the value of simtemp_sysfs_mode is MODE_NOISY, a random temperature value is returned.
3) Otherwise a fixed value is returned. See NORMAL_TEMPERATURE_VALUE.
 
Return value: Simulated temperature sensor value.

------------------------------------------------------------------------------------

Function Prototype: static void simtemp_get_timestamp(void)

Brief Description: This function obtains the timestamp and stores it into simtemp_sysfs_timestamp.
 
Return value: void

------------------------------------------------------------------------------------




LINUX KERNEL SYSFS GET AND SET FUNCTIONS

------------------------------------------------------------------------------------

Variable Name: simtemp_sysfs_sampling_time

Variable Description: Variable holds the sampling time information in ms.

Get Function: static ssize_t simtemp_sysfs_sampling_time_show(struct device *d, struct device_attribute *attr, char *buf)

Set FUnction: static ssize_t simtemp_sysfs_sampling_time_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count)

------------------------------------------------------------------------------------

Variable Name: simtemp_sysfs_temperature_threshold

Variable Description: Variable holds the temperature error threshold in mC.

Get Function: static ssize_t simtemp_sysfs_temperature_threshold_show(struct device *d, struct device_attribute *attr, char *buf)

Set FUnction: static ssize_t simtemp_sysfs_temperature_threshold_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count)

------------------------------------------------------------------------------------

Variable Name: simtemp_sysfs_timestamp

Variable Description: Variable holds the timestamp information captured along with each temperature sensor value.
The information is formatted as follows: "YYYY-MM-DD, THH:MM:SS:mmmm".

Get Function: static ssize_t simtemp_sysfs_timestamp_show(struct device *d, struct device_attribute *attr, char *buf)

Set FUnction: static ssize_t simtemp_sysfs_timestamp_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count)

------------------------------------------------------------------------------------

Variable Name: simtemp_sysfs_temp_mC

Variable Description: Variable holds the temperature value returned by simtemp_get_temperature().

Get Function: static ssize_t simtemp_sysfs_temp_mc_show(struct device *d, struct device_attribute *attr, char *buf)

Set FUnction: static ssize_t simtemp_sysfs_temp_mc_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count)

------------------------------------------------------------------------------------

Variable Name: simtemp_sysfs_mode

Variable Function: Variable used to determine the sampling mode. Possible values: 0 - Normal, 1 - Noisy, 2 - Ramp.

Get Function: static ssize_t simtemp_sysfs_mode_show(struct device *d, struct device_attribute *attr, char *buf)

Set FUnction: static ssize_t simtemp_sysfs_mode_store(struct device *d, struct device_attribute *attr, const char *buf, size_t count)

------------------------------------------------------------------------------------




LINUX KERNEL VARIABLES

------------------------------------------------------------------------------------

Variable prototype: static struct hrtimer simtemp_sampling_timer

Variable Description: Variable used to set-up the hrtimer for the temperature sampling period.

------------------------------------------------------------------------------------

Variable prototype: static ktime_t simtemp_timer_period

Variable Description: Variable that holds the timer period for the temperature sampling.

------------------------------------------------------------------------------------

Variable prototype: static __s32 simtemp_temperature_sensor_reading

Variable Description: Variable that holds the temperature sensor reading.

------------------------------------------------------------------------------------

Variable prototype: static bool  simtemp_temperature_sensor_increment_flag

Variable Description: Boolean variable that indicates if the temperature reading is
incremented or decremented when ramp sampling mode is elected.

------------------------------------------------------------------------------------

Variable prototype: static dev_t dev_nr;

Variable Description: Variable for device identification.