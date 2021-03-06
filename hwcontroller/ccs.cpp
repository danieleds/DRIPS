#include "common.h"
#include "ccs.h"

#include <SPI.h>       // nRF24L01+
#include <RH_NRF24.h>  // nRF24L01+
#include <limits.h>

// ==== CONSTANTS ==== //

#define MSG_TYPE_KEEPALIVE 'K'
#define MSG_TYPE_CCS 'C'
#define MSG_TYPE_FCT 'F'

/**
 * The 'Quantization problem': In an ideal situation we would have messages sent as soon as their
 * timers expire. However we are able to send messages only within an handleCCS() which is called
 * only once every loop(), so we have to take into consideration the maximum delay
 * caused by this 'quantization', TIMESPAN_LOOP_MAX. The value is doubled to consider both the
 * sender and the receiver.
 */
const uint16_t DELTA = 2 * TIMESPAN_LOOP_MAX;

/**
 * Time to wait from keepAliveTimeMarker before sending another keepAlive
 * It must be   TIMESPAN_KEEPALIVE > TIMESPAN_LOOP_MAX + ∂  otherwise it means we just send it every
 * time. Instead we decided to send one KeepAlive every two loops, give or take.
 */
const unsigned long TIMESPAN_KEEPALIVE = TIMESPAN_LOOP_MAX + DELTA;

/**
 * Time after which vehicles in the vehicle cache expire.
 *  - It must be   VEHICLE_CACHE_TTL > TIMESPAN_KEEPALIVE   otherwise the cache would always expire
 *    before receiving another KeepAlive (even in perfect conditions).
 *  - We decided   VEHICLE_CACHE_TTL > TIMESPAN_KEEPALIVE + ∂   because of the 'quantization problem'
 */
const uint16_t VEHICLE_CACHE_TTL = TIMESPAN_KEEPALIVE + DELTA;

/**
 * Duration of the wait after sending a CCS, and duration of the blinking of the LEDs.
 *  - Must be   TIMESPAN_X > 3 * TIMESPAN_LOOP_MAX   because we need to sample inbetween the special blinking
 *    period of my peer. Who sends the CCS stores the samples 1 loop after the beginning of the
 *    blinking, who received the request 1 loop before the end of the period.
 *  - Must be   TIMESPAN_X > DELTA   because it is the maximum possible time I can receive
 *    a FCT reply after sending our CCS
 */
const uint16_t TIMESPAN_X = max(5 * TIMESPAN_LOOP_MAX, DELTA) + 1;

/**
 * Value for the max length of the random backoff interval (ms).
 * Used after receiving a FCT or a CCS which is not for us,
 * so that for the next transmission we avoid too much synchronization
 * between the timings of the vehicles.
 */
const uint16_t TIMESPAN_MAX_BACKOFF = 10;

/**
 * maximum additional delay between two consecutive CCS procedure, used in order to desync
 * vehicles so that they messages collide with lower probability. This value considers the
 * transmission time of our biggest packet, a KeepAlive messge. The maximum transmit time of
 * a packet is 224us according to this source:
 * https://devzone.nordicsemi.com/question/13166/nrf24l01-timing-diagram/
 */
const uint16_t TIMESPAN_RANDOM_DESYNC_US = 5000;


// ==== TYPE DEFINITIONS ==== //

typedef enum State : uint8_t {
    ST_BEGIN,
    ST_WAIT_TO_BLINK,
    ST_BLINK,
    ST_INTERPRETATE,

    // The state returned by handlePeriodicActions() when no state change should occur.
    // Never return this from a state handler.
    ST_CURRENT
} State;

typedef struct Vehicle {
    char address;
    char manufacturer[8];
    char model[8];
    bool priority;
    unsigned long receivedTime;
    RequestedAction requestedAction;
    CurrentAction currentAction;
} Vehicle;


// ==== FUNCTION DECLARATIONS ==== //
State FUN_ST_BEGIN();
State FUN_ST_WAIT_TO_BLINK();
State FUN_ST_BLINK();
State FUN_ST_INTERPRETATE();
State handlePeriodicActions();
void sendKeepAlive();
bool sendCCS();
int8_t vehicleIndex(char address);
inline bool isExpired(const Vehicle *vehicle);


