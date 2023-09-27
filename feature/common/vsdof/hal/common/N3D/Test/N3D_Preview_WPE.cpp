#include "H3D_HAL_UT_WPE.h"

class N3D_Preview_WPE : public N3DHALUTWPEBase
{
public:
    N3D_Preview_WPE() { init(); }
    virtual ~N3D_Preview_WPE() {}
protected:
    virtual const char *getUTCaseName() { return "N3D_Preview_WPE"; }
};

TEST_F(N3D_Preview_WPE, TEST)
{
    _pN3DKernel->WarpMain1(_inputImageMain1.get(), _outputImageMain1.get(), _outputMaskMain1.get());
    // _pN3DKernel->WarpMain2(_previewParam, _afInfoMain, _afInfoAuxi, _previewOutput);
    // _pN3DKernel->runLearning(_hwfefmData);

    // _pN3DKernel->updateNVRAM(&_nvram);

    //Dump output
    char dumpPath[256];
    MSize dumpSize = _outputImageMain1.get()->getImgSize();
    if(sprintf(dumpPath, UT_CASE_OUT_FOLDER"/BID_N3D_OUT_MV_Y_reqID_0__%dx%d_8_s0.yv12", dumpSize.w, dumpSize.h) >= 0)
    {
        _outputImageMain1.get()->saveToFile(dumpPath);
    }

    dumpSize = _outputMaskMain1.get()->getImgSize();
    if(sprintf(dumpPath, UT_CASE_OUT_FOLDER"/BID_N3D_OUT_MASK_M_reqID_0__%dx%d_8_s0.unknown", dumpSize.w, dumpSize.h) >= 0)
    {
        _outputMaskMain1.get()->saveToFile(dumpPath);
    }
}
