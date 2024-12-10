/** @file  sr1120_V3_reg.h
 *  @brief SR1120_V3 registers map. This file assumes a little-endian processor.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *  @author    SPARK FW Team.
 */
/*
 *                                                                     /##
 *                                                                 ,#####
 *                                  . /(######((/,.            .#######(
 *                          /######################         ##########*
 *                     /#######################.        /############.
 *                 *##########%/                    .###############
 *              /#########,                      ##########((#########.
 *            %#######/                      (#########%   #############(
 *          #######(                     *##########,     %#####(  ########
 *        %######.                    ##########(        ######*     #######(
 *      *######,                  (##########          .######.        #######
 *     (#####%                (##########.            ,######           .######*
 *    ######/             ,##########/               /######              ######/
 *   (#####*           %##########                  #######                %#####,
 *  ,######        (##########(                    %#####/                  ######
 *  ######           .%############%              /#####,                   *#####(
 *  #####(                 /#############*                                   ######
 * .#####*                      ,#############(                              ######
 * .#####*                           .#############(.                        ######
 *  #####/                                 (#############*                   %#####
 *  ######                    .###,             *#############/              ######
 *  /#####*                  ,######                  %############%.       ######.
 *   %#####                 (######                   /##########.         *#####(
 *    ######               %#####(                ,##########*            *######
 *     ######.            ######(              ##########(               #######
 *      #######          ######*           (##########                  ######%
 *       *######(      .######         *##########,                   #######.
 *         #######%   *######       %#########/                     ########
 *           ###############    (#########(                      (#######(
 *             *##########( /#########%                       %########.
 *                /################,                     /##########/
 *                #############(                ./%#############%
 *              ,###########         (######################/
 *             *#######%         *%#################%#
 *            (#####*
 *           ###(
 *          #
 */

// clang-format off

#ifndef SR1120_V3_H_
#define SR1120_V3_H_


/******************************************************************************/
/*                  Register HARDDISABLES_IOCONFIG (16 bits)                  */
/******************************************************************************/
#define REG16_HARDDISABLES_IOCONFIG 0x1

          /* [rw] Auto antenna switch
           */
#define   BIT_AUTSWANT  BIT(15)
            /* The automatic antenna diversity switching function of the frame
             * processor is enabled
             */
#define     AUTSWANT_0b1  MOV2MASK(0b1, BIT_AUTSWANT)
            /* The automatic antenna diversity switching function of the frame
             * processor is disabled
             */
#define     AUTSWANT_0b0  MOV2MASK(0b0, BIT_AUTSWANT)
            /* Get AUTSWANT value
             */
#define     GET_AUTSWANT(reg_val)  MASK2VAL(reg_val, BIT_AUTSWANT)
            /* AUTSWANT default value
             */
#define     AUTSWANT_DEFAULT  MOV2MASK(0x0, BIT_AUTSWANT)

          /* [rw] Antenna switch?
           */
#define   BIT_ANT_SW  BIT(14)
            /* Delegated to Gabriel M.
             */
#define     ANT_SW_0b1  MOV2MASK(0b1, BIT_ANT_SW)
            /* Delegated to Gabriel M.
             */
#define     ANT_SW_0b0  MOV2MASK(0b0, BIT_ANT_SW)
            /* Get ANT_SW value
             */
#define     GET_ANT_SW(reg_val)  MASK2VAL(reg_val, BIT_ANT_SW)
            /* ANT_SW default value
             */
#define     ANT_SW_DEFAULT  MOV2MASK(0x0, BIT_ANT_SW)

          /* [rw] Interrupt signal polarity
           */
#define   BIT_IRQPOLAR  BIT(13)
            /* Active high
             */
#define     IRQPOLAR_0b1  MOV2MASK(0b1, BIT_IRQPOLAR)
            /* Active low
             */
#define     IRQPOLAR_0b0  MOV2MASK(0b0, BIT_IRQPOLAR)
            /* Get IRQPOLAR value
             */
#define     GET_IRQPOLAR(reg_val)  MASK2VAL(reg_val, BIT_IRQPOLAR)
            /* IRQPOLAR default value
             */
#define     IRQPOLAR_DEFAULT  MOV2MASK(0x1, BIT_IRQPOLAR)

          /* [rw] Fast Master-In-Slave-Out bits
           */
#define   BIT_FASTMISO  BIT(12)
            /* Faster propagation mode
             */
#define     FASTMISO_0b1  MOV2MASK(0b1, BIT_FASTMISO)
            /* Standard propagation mode
             */
#define     FASTMISO_0b0  MOV2MASK(0b0, BIT_FASTMISO)
            /* Get FASTMISO value
             */
#define     GET_FASTMISO(reg_val)  MASK2VAL(reg_val, BIT_FASTMISO)
            /* FASTMISO default value
             */
#define     FASTMISO_DEFAULT  MOV2MASK(0x0, BIT_FASTMISO)

          /* [rw] SPI slave config
           */
#define   BITS_QSPI  BITS16(11, 10)
            /* QSPI with nRF microcontrollers compatibility
             */
#define     QSPI_0b11  MOV2MASK(0b11, BITS_QSPI)
            /* QSPI with one dummy cycle
             */
#define     QSPI_0b10  MOV2MASK(0b10, BITS_QSPI)
            /* QSPI with no dummy cycles
             */
#define     QSPI_0b01  MOV2MASK(0b01, BITS_QSPI)
            /* SPI Mode
             */
#define     QSPI_0b00  MOV2MASK(0b00, BITS_QSPI)
            /* Get QSPI value
             */
#define     GET_QSPI(reg_val)  MASK2VAL(reg_val, BITS_QSPI)
            /* QSPI default value
             */
#define     QSPI_DEFAULT  MOV2MASK(0x0, BITS_QSPI)

          /* [rw] Digital output driver impedance
           */
#define   BITS_OUTIMPED  BITS16(9, 8)
            /* Delegated to Michiel
             */
#define     OUTIMPED_0b11  MOV2MASK(0b11, BITS_OUTIMPED)
            /* Delegated to Michiel
             */
#define     OUTIMPED_0b10  MOV2MASK(0b10, BITS_OUTIMPED)
            /* Delegated to Michiel
             */
#define     OUTIMPED_0b01  MOV2MASK(0b01, BITS_OUTIMPED)
            /* Delegated to Michiel
             */
#define     OUTIMPED_0b00  MOV2MASK(0b00, BITS_OUTIMPED)
            /* Get OUTIMPED value
             */
#define     GET_OUTIMPED(reg_val)  MASK2VAL(reg_val, BITS_OUTIMPED)
            /* OUTIMPED default value
             */
#define     OUTIMPED_DEFAULT  MOV2MASK(0x0, BITS_OUTIMPED)

          /* [rw] Transceiver chip rate
           */
#define   BITS_CHIP_RATE  BITS16(7, 6)
            /* Invalid configuration
             */
#define     CHIP_RATE_0b11  MOV2MASK(0b11, BITS_CHIP_RATE)
            /* Selects 27.30 MHz
             */
#define     CHIP_RATE_0b10  MOV2MASK(0b10, BITS_CHIP_RATE)
            /* Selects 40.96 MHz
             */
#define     CHIP_RATE_0b01  MOV2MASK(0b01, BITS_CHIP_RATE)
            /* Selects 20.48 MHz
             */
#define     CHIP_RATE_0b00  MOV2MASK(0b00, BITS_CHIP_RATE)
            /* Get CHIP_RATE value
             */
#define     GET_CHIP_RATE(reg_val)  MASK2VAL(reg_val, BITS_CHIP_RATE)
            /* CHIP_RATE default value
             */
#define     CHIP_RATE_DEFAULT  MOV2MASK(0x0, BITS_CHIP_RATE)

          /* [rw] Disable 1V charge retention switch
           */
#define   BIT_V1SWDIS  BIT(5)
            /* The DC/DC voltage converter stays off regardless of the device's
             * power state
             */
#define     V1SWDIS_0b1  MOV2MASK(0b1, BIT_V1SWDIS)
            /* Depending on the device's power state, the DC/DC voltage
             * converter is turned off
             */
#define     V1SWDIS_0b0  MOV2MASK(0b0, BIT_V1SWDIS)
            /* Get V1SWDIS value
             */
#define     GET_V1SWDIS(reg_val)  MASK2VAL(reg_val, BIT_V1SWDIS)
            /* V1SWDIS default value
             */
#define     V1SWDIS_DEFAULT  MOV2MASK(0x0, BIT_V1SWDIS)

          /* [rw] Disable DC/DC voltage converter
           */
#define   BIT_DCDCDIS  BIT(4)
            /* The DC/DC voltage converter stays off regardless of the device's
             * power state
             */
#define     DCDCDIS_0b1  MOV2MASK(0b1, BIT_DCDCDIS)
            /* Depending on the device's power state, the DC/DC voltage
             * converter is turned off
             */
#define     DCDCDIS_0b0  MOV2MASK(0b0, BIT_DCDCDIS)
            /* Get DCDCDIS value
             */
#define     GET_DCDCDIS(reg_val)  MASK2VAL(reg_val, BIT_DCDCDIS)
            /* DCDCDIS default value
             */
#define     DCDCDIS_DEFAULT  MOV2MASK(0x0, BIT_DCDCDIS)

          /* [rw] Transceiver chip clock source
           */
#define   BITS_CHIP_CLK  BITS16(3, 2)
            /* The PLL will be powered up when the device is awake despite not
             * being used to offer the possibility to switch rapidly between the two clock
             * signals without having to wait for the PLL to settle
             */
#define     CHIP_CLK_0b11  MOV2MASK(0b11, BITS_CHIP_CLK)
            /* The input signal through the pin CLK_PLL is used as the chip
             * clock also disable integrated PLL subcircuit
             */
#define     CHIP_CLK_0b10  MOV2MASK(0b10, BITS_CHIP_CLK)
            /* The output driver of the pin CLK_PLL can be enabled to propagate
             * the PLL's output signal outside the package
             */
#define     CHIP_CLK_0b01  MOV2MASK(0b01, BITS_CHIP_CLK)
            /* Pin CLK_PLL is neither used as an input nor an output and remains
             * in a high impedance state
             */
#define     CHIP_CLK_0b00  MOV2MASK(0b00, BITS_CHIP_CLK)
            /* Get CHIP_CLK value
             */
#define     GET_CHIP_CLK(reg_val)  MASK2VAL(reg_val, BITS_CHIP_CLK)
            /* CHIP_CLK default value
             */
#define     CHIP_CLK_DEFAULT  MOV2MASK(0x0, BITS_CHIP_CLK)

          /* [rw] Crystal oscillator slow clock source
           */
#define   BITS_XTAL_CLK  BITS16(1, 0)
            /* The crystal oscillator will be powered up despite not being used
             * to offer the possibility to switch rapidly between the two clock signals without
             * having to wait for the crystal oscillator to settle
             */
#define     XTAL_CLK_0b11  MOV2MASK(0b11, BITS_XTAL_CLK)
            /* The input signal through the pin CLK_XTAL is used as the crystal
             * oscillator clock
             */
#define     XTAL_CLK_0b10  MOV2MASK(0b10, BITS_XTAL_CLK)
            /* Pin CLK_XTAL can be enabled to propagate the crystal oscillator's
             * output signal outside the package
             */
#define     XTAL_CLK_0b01  MOV2MASK(0b01, BITS_XTAL_CLK)
            /* Pin CLK_XTAL is neither used as an input nor an output and
             * remains in a high impedance state
             */
#define     XTAL_CLK_0b00  MOV2MASK(0b00, BITS_XTAL_CLK)
            /* Get XTAL_CLK value
             */
#define     GET_XTAL_CLK(reg_val)  MASK2VAL(reg_val, BITS_XTAL_CLK)
            /* XTAL_CLK default value
             */
#define     XTAL_CLK_DEFAULT  MOV2MASK(0x0, BITS_XTAL_CLK)

#define REG16_HARDDISABLES_IOCONFIG_DEFAULT  0x2000



/******************************************************************************/
/*                      Register V_I_TIME_REFS (16 bits)                      */
/******************************************************************************/
#define REG16_V_I_TIME_REFS 0x2

          /* [rw] Transceiver supply voltage reference tuning
           */
#define   BITS_VREFTUNE  BITS16(15, 12)
            /* Delegated to Michiel
             */
#define     SET_VREFTUNE(value)    MOV2MASK(value, BITS_VREFTUNE)
#define     GET_VREFTUNE(reg_val)  MASK2VAL(reg_val, BITS_VREFTUNE)
            /* VREFTUNE default value
             */
#define     VREFTUNE_DEFAULT  MOV2MASK(0x8, BITS_VREFTUNE)

          /* [rw] Transceiver current reference tuning
           */
#define   BITS_IREFTUNE  BITS16(11, 6)
            /* Delegated to Michiel
             */
#define     SET_IREFTUNE(value)    MOV2MASK(value, BITS_IREFTUNE)
#define     GET_IREFTUNE(reg_val)  MASK2VAL(reg_val, BITS_IREFTUNE)
            /* IREFTUNE default value
             */
#define     IREFTUNE_DEFAULT  MOV2MASK(0x20, BITS_IREFTUNE)

          /* [wo] Transceiver current reference tuning pin enable
           */
#define   BIT_IREFPINE  BIT(5)
            /* Delegated to Michiel
             */
#define     IREFPINE_0b1  MOV2MASK(0b1, BIT_IREFPINE)
            /* Delegated to Michiel
             */
#define     IREFPINE_0b0  MOV2MASK(0b0, BIT_IREFPINE)
            /* IREFPINE default value
             */
#define     IREFPINE_DEFAULT  MOV2MASK(0x0, BIT_IREFPINE)

          /* [rw] Delay line tuning value
           */
#define   BITS_DLTUNING  BITS16(4, 0)
            /* Higher value speeds propagation through delay cells. Each
             * increment reduces adjacent pulse delay within chip clock cycle by approx.
             * [Michiel?] picoseconds
             */
#define     SET_DLTUNING(value)    MOV2MASK(value, BITS_DLTUNING)
#define     GET_DLTUNING(reg_val)  MASK2VAL(reg_val, BITS_DLTUNING)
            /* DLTUNING default value
             */
#define     DLTUNING_DEFAULT  MOV2MASK(0x10, BITS_DLTUNING)

          /* [ro] Delay line end lags clock edge
           */
#define   BIT_DL_LAGS  BIT(5)
            /* Delay longer than chip clock period with current DLTUNING
             */
#define     DL_LAGS_0b1  MOV2MASK(0b1, BIT_DL_LAGS)
            /* Delay shorter or aligned
             */
#define     DL_LAGS_0b0  MOV2MASK(0b0, BIT_DL_LAGS)
            /* Get DL_LAGS value
             */
#define     GET_DL_LAGS(reg_val)  MASK2VAL(reg_val, BIT_DL_LAGS)
            /* DL_LAGS default value
             */
#define     DL_LAGS_DEFAULT  MOV2MASK(0, BIT_DL_LAGS)

#define REG16_V_I_TIME_REFS_DEFAULT  0x8810



/******************************************************************************/
/*                     Register PLLWAIT_PLLRES (16 bits)                      */
/******************************************************************************/
#define REG16_PLLWAIT_PLLRES 0x3

          /* [rw] Unused bits??
           */
#define   BITS_UNUSEDECO  BITS16(15, 11)
            /* Delegated to Michiel
             */
#define     SET_UNUSEDECO(value)    MOV2MASK(value, BITS_UNUSEDECO)
#define     GET_UNUSEDECO(reg_val)  MASK2VAL(reg_val, BITS_UNUSEDECO)
            /* UNUSEDECO default value
             */
#define     UNUSEDECO_DEFAULT  MOV2MASK(0x0, BITS_UNUSEDECO)

          /* [rw] RXBWTUNE??
           */
#define   BITS_RXBWTUNE  BITS16(10, 8)
            /* Delegated to Michiel
             */
#define     SET_RXBWTUNE(value)    MOV2MASK(value, BITS_RXBWTUNE)
#define     GET_RXBWTUNE(reg_val)  MASK2VAL(reg_val, BITS_RXBWTUNE)
            /* RXBWTUNE default value
             */
#define     RXBWTUNE_DEFAULT  MOV2MASK(0x4, BITS_RXBWTUNE)

          /* [rw] PLL Settling Delay
           */
#define   BITS_PLLDELAY  BITS16(7, 3)
            /* Specifies wait time for integrated PLL to lock onto reference
             * clock and stabilize before enabling other subcircuits with faster settling
             * times. Wait time is PLLDELAY*8 crystal oscillator clock cycles.
             */
#define     SET_PLLDELAY(value)    MOV2MASK(value, BITS_PLLDELAY)
#define     GET_PLLDELAY(reg_val)  MASK2VAL(reg_val, BITS_PLLDELAY)
            /* PLLDELAY default value
             */
#define     PLLDELAY_DEFAULT  MOV2MASK(0xb, BITS_PLLDELAY)

          /* [rw] PLL VCO Resistor Tuning
           */
#define   BITS_PLLRESIST  BITS16(2, 0)
            /* Calibrates PLL's VCO resistors digitally to achieve desired
             * frequency across operating conditions. Resistance increases linearly with higher
             * binary values in this field
             */
#define     SET_PLLRESIST(value)    MOV2MASK(value, BITS_PLLRESIST)
#define     GET_PLLRESIST(reg_val)  MASK2VAL(reg_val, BITS_PLLRESIST)
            /* PLLRESIST default value
             */
#define     PLLRESIST_DEFAULT  MOV2MASK(0x4, BITS_PLLRESIST)

#define REG16_PLLWAIT_PLLRES_DEFAULT  0x045C



/******************************************************************************/
/*                      Register CCA_SETTINGS (16 bits)                       */
/******************************************************************************/
#define REG16_CCA_SETTINGS 0x4

          /* [rw] CCA Retry Interval
           */
#define   BITS_CCAINTERV  BITS16(15, 10)
            /* Time between consecutive CCAs after failed tests. Interval =
             * (CCAINTERV+1)*32 chip clock cycles. Irrelevant if MAXRETRY is 0b0000, as no
             * further CCAs occur after failure
             */
#define     SET_CCAINTERV(value)    MOV2MASK(value, BITS_CCAINTERV)
#define     GET_CCAINTERV(reg_val)  MASK2VAL(reg_val, BITS_CCAINTERV)
            /* CCAINTERV default value
             */
#define     CCAINTERV_DEFAULT  MOV2MASK(0x3, BITS_CCAINTERV)

          /* [rw] Ignore Preamble during CCAs
           */
#define   BIT_IGNORPKT  BIT(9)
            /* Disregards preamble detection during CCA; prevents reception of
             * incoming frames
             */
#define     IGNORPKT_0b1  MOV2MASK(0b1, BIT_IGNORPKT)
            /* Interrupts CCA to receive detected frame preamble, resuming
             * transmission attempts afterward
             */
#define     IGNORPKT_0b0  MOV2MASK(0b0, BIT_IGNORPKT)
            /* Get IGNORPKT value
             */
#define     GET_IGNORPKT(reg_val)  MASK2VAL(reg_val, BIT_IGNORPKT)
            /* IGNORPKT default value
             */
#define     IGNORPKT_DEFAULT  MOV2MASK(0x1, BIT_IGNORPKT)

          /* [rw] Immediate Transmission
           */
#define   BIT_TXANYWAY  BIT(8)
            /* Skips last CCA attempt, assuming clear channel, for immediate
             * standalone frame transmission
             */
#define     TXANYWAY_0b1  MOV2MASK(0b1, BIT_TXANYWAY)
            /* Cancels transmission on last CCA fail, sets CCAFAILI interrupt
             */
#define     TXANYWAY_0b0  MOV2MASK(0b0, BIT_TXANYWAY)
            /* Get TXANYWAY value
             */
#define     GET_TXANYWAY(reg_val)  MASK2VAL(reg_val, BIT_TXANYWAY)
            /* TXANYWAY default value
             */
#define     TXANYWAY_DEFAULT  MOV2MASK(0x1, BIT_TXANYWAY)

          /* [rw] Max CCA Retry
           */
#define   BITS_MAXRETRY  BITS16(7, 4)
            /* Number of failed CCAs before final test to proceed with frame
             * transmission. If TXANYWAY is '1', attempts equal MAXRETRY; if '0', attempts =
             * MAXRETRY+1. To disable CCA, set MAXRETRY to 0b0000 and TXANYWAY to '1'
             */
#define     SET_MAXRETRY(value)    MOV2MASK(value, BITS_MAXRETRY)
#define     GET_MAXRETRY(reg_val)  MASK2VAL(reg_val, BITS_MAXRETRY)
            /* MAXRETRY default value
             */
