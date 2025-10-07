#include "main.h"
#include "HX711.h"




static inline void setupADC(void)
{
    Adc *const adc = ADC0;

    // Route pins A0 and A1 to ADC peripheral
    pinPeripheral(A0, PIO_ANALOG);
    pinPeripheral(A1, PIO_ANALOG);

    // Disable before reconfig
    while (adc->SYNCBUSY.bit.ENABLE)
        ;
    adc->CTRLA.bit.ENABLE = 0;
    while (adc->SYNCBUSY.bit.ENABLE)
        ;

    // one sample
    adc->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_1 | ADC_AVGCTRL_ADJRES(0);

    // Minimal sample time (increase if high source impedance)
    adc->SAMPCTRL.reg = 0;

    // ADC clock prescaler (try DIV8 for stability)
    adc->CTRLA.bit.PRESCALER = ADC_CTRLA_PRESCALER_DIV8_Val;

    // Resolution
    adc->CTRLB.bit.RESSEL = ADC_CTRLB_RESSEL_12BIT_Val;
    adc->CTRLB.bit.FREERUN = 0; // single-shot mode

    // Enable ADC
    while (adc->SYNCBUSY.reg)
        ;
    adc->CTRLA.bit.ENABLE = 1;
    while (adc->SYNCBUSY.reg)
        ;

    // Do a dummy conversion once (helps settle ref/mux)
    adc->INPUTCTRL.reg =
        ADC_INPUTCTRL_MUXPOS(g_APinDescription[A0].ulADCChannelNumber) |
        ADC_INPUTCTRL_MUXNEG_GND;
    while (adc->SYNCBUSY.bit.INPUTCTRL)
        ;
    adc->INTFLAG.reg = ADC_INTFLAG_RESRDY;
    adc->SWTRIG.bit.START = 1;
    while (!adc->INTFLAG.bit.RESRDY)
        ;
    (void)adc->RESULT.reg;
}

static inline uint16_t readADC(const uint8_t pin)
{
    Adc *const adc = ADC0;

    // Select channel for this pin
    adc->INPUTCTRL.reg =
        ADC_INPUTCTRL_MUXPOS(g_APinDescription[pin].ulADCChannelNumber) |
        ADC_INPUTCTRL_MUXNEG_GND;
    while (adc->SYNCBUSY.bit.INPUTCTRL)
        ;

    // Clear ready flag
    adc->INTFLAG.reg = ADC_INTFLAG_RESRDY;

    // Start conversion
    adc->SWTRIG.bit.START = 1;

    // Wait for completion
    while (!adc->INTFLAG.bit.RESRDY)
        ;

    return adc->RESULT.reg;
}

void print(uint16_t *const arr)
{
    for (int i = 0; i < ARR_LEN; ++i)
        Serial.println(arr[i]);
}

void print(float *const arr)
{
    for (int i = 0; i < ARR_LEN; ++i)
        Serial.println(arr[i]);
}

// 138 us to excecute
void TimerHandler()
{
    // auto start = micros();
    light[array_index] = readADC(LIGHT_SENSOR);
    vibration[array_index] = Accel.readZ();
    sound[array_index++] = readADC(SOUND_SENSOR);
    // auto dur = micros() - start;
    // Serial.println(dur);
    if (array_index == ARR_LEN)
    {
        arrays_full = true;
        array_index = 0;
    }
}

void send_arrays_binary(float lift) {

    uint32_t header[4] = {123456, 654321, ARR_LEN, (uint32_t)(lift * 1000 + 0.5f)};
    Serial.write((uint8_t*)&header, sizeof(header));
    Serial.flush();
    delay(5);

    const uint8_t* ptrs[3] = {
        (uint8_t*)sound,
        (uint8_t*)light,
        (uint8_t*)vibration
    };

    // each array: ARR_LEN * 2 bytes
    const size_t ARR_BYTES = ARR_LEN * sizeof(uint16_t);

    for (int arr_i = 0; arr_i < 3; arr_i++) {
        const uint8_t* src = ptrs[arr_i];
        size_t remaining = ARR_BYTES;
        uint16_t chunk_id = 0;

        while (remaining > 0) {
            size_t n = remaining > CHUNK_SIZE ? CHUNK_SIZE : remaining;

            // 8-byte packet header
            struct __attribute__((packed)) {
                uint8_t tag;        // 0xA0 + array index
                uint16_t chunk_id;  // sequential packet id
                uint16_t len;       // payload bytes
                uint8_t arr_idx;    // 0=sound,1=light,2=vibration
                uint8_t reserved;
            } pkt_hdr = { (uint8_t)(0xA0 + arr_i), chunk_id, (uint16_t)n, (uint8_t)arr_i, 0 };

            Serial.write((uint8_t*)&pkt_hdr, sizeof(pkt_hdr));
            Serial.write(src, n);
            Serial.flush();

            src += n;
            remaining -= n;
            chunk_id++;
            delay(1);  // tiny gap gives host breathing room
        }
    }

    const char end[] = "END";
    Serial.write((uint8_t*)end, sizeof(end) - 1);
    Serial.flush();
}


void setup()
{
    Serial.begin(115200);
    // while(!Serial.available());
    // Serial.read();

    while(!Serial);
    setupADC();
    Accel.begin();
    ITimer.attachInterruptInterval(TIMER_INTERVAL_US, TimerHandler);
    scale.begin();
}

void loop()
{
    float weight;

    // while(1)
    // {
    //     Serial.println(scale.get_weight());
    //     if (Serial.available())
    //     {
    //         if (Serial.read() == 't')
    //             scale.tare();
    //     }
    // }

    if (arrays_full && Serial.available())
    {
        arrays_full = false; 
        noInterrupts();
        switch (Serial.read())
        {
            case 't':
                scale.tare();
                break;

            case 'w':
                weight = scale.get_weight();
                Serial.write((uint8_t*)&weight, sizeof(float));
                break;

            case ' ':
                send_arrays_binary(scale.get_weight());

            default:
                break;
        }
        interrupts();
    }
}












