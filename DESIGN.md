# Cache Simulator
## 1. Einleitung
### 1.1 Projektziel
Ziel dieses Projekts ist die Entwicklung eines Cache-Simulators, der die Funktionalität eines Dirty-Write-Back-Cache mit 
einem Write-Back-Cache mit einer LRU-Ersatzstrategie (Least Recently Used) und einer Write-Allocate-Strategie simuliert.
Diese Konfiguration soll es ermöglichen, die Leistung des Caches unter verschiedenen Bedingungen zu analysieren. 
Insbesondere sollen mit der Simulation verschiedene Metriken wie Cache-Hit- und Miss-Raten sowie die Auswirkungen von 
Write-Backs auf den Hauptspeicher analysiert werden. Die Simulation soll die Auswirkungen von Rückschreibungen auf den 
Hauptspeicher untersuchen, um den Einfluss von Cache-Parametern wie Assoziativität, Cache-Größe und Zeilengröße auf die 
Effizienz zu analysieren.

### 1.2 Cache-Architektur
 - Least Recently Used (LRU) - Ersetzungsstrategie: Bei dieser Strategie wird immer diejenige Cache-Zeile ersetzt, die 
   am längsten nicht benutzt worden ist.
 - Write-Allocate-Policy: Bei dieser Strategie wird, wenn auf eine Adresse geschrieben wird, die nicht im Cache enthalten 
   der entsprechende Block zuerst in den Cache geladen. Dies ermöglicht einen späteren Zugriff auf denselben Block 
   direkt aus dem Cache, wodurch teure Zugriffe auf den Hauptspeicher vermieden werden.
 - Write-Back-Cache: Bei einem Write-Back-Cache werden Änderungen zunächst nur im Cache gespeichert. Erst wenn ein Block 
   aus dem Cache entfernt wird (z.B. durch Ersetzen des Blocks), werden die Daten in den Hauptspeicher zurückgeschrieben. 
 - Cache-Organisator: Der Cache kann auf unterschiedliche Weise organisiert werden: 
   - Vollständig Assoziativ (Fully Associative): Es gibt nur ein Set, und jede Zeile kann an einer beliebigen Position 
     innerhalb des Caches gespeichert werden.
   - Direkt Abgebildet (Direct Mapped): Jede Speicheradresse ist genau einer Cache-Zeile zugeordnet.
   - Set-Assoziativ: Speicheradressen werden bestimmten Sets zugewiesen. Jedes Set kann eine konfigurierbare Anzahl von 
     Cache-Zeilen enthalten, je nach dem Grad der Assoziativität.
 - Die Assoziativität, die Cache-Größe und die Zeilen-Größe sind konfigurierbare Parameter, mit denen die Anzahl der 
   Sets und die Anzahl der Zeilen pro Set bestimmt werden.

### 1.3 Übersicht der Design-Entscheidungen
 - Cache-Struktur: Der Cache ist als eine Struktur aufgebaut, die eine Reihe von Cache-Sets enthält. Jedes Set besteht 
   aus mehreren Cache-Zeilen, abhängig vom gewählten Grad der Assoziativität.
 - CacheSet-Struktur: Ein Cache-Set enthält ein Array von Cache-Zeilen. Diese Struktur ermöglicht die Organisation und 
   Verwaltung von mehreren Cache-Zeilen innerhalb eines Sets.
 - CacheLine-Struktur: Eine Cache-Zeile enthält Informationen wie das Tag 
   (zur Identifizierung des zugehörigen Speicherblocks), eine is_valid-Flag, die anzeigt, ob die Daten gültig sind, 
   ein is_dirty-flag für geänderte Daten, die noch nicht in den Hauptspeicher zurückgeschrieben wurden, und eine 
   lru_order Variable, die zur Verwaltung der LRU-Reihenfolge verwendet wird.

## 2. Aufbau des Simulators
### 2.1 Strukturen
#### 2.1.1 CacheLine Struktur
Das CacheLine-Struct beschreibt eine einzelne Cache-Zeile (bzw. einen Block) im Cache. Jede Zeile enthält die folgenden Felder:
 - tag: Ein Wert, der den gespeicherten Inhalt identifiziert.
 - is_valid: Ein Flag, das angibt, ob der Inhalt der Zeile gültig und verwendbar ist.
 - is_dirty: Ein Flag, das anzeigt, ob der gespeicherte Inhalt geändert wurde und noch in den Hauptspeicher 
   zurückgeschrieben werden muss.
 - lru_order: Ein Wert, der die Reihenfolge der Verwendung angibt und bestimmt, wann die Zeile gemäß der LRU-Strategie 
   ersetzt wird.

