import numpy as np
import pandas as pd

# Lire le fichier CSV
df = pd.read_csv('5_cycles.csv')

# Extraire la colonne à lisser
colonne = df['CPU Temperature (°C)']

# Définir le filtre de lissage
taille_filtre = 61  # Taille impaire du filtre (par exemple, 3, 5, 7)
filtre = filtre = np.ones(taille_filtre) / taille_filtre

# Appliquer le filtre de lissage à la colonne
colonne_lissee = np.convolve(colonne, filtre, mode='same')

# Remplacer la colonne d'origine par la colonne lissée arrondie à la décimale
df['CPU Temperature lissée (°C), taille filtre = ' + str(taille_filtre)] = colonne_lissee.round(decimals=1)

# Écrire le résultat dans un nouveau fichier CSV
df.to_csv('5_cycles_smoothed.csv', index=False)
