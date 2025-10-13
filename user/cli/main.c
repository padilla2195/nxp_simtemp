#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <poll.h>



/* Possible Menu Options */
#define MENU_CONF_PERIOD               1U
#define MENU_CONF_THRES                2U
#define MENU_CONF_MODE                 3U
#define MENU_READ_TEMP                 4U
#define MENU_READ_TEMP_SEVERAL_RECORDS 5U
#define MENU_EXIT                      7U



#ifdef _WIN32
#define CLEAN_SCREEN system("cls")
#else
#define CLEAN_SCREEN system("clear")
#endif

/* Global Variables */
int sampling_time = 200;      /* Temperature sampling time, by default it is set to 200ms */
int period_counter = 0;       /* Variable to count the periods to get the temperature and the alerts */
int deviceFile;               /* Varible that holds the result of opening the device file */
char sysfs_read_buffer[256];  /* Variable that holds the returned information from a sysfs read operation */


int print_menu(void)
{
    int option = 0;
    printf("NXP Simtemp Application \n");
    printf("1. Configure sampling period \n");
    printf("2. Configure threshold \n");
    printf("3. Configure Mode (Normal, Noisy, Ramp) \n");
    printf("4. Read temperature and timestamp \n");
    printf("5. Read temperature and timestamp (Several Records) \n");
    printf("6. Test mode \n");
    printf("7. Exit \n");
    printf("Choose an option: ");
    scanf("%d", &option);
    return option;
}



void clean_buffer()
{
    char c;
    while ((c = getchar()) != EOF && c != '\n');
} /* This will clean the buffer */



void stop_system()
{
    clean_buffer();
    printf("Press Enter to continue...");
    getchar();
} /* This will wait for the user to press Enter, similar to system("pause") */



void write_sysfs_file_integer_value(char * sysfs_path, char * message_input)
{
    int fd;
    int value;
    int data_len;
    char buffer[32];

    /* Request the data to be written to the sysfs file */
    printf("%s", message_input);
    scanf("%d", &value);

    /* Open the sysfs file */
    fd = open(sysfs_path, O_WRONLY);
    if(fd == -1)
    {
        printf("Error Opening the Sysfs file \n");
    }
    else
    {
        /* Convert integer value to string */
        data_len = snprintf(buffer, sizeof(buffer), "%d\n", value);
        if (data_len < 0)
        {
            printf("Error Converting the inpunt integer value to string \n");
            close(fd);
        }
        else
        {
            /* Write the string to the sysfs file */
            if(write(fd, buffer, data_len) == -1)
            {
                close(fd);
                printf("Error writing to the sysfs file \n");
            }
            else
            {
                printf("The value has been written succesfully! \n");
            }
        }
    }

    close(fd);
}



void read_sysfs_file(char * sysfs_path, char * printMessage)
{
    FILE *fd = NULL;
    fd = fopen(sysfs_path, "r");
    if(fd != NULL)
    {
        if(fgets(sysfs_read_buffer, sizeof(sysfs_read_buffer), fd) != NULL)
        {
            printf("%s: %s\n", printMessage, sysfs_read_buffer);
            fclose(fd);
        }
        else
        {
            printf("Error reading the sysfs file \n");
        }
    }
    else
    {
        printf("Error opening the sysfs file \n");
    }
}



void read_temperature_and_timestamp()
{
    /* Poll variables */
    struct pollfd my_poll;
    
    /* Support variables for printing sysfs variables */
    int threshold_crossed;

    /* Poll initialization */
    memset(&my_poll, 0, sizeof(my_poll));
    my_poll.fd = deviceFile;
    my_poll.events = POLLIN;
    period_counter = 0;

    while(1)
    {
        poll(&my_poll, 1, sampling_time);
        if(my_poll.revents & POLLIN)
        {
            /* Temperature */
            read_sysfs_file("/sys/class/simtemp_class/simtemp_dev0/simtemp_sysfs_temp_mC", "Temperature reading in mC");
            
            /* Sampling time */
            read_sysfs_file("/sys/class/simtemp_class/simtemp_dev0/simtemp_sysfs_sampling_time", "Sampling Time");
            sampling_time = atoi(sysfs_read_buffer);
            printf("It took %d ms to get the temperature. \n", sampling_time * period_counter);
            
            /* TimeStamp */
            read_sysfs_file("/sys/class/simtemp_class/simtemp_dev0/simtemp_sysfs_timestamp", "Time Stamp");

            /* Threshold */
            read_sysfs_file("/sys/class/simtemp_class/simtemp_dev0/simtemp_sysfs_temperature_threshold", "Threshold");

            /* Flags */
            read_sysfs_file("/sys/class/simtemp_class/simtemp_dev0/simtemp_sysfs_flags", "Flag Variable Value");
            threshold_crossed = (int)sysfs_read_buffer[0];
            if(threshold_crossed & 0x02)
            {
                printf("ALERT: threshold has been crossed!\n");
            }
            else
            {
                printf("Temperature Ok!\n");
            }

            printf("\n \n");

            break;
        }
        else
        {
            period_counter++;
        }
    }
}



void read_temperature_and_timestamp_print_several_records()
{
    int number_of_records_to_print = 0;
    printf("How Many Records would you like to print?: ");
    scanf("%d", &number_of_records_to_print);
    while(number_of_records_to_print > 0U)
    {
        read_temperature_and_timestamp();
        number_of_records_to_print--;
    }
}



int main()
{
    /* Variable to interact with the menu */
    int selectedOption = 0;

    /* Open the Device file*/
    deviceFile = open("/dev/simtemp_dev0", O_RDONLY);
    if(deviceFile < 0)
    {
        perror("Could not open the device file \n");
    }
    /* Print the menu only if the device was able to be opened */
    else
    {
        while(selectedOption != MENU_EXIT)
        {
            selectedOption = print_menu();

            switch(selectedOption)
            {
                case MENU_CONF_PERIOD:
                    write_sysfs_file_integer_value("/sys/class/simtemp_class/simtemp_dev0/simtemp_sysfs_sampling_time", "Enter the Sampling time in ms:  ");
                    break;

                case MENU_CONF_THRES:
                    write_sysfs_file_integer_value("/sys/class/simtemp_class/simtemp_dev0/simtemp_sysfs_temperature_threshold", "Enter the threshold in mC:  ");  
                    break;

                case MENU_CONF_MODE:
                    write_sysfs_file_integer_value("/sys/class/simtemp_class/simtemp_dev0/simtemp_sysfs_mode", "Enter the temperature mode, 0-Normal, 1-Noisy, 2-Ramp:  ");
                    break;

                case MENU_READ_TEMP:
                    read_temperature_and_timestamp();
                    break;

                case MENU_READ_TEMP_SEVERAL_RECORDS:
                    read_temperature_and_timestamp_print_several_records();
                    break;

                case MENU_EXIT:
                    printf("GoodBye! \n");
                    break;
                default:
                    printf("An incorrect option was selected. Try again...\n");
                    break;
            }
            if(selectedOption != MENU_EXIT)
            {
                stop_system();
                CLEAN_SCREEN;
            }
        }
    }

    /* Close the device file */
    close(deviceFile);

    /* Return 0 */
    return 0;
}