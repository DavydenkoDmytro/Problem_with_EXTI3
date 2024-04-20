Version:
	V2.0.4: 
	- APDS9960 - implemented (need some fixes)
    - I2C Arbiter
    - QSPY ON
    - RTC - base functionality (sec IRQ)
      - Auto calibration
    - VFD control - base indication functionality
      - VFD changed from independent AO to HSM under UI AO
    - UI - build all types of data that should be displayed on VFD
    - LED HSM under UI AO
    - HAL BUZZER on TIM1 CH3 - based on IRQ (reworked with minor changes)
    - BUZZER HSM under UI AO
    - Buttons
    - PRESSURE AO added (bmp180 bs EMPTY)
    
   
   
DMA1_Channel1 - tim4_ch1
DMA1_Channel2 - SPI RX (tim1 ch1 уже нельзя)
DMA1_Channel3 - SPI TX
DMA1_Channel4 - I2C TX 
DMA1_Channel5 - I2C RX
DMA1_Channel6 - tim1_ch3 (BUZZER)
DMA1_Channel7

tim1 - BUZZER tim1_ch3
tim2 - pwm pa1
tim3 - интервальный таймер IRQHandler
tim4 - tim4_ch1 - dma1_ch1 - RGB LED

   