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
 * MediaTek Inc. (C) 2018. All rights reserved.
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

#ifndef _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_3RDPARTY_PLUGIN_PIPELINEPLUGIN_H_
#define _MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_3RDPARTY_PLUGIN_PIPELINEPLUGIN_H_

#include <mtkcam/def/common.h>
#include <mtkcam/utils/imgbuf/IImageBuffer.h>
#include <mtkcam/utils/metadata/IMetadata.h>
#include <mtkcam3/3rdparty/plugin/Reflection.h>
#include <mtkcam3/3rdparty/plugin/PluginInterceptor.h>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <deque>
#include <mutex>

/******************************************************************************
 *
 ******************************************************************************/
namespace NSCam {
namespace NSPipelinePlugin {

typedef MUINT64 PLUGIN_ID_TYPE;

typedef enum buffer_capability
{
    eBufCap_MSS  = 1 << 0,
    eBufCap_IMGO = 2 << 0

} eBufferCapability;

/******************************************************************************
 *
 ******************************************************************************/
template<class T>
class PipelinePlugin
{
public:
    typedef typename std::shared_ptr<PipelinePlugin<T>> Ptr;
    typedef typename std::weak_ptr<PipelinePlugin<T>> WeakPtr;

    typedef typename T::Property Property;
    struct Selection : T::Selection {
        typedef typename std::shared_ptr<Selection> Ptr;

        struct Token;
        typedef typename std::shared_ptr<Token> TokenPtr;
        TokenPtr mTokenPtr;

        static TokenPtr createToken(MINT32 iUniqueKey, MINT32 iReqNum, MINT32 iIndexNum, MINT32 sensorId);
        static TokenPtr createToken(MINT32 iUniqueKey, MINT32 iReqNum, MINT32 iIndexNum, MINT32 sensorId, MBOOL isPhysical);
    };

    struct Request : T::Request {
        typedef typename std::shared_ptr<Request> Ptr;
    };

    typedef typename T::ConfigParam ConfigParam;
    typedef typename T::RecommendedParam RecommendedParam;

    /*
     * Every plugin must define a interface to offer the plug point's capabilities.
     * To expose the support buffer formats and sizes, for provider to select
     * what it want to meet its requirement.
     *
     */
    class IInterface {
    public:
        typedef typename std::shared_ptr<IInterface> Ptr;

        /*
         * Provide the list of buffer size and format, which plug point could do
         *
         * @param[out] sel: the offering selection
         * @return 0 indicates success; otherwise failure.
         */
        virtual MERROR              offer(Selection& sel) = 0;

        virtual                     ~IInterface() { };
    };

    /*
     * The request using asynchronous call must be with a callback pointer.
     * The provider must send a callback if the call to process() is successful.
     *
     */
    class RequestCallback {
    public:
        typedef typename std::shared_ptr<RequestCallback> Ptr;

        /*
         * Cancel a request which have sent to plugin successfully
         *
         * @param[in] req: the request pointer
         */
        virtual void                onAborted(typename Request::Ptr) = 0;

        /*
         * Notify a completed result and request result
         *
         * @param[in] req: the request pointer
         * @param[in] err: result status
         */
        virtual void                onCompleted(typename Request::Ptr, MERROR) = 0;

        /*
         * Notify the next capture
         *
         * @param[in] req: the request pointer
         */
        virtual void                onNextCapture(typename Request::Ptr) = 0;

        /*
         * Configure the frame to be the raw-to-yuv frame
         *
         * @param[in] req: the request to be main frame
         * @return false: failed to configure main frame
         *         true: success to configure main frame
         */
        virtual bool                onConfigMainFrame(typename Request::Ptr)
        {
            return false;
        }


        virtual                     ~RequestCallback() { };
    };

   /*
    * A plugin could have multiple implementations for different features.
    * The provider will follow the specification to exchange data or processing buffer.
    *
    */
    class IProvider {
    public:
        typedef typename std::shared_ptr<IProvider> Ptr;

        /*
         * get the property to expose the plugin's characteristic
         *
         * @return the required information to plug point
         */
        virtual const Property&     property() = 0;

