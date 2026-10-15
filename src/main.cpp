#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

// Definice pinů (MicroCore číslování odpovídá PBx)
#define BUTTON_PIN 1  // PB1 (Fyzický Pin 6) - Start / Reset tlačítko
#define SWITCH_PIN 3  // PB3 (Fyzický Pin 2) - Přepínač času (HIGH = 35s, LOW = 25s)
#define BUZZER_PIN 0  // PB0 (Fyzický Pin 5) - Pasivní buzzer
#define MOTOR_PIN  4  // PB4 (Fyzický Pin 3) - Spínání tranzistoru motorku

// Definice frekvence tónu v Hz
#define TONE_FREQ 2700 

// Globální příznaky pro komunikaci s přerušením
volatile bool reset_requested = false;

// Pomocná funkce pro současné pípnutí a vibraci
void beep_and_vibrate(uint16_t duration_ms) {
  digitalWrite(MOTOR_PIN, HIGH);  // Sepne tranzistor a spustí motorek
  tone(BUZZER_PIN, TONE_FREQ);    // Spustí generování zvuku
  
  delay(duration_ms);             // Akce trvá určený čas
  
  noTone(BUZZER_PIN);             // Zastaví zvuk
  digitalWrite(MOTOR_PIN, LOW);   // Rozepne tranzistor a zastaví motorek
}

// Přerušení PCINT0 (hlídá změny na pinech)
ISR(PCINT0_vect) {
  // Pokud došlo ke stisku tlačítka (logická 1 na PB1 při externím pull-downu)
  if (digitalRead(BUTTON_PIN) == HIGH) {
    reset_requested = true; // Nastavíme příznak pro reset/probuzení
  }
}

// Funkce, která bezpečně počká 1 sekundu a zároveň hlídá stisk tlačítka pro reset
bool wait_one_second_or_reset() {
  for (int i = 0; i < 20; i++) {
    delay(50);
    if (reset_requested) {
      reset_requested = false; // Vynulovat příznak
      delay(50);               // Odstranění zákmorů (debouncing)
      return true;             // Detekován stisk -> signalizujeme reset
    }
  }
  return false;
}

void setup() {
  // Nastavení výstupů
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  
  // Bezpečné výchozí vypnutí obou periferií
  noTone(BUZZER_PIN);
  digitalWrite(MOTOR_PIN, LOW);

  // Konfigurace vstupů (externí pull-down rezistory)
  pinMode(BUTTON_PIN, INPUT);
  pinMode(SWITCH_PIN, INPUT);

  // Nastavení Pin Change Interrupt pro buzení a reset
  GIMSK |= (1 << PCIE);    // Povolit přerušení Pin Change obecně
  PCMSK |= (1 << PCINT1);  // Aktivovat sledování pinu PB1 (tlačítko)
  
  sei(); // Povolit globální přerušení
}

void loop() {
  // --- REŽIM HLUBOKÉHO SPÁNKU ---
  // Pro jistotu před spánkem vše natvrdo vypneme (nulový odběr na výstupech)
  noTone(BUZZER_PIN);
  digitalWrite(MOTOR_PIN, LOW);
  
  // Procesor spí, dokud na PB1 nepřijde logická 1 (stisk tlačítka)
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();
  
  // --- PROBUZENÍ ---
  sleep_disable();
  reset_requested = false; // Vyčistit stav po probuzení
  delay(50);               // Debounce po probuzení

  // Spustíme hlavní odpočítávací cyklus
  while (true) {
    // 1. Načtení celkového času podle stavu přepínače
    int celkovy_cas = (digitalRead(SWITCH_PIN) == HIGH) ? 35 : 25;
    bool preruseno = false;

    // 2. Smyčka odpočtu od sekundy 1 do konce
    for (int sekunda = 1; sekunda <= celkovy_cas; sekunda++) {
      
      // Počkáme 1 sekundu, pokud funkce vrátí true, uživatel stiskl tlačítko znova
      if (wait_one_second_or_reset()) {
        preruseno = true;
        break; // Přerušíme stávající odpočet a skočíme na začátek 'while' smyčky
      }

      int zbyva_sekund = celkovy_cas - sekunda;

      // 10 sekund před koncem pípne a zavibruje dlouze
      if (zbyva_sekund == 10) {
        beep_and_vibrate(300);
      }
      // 5 sekund před koncem pípne a zavibruje dlouze
      else if (zbyva_sekund == 5) {
        beep_and_vibrate(300);
      }
      // Každou další vteřinu do nuly pípne a zavibruje krátce (4, 3, 2, 1 sekunda do konce)
      else if (zbyva_sekund >= 1 && zbyva_sekund <= 4) {
        beep_and_vibrate(100);
      }
    }

    // Pokud byl odpočet přerušen stiskem tlačítka, smyčka 'while' skočí na začátek a počítá znovu
    if (preruseno) {
      continue; 
    }

    // --- DOSAŽENÍ NULY ---
    // Pokud odpočet doběhl úspěšně do konce bez přerušení, pípne a zavibruje 3x v nule
    for (int i = 0; i < 3; i++) {
      beep_and_vibrate(200);
      if (i < 2) delay(100);
    }

    // Počkáme, dokud uživatel tlačítko nepustí, aby se zařízení hned znovu neprobudilo
    while (digitalRead(BUTTON_PIN) == HIGH) {
      delay(10);
    }
    delay(50); // Koncový debounce při puštění tlačítka

    break; // Vyskočíme z 'while' smyčky a procesor se na začátku 'loop' opět uspí
  }
}

