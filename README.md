# Abschlussprojekt SoftSkills
Abschlussprojekt für das Modul SoftSkills an der Carl-von-Ossietzky-Universität Oldenburg.

MED (Modular Entrance Devices) stellen verschiedene Funktionen rund um die Haustür bereit:

So können sich Nutzer auf verschiedene Arten (per Sound, Telegram oder Discord) benachrichtigen lassen,
wenn jemand klingelt, einen Brief einwirft oder die Haustür geöffnet bzw. geschlossen wird. 
Diese Events können darüber hinaus auch in einer Historientabelle gespeichert werden, 
um sie auch im Nachhinein noch genau nachvollziehen zu können.
Zusätzlich kann auch in die andere Richtung kommuniziert werden,
in dem der Benutzer via Website oder Messenger eine Nachricht an ein im Gerät enthaltenes Display schicken kann, welches diese dann anzeigt.
Alle Events sowie die Aktionen, die auf Sie folgen, können schnell über eine Website zentral betrachtet bzw. konfiguriert werden



Erstellt von Hagen Eickhoff, Jamal Frerichs, Jonas Köhler und Marcel Oppermann.
 
Ausführliche Dokumentation im [WordPress-Blog](https://wp.uni-oldenburg.de/soft-skills-und-technische-kompetenz-wise20202021-sosepg-2/).

### Aufbau des Repositories
- /hardware
    - /3d_models
        - CAD-Modelle (Klingel-Case, RasPi-Case und Briefkasten-Case) für Cases für RasPi sowie beide Microcontroller
- /software
  - /NodeRED
    - Flows für NodeRED
  - /Detector
    - Code für Microcontroller, der eingehende Briefe erfasst und meldet
  - /Klingel
    - Code für Microcontroller, der Klingelknopf und Textdisplay bedient
  - install<span/>.sh
    - Installationsskript zum ersten Aufsetzen eines Raspberry Pi mit der benötigten Software
