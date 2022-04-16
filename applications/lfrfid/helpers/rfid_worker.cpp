#include "rfid_worker.h"

RfidWorker::RfidWorker() {
}

RfidWorker::~RfidWorker() {
}

void RfidWorker::start_read() {
    reader.start();
}

bool RfidWorker::read() {
    static const uint8_t data_size = LFRFID_KEY_SIZE;
    uint8_t data[data_size] = {0};
    LfrfidKeyType type;

    bool result = reader.read(&type, data, data_size);

    if(result) {
        key.set_type(type);
        key.set_data(data, data_size);
    };

    return result;
}

void t5577_clear_password_and_config_to_EM() {
    RfidWriter writer;
    const uint32_t default_passwords[] = {
        0x51243648,
        0x000D8787,
        0x19920427,
        0x50524F58,
        0xF9DCEBA0,
        0x65857569,
        0x05D73B9F,
        0x89A69E60,
        0x314159E0,
        0xAA55BBBB,
        0xA5B4C3D2,
        0x1C0B5848,
        0x00434343,
        0x444E4752,
        0x4E457854,
        0x44B44CAE,
        0x88661858,
        0xE9920427,
        0x575F4F4B,
        0x50520901,
        0x20206666,
        0x65857569,
        0x5469616E,
        0x7686962A,
        0xC0F5009A,
        0x07CEE75D,
        0xfeedbeef,
        0xdeadc0de,
        0x00000000,
        0x11111111,
        0x22222222,
        0x33333333,
        0x44444444,
        0x55555555,
        0x66666666,
        0x77777777,
        0x88888888,
        0x99999999,
        0xAAAAAAAA,
        0xBBBBBBBB,
        0xCCCCCCCC,
        0xDDDDDDDD,
        0xEEEEEEEE,
        0xFFFFFFFF,
        0xa0a1a2a3,
        0xb0b1b2b3,
        0x50415353,
        0x00000001,
        0x00000002,
        0x0000000a,
        0x0000000b,
        0x01020304,
        0x02030405,
        0x03040506,
        0x04050607,
        0x05060708,
        0x06070809,
        0x0708090A,
        0x08090A0B,
        0x090A0B0C,
        0x0A0B0C0D,
        0x0B0C0D0E,
        0x0C0D0E0F,
        0x01234567,
        0x12345678,
        0x10000000,
        0x20000000,
        0x30000000,
        0x40000000,
        0x50000000,
        0x60000000,
        0x70000000,
        0x80000000,
        0x90000000,
        0xA0000000,
        0xB0000000,
        0xC0000000,
        0xD0000000,
        0xE0000000,
        0xF0000000,
        0x10101010,
        0x01010101,
        0x11223344,
        0x22334455,
        0x33445566,
        0x44556677,
        0x55667788,
        0x66778899,
        0x778899AA,
        0x8899AABB,
        0x99AABBCC,
        0xAABBCCDD,
        0xBBCCDDEE,
        0xCCDDEEFF,
        0x0CB7E7FC,
        0xFABADA11,
        0x87654321,
        0x12341234,
        0x69696969,
        0x12121212,
        0x12344321,
        0x1234ABCD,
        0x11112222,
        0x13131313,
        0x10041004,
        0x31415926,
        0xabcd1234,
        0x20002000,
        0x19721972,
        0xaa55aa55,
        0x55aa55aa,
        0x4f271149,
        0x07d7bb0b,
        0x9636ef8f,
        0xb5f44686,
        0x9E3779B9,
        0xC6EF3720,
        0x7854794A,
        0xF1EA5EED,
        0x69314718,
        0x57721566,
        0x93C467E3,
        0x27182818,
        0x50415353
    };
    const uint8_t default_passwords_len = sizeof(default_passwords)/sizeof(uint32_t);
    const uint32_t em_config_block_data = 0b00000000000101001000000001000000; //no pwd&aor config block

    for(uint8_t i = 0; i < default_passwords_len; i++) {
        printf("Trying key %u of %u: 0x%08lX\r\n", i+1, default_passwords_len, default_passwords[i]);
        FURI_CRITICAL_ENTER();
        writer.start();
        writer.write_block(0, 0, false, em_config_block_data, true, default_passwords[i]);
        writer.write_reset();
        writer.stop();
        FURI_CRITICAL_EXIT();
    }

}

bool RfidWorker::detect() {
    return reader.detect();
}

bool RfidWorker::any_read() {
    return reader.any_read();
}

void RfidWorker::stop_read() {
    reader.stop();
}

void RfidWorker::start_write() {
    write_result = WriteResult::Nothing;
    write_sequence = new TickSequencer();
    validate_counts = 0;

    write_sequence->do_every_tick(1, std::bind(&RfidWorker::sq_write, this));
    write_sequence->do_after_tick(2, std::bind(&RfidWorker::sq_write_start_validate, this));
    write_sequence->do_every_tick(30, std::bind(&RfidWorker::sq_write_validate, this));
    write_sequence->do_every_tick(1, std::bind(&RfidWorker::sq_write_stop_validate, this));
}

RfidWorker::WriteResult RfidWorker::write() {
    write_sequence->tick();
    return write_result;
}

void RfidWorker::stop_write() {
    delete write_sequence;
    reader.stop();
}

void RfidWorker::start_emulate() {
    emulator.start(key.get_type(), key.get_data(), key.get_type_data_count());
}

void RfidWorker::stop_emulate() {
    emulator.stop();
}

void RfidWorker::sq_write() {
    for(size_t i = 0; i < 5; i++) {
        switch(key.get_type()) {
        case LfrfidKeyType::KeyEM4100:
            writer.start();
            writer.write_em(key.get_data());
            writer.stop();
            break;
        case LfrfidKeyType::KeyH10301:
            writer.start();
            writer.write_hid(key.get_data());
            writer.stop();
            break;
        case LfrfidKeyType::KeyI40134:
            writer.start();
            writer.write_indala(key.get_data());
            writer.stop();
            break;
        }
    }
}

void RfidWorker::sq_write_start_validate() {
    switch(key.get_type()) {
    case LfrfidKeyType::KeyEM4100:
    case LfrfidKeyType::KeyH10301:
        reader.start_forced(RfidReader::Type::Normal);
        break;
    case LfrfidKeyType::KeyI40134:
        reader.start_forced(RfidReader::Type::Indala);
        break;
    }
}

void RfidWorker::sq_write_validate() {
    static const uint8_t data_size = LFRFID_KEY_SIZE;
    uint8_t data[data_size] = {0};
    LfrfidKeyType type;

    bool result = reader.read(&type, data, data_size);

    if(result && (write_result != WriteResult::Ok)) {
        if (write_result == WriteResult::NotWritable){
            t5577_clear_password_and_config_to_EM();
        }
        if(validate_counts > (5 * 2)) {
            write_result = WriteResult::NotWritable;
        }

        if(type == key.get_type()) {
            if(memcmp(data, key.get_data(), key.get_type_data_count()) == 0) {
                write_result = WriteResult::Ok;
                validate_counts = 0;
            } else {
                validate_counts++;
            }
        } else {
            validate_counts++;
        }
    };
}

void RfidWorker::sq_write_stop_validate() {
    reader.stop();
}