#define     MAXRETRY_DEFAULT  MOV2MASK(0x0, BITS_MAXRETRY)

          /* [rw] CCA Receiver Power Time
           */
#define   BITS_CCAONTIME  BITS16(3, 0)
            /* Registers how long the receiver listens for interference before a
             * frame transmission. Duration = (CCAONTIME+1)*8 chip clock cycles/antenna
             * (doubled with antenna diversity check)
             */
#define     SET_CCAONTIME(value)    MOV2MASK(value, BITS_CCAONTIME)
#define     GET_CCAONTIME(reg_val)  MASK2VAL(reg_val, BITS_CCAONTIME)
            /* CCAONTIME default value
             */
#define     CCAONTIME_DEFAULT  MOV2MASK(0x1, BITS_CCAONTIME)

#define REG16_CCA_SETTINGS_DEFAULT  0x0F01



/******************************************************************************/
/*                     Register CCA_THRES_GAIN (16 bits)                      */
/******************************************************************************/
#define REG16_CCA_THRES_GAIN 0x5

          /* [rw] CCA RF Amplifier Gain
           */
#define   BITS_CCARFGAIN  BITS16(14, 8)
            /* Replaces PKTRFGAIN during CCA. Adjusts RF amplifier gain during
             * CCA, useful for auto-reply frame reception after CCA. Ensures correct RF gain
             * for both functions, especially when time for adjusting gain is limited
             */
#define     SET_CCARFGAIN(value)    MOV2MASK(value, BITS_CCARFGAIN)
#define     GET_CCARFGAIN(reg_val)  MASK2VAL(reg_val, BITS_CCARFGAIN)
            /* CCARFGAIN default value
             */
#define     CCARFGAIN_DEFAULT  MOV2MASK(0x0, BITS_CCARFGAIN)

          /* [rw] CCA Antenna Diversity
           */
#define   BIT_CCASWANT  BIT(7)
            /* CCA on two antennas
             */
#define     CCASWANT_0b1  MOV2MASK(0b1, BIT_CCASWANT)
            /* CCA on one antenna
             */
#define     CCASWANT_0b0  MOV2MASK(0b0, BIT_CCASWANT)
            /* Get CCASWANT value
             */
#define     GET_CCASWANT(reg_val)  MASK2VAL(reg_val, BIT_CCASWANT)
            /* CCASWANT default value
             */
#define     CCASWANT_DEFAULT  MOV2MASK(0x0, BIT_CCASWANT)

          /* [rw] CCA Energy Threshold
           */
#define   BITS_CCATHRES  BITS16(6, 0)
            /* Sets max received signal amplitude indicating channel
             * interference for CCA (clear-channel assessment). Threshold energy within one
             * receiver window determines peak energy threshold. Higher value = lower
             * threshold, higher sensitivity
             */
#define     SET_CCATHRES(value)    MOV2MASK(value, BITS_CCATHRES)
#define     GET_CCATHRES(reg_val)  MASK2VAL(reg_val, BITS_CCATHRES)
            /* CCATHRES default value
             */
#define     CCATHRES_DEFAULT  MOV2MASK(0x50, BITS_CCATHRES)

#define REG16_CCA_THRES_GAIN_DEFAULT  0x0050



/******************************************************************************/
/*                    Register RF_GAIN_MANUGAIN (16 bits)                     */
/******************************************************************************/
#define REG16_RF_GAIN_MANUGAIN 0x6

          /* [rw] Manual Gain Control
           */
#define   BITS_MANUGAIN  BITS16(14, 8)
            /* When set to valid binary gain (0b0000000 to 0b1110011), disables
             * AGC loop and uses this value. Each increment = 0.5 dB gain increase. Out of
             * range values enable AGC loop
             */
#define     SET_MANUGAIN(value)    MOV2MASK(value, BITS_MANUGAIN)
#define     GET_MANUGAIN(reg_val)  MASK2VAL(reg_val, BITS_MANUGAIN)
            /* MANUGAIN default value
             */
#define     MANUGAIN_DEFAULT  MOV2MASK(0x7f, BITS_MANUGAIN)

          /* [rw] Enable Manual Gains
           */
#define   BIT_MANRFTUN  BIT(7)
            /* Enable AGC controls for RF_GAIN
             */
#define     MANRFTUN_0b1  MOV2MASK(0b1, BIT_MANRFTUN)
            /* Overrides AGC controls for RF_GAIN, using PKTRFGAIN and CCARFGAIN
             * values to define it
             */
#define     MANRFTUN_0b0  MOV2MASK(0b0, BIT_MANRFTUN)
            /* Get MANRFTUN value
             */
#define     GET_MANRFTUN(reg_val)  MASK2VAL(reg_val, BIT_MANRFTUN)
            /* MANRFTUN default value
             */
#define     MANRFTUN_DEFAULT  MOV2MASK(0x0, BIT_MANRFTUN)

          /* [rw] RX RF Amplifier Gain
           */
#define   BITS_PKTRFGAIN  BITS16(6, 0)
            /* Controls receiver RF amplifier gain except during CCA [delegated
             * to Michiel]
             */
#define     SET_PKTRFGAIN(value)    MOV2MASK(value, BITS_PKTRFGAIN)
#define     GET_PKTRFGAIN(reg_val)  MASK2VAL(reg_val, BITS_PKTRFGAIN)
            /* PKTRFGAIN default value
             */
#define     PKTRFGAIN_DEFAULT  MOV2MASK(0x0, BITS_PKTRFGAIN)

#define REG16_RF_GAIN_MANUGAIN_DEFAULT  0x7F00



/******************************************************************************/
/*                  Register IF_BASEBAND_GAIN_LNA (16 bits)                   */
/******************************************************************************/
#define REG16_IF_BASEBAND_GAIN_LNA 0x7

          /* [rw] LNA Resonance Frequency
           */
#define   BITS_LNA_FREQ  BITS16(15, 12)
            /* Delegated to Michiel
             */
#define     SET_LNA_FREQ(value)    MOV2MASK(value, BITS_LNA_FREQ)
#define     GET_LNA_FREQ(reg_val)  MASK2VAL(reg_val, BITS_LNA_FREQ)
            /* LNA_FREQ default value
             */
#define     LNA_FREQ_DEFAULT  MOV2MASK(0x8, BITS_LNA_FREQ)

          /* [rw] LNA Bias Tuning
           */
#define   BITS_LNA_BIAS  BITS16(11, 8)
            /* Delegated to Michiel
             */
#define     SET_LNA_BIAS(value)    MOV2MASK(value, BITS_LNA_BIAS)
#define     GET_LNA_BIAS(reg_val)  MASK2VAL(reg_val, BITS_LNA_BIAS)
            /* LNA_BIAS default value
             */
#define     LNA_BIAS_DEFAULT  MOV2MASK(0xc, BITS_LNA_BIAS)

          /* [rw] Distribute IF Gain
           */
#define   BIT_DISTGAIN  BIT(7)
            /* Equalizes gain across first three IF variable gain amplifiers
             */
#define     DISTGAIN_0b1  MOV2MASK(0b1, BIT_DISTGAIN)
            /* Optimizes noise figure by stacking gain in early amplifiers
             */
#define     DISTGAIN_0b0  MOV2MASK(0b0, BIT_DISTGAIN)
            /* Get DISTGAIN value
             */
#define     GET_DISTGAIN(reg_val)  MASK2VAL(reg_val, BIT_DISTGAIN)
            /* DISTGAIN default value
             */
#define     DISTGAIN_DEFAULT  MOV2MASK(0x0, BIT_DISTGAIN)

          /* [rw] VGA gain?
           */
#define   BITS_VGA3GAIN  BITS16(6, 4)
            /* Delegated to Michiel
             */
#define     SET_VGA3GAIN(value)    MOV2MASK(value, BITS_VGA3GAIN)
#define     GET_VGA3GAIN(reg_val)  MASK2VAL(reg_val, BITS_VGA3GAIN)
            /* VGA3GAIN default value
             */
#define     VGA3GAIN_DEFAULT  MOV2MASK(0x4, BITS_VGA3GAIN)

          /* [rw] Integrator Gain
           */
#define   BITS_INTEGGAIN  BITS16(3, 0)
            /* Adjusts receiver integrator gain by modifying input resistor
             * value. Higher value reduces input resistance, boosting gain, which can impact
             * saturation, offsets, and noise
             */
#define     SET_INTEGGAIN(value)    MOV2MASK(value, BITS_INTEGGAIN)
#define     GET_INTEGGAIN(reg_val)  MASK2VAL(reg_val, BITS_INTEGGAIN)
            /* INTEGGAIN default value
             */
#define     INTEGGAIN_DEFAULT  MOV2MASK(0x8, BITS_INTEGGAIN)

#define REG16_IF_BASEBAND_GAIN_LNA_DEFAULT  0x8C48



/******************************************************************************/
/*                   Register RXBANDFRE_CFG1FREQ (16 bits)                    */
/******************************************************************************/
#define REG16_RXBANDFRE_CFG1FREQ 0x8

          /* [rw] Pulse 1 Frequency Configuration
           */
#define   BITS_CFG1FREQ  BITS16(13, 8)
            /* These fields set pulse center frequencies for corresponding
             * 'pulse configurations'. CFG1FREQ also tunes RF demodulator band, defining
             * optimal reception frequency
             */
#define     SET_CFG1FREQ(value)    MOV2MASK(value, BITS_CFG1FREQ)
#define     GET_CFG1FREQ(reg_val)  MASK2VAL(reg_val, BITS_CFG1FREQ)
            /* CFG1FREQ default value
             */
#define     CFG1FREQ_DEFAULT  MOV2MASK(0x20, BITS_CFG1FREQ)

          /* [rw] RX Band Frequency
           */
#define   BITS_RXBANDFRE  BITS16(5, 0)
            /* Delegated to Michiel
             */
#define     SET_RXBANDFRE(value)    MOV2MASK(value, BITS_RXBANDFRE)
#define     GET_RXBANDFRE(reg_val)  MASK2VAL(reg_val, BITS_RXBANDFRE)
            /* RXBANDFRE default value
             */
#define     RXBANDFRE_DEFAULT  MOV2MASK(0x20, BITS_RXBANDFRE)

#define REG16_RXBANDFRE_CFG1FREQ_DEFAULT  0x2020



/******************************************************************************/
/*                    Register CFG2FREQ_CFG3FREQ (16 bits)                    */
/******************************************************************************/
#define REG16_CFG2FREQ_CFG3FREQ 0x9

          /* [rw] Pulse 3 Frequency Configuration
           */
#define   BITS_CFG3FREQ  BITS16(13, 8)
            /* These fields set pulse center frequencies for corresponding
             * 'pulse configurations'
             */
#define     SET_CFG3FREQ(value)    MOV2MASK(value, BITS_CFG3FREQ)
#define     GET_CFG3FREQ(reg_val)  MASK2VAL(reg_val, BITS_CFG3FREQ)
            /* CFG3FREQ default value
             */
#define     CFG3FREQ_DEFAULT  MOV2MASK(0x20, BITS_CFG3FREQ)

          /* [rw] Pulse 2 Frequency Configuration
           */
#define   BITS_CFG2FREQ  BITS16(5, 0)
            /* These fields set pulse center frequencies for corresponding
             * 'pulse configurations'
             */
#define     SET_CFG2FREQ(value)    MOV2MASK(value, BITS_CFG2FREQ)
#define     GET_CFG2FREQ(reg_val)  MASK2VAL(reg_val, BITS_CFG2FREQ)
            /* CFG2FREQ default value
             */
#define     CFG2FREQ_DEFAULT  MOV2MASK(0x20, BITS_CFG2FREQ)

#define REG16_CFG2FREQ_CFG3FREQ_DEFAULT  0x2020



/******************************************************************************/
/*               Register CFG_WIDTHS_TXPWR_RANDPULSE (16 bits)                */
/******************************************************************************/
#define REG16_CFG_WIDTHS_TXPWR_RANDPULSE 0xA

          /* [rw] Random Pulse Polarity
           */
#define   BIT_RANDPULS  BIT(15)
            /* Transmitted pulse oscillations have randomized polarities using
             * an 11-bit LFSR for frequency spectrum uniformity
             */
#define     RANDPULS_0b1  MOV2MASK(0b1, BIT_RANDPULS)
            /* All pulses have the same polarity
             */
#define     RANDPULS_0b0  MOV2MASK(0b0, BIT_RANDPULS)
            /* Get RANDPULS value
             */
#define     GET_RANDPULS(reg_val)  MASK2VAL(reg_val, BIT_RANDPULS)
            /* RANDPULS default value
             */
#define     RANDPULS_DEFAULT  MOV2MASK(0x1, BIT_RANDPULS)

          /* [rw] Transmit Power Control
           */
#define   BITS_TX_POWER  BITS16(14, 12)
            /* Transmit Power Control: This field adjusts transmitter power with
             * 0.6 dB attenuation per 2-bit increment. Default 0b00 gives max power, while 0b11
             * reduces it by 1.8 dB.         [Update: Now three bits, potential meaning
             * change.]
             */
#define     SET_TX_POWER(value)    MOV2MASK(value, BITS_TX_POWER)
#define     GET_TX_POWER(reg_val)  MASK2VAL(reg_val, BITS_TX_POWER)
            /* TX_POWER default value
             */
#define     TX_POWER_DEFAULT  MOV2MASK(0x4, BITS_TX_POWER)

          /* [rw] Pulse 3 Width Configuration
           */
#define   BITS_CFG3WIDTH  BITS16(10, 8)
            /* These fields set pulse durations for corresponding 'pulse
             * configurations'. Also impacts transmission power
             */
#define     SET_CFG3WIDTH(value)    MOV2MASK(value, BITS_CFG3WIDTH)
#define     GET_CFG3WIDTH(reg_val)  MASK2VAL(reg_val, BITS_CFG3WIDTH)
            /* CFG3WIDTH default value
             */
#define     CFG3WIDTH_DEFAULT  MOV2MASK(0x4, BITS_CFG3WIDTH)

          /* [rw] Pulse 2 Width Configuration
           */
#define   BITS_CFG2WIDTH  BITS16(6, 4)
            /* These fields set pulse durations for corresponding 'pulse
             * configurations'. Also impacts transmission power
             */
#define     SET_CFG2WIDTH(value)    MOV2MASK(value, BITS_CFG2WIDTH)
#define     GET_CFG2WIDTH(reg_val)  MASK2VAL(reg_val, BITS_CFG2WIDTH)
            /* CFG2WIDTH default value
             */
#define     CFG2WIDTH_DEFAULT  MOV2MASK(0x4, BITS_CFG2WIDTH)

          /* [rw] Pulse 1 Width Configuration
           */
#define   BITS_CFG1WIDTH  BITS16(2, 0)
            /* These fields set pulse durations for corresponding 'pulse
             * configurations'. Also impacts transmission power
             */
#define     SET_CFG1WIDTH(value)    MOV2MASK(value, BITS_CFG1WIDTH)
#define     GET_CFG1WIDTH(reg_val)  MASK2VAL(reg_val, BITS_CFG1WIDTH)
            /* CFG1WIDTH default value
             */
#define     CFG1WIDTH_DEFAULT  MOV2MASK(0x4, BITS_CFG1WIDTH)

#define REG16_CFG_WIDTHS_TXPWR_RANDPULSE_DEFAULT  0xC444



/******************************************************************************/
/*                      Register TX_PULSE_POS (16 bits)                       */
/******************************************************************************/
#define REG16_TX_PULSE_POS 0xB

          /* [rw] Pulse 9 Position Configuration
           */
#define   BITS_POS9PULSE  BITS16(15, 14)
            /* These register fields each selects one of the three 'sets' of
             * pulse parameter values for their respective transmitted pulse position. A 'set'
             * of pulse parameter values include a 3-bit pulse width code held in one of the
             * register fields from CFG1WIDTH to CFG3WIDTH and a 6-bit pulse frequency code
             * held in one of the register fields from CFG1FREQ to CFG3FREQ
             */
#define     SET_POS9PULSE(value)    MOV2MASK(value, BITS_POS9PULSE)
#define     GET_POS9PULSE(reg_val)  MASK2VAL(reg_val, BITS_POS9PULSE)
            /* POS9PULSE default value
             */
#define     POS9PULSE_DEFAULT  MOV2MASK(0x0, BITS_POS9PULSE)

          /* [rw] Pulse 7 Position Configuration
           */
#define   BITS_POS7PULSE  BITS16(13, 12)
            /* These register fields each selects one of the three 'sets' of
             * pulse parameter values for their respective transmitted pulse position. A 'set'
             * of pulse parameter values include a 3-bit pulse width code held in one of the
             * register fields from CFG1WIDTH to CFG3WIDTH and a 6-bit pulse frequency code
             * held in one of the register fields from CFG1FREQ to CFG3FREQ
             */
#define     SET_POS7PULSE(value)    MOV2MASK(value, BITS_POS7PULSE)
#define     GET_POS7PULSE(reg_val)  MASK2VAL(reg_val, BITS_POS7PULSE)
            /* POS7PULSE default value
             */
#define     POS7PULSE_DEFAULT  MOV2MASK(0x0, BITS_POS7PULSE)

          /* [rw] Pulse 6 Position Configuration
           */
#define   BITS_POS6PULSE  BITS16(11, 10)
            /* These register fields each selects one of the three 'sets' of
             * pulse parameter values for their respective transmitted pulse position. A 'set'
             * of pulse parameter values include a 3-bit pulse width code held in one of the
             * register fields from CFG1WIDTH to CFG3WIDTH and a 6-bit pulse frequency code
             * held in one of the register fields from CFG1FREQ to CFG3FREQ
             */
#define     SET_POS6PULSE(value)    MOV2MASK(value, BITS_POS6PULSE)
#define     GET_POS6PULSE(reg_val)  MASK2VAL(reg_val, BITS_POS6PULSE)
            /* POS6PULSE default value
             */
#define     POS6PULSE_DEFAULT  MOV2MASK(0x0, BITS_POS6PULSE)

          /* [rw] Pulse 5 Position Configuration
           */
#define   BITS_POS5PULSE  BITS16(9, 8)
            /* These register fields each selects one of the three 'sets' of
             * pulse parameter values for their respective transmitted pulse position. A 'set'
             * of pulse parameter values include a 3-bit pulse width code held in one of the
             * register fields from CFG1WIDTH to CFG3WIDTH and a 6-bit pulse frequency code
             * held in one of the register fields from CFG1FREQ to CFG3FREQ
             */
#define     SET_POS5PULSE(value)    MOV2MASK(value, BITS_POS5PULSE)
#define     GET_POS5PULSE(reg_val)  MASK2VAL(reg_val, BITS_POS5PULSE)
            /* POS5PULSE default value
             */
#define     POS5PULSE_DEFAULT  MOV2MASK(0x0, BITS_POS5PULSE)

          /* [rw] Pulse 4 Position Configuration
           */
#define   BITS_POS4PULSE  BITS16(7, 6)
            /* These register fields each selects one of the three 'sets' of
             * pulse parameter values for their respective transmitted pulse position. A 'set'
             * of pulse parameter values include a 3-bit pulse width code held in one of the
             * register fields from CFG1WIDTH to CFG3WIDTH and a 6-bit pulse frequency code
             * held in one of the register fields from CFG1FREQ to CFG3FREQ
             */
#define     SET_POS4PULSE(value)    MOV2MASK(value, BITS_POS4PULSE)
#define     GET_POS4PULSE(reg_val)  MASK2VAL(reg_val, BITS_POS4PULSE)
            /* POS4PULSE default value
             */
#define     POS4PULSE_DEFAULT  MOV2MASK(0x1, BITS_POS4PULSE)

          /* [rw] Pulse 3 Position Configuration
           */
#define   BITS_POS3PULSE  BITS16(5, 4)
            /* These register fields each selects one of the three 'sets' of
             * pulse parameter values for their respective transmitted pulse position. A 'set'
             * of pulse parameter values include a 3-bit pulse width code held in one of the
             * register fields from CFG1WIDTH to CFG3WIDTH and a 6-bit pulse frequency code
             * held in one of the register fields from CFG1FREQ to CFG3FREQ
             */
#define     SET_POS3PULSE(value)    MOV2MASK(value, BITS_POS3PULSE)
#define     GET_POS3PULSE(reg_val)  MASK2VAL(reg_val, BITS_POS3PULSE)
            /* POS3PULSE default value
             */
