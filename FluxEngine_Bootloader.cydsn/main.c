/*****************************************************************************
* File Name: main.c
*
* Version: 3.0
*
* Description:
*   The main C file for the USB HID Bootloader for PSoC 5LP associated with 
*   CE95391.
*
*	Checks for the bootloader run type and launches the bootloader accordingly.
*
******************************************************************************
* Copyright (C) 2016, Cypress Semiconductor Corporation.
******************************************************************************
* This software is owned by Cypress Semiconductor Corporation (Cypress) and is
* protected by and subject to worldwide patent protection (United States and
* foreign), United States copyright laws and international treaty provisions.
* Cypress hereby grants to licensee a personal, non-exclusive, non-transferable
* license to copy, use, modify, create derivative works of, and compile the
* Cypress Source Code and derivative works for the sole purpose of creating
* custom software in support of licensee product to be used only in conjunction
* with a Cypress integrated circuit as specified in the applicable agreement.
* Any reproduction, modification, translation, compilation, or representation of
* this software except as specified above is prohibited without the express
* written permission of Cypress.
*
* Disclaimer: CYPRESS MAKES NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, WITH
* REGARD TO THIS MATERIAL, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* Cypress reserves the right to make changes without further notice to the
* materials described herein. Cypress does not assume any liability arising out
* of the application or use of any product or circuit described herein. Cypress
* does not authorize its products for use as critical components in life-support
* systems where a malfunction or failure may reasonably be expected to result in
* significant injury to the user. The inclusion of Cypress' product in a life-
* support systems application implies that the manufacturer assumes all risk of
* such use and in doing so indemnifies Cypress against all charges. Use may be
* limited by and subject to the applicable Cypress software license agreement.
*****************************************************************************/
#include <project.h>


int main()
{
    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    
	/* Check to see if we should only run bootloader, or eventually move to the bootloadable code.
	   The bootloader uses the macro BL_GET_RUN_TYPE to decide how to proceed.  
	   If the switch is held down the user wants the bootloader to wait for a start command.
       Refer to the Bootloader Componet datasheet for more information.
    */
    PWM_1_Start();
	if((Wait_Forever_Read() == 0/*Pressed*/) || (Bootloader_GET_RUN_TYPE == Bootloader_START_BTLDR))
	{	
		Bootloader_SET_RUN_TYPE(Bootloader_START_BTLDR);
	}

	/* Start Bootloader - we never return from this function */
	Bootloader_Start();
    /* BL_Start() will not return and it stays in the BL_Statrt() */

    /* CyGlobalIntEnable; */ /* Uncomment this line to enable global interrupts. */
	
    for(;;)
    {
        /* Place your application code here. */
    }
}

/* [] END OF FILE */
