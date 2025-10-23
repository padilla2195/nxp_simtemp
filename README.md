NXP Systems Software Engineer Candidate Challenge

GOAL
Build a small system that simulates a temperature sensor in the Linux Kernel with a user app to configure/read it.

SUMMARY and GOALS
* Kernel module
* User-Kernel communication: Character device ith read() + poll/epoll configuration via sysfs and or ioctl
* Device Tree (DT)
* User space app (python or C++)
* Git hygiene
* Design quality

DEMO VIDEO
The link to the live video demo can be found at docs/NXP_SIMTEMP_DEMO.mp4

BUILD KERNEL DEVICE AND USER APP
To build the the kernel module and the user application, go to /scripts folder and execute the build.sh script.

RUN DEMO
To insmod the Kernel device and execute the user application, go to /scripts folder and execute the run_demo.sh script. The script will insmod the Kenrel module and execute the user application. Once user application is exit the Kernel module is removed.

BUILD AND RUN DEMO
The script /scripts/build_and_run_demo.sh combines the build process and application execution in one single script.

USER APPLICATION USAGE
Once the user application is loaded, you will be requested to select an option from the following menu:
1. Configure sampling period --> Change the sampling period (by default the period is set to 200ms)
2. Configure threshold --> Change the temperature threshold (by default the threshold is set to 40°C. If the threshold is crossed a notification is raised)
3. Configure Mode (Normal, Noisy, Ramp) --> Change the sampling mode (Normal --> A fixed temperature value is reported, Noisy --> Random temperature values are reported, Ramp --> The temperature is incremented until certain threshold and then is decremented)
4. Read temperature and timestamp --> Get one temperature sample
5. Read temperature and timestamp (Several Records) --> User indicates how many samples need to reported
6. Test mode --> The sampling mode is set to normal. The temperature threshold is set to a value below the temperature and the application waits until an alert is reported. The application reports the time that took to detect the fault.
7. Exit --> Exit the application. Kernel module is removed.