#define     POS3PULSE_DEFAULT  MOV2MASK(0x0, BITS_POS3PULSE)

          /* [rw] Pulse 2 Position Configuration
           */
#define   BITS_POS2PULSE  BITS16(3, 2)
            /* These register fields each selects one of the three 'sets' of
             * pulse parameter values for their respective transmitted pulse position. A 'set'
             * of pulse parameter values include a 3-bit pulse width code held in one of the
             * register fields from CFG1WIDTH to CFG3WIDTH and a 6-bit pulse frequency code
             * held in one of the register fields from CFG1FREQ to CFG3FREQ
             */
#define     SET_POS2PULSE(value)    MOV2MASK(value, BITS_POS2PULSE)
#define     GET_POS2PULSE(reg_val)  MASK2VAL(reg_val, BITS_POS2PULSE)
            /* POS2PULSE default value
             */
#define     POS2PULSE_DEFAULT  MOV2MASK(0x1, BITS_POS2PULSE)

          /* [rw] Pulse 1 Position Configuration
           */
#define   BITS_POS1PULSE  BITS16(1, 0)
            /* These register fields each selects one of the three 'sets' of
             * pulse parameter values for their respective transmitted pulse position. A 'set'
             * of pulse parameter values include a 3-bit pulse width code held in one of the
             * register fields from CFG1WIDTH to CFG3WIDTH and a 6-bit pulse frequency code
             * held in one of the register fields from CFG1FREQ to CFG3FREQ
             */
#define     SET_POS1PULSE(value)    MOV2MASK(value, BITS_POS1PULSE)
#define     GET_POS1PULSE(reg_val)  MASK2VAL(reg_val, BITS_POS1PULSE)
            /* POS1PULSE default value
             */
#define     POS1PULSE_DEFAULT  MOV2MASK(0x0, BITS_POS1PULSE)

#define REG16_TX_PULSE_POS_DEFAULT  0x0044



/******************************************************************************/
/*                     Register SLPPERIOD_15_0 (16 bits)                      */
/******************************************************************************/
#define REG16_SLPPERIOD_15_0 0xC

          /* [rw] Sleep Cycle Duration [15:0]
           */
#define   BITS_SLPPERIOD_15_0  BITS16(15, 0)
            /* This register sets the power cycle duration in clock cycles. The
             * wake-up timer's count resets at this value
             */
#define     SET_SLPPERIOD_15_0(value)    MOV2MASK(value, BITS_SLPPERIOD_15_0)
#define     GET_SLPPERIOD_15_0(reg_val)  MASK2VAL(reg_val, BITS_SLPPERIOD_15_0)
            /* SLPPERIOD_15_0 default value
             */
#define     SLPPERIOD_15_0_DEFAULT  MOV2MASK(0x7ff, BITS_SLPPERIOD_15_0)

#define REG16_SLPPERIOD_15_0_DEFAULT  0x07FF



/******************************************************************************/
/*                   Register SLPPERIOD_PWRUPDLAY (16 bits)                   */
/******************************************************************************/
#define REG16_SLPPERIOD_PWRUPDLAY 0xD

          /* [rw] Transceiver Power-Up Delay
           */
#define   BITS_PWRUPDLAY  BITS16(15, 8)
            /* This field delays transmitter/receiver power-up after sleep.
             * Delay is the field's value multiplied by 8 chip clock periods
             */
#define     SET_PWRUPDLAY(value)    MOV2MASK(value, BITS_PWRUPDLAY)
#define     GET_PWRUPDLAY(reg_val)  MASK2VAL(reg_val, BITS_PWRUPDLAY)
            /* PWRUPDLAY default value
             */
#define     PWRUPDLAY_DEFAULT  MOV2MASK(0x0, BITS_PWRUPDLAY)

          /* [rw] Sleep Cycle Duration [23:16]
           */
#define   BITS_SLPPERIOD_23_16  BITS16(7, 0)
            /* This register sets the power cycle duration in clock cycles. The
             * wake-up timer's count resets at this value
             */
#define     SET_SLPPERIOD_23_16(value)    MOV2MASK(value, BITS_SLPPERIOD_23_16)
#define     GET_SLPPERIOD_23_16(reg_val)  MASK2VAL(reg_val, BITS_SLPPERIOD_23_16)
            /* SLPPERIOD_23_16 default value
             */
#define     SLPPERIOD_23_16_DEFAULT  MOV2MASK(0x0, BITS_SLPPERIOD_23_16)

#define REG16_SLPPERIOD_PWRUPDLAY_DEFAULT  0x0000



/******************************************************************************/
/*                   Register TIMELIMIT_BIASDELAY (16 bits)                   */
/******************************************************************************/
#define REG16_TIMELIMIT_BIASDELAY 0xE

          /* [rw] Receiver Timeout
           */
#define   BITS_TIMEOUT  BITS16(15, 3)
            /* Defines chip clock cycles for wake-up timer to trigger timeout
             * event. If set while receiver is idle and awake, it cancels scheduled standalone
             * frame transmission. If auto-reply is enabled, it initiates a
             * 'transmission-on-timeout.'
             */
#define     SET_TIMEOUT(value)    MOV2MASK(value, BITS_TIMEOUT)
#define     GET_TIMEOUT(reg_val)  MASK2VAL(reg_val, BITS_TIMEOUT)
            /* TIMEOUT default value
             */
#define     TIMEOUT_DEFAULT  MOV2MASK(0x0, BITS_TIMEOUT)

          /* [rw] Alternate Auto-Reply Timeout
           */
#define   BIT_ALTRPLTO  BIT(2)
            /* Overrides the auto-reply timeout. Uses dedicated timer defined by
             * RXTIMEOUT
             */
#define     ALTRPLTO_0b1  MOV2MASK(0b1, BIT_ALTRPLTO)
            /* Hardcoded auto-reply timeout is used
             */
#define     ALTRPLTO_0b0  MOV2MASK(0b0, BIT_ALTRPLTO)
            /* Get ALTRPLTO value
             */
#define     GET_ALTRPLTO(reg_val)  MASK2VAL(reg_val, BIT_ALTRPLTO)
            /* ALTRPLTO default value
             */
#define     ALTRPLTO_DEFAULT  MOV2MASK(0x0, BIT_ALTRPLTO)

          /* [rw] Bias Settling Delay
           */
#define   BITS_BIASDELAY  BITS16(1, 0)
            /* This field defines the wait time after powering up the
             * transmitter or receiver
             */
#define     SET_BIASDELAY(value)    MOV2MASK(value, BITS_BIASDELAY)
#define     GET_BIASDELAY(reg_val)  MASK2VAL(reg_val, BITS_BIASDELAY)
            /* BIASDELAY default value
             */
#define     BIASDELAY_DEFAULT  MOV2MASK(0x0, BITS_BIASDELAY)

#define REG16_TIMELIMIT_BIASDELAY_DEFAULT  0x0000



/******************************************************************************/
/*                    Register TIMERCFG_SLEEPCFG (16 bits)                    */
/******************************************************************************/
#define REG16_TIMERCFG_SLEEPCFG 0xF

          /* [rw] Sleep Depth
           */
#define   BITS_SLPDEPTH_WAKEONCE  BITS16(15, 14)
            /* Deep sleep
             */
#define     SLPDEPTH_WAKEONCE_0b11  MOV2MASK(0b11, BITS_SLPDEPTH_WAKEONCE)
            /* Shallow sleep
             */
#define     SLPDEPTH_WAKEONCE_0b10  MOV2MASK(0b10, BITS_SLPDEPTH_WAKEONCE)
            /* Lightest sleep with wakeonce
             */
#define     SLPDEPTH_WAKEONCE_0b01  MOV2MASK(0b01, BITS_SLPDEPTH_WAKEONCE)
            /* Lightest sleep without wakeonce
             */
#define     SLPDEPTH_WAKEONCE_0b00  MOV2MASK(0b00, BITS_SLPDEPTH_WAKEONCE)
            /* Get SLPDEPTH_WAKEONCE value
             */
#define     GET_SLPDEPTH_WAKEONCE(reg_val)  MASK2VAL(reg_val, BITS_SLPDEPTH_WAKEONCE)
            /* SLPDEPTH_WAKEONCE default value
             */
#define     SLPDEPTH_WAKEONCE_DEFAULT  MOV2MASK(0x3, BITS_SLPDEPTH_WAKEONCE)

          /* [rw] Sleep on Frame Transmission End
           */
#define   BIT_SLPTXEND  BIT(13)
            /* Frame transmission end auto-sets SLEEP bit
             */
#define     SLPTXEND_0b1  MOV2MASK(0b1, BIT_SLPTXEND)
            /* Disable timers sync for this condition
             */
#define     SLPTXEND_0b0  MOV2MASK(0b0, BIT_SLPTXEND)
            /* Get SLPTXEND value
             */
#define     GET_SLPTXEND(reg_val)  MASK2VAL(reg_val, BIT_SLPTXEND)
            /* SLPTXEND default value
             */
#define     SLPTXEND_DEFAULT  MOV2MASK(0x0, BIT_SLPTXEND)

          /* [rw] Sleep on CCA Failure
           */
#define   BIT_SLPCCAFA  BIT(12)
            /* CCA failure causes auto-setting of SLEEP bit
             */
#define     SLPCCAFA_0b1  MOV2MASK(0b1, BIT_SLPCCAFA)
            /* Disable timers sync for this condition
             */
#define     SLPCCAFA_0b0  MOV2MASK(0b0, BIT_SLPCCAFA)
            /* Get SLPCCAFA value
             */
#define     GET_SLPCCAFA(reg_val)  MASK2VAL(reg_val, BIT_SLPCCAFA)
            /* SLPCCAFA default value
             */
#define     SLPCCAFA_DEFAULT  MOV2MASK(0x0, BIT_SLPCCAFA)

          /* [rw] Sleep on Frame Reception End
           */
#define   BITS_SLPRXEND  BITS16(11, 9)
            /* Conditions for auto-setting SLEEP bit at end of frame reception.
             * Similar conditions as SYNRXEND apply
             */
#define     SLPRXEND_0b1  MOV2MASK(0b1, BITS_SLPRXEND)
            /* Disable timers sync for this condition
             */
#define     SLPRXEND_0b0  MOV2MASK(0b0, BITS_SLPRXEND)
            /* Get SLPRXEND value
             */
#define     GET_SLPRXEND(reg_val)  MASK2VAL(reg_val, BITS_SLPRXEND)
            /* SLPRXEND default value
             */
#define     SLPRXEND_DEFAULT  MOV2MASK(0x0, BITS_SLPRXEND)

          /* [rw] Sleep on Receiver Timeout
           */
#define   BIT_SLPTIMEO  BIT(8)
            /* Receiver timeouts automatically set register bit SLEEP
             */
#define     SLPTIMEO_0b1  MOV2MASK(0b1, BIT_SLPTIMEO)
            /* Disable timers sync for this condition
             */
#define     SLPTIMEO_0b0  MOV2MASK(0b0, BIT_SLPTIMEO)
            /* Get SLPTIMEO value
             */
#define     GET_SLPTIMEO(reg_val)  MASK2VAL(reg_val, BIT_SLPTIMEO)
            /* SLPTIMEO default value
             */
#define     SLPTIMEO_DEFAULT  MOV2MASK(0x0, BIT_SLPTIMEO)

          /* [rw] Auto Wake-up Enable
           */
#define   BIT_AUTOWAKE  BIT(7)
            /* Device wakes automatically based on wake-up timer. Transition
             * from '1' to '0' wakes the device immediately
             */
#define     AUTOWAKE_0b1  MOV2MASK(0b1, BIT_AUTOWAKE)
            /* Disable timers sync for this condition
             */
#define     AUTOWAKE_0b0  MOV2MASK(0b0, BIT_AUTOWAKE)
            /* Get AUTOWAKE value
             */
#define     GET_AUTOWAKE(reg_val)  MASK2VAL(reg_val, BIT_AUTOWAKE)
            /* AUTOWAKE default value
             */
#define     AUTOWAKE_DEFAULT  MOV2MASK(0x0, BIT_AUTOWAKE)

          /* [rw] Sync Timers on Device Wake-up
           */
#define   BIT_SYNWAKUP  BIT(6)
            /* Both timers sync upon waking from sleep, aiding sleep level
             * changes
             */
#define     SYNWAKUP_0b1  MOV2MASK(0b1, BIT_SYNWAKUP)
            /* Disable timers sync for this condition
             */
#define     SYNWAKUP_0b0  MOV2MASK(0b0, BIT_SYNWAKUP)
            /* Get SYNWAKUP value
             */
#define     GET_SYNWAKUP(reg_val)  MASK2VAL(reg_val, BIT_SYNWAKUP)
            /* SYNWAKUP default value
             */
#define     SYNWAKUP_DEFAULT  MOV2MASK(0x0, BIT_SYNWAKUP)

          /* [rw] Sync Timers on Frame Transmission End
           */
#define   BIT_SYNTXEND  BIT(5)
            /* Both timers sync at the end of standalone frame transmission
             */
#define     SYNTXEND_0b1  MOV2MASK(0b1, BIT_SYNTXEND)
            /* Disable timers sync for this condition
             */
#define     SYNTXEND_0b0  MOV2MASK(0b0, BIT_SYNTXEND)
            /* Get SYNTXEND value
             */
#define     GET_SYNTXEND(reg_val)  MASK2VAL(reg_val, BIT_SYNTXEND)
            /* SYNTXEND default value
             */
#define     SYNTXEND_DEFAULT  MOV2MASK(0x0, BIT_SYNTXEND)

          /* [rw] Sync Timers on Frame Reception Start
           */
#define   BIT_SYNRXSTA  BIT(4)
            /* Timers sync at start of standalone frame reception, even if due
             * to error
             */
#define     SYNRXSTA_0b1  MOV2MASK(0b1, BIT_SYNRXSTA)
            /* Disable timers sync for this condition
             */
#define     SYNRXSTA_0b0  MOV2MASK(0b0, BIT_SYNRXSTA)
            /* Get SYNRXSTA value
             */
#define     GET_SYNRXSTA(reg_val)  MASK2VAL(reg_val, BIT_SYNRXSTA)
            /* SYNRXSTA default value
             */
#define     SYNRXSTA_DEFAULT  MOV2MASK(0x0, BIT_SYNRXSTA)

          /* [rw] Sync Timers on Frame Reception End
           */
#define   BITS_SYNRXEND  BITS16(3, 1)
            /* Trigger an automatic timer synchronization at the end of its
             * reception when the adderss is matched
             */
#define     SYNRXEND_0b100  MOV2MASK(0b100, BITS_SYNRXEND)
            /* Trigger an automatic timer synchronization at the end of its
             * reception when the adderss is broadcast
             */
#define     SYNRXEND_0b010  MOV2MASK(0b010, BITS_SYNRXEND)
            /* Trigger an automatic timer synchronization at the end of its
             * reception when CRC is passed or no CRC
             */
#define     SYNRXEND_0b001  MOV2MASK(0b001, BITS_SYNRXEND)
            /* Disable timers sync for this condition
             */
#define     SYNRXEND_0b000  MOV2MASK(0b000, BITS_SYNRXEND)
            /* Get SYNRXEND value
             */
#define     GET_SYNRXEND(reg_val)  MASK2VAL(reg_val, BITS_SYNRXEND)
            /* SYNRXEND default value
             */
#define     SYNRXEND_DEFAULT  MOV2MASK(0x0, BITS_SYNRXEND)

          /* [rw] Sync Timers on Receiver Timeout
           */
#define   BIT_SYNTIMEO  BIT(0)
            /* Both wake-up timers sync upon receiver timeout
             */
#define     SYNTIMEO_0b1  MOV2MASK(0b1, BIT_SYNTIMEO)
            /* Disable timers sync for this condition
             */
#define     SYNTIMEO_0b0  MOV2MASK(0b0, BIT_SYNTIMEO)
            /* Get SYNTIMEO value
             */
#define     GET_SYNTIMEO(reg_val)  MASK2VAL(reg_val, BIT_SYNTIMEO)
            /* SYNTIMEO default value
             */
#define     SYNTIMEO_DEFAULT  MOV2MASK(0x0, BIT_SYNTIMEO)

#define REG16_TIMERCFG_SLEEPCFG_DEFAULT  0xC000



/******************************************************************************/
/*                           Register IRQ (16 bits)                           */
/******************************************************************************/
#define REG16_IRQ 0x10

          /* [wo] Reception data buffer load interrupt threshold
           */
#define   BITS_BUFTHRES  BITS16(15, 11)
            /* Reception data buffer load interrupt threshold: Write-only, sets
             * buffer load for BUFLOADI trigger (0b00000 disables)
             */
#define     SET_BUFTHRES(value)  MOV2MASK(value, BITS_BUFTHRES)
            /* BUFTHRES default value
             */
#define     BUFTHRES_DEFAULT  MOV2MASK(0x0, BITS_BUFTHRES)

          /* [wo] Clear-channel assessment failure interrupt enable
           */
#define   BIT_CCAFAILE  BIT(10)
            /* Controls IRQ signal based on CCAFAILI, no effect on CCAFAILI
             */
#define     CCAFAILE_0b1  MOV2MASK(0b1, BIT_CCAFAILE)
            /* Disable this interrupt
             */
#define     CCAFAILE_0b0  MOV2MASK(0b0, BIT_CCAFAILE)
            /* CCAFAILE default value
             */
#define     CCAFAILE_DEFAULT  MOV2MASK(0x1, BIT_CCAFAILE)

          /* [wo] Crystal oscillator timer interrupt enable
           */
#define   BIT_XOTIMERE  BIT(9)
            /* Controls IRQ signal based on XOTIMERI, no effect on XOTIMERI
             */
#define     XOTIMERE_0b1  MOV2MASK(0b1, BIT_XOTIMERE)
            /* Disable this interrupt
             */
#define     XOTIMERE_0b0  MOV2MASK(0b0, BIT_XOTIMERE)
            /* XOTIMERE default value
             */
#define     XOTIMERE_DEFAULT  MOV2MASK(0x0, BIT_XOTIMERE)

          /* [wo] Wake-up interrupt enable
           */
#define   BIT_WAKEUPE  BIT(8)
            /* Controls IRQ signal based on WAKEUPI, no effect on WAKEUPI
             */
#define     WAKEUPE_0b1  MOV2MASK(0b1, BIT_WAKEUPE)
            /* Disable this interrupt
             */
#define     WAKEUPE_0b0  MOV2MASK(0b0, BIT_WAKEUPE)
            /* WAKEUPE default value
             */
#define     WAKEUPE_DEFAULT  MOV2MASK(0x0, BIT_WAKEUPE)

          /* [wo] Auto-reply frame transmission end interrupt enable
           */
#define   BIT_ARTXENDE  BIT(7)
            /* Controls IRQ signal based on ARTXENDI, no effect on ARTXENDI
             */
#define     ARTXENDE_0b1  MOV2MASK(0b1, BIT_ARTXENDE)
            /* Disable this interrupt
             */
#define     ARTXENDE_0b0  MOV2MASK(0b0, BIT_ARTXENDE)
            /* ARTXENDE default value
             */
#define     ARTXENDE_DEFAULT  MOV2MASK(0x1, BIT_ARTXENDE)

          /* [wo] Auto-reply frame reception end interrupt enable
           */
#define   BIT_ARRXENDE  BIT(6)
            /* Controls IRQ signal based on ARRXENDI, no effect on ARRXENDI
             */
#define     ARRXENDE_0b1  MOV2MASK(0b1, BIT_ARRXENDE)
            /* Disable this interrupt
             */
#define     ARRXENDE_0b0  MOV2MASK(0b0, BIT_ARRXENDE)
            /* ARRXENDE default value
             */
#define     ARRXENDE_DEFAULT  MOV2MASK(0x1, BIT_ARRXENDE)

          /* [wo] Standalone frame transmission end interrupt enable
           */
#define   BIT_TXENDE  BIT(5)
            /* Controls IRQ signal based on TXENDI, no effect on TXENDI
             */
#define     TXENDE_0b1  MOV2MASK(0b1, BIT_TXENDE)
            /* Disable this interrupt
             */
#define     TXENDE_0b0  MOV2MASK(0b0, BIT_TXENDE)
            /* TXENDE default value
             */
#define     TXENDE_DEFAULT  MOV2MASK(0x1, BIT_TXENDE)

          /* [wo] Standalone frame reception end interrupt enable
           */
#define   BIT_RXENDE  BIT(4)
            /* Controls IRQ signal based on RXENDI, no effect on RXENDI
             */
