/**
  ******************************************************************************
  * @file    usbio.c
  * $Author: wdluo $
  * $Revision: 67 $
  * $Date:: 2012-08-15 19:00:29 +0800 #$
  * @brief   USB上层函数.
  ******************************************************************************
  * @attention
  *
  *<h3><center>&copy; Copyright 2009-2012, ViewTool</center>
  *<center><a href="http:\\www.viewtool.com">http://www.viewtool.com</a></center>
  *<center>All Rights Reserved</center></h3>
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "usb_io.h"
#include "string.h"


#include  "usbd_usr.h"
#include  "usbd_desc.h"




uint8_t USB_RecvData[512] = {0x00};                         //USB接收数据
uint16_t USB_RecvDataLen = 0;                               //接收数据长度

uint8_t *USB_SendData = 0;                                  //发送数据
uint16_t USB_SendDataLen = 0;                               //发送数据长度


USB_OTG_CORE_HANDLE USB_OTG_dev;

/* Private functions ---------------------------------------------------------*/

#define USB_DISCONNECT                      GPIOC
#define USB_DISCONNECT_PIN                  GPIO_Pin_3
#define RCC_APB2Periph_GPIO_DISCONNECT      RCC_APB2Periph_GPIOC






void USB_IO_DeInit()
{
    USBD_DeInit(&USB_OTG_dev);
    GPIO_ResetBits(USB_DISCONNECT,USB_DISCONNECT_PIN);
	delay_us(500000);                                       //延时一下
	
    GPIO_DeInit(USB_DISCONNECT);
}



void USB_IO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    //相关时钟初始化
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIO_DISCONNECT, ENABLE);
    //USB上拉引脚初始化
    GPIO_InitStructure.GPIO_Pin = USB_DISCONNECT_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(USB_DISCONNECT, &GPIO_InitStructure);

    //USB设备初始化
    USBD_Init(&USB_OTG_dev,
              USB_OTG_FS_CORE_ID,
              &USR_desc,
              &USBD_CDC_cb,
              &USR_cb);
    GPIO_SetBits(USB_DISCONNECT, USB_DISCONNECT_PIN);
}

static uint8_t USB_IntState = 0;
uint8_t USB_IO_GetState(void)
{
    return USB_IntState;//通过判断有没有进入过USB中断来判断USB的插入
}

extern uint32_t USBD_OTG_ISR_Handler (USB_OTG_CORE_HANDLE *pdev);
void USB_IO_Int()
{
    USB_IntState = 1;
    USBD_OTG_ISR_Handler(&USB_OTG_dev);
}
/* Private functions ---------------------------------------------------------*/

static uint16_t usbSendCount = 0;

void USB_IO_SendData(uint8_t *dat, uint16_t len)
{
    USB_SendData = dat;
    USB_SendDataLen = len;
    usbSendCount = 0;

    USB_IO_SendDataC();
}

void USB_IO_SendDataC()
{
    if(USB_SendDataLen > USB_MAXDATASIZE)										//??????????
    {
        USB_SendDataLen = USB_SendDataLen - USB_MAXDATASIZE;		//????
        USBD_CDC_SendReport(&USB_OTG_dev, USB_SendData + (usbSendCount * USB_MAXDATASIZE), USB_MAXDATASIZE);
        usbSendCount++;
    }
    else
    {
        if(USB_SendDataLen > 0)
        {
            USBD_CDC_SendReport(&USB_OTG_dev, USB_SendData + (usbSendCount * USB_MAXDATASIZE), USB_SendDataLen);
            USB_SendDataLen = 0;
            usbSendCount = 0;
        }
    }

}

static uint16_t countAll, countNow, delayCount = 0;//接收数据所用变量,按协议计算出总共要接收的数据长度,当前已经接收到的数据长度,等待下一包的等待时间
void USB_IO_RecvDataC(uint16_t len)
{
    uint16_t countOne;

    delayCount = 0x64;
    countOne = len;
    memcpy(USB_RecvData + countNow, USB_Rx_Buffer, countOne);
    if(!countNow)
    {
        //按协议计算出总共要接收的数据长度
    }
    countNow = countOne + countNow;
    if(countNow >= countAll)
    {
        USB_RecvDataLen = countAll;
        countNow = 0;
        delayCount = 0;
    }
}

void USB_TIMxInt()
{
    if(delayCount > 0)
    {
        delayCount--;
        if(delayCount == 0 && countNow > 0)
        {
            USB_RecvDataLen = countNow;
            countNow = 0;
            countAll = 0;
        }
    }
}



/*********************************END OF FILE**********************************/

