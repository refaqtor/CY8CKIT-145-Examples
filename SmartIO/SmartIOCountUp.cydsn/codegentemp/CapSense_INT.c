/***************************************************************************//**
* \file CapSense_INT.c
* \version 3.0
*
* \brief
*   This file contains the source code for implementation of the CapSense component
*   Interrupt Service Routine (ISR).
*
* \see CapSense P4 v3.0 Datasheet
*
*//*****************************************************************************
* Copyright (2016), Cypress Semiconductor Corporation.
********************************************************************************
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
*******************************************************************************/
#include "cytypes.h"
#include "cyfitter.h"
#include "CapSense_Sensing.h"
#include "cyapicallbacks.h"

/*******************************************************************************
* Static Function Prototypes
*******************************************************************************/
/**
* \if SECTION_CAPSENSE_INTERNAL
* \addtogroup group_capsense_internal
* \{
*/

#if (((CapSense_ENABLE == CapSense_CSD_EN) || (CapSense_ENABLE == CapSense_CSD_CSX_EN)) && \
     (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN))
    static void CapSense_SsNextFrequencyScan(void);
#endif /* (((CapSense_ENABLE == CapSense_CSD_EN) || (CapSense_ENABLE == CapSense_CSD_CSX_EN)) && \
            (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN)) */

#if (((CapSense_ENABLE == CapSense_CSD_EN) || (CapSense_ENABLE == CapSense_CSD_CSX_EN)) && \
     (CapSense_ENABLE == CapSense_CSDV2))
    static void CapSense_SsPostScan(void);
    CY_INLINE static void CapSense_SsInitNextScan(void);
    #if (CapSense_ENABLE == CapSense_CSD_GANGED_SNS_EN)
        CY_INLINE static void CapSense_SsInitNextGangedScan(void);
    #endif /* (CapSense_ENABLE == CapSense_CSD_GANGED_SNS_EN) */  
#endif /* (((CapSense_ENABLE == CapSense_CSD_EN) || (CapSense_ENABLE == CapSense_CSD_CSX_EN)) && \
           (CapSense_ENABLE == CapSense_CSDV2)) */ 
            
/** \}
* \endif */


/**
* \if SECTION_CAPSENSE_INTERRUPT
* \addtogroup group_capsense_interrupt
* \{
*/


#if ((CapSense_ENABLE == CapSense_CSD_EN) || (CapSense_ENABLE == CapSense_CSD_CSX_EN))