#define     RXENDE_0b1  MOV2MASK(0b1, BIT_RXENDE)
            /* Disable this interrupt
             */
#define     RXENDE_0b0  MOV2MASK(0b0, BIT_RXENDE)
            /* RXENDE default value
             */
#define     RXENDE_DEFAULT  MOV2MASK(0x1, BIT_RXENDE)

          /* [wo] Frame address field match interrupt enable
           */
#define   BIT_ADDRMATE  BIT(3)
            /* Controls IRQ signal based on ADDRMATI, no effect on ADDRMATI
             */
#define     ADDRMATE_0b1  MOV2MASK(0b1, BIT_ADDRMATE)
            /* Disable this interrupt
             */
#define     ADDRMATE_0b0  MOV2MASK(0b0, BIT_ADDRMATE)
            /* ADDRMATE default value
             */
#define     ADDRMATE_DEFAULT  MOV2MASK(0x0, BIT_ADDRMATE)

          /* [wo] Broadcast frame reception end interrupt enable
           */
#define   BIT_BRDCASTE  BIT(2)
            /* Controls IRQ signal based on BRDCASTI, no effect on BRDCASTI
             */
#define     BRDCASTE_0b1  MOV2MASK(0b1, BIT_BRDCASTE)
            /* Disable this interrupt
             */
#define     BRDCASTE_0b0  MOV2MASK(0b0, BIT_BRDCASTE)
            /* BRDCASTE default value
             */
#define     BRDCASTE_DEFAULT  MOV2MASK(0x0, BIT_BRDCASTE)

          /* [wo] CRC pass interrupt enable
           */
#define   BIT_CRCPASSE  BIT(1)
            /* Controls IRQ signal based on CRCPASSI, no effect on CRCPASSI
             */
#define     CRCPASSE_0b1  MOV2MASK(0b1, BIT_CRCPASSE)
            /* Disable this interrupt
             */
#define     CRCPASSE_0b0  MOV2MASK(0b0, BIT_CRCPASSE)
            /* CRCPASSE default value
             */
#define     CRCPASSE_DEFAULT  MOV2MASK(0x0, BIT_CRCPASSE)

          /* [wo] Receiver timeout interrupt enable
           */
#define   BIT_TIMEOUTE  BIT(0)
            /* Enable receiver timeout interrupt
             */
#define     TIMEOUTE_0b1  MOV2MASK(0b1, BIT_TIMEOUTE)
            /* Disable this interrupt
             */
#define     TIMEOUTE_0b0  MOV2MASK(0b0, BIT_TIMEOUTE)
            /* TIMEOUTE default value
             */
#define     TIMEOUTE_DEFAULT  MOV2MASK(0x1, BIT_TIMEOUTE)

          /* [ro] Transmission data buffer overflow interrupt
           */
#define   BIT_TXOVRFLI  BIT(15)
            /* Set to '1' when SPI writes full transmission buffer
             */
#define     TXOVRFLI_0b1  MOV2MASK(0b1, BIT_TXOVRFLI)
            /* Cleared by FLUSHTX
             */
#define     TXOVRFLI_0b0  MOV2MASK(0b0, BIT_TXOVRFLI)
            /* Get TXOVRFLI value
             */
#define     GET_TXOVRFLI(reg_val)  MASK2VAL(reg_val, BIT_TXOVRFLI)
            /* TXOVRFLI default value
             */
#define     TXOVRFLI_DEFAULT  MOV2MASK(0, BIT_TXOVRFLI)

          /* [ro] Transmission data buffer underflow interrupt
           */
#define   BIT_TXUDRFLI  BIT(14)
            /* Set to '1' when incomplete frame due to buffer underflow
             */
#define     TXUDRFLI_0b1  MOV2MASK(0b1, BIT_TXUDRFLI)
            /* Cleared by FLUSHTX
             */
#define     TXUDRFLI_0b0  MOV2MASK(0b0, BIT_TXUDRFLI)
            /* Get TXUDRFLI value
             */
#define     GET_TXUDRFLI(reg_val)  MASK2VAL(reg_val, BIT_TXUDRFLI)
            /* TXUDRFLI default value
             */
#define     TXUDRFLI_DEFAULT  MOV2MASK(0, BIT_TXUDRFLI)

          /* [ro] Reception data buffer overflow interrupt
           */
#define   BIT_RXOVRFLI  BIT(13)
            /* Set to '1' when frame processor overflows reception buffer
             */
#define     RXOVRFLI_0b1  MOV2MASK(0b1, BIT_RXOVRFLI)
            /* Cleared by FLUSHRX
             */
#define     RXOVRFLI_0b0  MOV2MASK(0b0, BIT_RXOVRFLI)
            /* Get RXOVRFLI value
             */
#define     GET_RXOVRFLI(reg_val)  MASK2VAL(reg_val, BIT_RXOVRFLI)
            /* RXOVRFLI default value
             */
#define     RXOVRFLI_DEFAULT  MOV2MASK(0, BIT_RXOVRFLI)

          /* [ro] Reception data buffer underflow interrupt
           */
#define   BIT_RXUDRFLI  BIT(12)
            /* Set to '1' when SPI reads empty reception buffer
             */
#define     RXUDRFLI_0b1  MOV2MASK(0b1, BIT_RXUDRFLI)
            /* Cleared by FLUSHRX
             */
#define     RXUDRFLI_0b0  MOV2MASK(0b0, BIT_RXUDRFLI)
            /* Get RXUDRFLI value
             */
#define     GET_RXUDRFLI(reg_val)  MASK2VAL(reg_val, BIT_RXUDRFLI)
            /* RXUDRFLI default value
             */
#define     RXUDRFLI_DEFAULT  MOV2MASK(0, BIT_RXUDRFLI)

          /* [ro] Reception data buffer load interrupt
           */
#define   BIT_BUFLOADI  BIT(11)
            /* Set to '1' when reception data buffer load exceeds BUFTHRES
             */
#define     BUFLOADI_0b1  MOV2MASK(0b1, BIT_BUFLOADI)
            /* Cleared by buffer load decrease or disabling
             */
#define     BUFLOADI_0b0  MOV2MASK(0b0, BIT_BUFLOADI)
            /* Get BUFLOADI value
             */
#define     GET_BUFLOADI(reg_val)  MASK2VAL(reg_val, BIT_BUFLOADI)
            /* BUFLOADI default value
             */
#define     BUFLOADI_DEFAULT  MOV2MASK(0, BIT_BUFLOADI)

          /* [ro] Clear-channel assessment failure interrupt
           */
#define   BIT_CCAFAILI  BIT(10)
            /* This is a sticky bit. Set to '1' when CCA failure cancels
             * standalone frame transmission
             */
#define     GET_CCAFAILI(reg_val)  MASK2VAL(reg_val, BIT_CCAFAILI)
            /* CCAFAILI default value
             */
#define     CCAFAILI_DEFAULT  MOV2MASK(0, BIT_CCAFAILI)

          /* [ro] Crystal oscillator timer interrupt
           */
#define   BIT_XOTIMERI  BIT(9)
            /* This is a sticky bit. Set to '1' when crystal oscillator wake-up
             * timer matches SLPPERIOD during sleep (except lightest sleep)
             */
#define     GET_XOTIMERI(reg_val)  MASK2VAL(reg_val, BIT_XOTIMERI)
            /* XOTIMERI default value
             */
#define     XOTIMERI_DEFAULT  MOV2MASK(0, BIT_XOTIMERI)

          /* [ro] Wake-up interrupt
           */
#define   BIT_WAKEUPI  BIT(8)
            /* This is a sticky bit. Set to '1' when device finishes waking from
             * sleep
             */
#define     GET_WAKEUPI(reg_val)  MASK2VAL(reg_val, BIT_WAKEUPI)
            /* WAKEUPI default value
             */
#define     WAKEUPI_DEFAULT  MOV2MASK(0, BIT_WAKEUPI)

          /* [ro] Auto-reply frame transmission end interrupt
           */
#define   BIT_ARTXENDI  BIT(7)
            /* This is a sticky bit. Set to '1' when auto-reply frame
             * transmission completes
             */
#define     GET_ARTXENDI(reg_val)  MASK2VAL(reg_val, BIT_ARTXENDI)
            /* ARTXENDI default value
             */
#define     ARTXENDI_DEFAULT  MOV2MASK(0, BIT_ARTXENDI)

          /* [ro] Auto-reply frame reception end interrupt
           */
#define   BIT_ARRXENDI  BIT(6)
            /* This is a sticky bit. Set to '1' when auto-reply frame reception
             * completes
             */
#define     GET_ARRXENDI(reg_val)  MASK2VAL(reg_val, BIT_ARRXENDI)
            /* ARRXENDI default value
             */
#define     ARRXENDI_DEFAULT  MOV2MASK(0, BIT_ARRXENDI)

          /* [ro] Standalone frame transmission end interrupt
           */
#define   BIT_TXENDI  BIT(5)
            /* This is a sticky bit. Set to '1' when standalone frame
             * transmission completes
             */
#define     GET_TXENDI(reg_val)  MASK2VAL(reg_val, BIT_TXENDI)
            /* TXENDI default value
             */
#define     TXENDI_DEFAULT  MOV2MASK(0, BIT_TXENDI)

          /* [ro] Standalone frame reception end interrupt
           */
#define   BIT_RXENDI  BIT(4)
            /* This is a sticky bit. Set to '1' when standalone frame reception
             * completes
             */
#define     GET_RXENDI(reg_val)  MASK2VAL(reg_val, BIT_RXENDI)
            /* RXENDI default value
             */
#define     RXENDI_DEFAULT  MOV2MASK(0, BIT_RXENDI)

          /* [ro] Frame address field match interrupt
           */
#define   BIT_ADDRMATI  BIT(3)
            /* This is a sticky bit. Set to '1' when received address matches
             * device's RXADDRESS
             */
#define     GET_ADDRMATI(reg_val)  MASK2VAL(reg_val, BIT_ADDRMATI)
            /* ADDRMATI default value
             */
#define     ADDRMATI_DEFAULT  MOV2MASK(0, BIT_ADDRMATI)

          /* [ro] Broadcast frame reception end interrupt
           */
#define   BIT_BRDCASTI  BIT(2)
            /* This is a sticky bit. Set to '1' when broadcast address matches
             * in frame
             */
#define     GET_BRDCASTI(reg_val)  MASK2VAL(reg_val, BIT_BRDCASTI)
            /* BRDCASTI default value
             */
#define     BRDCASTI_DEFAULT  MOV2MASK(0, BIT_BRDCASTI)

          /* [ro] CRC pass interrupt
           */
#define   BIT_CRCPASSI  BIT(1)
            /* This is a sticky bit. Set to '1' when received CRC matches
             * computed CRC for a frame
             */
#define     GET_CRCPASSI(reg_val)  MASK2VAL(reg_val, BIT_CRCPASSI)
            /* CRCPASSI default value
             */
#define     CRCPASSI_DEFAULT  MOV2MASK(0, BIT_CRCPASSI)

          /* [ro] Receiver timeout interrupt
           */
#define   BIT_TIMEOUTI  BIT(0)
            /* This is a sticky bit. Set to '1' when wake-up timer matches
             * RXTIMEOUT during receiver timeout or auto-reply frame with no preamble detected
             *
             */
#define     GET_TIMEOUTI(reg_val)  MASK2VAL(reg_val, BIT_TIMEOUTI)
            /* TIMEOUTI default value
             */
#define     TIMEOUTI_DEFAULT  MOV2MASK(0, BIT_TIMEOUTI)

#define REG16_IRQ_DEFAULT  0x04F1



/******************************************************************************/
/*                   Register FRAMEPROC_PHASEDATA (16 bits)                   */
/******************************************************************************/
#define REG16_FRAMEPROC_PHASEDATA 0x11

          /* [ro] Preamble Phase Offset Data
           */
#define   BITS_PHASEDATA  BITS16(15, 8)
            /* This field assists in Round-Trip-Time Time-of-Flight delay
             * calculations for ranging with high accuracy. The data is collected during the
             * preamble, affecting delay measurement accuracy
             */
#define     GET_PHASEDATA(reg_val)  MASK2VAL(reg_val, BITS_PHASEDATA)
            /* PHASEDATA default value
             */
#define     PHASEDATA_DEFAULT  MOV2MASK(0x0, BITS_PHASEDATA)

          /* [wo] Radio Direction
           */
#define   BIT_RADIODIR  BIT(7)
            /* Reception mode
             */
#define     RADIODIR_0b1  MOV2MASK(0b1, BIT_RADIODIR)
            /* Transmission mode
             */
#define     RADIODIR_0b0  MOV2MASK(0b0, BIT_RADIODIR)
            /* RADIODIR default value
             */
#define     RADIODIR_DEFAULT  MOV2MASK(0x1, BIT_RADIODIR)

          /* [wo] Reply TX enable?
           */
#define   BIT_RPLYTXEN  BIT(6)
            /* Delegated to Gabriel M.
             */
#define     RPLYTXEN_0b1  MOV2MASK(0b1, BIT_RPLYTXEN)
            /* Delegated to Gabriel M.
             */
#define     RPLYTXEN_0b0  MOV2MASK(0b0, BIT_RPLYTXEN)
            /* RPLYTXEN default value
             */
#define     RPLYTXEN_DEFAULT  MOV2MASK(0x0, BIT_RPLYTXEN)

          /* [wo] RSSI only?
           */
#define   BIT_RSSIONLY  BIT(5)
            /* Delegated to Gabriel M.
             */
#define     RSSIONLY_0b1  MOV2MASK(0b1, BIT_RSSIONLY)
            /* Delegated to Gabriel M.
             */
#define     RSSIONLY_0b0  MOV2MASK(0b0, BIT_RSSIONLY)
            /* RSSIONLY default value
             */
#define     RSSIONLY_DEFAULT  MOV2MASK(0x0, BIT_RSSIONLY)

          /* [wo] Interleave frame data
           */
#define   BIT_INTRLEAV  BIT(4)
            /* 4x4 interleaver is applied before transmission and after
             * reception for robustness against burst errors. It interleaves data after FEC
             * encoding and deinterleaves after decoding
             */
#define     INTRLEAV_0b1  MOV2MASK(0b1, BIT_INTRLEAV)
            /* Disable the interleaver
             */
#define     INTRLEAV_0b0  MOV2MASK(0b0, BIT_INTRLEAV)
            /* INTRLEAV default value
             */
#define     INTRLEAV_DEFAULT  MOV2MASK(0x0, BIT_INTRLEAV)

          /* [wo] Enable PHY header field
           */
#define   BIT_PHYHDREN  BIT(3)
            /* Enables PHY headers in standalone frames
             */
#define     PHYHDREN_0b1  MOV2MASK(0b1, BIT_PHYHDREN)
            /* Disable PHY headers
             */
#define     PHYHDREN_0b0  MOV2MASK(0b0, BIT_PHYHDREN)
            /* PHYHDREN default value
             */
#define     PHYHDREN_DEFAULT  MOV2MASK(0x0, BIT_PHYHDREN)

          /* [wo] QoS PHY preset selection
           */
#define   BITS_PHYPRESET  BITS16(2, 0)
            /* Defines PHY preset for frame exchanges, applies if PHYHDREN=0
             */
#define     SET_PHYPRESET(value)  MOV2MASK(value, BITS_PHYPRESET)
            /* PHYPRESET default value
             */
#define     PHYPRESET_DEFAULT  MOV2MASK(0x0, BITS_PHYPRESET)

          /* [ro] Received PHY header field value
           */
#define   BITS_RXPHYPRES  BITS16(2, 0)
            /* This field hold the value of the last PHY header field received
             *
             */
#define     GET_RXPHYPRES(reg_val)  MASK2VAL(reg_val, BITS_RXPHYPRES)
            /* RXPHYPRES default value
             */
#define     RXPHYPRES_DEFAULT  MOV2MASK(0, BITS_RXPHYPRES)

#define REG16_FRAMEPROC_PHASEDATA_DEFAULT  0x0080



/******************************************************************************/
/*                        Register RSSI_RNSI (16 bits)                        */
/******************************************************************************/
#define REG16_RSSI_RNSI 0x12

          /* [ro] Received Noise Strength Indicator
           */
#define   BITS_RNSI  BITS16(14, 8)
            /* Shows last gain value when awaiting preamble detection. Reflects
             * noise level inversely; higher values denote lower noise floors. Interference
             * during the update affects its value
             */
#define     GET_RNSI(reg_val)  MASK2VAL(reg_val, BITS_RNSI)
            /* RNSI default value
             */
#define     RNSI_DEFAULT  MOV2MASK(0x0, BITS_RNSI)

          /* [ro] Received Signal Strength Indicator
           */
#define   BITS_RSSI  BITS16(6, 0)
            /* Displays the last baseband gain settled before sync word
             * detection. It inversely indicates signal strength; higher values reflect weaker
             * signals. It's overwritten upon new sync word detection, regardless of real
             * signal or noise
             */
#define     GET_RSSI(reg_val)  MASK2VAL(reg_val, BITS_RSSI)
            /* RSSI default value
             */
#define     RSSI_DEFAULT  MOV2MASK(0x0, BITS_RSSI)

#define REG16_RSSI_RNSI_DEFAULT  0x0000



/******************************************************************************/
/*                         Register RXTIME (16 bits)                          */
/******************************************************************************/
#define REG16_RXTIME 0x13

          /* [ro] Receiver sync time
           */
#define   BITS_RXSYNTIME  BITS16(15, 0)
            /* Displays lower 16 bits of chip clock count after last sync word
             * detection. Essential for precise time stamps in ranging, clock drift tracking,
             * and predicting remote node transmissions. Controller should maintain value above
             * power-up delay + received frame's preamble and sync time
             */
#define     GET_RXSYNTIME(reg_val)  MASK2VAL(reg_val, BITS_RXSYNTIME)
            /* RXSYNTIME default value
             */
#define     RXSYNTIME_DEFAULT  MOV2MASK(0x0, BITS_RXSYNTIME)

#define REG16_RXTIME_DEFAULT  0x0000



/******************************************************************************/
/*                         Register TXTIME (16 bits)                          */
/******************************************************************************/
#define REG16_TXTIME 0x14

          /* [ro] Tx sync word time stamp
           */
#define   BITS_TXSYNTIME  BITS16(15, 0)
            /* Shows lower 16 bits of chip clock count during last sync word
             * transmission
             */
#define     GET_TXSYNTIME(reg_val)  MASK2VAL(reg_val, BITS_TXSYNTIME)
            /* TXSYNTIME default value
             */
#define     TXSYNTIME_DEFAULT  MOV2MASK(0x0, BITS_TXSYNTIME)

#define REG16_TXTIME_DEFAULT  0x0000



/******************************************************************************/
/*                         Register IRQTIME (16 bits)                         */
/******************************************************************************/
#define REG16_IRQTIME 0x15

          /* [ro] IRQ signal timestamp
           */
#define   BITS_IRQTIME  BITS16(15, 0)
            /* Captures lower 16 bits of wake-up timer count when IRQ is
             * asserted by synchronous interrupt sources. Helpful for precise IRQ timing when
             * synchronized with chip clock. If read when no relevant interrupts set, returns
             * lower 16 bits of running wake-up timer count
             */
#define     GET_IRQTIME(reg_val)  MASK2VAL(reg_val, BITS_IRQTIME)
            /* IRQTIME default value
             */
#define     IRQTIME_DEFAULT  MOV2MASK(0x0, BITS_IRQTIME)

          /* [wo] Disable data buffer overflow/underflow interrupts
           */
#define   BIT_DISABUFI  BIT(0)
            /* TXOVRFLI, TXUDRFLI, RXOVRFLI, and RXUDRFLI can not trigger IRQ
             */
#define     DISABUFI_0b1  MOV2MASK(0b1, BIT_DISABUFI)
            /* TXOVRFLI, TXUDRFLI, RXOVRFLI, and RXUDRFLI can trigger IRQ
             */
#define     DISABUFI_0b0  MOV2MASK(0b0, BIT_DISABUFI)
            /* DISABUFI default value
             */
#define     DISABUFI_DEFAULT  MOV2MASK(0, BIT_DISABUFI)

#define REG16_IRQTIME_DEFAULT  0x0000



/******************************************************************************/
/*                     Register XTALCOUNT_15_0 (16 bits)                      */
/******************************************************************************/
#define REG16_XTALCOUNT_15_0 0x16

          /* [ro] Crystal oscillator wake-up timer count[15:0]
           */
