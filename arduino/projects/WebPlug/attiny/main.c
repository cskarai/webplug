#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include "../attinycmd.h"

//#define FAKE_ADC

void esp_reset_deactivate();

#define RESPONSE_BUF_SIZE 120
#define COMMAND_BUF_SIZE   16

#if RESPONSE_BUF_SIZE > 127
#error "Response buffer size can't be bigger than 127 byte"
#endif
#if COMMAND_BUF_SIZE > 127
#error "Response buffer size can t be bigger than 127 byte"
#endif

#define TIMER_1000HZ_BASE  (F_CPU / 1000)

#if TIMER_1000HZ_BASE < 2048
#define TIMER_FLAGS    _BV(CS01)
#define TIMER_TOP      ((TIMER_1000HZ_BASE + 4) / 8)
#elif TIMER_1000HZ_BASE < 16384
#define TIMER_FLAGS    _BV(CS01) | _BV(CS00)
#define TIMER_TOP      ((TIMER_1000HZ_BASE + 32) / 64)
#else
#define TIMER_FLAGS    _BV(CS02)
#define TIMER_TOP      ((TIMER_1000HZ_BASE + 128) / 256)
#endif

#define  ESP_RESET_LEN_MILLIS    10
#define  ESP_WATCHDOG_MILLIS     20000

#define  USI_DDR                 DDRA
#define  USI_PORT                PORTA
#define  USI_PIN                 PINA
#define  USI_SCK                 PA4
#define  USI_MISO                PA5
#define  USI_MOSI                PA6

#define  ESP_RESET_DDR           DDRA
#define  ESP_RESET_PORT          PORTA
#define  ESP_RESET_PIN_NUM       PA7

#define  ADC_ADMUX_CH_1          (0x02)
#define  ADC_ADMUX_CH_2          (0x03)
#define  ADC_ADNUX_DIFF_20X      (0x31)
#define  ADC_ADMUX_DIFF_20X_CAL  (0x25)

#define  ADC_AVG_SAMPLE          4
#define  ADC_AVG_MULTIPLIER      4
#define  ADC_CALIBRATION_SAMPLE  4
#define  ADC_AVG_SAMPLE_20X      4

volatile uint8_t  command_buf[COMMAND_BUF_SIZE];
volatile uint8_t  response_buf[RESPONSE_BUF_SIZE];

volatile uint8_t  command_head = 0;
volatile uint8_t  command_tail = 0;
volatile uint8_t  response_head = 0;
volatile uint8_t  response_tail = 0;
volatile uint8_t  send_nop_hi = 0;

volatile uint32_t timer_ticks = 0;

volatile uint8_t  esp_reset_cnt = 0;
volatile uint16_t esp_watchdog = ESP_WATCHDOG_MILLIS;

uint8_t           adc_admux  = ADC_ADMUX_CH_1;
int16_t           adc_avg    = 0;
uint8_t           adc_state  = 0;
int16_t           adc_offset = 0;

enum {
    ADC_ALTERING_SAMPLE    = 0x00,
    ADC_CALIBRATE_20X_SKIP = 0x10,
    ADC_CALIBRATE_20X      = 0x20,
    ADC_SAMPLE_20X_SKIP    = 0x30,
    ADC_SAMPLE_20X         = 0x40,
};

void esp_reset_activate();

uint32_t timer_millis()
{
  uint32_t tmp1, tmp2;
  do
  {
     tmp1 = timer_ticks;
     tmp2 = timer_ticks;
  }while(tmp1 != tmp2);
  return tmp1;
}

void timer_init()
{
  TCCR0A = _BV(WGM01);
  TCCR0B = TIMER_FLAGS;
  TCNT0 = 0;

  OCR0A = TIMER_TOP;

  /* enable 1000Hz interrupts */
  TIMSK0 |= _BV(OCIE0A);
}

ISR( TIM0_COMPA_vect )
{
  if( esp_reset_cnt )
  {
    if( ! --esp_reset_cnt )
      esp_reset_deactivate();
  }
  if( --esp_watchdog == 0 )
  {
    esp_watchdog = ESP_WATCHDOG_MILLIS;
    esp_reset_activate();
  }

  timer_ticks++;
}

void esp_reset_init()
{
  // set ESP reset to imput
  ESP_RESET_DDR  &= ~_BV(ESP_RESET_PIN_NUM);
  ESP_RESET_PORT &= ~_BV(ESP_RESET_PIN_NUM);
}

void esp_reset_deactivate()
{
  esp_reset_cnt = ESP_RESET_LEN_MILLIS;
  ESP_RESET_DDR  &= ~_BV(ESP_RESET_PIN_NUM);
}

void esp_reset_activate()
{
  esp_reset_cnt = ESP_RESET_LEN_MILLIS;
  ESP_RESET_DDR |= _BV(ESP_RESET_PIN_NUM);
}