#if (CapSense_ENABLE == CapSense_CSDV2)
    /* CSDv2 part */

    /*******************************************************************************
    * Function Name: CapSense_CSDPostSingleScan
    ****************************************************************************//**
    *
    * \brief
    *   This is an internal ISR function for single-sensor scanning implementation.
    *
    * \details
    *   This ISR handler is triggered when the user calls the
    *   CapSense_CSDScanExt() function.
    *
    *   The following tasks are performed for CSDv1 HW IP block:
    *   - Disable the CSD interrupt
    *   - Read the Counter register and update the data structure with raw data
    *   - Connect the Vref buffer to the AMUX bus
    *   - Update the Scan Counter
    *   - Reset the BUSY flag
    *   - Enable the CSD interrupt.
    *
    *   The following tasks are performed for CSDv2 HW IP block:
    *   - Check if the raw data is not noisy
    *   - Read the Counter register and update the data structure with raw data
    *   - Configure and start the scan for the next frequency if the
    *      multi-frequency is enabled
    *   - Update the Scan Counter
    *   - Reset the BUSY flag
    *   - Enable the CSD interrupt.
    *
    *   The ISR handler changes IMO and initializes scanning for the next frequency
    *   channels when multi-frequency scanning is enabled.
    *
    *   This function has two Macro Callbacks that allow to call user code from macros
    *   specified in a component's generated code. Refer to Macro Callbacks section
    *   of PSoC Creator� User Guide for details.
    *
    *******************************************************************************/
    CY_ISR(CapSense_CSDPostSingleScan)
    {
        #ifdef CapSense_ENTRY_CALLBACK
            CapSense_EntryCallback();
        #endif /* CapSense_ENTRY_CALLBACK */

        /* Clear pending interrupt */
        CY_SET_REG32(CapSense_INTR_PTR, CapSense_INTR_SET_MASK);

    #if (CapSense_ENABLE == CapSense_CSD_NOISE_METRIC_EN)
        if ((CapSense_CSD_NOISE_METRIC_TH < ((CY_GET_REG32(CapSense_RESULT_VAL1_PTR) &
                                                    CapSense_RESULT_VAL1_BAD_CONVS_MASK) >>
                                                    CapSense_RESULT_VAL1_BAD_CONVS_SHIFT)) &&
                                                    (0u < CapSense_badConversionsNum))
        {
            /* Decrement bad conversions number */
            CapSense_badConversionsNum--;

            /* Start the re-scan */
            CY_SET_REG32(CapSense_SEQ_START_PTR, CapSense_SEQ_START_AZ0_SKIP_MASK |
                                                         CapSense_SEQ_START_AZ1_SKIP_MASK |
                                                         CapSense_SEQ_START_START_MASK);
        }
        else
        {
    #endif /* (CapSense_ENABLE == CapSense_CSD_NOISE_METRIC_EN) */

            CapSense_SsPostScan();

            #if (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN)
                if (CapSense_FREQ_CHANNEL_2 > CapSense_scanFreqIndex)
                {
                    /* Scan the next channel */
                    CapSense_SsNextFrequencyScan();
                }
                else
                {
                    /* All channels are scanned. Set IMO to zero channel */
                    CapSense_scanFreqIndex = CapSense_FREQ_CHANNEL_0;
                    CapSense_ChangeImoFreq(CapSense_FREQ_CHANNEL_0);

                    #if (CapSense_ENABLE == CapSense_BLOCK_OFF_AFTER_SCAN_EN)
                        /*  Disable CSDv2 block */
                        CY_SET_REG32(CapSense_CONFIG_PTR, CapSense_configCsd);
                    #endif /* (CapSense_ENABLE == CapSense_BLOCK_OFF_AFTER_SCAN_EN) */    

                    /* Update Scan Counter */
                    CapSense_dsRam.scanCounter++;

                    /* Sensor is totally scanned. Reset BUSY flag */
                    CapSense_dsRam.status &= ~(CapSense_SW_STS_BUSY | CapSense_WDGT_SW_STS_BUSY);
                }   
            #else
                {
                    #if (CapSense_ENABLE == CapSense_BLOCK_OFF_AFTER_SCAN_EN)
                        /*  Disable CSDv2 block */
                        CY_SET_REG32(CapSense_CONFIG_PTR, CapSense_configCsd);
                    #endif /* (CapSense_ENABLE == CapSense_BLOCK_OFF_AFTER_SCAN_EN) */    

                    /* Update Scan Counter */
                    CapSense_dsRam.scanCounter++;

                    /* Sensor is totally scanned. Reset BUSY flag */
                    CapSense_dsRam.status &= ~(CapSense_SW_STS_BUSY | CapSense_WDGT_SW_STS_BUSY);
                }
            #endif /* (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN) */
    #if (CapSense_ENABLE == CapSense_CSD_NOISE_METRIC_EN)
        }
    #endif /* (CapSense_ENABLE == CapSense_CSD_NOISE_METRIC_EN) */

        #ifdef CapSense_EXIT_CALLBACK
            CapSense_ExitCallback();
        #endif /* CapSense_EXIT_CALLBACK */
    }


    /*******************************************************************************
    * Function Name: CapSense_CSDPostMultiScan
    ****************************************************************************//**
    *
    * \brief
    *   This is an internal ISR function for multiple sensor-scanning implementation.
    *
    * \details
    *   This ISR handler is triggered when the user calls the
    *   CapSense_Scan() or CapSense_ScanAllWidgets() APIs.
    *   The following tasks are performed:
    *   - Disable the CSD interrupt
    *   - Read the Counter register and update the data structure with raw data
    *   - Connect the Vref buffer to the AMUX bus
    *   - Disable the CSD block (after the widget has been scanned)
    *   - Update Scan Counter
    *   - Reset the BUSY flag
    *   - Enable the CSD interrupt.
    *
    *   The ISR handler initializes scanning for the previous sensor when the widget has
    *   more than one sensor.
    *   The ISR handler initializes scanning for the next widget when the
    *   CapSense_ScanAllWidgets() APIs are called and the project has more than
    *   one widget.
    *   The ISR handler changes the IMO and initializes scanning for the next frequency
    *   channels when multi-frequency scanning is enabled.
    *
    *   This function has two Macro Callbacks that allow to call user code from macros
    *   specified in a component's generated code. Refer to Macro Callbacks section
    *   of PSoC Creator� User Guide for details.
    *
    *******************************************************************************/
    CY_ISR(CapSense_CSDPostMultiScan)
    {
        /*  Declare and initialise ptr to sensor IO structure to appropriate address        */
        CapSense_FLASH_IO_STRUCT const *curSnsIOPtr = (CapSense_FLASH_IO_STRUCT const *)
                                                          CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2SnsFlash
                                                          + CapSense_sensorIndex;

        #ifdef CapSense_ENTRY_CALLBACK
            CapSense_EntryCallback();
        #endif /* CapSense_ENTRY_CALLBACK */

        /* Clear pending interrupt */
        CY_SET_REG32(CapSense_INTR_PTR, CapSense_INTR_SET_MASK);

        #if (CapSense_ENABLE == CapSense_CSD_NOISE_METRIC_EN)
            if ((CapSense_CSD_NOISE_METRIC_TH < ((CY_GET_REG32(CapSense_RESULT_VAL1_PTR) &
                                                      CapSense_RESULT_VAL1_BAD_CONVS_MASK) >>
                                                      CapSense_RESULT_VAL1_BAD_CONVS_SHIFT)) &&
                                                      (0u < CapSense_badConversionsNum))
            {
                /* Decrement bad conversions number */
                CapSense_badConversionsNum--;

                /* Start the re-scan */
                CY_SET_REG32(CapSense_SEQ_START_PTR, CapSense_SEQ_START_AZ0_SKIP_MASK |
                                                             CapSense_SEQ_START_AZ1_SKIP_MASK |
                                                             CapSense_SEQ_START_START_MASK);
            }
            else
            {
        #endif /* (CapSense_ENABLE == CapSense_CSD_NOISE_METRIC_EN) */
            
            CapSense_SsPostScan();

            #if (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN)
                /* Disable sensor when all frequency channels are scanned */
                if (CapSense_FREQ_CHANNEL_2 == CapSense_scanFreqIndex)
                {
                    /* Disable sensor */
                    CapSense_CSDDisconnectSns(curSnsIOPtr);
                }
            #else
                /* Disable sensor */
                CapSense_CSDDisconnectSns(curSnsIOPtr);  
            #endif /* (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN) */

            #if (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN)
                if (CapSense_FREQ_CHANNEL_2 > CapSense_scanFreqIndex)
                {
                     /* Scan the next channel */
                    CapSense_SsNextFrequencyScan();
                }
                else
                {
                     /* All channels are scanned. Set IMO to zero channel */
                    CapSense_scanFreqIndex = CapSense_FREQ_CHANNEL_0;
                    CapSense_ChangeImoFreq(CapSense_FREQ_CHANNEL_0);

                     /* Scan the next sensor */
                    CapSense_SsInitNextScan();   
                }
            #else 
                /* Scan the next sensor */
                CapSense_SsInitNextScan();
            #endif /* (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN) */
        #if (CapSense_ENABLE == CapSense_CSD_NOISE_METRIC_EN)
            }
        #endif /* (CapSense_ENABLE == CapSense_CSD_NOISE_METRIC_EN) */

        #ifdef CapSense_EXIT_CALLBACK
            CapSense_ExitCallback();
        #endif /* CapSense_EXIT_CALLBACK */
    }


    #if (CapSense_ENABLE == CapSense_CSD_GANGED_SNS_EN)
    /*******************************************************************************
    * Function Name: CapSense_CSDPostMultiScanGanged
    ****************************************************************************//**
    *
    * \brief
    *   This is an internal ISR function for multiple sensor scanning implementation
    *   for ganged sensors.
    *
    * \details
    *   This ISR handler is triggered when the user calls the
    *   CapSense_Scan() API for a ganged sensor or the
    *   CapSense_ScanAllWidgets() API in the project with ganged sensors.
    *
    *   The following tasks are performed:
    *   - Disable the CSD interrupt
    *   - Read the Counter register and update the data structure with raw data
    *   - Connect the Vref buffer to the AMUX bus
    *   - Disable the CSD block (after the widget has been scanned)
    *   - Update Scan Counter
    *   - Reset the BUSY flag
    *   - Enable the CSD interrupt.
    *
    *   The ISR handler initializes scanning for the previous sensor when the widget has
    *   more than one sensor.
    *   The ISR handler initializes scanning for the next widget when the
    *   CapSense_ScanAllWidgets() APIs are called and the project has more than
    *   one widget.
    *   The ISR handler changes the IMO and initializes scanning for the next frequency
    *   channels when multi-frequency scanning is enabled.
    *
    *   This function has two Macro Callbacks that allow to call user code from macros
    *   specified in a component's generated code. Refer to Macro Callbacks section
    *   of PSoC Creator� User Guide for details.
    *
    *******************************************************************************/
    CY_ISR(CapSense_CSDPostMultiScanGanged)
    {
        uint32 tempVal;

        /*  Declare and initialise ptr to sensor IO structure to appropriate address       */
        CapSense_FLASH_IO_STRUCT const *curSnsIOPtr =
            (CapSense_FLASH_IO_STRUCT const *)CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2SnsFlash
            + CapSense_sensorIndex;

        #ifdef CapSense_ENTRY_CALLBACK
            CapSense_EntryCallback();
        #endif /* CapSense_ENTRY_CALLBACK */

        /* Clear pending interrupt */
        CY_SET_REG32(CapSense_INTR_PTR, CapSense_INTR_SET_MASK);

        #if (CapSense_ENABLE == CapSense_CSD_NOISE_METRIC_EN)
            if ((CapSense_CSD_NOISE_METRIC_TH < ((CY_GET_REG32(CapSense_RESULT_VAL1_PTR) &
                                                      CapSense_RESULT_VAL1_BAD_CONVS_MASK) >>
                                                      CapSense_RESULT_VAL1_BAD_CONVS_SHIFT)) &&
                                                      (0u < CapSense_badConversionsNum))
            {
                /* Decrement bad conversions number */
                CapSense_badConversionsNum--;

                /* Start the re-scan */
                CY_SET_REG32(CapSense_SEQ_START_PTR, CapSense_SEQ_START_AZ0_SKIP_MASK |
                                                             CapSense_SEQ_START_AZ1_SKIP_MASK |
                                                             CapSense_SEQ_START_START_MASK);
            }
            else
            {
        #endif /* (CapSense_ENABLE == CapSense_CSD_NOISE_METRIC_EN) */

            CapSense_SsPostScan();

            #if (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN)
                if (CapSense_FREQ_CHANNEL_2 == CapSense_scanFreqIndex)
                {
                    /* Check ganged sns flag  */
                    if ((CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].staticConfig &
                        CapSense_GANGED_SNS_MASK) == CapSense_GANGED_SNS_MASK)
                    {
                        /* Get sns pointer */
                        CapSense_curFlashSnsPtr = (CapSense_FLASH_SNS_STRUCT const *)
                                                           CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2SnsFlash +
                                                           CapSense_sensorIndex;

                        /* Get number of ganged pins */
                        tempVal = CapSense_curFlashSnsPtr->numPins;

                        /* Get IO pointer  */
                        curSnsIOPtr = &CapSense_ioList[CapSense_curFlashSnsPtr->firstPinId];

                        /* Disconnect all ganged sensors */
                        do
                        {
                            CapSense_CSDDisconnectSns(curSnsIOPtr);
                            curSnsIOPtr++;
                            tempVal--;
                        } while (0u != tempVal);
                    }
                    else
                    {
                        /* Disconnect ganged sensor */
                        CapSense_CSDDisconnectSns(curSnsIOPtr);
                    }
                }        
            #else
                /* Check ganged sns flag  */
                if ((CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].staticConfig &
                    CapSense_GANGED_SNS_MASK) == CapSense_GANGED_SNS_MASK)
                {
                    /* Get sns pointer */
                    CapSense_curFlashSnsPtr = (CapSense_FLASH_SNS_STRUCT const *)
                                                       CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2SnsFlash +
                                                       CapSense_sensorIndex;

                    /* Get number of ganged pins */
                    tempVal = CapSense_curFlashSnsPtr->numPins;

                    /* Get IO pointer  */
                    curSnsIOPtr = &CapSense_ioList[CapSense_curFlashSnsPtr->firstPinId];

                    /* Disconnect all ganged sensors */
                    do
                    {
                        CapSense_CSDDisconnectSns(curSnsIOPtr);
                        curSnsIOPtr++;
                        tempVal--;
                    } while (0u != tempVal);
                }
                else
                {
                    /* Disconnect ganged sensor */
                    CapSense_CSDDisconnectSns(curSnsIOPtr);
                }
            #endif /* (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN) */

            #if (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN)
                if (CapSense_FREQ_CHANNEL_2 > CapSense_scanFreqIndex)
                {
                     /* Scan the next channel */
                    CapSense_SsNextFrequencyScan();
                }
                else
                {
                    /* All channels are scanned. Set IMO to zero channel */
                    CapSense_scanFreqIndex = CapSense_FREQ_CHANNEL_0;
                    CapSense_ChangeImoFreq(CapSense_FREQ_CHANNEL_0);

                 /* Scan the next sensor */
                    CapSense_SsInitNextGangedScan();
                }
            #else    
                 /* Scan the next sensor */
                CapSense_SsInitNextGangedScan();  
            #endif /* (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN) */
        #if (CapSense_ENABLE == CapSense_CSD_NOISE_METRIC_EN)
            }
        #endif /* (CapSense_ENABLE == CapSense_CSD_NOISE_METRIC_EN) */

        #ifdef CapSense_EXIT_CALLBACK
            CapSense_ExitCallback();
        #endif /* CapSense_EXIT_CALLBACK */
    }
    #endif /* (CapSense_ENABLE == CapSense_CSD_GANGED_SNS_EN) */

