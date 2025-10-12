#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <poll.h>

#define MENU_EXIT 5U
#define MENU_READ_TEMP 3U

#ifdef _WIN32
#define CLEAN_SCREEN system("cls")
#else
#define CLEAN_SCREEN system("clear")
#endif

/* Global Variables */
int sampling_time = 200; /* Temperature sampling time, by default it is set to 200ms */
int period_counter = 0;  /* Variable to count the periods to get the temperature and the alerts */


int print_menu(void)
{
    int option = 0;
    printf("NXP Simtemp Application \n");
    printf("1. Configure sampling period \n");
    printf("2. Configure threshold \n");
    printf("3. Read temperature and timestamp \n");
    printf("4. Test mode \n");
    printf("5. Exit \n");
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


void read_temperature_and_timestamp()
{
    int deviceFile;
    struct pollfd my_poll;
    FILE *fileOpenTemp = NULL;
    char buffer[256];

    /* Open the device file */
    deviceFile = open("/dev/simtemp_dev0", O_RDONLY);
    if(deviceFile < 0)
    {
        perror("Could not open the device file \n");
        return -1;
    }

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
            fileOpenTemp = fopen("/sys/class/simtemp_class/simtemp_dev0/simtemp_sysfs_temp_mC", "r");
            if(fileOpenTemp != NULL)
            {
                if(fgets(buffer, sizeof(buffer), fileOpenTemp) != NULL)
                {
                    printf("Temperature reading in mC: %s \n", buffer);
                    printf("It took %d ms to get the temperature. The sampling time temperature is: %d \n", sampling_time * period_counter, sampling_time);
                    fclose(fileOpenTemp);
                }
                else
                {
                    printf("Error reading the temperature \n");
                }
            }
            else
            {
                printf("Error reading the temperature \n");
            }
            break;
        }
        else
        {
            period_counter++;
        }
    }
}

int main()
{
    /* Variable to interact with the menu */
    int selectedOption = 0;

    /* print the menu */
    while(selectedOption != MENU_EXIT)
    {
        selectedOption = print_menu();

        switch(selectedOption)
        {
            case MENU_READ_TEMP:
                read_temperature_and_timestamp();
                break;

            case MENU_EXIT:
                printf("GoodBye! \n");
                break;
        }
        if(selectedOption != MENU_EXIT);
        {
            stop_system();
            CLEAN_SCREEN;
        }
    }
    return 0;
}