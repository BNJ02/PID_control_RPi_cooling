#include <pigpio.h>
#include <unistd.h> // Needed for the sleep function
#include <stdio.h>  // Needed for the printf function
#include <iostream> // Needed for the cout function
#include <fstream>  // Needed for the file stream
#include <string>   // Needed for the string class
#include <vector>   // Needed for the vector class
#include <chrono>   // Needed for the chrono class
#include <thread>   // Needed for the thread class
#include <ctime>    // Needed for the time function

// ------------------------------------------------------------------------------- //
// Compile with: g++ -Wall -o main main.cpp -lpigpio -lrt
// Compile with: clang++ -g -Wmost -Werror -o main main.cpp -lpigpio -lrt
// Run with: sudo ./main
// ------------------------------------------------------------------------------- //

// Function to get the CPU times
std::vector<size_t> get_cpu_times() {
    std::ifstream proc_stat("/proc/stat");
    proc_stat.ignore(5, ' '); // Skip the 'cpu' prefix.
    std::vector<size_t> times;
    for (size_t time; proc_stat >> time; times.push_back(time));
    return times;
}

// Function to get the CPU usage
double get_cpu_usage() {
    const std::vector<size_t> cpu_times_start = get_cpu_times();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    const std::vector<size_t> cpu_times_end = get_cpu_times();
    const auto ACTIVE_TIME = cpu_times_end[0] - cpu_times_start[0];
    const auto IDLE_TIME = cpu_times_end[3] - cpu_times_start[3];
    return static_cast<double>(ACTIVE_TIME) / (ACTIVE_TIME + IDLE_TIME);
}

// Function to get the CPU temperature
double get_cpu_temp() {
    FILE *temperatureFile;
    double T;
    temperatureFile = fopen ("/sys/class/thermal/thermal_zone0/temp", "r");
    if (temperatureFile == NULL)
      ; //print some message
    fscanf (temperatureFile, "%lf", &T);
    T /= 1000;
    fclose (temperatureFile);
    return T;
}

// Function to perform one cycle
void one_cycle(int pwm_ventilo, int minutes)
{
    for(int n = 0; n < minutes; n++) // Loop forever
    {
        // Get the CPU temperature
        double temp = get_cpu_temp();
        printf("La température du CPU est : %.2f C\n", temp);

        // Get the current time
        time_t now = time(0);
        tm *ltm = localtime(&now);

        // Print the current time
        std::cout << "Heure à Paris: " << ltm->tm_hour << ":" << ltm->tm_min << ":" << ltm->tm_sec << std::endl;

        // Print the CPU usage
        std::cout   << "CPU Usage: " << 100*get_cpu_usage() << "%" << std::endl
                    << "------------------------------------" << std::endl << std::endl;
        
        gpioPWM(18, pwm_ventilo);   // Set the duty cycle
        sleep(60);                  // Sleep for 60 seconds
    }
}

// Main function
int main(void)
{
    if (gpioInitialise() < 0) return 1;  // Return an error if initialisation failed

    gpioSetMode(18, PI_OUTPUT);  // Set GPIO 18 as output

    int freq = gpioGetPWMfrequency(18);
    printf("La fréquence réelle est : %d Hz\n", freq);

    gpioSetPWMfrequency(18, 20);

    freq = gpioGetPWMfrequency(18);
    printf("La fréquence réelle est : %d Hz\n", freq);

    setenv("GMT", "Europe/Paris", 1);   // Set the timezone to Paris
    tzset();

    // 5 cycles of 3 hours
    for(int i = 0; i < 5; i++) {
        one_cycle(255, 90);     // 1 heure 30
        one_cycle(0, 90);       // 1 heure 30
    }
    // Total = 15 hours

    std::cout << std::endl << std::endl << "Fin du programme, fin de cyclage !!!" << std::endl;

    gpioTerminate();  // Terminate the library before exiting

    return 0;
}
