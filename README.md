# SoloEngine
//Set this environment variables to resume stucking in the compile time

JAVA_FLAGS = -Dhttps.proxyHost=78.15.55.55 -Dhttps.proxyPort=32310 -Dhttp.proxyHost=78.15.55.55 -Dhttp.proxyPort=32310 
JAVA_OPTIONS = -Dhttps.proxyHost=78.15.55.55 -Dhttps.proxyPort=32310 -Dhttp.proxyHost=78.15.55.55 -Dhttp.proxyPort=32310



//Run below commands for using adb over wifi

1-Connect Android phone to host machine using USB cable (to start with)
2-Run "adb tcpip 5555" from a command prompt
3-Disconnect USB cable and run "adb connect <ip_address>:5555"
