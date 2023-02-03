
if [ -z "$DEVKITARM" ]; 
then
    echo "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM";
else
    make 2>&1 >/dev/null | grep ^$(pwd);
fi


