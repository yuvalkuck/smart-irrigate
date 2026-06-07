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
## every X minutes (X = 5, 10, 15, 30, 60) the average of:
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