// ==== VARIABLES ==== //

// Singleton instance of the radio driver
RH_NRF24 nrf24(10, 9); // CE, CS

#define _8_SPACES { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' }

/** The vehicles cache with info received from the network */
Vehicle vehicles[3] = {
    { 0, _8_SPACES, _8_SPACES, false, 0, ERA_STRAIGHT, ECA_STRAIGHT },
    { 0, _8_SPACES, _8_SPACES, false, 0, ERA_STRAIGHT, ECA_STRAIGHT },
    { 0, _8_SPACES, _8_SPACES, false, 0, ERA_STRAIGHT, ECA_STRAIGHT }
};

/**
 * This variable indicates the time at which the previous state ended.
 * In case of ST_BEGIN, it indicates the time at which the previous
 * protocol execution ended.
 *
 * Note that handlePeriodicActions() can updates the value too, in order to prepare
 * it for ST_BEGIN when resetting the procedure.
 */
unsigned long timeMarker = 0; // NOTE We can reduce accuracy to save space
unsigned long keepAliveTimeMarker = 0;
bool advertiseCCS = false;
uint16_t backoff = 0; // 0 means no backoff (*not* a zero-length backoff)
char currentPeer = 0;
uint16_t *fhtLeft;
uint16_t *fhtFront;
uint16_t *fhtRight;
uint16_t leftCCSIntensity = 0; // NOTE we can change it to uint8_t by scaling the value down in FUN_ST_BLINK
uint16_t frontCCSIntensity = 0;
uint16_t rightCCSIntensity = 0;
State state = ST_BEGIN;

typedef struct NetworkMessage {
    uint8_t size;
    uint8_t data[RH_NRF24_MAX_MESSAGE_LEN];
} NetworkMessage;

const uint8_t messages_len = 5;
NetworkMessage messages[messages_len];
uint8_t topMessageId = 0; // Index of the newest message


// ==== FUNCTION IMPLEMENTATIONS ==== //

bool setupCCS(uint16_t *_fhtLeft, uint16_t *_fhtFront, uint16_t *_fhtRight) {
    fhtLeft = _fhtLeft;
    fhtFront = _fhtFront;
    fhtRight = _fhtRight;

    if (!nrf24.init()) {
        Serial.println(F("Radio init failed!"));
        return false;
    }

    for (uint8_t i = 0; i < messages_len; i++) {
        messages[i].size = 0;
    }

    timeMarker = millis();
    return true;
}

/**
 * This function is meant to be called periodically. It should complete
 * its work as quick as possible and maintain an internal state so that
 * it can continue the next time it get called.
 */
void handleCCS() {
    switch (state) {
        case ST_BEGIN:
            state = FUN_ST_BEGIN();
            break;
        case ST_WAIT_TO_BLINK:
            state = FUN_ST_WAIT_TO_BLINK();
            break;
        case ST_BLINK:
            state = FUN_ST_BLINK();
            break;
        case ST_INTERPRETATE:
            state = FUN_ST_INTERPRETATE();
            break;
        default:
            Serial.println(F("INVALID STATE"));
    }
}

inline int positive_modulo(int i, int n) {
    return (i % n + n) % n;
}

void readCCSMessages() {
    while (nrf24.available())
    {
        uint8_t id = positive_modulo(topMessageId + 1, messages_len);
        uint8_t len = sizeof(messages[id].data);
        if (nrf24.recv(messages[id].data, &len))
        {
            messages[id].size = len;
            topMessageId = id;
        }
    }
}

State FUN_ST_BEGIN() {
    State r = handlePeriodicActions();
    if (r != ST_CURRENT) {
        return r;
    }

    // We use a delay because we want to unsynchronize the vehicles' loop
    if (backoff != 0) {
        const uint16_t timeToWait = 2 * TIMESPAN_X;
        if (millis() < timeMarker + timeToWait) {
            return ST_BEGIN;
        }

        delay(backoff);
        backoff = 0;
    } else {
        delayMicroseconds(random(0, TIMESPAN_RANDOM_DESYNC_US));
    }

    // NOTE What if, instead of trying to send and then hoping nothing collides, we first
    // listen to the channel to see if someone's talking? Kind of what 802.11 does:
    // https://en.wikipedia.org/wiki/Received_signal_strength_indication

    if (sendCCS()) {
        timeMarker = millis();
        return ST_WAIT_TO_BLINK;
    } else {
        timeMarker = millis();
        return ST_BEGIN;
    }
}

