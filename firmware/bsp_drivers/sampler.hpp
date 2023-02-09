#ifndef DRIVERS_SAMPLER_HPP
#define DRIVERS_SAMPLER_HPP

#include "FreeRTOS.h"
#include "semphr.h"

#include <drivers/mcspi.h>

#include <iec61851_hal.hpp>

extern uint32_t sigs;
extern uint32_t adcs;

extern "C" {
void adc_spi_cb(MCSPI_Handle handle, MCSPI_Transaction* transaction);
}

// FIXME (aw): choose proper numbers here
static constexpr uint8_t SPI_BURST_ADC_READS = 3;
static constexpr uint8_t CP_SAMPLE_SIZE = 10;
static constexpr uint16_t CP_SAMPLE_MAX_DEVIATION = 400;
static constexpr uint16_t CP_SAMPLE_MAX_MISFITS = 1;
static constexpr uint16_t PP_SAMPLE_MAX_DEVIATION = 500;

// ADC122S051 has 12 bits resolution
static constexpr uint8_t ADC_RESOLUTION = 12;
static constexpr uint16_t ADC_SELECT_CHANNEL_1 = (0x00U << 8);
static constexpr uint16_t ADC_SELECT_CHANNEL_2 = (0x08U << 8);

static constexpr float ADC_CHANNEL_1_R1 = 200;
static constexpr float ADC_CHANNEL_1_R2 = 56;
static constexpr float ADC_CHANNEL_1_R3 = 66.5;
static constexpr float ADC_CHANNEL_1_R1R2R3_parallel = 1./(1./ADC_CHANNEL_1_R1 + 1./ADC_CHANNEL_1_R2 + 1./ADC_CHANNEL_1_R3);
static constexpr float ADC_CHANNEL_1_SCALE = ADC_CHANNEL_1_R1/ADC_CHANNEL_1_R1R2R3_parallel*3.3;
static constexpr float ADC_CHANNEL_1_OFFSET = -3.3*ADC_CHANNEL_1_R1/ADC_CHANNEL_1_R2;

static constexpr float ADC_CHANNEL_2_SCALE = 1.;
static constexpr float ADC_CHANNEL_2_OFFSET = 0.;

enum class BurstSamplesTBD : uint8_t {
    HIGH,
    LOW
};

struct BurstADCSamples {
    uint16_t high_samples[SPI_BURST_ADC_READS];
    uint16_t low_samples[SPI_BURST_ADC_READS];
    BurstSamplesTBD tbd;
    bool high_done{false};
    bool low_done{false};
};

struct CombinedADCSample {
    uint16_t cp;
    uint16_t pp;
};

struct RawCPSignal {
    uint16_t high;
    uint16_t low;
    bool valid;
};

struct AveragingRingBuffer {
    explicit AveragingRingBuffer(uint16_t max_deviation);
    void add_value(uint16_t value);

    bool filled{false};

    uint32_t average{0};
    uint16_t misfits_count{0};

    uint16_t cur_idx{0};

    uint16_t values[CP_SAMPLE_SIZE];

    const uint16_t MAX_DEVIATION;
};

class Sampler {
public:
    Sampler();

    void trigger_high() {
        trigger_adc_readout(true);
    }

    void trigger_low() {
        trigger_adc_readout(false);
    }

    static void sampling_task_trampoline(void* ctx) {
        reinterpret_cast<Sampler*>(ctx)->loop();
    }

    iec61851::CPSignal get_latest_cp_signal();

private:
    void loop();

    void trigger_adc_readout(bool is_high);

    bool aquire_adc_sample_write_access();
    void release_adc_sample_write_access();
    bool adc_sample_write_access_aquired{false};

    uint16_t adc_spi_commands[SPI_BURST_ADC_READS];
    MCSPI_Transaction transaction;

    BurstADCSamples adc_sample;

    StaticSemaphore_t new_sample_semphr_buf;
    SemaphoreHandle_t new_sample_available;

    StaticSemaphore_t sample_buf_mutex_buf;
    SemaphoreHandle_t sample_buf_mutex;

    CombinedADCSample decode_burst_adc_samples(uint16_t* buffer);

    AveragingRingBuffer cp_high_avg{CP_SAMPLE_MAX_DEVIATION};
    AveragingRingBuffer cp_low_avg{CP_SAMPLE_MAX_DEVIATION};
    AveragingRingBuffer pp_avg{PP_SAMPLE_MAX_DEVIATION};

    RawCPSignal raw_cp_signal;
    StaticSemaphore_t raw_cp_signal_mutex_buf;
    SemaphoreHandle_t raw_cp_signal_mutex;

    friend void adc_spi_cb(MCSPI_Handle handle, MCSPI_Transaction* transaction);
};

#endif // DRIVERS_SAMPLER_HPP
