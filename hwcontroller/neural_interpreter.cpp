#include "neural_interpreter.h"
#include "common.h"
#include <avr/pgmspace.h>

#define INPUT_SIZE  30
#define OUTPUT_SIZE  8

static const float W[OUTPUT_SIZE][INPUT_SIZE] PROGMEM = {
    {1.775617599487304688e+00,2.802958965301513672e+00,7.459294795989990234e-01,-3.656647354364395142e-02,7.750281095504760742e-01,1.523251056671142578e+00,-1.090576276183128357e-01,1.503545165061950684e+00,7.702293992042541504e-01,1.183588743209838867e+00,3.974121809005737305e-01,1.568365693092346191e+00,9.788104295730590820e-01,2.507370710372924805e+00,1.031692147254943848e+00,3.272987306118011475e-01,4.484952092170715332e-01,-3.821421647444367409e-03,-4.824394583702087402e-01,-9.399337768554687500e-01,-2.260819375514984131e-01,-2.925634086132049561e-01,-3.706515580415725708e-02,-1.584395766258239746e-01,1.560788154602050781e-01,-1.086303293704986572e-01,-1.953092664480209351e-01,-7.421265244483947754e-01,-5.468401908874511719e-01,1.016547083854675293e+00},
    {-3.314923644065856934e-01,-3.418840408325195312e+00,-5.184948444366455078e-01,-2.548848390579223633e+00,-1.056260704994201660e+00,1.468386035412549973e-02,-6.496915340423583984e+00,-1.362059354782104492e+00,-5.167384147644042969e-01,-1.267956942319869995e-01,1.885680675506591797e+00,3.256483376026153564e-02,2.257135808467864990e-01,2.745921611785888672e-01,-1.570301502943038940e-01,1.639104843139648438e+00,1.307916641235351562e+00,4.009457230567932129e-01,4.000521898269653320e-01,-2.166443347930908203e+00,2.043535411357879639e-01,7.500487565994262695e-01,7.378987073898315430e-01,7.030028104782104492e-02,-2.260841578245162964e-01,-2.346417456865310669e-01,1.204799413681030273e+00,9.190534353256225586e-01,4.608459174633026123e-01,-6.761690974235534668e-01},
    {-3.877902925014495850e-01,-4.449991881847381592e-01,1.249293446540832520e+00,-1.725304245948791504e+00,-1.093546986579895020e+00,-3.293799469247460365e-03,3.106155633926391602e+00,1.276354670524597168e+00,-2.022995501756668091e-01,-1.570803523063659668e-01,-4.241185188293457031e+00,-4.785688221454620361e-01,-6.290406584739685059e-01,-5.685862302780151367e-01,-4.636932611465454102e-01,3.626718282699584961e+00,6.459150314331054688e-01,-9.752490520477294922e-01,-9.138956069946289062e-01,-1.255899548530578613e+00,7.716130614280700684e-01,4.989235103130340576e-01,3.543241694569587708e-02,3.446330130100250244e-01,-3.708139359951019287e-01,2.131604671478271484e+00,1.627158001065254211e-02,-5.500720739364624023e-01,-4.235852658748626709e-01,-1.110789328813552856e-01},
    {-6.107680797576904297e-01,1.400020480155944824e+00,-9.778590798377990723e-01,-1.306939959526062012e+00,-7.061159014701843262e-01,-7.749951481819152832e-01,4.525763988494873047e+00,-2.033372074365615845e-01,-2.562452256679534912e-01,-3.734005987644195557e-01,4.003107070922851562e+00,-4.207362234592437744e-01,1.062078848481178284e-01,-1.008341550827026367e+00,-4.721959307789802551e-02,1.626963019371032715e+00,8.488088846206665039e-01,-8.096882700920104980e-01,-1.124838709831237793e+00,-2.128626346588134766e+00,8.156850337982177734e-01,-3.140313029289245605e-01,-5.754714608192443848e-01,1.244359388947486877e-01,-1.659353375434875488e+00,-3.201050758361816406e-01,-4.511941224336624146e-03,7.522456645965576172e-01,-7.131495326757431030e-02,-1.783366322517395020e+00},
    {-7.475540973246097565e-03,2.518760561943054199e-01,-1.788057237863540649e-01,4.535668849945068359e+00,3.126109838485717773e-01,-5.103671550750732422e-02,-2.541399955749511719e+00,-6.831007599830627441e-01,8.047692179679870605e-01,1.704501174390316010e-02,-2.896836757659912109e+00,-2.605143785476684570e-01,-4.598968923091888428e-01,-1.213920488953590393e-01,-2.389636039733886719e-01,-4.301857352256774902e-01,-4.888609945774078369e-01,8.598434329032897949e-01,2.452253252267837524e-01,1.109684109687805176e+00,-3.922733664512634277e-01,-1.698332428932189941e-01,-2.955640554428100586e-01,-1.149298399686813354e-01,2.206230401992797852e+00,1.002433449029922485e-01,3.546113967895507812e-01,-4.629671573638916016e-01,-7.983152568340301514e-02,1.480431556701660156e+00},
    {-1.644554585218429565e-01,-7.900564372539520264e-02,-1.040473673492670059e-02,9.968091845512390137e-01,8.742116689682006836e-01,-4.236698448657989502e-01,-1.916944622993469238e+00,-6.776040196418762207e-01,-2.770588696002960205e-01,-2.135624438524246216e-01,3.046733379364013672e+00,-1.974480152130126953e-01,2.128361612558364868e-01,-4.872855544090270996e-01,8.726134896278381348e-02,-3.502702951431274414e+00,-1.164453387260437012e+00,3.156322985887527466e-03,5.649309754371643066e-01,3.451566457748413086e+00,-8.037850260734558105e-01,-2.888895869255065918e-01,-6.749713420867919922e-01,-4.401127099990844727e-01,1.637309908866882324e+00,-5.102036520838737488e-02,4.459336772561073303e-02,7.851188182830810547e-01,-3.136521903797984123e-03,-1.477427959442138672e+00},
    {-2.948495745658874512e-01,-2.327489703893661499e-01,-2.708773016929626465e-01,4.365440905094146729e-01,8.941333293914794922e-01,-2.230699360370635986e-01,3.695527315139770508e+00,2.714018821716308594e-01,-2.537755966186523438e-01,-1.658318191766738892e-01,-1.991463780403137207e+00,-3.655252158641815186e-01,-4.983066916465759277e-01,-4.716007709503173828e-01,-1.990496218204498291e-01,-3.189368963241577148e+00,-1.240115642547607422e+00,7.776383161544799805e-01,1.557221889495849609e+00,2.326730966567993164e+00,1.291795223951339722e-01,6.409890949726104736e-02,1.142151713371276855e+00,4.426164925098419189e-01,-1.389590144157409668e+00,-8.792893886566162109e-01,-1.048439025878906250e+00,-4.912267327308654785e-01,8.448519110679626465e-01,1.903455615043640137e+00},
    {-9.897769987583160400e-02,-1.369382143020629883e-01,-8.848320692777633667e-02,-1.332478523254394531e-01,-7.135222852230072021e-02,-6.343733519315719604e-02,-2.844653129577636719e-01,-6.991261243820190430e-02,-4.497965052723884583e-02,3.553322702646255493e-02,-2.813617289066314697e-01,-2.990043535828590393e-02,-1.139732748270034790e-01,-6.781265884637832642e-02,1.172644272446632385e-02,-3.445879518985748291e-01,-3.066362142562866211e-01,-4.158253967761993408e-01,-2.752136290073394775e-01,-2.763440012931823730e-01,-4.396421611309051514e-01,-3.336011469364166260e-01,-2.856976687908172607e-01,-2.877063155174255371e-01,-2.005815654993057251e-01,-3.298978507518768311e-01,-3.713607788085937500e-01,-4.034903943538665771e-01,-2.733865380287170410e-01,-1.392317116260528564e-01},
};