State FUN_ST_WAIT_TO_BLINK() {
    State r = handlePeriodicActions();
    if (r != ST_CURRENT) {
        return r;
    }

    if (millis() < timeMarker + TIMESPAN_X) { // TODO improve accuracy by waiting (TIMESPAN_X - half of the
                                              // time between consecutive received CCSs), only if I'm on
                                              // the receiver side
        return ST_WAIT_TO_BLINK;
    }

    advertiseCCS = true;
    timeMarker = millis();

    return ST_BLINK;
}

State FUN_ST_BLINK() {
    static bool sampled = false;
    //const uint16_t expectedFhtTime = 1998 * _us; // The time needed to compute a FHT (basically the duration of a call to readIrFrequencies(), without the sampling)
    //const uint16_t expectedProcessingTime = 3 * ((uint16_t)SAMPLING_PERIOD * (uint16_t)FHT_N + expectedFhtTime) / 1000; // ms

    /* Time needed to complete the three readIrFrequencies in the main loop */
    const uint16_t expectedProcessingTime = 261; // ms

    State r = handlePeriodicActions();
    if (r != ST_CURRENT) {
        return r;
    }

    if (millis() < timeMarker + TIMESPAN_X) {
        /**
         * Represents the time at which we should read the sampled (and transformed) data.
         *
         *   |------------ TIMESPAN_X ------------|
         *
         *   [........( sampling and fft )........]      <-- time diagram
         *                               ↳ we want to read here
         *            |------------------|
         *           expectedProcessingTime
         *
         * We compute the deadline time as follows:
         *     timeMarker                       // time at which ST_WAIT_TO_BLINK ended
         *   + (TIMESPAN_X / 2)                 // we center the start of the sampling within the total blinking period
         *   + (expectedProcessingTime / 2)     // we shift right so that the whole sampling is centered within the total blinking period
         */
        const unsigned long deadline = timeMarker + (TIMESPAN_X / 2) + (expectedProcessingTime / 2);

        if (!sampled && deadline < millis()) {
            // We already have the FHTs done in fhtLeft, fhtFront, fhtRight.
            // We extract the frequency we're interested in, and we pass it to ST_INTERPRETATE.

            leftCCSIntensity = fhtLeft[LED_CCS_BIN];
            frontCCSIntensity = fhtFront[LED_CCS_BIN];
            rightCCSIntensity = fhtRight[LED_CCS_BIN];
            if (LED_CCS_BIN > 0) {
                leftCCSIntensity = max(leftCCSIntensity, fhtLeft[LED_CCS_BIN-1]);
                frontCCSIntensity = max(frontCCSIntensity, fhtFront[LED_CCS_BIN-1]);
                rightCCSIntensity = max(rightCCSIntensity, fhtRight[LED_CCS_BIN-1]);
            }
            if (LED_CCS_BIN < (FHT_N / 2)-1) {
                leftCCSIntensity = max(leftCCSIntensity, fhtLeft[LED_CCS_BIN+1]);
                frontCCSIntensity = max(frontCCSIntensity, fhtFront[LED_CCS_BIN+1]);
                rightCCSIntensity = max(rightCCSIntensity, fhtRight[LED_CCS_BIN+1]);
            }

            sampled = true;
        }

        return ST_BLINK;
    }

    sampled = false;
    advertiseCCS = false;

    return ST_INTERPRETATE;
}

