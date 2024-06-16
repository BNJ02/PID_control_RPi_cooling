root@iPad:/home/bnj/RPi_prog/Blink_pin/C# ./main 2024-05-19 02:01:31 initInitialise: Can't lock /var/run/pigpio.pid

Comment forcer l'arrêt du process sachant que je viens de redémarrer ma RPi ?

Il semble que vous rencontriez un problème avec la bibliothèque pigpio sur votre Raspberry Pi. Le message d'erreur indique qu'il est impossible de verrouiller le fichier pid, ce qui peut arriver si le processus pigpio est toujours en cours d'exécution ou s'il s'est arrêté de manière inattendue.

Vous pouvez essayer les étapes suivantes pour résoudre ce problème :

Vérifiez si le processus pigpio est en cours d'exécution en utilisant la commande suivante :

```ps aux | grep pigpio```

Si le processus est en cours d'exécution, vous pouvez essayer de l'arrêter en utilisant la commande suivante :

```sudo killall pigpio```

Supprimez le fichier pid de pigpio en utilisant la commande suivante :

```sudo rm /var/run/pigpio.pid```

Redémarrez le service pigpio en utilisant la commande suivante :

```sudo systemctl restart pigpio```

Si vous rencontrez toujours des problèmes, vous pouvez essayer de réinstaller la bibliothèque pigpio en utilisant les commandes suivantes :

```sudo apt-get remove --purge pigpio```
```sudo apt-get update```
```sudo apt-get install pigpio```

Ensuite, redémarrez votre Raspberry Pi et essayez à nouveau d'exécuter votre programme.

root@iPad:/home/bnj/RPi_prog/Blink_pin/C# ./main
2024-05-19 02:13:25 initInitialise: bind to port 8888 failed (Address already in use)

Le message d'erreur "bind to port 8888 failed (Address already in use)" indique que le port 8888 est déjà utilisé par un autre processus sur votre Raspberry Pi.

Vous pouvez utiliser la commande suivante pour afficher les processus en cours d'exécution et leurs numéros de port associés :

```netstat -tuln```

Cherchez dans la liste le processus qui utilise le port 8888 et notez son numéro de processus (PID).

Ensuite, vous pouvez arrêter le processus en utilisant la commande suivante :

```sudo kill <PID>```

Remplacez <PID> par le numéro de processus que vous avez noté précédemment.

Si vous ne pouvez pas arrêter le processus ou si vous ne voulez pas l'arrêter, vous pouvez modifier le code de votre programme pour utiliser un autre port disponible.

Une fois que vous avez arrêté le processus qui utilise le port 8888 ou que vous avez modifié le code de votre programme pour utiliser un autre port, vous pouvez essayer à nouveau d'exécuter votre programme.

Où est son PID ?

root@iPad:/home/bnj/RPi_prog/Blink_pin/C# netstat -tuln
Active Internet connections (only servers)
Proto Recv-Q Send-Q Local Address           Foreign Address         State      
tcp        0      0 127.0.0.1:631           0.0.0.0:*               LISTEN     
tcp        0      0 0.0.0.0:514             0.0.0.0:*               LISTEN     
tcp        0      0 0.0.0.0:22              0.0.0.0:*               LISTEN     
tcp        0      0 127.0.0.1:39713         0.0.0.0:*               LISTEN     
tcp6       0      0 :::5900                 :::*                    LISTEN     
tcp6       0      0 :::514                  :::*                    LISTEN     
tcp6       0      0 :::8888                 :::*                    LISTEN

Vous pouvez utiliser la commande grep pour filtrer la sortie de netstat et afficher uniquement la ligne contenant le port 8888, ainsi que le PID associé. Essayez la commande suivante :

```netstat -tuln | grep ':8888'```

Cela devrait afficher une ligne similaire à celle-ci :

tcp6       0      0 :::8888                 :::*                    LISTEN      1234/myprogram

Le numéro après le mot LISTEN est le PID du processus qui utilise le port 8888. Dans cet exemple, le PID est 1234. Remplacez-le par le PID que vous voyez dans la sortie de la commande netstat.

Je n'ai pas de PID :

root@iPad:/home/bnj/RPi_prog/Blink_pin/C# netstat -tuln | grep ':8888'
tcp6       0      0 :::8888                 :::*                    LISTEN     
root@iPad:/home/bnj/RPi_prog/Blink_pin/C# 

Si vous ne voyez pas de PID associé au port 8888, cela peut signifier que le processus qui utilise ce port s'est arrêté de manière inattendue ou qu'il a été tué sans que le port soit libéré.

Dans ce cas, vous pouvez essayer de libérer manuellement le port en utilisant la commande suivante :

```fuser -k 8888/tcp```

Cette commande devrait tuer tous les processus qui utilisent le port 8888 TCP.

Si cela ne fonctionne pas, vous pouvez essayer de redémarrer le service réseau de votre Raspberry Pi en utilisant la commande suivante :

```sudo systemctl restart networking```

Cela devrait libérer tous les ports réseau et vous permettre de réutiliser le port 8888.

Si vous rencontrez toujours des problèmes, vous pouvez essayer de redémarrer complètement votre Raspberry Pi en utilisant la commande suivante :

```sudo reboot```

Une fois que votre Raspberry Pi a redémarré, vous devriez pouvoir utiliser le port 8888 sans rencontrer d'erreurs.