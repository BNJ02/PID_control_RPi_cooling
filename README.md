<!--
Author: Benjamin Lepourtois <benjamin.lepourtois@gmail.com>
Copyright: All rights reserved.
See the license attached to the root of the project.
-->

<center> 🇬🇧 English version below ! 🇬🇧</center>

# 🇫🇷 Asservissement et régulation du refroidissement d'une RPi avec un correcteur PID

Le but de ce projet est d'asservir et de réguler le refroidissement d'une RPi. Voici le kit que j'ai pu acheter : [Kit RPi4 acheté](https://www.amazon.fr/Bqeel-Alimentation-Interrupteur-Ventilateur-Dissipateur/dp/B082PSBBMM/ref=sr_1_14?dib=eyJ2IjoiMSJ9.8ZfM4XGhIburiNAfEtgKbJFrU5zvWGZoBZ-6HRG4gFjt2i8pjqskQJPU_KjPN7vEdm6KOWqjWq_iFT-oObGvz2xWPGZmdHsSbrJnYXjrB3F7Ad_vnLYooypQqPPFwqQe7gGHJ57mA7NC30jQawqD7L142dfsjkJJnwaJw5LuD1YDDkUX0XS7CnVAGJchgjCAU4LaBSgiruQE6XINpymChrsZDNNicJMLEG-OJog1AwlwJedFPN9K_zxn6UqhTWdGk9F0qHX4Xsbf0k8BBapWVNJrBmAnePdH5uOXKM0Qf_0.QHt9RVqp2MEe4xmj7bZPu1lMkk6QZU34N2WpOzn6x-8&dib_tag=se&keywords=rpi4&qid=1718524926&sr=8-14&th=1).

