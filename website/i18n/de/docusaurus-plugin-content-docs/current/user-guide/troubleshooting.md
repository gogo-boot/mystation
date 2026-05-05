# Hilfe & Problemlösung

Etwas funktioniert nicht? Diese Seite führt dich durch die häufigsten Probleme und deren Lösungen.

---

## WLAN & Netzwerkprobleme

### Der MyStation-WLAN-Hotspot erscheint nicht

**Symptome:** Du kannst `MyStation-XXXXXXXX` nicht in der WLAN-Liste deines Handys finden.

**Lösungen:**

1. **Noch etwas länger warten** — Das Gerät braucht bis zu 60 Sekunden zum Starten
2. **Überprüfen, ob das Gerät Strom hat** — Falls der Akku leer sein könnte, lade das Gerät über USB auf und versuche es
   erneut
3. **Anderes Handy oder Computer versuchen** — Manche Geräte haben Probleme, bestimmte WLAN-Netzwerke zu sehen
4. **Sicherstellen, dass du nach einem 2,4 GHz-Netzwerk suchst** — Falls dein Handy auf „Nur 5 GHz" eingestellt ist,
   ändere es auf „Automatisch"

---

### MyStation verbindet sich nicht mit deinem Heimnetzwerk

**Symptome:** Nach der Eingabe der WLAN-Daten schlägt die Verbindung weiterhin fehl.

**Lösungen:**

1. **WLAN-Passwort überprüfen** — Es unterscheidet Groß- und Kleinschreibung: `Passwort1` ist anders als `passwort1`
2. **Sicherstellen, dass es ein 2,4 GHz-Netzwerk ist** — 5 GHz wird nicht unterstützt
3. **Signalstärke prüfen** — Das Gerät näher an den Router stellen und erneut versuchen
4. **Router-Typ prüfen** — MyStation funktioniert mit normalen Heimroutern. Es funktioniert nicht mit Hotel-WLAN,
   öffentlichem WLAN mit Anmeldeseiten oder Firmennetzwerken mit besonderer Sicherheit

---

### Verbunden, aber Einstellungsseite nicht erreichbar

**Symptome:** Das Gerät ist mit dem WLAN verbunden, aber `http://mystation.local` oder die IP-Adresse öffnet sich nicht.

**Lösungen:**

1. **Sicherstellen, dass das Handy im Heim-WLAN ist, nicht im mobilen Internet** — Zuerst zum WLAN wechseln
2. **IP-Adresse direkt versuchen** — Die aktuelle IP-Adresse des Geräts in der Display-Fußzeile prüfen und im Browser
   öffnen (z.B. `http://192.168.1.42`)
3. **Anderen Browser versuchen** — Manche Browser blockieren lokale Adressen
4. **Router hat möglicherweise Geräteisolierung aktiviert** — Diese Einstellung verhindert, dass Geräte im gleichen
   Netzwerk miteinander kommunizieren. Router-Einstellungen prüfen oder Internetanbieter kontaktieren

---

## Anzeigeprobleme

### Display aktualisiert sich nicht

**Symptome:** Der Bildschirm zeigt alte Daten und aktualisiert sich nicht.

**Lösungen:**

1. **Gerät neu starten** — Den Einschalter auf AUS schieben, einige Sekunden warten, dann wieder auf EIN schieben.
   Dieses Gerät hat einen eingebauten Akku; der Einschalter ist der richtige Weg, es neu zu starten.
2. **WLAN prüfen** — Sicherstellen, dass das Gerät noch mit dem Heim-WLAN verbunden ist (2,4 GHz)
3. **Akkustand prüfen** — Ein leerer Akku kann dazu führen, dass Updates aufhören. Das Gerät über USB laden

---

### Display zeigt „Geisterbilder"

**Symptome:** Ein schwacher Schatten eines früheren Bildes ist noch sichtbar.

**Lösungen:**

1. **Das ist normal bei E-Paper-Bildschirmen** — Die Anzeigetechnologie kann schwache Spuren hinterlassen, die mit der
   Zeit verschwinden
2. Das Gerät führt periodisch eine vollständige Bildschirmaktualisierung durch, um diese Geister zu beseitigen

---

## Strom & Akku

### Akku wird zu schnell leer

**Lösungen:**

1. **Aktualisierungsintervall erhöhen** — Von alle 5 Minuten auf alle 10 oder 15 Minuten für Abfahrten wechseln, und von
   1 Stunde auf 3 Stunden für Wetter
2. **Schlafplan einrichten** — Ruhestunden konfigurieren (z.B. `22:30` bis `05:30`), damit das Gerät nachts nicht
   aktualisiert
3. **Näher am Router aufstellen** — Ein schwaches WLAN-Signal verbraucht deutlich mehr Strom
4. **Akkugesundheit prüfen** — Ältere Akkus halten weniger Ladung. Bei Bedarf ersetzen

---

### Gerät schaltet sich nicht ein

**Lösungen:**

1. **Akku laden** — Das Gerät an einen USB-Ladegerät anschließen und mindestens 30 Minuten warten. Der Akku ist fest
   eingebaut und kann nicht leicht entnommen oder ersetzt werden.