void add_response( uint16_t response )
{
  int8_t fre = (int8_t)(response_tail - response_head);
  if( fre <= 0 )
    fre += RESPONSE_BUF_SIZE;

  if( fre <= sizeof(uint16_t) ) {
    response = RES_OVERFLOW_ERROR;
    if( response_head >= sizeof(uint16_t) )
      response_head -= sizeof(uint16_t);
    else
      response_head = RESPONSE_BUF_SIZE - sizeof(uint16_t);
  }
  
  response_buf[ response_head ] = (uint8_t)(response & 0xFF);
  response_buf[ response_head + 1 ] = (uint8_t)(response >> 8);
  uint8_t new_head = response_head + 2;
  if( new_head >= RESPONSE_BUF_SIZE )
    new_head -= RESPONSE_BUF_SIZE;
  response_head = new_head;
}

uint16_t get_command() {
  int8_t csize = (int8_t)(command_head - command_tail);
  if( csize < 0 )
    csize += COMMAND_BUF_SIZE;
  
  if( csize < sizeof(uint16_t) )
    return CMD_NONE;
  
  uint16_t res = command_buf[command_tail] + (((uint16_t)command_buf[command_tail + 1]) << 8);
  uint8_t new_command_tail = command_tail + 2;
  if( new_command_tail >= COMMAND_BUF_SIZE )
    new_command_tail -= COMMAND_BUF_SIZE;
  command_tail = new_command_tail;
  return res;
}

ISR(USI_OVF_vect)
{
  command_buf[command_head++] = USIDR;
  if( command_head == COMMAND_BUF_SIZE )
    command_head = 0;
  
  USISR = _BV(USIOIF);
  if( send_nop_hi )
  {
    send_nop_hi = 0;
    USIDR = (RES_NOP >> 8);
  }
  else
  {
    if( response_head == response_tail ) {
      send_nop_hi++;
      USIDR = (uint8_t)(RES_NOP & 0xFF);
    } else {
      USIDR = response_buf[response_tail++];
      if( response_tail == RESPONSE_BUF_SIZE )
        response_tail = 0;
    }
  }
}

void usi_init() {
  USI_DDR |= _BV(USI_MISO);
  USI_DDR &= ~_BV(USI_MOSI);
  USI_DDR &= ~_BV(USI_SCK);
    
  USICR = (0<<USISIE) | (1<<USIOIE) |                 // Disable start condition and enable overflow interrupt.
          (0<<USIWM1) | (1<<USIWM0) |                 // Set USI to thre-wire mode.
          (1<<USICS1) | (0<<USICS0) | (0<<USICLK) |   // Shift Register Clock Source = External, positive edge
          (0<<USITC);                                 // No toggle of clock pin.

  // Clear the interrupt flags and reset the counter.
  USISR = (1<<USISIF) | (1<<USIOIF) | (1<<USIPF) |    // Clear interrupt flags.
          (1<<USIDC) | (0x0<<USICNT0);                // USI to sample 8 bits or 16 edge toggles.
          
  USIDR = (uint8_t)(RES_NOP & 0xFF);
  send_nop_hi = 1;
}

#if defined FAKE_ADC
int16_t fake_adc = 0;
#endif

void process_adc()
{
  if(ADCSRA & _BV(ADIF)) {
    ADCSRA |= _BV(ADIF);
      
    int16_t adc = ADCL;
    adc |= (ADCH << 8);
    
#if defined FAKE_ADC
    if(( adc_state & 0xF1 ) == ADC_ALTERING_SAMPLE+1 )
      adc = 0x200;
    else {
      adc = fake_adc;
      fake_adc += 0x3F;
      if( fake_adc > 0x7FF )
        fake_adc -= 0x800;
      if( adc > 0x3FF )
        adc = 0x7FF - adc;
    }
#endif /* FAKE_ADC */    
    
    switch( adc_state & 0xF0 )
    {
      case ADC_ALTERING_SAMPLE:
        {
          uint8_t adc_cnt = adc_state & 0x0F;

          if( ! ( adc_cnt & 1 ) )
            adc = -adc; // subtract the zero channel
      
          ADMUX  = adc_admux = (ADC_ADMUX_CH_1 == adc_admux) ? ADC_ADMUX_CH_2 : ADC_ADMUX_CH_1;

          adc_avg += adc;
          if( ++adc_state == (ADC_AVG_SAMPLE + ADC_ALTERING_SAMPLE) ) {
            adc_avg = adc_avg * ADC_AVG_MULTIPLIER;
      
            add_response( (adc_avg & 0x3FFF) | RES_ADC_RESULT );
            adc_state = ADC_ALTERING_SAMPLE;
            adc_avg = 0;
          }
        }
        break;
      case ADC_CALIBRATE_20X_SKIP:
        adc_avg = 0;
        adc_state = ADC_CALIBRATE_20X;
        break;
      case ADC_CALIBRATE_20X:
        {
          if( adc & 0x200 )
            adc |= 0xFC00;

          adc_avg += adc;
          if( ++adc_state == (ADC_CALIBRATE_20X + ADC_CALIBRATION_SAMPLE) ) {
            ADMUX = ADC_ADNUX_DIFF_20X;
            adc_offset = adc_avg / ADC_CALIBRATION_SAMPLE;

            add_response( (adc_offset & 0x3FF) | RES_ADC_20X_CALIBRATION );
            adc_state = ADC_SAMPLE_20X_SKIP;
          }
        }
        break;
      case ADC_SAMPLE_20X_SKIP:
        adc_avg = 0;
        adc_state = ADC_SAMPLE_20X;
        break;
      case ADC_SAMPLE_20X:
        {
          if( adc & 0x200 )
            adc |= 0xFC00;

          adc_avg += adc - adc_offset;
          if( ++adc_state == (ADC_SAMPLE_20X + ADC_AVG_SAMPLE_20X) ) {

            adc_avg = (adc_avg / ADC_AVG_SAMPLE_20X) * 4 / 5;
            add_response((adc_avg & 0x3FFF) | RES_ADC_RESULT);
            
            adc_state = ADC_SAMPLE_20X;
            adc_avg = 0;
          }
        }
        break;
    }
  }
}