static const float b[OUTPUT_SIZE] PROGMEM = { -2.333042770624160767e-01,6.632186174392700195e-01,4.364839494228363037e-01,-6.262275576591491699e-01,5.243270993232727051e-01,-2.948955297470092773e-01,1.413777768611907959e-01,-6.109816431999206543e-01 };

/**
 * Last returned value from neuralInterpretate.
 */
CrossroadStatus lastReturned = {0, 0, 0};

/**
 * Previous results calculated by the neural network.
 * Notice that they may differ from the actual returned value.
 */
CrossroadStatus prevResult[2] = {
    {0, 0, 0},
    {0, 0, 0}
};

/**
 * Calculate ranks from the input vector and store them in the output vector.
 * The computed values are actually the sorted indices, rather than usual ranks.
 *
 * Note that this function only works on arrays of size 5 for efficiency reasons.
 *
 * Example:
 *     d: [ 44,   23,   71,    9,   88 ]
 *   out: [0.75, 0.25, 0.00, 0.50, 1.00]
 */
static inline void ranks5(uint16_t *d_orig, float *out) {
    uint8_t i, j;
    out[0] = 0.0;
    out[1] = 0.25;
    out[2] = 0.50;
    out[3] = 0.75;
    out[4] = 1.0;

    uint16_t d[5];
    memcpy(&d, d_orig, sizeof(uint16_t) * 5);

    for (i = 1; i < 5; i++) {
        uint16_t tmp = d[i];
        float tmpi = out[i];
        for (j = i; j >= 1 && tmp < d[j-1]; j--) {
                d[j] = d[j-1];
                out[j] = out[j-1];
        }
        d[j] = tmp;
        out[j] = tmpi;
    }
}

static inline bool crossroadEquals(CrossroadStatus *c1, CrossroadStatus *c2) {
    return c1->left == c2 -> left && c1->front == c2->front && c1->right == c2->right;
}