#else
    /* CSDv1 part */

    /*******************************************************************************
    * Function Name: CapSense_CSDPostSingleScan
    ****************************************************************************//**
    *
    * \brief
    *   This is an internal ISR function for single-sensor scanning implementation.
    *
    * \details
    *   This ISR handler is triggered when the user calls the
    *   CapSense_CSDScanExt() function.
    *
    *   The following tasks are performed for CSDv1 HW IP block:
    *   - Disable the CSD interrupt
    *   - Read the Counter register and update the data structure with raw data
    *   - Connect the Vref buffer to the AMUX bus
    *   - Update the Scan Counter
    *   - Reset the BUSY flag
    *   - Enable the CSD interrupt.
    *
    *   The following tasks are performed for CSDv2 HW IP block:
    *   - Check if the raw data is not noisy
    *   - Read the Counter register and update the data structure with raw data
    *   - Configure and start the scan for the next frequency if the
    *         multi-frequency is enabled
    *   - Update the Scan Counter
    *   - Reset the BUSY flag
    *   - Enable the CSD interrupt.
    *
    *   The ISR handler changes IMO and initializes scanning for the next frequency
    *   channels when multi-frequency scanning is enabled.
    *
    *   This function has two Macro Callbacks that allow to call user code from macros
    *   specified in a component's generated code. Refer to Macro Callbacks section
    *   of PSoC Creator� User Guide for details.
    *
    *******************************************************************************/
    CY_ISR(CapSense_CSDPostSingleScan)
    {
        #ifdef CapSense_ENTRY_CALLBACK
            CapSense_EntryCallback();
        #endif /* CapSense_ENTRY_CALLBACK */

        /* Clear pending interrupt */
        CY_SET_REG32(CapSense_INTR_PTR, CapSense_INTR_SET_MASK);

        /* Read SlotResult from Raw Counter */
        CapSense_curRamSnsPtr->raw[CapSense_scanFreqIndex] = (uint16)CY_GET_REG32(CapSense_COUNTER_PTR);

        #if (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN)
        if (CapSense_FREQ_CHANNEL_2 > CapSense_scanFreqIndex)
        {
            /*  Connect Vref Buffer to AMUX bus. CSDv1 block is enabled */
            CY_SET_REG32(CapSense_CONFIG_PTR, CapSense_configCsd | CapSense_CMOD_PRECHARGE_CONFIG_CSD_EN);

            CapSense_SsNextFrequencyScan();
        }
        else
        #endif /* (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN) */
        {
            #if(CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN)
                CapSense_scanFreqIndex = CapSense_FREQ_CHANNEL_0;
                CapSense_ChangeImoFreq(CapSense_FREQ_CHANNEL_0);
            #endif  /* (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN) */

            #if (CapSense_ENABLE == CapSense_BLOCK_OFF_AFTER_SCAN_EN)
                /*  Disable CSDv1 block. Connect Vref Buffer to AMUX bus */
                CY_SET_REG32(CapSense_CONFIG_PTR, CapSense_configCsd | CapSense_CMOD_PRECHARGE_CONFIG);
            #else
                /*  Connect Vref Buffer to AMUX bus. CSDv1 block is enabled */
                CY_SET_REG32(CapSense_CONFIG_PTR, CapSense_configCsd | CapSense_CMOD_PRECHARGE_CONFIG_CSD_EN);
            #endif /* (CapSense_ENABLE == CapSense_BLOCK_OFF_AFTER_SCAN_EN) */

            /* Update Scan Counter */
            CapSense_dsRam.scanCounter++;

            /* Sensor is totally scanned. Reset BUSY flag */
            CapSense_dsRam.status &= ~(CapSense_SW_STS_BUSY | CapSense_WDGT_SW_STS_BUSY);
        }

        #ifdef CapSense_EXIT_CALLBACK
            CapSense_ExitCallback();
        #endif /* CapSense_EXIT_CALLBACK */
    }


    /*******************************************************************************
    * Function Name: CapSense_CSDPostMultiScan
    ****************************************************************************//**
    *
    * \brief
    *   This is an internal ISR function for multiple sensor-scanning implementation.
    *
    * \details
    *   This ISR handler is triggered when the user calls the
    *   CapSense_Scan() or CapSense_ScanAllWidgets() APIs.
    *   The following tasks are performed:
    *   - Disable the CSD interrupt
    *   - Read the Counter register and update the data structure with raw data
    *   - Connect the Vref buffer to the AMUX bus
    *   - Disable the CSD block (after the widget has been scanned)
    *   - Update Scan Counter
    *   - Reset the BUSY flag
    *   - Enable the CSD interrupt.
    *
    *   The ISR handler initializes scanning for the previous sensor when the widget has
    *   more than one sensor.
    *   The ISR handler initializes scanning for the next widget when the
    *   CapSense_ScanAllWidgets() APIs are called and the project has more than
    *   one widget.
    *   The ISR handler changes the IMO and initializes scanning for the next frequency
    *   channels when multi-frequency scanning is enabled.
    *
    *   This function has two Macro Callbacks that allow to call user code from macros
    *   specified in a component's generated code. Refer to Macro Callbacks section
    *   of PSoC Creator� User Guide for details.
    *
    *******************************************************************************/
    CY_ISR(CapSense_CSDPostMultiScan)
    {
        /*  Declare and initialise ptr to widget and sensor structures to appropriate address        */
        #if ((CapSense_ENABLE == CapSense_CSD_IDAC_COMP_EN) || \
            (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN)))
            CapSense_RAM_WD_BASE_STRUCT *ptrWdgt = (CapSense_RAM_WD_BASE_STRUCT *)
                                                            CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2WdgtRam;
        #endif /* ((CapSense_ENABLE == CapSense_CSD_IDAC_COMP_EN) || \
                   (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN))) */

        /*  Declare and initialise ptr to sensor IO structure to appropriate address        */
        CapSense_FLASH_IO_STRUCT const *curSnsIOPtr = (CapSense_FLASH_IO_STRUCT const *)
                                                          CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2SnsFlash
                                                          + CapSense_sensorIndex;

        #ifdef CapSense_ENTRY_CALLBACK
            CapSense_EntryCallback();
        #endif /* CapSense_ENTRY_CALLBACK */

        /* Clear pending interrupt */
        CY_SET_REG32(CapSense_INTR_PTR, CapSense_INTR_SET_MASK);

        /* Read SlotResult from Raw Counter */
        CapSense_curRamSnsPtr->raw[CapSense_scanFreqIndex] = (uint16)CY_GET_REG32(CapSense_COUNTER_PTR);

        /*  Connect Vref Buffer to AMUX bus */
        CY_SET_REG32(CapSense_CONFIG_PTR, CapSense_configCsd | CapSense_CMOD_PRECHARGE_CONFIG_CSD_EN);

        #if (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN)
        /* Disable sensor when all frequency channels are scanned */
        if (CapSense_FREQ_CHANNEL_2 == CapSense_scanFreqIndex)
        #endif /* (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN) */
        {
            /* Disable sensor */
            CapSense_CSDDisconnectSns(curSnsIOPtr);
        }

        #if (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN)
            if (CapSense_FREQ_CHANNEL_2 > CapSense_scanFreqIndex)
            {
                CapSense_SsNextFrequencyScan();
            }
            else
        #endif /* (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN) */
            {
                #if (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN)
                    CapSense_scanFreqIndex = CapSense_FREQ_CHANNEL_0;
                    CapSense_ChangeImoFreq(CapSense_FREQ_CHANNEL_0);
                #endif  /* (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN) */

                /* Check if all sensors are scanned in widget */
                if (0u < CapSense_sensorIndex)
                {
                    /*  Decrement snsIndex to configure next sensor in widget */
                    CapSense_sensorIndex--;

                    /*  Update global pointer to CapSense_RAM_SNS_STRUCT to current sensor  */
                    CapSense_curRamSnsPtr = (CapSense_RAM_SNS_STRUCT *)
                                                              CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2SnsRam
                                                              + CapSense_sensorIndex;

                    /*  Initialise ptr to sensor IO structure to appropriate address        */
                    curSnsIOPtr = (CapSense_FLASH_IO_STRUCT const *)
                                                              CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2SnsFlash
                                                              + CapSense_sensorIndex;

                    /* Configure PRS mode */
                    #if ((CapSense_DISABLE == CapSense_CSD_COMMON_SNS_CLK_EN) && \
                         (CapSense_CLK_SOURCE_DIRECT != CapSense_CSD_SNS_CLK_SOURCE))
                        CapSense_CSDConfigurePrsMode();
                    #endif /* ((CapSense_DISABLE == CapSense_CSD_COMMON_SNS_CLK_EN) && \
                               (CapSense_CLK_SOURCE_DIRECT != CapSense_CSD_SNS_CLK_SOURCE)) */

                    /* Setup Compensation IDAC for next sensor in widget */
                    #if ((CapSense_ENABLE == CapSense_CSD_IDAC_COMP_EN) || \
                         (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN)))
                        CapSense_SetUpIdacs(ptrWdgt);
                    #endif /* ((CapSense_ENABLE == CapSense_CSD_IDAC_COMP_EN) || \
                         (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN))) */

                    /* Enable sensor */
                    CapSense_CSDConnectSns(curSnsIOPtr);

                    /* Proceed scanning */
                    CapSense_CSDStartSample();
                }
                /*    Call scan next widget API if requested, if not, complete the scan  */
                else
                {
                    /* Current widget is totally scanned. Reset WIDGET BUSY flag */
                    CapSense_dsRam.status &= ~CapSense_WDGT_SW_STS_BUSY;

                    /* Check if all widgets have been scanned */
                    if (CapSense_ENABLE == CapSense_requestScanAllWidget)
                    {
                        /* Configure and begin scanning next widget */
                        CapSense_PostAllWidgetsScan();
                    }
                    else
                    {
                        #if (CapSense_ENABLE == CapSense_BLOCK_OFF_AFTER_SCAN_EN)
                            /*  Disable the CSD block. Connect Vref Buffer to AMUX bus */
                            CY_SET_REG32(CapSense_CONFIG_PTR, CapSense_configCsd | CapSense_CMOD_PRECHARGE_CONFIG);
                        #endif /* (CapSense_ENABLE == CapSense_BLOCK_OFF_AFTER_SCAN_EN)  */

                        /* All widgets are totally scanned. Reset BUSY flag */
                        CapSense_dsRam.status &= ~CapSense_SW_STS_BUSY;

                        /* Update scan Counter */
                        CapSense_dsRam.scanCounter++;
                    }
                }
            }

        #ifdef CapSense_EXIT_CALLBACK
            CapSense_ExitCallback();
        #endif /* CapSense_EXIT_CALLBACK */
    }


    #if (CapSense_ENABLE == CapSense_CSD_GANGED_SNS_EN)
    /*******************************************************************************
    * Function Name: CapSense_CSDPostMultiScanGanged
    ****************************************************************************//**
    *
    * \brief
    *   This is an internal ISR function for multiple sensor scanning implementation
    *   for ganged sensors.
    *
    * \details
    *   This ISR handler is triggered when the user calls the
    *   CapSense_Scan() API for a ganged sensor or the
    *   CapSense_ScanAllWidgets() API in the project with ganged sensors.
    *
    *   The following tasks are performed:
    *   - Disable the CSD interrupt
    *   - Read the Counter register and update the data structure with raw data
    *   - Connect the Vref buffer to the AMUX bus
    *   - Disable the CSD block (after the widget has been scanned)
    *   - Update Scan Counter
    *   - Reset the BUSY flag
    *   - Enable the CSD interrupt.
    *
    *   The ISR handler initializes scanning for the previous sensor when the widget has
    *   more than one sensor.
    *   The ISR handler initializes scanning for the next widget when the
    *   CapSense_ScanAllWidgets() APIs are called and the project has more than
    *   one widget.
    *   The ISR handler changes the IMO and initializes scanning for the next frequency
    *   channels when multi-frequency scanning is enabled.
    *
    *   This function has two Macro Callbacks that allow to call user code from macros
    *   specified in a component's generated code. Refer to Macro Callbacks section
    *   of PSoC Creator� User Guide for details.
    *
    *******************************************************************************/
    CY_ISR(CapSense_CSDPostMultiScanGanged)
    {
        uint32 tempVal;

        /*  Declare and initialise ptr to widget and sensor structures to appropriate address        */
        #if ((CapSense_ENABLE == CapSense_CSD_IDAC_COMP_EN) || \
            (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN)))
            CapSense_RAM_WD_BASE_STRUCT *ptrWdgt = (CapSense_RAM_WD_BASE_STRUCT *)
                                                            CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2WdgtRam;
        #endif /* ((CapSense_ENABLE == CapSense_CSD_IDAC_COMP_EN) || \
                   (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN))) */

        /*  Declare and initialise ptr to sensor IO structure to appropriate address       */
        CapSense_FLASH_IO_STRUCT const *curSnsIOPtr =
            (CapSense_FLASH_IO_STRUCT const *)CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2SnsFlash
            + CapSense_sensorIndex;

        #ifdef CapSense_ENTRY_CALLBACK
            CapSense_EntryCallback();
        #endif /* CapSense_ENTRY_CALLBACK */

        /* Clear pending interrupt */
        CY_SET_REG32(CapSense_INTR_PTR, CapSense_INTR_SET_MASK);

        /* Read SlotResult from Raw Counter */
       CapSense_curRamSnsPtr->raw[CapSense_scanFreqIndex] = (uint16)CY_GET_REG32(CapSense_COUNTER_PTR);

        /*  Connect Vref Buffer to AMUX bus */
        CY_SET_REG32(CapSense_CONFIG_PTR, CapSense_configCsd | CapSense_CMOD_PRECHARGE_CONFIG_CSD_EN);

        #if (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN)
        if (CapSense_FREQ_CHANNEL_2 == CapSense_scanFreqIndex)
        #endif /* (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN) */
        {
            /* Check ganged sns flag  */
            if ((CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].staticConfig &
                CapSense_GANGED_SNS_MASK) == CapSense_GANGED_SNS_MASK)
            {
                /* Get sns pointer */
                CapSense_curFlashSnsPtr = (CapSense_FLASH_SNS_STRUCT const *)
                                                   CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2SnsFlash +
                                                   CapSense_sensorIndex;

                /* Get number of ganged pins */
                tempVal = CapSense_curFlashSnsPtr->numPins;

                /* Get IO pointer  */
                curSnsIOPtr = &CapSense_ioList[CapSense_curFlashSnsPtr->firstPinId];

                /* Disconnect all ganged sensors */
                do
                {
                    CapSense_CSDDisconnectSns(curSnsIOPtr);
                    curSnsIOPtr++;
                    tempVal--;
                } while (0u != tempVal);
            }
            else
            {
                /* Disconnect ganged sensor */
                CapSense_CSDDisconnectSns(curSnsIOPtr);
            }
        }

        #if (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN)
            if (CapSense_FREQ_CHANNEL_2 > CapSense_scanFreqIndex)
            {
                CapSense_SsNextFrequencyScan();
            }
            else
        #endif /* (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN) */
            {
                #if (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN)
                    CapSense_scanFreqIndex = CapSense_FREQ_CHANNEL_0;
                    CapSense_ChangeImoFreq(CapSense_FREQ_CHANNEL_0);
                #endif  /* (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN) */

                /* Check if all sensors are scanned in the widget */
                if (0u < CapSense_sensorIndex)
                {
                    /*  Decrement snsIndex to configure next sensor in widget */
                    CapSense_sensorIndex--;

                    /*  Update global pointer to CapSense_RAM_SNS_STRUCT to current sensor  */
                    CapSense_curRamSnsPtr = (CapSense_RAM_SNS_STRUCT *)
                                                              CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2SnsRam
                                                              + CapSense_sensorIndex;

                    /* Configure PRS mode */
                    #if ((CapSense_DISABLE == CapSense_CSD_COMMON_SNS_CLK_EN) && \
                         (CapSense_CLK_SOURCE_DIRECT != CapSense_CSD_SNS_CLK_SOURCE))
                        CapSense_CSDConfigurePrsMode();
                    #endif /* ((CapSense_DISABLE == CapSense_CSD_COMMON_SNS_CLK_EN) && \
                               (CapSense_CLK_SOURCE_DIRECT != CapSense_CSD_SNS_CLK_SOURCE)) */

                    /* Set up Compensation IDAC for next sensor in widget */
                    #if ((CapSense_ENABLE == CapSense_CSD_IDAC_COMP_EN) || \
                         (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN)))
                        CapSense_SetUpIdacs(ptrWdgt);
                    #endif /* ((CapSense_ENABLE == CapSense_CSD_IDAC_COMP_EN) || \
                         (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN))) */

                    /* Check ganged sns flag  */
                    if (CapSense_GANGED_SNS_MASK ==
                       (CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].staticConfig &
                        CapSense_GANGED_SNS_MASK))
                    {
                        /* Get sns pointer */
                        CapSense_curFlashSnsPtr = (CapSense_FLASH_SNS_STRUCT const *)
                                                           CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2SnsFlash +
                                                           CapSense_sensorIndex;

                        /* Get number of ganged pins */
                        tempVal = CapSense_curFlashSnsPtr->numPins;

                        /* Get IO pointer  */
                        curSnsIOPtr = &CapSense_ioList[CapSense_curFlashSnsPtr->firstPinId];

                        /* Connect all ganged sensors  */
                        do
                        {
                            CapSense_CSDConnectSns(curSnsIOPtr);
                            curSnsIOPtr++;
                            tempVal--;
                        } while (0u != tempVal);
                    }
                    else
                    {
                        /*  Initialize ptr to sensor IO structure to appropriate address       */
                        curSnsIOPtr = (CapSense_FLASH_IO_STRUCT const *)CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2SnsFlash
                                       + CapSense_sensorIndex;

                        /* Enable sensor */
                        CapSense_CSDConnectSns(curSnsIOPtr);
                    }

                    /* Proceed scanning */
                    CapSense_CSDStartSample();
                }
                /*    Call scan next widget API if requested, if not, complete scan  */
                else
                {
                    /* Current widget is totally scanned. Reset WIDGET BUSY flag */
                    CapSense_dsRam.status &= ~CapSense_WDGT_SW_STS_BUSY;

                    /* Check if all widgets have been scanned */
                    if (CapSense_ENABLE == CapSense_requestScanAllWidget)
                    {
                        /* Configure and begin scanning next widget */
                        CapSense_PostAllWidgetsScan();
                    }
                    else
                    {
                        #if (CapSense_ENABLE == CapSense_BLOCK_OFF_AFTER_SCAN_EN)
                            /*  Disable the CSD block. Connect Vref Buffer to AMUX bus */
                            CY_SET_REG32(CapSense_CONFIG_PTR, CapSense_configCsd | CapSense_CMOD_PRECHARGE_CONFIG);
                        #endif /* (CapSense_ENABLE == CapSense_CSDV2) */

                        /* All widgets are totally scanned. Reset BUSY flag */
                        CapSense_dsRam.status &= ~CapSense_SW_STS_BUSY;

                        /* Update Scan Counter */
                        CapSense_dsRam.scanCounter++;
                    }
                }
            }

        #ifdef CapSense_EXIT_CALLBACK
            CapSense_ExitCallback();
        #endif /* CapSense_EXIT_CALLBACK */
    }
    #endif /* (CapSense_ENABLE == CapSense_CSD_GANGED_SNS_EN) */