![Kit RPi4 image venant d'Amazon](doc/RPi4_kit.png)

## 🇫🇷 Réalisation d'un module de commande du ventilateur

Il faut savoir que le moteur DC du ventilateur consomme à sa pleine puissance plus de courant que peut en fournir le GPIO (General Purpose Input Output) de la Raspberry Pi 4. Il faut alors réaliser un petit montage électronique pour l'adaptation de puissance :

![Montage électronique de commande du ventilateur](doc/Hardware_schematic.png)

La diode est placée ici comme diode de roue libre pour éviter une tension de contre-électromotrice qui endommagerai les composants autour notamment le transistor bipolaire.
<br>*"La diode de roue libre joue un rôle crucial dans la protection des circuits électroniques en fournissant un chemin de moindre résistance pour le courant induit lors de l'arrêt d'une bobine électromagnétique, prévenant ainsi les dommages causés par les pics de tension."*

![Exemple d'un signal PWM](doc/PWM_example.png)

Nous pouvons désormais contrôler notre moteur en PWM (Pulse Width Modulation) ou MLI (Modulation par Largeur d'Impulsion) ce qui nous permettra de contrôler sa fem (force électromotrice) donc sa vitesse, à l'aide d'un signal TOR (Tout Ou Rien) en modulant la largeur temporelle de l'impulsion (de période fixe).
<br><br> Nous voyons sur ce signal :
* Une fréquence de 20Hz (donc la période = 1/20 = 50ms)
* Un rapport cyclique de ~60%
* Une tension minimale de 0V
* Une tension maximale de 3.3V

On en déduit alors une valeur moyenne d'environ 0.6 * (3.3 - 0) = 1.98V, ce qui correspondera à la tension perçue par le moteur du ventilateur puisque le moteur DC peut-être assimilé à un filtre passe-bas grâce à sa bobine.

## 🇫🇷 Analyse du système : moteur DC, servant de ventilateur

[Le dossier "Step response" contient le code cette partie d'analyse](Step_response)

Pour analyser le système que nous avons, nous allons en entrée du système mettre un échelon et voir alors sa réponse. Concrètement, nous allons laisser monter en température la RPi4 avec une utilisation CPU constante jusqu'à ce qu'elle tende vers une température constante puis nous allons activer le ventilateur avec une commande maximale.

![Réponse indicielle du moteur DC](doc/Step_response.png)

Voici les résultats que j'ai obtenu avec mon ventilateur sur 5 cycles. 

Le but d'avoir 5 cycles est d'avoir une meilleure précision sur la lecture du temps de réponse du système, qui nous indique son comportement. 

![Temps de réponse d'un système du premier ordre](doc/1st_order_response.png)

Il faut savoir qu'à partir de l'analyse de ces réponses indicielles, nous pouvons assimiler notre moteur DC à un système du premier ordre puisqu'il n'a pas de dépassement de sa valeur finale. Voici alors les temps de réponse que j'ai obtenu sur les 5 cycles :

![Temps de réponse des 5 cycles](doc/Response_times.png)

## 🇫🇷 Calcul de la FTBO, de la FTBF avec le choix d'un correcteur PI
En pratique, l'ajout d'une action dérivée dans un correcteur PID n'est pas toujours nécessaire pour asservir un moteur DC. Dans de nombreux cas, un correcteur PI bien réglé peut fournir des performances satisfaisantes.

Voici alors le diagramme bloc que nous obtenous pour système en boucle fermée :

![Diagramme bloc du système en boucle fermée](doc/Block_diagram.png)

Pour le détail des calculs, à partir de la Fonction de Transfert en Boucle Ouverte (FTBO) et de la Fonction de Transfert en Boucle Fermée (FTBF), pour obtenir les coefficients de ki et kp de notre correcteur PI, je vous laisse consulter mes calculs sur [ma feuille de calcul](doc/Calculation_sheet.pdf).

## 🇫🇷 Réalisation d'un programme C++ complet pour l'asservissement
Il faut maintenant passer sur la partie programmation ! J'ai alors réalisé [un programme C++](PID_control_ventilator_analysis/main.cpp) permettant de réaliser le correcteur PID, de lire la température du CPU, de lire l'utilisation du CPU (en %) et d'enregistrer les performances du correcteur PI dans un fichier CSV. Ce fichier CSV peut alors être analysé par un petit script Python que j'ai réalisé sur [un Jupyter Notebook](PID_control_ventilator_analysis/Analyse.ipynb). Voici les performances obtenues :

![Performances du correcteur PI](doc/PI_perfs.png)

**Remarques** : 
* La consigne de température varie en fonction du pourcentage d'utilisation du CPU puisque le ventilateur n'a pas la capacité à tenir une température basse avec une utilisation du CPU élevée.
* Le ventilateur n'agit que sur la décroissance de la température du CPU, ce qui implique nous jouons avec l'inertie thermique du CPU pour la croissance de la température du CPU !

Dans le code, j'ai utilisé différentes boucles "for" imbriquées. 
* La première permet de ne pas effectuer trop de vidages du vecteur dans le fichier CSV dans un laps de temps, ce qui permet de diminuer significativement l'utilisation du CPU
* La seconde permet de scanner 3 fois moins souvent l'utilisation du CPU qu'une période d'application d'une valeur PWM, cela permet de ne pas changer la consigne de température trop vite, et donc que l'action intégrale ne s'emballe pas sur une impulsion. 
* La troisième permet de lire 10 fois plus vite la température du CPU (puis de faire la moyenne), pour filtrer les erreurs de mesure de la sonde de température du CPU.

### Partie de tests
#### 1. Asservissement :
Garder une utilisation CPU basse (peu de perturbations) et analyser les performances du système en statique (erreur statique) et en dynamique (temps de réponse, dépassement, stabilité).
#### 2. Régulation :
Augmenter l'utilisation du CPU, sur les différentes plages d'utilisation du CPU (0-25%, 25-50%, 50-75% et 75-100%), pour créer des perturbations. J'ai pu effectuer l'augmentation de l'utilisation du CPU en lancement un ou plusieurs scripts de machine learning sur la RPi4, ces scripts viennent de [ce dépôt](https://github.com/ageron/handson-ml3.git) lié au livre d'Aurélien Géron : [Hands-On Machine Learning with Scikit-Learn, Keras, and TensorFlow](https://www.oreilly.com/library/view/hands-on-machine-learning/9781098125967/), pour apprendre le ML. 

Analyser également les performances du système en statique (erreur statique) et en dynamique (temps de réponse, dépassement, stabilité).

C'est sur cette partie que nous allons, par tatonnement, ajuster les 2 coefficients de notre correcteur PI pour que le système en boucle fermée réponde à nos exigences !

## 🇫🇷 Réalisation du programme final
Pour réaliser ce programme final, qui fonctionnera en continu sur notre RPi, nous allons reprendre le code complet réalisé précédemment, y supprimer la partie enregistrement dans un fichier CSV et y rajouter une condition d'activation de l'asservissement que s'il est entre 7h et 00h (puisque je dors après) !

## 🇫🇷 Création du service "ventilateur.service" sur la RPi
Après compilation du programme C++ final, nous pouvons alors déplacer notre fichier binaire dans notre répertoire /opt (dédié pour cela), les 2 dernières commandes permettent de renommer le fichier binaire :
```
> ~/PID_control_RPi_cooling/PID_control_ventilator $ mv main.bin /opt 
> ~/PID_control_RPi_cooling/PID_control_ventilator $ cd /opt 
> /opt $ mv main.bin PI_ventilateur.bin
```
Nous pouvons alors créé notre service :
```
> ~ $ sudo nano /etc/systemd/system/ventilateur.service
```
Il faut alors éditer votre service (exemple : le mien) :
```
[Unit]
Description=Run Ventilator controlled with PI
After=multi-user.target

[Service]
ExecStart=/opt/PI_ventilateur.bin
Restart=always

[Install]
WantedBy=multi-user.target
```
Puis l'enregistrer (Ctrl+X puis taper "y" puis appuyer 2 fois sur la touche "Entrée")

Nous pouvons alors activer notre service, et le mettre en service :
```
> ~ $ systemctl enable ventilateur.service
> ~ $ systemctl start ventilateur.service
```

Pour connaître l'état de notre service :
```> ~ $ systemctl status ventilateur.service```

Pour arrêter notre service : 
```> ~ $ systemctl stop ventilateur.service```

**En espérant que vous ayez trouvé cette description complète et concise et que le projet vous a plu !**

---
***
___
<br>










# 🇬🇧 Controlling and regulating the cooling of a RPi with a PID controller

The aim of this project is to control and regulate the cooling of a RPi. Here is the kit I bought: [RPi4 kit purchased](https://www.amazon.fr/Bqeel-Alimentation-Interrupteur-Ventilateur-Dissipateur/dp/B082PSBBMM/ref=sr_1_14?dib=eyJ2IjoiMSJ9.8ZfM4XGhIburiNAfEtgKbJFrU5zvWGZoBZ-6HRG4gFjt2i8pjqskQJPU_KjPN7vEdm6KOWqjWq_iFT-oObGvz2xWPGZmdHsSbrJnYXjrB3F7Ad_vnLYooypQqPPFwqQe7gGHJ57mA7NC30jQawqD7L142dfsjkJJnwaJw5LuD1YDDkUX0XS7CnVAGJchgjCAU4LaBSgiruQE6XINpymChrsZDNNicJMLEG-OJog1AwlwJedFPN9K_zxn6UqhTWdGk9F0qHX4Xsbf0k8BBapWVNJrBmAnePdH5uOXKM0Qf_0.QHt9RVqp2MEe4xmj7bZPu1lMkk6QZU34N2WpOzn6x-8&dib_tag=se&keywords=rpi4&qid=1718524926&sr=8-14&th=1).

![RPi4 kit image from Amazon](doc/RPi4_kit.png)

## 🇬🇧 Design of a fan control module

It should be noted that the DC motor of the fan consumes more current at full power than the GPIO (General Purpose Input Output) of the Raspberry Pi 4 can provide. A small electronic circuit must therefore be designed for power adaptation:

![Electronic circuit for fan control](doc/Hardware_schematic.png)

The diode is placed here as a freewheeling diode to prevent a counter-electromotive force that could damage the surrounding components, particularly the bipolar transistor.
*"The freewheeling diode plays a crucial role in protecting electronic circuits by providing a path of least resistance for the induced current when stopping an electromagnetic coil, thus preventing damage caused by voltage spikes."*

![Example of a PWM signal](doc/PWM_example.png)

We can now control our motor using PWM (Pulse Width Modulation) or PWM (Pulse Width Modulation), which will allow us to control its EMF (electromotive force) and therefore its speed using a TOR (All or Nothing) signal by modulating the temporal width of the pulse (of fixed period).
We can see on this signal:

* A frequency of 20Hz (therefore the period = 1/20 = 50ms)
* A duty cycle of ~60%
* A minimum voltage of 0V
* A maximum voltage of 3.3V

We can then deduce an average value of approximately 0.6 \* (3.3 - 0) = 1.98V, which will correspond to the voltage perceived by the fan motor since the DC motor can be assimilated to a low-pass filter thanks to its coil.

## 🇬🇧 Analysis of the system: DC motor used as a fan

[The "Step response" folder contains the code for this analysis part](Step_response)

To analyze the system we have, we will put a step in input of the system and see its response. Concretely, we will let the RPi4 heat up with a constant CPU usage until it tends towards a constant temperature then we will activate the fan with a maximum command.

![Step response of the DC motor](doc/Step_response.png)

Here are the results I obtained with my fan over 5 cycles.

The aim of having 5 cycles is to have a better precision on the reading of the response time of the system, which indicates its behavior.

![Response time of a first order system](doc/1st_order_response.png)

It should be noted that from the analysis of these step responses, we can assimilate our DC motor to a first order system since it does not have an overshoot of its final value. Here are the response times I obtained over the 5 cycles:

![Response times of the 5 cycles](doc/Response_times.png)

## 🇬🇧 Calculation of the FTBO, FTBF with the choice of a PI controller

In practice, adding a derivative action in a PID controller is not always necessary to control a DC motor. In many cases, a well-tuned PI controller can provide satisfactory performance.

Here is the block diagram we obtain for the closed-loop system:

![Block diagram of the closed-loop system](doc/Block_diagram.png)

For the detail of the calculations, from the Open-Loop Transfer Function (FTBO) and the Closed-Loop Transfer Function (FTBF), to obtain the ki and kp coefficients of our PI controller, I invite you to consult my calculations on [my calculation sheet](doc/Calculation_sheet.pdf).

## 🇬🇧 Design of a complete C++ program for control

We now need to move on to the programming part! I have therefore designed [a C++ program](PID_control_ventilator_analysis/main.cpp) that allows to implement the PID controller, to read the CPU temperature, to read the CPU usage (in %) and to record the performances of the PI controller in a CSV file. This CSV file can then be analyzed by a small Python script that I have designed on [a Jupyter Notebook](PID_control_ventilator_analysis/Analyse.ipynb). Here are the performances obtained:

![Performances of the PI controller](doc/PI_perfs.png)

**Remarks**:

* The temperature setpoint varies depending on the CPU usage percentage since the fan does not have the capacity to maintain a low temperature with high CPU usage.
* The fan only acts on the decrease of the CPU temperature, which implies that we are playing with the thermal inertia of the CPU for the increase of the CPU temperature!

In the code, I used different nested "for" loops.

* The first one allows not to perform too many vector empties in the CSV file in a short time, which significantly reduces CPU usage.
* The second one allows to scan the CPU usage 3 times less often than a PWM value application period, which allows not to change the temperature setpoint too quickly, and therefore that the integral action does not go crazy on an impulse.
* The third one allows to read the CPU temperature 10 times faster (and to make the average), in order to filter the measurement errors of the CPU temperature sensor.

### Test part

#### 1. Control:

Keep a low CPU usage (few disturbances) and analyze the performance of the system in static (static error) and dynamic (response time, overshoot, stability).

#### 2. Regulation:

Increase the CPU usage, on the different CPU usage ranges (0-25%, 25-50%, 50-75% and 75-100%), to create disturbances. I was able to increase the CPU usage by launching one or more machine learning scripts on the RPi4, these scripts come from [this repository](https://github.com/ageron/handson-ml3.git) linked to Aurélien Géron's book: [Hands-On Machine Learning with Scikit-Learn, Keras, and TensorFlow](https://www.oreilly.com/library/view/hands-on-machine-learning/9781098125967/), to learn ML.

Also analyze the performance of the system in static (static error) and dynamic (response time, overshoot, stability).

This is the part where we will adjust the 2 coefficients of our PI controller by trial and error so that the closed-loop system meets our requirements!

## 🇬🇧 Design of the final program

To design this final program, which will run continuously on our RPi, we will take the complete code designed previously, remove the part of recording in a CSV file and add a condition of activation of the control only if it is between 7h and 00h (since I sleep after)!

## 🇬🇧 Creation of the "ventilateur.service" service on the RPi

After compiling the final C++ program, we can then move our binary file to our /opt directory (dedicated for this), the last two commands allow to rename the binary file:
```
> ~/PID_control_RPi_cooling/PID_control_ventilator $ mv main.bin /opt
> ~/PID_control_RPi_cooling/PID_control_ventilator $ cd /opt
> /opt $ mv main.bin PI_ventilateur.bin
```
We can then create our service:
```
> ~ $ sudo nano /etc/systemd/system/ventilateur.service
```
You must then edit your service (example: mine):
```
[Unit]
Description=Run Ventilator controlled with PI
After=multi-user.target

[Service]
ExecStart=/opt/PI_ventilateur.bin
Restart=always

[Install]
WantedBy=multi-user.target
```
Then save it (Ctrl+X then type "y" then press twice the "Enter" key)

We can then enable our service, and put it into service:
```
> ~ $ systemctl enable ventilateur.service
> ~ $ systemctl start ventilateur.service
```

To know the status of our service:
```> ~ $ systemctl status ventilateur.service```

To stop our service:
```> ~ $ systemctl stop ventilateur.service```

**Hoping that you found this description complete and concise and that the project pleased you!**