        /*
         * Negotiate buffer format and size between plug point and provider.
         * The provider should update the accept format and size.
         *
         * The provider should return -EINVAL if the offered selection do NOT meet
         * the requirement.
         *
         * @param[in] sel: the offering selection
         * @return 0 indicates success; otherwise failure.
         *
         */
        virtual MERROR              negotiate(Selection& sel) = 0;

        /* negotiateUpdate is used for the feature which wants to update the
         * negotiation result at feature setting policy.
         *
         * @param[in] sel: the offering selection
         * @return 0 indicates success; otherwise failure.
         */
        virtual MERROR              negotiateUpdate(Selection&) {
            return android::OK;
        }

        /*
         * Initialized procedure of plugin.  That may have multiple user to share a plugin
         * instance. Only the first call to init() will be invoked.
         *
         */
        virtual void                init() = 0;

        virtual void                config(const ConfigParam&) {}

        /*
         * query recommended frame setting from feature plugin
         * The provider should return -EINVAL if the input hint do NOT meet
         * the requirement or hint.
         *
         * @param[in]  : the offering App Metadata as input info and hint
         * @param[out] : the offering vector of App Metadata as per-frame's setting. (size > 2 if it is multiframe requirenent)
         * @return 0 indicates success; otherwise failure.
         */
        virtual MERROR              queryRecommendedFrameSetting(RecommendedParam&) {
            // default is not implement yet.
            return android::BAD_VALUE;
        }

        /*
         * Send a request to plugin provider.
         * Synchronous call to process() If callback pointer is null
         * Asynchronous call to process() If callback poiner is not null.
         *
         * @param[in] req: the request pointer
         * @param[in] cb: a callback for a asynchronous request
         * @return 0 indicates success; otherwise failure.
         */
        virtual MERROR              process(typename Request::Ptr,
                                            typename RequestCallback::Ptr = nullptr) = 0;

        /*
         * abort the specific requests and will block until the requests have been aborted.
         *
         * @param[in] req: list of aborting requests
         */
        virtual void                abort(std::vector<typename Request::Ptr>&) = 0;

        /*
         * Uninitialized procedure of plugin.  That may have multiple user to share a plugin
         * instance. Only the last call to uninit() will be invoked, which represent will users
         * had leaved.
         *
         */
        virtual void                uninit() = 0;
        virtual                     ~IProvider() { };
    };

    static Ptr                      getInstance(
                                        MINT32 iUniqueKey
                                    );
    /*
     * Create a shared pointer to empty request.
     */
    typename Request::Ptr           createRequest();

    /*
     * Get the instance of IInterface
     */
    typename IInterface::Ptr        getInterface();

    /*
     * Get the instances of IProvider.
     */
    const std::vector<typename IProvider::Ptr>&
                                    getProviders();

    /*
     * Get the instances of IProvider without create all plugin.
     * @param[in] mask: feature mask
     */
    const std::vector<typename IProvider::Ptr>&
                                    getProviders(MUINT64 mask);

    /*
     * Get the result of negotiation
     * @param[in] prov: the plugin provider
     * @return selection
     *
     */
    const Selection&                getSelection(
                                        typename IProvider::Ptr prov
                                    );
    /*
     * Create a shared pointer to empty selection.
     */
    typename Selection::Ptr         createSelection();

    /*
     * Push a selection into the provider's container
     * @param[in] prov: the plugin provider
     * @param[in] sel: the negotiated selection
     * @return selection
     *
     */
    MVOID                           pushSelection(
                                        typename IProvider::Ptr prov,
                                        typename Selection::Ptr sel
                                    );
    /*
     * Pop a selection from the provider's container
     * @param[in] prov: the plugin provider
     * @param[in] token: a token to get the specified selection
     * @return selection
     *
     */
    typename Selection::Ptr         popSelection(
                                        typename IProvider::Ptr prov,
                                        typename Selection::TokenPtr token = nullptr
                                    );
    /*
     * Return the first selection from the provider's container
     * @param[in] prov: the plugin provider
     * @param[in] token: a token to get the specified selection
     * @return selection
     *
     */
    typename Selection::Ptr         frontSelection(
                                        typename IProvider::Ptr prov,
                                        typename Selection::TokenPtr token = nullptr
                                    );

