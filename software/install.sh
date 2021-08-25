#!/bin/bash
    #TODO: - Check if more node-red packages are needed
    #      - Check if WIFI/Audio setup can be scripted
    #      - Test script x)
echo "Bitte stellen Sie sicher, dass sie bereits folgende Schritte erledigt haben:"
echo " - NodeRED installiert"
echo " - Systemupdates installiert (apt-get upgrade && apt-get upgrade)"
echo " - RaspberryPi-Setup abgeschlossen (Internet-Verbindung, Audio-Einstellungen)"

read -p "Haben Sie die Installationen durchgeführt (j/n)?" REPLY
echo 
if [[ $REPLY =~ ^[JjYy]$ ]]
then
    if [ $EUID -ne 0]
    then
        echo "Bitte führen Sie das script als root aus (sudo)."
        exit
    fi
    
    #install package to play sounds on RasPi directly
    echo "Installiere libasound2-dev..."
    sudo apt-get install libasound2-dev -y

    echo "Richte NodeRED als Autostart-Service ein und konfiguriere für RaspberryPi..."
    sudo systemctl enable nodered.service
    node-red-pi --max-old-space-size=256
    sudo systemctl start nodered.service

    echo "Installiere NodeRED-Module"
    cd ~/.node-red
    npm install node-red-contrib-speakerpi
    npm install node-red-contrib-discord
    npm install node-red-contrib-telegrambot --save
    npm install node-red-dashboard

    read
else
    echo "Bitte führen Sie die benötigten Installationen durch, bevor Sie fortfahren."
    exit
fi