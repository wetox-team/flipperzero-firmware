#include <techkom.h>
#include <furi_hal_nfc.h>


#define Techkom_F_SIG (13560000.0)
#define Techkom_T_SIG (1.0 / Techkom_F_SIG)
#define F_TIM (64000000.0)
#define T_TIM (1.0 / F_TIM)

void techkom_emulator(TechkomEmulator* emulator, FuriHalNfcTxRxContext* tx_rx) {
    tx_rx->tx_rx_type = FuriHalNfcTxRxTechkom;
    tx_rx->tx_bits = 64;

    // Copy CUID to the TX buffer
    memcpy(tx_rx->tx_data, emulator->cuid, 8);

    // Run the emulator
    furi_hal_nfc_tx_rx(tx_rx, 1);
}


// static void nfca_add_byte(TechkomSignal* techkom_signal, uint8_t byte) {
//     for(uint8_t i = 0; i < 8; i++) {
//         if(byte & (1 << i)) {
//             digital_signal_append(techkom_signal->tx_signal, techkom_signal->one);
//         } else {
//             digital_signal_append(techkom_signal->tx_signal, techkom_signal->zero);
//         }
//     }
// }
