## OpenThread-specific Whitefield Guide

**Build Instructions**:

The build instructions are identical to the ones laid out in the top-level README.md.

**Execute Instructions**:

```
$ cd whitefield

# Start the simulation
$ ./invoke_whitefield.sh config/wf_ot.cfg
```
Note: With OpenThread, the simulation runs in virtual time. This means that the amount of time that passes in real life is decoupled from the time that passes in the simulation. So, for example, it might take less than a minute in real life for Whitefield to simulate 10 minutes of an OpenThread network.

After running the simulation, the next step is to wait until the simulation has run to completion. Here is how to do that:
* Open the airline log file using the command ```less log/airline.log```. At this point, the simulation may still be running, or may have already terminated. Either way, proceed to the next step.
* Hold down the following keys: 'Shitf + g'. This will make the terminal track the end of the log file, even while it is still getting written to if the simulation is still running. Keep holding these two keys until the screen is no longer scrolling.
* When the screen stops scrolling, meaning that the log file is no longer being written to, this could mean one of two things: Either the simulation has completed, or (which can happen sometimes) the simulation is hanging.
* If the simulation has completed, you should see the following in the last few lines of the log file:
```bash
SIMULATION ENDED
INFO  Airline Caught Signal 2
INFO  00:00:00.000 [usock_cleanup:96] closed commline unix sockets
INFO  Sayonara 2
```
* If, on the other hand, you do not see the above messages, then the simulation has likely gotten stuck somewhere. To fix this:
    * Run the script ```./scripts/wfshell stop_whitefield```. This will kill all processes associated with the simulation.
    * After Whitefield is stopped, you can try running the simulation again.

**Configuration Options**:

The config file ```config/wf_ot.cfg``` contains parameters which can be modified to affect the simulation in different ways. 

The most important of these are:
* numOfNodes: Determines how many nodes will be simulated.
* simulationEndTime: Determines how many minutes _in virtual time_ will be simulated before Whitefield terminates.


**Gathering Logs/Data/Statistics**:

Once the simulation has executed, logs can be obtained from ```log/airline.log```, and pcap files for each node from ```pcap/pkt-0-0.pcap``` (for node 0, for example). The pcap files can be opened in Wireshark.

Once opened in Wireshark, do the following in order to allow Wireshark to correctly interpret and decode the pcap file:
* Add the network key as a decription key.
    1. Open the ```config/wf_ot.cfg``` file.
    2. Locate the ```nodeConfig``` parameter. This is a semicolon-separated list of key-value pairs.
    3. One of the 'keys' is ```networkkey```. Copy the corresponding value, e.g. ```00112233445566778899aabbccddeeff```.
    4. Go back to Wireshark, and click on Edit > Preferences. This will open up the Preferences window.
    5. In the options of the left, expand the Protocols list, and scroll down until you find IEEE 802.15.4.
    6. Select IEEE 802.15.4, and click on Edit... next to "Decryption Keys". This will open up a window where you can add/remove/edit decryption keys.
    7. On the lower left corner of the window, there is a '+' symbol. Click on this symbol.
    8. Paste the network key that you had copied from the config file.
    9. Under 'Key hash', select 'Thread hash'.
    10. Now click 'OK' in the bottom right corner of the window.
    10. Click 'OK' again to exit out of the Preferences window.
* Allow UDP messages to be decoded as CoAP.
    1. Locate a message whose Protocol field says UDP.
    2. Right-click on it. This will result in it being highlighted.
    3. Select Decode As... This will open up a window.
    4. In the window, click on the field under 'Current' to expand the drop-down list of protocols.
    5. Select CoAP from that list.
    6. Click 'OK' on the bottom right to exit out of this window.

**Additional Useful Information/Tips**

* DO NOT use the ```./scripts/monitor.sh``` or ```./scripts/whitefield_status.sh``` scripts. These are not intended to work with virtual time simulation!
* In the ```log/airline.log```, the simulation time is displayed in units of Microseconds. 
* Tips on navigating/looking through logs: 
    * Open the logs with the ```less``` command, e.g. ```less log/airline.log```.
    * You can jump to the top of the file by pressing 'gg' (g key twice).
    * You can jump to the end of the file by pressing 'Shift + g'.
    * You can search for any word in the file by typing '/' (forward slash) followed by your search entry, e.g. '/DataIndication' to search for 'DataIndication'.
    * Press the 'n' key to navigate to the next occurrence of your search, and 'Shift + n' for the previous occurrence.

