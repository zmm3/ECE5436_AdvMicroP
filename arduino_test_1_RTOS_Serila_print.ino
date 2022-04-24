 #include <Arduino_FreeRTOS.h>
 #include<semphr.h>  
 #define redLed 7  
 #define greenLed 8 

 //--------Ultra sonic ranging system decleration--------------------
 const int trigPin = 9;
 const int ecoPin = 10;
//variable dec
 long duration;
 int distanceCm;
 //---------Change detection using derativitive decleraton--------------
 long arr[4]={0,0,0,0};
 long velo[4] = {0,0,0,0};
 long newData;
 long derv;
 //---------Take picture---------------------------------------
 int takepic = 0;
 const int picPin = 2;
 //---------function decleration---------------------------------------
 int Deravitive(void){
  return (arr[0]+3*arr[1]-3*arr[2]-arr[3])/18;
 }
 int midDerivitive(void){
  return (velo[0]+3*velo[1]-3*velo[2]-velo[3])/3;
 }
 void mid_pur(int in){
  velo[3] = velo[2];
  velo[2] = velo[1];
  velo[1] = velo[0];
  velo[0] = in;
 }
 void MACQ_Put(int in){
  arr[3] = arr[2];
  arr[2] = arr[1];
  arr[1] = arr[0];
  arr[0] = in;
 }
 //---------Semaphore decleration--------------------------------------
 static SemaphoreHandle_t busy_Semaphore; //changed detected semaphore give
                                    //actiond due to change detected Semaphore take
 static SemaphoreHandle_t pic_Taken_Semaphore; //Picture need to be taken semaphore give
                                               //Picture taken DONE semaphore take
 static SemaphoreHandle_t new_Data_Semaphore; //New data generated by untra sonic system would give semaphore
                                              //the change detection task would take the semaphore
 TaskHandle_t ultraTask_handle = NULL;
 TaskHandle_t derviTask_handle = NULL;
 TaskHandle_t picTask_handle = NULL;
 
 void setup() {  
  Serial. begin(9600);
  busy_Semaphore = xSemaphoreCreateMutex();
  pic_Taken_Semaphore = xSemaphoreCreateMutex();
  new_Data_Semaphore = xSemaphoreCreateMutex();
  xTaskCreate(ultraRangeTask, "Untra range Task", 128, NULL, 1, &ultraTask_handle); 
  xTaskCreate(changeDetectTask, "Detect any change Task", 128, NULL, 1, &derviTask_handle);  
  xTaskCreate(takePicTask, "Takes a picture Task", 128, NULL, 1, &picTask_handle);  
 }  
 
 void loop() {}  
 
 void ultraRangeTask(void *pvParameters){  
  vTaskSuspend(derviTask_handle);
  pinMode(redLed, OUTPUT); 
  pinMode(trigPin, OUTPUT);
  pinMode(ecoPin, INPUT);
  const TickType_t xDelay = 100 / portTICK_PERIOD_MS; 
  while(1){  
   //Serial.print("Ultrasonic ranging system engage.\n");
   digitalWrite(trigPin, LOW);
   delayMicroseconds(2);
   digitalWrite(trigPin, HIGH);
   delayMicroseconds(10);
   digitalWrite(trigPin, LOW);
   duration = pulseIn(ecoPin, HIGH);
   distanceCm= duration*0.034/2;
   //distanceInch = duration*0.0133/2;
   if(distanceCm < 400){
    if(xSemaphoreTake(new_Data_Semaphore,0) == pdTRUE){
      newData = distanceCm;
      xSemaphoreGive(new_Data_Semaphore);
      vTaskResume(derviTask_handle);
      vTaskSuspend(ultraTask_handle);
    }
   }
   else
      newData = newData;
  }  
 }  
  void changeDetectTask(void *pvParameters){  
  pinMode(greenLed, OUTPUT);  
  const TickType_t xDelay = 100 / portTICK_PERIOD_MS; 
  while(1){  
    if(xSemaphoreTake(new_Data_Semaphore,0) == pdTRUE){
     //vTaskSuspend(ultraTask_handle);
      mid_pur(newData); // shift the new data in the array
      //MACQ_Put(midDerivitive());  // puting the first derivitive in the final array
      MACQ_Put(newData);  // putting the new data in the final array
                          // as there would no longer use of 2nd derivitive
      derv = abs(Deravitive()); // getting the abs of the drivitinve of the final array
      Serial.print("dV/dt: ");
      Serial.println(derv);
      if(derv > 100){
       if(xSemaphoreTake(pic_Taken_Semaphore,0) == pdTRUE){
        takepic = 1;
        vTaskResume(picTask_handle);
        xSemaphoreGive(pic_Taken_Semaphore);
       }
       else
        takepic = 0;
      }
      xSemaphoreGive(new_Data_Semaphore);
    vTaskResume(ultraTask_handle);
    }
  }  
 }
 
  void takePicTask(void *pvParameters){    
  const TickType_t xDelay = 10000 / portTICK_PERIOD_MS; 
  pinMode(picPin, OUTPUT);
  while(1){  
   if(xSemaphoreTake(busy_Semaphore,0) == pdTRUE){
      if(xSemaphoreTake(pic_Taken_Semaphore,0) == pdTRUE){
      Serial.print("taking piture\n");
      //vTaskSuspend(ultraTask_handle);
      //vTaskSuspend(derviTask_handle);
        
      if(takepic == 1){
       digitalWrite(picPin, HIGH);
//        for(int i = 0; i<4; i++){
//          arr[i] = 0;
//          velo[i] = 0;
//        }
        vTaskDelay(xDelay);
        
        Serial.print("picture Taken\n");
        takepic = 0;
        digitalWrite(picPin, LOW);
      }
      Serial.print("picture taken\n:");
      xSemaphoreGive(pic_Taken_Semaphore);
      //vTaskResume(ultraTask_handle);
      //vTaskResume(derviTask_handle);
     vTaskSuspend(picTask_handle);
     }
     xSemaphoreGive(busy_Semaphore);
   }
  }  
 }
