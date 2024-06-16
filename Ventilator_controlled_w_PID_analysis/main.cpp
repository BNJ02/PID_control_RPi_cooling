#include <pigpio.h> // Needed for the GPIO
#include <unistd.h> // Needed for the sleep function
#include <stdio.h>  // Needed for the printf function
#include <iostream> // Needed for the cout function
#include <fstream>  // Needed for the file stream
#include <string>   // Needed for the string class
#include <vector>   // Needed for the vector class
#include <tuple>    // Needed for the tuple class
#include <chrono>   // Needed for the chrono class
#include <thread>   // Needed for the thread class
#include <ctime>    // Needed for the time function
#include <csignal>  // Needed for the signal function

using namespace std;

// ------------------------------------------------------------------------------- //
// Compile with: g++ -Wall -o main main.cpp -lpigpio -lrt
// Compile with: clang++ -g -Wmost -Werror -o main main.cpp -lpigpio -lrt
// Run with: sudo ./main
// ------------------------------------------------------------------------------- //

// Function to get the CPU times
vector<size_t> get_cpu_times() {
    ifstream proc_stat("/proc/stat");
    proc_stat.ignore(5, ' '); // Skip the 'cpu' prefix.
    vector<size_t> times;
    for (size_t time; proc_stat >> time; times.push_back(time));

    return times;
}

// Function to get the CPU usage
double get_cpu_usage() {
    const vector<size_t> cpu_times_start = get_cpu_times();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    const vector<size_t> cpu_times_end = get_cpu_times();
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

// Function to get the CPU frequency
int get_cpu_freq() {
    ifstream file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
    if (!file) {
        cerr << "Erreur lors de l'ouverture du fichier de fréquence du CPU" << endl;
        return -1;
    }
    int freq;
    file >> freq;
    file.close();

    return freq;
}

// Function to control the PWM signal
float pid_controller(int setpoint, float measured_value, float kp, float ki, float kd, int dt, float& integral, float& last_error) {
    float error =  measured_value - float(setpoint);
    integral += error * dt;
    float derivative = (error - last_error) / dt;
    float output = kp * error + ki * integral + kd * derivative;
    last_error = error;

    return output;
}

// Function to clip the value between 120 and 255 (PWM signal)
int clip(int value) {
    int value_clipped = std::max(120, std::min(value, 255));
    if(value_clipped == 120) {
        value_clipped = 0;
    }

    return value_clipped;
}

// Function to append to a CSV file
void append_to_csv(const vector<tuple<int, float, string, float, int, float, float, float, float, float, int>>& values, const string& filename) {
    ofstream file(filename, std::ios_base::app);
    if (!file) {
        cerr << "Erreur lors de l'ouverture du fichier " << filename << endl;
        return;
    }

    // Append the values to the CSV file
    for (const auto& value : values) {
        file << int(get<0>(value)) << "," << 
                get<1>(value) << "," << 
                get<2>(value) << "," << 
                get<3>(value) << "," << 
                get<4>(value) << "," << 
                get<5>(value) << "," <<
                get<6>(value) << "," << 
                get<7>(value) << "," << 
                get<8>(value) << "," << 
                int(get<9>(value)) << "," << 
                get<10>(value) << "\n";
    }

    file.close();
}

// Function to create a CSV file
void create_csv(const string& filename, int tau_FTBO, int tau_FTBF, float ki, float kp, float kd, int dt, int nb_points) {
    ofstream file(filename, std::ios_base::out);
    if (!file) {
        cerr << "Erreur lors de l'ouverture du fichier " << filename << endl;
        return;
    }
    // Initial values
    file << "Tau FTBO,Tau FTBF,ki,kp,kd,dt,Nb de points de lecture Temp.\n";
    file << tau_FTBO << "," << tau_FTBF << "," << ki << "," << kp << "," << kd << "," << dt << "," << nb_points << "\n";
    
    // Headers
    file << "Consigne CPU Temp,Température CPU,Heure,CPU Usage (%),CPU Frequency (MHz),Erreur,Correction proportionnelle,P_range,Correction intégrale,PWM signal,PWM signal clipped\n";
    
    file.close();
}

// Set up
int setup() {
    // Set up the GPIO
    if (gpioInitialise() < 0) return 1;  // Return an error if initialisation failed
    gpioSetMode(18, PI_OUTPUT);  // Set GPIO 18 as output
    gpioSetPWMfrequency(18, 20);  // Set the PWM frequency to 20 Hz

    setenv("GMT", "Europe/Paris", 1);   // Set the timezone to Paris
    tzset();                            // Update the timezone

    return 0;  // Return 0 if everything is OK
}

// Main function
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
    float error = 0.0;
    float last_error = 0.0;
    float CPU_temperature_measured = 0.0;
    float CPU_usage = 0.0;
    float p_range = 0;

    string time_string = "";

    // Create the CSV file
    create_csv("enregistrement_PID_ventilateur_RPI.csv", tau_FTBO, tau_FTBF, ki, kp, kd, dt, nb_points);

    // Vector to store the values
    vector<tuple<int, float, string, float, int, float, float, float, float, float, int>> values;


    // Main loop
    while(true) {
        for(int i = 0; i < 4; i++)
        {
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
                printf("La température du CPU est : %.2f C\n", CPU_temperature_measured);

                // Get the current time
                time_t now = time(0);
                tm *ltm = localtime(&now);

                // Control the PWM signal
                float pwm_ventilo = 120 + pid_controller(CPU_temperature_setpoint, CPU_temperature_measured, kp * p_range, ki, kd, dt, integral, last_error);
                gpioPWM(18, clip(pwm_ventilo));   // Set the duty cycle

                // Get the current time and the error between the measured and setpoint values
                time_string = std::to_string(ltm->tm_hour) + ":" + std::to_string(ltm->tm_min) + ":" + std::to_string(ltm->tm_sec);
                error = CPU_temperature_measured - float(CPU_temperature_setpoint);

                // Print the values
                cout << "Erreur : " << error << endl <<
                        "Correction proportionnelle : " << kp * p_range * error << endl <<
                        "Correction intégrale : " << ki * integral << endl <<
                        "PWM signal : " << pwm_ventilo << endl << 
                        "CPU Usage : " << 100 * CPU_usage << "%" << endl << 
                        "Consigne : " << int(CPU_temperature_setpoint) << " C" << endl << endl;
                

                // Append the values to the CSV file
                values.push_back(std::make_tuple(   CPU_temperature_setpoint, 
                                                    CPU_temperature_measured, 
                                                    time_string, 
                                                    100 * CPU_usage, 
                                                    get_cpu_freq() / 1000, 
                                                    error, 
                                                    kp * p_range * error, 
                                                    p_range,
                                                    ki * integral,
                                                    pwm_ventilo,
                                                    clip(pwm_ventilo)));
            }
        }

        // Append the values to the CSV file                
        append_to_csv(values, "enregistrement_PID_ventilateur_RPI.csv");
        values.clear();
    }

    gpioTerminate();  // Terminate the library before exiting

    return 0;
}
