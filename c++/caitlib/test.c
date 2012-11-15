/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 * DESCRIPTION:
 *
 *
 * NOTES:
 *
 *
 *****************************************************************************/

#include "caitlib.h"

#include <unistd.h>

int main()
{
    CaitlibHandle handle = caitlib_init("test");
    const uint32_t port  = caitlib_create_port(handle, "midi-outX");

    int sampleRate = 44100; //jack_get_sample_rate(client);

    int m = sampleRate/1000; // whatever

    // 0 Par ch=1 c=7 v=99
    // 0 Par ch=1 c=10 v=63
    // 0 Par ch=1 c=0 v=0
    caitlib_put_control(handle, port, 0*m, 0, 7, 99);
    caitlib_put_control(handle, port, 0*m, 0, 10, 63);
    caitlib_put_control(handle, port, 0*m, 0, 0, 0);

    // 0 PrCh ch=1 p=0     -- TODO jack_midi_put_program()

    // 0 On ch=1 n=64 v=90
    // 325 Off ch=1 n=64 v=90
    // 384 On ch=1 n=62 v=90
    // 709 Off ch=1 n=62 v=90
    // 768 On ch=1 n=60 v=90
    // 1093 Off ch=1 n=60 v=90
    caitlib_put_note_on(handle, port,     0*m, 0, 64, 90);
    caitlib_put_note_off(handle, port,  325*m, 0, 64, 90);
    caitlib_put_note_on(handle, port,   384*m, 0, 62, 90);
    caitlib_put_note_off(handle, port,  709*m, 0, 62, 90);
    caitlib_put_note_on(handle, port,   768*m, 0, 60, 90);
    caitlib_put_note_off(handle, port, 1093*m, 0, 60, 90);

    // 1152 On ch=1 n=62 v=90
    // 1477 Off ch=1 n=62 v=90
    // 1536 On ch=1 n=64 v=90
    // 1861 Off ch=1 n=64 v=90
    // 1920 On ch=1 n=64 v=90
    // 2245 Off ch=1 n=64 v=90
    caitlib_put_note_on(handle, port,  1152*m, 0, 62, 90);
    caitlib_put_note_off(handle, port, 1477*m, 0, 62, 90);
    caitlib_put_note_on(handle, port,  1536*m, 0, 64, 90);
    caitlib_put_note_off(handle, port, 1861*m, 0, 64, 90);
    caitlib_put_note_on(handle, port,  1920*m, 0, 64, 90);
    caitlib_put_note_off(handle, port, 2245*m, 0, 64, 90);

    // 2304 On ch=1 n=64 v=90
    // 2955 Off ch=1 n=64 v=90
    // 3072 On ch=1 n=62 v=90
    // 3397 Off ch=1 n=62 v=90
    // 3456 On ch=1 n=62 v=90
    // 3781 Off ch=1 n=62 v=90
    caitlib_put_note_on(handle, port,  2304*m, 0, 64, 90);
    caitlib_put_note_off(handle, port, 2955*m, 0, 64, 90);
    caitlib_put_note_on(handle, port,  3072*m, 0, 62, 90);
    caitlib_put_note_off(handle, port, 3397*m, 0, 62, 90);
    caitlib_put_note_on(handle, port,  3456*m, 0, 62, 90);
    caitlib_put_note_off(handle, port, 3781*m, 0, 62, 90);

    // 3840 On ch=1 n=62 v=90
    // 4491 Off ch=1 n=62 v=90
    // 4608 On ch=1 n=64 v=90
    // 4933 Off ch=1 n=64 v=90
    // 4992 On ch=1 n=67 v=90
    // 5317 Off ch=1 n=67 v=90
    caitlib_put_note_on(handle, port,  3840*m, 0, 62, 90);
    caitlib_put_note_off(handle, port, 4491*m, 0, 62, 90);
    caitlib_put_note_on(handle, port,  4608*m, 0, 64, 90);
    caitlib_put_note_off(handle, port, 4933*m, 0, 64, 90);
    caitlib_put_note_on(handle, port,  4992*m, 0, 67, 90);
    caitlib_put_note_off(handle, port, 5317*m, 0, 67, 90);

    // 5376 On ch=1 n=67 v=90
    // 6027 Off ch=1 n=67 v=90
    // 6144 On ch=1 n=64 v=90
    // 6469 Off ch=1 n=64 v=90
    // 6528 On ch=1 n=62 v=90
    // 6853 Off ch=1 n=62 v=90
    caitlib_put_note_on(handle, port,  5376*m, 0, 67, 90);
    caitlib_put_note_off(handle, port, 6027*m, 0, 67, 90);
    caitlib_put_note_on(handle, port,  6144*m, 0, 64, 90);
    caitlib_put_note_off(handle, port, 6469*m, 0, 64, 90);
    caitlib_put_note_on(handle, port,  6528*m, 0, 62, 90);
    caitlib_put_note_off(handle, port, 6853*m, 0, 62, 90);

    // 6912 On ch=1 n=60 v=90
    // 7237 Off ch=1 n=60 v=90
    // 7296 On ch=1 n=62 v=90
    // 7621 Off ch=1 n=62 v=90
    // 7680 On ch=1 n=64 v=90
    // 8005 Off ch=1 n=64 v=90
    caitlib_put_note_on(handle, port,  6912*m, 0, 60, 90);
    caitlib_put_note_off(handle, port, 7237*m, 0, 60, 90);
    caitlib_put_note_on(handle, port,  7296*m, 0, 62, 90);
    caitlib_put_note_off(handle, port, 7621*m, 0, 62, 90);
    caitlib_put_note_on(handle, port,  7680*m, 0, 64, 90);
    caitlib_put_note_off(handle, port, 8005*m, 0, 64, 90);

    // 8064 On ch=1 n=64 v=90
    // 8389 Off ch=1 n=64 v=90
    // 8448 On ch=1 n=64 v=90
    // 9099 Off ch=1 n=64 v=90
    // 9216 On ch=1 n=62 v=90
    // 9541 Off ch=1 n=62 v=90
    caitlib_put_note_on(handle, port,  8064*m, 0, 64, 90);
    caitlib_put_note_off(handle, port, 8389*m, 0, 64, 90);
    caitlib_put_note_on(handle, port,  8448*m, 0, 64, 90);
    caitlib_put_note_off(handle, port, 9099*m, 0, 64, 90);
    caitlib_put_note_on(handle, port,  9216*m, 0, 62, 90);
    caitlib_put_note_off(handle, port, 9541*m, 0, 62, 90);

    // 9600 On ch=1 n=62 v=90
    // 9925 Off ch=1 n=62 v=90
    // 9984 On ch=1 n=64 v=90
    // 10309 Off ch=1 n=64 v=90
    // 10368 On ch=1 n=62 v=90
    // 10693 Off ch=1 n=62 v=90
    caitlib_put_note_on(handle, port,   9600*m, 0, 62, 90);
    caitlib_put_note_off(handle, port,  9925*m, 0, 62, 90);
    caitlib_put_note_on(handle, port,   9984*m, 0, 64, 90);
    caitlib_put_note_off(handle, port, 10309*m, 0, 64, 90);
    caitlib_put_note_on(handle, port,  10368*m, 0, 62, 90);
    caitlib_put_note_off(handle, port, 10693*m, 0, 62, 90);

    // 10752 On ch=1 n=60 v=90
    // 12056 Off ch=1 n=60 v=90
    caitlib_put_note_on(handle, port,  10752*m, 0, 60, 90);
    caitlib_put_note_off(handle, port, 12056*m, 0, 60, 90);

    // 13824 Par ch=1 c=64 v=0 -- ??

    while (1)
        sleep(1);

    caitlib_close(handle);

    return 0;
}