void adc_start()
{
  adc_avg = 0;
  adc_state = ADC_ALTERING_SAMPLE;
  ADCSRA = 0;
  ADCSRB = 0;
  ADCSRA = (1<<ADEN)|(0<<ADIE)|(1<<ADATE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADIF);
  ADMUX  = adc_admux = ADC_ADMUX_CH_1;
  ADCSRA |= (1<<ADSC);
  
  ADMUX  = adc_admux = (ADC_ADMUX_CH_1 == adc_admux) ? ADC_ADMUX_CH_2 : ADC_ADMUX_CH_1;
}

void adc_start_20x()
{
  adc_avg = 0;
  adc_state = ADC_CALIBRATE_20X_SKIP;
  ADCSRA = 0;
  ADCSRB = 0;
  ADCSRA = (1<<ADEN)|(0<<ADIE)|(1<<ADATE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADIF);
  ADCSRB = (1<<BIN);
  ADMUX  = ADC_ADMUX_DIFF_20X_CAL;
  ADCSRA |= (1<<ADSC);
}

void adc_stop()
{
  ADCSRA = 0;
  ADCSRB = 0;
}

void process_command()
{
  uint16_t cmd = get_command();
  switch(cmd)
  {
    case CMD_REQUEST_HEARTBEAT:
      esp_watchdog = ESP_WATCHDOG_MILLIS;
      add_response(RES_HEARTBEAT_OK);
      break;
    case CMD_READ_PIN_A:
      add_response(RES_PIN_A + PINA);
      break;
    case CMD_READ_PIN_B:
      add_response(RES_PIN_B + PINB);
      break;
    case CMD_READ_DDR_A:
      add_response(RES_DDR_A + DDRA);
      break;
    case CMD_READ_DDR_B:
      add_response(RES_DDR_B + DDRB);
      break;
    case CMD_READ_PORT_A:
      add_response(RES_PORT_A + PORTA);
      break;
    case CMD_READ_PORT_B:
      add_response(RES_PORT_B + PORTB);
      break;
    case CMD_ADC_START:
      adc_start();
      break;
    case CMD_ADC_START_20X:
      adc_start_20x();
      break;
    case CMD_ADC_STOP:
      adc_stop();
      break;
    case CMD_ESP_RESET:
      esp_reset_activate();
      break;
    case CMD_NOP:
    case CMD_NONE:
    default:
      {
        uint8_t arg = (uint8_t)(cmd & 0xFF);
        cmd = cmd & 0xFF00;
        
        switch(cmd)
        {
            case CMD_SET_DDR_A:
              DDRA = arg;
              break;
            case CMD_SET_DDR_A_BITS:
              DDRA |= arg;
              break;
            case CMD_RESET_DDR_A_BITS:
              DDRA &= ~arg;
              break;
            case CMD_SET_DDR_B:
              DDRB = arg;
              break;
            case CMD_SET_DDR_B_BITS:
              DDRB |= arg;
              break;
            case CMD_RESET_DDR_B_BITS:
              DDRB &= ~arg;
              break;
            case CMD_SET_PORT_A:
              PORTA = arg;
              break;
            case CMD_SET_PORT_A_BITS:
              PORTA |= arg;
              break;
            case CMD_RESET_PORT_A_BITS:
              PORTA &= ~arg;
              break;
            case CMD_TOGGLE_PORT_A_BITS:
              PINA = arg;
              break;
            case CMD_SET_PORT_B:
              PORTB = arg;
              break;
            case CMD_SET_PORT_B_BITS:
              PORTB |= arg;
              break;
            case CMD_RESET_PORT_B_BITS:
              PORTB &= ~arg;
              break;
            case CMD_TOGGLE_PORT_B_BITS:
              PINB = arg;
              break;
            default:
              /* do nothing */
              break;
        }
      }
      break;
  }
}

int main()
{
  // configure clock to 8MHz
  CLKPR = _BV(CLKPCE);
  CLKPR = 0;   // presc 1

  wdt_enable(WDTO_2S);

  usi_init();
  timer_init();
  esp_reset_init();
  sei();
  
  add_response(RES_SYSTEM_STARTED);
  
  while(1)
  {
    wdt_reset();
    
    process_adc();
    process_command();
  }
}