2. **Anderes USB-Kabel oder Ladegerät versuchen** — Manche Kabel oder Ladegeräte liefern nicht genug Strom. Ein
   bekanntes funktionierendes Kabel und ein Wandadapter verwenden (nicht nur ein Computer-USB-Port).
3. **Einschalter auf EIN schieben** — Nach dem Laden den Einschalter in die EIN-Position schieben

---

## Konfigurationsprobleme

### Konfigurierter Anzeigemodus wird nicht übernommen

**Symptome:** Du hast „Wetter Vollbild" oder „Abfahrt Vollbild" in den Einstellungen ausgewählt,
aber das Gerät zeigt nach dem Neustart immer Halb & Halb an.

**Lösungen:**

1. **Konfigurationsmodus erneut öffnen und neu speichern**
    - Taste 1 für 5 Sekunden halten
    - Sicherstellen, dass der richtige Anzeigemodus ausgewählt ist
    - „Einstellungen speichern" drücken
2. **Auf ein Softwareupdate prüfen** — Taste 3 für 5 Sekunden halten, um ein Update auszulösen.
   Dieses Problem ist in aktuellen Softwareversionen behoben.

---

### Display zeigt falsche Uhrzeit (1 Stunde vor)

**Symptome:** Die in der Fußzeile angezeigte Uhrzeit ist 1 Stunde vor der tatsächlichen Zeit.
Dies passiert tendenziell nach mehreren Aktualisierungszyklen ohne vollständigen Neustart.

**Lösungen:**

1. **Gerät neu starten** — Den Einschalter auf AUS schieben, dann zurück auf EIN. Das Gerät synchronisiert die Uhrzeit
   bei jedem Neustart. Dieses Problem kann rund um die Zeitumstellungen im März und Oktober auftreten.

---

### Einstellungen werden nicht gespeichert / werden zurückgesetzt

**Lösungen:**

1. **Sicherstellen, dass „Einstellungen speichern" geklickt wurde** — Nach einer Bestätigungsmeldung suchen
2. **Während des Speicherns nicht ausschalten** — Auf die Bestätigung warten, bevor der Strom unterbrochen wird
3. **Werksreset versuchen** — Falls Einstellungen nie gespeichert werden, muss möglicherweise der interne Speicher
   gelöscht werden. Taste 1 + Taste 2 für 5 Sekunden halten, um einen Werksreset durchzuführen, dann neu einrichten

---

## Datenprobleme

### Wetter wird nicht angezeigt

**Symptome:** Der Wetterbereich zeigt `---` oder gar keine Informationen.

**Lösungen:**

1. **Überprüfen, ob der Standort korrekt eingestellt ist** — Konfigurationsmodus öffnen und Standort überprüfen
2. **Internetverbindung prüfen** — Mit einem anderen Gerät prüfen, ob der Router Internetzugang hat
3. **Hinweis:** MyStation verwendet den Deutschen Wetterdienst (DWD), der nur Deutschland abdeckt. Für Standorte
   außerhalb Deutschlands werden keine Wetterdaten angezeigt.

---

### Abfahrten werden nicht angezeigt

**Symptome:** Der Abfahrtsbereich ist leer oder zeigt keine Einträge.

**Lösungen:**

1. **Überprüfen, ob eine Haltestelle ausgewählt ist** — Konfigurationsmodus öffnen und Haltestelle prüfen
2. **Uhrzeit prüfen** — Sehr früh morgens oder spät abends gibt es möglicherweise tatsächlich keine Abfahrten
3. **Verkehrsmittelfilter prüfen** — Wenn alle Verkehrsmitteltypen deaktiviert sind, wird nichts angezeigt. Mindestens
   einen Typ aktivieren (Bus, S-Bahn usw.)
4. **Internetverbindung prüfen** — Abfahrten erfordern eine funktionierende Internetverbindung

---

### Softwareupdate fehlgeschlagen

**Symptome:** Das Gerät ist neu gestartet, aber die Softwareversion hat sich nicht geändert.

**Lösungen:**

1. **Manuelles Update auslösen** — Taste 3 für 5 Sekunden halten, um eine Update-Prüfung zu erzwingen
2. **Internetverbindung prüfen** — Das Update erfordert eine funktionierende Internetverbindung
3. **Bis zur nächsten Nacht warten** — Das Gerät versucht es automatisch erneut

> 💡 **Warum Updates wichtig sind:** Die Dienste, die Wetter- und Abfahrtsdaten liefern, ändern
> gelegentlich, wie sie ihre Daten formatieren. Ein Softwareupdate ist notwendig, damit MyStation
> kompatibel bleibt. Wenn Updates deaktiviert sind und sich das Datenformat ändert, können
> Wetter- oder Abfahrtsinformationen aufhören zu erscheinen.

---

## Werksreset

Verwende dies, wenn das Gerät völlig falsch konfiguriert ist oder du von vorne anfangen möchtest.

> ⚠️ **Alle Einstellungen werden dauerhaft gelöscht.** Du musst das Gerät von Anfang an neu einrichten.

**Schritte:**

1. **Taste 1 + Taste 2 gleichzeitig** für **5 Sekunden** gedrückt halten
2. Den Reset auf dem Display bestätigen
3. Dem [Schnellstart](quick-start.md) folgen, um das Gerät neu einzurichten

