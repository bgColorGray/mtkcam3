/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2019. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#ifndef _MTK_HARDWARE_MTKCAM3_INCLUDE_MTKCAM3_PLUGIN_STREAMINFO_TYPES_H_
#define _MTK_HARDWARE_MTKCAM3_INCLUDE_MTKCAM3_PLUGIN_STREAMINFO_TYPES_H_

#include <ios>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include <mtkcam/def/common.h>
#include <mtkcam/def/ImageBufferInfo.h>

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam::plugin::streaminfo {


/**
 * Plugin Id
 */
enum class PluginId : uint32_t
{
    BAD = 0,    // Bad id
    P1STT,

};


static inline std::string toString(PluginId const o)
{
    switch (o)
    {
    case PluginId::BAD:             return "BAD";
    case PluginId::P1STT:           return "P1STT";
    default: break;
    }
    return "TBD?" + std::to_string(static_cast<uint32_t>(o));
}


/**
 * Private Data Id
 */
enum class PrivateDataId : uint32_t
{
    BAD = 0,    // Bad id
    COMMON,     // common id for get ptr
    P1STT,
};


static inline std::string toString(PrivateDataId const o)
{
    switch (o)
    {
    case PrivateDataId::BAD:        return "BAD";
    case PrivateDataId::P1STT:      return "P1STT";
    default: break;
    }
    return "TBD?" + std::to_string(static_cast<uint32_t>(o));
}

struct Private_Handle
{
    uint32_t privId;
    uint32_t privSize;
};

/**
 * This structure is corresponding to PrivateDataId::P1STT.
 *
 * @param useLcso: use Lcso if true.
 * @param useLcsho: use Lcesho if true.
 * @param lcsoInfo: store LCSO buffer layout type for user calling heap->createImageBuffers_FromBlobHeap() API
 * @param lcshoInfo: store LCSHO buffer layout type for user calling heap->createImageBuffers_FromBlobHeap() API
 * @param totalSize: heap total size in byte
 */
struct P1STT
{
    uint32_t privId;
    uint32_t privSize;
    bool    useLcso;
    bool    useLcsho;
    ImageBufferInfo mLcsoInfo;
    ImageBufferInfo mLcshoInfo;
    size_t totalSize;
    uint32_t maxBatch;
    int32_t logLevel;
};

struct P1STT_QueryParam
{
    uint32_t p1Batch = 1;
};

static inline std::string toString(P1STT const& o)
{
    std::ostringstream oss;
    oss << "{";
    oss << " .useLcso=" << o.useLcso;
    oss << " .useLcsho=" << o.useLcsho;
    oss << " .LcsoOffset Cnt =" << (o.useLcso ? o.mLcsoInfo.count : 0);
    oss << " .LcshoOffset Cnt =" << (o.useLcsho ? o.mLcshoInfo.count : 0);
    oss << " }";
    return oss.str();
}

/**
 * The tuple element definition of Private data types.
 */
template <auto _Id, class _Tp>
struct __tuple_element
{
    static const auto id = _Id;
    using type = _Tp;
};

/**
 * The tuple definition of Private data types.
 */
using PrivateDataTuple = std::common_type<
    __tuple_element< PrivateDataId::P1STT, P1STT >,
    __tuple_element< PrivateDataId::COMMON, uint8_t >
>;

template<class Type, class TupleType> struct __find_element_by_type;
template<class Type, class Head, class... Tail>
struct __find_element_by_type<Type, std::common_type<Head, Tail...>>
    :  __find_element_by_type<Type, std::common_type<Tail...>> {};
template<class Type, auto Id, class... Tail>
struct __find_element_by_type<Type, std::common_type<__tuple_element<Id, Type>, Tail...>>
{
    using value = __tuple_element<Id, Type>;
};

template<auto Id, class TupleType> struct __find_element_by_id;
template<auto Id, class Head, class... Tail>
struct __find_element_by_id<Id, std::common_type<Head, Tail...>>
    :  __find_element_by_id<Id, std::common_type<Tail...>> {};
template<class Type, auto Id, class... Tail>
struct __find_element_by_id<Id, std::common_type<__tuple_element<Id, Type>, Tail...>>
{
    using value = __tuple_element<Id, Type>;
};

////////////////////////////////////////////////////////////////////////////////
//  The definition of Private data type and its access.
////////////////////////////////////////////////////////////////////////////////



/**
 * The definition of Private data type.
 */
using PrivateDataT = std::shared_ptr<std::vector<uint8_t>>;

/**
 * get private data size
 */
inline
size_t get_datasize(const PrivateDataT& operand) noexcept
{
    return (operand != nullptr) ? (operand->size()) : 0;
}

/**
 * get private data id
 */
inline
uint32_t get_dataid(const void *pdata) noexcept
{
    return (pdata != nullptr) ? ((reinterpret_cast<const Private_Handle *>(pdata))->privId) : (uint32_t)PrivateDataId::BAD;
}

/**
 * Obtains a pointer to the value of a given type, returns null on error.
 */
template<class T>
inline
T* get_variable_data_if(const PrivateDataT& operand) noexcept
{
    using Element = typename __find_element_by_type<T, PrivateDataTuple>::value;
    using Type = typename Element::type;
    static_assert(std::is_standard_layout_v<Type>);
    static_assert(std::is_trivial_v<Type>);
    static_assert(std::is_same_v<T, Type>);

    if ( operand == nullptr ) {
        return nullptr;
    }

    if ( Element::id != static_cast<PrivateDataId>(get_dataid(operand->data()))
      && Element::id != PrivateDataId::COMMON ) {
        return nullptr;
    }

    return reinterpret_cast<Type*>(operand->data());
}

/**
 * Obtains a pointer to the value of a given type, returns null on error.
 */
template<class T>
inline
const T* get_data_if(const PrivateDataT& operand) noexcept
{
    return get_variable_data_if<T>(operand);
}

/**
 * Checks if it holds a data.
 */
inline
bool has_data(const PrivateDataT& operand) noexcept
{
    return (operand != nullptr)
        && (PrivateDataId::BAD != static_cast<PrivateDataId>(get_dataid(operand->data())))
        && !operand->empty()
            ;
}

/**
 * copy private data
 */
inline
PrivateDataT copy_privdata(const PrivateDataT& operand) noexcept
{
    PrivateDataT p = std::make_shared<std::vector<uint8_t>>();
    if ( !has_data(operand) )
    {
        return nullptr;
    }
    uint32_t size = (reinterpret_cast<const Private_Handle *>(operand->data()))->privSize;
    p->resize(size);
    memcpy(p->data(), operand->data(), size);
    return p;
}

/**
 * get private data
 */
inline
PrivateDataT create_privdata(const void *pdata) noexcept
{
    PrivateDataT p = std::make_shared<std::vector<uint8_t>>();
    if (pdata == nullptr)
    {
        return nullptr;
    }
    uint32_t size = (reinterpret_cast<const Private_Handle *>(pdata))->privSize;
    p->resize(size);
    memcpy(p->data(), pdata, size);
    return p;
}


/******************************************************************************
 *
 ******************************************************************************/
};  //namespace NSCam::plugin::streaminfo
#endif//_MTK_HARDWARE_MTKCAM3_INCLUDE_MTKCAM3_PLUGIN_STREAMINFO_TYPES_H_
