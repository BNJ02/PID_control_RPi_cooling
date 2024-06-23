/**
 * @file main.cpp
 * @brief Main application file to performs the PI controller for the RPi service.
 */

#include <pigpio.h> // Needed for the GPIO
#include <unistd.h> // Needed for the sleep function
#include <fstream>  // Needed for the file stream
#include <string>   // Needed for the string class
#include <vector>   // Needed for the vector class
#include <chrono>   // Needed for the chrono class
#include <thread>   // Needed for the thread class
#include <ctime>    // Needed for the time function
#include <cstdlib>  // Needed for the setenv function

using namespace std;

// ------------------------------------------------------------------------------- //
// Compile with: g++ -Wall -o main main.cpp -lpigpio -lrt
// Compile with: clang++ -g -Wmost -Werror -o main main.cpp -lpigpio -lrt
// Run with: sudo ./main.bin
// ------------------------------------------------------------------------------- //

/**
 * @brief Function to get the CPU times.
 * @return vector<size_t> Vector of CPU times.
 */
vector<size_t> get_cpu_times() {
    ifstream proc_stat("/proc/stat");
    proc_stat.ignore(5, ' '); // Skip the 'cpu' prefix.
    vector<size_t> times;
    for (size_t time; proc_stat >> time; times.push_back(time));

    return times;
}

/**
 * @brief Function to get the CPU usage.
 * @return double CPU usage.
 */
double get_cpu_usage() {
    const vector<size_t> cpu_times_start = get_cpu_times();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    const vector<size_t> cpu_times_end = get_cpu_times();
    const auto ACTIVE_TIME = cpu_times_end[0] - cpu_times_start[0];
    const auto IDLE_TIME = cpu_times_end[3] - cpu_times_start[3];

    return static_cast<double>(ACTIVE_TIME) / (ACTIVE_TIME + IDLE_TIME);
}

/**
 * @brief Function to get the CPU temperature.
 * @return double CPU temperature.
 */
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

/**
 * @brief Function to control the PWM signal.
 * @param setpoint Setpoint value.
 * @param measured_value Measured value.
 * @param kp Proportional gain.
 * @param ki Integral gain.
 * @param kd Derivative gain.
 * @param dt Sampling time.
 * @param integral Integral value.
 * @param last_error Last error value.
 * @return float PWM signal.
 */
float pid_controller(int setpoint, float measured_value, float kp, float ki, float kd, int dt, float& integral, float& last_error) {
    float error =  measured_value - float(setpoint);
    integral += error * dt;
    float derivative = (error - last_error) / dt;
    float output = kp * error + ki * integral + kd * derivative;
    last_error = error;

    return output;
}

/**
 * @brief Function to clip the value between 120 and 255 (PWM signal).
 * @param value Value to clip.
 * @return int Clipped value.
 */
int clip(int value) {
    int value_clipped = std::max(120, std::min(value, 255));
    if(value_clipped == 120) {
        value_clipped = 0;
    }

    return value_clipped;
}

/**
 * @brief Function to set up the GPIO and the PiGPIO service.
 * @return int 0 if successful, 1 otherwise.
 */
int setup() {
    // Remove the files if it exists
    system("sudo rm /var/run/pigpio.pid");  // Remove the file if it exists
    system("fuser -k 8888/tcp"); // Kill the process if it exists
    // Set up the GPIO
    if (gpioInitialise() < 0) return 1;  // Return an error if initialisation failed
    gpioSetMode(18, PI_OUTPUT);  // Set GPIO 18 as output
    gpioSetPWMfrequency(18, 20);  // Set the PWM frequency to 20 Hz

    setenv("GMT", "Europe/Paris", 1);   // Set the timezone to Paris
    tzset();                            // Update the timezone

    return 0;  // Return 0 if everything is OK
}

/**
 * @brief Main function that performs the PI controller.
 * @return int Program exit code.
 */
int main(void)
{
    // Set up the GPIO
    if (setup() != 0) return 1;  // Return an error if initialisation failed


    // PID parameters
    int CPU_temperature_setpoint = 70;              // Consigne de température du CPU
    
    const int tau_FTBO = 230;                       // Constante de temps du premier ordre en FTBO
    const int tau_FTBF = 300;                       // Constante de temps du premier ordre en FTBF
   
    const float ki = 1 / float(tau_FTBF);           // Coefficient Intégral
    const float kp = float(tau_FTBO) * ki * 60;     // Coefficient Proportionnel (* 10 pour une meilleure rapidité de réponse)
    const float kd = 0.0;                           // Coefficient Dérivé
    const int dt = 5000;                            // Période d'échantillonnage, pas de temps (en millisecondes)
    const int nb_points = 10;                       // Nb de points pour la lecture de la température sur une période de dt

    // Initialisation of the other PID parameters 
    float integral = 0.0;                   
    float last_error = 0.0;
    float CPU_temperature_measured = 0.0;
    float CPU_usage = 0.0;
    float p_range = 0;
    float pwm_ventilo = 0;

    time_t now = time(0); 

    // Main loop
    while(true) {
        // Get the current time
        now = time(0);
        tm *ltm = localtime(&now);

        // I sleep between midnight and 7 a.m.
        if (ltm->tm_hour >= 7) {
            // Get the CPU temperature
            CPU_usage = get_cpu_usage();

            for(int j = 0; j < 3; j++)
            {
                // Update setpoint if needed (depending on the CPU usage)
                if(CPU_usage < 0.25) { CPU_temperature_setpoint = 60; p_range = 1.3; } 
                else if(CPU_usage < 0.5) { CPU_temperature_setpoint = 70; p_range = 1; }
                else if(CPU_usage < 0.75) { CPU_temperature_setpoint = 75; p_range = 1.3; } 
                else { CPU_temperature_setpoint = 85; p_range = 1.5; }

                // Get the CPU temperature
                for(int k = 0; k < nb_points; k++)
                {
                    CPU_temperature_measured += get_cpu_temp();
                    std::this_thread::sleep_for(std::chrono::milliseconds(dt / nb_points)); // Sleep for the time step
                }
                CPU_temperature_measured /= nb_points;

                // Control the PWM signal
                pwm_ventilo = 120 + pid_controller(CPU_temperature_setpoint, CPU_temperature_measured, kp * p_range, ki, kd, dt, integral, last_error);
                gpioPWM(18, clip(pwm_ventilo));   // Set the duty cycle
            }
        }
        else {
            gpioPWM(18, 0);   // Set the duty cycle to 0
            std::this_thread::sleep_for(std::chrono::seconds(60)); // Sleep for the time step
        }
    }
    gpioTerminate();  // Terminate the library before exiting

    return 0;
}