#### 2.1.2 CacheSet Struktur
Das CacheSet-Struct stellt ein Set von Cache-Zeilen dar. Es enthält:
 - lines: Ein Array von CacheLine-Strukturen. Die Anzahl der Zeilen wird 
   durch den Assoziativitätsgrad des Caches bestimmt.

#### 2.1.3 CacheStatistik Struktur
Das CacheStats-Struct speichert Informationen über die Leistung des Caches. Es enthält:
 - cache_hits: Die Anzahl der Cache-Treffer (Hits), die während der Simulation auftreten.
 - cache_misses: Die Anzahl der Cache-Fehler (Misses), bei denen auf den Hauptspeicher zugegriffen werden musste.
 - dirty_write_backs: Die Anzahl der Schreiboperationen, bei denen geänderte Daten vom Cache in den Hauptspeicher 
   zurückgeschrieben werden mussten.

#### 2.1.4 Cache Struktur
Das Cache-Struct speichert die komplette Cache-Konfiguration und enthält:
 - associativity: Der Assoziativitätsgrad, d.h. die Anzahl der Cache-Zeilen pro Set.
 - cache_size: Die Gesamtgröße des Caches in Kilobyte.
 - line_size: Die Größe jeder Cache-Zeile in Bytes.
 - miss_penalty: Die Anzahl der Zyklen, die ein Cache-Miss verursacht.
 - dirty_write_back_penalty: Die Anzahl der Zyklen, die für ein Dirty-Write-Back benötigt werden.
 - num_sets: Die Anzahl der Sets im Cache, die durch die Cache-Größe, die Zeilengröße und die Assoziativität bestimmt 
   wird.
 - sets: Ein Array von CacheSets-Strukturen, das alle Sets des Caches enthält.
 - stats: Eine CacheStats-Struktur, die die Leistung des Caches in 
   Form von Hits, Misses und Write-Backs aufzeichnet.
 - log_line_size, log_num_sets: Die Logarithmen der Cache-Zeilen- und Set-Anzahl, die für 
   eine schnellere Adressberechnung vorab berechnet werden.

#### 2.1.5 CacheOp Struktur
Das CacheOp-Struct speichert die Informationen über eine einzelne Cache-Operation und enthält:
 - address_type: Der Typ des Zugriffs (Lesen oder Schreiben).
 - address: Die virtuelle Adresse, auf die zugegriffen wird.
 - instructions: Die Anzahl der Instruktionen, die während dieser Cache-Operation ausgeführt werden.

### 2.2 Funktionen
#### 2.2.1 Cache Initialisierungs- und Verwaltungsfunktionen
 - initialize_cache: Diese Funktion initialisiert den Cache basierend auf den übergebenen Konfigurationsparametern 
   (Größe, Assoziativität, Zeilengröße). Sie erstellt die Cache-Struktur und allokiert Speicher für die Sets und Zeilen.
 - free_cache: Diese Funktion gibt den für den Cache verwendeten Speicher frei.

#### 2.2.2 Cache-Zugriffsfunktionen
 - initialize_cache_operation: Diese Funktion initialisiert eine Cache-Operation (z.B. Lesen oder Schreiben) und 
   bereitet die notwendigen Parameter vor.
 - access_cache: Die Hauptfunktion für den Cache-Zugriff. Sie behandelt Cache-Hits, Cache-Misses und Write-Backs 
   entsprechend der gewählten Cache-Strategie.

#### 2.2.3 LRU Handling Funktionen
 - update_lru_order: Diese Funktion aktualisiert die LRU-Reihenfolge der Cache-Zeilen in einem Set, nachdem auf eine 
   Zeile zugegriffen wurde.
 - find_lru_line_index: Diese Funktion sucht die Zeile mit der ältesten Nutzung (basierend auf der LRU-Reihenfolge) und 
   gibt ihren Index zurück, um diese zu ersetzen.

#### 2.2.4 Adressverarbeitungsfunktionen
 - extract_set_index: Extrahiert den Set-Index aus einer gegebenen Adresse, um zu bestimmen, welches Set für den Zugriff 
   verwendet wird.
 - extract_tag_number: Extrahiert den Tag aus einer Adresse, um zu überprüfen, ob sich der entsprechende Block im Cache 
   befindet.

