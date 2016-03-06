/*******************************************************************************
* File Name: P23.h  
* Version 2.20
*
* Description:
*  This file contains Pin function prototypes and register defines
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_PINS_P23_H) /* Pins P23_H */
#define CY_PINS_P23_H

#include "cytypes.h"
#include "cyfitter.h"
#include "P23_aliases.h"


/***************************************
*     Data Struct Definitions
***************************************/

/**
* \addtogroup group_structures
* @{
*/
    
/* Structure for sleep mode support */
typedef struct
{
    uint32 pcState; /**< State of the port control register */
    uint32 sioState; /**< State of the SIO configuration */
    uint32 usbState; /**< State of the USBIO regulator */
} P23_BACKUP_STRUCT;

/** @} structures */


/***************************************
*        Function Prototypes             
***************************************/
/**
* \addtogroup group_general
* @{
*/
uint8   P23_Read(void);
void    P23_Write(uint8 value);
uint8   P23_ReadDataReg(void);
#if defined(P23__PC) || (CY_PSOC4_4200L) 
    void    P23_SetDriveMode(uint8 mode);
#endif
void    P23_SetInterruptMode(uint16 position, uint16 mode);
uint8   P23_ClearInterrupt(void);
/** @} general */

/**
* \addtogroup group_power
* @{
*/
void P23_Sleep(void); 
void P23_Wakeup(void);
/** @} power */


/***************************************
*           API Constants        
***************************************/
#if defined(P23__PC) || (CY_PSOC4_4200L) 
    /* Drive Modes */
    #define P23_DRIVE_MODE_BITS        (3)
    #define P23_DRIVE_MODE_IND_MASK    (0xFFFFFFFFu >> (32 - P23_DRIVE_MODE_BITS))

    /**
    * \addtogroup group_constants
    * @{
    */
        /** \addtogroup driveMode Drive mode constants
         * \brief Constants to be passed as "mode" parameter in the P23_SetDriveMode() function.
         *  @{
         */
        #define P23_DM_ALG_HIZ         (0x00u) /**< \brief High Impedance Analog   */
        #define P23_DM_DIG_HIZ         (0x01u) /**< \brief High Impedance Digital  */
        #define P23_DM_RES_UP          (0x02u) /**< \brief Resistive Pull Up       */
        #define P23_DM_RES_DWN         (0x03u) /**< \brief Resistive Pull Down     */
        #define P23_DM_OD_LO           (0x04u) /**< \brief Open Drain, Drives Low  */
        #define P23_DM_OD_HI           (0x05u) /**< \brief Open Drain, Drives High */
        #define P23_DM_STRONG          (0x06u) /**< \brief Strong Drive            */
        #define P23_DM_RES_UPDWN       (0x07u) /**< \brief Resistive Pull Up/Down  */
        /** @} driveMode */
    /** @} group_constants */
#endif

/* Digital Port Constants */
#define P23_MASK               P23__MASK
#define P23_SHIFT              P23__SHIFT
#define P23_WIDTH              1u

/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in P23_SetInterruptMode() function.
     *  @{
     */
        #define P23_INTR_NONE      ((uint16)(0x0000u)) /**< \brief Disabled             */
        #define P23_INTR_RISING    ((uint16)(0x5555u)) /**< \brief Rising edge trigger  */
        #define P23_INTR_FALLING   ((uint16)(0xaaaau)) /**< \brief Falling edge trigger */
        #define P23_INTR_BOTH      ((uint16)(0xffffu)) /**< \brief Both edge trigger    */
    /** @} intrMode */
/** @} group_constants */

/* SIO LPM definition */
#if defined(P23__SIO)
    #define P23_SIO_LPM_MASK       (0x03u)
#endif

/* USBIO definitions */
#if !defined(P23__PC) && (CY_PSOC4_4200L)
    #define P23_USBIO_ENABLE               ((uint32)0x80000000u)
    #define P23_USBIO_DISABLE              ((uint32)(~P23_USBIO_ENABLE))
    #define P23_USBIO_SUSPEND_SHIFT        CYFLD_USBDEVv2_USB_SUSPEND__OFFSET
    #define P23_USBIO_SUSPEND_DEL_SHIFT    CYFLD_USBDEVv2_USB_SUSPEND_DEL__OFFSET
    #define P23_USBIO_ENTER_SLEEP          ((uint32)((1u << P23_USBIO_SUSPEND_SHIFT) \
                                                        | (1u << P23_USBIO_SUSPEND_DEL_SHIFT)))
    #define P23_USBIO_EXIT_SLEEP_PH1       ((uint32)~((uint32)(1u << P23_USBIO_SUSPEND_SHIFT)))
    #define P23_USBIO_EXIT_SLEEP_PH2       ((uint32)~((uint32)(1u << P23_USBIO_SUSPEND_DEL_SHIFT)))
    #define P23_USBIO_CR1_OFF              ((uint32)0xfffffffeu)
#endif


/***************************************
*             Registers        
***************************************/
/* Main Port Registers */
#if defined(P23__PC)
    /* Port Configuration */
    #define P23_PC                 (* (reg32 *) P23__PC)
#endif
/* Pin State */
#define P23_PS                     (* (reg32 *) P23__PS)
/* Data Register */
#define P23_DR                     (* (reg32 *) P23__DR)
/* Input Buffer Disable Override */
#define P23_INP_DIS                (* (reg32 *) P23__PC2)

/* Interrupt configuration Registers */
#define P23_INTCFG                 (* (reg32 *) P23__INTCFG)
#define P23_INTSTAT                (* (reg32 *) P23__INTSTAT)

/* "Interrupt cause" register for Combined Port Interrupt (AllPortInt) in GSRef component */
#if defined (CYREG_GPIO_INTR_CAUSE)
    #define P23_INTR_CAUSE         (* (reg32 *) CYREG_GPIO_INTR_CAUSE)
#endif

/* SIO register */
#if defined(P23__SIO)
    #define P23_SIO_REG            (* (reg32 *) P23__SIO)
#endif /* (P23__SIO_CFG) */

/* USBIO registers */
#if !defined(P23__PC) && (CY_PSOC4_4200L)
    #define P23_USB_POWER_REG       (* (reg32 *) CYREG_USBDEVv2_USB_POWER_CTRL)
    #define P23_CR1_REG             (* (reg32 *) CYREG_USBDEVv2_CR1)
    #define P23_USBIO_CTRL_REG      (* (reg32 *) CYREG_USBDEVv2_USB_USBIO_CTRL)
#endif    
    
    
/***************************************
* The following code is DEPRECATED and 
* must not be used in new designs.
***************************************/
/**
* \addtogroup group_deprecated
* @{
*/
#define P23_DRIVE_MODE_SHIFT       (0x00u)
#define P23_DRIVE_MODE_MASK        (0x07u << P23_DRIVE_MODE_SHIFT)
/** @} deprecated */

#endif /* End Pins P23_H */


/* [] END OF FILE */
