#pragma GCC optimize ("O3")
#include "neural_interpreter.h"
#include <Arduino.h>
#include <avr/pgmspace.h>

#define INPUT_SIZE  30
#define OUTPUT_SIZE  8

static const float W[OUTPUT_SIZE][INPUT_SIZE] PROGMEM = {
    {-5.174560938030481339e-03,4.323113709688186646e-02,1.574998050928115845e-01,1.356151551008224487e-01,-7.472777366638183594e+00,7.874737679958343506e-02,8.271104097366333008e-02,-8.267157554626464844e+00,5.144594609737396240e-02,1.229949668049812317e-01,-5.944294929504394531e+00,9.134565293788909912e-02,1.320530381053686142e-02,2.571480348706245422e-02,6.535486318171024323e-03,1.520500898361206055e+00,1.467838287353515625e+00,1.418365359306335449e+00,1.329865097999572754e+00,-1.343924283981323242e+00,8.049144744873046875e-01,7.550454139709472656e-01,6.920535564422607422e-01,3.686124086380004883e-01,1.608568310737609863e+00,1.726328879594802856e-01,1.215328946709632874e-01,1.012995615601539612e-01,-1.962472796440124512e-01,4.185398578643798828e+00},
    {-1.567403040826320648e-02,-3.388966247439384460e-02,-3.744800761342048645e-02,-3.510410338640213013e-02,-7.589513301849365234e+00,2.788204699754714966e-02,5.815893784165382385e-02,-8.218542098999023438e+00,6.792902201414108276e-02,9.239750797860324383e-04,6.348986625671386719e+00,4.850079119205474854e-02,9.472329914569854736e-02,7.168298959732055664e-02,9.189595282077789307e-02,7.758669853210449219e-01,8.326662182807922363e-01,8.708836436271667480e-01,7.112972140312194824e-01,-1.934093713760375977e+00,3.960700705647468567e-02,2.052290439605712891e-01,1.704560965299606323e-01,-2.123631834983825684e-01,9.341653585433959961e-01,1.071541190147399902e+00,1.058776259422302246e+00,1.015639781951904297e+00,1.302021980285644531e+00,-3.275307416915893555e+00},
    {-1.326852012425661087e-02,3.265880793333053589e-02,1.816290616989135742e-02,1.129292100667953491e-01,-7.707635879516601562e+00,9.232232719659805298e-02,6.018499284982681274e-02,8.634367942810058594e+00,-9.120551985688507557e-04,4.158794134855270386e-02,-6.048325538635253906e+00,1.842914894223213196e-02,-4.956730175763368607e-03,7.180666178464889526e-02,1.251200679689645767e-02,1.003541111946105957e+00,9.655680656433105469e-01,9.301207065582275391e-01,8.347409367561340332e-01,-1.789544463157653809e+00,4.308866858482360840e-01,4.899841845035552979e-01,5.124403834342956543e-01,7.440149784088134766e-01,-2.667925655841827393e-01,-4.557971358299255371e-01,-4.775492846965789795e-01,-3.765933215618133545e-01,-7.162643074989318848e-01,3.624796628952026367e+00},
    {-1.789971813559532166e-02,-5.510190874338150024e-02,-3.347074985504150391e-02,2.197545208036899567e-02,-7.958334445953369141e+00,1.847334206104278564e-02,2.907848730683326721e-02,8.717588424682617188e+00,-5.221234261989593506e-02,2.599002048373222351e-02,6.269541740417480469e+00,-3.082588591496460140e-05,2.418964914977550507e-02,2.860078960657119751e-02,5.453054606914520264e-02,3.674248754978179932e-01,4.271007776260375977e-01,3.782775104045867920e-01,2.921128571033477783e-01,-2.516430616378784180e+00,-1.992981731891632080e-01,-1.307842284440994263e-01,-1.091350913047790527e-01,2.002646327018737793e-01,-9.627278447151184082e-01,5.521728992462158203e-01,6.260806918144226074e-01,5.566620230674743652e-01,8.450858592987060547e-01,-3.752229213714599609e+00},
    {8.409234136343002319e-02,-5.451032891869544983e-02,8.826982229948043823e-02,-3.673277795314788818e-02,8.060776710510253906e+00,9.872677735984325409e-03,-8.795814588665962219e-03,-8.771212577819824219e+00,4.318473488092422485e-02,-9.921794990077614784e-04,-6.371617794036865234e+00,3.319921344518661499e-02,6.324592977762222290e-02,5.861177574843168259e-03,-2.964457124471664429e-02,-5.400705933570861816e-01,-5.533349514007568359e-01,-5.204930305480957031e-01,-3.778814673423767090e-01,2.712501525878906250e+00,1.924300491809844971e-01,6.212535500526428223e-02,7.578328996896743774e-02,-1.670269072055816650e-01,8.533681035041809082e-01,-5.360611081123352051e-01,-6.257094740867614746e-01,-5.761876702308654785e-01,-8.088610768318176270e-01,3.495594501495361328e+00},
    {7.186581939458847046e-02,-6.167155131697654724e-02,-6.270042061805725098e-02,-8.410155028104782104e-02,7.624531745910644531e+00,-3.396334499120712280e-02,-4.380069673061370850e-02,-8.356470108032226562e+00,-8.194985985755920410e-02,-5.805407091975212097e-02,5.861925601959228516e+00,-1.094590406864881516e-02,3.981524333357810974e-02,-3.617345914244651794e-02,3.428677469491958618e-02,-9.198326468467712402e-01,-1.039330244064331055e+00,-8.685953617095947266e-01,-8.007493019104003906e-01,1.695275187492370605e+00,-4.851323664188385010e-01,-4.669991135597229004e-01,-4.247808456420898438e-01,-7.377846837043762207e-01,3.361880481243133545e-01,3.802911043167114258e-01,3.718129098415374756e-01,3.133928775787353516e-01,5.855860114097595215e-01,-3.414358377456665039e+00},
    {2.587336860597133636e-02,-1.510555204004049301e-02,3.179832547903060913e-02,-7.746997475624084473e-02,7.542304992675781250e+00,-7.593306899070739746e-02,5.301274359226226807e-02,8.034039497375488281e+00,-7.030908018350601196e-03,4.948192834854125977e-02,-6.129320144653320312e+00,-2.681781724095344543e-02,-1.251405104994773865e-02,-6.489388644695281982e-03,3.755271434783935547e-02,-7.587469220161437988e-01,-9.050998091697692871e-01,-7.051690220832824707e-01,-8.305149674415588379e-01,1.915488719940185547e+00,-1.239581704139709473e-01,-2.206771075725555420e-01,-1.602651178836822510e-01,2.303100377321243286e-01,-8.705323338508605957e-01,-9.438465237617492676e-01,-9.860798716545104980e-01,-9.571824669837951660e-01,-1.237997770309448242e+00,2.932813882827758789e+00},
    {-1.250160932540893555e-01,-9.565238840878009796e-03,-1.109689623117446899e-01,-9.098423272371292114e-02,7.498820304870605469e+00,-1.409978121519088745e-01,-3.297085314989089966e-02,8.291591644287109375e+00,-9.904710203409194946e-02,-5.311944708228111267e-02,6.087755203247070312e+00,-3.753607347607612610e-02,-2.569223381578922272e-02,-5.767070502042770386e-02,-1.283250935375690460e-02,-1.393471121788024902e+00,-1.319661736488342285e+00,-1.446302175521850586e+00,-1.187134146690368652e+00,1.089385986328125000e+00,-7.773537635803222656e-01,-6.215009689331054688e-01,-7.181451320648193359e-01,-5.252249836921691895e-01,-1.594378829002380371e+00,-7.475144416093826294e-02,-1.041681990027427673e-01,-9.867779910564422607e-02,1.382901221513748169e-01,-4.014429092407226562e+00},
};

static const float b[OUTPUT_SIZE] PROGMEM = { 2.179429054260253906e+00, 6.460731029510498047e-01, 8.804201483726501465e-01, -5.919578075408935547e-01, 4.379630982875823975e-01, -8.781525492668151855e-01, -5.976638793945312500e-01, -2.076110363006591797e+00 };

CrossroadStatus neuralInterpretate(uint16_t *fhtLeft, uint16_t *fhtFront, uint16_t *fhtRight) {
    // We make them static so that:
    //  1. we can be more efficient (no stack allocations)
    //  2. as they're big, we're always sure to have enough memory to hold them
    static float x[INPUT_SIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    static float y[OUTPUT_SIZE];
    static float Wrow[INPUT_SIZE];

    // FIXME Preprocess fhtLeft, fhtFront, fhtRight to fill x

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
    result.left = ((maxIndex >> 2) & 1) == 1; // maxIndex IN (4, 5, 6, 7)
    result.front = ((maxIndex >> 1) & 1) == 1; // maxIndex IN (2, 3, 6, 7)
    result.right = ((maxIndex >> 0) & 1) == 1; // maxIndex IN (1, 3, 5, 7)

    return result;
}