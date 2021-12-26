/*
 * dellfan - user space utility to control the fan speed on Dell Laptops.
 *
 * SMM Management code from i8k. See file drivers/char/i8k.c at Linux kernel.
 *
 * Copyright (C) 2001  Massimo Dal Zotto <dz@debian.org>
 * Copyright (C) 2014  Carlos Alberto Lopez Perez <clopez@igalia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/io.h>

/* This is some information about the codes for different functions on the Dell SMBIOS */

/*
  http://brouille.tuxfamily.org/junk/fan.c
  cmd     function  (thanks to Andrew Paprocki)
  0x00a3  get current speed indicator of a fan (args: fan)
  0x01a3  set speed of a fan (args: fan & speed)
  0x02a3  get RPM of a fan (args: fan)
  0x03a3  ??? (1 byte)
  0x04a3  get nominal fan speed (2 args)
  0x05a3  get fan tolerance speed (2 args)
  0x10a3  get sensor temperature (1 arg: sensor#)
  0x11a3  ???
  0x12a3  arg 0x0003=NBSVC-Query
          arg 0x0000=NBSVC-Clear
          arg 0x122=NBSVC-Start Trend
          arg 0x0100=NBSVC-Stop Trend
          arg 0x02??=NBSVC-Read
  0x21a3  ??? (2 args: 1 byte (oder 0x16) + 1 byte)
  0x22a3  get charger info (1 arg)
  0x23a3  ??? (4 args: 2x 1 byte, 1xword, 1xdword)
  0x24a3  get adaptor info status (1 arg oder 0x03)
  0x30a3  ??? (no args)
  0x31a3  ??? (no args)
  0x32a3  ??? (no args)
  0x33a3  ??? (no args)
  0x36a3  get hotkey scancode list (args see diags)
  0x37a3  ??? (no args)
  0x40a3  get docking state (no args)
  0xf0a3  ??? (2 args)
  0xfea3  check SMBIOS version (1 arg)
  0xffa3  check SMBIOS interface (returns:"DELLDIAG")
*/

/* The codes for DISABLE_BIOS_* were obtained experimentally on a E6420 with the
 * following algorithm:
 * probing many codes in a loop
 * putting the speed to maximum, sleeping some seconds, and checking the speed back.
 * Check the function probecodes()
 */
#define DISABLE_BIOS_METHOD1 0x30a3
#define ENABLE_BIOS_METHOD1  0x31a3
#define DISABLE_BIOS_METHOD2 0x34a3
#define ENABLE_BIOS_METHOD2  0x35a3
#define ENABLE_FN            0x32a3
#define SET_FAN              0x01a3
#define GET_FAN              0x00a3
#define FN_STATUS            0x0025

void init_ioperm(void)
{
    if (ioperm(0xb2, 4, 1) || ioperm(0x84, 4, 1)) {
	perror("ioperm");
	exit(EXIT_FAILURE);
    }
}

struct smm_regs {
    unsigned int eax;
    unsigned int ebx __attribute__ ((packed));
    unsigned int ecx __attribute__ ((packed));
    unsigned int edx __attribute__ ((packed));
    unsigned int esi __attribute__ ((packed));
    unsigned int edi __attribute__ ((packed));
};

static int i8k_smm(struct smm_regs *regs)
{
    int rc;
    int eax = regs->eax;


    asm volatile("pushq %%rax\n\t"
            "movl 0(%%rax),%%edx\n\t"
            "pushq %%rdx\n\t"
            "movl 4(%%rax),%%ebx\n\t"
            "movl 8(%%rax),%%ecx\n\t"
            "movl 12(%%rax),%%edx\n\t"
            "movl 16(%%rax),%%esi\n\t"
            "movl 20(%%rax),%%edi\n\t"
            "popq %%rax\n\t"
            "out %%al,$0xb2\n\t"
            "out %%al,$0x84\n\t"
            "xchgq %%rax,(%%rsp)\n\t"
            "movl %%ebx,4(%%rax)\n\t"
            "movl %%ecx,8(%%rax)\n\t"
            "movl %%edx,12(%%rax)\n\t"
            "movl %%esi,16(%%rax)\n\t"
            "movl %%edi,20(%%rax)\n\t"
            "popq %%rdx\n\t"
            "movl %%edx,0(%%rax)\n\t"
            "pushfq\n\t"
            "popq %%rax\n\t"
            "andl $1,%%eax\n"
            :"=a"(rc)
            :    "a"(regs)
            :    "%ebx", "%ecx", "%edx", "%esi", "%edi");

    if (rc != 0 || (regs->eax & 0xffff) == 0xffff || regs->eax == eax)
            return -1;

    return 0;
}