## OpenThread-Whitefield Integration Aim

Validate OpenThread claims in realistic 802.15.4 environment possibly in
standalone mode i.e., without having to worry about third-party OS quirks.

Using Whitefield it should be possible to test following:
1. OpenThread scalability 
    * max routers per BR
    * max child nodes per routers
    * max BRs
    * max hops
2. Openthread performance under realistic conditions
    * Network convergence time
    * Node join time
    * packet delivery rate
    * Control Overhead incurred under different scenarios
    * Repair time (involving router failure, leader failure, BR failure)
    * Reboot handling of Router/Leader/BR
3. Possible to simulate following scenarios
    * High node density scenario
    * Variable data-rate per node
    * Impact of node mobility
    * With and without memory constraints
    * Battery performance

## OpenThread Integration Option

### Integrate by adding new platform in core OpenThread

Adding a new platform of whitefield in OpenThread just like cc2538 or posix
(already present in OpenThread). This option allows to directly use core
OpenThread in Whitefield.

Advantage: Can test OpenThread in standalone mode without any dependency on
another OS.

Disadvantage: In real world, the OpenThread core would be used along with some
another OS/platform (such as [Zephyr]/[RIOT]/[FreeRTOS]). Only core-openthread
could be tested with such integration mode.

This mode of integration best satisfies the aim mentioned above and this is the
mode targeted first.

### Integrate using existing OpenThread supported OS ([RIOT]/[Zephyr]/[FreeRTOS])

Advantage: Get existing OpenThread integration support in existing popular IoT
OS.

Disadvantage: Have to follow the quirks of the IoT OS used. If the aim is to
validate OpenThread then this is unnecessary albeit counter-productive.

This should have been the simplest mode of integration but it means adding
Whitefield as a platform/driver/target in [RIOT]/[Zephyr]/[FreeRTOS]. Currently
Whitefield integrates with [RIOT] as a separate package and not as a
platform/driver (just like OpenThread), and thus cannot use OpenThread (because
two packages cannot be used together). [Zephyr] is not supported in Whitefield
at the moment.

Whitefield works as a separate target currently only in [Contiki].
Unfortunately, Contiki does not support OpenThread integration at the moment.

Another option is to try [OT-RTOS] which is based on [FreeRTOS], but it also
requires to add platform support in [FreeRTOS].

### Integrate by externally attaching to Posix platform

This is the least intrusive mode of integration in OpenThread i.e., use
existing posix platform and override radio transmit/receive (send/recv) with
Whitefield transmit/receive using preloading techniques such as LD\_PRELOAD, or
GCC -wrap option.

Advantage: Can readily use any OpenThread binary directly in Whitefield.

Disadvantage: The preloaded lib needs to be in sync with OpenThread APIs. This
is difficult to enforce with this mode of integration.

## OpenThread TODOs
1. Prepare call flows for documentation. This may help future drivers writing
   new porting interfaces.
2. How is OpenThread-Whitefield different from OpenThread-Posix?
    a. ACKs are handled in Whitefield. (This is yet to be handled)

## Current Stage for OpenThread Integration:
Have created a separate platform for whitefield in OpenThread. Commline
init/deinit is handled but the actual commline send/recv is not handled as of
now. In radioTransmit/Receive the posix-UDP send/recv is done to tx/rx
messages. Using this I have verified that the Leader and Router node work fine.
Steps to use the OpenThread integration:
1. ./invoke\_whitefield.sh config/wf\_ot.cfg ... This step just turns on all
   the nodes but does not start the network.
2. ./scripts/init\_openthread\_nodes.sh ... This step starts the openthread
   network subsystem. You can change this script if you want to use different
   config (for e.g., diff panID).

## FAQs:

[FreeRTOS]: https://freertos.org/
[RIOT]: https://www.riot-os.org/
[Zephyr]: https://www.zephyrproject.org/
[Contiki]: http://www.contiki-os.org/
[OT-RTOS]: https://openthread.io/platforms/ot-rtos

