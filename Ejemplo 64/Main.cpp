/* Conexiones 
		Canal 1 			Canal 2		Canal 3		Canal 4
			PA6 Base 			PA7				PB0				PB1
*/

// *********************** LIBRERIAS **********************
		#include <stdio.h>
		#include "STM32F7xx.h"
// ********************************************************

// *********************** VARIABLES GLOBALES *************
		// Variables UART
			char caracter_frase='1'; // Variable donde le guardare los caracteres que vaya recorriendo en una frase
			short caracter1=0;
			short caracter2=0;  // Variables donde guardare la fragmentacion del valor a enviar
			short caracter3=0;
			short caracter4=0; 
			char dato_recibido=0;
			char vector_recibido[4]={0};
			short iterador=0;
			
		// Variables Systick
			short contador_delay=0;
			
		// Variables Timers
			short base=1800;
			short primer=1500;
			short segundo=500;
			short garra=1100;
			float grados_primer=0;
			float grados_segundo=0;
			float grados_base=0;
			float grados_garra=0;
			bool bandera_preguntar=0;
// ********************************************************

// *********************** FUNCIONES **********************
		void delay(short tiempo){
			contador_delay=0;
			while(contador_delay<=tiempo){}
		}
		void enviar_caracter(char caracter){ // Inicio de enviar uncaracter 
			UART4->TDR =caracter;
			while((UART4->ISR &=0x80)==0){}  
	} // Fin funcion de enviar un caracter 
		void enviar_frase(char frase[]){ // Inicio funcion enviar frase 
		for(int i=0;caracter_frase!='*';i++){ // Inicio for para enviar
			caracter_frase=frase[i]; // Lo que hago es que a una variable externa le mando el caracter i de la frase 
			UART4->TDR = caracter_frase; // El contenido de la variable externa la mando al registro de transmision para mandar por el serial 
			while((UART4->ISR &=0x80)==0){} // While de tiempo para una correcta transmision de datos  
		} // Fin for para enviar 
		caracter_frase='1';
	} // Fin funcion enviar frase 
		void dividir_datos(short resultado){ // Inicio de funcion de dividir datos 
				caracter1 = resultado/1000;
				caracter2 = (resultado/100)%10;
				caracter3 = (resultado%100)/10;
				caracter4 = (resultado%10);
		} // Fin funcion de dividir numeros
// ********************************************************

// *********************** INTERRUPCIONES *****************
		extern "C"{
			void SysTick_Handler(void){
				contador_delay++;
				if(contador_delay>8000){
					contador_delay=0;
				} 
			}
			void UART4_IRQHandler(void){
				if(UART4->ISR & 0x20){ // Si el dato esta completamente recibido 
						dato_recibido = UART4->RDR; // Guardo el dato 
						vector_recibido[iterador]=dato_recibido;
						iterador++;
						if(iterador>3){
							if(vector_recibido[0]=='b' || vector_recibido[0]=='B'){
								base=((vector_recibido[1]-0x30)*100+(vector_recibido[2]-0x30)*10+(vector_recibido[3]-0x30));
								grados_base=base*9.2889+585.6;
								TIM5->CCR1=grados_base;
							}else if(vector_recibido[0]=='p' || vector_recibido[0]=='P'){
								primer=(vector_recibido[1]-0x30)*100+(vector_recibido[2]-0x30)*10+(vector_recibido[3]-0x30);
								grados_primer=primer*9.2889+585.6;
								TIM5->CCR2=grados_primer;
							}else if(vector_recibido[0]=='s' || vector_recibido[0]=='S'){
								segundo=(vector_recibido[1]-0x30)*100+(vector_recibido[2]-0x30)*10+(vector_recibido[3]-0x30);
								grados_segundo=segundo*9.2889+585.6;
								TIM5->CCR3=grados_segundo;
							}else if(vector_recibido[0]=='g' || vector_recibido[0]=='G'){
								garra=(vector_recibido[1]-0x30)*100+(vector_recibido[2]-0x30)*10+(vector_recibido[3]-0x30);
								if(garra>=126){
									garra=126;
								}
								grados_garra=garra*9.2889+585.6;
								TIM5->CCR4=grados_garra;
							}else if(vector_recibido[0]=='q' || vector_recibido[0]=='Q'){
								bandera_preguntar=1;
							}else {
								enviar_frase("Comando Incorrecto*");
								iterador=0;
							}
							iterador=0;
						}
				}
			}
			void TIM3_IRQHandler(void){
				TIM3->SR &=~(0x01); // Apago la bandera de la interrupcion colocando un '0' en el bit 0
				
			}
		}
