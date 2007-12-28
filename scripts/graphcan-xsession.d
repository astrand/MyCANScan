# This script starts graphcan in an xterm and runs icewm.
# It is suitable for an /etc/X11/Xsession.d directory, say on 
# the EEE PC
if [ -e /dev/ttyCAN ]; then    
    xterm -e "/home/user/mycanscan/graphcan/graphcan F" &
    exec icewm
fi