#define   BITS_XTALTIME_15_0  BITS16(15, 0)
            /* Reflects wake-up timer count on crystal oscillator clock since
             * nCS signal pull-down. Reading after nCS pull-down returns its value. Refreshed
             * by toggling nCS. This timer runs on crystal oscillator clock constantly.
             */
#define     GET_XTALTIME_15_0(reg_val)  MASK2VAL(reg_val, BITS_XTALTIME_15_0)
            /* XTALTIME_15_0 default value
             */
#define     XTALTIME_15_0_DEFAULT  MOV2MASK(0x0, BITS_XTALTIME_15_0)

#define REG16_XTALCOUNT_15_0_DEFAULT  0x0000



/******************************************************************************/
/*                     Register XTALCOUNT_23_16 (16 bits)                     */
/******************************************************************************/
#define REG16_XTALCOUNT_23_16 0x17

          /* [ro] Crystal oscillator wake-up timer count[23:16]
           */
#define   BITS_XTALTIME_23_16  BITS16(7, 0)
            /* Reflects wake-up timer count on crystal oscillator clock since
             * nCS signal pull-down. Reading after nCS pull-down returns its value. Refreshed
             * by toggling nCS. This timer runs on crystal oscillator clock constantly.
             */
#define     GET_XTALTIME_23_16(reg_val)  MASK2VAL(reg_val, BITS_XTALTIME_23_16)
            /* XTALTIME_23_16 default value
             */
#define     XTALTIME_23_16_DEFAULT  MOV2MASK(0x0, BITS_XTALTIME_23_16)

#define REG16_XTALCOUNT_23_16_DEFAULT  0x0000



/******************************************************************************/
/*                         Register PHY_0_1 (16 bits)                         */
/******************************************************************************/
#define REG16_PHY_0_1 0x18

          /* [rw] Chip modulation code, Preset_1[0]
           */
#define   BIT_CHIPCODE1_0  BIT(15)
            /* Selects code for frame data modulation
             */
#define     SET_CHIPCODE1_0(value)    MOV2MASK(value, BIT_CHIPCODE1_0)
#define     GET_CHIPCODE1_0(reg_val)  MASK2VAL(reg_val, BIT_CHIPCODE1_0)
            /* CHIPCODE1_0 default value
             */
#define     CHIPCODE1_0_DEFAULT  MOV2MASK(0x0, BIT_CHIPCODE1_0)

          /* [rw] Forward error correction rate, Preset_1
           */
#define   BITS_FEC_RATE1  BITS16(14, 12)
            /* Sets FEC redundancy rate for frames. 0b000 disables FEC, directly
             * pushes data to interleaver. Other values adjust trade-off between data rate and
             * link margin. FEC introduces 32-chip decoding delay
             */
#define     SET_FEC_RATE1(value)    MOV2MASK(value, BITS_FEC_RATE1)
#define     GET_FEC_RATE1(reg_val)  MASK2VAL(reg_val, BITS_FEC_RATE1)
            /* FEC_RATE1 default value
             */
#define     FEC_RATE1_DEFAULT  MOV2MASK(0x3, BITS_FEC_RATE1)

          /* [rw] Expect auto-reply frame, Preset_0
           */
#define   BIT_EXPECRP0  BIT(11)
            /* Enables receiver to listen for and reply to auto-reply frames
             * after transmitting standalone frames
             */
#define     EXPECRP0_0b1  MOV2MASK(0b1, BIT_EXPECRP0)
            /* Receiver doesn't listen for auto-reply frames after transmitting
             * standalone frames
             */
#define     EXPECRP0_0b0  MOV2MASK(0b0, BIT_EXPECRP0)
            /* Get EXPECRP0 value
             */
#define     GET_EXPECRP0(reg_val)  MASK2VAL(reg_val, BIT_EXPECRP0)
            /* EXPECRP0 default value
             */
#define     EXPECRP0_DEFAULT  MOV2MASK(0x0, BIT_EXPECRP0)

          /* [rw] Auto-reply frame address, Preset_0
           */
#define   BIT_RPLYADD0  BIT(10)
            /* Auto-reply frame address matches standalone frame address
             */
#define     RPLYADD0_0b1  MOV2MASK(0b1, BIT_RPLYADD0)
            /* Auto-reply frame address uses TXADDRESS as auto-reply frame
             * address
             */
#define     RPLYADD0_0b0  MOV2MASK(0b0, BIT_RPLYADD0)
            /* Get RPLYADD0 value
             */
#define     GET_RPLYADD0(reg_val)  MASK2VAL(reg_val, BIT_RPLYADD0)
            /* RPLYADD0 default value
             */
#define     RPLYADD0_DEFAULT  MOV2MASK(0x0, BIT_RPLYADD0)

          /* [rw] Inter-symbol interference mitigation level, Preset_0
           */
#define   BITS_ISIMITIG0  BITS16(9, 8)
            /* Sets ISI mitigation pauses before each symbol. Helps prevent
             * interference between symbols. Values 0b00 to 0b10 provide varying levels of
             * protection
             */
#define     SET_ISIMITIG0(value)    MOV2MASK(value, BITS_ISIMITIG0)
#define     GET_ISIMITIG0(reg_val)  MASK2VAL(reg_val, BITS_ISIMITIG0)
            /* ISIMITIG0 default value
             */
#define     ISIMITIG0_DEFAULT  MOV2MASK(0x0, BITS_ISIMITIG0)

          /* [rw] Chip repetition factor, Preset_0
           */
#define   BITS_CHIPREPE0  BITS16(7, 6)
            /* Controls chip redundancy in frames. Values indicate how many
             * times each chip is repeated. Higher factors increase link margin but have
             * diminishing returns beyond 6x
             */
#define     SET_CHIPREPE0(value)    MOV2MASK(value, BITS_CHIPREPE0)
#define     GET_CHIPREPE0(reg_val)  MASK2VAL(reg_val, BITS_CHIPREPE0)
            /* CHIPREPE0 default value
             */
#define     CHIPREPE0_DEFAULT  MOV2MASK(0x0, BITS_CHIPREPE0)

          /* [rw] Chip modulation code, Preset_0
           */
#define   BITS_CHIPCODE0  BITS16(5, 3)
            /* Selects code for frame data modulation
             */
#define     SET_CHIPCODE0(value)    MOV2MASK(value, BITS_CHIPCODE0)
#define     GET_CHIPCODE0(reg_val)  MASK2VAL(reg_val, BITS_CHIPCODE0)
            /* CHIPCODE0 default value
             */
#define     CHIPCODE0_DEFAULT  MOV2MASK(0x2, BITS_CHIPCODE0)

          /* [rw] Forward error correction rate, Preset_0
           */
#define   BITS_FEC_RATE0  BITS16(2, 0)
            /* Sets FEC redundancy rate for frames. 0b000 disables FEC, directly
             * pushes data to interleaver. Other values adjust trade-off between data rate and
             * link margin. FEC introduces 32-chip decoding delay
             */
#define     SET_FEC_RATE0(value)    MOV2MASK(value, BITS_FEC_RATE0)
#define     GET_FEC_RATE0(reg_val)  MASK2VAL(reg_val, BITS_FEC_RATE0)
            /* FEC_RATE0 default value
             */
#define     FEC_RATE0_DEFAULT  MOV2MASK(0x3, BITS_FEC_RATE0)

#define REG16_PHY_0_1_DEFAULT  0x3013



/******************************************************************************/
/*                         Register PHY_1_2 (16 bits)                         */
/******************************************************************************/
#define REG16_PHY_1_2 0x19

          /* [rw] Chip repetition factor, Preset_2
           */
#define   BITS_CHIPREPE2  BITS16(15, 14)
            /* Controls chip redundancy in frames. Values indicate how many
             * times each chip is repeated. Higher factors increase link margin but have
             * diminishing returns beyond 6x
             */
#define     SET_CHIPREPE2(value)    MOV2MASK(value, BITS_CHIPREPE2)
#define     GET_CHIPREPE2(reg_val)  MASK2VAL(reg_val, BITS_CHIPREPE2)
            /* CHIPREPE2 default value
             */
#define     CHIPREPE2_DEFAULT  MOV2MASK(0x0, BITS_CHIPREPE2)

          /* [rw] Chip modulation code, Preset_2
           */
#define   BITS_CHIPCODE2  BITS16(13, 11)
            /* Selects code for frame data modulation
             */
#define     SET_CHIPCODE2(value)    MOV2MASK(value, BITS_CHIPCODE2)
#define     GET_CHIPCODE2(reg_val)  MASK2VAL(reg_val, BITS_CHIPCODE2)
            /* CHIPCODE2 default value
             */
#define     CHIPCODE2_DEFAULT  MOV2MASK(0x2, BITS_CHIPCODE2)

          /* [rw] Forward error correction rate, Preset_2
           */
#define   BITS_FEC_RATE2  BITS16(10, 8)
            /* Sets FEC redundancy rate for frames. 0b000 disables FEC, directly
             * pushes data to interleaver. Other values adjust trade-off between data rate and
             * link margin. FEC introduces 32-chip decoding delay
             */
#define     SET_FEC_RATE2(value)    MOV2MASK(value, BITS_FEC_RATE2)
#define     GET_FEC_RATE2(reg_val)  MASK2VAL(reg_val, BITS_FEC_RATE2)
            /* FEC_RATE2 default value
             */
#define     FEC_RATE2_DEFAULT  MOV2MASK(0x3, BITS_FEC_RATE2)

          /* [rw] Expect auto-reply frame, Preset_1
           */
#define   BIT_EXPECRP1  BIT(7)
            /* Enables receiver to listen for and reply to auto-reply frames
             * after transmitting standalone frames
             */
#define     EXPECRP1_0b1  MOV2MASK(0b1, BIT_EXPECRP1)
            /* Receiver doesn't listen for auto-reply frames after transmitting
             * standalone frames
             */
#define     EXPECRP1_0b0  MOV2MASK(0b0, BIT_EXPECRP1)
            /* Get EXPECRP1 value
             */
#define     GET_EXPECRP1(reg_val)  MASK2VAL(reg_val, BIT_EXPECRP1)
            /* EXPECRP1 default value
             */
#define     EXPECRP1_DEFAULT  MOV2MASK(0x0, BIT_EXPECRP1)

          /* [rw] Auto-reply frame address, Preset_1
           */
#define   BIT_RPLYADD1  BIT(6)
            /* Auto-reply frame address matches standalone frame address
             */
#define     RPLYADD1_0b1  MOV2MASK(0b1, BIT_RPLYADD1)
            /* Auto-reply frame address uses TXADDRESS as auto-reply frame
             * address
             */
#define     RPLYADD1_0b0  MOV2MASK(0b0, BIT_RPLYADD1)
            /* Get RPLYADD1 value
             */
#define     GET_RPLYADD1(reg_val)  MASK2VAL(reg_val, BIT_RPLYADD1)
            /* RPLYADD1 default value
             */
#define     RPLYADD1_DEFAULT  MOV2MASK(0x0, BIT_RPLYADD1)

          /* [rw] Inter-symbol interference mitigation level, Preset_1
           */
#define   BITS_ISIMITIG1  BITS16(5, 4)
            /* Sets ISI mitigation pauses before each symbol. Helps prevent
             * interference between symbols. Values 0b00 to 0b10 provide varying levels of
             * protection
             */
#define     SET_ISIMITIG1(value)    MOV2MASK(value, BITS_ISIMITIG1)
#define     GET_ISIMITIG1(reg_val)  MASK2VAL(reg_val, BITS_ISIMITIG1)
            /* ISIMITIG1 default value
             */
#define     ISIMITIG1_DEFAULT  MOV2MASK(0x0, BITS_ISIMITIG1)

          /* [rw] Chip repetition factor, Preset_1
           */
#define   BITS_CHIPREPE1  BITS16(4, 2)
            /* Controls chip redundancy in frames. Values indicate how many
             * times each chip is repeated. Higher factors increase link margin but have
             * diminishing returns beyond 6x
             */
#define     SET_CHIPREPE1(value)    MOV2MASK(value, BITS_CHIPREPE1)
#define     GET_CHIPREPE1(reg_val)  MASK2VAL(reg_val, BITS_CHIPREPE1)
            /* CHIPREPE1 default value
             */
#define     CHIPREPE1_DEFAULT  MOV2MASK(0x0, BITS_CHIPREPE1)

          /* [rw] Chip modulation code, Preset_1[2:1]
           */
#define   BITS_CHIPCODE1_2_1  BITS16(1, 0)
            /* Selects code for frame data modulation
             */
#define     SET_CHIPCODE1_2_1(value)    MOV2MASK(value, BITS_CHIPCODE1_2_1)
#define     GET_CHIPCODE1_2_1(reg_val)  MASK2VAL(reg_val, BITS_CHIPCODE1_2_1)
            /* CHIPCODE1_2_1 default value
             */
#define     CHIPCODE1_2_1_DEFAULT  MOV2MASK(0x1, BITS_CHIPCODE1_2_1)

#define REG16_PHY_1_2_DEFAULT  0x1301



/******************************************************************************/
/*                         Register PHY_2_3 (16 bits)                         */
/******************************************************************************/
#define REG16_PHY_2_3 0x1A

          /* [rw] Expect auto-reply frame, Preset_3
           */
#define   BIT_EXPECRP3  BIT(15)
            /* Enables receiver to listen for and reply to auto-reply frames
             * after transmitting standalone frames
             */
#define     EXPECRP3_0b1  MOV2MASK(0b1, BIT_EXPECRP3)
            /* Receiver doesn't listen for auto-reply frames after transmitting
             * standalone frames
             */
#define     EXPECRP3_0b0  MOV2MASK(0b0, BIT_EXPECRP3)
            /* Get EXPECRP3 value
             */
#define     GET_EXPECRP3(reg_val)  MASK2VAL(reg_val, BIT_EXPECRP3)
            /* EXPECRP3 default value
             */
#define     EXPECRP3_DEFAULT  MOV2MASK(0x0, BIT_EXPECRP3)

          /* [rw] Auto-reply frame address, Preset_3
           */
#define   BIT_RPLYADD3  BIT(14)
            /* Auto-reply frame address matches standalone frame address
             */
#define     RPLYADD3_0b1  MOV2MASK(0b1, BIT_RPLYADD3)
            /* Auto-reply frame address uses TXADDRESS as auto-reply frame
             * address
             */
#define     RPLYADD3_0b0  MOV2MASK(0b0, BIT_RPLYADD3)
            /* Get RPLYADD3 value
             */
#define     GET_RPLYADD3(reg_val)  MASK2VAL(reg_val, BIT_RPLYADD3)
            /* RPLYADD3 default value
             */
#define     RPLYADD3_DEFAULT  MOV2MASK(0x0, BIT_RPLYADD3)

          /* [rw] Inter-symbol interference mitigation level, Preset_3
           */
#define   BITS_ISIMITIG3  BITS16(13, 12)
            /* Sets ISI mitigation pauses before each symbol. Helps prevent
             * interference between symbols. Values 0b00 to 0b10 provide varying levels of
             * protection
             */
#define     SET_ISIMITIG3(value)    MOV2MASK(value, BITS_ISIMITIG3)
#define     GET_ISIMITIG3(reg_val)  MASK2VAL(reg_val, BITS_ISIMITIG3)
            /* ISIMITIG3 default value
             */
#define     ISIMITIG3_DEFAULT  MOV2MASK(0x0, BITS_ISIMITIG3)

          /* [rw] Chip repetition factor, Preset_3
           */
#define   BITS_CHIPREPE3  BITS16(11, 10)
            /* Controls chip redundancy in frames. Values indicate how many
             * times each chip is repeated. Higher factors increase link margin but have
             * diminishing returns beyond 6x
             */
#define     SET_CHIPREPE3(value)    MOV2MASK(value, BITS_CHIPREPE3)
#define     GET_CHIPREPE3(reg_val)  MASK2VAL(reg_val, BITS_CHIPREPE3)
            /* CHIPREPE3 default value
             */
#define     CHIPREPE3_DEFAULT  MOV2MASK(0x0, BITS_CHIPREPE3)

          /* [rw] Chip modulation code, Preset_3
           */
#define   BITS_CHIPCODE3  BITS16(9, 7)
            /* Selects code for frame data modulation
             */
#define     SET_CHIPCODE3(value)    MOV2MASK(value, BITS_CHIPCODE3)
#define     GET_CHIPCODE3(reg_val)  MASK2VAL(reg_val, BITS_CHIPCODE3)
            /* CHIPCODE3 default value
             */
#define     CHIPCODE3_DEFAULT  MOV2MASK(0x2, BITS_CHIPCODE3)

          /* [rw] Forward error correction rate, Preset_3
           */
#define   BITS_FEC_RATE3  BITS16(6, 4)
            /* Sets FEC redundancy rate for frames. 0b000 disables FEC, directly
             * pushes data to interleaver. Other values adjust trade-off between data rate and
             * link margin. FEC introduces 32-chip decoding delay
             */
#define     SET_FEC_RATE3(value)    MOV2MASK(value, BITS_FEC_RATE3)
#define     GET_FEC_RATE3(reg_val)  MASK2VAL(reg_val, BITS_FEC_RATE3)
            /* FEC_RATE3 default value
             */
#define     FEC_RATE3_DEFAULT  MOV2MASK(0x3, BITS_FEC_RATE3)

          /* [rw] Expect auto-reply frame, Preset_2
           */
#define   BIT_EXPECRP2  BIT(3)
            /* Enables receiver to listen for and reply to auto-reply frames
             * after transmitting standalone frames
             */
#define     EXPECRP2_0b1  MOV2MASK(0b1, BIT_EXPECRP2)
            /* Receiver doesn't listen for auto-reply frames after transmitting
             * standalone frames
             */
#define     EXPECRP2_0b0  MOV2MASK(0b0, BIT_EXPECRP2)
            /* Get EXPECRP2 value
             */
#define     GET_EXPECRP2(reg_val)  MASK2VAL(reg_val, BIT_EXPECRP2)
            /* EXPECRP2 default value
             */
#define     EXPECRP2_DEFAULT  MOV2MASK(0x0, BIT_EXPECRP2)

          /* [rw] Auto-reply frame address, Preset_2
           */
#define   BIT_RPLYADD2  BIT(2)
            /* Auto-reply frame address matches standalone frame address
             */
#define     RPLYADD2_0b1  MOV2MASK(0b1, BIT_RPLYADD2)
            /* Auto-reply frame address uses TXADDRESS as auto-reply frame
             * address
             */
#define     RPLYADD2_0b0  MOV2MASK(0b0, BIT_RPLYADD2)
            /* Get RPLYADD2 value
             */
#define     GET_RPLYADD2(reg_val)  MASK2VAL(reg_val, BIT_RPLYADD2)
            /* RPLYADD2 default value
             */
#define     RPLYADD2_DEFAULT  MOV2MASK(0x0, BIT_RPLYADD2)

          /* [rw] Inter-symbol interference mitigation level, Preset_2
           */
#define   BITS_ISIMITIG2  BITS16(1, 0)
            /* Sets ISI mitigation pauses before each symbol. Helps prevent
             * interference between symbols. Values 0b00 to 0b10 provide varying levels of
             * protection
             */
#define     SET_ISIMITIG2(value)    MOV2MASK(value, BITS_ISIMITIG2)
#define     GET_ISIMITIG2(reg_val)  MASK2VAL(reg_val, BITS_ISIMITIG2)
            /* ISIMITIG2 default value
             */
#define     ISIMITIG2_DEFAULT  MOV2MASK(0x0, BITS_ISIMITIG2)

#define REG16_PHY_2_3_DEFAULT  0x0130



/******************************************************************************/
/*                         Register PHY_4_5 (16 bits)                         */
/******************************************************************************/
#define REG16_PHY_4_5 0x1B

          /* [rw] Chip modulation code, Preset_5[0]
           */
#define   BIT_CHIPCODE5_0  BIT(15)
            /* Selects code for frame data modulation
             */
#define     SET_CHIPCODE5_0(value)    MOV2MASK(value, BIT_CHIPCODE5_0)
#define     GET_CHIPCODE5_0(reg_val)  MASK2VAL(reg_val, BIT_CHIPCODE5_0)
            /* CHIPCODE5_0 default value
             */
#define     CHIPCODE5_0_DEFAULT  MOV2MASK(0x0, BIT_CHIPCODE5_0)

          /* [rw] Forward error correction rate, Preset_5
           */
#define   BITS_FEC_RATE5  BITS16(14, 12)
            /* Sets FEC redundancy rate for frames. 0b000 disables FEC, directly
             * pushes data to interleaver. Other values adjust trade-off between data rate and
             * link margin. FEC introduces 32-chip decoding delay
             */
