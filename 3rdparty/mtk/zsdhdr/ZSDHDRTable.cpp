/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly
 * prohibited.
 */
/* MediaTek Inc. (C) 2020. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY
 * ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY
 * THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK
 * SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO
 * RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN
 * FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER
 * WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT
 * ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER
 * TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#include "ZSDHDRImpl.h"

#include <vector>
#include <map>

#include <mtkcam/utils/metadata/client/mtk_metadata_tag.h>
#include <mtkcam/utils/metadata/hal/mtk_platform_metadata_tag.h>

namespace NSCam {
namespace NSPipelinePlugin {
/******************************************************************************
 *
 ******************************************************************************/
void ZSDHDRPluginProviderImp::initRequestTable() {
#define ZSDHDR_REQUEST_MAPPING_TABLE_BEGIN()\
mRequestTable =\
{

#define ADD_ITEM_BEGIN(mode, hdrMode)\
    {\
        {\
            mode,\
            hdrMode\
        },\
        {

#define ADD_REQUEST_ZSL_HISTORY(exp, idx)\
            {\
                exp,\
                idx,\
                Category::eZSL_History,\
                true\
            },

#define ADD_REQUEST_ZSL_INFLIGHT(exp, idx)\
            {\
                exp,\
                idx,\
                Category::eZSL_Inflight,\
                true\
            },

#define ADD_REQUEST_NONZSL()\
            {\
                -1,\
                -1,\
                Category::eNonZSL,\
                false\
            },

#define ADD_ITEM_END()\
        }\
    },

#define ZSDHDR_REQUEST_MAPPING_TABLE_END()\
};

ZSDHDR_REQUEST_MAPPING_TABLE_BEGIN()
    ADD_ITEM_BEGIN(Zsl_Mode, MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER_2EXP)
        ADD_REQUEST_ZSL_HISTORY(MTK_STAGGER_IMGO_NE, 0)
        ADD_REQUEST_ZSL_HISTORY(MTK_STAGGER_IMGO_NE, 1)
        ADD_REQUEST_ZSL_HISTORY(MTK_STAGGER_IMGO_SE, 1)
        ADD_REQUEST_ZSL_HISTORY(MTK_STAGGER_IMGO_NE, 2)
        ADD_REQUEST_ZSL_HISTORY(MTK_STAGGER_IMGO_SE, 2)
        ADD_REQUEST_ZSL_HISTORY(MTK_STAGGER_IMGO_NE, 3)
        ADD_REQUEST_ZSL_HISTORY(MTK_STAGGER_IMGO_SE, 3)
    ADD_ITEM_END()

    ADD_ITEM_BEGIN(Zsl_Mode, MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER_3EXP)
        ADD_REQUEST_ZSL_HISTORY(MTK_STAGGER_IMGO_NE, 0)
        ADD_REQUEST_ZSL_HISTORY(MTK_STAGGER_IMGO_ME, 0)
        ADD_REQUEST_ZSL_HISTORY(MTK_STAGGER_IMGO_SE, 0)
        ADD_REQUEST_ZSL_HISTORY(MTK_STAGGER_IMGO_ME, 1)
        ADD_REQUEST_ZSL_HISTORY(MTK_STAGGER_IMGO_SE, 1)
        ADD_REQUEST_ZSL_HISTORY(MTK_STAGGER_IMGO_ME, 2)
        ADD_REQUEST_ZSL_HISTORY(MTK_STAGGER_IMGO_SE, 2)
    ADD_ITEM_END()

    ADD_ITEM_BEGIN(Zsl_NonZsl_Mode, MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER_2EXP)
        ADD_REQUEST_ZSL_INFLIGHT(MTK_STAGGER_IMGO_NE, 0)
        ADD_REQUEST_ZSL_INFLIGHT(MTK_STAGGER_IMGO_SE, 0)
        ADD_REQUEST_ZSL_INFLIGHT(MTK_STAGGER_IMGO_NE, 1)
        ADD_REQUEST_ZSL_INFLIGHT(MTK_STAGGER_IMGO_SE, 1)
        ADD_REQUEST_ZSL_INFLIGHT(MTK_STAGGER_IMGO_NE, 2)
        ADD_REQUEST_ZSL_INFLIGHT(MTK_STAGGER_IMGO_SE, 2)
        ADD_REQUEST_NONZSL()
    ADD_ITEM_END()

    ADD_ITEM_BEGIN(Zsl_NonZsl_Mode, MTK_HDR_FEATURE_HDR_HAL_MODE_STAGGER_3EXP)
        ADD_REQUEST_ZSL_INFLIGHT(MTK_STAGGER_IMGO_NE, 0)
        ADD_REQUEST_ZSL_INFLIGHT(MTK_STAGGER_IMGO_SE, 0)
        ADD_REQUEST_ZSL_INFLIGHT(MTK_STAGGER_IMGO_NE, 1)
        ADD_REQUEST_ZSL_INFLIGHT(MTK_STAGGER_IMGO_SE, 1)
        ADD_REQUEST_ZSL_INFLIGHT(MTK_STAGGER_IMGO_NE, 2)
        ADD_REQUEST_ZSL_INFLIGHT(MTK_STAGGER_IMGO_SE, 2)
        ADD_REQUEST_NONZSL()
    ADD_ITEM_END()

ZSDHDR_REQUEST_MAPPING_TABLE_END()
}
/******************************************************************************
 *
 ******************************************************************************/
};  // namespace NSPipelinePlugin
};  // namespace NSCam