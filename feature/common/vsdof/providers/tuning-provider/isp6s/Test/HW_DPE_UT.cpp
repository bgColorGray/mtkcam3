#include "StereoTuningProviderUT.h"
#include "../tunings/DPE_Config.h"

#define MY_LOGD(fmt, arg...)    printf("[D][%s]" fmt"\n", __func__, ##arg)
#define MY_LOGI(fmt, arg...)    printf("[I][%s]" fmt"\n", __func__, ##arg)
#define MY_LOGW(fmt, arg...)    printf("[W][%s] WRN(%5d):" fmt"\n", __func__, __LINE__, ##arg)
#define MY_LOGE(fmt, arg...)    printf("[E][%s] %s ERROR(%5d):" fmt"\n", __func__,__FILE__, __LINE__, ##arg)

class HW_DPE_UT : public StereoTuningProviderUTBase
{
public:
    HW_DPE_UT() : StereoTuningProviderUTBase() { init(); }

    virtual ~HW_DPE_UT() {}
};

//=============================================================================
//
//=============================================================================
TEST_F(HW_DPE_UT, TEST)
{
    static_assert (sizeof(REG_DVP_CORE_CFG) == sizeof(DVP_CORE_CFG),    "REG_DVP_CORE_CFG Size is incorrect");
    static_assert (sizeof(REG_DVS_OCC_PQ_CFG) == sizeof(DVS_OCC_CFG),   "REG_DVS_OCC_PQ_CFG Size is incorrect");
    static_assert (sizeof(REG_DVP_CORE_CFG) == sizeof(DVP_CORE_CFG),    "REG_DVP_CORE_CFG Size is incorrect");

    REG_DVS_ME_CFG      dvsMeConfig;
    REG_DVS_OCC_PQ_CFG  dvsOccPqConfig;
    REG_DVP_CORE_CFG    dvpCoreConfig;

    ::memcpy(&dvsMeConfig, &_config.Dpe_DVSSettings.TuningBuf_ME, sizeof(dvsMeConfig));
    ::memcpy(&dvsOccPqConfig, &_config.Dpe_DVSSettings.TuningBuf_OCC, sizeof(dvsOccPqConfig));
    ::memcpy(&dvpCoreConfig, &_config.Dpe_DVPSettings.TuningBuf_CORE, sizeof(dvpCoreConfig));

    printf("===== DVS CTRL Parameters =====\n");
    printf("Dpe_engineSelect                              %d\n", _config.Dpe_engineSelect);
    printf("Dpe_is16BitMode                               %d\n", _config.Dpe_is16BitMode);
    printf("Dpe_DVSSettings.mainEyeSel                    %d\n", _config.Dpe_DVSSettings.mainEyeSel);
    printf("Dpe_DVSSettings.is_pd_mode                    %d\n", _config.Dpe_DVSSettings.is_pd_mode);
    printf("Dpe_DVSSettings.pd_frame_num                  %d\n", _config.Dpe_DVSSettings.pd_frame_num);
    printf("Dpe_DVSSettings.pd_st_x                       %d\n", _config.Dpe_DVSSettings.pd_st_x);
    printf("Dpe_DVSSettings.frmWidth                      %d\n", _config.Dpe_DVSSettings.frmWidth);
    printf("Dpe_DVSSettings.frmHeight                     %d\n", _config.Dpe_DVSSettings.frmHeight);
    printf("Dpe_DVSSettings.L_engStart_x                  %d\n", _config.Dpe_DVSSettings.L_engStart_x);
    printf("Dpe_DVSSettings.R_engStart_x                  %d\n", _config.Dpe_DVSSettings.R_engStart_x);
    printf("Dpe_DVSSettings.engStart_y                    %d\n", _config.Dpe_DVSSettings.engStart_y);
    printf("Dpe_DVSSettings.engWidth                      %d\n", _config.Dpe_DVSSettings.engWidth);
    printf("Dpe_DVSSettings.engHeight                     %d\n", _config.Dpe_DVSSettings.engHeight);
    printf("Dpe_DVSSettings.engStart_y                    %d\n", _config.Dpe_DVSSettings.engStart_y);
    printf("Dpe_DVSSettings.occWidth                      %d\n", _config.Dpe_DVSSettings.occWidth);
    printf("Dpe_DVSSettings.occStart_x                    %d\n", _config.Dpe_DVSSettings.occStart_x);
    printf("Dpe_DVSSettings.pitch                         %d\n", _config.Dpe_DVSSettings.pitch);
    printf("Dpe_DVSSettings.SubModule_EN.sbf_en           %d\n", _config.Dpe_DVSSettings.SubModule_EN.sbf_en);
    printf("Dpe_DVSSettings.SubModule_EN.conf_en          %d\n", _config.Dpe_DVSSettings.SubModule_EN.conf_en);
    printf("Dpe_DVSSettings.SubModule_EN.occ_en           %d\n", _config.Dpe_DVSSettings.SubModule_EN.occ_en);
    printf("Dpe_DVSSettings.Iteration.y_IterTimes         %d\n", _config.Dpe_DVSSettings.Iteration.y_IterTimes);
    printf("Dpe_DVSSettings.Iteration.y_IterStartDirect_0 %d\n", _config.Dpe_DVSSettings.Iteration.y_IterStartDirect_0);
    printf("Dpe_DVSSettings.Iteration.y_IterStartDirect_1 %d\n", _config.Dpe_DVSSettings.Iteration.y_IterStartDirect_1);
    printf("Dpe_DVSSettings.Iteration.x_IterStartDirect_0 %d\n", _config.Dpe_DVSSettings.Iteration.x_IterStartDirect_0);
    printf("Dpe_DVSSettings.Iteration.x_IterStartDirect_1 %d\n\n", _config.Dpe_DVSSettings.Iteration.x_IterStartDirect_1);

    printf("===== DVS ME Parameters =====\n");
    printf("c_cand_num                 %d\n", dvsMeConfig.DVS_ME_00.Bits.c_cand_num);
    printf("c_dv_start_pos             %d\n", dvsMeConfig.DVS_ME_00.Bits.c_dv_start_pos);
    printf("c_cen_ws_mode              %d\n", dvsMeConfig.DVS_ME_00.Bits.c_cen_ws_mode);
    printf("c_cen_color_thd            %d\n", dvsMeConfig.DVS_ME_00.Bits.c_cen_color_thd);
    printf("c_cen_avg_ws_mode          %d\n", dvsMeConfig.DVS_ME_00.Bits.c_cen_avg_ws_mode);
    printf("c_ad_weight                %d\n", dvsMeConfig.DVS_ME_00.Bits.c_ad_weight);
    printf("c_lut_cen_c00-09           % 3d % 3d % 3d % 3d % 3d % 3d % 3d % 3d % 3d % 3d\n",
            dvsMeConfig.DVS_ME_01.Bits.c_lut_cen_c00,
            dvsMeConfig.DVS_ME_01.Bits.c_lut_cen_c01,
            dvsMeConfig.DVS_ME_01.Bits.c_lut_cen_c02,
            dvsMeConfig.DVS_ME_01.Bits.c_lut_cen_c03,
            dvsMeConfig.DVS_ME_02.Bits.c_lut_cen_c04,
            dvsMeConfig.DVS_ME_02.Bits.c_lut_cen_c05,
            dvsMeConfig.DVS_ME_02.Bits.c_lut_cen_c06,
            dvsMeConfig.DVS_ME_02.Bits.c_lut_cen_c07,
            dvsMeConfig.DVS_ME_03.Bits.c_lut_cen_c08,
            dvsMeConfig.DVS_ME_03.Bits.c_lut_cen_c09);
    printf("c_lut_ad_c00-10            % 3d % 3d % 3d % 3d % 3d % 3d % 3d % 3d % 3d % 3d % 3d\n",
            dvsMeConfig.DVS_ME_03.Bits.c_lut_ad_c00,
            dvsMeConfig.DVS_ME_03.Bits.c_lut_ad_c01,
            dvsMeConfig.DVS_ME_04.Bits.c_lut_ad_c02,
            dvsMeConfig.DVS_ME_04.Bits.c_lut_ad_c03,
            dvsMeConfig.DVS_ME_04.Bits.c_lut_ad_c04,
            dvsMeConfig.DVS_ME_04.Bits.c_lut_ad_c05,
            dvsMeConfig.DVS_ME_05.Bits.c_lut_ad_c06,
            dvsMeConfig.DVS_ME_05.Bits.c_lut_ad_c07,
            dvsMeConfig.DVS_ME_05.Bits.c_lut_ad_c08,
            dvsMeConfig.DVS_ME_05.Bits.c_lut_ad_c09,
            dvsMeConfig.DVS_ME_06.Bits.c_lut_ad_c10);
    printf("c_lut_conf_c00-06          % 3d % 3d % 3d % 3d % 3d % 3d % 3d\n",
            dvsMeConfig.DVS_ME_06.Bits.c_lut_conf_c00,
            dvsMeConfig.DVS_ME_06.Bits.c_lut_conf_c01,
            dvsMeConfig.DVS_ME_06.Bits.c_lut_conf_c02,
            dvsMeConfig.DVS_ME_07.Bits.c_lut_conf_c03,
            dvsMeConfig.DVS_ME_07.Bits.c_lut_conf_c04,
            dvsMeConfig.DVS_ME_07.Bits.c_lut_conf_c05,
            dvsMeConfig.DVS_ME_07.Bits.c_lut_conf_c06);
    printf("c_recursive_mode           %d\n", dvsMeConfig.DVS_ME_08.Bits.c_recursive_mode);
    printf("c_rpd_en                   %d\n", dvsMeConfig.DVS_ME_08.Bits.c_rpd_en);
    printf("c_rpd_thd                  %d\n", dvsMeConfig.DVS_ME_08.Bits.c_rpd_thd);
    printf("c_rpd_chk_ratio            %d\n", dvsMeConfig.DVS_ME_08.Bits.c_rpd_chk_ratio);
    printf("c_lut_p1_c00-07            % 4d % 4d % 4d % 4d % 4d % 4d % 4d % 4d\n",
            dvsMeConfig.DVS_ME_09.Bits.c_lut_p1_c00,
            dvsMeConfig.DVS_ME_09.Bits.c_lut_p1_c01,
            dvsMeConfig.DVS_ME_09.Bits.c_lut_p1_c02,
            dvsMeConfig.DVS_ME_10.Bits.c_lut_p1_c03,
            dvsMeConfig.DVS_ME_10.Bits.c_lut_p1_c04,
            dvsMeConfig.DVS_ME_10.Bits.c_lut_p1_c05,
            dvsMeConfig.DVS_ME_11.Bits.c_lut_p1_c06,
            dvsMeConfig.DVS_ME_11.Bits.c_lut_p1_c07);
    printf("c_lut_p1_c08-14            % 4d % 4d % 4d % 4d % 4d % 4d % 4d\n",
            dvsMeConfig.DVS_ME_11.Bits.c_lut_p1_c08,
            dvsMeConfig.DVS_ME_12.Bits.c_lut_p1_c09,
            dvsMeConfig.DVS_ME_12.Bits.c_lut_p1_c10,
            dvsMeConfig.DVS_ME_12.Bits.c_lut_p1_c11,
            dvsMeConfig.DVS_ME_13.Bits.c_lut_p1_c12,
            dvsMeConfig.DVS_ME_13.Bits.c_lut_p1_c13,
            dvsMeConfig.DVS_ME_13.Bits.c_lut_p1_c14);
    printf("c_lut_p2_c00-07            % 4d % 4d % 4d % 4d % 4d % 4d % 4d % 4d\n",
            dvsMeConfig.DVS_ME_14.Bits.c_lut_p2_c00,
            dvsMeConfig.DVS_ME_14.Bits.c_lut_p2_c01,
            dvsMeConfig.DVS_ME_14.Bits.c_lut_p2_c02,
            dvsMeConfig.DVS_ME_15.Bits.c_lut_p2_c03,
            dvsMeConfig.DVS_ME_15.Bits.c_lut_p2_c04,
            dvsMeConfig.DVS_ME_15.Bits.c_lut_p2_c05,
            dvsMeConfig.DVS_ME_16.Bits.c_lut_p2_c06,
            dvsMeConfig.DVS_ME_16.Bits.c_lut_p2_c07);
    printf("c_lut_p2_c08-14            % 4d % 4d % 4d % 4d % 4d % 4d % 4d\n",
            dvsMeConfig.DVS_ME_16.Bits.c_lut_p2_c08,
            dvsMeConfig.DVS_ME_17.Bits.c_lut_p2_c09,
            dvsMeConfig.DVS_ME_17.Bits.c_lut_p2_c10,
            dvsMeConfig.DVS_ME_17.Bits.c_lut_p2_c11,
            dvsMeConfig.DVS_ME_18.Bits.c_lut_p2_c12,
            dvsMeConfig.DVS_ME_18.Bits.c_lut_p2_c13,
            dvsMeConfig.DVS_ME_18.Bits.c_lut_p2_c14);
    printf("c_lut_p3_c00-07            % 4d % 4d % 4d % 4d % 4d % 4d % 4d % 4d\n",
            dvsMeConfig.DVS_ME_19.Bits.c_lut_p3_c00,
            dvsMeConfig.DVS_ME_19.Bits.c_lut_p3_c01,
            dvsMeConfig.DVS_ME_19.Bits.c_lut_p3_c02,
            dvsMeConfig.DVS_ME_20.Bits.c_lut_p3_c03,
            dvsMeConfig.DVS_ME_20.Bits.c_lut_p3_c04,
            dvsMeConfig.DVS_ME_20.Bits.c_lut_p3_c05,
            dvsMeConfig.DVS_ME_21.Bits.c_lut_p3_c06,
            dvsMeConfig.DVS_ME_21.Bits.c_lut_p3_c07);
    printf("c_wgt_dir_0               %d\n", dvsMeConfig.DVS_ME_22.Bits.c_wgt_dir_0);
    printf("c_wgt_dir_1_3             %d\n", dvsMeConfig.DVS_ME_22.Bits.c_wgt_dir_1_3);
    printf("c_wgt_dir_2               %d\n", dvsMeConfig.DVS_ME_22.Bits.c_wgt_dir_2);
    printf("c_wgt_dir_4               %d\n", dvsMeConfig.DVS_ME_22.Bits.c_wgt_dir_4);
    printf("c_fractional_cv           %d\n", dvsMeConfig.DVS_ME_23.Bits.c_fractional_cv);
    printf("c_subpixel_ip             %d\n", dvsMeConfig.DVS_ME_23.Bits.c_subpixel_ip);
    printf("c_conf_dv_diff            %d\n", dvsMeConfig.DVS_ME_24.Bits.c_conf_dv_diff);
    printf("c_bypass_p3_thd           %d\n", dvsMeConfig.DVS_ME_24.Bits.c_bypass_p3_thd);
    printf("c_p3_by_conf_rpd          %d\n", dvsMeConfig.DVS_ME_24.Bits.c_p3_by_conf_rpd);
    printf("c_p3_conf_thd             %d\n", dvsMeConfig.DVS_ME_24.Bits.c_p3_conf_thd);
    printf("c_conf_cost_diff          %d\n\n", dvsMeConfig.DVS_ME_24.Bits.c_conf_cost_diff);

    printf("===== DVS OCC Parameters =====\n");
    printf("c_owc_upd_uq_en     %d\n", dvsOccPqConfig.DVS_OCC_PQ_0.Bits.c_owc_upd_uq_en);
    printf("c_owc_uq_value      %d\n", dvsOccPqConfig.DVS_OCC_PQ_0.Bits.c_owc_uq_value);
    printf("c_owc_en            %d\n", dvsOccPqConfig.DVS_OCC_PQ_0.Bits.c_owc_en);
    printf("c_owc_tlr_thr       %d\n", dvsOccPqConfig.DVS_OCC_PQ_0.Bits.c_owc_tlr_thr);
    printf("c_ohf_upd_uq_en     %d\n", dvsOccPqConfig.DVS_OCC_PQ_1.Bits.c_ohf_upd_uq_en);
    printf("c_ohf_uq_value      %d\n", dvsOccPqConfig.DVS_OCC_PQ_1.Bits.c_ohf_uq_value);
    printf("c_ohf_en            %d\n", dvsOccPqConfig.DVS_OCC_PQ_1.Bits.c_ohf_en);
    printf("c_ohf_tlr_thr       %d\n", dvsOccPqConfig.DVS_OCC_PQ_1.Bits.c_ohf_tlr_thr);
    printf("c_lrc_upd_uq_en     %d\n", dvsOccPqConfig.DVS_OCC_PQ_2.Bits.c_lrc_upd_uq_en);
    printf("c_lrc_thr_ng        %d\n", dvsOccPqConfig.DVS_OCC_PQ_2.Bits.c_lrc_thr_ng);
    printf("c_lrc_thr           %d\n", dvsOccPqConfig.DVS_OCC_PQ_2.Bits.c_lrc_thr);
    printf("c_sbf_h_size_mode   %d\n", dvsOccPqConfig.DVS_OCC_PQ_3.Bits.c_sbf_h_size_mode);
    printf("c_sbf_v_size_mode   %d\n", dvsOccPqConfig.DVS_OCC_PQ_3.Bits.c_sbf_v_size_mode);
    printf("c_sbf_thr           %d\n", dvsOccPqConfig.DVS_OCC_PQ_3.Bits.c_sbf_thr);
    printf("c_ups_dv_thr        %d\n", dvsOccPqConfig.DVS_OCC_PQ_4.Bits.c_ups_dv_thr);
    printf("c_ups_bld_en        %d\n", dvsOccPqConfig.DVS_OCC_PQ_4.Bits.c_ups_bld_en);
    printf("c_ups_bld_step      %d\n", dvsOccPqConfig.DVS_OCC_PQ_4.Bits.c_ups_bld_step);
    printf("c_ups_weight_mode   %d\n", dvsOccPqConfig.DVS_OCC_PQ_4.Bits.c_ups_weight_mode);
    printf("c_occ_dv_min_uq_thr %d\n", dvsOccPqConfig.DVS_OCC_PQ_4.Bits.c_occ_dv_min_uq_thr);
    printf("c_occ_dv_rmv_rp_en  %d\n", dvsOccPqConfig.DVS_OCC_PQ_4.Bits.c_occ_dv_rmv_rp_en);
    printf("c_occ_dv_msb_sft    %d\n", dvsOccPqConfig.DVS_OCC_PQ_4.Bits.c_occ_dv_msb_sft);
    printf("c_occ_dv_lsb_sft    %d\n", dvsOccPqConfig.DVS_OCC_PQ_4.Bits.c_occ_dv_lsb_sft);
    printf("c_occ_dv_offset     %d\n", dvsOccPqConfig.DVS_OCC_PQ_4.Bits.c_occ_dv_offset);
    printf("c_occ_flag_debug    %d\n", dvsOccPqConfig.DVS_OCC_PQ_4.Bits.c_occ_flag_debug);
    printf("c_fx_baseline       %d\n", dvsOccPqConfig.DVS_OCC_PQ_5.Bits.c_fx_baseline);
    printf("c_depth_format      %d\n\n", dvsOccPqConfig.DVS_OCC_PQ_5.Bits.c_depth_format);

    printf("===== DVP CTRL Parameters =====\n");
    printf("mainEyeSel           %d\n", _config.Dpe_DVPSettings.mainEyeSel);
    printf("Y_only               %d\n", _config.Dpe_DVPSettings.Y_only);
    printf("disp_guide_en        %d\n", _config.Dpe_DVPSettings.disp_guide_en);
    printf("frmWidth             %d\n", _config.Dpe_DVPSettings.frmWidth);
    printf("frmHeight            %d\n", _config.Dpe_DVPSettings.frmHeight);
    printf("engStart_x           %d\n", _config.Dpe_DVPSettings.engStart_x);
    printf("engStart_y           %d\n", _config.Dpe_DVPSettings.engStart_y);
    printf("engWidth             %d\n", _config.Dpe_DVPSettings.engWidth);
    printf("engHeight            %d\n", _config.Dpe_DVPSettings.engHeight);
    printf("SubModule_EN.asf_crm_en       %d\n", _config.Dpe_DVPSettings.SubModule_EN.asf_crm_en);
    printf("SubModule_EN.asf_rm_en        %d\n", _config.Dpe_DVPSettings.SubModule_EN.asf_rm_en);
    printf("SubModule_EN.asf_rd_en        %d\n", _config.Dpe_DVPSettings.SubModule_EN.asf_rd_en);
    printf("SubModule_EN.asf_hf_en        %d\n", _config.Dpe_DVPSettings.SubModule_EN.asf_hf_en);
    printf("SubModule_EN.wmf_hf_en        %d\n", _config.Dpe_DVPSettings.SubModule_EN.wmf_hf_en);
    printf("SubModule_EN.wmf_filt_en      %d\n", _config.Dpe_DVPSettings.SubModule_EN.wmf_filt_en);
    printf("SubModule_EN.asf_hf_rounds    %d\n", _config.Dpe_DVPSettings.SubModule_EN.asf_hf_rounds);
    printf("SubModule_EN.asf_nb_rounds    %d\n", _config.Dpe_DVPSettings.SubModule_EN.asf_nb_rounds);
    printf("SubModule_EN.wmf_filt_rounds  %d\n", _config.Dpe_DVPSettings.SubModule_EN.wmf_filt_rounds);
    printf("SubModule_EN.asf_recursive_en %d\n", _config.Dpe_DVPSettings.SubModule_EN.asf_recursive_en);
    printf("c_asf_thd_y          %d\n", dvpCoreConfig.DVP_CORE_00.Bits.c_asf_thd_y);
    printf("c_asf_thd_cbcr       %d\n", dvpCoreConfig.DVP_CORE_00.Bits.c_asf_thd_cbcr);
    printf("c_asf_arm_ud         %d\n", dvpCoreConfig.DVP_CORE_00.Bits.c_asf_arm_ud);
    printf("c_asf_arm_lr         %d\n", dvpCoreConfig.DVP_CORE_00.Bits.c_asf_arm_lr);
    printf("c_rmv_mean_lut       % 3d % 3d % 3d % 3d % 3d % 3d % 3d % 3d\n",
                dvpCoreConfig.DVP_CORE_01.Bits.c_rmv_mean_lut_0,
                dvpCoreConfig.DVP_CORE_01.Bits.c_rmv_mean_lut_1,
                dvpCoreConfig.DVP_CORE_01.Bits.c_rmv_mean_lut_2,
                dvpCoreConfig.DVP_CORE_01.Bits.c_rmv_mean_lut_3,
                dvpCoreConfig.DVP_CORE_02.Bits.c_rmv_mean_lut_4,
                dvpCoreConfig.DVP_CORE_02.Bits.c_rmv_mean_lut_5,
                dvpCoreConfig.DVP_CORE_02.Bits.c_rmv_mean_lut_6,
                dvpCoreConfig.DVP_CORE_02.Bits.c_rmv_mean_lut_7);
    printf("c_rmv_dsty_lut       % 3d % 3d % 3d % 3d % 3d % 3d % 3d % 3d\n",
                dvpCoreConfig.DVP_CORE_03.Bits.c_rmv_dsty_lut_0,
                dvpCoreConfig.DVP_CORE_03.Bits.c_rmv_dsty_lut_1,
                dvpCoreConfig.DVP_CORE_03.Bits.c_rmv_dsty_lut_2,
                dvpCoreConfig.DVP_CORE_03.Bits.c_rmv_dsty_lut_3,
                dvpCoreConfig.DVP_CORE_04.Bits.c_rmv_dsty_lut_4,
                dvpCoreConfig.DVP_CORE_04.Bits.c_rmv_dsty_lut_5,
                dvpCoreConfig.DVP_CORE_04.Bits.c_rmv_dsty_lut_6,
                dvpCoreConfig.DVP_CORE_04.Bits.c_rmv_dsty_lut_7);
    printf("c_hf_thin_en         %d\n", dvpCoreConfig.DVP_CORE_07.Bits.c_hf_thin_en);
    printf("c_hf_thd_thin        %d\n", dvpCoreConfig.DVP_CORE_07.Bits.c_hf_thd_thin);
    printf("c_hf_thd             %d %d %d %d %d %d %d %d %d %d\n",
                dvpCoreConfig.DVP_CORE_05.Bits.c_hf_thd_r1,
                dvpCoreConfig.DVP_CORE_05.Bits.c_hf_thd_r2,
                dvpCoreConfig.DVP_CORE_05.Bits.c_hf_thd_r3,
                dvpCoreConfig.DVP_CORE_05.Bits.c_hf_thd_r4,
                dvpCoreConfig.DVP_CORE_06.Bits.c_hf_thd_r5,
                dvpCoreConfig.DVP_CORE_06.Bits.c_hf_thd_r6,
                dvpCoreConfig.DVP_CORE_06.Bits.c_hf_thd_r7,
                dvpCoreConfig.DVP_CORE_06.Bits.c_hf_thd_r8,
                dvpCoreConfig.DVP_CORE_07.Bits.c_hf_thd_r9,
                dvpCoreConfig.DVP_CORE_07.Bits.c_hf_thd_r10);
    printf("c_wmf_wgt_lut[00-07] % 4d % 4d % 4d % 4d % 4d % 4d % 4d % 4d\n",
                dvpCoreConfig.DVP_CORE_08.Bits.c_wmf_wgt_lut_0,
                dvpCoreConfig.DVP_CORE_08.Bits.c_wmf_wgt_lut_1,
                dvpCoreConfig.DVP_CORE_08.Bits.c_wmf_wgt_lut_2,
                dvpCoreConfig.DVP_CORE_08.Bits.c_wmf_wgt_lut_3,
                dvpCoreConfig.DVP_CORE_09.Bits.c_wmf_wgt_lut_4,
                dvpCoreConfig.DVP_CORE_09.Bits.c_wmf_wgt_lut_5,
                dvpCoreConfig.DVP_CORE_09.Bits.c_wmf_wgt_lut_6,
                dvpCoreConfig.DVP_CORE_09.Bits.c_wmf_wgt_lut_7);
    printf("c_wmf_wgt_lut[08-15] % 4d % 4d % 4d % 4d % 4d % 4d % 4d % 4d\n",
                dvpCoreConfig.DVP_CORE_10.Bits.c_wmf_wgt_lut_8,
                dvpCoreConfig.DVP_CORE_10.Bits.c_wmf_wgt_lut_9,
                dvpCoreConfig.DVP_CORE_10.Bits.c_wmf_wgt_lut_10,
                dvpCoreConfig.DVP_CORE_10.Bits.c_wmf_wgt_lut_11,
                dvpCoreConfig.DVP_CORE_11.Bits.c_wmf_wgt_lut_12,
                dvpCoreConfig.DVP_CORE_11.Bits.c_wmf_wgt_lut_13,
                dvpCoreConfig.DVP_CORE_11.Bits.c_wmf_wgt_lut_14,
                dvpCoreConfig.DVP_CORE_11.Bits.c_wmf_wgt_lut_15);
    printf("c_wmf_wgt_lut[16-24] % 4d % 4d % 4d % 4d % 4d % 4d % 4d % 4d % 4d\n",
                dvpCoreConfig.DVP_CORE_12.Bits.c_wmf_wgt_lut_16,
                dvpCoreConfig.DVP_CORE_12.Bits.c_wmf_wgt_lut_17,
                dvpCoreConfig.DVP_CORE_12.Bits.c_wmf_wgt_lut_18,
                dvpCoreConfig.DVP_CORE_12.Bits.c_wmf_wgt_lut_19,
                dvpCoreConfig.DVP_CORE_13.Bits.c_wmf_wgt_lut_20,
                dvpCoreConfig.DVP_CORE_13.Bits.c_wmf_wgt_lut_21,
                dvpCoreConfig.DVP_CORE_13.Bits.c_wmf_wgt_lut_22,
                dvpCoreConfig.DVP_CORE_13.Bits.c_wmf_wgt_lut_23,
                dvpCoreConfig.DVP_CORE_14.Bits.c_wmf_wgt_lut_24);
    printf("c_hf_mode            %d %d %d %d %d %d %d %d %d %d\n",
                dvpCoreConfig.DVP_CORE_15.Bits.c_hf_mode_r1,
                dvpCoreConfig.DVP_CORE_15.Bits.c_hf_mode_r2,
                dvpCoreConfig.DVP_CORE_15.Bits.c_hf_mode_r3,
                dvpCoreConfig.DVP_CORE_15.Bits.c_hf_mode_r4,
                dvpCoreConfig.DVP_CORE_15.Bits.c_hf_mode_r5,
                dvpCoreConfig.DVP_CORE_15.Bits.c_hf_mode_r6,
                dvpCoreConfig.DVP_CORE_15.Bits.c_hf_mode_r7,
                dvpCoreConfig.DVP_CORE_15.Bits.c_hf_mode_r8,
                dvpCoreConfig.DVP_CORE_15.Bits.c_hf_mode_r9,
                dvpCoreConfig.DVP_CORE_15.Bits.c_hf_mode_r10);
    printf("-------------------------------");
}