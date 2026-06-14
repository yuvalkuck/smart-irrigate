# design configuration
## start/stop
1. start/stop on demand + duration
   - or 
1. select repeat type
   - evry x days
   - on days select specific
   - start if temperature is above & humidity is below
   - start it barometric pressure is below or above

## time
1. select start time
3. select duration

# payloads
## on start
   - current configuration
   - current active valvule
   - current remain duration of of active valvule
## Average Environment Scale
average per minutes (X = 5, 10, 15, 30, 60, 90, 120)
   - current temperature
   - current humidity
   - current barometric pressure
   - current gas resistance
## on event start/stop:
   - valvule status
   - duration
## On Demand:
   - valvule status
   - duration remaining
   - full configuration
   - start/stop valvule, optional alt duration
# Configuration - plan
---
 == THIS REPEAT IS A MESS ==
---

## Next Repeat
   - Every x specicif days,weeks,  
     - 01:0123456 - week days - char bitwise
     - 02:48 - max weeks (1-52)
   - Every x counter minutes (x = 5,10,15,30,60)
     - 03:1440 - max minutes (1-1440)
     - 04: reserved 
   - Environment Conditions {next version}
     - start T > xT (temperature)
     - start H < xH (humidity)
     - start P > xP (barometric pressure)
     - avoid T < xT (temperature)
     - avoid H > xH (humidity)
     - avoid P < xP (barometric pressure)
   
## Task in a day 
   - start time in minutes (1-1440)
   - duration in minutes max (1-120)
   - Next Repeat
## Valve
   - id
   - vector<Task>
   

# Format Layout
## Commands
format is binary there for all structures are ajusted by fixed size
### Segment 01
1. replace configuration
2. start on deman valve,task
3. stop on deman valve
4. Suspend valve
5. Release valve
6. get active status
7. get next task to run
### Segment 02
11. System Restart
   
### Configuration
```
   revision [0001], command [99] ,today index [001], timestamp 1781100200 \n
   valve: [Task] \t [Task] \n
   valve: [Task] \t [Task] \n
   ```
   seperation by \n & \t is just for visualization 
   if valve is not use it number will exist but will follow by 0x0000 (one short)
   
   example with seperated fields
```
   0001 01 0001 4545454545 \n 
   01 1440 1440 02 48 \t 1440 1440 03 1440 \n
   02 1440 1440 02 48 \t 1440 1440 02 135. \t 1440 1440 03 44.. \n
   03 .... \n
   ```
   real payload
   (.) dots represend empty bytes reset to 0x00
````
000101000100014545454545
01144014400248    14401440031440
02144014400248    1440144002135.  144014400344..
03
04144014400248    144014400344..
```
### On Demand Start
   revision [0001] command 02 valve [01] task [02]
### On Demand Stop  
   revision [0001] command 03 valve [01]
### Suspend Valve
   revision [0001] command 04 valve [01]
### Release Valve
   revision [0001] command 05 valve [01]      
### Get Active Status include hold valve
   revision [0001] command 06
### Get Next Task to Run 
   revision [0001] command 07
### Restart
   revision [0001] command 11   
       
