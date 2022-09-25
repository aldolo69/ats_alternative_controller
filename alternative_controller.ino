
#include <avr/io.h>
#include <util/delay.h>



/*

  al reset parte con tutto spento e posizione iniziale=0
  ats si posiziona in automatico
  pausa 10 secondi

  nel loop

  se c'e' forzatura non controlla posizioni
  altrimenti  controlla le 3 posizioni. 1=fv fisso 2=enel fisso 3=automatico

  i posizione 3  controlla lo stato del fv. se non c'e' aspetta 5 secondi . se non c'e' ancora fv forza posizione 2 per 10 minuti

  posizione 1 2 3. se vede un cambio diposizione -->attiva il giusto relè, pausa 1 secondo



         2.2
  -->|---\|\|----
  pin6 bit1     ///
   rosso
  +5---\|\|-o
     1k   |
        O O O                 100k    100k
        | | |    --Giallo------------|\|\|---|\|\|\----
        v v v                              |         ///
        - - -    --Blu----------------------------------adc  0v=verde  4v=blu  2v=giallo pin7 bit2
        | | |                          soglie 0...1  1...3 3..5
        -----
          |
          |
     1k |
  ---\|\|-
  ///      giallo =solare fisso
          blu  =enel fisso
           verde = automatico

      -------|
             |   ---------+5
   enel out /   (  _
            | |  ) ^ diodo
      -------   (
                 |         2.2k
                  \_______\/\/---pin5 bit0
                  /npn
                ///


      -------|
             |   ---------+5
   fv out   /   (  _
            | |  ) ^ diodo
      -------   (
                 |         2.2k
                  \_______\/\/---pin3 bit4
                  /npn
                ///                      -------------------pin2 bit3
                                         |
          4.7n  330k      fotoacc        |        27k
  fv in  -||--|\|\|--------    -----------------|\|\|\----+5
                        ^  v  /          |
                        |  |  \----     ---
         -------------------      |     --- 100n
                                 ///     |
                                        ///
*/

// MOSI pin 5 corrisponde a "0"
// MISO pin 6 corrisponde a "1"
// CLK pin 7 corrisponde a "2"
//pin 2 corrisponde a "3"
//pin 3 corrisponde a "4"

#define CHECK_FV 3
#define RELE_FV 4
#define RELE_ENEL 0
#define LED_ADC_INPUT A1
#define LED_ROSSO_OUTPUT 1

#define FV 1
#define ENEL 2
#define AUTO 3

//#define SERIALPRINT


#define LUNGHEZZACHECK 50
//antirimbalzo di X millisecondi

#ifdef SERIALPRINT
#define ATTESARITORNOFV (1000UL*15UL)
#else
#define ATTESARITORNOFV (1000UL*60UL*10UL)
#endif.



static char stato = 0;
static char statoRele = 0;




void staccaFV()
{
  digitalWrite(RELE_FV, HIGH);
}

void attaccaFV()
{
  digitalWrite(RELE_FV, LOW);
}

void staccaENEL()
{
  digitalWrite(RELE_ENEL, HIGH);
}


void attaccaENEL()
{
  digitalWrite(RELE_ENEL, LOW);
}



//  soglie 0...1,5  1,5...3 3..5
boolean ledAUTO(int adc)
{
  return (adc < (1024 * 1 / 5)) ? true : false;

}

boolean ledFV(int adc)
{ //1.7
  return (adc <= (1024 * 3 / 5) && adc >= (1024 * 1 / 5)) ? true : false;
}

boolean ledENEL(int adc)
{ //3.8v
  return (adc > (1024 * 3 / 5) ) ? true : false;
}

boolean checkFotovoltaicoPresente()
{
  return digitalRead(CHECK_FV) == LOW;
}

int adc_read (void)
{
  return analogRead(LED_ADC_INPUT);
}

void setup()
{


  pinMode(LED_ADC_INPUT, INPUT);
  pinMode(RELE_FV, OUTPUT);
  pinMode(RELE_ENEL, OUTPUT);
  pinMode(CHECK_FV, INPUT);
  pinMode(LED_ROSSO_OUTPUT, OUTPUT);

#ifdef SERIALPRINT
  Serial.begin(115200);
#endif

}


void allineaRele(char stato)
{
  statoRele = stato;
  switch (stato)
  {
    case FV:
#ifdef SERIALPRINT
      Serial.println("ReleFV");
#endif
      staccaENEL();
      ritardo(70);//delay(100);
      attaccaFV();
      ritardo(70);//delay(100);
      break;
    case ENEL:
#ifdef SERIALPRINT
      Serial.println("ReleENEL");
#endif
      staccaFV();
      ritardo(70);//delay(100);
      attaccaENEL();
      ritardo(70);//delay(100);
      break;
  }

}


