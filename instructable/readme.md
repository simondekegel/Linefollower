# Instructable

Een instructable is een stappenplan - zonder verdere uitleg - hoe je vertrekkend van de bill of materials en gebruik makend van de technische tekeningen de robot kan nabouwen. Ook de nodige stappen om de microcontroller te compileren en te uploaden staan beschreven.  

### stap 0
Weet wat je doet, kopieer niet gewoon van een iemand anders. Maak zelf je schema's en denk zelf na over wat je doet.
Blind kopieeren lijdt tot fouten en niet weten hoe je deze weg werkt. 
& CHATGPT CODE TREKT OP NIE VEEL!!!!!

### stap 1
bestel alle componenten uit de bill of materials, bestel zo veel mogelijk componenten van dezelfde website om de verzend kosten zo laag mogelijk te houden (meeste zijn beschikbaar op tinytronics (hc-08, sensor, motordrive, motoren & arduino nano).  

### stap 2
Als je een arduino nano clone hebt gekocht zal je bij elke opstart van het ardiuni.ide programma de com poort moeten aanduiden waar de nano op zit en dan in de lijst van mogelijke microcontrollers de juiste versie van de nano kiezen, ik heb een gewoone arduino nano gebruikt.

### stap 3-1
Maak test programmas en test elk componenten voor een goede correcte werking. 
zorg dat de sensoren een duidelijk verschil geven in een reflectie van wit en zwart, dat de motordrive werkt (vergeet zeker niet de EEP pin aan 5V te hangen) met pwm en zorg dat de hc-08 kan schrijven en ontvangen (als deze aan de rx & tx pin wordt gekoppeld moet je geen software matige serial comminicatie programmeren, ENKEL DE BAUDRATE INSTELLEN).


### stap 3-2
Maak een test opstelling op een breadbord, de motoren worden hier vervangen door leds omdat de arduino niet genoeg stroom kan leveren voor alles telaten werken.
Als alles op het breadbord staat voeg je langzaam alle test programmas samen. deze moeten nog niet samen werken om de Led's te sturen maar dit is vooral een test of de arduino alles kan lezen en schrijven wat er in de normaale werking van het hoofdprogramma zou gebeuren. 
(lijkt omslagtig maar was een verzekering dat de arduno nano kon doen wat we wouden).

### stap 4
Voor studenten: BEKIJK HET LEERPAD!!!!

Voor het programma zullen we starten met de variabele toevoegen uit de test programmas. Deze zullen de PWM pinnen: 3, 5, 6, 11. Sensor pinnen: A0 ->A7. 
Led pin 12 & knop pin 13. rx en tx moeten niet toegevoegd worden zolang de hc-08 op deze pinnen zit.

In de loop zal je moeten werken met een non blokking wait die zal ingesteld worden aan de hand van een variable genaamd cyclustime. alles in de loop wordt in de context van deze non blokking wait geprogrammeerd. deze zal de snelheid van het programma bepalen.