    /*
     * Return the number of stored selection in the provider's container
     * @param[in] prov: the plugin provider
     * @return number of selection
     *
     */
    size_t                          numOfSelection(
                                        typename IProvider::Ptr prov
                                    );
    /*
     * Dump the all plugin's peropties and selections
     *
     * @param[in] os outputstream
     */
    MVOID                           dump(
                                        std::ostream& os
                                    );

                                    PipelinePlugin(MINT32 hash);

    virtual                         ~PipelinePlugin();

private:
    typename IInterface::Ptr        mpInterface;
    std::mutex                      mMutex;
    std::vector<typename IProvider::Ptr>
                                    mpProviders;
    std::map<typename IProvider::Ptr, std::deque<typename Selection::Ptr>>
                                    mpSelections;
    MINT32                            mInstanceHash;

    static std::mutex                 mInstancesMutex;
    static std::map<MINT32, WeakPtr>  mInstances;
};


/******************************************************************************
 * Common
 ******************************************************************************/
typedef std::shared_ptr<NSCam::IMetadata> MetadataPtr;


/******************************************************************************
 * Buffer & Metadata Handle
 ******************************************************************************/
class MetadataHandle
{
public:
    typedef typename std::shared_ptr<MetadataHandle> Ptr;

    /*
     * Acquire the pointer of metadata
     *
     * @return the pointer of metadata
     */
    virtual NSCam::IMetadata*       acquire() = 0;

    /*
     * Release the metadata to the caller
     */
    virtual MVOID                   release() = 0;

    /*
     * Dump the handle info
     *
     * @param[in] os outputstream
     */
    virtual MVOID                   dump(
                                        std::ostream& os
                                    ) const = 0;

    virtual                         ~MetadataHandle() { };
};

class BufferHandle
{
public:
    typedef typename std::shared_ptr<BufferHandle> Ptr;

    /*
     * Acquire the pointer of locked image buffer
     *
     * @param[in] usage: the buffer usage
     * @return the pointer of image buffer
     */
    virtual NSCam::IImageBuffer*    acquire(
                                        MINT usage = eBUFFER_USAGE_HW_CAMERA_READWRITE |
                                                     eBUFFER_USAGE_SW_READ_OFTEN
                                    ) = 0;

    /*
     * Acquire the pointer of locked image buffer
     *
     * @param[in] usage: the buffer usage
     * @return the pointer of image buffer
     */
    virtual std::vector<NSCam::IImageBuffer*>
                                    acquireAll(
                                        MINT usage = eBUFFER_USAGE_HW_CAMERA_READWRITE |
                                                     eBUFFER_USAGE_SW_READ_OFTEN
                                    ) {
        return {};
    }

    /*
     * Release the image buffer to the caller
     */
    virtual MVOID                   release() = 0;

    /*
     * Dump the handle info
     *
     * @param[in] os outputstream
     */
    virtual MVOID                   dump(
                                        std::ostream& os
                                    ) const = 0;

    virtual                         ~BufferHandle() { };
};

std::ostream& operator<<(std::ostream&, const BufferHandle::Ptr);
std::ostream& operator<<(std::ostream&, const MetadataHandle::Ptr);

/******************************************************************************
 * Buffer & Metadata Selection
 ******************************************************************************/
class MetadataSelection
{
public:
                                    MetadataSelection();

                                    MetadataSelection(
                                        const MetadataSelection&
                                    );

    MetadataSelection&              operator = (
                                        const MetadataSelection&
                                    );


    virtual                         ~MetadataSelection();

    /*
     * [Provider] set the metadata whether required or not
     * It will get a null metadata in enque phrase if not required
     *
     * @param[in] required
     */
    MetadataSelection&              setRequired(
                                        MBOOL required
                                    );

    MBOOL                           getRequired() const;

    /*
     * [User] add the control metadata, which is the original frame metadata.
     * Only set metadata in input port
     *
     * @param[in] control: the shared pointer of metadata
     */
    MetadataSelection&              setControl(
                                        MetadataPtr control
                                    );

