#include "../bsp_drivers/sampler.hpp"

#include <string.h>

#include "ti_drivers_open_close.h"

#include "../bsp_drivers/task_config.h"

uint32_t sigs = 0;
uint32_t adcs = 0;

void adc_spi_cb(MCSPI_Handle handle, MCSPI_Transaction* transaction) {
    auto ctx = reinterpret_cast<Sampler*>(transaction->args);

    auto& sample = ctx->adc_sample;

    adcs++;

    if (sample.tbd == BurstSamplesTBD::HIGH) {
        sample.high_done = true;
    } else if (sample.tbd == BurstSamplesTBD::LOW) {
        sample.low_done = true;
    }

    if (sample.low_done && sample.high_done) {
        ctx->release_adc_sample_write_access();
    }
}

AveragingRingBuffer::AveragingRingBuffer(uint16_t max_deviation) : MAX_DEVIATION(max_deviation) {
    memset(values, 0, sizeof(values));
}

void AveragingRingBuffer::add_value(uint16_t new_value) {
    values[cur_idx] = new_value;

    // increase buffer pointer
    cur_idx++;
    if (cur_idx == CP_SAMPLE_SIZE) {
        cur_idx = 0;
        filled = true;
    }

    // calculate average
    average = 0;
    for (size_t i = 0; i < CP_SAMPLE_SIZE; i++) {
        average += values[i];
    }
    average = average / CP_SAMPLE_SIZE;

    // calculate deviations
    misfits_count = 0;
    for (size_t i = 0; i < CP_SAMPLE_SIZE; i++) {
        const auto value = values[i];
        uint16_t deviation = (value > average) ? (value - average) : (average - value);
        if (deviation > MAX_DEVIATION) {
            misfits_count++;
        }
    }
}

Sampler::Sampler() {
    // for now, the the first and second sample should be channel 1 and the third channel 2
    adc_spi_commands[0] = ADC_SELECT_CHANNEL_1;
    adc_spi_commands[1] = ADC_SELECT_CHANNEL_2;
    adc_spi_commands[2] = ADC_SELECT_CHANNEL_1;

    MCSPI_Transaction_init(&transaction);
    transaction.channel = 1; // FIXME (aw): constant
    transaction.csDisable = true;
    transaction.dataSize = 16;
    transaction.count = SPI_BURST_ADC_READS;
    transaction.txBuf = adc_spi_commands;
    // spi_xfer.rxBuf will be set depending on sample polarization
    transaction.args = this;

    new_sample_available = xSemaphoreCreateBinaryStatic(&new_sample_semphr_buf);

    sample_buf_mutex = xSemaphoreCreateBinaryStatic(&sample_buf_mutex_buf);
    xSemaphoreGive(sample_buf_mutex);

    raw_cp_signal_mutex = xSemaphoreCreateBinaryStatic(&raw_cp_signal_mutex_buf);
    xSemaphoreGive(raw_cp_signal_mutex);
}

inline bool Sampler::aquire_adc_sample_write_access() {
    if (xSemaphoreTakeFromISR(sample_buf_mutex, nullptr) == pdFALSE) {
        // we couldn't aquire the buffer mutex, so it is probably in use
        // right now, so there's nothing we can do
        return false;
    }

    return true;
}

// FIXME (aw): still don't get why this yield from isr is needed?
void Sampler::release_adc_sample_write_access() {
    adc_sample_write_access_aquired = false;
    // release sample buffer
    BaseType_t higher_prio_woken_a, higher_prio_woken_b;
    xSemaphoreGiveFromISR(sample_buf_mutex, &higher_prio_woken_a);
    // notify loop
    xSemaphoreGiveFromISR(new_sample_available, &higher_prio_woken_b);

    portYIELD_FROM_ISR((higher_prio_woken_a | higher_prio_woken_b));
}

