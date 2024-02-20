Import("env")

import glob
import re
from serial.tools.list_ports import comports
import subprocess

# Fonction exécutée avant l'upload
def before_upload(source, target, env):
    print("Lecture du Chip ID...")

    try:
        # print("Forcer le reset de l'ESP32...")
        # subprocess.run("esptool.py --chip esp8266 --port /dev/cu.usbserial* --before no_reset erase_flash")

        
        output = subprocess.check_output(
            f"esptool.py --chip esp8266 --port /dev/cu.usbserial* chip_id", 
            shell=True,
            text=True
        )

        # Utilise des expressions régulières pour extraire MAC et Chip ID
        mac_match = re.search(r"MAC:\s+([0-9a-fA-F:]+)", output)
        chip_id_match = re.search(r"Chip ID:\s+0x([0-9a-fA-F]+)", output)

        if mac_match and chip_id_match:
            mac_address = mac_match.group(1)
            chip_id = chip_id_match.group(1)
            
            print(f"====> MAC Address: {mac_address}")
            print(f"====> Chip ID: {chip_id}")
        else:
            print("Impossible d'extraire les informations. Veuillez vérifier la sortie de la commande.")

    except subprocess.CalledProcessError as e:
            print(f"Erreur lors de l'exécution de la commande esptool.py : {e}")



env.AddPreAction("upload", before_upload)