    /*
     * [Provider] add the addtional metadata, which will be applied into pipeline frame.
     * Only set metadata in input port
     * ex: HDR has to control the exposure time in every frame.
     *
     * @param[in] addtional: the shared pointer of metadata
     */
    MetadataSelection&              setAddtional(
                                        MetadataPtr additional
                                    );

    /*
     * [Provider] add the dummy metadata. It's used in creating extra pipeline frames
     * before capture starting.
     * ex: HDR could use rear dummy frame to stablize AE.
     *
     * @param[in] addtional: the shared pointer of metadata
     */
    MetadataSelection&              setDummy(
                                        MetadataPtr additional
                                    );
    /*
     * Get the control metadata
     *
     * @return the shared pointer of metadata
     */
    MetadataPtr                     getControl() const;

    /*
     * Get the addtional metadata
     *
     * @return the shared pointer of metadata
     */
    MetadataPtr                     getAddtional() const;

    /*
     * Get the dummy metadata.
     *
     * @return the shared pointer of metadata
     */
    MetadataPtr                     getDummy() const;

    /*
     * Dump the selection info
     *
     * @param[in] os outputstream
     */
    MVOID dump(std::ostream& os) const;

private:
    class               Implementor;
    Implementor*        mImpl;
};

class BufferSelection
{
public:

                                    BufferSelection();

                                    BufferSelection(
                                        const BufferSelection&
                                    );

    BufferSelection&                operator = (
                                        const BufferSelection&
                                    );

    virtual                         ~BufferSelection();

    /*
     * [Provider] set the buffer whether required or not
     * The request will get a null buffer in enque phrase if not required
     *
     * @param[in] required
     */
    BufferSelection&                setRequired(
                                        MBOOL required
                                    );

    MBOOL                           getRequired() const;

    /*
     * [Provider] set the output buffer to optional
     * The request will probably get a buffer on data flow's demand if set optional
     *
     * @param[in] optional
     */
    BufferSelection&                setOptional(
                                        MBOOL optional
                                    );

    MBOOL                           getOptional() const;

    /*
     * [Provider] set the specified size if have added the size enum[SPECIFIED]
     *
     * @param[in] size: the width and height
     */
    BufferSelection&                setSpecifiedSize(
                                        const MSize& size
                                    );

    /*
     * [Provider] set the buffer alignment for all size enum
     *
     * @param[in] width: the width alignment
     * @param[in] height: the height alignment
     */
    BufferSelection&                setAlignment(
                                        MUINT32 width,
                                        MUINT32 height
                                    );

    /*
     * [Provider] add an acceptable image format for this buffer port
     *
     * @param[in] fmt EImageFormat
     */
    BufferSelection&                addAcceptedFormat(
                                        MINT fmt
                                    );
    /*
     * [Provider] add an acceptable image size for this buffer port
     *
     * @param[in] size EImageSize
     */
    BufferSelection&                addAcceptedSize(
                                        MINT size
                                    );

    /*
     * [Interface] add the supported image format for this buffer port
     *
     * @param[in] fmt EImageFormat
     */
    BufferSelection&                addSupportFormat(
                                        MINT fmt
                                    );

    /*
     * [Interface] add the supported image size for this buffer port
     *
     * @param[in] size EImageSize
     */
    BufferSelection&                addSupportSize(
                                        MINT size
                                    );


    /*
     * Check the negotiated result whether valid or not.
     * That must have the intersectional of formats and sizes
     *
     * @return valid if have successful negotiation
     */
    MBOOL                           isValid() const;

    /*
     * Get the intersection formats between interface side and provider side
     *
     * @return the vector of formats
     */
    const std::vector<MINT>&        getFormats() const;

    /*
     * Get the intersection sizes between interface side and provider side
     *
     * @return the vector of sizes
     */
    const std::vector<MINT>&        getSizes() const;

    /*
     * Get the specified size.
     *
     * @return the image's width and height
     */
    const MSize&                    getSpecifiedSize() const;

    /*
     * Get the alignment
     *
     * @return the alignment of width and height
     */
    MVOID                           getAlignment(MUINT32&,  MUINT32&) const;
    MVOID                           getAlignment(MSize&) const;

