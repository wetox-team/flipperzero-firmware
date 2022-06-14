#include <techkom.h>
#include <furi_hal_nfc.h>

#define Techkom_F_SIG (13560000.0)
#define Techkom_T_SIG (1.0 / Techkom_F_SIG)
#define F_TIM (64000000.0)
#define T_TIM (1.0 / F_TIM)

TechkomSignal* techkom_signal_alloc() {
    TechkomSignal* techkom_signal = malloc(sizeof(TechkomSignal));
    techkom_signal->one = digital_signal_alloc(20);
    techkom_signal->zero = digital_signal_alloc(20);
    techkom_signal->end_one = digital_signal_alloc(20);
    techkom_signal->end_zero = digital_signal_alloc(20);
    techkom_signal->end = digital_signal_alloc(20);
    techkom_add_bit(techkom_signal->one, 1);
    techkom_add_bit(techkom_signal->zero, 0);
    techkom_add_bit(techkom_signal->end_one, ONE_END);
    techkom_add_bit(techkom_signal->end_zero, ZERO_END);
    techkom_add_bit(techkom_signal->end, QUIET);
    techkom_signal->tx_signal = digital_signal_alloc(2048);

    return techkom_signal;
}

void techkom_signal_free(TechkomSignal* techkom_signal) {
    furi_assert(techkom_signal);

    digital_signal_free(techkom_signal->one);
    digital_signal_free(techkom_signal->zero);
    digital_signal_free(techkom_signal->tx_signal);
    free(techkom_signal);
}

void techkom_emulator(TechkomEmulator* emulator, FuriHalNfcTxRxContext* tx_rx) {
    tx_rx->tx_rx_type = FuriHalNfcTxRxTechkom;
    tx_rx->tx_bits = 64;

    // Copy CUID to the TX buffer
    memcpy(tx_rx->tx_data, emulator->cuid, 8);

    // Run the emulator
    furi_hal_nfc_tx_rx(tx_rx, 100);
}

void techkom_add_bit(DigitalSignal* signal, uint8_t bit) {
    furi_assert(signal);

    if(bit == ONE) {
        signal->start_level = true;
        signal->edge_timings[0] = PAUSE_TIM * Techkom_T_SIG;
        signal->edge_timings[1] = ONE_FIRST * Techkom_T_SIG;
        signal->edge_timings[2] = PAUSE_TIM * Techkom_T_SIG;
        signal->edge_timings[3] = ONE_SECOND * Techkom_T_SIG;
        signal->edge_cnt = 4;
    } else if(bit == ZERO) {
        signal->start_level = true;
        signal->edge_timings[0] = PAUSE_TIM * Techkom_T_SIG;
        signal->edge_timings[1] = ZERO_FIRST * Techkom_T_SIG;
        signal->edge_timings[2] = PAUSE_TIM * Techkom_T_SIG;
        signal->edge_timings[3] = ZERO_SECOND * Techkom_T_SIG;
        signal->edge_cnt = 4;
    } else if(bit == ONE_END) {
        signal->start_level = true;
        signal->edge_timings[0] = PAUSE_TIM * Techkom_T_SIG;
        signal->edge_timings[1] = ONE_FIRST * Techkom_T_SIG;
        signal->edge_timings[2] = PAUSE_TIM * Techkom_T_SIG;
        signal->edge_timings[3] = ONE_SECOND_END * Techkom_T_SIG;
        signal->edge_cnt = 4;
    } else if(bit == ZERO_END) {
        signal->start_level = true;
        signal->edge_timings[0] = PAUSE_TIM * Techkom_T_SIG;
        signal->edge_timings[1] = ZERO_FIRST * Techkom_T_SIG;
        signal->edge_timings[2] = PAUSE_TIM * Techkom_T_SIG;
        signal->edge_timings[3] = ZERO_SECOND_END * Techkom_T_SIG;
        signal->edge_cnt = 4;
    } else if(bit == QUIET) {
        signal->start_level = false;
        signal->edge_timings[0] = QUIET_PERIOD * Techkom_T_SIG;
        signal->edge_cnt = 1;
    }
}

void techkom_signal_encode(TechkomSignal* techkom_signal, uint8_t* data) {
    furi_assert(techkom_signal);
    furi_assert(data);

    uint16_t bits = sizeof(data) * 8;

    techkom_signal->tx_signal->start_level = true;

    for(size_t cycles = 0; cycles < 5; cycles++) {
        for(size_t i = 0; i < bits / 8; i++) {
            for(size_t j = 0; j < 8; j++) {
                if(FURI_BIT(data[i], j)) {
                    if(j == 7) {
                        digital_signal_append(techkom_signal->tx_signal, techkom_signal->end_one);
                    } else {
                        digital_signal_append(techkom_signal->tx_signal, techkom_signal->one);
                    }
                } else {
                    if(j == 7) {
                        digital_signal_append(techkom_signal->tx_signal, techkom_signal->end_zero);
                    } else {
                        digital_signal_append(techkom_signal->tx_signal, techkom_signal->zero);
                    }
                }
            }
        }
        digital_signal_append(techkom_signal->tx_signal, techkom_signal->end);
    }
}
