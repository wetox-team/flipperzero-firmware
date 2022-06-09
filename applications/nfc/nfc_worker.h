#pragma once

#include "nfc_device.h"

#define Techkom_F_SIG (13560000.0)
#define Techkom_T_SIG (1.0 / Techkom_F_SIG)
#define F_TIM (64000000.0)
#define T_TIM (1.0 / F_TIM)

typedef struct NfcWorker NfcWorker;

typedef enum {
    // Init states
    NfcWorkerStateNone,
    NfcWorkerStateBroken,
    NfcWorkerStateReady,
    // Main worker states
    NfcWorkerStateDetect,
    NfcWorkerStateEmulate,
    NfcWorkerStateReadEMVApp,
    NfcWorkerStateReadEMVData,
    NfcWorkerStateEmulateApdu,
    NfcWorkerStateField,
    NfcWorkerStateReadMifareUltralight,
    NfcWorkerStateEmulateMifareUltralight,
    NfcWorkerStateReadMifareClassic,
    NfcWorkerStateEmulateMifareClassic,
    NfcWorkerStateReadMifareDesfire,
    NfcWorkerStateReadTechkom,
    NfcWorkerStateEmulateTechkom,
    // Transition
    NfcWorkerStateStop,
} NfcWorkerState;

typedef enum {
    // Reserve first 50 events for application events
    NfcWorkerEventReserved = 50,

    // Nfc worker common events
    NfcWorkerEventSuccess,
    NfcWorkerEventFail,
    NfcWorkerEventNoCardDetected,
    // Mifare Classic events
    NfcWorkerEventNoDictFound,
    NfcWorkerEventDetectedClassic1k,
    NfcWorkerEventDetectedClassic4k,
    NfcWorkerEventNewSector,
    NfcWorkerEventFoundKeyA,
    NfcWorkerEventFoundKeyB,
    NfcWorkerEventStartReading,
} NfcWorkerEvent;

typedef void (*NfcWorkerCallback)(NfcWorkerEvent event, void* context);

NfcWorker* nfc_worker_alloc();

NfcWorkerState nfc_worker_get_state(NfcWorker* nfc_worker);

void nfc_worker_free(NfcWorker* nfc_worker);

void nfc_worker_start(
    NfcWorker* nfc_worker,
    NfcWorkerState state,
    NfcDeviceData* dev_data,
    NfcWorkerCallback callback,
    void* context);

void nfc_worker_stop(NfcWorker* nfc_worker);

void nfc_worker_emulate_techkom(NfcWorker* nfc_worker);
