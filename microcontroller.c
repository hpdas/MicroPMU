#include <lm4f120h5qr.h>
#include <arm_math.h>
#include <math.h>
#define ADC_FLAG_NONE 0x00000000
#define ADC_FLAG_TOGGLE_LED 0x00000001
char readChar(void);
int get_time_ref();
volatile static float32_t adcResult[50];
//float32_t sin_sample,cos_sample;
float32_t sine_sum,cosine_sum,MAG,PHG=0;
float32_t IMAG[50],REAL[50];
int time_ref,TIME_STAMP[50];
int constant,reference,count[50];
float32_t sin_multiple[50],cos_multiple[50];
volatile static uint8_t adc_flag = ADC_FLAG_NONE;
void ADC1SS3_Handler(void){
   adc_flag = ADC_FLAG_TOGGLE_LED;
   ADC1->ISC = (1<<3);
}
void TIMER0A_Handler(void){
   TIMER0->ICR |= (1<<0);
}
int main()
{
   int i;
   for(i=0;i<50;i++)sin_multiple[i] = arm_sin_f32(2*PI*(49-i)/50);
   for(i=0;i<50;i++)cos_multiple[i] = arm_cos_f32(2*PI*(49-i)/50);

   //CONFIGURE THE UART
   SYSCTL->RCGCUART |= (1<<1);
   SYSCTL->RCGCGPIO |= (1<<1);
   GPIOB->AFSEL = (1<<1)|(1<<0);
   GPIOB->PCTL = (1<<0)|(1<<4);
   GPIOB->DEN = (1<<0)|(1<<1);

   UART1->CTL &= ~(1<<0);
   UART1->IBRD = 104;
   UART1->FBRD = 11;
   UART1->LCRH = (0x3<<5);
   UART1->CC = 0x0;
   UART1->CTL = (1<<0)|(1<<8)|(1<<9);

   //CONFIGURE GPIOF LED
   SYSCTL->RCGCGPIO |= (1<<5);
   GPIOF->DEN = 0xff;
   GPIOF->AFSEL = 0x00;
   GPIOF->DIR = 0xff;
   GPIOF->DATA &= ~((1<<1)|(1<<2)|(1<<3));

   //CONFIGURE THE ADC
   SYSCTL->RCGCADC = (1<<1);//ENABLE CLOCK TO RCGCADC REGISTER-WE ARE SETTING BIT 1 BECAUSE WE WILL USE ADC1 NOT ADC0
   SYSCTL->RCGCGPIO |= (1<<4);//ENABLE PORT E, SO SET BIT 4
   GPIOE->DIR &= ~(1<<1);// WE WILL USE PIN 1, PE1 AS INPUT SO WE NEED TO CLEAR IT
   GPIOE->AFSEL = (1<<1);//which pin using for analog input, here PE1,ALTERNATE FUNCTION SELECT(AFSEL)
   GPIOE->DEN &= ~(1<<1);//DEN bit (Digital Enable bit) cleared to activate theanalog input pin, as PE1 is analog
   GPIOE->AMSEL = (1<<1);//disable analog isolation circuit

   //CONFIGURE THE SAMPLE SEQUENCER
   ADC1->ACTSS &= ~(1<<3);//first disable sequencer before configuring, as we
                            //are choosing SS3, 1<<3 came
   ADC1->EMUX = ((0x5)<<12);//Difference//Configure the trigger what starts the
                              //conversion, how do u tell the microcontroller to begin reading and then
                              //converting
   ADC1->SSMUX3 = 2;//which analog input you are choosing, as we are using PE1,it is AN2, so 2 used
   ADC1->SSCTL3 = 0x6;//HAVE TO TAKE CARE TO SET THIS
   ADC1->IM = (1<<3);//as we are using SS3, 1<<3
   ADC1->ACTSS |= (1<<3);//previously we disabled the sequencer, now we need to enable it

   //SET THE ADC INTERRUPT SIGNAL
   ADC1->ISC = (1<<3);//clear the interrupt flags
   NVIC_EnableIRQ(ADC1SS3_IRQn);//ADC1SS3_IRQn is an interrupt specified in the lm4f120h5qr header, we are including it in cstartup_M file


   //CONFIGURE THE TIMER
   SYSCTL->RCGCTIMER |= (1<<0);//ENABLE CLOCK TO RCGCTIMER REGISTER
   TIMER0->CTL &= ~(1<<0);//ENSURE TIMER IS DISABLED BEFORE MAKING ANY CHANGES
   TIMER0->CFG |= 0x00000000;//GPM CONFIGURATION REGISTER
   TIMER0->TAMR |= ((0x2)<<0);//WE ARE USING TIMER A so tAmr(There are two type of timers, A and B, within Timer A, Timer0,1 are there)
   //0x2 is for periodic mode of timer operation
   TIMER0->TAMR &= ~(1<<4);//WE ARE PUTTING O IN BIT 4 SO THAT THE TIMER
                              //DIRECTION IS COUNT-DOWN NOT COUNT-UP
   TIMER0->TAILR = 0x1900;//AS WE HAVE DOWN COUNTER, WE NEED TO SET THE STARTING
                           //VALUE OF COUNTING, HERE IT IS 16,000,000 (ONE CLOCK CYCLE)
   TIMER0->CTL |= (1<<5);//THIS IS DONE TO ENABLE OUTPUT TIMER A ADC TRIGGER
   char c[10];
   int val[10];
   constant = 0;
   while((UART1->FR & (1<<4)) != 0);
   c[0] = UART1->DR;
   val[0] = c[0];
   constant = constant + val[0] - 48;
   for(int i=1;i<10;i++)
   {
     GPIOF->DATA = (1<<((i%3)+1));
     c[i] = readChar();
     val[i] = c[i];
     if(i<7)constant = constant + (val[i]-48)*pow(10,i);
   }
   reference = 0;
   //SET THE TIMER INTERRUPT SIGNAL
   TIMER0->IMR |= (1<<0);//INTERRUPT IS ENABLED
   NVIC_EnableIRQ(TIMER0A_IRQn);//INTERRUPT FUNCTION
   TIMER0->CTL |= (1<<0);//NOW WE ENABLE THE TIMER

   while(1){
   if((UART1->FR & (1<<4)) == 0)
   {
   time_ref = get_time_ref();
   reference = 0;
   }
   if(adc_flag == ADC_FLAG_TOGGLE_LED){
   //SHIFT THE adcResult VALUES AND KEEP UPDATED VALUE IN 0TH PLACE
   cosine_sum = cosine_sum - (adcResult[49]/25);
   for(i=49;i>0;i--){
   adcResult[i] = adcResult[i-1];
   }
   adcResult[0] = (((ADC1->SSFIFO3)*3.3/4096)-1.65);
   cosine_sum = 0;
   sine_sum = 0;
   for(i=0;i<50;i++)
   {
   cosine_sum = cosine_sum + adcResult[i]*cos_multiple[i];
   sine_sum = sine_sum + adcResult[i]*sin_multiple[i];
   }

   for(i=49;i>0;i--){
   IMAG[i] = IMAG[i-1];
   REAL[i] = REAL[i-1];
   TIME_STAMP[i] = TIME_STAMP[i-1];
   count[i] = count[i-1];
   }
   IMAG[0] = sine_sum;
   REAL[0] = cosine_sum;
   TIME_STAMP[0] = time_ref;
   count[0] = reference;

   reference++;
   adc_flag = ADC_FLAG_NONE;
   }
   }
   return 0;
 }
int get_time_ref()
{
   char c[10];
   int val[10];
   int time_ref = 0;
   for(int i=0;i<10;i++)
   {
     GPIOF->DATA = (1<<((i%3)+1));
     c[i] = readChar();
     val[i] = c[i];
     if(i>5)time_ref = time_ref + (val[i]-48)*pow(10,9-i);
   }
  return time_ref;
}
char readChar(void)
{
   char c;
   while((UART1->FR & (1<<4)) != 0);
   c = UART1->DR;
   return c;
}
 
