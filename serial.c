#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "serial.h"

#define MAX_SETTING_NAME_STR_LEN 15

unsigned int tty_baud_to_value(speed_t speed)
{
    int i = 0;

    do
    {
        if (speed == speeds[i].speed)
        {
            if (speeds[i].value & 0x8000u)
            {
                return ((unsigned)(speeds[i].value) & 0x7fffU) * 200;
            }
            return speeds[i].value;
        }
    } while (++i < NUM_SPEEDS);

    return 0;
}

const char *nth_string(const char *strings, int n)
{
    while (n)
    {
        if (*strings++ == '\0')
        {
            if (*strings == '\0') /* reached end of strings */
                break;
            n--;
        }
    }
    return strings;
}

static tcflag_t *get_ptr_to_tcflag(unsigned type, const struct termios *mode)
{
    static const uint8_t tcflag_offsets[] ALIGN1 = {
        offsetof(struct termios, c_cflag), /* control */
        offsetof(struct termios, c_iflag), /* input */
        offsetof(struct termios, c_oflag), /* output */
        offsetof(struct termios, c_lflag)  /* local */
    };
    if (type <= local)
    {
        return (tcflag_t *)(((char *)mode) + tcflag_offsets[type]);
    }
    return NULL;
}

/**
*@fn get_tl_settings
*@brief Get terminal line settings with particular device name
*@param mode struct termios
*@param all flag to display all 
*@param tl_settings buffer of terminal settings
*@return Returns '0' on success,
*        Returns '1' on failure
*/
static int get_tl_settings(const struct termios *mode, int all, char tl_settings[][MAX_SETTING_NAME_STR_LEN])
{
    int i, num = 0;
    tcflag_t *bitsp;
    unsigned long mask;
    int prev_type = control;
    char setting[MAX_SETTING_NAME_STR_LEN] = {0};

    for (i = 0; i < NUM_mode_info; ++i)
    {
        if (mode_info[i].flags & OMIT)
            continue;
        if (mode_info[i].type != prev_type)
        {
            prev_type = mode_info[i].type;
        }

        bitsp = get_ptr_to_tcflag(mode_info[i].type, mode);
        mask = mode_info[i].mask ? mode_info[i].mask : mode_info[i].bits;
        if ((*bitsp & mask) == mode_info[i].bits)
        {
            if (all || (mode_info[i].flags & SANE_UNSET))
            {
                strncpy(tl_settings[num++], nth_string(mode_name, i), MAX_SETTING_NAME_STR_LEN - 1);
            }
        }
        else
        {
            if ((all && mode_info[i].flags & REV) || (!all && (mode_info[i].flags & (SANE_SET | REV)) == (SANE_SET | REV)))
            {
                snprintf(setting, MAX_SETTING_NAME_STR_LEN - 1, "-%s", nth_string(mode_name, i));
                strncpy(tl_settings[num++], setting, MAX_SETTING_NAME_STR_LEN - 1);
            }
        }
    }

    return EXIT_SUCCESS;
}

/**
*@fn get_speed_baud
*@brief Get terminal line speed with particular device name
*@param mode struct termios
*@param ispeed_p input speed pointer
*@param ospeed_p output speed pointer
*@return Returns '0' on success,
*        Returns '1' on failure
*/
static int get_speed_baud(const struct termios *mode, unsigned int *ispeed_p, unsigned int *ospeed_p)
{
    unsigned long ispeed, ospeed;

    ispeed = cfgetispeed(mode);
    ospeed = cfgetospeed(mode);
    if (ispeed == 0 || ispeed == ospeed)
    {
        ispeed = ospeed; /* in case ispeed was 0 */
    }
    *ispeed_p = tty_baud_to_value(ispeed);
    *ospeed_p = tty_baud_to_value(ospeed);

    return EXIT_SUCCESS;
}

// For testing
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf(" [Input Error]\n");
        exit(1);
    }

    char dev_tty[20] = {'\0'};
    strncpy(dev_tty, argv[1], 20 - 1);

    char tl_settings[NUM_mode_info][MAX_SETTING_NAME_STR_LEN] = {0};

    struct termios mode;
    memset(&mode, 0, sizeof(mode));
    int fd = open(dev_tty, O_RDWR);
    if (fd == -1)
    {
        printf(" Error in open %s\n", dev_tty);
        return 0;
    }
    if (tcgetattr(fd, &mode))
    {
        printf(" Error in calling tcgetattr for %s\n", dev_tty);
        return 0;
    }

    // Get Terminal Settings
    int ret = get_tl_settings(&mode, 1, tl_settings);
    if (ret == 0)
    {
        for (int i = 0; i < NUM_mode_info; i++)
        {
            if (strnlen(tl_settings[i], MAX_SETTING_NAME_STR_LEN) != 0)
            {
                printf(" %s ",tl_settings[i]);
            }
        }
    }
    
    print("\n");

    // Get speed baud
    unsigned int ispeed, ospeed = 0;
    if (get_speed_baud(&mode, &ispeed, &ospeed) == 0)
    {
        printf(" ispeed = %d, ospeed = %d\n", ispeed, ospeed);
    }

    return 0;
}
