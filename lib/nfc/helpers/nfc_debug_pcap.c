#include "nfc_debug_pcap.h"

#include <furi_hal_rtc.h>
#include <stream_buffer.h>

#define TAG "NfcDebugPcap"

#define PCAP_MAGIC 0xa1b2c3d4
#define PCAP_MAJOR 2
#define PCAP_MINOR 4
#define DLT_ISO_14443 264

#define DATA_PICC_TO_PCD 0xFF
#define DATA_PCD_TO_PICC 0xFE
#define DATA_PICC_TO_PCD_CRC_DROPPED 0xFB
#define DATA_PCD_TO_PICC_CRC_DROPPED 0xFA

#define NFC_DEBUG_PCAP_FILENAME EXT_PATH("nfc/debug.trace")
#define NFC_DEBUG_PCAP_BUFFER_SIZE 64

struct NfcDebugPcapWorker {
    bool alive;
    Storage* storage;
    File* file;
    StreamBufferHandle_t stream;
    FuriThread* thread;
};

static File* nfc_debug_pcap_open(Storage* storage) {
    File* file = storage_file_alloc(storage);
    if(!storage_file_open(file, NFC_DEBUG_PCAP_FILENAME, FSAM_WRITE, FSOM_OPEN_APPEND)) {
        storage_file_free(file);
        return NULL;
    }
    // if(!storage_file_tell(file)) {
    // struct {
    //     uint32_t magic;
    //     uint16_t major, minor;
    //     uint32_t reserved[2];
    //     uint32_t snaplen;
    //     uint32_t link_type;
    // } __attribute__((__packed__)) pcap_hdr = {
    //     .magic = PCAP_MAGIC,
    //     .major = PCAP_MAJOR,
    //     .minor = PCAP_MINOR,
    //     .snaplen = FURI_HAL_NFC_DATA_BUFF_SIZE,
    //     .link_type = DLT_ISO_14443,
    // };
    // if(storage_file_write(file, &pcap_hdr, sizeof(pcap_hdr)) != sizeof(pcap_hdr)) {
    //     FURI_LOG_E(TAG, "Failed to write pcap header");
    // }
    // }
    return file;
}

static void nfc_debug_pcap_write(
    NfcDebugPcapWorker* instance,
    bool is_response,
    uint8_t* data,
    uint16_t len,
    uint8_t* parity,
    int parity_len) {
    FuriHalRtcDateTime datetime;
    furi_hal_rtc_get_datetime(&datetime);

    if(len == 0) {
        return;
    }

    uint8_t frame[len + parity_len];
    memcpy(frame, data, len);
    memcpy(frame + len, parity, parity_len);

    struct {
        uint32_t timestamp;
        uint16_t duration;
        uint16_t data_len : 15; // 15 bits
        bool isResponse : 1; // 1 bit, 0 for request, 1 for response
    } __attribute__((__packed__)) tracelog_hdr = {
        .timestamp = furi_get_tick(),
        .duration = 124 * (len + parity_len) * 8,
        .data_len = len,
        .isResponse = is_response,
    };

    xStreamBufferSend(instance->stream, &tracelog_hdr, sizeof(tracelog_hdr), FuriWaitForever);
    xStreamBufferSend(instance->stream, frame, sizeof(frame), FuriWaitForever);
}

static void nfc_debug_pcap_write_tx(
    uint8_t* data,
    uint16_t bits,
    bool crc_dropped,
    void* context,
    uint8_t* parity,
    int parity_len) {
    UNUSED(crc_dropped);
    NfcDebugPcapWorker* instance = context;
    bool is_response = false;
    nfc_debug_pcap_write(instance, is_response, data, bits / 8, parity, parity_len);
}

static void nfc_debug_pcap_write_rx(
    uint8_t* data,
    uint16_t bits,
    bool crc_dropped,
    void* context,
    uint8_t* parity,
    int parity_len) {
    UNUSED(crc_dropped);
    NfcDebugPcapWorker* instance = context;
    bool is_response = true;
    nfc_debug_pcap_write(instance, is_response, data, bits / 8, parity, parity_len);
}

int32_t nfc_debug_pcap_thread(void* context) {
    NfcDebugPcapWorker* instance = context;
    uint8_t buffer[NFC_DEBUG_PCAP_BUFFER_SIZE];

    while(instance->alive) {
        size_t ret =
            xStreamBufferReceive(instance->stream, buffer, NFC_DEBUG_PCAP_BUFFER_SIZE, 50);
        if(storage_file_write(instance->file, buffer, ret) != ret) {
            FURI_LOG_E(TAG, "Failed to write pcap data");
        }
    }

    return 0;
}

NfcDebugPcapWorker* nfc_debug_pcap_alloc(Storage* storage) {
    NfcDebugPcapWorker* instance = malloc(sizeof(NfcDebugPcapWorker));

    instance->alive = true;

    instance->storage = storage;

    instance->file = nfc_debug_pcap_open(storage);

    instance->stream = xStreamBufferCreate(4096, 1);

    instance->thread = furi_thread_alloc();
    furi_thread_set_name(instance->thread, "PcapWorker");
    furi_thread_set_stack_size(instance->thread, 1024);
    furi_thread_set_callback(instance->thread, nfc_debug_pcap_thread);
    furi_thread_set_context(instance->thread, instance);
    furi_thread_start(instance->thread);

    return instance;
}

void nfc_debug_pcap_free(NfcDebugPcapWorker* instance) {
    furi_assert(instance);

    instance->alive = false;

    furi_thread_join(instance->thread);
    furi_thread_free(instance->thread);

    vStreamBufferDelete(instance->stream);

    if(instance->file) storage_file_free(instance->file);

    instance->storage = NULL;

    free(instance);
}

void nfc_debug_pcap_prepare_tx_rx(
    NfcDebugPcapWorker* instance,
    FuriHalNfcTxRxContext* tx_rx,
    bool is_picc) {
    if(!instance || !instance->file) return;

    if(is_picc) {
        tx_rx->sniff_tx = nfc_debug_pcap_write_rx;
        tx_rx->sniff_rx = nfc_debug_pcap_write_tx;
    } else {
        tx_rx->sniff_tx = nfc_debug_pcap_write_tx;
        tx_rx->sniff_rx = nfc_debug_pcap_write_rx;
    }

    tx_rx->sniff_context = instance;
}
