
adc_init

ADC_VR adc_input
ADC_TEMP adc_input
ADC_AN11 adc_input

TEMP adc_sample . CR

VOLTAGE adc_sample . CR

AN11 adc_sample . CR


: run-sample
    lcd_init
    lcd_clear 
    s" voltage " lcd_append_string
    
    sample activate  
    begin 
        0 8 lcd_position 
        VOLTAGE  adc_sample 100 * 333 /  lcd_append_decimal 
        lcd_append_space  
        500 ms 
   again
;
