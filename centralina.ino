#include "Timer.h"

Timer t;

//Pin arduino:
const int RED_R = 5;      //relay rosso
const int YELLOW_R = 6;   //relay giallo
const int GREEN_R = 7;    //relay verde
const int BEEP_R =8;      //relay sirena

const int OUTIN_B = 14;   //switch outdoor-indoor
const int STEP_B = 3;     //pulsante step
const int START_B = 2;    //pulsante start
const int REC_B = 4;      //switch recupero

const long long INDOOR_TIME = 90000LL;    // 1 min e 30 sec
const long long OUTDOOR_TIME = 150000LL;  // 2 min e 30 sec

const long long yellowTime = 1000*30;     // tempo giallo: 30 sec
long long greenTime = 0;                  // tempo verde: da assegnare 

const long preTime = 1000*10; //tempo prima dello start: 10 sec

int greenID=-1, yellowID=-1, redID=-1; // ID per il timer

const long BEEP_TIME = 500;      //tempo sirena: 0.5 sec
const long BEEP_INTERVAL = 1000; //intervallo sirena: 1 sec

const int IDLE_STATE = 0;    
const int ABCICLE_STATE = 1;
const int CDCICLE_STATE =2;
int state;  //stato della macchina

boolean recupero = false; //stato recupero

void setup() {
  
  initRele();
  initButton(START_B);
  initButton(STEP_B);
  initButton(OUTIN_B);
  initButton(REC_B);
  
  //stato iniziale 
  state = IDLE_STATE;
  toRed();
  
  // Inizializzo porta seriale
  Serial.begin(9600);  
  Serial.println("Centralina avviata");
}

void loop() {
  //aggiorno timer
  t.update();
  
  //NB: il digital read dei bottoni restituisce: 
  //    0 se sono pigiati
  //    1 altrimenti
  
  //Controllo outdoor (2:30) indor (1:30) 
  //assegno il tempo del verde di conseguenza
  if(!digitalRead(OUTIN_B)) {
    //indor
    greenTime = INDOOR_TIME;
  } else {
    //outdoor
    greenTime = OUTDOOR_TIME;

  }    
  
  //controllo pressione bottone start
  startButtonCtrl();
  
  switch(state) {
   case IDLE_STATE:
     toRed(); 
      
     break;
     
   case ABCICLE_STATE:
     //controllo pressione bottone step
     stepCtrl();        
     break;
     
     
   case CDCICLE_STATE:
     //controllo pressione bottone step
     stepCtrl();
     break;
  }
  

}

void initButton(int b) {
  pinMode(b, INPUT);
  digitalWrite(b, HIGH);  
}

void initRele() {
  for(int i=5; i<13; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, HIGH);
  }
}


void toGreen() {
 beep(1);
 
 //accende il verde
 digitalWrite(GREEN_R, LOW); 
 digitalWrite(YELLOW_R, HIGH); 
 digitalWrite(RED_R, HIGH); 
 
 //esegue toYellow() dopo il tempo assegnato al verde
 yellowID = t.after(greenTime, toYellow);
}

void toYellow() {
 //accende il giallo
 digitalWrite(GREEN_R, HIGH); 
 digitalWrite(YELLOW_R, LOW); 
 digitalWrite(RED_R, HIGH); 

 //esegue toRed() dopo il tempo assegnato al giallo
 redID = t.after(yellowTime, toRed);
}

void toRed() {
 //accende il rosso
 digitalWrite(GREEN_R, HIGH); 
 digitalWrite(YELLOW_R, HIGH); 
 digitalWrite(RED_R, LOW); 

 if(state == ABCICLE_STATE) {
   beep(2);
   //esegue toGreen() dopo il tempo pre inizio tiri
   greenID = t.after(preTime, toGreen);
   //passa allo stato CD
   state = CDCICLE_STATE;
   
 } else if ( state == CDCICLE_STATE) {
   //controlla se c'è richiesta di recupero
   if(!digitalRead(REC_B)) {
     //richiesta recupero 4 beep 
     beep(4);  
   } else {
     beep(3);
   }
   //passo allo stato di attesa
   state = IDLE_STATE;
 }
}

//esegue un beep "count" volte
void beep(int count) {
  if(count > 0)
    startBeep();
  
  if(count >1)
    t.every(BEEP_INTERVAL, startBeep, count-1);
}

void startBeep() {
  digitalWrite(BEEP_R, LOW);
  t.after(BEEP_TIME, stopBeep);
}

void stopBeep() {
  digitalWrite(BEEP_R, HIGH);
}

void stopTimers() {
 if(redID >=0)
  t.stop(redID);
 
 if(yellowID >=0)
  t.stop(yellowID);
 
 if(greenID >=0)
  t.stop(greenID);
 
 greenID = yellowID = redID = -1; 
}


void stepCtrl() {
  if(!digitalRead(STEP_B) && digitalRead(RED_R) == HIGH) {
   //e' stato premuto il bottone di step a ciclo avviato
   //interrompo i timers, e passo al rosso
   stopTimers();
   toRed();
 }
}


void startButtonCtrl() {
  //il debounce evita paciughi nella lettura della pressione del bottone
  static int debounceStart =0;
  static boolean startPush = false; 
  
  if(digitalRead(START_B))
    debounceStart++;
  if(!startPush && !digitalRead(START_B)) {
    //il bottone e' stato premuto
    startPushed(); 
    startPush= true;
    debounceStart=0;
  }
  if(startPush && debounceStart>500 && digitalRead(START_B))
    startPush=false;
}

void startPushed() {
  Serial.println("start premuto");
  
  if(state == IDLE_STATE) {
     Serial.println("START");
     
     if(!digitalRead(REC_B)) {
       //richiesta recupero accettata
       recupero = true;
       state = CDCICLE_STATE;
     } else {
       //no recupero, si passo allo stato AB
       recupero = false;
       state = ABCICLE_STATE;
     }

     beep(2);
     //dopo il preTime esegue toGreen()
     greenID = t.after(preTime, toGreen);
     
  } else if(state == ABCICLE_STATE || state == CDCICLE_STATE) {
     //premere il bottone start durante fa resetta sutto allo stato di attesa
     Serial.println("RESET"); 
     stopTimers();
     state = IDLE_STATE;
  }
}
  
  