#### 2.2.5 Cache-Statistikfunktionen
get_cache_hits, get_cache_misses, get_dirty_write_backs: Diese Funktionen geben die aktuellen Werte der Cache-Statistiken 
zurück, um die Leistung des Caches zu analysieren.

#### 2.2.6 Utility Funktion
 - log2: Diese Funktion berechnet den Logarithmus zur Basis 2, der für die Cache-Adressenberechnung.

## 3. Funktionsweise des Programms
### 3.1 Cache Initialisierung
Der Cache wird durch die Funktion initialize_cache mit den Parametern 
(wie Assoziativität, Zeilen-Größe und Cache-Größe) initialisiert. Die Anzahl der Sets 
und die Assoziativität werden während der Initialisierung berechnet. Der Speicher für die Cache-Datenstrukturen, einschließlich 
die Cache-Sets und Cache-Zeilen, wird zugewiesen. Für jede Cache-Zeile werden die lru_order 
und die Dirty- und Valid-Flags für jede Cache-Zeile initialisiert. Die Tag-Werte werden auf 0 gesetzt, um leere Einträge zu markieren. 
Diese Initialisierungsstrategie gewährleistet eine effiziente Speicherverwaltung und bereitet den Cache für den Zugriff vor.

### 3.2 Cache-Zugriffe
Jede Cache-Operation wird von der Funktion initialize_cache_operation initialisiert und dann von access_cache verarbeitet. 
Der Ablauf einer Cache-Operation folgt diesen Schritten:
 - Zuerst wird der Set-Index und die Tag-Nummer aus der virtuellen Adresse extrahiert.
 - Anschließend werden die Cache-Lines des entsprechenden Sets durchsucht, um festzustellen, ob ein Cache-Hit oder Miss vorliegt:
   - Hit: Die Daten bleiben im Cache erhalten, und die LRU-Reihenfolge der betroffenen Cache-Line wird aktualisiert. 
     Falls es sich um eine Schreiboperation handelt, wird das Dirty-Flag gesetzt.
   - Miss: Wird ein Miss festgestellt, wird nach einer freien oder am wenigsten verwendeten (höchster LRU-Wert) Cache-Line 
     gesucht. Diese wird dann mit den neuen Daten beschrieben, und die LRU-Reihenfolge wird aktualisiert. 
     Falls die verdrängte Line dirty ist, erfolgt ein Write-Back in den Hauptspeicher.

### 3.3 LRU-Verwaltung
Nach jedem Cache-Zugriff wird die LRU-Reihenfolge der Cache-Lines des betroffenen Sets aktualisiert. 
Die gerade verwendete Cache-Line wird als am häufigsten genutzt markiert (lru_order = 0), und die übrigen Cache-Lines 
werden entsprechend um eins erhöht. Diese Aktualisierung gewährleistet, dass stets die am 
wenigsten verwendete Cache-Line für eine Ersetzung bereitsteht, wenn ein Miss auftritt.

### 3.4 Statistiken sammeln
Der Cache-Simulator erfasst verschiedene Statistiken, um die Effizienz des Caches zu bewerten. Dazu gehören Cache-Hits, 
Cache-Misses und Write-Backs. Diese Statistiken werden bei jeder Cache-Operation inkrementiert. Zusätzlich werden 
allgemeine Trace-File-Statistiken wie memory_access_count, load_count, store_count, instruction_count und cycle_count 
gesammelt. Diese Daten ermöglichen eine detaillierte Analyse der Cache-Performance und der Auswirkungen von Parametern 
wie Assoziativität und Zeilengröße auf die Effizienz des Caches.

## 4. Design-Entscheidungen
### 4.1 Cache-Statistiken und Trace-File-Statistiken (anderer Name für Trace-File-Statistiken)
 - Cache-Statistiken: Diese sind in der Struktur CacheStats enthalten, die in der Cache-Struktur gespeichert ist. Diese 
   Statistiken werden von der Cache-Architektur beeinflusst und ermöglichen die Auswertung der Effizienz des Caches 
   hinsichtlich seiner Konfiguration.
 - Trace-File-Statistiken: Diese Statistiken befinden sich in der Struktur TraceStats, die unabhängig von der 
   Cache-Struktur in der main.c-Datei definiert ist. Sie spiegeln die Zugriffsoperationen wider, die aus dem Trace-File 
   stammen, und ermöglichen so die Auswertung des gesamten Programmablaufs.

