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

float quantile(const uint16_t *arr, const uint16_t n, const float per, float lr=.03f, const int max_iter=40, const int close_enough=40)
{

    // Serial.println(" ");
    // Serial.println("***");
    // Serial.println(__LINE__);
    // target number of elements above threshold
    float target = (float)n * (1.0f - per);

    // find min and max
    uint16_t low = arr[0], high = arr[0];
    for (uint16_t i = 1; i < n; i++)
    {
        if (arr[i] < low)
            low = arr[i];
        if (arr[i] > high)
            high = arr[i];
    }

    // initial guess
    float thres = (high - low) * per + low;
    float pre_error = 0;

    for (int iter = 0; iter < max_iter; iter++)
    {
        int count = 0;
        for (uint16_t i = 0; i < n; i++)
            if (arr[i] > thres)
                count++;

        float error = (float)count - target;

        // Serial.print(thres);
        // Serial.print(" ");
        // Serial.print(count);
        // Serial.print(" ");
        // Serial.print(error);
        // Serial.print(" ");
        // Serial.println(iter);

        if (fabsf(error) < close_enough)
            break; // close enough

        // shrink learning rate
        // if error bouncing positive and negative, cut it in half.
        if (iter && (error * pre_error) < 0)
            lr *= 0.6f;
        else        
            lr *= 0.95f;

        thres += error * lr;
        pre_error = error;
    }

    return thres;
}

float quantile(const float *arr,    const uint16_t n, const float per, float lr=.03f, const int max_iter=40, const int close_enough=40)
{
    // target number of elements above threshold
    float target = (float)n * (1.0f - per);
    // Serial.println(" ");
    // Serial.println("***");
    // Serial.println(__LINE__);
    // find min and max
    float low = arr[0], high = arr[0];
    for (uint16_t i = 1; i < n; i++)
    {
        if (arr[i] < low)
            low = arr[i];
        if (arr[i] > high)
            high = arr[i];
    }

    // initial guess
    float thres = (high - low) * per + low;
    float pre_error = 0;

    for (int iter = 0; iter < max_iter; iter++)
    {
        int count = 0;
        for (uint16_t i = 0; i < n; i++)
            if (arr[i] > thres)
                count++;

        float error = (float)count - target;

        // Serial.print(thres);
        // Serial.print(" ");
        // Serial.print(count);
        // Serial.print(" ");
        // Serial.print(error);
        // Serial.print(" ");
        // Serial.println(iter);

        if (fabsf(error) < close_enough)
            break; // close enough

        // shrink learning rate
        // if error bouncing positive and negative, cut it in half.
        if (iter && (error * pre_error) < 0)
            lr *= 0.6f;
        else        
            lr *= 0.95f;

        thres += error * lr;
        pre_error = error;
    }

    return thres;
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

// about 20 ms to excecute
// which is fine becuase it takes 125ms to fill the array anyways.
float RPM(uint16_t *const arr)
{
    static float float_arr[ARR_LEN];
    uint16_t n = ARR_LEN;
    static float previous_rpm;
    // static float avg_rpm = 0;
    // auto start = micros();
    // print(arr);
    // clip to quantile range
    uint16_t top = quantile(arr, n, QUANTILE);
    uint16_t bottom = quantile(arr, n, 1.0f - QUANTILE);

    // just picking up ambient noise.
    if (top - bottom < 20 || bottom > 1000)
        return 0;

    for (uint16_t i = 0; i < n; i++)
    {
        if (arr[i] > top)
            float_arr[i] = top;
        else if (arr[i] < bottom)
            float_arr[i] = bottom;
        else
            float_arr[i] = (float)arr[i];
    }

    // EMA smoothing
    float_arr[0] = (float)arr[0];
    for (uint16_t i = 1; i < n; i++)
        float_arr[i] = (uint16_t)(EMA * float_arr[i - 1] + (1.0f - EMA) * float_arr[i]);

    // derivative with jump
    for (uint16_t i = 0; i < n - DERIVATIVE_JUMP; i++)
    {
        if (float_arr[i + DERIVATIVE_JUMP] > float_arr[i])
            float_arr[i] = (uint16_t)(float_arr[i + DERIVATIVE_JUMP] - float_arr[i]);
        else
            float_arr[i] = 0;
    }
    n -= DERIVATIVE_JUMP;
    
    // quantile clip high then binary threshold
    top = quantile(float_arr, n, QUANTILE - 0.05f);
    uint16_t minv = float_arr[0], maxv = float_arr[0];
    for (uint16_t i = 1; i < n; i++)
    {        
        if (float_arr[i] > top)
            float_arr[i] = top;
        if (float_arr[i] < minv)
            minv = float_arr[i];
        if (float_arr[i] > maxv)
            maxv = float_arr[i];
    }

    // count rising edges
    uint16_t middle = (uint16_t)((minv + maxv) / 2);
    int prev = (float_arr[0] > middle);
    float count = 0;
    float edge_delay_sum = 0;
    uint16_t previous_edge_time = 0;
    for (uint16_t i = 1; i < n; i++)
    {
        int curr = float_arr[i] > middle;
        if (!prev && curr)
        {
            if (previous_edge_time)
                edge_delay_sum += i - previous_edge_time;
            count++;
            previous_edge_time = i;
        }
        prev = curr;
    }

    // Serial.println(micros() - start);
    
    if (count > 5)
    {
        const float rpm = count / (edge_delay_sum * (TIMER_INTERVAL_US / 60e6)) / BLADES;
        const float pre_rpm = previous_rpm;
        previous_rpm = rpm;
        if (fabsf(rpm - pre_rpm) < 800)
            return rpm;
    }
    return 0;
}

#define SOUND_EMA 0.9f
float process_sound(uint16_t *const arr)
{

    // Mean
    uint32_t mean_int = 0;
    for (uint16_t i = 0; i < ARR_LEN; i++)
        mean_int += arr[i];
    const float mean = (double)mean_int / ARR_LEN;

    // RMS
    float sumsq = 0;
    for (uint16_t i = 0; i < ARR_LEN; i++)
    {
        float centered = arr[i] - mean;
        sumsq += centered * centered;
    }
    float rms = sqrtf(sumsq / ARR_LEN);

    // dB relative to ADC full scale
    return 20.0f * log10f(rms / 4095.0);
}

// 7 us-ish
void TimerHandler()
{
    light[array_using][array_index] = readADC(LIGHT_SENSOR);
    sound[array_using][array_index++] = readADC(SOUND_SENSOR);

    if (array_index == ARR_LEN)
    {
        array_using = !array_using;
        arrays_full = true;
        array_index = 0;
    }
    in_isr = false;
}

void setup()
{
    Serial.begin(115200);
    setupADC();

    // while(!Serial.available());
    // Serial.read();
    ITimer.attachInterruptInterval(TIMER_INTERVAL_US, TimerHandler);
    scale.begin();
    scale.tare();
}

void loop()
{
    if (arrays_full)// && Serial.available())
    {
        Serial.read();
        arrays_full = false;
        // Serial.read();
        uint16_t *const light_buf = light[!array_using];
        uint16_t *sound_buf = sound[!array_using];
        auto rpm = RPM(light_buf);
        Serial.print(rpm);
        Serial.print(" ");
        Serial.print(process_sound(sound_buf));
        Serial.print(" ");
        Serial.println(scale.get_weight(20), 2);

        if (Serial.available())
        {   
            if (Serial.read() == 's')
            {
                noInterrupts();
                for (int i = 0; i < ARR_LEN; ++i)
                {
                    Serial.println(sound_buf[i]);
                }
                interrupts();
            }
            scale.tare();
        }
    }
}













