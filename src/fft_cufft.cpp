#include "fft_cufft.h"
#include <cublas_v2.h>

cuFFT::cuFFT()
{
    CublasErrorCheck(cublasCreate(&cublas));
}

void cuFFT::init(unsigned width, unsigned height, unsigned num_of_feats, unsigned num_of_scales)
{
    m_width = width;
    m_height = height;
    m_num_of_feats = num_of_feats;
    m_num_of_scales = num_of_scales;

    std::cout << "FFT: cuFFT" << std::endl;

    // FFT forward one scale
    {
        CufftErrorCheck(cufftPlan2d(&plan_f, int(m_height), int(m_width), CUFFT_R2C));
    }
#ifdef BIG_BATCH
    // FFT forward all scales
    if (m_num_of_scales > 1 && BIG_BATCH_MODE) {
        int rank = 2;
        int n[] = {(int)m_height, (int)m_width};
        int howmany = m_num_of_scales;
        int idist = m_height * m_width, odist = m_height * (m_width / 2 + 1);
        int istride = 1, ostride = 1;
        int *inembed = n, onembed[] = {(int)m_height, (int)m_width / 2 + 1};

        CufftErrorCheck(cufftPlanMany(&plan_f_all_scales, rank, n, inembed, istride, idist, onembed, ostride, odist,
                                      CUFFT_R2C, howmany));
        CufftErrorCheck(cufftSetStream(plan_f_all_scales, cudaStreamPerThread));
    }
#endif
    // FFT forward window one scale
    {
        int rank = 2;
        int n[] = {int(m_height), int(m_width)};
        int howmany = int(m_num_of_feats);
        int idist = int(m_height * m_width), odist = int(m_height * (m_width / 2 + 1));
        int istride = 1, ostride = 1;
        int *inembed = n, onembed[] = {int(m_height), int(m_width / 2 + 1)};

        CufftErrorCheck(
            cufftPlanMany(&plan_fw, rank, n, inembed, istride, idist, onembed, ostride, odist, CUFFT_R2C, howmany));
        CufftErrorCheck(cufftSetStream(plan_fw, cudaStreamPerThread));
    }
#ifdef BIG_BATCH
    // FFT forward window all scales all feats
    if (m_num_of_scales > 1 && BIG_BATCH_MODE) {
        int rank = 2;
        int n[] = {(int)m_height, (int)m_width};
        int howmany = m_num_of_scales * m_num_of_feats;
        int idist = m_height * m_width, odist = m_height * (m_width / 2 + 1);
        int istride = 1, ostride = 1;
        int *inembed = n, onembed[] = {(int)m_height, (int)m_width / 2 + 1};

        CufftErrorCheck(cufftPlanMany(&plan_fw_all_scales, rank, n, inembed, istride, idist, onembed, ostride, odist,
                                      CUFFT_R2C, howmany));
        CufftErrorCheck(cufftSetStream(plan_fw_all_scales, cudaStreamPerThread));
    }
#endif
    // FFT inverse one scale
    {
        int rank = 2;
        int n[] = {int(m_height), int(m_width)};
        int howmany = int(m_num_of_feats);
        int idist = int(m_height * (m_width / 2 + 1)), odist = 1;
        int istride = 1, ostride = int(m_num_of_feats);
        int inembed[] = {int(m_height), int(m_width / 2 + 1)}, *onembed = n;

        CufftErrorCheck(cufftPlanMany(&plan_i_features, rank, n, inembed, istride, idist, onembed, ostride, odist,
                                      CUFFT_C2R, howmany));
        CufftErrorCheck(cufftSetStream(plan_i_features, cudaStreamPerThread));
    }
    // FFT inverse all scales
#ifdef BIG_BATCH
    if (m_num_of_scales > 1 && BIG_BATCH_MODE) {
        int rank = 2;
        int n[] = {(int)m_height, (int)m_width};
        int howmany = m_num_of_feats * m_num_of_scales;
        int idist = m_height * (m_width / 2 + 1), odist = 1;
        int istride = 1, ostride = m_num_of_feats * m_num_of_scales;
        int inembed[] = {(int)m_height, (int)m_width / 2 + 1}, *onembed = n;

        CufftErrorCheck(cufftPlanMany(&plan_i_features_all_scales, rank, n, inembed, istride, idist, onembed, ostride,
                                      odist, CUFFT_C2R, howmany));
        CufftErrorCheck(cufftSetStream(plan_i_features_all_scales, cudaStreamPerThread));
    }
#endif
    // FFT inverse one channel one scale
    {
        int rank = 2;
        int n[] = {int(m_height), int(m_width)};
        int howmany = 1;
        int idist = int(m_height * (m_width / 2 + 1)), odist = 1;
        int istride = 1, ostride = 1;
        int inembed[] = {int(m_height), int(m_width / 2 + 1)}, *onembed = n;

        CufftErrorCheck(
            cufftPlanMany(&plan_i_1ch, rank, n, inembed, istride, idist, onembed, ostride, odist, CUFFT_C2R, howmany));
        CufftErrorCheck(cufftSetStream(plan_i_1ch, cudaStreamPerThread));
    }
#ifdef BIG_BATCH
    // FFT inverse one channel all scales
    if (m_num_of_scales > 1 && BIG_BATCH_MODE) {
        int rank = 2;
        int n[] = {(int)m_height, (int)m_width};
        int howmany = m_num_of_scales;
        int idist = m_height * (m_width / 2 + 1), odist = 1;
        int istride = 1, ostride = m_num_of_scales;
        int inembed[] = {(int)m_height, (int)m_width / 2 + 1}, *onembed = n;

        CufftErrorCheck(cufftPlanMany(&plan_i_1ch_all_scales, rank, n, inembed, istride, idist, onembed, ostride, odist,
                                      CUFFT_C2R, howmany));
        CufftErrorCheck(cufftSetStream(plan_i_1ch_all_scales, cudaStreamPerThread));
    }
#endif
}

