/*
 *	11/26/17 by C.P.: Version 0.8.0 - Universal Version
 */

#include "usbd_midi_if.h"
#include "stm32f7xx_hal.h"

static void MIDI_HandleMessage(const uint8_t * const pData, uint16_t length);

extern DAC_HandleTypeDef hdac;
USBD_MIDI_ItfTypeDef USBD_Interface_midi_fops_FS =
        {
		        MIDI_HandleMessage,
        };

//See the USB MIDI spec, page 16 for these.
typedef enum {
	RESERVED = 0x0u,
	CABLE_RESERVED = 0x1u,
	SYSTEM_COMMON_2 = 0x2u,
	SYSTEM_COMMON_3 = 0x3u,
	SYSEX_START = 0x4u,
	SYSEX_ENDS_1 = 0x5u,
	SYSEX_ENDS_2 = 0x6u,
	SYSEX_ENDS_3 = 0x7u,
	NOTE_STOP = 0x8u,
	NOTE_PLAY = 0x9u,
	POLY_KEYPRESS = 0xAu,
	CONTROL_CHANGE = 0xBu,
	PROGRAM_CHANGE = 0xCu,
	CHANNEL_PRESSURE = 0xDu,
	PITCHBEND_CHANGE = 0xEu,
	SINGLE_BYTE_UNPARSED = 0xFu,
} CIN_Types;
#define CABLE_INDEX_MASK(x) (((x) & 0xF0u) >> 4u)
#define CIN_MASK(x) ((CIN_Types)((x) & 0x0Fu))

//Used to define the range of this Midi-2-CV.
#define NOTE_C2 36
#define NOTE_C7 96

//The STM32 ADC is not completely linear.
#define ADC_LIN_OFFSET 92
#define ADC_MAX ((((uint8_t)1u)<<12u) - ADC_LIN_OFFSET)
#define ADC_MIN (ADC_LIN_OFFSET)
#define ADC_RANGE (ADC_MAX - ADC_MIN)

static void MIDI_HandleMessage(const uint8_t * const pData, uint16_t length)
{
    uint8_t cableIndex = CABLE_INDEX_MASK(pData[0]);
	CIN_Types codeIndexNumber = CIN_MASK(pData[0]);
    uint8_t midi_0 = length > 1 ? pData[1] : 0u;
    uint8_t midi_1 = length > 2 ? pData[2] : 0u;
	uint8_t midi_2 = length > 3 ? pData[3] : 0u;

    switch (codeIndexNumber)
    {
        case NOTE_STOP:
            HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GATE_1_GPIO_Port, GATE_1_Pin, GPIO_PIN_RESET);
            break;
        case NOTE_PLAY:
            if (midi_2 != 0 && midi_1 >= NOTE_C2 && midi_1 <= NOTE_C7) {
                uint8_t note_offset = midi_0 - NOTE_C2;
                int data = (note_offset * ADC_RANGE) / (NOTE_C7 - NOTE_C2) + ADC_MIN;
                HAL_GPIO_WritePin(GATE_1_GPIO_Port, GATE_1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET);
                HAL_DAC_SetValue(&hdac, DAC1_CHANNEL_1, DAC_ALIGN_12B_R, data);
            } else {
                HAL_GPIO_WritePin(GATE_1_GPIO_Port, GATE_1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
            }
            break;
        default:
            break;
    }
}