/*#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

// Definice pinů (MicroCore číslování odpovídá PBx)
#define BUTTON_PIN 1  // PB1 (Fyzický Pin 6) - Start / Reset tlačítko
#define SWITCH_PIN 3  // PB3 (Fyzický Pin 2) - Přepínač času (HIGH = 35s, LOW = 25s)
#define BUZZER_PIN 0  // PB0 (Fyzický Pin 5) - Pasivní buzzer

// Definice frekvence tónu v Hz (pro piezo je ideální cca 2000 až 4000 Hz)
#define TONE_FREQ 2700 

// Globální příznaky pro komunikaci s přerušením
volatile bool reset_requested = false;

// Pomocná funkce pro pípnutí pasivního buzzeru
void beep(uint16_t duration_ms) {
  tone(BUZZER_PIN, TONE_FREQ); // Spustí generování frekvence
  delay(duration_ms);          // Počká určenou dobu
  noTone(BUZZER_PIN);          // Zastaví generování frekvence a vypne pin
}

// Přerušení PCINT0 (hlídá změny na pinech)
ISR(PCINT0_vect) {
  // Pokud došlo ke stisku tlačítka (logická 1 na PB1 při externím pull-downu)
  if (digitalRead(BUTTON_PIN) == HIGH) {
    reset_requested = true; // Nastavíme příznak pro reset/probuzení
  }
}

// Funkce, která bezpečně počká 1 sekundu a zároveň hlídá stisk tlačítka pro reset
bool wait_one_second_or_reset() {
  // Rozdělíme 1 sekundu na 20 menších úseků po 50 ms pro rychlou reakci na tlačítko
  for (int i = 0; i < 20; i++) {
    delay(50);
    if (reset_requested) {
      reset_requested = false; // Vynulovat příznak
      delay(50);               // Odstranění zákmorů (debouncing)
      return true;             // Detekován stisk -> signalizujeme reset
    }
  }
  return false;
}

void setup() {
  // MicroCore funkce tone() si pin sama nastaví jako výstupní, ale pro jistotu:
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  // Konfigurace vstupů (externí pull-down rezistory)
  pinMode(BUTTON_PIN, INPUT);
  pinMode(SWITCH_PIN, INPUT);

  // Nastavení Pin Change Interrupt pro buzení a reset
  GIMSK |= (1 << PCIE);    // Povolit přerušení Pin Change obecně
  PCMSK |= (1 << PCINT1);  // Aktivovat sledování pinu PB1 (tlačítko)
  
  sei(); // Povolit globální přerušení
}

void loop() {
  // --- REŽIM HLUBOKÉHO SPÁNKU ---
  // Vypneme buzzer, abychom měli jistotu nulového odběru
  noTone(BUZZER_PIN);
  
  // Procesor spí, dokud na PB1 nepřijde logická 1 (stisk tlačítka)
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();
  
  // --- PROBUZENÍ ---
  sleep_disable();
  reset_requested = false; // Vyčistit stav po probuzení
  delay(50);               // Debounce po probuzení

  // Spustíme hlavní odpočítávací cyklus
  while (true) {
    // 1. Načtení celkového času podle stavu přepínače
    int celkovy_cas = (digitalRead(SWITCH_PIN) == HIGH) ? 35 : 25;
    bool preruseno = false;

    // 2. Smyčka odpočtu od sekundy 1 do konce
    for (int sekunda = 1; sekunda <= celkovy_cas; sekunda++) {
      
      // Počkáme 1 sekundu, pokud funkce vrátí true, uživatel stiskl tlačítko znova
      if (wait_one_second_or_reset()) {
        preruseno = true;
        break; // Přerušíme stávající odpočet a skočíme na začátek 'while' smyčky
      }

      int zbyva_sekund = celkovy_cas - sekunda;

      // 10 sekund před koncem pípne dlouze
      if (zbyva_sekund == 10) {
        beep(300);
      }
      // 5 sekund před koncem pípne dlouze
      else if (zbyva_sekund == 5) {
        beep(300);
      }
      // Každou další vteřinu do nuly pípne krátce (4, 3, 2, 1 sekunda do konce)
      else if (zbyva_sekund >= 1 && zbyva_sekund <= 4) {
        beep(100);
      }
    }

    // Pokud byl odpočet přerušen stiskem tlačítka, smyčka 'while' skočí na začátek a počítá znovu
    if (preruseno) {
      continue; 
    }

    // --- DOSAŽENÍ NULY ---
    // Pokud odpočet doběhl úspěšně do konce bez přerušení, pípne 3x v nule
    for (int i = 0; i < 3; i++) {
      beep(200);
      if (i < 2) delay(100);
    }

    // Počkáme, dokud uživatel tlačítko nepustí, aby se zařízení hned znovu neprobudilo
    while (digitalRead(BUTTON_PIN) == HIGH) {
      delay(10);
    }
    delay(50); // Koncový debounce při puštění tlačítka

    break; // Vyskočíme z 'while' smyčky a procesor se na začátku 'loop' opět uspí
  }
}
*/