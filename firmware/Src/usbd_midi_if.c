/*
 *	11/26/17 by C.P.: Version 0.8.0 - Universal Version
 */

#include "usbd_midi_if.h"
#include "stm32f7xx_hal.h"

static uint16_t MIDI_DataRx(uint8_t *msg, uint16_t length);
static uint16_t MIDI_DataTx(uint8_t *msg, uint16_t length);
extern DAC_HandleTypeDef hdac;
USBD_MIDI_ItfTypeDef USBD_Interface_midi_fops_FS =
        {
                MIDI_DataRx,
                MIDI_DataTx
        };

//See the USB MIDI spec, page 16 for these.
#define MSG_NOTE_STOP 0x80u
#define MSG_NOTE_PLAY 0x90u
#define MSG_POLY_KEYPRESS 0xA0u
#define MSG_CONTROL_CHANGE 0xB0u
#define MSG_PROGRAM_CHANGE 0xC0u
#define MSG_CHANNEL_PRESSURE 0xD0u
#define MSG_PITCHBEND_CHANGE 0xE0u
#define MSG_SINGLE_BYTE 0xF0u

//Used to define the range of this Midi-2-CV.
#define NOTE_C2 36
#define NOTE_C7 96

//The STM32 ADC is not completely linear.
#define ADC_LIN_OFFSET 92
#define ADC_MAX ((((uint8_t)1u)<<12u) - ADC_LIN_OFFSET)
#define ADC_MIN (ADC_LIN_OFFSET)
#define ADC_RANGE (ADC_MAX - ADC_MIN)

static uint16_t MIDI_DataRx(uint8_t *msg, uint16_t length)
{
    uint8_t chan = msg[1] & 0xfu;
    uint8_t msgtype = msg[1] & 0xf0u;
    uint8_t b1 =  msg[2];
    uint8_t b2 =  msg[3];
    uint16_t b = ((b2 & 0x7fu) << 7u) | (b1 & 0x7fu);

    switch (msgtype)
    {
        case MSG_NOTE_STOP:
            HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GATE_1_GPIO_Port, GATE_1_Pin, GPIO_PIN_RESET);
            break;
        case MSG_NOTE_PLAY:
            if (b2 != 0 && b1 >= NOTE_C2 && b1 <= NOTE_C7) {
                uint8_t note_offset = b1 - NOTE_C2;
                int data = (note_offset * ADC_RANGE) / (NOTE_C7 - NOTE_C2) + ADC_MIN;
                HAL_GPIO_WritePin(GATE_1_GPIO_Port, GATE_1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET);
                HAL_DAC_SetValue(&hdac, DAC1_CHANNEL_1, DAC_ALIGN_12B_R, data);
            } else {
                HAL_GPIO_WritePin(GATE_1_GPIO_Port, GATE_1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
            }
            break;
        case MSG_POLY_KEYPRESS:
            break;
        case MSG_CONTROL_CHANGE:
            break;
        case MSG_PROGRAM_CHANGE:
            break;
        case MSG_CHANNEL_PRESSURE:
            break;
        case MSG_PITCHBEND_CHANGE:
            break;
        case MSG_SINGLE_BYTE:
            break;
    }
    return 0;
}

static uint16_t MIDI_DataTx(uint8_t *msg, uint16_t length)
{
    uint32_t i = 0;
    while (i < length) {
        APP_Rx_Buffer[APP_Rx_ptr_in] = *(msg + i);
        APP_Rx_ptr_in++;
        i++;
        if (APP_Rx_ptr_in == APP_RX_DATA_SIZE) {
            APP_Rx_ptr_in = 0;
        }
    }
    return USBD_OK;
}