### 4.2 Initialisierungsstrategie
 - Cache-Initialisierung: Der Cache wird beim Programmstart vollständig initialisiert. Diese Entscheidung sorgt dafür, 
   dass während der Simulation keine zusätzlichen Speicherzuweisungen notwendig sind, was die Laufzeitleistung verbessert. 
   Obwohl dies die Startzeit minimal verlängert, verhindert es spätere Verzögerungen bei der Zuweisung von Speicher 
   für Cache-Sets oder -Zeilen.
 - Cache-Operation-Initialisierung: Jede Cache-Operation wird dynamisch beim Einlesen einer Trace-File-Zeile initialisiert. 
   Eine vorzeitige Initialisierung aller Operationen wäre speicherintensiver und ineffizient, da jede Operation dann 
   doppelt verarbeitet werden müsste (einmal bei der Initialisierung und einmal beim Zugriff).

### 4.3 Implementierung der LRU-Ersetzungsstrategie
 - LRU-Initialisierung: Jede Cache-Line erhält bei der Cache-Initialisierung eine eindeutige LRU-Reihenfolge, basierend 
   auf ihrem Index innerhalb des Sets. Dies erleichtert den späteren Zugriff auf die am wenigsten verwendete Line.
 - LRU-Aktualisierung: Nach jedem Zugriff wird die betroffene Cache-Line als am häufigsten verwendet markiert 
   (lru_order = 0), und alle anderen Lines im Set verschieben sich entsprechend. Dadurch wird sichergestellt, dass die 
   älteste Line stets korrekt identifiziert wird.
 - Ermittlung der LRU-Line: Beim Auftreten eines Cache-Misses wird die Line mit der höchsten lru_order 
   (am wenigsten verwendet) für eine Ersetzung gewählt. Falls eine ungültige Line vorhanden ist, wird diese sofort verwendet.

### 4.4 Fehlerbehandlung und Speicherfreigabe
 - Speicherverwaltung: Nach Abschluss der Simulation wird der belegte Speicher durch die Funktion 
   free_cache freigegeben.
 - Fehlererkennung und -behandlung: Die Fehlerbehandlung im Cache-Simulator stellt sicher, dass falsche Eingaben und 
   ungültige Speicherzugriffe erkannt und korrekt gehandhabt werden. Dazu werden mehrere Validierungen eingesetzt:
   - Argumentvalidierung: Die Funktion validate_args überprüft die vom Benutzer eingegebenen Konfigurationsparameter. 
     Sie stellt sicher, dass alle übergebenen Werte gültig sind.
   - Speicherzugriffsprüfung: In der Funktion access_cache wird sichergestellt, dass der berechnete Set-Index innerhalb 
     der gültigen Bereichsgrenzen liegt. Ein Überschreiten des Set-Index-Bereichs führt zu einer Fehlermeldung und einem 
     Programmabbruch.

### 4.5 Simulator-Optimierungen
 - Optimierung der Leistung: Um die Zugriffszeiten zu verbessern, wurden bestimmte Berechnungen wie Set-Index oder Tag-Extraktion optimiert.
   So wird beispielsweise der Logarithmus der Cache-Größen und Zeilengrößen im Voraus berechnet und in der Cache-Struktur 
   gespeichert, um zeitaufwändige Berechnungen des Logarithmus vermeiden.

## 5. Fazit und Ausblick
### 5.1 Zusammenfassung
Die wichtigsten Designentscheidungen, wie die vollständige Vorinitialisierung des Caches, die effiziente Implementierung
der LRU-Ersetzungsstrategie und die klare Trennung von Cache- und Tracefile-Statistiken tragen zur Effizienz und 
Modularität des Cache-Simulators bei. Diese Ansätze ermöglichen eine schnelle Verarbeitung von Cache-Zugriffen und 
eine präzise Analyse der Cache-Leistung.

### 5.2 Verbesserungspotenzial
In zukünftigen Erweiterungen könnten alternative Ersetzungsstrategien wie FIFO oder Zufall implementiert werden. 
Der Simulator könnte auch so erweitert werden, dass er Multi-Level-Caches unterstützt, um eine realistischere Simulation 
moderner Hardware-Architekturen zu ermöglichen.

## 6. Quellen
 - Digitale Systeme: Vorlesungsskripte
 - DeepL zum Übersetzen von Texten
 - Internetquellen:
    - https://en.wikipedia.org/wiki/Cache_(computing)
    - https://en.algorithmica.org/hpc/cpu-cache/associativity/
    - https://www.linkedin.com/advice/0/what-difference-between-write-allocate-no-write-allocate-703fc