#define     SET_FEC_RATE5(value)    MOV2MASK(value, BITS_FEC_RATE5)
#define     GET_FEC_RATE5(reg_val)  MASK2VAL(reg_val, BITS_FEC_RATE5)
            /* FEC_RATE5 default value
             */
#define     FEC_RATE5_DEFAULT  MOV2MASK(0x3, BITS_FEC_RATE5)

          /* [rw] Expect auto-reply frame, Preset_4
           */
#define   BIT_EXPECRP4  BIT(11)
            /* Enables receiver to listen for and reply to auto-reply frames
             * after transmitting standalone frames
             */
#define     EXPECRP4_0b1  MOV2MASK(0b1, BIT_EXPECRP4)
            /* Receiver doesn't listen for auto-reply frames after transmitting
             * standalone frames
             */
#define     EXPECRP4_0b0  MOV2MASK(0b0, BIT_EXPECRP4)
            /* Get EXPECRP4 value
             */
#define     GET_EXPECRP4(reg_val)  MASK2VAL(reg_val, BIT_EXPECRP4)
            /* EXPECRP4 default value
             */
#define     EXPECRP4_DEFAULT  MOV2MASK(0x0, BIT_EXPECRP4)

          /* [rw] Auto-reply frame address, Preset_4
           */
#define   BIT_RPLYADD4  BIT(10)
            /* Auto-reply frame address matches standalone frame address
             */
#define     RPLYADD4_0b1  MOV2MASK(0b1, BIT_RPLYADD4)
            /* Auto-reply frame address uses TXADDRESS as auto-reply frame
             * address
             */
#define     RPLYADD4_0b0  MOV2MASK(0b0, BIT_RPLYADD4)
            /* Get RPLYADD4 value
             */
#define     GET_RPLYADD4(reg_val)  MASK2VAL(reg_val, BIT_RPLYADD4)
            /* RPLYADD4 default value
             */
#define     RPLYADD4_DEFAULT  MOV2MASK(0x0, BIT_RPLYADD4)

          /* [rw] Inter-symbol interference mitigation level, Preset_4
           */
#define   BITS_ISIMITIG4  BITS16(9, 8)
            /* Sets ISI mitigation pauses before each symbol. Helps prevent
             * interference between symbols. Values 0b00 to 0b10 provide varying levels of
             * protection
             */
#define     SET_ISIMITIG4(value)    MOV2MASK(value, BITS_ISIMITIG4)
#define     GET_ISIMITIG4(reg_val)  MASK2VAL(reg_val, BITS_ISIMITIG4)
            /* ISIMITIG4 default value
             */
#define     ISIMITIG4_DEFAULT  MOV2MASK(0x0, BITS_ISIMITIG4)

          /* [rw] Chip repetition factor, Preset_4
           */
#define   BITS_CHIPREPE4  BITS16(7, 6)
            /* Controls chip redundancy in frames. Values indicate how many
             * times each chip is repeated. Higher factors increase link margin but have
             * diminishing returns beyond 6x
             */
#define     SET_CHIPREPE4(value)    MOV2MASK(value, BITS_CHIPREPE4)
#define     GET_CHIPREPE4(reg_val)  MASK2VAL(reg_val, BITS_CHIPREPE4)
            /* CHIPREPE4 default value
             */
#define     CHIPREPE4_DEFAULT  MOV2MASK(0x0, BITS_CHIPREPE4)

          /* [rw] Chip modulation code, Preset_4
           */
#define   BITS_CHIPCODE4  BITS16(5, 3)
            /* Selects code for frame data modulation
             */
#define     SET_CHIPCODE4(value)    MOV2MASK(value, BITS_CHIPCODE4)
#define     GET_CHIPCODE4(reg_val)  MASK2VAL(reg_val, BITS_CHIPCODE4)
            /* CHIPCODE4 default value
             */
#define     CHIPCODE4_DEFAULT  MOV2MASK(0x2, BITS_CHIPCODE4)

          /* [rw] Forward error correction rate, Preset_4
           */
#define   BITS_FEC_RATE4  BITS16(2, 0)
            /* Sets FEC redundancy rate for frames. 0b000 disables FEC, directly
             * pushes data to interleaver. Other values adjust trade-off between data rate and
             * link margin. FEC introduces 32-chip decoding delay
             */
#define     SET_FEC_RATE4(value)    MOV2MASK(value, BITS_FEC_RATE4)
#define     GET_FEC_RATE4(reg_val)  MASK2VAL(reg_val, BITS_FEC_RATE4)
            /* FEC_RATE4 default value
             */
#define     FEC_RATE4_DEFAULT  MOV2MASK(0x3, BITS_FEC_RATE4)

#define REG16_PHY_4_5_DEFAULT  0x3013



/******************************************************************************/
/*                         Register PHY_5_6 (16 bits)                         */
/******************************************************************************/
#define REG16_PHY_5_6 0x1C

          /* [rw] Chip repetition factor, Preset_6
           */
#define   BITS_CHIPREPE6  BITS16(15, 14)
            /* Controls chip redundancy in frames. Values indicate how many
             * times each chip is repeated. Higher factors increase link margin but have
             * diminishing returns beyond 6x
             */
#define     SET_CHIPREPE6(value)    MOV2MASK(value, BITS_CHIPREPE6)
#define     GET_CHIPREPE6(reg_val)  MASK2VAL(reg_val, BITS_CHIPREPE6)
            /* CHIPREPE6 default value
             */
#define     CHIPREPE6_DEFAULT  MOV2MASK(0x0, BITS_CHIPREPE6)

          /* [rw] Chip modulation code, Preset_6
           */
#define   BITS_CHIPCODE6  BITS16(13, 11)
            /* Selects code for frame data modulation
             */
#define     SET_CHIPCODE6(value)    MOV2MASK(value, BITS_CHIPCODE6)
#define     GET_CHIPCODE6(reg_val)  MASK2VAL(reg_val, BITS_CHIPCODE6)
            /* CHIPCODE6 default value
             */
#define     CHIPCODE6_DEFAULT  MOV2MASK(0x2, BITS_CHIPCODE6)

          /* [rw] Forward error correction rate, Preset_6
           */
#define   BITS_FEC_RATE6  BITS16(10, 8)
            /* Sets FEC redundancy rate for frames. 0b000 disables FEC, directly
             * pushes data to interleaver. Other values adjust trade-off between data rate and
             * link margin. FEC introduces 32-chip decoding delay
             */
#define     SET_FEC_RATE6(value)    MOV2MASK(value, BITS_FEC_RATE6)
#define     GET_FEC_RATE6(reg_val)  MASK2VAL(reg_val, BITS_FEC_RATE6)
            /* FEC_RATE6 default value
             */
#define     FEC_RATE6_DEFAULT  MOV2MASK(0x3, BITS_FEC_RATE6)

          /* [rw] Expect auto-reply frame, Preset_5
           */
#define   BIT_EXPECRP5  BIT(7)
            /* Enables receiver to listen for and reply to auto-reply frames
             * after transmitting standalone frames
             */
#define     EXPECRP5_0b1  MOV2MASK(0b1, BIT_EXPECRP5)
            /* Receiver doesn't listen for auto-reply frames after transmitting
             * standalone frames
             */
#define     EXPECRP5_0b0  MOV2MASK(0b0, BIT_EXPECRP5)
            /* Get EXPECRP5 value
             */
#define     GET_EXPECRP5(reg_val)  MASK2VAL(reg_val, BIT_EXPECRP5)
            /* EXPECRP5 default value
             */
#define     EXPECRP5_DEFAULT  MOV2MASK(0x0, BIT_EXPECRP5)

          /* [rw] Auto-reply frame address, Preset_5
           */
#define   BIT_RPLYADD5  BIT(6)
            /* Enables receiver to listen for and reply to auto-reply frames
             * after transmitting standalone frames
             */
#define     RPLYADD5_0b1  MOV2MASK(0b1, BIT_RPLYADD5)
            /* Receiver doesn't listen for auto-reply frames after transmitting
             * standalone frames
             */
#define     RPLYADD5_0b0  MOV2MASK(0b0, BIT_RPLYADD5)
            /* Get RPLYADD5 value
             */
#define     GET_RPLYADD5(reg_val)  MASK2VAL(reg_val, BIT_RPLYADD5)
            /* RPLYADD5 default value
             */
#define     RPLYADD5_DEFAULT  MOV2MASK(0x0, BIT_RPLYADD5)

          /* [rw] Inter-symbol interference mitigation level, Preset_5
           */
#define   BITS_ISIMITIG5  BITS16(5, 4)
            /* Sets ISI mitigation pauses before each symbol. Helps prevent
             * interference between symbols. Values 0b00 to 0b10 provide varying levels of
             * protection
             */
#define     SET_ISIMITIG5(value)    MOV2MASK(value, BITS_ISIMITIG5)
#define     GET_ISIMITIG5(reg_val)  MASK2VAL(reg_val, BITS_ISIMITIG5)
            /* ISIMITIG5 default value
             */
#define     ISIMITIG5_DEFAULT  MOV2MASK(0x0, BITS_ISIMITIG5)

          /* [rw] Chip repetition factor, Preset_5
           */
#define   BITS_CHIPREPE5  BITS16(3, 2)
            /* Controls chip redundancy in frames. Values indicate how many
             * times each chip is repeated. Higher factors increase link margin but have
             * diminishing returns beyond 6x
             */
#define     SET_CHIPREPE5(value)    MOV2MASK(value, BITS_CHIPREPE5)
#define     GET_CHIPREPE5(reg_val)  MASK2VAL(reg_val, BITS_CHIPREPE5)
            /* CHIPREPE5 default value
             */
#define     CHIPREPE5_DEFAULT  MOV2MASK(0x0, BITS_CHIPREPE5)

          /* [rw] Chip modulation code, Preset_5[2:1]
           */
#define   BITS_CHIPCODE5_2_1  BITS16(1, 0)
            /* Selects code for frame data modulation
             */
#define     SET_CHIPCODE5_2_1(value)    MOV2MASK(value, BITS_CHIPCODE5_2_1)
#define     GET_CHIPCODE5_2_1(reg_val)  MASK2VAL(reg_val, BITS_CHIPCODE5_2_1)
            /* CHIPCODE5_2_1 default value
             */
#define     CHIPCODE5_2_1_DEFAULT  MOV2MASK(0x1, BITS_CHIPCODE5_2_1)

#define REG16_PHY_5_6_DEFAULT  0x1301



/******************************************************************************/
/*                         Register PHY_6_7 (16 bits)                         */
/******************************************************************************/
#define REG16_PHY_6_7 0x1D

          /* [rw] Expect auto-reply frame, Preset_7
           */
#define   BIT_EXPECRP7  BIT(15)
            /* Enables receiver to listen for and reply to auto-reply frames
             * after transmitting standalone frames
             */
#define     EXPECRP7_0b1  MOV2MASK(0b1, BIT_EXPECRP7)
            /* Receiver doesn't listen for auto-reply frames after transmitting
             * standalone frames
             */
#define     EXPECRP7_0b0  MOV2MASK(0b0, BIT_EXPECRP7)
            /* Get EXPECRP7 value
             */
#define     GET_EXPECRP7(reg_val)  MASK2VAL(reg_val, BIT_EXPECRP7)
            /* EXPECRP7 default value
             */
#define     EXPECRP7_DEFAULT  MOV2MASK(0x0, BIT_EXPECRP7)

          /* [rw] Auto-reply frame address, Preset_7
           */
#define   BIT_RPLYADD7  BIT(14)
            /* Enables receiver to listen for and reply to auto-reply frames
             * after transmitting standalone frames
             */
#define     RPLYADD7_0b1  MOV2MASK(0b1, BIT_RPLYADD7)
            /* Receiver doesn't listen for auto-reply frames after transmitting
             * standalone frames
             */
#define     RPLYADD7_0b0  MOV2MASK(0b0, BIT_RPLYADD7)
            /* Get RPLYADD7 value
             */
#define     GET_RPLYADD7(reg_val)  MASK2VAL(reg_val, BIT_RPLYADD7)
            /* RPLYADD7 default value
             */
#define     RPLYADD7_DEFAULT  MOV2MASK(0x0, BIT_RPLYADD7)

          /* [rw] Inter-symbol interference mitigation level, Preset_7
           */
#define   BITS_ISIMITIG7  BITS16(13, 12)
            /* Sets ISI mitigation pauses before each symbol. Helps prevent
             * interference between symbols. Values 0b00 to 0b10 provide varying levels of
             * protection
             */
#define     SET_ISIMITIG7(value)    MOV2MASK(value, BITS_ISIMITIG7)
#define     GET_ISIMITIG7(reg_val)  MASK2VAL(reg_val, BITS_ISIMITIG7)
            /* ISIMITIG7 default value
             */
#define     ISIMITIG7_DEFAULT  MOV2MASK(0x0, BITS_ISIMITIG7)

          /* [rw] Chip repetition factor, Preset_7
           */
#define   BITS_CHIPREPE7  BITS16(11, 10)
            /* Controls chip redundancy in frames. Values indicate how many
             * times each chip is repeated. Higher factors increase link margin but have
             * diminishing returns beyond 6x
             */
#define     SET_CHIPREPE7(value)    MOV2MASK(value, BITS_CHIPREPE7)
#define     GET_CHIPREPE7(reg_val)  MASK2VAL(reg_val, BITS_CHIPREPE7)
            /* CHIPREPE7 default value
             */
#define     CHIPREPE7_DEFAULT  MOV2MASK(0x0, BITS_CHIPREPE7)

          /* [rw] Chip modulation code, Preset_7
           */
#define   BITS_CHIPCODE7  BITS16(9, 7)
            /* Selects code for frame data modulation
             */
#define     SET_CHIPCODE7(value)    MOV2MASK(value, BITS_CHIPCODE7)
#define     GET_CHIPCODE7(reg_val)  MASK2VAL(reg_val, BITS_CHIPCODE7)
            /* CHIPCODE7 default value
             */
#define     CHIPCODE7_DEFAULT  MOV2MASK(0x2, BITS_CHIPCODE7)

          /* [rw] Forward error correction rate, Preset_7
           */
#define   BITS_FEC_RATE7  BITS16(6, 4)
            /* Sets FEC redundancy rate for frames. 0b000 disables FEC, directly
             * pushes data to interleaver. Other values adjust trade-off between data rate and
             * link margin. FEC introduces 32-chip decoding delay
             */
#define     SET_FEC_RATE7(value)    MOV2MASK(value, BITS_FEC_RATE7)
#define     GET_FEC_RATE7(reg_val)  MASK2VAL(reg_val, BITS_FEC_RATE7)
            /* FEC_RATE7 default value
             */
#define     FEC_RATE7_DEFAULT  MOV2MASK(0x3, BITS_FEC_RATE7)

          /* [rw] Expect auto-reply frame, Preset_6
           */
#define   BIT_EXPECRP6  BIT(3)
            /* Enables receiver to listen for and reply to auto-reply frames
             * after transmitting standalone frames
             */
#define     EXPECRP6_0b1  MOV2MASK(0b1, BIT_EXPECRP6)
            /* Receiver doesn't listen for auto-reply frames after transmitting
             * standalone frames
             */
#define     EXPECRP6_0b0  MOV2MASK(0b0, BIT_EXPECRP6)
            /* Get EXPECRP6 value
             */
#define     GET_EXPECRP6(reg_val)  MASK2VAL(reg_val, BIT_EXPECRP6)
            /* EXPECRP6 default value
             */
#define     EXPECRP6_DEFAULT  MOV2MASK(0x0, BIT_EXPECRP6)

          /* [rw] Auto-reply frame address, Preset_6
           */
#define   BIT_RPLYADD6  BIT(2)
            /* Enables receiver to listen for and reply to auto-reply frames
             * after transmitting standalone frames
             */
#define     RPLYADD6_0b1  MOV2MASK(0b1, BIT_RPLYADD6)
            /* Receiver doesn't listen for auto-reply frames after transmitting
             * standalone frames
             */
#define     RPLYADD6_0b0  MOV2MASK(0b0, BIT_RPLYADD6)
            /* Get RPLYADD6 value
             */
#define     GET_RPLYADD6(reg_val)  MASK2VAL(reg_val, BIT_RPLYADD6)
            /* RPLYADD6 default value
             */
#define     RPLYADD6_DEFAULT  MOV2MASK(0x0, BIT_RPLYADD6)

          /* [rw] Inter-symbol interference mitigation level, Preset_6
           */
#define   BITS_ISIMITIG6  BITS16(1, 0)
            /* Sets ISI mitigation pauses before each symbol. Helps prevent
             * interference between symbols. Values 0b00 to 0b10 provide varying levels of
             * protection
             */
#define     SET_ISIMITIG6(value)    MOV2MASK(value, BITS_ISIMITIG6)
#define     GET_ISIMITIG6(reg_val)  MASK2VAL(reg_val, BITS_ISIMITIG6)
            /* ISIMITIG6 default value
             */
#define     ISIMITIG6_DEFAULT  MOV2MASK(0x0, BITS_ISIMITIG6)

#define REG16_PHY_6_7_DEFAULT  0x0130



/******************************************************************************/
/*                      Register PREAMB_DEBUG (16 bits)                       */
/******************************************************************************/
#define REG16_PREAMB_DEBUG 0x2D

          /* [rw] Main debug and observability options
           */
#define   BITS_MAINDEBUG  BITS16(15, 12)
            /* Snables debugging features. Controls package pin signals,
             * receiver integration window, phase tracking, and probe selection. Allows
             * arbitrary chip stream transmission through package pin
             */
#define     SET_MAINDEBUG(value)    MOV2MASK(value, BITS_MAINDEBUG)
#define     GET_MAINDEBUG(reg_val)  MASK2VAL(reg_val, BITS_MAINDEBUG)
            /* MAINDEBUG default value
             */
#define     MAINDEBUG_DEFAULT  MOV2MASK(0x0, BITS_MAINDEBUG)

          /* [rw] Substitute payload with test data
           */
#define   BIT_TESTDATA  BIT(11)
            /* Replaces payload with fixed pattern for debugging
             */
#define     TESTDATA_0b1  MOV2MASK(0b1, BIT_TESTDATA)
            /* Uses the payload in TX buffer
             */
#define     TESTDATA_0b0  MOV2MASK(0b0, BIT_TESTDATA)
            /* Get TESTDATA value
             */
#define     GET_TESTDATA(reg_val)  MASK2VAL(reg_val, BIT_TESTDATA)
            /* TESTDATA default value
             */
#define     TESTDATA_DEFAULT  MOV2MASK(0x0, BIT_TESTDATA)

          /* [rw] Receiver's bit decision threshold offset
           */
#define   BITS_BITOFFSET  BITS16(10, 8)
            /* Sets offset to bit decision threshold for non-differential chip
             * codes. Range: -2.0 to +1.5 LSB
             */
#define     SET_BITOFFSET(value)    MOV2MASK(value, BITS_BITOFFSET)
#define     GET_BITOFFSET(reg_val)  MASK2VAL(reg_val, BITS_BITOFFSET)
            /* BITOFFSET default value
             */
#define     BITOFFSET_DEFAULT  MOV2MASK(0x0, BITS_BITOFFSET)

          /* [rw] Max preamble signal level
           */
#define   BITS_MAXSIGLVL  BITS16(7, 6)
            /* Sets threshold for received signal energy. Controls AGC loop for
             * integration window energy
             */
#define     SET_MAXSIGLVL(value)    MOV2MASK(value, BITS_MAXSIGLVL)
#define     GET_MAXSIGLVL(reg_val)  MASK2VAL(reg_val, BITS_MAXSIGLVL)
            /* MAXSIGLVL default value
             */
#define     MAXSIGLVL_DEFAULT  MOV2MASK(0x2, BITS_MAXSIGLVL)

          /* [rw] Signal floor target?
           */
#define   BITS_SIGFLOOR  BITS16(5, 4)
            /* Delegated to Gabriel M.
             */
#define     SET_SIGFLOOR(value)    MOV2MASK(value, BITS_SIGFLOOR)
#define     GET_SIGFLOOR(reg_val)  MASK2VAL(reg_val, BITS_SIGFLOOR)
            /* SIGFLOOR default value
             */
#define     SIGFLOOR_DEFAULT  MOV2MASK(0x0, BITS_SIGFLOOR)

          /* [rw] Summation of ADC samples??
           */
#define   BIT_SUMRXADC  BIT(3)
            /* Delegated to Gabriel M.
             */
