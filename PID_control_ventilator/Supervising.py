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
    cpu_usage = 0.0
    cpu_temp = 0.0

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

        for i in range (0, 10):
            cpu_temp += get_cpu_temp()
            time.sleep(0.1)
        cpu_temp /= 10

        print("--------------------------------------------")
        print("Consigne :\t{}°C".format(setpoint))
        print("Mesuré :\t{:.2f}°C".format(cpu_temp))
        print("CPU usage :\t{}%".format(cpu_usage))
        
        time.sleep(5)