
int ir_pin=3; // digital pin the IR reciever Vout is connected to

long no_change_time=40000; //No change in IR pin for this duration (microseconds) will mark the end of pattern.
const int array_size=300; //Max times to log.  If array is full, it will report the pattern to that point. 
long t_array[array_size]; //Array of times of when the IR pin changes state.
int count=0; // Number of times logged to array.

boolean log_micros=true;  //Flag to indicate if able to log times to array
long last_micros=0; //previous array time entry



void setup() {
   pinMode(ir_pin, INPUT);

   //attach an interupt to log when the IR pin changes state
   attachInterrupt(digitalPinToInterrupt(ir_pin),interupt,CHANGE);

  //Initiate Serial for Communication
   Serial.begin(9600); 
    
}



void loop() {

  if (count>0){
    log_micros=false; //make sure no more data added to array while we report it

         Serial.println("Orbiter entered sight of View");
         Serial.println("Doing 60° sweep");
         
                    // insert 60deg sweep here
                    
         Serial.println("Doing sweep to 0");
         
                    // insert sweep to 0 here
                    
         Serial.println("Waiting 30s");
         delay(30000);

      log_micros=true; count=0; //Reset array
    }
}
  

// Interupt to log each time the IR signal changes state 
void interupt(){
  if (log_micros){
    long m=micros();
    if (count>0&&(m-last_micros)>no_change_time){
      log_micros=false;
    }else{
       t_array[count]=m;
       count++;
       last_micros=m;
       if (count>=array_size) log_micros=false;
     }
  }
}
  