#endif /* (CapSense_ENABLE == CapSense_CSDV2) */
#endif /* ((CapSense_ENABLE == CapSense_CSD_EN) || (CapSense_ENABLE == CapSense_CSD_CSX_EN)) */

/** \}
 * \endif */

#if ((CapSense_ENABLE == CapSense_CSD_EN) || (CapSense_ENABLE == CapSense_CSD_CSX_EN))

#if (CapSense_ENABLE == CapSense_CSDV2)
    /*******************************************************************************
    * Function Name: CapSense_SsPostScan
    ****************************************************************************//**
    *
    * \brief
    *   This function reads rawdata and releases required HW resoures after scan.
    *
    * \details
    *   This function performs following tasks after scan:
    *   - Reads SlotResult from Raw Counter;
    *   - Inits bad Conversions number;
    *   - Disconnects Vrefhi from AMUBUF positive input;
    *   - Disconnects AMUBUF output from CSDBUSB with sych PHI2+HSCMP;
    *   - Opens HCBV and HCBG switches.
    * 
    *******************************************************************************/
    static void CapSense_SsPostScan(void)
    {
        uint32 tmpRawData;
        uint32 tmpMaxCount;
        
        /* Read SlotResult from Raw Counter */
        tmpMaxCount = ((1uL << CapSense_curWidgetResolution) - 1uL);
        tmpRawData = CapSense_RESULT_VAL1_VALUE_MASK & CY_GET_REG32(CapSense_COUNTER_PTR);
        
        if(tmpRawData < tmpMaxCount)
        {
            CapSense_curRamSnsPtr->raw[CapSense_scanFreqIndex] = LO16(tmpRawData);
        }
        else
        {
            CapSense_curRamSnsPtr->raw[CapSense_scanFreqIndex] = LO16(tmpMaxCount);
        }

        #if (CapSense_ENABLE == CapSense_CSD_NOISE_METRIC_EN)
            /*  Init bad Conversions number */
            CapSense_badConversionsNum = CapSense_BAD_CONVERSIONS_NUM;
        #endif /* (CapSense_ENABLE == CapSense_CSD_NOISE_METRIC_EN) */

        #if ((CapSense_ENABLE == CapSense_CSD_SHIELD_EN) && \
             (CapSense_2000_MV <= CapSense_CYDEV_VDDA_MV))
            /*  Disconnect Vrefhi from AMUBUF positive input. Disconnect AMUBUF output from CSDBUSB with sych PHI2+HSCMP  */
            CY_SET_REG32(CapSense_SW_AMUXBUF_SEL_PTR, CapSense_SW_AMUXBUF_SEL_SW_DEFAULT);
        #endif /* ((CapSense_ENABLE == CapSense_CSD_SHIELD_EN) && \
                   (CapSense_2000_MV <= CapSense_CYDEV_VDDA_MV)) */

        #if (CapSense_ENABLE == CapSense_CSD_SHIELD_EN)
            /* Open HCBV and HCBG switches */
            CY_SET_REG32(CapSense_SW_SHIELD_SEL_PTR, CapSense_SW_SHIELD_SEL_SW_HCBV_STATIC_OPEN |
                                                             CapSense_SW_SHIELD_SEL_SW_HCBG_STATIC_OPEN);
        #endif /* (CapSense_ENABLE == CapSense_CSD_SHIELD_EN) */
    }


    /*******************************************************************************
    * Function Name: CapSense_SsInitNextScan
    ****************************************************************************//**
    *
    * \brief
    *   This function initializes the next sensor scan.
    *
    * \details
    *   The function decrements the sensor index, updates sense clock for matrix 
    *   or touchpad widgets only, sets up Compensation IDAC, enables the sensor and
    *   scans it. When all sensors are scanned it continues to set up the next widget
    *   until all widgets are scanned. The CSD block is disabled when all widgets are 
    *   scanned.
    * 
    *******************************************************************************/
    CY_INLINE static void CapSense_SsInitNextScan(void)
    {
        /*  Declare and initialise ptr to widget and sensor structures to appropriate address */
        #if ((((CapSense_ENABLE == CapSense_CSD_IDAC_COMP_EN) && \
              (CapSense_ENABLE == CapSense_CSD_DEDICATED_IDAC_COMP_EN) || \
              (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN)))) || \
             (((CapSense_DISABLE == CapSense_CSD_COMMON_SNS_CLK_EN) && \
              (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN))) && \
              (CapSense_CLK_SOURCE_DIRECT == CapSense_CSD_SNS_CLK_SOURCE)))
            CapSense_RAM_WD_BASE_STRUCT *ptrWdgt = (CapSense_RAM_WD_BASE_STRUCT *)
                                                            CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2WdgtRam;
        #endif /* ((((CapSense_ENABLE == CapSense_CSD_IDAC_COMP_EN) && \
                   (CapSense_ENABLE == CapSense_CSD_DEDICATED_IDAC_COMP_EN) || \
                   (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN)))) || \
                   (((CapSense_DISABLE == CapSense_CSD_COMMON_SNS_CLK_EN) && \
                   (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN))) && \
                   (CapSense_CLK_SOURCE_DIRECT == CapSense_CSD_SNS_CLK_SOURCE))) */

        /*  Declare and initialise ptr to sensor IO structure to appropriate address        */  
        CapSense_FLASH_IO_STRUCT const *curSnsIOPtr = (CapSense_FLASH_IO_STRUCT const *)
                                                          CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2SnsFlash 
                                                          + CapSense_sensorIndex;  
                   
        /* Check if all sensors are scanned in widget */
        if (0u < CapSense_sensorIndex)
        {
            /*  Decrement snsIndex to configure next sensor in widget */
            CapSense_sensorIndex--;

            /*  Update global pointer to CapSense_RAM_SNS_STRUCT to current sensor  */
            CapSense_curRamSnsPtr = (CapSense_RAM_SNS_STRUCT *)
                                                      CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2SnsRam
                                                      + CapSense_sensorIndex; 
            
            /*  Initialise ptr to sensor IO structure to appropriate address        */
            curSnsIOPtr = (CapSense_FLASH_IO_STRUCT const *)
                                                      CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2SnsFlash 
                                                      + CapSense_sensorIndex;

            #if ((CapSense_DISABLE == CapSense_CSD_COMMON_SNS_CLK_EN) && \
                 (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN)))
                #if (CapSense_CLK_SOURCE_DIRECT != CapSense_CSD_SNS_CLK_SOURCE)
                    /* Configure PRS mode */
                    CapSense_CSDConfigurePrsMode();
                #else
                    if (CapSense_dsFlash.wdgtArray[(CapSense_widgetIndex)].numCols <= CapSense_sensorIndex)
                    {
                        /*  Set sense clock divider  */
                        CY_SET_REG32(CapSense_SENSE_PERIOD_PTR, CapSense_DEFAULT_SENSE_PERIOD | (uint32)(ptrWdgt->rowSnsClk - 1u)); 
                    }
                    else
                    {
                        /*  Set sense clock divider  */
                        CY_SET_REG32(CapSense_SENSE_PERIOD_PTR, CapSense_DEFAULT_SENSE_PERIOD | (uint32)(ptrWdgt->snsClk - 1u)); 
                    }
                #endif /* (CapSense_CLK_SOURCE_DIRECT != CapSense_CSD_SNS_CLK_SOURCE) */      
            #endif /* ((CapSense_DISABLE == CapSense_CSD_COMMON_SNS_CLK_EN) && \
                 (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN)))  */

            /* Setup Compensation IDAC for next sensor in widget */
            #if (((CapSense_ENABLE == CapSense_CSD_IDAC_COMP_EN) && \
                 (CapSense_ENABLE == CapSense_CSD_DEDICATED_IDAC_COMP_EN) || \
                 (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN))))
                CapSense_SetUpIdacs(ptrWdgt);
            #endif /* ((CapSense_ENABLE == CapSense_CSD_IDAC_COMP_EN) && \
                 (CapSense_ENABLE == CapSense_CSD_DEDICATED_IDAC_COMP_EN) || \
                 (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN)))) */
            
            /* Enable sensor */
            CapSense_CSDConnectSns(curSnsIOPtr);

            /* Proceed scanning */
            CapSense_CSDStartSample();
        }
        /*    Call scan next widget API if requested, if not, complete the scan  */
        else 
        {
            #if ((CapSense_ENABLE == CapSense_CSD_SHIELD_EN) && \
                 (0u != CapSense_CSD_TOTAL_SHIELD_COUNT))
                /*	Disable shield pins */
                CapSense_DisableShieldElectrodes();
            #endif /* ((CapSense_ENABLE == CapSense_CSD_SHIELD_EN) && \
                       (0u != CapSense_CSD_TOTAL_SHIELD_COUNT)) */
            
            /* Current widget is totally scanned. Reset WIDGET BUSY flag */
            CapSense_dsRam.status &= ~CapSense_WDGT_SW_STS_BUSY;

            /* Check if all widgets have been scanned */
            if (CapSense_ENABLE == CapSense_requestScanAllWidget)  
            {
                /* Configure and begin scanning next widget */
                CapSense_PostAllWidgetsScan();
            }
            else
            {
                #if (CapSense_ENABLE == CapSense_BLOCK_OFF_AFTER_SCAN_EN)
                    /*  Disable the CSD block */
                    CY_SET_REG32(CapSense_CONFIG_PTR, CapSense_configCsd);
                #endif /* (CapSense_ENABLE == CapSense_BLOCK_OFF_AFTER_SCAN_EN) */

                /* All widgets are totally scanned. Reset BUSY flag */
                CapSense_dsRam.status &= ~CapSense_SW_STS_BUSY;
                
                /* Update scan Counter */
                CapSense_dsRam.scanCounter++;
            }
        }
    
    }

    
    #if (CapSense_ENABLE == CapSense_CSD_GANGED_SNS_EN)
    /*******************************************************************************
    * Function Name: CapSense_SsInitNextGangedScan
    ****************************************************************************//**
    *
    * \brief
    *   This function initializes the next ganged sensor scan.
    *
    * \details
    *   The function decrements the sensor index, updates sense clock for matrix 
    *   or touchpad widgets only, sets up Compensation IDAC, enables the ganged sensor
    *   and scans it. When all sensors are scanned it continues to set up the next 
    *   widget until all widgets are scanned. The CSD block is disabled when all 
    *   widgets are scanned.
    * 
    *******************************************************************************/
    CY_INLINE static void CapSense_SsInitNextGangedScan(void)
    {
        uint32 tempVal;

        /*  Declare and initialise ptr to widget and sensor structures to appropriate address */
        #if ((((CapSense_ENABLE == CapSense_CSD_IDAC_COMP_EN) && \
              (CapSense_ENABLE == CapSense_CSD_DEDICATED_IDAC_COMP_EN) || \
              (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN)))) || \
             (((CapSense_DISABLE == CapSense_CSD_COMMON_SNS_CLK_EN) && \
              (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN))) && \
              (CapSense_CLK_SOURCE_DIRECT == CapSense_CSD_SNS_CLK_SOURCE)))
            CapSense_RAM_WD_BASE_STRUCT *ptrWdgt = (CapSense_RAM_WD_BASE_STRUCT *)
                                                            CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2WdgtRam;
        #endif /* ((((CapSense_ENABLE == CapSense_CSD_IDAC_COMP_EN) && \
                   (CapSense_ENABLE == CapSense_CSD_DEDICATED_IDAC_COMP_EN) || \
                   (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN)))) || \
                   (((CapSense_DISABLE == CapSense_CSD_COMMON_SNS_CLK_EN) && \
                   (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN))) && \
                   (CapSense_CLK_SOURCE_DIRECT == CapSense_CSD_SNS_CLK_SOURCE))) */

        /*  Declare and initialise ptr to sensor IO structure to appropriate address        */  
        CapSense_FLASH_IO_STRUCT const *curSnsIOPtr = (CapSense_FLASH_IO_STRUCT const *)
                                                          CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2SnsFlash 
                                                          + CapSense_sensorIndex;

        /* Check if all sensors are scanned in the widget */
        if (0u < CapSense_sensorIndex)
        {
            /*  Decrement snsIndex to configure next sensor in widget */
            CapSense_sensorIndex--;

            /*  Update global pointer to CapSense_RAM_SNS_STRUCT to current sensor  */
            CapSense_curRamSnsPtr = (CapSense_RAM_SNS_STRUCT *)
                                                      CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2SnsRam
                                                      + CapSense_sensorIndex;

            #if ((CapSense_DISABLE == CapSense_CSD_COMMON_SNS_CLK_EN) && \
                 (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN)))
                #if (CapSense_CLK_SOURCE_DIRECT != CapSense_CSD_SNS_CLK_SOURCE)
                    /* Configure PRS mode */
                    CapSense_CSDConfigurePrsMode();
                #else
                    if (CapSense_dsFlash.wdgtArray[(CapSense_widgetIndex)].numCols <= CapSense_sensorIndex)
                    {
                        /*  Set sense clock divider  */
                        CY_SET_REG32(CapSense_SENSE_PERIOD_PTR, CapSense_DEFAULT_SENSE_PERIOD | (uint32)(ptrWdgt->rowSnsClk - 1u));
                    }
                    else
                    {
                        /*  Set sense clock divider  */
                        CY_SET_REG32(CapSense_SENSE_PERIOD_PTR, CapSense_DEFAULT_SENSE_PERIOD | (uint32)(ptrWdgt->snsClk - 1u));
                    }
                #endif /* (CapSense_CLK_SOURCE_DIRECT != CapSense_CSD_SNS_CLK_SOURCE) */
            #endif /* ((CapSense_DISABLE == CapSense_CSD_COMMON_SNS_CLK_EN) && \
                 (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN)))  */

            /* Set up Compensation IDAC for next sensor in widget */
            #if (((CapSense_ENABLE == CapSense_CSD_IDAC_COMP_EN) && \
                 (CapSense_ENABLE == CapSense_CSD_DEDICATED_IDAC_COMP_EN) || \
                 (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN))))
                CapSense_SetUpIdacs(ptrWdgt);
            #endif /* ((CapSense_ENABLE == CapSense_CSD_IDAC_COMP_EN) && \
                 (CapSense_ENABLE == CapSense_CSD_DEDICATED_IDAC_COMP_EN) || \
                 (CapSense_ENABLE == (CapSense_CSD_MATRIX_WIDGET_EN | CapSense_CSD_TOUCHPAD_WIDGET_EN)))) */

            /* Check ganged sns flag  */
            if (CapSense_GANGED_SNS_MASK ==
               (CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].staticConfig &
                CapSense_GANGED_SNS_MASK))
            {
                /* Get sns pointer */
                CapSense_curFlashSnsPtr = (CapSense_FLASH_SNS_STRUCT const *)
                                                   CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2SnsFlash +
                                                   CapSense_sensorIndex;

                /* Get number of ganged pins */
                tempVal = CapSense_curFlashSnsPtr->numPins;

                /* Get IO pointer  */
                curSnsIOPtr = &CapSense_ioList[CapSense_curFlashSnsPtr->firstPinId];

                /* Connect all ganged sensors  */
                do
                {
                    CapSense_CSDConnectSns(curSnsIOPtr);
                    curSnsIOPtr++;
                    tempVal--;
                } while (0u != tempVal);
            }
            else
            {
                /*  Initialize ptr to sensor IO structure to appropriate address       */
                curSnsIOPtr = (CapSense_FLASH_IO_STRUCT const *)CapSense_dsFlash.wdgtArray[CapSense_widgetIndex].ptr2SnsFlash
                               + CapSense_sensorIndex;

                /* Enable sensor */
                CapSense_CSDConnectSns(curSnsIOPtr);
            }

            /* Proceed scanning */
            CapSense_CSDStartSample();
        }
        /*    Call scan next widget API if requested, if not, complete scan  */
        else
        {
            #if ((CapSense_ENABLE == CapSense_CSD_SHIELD_EN) && \
                 (0u != CapSense_CSD_TOTAL_SHIELD_COUNT))
                /*  Disable shield pins */
                CapSense_DisableShieldElectrodes();
            #endif /* ((CapSense_ENABLE == CapSense_CSD_SHIELD_EN) && \
                       (0u != CapSense_CSD_TOTAL_SHIELD_COUNT)) */

            /* Current widget is totally scanned. Reset WIDGET BUSY flag */
            CapSense_dsRam.status &= ~CapSense_WDGT_SW_STS_BUSY;

            /* Check if all widgets have been scanned */
            if (CapSense_ENABLE == CapSense_requestScanAllWidget)
            {
                /* Configure and begin scanning next widget */
                CapSense_PostAllWidgetsScan();
            }
            else
            {
                #if (CapSense_ENABLE == CapSense_BLOCK_OFF_AFTER_SCAN_EN)
                    /*  Disable the CSD block */
                    CY_SET_REG32(CapSense_CONFIG_PTR, CapSense_configCsd);
                #endif /* (CapSense_ENABLE == CapSense_BLOCK_OFF_AFTER_SCAN_EN) */

                /* All widgets are totally scanned. Reset BUSY flag */
                CapSense_dsRam.status &= ~CapSense_SW_STS_BUSY;

                /* Update Scan Counter */
                CapSense_dsRam.scanCounter++;
            }
        }
    }
    #endif /* (CapSense_ENABLE == CapSense_CSD_GANGED_SNS_EN) */
#endif /* (CapSense_ENABLE == CapSense_CSDV2) */ 
    

#if (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN)
/*******************************************************************************
* Function Name: CapSense_SsNextFrequencyScan
****************************************************************************//**
*
* \brief
*   This function scans the sensor on the next frequency channel.
*
* \details
*   The function increments the frequency channel, changes IMO and initializes 
*   the scanning process of the same sensor.
* 
*******************************************************************************/
static void CapSense_SsNextFrequencyScan(void)
{
    CapSense_scanFreqIndex++;

    /* Set Immunity */
    CapSense_ChangeImoFreq((uint32)CapSense_scanFreqIndex);
    
    /* Proceed scanning */
    CapSense_CSDStartSample();       
}
#endif /* (CapSense_ENABLE == CapSense_MULTI_FREQ_SCAN_EN) */    
#endif /* ((CapSense_ENABLE == CapSense_CSD_EN) || (CapSense_ENABLE == CapSense_CSD_CSX_EN)) */    


/* [] END OF FILE */