int send(unsigned int cmd, unsigned int arg) {

    struct smm_regs regs = { .eax = cmd, };

    regs.ebx = arg;

    i8k_smm(&regs);
    return regs.eax ;

}

/* sets the speed and returns the speed of the fan after that */
int set_speed(int speed) {

    if ( speed == 0 ) {
        send(SET_FAN,0x0000);
    } else if ( speed == 1 ) {
        send(SET_FAN,0x0100);
    } else if ( speed == 2 ) {
        send(SET_FAN,0x0200);
    } else {
        printf("Ignoring unknown speed: %d\n",speed);
    }
    // return the speed the fan has now
    return send(GET_FAN,0);
}


int probecodes (void) {

    printf("Please tune the startcode by editing the source code\n");
    /* Remove the following exit call to enable this routine.
     *
     * *WARNING:* Proving for random codes in the SMBIOS management can
     * cause unexpected behaviour (even crashes or data loss) on your machine.
     *
     * USE AT YOUR OWN RISK!!
     */
    exit(EXIT_FAILURE);

    /* If you want to test this fast, use startcode=0x30a0 (for example)
     * and you should see that the code 0x30a3 is detected.
     * If you want to test all the codes then use startcode=0x0000 but
     * this will take a while.
     */
    int startcode=0x0001;
    int fanstatus, trycode;

    // Set the speed to 2, sleep 3 seconds and get the speed of the fan
    set_speed(2);
    sleep(3);
    fanstatus = send(GET_FAN,0);
    if (fanstatus == 2 ) {
        printf ("Your fan status is already set to full speed.\n"
                "In order for this to work, please set it to auto (enable BIOS control)\n"
                "And stop any process that is consuming CPU resources\n");
        exit(EXIT_FAILURE);
    }

    for (trycode=startcode; trycode <= 0xFFFF; trycode++) {
        printf ("Probing code: %#06x\n",trycode);
        // Send the code
        send(trycode,0);
        // Set the speed to 2, sleep 3 seconds and get the speed of the fan
        set_speed(2);
        sleep(3);
        fanstatus = send(GET_FAN,0);
        // If the fan is still 2 this is because the previous code disabled
        // the BIOS control of the fan.
        // Please ensure that your system is idle when running this (otherwise
        // the BIOS could set the fan to max speed to cool down your system
        // because of the load)
        if (fanstatus == 2 ) {
            printf ("The code %#06x disabled the FAN control!!!\n",trycode);
            printf ("Enabling BIOS control back...\n");
            send(ENABLE_BIOS_METHOD1,0);
            sleep(3);
            fanstatus = send(GET_FAN,0);
            if (fanstatus == 2 ) {
                printf ("ERROR: Unable to bring BIOS control back.\n");
                exit (EXIT_FAILURE);
            }
        }
    }
}


int main(int argc, char **argv) {

    int speed, disable;
    unsigned int tries;
    unsigned int xarg;

    if (geteuid() != 0) {
        printf("need root privileges\n");
        exit(EXIT_FAILURE);
    }

    if (argc < 2) {
        printf ("Use: %s speed [disable]\n",argv[0]);
        printf ("\tspeed = {0,1,2}\n");
        printf ("\t\t\t[Use speed=9 to probe for SMBIOS codes to disable the BIOS fan control.]\n");
        printf ("\tdisable = {0,1}\n");
        exit(EXIT_FAILURE);
    }

    speed = atoi(argv[1]);
    init_ioperm();

    if (speed == 9)
        return probecodes();

    if (argc > 2) {
        disable = atoi(argv[2]);
        if (disable == 1) {
            send(DISABLE_BIOS_METHOD1,0);
            printf ("BIOS CONTROL DISABLED\n");
        }
        else if (disable == 0) {
            send(ENABLE_BIOS_METHOD1,0);
            printf ("BIOS CONTROL ENABLED\n");
        }
        else  {
            printf ("Use 0 to enable bios control or 1 to disable it\n");
            exit(EXIT_FAILURE);
        }
    }

    printf ("Setting speed to: %d\n", speed );
    printf ("Speed is now at: %d\n", set_speed(speed));

}
