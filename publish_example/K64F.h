#if !defined(K64F_H)
#define K64F_H

BusOut led2 (LED_BLUE);
BusOut r (D5);
BusOut g (D9);
BusOut b (D8);
DigitalIn Up(A2); DigitalIn Down(A3); DigitalIn Right(A4); DigitalIn Left(A5); DigitalIn Click(D4);
AnalogIn ain1(A0); AnalogIn ain2(A1);

#define LED2_OFF 1
#define LED2_ON 0

static uint32_t linkStatus(void)
{
    return (1);
}

#endif