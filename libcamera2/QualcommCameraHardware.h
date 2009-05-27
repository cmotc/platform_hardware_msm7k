/*
** Copyright 2008, Google Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

#ifndef ANDROID_HARDWARE_QUALCOMM_CAMERA_HARDWARE_H
#define ANDROID_HARDWARE_QUALCOMM_CAMERA_HARDWARE_H

#include <ui/CameraHardwareInterface.h>
#include <utils/MemoryBase.h>
#include <utils/MemoryHeapBase.h>
#include <stdint.h>

extern "C" {
#include <linux/android_pmem.h>
#include <media/msm_camera.h>
#include <camera.h>
}

namespace android {

class QualcommCameraHardware : public CameraHardwareInterface {
public:

    virtual sp<IMemoryHeap> getPreviewHeap() const;
    virtual sp<IMemoryHeap> getRawHeap() const;

    virtual status_t    dump(int fd, const Vector<String16>& args) const;
    virtual status_t    startPreview(preview_callback cb, void* user);
    virtual void        stopPreview();
    virtual status_t    startRecording(recording_callback cb, void* user);
    virtual void        stopRecording();
    virtual bool        recordingEnabled();
    virtual void        releaseRecordingFrame(const sp<IMemory>& mem);
    virtual status_t    autoFocus(autofocus_callback, void *user);
    virtual status_t    takePicture(shutter_callback,
                                    raw_callback,
                                    jpeg_callback,
                                    void* user);
    virtual status_t    cancelPicture(bool cancel_shutter,
                                      bool cancel_raw, bool cancel_jpeg);
    virtual status_t    setParameters(const CameraParameters& params);
    virtual CameraParameters  getParameters() const;

    virtual void release();

    static sp<CameraHardwareInterface> createInstance();
    static sp<QualcommCameraHardware> getInstance();

    

    bool native_set_dimension (int camfd);
	void reg_unreg_buf(int camfd, int width, int height,
                       int pmempreviewfd, uint8_t *prev_buf,
                       int pmem_type,
                       bool unregister,
                       bool active);
    bool native_register_preview_bufs(int camfd,
                                      struct msm_frame_t *frame,bool active);
	bool native_unregister_preview_bufs(int camfd, int pmempreviewfd,
                                        uint8_t *prev_buf);
    
	bool native_start_preview(int camfd);
	bool native_stop_preview(int camfd);

	bool native_register_snapshot_bufs(int camfd, int pmemthumbnailfd,
                                       int pmemsnapshotfd,
                                       uint8_t *thumbnail_buf,
                                       uint8_t *main_img_buf);
    bool native_unregister_snapshot_bufs(int camfd, int pmemThumbnailfd,
                                         int pmemSnapshotfd,
                                         uint8_t *thumbnail_buf,
                                         uint8_t *main_img_buf);
    bool native_get_picture(int camfd);
	bool native_start_snapshot(int camfd);
    bool native_stop_snapshot(int camfd);
    bool native_jpeg_encode (int pmemThumbnailfd, int pmemSnapshotfd,
                             uint8_t *thumbnail_buf, uint8_t *main_img_buf);

	bool native_set_zoom(int camfd, void *pZm);
	bool native_get_zoom(int camfd, void *pZm);

    void receivePreviewFrame(struct msm_frame_t *frame);
    void receiveJpegPicture(void);
    void receiveJpegPictureFragment(
        uint8_t * buff_ptr , uint32_t buff_size);
	bool        previewEnabled(); 
	

private:

    QualcommCameraHardware();
    virtual ~QualcommCameraHardware();
    status_t startPreviewInternal();
    void stopPreviewInternal();
    friend void *auto_focus_thread(void *user);
    void runAutoFocus();
    void cancelAutoFocus();

    static wp<QualcommCameraHardware> singleton;

    /* These constants reflect the number of buffers that libmmcamera requires
       for preview and raw, and need to be updated when libmmcamera
       changes.
    */
    static const int kPreviewBufferCount = 4;
    static const int kRawBufferCount = 1;
    static const int kJpegBufferCount = 1;
    static const int kRawFrameHeaderSize = 0;

    //TODO: put the picture dimensions in the CameraParameters object;
    CameraParameters mParameters;
    int mPreviewHeight;
    int mPreviewWidth;
    int mRawHeight;
    int mRawWidth;
	unsigned int frame_size;
	int mBrightness;
	float mZoomValuePrev;
    float mZoomValueCurr;
	bool mZoomInitialised;
	bool mCameraRunning;
    bool mPreviewInitialized;
    
    // This class represents a heap which maintains several contiguous
    // buffers.  The heap may be backed by pmem (when pmem_pool contains
    // the name of a /dev/pmem* file), or by ashmem (when pmem_pool == NULL).

    struct MemPool : public RefBase {
        MemPool(int buffer_size, int num_buffers,
                int frame_size,
                int frame_offset,
                const char *name);

        virtual ~MemPool() = 0;

        void completeInitialization();
        bool initialized() const { 
            return mHeap != NULL && mHeap->base() != MAP_FAILED;
        }

        virtual status_t dump(int fd, const Vector<String16>& args) const;

        int mBufferSize;
        int mNumBuffers;
        int mFrameSize;
        int mFrameOffset;
        sp<MemoryHeapBase> mHeap;
        sp<MemoryBase> *mBuffers;

        const char *mName;
    };

    struct AshmemPool : public MemPool {
        AshmemPool(int buffer_size, int num_buffers,
                   int frame_size,
                   int frame_offset,
                   const char *name);
    };

    struct PmemPool : public MemPool {
        PmemPool(const char *pmem_pool,
                int buffer_size, int num_buffers,
                int frame_size,
                int frame_offset,
                const char *name);
        virtual ~PmemPool(){ }
        int mFd;
        uint32_t mAlignedSize;
        struct pmem_region mSize;
    };

    struct PreviewPmemPool : public PmemPool {
        virtual ~PreviewPmemPool();
        PreviewPmemPool(int buffer_size, int num_buffers,
                        int frame_size,
                        int frame_offset,
                        const char *name);
    };

    struct RawPmemPool : public PmemPool {
        virtual ~RawPmemPool();
        RawPmemPool(const char *pmem_pool,
                    int buffer_size, int num_buffers,
                    int frame_size,
                    int frame_offset,
                    const char *name);
    };

    sp<PreviewPmemPool> mPreviewHeap;
    sp<RawPmemPool> mRawHeap;
    sp<AshmemPool> mJpegHeap;
	
    void startCamera();
    bool initPreview();
    void deinitPreview();
    bool initRaw(bool initJpegHeap);
    void deinitRaw();

    bool mFrameThreadRunning;
    Mutex mFrameThreadWaitLock;
    Condition mFrameThreadWait;
    friend void *frame_thread(void *user);
    void runFrameThread(void *data);

    void initDefaultParameters();

    void setSensorPreviewEffect(int, const char*);
    void setSensorWBLighting(int, const char*);
    void setAntiBanding(int, const char*);
    void setBrightness(int);
    void performZoom(bool);

    Mutex mLock;
    bool mReleasedRecordingFrame;

    void notifyShutter();
 
    void receiveRawPicture(void);

    Mutex mCallbackLock;
	Mutex mRecordLock;
	Mutex mRecordFrameLock;
	Condition mRecordWait;
    Condition mStateWait;

    /* mJpegSize keeps track of the size of the accumulated JPEG.  We clear it
       when we are about to take a picture, so at any time it contains either
       zero, or the size of the last JPEG picture taken.
    */
    uint32_t mJpegSize;
   

    shutter_callback    mShutterCallback;
    raw_callback        mRawPictureCallback;
    jpeg_callback       mJpegPictureCallback;
    void                *mPictureCallbackCookie;

    autofocus_callback  mAutoFocusCallback;
    void                *mAutoFocusCallbackCookie;

    preview_callback    mPreviewCallback;
    void                *mPreviewCallbackCookie;
    recording_callback  mRecordingCallback;
    void                *mRecordingCallbackCookie;
    unsigned int        mPreviewFrameSize;
    int                 mRawSize;
    int                 mJpegMaxSize;

#if DLOPEN_LIBMMCAMERA
    void *libmmcamera;
#endif

    int mCameraControlFd;
    cam_parm_info_t mZoom;
    cam_ctrl_dimension_t mDimension;
    int mPmemThumbnailFd;
    int mPmemSnapshotFd;
    ssize_t mPreviewFrameOffset;
    uint8_t *mThumbnailBuf;
    uint8_t *mMainImageBuf;
    bool mAutoFocusThreadRunning;
    Mutex mAutoFocusThreadLock;
    int mAutoFocusFd;

    pthread_t mCamConfigThread;
    pthread_t mFrameThread;

    struct msm_frame_t frames[kPreviewBufferCount];
    bool mInPreviewCallback;
};

}; // namespace android

#endif
