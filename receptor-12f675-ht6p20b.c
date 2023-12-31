#include<12F675.h>
//#device adc=8

#FUSES NOWDT                    //No Watch Dog Timer
#FUSES INTRC_IO                 //Internal RC Osc, no CLKOUT
#FUSES NOCPD                    //No EE protection
#FUSES PROTECT                  //Code protected from reads
#FUSES NOMCLR                   //Master Clear pin used for I/O
#FUSES PUT                      //Power Up Timer
#FUSES NOBROWNOUT               //No brownout reset

#use delay(clock=4000000)

//#ignore_warnings 203,202  //desconsidera mensagem de 'condition always true'

#ZERO_RAM
#use fast_io(a)
#byte gpio=0x05
//#bit LEDG=0x05.4//pino A4 led 
#bit RF  =0x05.3//entrada do sinal de RF pino 4
#define NBIT 28 // numero de bits
#define T_basico 500 //tempo basico (pulso inicial da transmiss�o) 
#define t_intervalo 400 // intervalo entre transmiss�es (400 a 5000) 
#ROM  0X3FF = {0X3460}

#define liga PIN_A5
#define botao PIN_A0

int1 LEDG;
int32 buffer=0; //buffer de dados a receber
int32 bufferx=0;//buffer de copia da transmiss�o anterior
int16 timerx=0;
void testa_lrn(); //prototipo
  int a=0;

//==============================================================================
//          Rotina para captura de bits enviados pelo tx (ht6p20b)
//==============================================================================

int1 get_htp(void) 
 {
    
   int8  x=0;
   buffer=0;
   while(!RF)
   {
 
     if(!++timerx) 
        return false;
   }

  if(timerx>t_intervalo)//  intervalo entre transmiss�es (23te x pulso basico)
    { 

      for(x=0;x<NBIT-1;x++)
        {
         while(RF) 
         {
         
         }
         delay_us(T_basico/2);//meio t_basico

         if(RF){
         return FALSE;
         }

         delay_us(T_basico);

         if(!RF) {
         buffer|=0x08000000;
         buffer>>=1;
         }
         else buffer>>=1;

         delay_us(T_basico);

         if(!RF){  return FALSE; }
      }

      return TRUE;
   }

   else return FALSE;
}


//==============================================================================
//          Rotina de pesquisa por controle gravado na eeprom
//==============================================================================
int1 procura()
{

  int8 a;
  for(a=0;a<126;a+=3){
    if(make8(buffer,2)!=read_eeprom(a)) continue ;//achou o primeiro byte?
    if(make8(buffer,1)!=read_eeprom(a+1))continue ;//achou o segundo byte?
    if(make8(buffer,0)!=read_eeprom(a+2))continue ;//achou o terceiro byte?
    return true;//sim! Os tres!
  }
  return false;//n�o!
}

//==============================================================================
//    Rotina para obter o endere�o que deve ser gravado o novo controle
//==============================================================================

int8 end_gravar(){
  int8 a;

   for(a=0;a<126;a+=3){
    if(read_eeprom(a)!=0xff) continue ;//byte livre?
    if(read_eeprom(a+1)!=0xff) continue ;//byte livre?
    if(read_eeprom(a+2)!=0xff) continue ;//byte livre?
    return a;//retorna o endere�o inicial de 3 bytes livres
   }
   return 0xff;//mem�ria cheia
}

//==============================================================================
//    Rotina de grava��o na EEprom do novo controle
//==============================================================================


int8 gravar()
{

  int8 adr;
  adr=end_gravar();
  if(!(adr+1)){return 0xff;}//n�o grava pois mem�ria est� cheia
  else{
  write_eeprom(adr,make8(buffer,2));//grava o primeiro byte
  write_eeprom(adr+1,make8(buffer,1));//grava o segundo byte
  write_eeprom(adr+2,make8(buffer,0));//grava o terceiro byte
  disable_interrupts(INT_TIMER0);
  return 0x00;//ok
  }
  
}

//==============================================================================
//          Rotina de tratamento do tx recebido
//==============================================================================

void action()
 {
 
  if(procura()) //se tx foi encontrado na eeprom vai acionar as sa�das
  {
   int8 aux;
  
   aux= make8(buffer,2);
   
    if((bit_test(aux,7))&&(bit_test(aux,6))||(bit_test(aux,7))||(bit_test(aux,6)))
    {
          
        for(int i = 0; i < 50; i++){
          output_toggle(liga); 
            output_toggle(pin_a4); 
            delay_ms(30);
        }
         output_low(liga);   
          output_high(pin_a4);
    }
  
    LEDG=0;
 }
   
  else{
  
   if(LEDG)
   {
      if(!gravar()) //grava��o ok, led learn apaga
      { 
        LEDG=0; 
        output_high(pin_a4);
        delay_ms(300);
      }
   }
  }

} 
 
//==============================================================================
//         Rotina de recep��o do tx comparando 2 recep��es consecutivas
//==============================================================================


void recebe_tx()
{ 
   static int1 fr;
    if(get_htp())
     {
      if(!fr) //salva a primeira recep��o
      {
        bufferx=buffer; 
        fr=1;
      }
      else
      {
         fr=0;
         if(bufferx==buffer)
            { 
               action(); 
               bufferx=0; 
            }//compara a primeira com a segunda
      }
    }
}





//==============================================================================
//         IMER0 CONTAGEM DO BOT�O PROG
//==============================================================================


//==============================================================================
//          Rotina principal
//==============================================================================


void main()
{
 set_tris_a(0b001001);
 output_low(liga); //desliga saida
 output_high(pin_a4); //desliga led

   setup_adc_ports(NO_ANALOGS|VSS_VDD);
   setup_adc(ADC_CLOCK_DIV_2);
   setup_timer_0(RTCC_INTERNAL|RTCC_DIV_256);
   setup_timer_1(T1_INTERNAL|T1_DIV_BY_8);
  // setup_ccp1(CCP_OFF);
  // setup_comparator(NC_NC);
   setup_vref(FALSE);
  
   LEDG=0;   








//==============================================================================
//           Loop principal
//==============================================================================

   while(true)
   {    
    if(input(botao))
      {
         LEDG = 1;
         output_low(pin_a4);
         delay_ms(200);
      }
     recebe_tx();//verifica se houve uma recep��o de tx      
   }
}