void Sampler::trigger_adc_readout(bool is_high) {
    // FIXME (aw): the check for MCSPI_TRANSFER_COMPLETED is an implementation detail whose behavior might change in
    // future
    if (transaction.status != MCSPI_TRANSFER_COMPLETED) {
        // NOTE: this can only happen, if the high or low part of the pwm are so short, that
        // a new read will be triggered before the old one finished, so here we return and will wait until we get the
        // proper pulse - but this won't make much sense, because the sampling interval might be to short to get
        // accurate data
        return;
    }

    if (!adc_sample_write_access_aquired) {
        // don't have the write access
        if (!aquire_adc_sample_write_access()) {
            // we couldn't get the write access, because a read is ongoing, so
            // we're losing this frame
            return;
        }
        // we got the write access, so remember and start a fresh sample
        adc_sample_write_access_aquired = true;
        adc_sample.tbd = (is_high) ? BurstSamplesTBD::HIGH : BurstSamplesTBD::LOW;
        adc_sample.high_done = false;
        adc_sample.low_done = false;
    } else {
        // NOTE: this should only happen when one or the other sample got skipped due to
        // too short duty or non-duty cycle
        if ((adc_sample.high_done && is_high) || (adc_sample.low_done && !is_high)) {
            // already did this sample, so we don't need to repeat
            return;
        }

        adc_sample.tbd = (is_high) ? BurstSamplesTBD::HIGH : BurstSamplesTBD::LOW;
    }

    if (adc_sample.tbd == BurstSamplesTBD::HIGH) {
        transaction.rxBuf = adc_sample.high_samples;
    } else if (adc_sample.tbd == BurstSamplesTBD::LOW) {
        transaction.rxBuf = adc_sample.low_samples;
    }

    if (SystemP_SUCCESS != MCSPI_transfer(gMcspiHandle[ADC_SPI], &transaction)) {
        // FIXME (aw): what to do here, technically that shouldn't be possible!
    }
}

CombinedADCSample Sampler::decode_burst_adc_samples(uint16_t* buffer) {
    // NOTE: this function assumes to have exclusive read access
    // NOTE: assuming the memory layout mentioned in the constructor
    return {buffer[1], buffer[2]};
}

iec61851::CPSignal Sampler::get_latest_cp_signal() {
    iec61851::CPSignal cp_signal;

    xSemaphoreTake(raw_cp_signal_mutex, portMAX_DELAY);

    if (raw_cp_signal.valid) {
        cp_signal.valid = true;
        // FIXME (aw): the conversion could also be done outside of the mutex
        cp_signal.high = ADC_CHANNEL_1_OFFSET + (ADC_CHANNEL_1_SCALE * raw_cp_signal.high) / (1 << ADC_RESOLUTION);
        cp_signal.low = ADC_CHANNEL_1_OFFSET + (ADC_CHANNEL_1_SCALE * raw_cp_signal.low) / (1 << ADC_RESOLUTION);
    } else {
        cp_signal.valid = false;
    }

    xSemaphoreGive(raw_cp_signal_mutex);

    return cp_signal;
}

void Sampler::loop() {
    while (1) {
        // NOTE (aw): this could possibly be implemented by task notification
        if (xSemaphoreTake(new_sample_available, portMAX_DELAY) == pdFALSE) {
            continue;
        }

        sigs++;

        xSemaphoreTake(sample_buf_mutex, portMAX_DELAY);

        auto high_sample = decode_burst_adc_samples(adc_sample.high_samples);
        auto low_sample = decode_burst_adc_samples(adc_sample.low_samples);

        xSemaphoreGive(sample_buf_mutex);

        cp_high_avg.add_value(high_sample.cp);
        cp_low_avg.add_value(low_sample.cp);
        // NOTE: pp from the high part should be enough
        pp_avg.add_value(high_sample.pp);

        xSemaphoreTake(raw_cp_signal_mutex, portMAX_DELAY);
        if ((cp_high_avg.misfits_count <= CP_SAMPLE_MAX_MISFITS) &&
            (cp_low_avg.misfits_count <= CP_SAMPLE_MAX_MISFITS) && cp_high_avg.filled && cp_low_avg.filled) {
            raw_cp_signal.high = cp_high_avg.average;
            raw_cp_signal.low = cp_low_avg.average;
            raw_cp_signal.valid = true;
        } else {
            raw_cp_signal.valid = false;
        }
        xSemaphoreGive(raw_cp_signal_mutex);
    }
}
