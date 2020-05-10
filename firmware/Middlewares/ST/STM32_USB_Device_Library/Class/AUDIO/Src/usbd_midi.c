/**
  ******************************************************************************
  * @file    usbd_midi.c
  ******************************************************************************

    (CC at)2016 by D.F.Mac. @TripArts Music

*/

/* Includes ------------------------------------------------------------------*/
#include "usbd_midi.h"
#include "usbd_desc.h"
#include "stm32f7xx_hal_conf.h"
#include "usbd_ctlreq.h"
#include "stm32f7xx_hal.h"

static uint8_t  USBD_MIDI_Init (USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  USBD_MIDI_DeInit (USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  USBD_MIDI_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_MIDI_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum);

static uint8_t  *USBD_MIDI_GetCfgDesc (uint16_t *length);
//uint8_t  *USBD_MIDI_GetDeviceQualifierDescriptor (uint16_t *length);
USBD_HandleTypeDef *pInstance = NULL;

uint32_t APP_Rx_ptr_in  = 0;
uint32_t APP_Rx_ptr_out = 0;
uint32_t APP_Rx_length  = 0;
uint8_t  USB_Tx_State = 0;

__ALIGN_BEGIN uint8_t USB_Rx_Buffer[MIDI_DATA_OUT_PACKET_SIZE] __ALIGN_END ;
__ALIGN_BEGIN uint8_t APP_Rx_Buffer[APP_RX_DATA_SIZE] __ALIGN_END ;

/* USB MIDI interface class callbacks structure */
USBD_ClassTypeDef  USBD_MIDI =
        {
                USBD_MIDI_Init,
                USBD_MIDI_DeInit,
                NULL,
                NULL,
                NULL,
                USBD_MIDI_DataIn,
                USBD_MIDI_DataOut,
                NULL,
                NULL,
                NULL,
                USBD_MIDI_GetCfgDesc, // HS
                USBD_MIDI_GetCfgDesc,// FS
                NULL,// OTHER SPEED
                NULL,// DEVICE_QUALIFIER
        };

#define MIDI_JACK_USB_OUT 0x01
#define MIDI_JACK_DEV_OUT 0x02
#define MIDI_JACK_USB_IN 0x03
#define MIDI_JACK_DEV_IN 0x04

/* USB MIDI device Configuration Descriptor */
__ALIGN_BEGIN uint8_t USBD_MIDI_CfgDesc[USB_MIDI_CONFIG_DESC_SIZ] __ALIGN_END =
        {
                //Standard Descriptor
                0x09,        // bLength
                0x02,        // bDescriptorType (Configuration)
                0x65, 0x00,  // wTotalLength 101
                0x02,        // bNumInterfaces 2
                0x01,        // bConfigurationValue
                0x00,        // iConfiguration (String Index)
                0xC0,        // bmAttributes (Self Powered)
                0x05,        // bMaxPower 10mA

                /** So, this block and the next block doesn't actually do anything. However, it is required per USB MIDI spec. */
                //Audio interface
                0x09,        // bLength
                0x04,        // bDescriptorType (Interface)
                0x00,        // bInterfaceNumber 0
                0x00,        // bAlternateSetting
                0x00,        // bNumEndpoints 0
                0x01,        // bInterfaceClass (Audio)
                0x01,        // bInterfaceSubClass (Audio control)
                0x00,        // bInterfaceProtocol
                0x00,        // iInterface (String Index)

                //Class Specific Audio Interface
                0x09,        // bLength
                0x24,        // bDescriptorType (See Next Line)
                0x01,        // bDescriptorSubtype (CS_INTERFACE -> HEADER)
                0x00, 0x01,  // bcdADC 1.00
                0x09, 0x00,  // wTotalLength 9
                0x01,        // binCollection 0x01
                0x01,        // baInterfaceNr 1

                /** And from here on, things make sense. A bit */
                //MIDI streaming interface
                0x09,        // bLength
                0x04,        // bDescriptorType (Interface)
                0x01,        // bInterfaceNumber 1
                0x00,        // bAlternateSetting
                0x02,        // bNumEndpoints 2
                0x01,        // bInterfaceClass (Audio)
                0x03,        // bInterfaceSubClass (MIDI Streaming)
                0x00,        // bInterfaceProtocol
                USBD_IDX_INTERFACE_STR, // iInterface (String Index)

                //MIDI Interface Header Descriptor
                0x07,        // bLength
                0x24,        // bDescriptorType (See Next Line)
                0x01,        // bDescriptorSubtype (CS_INTERFACE -> MS_HEADER)
                0x00, 0x01,  // bcdMSC 1.00
                0x41, 0x00,  // wTotalLength 41

                /** So, please note where MIDI jacks reference each other. This is way more strict than it seems,
                 * or Windows drivers will politely refuse to load without explanation.
                 */
                //MIDI In Jack 1
                0x06,        // bLength
                0x24,        // bDescriptorType (See Next Line)
                0x02,        // bDescriptorSubtype (CS_INTERFACE -> MIDI_IN_JACK)
                0x01,        // bJackType (EMBEDDED)
                MIDI_JACK_USB_OUT, // bJackID
                0x00,        // iJack

                //MIDI In Jack 2
                0x06,        // bLength
                0x24,        // bDescriptorType (See Next Line)
                0x02,        // bDescriptorSubtype (CS_INTERFACE -> MIDI_IN_JACK)
                0x02,        // bJackType (EXTERNAL)
                MIDI_JACK_DEV_OUT, // bJackID
                0x00,        // iJack

                //MIDI Out Jack 1
                0x09,        // bLength
                0x24,        // bDescriptorType (See Next Line)
                0x03,        // bDescriptorSubtype (CS_INTERFACE -> MIDI_OUT_JACK)
                0x01,        // bJackType (EMBEDDED)
                MIDI_JACK_USB_IN, // bJackID
                0x01,        // bNrInputPins
                MIDI_JACK_DEV_OUT, // baSourceID(1)
                0x00,        // baSourcePin(1)
                0x00,        // iJack

                //MIDI Out Jack 2
                0x09,        // bLength
                0x24,        // bDescriptorType (See Next Line)
                0x03,        // bDescriptorSubtype (CS_INTERFACE -> MIDI_OUT_JACK)
                0x02,        // bJackType (EXTERNAL)
                MIDI_JACK_DEV_IN, // bJackID
                0x01,        // bNrInputPins
                MIDI_JACK_USB_OUT, // baSourceID(1)
                0x00,        // baSourcePin(1)
                0x00,        // iJack

                //OUT Endpoint descriptor
                0x09,        // bLength
                0x05,        // bDescriptorType (See Next Line)
                MIDI_OUT_EP, // bEndpointAddress (OUT/H2D)
                0x02,        // bmAttributes (Bulk)
                0x40, 0x00,  // wMaxPacketSize 64
                0x00,        // bInterval 0 (Ignored for bulk)
                0x00,        // bRefresh
                0x00,        // bSyncAddress

                //Class-specific MS Bulk OUT Descriptor
                0x05,        // bLength
                0x25,        // bDescriptorType (See Next Line)
                0x01,        // bDescriptorSubtype (CS_ENDPOINT -> MS_GENERAL)
                0x01,        // bNumEmbMIDIJack (num of MIDI **IN** Jacks)
                MIDI_JACK_USB_OUT, // BaAssocJackID(1) 1

                //IN Endpoint descriptor
                0x09,        // bLength
                0x05,        // bDescriptorType (See Next Line)
                MIDI_IN_EP,  // bEndpointAddress (OUT/H2D)
                0x02,        // bmAttributes (Bulk)
                0x40, 0x00,  // wMaxPacketSize 64
                0x00,        // bInterval  0 (Ignored for bulk)
                0x00,        // bRefresh
                0x00,        // bSyncAddress

                //Class-specific MS Bulk IN Descriptor
                0x05,        // bLength
                0x25,        // bDescriptorType (See Next Line)
                0x01,        // bDescriptorSubtype (CS_ENDPOINT -> MS_GENERAL)
                0x01,        // bNumEmbMIDIJack(1) (num of MIDI **OUT** Jacks)
                MIDI_JACK_USB_IN, // BaAssocJackID(1) 1
        };

static uint8_t USBD_MIDI_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx){
    pInstance = pdev;
    USBD_LL_OpenEP(pdev,MIDI_IN_EP,USBD_EP_TYPE_BULK,MIDI_DATA_IN_PACKET_SIZE);
    USBD_LL_OpenEP(pdev,MIDI_OUT_EP,USBD_EP_TYPE_BULK,MIDI_DATA_OUT_PACKET_SIZE);
    USBD_LL_PrepareReceive(pdev,MIDI_OUT_EP,(uint8_t*)(USB_Rx_Buffer),MIDI_DATA_OUT_PACKET_SIZE);
    return 0;
}