    /*
     * set the special platform-related preferable ability
     *
     */
    MVOID                           setCapability(eBufferCapability cap);

    MBOOL                     hasCapability(eBufferCapability cap) const;

    /*
     * Dump the selection info
     *
     * @param[in] os outputstream
     */
    MVOID dump(std::ostream& os) const;

private:
    class               Implementor;
    Implementor*        mImpl;
};

enum EImageSize
{
    eImgSize_Full,
    eImgSize_Resized,
    eImgSize_Quarter,
    eImgSize_Specified,
    eImgSize_Arbitrary,
};

std::ostream& operator<<(std::ostream&, const BufferSelection&);
std::ostream& operator<<(std::ostream&, const MetadataSelection&);

/******************************************************************************
 * Plugin Registry
 ******************************************************************************/
template<class T>
class PluginRegistry {

public:
    typedef typename PipelinePlugin<T>::IProvider::Ptr (*ConstructProvider)();
    typedef typename PipelinePlugin<T>::IInterface::Ptr (*ConstructInterface)();

    struct ConstructProviderEntry
    {
        ConstructProviderEntry(ConstructProvider c)
            : mConstruct(c), mID(0)
        {}
        ConstructProviderEntry(ConstructProvider c, PLUGIN_ID_TYPE id)
            : mConstruct(c), mID(id)
        {}

        ConstructProvider mConstruct;
        PLUGIN_ID_TYPE mID;
    };

    typedef std::vector<ConstructProviderEntry> ProviderRegistry;
    typedef std::vector<ConstructInterface> InterfaceRegistry;

    static ProviderRegistry& ofProvider() {
        return sProviderRegistry;
    }

    static InterfaceRegistry& ofInterface() {
        return sInterfaceRegistry;
    }

    static void addProvider(ConstructProvider fnConstructor);
    static void addProvider(ConstructProvider fnConstructor, PLUGIN_ID_TYPE id);

    static void addInterface(ConstructInterface fnConstructor);

private:
    static ProviderRegistry sProviderRegistry;
    static InterfaceRegistry sInterfaceRegistry;
};

template<class T>
class PluginRegister {
public:
    typedef typename PluginRegistry<T>::ConstructProvider ConstructProvider;
    typedef typename PluginRegistry<T>::ConstructInterface ConstructInterface;

    PluginRegister(ConstructProvider fnConstructor) {
        PluginRegistry<T>::addProvider(fnConstructor);
    }

    PluginRegister(ConstructProvider fnConstructor, PLUGIN_ID_TYPE id) {
        PluginRegistry<T>::addProvider(fnConstructor, id);
    }

    PluginRegister(ConstructInterface fnConstructor) {
        PluginRegistry<T>::addInterface(fnConstructor);
    }
};

#define REGISTER_PLUGIN_PROVIDER(T, type) \
  PipelinePlugin<T>::IProvider::Ptr _CreatePluginProvider_##type() \
  { \
      return std::make_shared<Interceptor<T,type>>(STRINGIZE(type)); \
  }; \
  static PluginRegister<T> gPipelinePluginProvider_##type(_CreatePluginProvider_##type);

#define REGISTER_PLUGIN_PROVIDER_DYNAMIC(T, type, id) \
  PipelinePlugin<T>::IProvider::Ptr _CreatePluginProvider_##type() \
  { \
      return std::make_shared<Interceptor<T,type>>(STRINGIZE(type)); \
  }; \
  static PluginRegister<T> gPipelinePluginProvider_##type(_CreatePluginProvider_##type, id);

#define REGISTER_PLUGIN_INTERFACE(T, type) \
  PipelinePlugin<T>::IInterface::Ptr _CreatePluginInterface_##type() \
  { \
      return std::make_shared<type>(); \
  }; \
  static PluginRegister<T> gPipelinePluginInterface_##type(_CreatePluginInterface_##type);

/******************************************************************************
*
******************************************************************************/
};  //namespace NSPipelinePlugin
};  //namespace NSCam
#endif //_MTK_HARDWARE_MTKCAM_INCLUDE_MTKCAM_3RDPARTY_PLUGIN_PIPELINEPLUGIN_H_