#define     SET_SUMRXADC(value)    MOV2MASK(value, BIT_SUMRXADC)
#define     GET_SUMRXADC(reg_val)  MASK2VAL(reg_val, BIT_SUMRXADC)
            /* SUMRXADC default value
             */
#define     SUMRXADC_DEFAULT  MOV2MASK(0x0, BIT_SUMRXADC)

          /* [rw] Decision feedback equalizer config
           */
#define   BITS_DFECONFIG  BITS16(2, 0)
            /* Delegated to Gabriel M.
             */
#define     SET_DFECONFIG(value)    MOV2MASK(value, BITS_DFECONFIG)
#define     GET_DFECONFIG(reg_val)  MASK2VAL(reg_val, BITS_DFECONFIG)
            /* DFECONFIG default value
             */
#define     DFECONFIG_DEFAULT  MOV2MASK(0x0, BITS_DFECONFIG)

#define REG16_PREAMB_DEBUG_DEFAULT  0x0080



/******************************************************************************/
/*                         Register PRELUDE (16 bits)                         */
/******************************************************************************/
#define REG16_PRELUDE 0x2E

          /* [rw] Frame phase tracking bandwidth
           */
#define   BIT_PHSTRKBW  BIT(15)
            /* Delegated to Gabriel M.
             */
#define     PHSTRKBW_0b1  MOV2MASK(0b1, BIT_PHSTRKBW)
            /* Delegated to Gabriel M.
             */
#define     PHSTRKBW_0b0  MOV2MASK(0b0, BIT_PHSTRKBW)
            /* Get PHSTRKBW value
             */
#define     GET_PHSTRKBW(reg_val)  MASK2VAL(reg_val, BIT_PHSTRKBW)
            /* PHSTRKBW default value
             */
#define     PHSTRKBW_DEFAULT  MOV2MASK(0x0, BIT_PHSTRKBW)

          /* [rw] AGC loop bandwidth in fast convergence
           */
#define   BIT_GAINCTBW  BIT(14)
            /* The value held in the register field PREADETBW is the one that
             * sets the bandwidth of the digital filters when the gain control loop is in its
             * fast convergence state even if the frame processor is in the process of tracking
             * a preamble already detected
             */
#define     GAINCTBW_0b1  MOV2MASK(0b1, BIT_GAINCTBW)
            /* None of the behaviors described in the descriptions of register
             * fields PREADETBW and PREATRKBW are overriden
             */
#define     GAINCTBW_0b0  MOV2MASK(0b0, BIT_GAINCTBW)
            /* Get GAINCTBW value
             */
#define     GET_GAINCTBW(reg_val)  MASK2VAL(reg_val, BIT_GAINCTBW)
            /* GAINCTBW default value
             */
#define     GAINCTBW_DEFAULT  MOV2MASK(0x1, BIT_GAINCTBW)

          /* [rw] Sync word bit mismatch tolerance
           */
#define   BITS_SWBITTOL  BITS16(13, 12)
            /* Sets tolerance for bit mismatches in sync word detection.
             * Enhances accuracy without changing sync word length
             */
#define     SET_SWBITTOL(value)    MOV2MASK(value, BITS_SWBITTOL)
#define     GET_SWBITTOL(reg_val)  MASK2VAL(reg_val, BITS_SWBITTOL)
            /* SWBITTOL default value
             */
#define     SWBITTOL_DEFAULT  MOV2MASK(0x2, BITS_SWBITTOL)

          /* [rw] Soft sync word correlation threshold
           */
#define   BITS_SOFTSWTHR  BITS16(11, 8)
            /* Determines correlation for sync word detection. Higher values
             * require stronger signal. Works alongside SWBITTOL
             */
#define     SET_SOFTSWTHR(value)    MOV2MASK(value, BITS_SOFTSWTHR)
#define     GET_SOFTSWTHR(reg_val)  MASK2VAL(reg_val, BITS_SOFTSWTHR)
            /* SOFTSWTHR default value
             */
#define     SOFTSWTHR_DEFAULT  MOV2MASK(0xa, BITS_SOFTSWTHR)

          /* [rw] Preamble track IIR filter bandwidth
           */
#define   BITS_PREATRKBW  BITS16(7, 6)
            /* Similar to PREADETBW, but for preamble tracking state. Affects
             * phase tracking and automatic antenna selection
             */
#define     SET_PREATRKBW(value)    MOV2MASK(value, BITS_PREATRKBW)
#define     GET_PREATRKBW(reg_val)  MASK2VAL(reg_val, BITS_PREATRKBW)
            /* PREATRKBW default value
             */
#define     PREATRKBW_DEFAULT  MOV2MASK(0x2, BITS_PREATRKBW)

          /* [rw] Preamble detect IIR filter bandwidth
           */
#define   BITS_PREADETBW  BITS16(5, 4)
            /* Sets bandwidth of low-pass filter for preamble amplitude
             * extraction. Higher values for quicker reaction, lower for accuracy
             */
#define     SET_PREADETBW(value)    MOV2MASK(value, BITS_PREADETBW)
#define     GET_PREADETBW(reg_val)  MASK2VAL(reg_val, BITS_PREADETBW)
            /* PREADETBW default value
             */
#define     PREADETBW_DEFAULT  MOV2MASK(0x2, BITS_PREADETBW)

          /* [rw] Preamble detect threshold
           */
#define   BITS_PREAMBTHR  BITS16(3, 0)
            /* Correlation level for preamble detection. Compares receiver
             * samples to expected preamble pattern. Triggers frame processor change
             */
#define     SET_PREAMBTHR(value)    MOV2MASK(value, BITS_PREAMBTHR)
#define     GET_PREAMBTHR(reg_val)  MASK2VAL(reg_val, BITS_PREAMBTHR)
            /* PREAMBTHR default value
             */
#define     PREAMBTHR_DEFAULT  MOV2MASK(0xc, BITS_PREAMBTHR)

#define REG16_PRELUDE_DEFAULT  0x6AAC



/******************************************************************************/
/*                      Register PREAMB_SWLEN (16 bits)                       */
/******************************************************************************/
#define REG16_PREAMB_SWLEN 0x2F

          /* [rw] Sync word length
           */
#define   BITS_SWLENGTH  BITS16(7, 6)
            /* Delegated to Gabriel M.
             */
#define     SET_SWLENGTH(value)    MOV2MASK(value, BITS_SWLENGTH)
#define     GET_SWLENGTH(reg_val)  MASK2VAL(reg_val, BITS_SWLENGTH)
            /* SWLENGTH default value
             */
#define     SWLENGTH_DEFAULT  MOV2MASK(0x2, BITS_SWLENGTH)

          /* [rw] Preamble length
           */
#define   BITS_PREAMBLEN  BITS16(5, 0)
            /* Determines the number of preamble symbols in a frame using the
             * formula involving chips per symbol and chip multiplier. Influences frame
             * reception prediction and ranging
             */
#define     SET_PREAMBLEN(value)    MOV2MASK(value, BITS_PREAMBLEN)
#define     GET_PREAMBLEN(reg_val)  MASK2VAL(reg_val, BITS_PREAMBLEN)
            /* PREAMBLEN default value
             */
#define     PREAMBLEN_DEFAULT  MOV2MASK(0x4, BITS_PREAMBLEN)

#define REG16_PREAMB_SWLEN_DEFAULT  0x0084



/******************************************************************************/
/*                      Register SYNCWORD_15_0 (16 bits)                      */
/******************************************************************************/
#define REG16_SYNCWORD_15_0 0x30

          /* [rw] Sync word[15:0]
           */
#define   BITS_SYNCWORD_15_0  BITS16(15, 0)
            /* Contains the pattern for constructing the sync word in
             * transmitted and received frames, based on settings in CHIPREPET and SWLENGTH
             */
#define     SET_SYNCWORD_15_0(value)    MOV2MASK(value, BITS_SYNCWORD_15_0)
#define     GET_SYNCWORD_15_0(reg_val)  MASK2VAL(reg_val, BITS_SYNCWORD_15_0)
            /* SYNCWORD_15_0 default value
             */
#define     SYNCWORD_15_0_DEFAULT  MOV2MASK(0xc11d, BITS_SYNCWORD_15_0)

#define REG16_SYNCWORD_15_0_DEFAULT  0xC11D



/******************************************************************************/
/*                     Register SYNCWORD_31_16 (16 bits)                      */
/******************************************************************************/
#define REG16_SYNCWORD_31_16 0x31

          /* [rw] Sync word[31:16]
           */
#define   BITS_SYNCWORD_31_16  BITS16(15, 0)
            /* Contains the pattern for constructing the sync word in
             * transmitted and received frames, based on settings in CHIPREPET and SWLENGTH
             */
#define     SET_SYNCWORD_31_16(value)    MOV2MASK(value, BITS_SYNCWORD_31_16)
#define     GET_SYNCWORD_31_16(reg_val)  MASK2VAL(reg_val, BITS_SYNCWORD_31_16)
            /* SYNCWORD_31_16 default value
             */
#define     SYNCWORD_31_16_DEFAULT  MOV2MASK(0x5ea6, BITS_SYNCWORD_31_16)

#define REG16_SYNCWORD_31_16_DEFAULT  0x5EA6



/******************************************************************************/
/*                        Register CRC_15_1 (16 bits)                         */
/******************************************************************************/
#define REG16_CRC_15_1 0x32

          /* [rw] CRC_POLY[15:1]
           */
#define   BITS_CRC_POLY_15_1  BITS16(15, 1)
            /* Enables and defines CRC polynomial for frames. CRC remainder
             * length can be 13, 19, 25, or 31 bits. Choice affects error detection reliability
             * based on payload size and bit error rate
             */
#define     SET_CRC_POLY_15_1(value)    MOV2MASK(value, BITS_CRC_POLY_15_1)
#define     GET_CRC_POLY_15_1(reg_val)  MASK2VAL(reg_val, BITS_CRC_POLY_15_1)
            /* CRC_POLY_15_1 default value
             */
#define     CRC_POLY_15_1_DEFAULT  MOV2MASK(0x313a, BITS_CRC_POLY_15_1)

          /* [rc] 1
           */
#define   BIT_CONST  BIT(0)
            /* CONST default value
             */
#define     CONST_DEFAULT  MOV2MASK(0x1, BIT_CONST)

#define REG16_CRC_15_1_DEFAULT  0x6275



/******************************************************************************/
/*                        Register CRC_30_16 (16 bits)                        */
/******************************************************************************/
#define REG16_CRC_30_16 0x33

          /* [rw] CRC_POLY[30:16]
           */
#define   BITS_CRC_POLY_30_16  BITS16(14, 0)
            /* Enables and defines CRC polynomial for frames. CRC remainder
             * length can be 13, 19, 25, or 31 bits. Choice affects error detection reliability
             * based on payload size and bit error rate
             */
#define     SET_CRC_POLY_30_16(value)    MOV2MASK(value, BITS_CRC_POLY_30_16)
#define     GET_CRC_POLY_30_16(reg_val)  MASK2VAL(reg_val, BITS_CRC_POLY_30_16)
            /* CRC_POLY_30_16 default value
             */
#define     CRC_POLY_30_16_DEFAULT  MOV2MASK(0x372, BITS_CRC_POLY_30_16)

#define REG16_CRC_30_16_DEFAULT  0x0372



/******************************************************************************/
/*                   Register FRAMECFG_SAVETOBUF (16 bits)                    */
/******************************************************************************/
#define REG16_FRAMECFG_SAVETOBUF 0x34

          /* [rw] Frame address header enable and uses??
           */
#define   BITS_ADDRFIELD  BITS16(15, 14)
            /* The content of each frame address field received must either
             * match the reception address of this device or a valid broadcast address for its
             * sub-network
             */
#define     ADDRFIELD_0b11  MOV2MASK(0b11, BITS_ADDRFIELD)
            /* This device will not abort the reception process of any incoming
             * frame based on its frame address but can only sent auto-reply frames in response
             *
             */
#define     ADDRFIELD_0b10  MOV2MASK(0b10, BITS_ADDRFIELD)
            /* The content of frame address fields received isn't used by the
             * frame processor to make any decision
             */
#define     ADDRFIELD_0b01  MOV2MASK(0b01, BITS_ADDRFIELD)
            /* No address field is included in the header of any frame
             * transmitted or received
             */
#define     ADDRFIELD_0b00  MOV2MASK(0b00, BITS_ADDRFIELD)
            /* Get ADDRFIELD value
             */
#define     GET_ADDRFIELD(reg_val)  MASK2VAL(reg_val, BITS_ADDRFIELD)
            /* ADDRFIELD default value
             */
#define     ADDRFIELD_DEFAULT  MOV2MASK(0x0, BITS_ADDRFIELD)

          /* [rw] Frame address field length
           */
#define   BIT_ADDRLEN  BIT(13)
            /* Frame address fields is 16 bits
             */
#define     ADDRLEN_0b1  MOV2MASK(0b1, BIT_ADDRLEN)
            /* Frame address fields is 8 bits
             */
#define     ADDRLEN_0b0  MOV2MASK(0b0, BIT_ADDRLEN)
            /* Get ADDRLEN value
             */
#define     GET_ADDRLEN(reg_val)  MASK2VAL(reg_val, BIT_ADDRLEN)
            /* ADDRLEN default value
             */
#define     ADDRLEN_DEFAULT  MOV2MASK(0x0, BIT_ADDRLEN)

          /* [rw] Standalone retry count header enable
           */
#define   BIT_RETRYHDR  BIT(12)
            /* Includes retry count field in standalone frame header, aiding
             * analysis of retries and synchronization
             */
#define     RETRYHDR_0b1  MOV2MASK(0b1, BIT_RETRYHDR)
            /* Do not include the information in packet
             */
#define     RETRYHDR_0b0  MOV2MASK(0b0, BIT_RETRYHDR)
            /* Get RETRYHDR value
             */
#define     GET_RETRYHDR(reg_val)  MASK2VAL(reg_val, BIT_RETRYHDR)
            /* RETRYHDR default value
             */
#define     RETRYHDR_DEFAULT  MOV2MASK(0x0, BIT_RETRYHDR)

          /* [rw] Payload size header enable
           */
#define   BIT_SIZEHDR  BIT(11)
            /* Appends 8-bit payload size field in frame header, indicating data
             * size for variable length payloads
             */
#define     SIZEHDR_0b1  MOV2MASK(0b1, BIT_SIZEHDR)
            /* Do not include the information in packet
             */
#define     SIZEHDR_0b0  MOV2MASK(0b0, BIT_SIZEHDR)
            /* Get SIZEHDR value
             */
#define     GET_SIZEHDR(reg_val)  MASK2VAL(reg_val, BIT_SIZEHDR)
            /* SIZEHDR default value
             */
#define     SIZEHDR_DEFAULT  MOV2MASK(0x0, BIT_SIZEHDR)

          /* [rw] Payload size parity sub-field enable
           */
#define   BIT_SIZEPARI  BIT(10)
            /* Adds 5 parity bits to payload size fields in frames for enhanced
             * integrity and error detection
             */
#define     SIZEPARI_0b1  MOV2MASK(0b1, BIT_SIZEPARI)
            /* Does not add parity bits
             */
#define     SIZEPARI_0b0  MOV2MASK(0b0, BIT_SIZEPARI)
            /* Get SIZEPARI value
             */
#define     GET_SIZEPARI(reg_val)  MASK2VAL(reg_val, BIT_SIZEPARI)
            /* SIZEPARI default value
             */
#define     SIZEPARI_DEFAULT  MOV2MASK(0x0, BIT_SIZEPARI)

          /* [rw] Transmitted payload size source
           */
#define   BIT_SIZESRC  BIT(9)
            /* Uses transmission data buffer size for payload length in frames.
             * Streamlines variable payload transmission
             */
#define     SIZESRC_0b1  MOV2MASK(0b1, BIT_SIZESRC)
            /* Uses TXPKTSIZE for payload length
             */
#define     SIZESRC_0b0  MOV2MASK(0b0, BIT_SIZESRC)
            /* Get SIZESRC value
             */
#define     GET_SIZESRC(reg_val)  MASK2VAL(reg_val, BIT_SIZESRC)
            /* SIZESRC default value
             */
#define     SIZESRC_DEFAULT  MOV2MASK(0x0, BIT_SIZESRC)

          /* [rw] Optimize Ranging Data Sampling Time
           */
#define   BIT_OPTRANGI  BIT(8)
            /* Delegated to Gabriel M.
             */
#define     OPTRANGI_0b1  MOV2MASK(0b1, BIT_OPTRANGI)
            /* Delegated to Gabriel M.
             */
#define     OPTRANGI_0b0  MOV2MASK(0b0, BIT_OPTRANGI)
            /* Get OPTRANGI value
             */
#define     GET_OPTRANGI(reg_val)  MASK2VAL(reg_val, BIT_OPTRANGI)
            /* OPTRANGI default value
             */
#define     OPTRANGI_DEFAULT  MOV2MASK(0x0, BIT_OPTRANGI)

          /* [rw] Enable standalone sync time stamp
           */
#define   BIT_TIMFIELD  BIT(7)
            /* Includes standalone frame receiver sync time stamp field in
             * auto-reply frames. Enhances synchronization
             */
#define     TIMFIELD_0b1  MOV2MASK(0b1, BIT_TIMFIELD)
            /* Do not include the information in packet
             */
#define     TIMFIELD_0b0  MOV2MASK(0b0, BIT_TIMFIELD)
            /* Get TIMFIELD value
             */
#define     GET_TIMFIELD(reg_val)  MASK2VAL(reg_val, BIT_TIMFIELD)
            /* TIMFIELD default value
             */
#define     TIMFIELD_DEFAULT  MOV2MASK(0x0, BIT_TIMFIELD)

          /* [rw] Enable phase offset data
           */
#define   BIT_PHSFIELD  BIT(6)
            /* Includes phase offset data field in auto-reply frames after
             * payload. Facilitates efficient ranging applications
             */
#define     PHSFIELD_0b1  MOV2MASK(0b1, BIT_PHSFIELD)
            /* Do not include the information in packet
             */
#define     PHSFIELD_0b0  MOV2MASK(0b0, BIT_PHSFIELD)
            /* Get PHSFIELD value
             */
#define     GET_PHSFIELD(reg_val)  MASK2VAL(reg_val, BIT_PHSFIELD)
            /* PHSFIELD default value
             */
#define     PHSFIELD_DEFAULT  MOV2MASK(0x0, BIT_PHSFIELD)

          /* [rw] Save RSSI and antenna selection
           */
#define   BIT_SAVERSSI  BIT(5)
            /* Logs RSSI value and antenna selection in reception buffer.
             * Provides info on received signal strength and antenna choice
             */
#define     SAVERSSI_0b1  MOV2MASK(0b1, BIT_SAVERSSI)
            /* Do not save the information
             */
#define     SAVERSSI_0b0  MOV2MASK(0b0, BIT_SAVERSSI)
            /* Get SAVERSSI value
             */
#define     GET_SAVERSSI(reg_val)  MASK2VAL(reg_val, BIT_SAVERSSI)
            /* SAVERSSI default value
             */
#define     SAVERSSI_DEFAULT  MOV2MASK(0x0, BIT_SAVERSSI)

          /* [rw] Save receiver sync time stamp
           */
#define   BIT_SAVETIME  BIT(4)
            /* Records RXSYNTIME in reception buffer for received frames,
             * capturing receiver synchronization timing
             */
#define     SAVETIME_0b1  MOV2MASK(0b1, BIT_SAVETIME)
            /* Do not save the information
             */
#define     SAVETIME_0b0  MOV2MASK(0b0, BIT_SAVETIME)
            /* Get SAVETIME value
             */
#define     GET_SAVETIME(reg_val)  MASK2VAL(reg_val, BIT_SAVETIME)
            /* SAVETIME default value
             */
#define     SAVETIME_DEFAULT  MOV2MASK(0x0, BIT_SAVETIME)

          /* [rw] Save phase offset data
           */
#define   BIT_SAVEPHS  BIT(3)
            /* Stores PHASEDATA in reception buffer for received frames. Enables
             * phase offset data recording, essential for some applications
             */
#define     SAVEPHS_0b1  MOV2MASK(0b1, BIT_SAVEPHS)
            /* Do not save the information
             */
#define     SAVEPHS_0b0  MOV2MASK(0b0, BIT_SAVEPHS)
            /* Get SAVEPHS value
             */
#define     GET_SAVEPHS(reg_val)  MASK2VAL(reg_val, BIT_SAVEPHS)
            /* SAVEPHS default value
             */
#define     SAVEPHS_DEFAULT  MOV2MASK(0x0, BIT_SAVEPHS)

          /* [rw] Save frame address header
           */