void cuFFT::set_window(const MatDynMem &window)
{
    m_window = window;
}

void cuFFT::forward(MatDynMem & real_input, ComplexMat & complex_result)
{
    if (BIG_BATCH_MODE && real_input.rows == int(m_height * m_num_of_scales)) {
        CufftErrorCheck(cufftExecR2C(plan_f_all_scales, reinterpret_cast<cufftReal *>(real_input.deviceMem()),
                                     complex_result.get_p_data()));
    } else {
        NORMAL_OMP_CRITICAL
        {
            CufftErrorCheck(
                cufftExecR2C(plan_f, reinterpret_cast<cufftReal *>(real_input.deviceMem()), complex_result.get_p_data()));
            cudaStreamSynchronize(cudaStreamPerThread);
        }
    }
    return;
}

void cuFFT::forward_window(MatDynMem &feat, ComplexMat & complex_result, MatDynMem &temp)
{
    uint n_channels = feat.size[0];
    cufftReal *temp_data = temp.deviceMem();

    assert(feat.dims == 3);
    assert(n_channels == m_num_of_feats || n_channels == m_num_of_feats * m_num_of_scales);

    for (uint i = 0; i < n_channels; ++i) {
        cv::Mat feat_plane(feat.dims - 1, feat.size + 1, feat.cv::Mat::type(), feat.ptr<void>(i));
        cv::Mat temp_plane(temp.dims - 1, temp.size + 1, temp.cv::Mat::type(), temp.ptr(i));
        temp_plane = feat_plane.mul(m_window);
    }
    CufftErrorCheck(cufftExecR2C((n_channels == m_num_of_feats) ? plan_fw : plan_fw_all_scales,
                                 temp_data, complex_result.get_p_data()));
}

void cuFFT::inverse(ComplexMat &complex_input, MatDynMem &real_result)
{
    uint n_channels = complex_input.n_channels;
    cufftComplex *in = reinterpret_cast<cufftComplex *>(complex_input.get_p_data());
    cufftReal *out = real_result.deviceMem();
    float alpha = 1.0 / (m_width * m_height);
    cufftHandle plan;

    if (n_channels == 1) {
        CufftErrorCheck(cufftExecC2R(plan_i_1ch, in, out));
        CublasErrorCheck(cublasSscal(cublas, real_result.total(), &alpha, out, 1));
        return;
    } else if (n_channels == m_num_of_scales) {
        CufftErrorCheck(cufftExecC2R(plan_i_1ch_all_scales, in, out));
        CublasErrorCheck(cublasSscal(cublas, real_result.total(), &alpha, out, 1));
        return;
    } else if (n_channels == m_num_of_feats * m_num_of_scales) {
        CufftErrorCheck(cufftExecC2R(plan_i_features_all_scales, in, out));
        cudaStreamSynchronize(cudaStreamPerThread);
        return;
    }
    CufftErrorCheck(cufftExecC2R(plan_i_features, in, out));
    return;
}

cuFFT::~cuFFT()
{
    CublasErrorCheck(cublasDestroy(cublas));

    CufftErrorCheck(cufftDestroy(plan_f));
    CufftErrorCheck(cufftDestroy(plan_fw));
    CufftErrorCheck(cufftDestroy(plan_i_1ch));
    CufftErrorCheck(cufftDestroy(plan_i_features));

    if (BIG_BATCH_MODE) {
        CufftErrorCheck(cufftDestroy(plan_f_all_scales));
        CufftErrorCheck(cufftDestroy(plan_fw_all_scales));
        CufftErrorCheck(cufftDestroy(plan_i_1ch_all_scales));
        CufftErrorCheck(cufftDestroy(plan_i_features_all_scales));
    }
}