boolean determinaStatoFV()
{
  //se vedi persistenza allora cambia lo stato
  for (;;)
  {
    char contatore;
    for (contatore = 0; contatore < LUNGHEZZACHECK; contatore++)
    {
      if (checkFotovoltaicoPresente() != true) break;
      ritardo(1);//delay(1);
    }
    if (contatore == LUNGHEZZACHECK) return true;

    for (contatore = 0; contatore < LUNGHEZZACHECK; contatore++)
    {
      if (checkFotovoltaicoPresente() != false) break;
      ritardo(1);//delay(1);
    }
    if (contatore == LUNGHEZZACHECK) return false;
  }
  return false;
}


char determinaStatoCommutatore()
{


  char contatore;
  for (;;)
  {
    for (contatore = 0; contatore < LUNGHEZZACHECK; contatore++)
    {
      if (ledFV(adc_read())  != true) break;
      ritardo(1);//delay(1);

    }
    if (contatore == LUNGHEZZACHECK) return FV;


    for (contatore = 0; contatore < LUNGHEZZACHECK; contatore++)
    {
      if (ledENEL(adc_read())  != true) break;
      ritardo(1);//delay(1);
    }
    if (contatore == LUNGHEZZACHECK) return ENEL;


    for (contatore = 0; contatore < LUNGHEZZACHECK; contatore++)
    {
      if (ledAUTO(adc_read()) != true) break;
      ritardo(1);//delay(1);
    }
    if (contatore == LUNGHEZZACHECK) return AUTO;

  }

}




unsigned long millisDiversoDaZero() {
  unsigned long mill = millis();
  if (mill == 0)
  {
    mill = 1;
    delay(1);
  }
  return mill; //per non avere mai 0 che significa nessuna attesa
}



void ritardo(int rit)
{
  for (; rit; rit--)
  {
    ledStato();
    delay(1);
  }
}


void ledStato()
{
  static char contatore = 0;
  static unsigned long startAttesaProssimoEvento = 0;
  static unsigned long attesa = 0;

  if (statoRele == 0) return;

  if ((millis() - startAttesaProssimoEvento) < attesa) return;

  startAttesaProssimoEvento = millisDiversoDaZero();

  if (digitalRead(LED_ROSSO_OUTPUT) == LOW)
  {
    digitalWrite(LED_ROSSO_OUTPUT, HIGH);
    attesa = 100;
    return;
  }

  digitalWrite(LED_ROSSO_OUTPUT, LOW);
  contatore++;


  if (contatore >= statoRele)
  {
    contatore = 0;
    attesa = 1000;
  }
  else
  {
    attesa = 100;
  }

}



void loop()
{
  static unsigned long startAttesaFv = 0;
  static char statoProssimo = 0;
  bool fotovoltaicoPresente = false;


  statoProssimo = determinaStatoCommutatore();


  ledStato();



  if (statoProssimo == AUTO && startAttesaFv != 0)
  {
#ifdef SERIALPRINT
    Serial.println("Attesa");
#endif

    //sono sempre in modo auto  e in fase enel forzata
    if ((millis() - startAttesaFv) > ATTESARITORNOFV)
    {
      //passati tot minuti. si torna all'auto normale
      startAttesaFv = 0;
#ifdef SERIALPRINT
      Serial.println("FineAttesa");
#endif

    }
    else
    {
      //forzo enel per ATTESARITORNOFV millisecondi
      allineaRele(ENEL);
    }
    stato = statoProssimo;
    return;
  }


  if (statoProssimo == AUTO && startAttesaFv == 0)
  {
    stato = statoProssimo;

    //se stato è automatico e non è cambiato bisogna controllare che il fv sia attivo
    unsigned long startControlloFv = millisDiversoDaZero();
    boolean statoFv = false;
    for (;;)
    {
      if ((millis() - startControlloFv) >= 1000)
      {
        break;
      }
      if (determinaStatoFV() == true)
      { 
        statoFv = true;
        break;
      }
    }

    if (statoFv == false)
    {
      //1 secondo senza fv
      startAttesaFv = millisDiversoDaZero();
      allineaRele(ENEL);
    }
    else
    {
      allineaRele(FV);
      startAttesaFv = 0;
    }
    return;
  }


  if (statoProssimo == FV )
  {
    stato = FV;
    allineaRele(FV);

    startAttesaFv = 0;
    return;
  }

  if (statoProssimo == ENEL )
  {
    stato = ENEL;
    allineaRele(ENEL);

    startAttesaFv = 0;
    return;
  }

}