static inline void preprocess(uint16_t *fhtLeft, uint16_t *fhtFront, uint16_t *fhtRight, float out[INPUT_SIZE]) {
    // Integer vector
    uint16_t val[15];

    val[0] = max(fhtLeft[LED1_BIN], max(fhtLeft[LED1_BIN-1], fhtLeft[LED1_BIN+1]));
    val[1] = max(fhtLeft[LED2_BIN], max(fhtLeft[LED2_BIN-1], fhtLeft[LED2_BIN+1]));
    val[2] = max(fhtLeft[LED3_BIN], max(fhtLeft[LED3_BIN-1], fhtLeft[LED3_BIN+1]));
    val[3] = max(fhtLeft[LED4_BIN], max(fhtLeft[LED4_BIN-1], fhtLeft[LED4_BIN+1]));
    val[4] = max(fhtLeft[LED5_BIN], max(fhtLeft[LED5_BIN-1], fhtLeft[LED5_BIN+1]));
    val[5] = max(fhtFront[LED1_BIN], max(fhtFront[LED1_BIN-1], fhtFront[LED1_BIN+1]));
    val[6] = max(fhtFront[LED2_BIN], max(fhtFront[LED2_BIN-1], fhtFront[LED2_BIN+1]));
    val[7] = max(fhtFront[LED3_BIN], max(fhtFront[LED3_BIN-1], fhtFront[LED3_BIN+1]));
    val[8] = max(fhtFront[LED4_BIN], max(fhtFront[LED4_BIN-1], fhtFront[LED4_BIN+1]));
    val[9] = max(fhtFront[LED5_BIN], max(fhtFront[LED5_BIN-1], fhtFront[LED5_BIN+1]));
    val[10] = max(fhtRight[LED1_BIN], max(fhtRight[LED1_BIN-1], fhtRight[LED1_BIN+1]));
    val[11] = max(fhtRight[LED2_BIN], max(fhtRight[LED2_BIN-1], fhtRight[LED2_BIN+1]));
    val[12] = max(fhtRight[LED3_BIN], max(fhtRight[LED3_BIN-1], fhtRight[LED3_BIN+1]));
    val[13] = max(fhtRight[LED4_BIN], max(fhtRight[LED4_BIN-1], fhtRight[LED4_BIN+1]));
    val[14] = max(fhtRight[LED5_BIN], max(fhtRight[LED5_BIN-1], fhtRight[LED5_BIN+1]));

    // Compute ranks
    ranks5(&(val[0]), &(out[15]));
    ranks5(&(val[5]), &(out[20]));
    ranks5(&(val[10]), &(out[25]));

    // Compute max
    float maxIntensity = val[0];
    for (uint8_t i = 0; i < 15; i++) {
        if (val[i] > maxIntensity) {
            maxIntensity = val[i];
        }
    }

    // Normalize intensity
    for (uint8_t i = 0; i < 15; i++) {
        out[i] = val[i] / maxIntensity;
    }
}

CrossroadStatus neuralInterpretate(uint16_t *fhtLeft, uint16_t *fhtFront, uint16_t *fhtRight) {
    // We make them static so that:
    //  1. we can be more efficient (no stack allocations)
    //  2. as they're big, we're always sure to have enough memory to hold them
    static float x[INPUT_SIZE];
    static float y[OUTPUT_SIZE];
    static float Wrow[INPUT_SIZE];

    // Fill x with the preprocessed input data
    preprocess(fhtLeft, fhtFront, fhtRight, x);

    // Initialize y = b so that we don't have to sum it later
    memcpy_P(y, &b, sizeof(float) * OUTPUT_SIZE);

    // Matrix multiplication
    for (uint8_t i = 0; i < OUTPUT_SIZE; i++) {
        // Get a row from progmem
        memcpy_P(Wrow, &(W[i]), sizeof(float) * INPUT_SIZE);
        for (uint8_t j = 0; j < INPUT_SIZE; j++) {
            y[i] += x[j] * Wrow[j];
        }
    }

    // Interpretate the result
    uint8_t maxIndex = 0;
    for (uint8_t i = 1; i < OUTPUT_SIZE; i++) {
        if (y[i] > y[maxIndex]) {
            maxIndex = i;
        }
    }

    CrossroadStatus result;
    result.left = ((maxIndex >> 2) & 1) == 1;  // maxIndex IN (4, 5, 6, 7) -> 1xx
    result.front = ((maxIndex >> 1) & 1) == 1; // maxIndex IN (2, 3, 6, 7) -> x1x
    result.right = ((maxIndex >> 0) & 1) == 1; // maxIndex IN (1, 3, 5, 7) -> xx1

    CrossroadStatus retval;

    // Checks for error tolerance.
    if (crossroadEquals(&result, &(prevResult[0]))) {
        retval = result;
    } else if (crossroadEquals(&result, &(prevResult[1]))) {
        retval = result;
    } else if (crossroadEquals(&(prevResult[0]), &(prevResult[1]))) {
        retval = prevResult[0];
    } else {
        retval = lastReturned;
    }

    lastReturned = retval;
    prevResult[1] = prevResult[0];
    prevResult[0] = result;

    return retval;
}