#define   BIT_SAVEADDR  BIT(2)
            /* Saves frame address field in reception buffer for received
             * frames, enabling address extraction
             */
#define     SAVEADDR_0b1  MOV2MASK(0b1, BIT_SAVEADDR)
            /* Do not save the information
             */
#define     SAVEADDR_0b0  MOV2MASK(0b0, BIT_SAVEADDR)
            /* Get SAVEADDR value
             */
#define     GET_SAVEADDR(reg_val)  MASK2VAL(reg_val, BIT_SAVEADDR)
            /* SAVEADDR default value
             */
#define     SAVEADDR_DEFAULT  MOV2MASK(0x0, BIT_SAVEADDR)

          /* [rw] Save payload size header
           */
#define   BIT_SAVESIZE  BIT(1)
            /* Records payload size header in reception buffer, aiding payload
             * identification when multiple sizes are stored together
             */
#define     SAVESIZE_0b1  MOV2MASK(0b1, BIT_SAVESIZE)
            /* Do not save the information
             */
#define     SAVESIZE_0b0  MOV2MASK(0b0, BIT_SAVESIZE)
            /* Get SAVESIZE value
             */
#define     GET_SAVESIZE(reg_val)  MASK2VAL(reg_val, BIT_SAVESIZE)
            /* SAVESIZE default value
             */
#define     SAVESIZE_DEFAULT  MOV2MASK(0x0, BIT_SAVESIZE)

          /* [rw] Save CRC mismatch pattern
           */
#define   BIT_SAVECRC  BIT(0)
            /* Stores XOR of received and computed CRC remainders in reception
             * buffer
             */
#define     SAVECRC_0b1  MOV2MASK(0b1, BIT_SAVECRC)
            /* Do not save the information
             */
#define     SAVECRC_0b0  MOV2MASK(0b0, BIT_SAVECRC)
            /* Get SAVECRC value
             */
#define     GET_SAVECRC(reg_val)  MASK2VAL(reg_val, BIT_SAVECRC)
            /* SAVECRC default value
             */
#define     SAVECRC_DEFAULT  MOV2MASK(0x0, BIT_SAVECRC)

#define REG16_FRAMECFG_SAVETOBUF_DEFAULT  0x0000



/******************************************************************************/
/*                        Register RXADDRESS (16 bits)                        */
/******************************************************************************/
#define REG16_RXADDRESS 0x35

          /* [rw] Reception frame address
           */
#define   BITS_RXADDRESS  BITS16(15, 0)
            /* Defines device's reception address. Compared with received
             * frame's address field for decisions, interrupts, and events
             */
#define     SET_RXADDRESS(value)    MOV2MASK(value, BITS_RXADDRESS)
#define     GET_RXADDRESS(reg_val)  MASK2VAL(reg_val, BITS_RXADDRESS)
            /* RXADDRESS default value
             */
#define     RXADDRESS_DEFAULT  MOV2MASK(0xaddb, BITS_RXADDRESS)

#define REG16_RXADDRESS_DEFAULT  0xADDB



/******************************************************************************/
/*                        Register TXADDRESS (16 bits)                        */
/******************************************************************************/
#define REG16_TXADDRESS 0x36

          /* [rw] Transmitted frame address
           */
#define   BITS_TXADDRESS  BITS16(15, 0)
            /* Sets address in transmitted frames (except auto-reply with
             * RPLYADDR)
             */
#define     SET_TXADDRESS(value)    MOV2MASK(value, BITS_TXADDRESS)
#define     GET_TXADDRESS(reg_val)  MASK2VAL(reg_val, BITS_TXADDRESS)
            /* TXADDRESS default value
             */
#define     TXADDRESS_DEFAULT  MOV2MASK(0xadda, BITS_TXADDRESS)

#define REG16_TXADDRESS_DEFAULT  0xADDA



/******************************************************************************/
/*                      Register RX_TX_SIZEREG (16 bits)                      */
/******************************************************************************/
#define REG16_RX_TX_SIZEREG 0x37

          /* [rw] Transmitted packet size
           */
#define   BITS_TXPKTSIZE  BITS16(15, 8)
            /* Sets payload bytes for transmitted frames
             */
#define     SET_TXPKTSIZE(value)    MOV2MASK(value, BITS_TXPKTSIZE)
#define     GET_TXPKTSIZE(reg_val)  MASK2VAL(reg_val, BITS_TXPKTSIZE)
            /* TXPKTSIZE default value
             */
#define     TXPKTSIZE_DEFAULT  MOV2MASK(0x10, BITS_TXPKTSIZE)

          /* [rw] Received packet size
           */
#define   BITS_RXPKTSIZE  BITS16(7, 0)
            /* Defines payload bytes to decode
             */
#define     SET_RXPKTSIZE(value)    MOV2MASK(value, BITS_RXPKTSIZE)
#define     GET_RXPKTSIZE(reg_val)  MASK2VAL(reg_val, BITS_RXPKTSIZE)
            /* RXPKTSIZE default value
             */
#define     RXPKTSIZE_DEFAULT  MOV2MASK(0x10, BITS_RXPKTSIZE)

#define REG16_RX_TX_SIZEREG_DEFAULT  0x1010



/******************************************************************************/
/*                          Register FIFOS (8 bits)                           */
/******************************************************************************/
#define REG8_FIFOS 0x38

          /* [rw] Transmission data buffer
           */
#define   BITS_TXBUFFER  BITS8(7, 0)
            /* TXBUFFER default value
             */
#define     TXBUFFER_DEFAULT  MOV2MASK(0x0, BITS_TXBUFFER)

          /* [ro] Reception data buffer
           */
#define   BITS_RXBUFFER  BITS8(7, 0)
            /* RXBUFFER default value
             */
#define     RXBUFFER_DEFAULT  MOV2MASK(0, BITS_RXBUFFER)

#define REG8_FIFOS_DEFAULT  0x0000



/******************************************************************************/
/*                        Register RXBUFLOAD (8 bits)                         */
/******************************************************************************/
#define REG8_RXBUFLOAD 0x39

          /* [ro] Reception data buffer load
           */
#define   BITS_RXBUFLOAD  BITS8(7, 0)
            /* Returns bytes stored in reception buffer
             */
#define     GET_RXBUFLOAD(reg_val)  MASK2VAL(reg_val, BITS_RXBUFLOAD)
            /* RXBUFLOAD default value
             */
#define     RXBUFLOAD_DEFAULT  MOV2MASK(0x0, BITS_RXBUFLOAD)

#define REG8_RXBUFLOAD_DEFAULT  0x0000



/******************************************************************************/
/*                        Register TXBUFLOAD (8 bits)                         */
/******************************************************************************/
#define REG8_TXBUFLOAD 0x3A

          /* [ro] Transmission data buffer load
           */
#define   BITS_TXBUFLOAD  BITS8(7, 0)
            /* Returns bytes in transmission buffer
             */
#define     GET_TXBUFLOAD(reg_val)  MASK2VAL(reg_val, BITS_TXBUFLOAD)
            /* TXBUFLOAD default value
             */
#define     TXBUFLOAD_DEFAULT  MOV2MASK(0x0, BITS_TXBUFLOAD)

#define REG8_TXBUFLOAD_DEFAULT  0x0000



/******************************************************************************/
/*                         Register ACTIONS (8 bits)                          */
/******************************************************************************/
#define REG8_ACTIONS 0x3B

          /* [rw] Automatic transmission
           */
#define   BIT_AUTOTX  BIT(7)
            /* STARTTX set by wake-up event, transmits frame when in
             * 'transmission mode'. Useful for periodic wake-up transmissions with preset data
             *
             */
#define     AUTOTX_0b1  MOV2MASK(0b1, BIT_AUTOTX)
            /* Requires STARTTX, RELOADTX, or conditions for auto-reply
             */
#define     AUTOTX_0b0  MOV2MASK(0b0, BIT_AUTOTX)
            /* Get AUTOTX value
             */
#define     GET_AUTOTX(reg_val)  MASK2VAL(reg_val, BIT_AUTOTX)
            /* AUTOTX default value
             */
#define     AUTOTX_DEFAULT  MOV2MASK(0x0, BIT_AUTOTX)

          /* [wo] Split buffer for dual payloads
           */
#define   BIT_SPLITTX  BIT(6)
            /* With STARTTX allows two different payloads in buffer for
             * auto-reply and standalone frames
             */
#define     SPLITTX_0b1  MOV2MASK(0b1, BIT_SPLITTX)
            /* Don not split payload
             */
#define     SPLITTX_0b0  MOV2MASK(0b0, BIT_SPLITTX)
            /* SPLITTX default value
             */
#define     SPLITTX_DEFAULT  MOV2MASK(0x0, BIT_SPLITTX)

          /* [wo] Reload buffer and restart transmission
           */
#define   BIT_RELOADTX  BIT(5)
            /* Loads buffer's 'shadow' pointer to read pointer for easy
             * reloading and retransmission
             */
#define     RELOADTX_0b1  MOV2MASK(0b1, BIT_RELOADTX)
            /* Use buffer's pointer
             */
#define     RELOADTX_0b0  MOV2MASK(0b0, BIT_RELOADTX)
            /* RELOADTX default value
             */
#define     RELOADTX_DEFAULT  MOV2MASK(0x0, BIT_RELOADTX)

          /* [wo] Start transmission
           */
#define   BIT_STARTTX  BIT(4)
            /* Schedules frame transmission when processor is awake, idle, and
             * in 'transmission mode'. Also clears DOUBLETX
             */
#define     STARTTX_0b1  MOV2MASK(0b1, BIT_STARTTX)
            /* Can't unset signal
             */
#define     STARTTX_0b0  MOV2MASK(0b0, BIT_STARTTX)
            /* STARTTX default value
             */
#define     STARTTX_DEFAULT  MOV2MASK(0x0, BIT_STARTTX)

          /* [wo] Flush and reset transmission buffer
           */
#define   BIT_FLUSHTX  BIT(3)
            /* Resets transmission buffer state, clears hidden signals STARTTX,
             * DOUBLETX, overflow/underflow flags
             */
#define     FLUSHTX_0b1  MOV2MASK(0b1, BIT_FLUSHTX)
            /* No effect
             */
#define     FLUSHTX_0b0  MOV2MASK(0b0, BIT_FLUSHTX)
            /* FLUSHTX default value
             */
#define     FLUSHTX_DEFAULT  MOV2MASK(0x1, BIT_FLUSHTX)

          /* [wo] Flush and reset reception buffer
           */
#define   BIT_FLUSHRX  BIT(2)
            /* Resets reception buffer state, clears overflow/underflow flags
             */
#define     FLUSHRX_0b1  MOV2MASK(0b1, BIT_FLUSHRX)
            /* No effect
             */
#define     FLUSHRX_0b0  MOV2MASK(0b0, BIT_FLUSHRX)
            /* FLUSHRX default value
             */
#define     FLUSHRX_DEFAULT  MOV2MASK(0x1, BIT_FLUSHRX)

          /* [wo] Initialize wake-up timers
           */
#define   BIT_INITIMER  BIT(1)
            /* Resets both timers to 0
             */
#define     INITIMER_0b1  MOV2MASK(0b1, BIT_INITIMER)
            /* Do not reset both timers
             */
#define     INITIMER_0b0  MOV2MASK(0b0, BIT_INITIMER)
            /* INITIMER default value
             */
#define     INITIMER_DEFAULT  MOV2MASK(0x1, BIT_INITIMER)

          /* [wo] Sleep mode control
           */
#define   BIT_SLEEP  BIT(0)
            /* Puts device to sleep if awake and idle
             */
#define     SLEEP_0b1  MOV2MASK(0b1, BIT_SLEEP)
            /* Wakes up device if asleep and AUTOWAKE is cleared
             */
#define     SLEEP_0b0  MOV2MASK(0b0, BIT_SLEEP)
            /* SLEEP default value
             */
#define     SLEEP_DEFAULT  MOV2MASK(0x1, BIT_SLEEP)

          /* [ro] Double payload transmission enabled
           */
#define   BIT_DOUBLETX  BIT(6)
            /* This is a sticky bit. Double payload transmission enabled???
             */
#define     GET_DOUBLETX(reg_val)  MASK2VAL(reg_val, BIT_DOUBLETX)
            /* DOUBLETX default value
             */
#define     DOUBLETX_DEFAULT  MOV2MASK(0, BIT_DOUBLETX)

          /* [ro] Standalone transmission pending
           */
#define   BIT_TXPENDIN  BIT(4)
            /* This is a sticky bit. When using hidden STARTTX signal
             */
#define     GET_TXPENDIN(reg_val)  MASK2VAL(reg_val, BIT_TXPENDIN)
            /* TXPENDIN default value
             */
#define     TXPENDIN_DEFAULT  MOV2MASK(0, BIT_TXPENDIN)

          /* [ro] Last standalone transmission's CCA failures
           */
#define   BITS_TXRETRIES  BITS8(3, 0)
            /* Holds failed CCA test count before last standalone frame attempt.
             * Incremented during CCA with strong interference, cleared by STARTTX, RELOADTX,
             * or AUTOTX. Can overflow if MAXRETRY is too small
             */
#define     GET_TXRETRIES(reg_val)  MASK2VAL(reg_val, BITS_TXRETRIES)
            /* TXRETRIES default value
             */
#define     TXRETRIES_DEFAULT  MOV2MASK(0, BITS_TXRETRIES)

#define REG8_ACTIONS_DEFAULT  0x000F



/******************************************************************************/
/*                       Register POWER_STATE (8 bits)                        */
/******************************************************************************/
#define REG8_POWER_STATE 0x3C

          /* [ro] Transmitting/Receiving frame
           */
#define   BIT_INFRAME  BIT(7)
            /* This is a sticky bit. '1' during frame transmission/reception
             * after synchronization word
             */
#define     GET_INFRAME(reg_val)  MASK2VAL(reg_val, BIT_INFRAME)
            /* INFRAME default value
             */
#define     INFRAME_DEFAULT  MOV2MASK(0x0, BIT_INFRAME)

          /* [ro] Transmitter enabled
           */
#define   BIT_TX_EN  BIT(6)
            /* This is a sticky bit. '1' if transmitter powered up
             */
#define     GET_TX_EN(reg_val)  MASK2VAL(reg_val, BIT_TX_EN)
            /* TX_EN default value
             */
#define     TX_EN_DEFAULT  MOV2MASK(0x0, BIT_TX_EN)

          /* [ro] Receiver enabled
           */
#define   BIT_RX_EN  BIT(5)
            /* This is a sticky bit. '1' if receiver powered up
             */
#define     GET_RX_EN(reg_val)  MASK2VAL(reg_val, BIT_RX_EN)
            /* RX_EN default value
             */
#define     RX_EN_DEFAULT  MOV2MASK(0x0, BIT_RX_EN)

          /* [ro] Fully awake
           */
#define   BIT_AWAKE  BIT(4)
            /* This is a sticky bit. '1' if frame processor awake, regardless of
             * activity. Matches delay line circuitry state
             */
#define     GET_AWAKE(reg_val)  MASK2VAL(reg_val, BIT_AWAKE)
            /* AWAKE default value
             */
#define     AWAKE_DEFAULT  MOV2MASK(0x0, BIT_AWAKE)

          /* [ro] Frame processor powered on
           */
#define   BIT_PROC_ON  BIT(3)
            /* This is a sticky bit. '1' if frame processor powered and reset
             * released
             */
#define     GET_PROC_ON(reg_val)  MASK2VAL(reg_val, BIT_PROC_ON)
            /* PROC_ON default value
             */
#define     PROC_ON_DEFAULT  MOV2MASK(0x0, BIT_PROC_ON)

          /* [ro] DC/DC voltage converter enabled
           */
#define   BIT_DCDC_EN  BIT(2)
            /* This is a sticky bit. '1' if DC/DC converter enabled
             */
#define     GET_DCDC_EN(reg_val)  MASK2VAL(reg_val, BIT_DCDC_EN)
            /* DCDC_EN default value
             */
#define     DCDC_EN_DEFAULT  MOV2MASK(0x0, BIT_DCDC_EN)

          /* [ro] Phase-locked loop enabled
           */
#define   BIT_PLL_EN  BIT(1)
            /* This is a sticky bit. '1' if PLL powered up
             */
#define     GET_PLL_EN(reg_val)  MASK2VAL(reg_val, BIT_PLL_EN)
            /* PLL_EN default value
             */
#define     PLL_EN_DEFAULT  MOV2MASK(0x0, BIT_PLL_EN)

          /* [ro] Reference buffers enabled
           */
#define   BIT_REF_EN  BIT(0)
            /* This is a sticky bit. '1' if voltage/current reference buffers
             * powered up
             */
#define     GET_REF_EN(reg_val)  MASK2VAL(reg_val, BIT_REF_EN)
            /* REF_EN default value
             */
#define     REF_EN_DEFAULT  MOV2MASK(0x0, BIT_REF_EN)

#define REG8_POWER_STATE_DEFAULT  0x0000



/******************************************************************************/
/*                      Register SUPPLY_MONITOR (8 bits)                      */
/******************************************************************************/
#define REG8_SUPPLY_MONITOR 0x3D

          /* [ro] Supply voltage level
           */
#define   BITS_VSUPPLY  BITS8(2, 0)
            /* Reflects voltage range based on ADC code
             */
#define     GET_VSUPPLY(reg_val)  MASK2VAL(reg_val, BITS_VSUPPLY)
            /* VSUPPLY default value
             */
#define     VSUPPLY_DEFAULT  MOV2MASK(0x0, BITS_VSUPPLY)

#define REG8_SUPPLY_MONITOR_DEFAULT  0x0000



/******************************************************************************/
/*                       Register DCRO_CONFIG (8 bits)                        */
/******************************************************************************/
#define REG8_DCRO_CONFIG 0x3E

          /* [wo] Trigger DCRO frequency measurement
           */
#define   BITS_COUNTCODE  BITS8(5, 0)
            /* Enables DCRO for 48.828125 ns to measure frequency. Avoid during
             * transmitter/receiver operation
             */
#define     SET_COUNTCODE(value)  MOV2MASK(value, BITS_COUNTCODE)
            /* COUNTCODE default value
             */
#define     COUNTCODE_DEFAULT  MOV2MASK(0x0, BITS_COUNTCODE)

          /* [ro] DCRO frequency code's effective frequency
           */
#define   BITS_CODEFREQ  BITS8(7, 0)
            /* Returns 8-bit value representing DCRO oscillation frequency
             * measurement in the range of 5242.88 MHz to 10465.28 MHz
             */
#define     GET_CODEFREQ(reg_val)  MASK2VAL(reg_val, BITS_CODEFREQ)
            /* CODEFREQ default value
             */
#define     CODEFREQ_DEFAULT  MOV2MASK(0, BITS_CODEFREQ)

#define REG8_DCRO_CONFIG_DEFAULT  0x0000



/******************************************************************************/
/*                           Register NVM (8 bits)                            */
/******************************************************************************/
#define REG8_NVM 0x3F

          /* [wo] ROM power switch
           */
#define   BIT_ROMPWRSW  BIT(7)
            /* Enables ROM
             */
#define     ROMPWRSW_0b1  MOV2MASK(0b1, BIT_ROMPWRSW)
            /* Disables ROM
             */
#define     ROMPWRSW_0b0  MOV2MASK(0b0, BIT_ROMPWRSW)
            /* ROMPWRSW default value
             */
#define     ROMPWRSW_DEFAULT  MOV2MASK(0x0, BIT_ROMPWRSW)

          /* [wo] ROM byte address
           */
#define   BITS_ROM_ADDR  BITS8(6, 0)
            /* Sets ROM address to read, requires core voltage and ROMPWRSW =
             * '1'
             */
#define     SET_ROM_ADDR(value)  MOV2MASK(value, BITS_ROM_ADDR)
            /* ROM_ADDR default value
             */
#define     ROM_ADDR_DEFAULT  MOV2MASK(0x0, BITS_ROM_ADDR)

          /* [ro] ROM output byte
           */
#define   BITS_ROM_BYTE  BITS8(7, 0)
            /* Reflects current ROM module output value, invalid if module off
             * or not addressed
             */
#define     GET_ROM_BYTE(reg_val)  MASK2VAL(reg_val, BITS_ROM_BYTE)
            /* ROM_BYTE default value
             */
#define     ROM_BYTE_DEFAULT  MOV2MASK(0, BITS_ROM_BYTE)

#define REG8_NVM_DEFAULT  0x0000



#endif /* SR1120_V3_H_ */

// clang-format on
