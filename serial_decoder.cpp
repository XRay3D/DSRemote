/*
***************************************************************************
*
* Author: Teunis van Beelen
*
* Copyright (C) 2016 - 2020 Teunis van Beelen
*
* Email: teuniz@protonmail.com
*
***************************************************************************
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************
*
* RS-232 protocol:
*
* settings in this example: 8N1
*
* character value in this example: 'G', 71, 0x47, 0b01000111
*
*
*
*    idle                   start   1     1     1     0     0     0     1     0   stop            idle
*                            bit                                                   bit
* -------------------------+     +-----+-----+-----+                 +-----+     +-----+-----------------------------
*             +12V         |     |                 |                 |     |     |
*                          |     |                 |                 |     |     |
*                          |     |                 |                 |     |     |
*                          |     |                 |                 |     |     |
*                    -12V  +-----+                 +-----+-----+-----+     +-----+
*
*                                   0     1     2     3     4     5     6     7
*
*                                  LSB                                       MSB
*
*
* default (positive )line RS-232 LSB first (little endian)
*
* negative: everything inverted, used by microcontrollers, TTL/CMOS level
*
***************************************************************************
*/

#include "mainwindow.h"

void UiMainWindow::serialDecoder(struct DeviceSettings* d_parms) {
    int i, j, threshold[MAX_CHNS], uart_tx_start, uart_tx_data_bit, uart_rx_start, uart_rx_data_bit,
        uart_parity_bit, uart_parity, spi_data_mosi_bit, spi_data_miso_bit,
        spi_mosi_bit0_pos = 0, spi_miso_bit0_pos = 0, spi_clk_new, spi_clk_old, spi_chars = 1,
        spi_timeout, spi_timeout_cntr, stop_bit_error;

    unsigned int uart_val = 0, spi_mosi_val = 0, spi_miso_val = 0;

    short s_max, s_min;

    double uart_sample_per_bit, uart_tx_x_pos, uart_rx_x_pos, bit_per_volt;

    d_parms->mathDecodeUartTxNval = 0;

    d_parms->mathDecodeUartRxNval = 0;

    d_parms->mathDecodeSpiMosiNval = 0;

    d_parms->mathDecodeSpiMisoNval = 0;

    if(d_parms->waveBufsz < 32)
        return;

    if(d_parms->mathDecodeThresholdAuto) {
        for(j = 0; j < MAX_CHNS; j++) {
            if(!d_parms->chan[j].Display)
                continue;

            s_max = -32768;
            s_min = 32767;

            for(int i{}; i < d_parms->waveBufsz; i++) {
                if(d_parms->waveBuf[j][i] > s_max)
                    s_max = d_parms->waveBuf[j][i];
                if(d_parms->waveBuf[j][i] < s_min)
                    s_min = d_parms->waveBuf[j][i];
            }

            threshold[j] = (s_max + s_min) / 2;
        }
    } else if(d_parms->mathDecodeMode == DECODE_MODE_UART) {
        if(d_parms->mathDecodeUartTx) {
            if(d_parms->modelSerie == 6) {
                bit_per_volt = 32.0 / d_parms->chan[d_parms->mathDecodeUartTx - 1].scale;

                threshold[d_parms->mathDecodeUartTx - 1]
                    = (d_parms->mathDecodeThresholdUartTx
                          + d_parms->chan[d_parms->mathDecodeUartTx - 1].offset)
                    * bit_per_volt;
            } else {
                bit_per_volt = 25.0 / d_parms->chan[d_parms->mathDecodeUartTx - 1].scale;

                threshold[d_parms->mathDecodeUartTx - 1]
                    = (d_parms->chan[d_parms->mathDecodeUartTx - 1].mathDecodeThreshold
                          + d_parms->chan[d_parms->mathDecodeUartTx - 1].offset)
                    * bit_per_volt;
            }
        }

        if(d_parms->mathDecodeUartRx) {
            if(d_parms->modelSerie == 6) {
                bit_per_volt = 32.0 / d_parms->chan[d_parms->mathDecodeUartRx - 1].scale;

                threshold[d_parms->mathDecodeUartRx - 1]
                    = (d_parms->mathDecodeThresholdUartRx
                          + d_parms->chan[d_parms->mathDecodeUartRx - 1].offset)
                    * bit_per_volt;
            } else {
                bit_per_volt = 25.0 / d_parms->chan[d_parms->mathDecodeUartRx - 1].scale;

                threshold[d_parms->mathDecodeUartRx - 1]
                    = (d_parms->chan[d_parms->mathDecodeUartRx - 1].mathDecodeThreshold
                          + d_parms->chan[d_parms->mathDecodeUartRx - 1].offset)
                    * bit_per_volt;
            }
        }
    } else if(d_parms->mathDecodeMode == DECODE_MODE_SPI) {
        if(d_parms->modelSerie == 6) {
            bit_per_volt = 32.0 / d_parms->chan[d_parms->mathDecodeSpiClk].scale;

            threshold[d_parms->mathDecodeSpiClk]
                = (d_parms->chan[2].mathDecodeThreshold
                      + d_parms->chan[d_parms->mathDecodeSpiClk].offset)
                * bit_per_volt;
        } else {
            bit_per_volt = 25.0 / d_parms->chan[d_parms->mathDecodeSpiClk].scale;

            threshold[d_parms->mathDecodeSpiClk]
                = (d_parms->chan[d_parms->mathDecodeSpiClk].mathDecodeThreshold
                      + d_parms->chan[d_parms->mathDecodeSpiClk].offset)
                * bit_per_volt;
        }

        if(d_parms->mathDecodeSpiMosi) {
            if(d_parms->modelSerie == 6) {
                bit_per_volt = 32.0 / d_parms->chan[d_parms->mathDecodeSpiMosi - 1].scale;

                threshold[d_parms->mathDecodeSpiMosi - 1]
                    = (d_parms->chan[1].mathDecodeThreshold
                          + d_parms->chan[d_parms->mathDecodeSpiMosi - 1].offset)
                    * bit_per_volt;
            } else {
                bit_per_volt = 25.0 / d_parms->chan[d_parms->mathDecodeSpiMosi - 1].scale;

                threshold[d_parms->mathDecodeSpiMosi - 1]
                    = (d_parms->chan[d_parms->mathDecodeSpiMosi - 1].mathDecodeThreshold
                          + d_parms->chan[d_parms->mathDecodeSpiMosi - 1].offset)
                    * bit_per_volt;
            }
        }

        if(d_parms->mathDecodeSpiMiso) {
            if(d_parms->modelSerie == 6) {
                bit_per_volt = 32.0 / d_parms->chan[d_parms->mathDecodeSpiMiso - 1].scale;

                threshold[d_parms->mathDecodeSpiMiso - 1]
                    = (d_parms->chan[0].mathDecodeThreshold
                          + d_parms->chan[d_parms->mathDecodeSpiMiso - 1].offset)
                    * bit_per_volt;
            } else {
                bit_per_volt = 25.0 / d_parms->chan[d_parms->mathDecodeSpiMiso - 1].scale;

                threshold[d_parms->mathDecodeSpiMiso - 1]
                    = (d_parms->chan[d_parms->mathDecodeSpiMiso - 1].mathDecodeThreshold
                          + d_parms->chan[d_parms->mathDecodeSpiMiso - 1].offset)
                    * bit_per_volt;
            }
        }

        if(d_parms->mathDecodeSpiCs) {
            if(d_parms->modelSerie == 6) {
                bit_per_volt = 32.0 / d_parms->chan[d_parms->mathDecodeSpiCs - 1].scale;

                threshold[d_parms->mathDecodeSpiCs - 1]
                    = (d_parms->chan[3].mathDecodeThreshold
                          + d_parms->chan[d_parms->mathDecodeSpiCs - 1].offset)
                    * bit_per_volt;
            } else {
                bit_per_volt = 25.0 / d_parms->chan[d_parms->mathDecodeSpiCs - 1].scale;

                threshold[d_parms->mathDecodeSpiCs - 1]
                    = (d_parms->chan[d_parms->mathDecodeSpiCs - 1].mathDecodeThreshold
                          + d_parms->chan[d_parms->mathDecodeSpiCs - 1].offset)
                    * bit_per_volt;
            }
        }
    }

    if(d_parms->mathDecodeMode == DECODE_MODE_UART) {
        d_parms->mathDecodeUartTxNval = 0;

        d_parms->mathDecodeUartRxNval = 0;

        if(d_parms->waveMemViewEnabled)
            uart_sample_per_bit = d_parms->samplerate / (double)d_parms->mathDecodeUartBaud;
        else if(d_parms->timebasedelayenable)
            uart_sample_per_bit = (100.0 / d_parms->timebasedelayscale)
                / (double)d_parms->mathDecodeUartBaud;
        else
            uart_sample_per_bit = (100.0 / d_parms->timebasescale)
                / (double)d_parms->mathDecodeUartBaud;

        if(uart_sample_per_bit < 3)
            return;

        uart_tx_start = 0;

        uart_tx_data_bit = 0;

        uart_tx_x_pos = 1;

        uart_rx_start = 0;

        uart_rx_data_bit = 0;

        uart_rx_x_pos = 1;

        //     if(d_parms->modelserie == 6)
        //     {
        //       printf("chanscale: %f\n"
        //             "chanoffset: %f\n"
        //             "math_decode_threshold_uart_tx: %f\n"
        //             "math_decode_uart_tx: %i\n"
        //             "threshold: %i\n"
        //             "uart_sample_per_bit: %f\n"
        //             "waveBufsz: %i\n",
        //             d_parms->chan[d_parms->math_decode_uart_tx - 1].scale,
        //             d_parms->chan[d_parms->math_decode_uart_tx - 1].offset,
        //             d_parms->math_decode_threshold_uart_tx,
        //             d_parms->math_decode_uart_tx,
        //             threshold[d_parms->math_decode_uart_tx - 1],
        //             uart_sample_per_bit,
        //             d_parms->waveBufsz);
        //     }
        //     else
        //     {
        //
        //       printf("chanscale: %f\n"
        //             "chanoffset: %f\n"
        //             "math_decode_threshold: %f\n"
        //             "math_decode_uart_tx: %i\n"
        //             "threshold: %i\n"
        //             "uart_sample_per_bit: %f\n"
        //             "waveBufsz: %i\n",
        //             d_parms->chan[d_parms->math_decode_uart_tx - 1].scale,
        //             d_parms->chan[d_parms->math_decode_uart_tx - 1].offset,
        //             d_parms->math_decode_threshold[d_parms->math_decode_uart_tx - 1],
        //             d_parms->math_decode_uart_tx,
        //             threshold[d_parms->math_decode_uart_tx - 1],
        //             uart_sample_per_bit,
        //             d_parms->waveBufsz);
        //     }

        if(d_parms->mathDecodeUartTx) {
            if(d_parms->chan[d_parms->mathDecodeUartTx - 1].Display) // don't try to decode if channel isn't enabled...
            {
                for(i = 1; i < d_parms->waveBufsz; i++) {
                    if(d_parms->mathDecodeUartTxNval >= DECODE_MAX_CHARS)
                        break;

                    if(!uart_tx_start) {
                        if(d_parms->mathDecodeUartPol) // positive, line level RS-232
                        {
                            if(d_parms->modelSerie == 6) {
                                if(d_parms->waveBuf[d_parms->mathDecodeUartTx - 1][i - 1]
                                    >= d_parms->mathDecodeThresholdUartTx) {
                                    if(d_parms->waveBuf[d_parms->mathDecodeUartTx - 1][i]
                                        < d_parms->mathDecodeThresholdUartTx) {
                                        uart_tx_start = 1;

                                        uart_val = 0;

                                        uart_tx_x_pos = (uart_sample_per_bit * 1.5) + i;

                                        i = uart_tx_x_pos - 1;
                                    }
                                }
                            } else // modelserie = 1, 2 or 4
                                if(d_parms->waveBuf[d_parms->mathDecodeUartTx - 1][i - 1]
                                    >= threshold[d_parms->mathDecodeUartTx - 1]) {
                                    if(d_parms->waveBuf[d_parms->mathDecodeUartTx - 1][i]
                                        < threshold[d_parms->mathDecodeUartTx - 1]) {
                                        uart_tx_start = 1;

                                        uart_val = 0;

                                        uart_tx_x_pos = (uart_sample_per_bit * 1.5) + i;

                                        i = uart_tx_x_pos - 1;
                                    }
                                }
                        } else // negative, cpu level TTL/CMOS
                            if(d_parms->modelSerie == 6) {
                                if(d_parms->waveBuf[d_parms->mathDecodeUartTx - 1][i - 1]
                                    < d_parms->mathDecodeThresholdUartTx) {
                                    if(d_parms->waveBuf[d_parms->mathDecodeUartTx - 1][i]
                                        >= d_parms->mathDecodeThresholdUartTx) {
                                        uart_tx_start = 1;

                                        uart_val = 0;

                                        uart_tx_x_pos = (uart_sample_per_bit * 1.5) + i;

                                        i = uart_tx_x_pos - 1;
                                    }
                                }
                            } else // modelserie = 1, 2 or 4
                                if(d_parms->waveBuf[d_parms->mathDecodeUartTx - 1][i - 1]
                                    < threshold[d_parms->mathDecodeUartTx - 1]) {
                                    if(d_parms->waveBuf[d_parms->mathDecodeUartTx - 1][i]
                                        >= threshold[d_parms->mathDecodeUartTx - 1]) {
                                        uart_tx_start = 1;

                                        uart_val = 0;

                                        uart_tx_x_pos = (uart_sample_per_bit * 1.5) + i;

                                        i = uart_tx_x_pos - 1;
                                    }
                                }
                    } else // uart_rx_start != 0
                    {
                        if(d_parms->modelSerie == 6) {
                            if(d_parms->waveBuf[d_parms->mathDecodeUartTx - 1][i]
                                >= d_parms->mathDecodeThresholdUartTx)
                                uart_val += (1 << uart_tx_data_bit);
                        } else // modelserie = 1, 2 or 4
                        {
                            if(d_parms->waveBuf[d_parms->mathDecodeUartTx - 1][i]
                                >= threshold[d_parms->mathDecodeUartTx - 1])
                                uart_val += (1 << uart_tx_data_bit);
                        }

                        if(++uart_tx_data_bit == d_parms->mathDecodeUartWidth) {
                            if((d_parms->mathDecodeUartEnd)
                                && (d_parms->mathDecodeFormat != 4)) // big endian?
                            {
                                uart_val = reverseBitOrder8(uart_val);

                                uart_val >>= (8 - uart_tx_data_bit);
                            }

                            if(!d_parms->mathDecodeUartPol) // positive, line level RS-232 or negative, cpu level TTL/CMOS?
                            {
                                uart_val = ~uart_val;

                                uart_val &= (0xff >> (8 - uart_tx_data_bit));
                            }

                            d_parms->mathDecodeUartTxVal[d_parms->mathDecodeUartTxNval]
                                = uart_val;

                            d_parms->mathDecodeUartTxValPos[d_parms->mathDecodeUartTxNval]
                                = i - (uart_tx_data_bit * uart_sample_per_bit)
                                + (0.5 * uart_sample_per_bit);

                            uart_tx_data_bit = 0;

                            uart_tx_start = 0;

                            d_parms->mathDecodeUartTxErr[d_parms->mathDecodeUartTxNval] = 0;

                            if(d_parms->mathDecodeUartPar) {
                                uart_tx_x_pos += uart_sample_per_bit;

                                i = uart_tx_x_pos;

                                if(i < d_parms->waveBufsz) {
                                    if(d_parms->modelSerie == 6) {
                                        if(d_parms->waveBuf[d_parms->mathDecodeUartTx - 1][i]
                                            >= d_parms->mathDecodeThresholdUartTx)
                                            if(d_parms->mathDecodeUartPol)
                                                uart_parity_bit = 1;
                                            else
                                                uart_parity_bit = 0;
                                        else if(d_parms->mathDecodeUartPol)
                                            uart_parity_bit = 0;
                                        else
                                            uart_parity_bit = 1;
                                    } else {
                                        if(d_parms->waveBuf[d_parms->mathDecodeUartTx - 1][i]
                                            >= threshold[d_parms->mathDecodeUartTx - 1])
                                            if(d_parms->mathDecodeUartPol)
                                                uart_parity_bit = 1;
                                            else
                                                uart_parity_bit = 0;
                                        else if(d_parms->mathDecodeUartPol)
                                            uart_parity_bit = 0;
                                        else
                                            uart_parity_bit = 1;
                                    }

                                    for(j = 0, uart_parity = 0;
                                        j < d_parms->mathDecodeUartWidth;
                                        j++)
                                        uart_parity += ((uart_val >> j) & 1);

                                    if(d_parms->mathDecodeUartPar & 1)
                                        uart_parity++;

                                    if((uart_parity & 1) != uart_parity_bit)
                                        d_parms
                                            ->mathDecodeUartTxErr[d_parms->mathDecodeUartTxNval]
                                            = 1;
                                }
                            }

                            uart_tx_x_pos += uart_sample_per_bit;

                            i = uart_tx_x_pos;

                            stop_bit_error = 0; // check stop bit

                            if(i < d_parms->waveBufsz) {
                                if(d_parms->modelSerie == 6) {
                                    if(d_parms->waveBuf[d_parms->mathDecodeUartTx - 1][i]
                                        >= d_parms->mathDecodeThresholdUartTx)
                                        stop_bit_error = 1;
                                } else // modelserie = 1, 2 or 4
                                {
                                    if(d_parms->waveBuf[d_parms->mathDecodeUartTx - 1][i]
                                        >= threshold[d_parms->mathDecodeUartTx - 1])
                                        stop_bit_error = 1;
                                }

                                if(d_parms->mathDecodeUartPol) {
                                    if(stop_bit_error)
                                        stop_bit_error = 0;
                                    else
                                        stop_bit_error = 1;
                                }

                                if(stop_bit_error)
                                    d_parms
                                        ->mathDecodeUartTxErr[d_parms->mathDecodeUartTxNval]
                                        = 1;
                            }

                            if(d_parms->mathDecodeUartStop == 1)
                                uart_tx_x_pos += uart_sample_per_bit / 2;
                            else if(d_parms->mathDecodeUartStop == 2)
                                uart_tx_x_pos += uart_sample_per_bit;

                            i = uart_tx_x_pos - 1;

                            d_parms->mathDecodeUartTxNval++;
                        } else {
                            uart_tx_x_pos += uart_sample_per_bit;

                            i = uart_tx_x_pos - 1;
                        }
                    }
                }
            }
        }

        if(d_parms->mathDecodeUartRx) {
            if(d_parms->chan[d_parms->mathDecodeUartRx - 1]
                    .Display) // don't try to decode if channel isn't enabled...
            {
                for(i = 1; i < d_parms->waveBufsz; i++) {
                    if(d_parms->mathDecodeUartRxNval >= DECODE_MAX_CHARS)
                        break;

                    if(!uart_rx_start) {
                        if(d_parms->mathDecodeUartPol) // positive, line level RS-232
                        {
                            if(d_parms->modelSerie == 6) {
                                if(d_parms->waveBuf[d_parms->mathDecodeUartRx - 1][i - 1]
                                    >= d_parms->mathDecodeThresholdUartRx) {
                                    if(d_parms->waveBuf[d_parms->mathDecodeUartRx - 1][i]
                                        < d_parms->mathDecodeThresholdUartRx) {
                                        uart_rx_start = 1;

                                        uart_val = 0;

                                        uart_rx_x_pos = (uart_sample_per_bit * 1.5) + i;

                                        i = uart_rx_x_pos - 1;
                                    }
                                }
                            } else // modelserie = 1, 2 or 4
                                if(d_parms->waveBuf[d_parms->mathDecodeUartRx - 1][i - 1]
                                    >= threshold[d_parms->mathDecodeUartRx - 1]) {
                                    if(d_parms->waveBuf[d_parms->mathDecodeUartRx - 1][i]
                                        < threshold[d_parms->mathDecodeUartRx - 1]) {
                                        uart_rx_start = 1;

                                        uart_val = 0;

                                        uart_rx_x_pos = (uart_sample_per_bit * 1.5) + i;

                                        i = uart_rx_x_pos - 1;
                                    }
                                }
                        } else // negative, cpu level TTL/CMOS
                            if(d_parms->modelSerie == 6) {
                                if(d_parms->waveBuf[d_parms->mathDecodeUartRx - 1][i - 1]
                                    < d_parms->mathDecodeThresholdUartRx) {
                                    if(d_parms->waveBuf[d_parms->mathDecodeUartRx - 1][i]
                                        >= d_parms->mathDecodeThresholdUartRx) {
                                        uart_rx_start = 1;

                                        uart_val = 0;

                                        uart_rx_x_pos = (uart_sample_per_bit * 1.5) + i;

                                        i = uart_rx_x_pos - 1;
                                    }
                                }
                            } else // modelserie = 1, 2 or 4
                                if(d_parms->waveBuf[d_parms->mathDecodeUartRx - 1][i - 1]
                                    < threshold[d_parms->mathDecodeUartRx - 1]) {
                                    if(d_parms->waveBuf[d_parms->mathDecodeUartRx - 1][i]
                                        >= threshold[d_parms->mathDecodeUartRx - 1]) {
                                        uart_rx_start = 1;

                                        uart_val = 0;

                                        uart_rx_x_pos = (uart_sample_per_bit * 1.5) + i;

                                        i = uart_rx_x_pos - 1;
                                    }
                                }
                    } else // uart_rx_start != 0
                    {
                        if(d_parms->modelSerie == 6) {
                            if(d_parms->waveBuf[d_parms->mathDecodeUartRx - 1][i]
                                >= d_parms->mathDecodeThresholdUartRx)
                                uart_val += (1 << uart_rx_data_bit);
                        } else // modelserie = 1, 2 or 4
                        {
                            if(d_parms->waveBuf[d_parms->mathDecodeUartRx - 1][i]
                                >= threshold[d_parms->mathDecodeUartRx - 1])
                                uart_val += (1 << uart_rx_data_bit);
                        }

                        if(++uart_rx_data_bit == d_parms->mathDecodeUartWidth) {
                            if((d_parms->mathDecodeUartEnd)
                                && (d_parms->mathDecodeFormat != 4)) // big endian?
                            {
                                uart_val = reverseBitOrder8(uart_val);

                                uart_val >>= (8 - uart_rx_data_bit);
                            }

                            if(!d_parms->mathDecodeUartPol) // positive, line level RS-232 or negative, cpu level TTL/CMOS?
                            {
                                uart_val = ~uart_val;

                                uart_val &= (0xff >> (8 - uart_rx_data_bit));
                            }

                            d_parms->mathDecodeUartRxVal[d_parms->mathDecodeUartRxNval]
                                = uart_val;

                            d_parms->mathDecodeUartRxValPos[d_parms->mathDecodeUartRxNval]
                                = i - (uart_rx_data_bit * uart_sample_per_bit)
                                + (0.5 * uart_sample_per_bit);

                            uart_rx_data_bit = 0;

                            uart_rx_start = 0;

                            d_parms->mathDecodeUartRxErr[d_parms->mathDecodeUartRxNval] = 0;

                            if(d_parms->mathDecodeUartPar) {
                                uart_rx_x_pos += uart_sample_per_bit;

                                i = uart_rx_x_pos;

                                if(i < d_parms->waveBufsz) {
                                    if(d_parms->modelSerie == 6) {
                                        if(d_parms->waveBuf[d_parms->mathDecodeUartRx - 1][i]
                                            >= d_parms->mathDecodeThresholdUartRx)
                                            if(d_parms->mathDecodeUartPol)
                                                uart_parity_bit = 1;
                                            else
                                                uart_parity_bit = 0;
                                        else if(d_parms->mathDecodeUartPol)
                                            uart_parity_bit = 0;
                                        else
                                            uart_parity_bit = 1;
                                    } else {
                                        if(d_parms->waveBuf[d_parms->mathDecodeUartRx - 1][i]
                                            >= threshold[d_parms->mathDecodeUartRx - 1])
                                            if(d_parms->mathDecodeUartPol)
                                                uart_parity_bit = 1;
                                            else
                                                uart_parity_bit = 0;
                                        else if(d_parms->mathDecodeUartPol)
                                            uart_parity_bit = 0;
                                        else
                                            uart_parity_bit = 1;
                                    }

                                    for(j = 0, uart_parity = 0;
                                        j < d_parms->mathDecodeUartWidth;
                                        j++)
                                        uart_parity += ((uart_val >> j) & 1);

                                    if(d_parms->mathDecodeUartPar & 1)
                                        uart_parity++;

                                    if((uart_parity & 1) != uart_parity_bit)
                                        d_parms
                                            ->mathDecodeUartRxErr[d_parms->mathDecodeUartRxNval]
                                            = 1;
                                }
                            }

                            uart_rx_x_pos += uart_sample_per_bit;

                            i = uart_rx_x_pos;

                            stop_bit_error = 0; // check stop bit

                            if(i < d_parms->waveBufsz) {
                                if(d_parms->modelSerie == 6) {
                                    if(d_parms->waveBuf[d_parms->mathDecodeUartRx - 1][i]
                                        >= d_parms->mathDecodeThresholdUartRx)
                                        stop_bit_error = 1;
                                } else // modelserie = 1, 2 or 4
                                {
                                    if(d_parms->waveBuf[d_parms->mathDecodeUartRx - 1][i]
                                        >= threshold[d_parms->mathDecodeUartRx - 1])
                                        stop_bit_error = 1;
                                }

                                if(d_parms->mathDecodeUartPol) {
                                    if(stop_bit_error)
                                        stop_bit_error = 0;
                                    else
                                        stop_bit_error = 1;
                                }

                                if(stop_bit_error)
                                    d_parms
                                        ->mathDecodeUartRxErr[d_parms->mathDecodeUartRxNval]
                                        = 1;
                            }

                            if(d_parms->mathDecodeUartStop == 1)
                                uart_rx_x_pos += uart_sample_per_bit / 2;
                            else if(d_parms->mathDecodeUartStop == 2)
                                uart_rx_x_pos += uart_sample_per_bit;

                            i = uart_rx_x_pos - 1;

                            d_parms->mathDecodeUartRxNval++;
                        } else {
                            uart_rx_x_pos += uart_sample_per_bit;

                            i = uart_rx_x_pos - 1;
                        }
                    }
                }
            }
        }
    }

    if(d_parms->mathDecodeMode == DECODE_MODE_SPI) {
        d_parms->mathDecodeSpiMosiNval = 0;

        d_parms->mathDecodeSpiMisoNval = 0;

        spi_data_mosi_bit = 0;

        spi_data_miso_bit = 0;

        spi_mosi_val = 0;

        spi_miso_val = 0;

        spi_timeout_cntr = 0;

        if(d_parms->mathDecodeSpiWidth > 24)
            spi_chars = 4;
        else if(d_parms->mathDecodeSpiWidth > 16)
            spi_chars = 3;
        else if(d_parms->mathDecodeSpiWidth > 8)
            spi_chars = 2;
        else
            spi_chars = 1;

        if(d_parms->mathDecodeSpiEdge) // sample at rising edge of spi clock?
            spi_clk_old = 1;
        else
            spi_clk_old = 0;

        if(!d_parms->chan[d_parms->mathDecodeSpiClk].Display) // without a clock we can't do much...
            goto SPI_DECODE_OUT;

        if(d_parms->mathDecodeSpiMode) // use chip select line?
        {
            if(d_parms->mathDecodeSpiCs) // is chip select channel selected?
            {
                if(!d_parms->chan[d_parms->mathDecodeSpiCs].Display) // is selected channel for CS enabled?
                    goto SPI_DECODE_OUT;
            } else {
                goto SPI_DECODE_OUT;
            }
        } else // use timeout to detect start of frame
        {
            if(d_parms->timebasedelayenable)
                spi_timeout = d_parms->mathDecodeSpiTimeout
                    / (d_parms->timebasedelayscale / 100.0);
            else
                spi_timeout = d_parms->mathDecodeSpiTimeout / (d_parms->timebasescale / 100.0);
        }

        for(int i{}; i < d_parms->waveBufsz; i++) {
            if(d_parms->mathDecodeSpiMosiNval >= DECODE_MAX_CHARS)
                break;

            if(d_parms->mathDecodeSpiMode) // use chip select line?
            {
                if(d_parms->mathDecodeSpiSelect) // use positive chip select?
                {
                    if(d_parms->waveBuf[d_parms->mathDecodeSpiCs - 1][i]
                        < threshold[d_parms->mathDecodeSpiCs - 1]) {
                        spi_data_mosi_bit = 0;

                        spi_data_miso_bit = 0;

                        spi_mosi_val = 0;

                        spi_miso_val = 0;

                        continue; // chip select is not active
                    }
                } else // use negative chip select?
                    if(d_parms->waveBuf[d_parms->mathDecodeSpiCs - 1][1]
                        >= threshold[d_parms->mathDecodeSpiCs - 1]) {
                        spi_data_mosi_bit = 0;

                        spi_data_miso_bit = 0;

                        spi_mosi_val = 0;

                        spi_miso_val = 0;

                        continue; // chip select is not active
                    }
            } else // use timeout to detect start of frame
                if(spi_timeout_cntr > spi_timeout) {
                    spi_data_mosi_bit = 0;

                    spi_data_miso_bit = 0;

                    spi_mosi_val = 0;

                    spi_miso_val = 0;
                } else {
                    spi_timeout_cntr++;
                }

            if(d_parms->waveBuf[d_parms->mathDecodeSpiClk][i]
                >= threshold[d_parms->mathDecodeSpiClk])
                spi_clk_new = 1;
            else
                spi_clk_new = 0;

            if(spi_clk_old == spi_clk_new)
                continue; // no clock change

            if(d_parms->mathDecodeSpiEdge != spi_clk_new) // wrong clock edge?
            {
                spi_clk_old = spi_clk_new;

                continue;
            }

            spi_timeout_cntr = 0;

            spi_clk_old = spi_clk_new;

            if(d_parms->mathDecodeSpiMosi) {
                if(d_parms->chan[d_parms->mathDecodeSpiMosi - 1]
                        .Display) // don't try to decode if channel isn't enabled...
                {
                    if(d_parms->waveBuf[d_parms->mathDecodeSpiMosi - 1][i]
                        >= threshold[d_parms->mathDecodeSpiMosi - 1])
                        spi_mosi_val += (1 << spi_data_mosi_bit);

                    if(!spi_data_mosi_bit)
                        spi_mosi_bit0_pos = i;

                    if(++spi_data_mosi_bit == d_parms->mathDecodeSpiWidth) {
                        if((d_parms->mathDecodeSpiEnd)
                            && (d_parms->mathDecodeFormat != 4)) // big endian?
                        {
                            spi_mosi_val = reverseBitOrder32(spi_mosi_val);

                            spi_mosi_val >>= (32 - spi_data_mosi_bit);
                        }

                        if(!d_parms->mathDecodeSpiPol) {
                            spi_mosi_val = ~spi_mosi_val;

                            switch(spi_chars) {
                            case 1:
                                spi_mosi_val &= 0xff;
                                break;
                            case 2:
                                spi_mosi_val &= 0xffff;
                                break;
                            case 3:
                                spi_mosi_val &= 0xffffff;
                                break;
                            }
                        }

                        d_parms->mathDecodeSpiMosiVal[d_parms->mathDecodeSpiMosiNval]
                            = spi_mosi_val;

                        d_parms->mathDecodeSpiMosiValPos[d_parms->mathDecodeSpiMosiNval]
                            = spi_mosi_bit0_pos;

                        d_parms
                            ->mathDecodeSpiMosiValPosEnd[d_parms->mathDecodeSpiMosiNval++]
                            = i;

                        spi_data_mosi_bit = 0;

                        spi_mosi_val = 0;
                    }
                }
            }

            if(d_parms->mathDecodeSpiMiso) {
                if(d_parms->chan[d_parms->mathDecodeSpiMiso - 1]
                        .Display) // don't try to decode if channel isn't enabled...
                {
                    if(d_parms->waveBuf[d_parms->mathDecodeSpiMiso - 1][i]
                        >= threshold[d_parms->mathDecodeSpiMiso - 1])
                        spi_miso_val += (1 << spi_data_miso_bit);

                    if(!spi_data_miso_bit)
                        spi_miso_bit0_pos = i;

                    if(++spi_data_miso_bit == d_parms->mathDecodeSpiWidth) {
                        if((d_parms->mathDecodeSpiEnd)
                            && (d_parms->mathDecodeFormat != 4)) // big endian?
                        {
                            spi_miso_val = reverseBitOrder32(spi_miso_val);

                            spi_miso_val >>= (32 - spi_data_miso_bit);
                        }

                        if(!d_parms->mathDecodeSpiPol)
                            spi_miso_val = ~spi_miso_val;

                        d_parms->mathDecodeSpiMisoVal[d_parms->mathDecodeSpiMisoNval]
                            = spi_miso_val;

                        d_parms->mathDecodeSpiMisoValPos[d_parms->mathDecodeSpiMisoNval]
                            = spi_miso_bit0_pos;

                        d_parms
                            ->mathDecodeSpiMisoValPosEnd[d_parms->mathDecodeSpiMisoNval++]
                            = i;

                        spi_data_miso_bit = 0;

                        spi_miso_val = 0;
                    }
                }
            }
        }

    SPI_DECODE_OUT:

        i = 0; // FIXME
    }
}

inline unsigned char UiMainWindow::reverseBitOrder8(unsigned char byte) {
    byte = (byte & 0xf0) >> 4 | (byte & 0x0f) << 4;
    byte = (byte & 0xcc) >> 2 | (byte & 0x33) << 2;
    byte = (byte & 0xaa) >> 1 | (byte & 0x55) << 1;

    return byte;
}

inline unsigned int UiMainWindow::reverseBitOrder32(unsigned int val) {
    val = (val & 0xffff0000) >> 16 | (val & 0x0000ffff) << 16;
    val = (val & 0xff00ff00) >> 8 | (val & 0x00ff00ff) << 8;
    val = (val & 0xf0f0f0f0) >> 4 | (val & 0x0f0f0f0f) << 4;
    val = (val & 0xcccccccc) >> 2 | (val & 0x33333333) << 2;
    val = (val & 0xaaaaaaaa) >> 1 | (val & 0x55555555) << 1;

    return val;
}
