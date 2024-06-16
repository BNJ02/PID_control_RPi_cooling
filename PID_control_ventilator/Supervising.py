import time
import psutil
import os

# Function to get the CPU usage
def get_cpu_usage():
    return psutil.cpu_percent(interval=0.5)

# Function to get the CPU temperature
def get_cpu_temp():
    with open("/sys/class/thermal/thermal_zone0/temp", "r") as temperatureFile:
        T = float(temperatureFile.read()) / 1000
    return T

# Main function
if __name__ == "__main__":
    # Main loop to display informations of supervising
    while True:
        cpu_usage = get_cpu_usage()
        if cpu_usage < 25:
            setpoint = 60
        elif cpu_usage < 50:
            setpoint = 70
        elif cpu_usage < 75:
            setpoint = 75
        else:
            setpoint = 80

        print("---------------------------------------------------")
        print("Consigne : {}°C".format(setpoint))
        print("Mesuré : {:.2f}°C".format(get_cpu_temp()))
        print("CPU usage : {}%".format(cpu_usage))
        
        time.sleep(5)