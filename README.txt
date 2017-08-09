
1. Compiled on Linux machine. 
	In order to compile type: make

2. Commands you can run in stations/hosts
   
   send-<destination name>-<message> // send message to a destination host
   ex: send-B-hello
   
   show-ARP 	// show the ARP mapping table for station
   show-ARP RT  // show the ARP mapping table for router
   show-SELF    // show the Self Learning Table information
   
   Timer Info : sleep time is given for 6 seconds and counter is set to 30.
   The count value decreases after every 6 seconds. So effectively after 180 seconds, if the station is inactive the data will be removed
   from the bridge table. If the station is active then the count will be reset to 30.  

3. To start the emulation, run

   run_simulation

   which emulates the following network topology

   
          B              C                D
          |              |                |
         Cs1-----R1------Cs2------R2-----Cs3
          |                              
          A                		  

    A ,B ,C, D are hosts/stations.
    Cs1, Cs2, and Cs3 are bridges.
    R1 and R2 are routers.

DIFFICULTIES FACED
-------------------
1.Initially we faced issues in reading the data from the hardcoded file but later we found out how to fetch the detals and stored it in a structure

2.The data was not getting stored to the .addr and .port files, after using strncat and fprintf functionality we were able to achieve it.

3.We faced difficulties in the payload parameter of the ethernet frame because when one frame is sent it is received twice in the bridge 
and broadcasted. It was happening due to creation of two structures in the payload parameter. But then, after removing the two structures from the
payload the issue got resolved.
    
4.Since the ARP Cache was not updated properly, stations/routers were asking for ARP request always.This was because the MAC address value was null
before assigning.Due to this, the MAC address was not updated in the ARP Cache.

5. Due to the ARP cache issue, all messages were getting sent to a single station.So we had to do a lot of debugging to find out where the exact
issue was.

LOG OF THE PROGRESS
-------------------

1. We started with a basic bridge-station program where the messages are broadcasted to all the bridges.
2. Then we started implementing the Self-Learning Functionality of the bridge.
3. Once the Self Learning was working, we started the ARP implementation.
	->	Initially tried with A -> B sending ...
	-> 	Once ARP was working from A -> B ,started the code for router.
4. Router code was similar to station code except for a few fucntionality differences.
5. Once the router was done, next was implementing the routing functionality.
6. There were issues when combining both Routing and ARP functionalities but we were able to solve the issues and deliver the packet.
7. After routing , we worked on the timer , pendingqueue , accept and reject messages etc.

HOW THE WORK IS SEPARATED
-------------------------
We initially discussed the flow of the project. Started with the station and bridge file initialisations individually. Later discussed the
SelfLearning, ARP and Routing coding and implemented together. 

  