State FUN_ST_INTERPRETATE() {
    State r = handlePeriodicActions();
    if (r != ST_CURRENT) {
        return r;
    }

    const uint8_t absNoiseThreshold = 20;
    const uint8_t diffThresholdPercentage = 20;

    int8_t destIndex = -1;

    // Do something with leftCCSIntensity, frontCCSIntensity, rightCCSIntensity
    if (leftCCSIntensity >= absNoiseThreshold &&
        leftCCSIntensity * (100-diffThresholdPercentage) / 100 > frontCCSIntensity &&
        leftCCSIntensity * (100-diffThresholdPercentage) / 100 > rightCCSIntensity) {

            destIndex = 0;

    } else if (frontCCSIntensity >= absNoiseThreshold &&
               frontCCSIntensity * (100-diffThresholdPercentage) / 100 > leftCCSIntensity &&
               frontCCSIntensity * (100-diffThresholdPercentage) / 100 > rightCCSIntensity) {

            destIndex = 1;

    } else if (rightCCSIntensity >= absNoiseThreshold &&
               rightCCSIntensity * (100-diffThresholdPercentage) / 100 > leftCCSIntensity &&
               rightCCSIntensity * (100-diffThresholdPercentage) / 100 > frontCCSIntensity) {

            destIndex = 2;
    }

    if (destIndex >= 0) {
        if (crossroad[destIndex].validUntil > millis()) {
            int8_t vehicleId = vehicleIndex(currentPeer);
            if (vehicleId != -1) {
                // The "orientation" field is set by interpretateSensorData in the main file.
                crossroad[destIndex].address = vehicles[vehicleId].address;
                memcpy(&crossroad[destIndex].manufacturer, vehicles[vehicleId].manufacturer, 8);
                memcpy(&crossroad[destIndex].model, vehicles[vehicleId].model, 8);
                crossroad[destIndex].priority = vehicles[vehicleId].priority;
                crossroad[destIndex].requestedAction = vehicles[vehicleId].requestedAction;
                crossroad[destIndex].currentAction = vehicles[vehicleId].currentAction;
            }
        }
    }

    timeMarker = millis();
    return ST_BEGIN;
}

/**
 * Given the address of a vehicle, returns its index in the vehicles array.
 * If no vehicle is found, returns -1.
 */
int8_t vehicleIndex(char address) {
    for (uint8_t i = 0; i < 3; i++) {
        if (address == vehicles[i].address) {
            return i;
        }
    }

    return -1;
}

/**
 * @return  if different than ST_CURRENT, indicates the state that the caller
 *          should immediately return.
 */
