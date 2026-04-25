#!/bin/bash -e
#----------------------------------------------------------
#  Script: launch.sh
#  Author: Michael Benjamin
#  LastEd: May 11th 2026
#----------------------------------------------------------
#  Part 1: Set Exit actions and declare global var defaults
#----------------------------------------------------------
TIME_WARP=30
COMMUNITY="alpha"
GUI="yes"

DEPTH_THRESH=5
DIST_THRESH=30

#----------------------------------------------------------
#  Part 2: Check for and handle command-line arguments
#----------------------------------------------------------
for ARGI; do
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ] ; then
	echo "launch.sh [SWITCHES] [time_warp]   "
	echo "  --help, -h           Show this help message"
	echo "  --depth=N            Determine depth threshold"
	echo "  --dist=N             Determine distance threshold"
	exit 0;
    elif [ "${ARGI}" = "--nogui" ] ; then
	GUI="no"
    elif [ "${ARGI:0:8}" = "--depth=" ] ; then
        DEPTH_THRESH="${ARGI#--depth=}"
    elif [ "${ARGI:0:7}" = "--dist=" ] ; then
        DIST_THRESH="${ARGI#--dist=}"
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then 
        TIME_WARP=$ARGI
    else 
        echo "launch.sh Bad arg:" $ARGI " Exiting with code: 1"
        exit 1
    fi
done


#----------------------------------------------------------
#  Part 3: Launch the processes
#----------------------------------------------------------
echo "Launching $COMMUNITY MOOS Community with WARP:" $TIME_WARP
echo "  Depth threshold: $DEPTH_THRESH"
echo "  Distance threshold: $DIST_THRESH"

nsplug alpha.moos targ_alpha.moos -f   \
    WARP=$TIME_WARP                    \
    DEPTH_THRESH=$DEPTH_THRESH         \
    DIST_THRESH=$DIST_THRESH
    
pAntler targ_alpha.moos --MOOSTimeWarp=$TIME_WARP >& /dev/null &
uMAC -t targ_alpha.moos
kill -- -$$
