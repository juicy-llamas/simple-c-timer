
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>

#define DOUBLE_NAN (0.0 / 0.0)

/*
 * Simple, reasonably accurate, graphical timer in C that gives time
 * estimates.
 */

#define err( cond, msg ) ({                         \
    if ( (cond) ) {                                 \
        fprintf( stderr, "Error: %s\nReason: %s\n", \
                msg, strerror( errno ) );           \
        exit( 1 );                                  \
    }                                               \
})

// takes one argument
int main ( int argc, char** argv ) {
    // first get time
    struct timespec prev_t, cur_t, wait_t;
    err( clock_gettime( CLOCK_REALTIME, &prev_t ) != 0, "gettime failed" );

    // parse times.
    if ( argc < 2 ) {
        fprintf( stderr, "usage: %s <[time in hrs]:>[time in min]<:[time in secs]>\ntimes can be floating point numbers in xxx.yyy format\n\nNote that precedence goes min, min:sec, and hr:min:sec.\nSo if you want 3.5 hours, you could do 3.5:0:0 or 3:30:0, but 3.5:0 is 3.5 minutes\n", argv[ 0 ] );
        return 2;
    }

    double wait_time, elapsed_time, target = DOUBLE_NAN;
    double last_time_diff, max_time_diff = 0;
    int errc = 0;
    
    err( ( errc = sscanf( argv[ 1 ], "%lf:%lf:%lf", &wait_time, 
                &elapsed_time, &target ) ) == 0, "sscanf failed" );
    if ( isnan( wait_time ) )
        return 3;
    // if wait time is a number only, it is in minutes
    wait_time *= 60;
    // if elapsed_time is a number, it is in seconds. 
    // wait time doesn't change
    if ( ! isnan( elapsed_time ) ) {
        wait_time += elapsed_time;
    }
    // if target is a number, then wait_time is in hours, 
    // elapsed_time is in minutes, and target is in seconds.
    if ( ! isnan( target ) ) {
        wait_time *= 60;
        wait_time += target;
    }
    elapsed_time = 0;
target = 0;

    while ( 1 ) {
        // print time left, minus one sec cause you're seeing the message
        // *after* the nth second
        int secs = (int)( wait_time - elapsed_time - 1 );
        int hrs = secs / 3600;
        int rem = secs % 3600;
        int min = rem / 60;
        int sec = rem % 60;
        printf( "%02i:%02i:%02i\r", hrs, min, sec );
        // this is necessary
        fflush( stdout );

        // the time we're 'actually at'
        // cur_t and prev_t are as close together as possible for
        // accuracy.
        err( clock_gettime( CLOCK_REALTIME, &cur_t ) != 0,
                "gettime 2 failed" );
        last_time_diff = 
                (double)( cur_t.tv_sec  - prev_t.tv_sec  ) +
                (double)( cur_t.tv_nsec - prev_t.tv_nsec ) / 
                    1000000000.0f;
        err( clock_gettime( CLOCK_REALTIME, &prev_t ) != 0, 
                "gettime 3 failed" );

        // measure elapsed time here because it's most accurate
        if ( elapsed_time + last_time_diff >= wait_time )
            break;
        // we didn't do this before
        elapsed_time = elapsed_time + last_time_diff;

        // get max time diff
        max_time_diff = last_time_diff > max_time_diff ? 
                        last_time_diff : max_time_diff;
        
        double true_wait = 0;

        // if we have more than a second, we adjust target and calculate
        // the time gap. Target is our "target" loop delay time, so we
        // adjust wait time to hit the target more accurately.
        if ( wait_time - elapsed_time > 1 ) {
            target += 1;
            true_wait = target - elapsed_time;
        // otherwise, we take the remainder and subtract it by the max
        // time diff. If the time is longer than the max time diff,
        // it tries the last time diff. If that doesn't work, then it just
        // continues until the elapsed time expires.
        } else {
            true_wait = ( wait_time - elapsed_time ) - max_time_diff;
            true_wait = true_wait < 0 ? 
                    ( wait_time - elapsed_time ) - last_time_diff 
                    : true_wait;
            if ( true_wait < 0 )
                continue;
        }

        // assign wait time to wait_t struct
        wait_t.tv_sec = (time_t)true_wait;
        wait_t.tv_nsec = (long)( (true_wait - wait_t.tv_sec) * 
                1000000000.0f );
        
        // we're not dealing with sighandlers, so no remainder
        err( clock_nanosleep( CLOCK_REALTIME, 0, &wait_t, 0 ) != 0, 
                "nanosleep failed" );
    }

    puts( "\nTimer is done" );
    return 0;
}