State handlePeriodicActions() {
    readCCSMessages();

    if (keepAliveTimeMarker + TIMESPAN_KEEPALIVE <= millis()) {
        sendKeepAlive();
        keepAliveTimeMarker = millis();
    }

    for (uint8_t i = 0; i < messages_len; i++)
    {
        // Start from the oldest message
        uint8_t index = positive_modulo(topMessageId - (messages_len - i - 1), messages_len);
        if (messages[index].size == 0) {
            continue;
        }

        uint8_t *buf = messages[index].data;
        // Remove the message from the buffer
        messages[index].size = 0;

        if (buf[0] == MSG_TYPE_KEEPALIVE) {
            // KeepAlive

            // find vehicle in the cache
            uint8_t index = 255; // 255 = not found
            for (uint8_t i = 0; i < 3; i++) {
                if (vehicles[i].address == buf[1]) {
                    index = i;
                    break;
                }
            }

            if (index == 255) {
                // vehicle wasn't in the cache,
                // pick oldest entry in vehicles
                unsigned long min = ULONG_MAX;
                for (uint8_t i = 0; i < 3; i++) {
                    if (vehicles[i].receivedTime < min) {
                        min = vehicles[i].receivedTime;
                        index = i;
                    }
                }
            }

            // replace the oldest entry with the new info
            if (isValidRequestedAction(buf[2]) && isValidCurrentAction(buf[3])) {
                vehicles[index].address = buf[1];
                vehicles[index].requestedAction = static_cast<RequestedAction>(buf[2]);
                vehicles[index].currentAction = static_cast<CurrentAction>(buf[3]);
                memcpy(&(vehicles[index].manufacturer), &buf[4], 8);
                memcpy(&(vehicles[index].model), &buf[12], 8);
                vehicles[index].priority = buf[20] != 0;
                vehicles[index].receivedTime = millis();

                // If it is present in crossroad, update its information
                for (uint8_t i = 0; i < 3; i++) {
                    if (crossroad[i].address == vehicles[index].address) {
                        crossroad[i].priority = vehicles[index].priority;
                        crossroad[i].requestedAction = vehicles[index].requestedAction;
                        crossroad[i].currentAction = vehicles[index].currentAction;
                        break;
                    }
                }
            }

        } else if (buf[0] == MSG_TYPE_CCS) {
            // CCS
            const bool isForMe = buf[1] == ADDRESS;
            if (state == ST_BEGIN) {
                if (isForMe) {
                    // Find the peer in vehicles
                    int8_t vehicleId = vehicleIndex(buf[2]);
                    if (vehicleId != -1) {
                        currentPeer = vehicles[vehicleId].address;
                        timeMarker = millis();
                        return ST_WAIT_TO_BLINK;
                    } else {
                        // We received a CCS request from someone which is not in our cache
                        currentPeer = buf[2];
                        timeMarker = millis();
                        return ST_WAIT_TO_BLINK;
                    }
                } else {
                    backoff = random(1, TIMESPAN_MAX_BACKOFF);
                    timeMarker = millis();
                    return ST_BEGIN;
                }
            } else if (state == ST_WAIT_TO_BLINK || state == ST_BLINK) {
                const char sender = buf[2];
                if (!(isForMe && sender == currentPeer)) {
                    // Send pardoned FCT
                    uint8_t data[2];
                    data[0] = MSG_TYPE_FCT;
                    data[1] = currentPeer;
                    nrf24.send(data, sizeof(data));
                    nrf24.waitPacketSent();
                }
            } else if (state == ST_INTERPRETATE) {
                if (isForMe) {
                    // Send non-pardoned FCT
                    uint8_t data[2];
                    data[0] = MSG_TYPE_FCT;
                    data[1] = '\0';
                    nrf24.send(data, sizeof(data));
                    nrf24.waitPacketSent();
                }
            }

        } else if (buf[0] == MSG_TYPE_FCT) {
            // FCT
            const bool pardoned = buf[1] == ADDRESS;
            if (state == ST_BEGIN || (state == ST_WAIT_TO_BLINK && !pardoned) || (state == ST_BLINK && !pardoned)) {
                advertiseCCS = false;

                // Choose backoff
                backoff = random(1, TIMESPAN_MAX_BACKOFF);
                timeMarker = millis();

                // Instruct to go back to begin
                return ST_BEGIN;
            }
        }
    }

    return ST_CURRENT;
}

void sendKeepAlive() {
    uint8_t data[21];
    data[0] = MSG_TYPE_KEEPALIVE;
    data[1] = ADDRESS;
    data[2] = requestedAction;
    data[3] = currentAction;
    memcpy(&data[4], &(MANUFACTURER),  8);
    memcpy(&data[12], &(MODEL), 8);
    data[20] = hasPriority;

    nrf24.send(data, sizeof(data));
    nrf24.waitPacketSent();
}

bool sendCCS() {
    static uint8_t vehicleId = 0;

    // Find an unexpired vehicle to send the CCS using a Round-Robin policy
    for (uint8_t i = 1; i < 3+1; i++) {
        if (!isExpired(&vehicles[(i + vehicleId) % 3])) {
            vehicleId = (i + vehicleId) % 3;
            break;
        }
    }

    if (!isExpired(&vehicles[vehicleId])) {
        // The cache is not expired, which means an actual vehicle has been found
        currentPeer = vehicles[vehicleId].address;

        uint8_t data[3];
        data[0] = MSG_TYPE_CCS;
        data[1] = vehicles[vehicleId].address;
        data[2] = ADDRESS;

        nrf24.send(data, sizeof(data));
        nrf24.waitPacketSent();

        return true;
    }

    return false;
}

bool isExpired(const Vehicle *vehicle) {
    const unsigned long currTime = millis();
    return (vehicle->receivedTime + VEHICLE_CACHE_TTL < currTime) || (currTime < VEHICLE_CACHE_TTL);
}