static uint8_t USBD_MIDI_DeInit (USBD_HandleTypeDef *pdev, uint8_t cfgidx){
    pInstance = NULL;
    USBD_LL_CloseEP(pdev,MIDI_IN_EP);
    USBD_LL_CloseEP(pdev,MIDI_OUT_EP);
    return 0;
}

static uint8_t USBD_MIDI_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum){

    if (USB_Tx_State == 1){
        USB_Tx_State = 0;
    }
    return USBD_OK;
}

static uint8_t  USBD_MIDI_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    USBD_MIDI_ItfTypeDef *pMidi = (USBD_MIDI_ItfTypeDef *)(pdev->pUserData);
    uint16_t USB_Rx_Cnt = ((PCD_HandleTypeDef*)pdev->pData)->OUT_ep[epnum].xfer_count;

    pMidi->pIf_MidiRx((uint8_t *)&USB_Rx_Buffer, USB_Rx_Cnt);

    USBD_LL_PrepareReceive(pdev,MIDI_OUT_EP,(uint8_t*)(USB_Rx_Buffer),MIDI_DATA_OUT_PACKET_SIZE);
    return USBD_OK;
}

void USBD_MIDI_SendPacket (){
    uint16_t USB_Tx_ptr;
    uint16_t USB_Tx_length;

    if(USB_Tx_State != 1){
        if (APP_Rx_ptr_out == APP_RX_DATA_SIZE){
            APP_Rx_ptr_out = 0;
        }

        if(APP_Rx_ptr_out == APP_Rx_ptr_in){
            USB_Tx_State = 0;
            return;
        }

        if(APP_Rx_ptr_out > APP_Rx_ptr_in){
            APP_Rx_length = APP_RX_DATA_SIZE - APP_Rx_ptr_out;
        }else{
            APP_Rx_length = APP_Rx_ptr_in - APP_Rx_ptr_out;
        }

        if (APP_Rx_length > MIDI_DATA_IN_PACKET_SIZE){
            USB_Tx_ptr = APP_Rx_ptr_out;
            USB_Tx_length = MIDI_DATA_IN_PACKET_SIZE;
            APP_Rx_ptr_out += MIDI_DATA_IN_PACKET_SIZE;
            APP_Rx_length -= MIDI_DATA_IN_PACKET_SIZE;
        }else{
            USB_Tx_ptr = APP_Rx_ptr_out;
            USB_Tx_length = APP_Rx_length;
            APP_Rx_ptr_out += APP_Rx_length;
            APP_Rx_length = 0;
        }
        USB_Tx_State = 1;
        USBD_LL_Transmit (pInstance, MIDI_IN_EP,(uint8_t*)&APP_Rx_Buffer[USB_Tx_ptr],USB_Tx_length);
    }
}

static uint8_t *USBD_MIDI_GetCfgDesc (uint16_t *length){
    *length = sizeof (USBD_MIDI_CfgDesc);
    return USBD_MIDI_CfgDesc;
}

uint8_t USBD_MIDI_RegisterInterface(USBD_HandleTypeDef *pdev, USBD_MIDI_ItfTypeDef *fops)
{
    uint8_t ret = USBD_FAIL;

    if(fops != NULL){
        pdev->pUserData = fops;
        ret = USBD_OK;
    }

    return ret;
}
