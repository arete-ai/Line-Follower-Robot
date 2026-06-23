#ifndef F_CPU
#define F_CPU 16000000UL // 16 MHz für Arduino Nano
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <stdlib.h>

// --- Globale Variablen für Motorschutz ---
int16_t last_speed_m1 = 0;
int16_t last_speed_m2 = 0;

// --- Initialisierungs-Funktionen ---
void initADC() {
    ADMUX = (1 << REFS0); // Referenzspannung AVCC
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // ADC an, Prescaler 128
}

uint16_t readADC(uint8_t pin) {
    ADMUX = (ADMUX & 0xF0) | (pin & 0x0F);
    ADCSRA |= (1 << ADSC); // Wandlung starten
    while (ADCSRA & (1 << ADSC)); // Warten bis fertig
    return ADC;
}

void initPWM() {
    // PB1 (M1_EN) und PB2 (M2_EN) als Ausgänge
    DDRB |= (1 << PB1) | (1 << PB2);
    // Timer1 im Fast PWM Modus 14 (TOP = ICR1)
    TCCR1A |= (1 << COM1A1) | (1 << COM1B1) | (1 << WGM11);
    TCCR1B |= (1 << WGM13) | (1 << WGM12) | (1 << CS11); // Prescaler 8
    ICR1 = 1023; // Passend zur 10-Bit ADC Auflösung
}

// --- Modulare Motorsteuerung mit Batterieschutz ---
void setMotor(uint8_t motor, int16_t speed) {
    // Geschwindigkeit auf -1023 bis +1023 begrenzen
    if (speed > 1023) speed = 1023;
    if (speed < -1023) speed = -1023;

    if (motor == 1) { // Linker Motor
        // Schutz: Kurze Pause bei hartem Richtungswechsel [cite: 168, 169]
        if ((last_speed_m1 > 0 && speed < 0) || (last_speed_m1 < 0 && speed > 0)) {
            OCR1A = 0; _delay_ms(50);
        }
        if (speed >= 0) {
            PORTB &= ~(1 << PB0); // Vorwärts
            OCR1A = speed;
        } else {
            PORTB |= (1 << PB0); // Rückwärts
            OCR1A = -speed;
        }
        last_speed_m1 = speed;
    } else { // Rechter Motor
        if ((last_speed_m2 > 0 && speed < 0) || (last_speed_m2 < 0 && speed > 0)) {
            OCR1B = 0; _delay_ms(50);
        }
        if (speed >= 0) {
            PORTB &= ~(1 << PB3); // Vorwärts
            OCR1B = speed;
        } else {
            PORTB |= (1 << PB3); // Rückwärts
            OCR1B = -speed;
        }
        last_speed_m2 = speed;
    }
}

// --- Hauptprogramm ---
int main() {
    // Richtungs-Pins (PB0, PB3) als Ausgänge setzen
    DDRB |= (1 << PB0) | (1 << PB3);
    
    // Taster SW2 (PD2) und Jumper (PD4) als Eingänge mit Pull-Up
    DDRD &= ~((1 << PD2) | (1 << PD4));
    PORTD |= (1 << PD2) | (1 << PD4);

    initADC();
    initPWM();

    // 1. Schwellenwert aus EEPROM laden 
    uint16_t threshold = eeprom_read_word((uint16_t*)0x00);
    
    // 2. Start-Modus überprüfen (PD4 auf GND?) 
    uint8_t mode_p_regler = !(PIND & (1 << PD4)); // 1 wenn Jumper gesteckt (LOW)

    while (1) {
        // Sensoren und Poti auslesen
        uint16_t ir_links = readADC(0);
        uint16_t ir_rechts = readADC(1);
        uint16_t base_speed = readADC(7); // Geschwindigkeit per POT1 

        // --- Kalibrierungs-Logik (SW2 gedrückt) --- 
        if (!(PIND & (1 << PD2))) {
            threshold = (ir_links + ir_rechts) / 2; // Durchschnitt auf Weiß messen
            threshold += 150; // Puffer aufschlagen, um Schwarz sicher zu erkennen
            eeprom_write_word((uint16_t*)0x00, threshold); // In EEPROM speichern
            
            // Visuelles Feedback: Motoren stoppen kurz
            setMotor(1, 0); setMotor(2, 0);
            _delay_ms(1000);
        }

        // --- Fallback: Beide Sensoren sehen Weiß (Linie komplett verloren) --- 
        if (ir_links < threshold && ir_rechts < threshold) {
            setMotor(1, -(base_speed / 2)); // Langsam rückwärts fahren
            setMotor(2, -(base_speed / 2));
            continue; // Nächster Schleifendurchlauf
        }

        // --- Modus 2: P-Regelung --- [cite: 161, 162, 163]
        if (mode_p_regler) {
            float kP = 0.6; // Tuning-Faktor (muss ggf. in der Praxis angepasst werden)
            int16_t error = ir_links - ir_rechts; // Abweichung berechnen
            int16_t correction = (int16_t)(error * kP); // Proportionale Korrektur

            // Geschwindigkeit proportional anpassen
            setMotor(1, base_speed - correction);
            setMotor(2, base_speed + correction);
        } 
        // --- Modus 1: Bang-Bang-Steuerung --- [cite: 150-154]
        else {
            if (ir_links >= threshold && ir_rechts >= threshold) {
                // Beide auf Schwarz -> Geradeaus 
                setMotor(1, base_speed);
                setMotor(2, base_speed);
            } else if (ir_links >= threshold && ir_rechts < threshold) {
                // Nur links auf Schwarz -> Korrektur nach links 
                setMotor(1, -(base_speed / 2)); 
                setMotor(2, base_speed);
            } else if (ir_links < threshold && ir_rechts >= threshold) {
                // Nur rechts auf Schwarz -> Korrektur nach rechts 
                setMotor(1, base_speed);
                setMotor(2, -(base_speed / 2)); 
            }
        }
    }
    return 0;
}