// ********************************************************

// *********************** MAIN ***************************
int main(void){
		// ******************* PUERTOS ************************

	RCC->AHB1ENR |=0x07; // A, B y C
		// ****************************************************
	
		// ******************* PINES **************************
				GPIOA->MODER |=0xAA; // ALTERNATIVO pin 0, 1, 2 y 3
				GPIOA->AFR[0]=0x2222; // FUNCION ALTERNA 2 pin 0, 1, 2 y 3
	
				GPIOC->MODER |=0xA00000; // Alternativo 11 
				GPIOC->AFR[1]=0x8800; // Funcion alterna 8 pin 11 
		// ****************************************************
	
		// ******************* SYSTICK ************************
				SystemCoreClockUpdate();
				SysTick_Config(SystemCoreClock/1000); // 1ms 
		// ****************************************************	
	
		// ******************* UART ***************************
				RCC->APB1ENR |=0x80000; // Activo el reloj del UART 4 
				UART4->BRR =0x683; // 9600 Baudios
				UART4->CR1 |=0x2C; // Activo Rx, Tx y la interrupcion por Rx
				UART4->CR1 |=0x01; // Habilito el modulo UART
				NVIC_EnableIRQ(UART4_IRQn); 
		// ****************************************************
	
		// ******************* TIMERS *************************
				RCC->APB1ENR |=0x08; // Activo el reloj del Timer 5
				TIM5->EGR |=0x01; // Habilito la actualizacion de los registros cuando cambie CCRx
				TIM5->PSC =15; // Esto para tener frecuencua de 1MHz cuyo periodo es de 1us 
				TIM5->DIER |=0x01; // Activo la interrupcion por actualizacion de conteo 
				TIM5->ARR =20000; // El periodo del conteo sera de 10us para conseguir el pulso de dicho ancho
				TIM5->CCER |=0x1111; // Conecto la señal PWM a su pin fisico en el canal 1, 2, 3 Y 4
				TIM5->CCMR1 = 0x6060; // PWM moodo 1 canal 1 y 2 
				TIM5->CCMR2 =0x6060; // PWM modo 1 canal 3 Y 4
				TIM5->CCR1 =base; // Señal de comparacion 1
				TIM5->CCR2 =primer; // Señal de comparacion 2
				TIM5->CCR3 =segundo; // Señal de comparacion 3
				TIM5->CCR4 =garra; // Señal de comparacion 4 
				TIM5->CR1 |=0x01;  // Activo el conteo 
				NVIC_EnableIRQ(TIM3_IRQn); // Activo la interrupcion 
		// ****************************************************
	
		// ******************* BUCLE **************************
				while(true){
					if(bandera_preguntar==1){
						bandera_preguntar=0;
						// Mostrar base
						enviar_frase("Base*");
						dividir_datos(grados_base);
						enviar_caracter(' ');
						enviar_caracter(caracter1+0x30);
						enviar_caracter(caracter2+0x30);
						enviar_caracter(caracter3+0x30);
						enviar_caracter(caracter4+0x30);
						enviar_caracter('\n');
						// Mostrar Primer
						enviar_frase("Primer*");
						dividir_datos(grados_primer);
						enviar_caracter(' ');
						enviar_caracter(caracter1+0x30);
						enviar_caracter(caracter2+0x30);
						enviar_caracter(caracter3+0x30);
						enviar_caracter(caracter4+0x30);
						enviar_caracter('\n');
						// Mostrar Segundo
						enviar_frase("Segundo*");
						enviar_caracter(' ');
						dividir_datos(grados_segundo);
						enviar_caracter(caracter1+0x30);
						enviar_caracter(caracter2+0x30);
						enviar_caracter(caracter3+0x30);
						enviar_caracter(caracter4+0x30);
						enviar_caracter('\n');
						// Mostrar Garra
						enviar_frase("Garra*");
						enviar_caracter(' ');
						dividir_datos(grados_garra);
						enviar_caracter(caracter1+0x30);
						enviar_caracter(caracter2+0x30);
						enviar_caracter(caracter3+0x30);
						enviar_caracter(caracter4+0x30);
						enviar_caracter('\n');
					}
				}
		// ****************************************************
}
// ********************************************************