//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	DOOM keyboard input via linux tty
//


#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "config.h"
#include "deh_str.h"
#include "doomtype.h"
#include "doomkeys.h"
#include "i_joystick.h"
#include "i_system.h"
#include "i_swap.h"
#include "i_timer.h"
#include "i_video.h"
#include "i_scale.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"
#include "tables.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

int vanilla_keyboard_mapping = 1;

// Is the shift key currently down?

static int shiftdown = 0;

// Lookup table for mapping goggles x buttons to their doom keycode
static const char goggles_to_doom[] =
{
    KEY_UPARROW,
    KEY_DOWNARROW,
    KEY_LEFTARROW,
    KEY_RIGHTARROW,
    KEY_FIRE,
    0x0,
    KEY_ENTER,
    KEY_USE,
};

static const int voltage_to_button[] =
{
    800,
    1600,
    1000,
    1200,
    1400,
    1800,
    400,
    600,
};

static int old_mode = -1;
static struct termios old_term;
static int adc_fd = -1;
static const void *ADC_BASE = 0x60632000;
static void *adc_memory = NULL;
static int adc_pressed_btn = -1;

uint32_t adc_readreg(uint32_t offset)
{
    return *(uint32_t*)(adc_memory + offset);
}

void adc_writereg(uint32_t offset, uint32_t value)
{
    *(uint32_t*)(adc_memory + offset) = value;
}

void btn_shutdown(void)
{
    /* Shut down nicely. */

    printf("Cleaning up.\n");
    fflush(stdout);

    printf("Exiting normally.\n");

    munmap(adc_memory, 0x1000);
    close(adc_fd);

    exit(0);
}

static int btn_init(void)
{
    printf("adc init\n");
    adc_fd = open("/dev/mem", O_RDWR);
    if(adc_fd == -1) {
        fprintf(stderr, "Failed to open /dev/mem\n");
	exit(1);
    }
    adc_memory = mmap(0, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, adc_fd, ADC_BASE);
    if(adc_memory == NULL) {
        fprintf(stderr, "Failed to mmap /dev/mem\n");
	exit(1);
    }
    return 0;
}

int btn_read(int *pressed, unsigned char *key)
{
    if(adc_memory == NULL)
        return 0; // adc not initialized yet

    uint32_t v4 = adc_readreg(0x18);
    if(v4) {
        adc_writereg(0x18, v4 & ~1);
        adc_writereg(0x800, v4 & ~1);
    }
    adc_writereg(0x40, 0x8C);
    if(adc_readreg(0x38) != 6)
        adc_writereg(0x38, 6);
    
    int val = adc_readreg(0x810);
    int voltage = val * 1800 / 1023;

    for(int i = 0; i < 8; i++) {
        if(voltage > voltage_to_button[i] - 100 && voltage < voltage_to_button[i] + 100) {
	    if(adc_pressed_btn == -1) {
		//printf("pressed %d\n", i);
                *key = i;
	        *pressed = 1;
	        adc_pressed_btn = i;
	        return 1;
	    } else if(adc_pressed_btn != i) {
                *key = adc_pressed_btn;
		*pressed = 0;
		adc_pressed_btn = -1;
		//printf("released1 %d\n", *key);
		return 1;
	    } else {
		return 0;
	    }
	}
    }

    if(adc_pressed_btn != -1) {
        *key = adc_pressed_btn;
	*pressed = 0;
	adc_pressed_btn = -1;
	//printf("released2 %d\n", *key);
	return 1;
    }

    return 0;
}

static unsigned char TranslateKey(unsigned char key)
{
    if (key < sizeof(goggles_to_doom))
        return goggles_to_doom[key];
    else
        return 0x0;

    //default:
    //  return tolower(key);
}

void I_GetEvent(void)
{
    event_t event;
    int pressed;
    unsigned char key;

    // put event-grabbing stuff in here
    
    while (btn_read(&pressed, &key))
    {
        // process event
        
        if (pressed)
        {
	    //printf("keydown %d\n", key);
            // data1 has the key pressed, data2 has the character
            // (shift-translated, etc)
            event.type = ev_keydown;
            event.data1 = TranslateKey(key);
            event.data2 = 0;

            if (event.data1 != 0)
            {
                D_PostEvent(&event);
            }
        }
        else
        {
	    //printf("keyup %d\n", key);
            event.type = ev_keyup;
            event.data1 = TranslateKey(key);

            // data2 is just initialized to zero for ev_keyup.
            // For ev_keydown it's the shifted Unicode character
            // that was typed, but if something wants to detect
            // key releases it should do so based on data1
            // (key ID), not the printable char.

            event.data2 = 0;

            if (event.data1 != 0)
            {
                D_PostEvent(&event);
            }
            break;
        }
    }


                /*
            case SDL_MOUSEMOTION:
                event.type = ev_mouse;
                event.data1 = mouse_button_state;
                event.data2 = AccelerateMouse(sdlevent.motion.xrel);
                event.data3 = -AccelerateMouse(sdlevent.motion.yrel);
                D_PostEvent(&event);
                break;
                */
}

void I_InitInput(void)
{
    btn_init();

    //UpdateFocus();
}

